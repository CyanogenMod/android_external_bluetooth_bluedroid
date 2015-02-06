/******************************************************************************
 *
 *  Copyright (C) 2003-2014 Broadcom Corporation
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

/*****************************************************************************
**
**  Name:          vendor_ble.c
**
**  Description:   This file contains vendor specific feature for BLE
**
******************************************************************************/
#include <string.h>
#include "bt_target.h"

#if (BLE_INCLUDED == TRUE)
#include "bt_types.h"
#include "hcimsgs.h"
#include "btu.h"
#include "vendor_ble.h"
#include "vendor_hcidefs.h"
#include "gatt_int.h"

/*** This needs to be moved to a VSC control block eventually per coding conventions ***/
#if VENDOR_DYNAMIC_MEMORY == FALSE
tBTM_BLE_VENDOR_CB  btm_ble_vendor_cb;
#endif

static const BD_ADDR     na_bda= {0};

/*******************************************************************************
**         Resolve Address Using IRK List functions
*******************************************************************************/


/*******************************************************************************
**
** Function         btm_ble_vendor_enq_irk_pending
**
** Description      add target address into IRK pending operation queue
**
** Parameters       target_bda: target device address
**                  add_entry: TRUE for add entry, FALSE for remove entry
**
** Returns          void
**
*******************************************************************************/
void btm_ble_vendor_enq_irk_pending(BD_ADDR target_bda, BD_ADDR psuedo_bda, UINT8 to_add)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_pend_q.irk_q == 0 || btm_ble_vendor_cb.irk_pend_q.irk_q == NULL)
        return;
    tBTM_BLE_IRK_Q          *p_q = &btm_ble_vendor_cb.irk_pend_q;

    memcpy(p_q->irk_q[p_q->q_next], target_bda, BD_ADDR_LEN);
    memcpy(p_q->irk_q_random_pseudo[p_q->q_next], psuedo_bda, BD_ADDR_LEN);
    p_q->irk_q_action[p_q->q_next] = to_add;

    p_q->q_next ++;
    p_q->q_next %= btm_cb.cmn_ble_vsc_cb.max_irk_list_sz;
#endif
    return ;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_find_irk_pending_entry
**
** Description      check to see if the action is in pending list
**
** Parameters       TRUE: action pending;
**                  FALSE: new action
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btm_ble_vendor_find_irk_pending_entry(BD_ADDR psuedo_addr, UINT8 action)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_pend_q.irk_q == 0 || btm_ble_vendor_cb.irk_pend_q.irk_q == NULL)
        return FALSE;
    tBTM_BLE_IRK_Q          *p_q = &btm_ble_vendor_cb.irk_pend_q;
    UINT8   i;

    for (i = p_q->q_pending; i != p_q->q_next; )
    {
        if (memcmp(p_q->irk_q_random_pseudo[i], psuedo_addr, BD_ADDR_LEN) == 0 &&
            action == p_q->irk_q_action[i])
            return TRUE;

        i ++;
        i %= btm_cb.cmn_ble_vsc_cb.max_irk_list_sz;
    }
#endif
    return FALSE;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_deq_irk_pending
**
** Description      add target address into IRK pending operation queue
**
** Parameters       target_bda: target device address
**                  add_entry: TRUE for add entry, FALSE for remove entry
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btm_ble_vendor_deq_irk_pending(BD_ADDR target_bda, BD_ADDR psuedo_addr)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_pend_q.irk_q == 0 || btm_ble_vendor_cb.irk_pend_q.irk_q == NULL)
        return FALSE;
    tBTM_BLE_IRK_Q          *p_q = &btm_ble_vendor_cb.irk_pend_q;

    if (p_q->q_next != p_q->q_pending)
    {
        memcpy(target_bda, p_q->irk_q[p_q->q_pending], BD_ADDR_LEN);
        memcpy(psuedo_addr, p_q->irk_q_random_pseudo[p_q->q_pending], BD_ADDR_LEN);

        p_q->q_pending ++;
        p_q->q_pending %= btm_cb.cmn_ble_vsc_cb.max_irk_list_sz;

        return TRUE;
    }
