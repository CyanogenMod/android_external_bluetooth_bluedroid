/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributols may be used to endolse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/******************************************************************************
 *
 *   This file contains L2cap Socket interface functions to the  BT JNI layer
 *
 ******************************************************************************/
#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/ioctl.h>

#define LOG_TAG "BTIF_SOCK"
#include "btif_common.h"
#include "btif_util.h"

#include "bd.h"

#include "bta_api.h"
#include "btif_sock_thread.h"
#include "btif_sock_sdp.h"
#include "btif_sock_util.h"

#include "bt_target.h"
#include "gki.h"
#include "hcimsgs.h"
#include "sdp_api.h"
#include "btu.h"
#include "btm_api.h"
#include "btm_int.h"
#include "bta_jv_api.h"
#include "bta_jv_co.h"
#include "port_api.h"
#include "btif_sock.h"

#include <cutils/log.h>
#include <hardware/bluetooth.h>
#define asrt(s) if(!(s)) APPL_TRACE_ERROR("## %s assert %s failed at line:%d ##",__FUNCTION__, #s, __LINE__)

extern void uuid_to_string(bt_uuid_t *p_uuid, char *str);
static inline void logu(const char* title, const uint8_t * p_uuid)
{
    char uuids[128];
    uuid_to_string((bt_uuid_t*)p_uuid, uuids);
    ALOGD("%s: %s", title, uuids);
}

#define MAX_L2C_SESSION BTA_JV_MAX_L2C_SR_SESSION //3 by default
#define INVALID_L2C_HANDLE -1
typedef struct {
    int outgoing_congest : 1;
    int pending_sdp_request : 1;
    int doing_sdp_request : 1;
    int server : 1;
    int client : 1;
    int connected : 1;
    int closing : 1;
} flags_t;

typedef struct {
  flags_t f;
  uint32_t id;
  BOOLEAN in_use;
  int security;
  int psm;
  bt_bdaddr_t addr;
  uint8_t service_uuid[16];
  char service_name[256];
  int fd, app_fd;
  int put_size;
  int put_size_set;
  uint8_t* packet;
  int sdp_handle;
  int l2c_handle;
  int role;
  BUFFER_Q incoming_que;
} l2c_slot_t;

static l2c_slot_t l2c_slots[MAX_L2C_SOCK_CHANNEL];
static volatile int pth = -1; //poll thread handle
static void jv_dm_cback(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data);
static void cleanup_l2c_slot(l2c_slot_t* ls);
static inline void close_l2c_connection(int l2c_handle, int server);
static bt_status_t dm_get_remote_service_record(bt_bdaddr_t *remote_addr,
                                                    bt_uuid_t *uuid);
static void *l2cap_cback(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data);
static inline BOOLEAN send_app_psm(l2c_slot_t* ls);
static pthread_mutex_t slot_lock;
#define is_init_done() (pth != -1)
static inline void clear_slot_flag(flags_t* f)
{
    memset(f, 0, sizeof(*f));
}

static inline void bd_copy(UINT8* dest, UINT8* src, BOOLEAN swap)
{
    if (swap)
    {
        int i;
        for (i =0; i < 6 ;i++)
            dest[i]= src[5-i];
    }
    else memcpy(dest, src, 6);
}
static inline void free_gki_que(BUFFER_Q* q)
{
    while(!GKI_queue_is_empty(q))
           GKI_freebuf(GKI_dequeue(q));
}
static void init_l2c_slots()
{
    int i;
    memset(l2c_slots, 0, sizeof(l2c_slot_t)*MAX_L2C_SOCK_CHANNEL);
    for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
    {
        l2c_slots[i].psm = -1;
        l2c_slots[i].f.client = FALSE;
        l2c_slots[i].put_size_set = FALSE;
        l2c_slots[i].sdp_handle = 0;
        l2c_slots[i].l2c_handle = INVALID_L2C_HANDLE;
        l2c_slots[i].fd = l2c_slots[i].app_fd = -1;
        l2c_slots[i].id = BASE_L2C_SLOT_ID + i;
        GKI_init_q(&l2c_slots[i].incoming_que);
    }
    BTA_JvRegisterL2cCback(jv_dm_cback);
    init_slot_lock(&slot_lock);
}
bt_status_t btsock_l2c_init(int poll_thread_handle)
{
    pth = poll_thread_handle;
    init_l2c_slots();
    return BT_STATUS_SUCCESS;
}
void btsock_l2c_cleanup()
{
    int curr_pth = pth;
    pth = -1;
    btsock_thread_exit(curr_pth);
    lock_slot(&slot_lock);
    int i;
    for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
    {
        if(l2c_slots[i].in_use)
            cleanup_l2c_slot(&l2c_slots[i]);
    }
    unlock_slot(&slot_lock);
}
static inline l2c_slot_t* find_free_slot()
{
    int i;
    for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
    {
        if(l2c_slots[i].fd == -1)
        {
             return &l2c_slots[i];
        }
    }
    return NULL;
}
static inline l2c_slot_t* find_l2c_slot_by_id(uint32_t id)
{
    int i;
    if(id)
    {
        for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
        {
            if(l2c_slots[i].in_use && (l2c_slots[i].id == id))
            {
                return &l2c_slots[i];
            }
        }
    }
    APPL_TRACE_WARNING("invalid l2c slot id: %d", id);
    return NULL;
}
static inline l2c_slot_t* find_l2c_slot_by_pending_sdp()
{
    uint32_t min_id = (uint32_t)-1;
    int slot = -1;
    int i;
    for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
    {
        if(l2c_slots[i].in_use && l2c_slots[i].f.pending_sdp_request)
        {
            if(l2c_slots[i].id < min_id)
            {
                min_id = l2c_slots[i].id;
                slot = i;
            }
        }
    }
    if(0<= slot && slot < MAX_L2C_SOCK_CHANNEL)
        return &l2c_slots[slot];
    return NULL;
}
static inline l2c_slot_t* find_l2c_slot_requesting_sdp()
{
    int i;
    for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
    {
        if(l2c_slots[i].in_use && l2c_slots[i].f.doing_sdp_request)
                return &l2c_slots[i];
    }
    APPL_TRACE_DEBUG("can not find any slot is requesting sdp");
    return NULL;
}

