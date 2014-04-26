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
 *  Filename:      userial.c
 *
 *  Description:   Contains open/read/write/close functions on serial port
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial"

#include <assert.h>
#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/socket.h>

#include "bt_hci_bdroid.h"
#include "userial.h"
#include "utils.h"
#include "bt_vendor_lib.h"
#include "bt_utils.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef USERIAL_DBG
#define USERIAL_DBG FALSE
#endif

#if (USERIAL_DBG == TRUE)
#define USERIALDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define USERIALDBG(param, ...) {}
#endif

#define MAX_SERIAL_PORT (USERIAL_PORT_3 + 1)

enum {
    USERIAL_RX_EXIT,
    USERIAL_RX_FLOW_OFF,
    USERIAL_RX_FLOW_ON
};

/******************************************************************************
**  Externs
******************************************************************************/

extern bt_vendor_interface_t *bt_vnd_if;

/******************************************************************************
**  Local type definitions
******************************************************************************/

typedef struct
{
    int             fd;
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

/*****************************************************************************
**   Socket signal functions to wake up userial_read_thread for termination
**
**   creating an unnamed pair of connected sockets
**      - signal_fds[0]: join fd_set in select call of userial_read_thread
**      - signal_fds[1]: trigger from userial_close
*****************************************************************************/
static int signal_fds[2] = { -1, -1 };
static uint8_t rx_flow_on = TRUE;

static inline int create_signal_fds(fd_set *set) {
    if (signal_fds[0] == -1 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds) == -1) {
        ALOGE("%s socketpair failed: %s", __func__, strerror(errno));
        return -1;
    }

    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}

static inline int send_wakeup_signal(char sig_cmd) {
    assert(signal_fds[0] != -1);
    assert(signal_fds[1] != -1);
    return send(signal_fds[1], &sig_cmd, sizeof(sig_cmd), 0);
}

static inline char reset_signal() {
    assert(signal_fds[0] != -1);
    assert(signal_fds[1] != -1);

    char sig_recv = -1;
    recv(signal_fds[0], &sig_recv, sizeof(sig_recv), MSG_WAITALL);
    return sig_recv;
}

static inline int is_signaled(fd_set *set) {
    assert(signal_fds[0] != -1);
    assert(signal_fds[1] != -1);
    return FD_ISSET(signal_fds[0], set);
}

/*******************************************************************************
**
** Function        select_read
**
** Description     check if fd is ready for reading and listen for termination
**                  signal. need to use select in order to avoid collision
**                  between read and close on the same fd
**
** Returns         -1: termination
**                 >=0: numbers of bytes read back from fd
**
*******************************************************************************/
static int select_read(int fd, uint8_t *pbuf, int len)
{
    fd_set input;
    int n = 0, ret = -1;
    char reason = 0;

    while (userial_running)
    {
        /* Initialize the input fd set */
        FD_ZERO(&input);
        if (rx_flow_on == TRUE)
        {
            FD_SET(fd, &input);
        }
        int fd_max = create_signal_fds(&input);
        fd_max = fd_max > fd ? fd_max : fd;

        /* Do the select */
        n = select(fd_max+1, &input, NULL, NULL, NULL);
        if(is_signaled(&input))
        {
            reason = reset_signal();
            if (reason == USERIAL_RX_EXIT)
            {
                USERIALDBG("RX termination");
                return -1;
            }
            else if (reason == USERIAL_RX_FLOW_OFF)
            {
                USERIALDBG("RX flow OFF");
                rx_flow_on = FALSE;
            }
            else if (reason == USERIAL_RX_FLOW_ON)
            {
                USERIALDBG("RX flow ON");
                rx_flow_on = TRUE;
            }
        }

        if (n > 0)
        {
            /* We might have input */
            if (FD_ISSET(fd, &input))
            {
                ret = read(fd, pbuf, (size_t)len);
                if (0 == ret)
                    ALOGW( "read() returned 0!" );

                return ret;
            }
        }
        else if (n < 0)
            ALOGW( "select() Failed");
        else if (n == 0)
            ALOGW( "Got a select() TIMEOUT");

    }

    return ret;
}

static void *userial_read_thread(void *arg)
{
    int rx_length = 0;
    HC_BT_HDR *p_buf = NULL;
    uint8_t *p;
    UNUSED(arg);

    USERIALDBG("Entering userial_read_thread()");
    prctl(PR_SET_NAME, (unsigned long)"userial_read", 0, 0, 0);

    rx_flow_on = TRUE;
    userial_running = 1;

    raise_priority_a2dp(TASK_HIGH_USERIAL_READ);

    while (userial_running)
    {
        if (bt_hc_cbacks)
        {
            p_buf = (HC_BT_HDR *) bt_hc_cbacks->alloc(
                        BT_HC_HDR_SIZE + HCI_MAX_FRAME_SIZE + 1); /* H4 HDR = 1 */
        }
        else
            p_buf = NULL;

        if (p_buf != NULL)
        {
            p_buf->offset = 0;
            p_buf->layer_specific = 0;

            p = (uint8_t *) (p_buf + 1);
            rx_length = select_read(userial_cb.fd, p, HCI_MAX_FRAME_SIZE + 1);
        }
        else
        {
            rx_length = 0;
            utils_delay(100);
            ALOGW("userial_read_thread() failed to gain buffers");
            continue;
        }


        if (rx_length > 0)
        {
            p_buf->len = (uint16_t)rx_length;
            utils_enqueue(&(userial_cb.rx_q), p_buf);
            bthc_signal_event(HC_EVENT_RX);
        }
        else /* either 0 or < 0 */
        {
            ALOGW("select_read return size <=0:%d, exiting userial_read_thread",\
                 rx_length);
            /* if we get here, we should have a buffer */
            bt_hc_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
            /* negative value means exit thread */
            break;
        }
    } /* for */

    userial_running = 0;
    USERIALDBG("Leaving userial_read_thread()");
    pthread_exit(NULL);

    return NULL;    // Compiler friendly
}


