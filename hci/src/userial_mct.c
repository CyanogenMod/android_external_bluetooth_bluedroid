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
 *  Filename:      userial_mct.c
 *
 *  Description:   Contains open/read/write/close functions on multi-channels
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial_mct"

#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#ifdef QCOM_WCN_SSR
#include <termios.h>
#include <sys/ioctl.h>
#endif
#include <sys/prctl.h>
#include "bt_hci_bdroid.h"
#include "userial.h"
#include "utils.h"
#include "vendor.h"
#include "bt_vendor_lib.h"
#include "bt_utils.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define USERIAL_DBG TRUE

#ifndef USERIAL_DBG
#define USERIAL_DBG FALSE
#endif

#if (USERIAL_DBG == TRUE)
#define USERIALDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define USERIALDBG(param, ...) {}
#endif

#define MAX_SERIAL_PORT (USERIAL_PORT_3 + 1)
#define MAX_RETRIAL_CLOSE 50

enum {
    USERIAL_RX_EXIT,
};

typedef enum {
    USERIAL_STATE_OPENING,
    USERIAL_STATE_OPENED,
    USERIAL_STATE_IDLE
} tUSERIAL_STATE;

/******************************************************************************
**  Externs
******************************************************************************/
uint16_t hci_mct_receive_evt_msg(void);
uint16_t hci_mct_receive_acl_msg(void);


/******************************************************************************
**  Local type definitions
******************************************************************************/

typedef struct
{
    int             fd[CH_MAX];
    uint8_t         port;
    pthread_t       read_thread;
    BUFFER_Q        rx_q;
    HC_BT_HDR      *p_rx_hdr;
} tUSERIAL_CB;

/******************************************************************************
**  Static variables
******************************************************************************/

static tUSERIAL_CB userial_cb;
static volatile uint8_t userial_running = 0;
static volatile uint8_t userial_close_pending = FALSE;
static volatile tUSERIAL_STATE userial_state = USERIAL_STATE_IDLE;

/******************************************************************************
**  Static functions
******************************************************************************/

/*****************************************************************************
**   Socket signal functions to wake up userial_read_thread for termination
**
**   creating an unnamed pair of connected sockets
**      - signal_fds[0]: join fd_set in select call of userial_read_thread
**      - signal_fds[1]: trigger from userial_close
*****************************************************************************/
static int signal_fds[2]={0,1};
static inline int create_signal_fds(fd_set* set)
{
    if(signal_fds[0]==0 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds)<0)
    {
        ALOGE("create_signal_sockets:socketpair failed, errno: %d", errno);
        return -1;
    }
    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}
static inline int send_wakeup_signal(char sig_cmd)
{
    return TEMP_FAILURE_RETRY(send(signal_fds[1], &sig_cmd, sizeof(sig_cmd), 0));
}
static inline char reset_signal()
{
    char sig_recv = -1;
    TEMP_FAILURE_RETRY(recv(signal_fds[0], &sig_recv, sizeof(sig_recv), MSG_WAITALL));
    return sig_recv;
}
static inline int is_signaled(fd_set* set)
{
    return FD_ISSET(signal_fds[0], set);
}