static inline l2c_slot_t* find_l2c_slot_by_fd(int fd)
{
    int i;
    if(fd >= 0)
    {
        for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
        {
            if(l2c_slots[i].fd == fd)
            {
                if(l2c_slots[i].in_use)
                    return &l2c_slots[i];
                else
                {
                    APPL_TRACE_ERROR("invalid l2c slot id, cannot be 0");
                    break;
                }
            }
        }
    }
    return NULL;
}


static l2c_slot_t* alloc_l2c_slot(const bt_bdaddr_t *addr, const char* name, const uint8_t* uuid, int channel, int flags, BOOLEAN server)
{
    int security = 0;
    if(flags & BTSOCK_FLAG_ENCRYPT)
        security |= server ? BTM_SEC_IN_ENCRYPT : BTM_SEC_OUT_ENCRYPT;
    if(flags & BTSOCK_FLAG_AUTH) {
        security |= server ? BTM_SEC_IN_AUTHENTICATE : BTM_SEC_OUT_AUTHENTICATE;
    }
    l2c_slot_t* ls = find_free_slot();
    if(ls)
    {
        int fds[2] = {-1, -1};
        if(socketpair(AF_LOCAL, SOCK_STREAM, 0, fds))
        {
            APPL_TRACE_ERROR("socketpair failed, errno:%d", errno);
            return NULL;
        }
        ls->fd = fds[0];
        ls->app_fd = fds[1];

        APPL_TRACE_DEBUG("alloc_l2c_slot fd %d and app fd  %d is_server %d", ls->fd, ls->app_fd, server);
        ls->security = security;
        ls->psm = channel;
        if(uuid)
            memcpy(ls->service_uuid, uuid, sizeof(ls->service_uuid));
        else memset(ls->service_uuid, 0, sizeof(ls->service_uuid));
        if(name && *name)
            strlcpy(ls->service_name, name, sizeof(ls->service_name) -1);
        if(addr)
            ls->addr = *addr;
        ls->in_use = TRUE;
        ls->f.server = server;
    }
    return ls;
}
// l2c_slot_t* accept_ls = create_srv_accept_l2c_slot(srv_ls, p_open->rem_bda,p_opne->handle,  p_open->new_listen_handle);
static inline l2c_slot_t* create_srv_accept_l2c_slot(l2c_slot_t* srv_ls, const bt_bdaddr_t* addr,
                                        int open_handle, int new_listen_handle)
{
    l2c_slot_t *accept_ls = alloc_l2c_slot(addr, srv_ls->service_name, srv_ls->service_uuid, srv_ls->psm, 0, FALSE);
    if( accept_ls)
    {
        clear_slot_flag(&accept_ls->f);
        accept_ls->f.server = FALSE;
        accept_ls->f.connected = TRUE;
        accept_ls->security = srv_ls->security;
        accept_ls->put_size = srv_ls->put_size;
        accept_ls->role = srv_ls->role;
        accept_ls->l2c_handle = open_handle;
        //now update listen l2c_handle of server slot
        srv_ls->l2c_handle = new_listen_handle;
        //now swap the slot id
        uint32_t new_listen_id = accept_ls->id;
        accept_ls->id = srv_ls->id;
        srv_ls->id = new_listen_id;

        return accept_ls;
    }
    else
    {
        APPL_TRACE_ERROR(" accept_ls is NULL %s", __FUNCTION__);
        return NULL;
    }
}
bt_status_t btsock_l2c_listen(const char* service_name, const uint8_t* service_uuid, int channel,
                            int* sock_fd, int flags)
{
    int status = BT_STATUS_FAIL;

    APPL_TRACE_DEBUG("btsock_l2c_listen, service_name:%s", service_name);

    /* TODO find the available psm list for obex */
    if(sock_fd == NULL || (service_uuid == NULL))
    {
        APPL_TRACE_ERROR("invalid sock_fd:%p, uuid:%p", sock_fd, service_uuid);
        return BT_STATUS_PARM_INVALID;
    }
    *sock_fd = -1;
    if(!is_init_done())
        return BT_STATUS_NOT_READY;

    /* validate it for FTP and OPP */
    //Check the service_uuid. overwrite the channel # if reserved
    int reserved_channel = get_reserved_l2c_channel(service_uuid);
    if(reserved_channel > 0)
    {
        channel = reserved_channel;
    }
    else
    {
        return BT_STATUS_FAIL;
    }

    lock_slot(&slot_lock);
    l2c_slot_t* ls = alloc_l2c_slot(NULL, service_name, service_uuid, channel, flags, TRUE);
    if(ls)
    {
        APPL_TRACE_DEBUG("BTA_JvCreateRecordByUser:%s", service_name);
        BTA_JvCreateRecordByUser((void *)(ls->id));
        APPL_TRACE_DEBUG("BTA_JvCreateRecordByUser userdata :%d", service_name);
        *sock_fd = ls->app_fd;
        ls->app_fd = -1; //the fd ownelship is transferred to app
        status = BT_STATUS_SUCCESS;
        btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_EXCEPTION, ls->id);
    }
    unlock_slot(&slot_lock);
    return status;
}


