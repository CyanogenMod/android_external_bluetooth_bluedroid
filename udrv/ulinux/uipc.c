/*****************************************************************************
 **
 **  Name:           uipc.c
 **
 **  Description:    UIPC wrapper interface definition
 **
 **  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <cutils/sockets.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>

#include "gki.h"
#include "data_types.h"
#include "uipc.h"

#define PCM_FILENAME "/data/test.pcm"
#define SOCKPATH "/data/misc/bluedroid/.a2dp_sock"

/* TEST CODE TO VERIFY DATAPATH */

#define SOURCE_MODE_PCMFILE 1
#define SOURCE_MODE_A2DP_SOCKET 2
#define SOURCE_MODE SOURCE_MODE_A2DP_SOCKET

static int listen_fd = -1;
static int sock_fd = -1;
static int source_fd = -1;

static inline int create_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
   
    if(socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0)
    {
        if(listen(s, 5) == 0)
        {
            APPL_TRACE_EVENT2("listen to local socket:%s, fd:%d", name, s);
            return s;
        }
        else 
            APPL_TRACE_EVENT3("listen to local socket:%s, fd:%d failed, errno:%d", name, s, errno);        
    }
    else 
        APPL_TRACE_EVENT3("create local socket:%s fd:%d, failed, errno:%d", name, s, errno);
    close(s);
    return -1;
}

int wait_socket(void)
{
    struct sockaddr_un remote;
    int t;

    APPL_TRACE_DEBUG0("wait socket");

    listen_fd = create_server_socket(SOCKPATH);

    if ((sock_fd = accept(listen_fd, (struct sockaddr *)&remote, &t)) == -1) {
         APPL_TRACE_ERROR1("a2dp sock accept failed (%d)", errno);
         return -1;
    }

    return sock_fd;
}

int open_source(void)
{
    int s;
#if (SOURCE_MODE == SOURCE_MODE_PCMFILE)
    s = open((char*)PCM_FILENAME  , O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (s == -1)
    {
        APPL_TRACE_ERROR0("unable to open pcm file\n");
        s = -1;  
        return 0;
    }
#endif

#if (SOURCE_MODE == SOURCE_MODE_A2DP_SOCKET)
    s = wait_socket();
#endif
    return s;
}

void close_source(int fd)
{
    close(fd);

#if (SOURCE_MODE == SOURCE_MODE_A2DP_SOCKET)
    close(listen_fd);
    listen_fd = -1;
#endif
}


/*******************************************************************************
 **
 ** Function         UIPC_Init
 **
 ** Description      Initialize UIPC module
 **
 ** Returns          void
 **
 *******************************************************************************/

UDRV_API void UIPC_Init(void *p_data)
{
    APPL_TRACE_DEBUG0("UIPC_Init");
}

/*******************************************************************************
 **
 ** Function         UIPC_Open
 **
 ** Description      Open UIPC interface
 **
 ** Returns          TRUE in case of success, FALSE in case of failure.
 **
 *******************************************************************************/
UDRV_API BOOLEAN UIPC_Open(tUIPC_CH_ID ch_id, tUIPC_RCV_CBACK *p_cback)
{
    APPL_TRACE_DEBUG2("UIPC_Open : ch_id %d, p_cback %x", ch_id, p_cback);

    return FALSE;
}

/*******************************************************************************
 **
 ** Function         UIPC_Close
 **
 ** Description      Close UIPC interface
 **
 ** Returns          void
 **
 *******************************************************************************/
UDRV_API void UIPC_Close(tUIPC_CH_ID ch_id)
{
    APPL_TRACE_DEBUG1("UIPC_Close : ch_id %d", ch_id);
    close_source(source_fd);
    source_fd = -1;
}

/*******************************************************************************
 **
 ** Function         UIPC_SendBuf
 **
 ** Description      Called to transmit a message over UIPC.
 **                  Message buffer will be freed by UIPC_SendBuf.
 **
 ** Returns          TRUE in case of success, FALSE in case of failure.
 **
 *******************************************************************************/
UDRV_API BOOLEAN UIPC_SendBuf(tUIPC_CH_ID ch_id, BT_HDR *p_msg)
{
    APPL_TRACE_DEBUG1("UIPC_SendBuf : ch_id %d", ch_id);
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         UIPC_Send
 **
 ** Description      Called to transmit a message over UIPC.
 **
 ** Returns          TRUE in case of success, FALSE in case of failure.
 **
 *******************************************************************************/
UDRV_API BOOLEAN UIPC_Send(tUIPC_CH_ID ch_id, UINT16 msg_evt, UINT8 *p_buf,
        UINT16 msglen)
{
    APPL_TRACE_DEBUG1("UIPC_Send : ch_id:%d", ch_id);
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         UIPC_ReadBuf
 **
 ** Description      Called to read a message from UIPC.
 **
 ** Returns          void
 **
 *******************************************************************************/
UDRV_API void UIPC_ReadBuf(tUIPC_CH_ID ch_id, BT_HDR *p_msg)
{
    APPL_TRACE_DEBUG1("UIPC_ReadBuf : ch_id:%d", ch_id);
}

/*******************************************************************************
 **
 ** Function         UIPC_Read
 **
 ** Description      Called to read a message from UIPC.
 **
 ** Returns          return the number of bytes read.
 **
 *******************************************************************************/
UDRV_API UINT32 UIPC_Read(tUIPC_CH_ID ch_id, UINT16 *p_msg_evt, UINT8 *p_buf,
        UINT32 len)
{
    int n;
    int n_read = 0;

    //APPL_TRACE_DEBUG1("UIPC_Read : ch_id %d", ch_id);

    if (source_fd == -1)
    {
        source_fd = open_source();
    }   

    while (n_read < (int)len)
    {
        n = read(source_fd, p_buf, len);
 
        if (n == 0)
        {
            APPL_TRACE_EVENT0("remote source detached");
            source_fd = -1;
            return 0;
        }
        n_read+=n;
        //APPL_TRACE_EVENT1("read %d bytes", n_read); 
    }

    return n_read;
}

/*******************************************************************************
**
** Function         UIPC_Ioctl
**
** Description      Called to control UIPC.
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern BOOLEAN UIPC_Ioctl(tUIPC_CH_ID ch_id, UINT32 request, void *param)
{
    APPL_TRACE_DEBUG2("#### UIPC_Ioctl : ch_ud %d, request %d ####", ch_id, request);
    return FALSE;
}