/*******************************************************************************
**
** Function        userial_evt_read_thread
**
** Description     The reading thread on EVT and ACL_IN channels
**
** Returns         void *
**
*******************************************************************************/
static void *userial_read_thread(void *arg)
{
    UNUSED(arg);

    fd_set input;
    int n;
    char reason = 0;

    USERIALDBG("Entering userial_read_thread()");

    userial_running = 1;

    prctl(PR_SET_NAME, (unsigned long)"bt_userial_mct", 0, 0, 0);
    raise_priority_a2dp(TASK_HIGH_USERIAL_READ);

    while (userial_running)
    {
        /* Initialize the input fd set */
        FD_ZERO(&input);
        FD_SET(userial_cb.fd[CH_EVT], &input);
        FD_SET(userial_cb.fd[CH_ACL_IN], &input);

        int fd_max = create_signal_fds(&input);
        fd_max = (fd_max>userial_cb.fd[CH_EVT]) ? fd_max : userial_cb.fd[CH_EVT];
        fd_max = (fd_max>userial_cb.fd[CH_ACL_IN]) ? fd_max : userial_cb.fd[CH_ACL_IN];

        /* Do the select */
        n = 0;
        n = TEMP_FAILURE_RETRY(select(fd_max+1, &input, NULL, NULL, NULL));
        if(is_signaled(&input))
        {
            reason = reset_signal();
            if (reason == USERIAL_RX_EXIT)
            {
                ALOGI("exiting userial_read_thread");
                userial_running = 0;
                break;
            }
        }

        if (n > 0)
        {
            /* We might have input */
            if (FD_ISSET(userial_cb.fd[CH_EVT], &input))
            {
                hci_mct_receive_evt_msg();
            }

            if (FD_ISSET(userial_cb.fd[CH_ACL_IN], &input))
            {
                hci_mct_receive_acl_msg();
            }
        }
        else if (n < 0)
            ALOGW( "select() Failed");
        else if (n == 0)
            ALOGW( "Got a select() TIMEOUT");
    } /* while */

    userial_running = 0;
    USERIALDBG("Leaving userial_evt_read_thread()");
    pthread_exit(NULL);

    return NULL;    // Compiler friendly
}
#ifdef QCOM_WCN_SSR
/*******************************************************************************
**
** Function        userial_dev_inreset
**
** Description     checks for H/w reset events
**
** Returns         reset status
**
*******************************************************************************/

uint8_t userial_dev_inreset()
{
    volatile int serial_bits;
    uint8_t dev_reset_done =0, retry_count = 0;
     ioctl(userial_cb.fd[CH_EVT], TIOCMGET, &serial_bits);
     if (serial_bits & TIOCM_OUT2) {
        while(serial_bits & TIOCM_OUT1) {
             ALOGW("userial_device in reset \n");
             utils_delay(2000);
             retry_count++;
             ioctl(userial_cb.fd[CH_EVT], TIOCMGET, &serial_bits);
             if((serial_bits & TIOCM_OUT1))
                dev_reset_done = 0;
             else
                dev_reset_done = 1;
             if(retry_count == 6)
               break;
          }
     }
    return dev_reset_done;
}
#endif


/*****************************************************************************
**   Userial API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_init
**
** Description     Initializes the userial driver
**
*******************************************************************************/
bool userial_init(void)
{
    int idx;

    USERIALDBG("userial_init");
    memset(&userial_cb, 0, sizeof(tUSERIAL_CB));
    for (idx=0; idx < CH_MAX; idx++)
        userial_cb.fd[idx] = -1;
    utils_queue_init(&(userial_cb.rx_q));
    return true;
}


/*******************************************************************************
**
** Function        userial_open
**
** Description     Open Bluetooth device with the port ID
**
*******************************************************************************/
bool userial_open(userial_port_t port)
{
    int result;

    USERIALDBG("userial_open(port:%d)", port);

    if (userial_running)
    {
        USERIALDBG("userial_open: userial_running =1");
        /* Userial is open; close it first */
        userial_close();
        utils_delay(50);
    }

    userial_state = USERIAL_STATE_OPENING;
    if (port >= MAX_SERIAL_PORT)
    {
        ALOGE("Port > MAX_SERIAL_PORT");
        userial_state = USERIAL_STATE_IDLE;
        return FALSE;
    }

    result = vendor_send_command(BT_VND_OP_USERIAL_OPEN, &userial_cb.fd);
    if ((result != 2) && (result != 4))
    {
        ALOGE("userial_open: wrong numbers of open fd in vendor lib [%d]!",
                result);
        ALOGE("userial_open: HCI MCT expects 2 or 4 open file descriptors");
        vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);
        userial_state = USERIAL_STATE_IDLE;
        return FALSE;
    }

    //This check handles the situation where userial_open takes time in BT_VND_OP_USERIAL_OPEN
    //opening and meanwhile the close request comes.This way it will lead to crash since call
    //flow will be like open-->close-->userial Rx thread created.
    if(userial_close_pending == TRUE)
    {
        ALOGW("userial_open:Already got close request for userial port so not opening");
        userial_close_pending = FALSE;
        userial_state = USERIAL_STATE_IDLE;
        return FALSE;
    }

    ALOGI("CMD=%d, EVT=%d, ACL_Out=%d, ACL_In=%d", \
        userial_cb.fd[CH_CMD], userial_cb.fd[CH_EVT], \
        userial_cb.fd[CH_ACL_OUT], userial_cb.fd[CH_ACL_IN]);

    if ((userial_cb.fd[CH_CMD] == -1) || (userial_cb.fd[CH_EVT] == -1) ||
        (userial_cb.fd[CH_ACL_OUT] == -1) || (userial_cb.fd[CH_ACL_IN] == -1))
    {
        ALOGE("userial_open: failed to open BT transport");
        vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);
        userial_state = USERIAL_STATE_IDLE;
        return FALSE;
    }

    userial_cb.port = port;

    /* Start listening thread */
    if (pthread_create(&userial_cb.read_thread, NULL, userial_read_thread, NULL) != 0)
    {
        ALOGE("pthread_create failed!");
        vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);
        userial_state = USERIAL_STATE_IDLE;
        return FALSE;
    }

    userial_state = USERIAL_STATE_OPENED;
    return TRUE;
}

