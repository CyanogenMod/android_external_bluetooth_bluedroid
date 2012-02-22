/************************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed 
 *  pursuant to the terms and conditions of a separate, written license 
 *  agreement executed between you and Broadcom (an "Authorized License").  
 *  Except as set forth in an Authorized License, Broadcom grants no license 
 *  (express or implied), right to use, or waiver of any kind with respect to 
 *  the Software, and Broadcom expressly reserves all rights in and to the 
 *  Software and all intellectual property rights therein.  
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS 
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
 *  ALL USE OF THE SOFTWARE.  
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization, 
 *         constitutes the valuable trade secrets of Broadcom, and you shall 
 *         use all reasonable efforts to protect the confidentiality thereof, 
 *         and to use this information only in connection with your use of 
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR 
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

/************************************************************************************
 *
 *  Filename:      bt_vendor_brcm.c
 *
 *  Description:   Bluetooth vendor library implementation
 * 
 ***********************************************************************************/

#define LOG_TAG "bt_vendor_brcm"

#include <utils/Log.h>
#include <pthread.h>
#include "bt_vendor_brcm.h"
#include "upio.h"
#include "utils.h"
#include "userial.h"

#ifndef BTVND_DBG
#define BTVND_DBG FALSE
#endif

#if (BTVND_DBG == TRUE)
#define BTVNDDBG LOGD
#else
#define BTVNDDBG
#endif

/************************************************************************************
**  Externs
************************************************************************************/

extern int num_hci_cmd_pkts;
void hci_h4_init(void);
void hci_h4_cleanup(void);
void hci_h4_send_msg(VND_BT_HDR *p_msg);
uint16_t hci_h4_receive_msg(void);
void hci_h4_get_acl_data_length(void);
void hw_config_start(void);
uint8_t hw_lpm_enable(uint8_t turn_on);
void hw_lpm_deassert_bt_wake(void);
void hw_lpm_allow_bt_device_sleep(void);
void hw_lpm_assert_bt_wake(void);
#if (SCO_CFG_INCLUDED == TRUE)
void hw_sco_config(void);
#endif

/************************************************************************************
**  Variables
************************************************************************************/

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;

/************************************************************************************
**  Static Variables
************************************************************************************/

static pthread_t worker_thread;
static volatile uint8_t lib_running = 0;
static pthread_mutex_t vnd_mutex;
static pthread_cond_t vnd_cond;
static volatile uint16_t ready_events = 0;
static volatile uint8_t tx_cmd_pkts_pending = FALSE;

static const tUSERIAL_CFG userial_init_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200
};

BUFFER_Q tx_q;

/************************************************************************************
**  Functions
************************************************************************************/

static void *bt_vndor_worker_thread(void *arg);   

void btvnd_signal_event(uint16_t event)
{
    pthread_mutex_lock(&vnd_mutex);
    ready_events |= event;
    pthread_cond_signal(&vnd_cond);
    pthread_mutex_unlock(&vnd_mutex);        
}

/*****************************************************************************
**
**   BLUETOOTH VENDOR LIBRARY INTERFACE FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb)
{
    pthread_attr_t thread_attr;
    struct sched_param param;
    int policy;

    BTVNDDBG("init");

    if (p_cb == NULL)
    {
        LOGE("init failed with no user callbacks!");
        return BT_VENDOR_STATUS_FAIL;
    }
    
    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    utils_init();
    upio_init();
    hci_h4_init();
    userial_init();

    utils_queue_init(&tx_q);
    
    if (lib_running)
    {    
        LOGW("init has been called repeatedly without calling cleanup ?");
    }
    
    lib_running = 1;
    ready_events = 0;
    pthread_mutex_init(&vnd_mutex, NULL);
    pthread_cond_init(&vnd_cond, NULL);
    pthread_attr_init(&thread_attr);
    
    if (pthread_create(&worker_thread, &thread_attr, bt_vndor_worker_thread, NULL) != 0)
    {
        LOGE("pthread_create failed!");
        lib_running = 0;
        return BT_VENDOR_STATUS_FAIL;
    }

    if(pthread_getschedparam(worker_thread, &policy, &param)==0)
    {
        policy = SCHED_FIFO;
        param.sched_priority = BTVND_MAIN_THREAD_PRIORITY;
        pthread_setschedparam(worker_thread, policy, &param);
    }

    return BT_VENDOR_STATUS_SUCCESS;
}


/** Chip power control */
static void set_power(bt_vendor_chip_power_state_t state)
{
    BTVNDDBG("set_power %d", state);

    if (state == BT_VENDOR_CHIP_PWR_OFF)
        upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
    else if (state == BT_VENDOR_CHIP_PWR_ON)
        upio_set_bluetooth_power(UPIO_BT_POWER_ON);    
}


