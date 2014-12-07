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
 *  Filename:      bluetooth.c
 *
 *  Description:   Bluetooth HAL implementation
 *
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>
#include <hardware/bt_hf_client.h>
#include <hardware/bt_av.h>
#include <hardware/bt_sock.h>
#include <hardware/bt_hh.h>
#include <hardware/bt_hd.h>
#include <hardware/bt_hl.h>
#include <hardware/bt_pan.h>
#include <hardware/bt_mce.h>
#include <hardware/bt_gatt.h>
#include <hardware/bt_rc.h>
#include <hardware/wipower.h>

#define LOG_NDDEBUG 0
#define LOG_TAG "bluedroid"

#include "btif_api.h"
#include "bd.h"
#include "bt_utils.h"
#include "l2cdefs.h"
#include "l2c_api.h"

#if TEST_APP_INTERFACE == TRUE
#include <bt_testapp.h>
#endif

/************************************************************************************
**  Constants & Macros
************************************************************************************/
#define BT_LE_LPP_WRITE_RSSI_THRESH  0x0001
#define BT_LE_LPP_MONITOR_RSSI_START 0x0002
#define BT_LE_LPP_MONITOR_RSSI_STOP  0x0003
#define BT_LE_LPP_READ_RSSI_THRESH   0x0004

#define is_profile(profile, str) ((strlen(str) == strlen(profile)) && strncmp((const char *)profile, str, strlen(str)) == 0)

#define BT_LE_LPP_RSSI_MONITOR_CMD_CPL_EVT  0x1002
#define BT_LE_LPP_RSSI_MONITOR_THRESH_EVT   0x1003

/************************************************************************************
**  Local type definitions
************************************************************************************/
typedef struct
{
    bt_bdaddr_t remote_bda;
    char        min_thresh;
    char        max_thresh;
    bool        enable;
}__attribute__((packed)) bt_le_lpp_monitor_rssi_cb_t;

typedef struct
{
    bt_bdaddr_t remote_bda;
#define RSSI_MONITOR_CMD_CPL_EVT 0x01
#define RSSI_MONITOR_THRESH_EVT  0x02
    uint8_t  type;

    union
    {
        tBTM_RSSI_MONITOR_CMD_CPL_CB_PARAM cmd_cpl_evt;
        tBTM_RSSI_MONITOR_EVENT_CB_PARAM   rssi_thresh_evt;
    } content;
}bt_le_lpp_rssi_monitor_evt_cb_t;

/************************************************************************************
**  Static variables
************************************************************************************/

bt_callbacks_t *bt_hal_cbacks = NULL;

/** Operating System specific callouts for resource management */
bt_os_callouts_t *bt_os_callouts = NULL;

/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/

/* list all extended interfaces here */

/* handsfree profile */
extern bthf_interface_t *btif_hf_get_interface();
/* handsfree profile - client */
extern bthf_client_interface_t *btif_hf_client_get_interface();
/* advanced audio profile */
extern btav_interface_t *btif_av_get_src_interface();
extern btav_interface_t *btif_av_get_sink_interface();
/*rfc l2cap*/
extern btsock_interface_t *btif_sock_get_interface();
/* hid host profile */
extern bthh_interface_t *btif_hh_get_interface();
/* hid device profile */
extern bthd_interface_t *btif_hd_get_interface();
/* health device profile */
extern bthl_interface_t *btif_hl_get_interface();
/*pan*/
extern btpan_interface_t *btif_pan_get_interface();
/*map client*/
extern btmce_interface_t *btif_mce_get_interface();
#if BLE_INCLUDED == TRUE
/* gatt */
extern btgatt_interface_t *btif_gatt_get_interface();
#endif
/* avrc target */
extern btrc_interface_t *btif_rc_get_interface();
/* avrc controller */
extern wipower_interface_t *get_wipower_interface();
extern btrc_interface_t *btif_rc_ctrl_get_interface();

