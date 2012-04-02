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
 *  Filename:      btif_dm.c
 *
 *  Description:   Contains Device Management (DM) related functionality
 *
 *
 ***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <hardware/bluetooth.h>

#include <utils/Log.h>

#include "gki.h"
#include "btu.h"
#include "bd.h"
#include "bta_api.h"
#include "btif_api.h"
#include "btif_util.h"
#include "btif_storage.h"
#include "btif_hh.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define COD_UNCLASSIFIED ((0x1F) << 8)
#define COD_HID_KEYBOARD  0x0540
#define COD_HID_POINTING  0x0580
#define COD_HID_COMBO     0x05C0

#define BTIF_DM_DEFAULT_INQ_MAX_RESULTS     0
#define BTIF_DM_DEFAULT_INQ_MAX_DURATION    10

typedef struct {
    bt_bond_state_t state;
    BD_ADDR bd_addr;
    UINT8   is_temp;
    UINT8   pin_code_len;
} btif_dm_pairing_cb_t;

#define BTA_SERVICE_ID_TO_SERVICE_MASK(id)       (1 << (id))

/******************************************************************************
**  Static functions
******************************************************************************/
static btif_dm_pairing_cb_t pairing_cb;
static bt_status_t btif_dm_get_remote_services(bt_bdaddr_t *remote_addr);
static void btif_dm_generic_evt(UINT16 event, char* p_param);
static void btif_dm_cb_create_bond(bt_bdaddr_t *bd_addr);
static void btif_dm_cb_hid_remote_name(tBTM_REMOTE_DEV_NAME *p_remote_name);

/******************************************************************************
**  Externs
******************************************************************************/
extern UINT16 bta_service_id_to_uuid_lkup_tbl [BTA_MAX_SERVICE_ID];
extern bt_status_t btif_hf_execute_service(BOOLEAN b_enable);
extern bt_status_t btif_av_execute_service(BOOLEAN b_enable);
extern bt_status_t btif_hh_execute_service(BOOLEAN b_enable);


/******************************************************************************
**  Functions
******************************************************************************/

