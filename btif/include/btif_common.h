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
 *  Filename:      btif_common.h
 *
 *  Description:   
 * 
 ***********************************************************************************/

#ifndef BTIF_COMMON_H
#define BTIF_COMMON_H

#include "data_types.h"
#include "bt_types.h"
#include "bta_api.h"

#ifndef LOG_TAG
#error "LOG_TAG not defined, please add in .c file prior to including bt_common.h"
#endif

#include <utils/Log.h>

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define ASSERTC(cond, msg, val) if (!(cond)) {LOGE("### ASSERT : %s line %d %s (%d) ###", __FILE__, __LINE__, msg, val);}

/* Calculate start of event enumeration; id is top 8 bits of event */
#define BTIF_SIG_START(id)       ((id) << 8)

/* for upstream the MSB bit is always SET */
#define BTIF_SIG_CB_BIT   (0x8000)
#define BTIF_SIG_CB_START(id)    (((id) << 8) | BTIF_SIG_CB_BIT)

/* BTIF sub-systems */
#define BTIF_CORE           0 /* core */
#define BTIF_DM             1
#define BTIF_HFP            2
#define BTIF_AV             3

extern bt_callbacks_t *bt_hal_cbacks;

#define CHECK_CALL_CBACK(P_CB, P_CBACK, ...)\
    if (P_CB && P_CB->P_CBACK) {            \
        P_CB->P_CBACK(__VA_ARGS__);         \
    }                                       \
    else {                                  \
        ASSERTC(0, "Callback is NULL", 0);  \
    }

/* btif events for requests that require context switch to btif task 
   on downstreams path */
enum
{
    BTIF_CORE_API_START = BTIF_SIG_START(BTIF_CORE),
    BTIF_CORE_STORAGE_NO_ACTION,
    BTIF_CORE_STORAGE_ADAPTER_WRITE,
    BTIF_CORE_STORAGE_ADAPTER_READ,
    BTIF_CORE_STORAGE_ADAPTER_READ_ALL,
    BTIF_CORE_STORAGE_REMOTE_WRITE,
    BTIF_CORE_STORAGE_REMOTE_READ,
    BTIF_CORE_STORAGE_REMOTE_READ_ALL,
    BTIF_CORE_STORAGE_READ_ALL,    
    BTIF_CORE_STORAGE_NOTIFY_STATUS,
    /* add here */

    BTIF_DM_API_START = BTIF_SIG_START(BTIF_DM),
    /* add here */

    BTIF_HFP_API_START = BTIF_SIG_START(BTIF_HFP),
    /* add here */

    BTIF_AV_API_START = BTIF_SIG_START(BTIF_AV),
    /* add here */
    
};

/* btif events for callbacks that require context switch to btif task
  on upstream path - Typically these would be non-BTA events
  that are generated by the BTIF layer */
enum
{
    BTIF_CORE_CB_START = BTIF_SIG_CB_START(BTIF_CORE),
    /* add here */

    BTIF_DM_CB_START = BTIF_SIG_CB_START(BTIF_DM),
    BTIF_DM_CB_DISCOVERY_STARTED, /* Discovery has started */
    BTIF_DM_CB_BONDING_STARTED,   /* Bonding process has started */
    BTIF_DM_CB_REMOVED_BONDING    /* Bonded device deleted */
};

/************************************************************************************
**  Type definitions for callback functions
************************************************************************************/

typedef void (tBTIF_CBACK) (UINT16 event, char *p_param);
typedef void (tBTIF_COPY_CBACK) (UINT16 event, char *p_dest, char *p_src);

/************************************************************************************
**  Type definitions and return values
************************************************************************************/

/* this type handles all btif context switches between BTU and HAL */
typedef struct
{
    BT_HDR               hdr;
    tBTIF_CBACK*         p_cb;    /* context switch callback */

    /* parameters passed to callback */
    UINT16               event;   /* message event id */
    char                 p_param[0]; /* parameter area needs to be last */
} tBTIF_CONTEXT_SWITCH_CBACK;


/************************************************************************************
**  Functions
************************************************************************************/
bt_status_t btif_transfer_context (tBTIF_CBACK *p_cback, UINT16 event, char* p_params, int param_len, tBTIF_COPY_CBACK *p_copy_cback);

/* 
 * BTIF_Events 
 */

void btif_enable_bluetooth_evt(tBTA_STATUS status, BD_ADDR local_bd);
void btif_disable_bluetooth_evt(void);
void btif_adapter_properties_evt(bt_status_t status, uint32_t num_props, bt_property_t *p_props);
void btif_remote_properties_evt(bt_status_t status, bt_bdaddr_t *remote_addr,
                                   uint32_t num_props, bt_property_t *p_props);
#endif /* BTIF_COMMON_H */

