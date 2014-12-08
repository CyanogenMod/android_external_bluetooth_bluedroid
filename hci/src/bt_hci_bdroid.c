/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_hci_bdroid.c
 *
 *  Description:   Bluedroid Bluetooth Host/Controller interface library
 *                 implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci_bdroid"

#include <assert.h>
#include <utils/Log.h>

#include "btsnoop.h"
#include "bt_hci_bdroid.h"
#include "bt_utils.h"
#include "bt_vendor_lib.h"
#include "hci.h"
#include "osi.h"
#include "thread.h"
#include "userial.h"
#include "utils.h"
#include "vendor.h"

#ifndef BTHC_DBG
#define BTHC_DBG FALSE
#endif

#if (BTHC_DBG == TRUE)
#define BTHCDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTHCDBG(param, ...) {}
#endif

/* Vendor epilog process timeout period  */
static const uint32_t EPILOG_TIMEOUT_MS = 3000;

/******************************************************************************
**  Externs
******************************************************************************/

extern int num_hci_cmd_pkts;
void lpm_init(void);
void lpm_cleanup(void);
void lpm_enable(uint8_t turn_on);
void lpm_wake_deassert(void);
void lpm_allow_bt_device_sleep(void);
void lpm_wake_assert(void);
void init_vnd_if(unsigned char *local_bdaddr);

/******************************************************************************
**  Variables
******************************************************************************/

bt_hc_callbacks_t *bt_hc_cbacks = NULL;
tHCI_IF *p_hci_if;
volatile bool fwcfg_acked;
// Cleanup state indication.
volatile bool has_cleaned_up = false;

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Host/Controller lib thread control block */
typedef struct
{
    thread_t        *worker_thread;
    pthread_mutex_t worker_thread_lock;
    bool            epilog_timer_created;
    timer_t         epilog_timer_id;
} bt_hc_cb_t;

/******************************************************************************
**  Static Variables
******************************************************************************/

static bt_hc_cb_t hc_cb;
static bool tx_cmd_pkts_pending = false;
static BUFFER_Q tx_q;

/******************************************************************************
**  Functions
******************************************************************************/

static void event_preload(UNUSED_ATTR void *context) {
  userial_open(USERIAL_PORT_1);
  vendor_send_command(BT_VND_OP_FW_CFG, NULL);
}

static void event_postload(UNUSED_ATTR void *context) {
  /* Start from SCO related H/W configuration, if SCO configuration
   * is required. Then, follow with reading requests of getting
   * ACL data length for both BR/EDR and LE.
   */
  int result = vendor_send_command(BT_VND_OP_SCO_CFG, NULL);
  if (result == -1)
    p_hci_if->get_acl_max_len();
}

static void event_tx(UNUSED_ATTR void *context) {
  /*
   *  We will go through every packets in the tx queue.
   *  Fine to clear tx_cmd_pkts_pending.
   */
  tx_cmd_pkts_pending = false;
  HC_BT_HDR *sending_msg_que[64];
  size_t sending_msg_count = 0;
  int sending_hci_cmd_pkts_count = 0;
  utils_lock();
  HC_BT_HDR *p_next_msg = tx_q.p_first;
  while (p_next_msg && sending_msg_count < ARRAY_SIZE(sending_msg_que))
  {
    if ((p_next_msg->event & MSG_EVT_MASK)==MSG_STACK_TO_HC_HCI_CMD)
    {
      /*
       *  if we have used up controller's outstanding HCI command
       *  credits (normally is 1), skip all HCI command packets in
       *  the queue.
       *  The pending command packets will be sent once controller
       *  gives back us credits through CommandCompleteEvent or
       *  CommandStatusEvent.
       */
      if (tx_cmd_pkts_pending ||
          (sending_hci_cmd_pkts_count >= num_hci_cmd_pkts))
      {
        tx_cmd_pkts_pending = true;
        p_next_msg = utils_getnext(p_next_msg);
        continue;
      }
      sending_hci_cmd_pkts_count++;
    }

    HC_BT_HDR *p_msg = p_next_msg;
    p_next_msg = utils_getnext(p_msg);
    utils_remove_from_queue_unlocked(&tx_q, p_msg);
    sending_msg_que[sending_msg_count++] = p_msg;
  }
  utils_unlock();
  for(size_t i = 0; i < sending_msg_count; i++)
    p_hci_if->send(sending_msg_que[i]);
  if (tx_cmd_pkts_pending)
    BTHCDBG("Used up Tx Cmd credits");
}

