/******************************************************************************

Copyright (c) 2013, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

/******************************************************************************
 *
 *  This file contains action functions for the BT and WLAN coex.
 *
 ******************************************************************************/

#include "bta_api.h"
#include "hcimsgs.h"
#include <cutils/log.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <netinet/in.h>
#include "btc_common.h"

static pthread_t hci_mon_thread;
#define LOG_TAG "BTC_COMMON"
static int hci_event_sock;
static int sock_id;

/* =========================================================================
FUNCTION     : btc_deinit

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_deinit()
{
    int status;
    ALOGV("%s: BTC DE-INIT ", __func__);
    btsnoop_reg(NULL);
    if (sock_id > 0) {
        ALOGV("BTC_Denit:sock_id  %d hci_event_sock: %d ", sock_id, hci_event_sock);
        if (hci_event_sock < 0) {
            status = btc_close_serv_socket("btc_hci");
            if (status) {
                ALOGV("CONNECT and CLOSE _serv_sock successfully");
            } else {
                ALOGE("CONNECT failed");
            }
        }
    }

    if (hci_event_sock > 0) {
        ALOGV("BTC_Denit:HCI sock id closing %d", hci_event_sock);
        close(hci_event_sock);
        hci_event_sock = -1;
    }

    if (sock_id > 0) {
        ALOGV("BTC_Denit: SOCK_id closing %d", sock_id);
        close(sock_id);
        sock_id = -1;
    }
}

/* =========================================================================
FUNCTION     : btc_init

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_init()
{
    ALOGV("%s: BTC INIT ", __func__);
    btsnoop_reg(btc_cmd_evt_handler);
    if (pthread_create(&hci_mon_thread, NULL, (void *)btc_thread, NULL) != 0) {
        perror("pthread_create");
    }
}

/* =========================================================================
FUNCTION     : btc_send_cmd_complete

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_send_cmd_complete(char *hci_evt, int len)
{
    int status = -1;
    ALOGV("%s: SEND CMD COMPL ", __func__);

    switch (hci_evt[EVT_OGF_OFFSET]) {
    case OGF_HOST_CTL:
        if (OCF_SET_AFH_HOST_CHANNEL_CLASSIFICATION == hci_evt[EVT_OCF_OFFSET]) {
            ALOGV("btc_process_event%x OGF %x OCF%x", hci_evt[1],
                hci_evt[5], hci_evt[4]);
        } else {
            return;
        }
    break;
    case OGF_LINK_POLICY:
        if (OCF_ROLE_DISCOVERY != hci_evt[EVT_OCF_OFFSET]) {
            return;
        } else {
            ALOGV("%s: read  OCF_ROLE_DISCOVERY ", __func__);
        }
    break;
    case OGF_INFO_PARAM:
        if (OCF_READ_BD_ADDR == hci_evt[EVT_OCF_OFFSET]) {
            ALOGV("btc_process_event   %x OGF %x OCF%x",
                hci_evt[1], hci_evt[5], hci_evt[4]);
            ALOGV("%s: Read BD_ADDRESS ", __func__);
        } else {
           return;
        }
    break;
    default:
        return;
    }

    if (hci_event_sock > 0) {
        status = write(hci_event_sock, hci_evt, len);
        if(status < 0) {
            ALOGV("sock write failed");
        }
    }
}

/* =========================================================================
FUNCTION     : btc_process_event

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_process_event(char *hci_evt, int len)
{
    int status = -1;
    ALOGV("%s: PROCESS EVENT ", __func__);

    switch (hci_evt[EVT_CODE_OFFSET]) {
    case EVT_INQUIRY_COMPLETE:
    case EVT_PIN_CODE_REQ:
    case EVT_LINK_KEY_NOTIFY:
    case EVT_CONN_COMPLETE:
    case EVT_CONN_REQUEST:
    case EVT_SYNC_CONN_COMPLETE:
    case EVT_READ_REMOTE_FEATURES_COMPLETE:
    case EVT_READ_REMOTE_VERSION_COMPLETE:
    case EVT_DISCONN_COMPLETE:
    case EVT_ROLE_CHANGE:
        ALOGV("btc_process_event   %x", hci_evt[EVT_CODE_OFFSET] );
        if (hci_event_sock > 0) {
            status = write(hci_event_sock, hci_evt, len);
            if (status < 0) {
                ALOGV("sock write failed");
            }
        }
    break;
    case EVT_CMD_COMPLETE:
         btc_send_cmd_complete(hci_evt, len);
    break;
    default:
    break;
    }
}

/* =========================================================================
FUNCTION     : btc_process_client_command

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_process_client_command(char *hci_cmd, int len)
{
    int fd = -1;
    UINT16 hci_handle = 0;
    ALOGE("%s ", __func__);

    if (hci_cmd[CMD_OGF_OFFSET] == OGF_INFO_PARAM) {
        if (hci_cmd[CMD_OCF_OFFSET] == OCF_READ_BD_ADDR) {
            ALOGV("HCI  READ_BD_ADDR Command  %x", hci_cmd[2]);
            BTA_DmHciRawCommand (HCI_READ_BD_ADDR, hci_cmd[3],(UINT8 *)&hci_cmd[3], NULL);
        } else {
            return;
        }
    } else if (hci_cmd[CMD_OGF_OFFSET] == OGF_HOST_CTL) {
        if (hci_cmd[CMD_OCF_OFFSET] == OCF_SET_AFH_HOST_CHANNEL_CLASSIFICATION) {
            ALOGV("SET_AFH_CLASS %x", hci_cmd[2]);
            BTA_DmHciRawCommand (HCI_SET_AFH_CHANNELS, hci_cmd[3] ,(UINT8 *)&hci_cmd[4], NULL);
        } else {
            return;
        }
 #if 0 /* We are not supporting this commands now. Will check Bludroid behavior on this commands and will add this support later */
    } else if (hci_cmd[CMD_OGF_OFFSET] == OGF_LINK_CTL) {
        if ((hci_cmd[CMD_OCF_OFFSET] == OCF_READ_REMOTE_FEATURES)) {
            ALOGV(" BTC OCF_READ_REMOTE_FEATURES OGF  %x %x %x %x %x %x ",
                hci_cmd[0], hci_cmd[1], hci_cmd[2],
                hci_cmd[3], hci_cmd[4], hci_cmd[5]);
            BTA_DmHciRawCommand (HCI_READ_RMT_FEATURES, hci_cmd[3],
                                (UINT8 *)&hci_cmd[4], NULL);
        } else if (hci_cmd[1] == OCF_READ_REMOTE_VERSION) {
             ALOGV(" BTC HCI_READ_RMT_VERSION_INFO OGF  %x %x %x %x %x %x",
                hci_cmd[0], hci_cmd[1], hci_cmd[2],
                hci_cmd[3], hci_cmd[4], hci_cmd[5]);
             BTA_DmHciRawCommand (HCI_READ_RMT_VERSION_INFO, hci_cmd[3],
                (UINT8 *)&hci_cmd[4], NULL);
        } else {
            return;
        }
#endif
    } else if (hci_cmd[CMD_OGF_OFFSET] == OGF_LINK_POLICY) {
        if (hci_cmd[CMD_OCF_OFFSET] == OCF_ROLE_DISCOVERY) {
            ALOGV("HCI OCF_ROLE_DISCOVERY Command  %x", hci_cmd[2]);
            BTA_DmHciRawCommand (HCI_ROLE_DISCOVERY, hci_cmd[3] ,(UINT8 *)&hci_cmd[4], NULL);
        } else {
            return;
        }
    } else {
        ALOGV("Not processing this command OGF  %x", hci_cmd[2]);
        return;
    }
}