#if TEST_APP_INTERFACE == TRUE
extern const btl2cap_interface_t *btif_l2cap_get_interface(void);
extern const btsdp_interface_t *btif_sdp_get_interface(void);
extern const btrfcomm_interface_t *btif_rfcomm_get_interface(void);
extern const btgatt_test_interface_t *btif_gatt_test_get_interface(void);
extern const btsmp_interface_t *btif_smp_get_interface(void);
extern const btgap_interface_t *btif_gap_get_interface(void);
#endif

/************************************************************************************
**  Functions
************************************************************************************/

static uint8_t interface_ready(void)
{
    /* add checks here that would prevent API calls other than init to be executed */
    if (bt_hal_cbacks == NULL)
        return FALSE;

    return TRUE;
}


/*****************************************************************************
**
**   BLUETOOTH HAL INTERFACE FUNCTIONS
**
*****************************************************************************/

static int init(bt_callbacks_t* callbacks )
{
    ALOGI("init");

    /* sanity check */
    if (interface_ready() == TRUE)
        return BT_STATUS_DONE;

    /* store reference to user callbacks */
    bt_hal_cbacks = callbacks;

    /* add checks for individual callbacks ? */

    bt_utils_init();

    /* init btif */
    btif_init_bluetooth();

    return BT_STATUS_SUCCESS;
}

static int initq(bt_callbacks_t* callbacks)
{
    ALOGI("initq");
    if(interface_ready()==FALSE)
        return BT_STATUS_NOT_READY; //halbacks have not been initialized for the interface yet, by the adapterservice
    bt_hal_cbacks->le_lpp_write_rssi_thresh_cb   = callbacks->le_lpp_write_rssi_thresh_cb;
    bt_hal_cbacks->le_lpp_read_rssi_thresh_cb    = callbacks->le_lpp_read_rssi_thresh_cb;
    bt_hal_cbacks->le_lpp_enable_rssi_monitor_cb = callbacks->le_lpp_enable_rssi_monitor_cb;
    bt_hal_cbacks->le_lpp_rssi_threshold_evt_cb  = callbacks->le_lpp_rssi_threshold_evt_cb;
    return BT_STATUS_SUCCESS;
}


static int enable( void )
{
    ALOGI("enable");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_enable_bluetooth();
}

static int disable(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_disable_bluetooth();
}

static void cleanup( void )
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return;

    btif_shutdown_bluetooth();

    /* hal callbacks reset upon shutdown complete callback */

    return;
}

static void ssrcleanup(void)
{
    btif_ssr_cleanup();
    return;
}

static int get_adapter_properties(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_adapter_properties();
}

static int get_adapter_property(bt_property_type_t type)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_adapter_property(type);
}

static int set_adapter_property(const bt_property_t *property)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;
    return btif_set_adapter_property(property);
}

int get_remote_device_properties(bt_bdaddr_t *remote_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_device_properties(remote_addr);
}

int get_remote_device_property(bt_bdaddr_t *remote_addr, bt_property_type_t type)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_device_property(remote_addr, type);
}

int set_remote_device_property(bt_bdaddr_t *remote_addr, const bt_property_t *property)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_set_remote_device_property(remote_addr, property);
}

int get_remote_service_record(bt_bdaddr_t *remote_addr, bt_uuid_t *uuid)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_service_record(remote_addr, uuid);
}

int get_remote_services(bt_bdaddr_t *remote_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_get_remote_services(remote_addr);
}

static int start_discovery(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_start_discovery();
}

static int cancel_discovery(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_cancel_discovery();
}

static int create_bond(const bt_bdaddr_t *bd_addr, int transport)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_create_bond(bd_addr, transport);
}

static int cancel_bond(const bt_bdaddr_t *bd_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_cancel_bond(bd_addr);
}

static int remove_bond(const bt_bdaddr_t *bd_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_remove_bond(bd_addr);
}

static int get_connection_state(const bt_bdaddr_t *bd_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return 0;

    return btif_dm_get_connection_state(bd_addr);
}

static int pin_reply(const bt_bdaddr_t *bd_addr, uint8_t accept,
                 uint8_t pin_len, bt_pin_code_t *pin_code)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_pin_reply(bd_addr, accept, pin_len, pin_code);
}