bt_status_t btsock_l2c_connect(const bt_bdaddr_t *bd_addr, const uint8_t* service_uuid,
        int channel, int* sock_fd, int flags)
{
    if(sock_fd == NULL || (service_uuid == NULL))
    {
        APPL_TRACE_ERROR("invalid sock_fd:%p, uuid:%p", sock_fd, service_uuid);
        return BT_STATUS_PARM_INVALID;
    }
    *sock_fd = -1;
    if(!is_init_done())
        return BT_STATUS_NOT_READY;
    int status = BT_STATUS_FAIL;
    lock_slot(&slot_lock);
    l2c_slot_t* ls = alloc_l2c_slot(bd_addr, NULL, service_uuid, channel, flags, FALSE);
    if(ls)
    {
        ls->f.client = TRUE;
        if(is_uuid_empty(service_uuid))
        {
            APPL_TRACE_DEBUG("connecting to l2cap channel:%d without service discovery", channel);
            if(BTA_JvL2capConnect(ls->security, ls->role, ls->psm, 672, ls->addr.address,
                        l2cap_cback, (void*)ls->id) == BTA_JV_SUCCESS)
            {
                if(send_app_psm(ls))
                {
                    btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP,
                                                        SOCK_THREAD_FD_RD, ls->id);
                    *sock_fd = ls->app_fd;
                    ls->app_fd = -1; //the fd ownelship is transferred to app
                    status = BT_STATUS_SUCCESS;
                }
                else cleanup_l2c_slot(ls);
            }
            else cleanup_l2c_slot(ls);
        }
        else
        {
            tSDP_UUID sdp_uuid;
            sdp_uuid.len = 16;
            memcpy(sdp_uuid.uu.uuid128, service_uuid, sizeof(sdp_uuid.uu.uuid128));
            logu("service_uuid", service_uuid);
            *sock_fd = ls->app_fd;
            ls->app_fd = -1; //the fd ownelship is transferred to app
            status = BT_STATUS_SUCCESS;
            l2c_slot_t* ls_doing_sdp = find_l2c_slot_requesting_sdp();
            if(ls_doing_sdp == NULL)
            {
                BTA_JvStartDiscovery((UINT8*)bd_addr->address, 1, &sdp_uuid, (void*)(ls->id));
                ls->f.pending_sdp_request = FALSE;
                ls->f.doing_sdp_request = TRUE;
            }
            else
            {
                ls->f.pending_sdp_request = TRUE;
                ls->f.doing_sdp_request = FALSE;
            }
            btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_RD, ls->id);
        }
    }
    unlock_slot(&slot_lock);
    return status;
}

