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
**  Name:          vendor_ble.h
**
**  Description:   This file contains vendor specific feature header
**                 for BLE
******************************************************************************/
#ifndef VENDOR_BLE_H
#define VENDOR_BLE_H

#include "btm_int.h"
#include "btm_ble_api.h"
#include "vendor_api.h"

/* RPA offload VSC specifics */
#define BTM_BLE_META_IRK_ENABLE         0x01
#define BTM_BLE_META_ADD_IRK_ENTRY      0x02
#define BTM_BLE_META_REMOVE_IRK_ENTRY   0x03
#define BTM_BLE_META_CLEAR_IRK_LIST     0x04
#define BTM_BLE_META_READ_IRK_ENTRY     0x05
#define BTM_BLE_META_CS_RESOLVE_ADDR    0x00000001
#define BTM_BLE_IRK_ENABLE_LEN          2

/* BLE meta vsc header: 1 bytes of sub_code, 1 byte of PCF action */
#define BTM_BLE_META_HDR_LENGTH     3
#define BTM_BLE_PF_FEAT_SEL_LEN     18
#define BTM_BLE_PCF_ENABLE_LEN      2
#define BTM_BLE_META_ADDR_LEN       7
#define BTM_BLE_META_UUID_LEN       40
#define BTM_BLE_META_ADD_IRK_LEN        24
#define BTM_BLE_META_REMOVE_IRK_LEN     8
#define BTM_BLE_META_CLEAR_IRK_LEN      1
#define BTM_BLE_META_READ_IRK_LEN       2
#define BTM_BLE_META_ADD_WL_ATTR_LEN    9

#define BTM_BLE_PF_SELECT_NONE              0
#define BTM_BLE_PF_ADDR_FILTER_BIT          BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_ADDR_FILTER)
#define BTM_BLE_PF_SRVC_DATA_BIT            BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_SRVC_DATA)
#define BTM_BLE_PF_SRVC_UUID_BIT            BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_SRVC_UUID)
#define BTM_BLE_PF_SRVC_SOL_UUID_BIT        BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_SRVC_SOL_UUID)
#define BTM_BLE_PF_LOCAL_NAME_BIT           BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_LOCAL_NAME)
#define BTM_BLE_PF_MANU_DATA_BIT            BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_MANU_DATA)
#define BTM_BLE_PF_SRVC_DATA_PATTERN_BIT    BTM_BLE_PF_BIT_TO_MASK(BTM_BLE_PF_SRVC_DATA_PATTERN)
typedef UINT8 tBTM_BLE_PF_SEL_MASK;

#define BTM_BLE_MAX_FILTER_COUNTER  (BTM_BLE_MAX_ADDR_FILTER + 1) /* per device filter + one generic filter indexed by 0 */

#define BTM_CS_IRK_LIST_INVALID     0xff

typedef struct
{
    BOOLEAN         in_use;
    BD_ADDR         bd_addr;        /* must be the address used in controller */
    BD_ADDR         psuedo_bda;        /* the random pseudo address */
    UINT8           index;
}tBTM_BLE_IRK_ENTRY;


typedef struct
{
    BD_ADDR         *irk_q;
    BD_ADDR         *irk_q_random_pseudo;
    UINT8           *irk_q_action;
    UINT8           q_next;
    UINT8           q_pending;
} tBTM_BLE_IRK_Q;

/* control block for BLE customer specific feature */
typedef struct
{
    BOOLEAN             enable;

    UINT8               op_type;
    tBLE_BD_ADDR        cur_filter_target;

    UINT8               irk_list_size;
    UINT8               irk_avail_size;
    tBTM_BLE_IRK_ENTRY  *irk_list;
    tBTM_BLE_IRK_Q      irk_pend_q;
    UINT8                max_filter_supported;
    tBTM_BLE_PF_CMPL_CBACK *p_scan_pf_cback;
}tBTM_BLE_VENDOR_CB;

#ifdef __cplusplus
extern "C"
{
#endif

#if VENDOR_DYNAMIC_MEMORY == FALSE
BTM_API extern tBTM_BLE_VENDOR_CB  btm_ble_vendor_cb;
#else
BTM_API extern tBTM_BLE_VENDOR_CB *btm_ble_vendor_ptr;
#define btm_ble_vendor_cb (*btm_ble_vendor_ptr)
#endif

extern void btm_ble_vendor_irk_list_known_dev(BOOLEAN enable);
extern tBTM_STATUS btm_ble_read_irk_entry(BD_ADDR target_bda);
extern void btm_ble_vendor_disable_irk_list(void);
extern BOOLEAN btm_ble_vendor_irk_list_load_dev(tBTM_SEC_DEV_REC *p_dev_rec);
extern void btm_ble_vendor_irk_list_remove_dev(tBTM_SEC_DEV_REC *p_dev_rec);
extern tBTM_STATUS btm_ble_enable_vendor_feature (BOOLEAN enable, UINT32 feature_bit);

extern void btm_ble_vendor_init(UINT8 max_irk_list_sz);
extern void btm_ble_vendor_cleanup(void);
extern BOOLEAN btm_ble_vendor_write_device_wl_attribute (tBLE_ADDR_TYPE addr_type, BD_ADDR bd_addr, UINT8 attribute);
extern tBTM_STATUS btm_ble_vendor_enable_irk_feature(BOOLEAN enable);
extern tBTM_BLE_IRK_ENTRY *btm_ble_vendor_find_irk_entry_by_psuedo_addr (BD_ADDR psuedo_bda);
extern UINT8 btm_ble_vendor_get_irk_list_size(void);
#ifdef __cplusplus
}
#endif

#endif