static int ssp_reply(const bt_bdaddr_t *bd_addr, bt_ssp_variant_t variant,
                       uint8_t accept, uint32_t passkey)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_ssp_reply(bd_addr, variant, accept, passkey);
}

static int read_energy_info()
{
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;
    btif_dm_read_energy_info();
    return BT_STATUS_SUCCESS;
}

static const void* get_profile_interface (const char *profile_id)
{
    ALOGI("get_profile_interface %s", profile_id);

    /* sanity check */
    if (interface_ready() == FALSE)
        return NULL;

    /* check for supported profile interfaces */
    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_ID))
        return btif_hf_get_interface();

    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_CLIENT_ID))
        return btif_hf_client_get_interface();

    if (is_profile(profile_id, BT_PROFILE_SOCKETS_ID))
        return btif_sock_get_interface();

    if (is_profile(profile_id, BT_PROFILE_PAN_ID))
        return btif_pan_get_interface();

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_ID))
        return btif_av_get_src_interface();

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SINK_ID))
        return btif_av_get_sink_interface();

    if (is_profile(profile_id, BT_PROFILE_HIDHOST_ID))
        return btif_hh_get_interface();

    if (is_profile(profile_id, BT_PROFILE_HIDDEV_ID))
        return btif_hd_get_interface();

    if (is_profile(profile_id, BT_PROFILE_HEALTH_ID))
        return btif_hl_get_interface();

    if (is_profile(profile_id, BT_PROFILE_MAP_CLIENT_ID))
        return btif_mce_get_interface();

#if ( BTA_GATT_INCLUDED == TRUE && BLE_INCLUDED == TRUE)
    if (is_profile(profile_id, BT_PROFILE_GATT_ID))
        return btif_gatt_get_interface();
#endif

    if (is_profile(profile_id, BT_PROFILE_AV_RC_ID))
        return btif_rc_get_interface();

    if (is_profile(profile_id, WIPOWER_PROFILE_ID))
        return get_wipower_interface();


    if (is_profile(profile_id, BT_PROFILE_AV_RC_CTRL_ID))
        return btif_rc_ctrl_get_interface();

    return NULL;
}
#if TEST_APP_INTERFACE == TRUE
static const void* get_testapp_interface(int test_app_profile)
{
    ALOGI("get_testapp_interface %d", test_app_profile);

    if (interface_ready() == FALSE) {
        return NULL;
    }
    switch(test_app_profile) {
        case TEST_APP_L2CAP:
            return btif_l2cap_get_interface();
        case TEST_APP_SDP:
            return btif_sdp_get_interface();
        case TEST_APP_RFCOMM:
            return btif_rfcomm_get_interface();
        case TEST_APP_GATT:
           return btif_gatt_test_get_interface();
        case TEST_APP_SMP:
           return btif_smp_get_interface();
        case TEST_APP_GAP:
           return btif_gap_get_interface();
        default:
            return NULL;
    }
    return NULL;
}

#endif //TEST_APP_INTERFACE

int dut_mode_configure(uint8_t enable)
{
    ALOGI("dut_mode_configure");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dut_mode_configure(enable);
}

int dut_mode_send(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    ALOGI("dut_mode_send");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dut_mode_send(opcode, buf, len);
}

#if BLE_INCLUDED == TRUE
int le_test_mode(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    ALOGI("le_test_mode");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_le_test_mode(opcode, buf, len);
}
#endif

int config_hci_snoop_log(uint8_t enable)
{
    ALOGI("config_hci_snoop_log");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_config_hci_snoop_log(enable);
}

static int set_os_callouts(bt_os_callouts_t *callouts) {
    bt_os_callouts = callouts;
    return BT_STATUS_SUCCESS;
}

