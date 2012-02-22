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
 *  Filename:      userial.c
 *
 *  Description:   
 * 
 ***********************************************************************************/
 
#define LOG_TAG "bt_userial"

#include <utils/Log.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include "bt_vendor_brcm.h"
#include "userial.h"
#include "utils.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#ifndef USERIAL_DBG
#define USERIAL_DBG FALSE
#endif

#if (USERIAL_DBG == TRUE)
#define USERIALDBG LOGD
#else
#define USERIALDBG
#endif

#define MAX_SERIAL_PORT (USERIAL_PORT_3 + 1)
#define READ_LIMIT (BTVND_USERIAL_READ_MEM_SIZE - BT_VND_HDR_SIZE)

enum {
    USERIAL_RX_EXIT,
    USERIAL_RX_FLOW_OFF,
    USERIAL_RX_FLOW_ON
};

/************************************************************************************
**  Local type definitions
************************************************************************************/

typedef struct
{
    int             sock;
    uint8_t         port;
    pthread_t       read_thread;
    tUSERIAL_CFG    cfg;
    BUFFER_Q        rx_q;
    VND_BT_HDR      *p_rx_hdr;
} tUSERIAL_CB;

/************************************************************************************
**  Static variables
************************************************************************************/

static tUSERIAL_CB userial_cb;
static volatile uint8_t userial_running = 0;

/* Mapping of USERIAL_PORT_x to device ports */
char userial_dev[][256] =
{
    BLUETOOTH_UART_DEVICE_PORT       /* USERIAL_PORT_1 (HCI)*/
};

/* for friendly debugging outpout string */
static uint32_t userial_baud_tbl[] =
{
    300,        /* USERIAL_BAUD_300          0 */
    600,        /* USERIAL_BAUD_600          1 */
    1200,       /* USERIAL_BAUD_1200         2 */
    2400,       /* USERIAL_BAUD_2400         3 */
    9600,       /* USERIAL_BAUD_9600         4 */
    19200,      /* USERIAL_BAUD_19200        5 */
    57600,      /* USERIAL_BAUD_57600        6 */
    115200,     /* USERIAL_BAUD_115200       7 */
    230400,     /* USERIAL_BAUD_230400       8 */
    460800,     /* USERIAL_BAUD_460800       9 */
    921600,     /* USERIAL_BAUD_921600       10 */
    1000000,    /* USERIAL_BAUD_1M           11 */
    1500000,    /* USERIAL_BAUD_1_5M         12 */
    2000000,    /* USERIAL_BAUD_2M           13 */
    3000000,    /* USERIAL_BAUD_3M           14 */
    4000000     /* USERIAL_BAUD_4M           15 */
};

/************************************************************************************
**  Static functions
************************************************************************************/

/*****************************************************************************
**   Socket signal functions to wake up userial_read_thread for termination
**
**   creating an unnamed pair of connected sockets
**      - signal_fds[0]: join fd_set in select call of userial_read_thread
**      - signal_fds[1]: trigger from userial_close
*****************************************************************************/
static int signal_fds[2]={0,1};
static uint8_t rx_flow_on = TRUE;
static inline int create_signal_fds(fd_set* set)
{
    if(signal_fds[0] == 0 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds) < 0)
    {
        LOGE("create_signal_sockets:socketpair failed, errno: %d", errno);
        return -1;
    }
    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}
static inline int send_wakeup_signal(char sig_cmd)
{
    return send(signal_fds[1], &sig_cmd, sizeof(sig_cmd), 0);     
}
static inline char reset_signal()
{
    char sig_recv = -1;
    recv(signal_fds[0], &sig_recv, sizeof(sig_recv), MSG_WAITALL);
    return sig_recv;
}
static inline int is_signaled(fd_set* set)
{
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
                    LOGW( "read() returned 0!" );

                return ret;
            }
        }
        else if (n < 0)
            LOGW( "select() Failed");
        else if (n == 0)
            LOGW( "Got a select() TIMEOUT");

    }    
    
    return ret;
}