#endif
    return FALSE;

}
/*******************************************************************************
**
** Function         btm_ble_vendor_find_irk_entry
**
** Description      find IRK entry in local host IRK list by static address
**
** Returns          IRK list entry pointer
**
*******************************************************************************/
tBTM_BLE_IRK_ENTRY * btm_ble_vendor_find_irk_entry(BD_ADDR target_bda)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_list == 0 || btm_ble_vendor_cb.irk_list == NULL)
        return NULL;

    tBTM_BLE_IRK_ENTRY  *p_irk_entry = &btm_ble_vendor_cb.irk_list[0];
    UINT8   i;

    for (i = 0; i < btm_cb.cmn_ble_vsc_cb.max_irk_list_sz; i ++, p_irk_entry++)
    {
        if (p_irk_entry->in_use && memcmp(p_irk_entry->bd_addr, target_bda, BD_ADDR_LEN) == 0)
        {
            return p_irk_entry ;
        }
    }
#endif
    return NULL;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_find_irk_entry_by_psuedo_addr
**
** Description      find IRK entry in local host IRK list by psuedo address
**
** Returns          IRK list entry pointer
**
*******************************************************************************/
tBTM_BLE_IRK_ENTRY * btm_ble_vendor_find_irk_entry_by_psuedo_addr (BD_ADDR psuedo_bda)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_list == 0 || btm_ble_vendor_cb.irk_list == NULL)
        return NULL;

    tBTM_BLE_IRK_ENTRY  *p_irk_entry = &btm_ble_vendor_cb.irk_list[0];
    UINT8   i;

    if(p_irk_entry == NULL)
        return NULL;

    for (i = 0; i < btm_cb.cmn_ble_vsc_cb.max_irk_list_sz; i ++, p_irk_entry++)
    {
        if (p_irk_entry->in_use && memcmp(p_irk_entry->psuedo_bda, psuedo_bda, BD_ADDR_LEN) == 0)
        {
            return p_irk_entry ;
        }
    }
#endif
    return NULL;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_alloc_irk_entry
**
** Description      allocate IRK entry in local host IRK list
**
** Returns          IRK list index
**
*******************************************************************************/
UINT8 btm_ble_vendor_alloc_irk_entry(BD_ADDR target_bda, BD_ADDR pseudo_bda)
{
#if BLE_PRIVACY_SPT == TRUE
    if(btm_ble_vendor_cb.irk_list == 0 || btm_ble_vendor_cb.irk_list == NULL)
        return BTM_CS_IRK_LIST_INVALID;

    tBTM_BLE_IRK_ENTRY  *p_irk_entry = &btm_ble_vendor_cb.irk_list[0];
    UINT8   i;

    for (i = 0; i < btm_cb.cmn_ble_vsc_cb.max_irk_list_sz; i ++, p_irk_entry++)
    {
        if (!p_irk_entry->in_use)
        {
            memcpy(p_irk_entry->bd_addr, target_bda, BD_ADDR_LEN);
            memcpy(p_irk_entry->psuedo_bda, pseudo_bda, BD_ADDR_LEN);

            p_irk_entry->index = i;
            p_irk_entry->in_use = TRUE;

            return i;
        }
    }
#endif
    return BTM_CS_IRK_LIST_INVALID;
}