static void bt_le_lpp_rssi_monitor_upstream_evt(uint16_t event, char* p_param)
{
    bt_le_lpp_rssi_monitor_evt_cb_t *p_cb = (bt_le_lpp_rssi_monitor_evt_cb_t*)p_param;
    ALOGD("%s: Event %d", __FUNCTION__, event);
    if(!p_cb)
        return;
    switch(event)
    {
    case BT_LE_LPP_RSSI_MONITOR_CMD_CPL_EVT:
        switch(p_cb->content.cmd_cpl_evt.subcmd)
        {
        case WRITE_RSSI_MONITOR_THRESHOLD:
            HAL_CBACK(bt_hal_cbacks, le_lpp_write_rssi_thresh_cb,
                      &p_cb->remote_bda, p_cb->content.cmd_cpl_evt.status);
            break;
        case READ_RSSI_MONITOR_THRESHOLD:
            HAL_CBACK(bt_hal_cbacks, le_lpp_read_rssi_thresh_cb,
                      &p_cb->remote_bda, p_cb->content.cmd_cpl_evt.detail.read_result.low,
                      p_cb->content.cmd_cpl_evt.detail.read_result.upper,
                      p_cb->content.cmd_cpl_evt.detail.read_result.alert,
                      p_cb->content.cmd_cpl_evt.status);

            break;
        case ENABLE_RSSI_MONITOR:
            HAL_CBACK(bt_hal_cbacks, le_lpp_enable_rssi_monitor_cb,
                      &p_cb->remote_bda, p_cb->content.cmd_cpl_evt.detail.enable,
                      p_cb->content.cmd_cpl_evt.status);
            break;
        default:
            break;
        }
        break;
    case BT_LE_LPP_RSSI_MONITOR_THRESH_EVT:
         ALOGD("%s rssi value = %d", __FUNCTION__, (int)(p_cb->content.rssi_thresh_evt.rssi_value));
         HAL_CBACK(bt_hal_cbacks, le_lpp_rssi_threshold_evt_cb,
                  &p_cb->remote_bda, p_cb->content.rssi_thresh_evt.rssi_event_type,
                  p_cb->content.rssi_thresh_evt.rssi_value);
        break;
    default:
        ALOGE("%s invalid event", __FUNCTION__);
    }
    ALOGD("%s exit", __FUNCTION__);
}

static void rssi_threshold_command_cb(BD_ADDR remote_bda, tBTM_RSSI_MONITOR_CMD_CPL_CB_PARAM *param)
{
    ALOGD("%s enter", __FUNCTION__);
    if(remote_bda && param)
    {
        bt_le_lpp_rssi_monitor_evt_cb_t btif_cb;
        memset(&btif_cb, 0, sizeof(btif_cb));
        bdcpy(btif_cb.remote_bda.address, remote_bda);
        btif_cb.type = RSSI_MONITOR_CMD_CPL_EVT;
        btif_cb.content.cmd_cpl_evt = *param;
        btif_transfer_context(bt_le_lpp_rssi_monitor_upstream_evt, BT_LE_LPP_RSSI_MONITOR_CMD_CPL_EVT,
                                 (char*) &btif_cb, sizeof(btif_cb), NULL);
    }
    ALOGD("%s exit", __FUNCTION__);
    return;
}

static void rssi_threshold_event_cb(BD_ADDR remote_bda, tBTM_RSSI_MONITOR_EVENT_CB_PARAM *param)
{
    ALOGD("%s enter", __FUNCTION__);
    if(remote_bda&& param)
    {
        bt_le_lpp_rssi_monitor_evt_cb_t btif_cb;
        memset(&btif_cb, 0, sizeof(btif_cb));
        bdcpy(btif_cb.remote_bda.address, remote_bda);
        btif_cb.type = RSSI_MONITOR_THRESH_EVT;
        btif_cb.content.rssi_thresh_evt = *param;
        btif_transfer_context(bt_le_lpp_rssi_monitor_upstream_evt, BT_LE_LPP_RSSI_MONITOR_THRESH_EVT,
                                 (char*) &btif_cb, sizeof(btif_cb), NULL);
    }
    ALOGD("%s exit", __FUNCTION__);
    return;
}