/*******************************************************************************
**
** Function        userial_read_thread
**
** Description     
**
** Returns         void *
**
*******************************************************************************/
static void *userial_read_thread(void *arg)
{
    int rx_length = 0;
    VND_BT_HDR *p_buf = NULL;
    uint8_t *p;

    USERIALDBG("Entering userial_read_thread()");
    
    rx_flow_on = TRUE;
    userial_running = 1;

    while (userial_running)
    {
        if (bt_vendor_cbacks)
        {
            p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BTVND_USERIAL_READ_MEM_SIZE);
        }
        else
            p_buf = NULL;

        if (p_buf != NULL)
        {
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            
            p = (uint8_t *) (p_buf + 1);
            rx_length = select_read(userial_cb.sock, p, READ_LIMIT);            
        }
        else
        {
            rx_length = 0;
            utils_delay(100);
            LOGW("userial_read_thread() failed to gain buffers");
            continue;
        }

        
        if (rx_length > 0)
        {
            p_buf->len = (uint16_t)rx_length;
            utils_enqueue(&(userial_cb.rx_q), p_buf);
            btvnd_signal_event(VND_EVENT_RX);
        }
        else /* either 0 or < 0 */
        {
            LOGW("select_read return size <=0:%d, exiting userial_read_thread", rx_length);
            /* if we get here, we should have a buffer */
            bt_vendor_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
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
**   Helper Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        baud_userial_to_tcio
**
** Description     helper function converts USERIAL baud rates into TCIO 
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_to_tcio_baud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_600)
        *baud = B600;
    else if (cfg_baud == USERIAL_BAUD_1200)
        *baud = B1200;
    else if (cfg_baud == USERIAL_BAUD_9600)
        *baud = B9600;
    else if (cfg_baud == USERIAL_BAUD_19200)
        *baud = B19200;
    else if (cfg_baud == USERIAL_BAUD_57600)
        *baud = B57600;
    else if (cfg_baud == USERIAL_BAUD_115200)
        *baud = B115200;
    else if (cfg_baud == USERIAL_BAUD_230400)
        *baud = B230400;
    else if (cfg_baud == USERIAL_BAUD_460800)
        *baud = B460800;
    else if (cfg_baud == USERIAL_BAUD_921600)
        *baud = B921600;
    else if (cfg_baud == USERIAL_BAUD_1M)
        *baud = B1000000;
    else if (cfg_baud == USERIAL_BAUD_2M)
        *baud = B2000000;
    else if (cfg_baud == USERIAL_BAUD_3M)
        *baud = B3000000;
    else if (cfg_baud == USERIAL_BAUD_4M)
        *baud = B4000000;
    else
    {
        LOGE( "userial_open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return FALSE;
    }
    
    return TRUE;
}

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
/*******************************************************************************
**
** Function        userial_ioctl_init_bt_wake
**
** Description     helper function to set the open state of the bt_wake if ioctl 
**                  is used. it should not hurt in the rfkill case but it might 
**                  be better to compile it out.
**
** Returns         none
**
*******************************************************************************/
void userial_ioctl_init_bt_wake(int fd)
{
    uint32_t bt_wake_state;
    
    /* assert BT_WAKE through ioctl */
    ioctl( fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
    ioctl( fd, USERIAL_IOCTL_BT_WAKE_GET_ST, &bt_wake_state);
    USERIALDBG("userial_ioctl_init_bt_wake read back BT_WAKE state=%i", bt_wake_state);
}
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)


/*****************************************************************************
**   Userial API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_init
**
** Description     Initializes the userial driver
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_init(void)
{
    USERIALDBG("userial_init");
    memset(&userial_cb, 0, sizeof(tUSERIAL_CB));
    userial_cb.sock = -1;
    utils_queue_init(&(userial_cb.rx_q));
    return TRUE;
}


/*******************************************************************************
**
** Function        userial_open
**
** Description     Open the indicated serial port with the given configuration
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_open(uint8_t port, tUSERIAL_CFG *p_cfg)
{
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;
    struct termios termios;
    struct sched_param param;
    int policy;
    pthread_attr_t thread_attr;
    char device_name[20];

    USERIALDBG("userial_open(port:%d, baud:%d)", port, p_cfg->baud);

    if (userial_running)
    {
        /* Userial is open; close it first */
        userial_close();
        utils_delay(50);
    }
    
    if (port >= MAX_SERIAL_PORT)
    {
        LOGE("Port > MAX_SERIAL_PORT");
        return FALSE;
    }

    if (!userial_to_tcio_baud(p_cfg->baud, &baud))
    {
        return FALSE;
    }
    
    if(p_cfg->fmt & USERIAL_DATABITS_8)
        data_bits = CS8;
    else if(p_cfg->fmt & USERIAL_DATABITS_7)
        data_bits = CS7;
    else if(p_cfg->fmt & USERIAL_DATABITS_6)
        data_bits = CS6;
    else if(p_cfg->fmt & USERIAL_DATABITS_5)
        data_bits = CS5;
    else
    {
        LOGE("userial_open: unsupported data bits");
        return FALSE;
    }

    if(p_cfg->fmt & USERIAL_PARITY_NONE)
        parity = 0;
    else if(p_cfg->fmt & USERIAL_PARITY_EVEN)
        parity = PARENB;
    else if(p_cfg->fmt & USERIAL_PARITY_ODD)
        parity = (PARENB | PARODD);
    else
    {
        LOGE("userial_open: unsupported parity bit mode");
        return FALSE;
    }

    if(p_cfg->fmt & USERIAL_STOPBITS_1)
        stop_bits = 0;
    else if(p_cfg->fmt & USERIAL_STOPBITS_2)
        stop_bits = CSTOPB;
    else
    {
        LOGE("userial_open: unsupported stop bits");
        return FALSE;
    }

    sprintf(device_name, "%s", userial_dev[port]);
    LOGI("userial_open: opening %s", device_name);
    
    if ((userial_cb.sock = open(device_name, O_RDWR)) == -1) 
    {
        LOGE("userial_open: unable to open %s", device_name);
        return FALSE;
    }

    tcflush(userial_cb.sock, TCIOFLUSH);

    tcgetattr(userial_cb.sock, &termios);
    cfmakeraw(&termios);
    termios.c_cflag |= (CRTSCTS | stop_bits);
    tcsetattr(userial_cb.sock, TCSANOW, &termios);

    tcflush(userial_cb.sock, TCIOFLUSH);

    /* set input/output baudrate */
    cfsetospeed(&termios, baud);
    cfsetispeed(&termios, baud);
    tcsetattr(userial_cb.sock, TCSANOW, &termios);

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    userial_ioctl_init_bt_wake(userial_cb.sock);
#endif

    USERIALDBG( "sock = %d", userial_cb.sock);

    userial_cb.port = port;
    memcpy(&userial_cb.cfg, p_cfg, sizeof(tUSERIAL_CFG));


    pthread_attr_init(&thread_attr);

    if (pthread_create(&(userial_cb.read_thread), &thread_attr, userial_read_thread, NULL) != 0 )
    {
        LOGE("pthread_create failed!\n\r");
        return FALSE;
    }
    
    if(pthread_getschedparam(userial_cb.read_thread, &policy, &param)==0)
    {
        policy = SCHED_FIFO;
        param.sched_priority = BTVND_USERIAL_READ_THREAD_PRIORITY;
        pthread_setschedparam(userial_cb.read_thread, policy, &param);
    }

    return TRUE;
}