/** Configure low power mode wake state */
static int lpm(bt_vendor_low_power_event_t event)
{
    uint8_t status = TRUE;

    switch (event)
    {
        case BT_VENDOR_LPM_DISABLE:
            btvnd_signal_event(VND_EVENT_LPM_DISABLE);
            break;
            
        case BT_VENDOR_LPM_ENABLE:
            btvnd_signal_event(VND_EVENT_LPM_ENABLE);
            break;
            
        case BT_VENDOR_LPM_WAKE_ASSERT:
            btvnd_signal_event(VND_EVENT_LPM_WAKE_DEVICE);
            break;
            
        case BT_VENDOR_LPM_WAKE_DEASSERT:
            btvnd_signal_event(VND_EVENT_LPM_ALLOW_SLEEP);
            break;            
    }
    
    return(status == TRUE) ? BT_VENDOR_STATUS_SUCCESS : BT_VENDOR_STATUS_FAIL;
}


/** Called prio to stack initialization */
static void preload(TRANSAC transac)
{
    BTVNDDBG("preload");
    btvnd_signal_event(VND_EVENT_PRELOAD);
}


/** Called post stack initialization */
static void postload(TRANSAC transac)
{
    BTVNDDBG("postload");
    btvnd_signal_event(VND_EVENT_POSTLOAD);
}


/** Transmit frame */
static int transmit_buf(TRANSAC transac, char *p_buf, int len)
{
    utils_enqueue(&tx_q, (void *) transac);
    
    btvnd_signal_event(VND_EVENT_TX);
    
    return BT_VENDOR_STATUS_SUCCESS;
}


/** Controls receive flow */
static int set_rxflow(bt_rx_flow_state_t state)
{
    BTVNDDBG("set_rxflow %d", state);

    userial_ioctl(((state == BT_RXFLOW_ON) ? USERIAL_OP_RXFLOW_ON : USERIAL_OP_RXFLOW_OFF), NULL);
        
    return BT_VENDOR_STATUS_SUCCESS;
}


/** Closes the interface */
static void cleanup( void )
{
    BTVNDDBG("cleanup");

    if (lib_running)
    {
        lib_running = 0;
        btvnd_signal_event(VND_EVENT_EXIT);
        pthread_join(worker_thread, NULL);
    }
    
    userial_close();
    hci_h4_cleanup();
    upio_cleanup();
    utils_cleanup();
    
    bt_vendor_cbacks = NULL;
}


static const bt_vendor_interface_t bluetoothVendorLibInterface = {
    sizeof(bt_vendor_interface_t),
    init,
    set_power,
    lpm,
    preload,
    postload,
    transmit_buf,
    set_rxflow,
    cleanup
};