static void bt_le_handle_lpp_monitor_rssi(uint16_t event, char *p_param)
{
    tBTM_STATUS status = BTM_ILLEGAL_ACTION;
    tBTM_RSSI_MONITOR_CMD_CPL_CB_PARAM error;
    bt_le_lpp_monitor_rssi_cb_t *p_cb = (bt_le_lpp_monitor_rssi_cb_t*)p_param;
    memset(&error, 0, sizeof(error));
    ALOGD("%s enter", __FUNCTION__);
    if(!p_cb)
        return;

    /* setup callback for command completion routine and event report */
    ALOGD("%s setup callback for BTM Layer", __FUNCTION__);
    btm_setup_rssi_threshold_callback(rssi_threshold_command_cb, rssi_threshold_event_cb);

    switch(event)
    {
    case BT_LE_LPP_WRITE_RSSI_THRESH:
        ALOGD("%s write rssi thrshold ", __FUNCTION__);
        status = BTM_Write_Rssi_Monitor_Threshold(p_cb->remote_bda.address, p_cb->min_thresh, p_cb->max_thresh);

        if (status != BTM_CMD_STARTED)
        {
            ALOGE("%s write rssi threshold fail", __FUNCTION__);
            error.subcmd = WRITE_RSSI_MONITOR_THRESHOLD;
            error.status = 1; /* 0 indicate success*/
        }
        break;
    case BT_LE_LPP_MONITOR_RSSI_START:
        ALOGD("%s starts monitoring rssi", __FUNCTION__);
        status = BTM_Enable_Rssi_Monitor(p_cb->remote_bda.address, 1);

        if (status != BTM_CMD_STARTED)
        {
            ALOGE("%s enable rssi monitor fail", __FUNCTION__);
            error.subcmd = ENABLE_RSSI_MONITOR;
            error.detail.enable = 1;
            error.status = 1; /* 0 indicate success*/
        }
        break;
    case BT_LE_LPP_MONITOR_RSSI_STOP:
        ALOGD("%s stop monitoring rssi", __FUNCTION__);
        status = BTM_Enable_Rssi_Monitor(p_cb->remote_bda.address, 0);

        if (status != BTM_CMD_STARTED)
        {
            ALOGE("%s disable rssi monitor fail", __FUNCTION__);
            error.subcmd = ENABLE_RSSI_MONITOR;
            error.detail.enable = 0;
            error.status = 1; /* 0 indicate success*/
        }
        break;
    case BT_LE_LPP_READ_RSSI_THRESH:
        ALOGD("%s read rssi threshold", __FUNCTION__);
        status = BTM_Read_Rssi_Monitor_Threshold(p_cb->remote_bda.address);

        if (status != BTM_CMD_STARTED)
        {
            ALOGE("%s read rssi threshold fail", __FUNCTION__);
            error.subcmd = READ_RSSI_MONITOR_THRESHOLD;
            error.status = 1; /* 0 indicate success*/
        }
        break;
    default:
        error.status = 1;
        ALOGE("%s Unknown event!!", __FUNCTION__);
        break;
    }

    if(status != BTM_CMD_STARTED)
    {
        BD_ADDR bda;
        bdcpy(bda, p_cb->remote_bda.address);
        rssi_threshold_command_cb(bda, &error);
        ALOGE("%s Rssi Monitor command fails", __FUNCTION__);
    }
    ALOGD("%s exit", __FUNCTION__);
    return;
}

static bt_status_t bt_le_lpp_write_rssi_threshold(const bt_bdaddr_t *remote_bda, char min, char max)
{
    bt_le_lpp_monitor_rssi_cb_t btif_cb;
    memset(&btif_cb, 0, sizeof(bt_le_lpp_monitor_rssi_cb_t));
    if(!remote_bda || min > max)
        return BT_STATUS_PARM_INVALID;
    bdcpy(btif_cb.remote_bda.address, remote_bda->address);
    btif_cb.min_thresh = min;
    btif_cb.max_thresh = max;
    return btif_transfer_context(bt_le_handle_lpp_monitor_rssi, BT_LE_LPP_WRITE_RSSI_THRESH,
                                (char*)&btif_cb, sizeof(bt_le_lpp_monitor_rssi_cb_t), NULL);
}

