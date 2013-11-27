/******************************************************************************
 *
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
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
 *  Filename:      btif_hd.c
 *
 *  Description:   HID Device Profile Bluetooth Interface
 *
 *
 ***********************************************************************************/
#include <hardware/bluetooth.h>
#include <hardware/bt_hd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define LOG_TAG "BTIF_HD"

#include "bta_api.h"
#include "bta_hd_api.h"

#include "btif_common.h"
#include "btif_util.h"
#include "btif_storage.h"
#include "btif_hd.h"

/* HD request events */
typedef enum
{
    BTIF_HD_DUMMY_REQ_EVT = 0
} btif_hd_req_evt_t;

btif_hd_cb_t btif_hd_cb;

static bthd_callbacks_t *bt_hd_callbacks = NULL;

static void intr_data_copy_cb(UINT16 event, char *p_dst, char *p_src)
{
    tBTA_HD_INTR_DATA *p_dst_data = (tBTA_HD_INTR_DATA *) p_dst;
    tBTA_HD_INTR_DATA *p_src_data = (tBTA_HD_INTR_DATA *) p_src;
    UINT8 *p_data;

    if (!p_src)
        return;

    if (event != BTA_HD_INTR_DATA_EVT)
        return;

    memcpy(p_dst, p_src, sizeof(tBTA_HD_INTR_DATA));

    p_data = ((UINT8 *) p_dst_data) + sizeof(tBTA_HD_INTR_DATA);

    memcpy(p_data, p_src_data->p_data, p_src_data->len);

    p_dst_data->p_data = p_data;
}

static void set_report_copy_cb(UINT16 event, char *p_dst, char *p_src)
{
    tBTA_HD_SET_REPORT *p_dst_data = (tBTA_HD_SET_REPORT *) p_dst;
    tBTA_HD_SET_REPORT *p_src_data = (tBTA_HD_SET_REPORT *) p_src;
    UINT8 *p_data;

    if (!p_src)
        return;

    if (event != BTA_HD_SET_REPORT_EVT)
        return;

    memcpy(p_dst, p_src, sizeof(tBTA_HD_SET_REPORT));

    p_data = ((UINT8 *) p_dst_data) + sizeof(tBTA_HD_SET_REPORT);

    memcpy(p_data, p_src_data->p_data, p_src_data->len);

    p_dst_data->p_data = p_data;
}


/*******************************************************************************
**
** Function         btif_hd_remove_device
**
** Description      Removes plugged device
**
** Returns          void
**
*******************************************************************************/
void btif_hd_remove_device(bt_bdaddr_t bd_addr)
{
    BTA_HdRemoveDevice((UINT8 *) &bd_addr);
    btif_storage_remove_hidd(&bd_addr);
}