bt_status_t btif_in_execute_service_request(tBTA_SERVICE_ID service_id,
                                                BOOLEAN b_enable)
{
    /* Check the service_ID and invoke the profile's BT state changed API */
    switch (service_id)
    {
         case BTA_HFP_SERVICE_ID:
         {
              btif_hf_execute_service(b_enable);
         }break;
         case BTA_A2DP_SERVICE_ID:
         {
              btif_av_execute_service(b_enable);
         }break;
         case BTA_HID_SERVICE_ID:
         {
              btif_hh_execute_service(b_enable);
         }

         default:
              BTIF_TRACE_ERROR1("%s: Unknown service being enabled", __FUNCTION__);
              return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         check_eir_remote_name
**
** Description      Check if remote name is in the EIR data
**
** Returns          TRUE if remote name found
**                  Populate p_remote_name, if provided and remote name found
**
*******************************************************************************/
static BOOLEAN check_eir_remote_name(tBTA_DM_SEARCH *p_search_data,
                            UINT8 *p_remote_name, UINT8 *p_remote_name_len)
{
    UINT8 *p_eir_remote_name = NULL;
    UINT8 remote_name_len = 0;

    /* Check EIR for remote name and services */
    if (p_search_data->inq_res.p_eir)
    {
        p_eir_remote_name = BTA_CheckEirData(p_search_data->inq_res.p_eir,
                BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);
        if (!p_eir_remote_name)
        {
            p_eir_remote_name = BTA_CheckEirData(p_search_data->inq_res.p_eir,
                    BTM_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
        }

        if (p_eir_remote_name)
        {
            if (remote_name_len > BD_NAME_LEN)
                remote_name_len = BD_NAME_LEN;

            if (p_remote_name && p_remote_name_len)
            {
                memcpy(p_remote_name, p_eir_remote_name, remote_name_len);
                *(p_remote_name + remote_name_len) = 0;
                *p_remote_name_len = remote_name_len;
            }

            return TRUE;
        }
    }

    return FALSE;

}

/*******************************************************************************
**
** Function         check_cached_remote_name
**
** Description      Check if remote name is in the NVRAM cache
**
** Returns          TRUE if remote name found
**                  Populate p_remote_name, if provided and remote name found
**
*******************************************************************************/
static BOOLEAN check_cached_remote_name(tBTA_DM_SEARCH *p_search_data,
                                UINT8 *p_remote_name, UINT8 *p_remote_name_len)
{
    bt_bdname_t bdname;
    bt_bdaddr_t remote_bdaddr;
    bt_property_t prop_name;

    /* check if we already have it in our btif_storage cache */
    bdcpy(remote_bdaddr.address, p_search_data->inq_res.bd_addr);
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, BT_PROPERTY_BDNAME,
                               sizeof(bt_bdname_t), &bdname);
    if (btif_storage_get_remote_device_property(
        &remote_bdaddr, &prop_name) == BT_STATUS_SUCCESS)
    {
        if (p_remote_name && p_remote_name_len)
        {
            strcpy((char *)p_remote_name, (char *)bdname.name);
            *p_remote_name_len = strlen((char *)p_remote_name);
        }
        return TRUE;
    }

    return FALSE;
}

BOOLEAN check_cod(const bt_bdaddr_t *remote_bdaddr, uint32_t cod)
{
    uint32_t    remote_cod;
    bt_property_t prop_name;

    /* check if we already have it in our btif_storage cache */
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, BT_PROPERTY_CLASS_OF_DEVICE,
                               sizeof(uint32_t), &remote_cod);
    if (btif_storage_get_remote_device_property((bt_bdaddr_t *)remote_bdaddr, &prop_name) == BT_STATUS_SUCCESS)
    {
        if ((remote_cod & 0x7ff) == cod)
            return TRUE;
    }

    return FALSE;
}

static void bond_state_changed(bt_status_t status, bt_bdaddr_t *bd_addr, bt_bond_state_t state)
{
    /* Send bonding state only once - based on outgoing/incoming we may receive duplicates */
    if ( (pairing_cb.state == state) && (state == BT_BOND_STATE_BONDING) )
        return;

    BTIF_TRACE_DEBUG3("%s: state=%d prev_state=%d", __FUNCTION__, state, pairing_cb.state);

    HAL_CBACK(bt_hal_cbacks, bond_state_changed_cb, status, bd_addr, state);

    if (state == BT_BOND_STATE_BONDING)
    {
        pairing_cb.state = state;
        bdcpy(pairing_cb.bd_addr, bd_addr->address);
    }
    else
        memset(&pairing_cb, 0, sizeof(pairing_cb));
}

/*******************************************************************************
**
** Function         hid_remote_name_cback
**
** Description      Remote name callback for HID device. Called in stack context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void hid_remote_name_cback(void *p_param)
{
    BTIF_TRACE_DEBUG1("%s", __FUNCTION__);

    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_HID_REMOTE_NAME,
        (char *)p_param, sizeof(tBTM_REMOTE_DEV_NAME), NULL);
}

/*******************************************************************************
**
** Function         btif_dm_cb_hid_remote_name
**
** Description      Remote name callback for HID device. Called in btif context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_cb_hid_remote_name(tBTM_REMOTE_DEV_NAME *p_remote_name)
{
    BTIF_TRACE_DEBUG3("%s: status=%d pairing_cb.state=%d", __FUNCTION__, p_remote_name->status, pairing_cb.state);
    if (pairing_cb.state == BT_BOND_STATE_BONDING)
    {
        bt_bdaddr_t remote_bd;

        bdcpy(remote_bd.address, pairing_cb.bd_addr);

        if (p_remote_name->status == BTM_SUCCESS)
        {
            bond_state_changed(BT_STATUS_SUCCESS, &remote_bd, BT_BOND_STATE_BONDED);
            //btif_hh_mouse_pairing(remote_bd);
        }
        else
            bond_state_changed(BT_STATUS_FAIL, &remote_bd, BT_BOND_STATE_NONE);
    }
}

/*******************************************************************************
**
** Function         btif_dm_cb_create_bond
**
** Description      Create bond initiated from the BTIF thread context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_cb_create_bond(bt_bdaddr_t *bd_addr)
{
    bond_state_changed(BT_STATUS_SUCCESS, bd_addr, BT_BOND_STATE_BONDING);

    /* TODO:
    **  1. Check if ACL is already up with this device
    **  2. Disable unpaired devices from connecting while we are bonding
    **  3. special handling for HID devices
    */
    if (check_cod(bd_addr, COD_HID_POINTING))
    {
        /* For HID mouse, no pairing required. Hence we fake the pairing process
             * by retrieving the remote device name.
             */
        BTIF_TRACE_DEBUG0("Create bond for hid mouse");
        tBTM_STATUS status;
        BD_ADDR bda;
        bdcpy(bda, bd_addr->address);

        BTIF_TRACE_DEBUG1("%s: Bypass bonding for HID Mouse", __FUNCTION__);
        status = BTM_ReadRemoteDeviceName(bda, hid_remote_name_cback);

        if (status != BTM_CMD_STARTED)
        {
            BTIF_TRACE_DEBUG2("%s: status = %d", __FUNCTION__, status);
            bond_state_changed(BT_STATUS_FAIL, bd_addr, BT_BOND_STATE_NONE);
            return;
        }
            /* Trigger SDP on the device */
        BTIF_TRACE_DEBUG1("%s:Starting SDP", __FUNCTION__);
        btif_dm_get_remote_services(bd_addr);
    }
    else
    {
        BTA_DmBond ((UINT8 *)bd_addr->address);
    }

}

/*******************************************************************************
**
** Function         btif_dm_cb_remove_bond
**
** Description      remove bond initiated from the BTIF thread context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_cb_remove_bond(bt_bdaddr_t *bd_addr)
{
     bdstr_t bdstr;
        /*special handling for HID devices */
#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))
    if (check_cod(bd_addr, COD_HID_POINTING )) {
        btif_hh_remove_device(*bd_addr);
        //todo
        //virtual_unplug(bd_addr);
        bond_state_changed(BT_STATUS_SUCCESS, bd_addr, BT_BOND_STATE_NONE);
        return;
    }
    else if (check_cod(bd_addr, COD_HID_KEYBOARD)|| check_cod(bd_addr, COD_HID_COMBO)) {
        btif_hh_remove_device(*bd_addr);
    }
#endif
    if (BTA_DmRemoveDevice((UINT8 *)bd_addr->address) == BTA_SUCCESS)
    {
        BTIF_TRACE_DEBUG1("Successfully removed bonding with device: %s",
                                            bd2str((bt_bdaddr_t *)bd_addr, &bdstr));
    }
    else
        BTIF_TRACE_DEBUG1("Removed bonding with device failed: %s",
                                            bd2str((bt_bdaddr_t *)bd_addr, &bdstr));


}

