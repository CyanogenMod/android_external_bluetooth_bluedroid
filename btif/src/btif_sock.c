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
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
#include "btif_sock.h"
#include "btif_sock_l2cap.h"
#endif

static bt_status_t btsock_listen(btsock_type_t type, const char* service_name,
                                const uint8_t* uuid, int channel, int* sock_fd, int flags);
static bt_status_t btsock_connect(const bt_bdaddr_t *bd_addr, btsock_type_t type,
                                  const uint8_t* uuid, int channel, int* sock_fd, int flags);
static bt_status_t btsock_get_sockopt(btsock_type_t type, int channel, btsock_option_type_t option_name,
                                            void *option_value, int *option_len);
static bt_status_t btsock_set_sockopt(btsock_type_t type, int channel, btsock_option_type_t option_name,
                                            void *option_value, int option_len);
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
                btsock_connect,
                btsock_get_sockopt,
                btsock_set_sockopt
       };
btsock_interface_t *btif_sock_get_interface()
{
    return &sock_if;
}

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
btsock_type_t bta_co_get_sock_type_by_id(uint32_t slot_id)
{
    if(slot_id >= BASE_RFCOMM_SLOT_ID && slot_id <= MAX_RFCOMM_SLOT_ID)
    {
        return  BTSOCK_RFCOMM;
    }
    else if(slot_id >= BASE_L2C_SLOT_ID && slot_id <= MAX_L2C_SLOT_ID)
    {
        return BTSOCK_L2CAP;
    }
    else
    {
        return 0xFFFF;
    }
}
#endif

bt_status_t btif_sock_init()
{
    static volatile int binit;
    if(!binit)
    {
        //fix me, the process doesn't exit right now. don't set the init flag for now
        //binit = 1;
        BTIF_TRACE_DEBUG("btsock initializing...");
        btsock_thread_init();
        int handle = btsock_thread_create(btsock_signaled, NULL);
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
        if(handle >= 0 && (btsock_rfc_init(handle) == BT_STATUS_SUCCESS) &&
                           (btsock_l2c_init(handle) == BT_STATUS_SUCCESS))
#else
        if(handle >= 0 && btsock_rfc_init(handle) == BT_STATUS_SUCCESS)
#endif
        {
            BTIF_TRACE_DEBUG("btsock successfully initialized");
            return BT_STATUS_SUCCESS;
        }
    }
    else BTIF_TRACE_ERROR("btsock interface already initialized");
    return BT_STATUS_FAIL;
}
void btif_sock_cleanup()
{
    btsock_rfc_cleanup();
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
    btsock_l2c_cleanup();
#endif
    BTIF_TRACE_DEBUG("leaving");
}

static bt_status_t btsock_listen(btsock_type_t type, const char* service_name,
        const uint8_t* service_uuid, int channel, int* sock_fd, int flags)
{
    if((service_uuid == NULL && channel <= 0) || sock_fd == NULL)
    {
        BTIF_TRACE_ERROR("invalid parameters, uuid:%p, channel:%d, sock_fd:%p", service_uuid, channel, sock_fd);
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
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
            status = btsock_l2c_listen(service_name, service_uuid, channel, sock_fd, flags);
#else
            BTIF_TRACE_ERROR("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
#endif
            break;
        case BTSOCK_SCO:
            BTIF_TRACE_ERROR("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            BTIF_TRACE_ERROR("unknown bt socket type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
    }
    return status;
}

static bt_status_t btsock_get_sockopt(btsock_type_t type, int channel, btsock_option_type_t option_name,
                                            void *option_value, int *option_len)
{
    if((channel <= 0) || (option_value == NULL) || (option_len == NULL))
    {
        BTIF_TRACE_ERROR("invalid parameters, channel:%d, option_value:%p, option_len:%p", channel,
                                                                        option_value, option_len);
        return BT_STATUS_PARM_INVALID;
    }

    bt_status_t status = BT_STATUS_FAIL;
    switch(type)
    {
        case BTSOCK_RFCOMM:
            status = btsock_rfc_get_sockopt(channel, option_name, option_value, option_len);
            break;
        case BTSOCK_L2CAP:
            BTIF_TRACE_ERROR("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        case BTSOCK_SCO:
            BTIF_TRACE_ERROR("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            BTIF_TRACE_ERROR("unknown bt socket type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
    }
    return status;
}

static bt_status_t btsock_set_sockopt(btsock_type_t type, int channel, btsock_option_type_t option_name,
                                            void *option_value, int option_len)
{
    if((channel <= 0) || (option_value == NULL))
    {
        BTIF_TRACE_ERROR("invalid parameters, channel:%d, option_value:%p", channel, option_value);
        return BT_STATUS_PARM_INVALID;
    }

    bt_status_t status = BT_STATUS_FAIL;
    switch(type)
    {
        case BTSOCK_RFCOMM:
            status = btsock_rfc_set_sockopt(channel, option_name, option_value, option_len);
            break;
        case BTSOCK_L2CAP:
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
            status = btsock_l2c_set_sockopt(channel, option_name, option_value, option_len);
#else
            BTIF_TRACE_ERROR("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
#endif
            break;
        case BTSOCK_SCO:
            BTIF_TRACE_ERROR("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            BTIF_TRACE_ERROR("unknown bt socket type:%d", type);
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
        BTIF_TRACE_ERROR("invalid parameters, bd_addr:%p, uuid:%p, channel:%d, sock_fd:%p",
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
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
            status = btsock_l2c_connect(bd_addr, uuid, channel, sock_fd, flags);
#else
            BTIF_TRACE_ERROR("bt l2cap socket type not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
#endif
            break;
        case BTSOCK_SCO:
            BTIF_TRACE_ERROR("bt sco socket not supported, type:%d", type);
            status = BT_STATUS_UNSUPPORTED;
            break;
        default:
            BTIF_TRACE_ERROR("unknown bt socket type:%d", type);
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
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
            btsock_l2c_signaled(fd, flags, user_id);
#else
            BTIF_TRACE_ERROR("bt l2cap socket type not supported, fd:%d, flags:%d", fd, flags);
#endif
            break;
        case BTSOCK_SCO:
            BTIF_TRACE_ERROR("bt sco socket type not supported, fd:%d, flags:%d", fd, flags);
            break;
        default:
            BTIF_TRACE_ERROR("unknown socket type:%d, fd:%d, flags:%d", type, fd, flags);
            break;
    }
}