/*******************************************************************************
**
** Function        userial_read
**
** Description     Read data from the userial channel
**
** Returns         Number of bytes actually read from the userial port and
**                 copied into p_data.  This may be less than len.
**
*******************************************************************************/
uint16_t userial_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len)
{
    int ret = -1;
    int ch_idx = (msg_id == MSG_HC_TO_STACK_HCI_EVT) ? CH_EVT : CH_ACL_IN;

    ret = TEMP_FAILURE_RETRY(read(userial_cb.fd[ch_idx], p_buffer, (size_t)len));
    if (ret <= 0)
        ALOGW( "userial_read: read() returned %d!", ret);

    return (uint16_t) ((ret >= 0) ? ret : 0);
}

/*******************************************************************************
**
** Function        userial_write
**
** Description     Write data to the userial port
**
** Returns         Number of bytes actually written to the userial port. This
**                 may be less than len.
**
*******************************************************************************/
uint16_t userial_write(uint16_t msg_id, const uint8_t *p_data, uint16_t len)
{
    int ret, total = 0;
    int ch_idx = (msg_id == MSG_STACK_TO_HC_HCI_CMD) ? CH_CMD : CH_ACL_OUT;

    while(len != 0)
    {
        ret = TEMP_FAILURE_RETRY(write(userial_cb.fd[ch_idx], p_data+total, len));
        total += ret;
        len -= ret;
    }

    return ((uint16_t)total);
}

void userial_close_reader(void) {
    // Join the reader thread if it is still running.
    if (userial_running) {
        send_wakeup_signal(USERIAL_RX_EXIT);
        int result = pthread_join(userial_cb.read_thread, NULL);
        USERIALDBG("%s Joined userial reader thread: %d", __func__, result);
        if (result)
            ALOGE("%s failed to join reader thread: %d", __func__, result);
        return;
    }
    ALOGW("%s Already closed userial reader thread", __func__);
}

/*******************************************************************************
**
** Function        userial_close
**
** Description     Close the userial port
**
*******************************************************************************/
void userial_close(void)
{
    int idx, result;
    int i = 0;

    USERIALDBG("userial_close");
    userial_close_pending = TRUE;
    while((userial_state == USERIAL_STATE_OPENING) && (i< MAX_RETRIAL_CLOSE))
    {
        usleep(200);
        i++;
    }

    if (i == MAX_RETRIAL_CLOSE)
        USERIALDBG("USERIAL CLOSE : Timeout in creating userial read thread");

    if (userial_running)
        send_wakeup_signal(USERIAL_RX_EXIT);

    if ((result=pthread_join(userial_cb.read_thread, NULL)) != 0)
        ALOGE( "pthread_join() FAILED result:%d", result);

    vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);

    userial_state = USERIAL_STATE_IDLE;
    userial_close_pending = FALSE;

    for (idx=0; idx < CH_MAX; idx++)
        userial_cb.fd[idx] = -1;
}