/*******************************************************************************
**
** Function         search_devices_copy_cb
**
** Description      Deep copy callback for search devices event
**
** Returns          void
**
*******************************************************************************/
static void search_devices_copy_cb(UINT16 event, char *p_dest, char *p_src)
{
    tBTA_DM_SEARCH *p_dest_data =  (tBTA_DM_SEARCH *) p_dest;
    tBTA_DM_SEARCH *p_src_data =  (tBTA_DM_SEARCH *) p_src;

    if (!p_src)
        return;

    BTIF_TRACE_DEBUG2("%s: event=%s", __FUNCTION__, dump_dm_search_event(event));
    memcpy(p_dest_data, p_src_data, sizeof(tBTA_DM_SEARCH));
    switch (event)
    {
        case BTA_DM_INQ_RES_EVT:
        {
            if (p_src_data->inq_res.p_eir)
            {
                p_dest_data->inq_res.p_eir = (UINT8 *)(p_dest + sizeof(tBTA_DM_SEARCH));
                memcpy(p_dest_data->inq_res.p_eir, p_src_data->inq_res.p_eir, HCI_EXT_INQ_RESPONSE_LEN);
            }
        }
        break;

        case BTA_DM_DISC_RES_EVT:
        {
            if (p_src_data->disc_res.raw_data_size && p_src_data->disc_res.p_raw_data)
            {
                p_dest_data->disc_res.p_raw_data = (UINT8 *)(p_dest + sizeof(tBTA_DM_SEARCH));
                memcpy(p_dest_data->disc_res.p_raw_data,
                    p_src_data->disc_res.p_raw_data, p_src_data->disc_res.raw_data_size);
            }
        }
        break;
    }
}

/******************************************************************************
**
**  BTIF DM callback events
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_pin_req_evt
**
** Description      Executes pin request event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_pin_req_evt(tBTA_DM_PIN_REQ *p_pin_req)
{
    bt_bdaddr_t bd_addr;
    bt_bdname_t bd_name;
    UINT32 cod;

    bdcpy(bd_addr.address, p_pin_req->bd_addr);
    memcpy(bd_name.name, p_pin_req->bd_name, BD_NAME_LEN);

    bond_state_changed(BT_STATUS_SUCCESS, &bd_addr, BT_BOND_STATE_BONDING);

    cod = devclass2uint(p_pin_req->dev_class);

    if ( cod == 0) {
        LOGD("cod is 0, set as unclassified");
        cod = COD_UNCLASSIFIED;
    }

    HAL_CBACK(bt_hal_cbacks, pin_request_cb,
                     &bd_addr, &bd_name, cod);
}

/*******************************************************************************
**
** Function         btif_dm_ssp_cfm_req_evt
**
** Description      Executes SSP confirm request event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_ssp_cfm_req_evt(tBTA_DM_SP_CFM_REQ *p_ssp_cfm_req)
{
    bt_bdaddr_t bd_addr;
    bt_bdname_t bd_name;
    UINT32 cod;
    BOOLEAN incoming_bonding;

    /* if bonding state is BT_BOND_STATE_BONDING, then we initiated it */
    incoming_bonding = !(pairing_cb.state == BT_BOND_STATE_BONDING);

    BTIF_TRACE_DEBUG1("%s", __FUNCTION__);
    bdcpy(bd_addr.address, p_ssp_cfm_req->bd_addr);
    memcpy(bd_name.name, p_ssp_cfm_req->bd_name, BD_NAME_LEN);

    /* Set the pairing_cb based on the local & remote authentication requirements */
    bond_state_changed(BT_STATUS_SUCCESS, &bd_addr, BT_BOND_STATE_BONDING);

    if ((p_ssp_cfm_req->loc_auth_req >= BTM_AUTH_AP_NO && p_ssp_cfm_req->rmt_auth_req >= BTM_AUTH_AP_NO) ||
        (p_ssp_cfm_req->loc_auth_req == BTM_AUTH_AP_NO || p_ssp_cfm_req->loc_auth_req == BTM_AUTH_AP_YES) ||
        (p_ssp_cfm_req->rmt_auth_req == BTM_AUTH_AP_NO || p_ssp_cfm_req->rmt_auth_req == BTM_AUTH_AP_YES))
        pairing_cb.is_temp = FALSE;
    else
        pairing_cb.is_temp = TRUE;

    /* If JustWorks auto-accept if we initiated bonding */
    if (p_ssp_cfm_req->just_works && !incoming_bonding)
    {
        BTIF_TRACE_EVENT1("%s: Auto-accept JustWorks pairing", __FUNCTION__);
        btif_dm_ssp_reply(&bd_addr, BT_SSP_VARIANT_CONSENT, TRUE, 0);
        return;
    }

    cod = devclass2uint(p_ssp_cfm_req->dev_class);

    if ( cod == 0) {
        LOGD("cod is 0, set as unclassified");
        cod = COD_UNCLASSIFIED;
    }

    /* TODO: pairing variant passkey_entry? */
    HAL_CBACK(bt_hal_cbacks, ssp_request_cb, &bd_addr, &bd_name,
                     cod, p_ssp_cfm_req->just_works ? BT_SSP_VARIANT_CONSENT : BT_SSP_VARIANT_PASSKEY_CONFIRMATION,
                     p_ssp_cfm_req->num_val);
}