/*******************************************************************************
**
** Function        userial_read
**
** Description     Read data from the userial port
**
** Returns         Number of bytes actually read from the userial port and 
**                 copied into p_data.  This may be less than len.
**
*******************************************************************************/
uint16_t  userial_read(uint8_t *p_buffer, uint16_t len)
{
    uint16_t total_len = 0;
    uint16_t copy_len = 0;
    uint8_t *p_data = NULL;
    
    do
    {
        if(userial_cb.p_rx_hdr != NULL)
        {
            p_data = ((uint8_t *)(userial_cb.p_rx_hdr + 1)) + (userial_cb.p_rx_hdr->offset);

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
                if (bt_vendor_cbacks)
                    bt_vendor_cbacks->dealloc((TRANSAC) userial_cb.p_rx_hdr, (char *) (userial_cb.p_rx_hdr + 1));
                
                userial_cb.p_rx_hdr = NULL;
            }
        }

        if(userial_cb.p_rx_hdr == NULL)
        {
            userial_cb.p_rx_hdr = (VND_BT_HDR *)utils_dequeue(&(userial_cb.rx_q));
        }
    } while ((userial_cb.p_rx_hdr != NULL) && (total_len < len));

#if 0
    if (total_len < len)
        USERIALDBG("userial_read() gives %d when asks %d", total_len, len);