static int create_server_sdp_record(l2c_slot_t* ls)
{
    int psm = ls->psm;
    if((ls->sdp_handle = add_l2c_sdp_rec(ls->service_name, ls->service_uuid, ls->psm)) <= 0)
    {
        return FALSE;
    }
    return TRUE;
}
static const char * jv_evt[] = {
    "BTA_JV_ENABLE_EVT",
    "BTA_JV_SET_DISCOVER_EVT",
    "BTA_JV_LOCAL_ADDR_EVT",
    "BTA_JV_LOCAL_NAME_EVT",
    "BTA_JV_REMOTE_NAME_EVT",
    "BTA_JV_SET_ENCRYPTION_EVT",
    "BTA_JV_GET_SCN_EVT",
    "BTA_JV_GET_PSM_EVT",
    "BTA_JV_DISCOVERY_COMP_EVT",
    "BTA_JV_SERVICES_LEN_EVT",
    "BTA_JV_SERVICE_SEL_EVT",
    "BTA_JV_CREATE_RECORD_EVT",
    "BTA_JV_UPDATE_RECORD_EVT",
    "BTA_JV_ADD_ATTR_EVT",
    "BTA_JV_DELETE_ATTR_EVT",
    "BTA_JV_CANCEL_DISCVRY_EVT",

    "BTA_JV_L2CAP_OPEN_EVT",
    "BTA_JV_L2CAP_CLOSE_EVT",
    "BTA_JV_L2CAP_START_EVT",
    "BTA_JV_L2CAP_CL_INIT_EVT",
    "BTA_JV_L2CAP_DATA_IND_EVT",
    "BTA_JV_L2CAP_CONG_EVT",
    "BTA_JV_L2CAP_READ_EVT",
    "BTA_JV_L2CAP_RECEIVE_EVT",
    "BTA_JV_L2CAP_WRITE_EVT",

    "BTA_JV_MAX_EVT"
};
static inline void free_l2c_slot_psm(l2c_slot_t* ls)
{
    if(ls->psm > 0)
    {
        if(ls->f.server && !ls->f.closing && (ls->l2c_handle != INVALID_L2C_HANDLE))
        {
            APPL_TRACE_DEBUG("free_l2c_slot_psm ls->id %d  ls->fd %d", ls->id, ls->fd);
            BTA_JvL2capStopServer(ls->l2c_handle, (void*)ls->id);
            ls->l2c_handle = INVALID_L2C_HANDLE;
        }
        ls->psm = 0;
        ls->put_size_set = FALSE;
    }
}
static void cleanup_l2c_slot(l2c_slot_t* ls)
{
    APPL_TRACE_DEBUG("cleanup slot:%d, fd:%d, psm:%d, sdp_handle:0x%x",
                                        ls->id, ls->fd, ls->psm, ls->sdp_handle);
    if(ls->fd != -1)
    {
        shutdown(ls->fd, 2);
        close(ls->fd);
        ls->fd = -1;
    }

    if(ls->app_fd != -1)
    {
        close(ls->app_fd);
        ls->app_fd = -1;
    }
    if(ls->sdp_handle > 0)
    {
        del_l2c_sdp_rec(ls->sdp_handle, ls->service_uuid);
        ls->sdp_handle = 0;
    }
    if((ls->l2c_handle != INVALID_L2C_HANDLE) && !ls->f.closing && !ls->f.server)
    {
        APPL_TRACE_DEBUG("closing l2cap connection, l2c_handle:0x%x", ls->l2c_handle);
        BTA_JvL2capClose(ls->l2c_handle, (void*)ls->id);
        ls->l2c_handle = INVALID_L2C_HANDLE;
    }
    free_l2c_slot_psm(ls);
    free_gki_que(&ls->incoming_que);

    //cleanup the flag
    memset(&ls->f, 0, sizeof(ls->f));
    ls->in_use = FALSE;
}
static inline BOOLEAN send_app_psm(l2c_slot_t* ls)
{
    if(sock_send_all(ls->fd, (const uint8_t*)&ls->psm, sizeof(ls->psm)) == sizeof(ls->psm))
    {
        return TRUE;
    }

    return FALSE;
}
static BOOLEAN send_app_connect_signal(int fd, const bt_bdaddr_t* addr, int channel, int status, int send_fd)
{
/*
    typedef struct {
    short size;
    bt_bdaddr_t bd_addr;
    int channel;
    int status;
} __attribute__((packed)) sock_connect_signal_t;
*/
    sock_connect_signal_t cs;
    cs.size = sizeof(cs);
    cs.bd_addr = *addr;
    cs.channel = channel;
    cs.status = status;
    if(send_fd != -1)
    {
        if(sock_send_fd(fd, (const uint8_t*)&cs, sizeof(cs), send_fd) == sizeof(cs))
            return TRUE;
        else APPL_TRACE_ERROR("sock_send_fd failed, fd:%d, send_fd:%d", fd, send_fd);
    }
    else if(sock_send_all(fd, (const uint8_t*)&cs, sizeof(cs)) == sizeof(cs))
    {
        return TRUE;
    }
    return FALSE;
}
static void on_cl_l2c_init(tBTA_JV_L2CAP_CL_INIT *p_init, uint32_t id)
{
   lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        if (p_init->status != BTA_JV_SUCCESS)
            cleanup_l2c_slot(ls);
        else
        {
            ls->l2c_handle = p_init->handle;
            APPL_TRACE_DEBUG("on_cl_l2c_init ls->l2c_handle %d", ls->l2c_handle);
        }
    }
    unlock_slot(&slot_lock);
}
static void  on_srv_l2c_listen_started(tBTA_JV_L2CAP_START *p_start, uint32_t id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        if (p_start->status != BTA_JV_SUCCESS)
            cleanup_l2c_slot(ls);
        else
        {
            ls->l2c_handle = p_start->handle;
            APPL_TRACE_DEBUG("on_srv_l2c_listen_started ls->l2c_handle %d", ls->l2c_handle);

            if(!send_app_psm(ls))
            {
                //closed
                APPL_TRACE_DEBUG("send_app_psm() failed, close ls->id:%d", ls->id);
                cleanup_l2c_slot(ls);
            }
        }
    }
    unlock_slot(&slot_lock);
}
static uint32_t on_srv_l2c_connect(tBTA_JV_L2CAP_SRV_OPEN *p_open, uint32_t id)
{
    uint32_t new_listen_slot_id = 0;
    lock_slot(&slot_lock);
    l2c_slot_t* srv_ls = find_l2c_slot_by_id(id);
    if(srv_ls)
    {
        l2c_slot_t* accept_ls = create_srv_accept_l2c_slot(srv_ls, (const bt_bdaddr_t*)p_open->rem_bda,
                                                           p_open->handle, p_open->new_listen_handle);
        if(accept_ls)
        {
            //start monitor the socket
            btsock_thread_add_fd(pth, srv_ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_EXCEPTION, srv_ls->id);
            btsock_thread_add_fd(pth, accept_ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_RD, accept_ls->id);
            APPL_TRACE_DEBUG("sending connect signal & app fd:%dto app server to accept() the connection",
                             accept_ls->app_fd);
            APPL_TRACE_DEBUG("server fd:%d, psm:%d", srv_ls->fd, srv_ls->psm);
            send_app_connect_signal(srv_ls->fd, &accept_ls->addr, srv_ls->psm, 0, accept_ls->app_fd);
            accept_ls->app_fd = -1; //the fd is closed after sent to app
            new_listen_slot_id = srv_ls->id;
        }
    }
    unlock_slot(&slot_lock);
    return new_listen_slot_id;
}
static void on_cli_l2c_connect(tBTA_JV_L2CAP_OPEN *p_open, uint32_t id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls && p_open->status == BTA_JV_SUCCESS)
    {
        bd_copy(ls->addr.address, p_open->rem_bda, 0);
        //notify app l2c is connected
        APPL_TRACE_DEBUG("call send_app_connect_signal, slot id:%d, fd:%d, l2c psm:%d, server:%d",
                         ls->id, ls->fd, ls->psm, ls->f.server);
        if(send_app_connect_signal(ls->fd, &ls->addr, ls->psm, 0, -1))
        {
            //start monitoring the socketpair to get call back when app writing data
            APPL_TRACE_DEBUG("on_l2c_connect_ind, connect signal sent, slot id:%d, l2c psm:%d, server:%d",
                             ls->id, ls->psm, ls->f.server);
            ls->f.connected = TRUE;
        }
        else APPL_TRACE_ERROR("send_app_connect_signal failed");
    }
    else if(ls)
        cleanup_l2c_slot(ls);
    unlock_slot(&slot_lock);
}
static void on_l2c_close(tBTA_JV_L2CAP_CLOSE * p_close, uint32_t id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        APPL_TRACE_DEBUG("on_l2c_close, slot id:%d, fd:%d, l2c psm:%d, server:%d",
                         ls->id, ls->fd, ls->psm, ls->f.server);
        free_l2c_slot_psm(ls);
        // l2c_handle already closed when receiving l2cap close event from stack.
        ls->f.connected = FALSE;
        cleanup_l2c_slot(ls);
    }
    unlock_slot(&slot_lock);
}
static void on_l2c_write_done(tBTA_JV_L2CAP_WRITE *p, uint32_t id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls && !ls->f.outgoing_congest)
    {
        //mointer the fd for any outgoing data
        btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_RD, ls->id);
    }
    unlock_slot(&slot_lock);
}
static void on_l2c_outgoing_congest(tBTA_JV_L2CAP_CONG *p, uint32_t id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        ls->f.outgoing_congest = p->cong ? 1 : 0;
        //mointer the fd for any outgoing data
        if(!ls->f.outgoing_congest)
            btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_RD, ls->id);
    }
    unlock_slot(&slot_lock);
}