static void btif_dm_ssp_key_notif_evt(tBTA_DM_SP_KEY_NOTIF *p_ssp_key_notif)
{
    bt_bdaddr_t bd_addr;
    bt_bdname_t bd_name;
    UINT32 cod;

    BTIF_TRACE_DEBUG1("%s", __FUNCTION__);
    bdcpy(bd_addr.address, p_ssp_key_notif->bd_addr);
    memcpy(bd_name.name, p_ssp_key_notif->bd_name, BD_NAME_LEN);

    bond_state_changed(BT_STATUS_SUCCESS, &bd_addr, BT_BOND_STATE_BONDING);

    cod = devclass2uint(p_ssp_key_notif->dev_class);

    if ( cod == 0) {
        LOGD("cod is 0, set as unclassified");
        cod = COD_UNCLASSIFIED;
    }

    HAL_CBACK(bt_hal_cbacks, ssp_request_cb, &bd_addr, &bd_name,
                     cod, BT_SSP_VARIANT_PASSKEY_NOTIFICATION,
                     p_ssp_key_notif->passkey);
}
/*******************************************************************************
**
** Function         btif_dm_auth_cmpl_evt
**
** Description      Executes authentication complete event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_auth_cmpl_evt (tBTA_DM_AUTH_CMPL *p_auth_cmpl)
{
    /* Save link key, if not temporary */
    bt_bdaddr_t bd_addr;
    bt_status_t status = BT_STATUS_FAIL;
    bt_bond_state_t state = BT_BOND_STATE_NONE;

    bdcpy(bd_addr.address, p_auth_cmpl->bd_addr);
    if ( (p_auth_cmpl->success == TRUE) && (p_auth_cmpl->key_present) )
    {
        if ((p_auth_cmpl->key_type < HCI_LKEY_TYPE_DEBUG_COMB)  || (p_auth_cmpl->key_type == HCI_LKEY_TYPE_AUTH_COMB) ||
            (p_auth_cmpl->key_type == HCI_LKEY_TYPE_CHANGED_COMB) || (!pairing_cb.is_temp))
        {
            bt_status_t ret;
            BTIF_TRACE_DEBUG3("%s: Storing link key. key_type=0x%x, is_temp=%d",
                __FUNCTION__, p_auth_cmpl->key_type, pairing_cb.is_temp);
            ret = btif_storage_add_bonded_device(&bd_addr,
                                p_auth_cmpl->key, p_auth_cmpl->key_type,
                                pairing_cb.pin_code_len);
            ASSERTC(ret == BT_STATUS_SUCCESS, "storing link key failed", ret);
        }
        else
        {
            BTIF_TRACE_DEBUG3("%s: Temporary key. Not storing. key_type=0x%x, is_temp=%d",
                __FUNCTION__, p_auth_cmpl->key_type, pairing_cb.is_temp);
        }
    }

    if (p_auth_cmpl->success)
    {
        status = BT_STATUS_SUCCESS;
        state = BT_BOND_STATE_BONDED;

        /* Trigger SDP on the device */
        btif_dm_get_remote_services(&bd_addr);
    }

    bond_state_changed(status, &bd_addr, state);
}

/******************************************************************************
**
** Function         btif_dm_search_devices_evt
**
** Description      Executes search devices callback events in btif context
**
** Returns          void
**
******************************************************************************/
static void btif_dm_search_devices_evt (UINT16 event, char *p_param)
{
    tBTA_DM_SEARCH *p_search_data;
    BTIF_TRACE_EVENT2("%s event=%s", __FUNCTION__, dump_dm_search_event(event));

    switch (event)
    {
        case BTA_DM_DISC_RES_EVT:
        {
            p_search_data = (tBTA_DM_SEARCH *)p_param;
            /* Remote name update */
            if (strlen((const char *) p_search_data->disc_res.bd_name))
            {
                bt_property_t properties[1];
                bt_bdaddr_t bdaddr;
                bt_status_t status;

                properties[0].type = BT_PROPERTY_BDNAME;
                properties[0].val = p_search_data->disc_res.bd_name;
                properties[0].len = strlen((char *)p_search_data->disc_res.bd_name)+1;
                bdcpy(bdaddr.address, p_search_data->disc_res.bd_addr);

                status = btif_storage_set_remote_device_property(&bdaddr, &properties[0]);
                ASSERTC(status == BT_STATUS_SUCCESS, "failed to save remote device property", status);
                HAL_CBACK(bt_hal_cbacks, remote_device_properties_cb,
                                 status, &bdaddr, 1, properties);
            }
            /* TODO: Services? */
        }
        break;

        case BTA_DM_INQ_RES_EVT:
        {
            /* inquiry result */
            UINT32 cod;
            UINT8 *p_eir_remote_name = NULL;
            bt_bdname_t bdname;
            bt_bdaddr_t bdaddr;
            UINT8 remote_name_len;
            UINT8 *p_cached_name = NULL;
            tBTA_SERVICE_MASK services = 0;
            bdstr_t bdstr;

            p_search_data = (tBTA_DM_SEARCH *)p_param;
            bdcpy(bdaddr.address, p_search_data->inq_res.bd_addr);

            BTIF_TRACE_DEBUG3("%s() %s device_type = 0x%x\n", __FUNCTION__, bd2str(&bdaddr, &bdstr),
#if (BLE_INCLUDED == TRUE)
                    p_search_data->inq_res.device_type);
#else
                    BT_DEVICE_TYPE_BREDR);
#endif
            bdname.name[0] = 0;

            cod = devclass2uint (p_search_data->inq_res.dev_class);

            if ( cod == 0) {
                LOGD("cod is 0, set as unclassified");
                cod = COD_UNCLASSIFIED;
            }

            if (!check_eir_remote_name(p_search_data, bdname.name, &remote_name_len))
                check_cached_remote_name(p_search_data, bdname.name, &remote_name_len);

            /* Check EIR for remote name and services */
            if (p_search_data->inq_res.p_eir)
            {
                BTA_GetEirService(p_search_data->inq_res.p_eir, &services);
                BTIF_TRACE_DEBUG2("%s()EIR BTA services = %08X", __FUNCTION__, (UINT32)services);
                /* TODO:  Get the service list and check to see which uuids we got and send it back to the client. */
            }


            {
                bt_property_t properties[5];
                bt_device_type_t dev_type;
                uint32_t num_properties = 0;
                bt_status_t status;

                memset(properties, 0, sizeof(properties));
                /* BD_ADDR */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                    BT_PROPERTY_BDADDR, sizeof(bdaddr), &bdaddr);
                num_properties++;
                /* BD_NAME */
                /* Don't send BDNAME if it is empty */
                if (bdname.name[0]) {
                    BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                               BT_PROPERTY_BDNAME,
                                               strlen((char *)bdname.name)+1, &bdname);
                    num_properties++;
                }

                /* DEV_CLASS */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                    BT_PROPERTY_CLASS_OF_DEVICE, sizeof(cod), &cod);
                num_properties++;
                /* DEV_TYPE */
