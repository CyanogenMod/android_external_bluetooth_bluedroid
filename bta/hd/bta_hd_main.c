/******************************************************************************
**
**  File Name:   bta_hd_main.c
**
**  Description: This is the state machine definition for the HID Device
**               service.
**
**  Copyright (c) 2002-2011, Broadcom Corp., All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
**
******************************************************************************/

#include <string.h>
#include "bt_target.h"
#include "data_types.h"
#include "bt_types.h"
#include "hidd_api.h"
#include "bta_hd_api.h"
#include "bta_hd_int.h"
#include "bd.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/


/* state machine states */
enum
{
    BTA_HD_IDLE_ST,
    BTA_HD_LISTEN_ST,
    BTA_HD_OPEN_ST
};

/* state machine action enumeration list */
enum
{
    BTA_HD_INIT_CON_ACT,
    BTA_HD_CLOSE_ACT,
    BTA_HD_DISABLE_ACT,
    BTA_HD_OPEN_ACT,
    BTA_HD_OPN_CB_ACT,
    BTA_HD_INPUT_ACT,
    BTA_HD_DISCNTD_ACT,
    BTA_HD_DISCNT_ACT,
    BTA_HD_NUM_ACTIONS
};

#define BTA_HD_IGNORE       BTA_HD_NUM_ACTIONS

/* type for action functions */
typedef void (*tBTA_HD_ACTION)(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);

/* action functions */
const tBTA_HD_ACTION bta_hd_action[] =
{
    bta_hd_init_con_act,
    bta_hd_close_act,
    bta_hd_disable_act,
    bta_hd_open_act,
    bta_hd_opn_cb_act,
    bta_hd_input_act,
    bta_hd_discntd_act,
    bta_hd_discnt_act
};

/* state table information */
#define BTA_HD_NUM_ACTS             1       /* number of actions */
#define BTA_HD_ACTION               0       /* position of action */
#define BTA_HD_NEXT_STATE           1       /* position of next state */
#define BTA_HD_NUM_COLS             2       /* number of columns in state tables */

/* state table for listen state */
const UINT8 bta_hd_st_listen[][BTA_HD_NUM_COLS] =
{
/* API_OPEN    */   {BTA_HD_INIT_CON_ACT,   BTA_HD_LISTEN_ST},
/* API_CLOSE   */   {BTA_HD_CLOSE_ACT,      BTA_HD_LISTEN_ST},
/* API_DISABLE */   {BTA_HD_DISABLE_ACT,    BTA_HD_IDLE_ST},
/* API_INPUT   */   {BTA_HD_IGNORE,         BTA_HD_LISTEN_ST},
/* CBACK       */   {BTA_HD_IGNORE,         BTA_HD_LISTEN_ST},
/* CONNECTED   */   {BTA_HD_OPEN_ACT,       BTA_HD_OPEN_ST},
/* DISCONNECT  */   {BTA_HD_DISCNTD_ACT,    BTA_HD_LISTEN_ST}
};