static void event_rx(UNUSED_ATTR void *context) {
#ifndef HCI_USE_MCT
  p_hci_if->rcv();

  if (tx_cmd_pkts_pending && num_hci_cmd_pkts > 0) {
    // Got HCI Cmd credits from controller. Send whatever data
    // we have in our tx queue. We can call |event_tx| directly
    // here since we're already on the worker thread.
    event_tx(NULL);
  }
#endif
}

static void event_lpm_enable(UNUSED_ATTR void *context) {
  lpm_enable(true);
}

static void event_lpm_disable(UNUSED_ATTR void *context) {
  lpm_enable(false);
}

static void event_lpm_wake_device(UNUSED_ATTR void *context) {
  lpm_wake_assert();
}

static void event_lpm_allow_sleep(UNUSED_ATTR void *context) {
  lpm_allow_bt_device_sleep();
}

static void event_lpm_idle_timeout(UNUSED_ATTR void *context) {
  lpm_wake_deassert();
}

static void event_epilog(UNUSED_ATTR void *context) {
  vendor_send_command(BT_VND_OP_EPILOG, NULL);
}

static void event_tx_cmd(void *msg) {
  HC_BT_HDR *p_msg = (HC_BT_HDR *)msg;

  BTHCDBG("%s: p_msg: %p, event: 0x%x", __func__, p_msg, p_msg->event);

  int event = p_msg->event & MSG_EVT_MASK;
  int sub_event = p_msg->event & MSG_SUB_EVT_MASK;
  if (event == MSG_CTRL_TO_HC_CMD && sub_event == BT_HC_AUDIO_STATE) {
    vendor_send_command(BT_VND_OP_SET_AUDIO_STATE, p_msg->data);
  } else {
    ALOGW("%s (event: 0x%x, sub_event: 0x%x) not supported", __func__, event, sub_event);
  }

  bt_hc_cbacks->dealloc(msg);
}

void bthc_rx_ready(void) {
  pthread_mutex_lock(&hc_cb.worker_thread_lock);

  if (hc_cb.worker_thread)
    thread_post(hc_cb.worker_thread, event_rx, NULL);

  pthread_mutex_unlock(&hc_cb.worker_thread_lock);
}

void bthc_tx(HC_BT_HDR *buf) {
  pthread_mutex_lock(&hc_cb.worker_thread_lock);

  if (hc_cb.worker_thread) {
    if (buf)
      utils_enqueue(&tx_q, buf);
    thread_post(hc_cb.worker_thread, event_tx, NULL);
  }

  pthread_mutex_unlock(&hc_cb.worker_thread_lock);
}

void bthc_idle_timeout(void) {
  pthread_mutex_lock(&hc_cb.worker_thread_lock);

  if (hc_cb.worker_thread)
    thread_post(hc_cb.worker_thread, event_lpm_idle_timeout, NULL);

  pthread_mutex_unlock(&hc_cb.worker_thread_lock);
}

/*******************************************************************************
**
** Function        epilog_wait_timeout
**
** Description     Timeout thread of epilog watchdog timer
**
** Returns         None
**
*******************************************************************************/
static void epilog_wait_timeout(UNUSED_ATTR union sigval arg)
{
    ALOGI("...epilog_wait_timeout...");

    thread_free(hc_cb.worker_thread);

    pthread_mutex_lock(&hc_cb.worker_thread_lock);
    hc_cb.worker_thread = NULL;
    pthread_mutex_unlock(&hc_cb.worker_thread_lock);
}