#if (BLE_INCLUDED == TRUE)
                /* FixMe: Assumption is that bluetooth.h and BTE enums match */
                dev_type = p_search_data->inq_res.device_type;
#else
                dev_type = BT_DEVICE_TYPE_BREDR;
#endif
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                    BT_PROPERTY_TYPE_OF_DEVICE, sizeof(dev_type), &dev_type);
                num_properties++;
                /* RSSI */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                    BT_PROPERTY_REMOTE_RSSI, sizeof(int8_t),
                                    &(p_search_data->inq_res.rssi));
                num_properties++;

                status = btif_storage_add_remote_device(&bdaddr, num_properties, properties);
                ASSERTC(status == BT_STATUS_SUCCESS, "failed to save remote device (inquiry)", status);

                /* Callback to notify upper layer of device */
                HAL_CBACK(bt_hal_cbacks, device_found_cb,
                                 num_properties, properties);
            }
        }
        break;

        case BTA_DM_INQ_CMPL_EVT:
        {
        }
        break;

        case BTA_DM_DISC_CMPL_EVT:
        case BTA_DM_SEARCH_CANCEL_CMPL_EVT:
        {
            HAL_CBACK(bt_hal_cbacks, discovery_state_changed_cb, BT_DISCOVERY_STOPPED);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_search_services_evt
**
** Description      Executes search services event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_search_services_evt(UINT16 event, char *p_param)
{
    tBTA_DM_SEARCH *p_data = (tBTA_DM_SEARCH*)p_param;

    BTIF_TRACE_EVENT2("%s:  event = %d", __FUNCTION__, event);
    switch (event)
    {
        case BTA_DM_DISC_RES_EVT:
        {
            bt_uuid_t uuid_arr[BT_MAX_NUM_UUIDS]; /* Max 32 services */
            bt_property_t prop;
            uint32_t i = 0,  j = 0;
            bt_bdaddr_t bd_addr;
            bt_status_t ret;

            bdcpy(bd_addr.address, p_data->disc_res.bd_addr);

            BTIF_TRACE_DEBUG3("%s:(result=0x%x, services 0x%x)", __FUNCTION__,
                    p_data->disc_res.result, p_data->disc_res.services);
            prop.type = BT_PROPERTY_UUIDS;
            prop.val = (void*)uuid_arr;
            prop.len = 0;
            for (i=0; i < BTA_MAX_SERVICE_ID; i++)
            {
                if(p_data->disc_res.services
                    &(tBTA_SERVICE_MASK)(BTA_SERVICE_ID_TO_SERVICE_MASK(i)))
                {
                     memset(&uuid_arr[j], 0, sizeof(bt_uuid_t));
                     uuid16_to_uuid128(bta_service_id_to_uuid_lkup_tbl[i], &uuid_arr[j]);
                     prop.len += sizeof(bt_uuid_t);
                     j++;
                }
            }
            /* Also write this to the NVRAM */
            ret = btif_storage_set_remote_device_property(&bd_addr, &prop);
            ASSERTC(ret == BT_STATUS_SUCCESS, "storing remote services failed", ret);

            /* Send the event to the BTIF */
            HAL_CBACK(bt_hal_cbacks, remote_device_properties_cb,
                             BT_STATUS_SUCCESS, &bd_addr, 1, &prop);
        }
        break;

        case BTA_DM_DISC_CMPL_EVT:
            /* fixme */
        break;

        default:
        {
            ASSERTC(0, "unhandled search services event", event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_remote_service_record_evt
**
** Description      Executes search service record event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_remote_service_record_evt(UINT16 event, char *p_param)
{
    tBTA_DM_SEARCH *p_data = (tBTA_DM_SEARCH*)p_param;

    BTIF_TRACE_EVENT2("%s:  event = %d", __FUNCTION__, event);
    switch (event)
    {
        case BTA_DM_DISC_RES_EVT:
        {
            bt_service_record_t rec;
            bt_property_t prop;
            uint32_t i = 0;
            bt_bdaddr_t bd_addr;

            memset(&rec, 0, sizeof(bt_service_record_t));
            bdcpy(bd_addr.address, p_data->disc_res.bd_addr);

            BTIF_TRACE_DEBUG3("%s:(result=0x%x, services 0x%x)", __FUNCTION__,
                    p_data->disc_res.result, p_data->disc_res.services);
            prop.type = BT_PROPERTY_SERVICE_RECORD;
            prop.val = (void*)&rec;
            prop.len = sizeof(rec);

            /* disc_res.result is overloaded with SCN. Cannot check result */
            p_data->disc_res.services &= ~BTA_USER_SERVICE_MASK;
            /* TODO: Get the UUID as well */
            rec.channel = p_data->disc_res.result - 3;
            /* TODO: Need to get the service name using p_raw_data */
            rec.name[0] = 0;

            HAL_CBACK(bt_hal_cbacks, remote_device_properties_cb,
                             BT_STATUS_SUCCESS, &bd_addr, 1, &prop);
        }
        break;

        default:
        {
           ASSERTC(0, "unhandled remote service record event", event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_upstreams_cback
**
** Description      Executes UPSTREAMS events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_upstreams_evt(UINT16 event, char* p_param)
{
    tBTA_DM_SEC_EVT dm_event = (tBTA_DM_SEC_EVT)event;
    tBTA_DM_SEC *p_data = (tBTA_DM_SEC*)p_param;
    tBTA_SERVICE_MASK service_mask;
    uint32_t i;
    bt_bdaddr_t bd_addr;

    BTIF_TRACE_EVENT1("btif_dm_upstreams_cback  ev: %d", event);

    switch (event)
    {
        case BTA_DM_ENABLE_EVT:
        {
             BD_NAME bdname;
             bt_status_t status;
             bt_property_t prop;
             prop.type = BT_PROPERTY_BDNAME;
             prop.len = 0;
             prop.val = (void*)bdname;

             status = btif_storage_get_adapter_property(&prop);
             /* Storage does not have a name yet.
             ** Use the default name and write it to the chip
             */
             if (status != BT_STATUS_SUCCESS)
             {
                 BTA_DmSetDeviceName((char *)BTM_DEF_LOCAL_NAME);
                 /* Hmmm...Should we store this too??? */
             }
             else
             {
                 /* A name exists in the storage. Make this the device name */
                 BTA_DmSetDeviceName((char*)prop.val);
             }

             /* for each of the enabled services in the mask, trigger the profile
              * enable */
             service_mask = btif_get_enabled_services_mask();
             for (i=0; i <= BTA_MAX_SERVICE_ID; i++)
             {
                 if (service_mask &
                     (tBTA_SERVICE_MASK)(BTA_SERVICE_ID_TO_SERVICE_MASK(i)))
                 {
                     btif_in_execute_service_request(i, TRUE);
                 }
             }
             /* This function will also trigger the adapter_properties_cb
             ** and bonded_devices_info_cb
             */
             btif_storage_load_bonded_devices();

             btif_enable_bluetooth_evt(p_data->enable.status, p_data->enable.bd_addr);
        }
        break;

        case BTA_DM_DISABLE_EVT:
            /* for each of the enabled services in the mask, trigger the profile
             * disable */
            service_mask = btif_get_enabled_services_mask();
            for (i=0; i <= BTA_MAX_SERVICE_ID; i++)
            {
                if (service_mask &
                    (tBTA_SERVICE_MASK)(BTA_SERVICE_ID_TO_SERVICE_MASK(i)))
                {
                    btif_in_execute_service_request(i, FALSE);
                }
            }
            btif_disable_bluetooth_evt();
            break;

        case BTA_DM_PIN_REQ_EVT:
            btif_dm_pin_req_evt(&p_data->pin_req);
            break;

        case BTA_DM_AUTH_CMPL_EVT:
            btif_dm_auth_cmpl_evt(&p_data->auth_cmpl);
            break;

        case BTA_DM_SP_CFM_REQ_EVT:
            btif_dm_ssp_cfm_req_evt(&p_data->cfm_req);
            break;
        case BTA_DM_SP_KEY_NOTIF_EVT:
            btif_dm_ssp_key_notif_evt(&p_data->key_notif);
            break;

        case BTA_DM_DEV_UNPAIRED_EVT:
            bdcpy(bd_addr.address, p_data->link_down.bd_addr);
            btif_storage_remove_bonded_device(&bd_addr);
            bond_state_changed(BT_STATUS_SUCCESS, &bd_addr, BT_BOND_STATE_NONE);
            break;

        case BTA_DM_AUTHORIZE_EVT:
        case BTA_DM_LINK_DOWN_EVT:
        case BTA_DM_SIG_STRENGTH_EVT:
        case BTA_DM_BUSY_LEVEL_EVT:
        case BTA_DM_BOND_CANCEL_CMPL_EVT:
        case BTA_DM_SP_RMT_OOB_EVT:
        case BTA_DM_SP_KEYPRESS_EVT:
        case BTA_DM_ROLE_CHG_EVT:
        case BTA_DM_BLE_KEY_EVT:
        case BTA_DM_BLE_SEC_REQ_EVT:
        case BTA_DM_BLE_PASSKEY_NOTIF_EVT:
        case BTA_DM_BLE_PASSKEY_REQ_EVT:
        case BTA_DM_BLE_OOB_REQ_EVT:
        case BTA_DM_BLE_LOCAL_IR_EVT:
        case BTA_DM_BLE_LOCAL_ER_EVT:
        case BTA_DM_BLE_AUTH_CMPL_EVT:
        default:
            BTIF_TRACE_WARNING1( "btif_dm_cback : unhandled event (%d)", event );
            break;
    }
} /* btui_security_cback() */


/*******************************************************************************
**
** Function         btif_dm_generic_evt
**
** Description      Executes non-BTA upstream events in BTIF context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_generic_evt(UINT16 event, char* p_param)
{
    BTIF_TRACE_EVENT2("%s: event=%d", __FUNCTION__, event);
    switch(event)
    {
        case BTIF_DM_CB_DISCOVERY_STARTED:
        {
            HAL_CBACK(bt_hal_cbacks, discovery_state_changed_cb, BT_DISCOVERY_STARTED);
        }
        break;

        case BTIF_DM_CB_CREATE_BOND:
        {
            btif_dm_cb_create_bond((bt_bdaddr_t *)p_param);
        }
        break;

        case BTIF_DM_CB_REMOVE_BOND:
        {
            btif_dm_cb_remove_bond((bt_bdaddr_t *)p_param);
        }
        break;

        case BTIF_DM_CB_HID_REMOTE_NAME:
        {
            btif_dm_cb_hid_remote_name((tBTM_REMOTE_DEV_NAME *)p_param);
        }
        break;

        default:
        {
            BTIF_TRACE_WARNING2("%s : Unknown event 0x%x", __FUNCTION__, event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         bte_dm_evt
**
** Description      Switches context from BTE to BTIF for all DM events
**
** Returns          void
**
*******************************************************************************/

void bte_dm_evt(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{
    bt_status_t status;

    /* switch context to btif task context (copy full union size for convenience) */
    status = btif_transfer_context(btif_dm_upstreams_evt, (uint16_t)event, (void*)p_data, sizeof(tBTA_DM_SEC), NULL);

    /* catch any failed context transfers */
    ASSERTC(status == BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         bte_search_devices_evt
**
** Description      Switches context from BTE to BTIF for DM search events
**
** Returns          void
**
*******************************************************************************/
static void bte_search_devices_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    UINT16 param_len = 0;

    if (p_data)
        param_len += sizeof(tBTA_DM_SEARCH);
    /* Allocate buffer to hold the pointers (deep copy). The pointers will point to the end of the tBTA_DM_SEARCH */
    switch (event)
    {
        case BTA_DM_INQ_RES_EVT:
        {
            if (p_data->inq_res.p_eir)
                param_len += HCI_EXT_INQ_RESPONSE_LEN;
        }
        break;

        case BTA_DM_DISC_RES_EVT:
        {
            if (p_data->disc_res.raw_data_size && p_data->disc_res.p_raw_data)
                param_len += p_data->disc_res.raw_data_size;
        }
        break;
    }
    BTIF_TRACE_DEBUG3("%s event=%s param_len=%d", __FUNCTION__, dump_dm_search_event(event), param_len);

    /* if remote name is available in EIR, set teh flag so that stack doesnt trigger RNR */
    if (event == BTA_DM_INQ_RES_EVT)
        p_data->inq_res.remt_name_not_required = check_eir_remote_name(p_data, NULL, NULL);

    btif_transfer_context (btif_dm_search_devices_evt , (UINT16) event, (void *)p_data, param_len,
        (param_len > sizeof(tBTA_DM_SEARCH)) ? search_devices_copy_cb : NULL);
}

/*******************************************************************************
**
** Function         bte_dm_search_services_evt
**
** Description      Switches context from BTE to BTIF for DM search services
**                  event
**
** Returns          void
**
*******************************************************************************/
static void bte_dm_search_services_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
   /* TODO: The only member that needs a deep copy is the p_raw_data. But not sure yet if this is needed. */
   btif_transfer_context(btif_dm_search_services_evt, event, (char*)p_data, sizeof(tBTA_DM_SEARCH), NULL);
}

/*******************************************************************************
**
** Function         bte_dm_remote_service_record_evt
**
** Description      Switches context from BTE to BTIF for DM search service
**                  record event
**
** Returns          void
**
*******************************************************************************/
static void bte_dm_remote_service_record_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
   /* TODO: The only member that needs a deep copy is the p_raw_data. But not sure yet if this is needed. */
   btif_transfer_context(btif_dm_remote_service_record_evt, event, (char*)p_data, sizeof(tBTA_DM_SEARCH), NULL);
}

/*****************************************************************************
**
**   btif api functions (no context switch)
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_start_discovery
**
** Description      Start device discovery/inquiry
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_start_discovery(void)
{
    tBTA_DM_INQ inq_params;
    tBTA_SERVICE_MASK services = 0;

    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    /* TODO: Do we need to handle multiple inquiries at the same time? */

    /* Set inquiry params and call API */
#if (BLE_INCLUDED == TRUE)
    inq_params.mode = BTA_DM_GENERAL_INQUIRY|BTA_BLE_GENERAL_INQUIRY;
#else
    inq_params.mode = BTA_DM_GENERAL_INQUIRY;
#endif
    inq_params.duration = BTIF_DM_DEFAULT_INQ_MAX_DURATION;

    inq_params.max_resps = BTIF_DM_DEFAULT_INQ_MAX_RESULTS;
    inq_params.report_dup = TRUE;

    inq_params.filter_type = BTA_DM_INQ_CLR;
    /* TODO: Filter device by BDA needs to be implemented here */

    /* find nearby devices */
    BTA_DmSearch(&inq_params, services, bte_search_devices_evt);

    /* Invoke the discovery_started callback */
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_DISCOVERY_STARTED, NULL, 0, NULL);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_cancel_discovery
**
** Description      Cancels search
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_cancel_discovery(void)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    BTA_DmSearchCancel();
    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_create_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_create_bond(const bt_bdaddr_t *bd_addr)
{
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: bd_addr=%s", __FUNCTION__, bd2str((bt_bdaddr_t *) bd_addr, &bdstr));

    if (pairing_cb.state != BT_BOND_STATE_NONE)
        return BT_STATUS_BUSY;

    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_CREATE_BOND,
                          (char *)bd_addr, sizeof(bt_bdaddr_t), NULL);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_cancel_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/

bt_status_t btif_dm_cancel_bond(const bt_bdaddr_t *bd_addr)
{
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: bd_addr=%s", __FUNCTION__, bd2str((bt_bdaddr_t *)bd_addr, &bdstr));

    /* TODO:
    **  1. Check if ACL is already up with this device
    **  2. Restore scan modes
    **  3. special handling for HID devices
    */
    BTA_DmBondCancel ((UINT8 *)bd_addr->address);
    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_remove_bond
**
** Description      Removes bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/

bt_status_t btif_dm_remove_bond(const bt_bdaddr_t *bd_addr)
{
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: bd_addr=%s", __FUNCTION__, bd2str((bt_bdaddr_t *)bd_addr, &bdstr));
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_REMOVE_BOND,
                          (char *)bd_addr, sizeof(bt_bdaddr_t), NULL);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_pin_reply
**
** Description      BT legacy pairing - PIN code reply
**
** Returns          bt_status_t
**
*******************************************************************************/

bt_status_t btif_dm_pin_reply( const bt_bdaddr_t *bd_addr, uint8_t accept,
                               uint8_t pin_len, bt_pin_code_t *pin_code)
{
    BTIF_TRACE_EVENT2("%s: accept=%d", __FUNCTION__, accept);

    BTA_DmPinReply( (UINT8 *)bd_addr->address, accept, pin_len, pin_code->pin);

    if (accept)
        pairing_cb.pin_code_len = pin_len;

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_ssp_reply
**
** Description      BT SSP Reply - Just Works, Numeric Comparison & Passkey Entry
**
** Returns          bt_status_t
**
*******************************************************************************/

bt_status_t btif_dm_ssp_reply(const bt_bdaddr_t *bd_addr,
                                 bt_ssp_variant_t variant, uint8_t accept,
                                 uint32_t passkey)
{
    if (variant == BT_SSP_VARIANT_PASSKEY_ENTRY)
    {
        /* This is not implemented in the stack.
         * For devices with display, this is not needed
        */
        BTIF_TRACE_WARNING1("%s: Not implemented", __FUNCTION__);
        return BT_STATUS_FAIL;
    }
    /* BT_SSP_VARIANT_CONSENT & BT_SSP_VARIANT_PASSKEY_CONFIRMATION supported */
    BTIF_TRACE_EVENT2("%s: accept=%d", __FUNCTION__, accept);
    BTA_DmConfirm( (UINT8 *)bd_addr->address, accept);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_adapter_property
**
** Description     Queries the BTA for the adapter property
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_get_adapter_property(bt_property_t *prop)
{
    bt_status_t status;

    BTIF_TRACE_EVENT2("%s: type=0x%x", __FUNCTION__, prop->type);
    switch (prop->type)
    {
        case BT_PROPERTY_BDNAME:
        {
            bt_bdname_t *bd_name = (bt_bdname_t*)prop->val;
            strcpy((char *)bd_name->name, (char *)BTM_DEF_LOCAL_NAME);
            prop->len = strlen((char *)bd_name->name)+1;
        }
        break;

        case BT_PROPERTY_ADAPTER_SCAN_MODE:
        {
            /* if the storage does not have it. Most likely app never set it. Default is NONE */
            bt_scan_mode_t *mode = (bt_scan_mode_t*)prop->val;
            *mode = BT_SCAN_MODE_NONE;
            prop->len = sizeof(bt_scan_mode_t);
        }
        break;

        case BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT:
        {
            uint32_t *tmt = (uint32_t*)prop->val;
            *tmt = 0;
            prop->len = sizeof(uint32_t);
        }
        break;

        default:
            prop->len = 0;
            return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_get_remote_services(bt_bdaddr_t *remote_addr)
{
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: remote_addr=%s", __FUNCTION__, bd2str(remote_addr, &bdstr));

    BTA_DmDiscover(remote_addr->address, BTA_ALL_SERVICE_MASK,
                   bte_dm_search_services_evt, TRUE);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_remote_service_record
**
** Description      Start SDP to get remote service record
**
**
** Returns          bt_status_t
*******************************************************************************/
bt_status_t btif_dm_get_remote_service_record(bt_bdaddr_t *remote_addr,
                                                    bt_uuid_t *uuid)
{
    tSDP_UUID sdp_uuid;
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: remote_addr=%s", __FUNCTION__, bd2str(remote_addr, &bdstr));

    sdp_uuid.len = MAX_UUID_SIZE;
    memcpy(sdp_uuid.uu.uuid128, uuid->uu, MAX_UUID_SIZE);

    BTA_DmDiscoverUUID(remote_addr->address, &sdp_uuid,
                       bte_dm_remote_service_record_evt, TRUE);

    return BT_STATUS_SUCCESS;
}

void btif_dm_execute_service_request(UINT16 event, char *p_param)
{
    BOOLEAN b_enable = FALSE;
    bt_status_t status;
    if (event == BTIF_DM_ENABLE_SERVICE)
    {
        b_enable = TRUE;
    }
    status = btif_in_execute_service_request(*((tBTA_SERVICE_ID*)p_param), b_enable);
    if (status == BT_STATUS_SUCCESS)
    {
        bt_property_t property;
        bt_uuid_t local_uuids[BT_MAX_NUM_UUIDS];
         /* Now send the UUID_PROPERTY_CHANGED event to the upper layer */
        BTIF_STORAGE_FILL_PROPERTY(&property, BT_PROPERTY_UUIDS,
                                    sizeof(local_uuids), local_uuids);
         btif_storage_get_adapter_property(&property);
         HAL_CBACK(bt_hal_cbacks, adapter_properties_cb,
                          BT_STATUS_SUCCESS, 1, &property);
    }
    return;
}