/* =========================================================================
FUNCTION     : btc_process_stack_command

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_process_stack_command(char *hci_cmd, int len)
{
    int fd = -1;
    int status = -1;
    ALOGV("%s: Stack Command ", __func__);
    if (hci_cmd[CMD_OGF_OFFSET] == OGF_LINK_CTL) {
        if (hci_cmd[CMD_OCF_OFFSET] == OCF_INQUIRY) {
            ALOGV("INQUIRY %x", hci_cmd[2]);
        } else if (hci_cmd[CMD_OCF_OFFSET] == OCF_INQUIRY_CANCEL) {
            ALOGV("INQUIRY CAN %x", hci_cmd[2]);
        } else if (hci_cmd[CMD_OCF_OFFSET] == OCF_PERIODIC_INQUIRY) {
            ALOGV("PERI INQUIRY %x", hci_cmd[2]);
        } else if (hci_cmd[CMD_OCF_OFFSET] == OCF_OCF_CREATE_CONN) {
            ALOGV("CREATE CONN %x", hci_cmd[2]);
        } else {
            return;
        }
    } else if (hci_cmd[CMD_OGF_OFFSET] == OGF_HOST_CTL) {
        if (hci_cmd[CMD_OCF_OFFSET] == OCF_RESET) {
            ALOGV("RESET %x", hci_cmd[2]);
        } else {
            return;
        }
    } else {
        return;
    }

    if (hci_event_sock > 0) {
        status = write(hci_event_sock, hci_cmd, len);
    }
}

/* =========================================================================
FUNCTION     : btc_cmd_evt_handler

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
void btc_cmd_evt_handler(HC_BT_HDR *p_buf)
{
    uint8_t *p = (uint8_t *)(p_buf + 1) + p_buf->offset;
    uint8_t tmp = 0;
    if(hci_event_sock <= 0) {
        ALOGV("%s: HCI RAW DATA  not processing %d", __func__,  hci_event_sock);
        return;
    }

    ALOGV("%s: HCI RAW DATA ", __func__);

    /* borrow one byte for H4 packet type indicator */
    p--;
    tmp = *p;
    switch (p_buf->event & BTC_EVT_MASK) {
    case BTC_HC_TO_STACK_HCI_EVT:
        *p = HCIT_TYPE_EVENT;
         btc_process_event((char *)p, p_buf->len+1);
         break;
    case BTC_STACK_TO_HC_HCI_CMD:
        *p = HCIT_TYPE_COMMAND;
         btc_process_stack_command((char *)p, p_buf->len+1);
         break;
    default:
         ALOGV("btc_cmd_evt_handler:Default case");
    }

    *p = tmp;
    p++;
}

