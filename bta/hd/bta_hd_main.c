/******************************************************************************
 *
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2005-2012 Broadcom Corporation
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
 *  This file contains the HID host main functions and state machine.
 *
 ******************************************************************************/

#include "bt_target.h"

#if defined(BTA_HD_INCLUDED) && (BTA_HD_INCLUDED == TRUE)

#include <string.h>

#include "bd.h"
#include "bta_hd_api.h"
#include "bta_hd_int.h"
#include "gki.h"

/*****************************************************************************
** Global data
*****************************************************************************/
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_HD_CB  bta_hd_cb;
#endif

static tBTA_HD_EVT state_disabled(BT_HDR *p_msg, tBTA_HD *cback_data) {
    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_API_ENABLE_EVT:
            bta_hd_api_enable((tBTA_HD_DATA *) p_msg);
            bta_hd_cb.state = BTA_HD_STATE_ENABLED;
            break;

        default:
            APPL_TRACE_WARNING2("%s: unexpected event (%d)", __FUNCTION__, p_msg->event);
            break;
    }

    return 0;
}

static tBTA_HD_EVT state_enabled(BT_HDR *p_msg, tBTA_HD *cback_data) {
    tBTA_HD_CBACK_DATA  *p_data = (tBTA_HD_CBACK_DATA *) p_msg;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_API_REGISTER_APP_EVT:
            bta_hd_register_sdp((tBTA_HD_REGISTER_APP *) p_msg);
            bta_hd_cb.state = BTA_HD_STATE_IDLE;
            break;

        case BTA_HD_API_DISABLE_EVT:
            bta_hd_api_disable();
            bta_hd_cb.state = BTA_HD_STATE_DISABLED;
            break;

        case BTA_HD_API_ADD_DEVICE_EVT:
            HID_DevPlugDevice(p_data->addr);
            break;

        case BTA_HD_API_REMOVE_DEVICE_EVT:
            HID_DevUnplugDevice(p_data->addr);
            break;

        default:
            APPL_TRACE_WARNING2("%s: unexpected event (%d)", __FUNCTION__, p_msg->event);
            break;
    }

    return 0;
}

static tBTA_HD_EVT state_idle(BT_HDR *p_msg, tBTA_HD *cback_data) {
    tBTA_HD_CBACK_DATA  *p_data = (tBTA_HD_CBACK_DATA *) p_msg;
    tBTA_HD_EVT         cback_event = 0;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_API_DISABLE_EVT:
            bta_hd_unregister_sdp();
            bta_hd_api_disable();
            bta_hd_cb.state = BTA_HD_STATE_DISABLED;
            break;

        case BTA_HD_API_UNREGISTER_APP_EVT:
            bta_hd_disconnect();
            bta_hd_unregister_sdp();
            bta_hd_cb.state = BTA_HD_STATE_ENABLED;
            break;

        case BTA_HD_API_CONNECT_EVT:
            bta_hd_connect();
            break;

        case BTA_HD_API_ADD_DEVICE_EVT:
            HID_DevPlugDevice(p_data->addr);
            break;

        case BTA_HD_API_REMOVE_DEVICE_EVT:
            HID_DevUnplugDevice(p_data->addr);
            break;

        case BTA_HD_INT_OPEN_EVT:
            HID_DevPlugDevice(p_data->addr);
            bta_sys_conn_open(BTA_ID_HD, 1, p_data->addr);
            cback_event = BTA_HD_OPEN_EVT;
            bdcpy(cback_data->conn.bda, p_data->addr);
            bdcpy(bta_hd_cb.bd_addr, p_data->addr);

            bta_hd_cb.state = BTA_HD_STATE_CONNECTED;
            break;

        case BTA_HD_INT_SEND_REPORT_EVT:
            bta_hd_send_report((tBTA_HD_SEND_REPORT *) p_msg);
            break;

        case BTA_HD_INT_GET_REPORT_EVT:
            bta_hd_get_report(p_data->data, p_data->p_data);
            break;

        case BTA_HD_INT_SET_REPORT_EVT:
            bta_hd_set_report(p_data->p_data);
            break;

        case BTA_HD_INT_SET_PROTOCOL_EVT:
            bta_hd_cb.boot_mode = (p_data->data == HID_PAR_PROTOCOL_BOOT_MODE);
            cback_event = BTA_HD_SET_PROTOCOL_EVT;
            cback_data->set_protocol = p_data->data;
            break;

        case BTA_HD_INT_INTR_DATA_EVT:
            bta_hd_intr_data(p_data->p_data);
            break;

        case BTA_HD_INT_SUSPEND_EVT:
            bta_sys_idle(BTA_ID_HD, 1, p_data->addr);
            break;

        case BTA_HD_INT_EXIT_SUSPEND_EVT:
            bta_sys_busy(BTA_ID_HD, 1, p_data->addr);
            bta_sys_idle(BTA_ID_HD, 1, p_data->addr);
            break;

        default:
            APPL_TRACE_WARNING2("%s: unexpected event (%d)", __FUNCTION__, p_msg->event);
            break;
    }

    return cback_event;
}