/*******************************************************************************
**
** Function         btm_ble_vendor_update_irk_list
**
** Description      update IRK entry in local host IRK list
**
** Returns          void
**
*******************************************************************************/
void btm_ble_vendor_update_irk_list(BD_ADDR target_bda, BD_ADDR pseudo_bda, BOOLEAN add)
{
#if BLE_PRIVACY_SPT == TRUE
    tBTM_BLE_IRK_ENTRY   *p_irk_entry = btm_ble_vendor_find_irk_entry(target_bda);
    UINT8       i;

    if (add)
    {
        if (p_irk_entry == NULL)
        {
            if ((i = btm_ble_vendor_alloc_irk_entry(target_bda, pseudo_bda)) == BTM_CS_IRK_LIST_INVALID)
            {
                BTM_TRACE_ERROR("max IRK capacity reached");
            }
        }
        else
        {
            BTM_TRACE_WARNING(" IRK already in queue");
        }
    }
    else
    {
        if (p_irk_entry != NULL)
        {
            memset(p_irk_entry, 0, sizeof(tBTM_BLE_IRK_ENTRY));
        }
        else
        {
            BTM_TRACE_ERROR("No IRK exist in list, can not remove");
        }
    }
#endif
    return ;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_irk_vsc_op_cmpl
**
** Description      IRK operation VSC complete handler
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void btm_ble_vendor_irk_vsc_op_cmpl (tBTM_VSC_CMPL *p_params)
{
    UINT8  status;
    UINT8  *p = p_params->p_param_buf, op_subcode;
    UINT16  evt_len = p_params->param_len;
    UINT8   i;
    tBTM_BLE_VENDOR_CB  *p_cb = &btm_ble_vendor_cb;
    BD_ADDR         target_bda, pseudo_bda, rra;


    STREAM_TO_UINT8(status, p);

    evt_len--;

    op_subcode   = *p ++;
    BTM_TRACE_DEBUG("btm_ble_vendor_irk_vsc_op_cmpl op_subcode = %d", op_subcode);
    if (evt_len < 1)
    {
        BTM_TRACE_ERROR("cannot interpret IRK VSC cmpl callback");
        return;
    }

    if (BTM_BLE_META_IRK_ENABLE == op_subcode)
    {
        BTM_TRACE_DEBUG("IRK enable: %d, %d", status, op_subcode);
        return;
    }
    if (btm_ble_vendor_cb.irk_list == NULL || btm_ble_vendor_cb.irk_list == 0)
    {
        BTM_TRACE_DEBUG("IRK list invalid or null");
        return;
    }
    else
    if (op_subcode == BTM_BLE_META_CLEAR_IRK_LIST)
    {
        if (status == HCI_SUCCESS)
        {
            STREAM_TO_UINT8(p_cb->irk_avail_size, p);
            p_cb->irk_list_size = 0;

            BTM_TRACE_DEBUG("p_cb->irk_list_size = %d", p_cb->irk_avail_size);

            for (i = 0; i < btm_cb.cmn_ble_vsc_cb.max_irk_list_sz; i ++)
                memset(&p_cb->irk_list[i], 0, sizeof(tBTM_BLE_IRK_ENTRY));
        }
    }
    else if (op_subcode == BTM_BLE_META_ADD_IRK_ENTRY)
    {
        if (!btm_ble_vendor_deq_irk_pending(target_bda, pseudo_bda))
        {
            BTM_TRACE_ERROR("no pending IRK operation");
            return;
        }

        if (status == HCI_SUCCESS)
        {
            STREAM_TO_UINT8(p_cb->irk_avail_size, p);
            btm_ble_vendor_update_irk_list(target_bda, pseudo_bda, TRUE);
        }
        else if (status == 0x07) /* BT_ERROR_CODE_MEMORY_CAPACITY_EXCEEDED  */
        {
            p_cb->irk_avail_size = 0;
            BTM_TRACE_ERROR("IRK Full ");
        }
        else
        {
            /* give the credit back if invalid parameter failed the operation */
            p_cb->irk_list_size ++;
        }
    }
    else if (op_subcode == BTM_BLE_META_REMOVE_IRK_ENTRY)
    {
        if (!btm_ble_vendor_deq_irk_pending(target_bda, pseudo_bda))
        {
            BTM_TRACE_ERROR("no pending IRK operation");
            return;
        }
        if (status == HCI_SUCCESS)
        {
            STREAM_TO_UINT8(p_cb->irk_avail_size, p);
            btm_ble_vendor_update_irk_list(target_bda, pseudo_bda, FALSE);
        }
        else
        {
            /* give the credit back if invalid parameter failed the operation */
            if (p_cb->irk_avail_size > 0)
                p_cb->irk_list_size --;
        }

    }
    else if (op_subcode == BTM_BLE_META_READ_IRK_ENTRY)
    {
        if (status == HCI_SUCCESS)
        {
            //STREAM_TO_UINT8(index, p);
            p += (1 + 16 + 1); /* skip index, IRK value, address type */
            STREAM_TO_BDADDR(target_bda, p);
            STREAM_TO_BDADDR(rra, p);
            btm_ble_refresh_rra(target_bda, rra);
        }
    }

}
/*******************************************************************************
**
** Function         btm_ble_remove_irk_entry
**
** Description      This function to remove an IRK entry from the list
**
** Parameters       ble_addr_type: address type
**                  ble_addr: LE adddress
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_remove_irk_entry(tBTM_SEC_DEV_REC *p_dev_rec)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8           param[20], *p;
    tBTM_STATUS     st;
    tBTM_BLE_VENDOR_CB  *p_cb = &btm_ble_vendor_cb;

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return BTM_MODE_UNSUPPORTED;

    if(p_cb->irk_list == 0 || p_cb->irk_list == NULL)
    {
        BTM_TRACE_ERROR("IRK list is null ..returning ");
        return BTM_MODE_UNSUPPORTED;
    }

    p = param;
    memset(param, 0, 20);

    UINT8_TO_STREAM(p, BTM_BLE_META_REMOVE_IRK_ENTRY);
    UINT8_TO_STREAM(p, p_dev_rec->ble.static_addr_type);
    BDADDR_TO_STREAM(p, p_dev_rec->ble.static_addr);

    if ((st = BTM_VendorSpecificCommand (HCI_VENDOR_BLE_RPA_VSC,
                                    BTM_BLE_META_REMOVE_IRK_LEN,
                                    param,
                                    btm_ble_vendor_irk_vsc_op_cmpl))
        != BTM_NO_RESOURCES)
    {
        btm_ble_vendor_enq_irk_pending(p_dev_rec->ble.static_addr, p_dev_rec->bd_addr, FALSE);
        p_cb->irk_list_size --;
    }

    return st;
#endif
    return BTM_MODE_UNSUPPORTED;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_clear_irk_list
**
** Description      This function clears the IRK entry list
**
** Parameters       None.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_vendor_clear_irk_list(void)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8           param[20], *p;
    tBTM_STATUS     st;

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return BTM_MODE_UNSUPPORTED;

    p = param;
    memset(param, 0, 20);

    UINT8_TO_STREAM(p, BTM_BLE_META_CLEAR_IRK_LIST);

    st = BTM_VendorSpecificCommand (HCI_VENDOR_BLE_RPA_VSC,
                                    BTM_BLE_META_CLEAR_IRK_LEN,
                                    param,
                                    btm_ble_vendor_irk_vsc_op_cmpl);

    return st;
#endif
    return BTM_MODE_UNSUPPORTED;
}
/*******************************************************************************
**
** Function         btm_ble_read_irk_entry
**
** Description      This function read an IRK entry by index
**
** Parameters       entry index.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_read_irk_entry(BD_ADDR target_bda)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8           param[20], *p;
    tBTM_STATUS     st = BTM_UNKNOWN_ADDR;
    tBTM_BLE_IRK_ENTRY *p_entry;

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return BTM_MODE_UNSUPPORTED;

    if ((p_entry = btm_ble_vendor_find_irk_entry(target_bda)) == NULL)
        return st;

    p = param;
    memset(param, 0, 20);

    UINT8_TO_STREAM(p, BTM_BLE_META_READ_IRK_ENTRY);
    UINT8_TO_STREAM(p, p_entry->index);

    st = BTM_VendorSpecificCommand (HCI_VENDOR_BLE_RPA_VSC,
                                    BTM_BLE_META_READ_IRK_LEN,
                                    param,
                                    btm_ble_vendor_irk_vsc_op_cmpl);

    return st;
#endif
    return BTM_MODE_UNSUPPORTED;
}


/*******************************************************************************
**
** Function         btm_ble_vendor_enable_irk_list_known_dev
**
** Description      This function add all known device with random address into
**                  IRK list.
**
** Parameters       enable: enable IRK list with known device, or disable it
**
** Returns          status
**
*******************************************************************************/
void btm_ble_vendor_irk_list_known_dev(BOOLEAN enable)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8               i;
    UINT8               count = 0;
    tBTM_SEC_DEV_REC    *p_dev_rec = &btm_cb.sec_dev_rec[0];

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return;

    /* add all known device with random address into IRK list */
    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i ++, p_dev_rec ++)
    {
        if (p_dev_rec->sec_flags & BTM_SEC_IN_USE)
        {
            if (btm_ble_vendor_irk_list_load_dev(p_dev_rec))
                count ++;
        }
    }

    if ((count > 0 && enable) || !enable)
        btm_ble_vendor_enable_irk_feature(enable);