/*******************************************************************************
**
** Function         btif_hd_upstreams_evt
**
** Description      Executes events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_hd_upstreams_evt(UINT16 event, char* p_param)
{
    tBTA_HD *p_data = (tBTA_HD *)p_param;

    BTIF_TRACE_API2("%s: event=%s", __FUNCTION__, dump_hd_event(event));

    switch (event)
    {
        case BTA_HD_ENABLE_EVT:
            BTIF_TRACE_DEBUG2("%s: status=%d", __FUNCTION__, p_data->status);
            if (p_data->status == BTA_HD_OK)
            {
                btif_storage_load_hidd();
                btif_hd_cb.status = BTIF_HD_ENABLED;
            }
            else
            {
                btif_hd_cb.status = BTIF_HD_DISABLED;
                BTIF_TRACE_WARNING1("Failed to enable BT-HD, status=%d", p_data->status);
            }
            break;

        case BTA_HD_DISABLE_EVT:
            BTIF_TRACE_DEBUG2("%s: status=%d", __FUNCTION__, p_data->status);
            btif_hd_cb.status = BTIF_HD_DISABLED;
            if (p_data->status == BTA_HD_OK)
                memset(&btif_hd_cb, 0, sizeof(btif_hd_cb));
            else
                BTIF_TRACE_WARNING1("Failed to disable BT-HD, status=%d", p_data->status);
            break;

        case BTA_HD_REGISTER_APP_EVT:
            {
                bt_bdaddr_t *addr = (bt_bdaddr_t*) &p_data->reg_status.bda;

                if (!p_data->reg_status.in_use)
                {
                    addr = NULL;
                }

                btif_hd_cb.app_registered = TRUE;
                HAL_CBACK(bt_hd_callbacks, application_state_cb, addr, BTHD_APP_STATE_REGISTERED);
        }
            break;

        case BTA_HD_UNREGISTER_APP_EVT:
            btif_hd_cb.app_registered = FALSE;
            HAL_CBACK(bt_hd_callbacks, application_state_cb, NULL, BTHD_APP_STATE_NOT_REGISTERED);
            break;

        case BTA_HD_OPEN_EVT:
            btif_storage_set_hidd((bt_bdaddr_t*) &p_data->conn.bda);

            HAL_CBACK(bt_hd_callbacks, connection_state_cb, (bt_bdaddr_t*) &p_data->conn.bda,
                BTHD_CONN_STATE_CONNECTED);
            break;

        case BTA_HD_CLOSE_EVT:
             HAL_CBACK(bt_hd_callbacks, connection_state_cb, (bt_bdaddr_t*) &p_data->conn.bda,
                BTHD_CONN_STATE_DISCONNECTED);
             break;

        case BTA_HD_GET_REPORT_EVT:
            HAL_CBACK(bt_hd_callbacks, get_report_cb,
                        p_data->get_report.report_type, p_data->get_report.report_id,
                        p_data->get_report.buffer_size);
            break;

        case BTA_HD_SET_REPORT_EVT:
            HAL_CBACK(bt_hd_callbacks, set_report_cb, p_data->set_report.report_type,
                p_data->set_report.report_id, p_data->set_report.len,
                p_data->set_report.p_data);
            break;

        case BTA_HD_SET_PROTOCOL_EVT:
            HAL_CBACK(bt_hd_callbacks, set_protocol_cb, p_data->set_protocol);
            break;

        case BTA_HD_INTR_DATA_EVT:
            HAL_CBACK(bt_hd_callbacks, intr_data_cb, p_data->intr_data.report_id,
                p_data->intr_data.len, p_data->intr_data.p_data);
            break;

        case BTA_HD_VC_UNPLUG_EVT:
            HAL_CBACK(bt_hd_callbacks, connection_state_cb, (bt_bdaddr_t*) &p_data->conn.bda,
                BTHD_CONN_STATE_DISCONNECTED);

            BTA_DmRemoveDevice((UINT8 *) &p_data->conn.bda);
            HAL_CBACK(bt_hd_callbacks, vc_unplug_cb);
            break;

        default:
            BTIF_TRACE_WARNING2("%s: unknown event (%d)", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         bte_hd_evt
**
** Description      Switches context from BTE to BTIF for all BT-HD events
**
** Returns          void
**
*******************************************************************************/

