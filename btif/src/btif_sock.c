/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
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
 *  Filename:      btif_sock.c
 *
 *  Description:   Bluetooth Socket Interface
 *
 *
 ***********************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>

#define LOG_TAG "BTIF_SOCK"
#include "btif_common.h"
#include "btif_util.h"

#include "bd.h"

#include "bta_api.h"
#include "btif_sock_thread.h"
#include "btif_sock_rfc.h"
#include <cutils/log.h>
#define info(fmt, ...)  ALOGI ("btif_sock: %s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define debug(fmt, ...) ALOGD ("btif_sock: %s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define error(fmt, ...) ALOGE ("btif_sock: ## ERROR : %s: " fmt "##",__FUNCTION__,  ## __VA_ARGS__)
#define asrt(s) if(!(s)) ALOGE ("btif_sock: ## %s assert %s failed at line:%d ##",__FUNCTION__, #s, __LINE__)

static bt_status_t btsock_listen(btsock_type_t type, const char* service_name,
                                const uint8_t* uuid, int channel, int* sock_fd, int flags);
static bt_status_t btsock_connect(const bt_bdaddr_t *bd_addr, btsock_type_t type,
                                  const uint8_t* uuid, int channel, int* sock_fd, int flags);

static void btsock_signaled(int fd, int type, int flags, uint32_t user_id);

/*******************************************************************************
**
** Function         btsock_ini
**
** Description     initializes the bt socket interface
**
** Returns         bt_status_t
**
*******************************************************************************/
static btsock_interface_t sock_if = {
                sizeof(sock_if),
                btsock_listen,
                btsock_connect
       };
btsock_interface_t *btif_sock_get_interface()
{
    return &sock_if;
}
bt_status_t btif_sock_init()
{
    debug("");


    static volatile int binit;
    if(!binit)
    {
        //fix me, the process doesn't exit right now. don't set the init flag for now
        //binit = 1;
        debug("btsock initializing...");
        btsock_thread_init();
        int handle = btsock_thread_create(btsock_signaled, NULL);
        if(handle >= 0 && btsock_rfc_init(handle) == BT_STATUS_SUCCESS)
        {
            debug("btsock successfully initialized");
            return BT_STATUS_SUCCESS;
        }
    }
    else error("btsock interface already initialized");
    return BT_STATUS_FAIL;
}
void btif_sock_cleanup()
{
    debug("");
    btsock_rfc_cleanup();
    debug("leaving");
}

static bt_status_t btsock_listen(btsock_type_t type, const char* service_name,
        const uint8_t* service_uuid, int channel, int* sock_fd, int flags)
{
    if((service_uuid == NULL && channel <= 0) || sock_fd == NULL)
    {
        error("invalid parameters, uuid:%p, channel:%d, sock_fd:%p", service_uuid, channel, sock_fd);
        return BT_STATUS_PARM_INVALID;
    }
    *sock_fd = -1;
    bt_status_t status = BT_STATUS_FAIL;
    switch(type)
    {
        case BTSOCK_RFCOMM:
            status = btsock_rfc_listen(service_name, service_uuid, channel, sock_fd, flags);
            break;
        case BTSOCK_L2CAP:
            error("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        case BTSOCK_SCO:
            error("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            error("unknown bt socket type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
    }
    return status;
}
static bt_status_t btsock_connect(const bt_bdaddr_t *bd_addr, btsock_type_t type,
        const uint8_t* uuid, int channel, int* sock_fd, int flags)
{
    if((uuid == NULL && channel <= 0) || bd_addr == NULL || sock_fd == NULL)
    {
        error("invalid parameters, bd_addr:%p, uuid:%p, channel:%d, sock_fd:%p",
                bd_addr, uuid, channel, sock_fd);
        return BT_STATUS_PARM_INVALID;
    }
    *sock_fd = -1;
    bt_status_t status = BT_STATUS_FAIL;
    switch(type)
    {
        case BTSOCK_RFCOMM:
            status = btsock_rfc_connect(bd_addr, uuid, channel, sock_fd, flags);
            break;
        case BTSOCK_L2CAP:
            error("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        case BTSOCK_SCO:
            error("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            error("unknown bt socket type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
    }
    return status;
}
static void btsock_signaled(int fd, int type, int flags, uint32_t user_id)
{
    switch(type)
    {
        case BTSOCK_RFCOMM:
            btsock_rfc_signaled(fd, flags, user_id);
            break;
        case BTSOCK_L2CAP:
            error("bt l2cap socket type not supported, fd:%d, flags:%d", fd, flags);
            break;
        case BTSOCK_SCO:
            error("bt sco socket type not supported, fd:%d, flags:%d", fd, flags);
            break;
        default:
            error("unknown socket type:%d, fd:%d, flags:%d", type, fd, flags);
            break;
    }
}