#endif
    return ;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_irk_list_load_dev
**
** Description      This function add a device which is using RPA into white list
**
** Parameters
**
** Returns          status
**
*******************************************************************************/
BOOLEAN btm_ble_vendor_irk_list_load_dev(tBTM_SEC_DEV_REC *p_dev_rec)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8           param[40], *p;
    tBTM_BLE_VENDOR_CB  *p_cb = &btm_ble_vendor_cb;
    BOOLEAN         rt = FALSE;
    tBTM_BLE_IRK_ENTRY  *p_irk_entry = NULL;
    BTM_TRACE_DEBUG ("btm_ble_vendor_irk_list_load_dev:max_irk_size=%d", p_cb->irk_avail_size);
    memset(param, 0, 40);

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return FALSE;

    if(p_cb->irk_list == 0 || p_cb->irk_list == NULL)
    {
        BTM_TRACE_ERROR("IRK list is null ..returning ");
        return FALSE;
    }
    if (p_dev_rec != NULL && /* RPA is being used and PID is known */
        (p_dev_rec->ble.key_type & BTM_LE_KEY_PID) != 0)
    {

        if ((p_irk_entry = btm_ble_vendor_find_irk_entry_by_psuedo_addr(p_dev_rec->bd_addr)) == NULL &&
            btm_ble_vendor_find_irk_pending_entry(p_dev_rec->bd_addr, TRUE) == FALSE)
        {

            if (p_cb->irk_avail_size > 0)
            {
                p = param;

                UINT8_TO_STREAM(p, BTM_BLE_META_ADD_IRK_ENTRY);
                ARRAY_TO_STREAM(p, p_dev_rec->ble.keys.irk, BT_OCTET16_LEN);
                UINT8_TO_STREAM(p, p_dev_rec->ble.static_addr_type);
                BDADDR_TO_STREAM(p,p_dev_rec->ble.static_addr);

                if (BTM_VendorSpecificCommand (HCI_VENDOR_BLE_RPA_VSC,
                                                BTM_BLE_META_ADD_IRK_LEN,
                                                param,
                                                btm_ble_vendor_irk_vsc_op_cmpl)
                       != BTM_NO_RESOURCES)
                {
                    btm_ble_vendor_enq_irk_pending(p_dev_rec->ble.static_addr, p_dev_rec->bd_addr, TRUE);
                    p_cb->irk_list_size ++;
                    rt = TRUE;
                }
            }
        }
        else
        {
            BTM_TRACE_ERROR("Device already in IRK list");
            rt = TRUE;
        }
    }
    else
    {
        BTM_TRACE_DEBUG("Device not a RPA enabled device");
    }

    if (rt)
    {
        btm_ble_vendor_enable_irk_feature(TRUE);
    }

    return rt;