/*******************************************************************************
**
** Function        epilog_wait_timer
**
** Description     Launch epilog watchdog timer
**
** Returns         None
**
*******************************************************************************/
static void epilog_wait_timer(void)
{
    int status;
    struct itimerspec ts;
    struct sigevent se;
    uint32_t timeout_ms = EPILOG_TIMEOUT_MS;

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &hc_cb.epilog_timer_id;
    se.sigev_notify_function = epilog_wait_timeout;
    se.sigev_notify_attributes = NULL;

    status = timer_create(CLOCK_MONOTONIC, &se, &hc_cb.epilog_timer_id);

    if (status == 0)
    {
        hc_cb.epilog_timer_created = true;
        ts.it_value.tv_sec = timeout_ms/1000;
        ts.it_value.tv_nsec = 1000000*(timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(hc_cb.epilog_timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("Failed to fire epilog watchdog timer");
    }
    else
    {
        ALOGE("Failed to create epilog watchdog timer");
        hc_cb.epilog_timer_created = false;
    }
}

/*****************************************************************************
**
**   BLUETOOTH HOST/CONTROLLER INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_hc_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    int result;

    ALOGI("init");

    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return BT_HC_STATUS_FAIL;
    }

    hc_cb.epilog_timer_created = false;
    fwcfg_acked = false;
    has_cleaned_up = false;

    pthread_mutex_init(&hc_cb.worker_thread_lock, NULL);

    /* store reference to user callbacks */
    bt_hc_cbacks = (bt_hc_callbacks_t *) p_cb;

    vendor_open(local_bdaddr);

    utils_init();
#ifdef HCI_USE_MCT
    extern tHCI_IF hci_mct_func_table;
    p_hci_if = &hci_mct_func_table;
#else
    extern tHCI_IF hci_h4_func_table;
    p_hci_if = &hci_h4_func_table;
#endif

    p_hci_if->init();

    userial_init();
    lpm_init();

    utils_queue_init(&tx_q);

    if (hc_cb.worker_thread)
    {
        ALOGW("init has been called repeatedly without calling cleanup ?");
    }

    // Set prio here and let hci worker thread inherit prio
    // remove once new thread api (thread_set_priority() ?)
    // can switch prio
    raise_priority_a2dp(TASK_HIGH_HCI_WORKER);

    hc_cb.worker_thread = thread_new("bt_hc_worker");
    if (!hc_cb.worker_thread) {
        ALOGE("%s unable to create worker thread.", __func__);
        return BT_HC_STATUS_FAIL;
    }

    return BT_HC_STATUS_SUCCESS;
}


/** Chip power control */
static void set_power(bt_hc_chip_power_state_t state)
{
    int pwr_state;

    BTHCDBG("set_power %d", state);

    /* Calling vendor-specific part */
    pwr_state = (state == BT_HC_CHIP_PWR_ON) ? BT_VND_PWR_ON : BT_VND_PWR_OFF;

    vendor_send_command(BT_VND_OP_POWER_CTRL, &pwr_state);
}


/** Configure low power mode wake state */
static int lpm(bt_hc_low_power_event_t event)
{
    switch (event)
    {
        case BT_HC_LPM_DISABLE:
            if (hc_cb.worker_thread)
                thread_post(hc_cb.worker_thread, event_lpm_disable, NULL);
            break;

        case BT_HC_LPM_ENABLE:
            if (hc_cb.worker_thread)
                thread_post(hc_cb.worker_thread, event_lpm_enable, NULL);
            break;

        case BT_HC_LPM_WAKE_ASSERT:
            if (hc_cb.worker_thread)
                thread_post(hc_cb.worker_thread, event_lpm_wake_device, NULL);
            break;

        case BT_HC_LPM_WAKE_DEASSERT:
            if (hc_cb.worker_thread)
                thread_post(hc_cb.worker_thread, event_lpm_allow_sleep, NULL);
            break;
    }
    return BT_HC_STATUS_SUCCESS;
}


/** Called prior to stack initialization */
static void preload(UNUSED_ATTR TRANSAC transac) {
  BTHCDBG("preload");
  if (hc_cb.worker_thread)
    thread_post(hc_cb.worker_thread, event_preload, NULL);
}

/** Called post stack initialization */
static void postload(UNUSED_ATTR TRANSAC transac) {
  BTHCDBG("postload");
  if (hc_cb.worker_thread)
    thread_post(hc_cb.worker_thread, event_postload, NULL);
}

/** Transmit frame */
static int transmit_buf(TRANSAC transac, UNUSED_ATTR char *p_buf, UNUSED_ATTR int len) {
  bthc_tx((HC_BT_HDR *)transac);
  return BT_HC_STATUS_SUCCESS;
}

/** Controls HCI logging on/off */
static int logging(bt_hc_logging_state_t state, char *p_path, bool save_existing) {
  BTHCDBG("logging %d", state);

  if (state != BT_HC_LOGGING_ON)
    btsnoop_close();
  else if (p_path != NULL)
    btsnoop_open(p_path, save_existing);

  return BT_HC_STATUS_SUCCESS;
}

/** sends command HC controller to configure platform specific behaviour */
static int tx_hc_cmd(TRANSAC transac, char *p_buf, int len) {
  BTHCDBG("tx_hc_cmd: transac %p", transac);

  if (!transac)
    return BT_HC_STATUS_FAIL;

  if (hc_cb.worker_thread)
    thread_post(hc_cb.worker_thread, event_tx_cmd, transac);
  return BT_HC_STATUS_SUCCESS;
}
static void ssr_cleanup (void) {
    BTHCDBG("ssr_cleanup");
    /* Calling vendor-specific part */
    vendor_ssrcleanup();
}


// Closes the interface.
// This routine is not thread safe.
static void cleanup(void)
{
    if (has_cleaned_up) {
        ALOGW("%s Already cleaned up for this session\n", __func__);
        return;
    }

    BTHCDBG("cleanup");

    if (hc_cb.worker_thread)
    {
        if (fwcfg_acked)
        {
            epilog_wait_timer();
            // Stop reading thread
            userial_close_reader();

            thread_post(hc_cb.worker_thread, event_epilog, NULL);
        }
        thread_free(hc_cb.worker_thread);

        pthread_mutex_lock(&hc_cb.worker_thread_lock);
        hc_cb.worker_thread = NULL;
        pthread_mutex_unlock(&hc_cb.worker_thread_lock);

        if (hc_cb.epilog_timer_created)
        {
            timer_delete(hc_cb.epilog_timer_id);
            hc_cb.epilog_timer_created = false;
        }
    }
    BTHCDBG("%s Finalizing cleanup\n", __func__);

    lpm_cleanup();
    userial_close();
    p_hci_if->cleanup();
    utils_cleanup();

    set_power(BT_VND_PWR_OFF);
    vendor_close();

    pthread_mutex_destroy(&hc_cb.worker_thread_lock);

    fwcfg_acked = false;
    bt_hc_cbacks = NULL;
    has_cleaned_up = true;
}

static const bt_hc_interface_t bluetoothHCLibInterface = {
    sizeof(bt_hc_interface_t),
    init,
    set_power,
    lpm,
    preload,
    postload,
    transmit_buf,
    logging,
    cleanup,
    tx_hc_cmd,
    ssr_cleanup
};

/*******************************************************************************
**
** Function        bt_hc_get_interface
**
** Description     Caller calls this function to get API instance
**
** Returns         API table
**
*******************************************************************************/
const bt_hc_interface_t *bt_hc_get_interface(void)
{
    return &bluetoothHCLibInterface;
}