/* state table for open state */
const UINT8 bta_hd_st_open[][BTA_HD_NUM_COLS] =
{
/* API_OPEN    */   {BTA_HD_IGNORE,         BTA_HD_OPEN_ST},
/* API_CLOSE   */   {BTA_HD_CLOSE_ACT,      BTA_HD_OPEN_ST},
/* API_DISABLE */   {BTA_HD_DISABLE_ACT,    BTA_HD_IDLE_ST},
/* API_INPUT   */   {BTA_HD_INPUT_ACT,      BTA_HD_OPEN_ST},
/* CBACK       */   {BTA_HD_OPN_CB_ACT,     BTA_HD_OPEN_ST},
/* CONNECTED   */   {BTA_HD_IGNORE,         BTA_HD_OPEN_ST},
/* DISCONNECT  */   {BTA_HD_DISCNT_ACT,     BTA_HD_LISTEN_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_HD_ST_TBL)[BTA_HD_NUM_COLS];

/* state table */
const tBTA_HD_ST_TBL bta_hd_st_tbl[] =
{
    bta_hd_st_listen,
    bta_hd_st_open
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* HD control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_HD_CB  bta_hd_cb;
#endif


/*******************************************************************************
**
** Function         bta_hd_api_enable
**
** Description      Handle an API enable event.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_hd_api_enable(tBTA_HD_DATA *p_data)
{
    tHID_DEV_SDP_INFO   sdp_info;
    tHID_DEV_REG_INFO   reg_info;
    tBTA_SEC            sec_mask = p_data->api_enable.sec_mask;
    tBTA_HD_STATUS      status = BTA_HD_FAIL;

    /* initialize control block */
    memset(&bta_hd_cb, 0, sizeof(tBTA_HD_CB));
    GKI_init_q (&bta_hd_cb.out_q);

    /* store parameters */
    bta_hd_cb.p_cback = p_data->api_enable.p_cback;

    /* register HID Device to HIDD   */
    reg_info.app_cback      = bta_hd_hidd_cback;
    bdcpy(reg_info.host_addr, p_data->api_enable.bd_addr);
    if(p_bta_hd_cfg->use_qos)
        reg_info.qos_info   = &p_bta_hd_cfg->qos;
    else
        reg_info.qos_info   = NULL;


    if(HID_DevRegister(&reg_info) == HID_SUCCESS)
    {
        HID_DevSetSecurityLevel("BT HID Combo Mouse/Keyboard", sec_mask);

        /* register HID Device to SDP   */
        memcpy(&sdp_info, &p_bta_hd_cfg->sdp_info, sizeof(tHID_DEV_SDP_INFO));
        if(p_data->api_enable.service_name[0])
        {
            BCM_STRNCPY_S(sdp_info.svc_name, sizeof(sdp_info.svc_name), p_data->api_enable.service_name,
// btla-specific ++
                sizeof(sdp_info.svc_name));
// btla-specific --
        }
        bta_hd_cb.sdp_handle    = HID_DevSetSDPRecord(&sdp_info);
    }

    if(bta_hd_cb.sdp_handle>0)
    {
        status = BTA_HD_SUCCESS;
        bta_hd_cb.state = BTA_HD_LISTEN_ST;
        bta_sys_add_uuid(UUID_SERVCLASS_HUMAN_INTERFACE);
    }

    /* call callback with enable event */
    (*bta_hd_cb.p_cback)(BTA_HD_ENABLE_EVT, (tBTA_HD *)&status);
}

/*******************************************************************************
**
** Function         bta_hd_sm_execute
**
** Description      State machine event handling function for HD
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_hd_sm_execute(tBTA_HD_CB *p_cb, UINT16 event, tBTA_HD_DATA *p_data)
{
    tBTA_HD_ST_TBL      state_table;
    UINT8               action;

    if(p_cb->state == BTA_HD_IDLE_ST)
    {
        APPL_TRACE_EVENT1("HD event=0x%x received in IDLE", event);
        return;
    }

    /* look up the state table for the current state */
    state_table = bta_hd_st_tbl[p_cb->state-1];

    event &= 0x00FF;

    APPL_TRACE_EVENT3("HD event=0x%x state=%d, next: %d",
        event, p_cb->state, state_table[event][BTA_HD_NEXT_STATE]);
    /* set next state */
    p_cb->state = state_table[event][BTA_HD_NEXT_STATE];
    action = state_table[event][BTA_HD_ACTION];

    /* execute action functions */
    if (action != BTA_HD_IGNORE)
    {
        (*bta_hd_action[action])(p_cb, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hd_hdl_event
**
** Description      HID Device main event handling function.
**                  
**
** Returns          BOOLEAN
**
*******************************************************************************/
BOOLEAN bta_hd_hdl_event(BT_HDR *p_msg)
{
    switch (p_msg->event)
    {
        /* handle enable event */
        case BTA_HD_API_ENABLE_EVT:
            bta_hd_api_enable((tBTA_HD_DATA *) p_msg);
            break;

        /* all others run through state machine */
        default:
            bta_hd_sm_execute(&bta_hd_cb, p_msg->event, (tBTA_HD_DATA *) p_msg);
            break;
    }
    return TRUE;   
}