#endif
    return FALSE;
}
/*******************************************************************************
**
** Function         btm_ble_vendor_irk_list_remove_dev
**
** Description      This function remove the device from IRK list
**
** Parameters
**
** Returns          status
**
*******************************************************************************/
void btm_ble_vendor_irk_list_remove_dev(tBTM_SEC_DEV_REC *p_dev_rec)
{
#if BLE_PRIVACY_SPT == TRUE
    tBTM_BLE_VENDOR_CB  *p_cs_cb = &btm_ble_vendor_cb;
    tBTM_BLE_IRK_ENTRY *p_irk_entry;

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return;

    if(p_cs_cb->irk_list == 0 || p_cs_cb->irk_list == NULL)
    {
        BTM_TRACE_ERROR("IRK list is null ..returning ");
        return;
    }
    if ((p_irk_entry = btm_ble_vendor_find_irk_entry_by_psuedo_addr(p_dev_rec->bd_addr)) != NULL &&
        btm_ble_vendor_find_irk_pending_entry(p_dev_rec->bd_addr, FALSE) == FALSE)
    {
        btm_ble_remove_irk_entry(p_dev_rec);
    }
    else
    {
        BTM_TRACE_ERROR("Device not in IRK list");
    }

    if (p_cs_cb->irk_list_size == 0)
        btm_ble_vendor_enable_irk_feature(FALSE);
#endif
}
/*******************************************************************************
**
** Function         btm_ble_vendor_disable_irk_list
**
** Description      disable LE resolve address feature
**
** Parameters
**
** Returns          status
**
*******************************************************************************/
void btm_ble_vendor_disable_irk_list(void)
{
#if BLE_PRIVACY_SPT == TRUE
    btm_ble_vendor_enable_irk_feature(FALSE);
#endif
}

/*******************************************************************************
**
** Function         btm_ble_vendor_get_irk_list_size
**
** Description      get the irk list size
**
** Parameters
**
** Returns          irk list size
**
*******************************************************************************/
UINT8 btm_ble_vendor_get_irk_list_size(void)
{
#if BLE_PRIVACY_SPT == TRUE
    return btm_ble_vendor_cb.irk_list_size;
#else
    return 0;
#endif
}