static void *l2cap_cback(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data)
{
    int rc;
    void* new_user_data = NULL;
    APPL_TRACE_DEBUG("event=%s", jv_evt[event]);

    switch (event)
    {
    case BTA_JV_L2CAP_START_EVT:
        on_srv_l2c_listen_started(&p_data->l2c_start, (uint32_t)user_data);
        break;

    case BTA_JV_L2CAP_CL_INIT_EVT:
        on_cl_l2c_init(&p_data->l2c_cl_init, (uint32_t)user_data);
        break;

    case BTA_JV_L2CAP_OPEN_EVT:
        BTA_JvSetPmProfile(p_data->l2c_open.handle,BTA_JV_PM_ID_1,BTA_JV_CONN_OPEN);
        on_cli_l2c_connect(&p_data->l2c_open, (uint32_t)user_data);
        break;
    case BTA_JV_L2CAP_SRV_OPEN_EVT:
        BTA_JvSetPmProfile(p_data->l2c_srv_open.handle,BTA_JV_PM_ALL,BTA_JV_CONN_OPEN);
        new_user_data = (void*)on_srv_l2c_connect(&p_data->l2c_srv_open, (uint32_t)user_data);
        break;

    case BTA_JV_L2CAP_CLOSE_EVT:
        APPL_TRACE_DEBUG("BTA_JV_L2CAP_CLOSE_EVT: user_data:%d", (uint32_t)user_data);
        on_l2c_close(&p_data->l2c_close, (uint32_t)user_data);
        break;

    case BTA_JV_L2CAP_READ_EVT:
        APPL_TRACE_DEBUG("BTA_JV_L2CAP_READ_EVT not used");
        break;

    case BTA_JV_L2CAP_WRITE_EVT:
        on_l2c_write_done(&p_data->l2c_write, (uint32_t)user_data);
        break;

    case BTA_JV_L2CAP_DATA_IND_EVT:
        APPL_TRACE_DEBUG("BTA_JV_L2CAP_DATA_IND_EVT not used");
        break;

    case BTA_JV_L2CAP_CONG_EVT:
        //on_l2c_cong(&p_data->l2c_cong);
        on_l2c_outgoing_congest(&p_data->l2c_cong, (uint32_t)user_data);
        break;
    default:
        APPL_TRACE_ERROR("unhandled event %d, slot id:%d", event, (uint32_t)user_data);
        break;
    }
    return new_user_data;
}