static bt_status_t bt_le_lpp_enable_rssi_monitor(const bt_bdaddr_t *remote_bda, int enable)
{
    bt_le_lpp_monitor_rssi_cb_t btif_cb;
    memset(&btif_cb, 0, sizeof(bt_le_lpp_monitor_rssi_cb_t));
    if(!remote_bda)
        return BT_STATUS_PARM_INVALID;
    bdcpy(btif_cb.remote_bda.address, remote_bda->address);
    btif_cb.enable = enable;
    return btif_transfer_context(bt_le_handle_lpp_monitor_rssi, enable?BT_LE_LPP_MONITOR_RSSI_START:BT_LE_LPP_MONITOR_RSSI_STOP,
                                 (char*)&btif_cb, sizeof(bt_le_lpp_monitor_rssi_cb_t), NULL);
}

static bt_status_t bt_le_lpp_read_rssi_threshold(const bt_bdaddr_t *remote_bda)
{
    bt_le_lpp_monitor_rssi_cb_t btif_cb;
    memset(&btif_cb, 0, sizeof(bt_le_lpp_monitor_rssi_cb_t));
    if(!remote_bda)
        return BT_STATUS_PARM_INVALID;
    bdcpy(btif_cb.remote_bda.address, remote_bda->address);
    return btif_transfer_context(bt_le_handle_lpp_monitor_rssi, BT_LE_LPP_READ_RSSI_THRESH,
                                 (char*)&btif_cb, sizeof(bt_le_lpp_monitor_rssi_cb_t), NULL);
}

static const bt_interface_t bluetoothInterface = {
    sizeof(bluetoothInterface),
    init,
    initq,
    enable,
    disable,
    cleanup,
    ssrcleanup,
    get_adapter_properties,
    get_adapter_property,
    set_adapter_property,
    get_remote_device_properties,
    get_remote_device_property,
    set_remote_device_property,
    get_remote_service_record,
    get_remote_services,
    start_discovery,
    cancel_discovery,
    create_bond,
    remove_bond,
    cancel_bond,
    get_connection_state,
    pin_reply,
    ssp_reply,
    get_profile_interface,
    dut_mode_configure,
    dut_mode_send,
#if BLE_INCLUDED == TRUE
    le_test_mode,
#else
    NULL,
#endif
    config_hci_snoop_log,
    set_os_callouts,
    read_energy_info,
#if TEST_APP_INTERFACE == TRUE
    get_testapp_interface,
#else
    NULL,
#endif
    bt_le_lpp_write_rssi_threshold,
    bt_le_lpp_enable_rssi_monitor,
    bt_le_lpp_read_rssi_threshold,
};

const bt_interface_t* bluetooth__get_bluetooth_interface ()
{
    /* fixme -- add property to disable bt interface ? */

    return &bluetoothInterface;
}

static int close_bluetooth_stack(struct hw_device_t* device)
{
    UNUSED(device);
    cleanup();
    return 0;
}

static int open_bluetooth_stack (const struct hw_module_t* module, char const* name,
                                 struct hw_device_t** abstraction)
{
    UNUSED(name);

    bluetooth_device_t *stack = malloc(sizeof(bluetooth_device_t) );
    if (stack)
    {
        memset(stack, 0, sizeof(bluetooth_device_t) );
        stack->common.tag = HARDWARE_DEVICE_TAG;
        stack->common.version = 0;
        stack->common.module = (struct hw_module_t*)module;
        stack->common.close = close_bluetooth_stack;
        stack->get_bluetooth_interface = bluetooth__get_bluetooth_interface;
    }
    else
    {
        ALOGE("%s: malloc() returned NULL", __FUNCTION__);
    }
    *abstraction = (struct hw_device_t*)stack;
    return 0;
}


static struct hw_module_methods_t bt_stack_module_methods = {
    .open = open_bluetooth_stack,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = BT_HARDWARE_MODULE_ID,
    .name = "Bluetooth Stack",
    .author = "The Android Open Source Project",
    .methods = &bt_stack_module_methods
};