static tBTA_HD_EVT state_connected(BT_HDR *p_msg, tBTA_HD *cback_data) {
    tBTA_HD_CBACK_DATA  *p_data = (tBTA_HD_CBACK_DATA *) p_msg;
    tBTA_HD_EVT         cback_event = 0;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_API_DISABLE_EVT:
            bta_hd_disconnect();
            bta_hd_cb.state = BTA_HD_STATE_REMOVING;
            break;

        case BTA_HD_API_UNREGISTER_APP_EVT:
            bta_hd_disconnect();
            bta_hd_cb.state = BTA_HD_STATE_DISABLING;
            break;

        case BTA_HD_API_VC_UNPLUG_EVT:
            bta_hd_vc_unplug();
            break;

        case BTA_HD_API_DISCONNECT_EVT:
            bta_hd_disconnect();
            break;

        case BTA_HD_API_REPORT_ERROR_EVT:
            bta_hd_report_error();
            break;

        case BTA_HD_INT_VC_UNPLUG_EVT:
            bta_sys_conn_close(BTA_ID_HD, 1, p_data->addr);

            HID_DevUnplugDevice(p_data->addr);

            cback_event = BTA_HD_VC_UNPLUG_EVT;
            bdcpy(cback_data->conn.bda, p_data->addr);
            bdcpy(bta_hd_cb.bd_addr, p_data->addr);
            break;

        case BTA_HD_INT_CLOSE_EVT:
            bta_sys_conn_close(BTA_ID_HD, 1, p_data->addr);

            if (bta_hd_cb.vc_unplug)
            {
                bta_hd_cb.vc_unplug = FALSE;
                HID_DevUnplugDevice(p_data->addr);
                cback_event = BTA_HD_VC_UNPLUG_EVT;
            }
            else
            {
                cback_event = BTA_HD_CLOSE_EVT;
            }

            bdcpy(cback_data->conn.bda, p_data->addr);
            memset(bta_hd_cb.bd_addr, 0, sizeof(BD_ADDR));

            bta_hd_cb.state = BTA_HD_STATE_IDLE;
            break;

        case BTA_HD_API_CONNECT_EVT:
            // ignore
            break;

        default:
            return state_idle(p_msg, cback_data);
    }

    return cback_event;
}

static tBTA_HD_EVT state_disabling(BT_HDR *p_msg, tBTA_HD *cback_data) {
    tBTA_HD_EVT cback_event = 0;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_INT_CLOSE_EVT:
            // first need to fire proper callback for disconnection
            cback_event = state_connected(p_msg, cback_data);
            bta_hd_cb.p_cback(cback_event, cback_data);

            // and now we can safely remove sdp
            bta_hd_unregister_sdp();
            bta_hd_cb.state = BTA_HD_STATE_ENABLED;
            break;

        default:
            // silently ignore
            break;
    }

    // always return 0 since close event is handled
    return 0;
}

static tBTA_HD_EVT state_removing(BT_HDR *p_msg, tBTA_HD *cback_data) {
    tBTA_HD_EVT cback_event = 0;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (p_msg->event) {
        case BTA_HD_INT_CLOSE_EVT:
            // first need to fire proper callback for disconnection
            cback_event = state_connected(p_msg, cback_data);
            bta_hd_cb.p_cback(cback_event, cback_data);

            // and now we can safely remove and unregister
            bta_hd_unregister_sdp();
            bta_hd_api_disable();
            bta_hd_cb.state = BTA_HD_STATE_DISABLED;
            break;

        default:
            // silently ignore
            break;
    }

    // always return 0 since close event is handled
    return 0;
}

/*******************************************************************************
**
** Function         bta_hd_hdl_event
**
** Description      HID device main event handling function.
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_hd_hdl_event(BT_HDR *p_msg)
{
    tBTA_HD_CBACK_DATA  *p_data = (tBTA_HD_CBACK_DATA *) p_msg;
    tBTA_HD_EVT         cback_event = 0;
    tBTA_HD             cback_data;

    APPL_TRACE_API2("%s: p_msg->event=%d", __FUNCTION__, p_msg->event);

    switch (bta_hd_cb.state) {
        case BTA_HD_STATE_DISABLED:
            cback_event = state_disabled(p_msg, &cback_data);
            break;

        case BTA_HD_STATE_ENABLED:
            cback_event = state_enabled(p_msg, &cback_data);
            break;

        case BTA_HD_STATE_IDLE:
            cback_event = state_idle(p_msg, &cback_data);
            break;

        case BTA_HD_STATE_CONNECTED:
            cback_event = state_connected(p_msg, &cback_data);
            break;

        case BTA_HD_STATE_DISABLING:
            cback_event = state_disabling(p_msg, &cback_data);
            break;

        case BTA_HD_STATE_REMOVING:
            cback_event = state_removing(p_msg, &cback_data);
            break;

        default:
            APPL_TRACE_WARNING2("%s: invalid state (%d)", __FUNCTION__, bta_hd_cb.state);
            break;
    }

    if (cback_event)
        bta_hd_cb.p_cback(cback_event, &cback_data);

    return (TRUE);
}

#endif /* BTA_HD_INCLUDED */