static void jv_dm_cback(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data)
{
    uint32_t id = (uint32_t)user_data;
    APPL_TRACE_DEBUG("jv_dm_cback: event:%d, slot id:%d", event, id);
    switch(event)
    {
        case BTA_JV_CREATE_RECORD_EVT:
            {
                lock_slot(&slot_lock);
                l2c_slot_t* ls = find_l2c_slot_by_id(id);
                if(ls && create_server_sdp_record(ls))
                {
                    //now start the l2cap server after sdp & channel # assigned
                    BTA_JvL2capStartServer(ls->security, ls->role, ls->psm, 672 , l2cap_cback,
                                            (void*)ls->id);
                }
                else if(ls)
                {
                    APPL_TRACE_ERROR("jv_dm_cback: cannot start server, slot found:%p", ls);
                    cleanup_l2c_slot(ls);
                }
                unlock_slot(&slot_lock);
                break;
            }
        case BTA_JV_DISCOVERY_COMP_EVT:
            {
                l2c_slot_t* ls = NULL;
                lock_slot(&slot_lock);
                if(p_data->disc_comp.status == BTA_JV_SUCCESS && p_data->disc_comp.psm)
                {
                    APPL_TRACE_DEBUG("BTA_JV_DISCOVERY_COMP_EVT, slot id:%d, status:%d, psm:%d",
                                      id, p_data->disc_comp.status, p_data->disc_comp.psm);

                    ls = find_l2c_slot_by_id(id);
                    if(ls && ls->f.doing_sdp_request)
                    {
                        if(BTA_JvL2capConnect(ls->security, ls->role, p_data->disc_comp.psm, 672, ls->addr.address,
                                    l2cap_cback, (void*)ls->id) == BTA_JV_SUCCESS)
                        {
                            ls->psm = p_data->disc_comp.psm;
                            ls->f.doing_sdp_request = FALSE;
                            if(!send_app_psm(ls))
                                cleanup_l2c_slot(ls);
                        }
                        else cleanup_l2c_slot(ls);
                    }
                    else if(ls)
                    {
                        APPL_TRACE_ERROR("DISCOVERY_COMP_EVT no pending sdp request, slot id:%d, \
                                flag sdp pending:%d, flag sdp doing:%d",
                                id, ls->f.pending_sdp_request, ls->f.doing_sdp_request);
                    }
                }
                else
                {
                    APPL_TRACE_ERROR("DISCOVERY_COMP_EVT slot id:%d, failed to find channle, \
                                      status:%d, psm:%d", id, p_data->disc_comp.status,
                                      p_data->disc_comp.psm);
                    ls = find_l2c_slot_by_id(id);
                    if(ls)
                        cleanup_l2c_slot(ls);
                }
                ls = find_l2c_slot_by_pending_sdp();
                if(ls)
                {
                    APPL_TRACE_DEBUG("BTA_JV_DISCOVERY_COMP_EVT, start another pending psm sdp request");
                    tSDP_UUID sdp_uuid;
                    sdp_uuid.len = 16;
                    memcpy(sdp_uuid.uu.uuid128, ls->service_uuid, sizeof(sdp_uuid.uu.uuid128));
                    BTA_JvStartDiscovery((UINT8*)ls->addr.address, 1, &sdp_uuid, (void*)(ls->id));
                    ls->f.pending_sdp_request = FALSE;
                    ls->f.doing_sdp_request = TRUE;
                }
                unlock_slot(&slot_lock);
                break;
            }
        default:
            APPL_TRACE_DEBUG("unhandled event:%d, slot id:%d", event, id);
            break;
    }

}
#define SENT_ALL 2
#define SENT_PARTIAL 1
#define SENT_NONE 0
#define SENT_FAILED (-1)
static int send_data_to_app(int fd, BT_HDR *p_buf)
{
    if(p_buf->len == 0)
        return SENT_ALL;
    int sent = send(fd, (UINT8 *)(p_buf + 1) + p_buf->offset,  p_buf->len, MSG_DONTWAIT);
    if(sent == p_buf->len)
        return SENT_ALL;

    if(sent > 0 && sent < p_buf->len)
    {
        //sent partial
        APPL_TRACE_ERROR("send partial, sent:%d, p_buf->len:%d", sent, p_buf->len);
        p_buf->offset += sent;
        p_buf->len -= sent;
        return SENT_PARTIAL;

    }
    if(sent < 0 &&
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
    {
        APPL_TRACE_ERROR("send none, EAGAIN or EWOULDBLOCK, errno:%d", errno);
        return SENT_NONE;
    }
    APPL_TRACE_ERROR("unknown send() error, sent:%d, p_buf->len:%d,  errno:%d", sent, p_buf->len, errno);
    return SENT_FAILED;
}
static BOOLEAN flush_incoming_que_on_wr_signal(l2c_slot_t* ls)
{
    while(!GKI_queue_is_empty(&ls->incoming_que))
    {
        BT_HDR *p_buf = GKI_dequeue(&ls->incoming_que);
        if (!p_buf)
        {
            APPL_TRACE_ERROR("GKI_dequeue() returned queue as empty");
            return FALSE;
        }
        int sent = send_data_to_app(ls->fd, p_buf);
        switch(sent)
        {
            case SENT_NONE:
            case SENT_PARTIAL:
                //add it back to the queue at same position
                GKI_enqueue_head (&ls->incoming_que, p_buf);
                //monitor the fd to get callback when app is ready to receive data
                btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_WR, ls->id);
                return TRUE;
            case SENT_ALL:
                GKI_freebuf(p_buf);
                break;
            case SENT_FAILED:
                GKI_freebuf(p_buf);
                return FALSE;
        }
    }

    //app is ready to receive data, tell stack to start the data flow
    //fix me: need a jv flow control api to serialize the call in stack
    return TRUE;
}
void btsock_l2c_signaled(int fd, int flags, uint32_t user_id)
{
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(user_id);
    if(ls)
    {
        APPL_TRACE_DEBUG("l2c slot id:%d, fd:%d, flags:%x", ls->id, fd, flags);
        BOOLEAN need_close = FALSE;
        if(flags & SOCK_THREAD_FD_RD)
        {
            //data available from app, tell stack we have outgoing data
            if(!ls->f.server)
            {
                if(ls->f.connected)
                {
                    int size = 0;
                    //make sure there's data pending in case the peer closed the socket
                    if((ioctl(ls->fd, FIONREAD, &size) == 0 && size) || !(flags & SOCK_THREAD_FD_EXCEPTION))
                    {
                        if(ls->put_size_set == TRUE)
                        {
                            if(size >= ls->put_size)
                            {
                                APPL_TRACE_DEBUG(" available size matched with ls->put_size");
                                BTA_JvL2capWrite(ls->l2c_handle, (UINT32)ls->id);
                                ls->put_size_set = FALSE;
                            }
                            else
                                btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_RD, ls->id);
                        }
                        else
                        {
                            BTA_JvL2capWrite(ls->l2c_handle, (UINT32)ls->id);
                        }
                    }
                }
                else
                {
                    APPL_TRACE_ERROR("SOCK_THREAD_FD_RD signaled when l2c is not connected, \
                                      slot id:%d, channel:%d", ls->id, ls->psm);
                    need_close = TRUE;
                }
            }
        }
        if(flags & SOCK_THREAD_FD_WR)
        {
            //app is ready to receive more data, tell stack to enable the data flow
            if(!ls->f.connected || !flush_incoming_que_on_wr_signal(ls))
            {
                need_close = TRUE;
                APPL_TRACE_ERROR("SOCK_THREAD_FD_WR signaled when l2c is not connected \
                                  or app closed fd, slot id:%d, channel:%d", ls->id, ls->psm);
            }

        }
        if(need_close || (flags & SOCK_THREAD_FD_EXCEPTION))
        {
            int size = 0;
            if(need_close || ioctl(ls->fd, FIONREAD, &size) != 0 || size == 0 )
            {
                //cleanup when no data pending
                APPL_TRACE_DEBUG("SOCK_THREAD_FD_EXCEPTION, cleanup, flags:%x, need_close:%d, pending size:%d",
                                flags, need_close, size);
                cleanup_l2c_slot(ls);
            }
            else
                APPL_TRACE_DEBUG("SOCK_THREAD_FD_EXCEPTION, cleanup pending, flags:%x, need_close:%d, pending size:%d",
                                flags, need_close, size);
        }
    }
    unlock_slot(&slot_lock);
}