/* =========================================================================
FUNCTION     : btc_thread

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
static void btc_thread(void *param)
{
    int  fd = 0, status = 0;
    int  evtlen = BTC_MAX_LEN;
    char event[BTC_MAX_LEN] = {0};
    hci_event_sock = -1;
    ALOGV("%s: btc_thread ", __func__);
    fd = btc_accept("btc_hci");
    if (fd > 0) {
        ALOGV("btc_accept returns %d", fd);
        hci_event_sock = fd;
    } else {
        ALOGE("btc_accept failed returns %d", fd);
        return;
    }
    do {
        memset(event, 0, BTC_MAX_LEN);
        status = read(hci_event_sock, &event, evtlen);
        if (status > 0) {
            ALOGV("Success: reading from HCI socket %d", status);
            btc_process_client_command(event, status);
        } else  {
            break;
        }
    } while (1);

    if (hci_event_sock >= 0) {
        ALOGV("btc_thread  closing hci_event_sock%d", hci_event_sock);
        close(hci_event_sock);
        hci_event_sock=-1;
    }
}

/* =========================================================================
FUNCTION     : btc_accept

DESCRIPTION  :

DEPENDENCIES : None

RETURN VALUE : None

============================================================================*/
static int btc_accept(char *name)
{
    int fd = -1;
    struct sockaddr_in  client_address;
    socklen_t clen;

    ALOGV("%s: ACCEPT ", __func__);
    sock_id = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_id < 0) {
        ALOGE("Socket creation failure");
        return fd;
    }

    ALOGV("covert name to android abstract name:%s %d", name, sock_id);
    if (socket_local_server_bind(sock_id,
        name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0) {
        if (listen(sock_id, 5) == 0) {
            ALOGV("BTC listen to local socket:%s, fd:%d", name, sock_id);
        } else {
            ALOGE("BTC listen to local socket:failed");
            return fd;
        }
    } else {
        close(sock_id);
        sock_id=-1;
        ALOGE("BTC sock ser bind failed");
        return fd;
    }

    clen = sizeof(client_address);
    ALOGV("before accept_server_socket");
    fd = accept(sock_id, (struct sockaddr *)&client_address, &clen);
    if (fd > 0) {
        ALOGV("BTC accepted fd:%d for server fd:%d", fd, sock_id);
        return fd;
    } else {
        ALOGE("BTC accept failed fd:%d sock d:%d error %s", fd, sock_id, strerror(errno));
        return fd;
    }
    return fd;
}

int btc_close_serv_socket(const char* name)
{
    int temp_sock;
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    ALOGV("%s: CLOSE SERVER SOCKET", __func__);

    temp_sock=socket_local_client_connect(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if(temp_sock) {
        ALOGV("connected to local socket:%s, s:  %d  temp_sock: %d", name, s,temp_sock);
        close(temp_sock);
        close(s);
        return TRUE;
    }
    else {
        ALOGE("connect to local socket:%s, fd:%d failed, errno:%d", name, s, errno);
    }

    close(s);
    return -1;
}