/*****************************************************************************
**   Userial API Functions
*****************************************************************************/

bool userial_init(void)
{
    USERIALDBG("userial_init");
    memset(&userial_cb, 0, sizeof(tUSERIAL_CB));
    userial_cb.fd = -1;
    utils_queue_init(&userial_cb.rx_q);
    return true;
}

bool userial_open(userial_port_t port) {
    if (port >= MAX_SERIAL_PORT) {
        ALOGE("%s serial port %d > %d (max).", __func__, port, MAX_SERIAL_PORT);
        return false;
    }

    if (!bt_vnd_if) {
        ALOGE("%s no vendor interface, cannot open serial port.", __func__);
        return false;
    }

    if (userial_running) {
        userial_close();
        utils_delay(50);
    }

    // Call in to the vendor-specific library to open the serial port.
    int fd_array[CH_MAX];
    int num_ports = bt_vnd_if->op(BT_VND_OP_USERIAL_OPEN, &fd_array);

    if (num_ports > 1) {
        ALOGE("%s opened wrong number of ports: got %d, expected 1.", __func__, num_ports);
        goto error;
    }

    userial_cb.fd = fd_array[0];
    if (userial_cb.fd == -1) {
        ALOGE("%s unable to open serial port.", __func__);
        goto error;
    }

    userial_cb.port = port;

    if (pthread_create(&userial_cb.read_thread, NULL, userial_read_thread, NULL)) {
        ALOGE("%s unable to spawn read thread.", __func__);
        goto error;
    }

    return true;

error:
    bt_vnd_if->op(BT_VND_OP_USERIAL_CLOSE, NULL);
    return false;
}

uint16_t userial_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len)
{
    uint16_t total_len = 0;
    uint16_t copy_len = 0;
    uint8_t *p_data = NULL;
    UNUSED(msg_id);

    do
    {
        if(userial_cb.p_rx_hdr != NULL)
        {
            p_data = ((uint8_t *)(userial_cb.p_rx_hdr + 1)) + \
                     (userial_cb.p_rx_hdr->offset);

            if((userial_cb.p_rx_hdr->len) <= (len - total_len))
                copy_len = userial_cb.p_rx_hdr->len;
            else
                copy_len = (len - total_len);

            memcpy((p_buffer + total_len), p_data, copy_len);

            total_len += copy_len;

            userial_cb.p_rx_hdr->offset += copy_len;
            userial_cb.p_rx_hdr->len -= copy_len;

            if(userial_cb.p_rx_hdr->len == 0)
            {
                if (bt_hc_cbacks)
                    bt_hc_cbacks->dealloc((TRANSAC) userial_cb.p_rx_hdr, \
                                              (char *) (userial_cb.p_rx_hdr+1));

                userial_cb.p_rx_hdr = NULL;
            }
        }

        if(userial_cb.p_rx_hdr == NULL)
        {
            userial_cb.p_rx_hdr=(HC_BT_HDR *)utils_dequeue(&(userial_cb.rx_q));
        }
    } while ((userial_cb.p_rx_hdr != NULL) && (total_len < len));

    return total_len;
}

uint16_t userial_write(uint16_t msg_id, const uint8_t *p_data, uint16_t len) {
    UNUSED(msg_id);

    uint16_t total = 0;
    while (len) {
        ssize_t ret = write(userial_cb.fd, p_data + total, len);
        switch (ret) {
            case -1:
                ALOGE("%s error writing to serial port: %s", __func__, strerror(errno));
                return total;
            case 0:  // don't loop forever in case write returns 0.
                return total;
            default:
                total += ret;
                len -= ret;
                break;
        }
    }

    return total;
}

void userial_close(void) {
    assert(bt_vnd_if != NULL);
    assert(bt_hc_cbacks != NULL);

    // Join the reader thread if it's still running.
    if (userial_running) {
        send_wakeup_signal(USERIAL_RX_EXIT);
        int result = pthread_join(userial_cb.read_thread, NULL);
        if (result)
            ALOGE("%s failed to join reader thread: %d", __func__, result);
    }

    // Ask the vendor-specific library to close the serial port.
    bt_vnd_if->op(BT_VND_OP_USERIAL_CLOSE, NULL);

    // Free all buffers still waiting in the RX queue.
    // TODO: use list data structure and clean this up.
    void *buf;
    while ((buf = utils_dequeue(&userial_cb.rx_q)) != NULL)
        bt_hc_cbacks->dealloc(buf, (char *) ((HC_BT_HDR *)buf + 1));

    userial_cb.fd = -1;
}

void userial_pause_reading(void) {
    if (userial_running)
        send_wakeup_signal(USERIAL_RX_FLOW_OFF);
}

void userial_resume_reading(void) {
    if (userial_running)
        send_wakeup_signal(USERIAL_RX_FLOW_ON);
}