int bta_co_l2c_data_incoming(void *user_data, BT_HDR *p_buf)
{
    uint32_t id = (uint32_t)user_data;
    int ret = 0;
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);


    APPL_TRACE_DEBUG("bta_co_l2c_data_incoming ls:%p id %d size %d", ls, id, p_buf->len);
    if(ls)
    {
        if(!GKI_queue_is_empty(&ls->incoming_que))
            GKI_enqueue(&ls->incoming_que, p_buf);
        else
        {
            int sent = send_data_to_app(ls->fd, p_buf);
            switch(sent)
            {
                case SENT_NONE:
                case SENT_PARTIAL:
                    //add it to the end of the queue
                    GKI_enqueue(&ls->incoming_que, p_buf);
                    //monitor the fd to get callback when app is ready to receive data
                    btsock_thread_add_fd(pth, ls->fd, BTSOCK_L2CAP, SOCK_THREAD_FD_WR, ls->id);
                    break;
                case SENT_ALL:
                    APPL_TRACE_DEBUG("bta_co_l2c_data_incoming sent all data to app");
                    GKI_freebuf(p_buf);
                    ret = 1;//enable the data flow
                    break;
                case SENT_FAILED:
                    GKI_freebuf(p_buf);
                    cleanup_l2c_slot(ls);
                    break;
            }
        }
     }
    unlock_slot(&slot_lock);
    return ret;//return 0 to disable data flow
}
int bta_co_l2c_data_outgoing_size(void *user_data, int *size)
{
    uint32_t id = (uint32_t)user_data;
    int ret = FALSE;
    *size = 0;
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        if(ioctl(ls->fd, FIONREAD, size) == 0)
        {
            APPL_TRACE_DEBUG("ioctl read avaiable size:%d, fd:%d", *size, ls->fd);
            ret = TRUE;
        }
        else
        {
            APPL_TRACE_ERROR("ioctl FIONREAD error, errno:%d, fd:%d", errno, ls->fd);
            cleanup_l2c_slot(ls);
        }
    }
    else APPL_TRACE_ERROR("bta_co_l2c_data_outgoing_size, invalid slot id:%d", id);
    unlock_slot(&slot_lock);
    return ret;
}
int bta_co_l2c_data_outgoing(void *user_data, UINT8* buf, UINT16 size)
{
    uint32_t id = (uint32_t)user_data;
    int ret = FALSE;
    lock_slot(&slot_lock);
    l2c_slot_t* ls = find_l2c_slot_by_id(id);
    if(ls)
    {
        int received = recv(ls->fd, buf, size, 0);
        if(received == size)
        {
            APPL_TRACE_ERROR ("bta_co_l2c_data_outgoing received :%d bytes from fd %d ", size, ls->fd);
            ret = TRUE;
        }
        else
        {
            APPL_TRACE_ERROR("recv error, errno:%d, fd:%d, size:%d, received:%d",
                             errno, ls->fd, size, received);
            cleanup_l2c_slot(ls);
        }
    }
    else APPL_TRACE_ERROR("bta_co_l2c_data_outgoing, invalid slot id:%d", id);
    unlock_slot(&slot_lock);
    return ret;
}

