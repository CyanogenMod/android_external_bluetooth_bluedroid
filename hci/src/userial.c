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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <utils/Log.h>

#include "bt_hci_bdroid.h"
#include "bt_utils.h"
#include "bt_vendor_lib.h"
#include "userial.h"
#include "utils.h"
#include "vendor.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef USERIAL_DBG
#define USERIAL_DBG TRUE
#endif

#if (USERIAL_DBG == TRUE)
#define USERIALDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define USERIALDBG(param, ...) {}
#endif

#define MAX_SERIAL_PORT (USERIAL_PORT_3 + 1)

// The set of events one can send to the userial read thread.
// Note that the values must be >= 0x8000000000000000 to guarantee delivery
// of the message (see eventfd(2) for details on blocking behaviour).
enum {
    USERIAL_RX_EXIT     = 0x8000000000000000ULL
};

/******************************************************************************
**  Externs
******************************************************************************/

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
static int event_fd = -1;

static inline int add_event_fd(fd_set *set) {
    if (event_fd == -1) {
      event_fd = eventfd(0, 0);
      if (event_fd == -1) {
          ALOGE("%s unable to create event fd: %s", __func__, strerror(errno));
          return -1;
      }
    }

    FD_SET(event_fd, set);
    return event_fd;
}

static inline void send_event(uint64_t event_id) {
    assert(event_fd != -1);
    eventfd_write(event_fd, event_id);
}

static inline uint64_t read_event() {
    assert(event_fd != -1);

    uint64_t value = 0;
    eventfd_read(event_fd, &value);
    return value;
}

static inline bool is_event_available(fd_set *set) {
    assert(event_fd != -1);
    return !!FD_ISSET(event_fd, set);
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

    while (userial_running)
    {
        /* Initialize the input fd set */
        FD_ZERO(&input);
        FD_SET(fd, &input);
        int fd_max = add_event_fd(&input);
        fd_max = fd_max > fd ? fd_max : fd;

        /* Do the select */
        n = TEMP_FAILURE_RETRY(select(fd_max+1, &input, NULL, NULL, NULL));
        if(is_event_available(&input))
        {
            uint64_t event = read_event();
            switch (event) {
                case USERIAL_RX_EXIT:
                    USERIALDBG("RX termination");
                    return -1;
            }
        }

        if (n > 0)
        {
            /* We might have input */
            if (FD_ISSET(fd, &input))
            {
                ret = TEMP_FAILURE_RETRY(read(fd, pbuf, (size_t)len));
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
            int userial_fd = userial_cb.fd;
            if (userial_fd != -1)
                rx_length = select_read(userial_fd, p, HCI_MAX_FRAME_SIZE + 1);
            else
                rx_length = 0;
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
            bthc_rx_ready();
        }
        else /* either 0 or < 0 */
        {
            ALOGW("select_read return size <=0:%d, exiting userial_read_thread",\
                 rx_length);
            /* if we get here, we should have a buffer */
            if (bt_hc_cbacks)
                bt_hc_cbacks->dealloc(p_buf);

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

    if (userial_running) {
        userial_close();
        utils_delay(50);
    }

    // Call in to the vendor-specific library to open the serial port.
    int fd_array[CH_MAX];
    for (int i = 0; i < CH_MAX; i++)
        fd_array[i] = -1;

    int num_ports = vendor_send_command(BT_VND_OP_USERIAL_OPEN, &fd_array);

    if (num_ports != 1) {
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
    vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);
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
                    bt_hc_cbacks->dealloc(userial_cb.p_rx_hdr);

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
        ssize_t ret = TEMP_FAILURE_RETRY(write(userial_cb.fd, p_data + total, len));
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

void userial_close_reader(void) {
    // Join the reader thread if it is still running.
    if (userial_running) {
        send_event(USERIAL_RX_EXIT);
        int result = pthread_join(userial_cb.read_thread, NULL);
        USERIALDBG("%s Joined userial reader thread: %d", __func__, result);
        if (result)
            ALOGE("%s failed to join reader thread: %d", __func__, result);
        return;
    }
    ALOGW("%s Already closed userial reader thread", __func__);
}

void userial_close(void) {
    assert(bt_hc_cbacks != NULL);

    // Join the reader thread if it's still running.
    if (userial_running) {
        send_event(USERIAL_RX_EXIT);
        int result = pthread_join(userial_cb.read_thread, NULL);
        if (result)
            ALOGE("%s failed to join reader thread: %d", __func__, result);
    }

    // Ask the vendor-specific library to close the serial port.
    vendor_send_command(BT_VND_OP_USERIAL_CLOSE, NULL);

    // Free all buffers still waiting in the RX queue.
    // TODO: use list data structure and clean this up.
    void *buf;
    while ((buf = utils_dequeue(&userial_cb.rx_q)) != NULL)
        bt_hc_cbacks->dealloc(buf);

    userial_cb.fd = -1;
}