/*******************************************************************************
**
** Function        bt_vndor_worker_thread
**
** Description     Mian worker thread
**
** Returns         void *
**
*******************************************************************************/
static void *bt_vndor_worker_thread(void *arg)
{
    uint16_t events;
    VND_BT_HDR *p_msg, *p_next_msg;
    
    BTVNDDBG("bt_vndor_worker_thread started");
    tx_cmd_pkts_pending = FALSE;

    while (lib_running)
    {
        pthread_mutex_lock(&vnd_mutex);
        while (ready_events == 0)
        {
            pthread_cond_wait(&vnd_cond, &vnd_mutex);			
        }
        events = ready_events;
        ready_events = 0;
        pthread_mutex_unlock(&vnd_mutex);

        if (events & VND_EVENT_RX)
        {
            hci_h4_receive_msg();

            if ((tx_cmd_pkts_pending == TRUE) && (num_hci_cmd_pkts > 0))
            {   
                /* Got HCI Cmd Credits from Controller.
                 * Prepare to send prior pending Cmd packets in the
                 * following VND_EVENT_TX session.
                 */
                events |= VND_EVENT_TX;
            }
        }

        if (events & VND_EVENT_PRELOAD)
        {
            userial_open(USERIAL_PORT_1, (tUSERIAL_CFG *) &userial_init_cfg);
            hw_config_start();
        }
        
        if (events & VND_EVENT_POSTLOAD)
        {
            /* Start from SCO related H/W configuration, if SCO configuration
             * is required. Then, follow with reading requests of getting 
             * ACL data length for both BR/EDR and LE.
             */        
#if (SCO_CFG_INCLUDED == TRUE)
            hw_sco_config();
#else
            hci_h4_get_acl_data_length();
#endif
        }
        
        if (events & VND_EVENT_TX)
        {
            /* 
             *  We will go through every packets in the tx queue.
             *  Fine to clear tx_cmd_pkts_pending.
             */
            tx_cmd_pkts_pending = FALSE;
            
            p_next_msg = tx_q.p_first;
            while (p_next_msg)
            {
                if ((p_next_msg->event & MSG_EVT_MASK) == MSG_STACK_TO_VND_HCI_CMD)
                {
                    /*
                     *  if we have used up controller's outstanding HCI command credits
                     *  (normally is 1), skip all HCI command packets in the queue. 
                     *  The pending command packets will be sent once controller gives 
                     *  back us credits through CommandCompleteEvent or CommandStatusEvent.
                     */
                    if ((tx_cmd_pkts_pending == TRUE) || (num_hci_cmd_pkts <= 0))
                    {
                        tx_cmd_pkts_pending = TRUE;
                        p_next_msg = utils_getnext(p_next_msg);
                        continue;
                    }
                }
                
                p_msg = p_next_msg;
                p_next_msg = utils_getnext(p_msg);
                utils_remove_from_queue(&tx_q, p_msg);
                hci_h4_send_msg(p_msg);
            }

            if (tx_cmd_pkts_pending == TRUE)
                BTVNDDBG("Used up Tx Cmd credits");
            
        }

        if (events & VND_EVENT_LPM_ENABLE)
        {
            hw_lpm_enable(TRUE);
        }

        if (events & VND_EVENT_LPM_DISABLE)
        {
            hw_lpm_enable(FALSE);
        }

        if (events & VND_EVENT_LPM_IDLE_TIMEOUT)
        {
            hw_lpm_deassert_bt_wake();        
        }

        if (events & VND_EVENT_LPM_ALLOW_SLEEP)
        {
            hw_lpm_allow_bt_device_sleep();        
        }

        if (events & VND_EVENT_LPM_WAKE_DEVICE)
        {
            hw_lpm_assert_bt_wake();        
        }
        
        if (events & VND_EVENT_EXIT)
            break;
    }

    BTVNDDBG("bt_vndor_worker_thread exiting");

    pthread_exit(NULL);
    
    return NULL;    // compiler friendly
}


/*******************************************************************************
**
** Function        bt_vendor_get_interface
**
** Description     Caller calls this function to get API instance
**
** Returns         API table
**
*******************************************************************************/
const bt_vendor_interface_t *bt_vendor_get_interface(void)
{  
    return &bluetoothVendorLibInterface; 
}