static inline l2c_slot_t* find_client_l2c_slot_by_psm(int psm)
{
    int i;
    if(psm > 0)
    {
        for(i = 0; i < MAX_L2C_SOCK_CHANNEL; i++)
        {
            if((l2c_slots[i].f.client) && (l2c_slots[i].psm == psm))
            {
                if(l2c_slots[i].in_use)
                    return &l2c_slots[i];
            }
        }
    }
    return NULL;
}

bt_status_t btsock_l2c_set_sockopt(int psm, btsock_option_type_t option_name,
                                            void *option_value, int option_len)
{
    int status = BT_STATUS_FAIL;

    APPL_TRACE_ERROR("btsock_l2c_get_sockopt channel is %d ", psm);
    if((psm < 1) || (option_value == NULL) || (option_len <= 0)
                     || (option_len > (int)sizeof(int)))
    {
        APPL_TRACE_ERROR("invalid l2c channel:%d or option_value:%p, option_len:%d",
                                        psm, option_value, option_len);
        return BT_STATUS_PARM_INVALID;
    }
    l2c_slot_t* ls = find_client_l2c_slot_by_psm(psm);
    if((ls) && ((option_name == BTSOCK_OPT_SET_PUT_MTU)))
    {
       if( (*((UINT32 *)option_value) <= 0))
       {
           return BT_STATUS_FAIL;
       }
       ls->put_size = (*((UINT32 *)option_value));
       ls->put_size_set = TRUE;
        APPL_TRACE_ERROR(" channel:%d or option_value:%p, option_len:%d ls->put_size %d",
                                        psm, option_value, option_len, ls->put_size);

        status = BT_STATUS_SUCCESS;
    }


    return status;
}

#endif