#endif

    return total_len;
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
uint16_t userial_write(uint8_t *p_data, uint16_t len)
{
    int ret, total = 0;

    while(len != 0)
    {
        ret = write(userial_cb.sock, p_data+total, len);
        total += ret;
        len -= ret;
    }

#if 0
    if (total < len)
        USERIALDBG("userial_write() does %d when asks %d", total, len);
#endif

    return ((uint16_t)total);
}

/*******************************************************************************
**
** Function        userial_close
**
** Description     Close the userial port
**
** Returns         None
**
*******************************************************************************/
void userial_close(void)
{
    int result;
    TRANSAC p_buf;
    
    USERIALDBG("userial_close(sock:%d)", userial_cb.sock);

    if (userial_running)
        send_wakeup_signal(USERIAL_RX_EXIT);
    
    if ((result=pthread_join(userial_cb.read_thread, NULL)) < 0)
        LOGE( "pthread_join() FAILED result:%d", result);

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    /* de-assert bt_wake BEFORE closing port */
    ioctl(userial_cb.sock, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
#endif

    if ((result=close(userial_cb.sock)) < 0)
        LOGE( "close(sock:%d) FAILED result:%d", userial_cb.sock, result);

    userial_cb.sock = -1;

    if (bt_vendor_cbacks)
    {
        while ((p_buf = utils_dequeue (&(userial_cb.rx_q))) != NULL)
        {
            bt_vendor_cbacks->dealloc(p_buf, (char *) ((VND_BT_HDR *)p_buf + 1));
        }
    }
}

/*******************************************************************************
**
** Function        userial_change_baud
**
** Description     Change baud rate of userial port
**
** Returns         None
**
*******************************************************************************/
void userial_change_baud(uint8_t baud)
{
    struct termios termios;

    USERIALDBG("userial_change_baud: Closing UART Port");
    userial_close();

    utils_delay(50);

    /* change baud rate in settings - leave everything else the same  */
    userial_cb.cfg.baud = baud;

    LOGI("userial_change_rate: Attempting to reopen the UART Port at %i", (unsigned int)userial_baud_tbl[baud]);

    userial_open(userial_cb.port, &userial_cb.cfg);
}

/*******************************************************************************
**
** Function        userial_ioctl
**
** Description     ioctl inteface 
**
** Returns         None
**
*******************************************************************************/
void userial_ioctl(userial_ioctl_op_t op, void *p_data)
{
    switch(op) 
    {
        case USERIAL_OP_RXFLOW_ON:
            if (userial_running)
                send_wakeup_signal(USERIAL_RX_FLOW_ON);
            break;
            
        case USERIAL_OP_RXFLOW_OFF:
            if (userial_running)
                send_wakeup_signal(USERIAL_RX_FLOW_OFF);
            break;
    
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
        case USERIAL_OP_ASSERT_BT_WAKE:
            USERIALDBG("##### userial_ioctl: Asserting BT_Wake ####");
            ioctl(userial_cb.sock, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
            break;
        
        case USERIAL_OP_DEASSERT_BT_WAKE:
            USERIALDBG("##### userial_ioctl: De-asserting BT_Wake ####");
            ioctl(userial_cb.sock, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
            break;

        case USERIAL_OP_GET_BT_WAKE_STATE:
            ioctl(userial_cb.sock, USERIAL_IOCTL_BT_WAKE_GET_ST, p_data);
            break;
#endif  //  (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

        case USERIAL_OP_INIT:
        default:
            break;
    }
}