/*******************************************************************************
**
** Function         btm_ble_vendor_enable_irk_feature
**
** Description      This function is called to enable or disable the RRA
**                  offloading feature.
**
** Parameters       enable: enable or disable the RRA offloading feature
**
** Returns          BTM_SUCCESS if successful
**
*******************************************************************************/
tBTM_STATUS btm_ble_vendor_enable_irk_feature(BOOLEAN enable)
{
#if BLE_PRIVACY_SPT == TRUE
    UINT8           param[20], *p;
    tBTM_STATUS     st = BTM_WRONG_MODE;
    tBTM_BLE_PF_COUNT *p_bda_filter;

    if (btm_cb.cmn_ble_vsc_cb.max_irk_list_sz == 0)
        return BTM_MODE_UNSUPPORTED;

    if (btm_ble_vendor_cb.enable != enable)
    {
        p = param;
        memset(param, 0, 20);

        /* select feature based on control block settings */
        UINT8_TO_STREAM(p, BTM_BLE_META_IRK_ENABLE);
        UINT8_TO_STREAM(p, enable ? 0x01 : 0x00);

        st = BTM_VendorSpecificCommand (HCI_VENDOR_BLE_RPA_VSC, BTM_BLE_IRK_ENABLE_LEN,
                                        param, btm_ble_vendor_irk_vsc_op_cmpl);

        btm_ble_vendor_cb.enable = enable;
    }

    return st;
#endif
    return BTM_MODE_UNSUPPORTED;
}


/*******************************************************************************
**
** Function         btm_ble_vendor_init
**
** Description      Initialize customer specific feature information in host stack
**
** Parameters  Max IRK list size
**                   Max filter supported
**
** Returns          void
**
*******************************************************************************/
void btm_ble_vendor_init(UINT8 max_irk_list_sz)
{
    memset(&btm_ble_vendor_cb, 0, sizeof(tBTM_BLE_VENDOR_CB));

#if BLE_PRIVACY_SPT == TRUE
    if (max_irk_list_sz > 0)
    {
        btm_ble_vendor_cb.irk_list =  (tBTM_BLE_IRK_ENTRY*)GKI_getbuf (sizeof (tBTM_BLE_IRK_ENTRY)
                                                                        * max_irk_list_sz);
        /*Zero initialize the List block*/
        if(btm_ble_vendor_cb.irk_list != 0 && btm_ble_vendor_cb.irk_list != NULL)
        {
            memset(btm_ble_vendor_cb.irk_list, 0, (sizeof (tBTM_BLE_IRK_ENTRY) * max_irk_list_sz));
        }

        btm_ble_vendor_cb.irk_pend_q.irk_q =  (BD_ADDR*) GKI_getbuf (sizeof (BD_ADDR) *
                                                                     max_irk_list_sz);
        btm_ble_vendor_cb.irk_pend_q.irk_q_random_pseudo = (BD_ADDR*)GKI_getbuf (sizeof (BD_ADDR) *
                                                                                 max_irk_list_sz);
        btm_ble_vendor_cb.irk_pend_q.irk_q_action = (UINT8*) GKI_getbuf (max_irk_list_sz);
    }

    btm_ble_vendor_cb.irk_avail_size = max_irk_list_sz;

    if (!HCI_LE_HOST_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_1]))
        return;
#endif
}

/*******************************************************************************
**
** Function         btm_ble_vendor_cleanup
**
** Description      Cleanup VSC specific dynamic memory
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void btm_ble_vendor_cleanup(void)
{
#if BLE_PRIVACY_SPT == TRUE
    if (btm_ble_vendor_cb.irk_list)
        GKI_freebuf(btm_ble_vendor_cb.irk_list);

    if (btm_ble_vendor_cb.irk_pend_q.irk_q)
       GKI_freebuf(btm_ble_vendor_cb.irk_pend_q.irk_q);

    if (btm_ble_vendor_cb.irk_pend_q.irk_q_random_pseudo)
        GKI_freebuf(btm_ble_vendor_cb.irk_pend_q.irk_q_random_pseudo);

    if (btm_ble_vendor_cb.irk_pend_q.irk_q_action)
        GKI_freebuf(btm_ble_vendor_cb.irk_pend_q.irk_q_action);
#endif
    memset(&btm_ble_vendor_cb, 0, sizeof(tBTM_BLE_VENDOR_CB));
}

#endif