static void bte_hd_evt(tBTA_HD_EVT event, tBTA_HD *p_data)
{
    bt_status_t status;
    int param_len = 0;
    tBTIF_COPY_CBACK *p_copy_cback = NULL;

    BTIF_TRACE_API2("%s event=%d", __FUNCTION__, event);

    switch (event)
    {
    case BTA_HD_ENABLE_EVT:
    case BTA_HD_DISABLE_EVT:
    case BTA_HD_UNREGISTER_APP_EVT:
        param_len = sizeof(tBTA_HD_STATUS);
        break;

    case BTA_HD_REGISTER_APP_EVT:
        param_len = sizeof(tBTA_HD_REG_STATUS);
        break;

    case BTA_HD_OPEN_EVT:
    case BTA_HD_CLOSE_EVT:
    case BTA_HD_VC_UNPLUG_EVT:
        param_len = sizeof(tBTA_HD_CONN);
        break;

    case BTA_HD_GET_REPORT_EVT:
        param_len += sizeof(tBTA_HD_GET_REPORT);
        break;

    case BTA_HD_SET_REPORT_EVT:
        param_len = sizeof(tBTA_HD_SET_REPORT) + p_data->set_report.len;
        p_copy_cback = set_report_copy_cb;
        break;

    case BTA_HD_SET_PROTOCOL_EVT:
        param_len += sizeof(p_data->set_protocol);
        break;

    case BTA_HD_INTR_DATA_EVT:
        param_len = sizeof(tBTA_HD_INTR_DATA) + p_data->intr_data.len;
        p_copy_cback = intr_data_copy_cb;
        break;

    }

    status = btif_transfer_context(btif_hd_upstreams_evt, (uint16_t)event,
        (void*)p_data, param_len, p_copy_cback);

    ASSERTC(status == BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function        init
**
** Description     Initializes BT-HD interface
**
** Returns         BT_STATUS_SUCCESS
**
*******************************************************************************/
static bt_status_t init(bthd_callbacks_t* callbacks )
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    bt_hd_callbacks = callbacks;
    memset(&btif_hd_cb, 0, sizeof(btif_hd_cb));

    btif_enable_service(BTA_HIDD_SERVICE_ID);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Cleans up BT-HD interface
**
** Returns          none
**
*******************************************************************************/
static void  cleanup(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (bt_hd_callbacks)
    {
        btif_disable_service(BTA_HIDD_SERVICE_ID);
        bt_hd_callbacks = NULL;
    }
}

/*******************************************************************************
**
** Function         register_app
**
** Description      Registers HID Device application
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t register_app(bthd_app_param_t *p_app_param, bthd_qos_param_t *p_in_qos,
                                            bthd_qos_param_t *p_out_qos)
{
    tBTA_HD_APP_INFO app_info;
    tBTA_HD_QOS_INFO in_qos;
    tBTA_HD_QOS_INFO out_qos;

    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application already registered", __FUNCTION__);
        return BT_STATUS_BUSY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    app_info.p_name = p_app_param->name;
    app_info.p_description = p_app_param->description;
    app_info.p_provider = p_app_param->provider;
    app_info.subclass = p_app_param->subclass;
    app_info.descriptor.dl_len = p_app_param->desc_list_len;
    app_info.descriptor.dsc_list = p_app_param->desc_list;

    in_qos.service_type = p_in_qos->service_type;
    in_qos.token_rate = p_in_qos->token_rate;
    in_qos.token_bucket_size = p_in_qos->token_bucket_size;
    in_qos.peak_bandwidth = p_in_qos->peak_bandwidth;
    in_qos.access_latency = p_in_qos->access_latency;
    in_qos.delay_variation = p_in_qos->delay_variation;

    out_qos.service_type = p_out_qos->service_type;
    out_qos.token_rate = p_out_qos->token_rate;
    out_qos.token_bucket_size = p_out_qos->token_bucket_size;
    out_qos.peak_bandwidth = p_out_qos->peak_bandwidth;
    out_qos.access_latency = p_out_qos->access_latency;
    out_qos.delay_variation = p_out_qos->delay_variation;

    BTA_HdRegisterApp(&app_info, &in_qos, &out_qos);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         unregister_app
**
** Description      Unregisters HID Device application
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t unregister_app(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    BTA_HdUnregisterApp();

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         connect
**
** Description      Connects to host
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t connect(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    BTA_HdConnect();

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      Disconnects from host
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t disconnect(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    BTA_HdDisconnect();

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         register_app
**
** Description      Registers HID Device application
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t send_report(bthd_report_type_t type, uint8_t id, uint16_t len, uint8_t *p_data)
{
    tBTA_HD_REPORT report;

    BTIF_TRACE_API4("%s: type=%d id=%d len=%d", __FUNCTION__, type, id, len);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    if (type == BTHD_REPORT_TYPE_INTRDATA)
    {
        report.type = BTHD_REPORT_TYPE_INPUT;
        report.use_intr = TRUE;
    }
    else
    {
        report.type = (type & 0x03);
        report.use_intr = FALSE;
    }

    report.id = id;
    report.len = len;
    report.p_data = p_data;

    BTA_HdSendReport(&report);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         report_error
**
** Description      Sends HANDSHAKE with error info for invalid SET_PROTOCOL
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t report_error(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    BTA_HdReportError();

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         virtual_cable_unplug
**
** Description      Sends Virtual Cable Unplug to host
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t virtual_cable_unplug(void)
{
    BTIF_TRACE_API1("%s", __FUNCTION__);

    if (!btif_hd_cb.app_registered)
    {
        BTIF_TRACE_WARNING1("%s: application not yet registered", __FUNCTION__);
        return BT_STATUS_NOT_READY;
    }

    if (btif_hd_cb.status != BTIF_HD_ENABLED)
    {
        BTIF_TRACE_WARNING2("%s: BT-HD not enabled, status=%d", __FUNCTION__, btif_hd_cb.status);
        return BT_STATUS_NOT_READY;
    }

    BTA_HdVirtualCableUnplug();

    return BT_STATUS_SUCCESS;
}

static const bthd_interface_t bthdInterface = {
    sizeof(bthdInterface),
    init,
    cleanup,
    register_app,
    unregister_app,
    connect,
    disconnect,
    send_report,
    report_error,
    virtual_cable_unplug,
};

/*******************************************************************************
**
** Function         btif_hd_execute_service
**
** Description      Enabled/disables BT-HD service
**
** Returns          BT_STATUS_SUCCESS
**
*******************************************************************************/
bt_status_t btif_hd_execute_service(BOOLEAN b_enable)
{
    BTIF_TRACE_API2("%s: b_enable=%d", __FUNCTION__, b_enable);

     if (b_enable)
         BTA_HdEnable(bte_hd_evt);
     else
         BTA_HdDisable();

     return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_hd_get_interface
**
** Description      Gets BT-HD interface
**
** Returns          bthd_interface_t
**
*******************************************************************************/
const bthd_interface_t *btif_hd_get_interface()
{
    BTIF_TRACE_API1("%s", __FUNCTION__);
    return &bthdInterface;
}
