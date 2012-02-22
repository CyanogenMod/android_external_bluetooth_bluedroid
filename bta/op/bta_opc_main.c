/*****************************************************************************
**
**  Name:           bta_opc_main.c
**
**  Description:    This file contains the file transfer client main functions 
**                  and state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bta_opc_int.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/

/* state machine states */
enum
{
  BTA_OPC_DISABLED_ST = 0,  /* Disabled */
  BTA_OPC_IDLE_ST,          /* Idle  */
  BTA_OPC_W4_CONN_ST,       /* Waiting for an Obex connect response */
  BTA_OPC_CONN_ST,          /* Connected - OPP Session is active */
  BTA_OPC_CLOSING_ST        /* Closing is in progress */
};

/* state machine action enumeration list */
enum
{
    BTA_OPC_START_CLIENT,
    BTA_OPC_STOP_CLIENT,
    BTA_OPC_INIT_PULL,
    BTA_OPC_INIT_PUSH,
    BTA_OPC_INIT_EXCH,
    BTA_OPC_CI_WRITE,
    BTA_OPC_CI_READ,
    BTA_OPC_CI_OPEN,
    BTA_OPC_OBX_CONN_RSP,
    BTA_OPC_CLOSE,
    BTA_OPC_OBX_PUT_RSP,
    BTA_OPC_OBX_GET_RSP,
    BTA_OPC_TRANS_CMPL,
    BTA_OPC_FREE_DB,
    BTA_OPC_IGNORE_OBX,
    BTA_OPC_FIND_SERVICE,
    BTA_OPC_INITIALIZE,
    BTA_OPC_CLOSE_COMPLETE,
    BTA_OPC_SET_DISABLE,
    BTA_OPC_CHK_CLOSE,
    BTA_OPC_DO_PUSH,
    BTA_OPC_DO_PULL,
    BTA_OPC_ENABLE,
    BTA_OPC_IGNORE
};

/* type for action functions */
typedef void (*tBTA_OPC_ACTION)(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);

/* action function list */
const tBTA_OPC_ACTION bta_opc_action[] =
{
    bta_opc_start_client,
    bta_opc_stop_client,
    bta_opc_init_pull,
    bta_opc_init_push,
    bta_opc_init_exch,
    bta_opc_ci_write,
    bta_opc_ci_read,
    bta_opc_ci_open,
    bta_opc_obx_conn_rsp,
    bta_opc_close,
    bta_opc_obx_put_rsp,
    bta_opc_obx_get_rsp,
    bta_opc_trans_cmpl,
    bta_opc_free_db,
    bta_opc_ignore_obx,
    bta_opc_find_service,
    bta_opc_initialize,
    bta_opc_close_complete,
    bta_opc_set_disable,
    bta_opc_chk_close,
    bta_opc_do_push,
    bta_opc_do_pull,
    bta_opc_enable
};


/* state table information */
#define BTA_OPC_ACTIONS             2       /* number of actions */
#define BTA_OPC_NEXT_STATE          2       /* position of next state */
#define BTA_OPC_NUM_COLS            3       /* number of columns in state tables */

/* state table for disabled state (enable is handled*/
static const UINT8 bta_opc_st_disabled[][BTA_OPC_NUM_COLS] =
{
/* Event                            Action 1            Action 2            Next state */
/* BTA_OPC_API_ENABLE_EVT      */   {BTA_OPC_ENABLE,    BTA_OPC_IGNORE,     BTA_OPC_IDLE_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_CLOSE_EVT       */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_PUSH_EVT        */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_PULL_EVT        */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_EXCH_EVT        */   {BTA_OPC_IGNORE,    BTA_OPC_IGNORE,     BTA_OPC_DISABLED_ST}
};

/* state table for idle state */
static const UINT8 bta_opc_st_idle[][BTA_OPC_NUM_COLS] =
{
/* Event                            Action 1                Action 2              Next state */
/* BTA_OPC_API_ENABLE_EVT      */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_INITIALIZE,    BTA_OPC_IGNORE,       BTA_OPC_DISABLED_ST},
/* BTA_OPC_API_CLOSE_EVT       */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_API_PUSH_EVT        */   {BTA_OPC_INIT_PUSH,     BTA_OPC_FIND_SERVICE, BTA_OPC_W4_CONN_ST},
/* BTA_OPC_API_PULL_EVT        */   {BTA_OPC_INIT_PULL,     BTA_OPC_FIND_SERVICE, BTA_OPC_W4_CONN_ST},
/* BTA_OPC_API_EXCH_EVT        */   {BTA_OPC_INIT_EXCH,     BTA_OPC_FIND_SERVICE, BTA_OPC_W4_CONN_ST},
/* BTA_OPC_SDP_OK_EVT          */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_SDP_FAIL_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_WRITE_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_READ_EVT         */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_OPEN_EVT         */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_CONN_RSP_EVT    */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_PASSWORD_EVT    */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_CLOSE_EVT       */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_PUT_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_GET_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_CMPL_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_CLOSE_CMPL_EVT      */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_IDLE_ST},
/* BTA_OPC_DISABLE_CMPL_EVT    */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,       BTA_OPC_DISABLED_ST}
};

/* state table for wait for authentication response state */
static const UINT8 bta_opc_st_w4_conn[][BTA_OPC_NUM_COLS] =
{
/* Event                            Action 1                Action 2                Next state */
/* BTA_OPC_API_ENABLE_EVT      */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_SET_DISABLE,   BTA_OPC_STOP_CLIENT,    BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_CLOSE_EVT       */   {BTA_OPC_STOP_CLIENT,   BTA_OPC_IGNORE,         BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_PUSH_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_API_PULL_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_API_EXCH_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_SDP_OK_EVT          */   {BTA_OPC_FREE_DB,       BTA_OPC_START_CLIENT,   BTA_OPC_W4_CONN_ST},
/* BTA_OPC_SDP_FAIL_EVT        */   {BTA_OPC_FREE_DB,       BTA_OPC_CLOSE_COMPLETE, BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_WRITE_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_CI_READ_EVT         */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_CI_OPEN_EVT         */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_OBX_CONN_RSP_EVT    */   {BTA_OPC_OBX_CONN_RSP,  BTA_OPC_IGNORE,         BTA_OPC_CONN_ST},
/* BTA_OPC_OBX_PASSWORD_EVT    */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_STOP_CLIENT,    BTA_OPC_W4_CONN_ST},
/* BTA_OPC_OBX_CLOSE_EVT       */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_CLOSE,          BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_PUT_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_OBX_GET_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,         BTA_OPC_W4_CONN_ST},
/* BTA_OPC_OBX_CMPL_EVT        */   {BTA_OPC_TRANS_CMPL,    BTA_OPC_STOP_CLIENT,    BTA_OPC_CLOSING_ST},
/* BTA_OPC_CLOSE_CMPL_EVT      */   {BTA_OPC_CLOSE_COMPLETE,BTA_OPC_IGNORE,         BTA_OPC_IDLE_ST},
/* BTA_OPC_DISABLE_CMPL_EVT    */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,         BTA_OPC_DISABLED_ST}
};

/* state table for open state */
static const UINT8 bta_opc_st_connected[][BTA_OPC_NUM_COLS] =
{
/* Event                            Action 1                  Action 2             Next state */
/* BTA_OPC_API_ENABLE_EVT      */   {BTA_OPC_IGNORE,          BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_SET_DISABLE,     BTA_OPC_STOP_CLIENT, BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_CLOSE_EVT       */   {BTA_OPC_STOP_CLIENT,     BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_PUSH_EVT        */   {BTA_OPC_INIT_PUSH,       BTA_OPC_DO_PUSH,     BTA_OPC_CONN_ST},
/* BTA_OPC_API_PULL_EVT        */   {BTA_OPC_INIT_PULL,       BTA_OPC_DO_PULL,     BTA_OPC_CONN_ST},
/* BTA_OPC_API_EXCH_EVT        */   {BTA_OPC_INIT_EXCH,       BTA_OPC_DO_PUSH,     BTA_OPC_CONN_ST},
/* BTA_OPC_SDP_OK_EVT          */   {BTA_OPC_IGNORE,          BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_SDP_FAIL_EVT        */   {BTA_OPC_IGNORE,          BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_CI_WRITE_EVT        */   {BTA_OPC_CI_WRITE,        BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_CI_READ_EVT         */   {BTA_OPC_CI_READ,         BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_CI_OPEN_EVT         */   {BTA_OPC_CI_OPEN,         BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_OBX_CONN_RSP_EVT    */   {BTA_OPC_IGNORE_OBX,      BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_OBX_PASSWORD_EVT    */   {BTA_OPC_IGNORE_OBX,      BTA_OPC_STOP_CLIENT, BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_CLOSE_EVT       */   {BTA_OPC_IGNORE_OBX,      BTA_OPC_CLOSE,       BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_PUT_RSP_EVT     */   {BTA_OPC_OBX_PUT_RSP,     BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_OBX_GET_RSP_EVT     */   {BTA_OPC_OBX_GET_RSP,     BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_OBX_CMPL_EVT        */   {BTA_OPC_TRANS_CMPL,      BTA_OPC_CHK_CLOSE,   BTA_OPC_CONN_ST},
/* BTA_OPC_CLOSE_CMPL_EVT      */   {BTA_OPC_IGNORE,          BTA_OPC_IGNORE,      BTA_OPC_CONN_ST},
/* BTA_OPC_DISABLE_CMPL_EVT    */   {BTA_OPC_IGNORE,          BTA_OPC_IGNORE,      BTA_OPC_DISABLED_ST}
};

/* state table for closing state */
static const UINT8 bta_opc_st_closing[][BTA_OPC_NUM_COLS] =
{
/* Event                            Action 1                Action 2             Next state */
/* BTA_OPC_API_ENABLE_EVT      */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_DISABLE_EVT     */   {BTA_OPC_SET_DISABLE,   BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_CLOSE_EVT       */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_PUSH_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_PULL_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_API_EXCH_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_SDP_OK_EVT          */   {BTA_OPC_FREE_DB,       BTA_OPC_CLOSE,       BTA_OPC_CLOSING_ST},
/* BTA_OPC_SDP_FAIL_EVT        */   {BTA_OPC_FREE_DB,       BTA_OPC_CLOSE,       BTA_OPC_CLOSING_ST},
/* BTA_OPC_CI_WRITE_EVT        */   {BTA_OPC_CLOSE_COMPLETE,BTA_OPC_IGNORE,      BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_READ_EVT         */   {BTA_OPC_CLOSE_COMPLETE,BTA_OPC_IGNORE,      BTA_OPC_IDLE_ST},
/* BTA_OPC_CI_OPEN_EVT         */   {BTA_OPC_CLOSE_COMPLETE,BTA_OPC_IGNORE,      BTA_OPC_IDLE_ST},
/* BTA_OPC_OBX_CONN_RSP_EVT    */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_STOP_CLIENT, BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_PASSWORD_EVT    */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_CLOSE_EVT       */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_CLOSE,       BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_PUT_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_GET_RSP_EVT     */   {BTA_OPC_IGNORE_OBX,    BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_OBX_CMPL_EVT        */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_CLOSING_ST},
/* BTA_OPC_CLOSE_CMPL_EVT      */   {BTA_OPC_CLOSE_COMPLETE,BTA_OPC_IGNORE,      BTA_OPC_IDLE_ST},
/* BTA_OPC_DISABLE_CMPL_EVT    */   {BTA_OPC_IGNORE,        BTA_OPC_IGNORE,      BTA_OPC_DISABLED_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_OPC_ST_TBL)[BTA_OPC_NUM_COLS];

/* state table */
const tBTA_OPC_ST_TBL bta_opc_st_tbl[] =
{
    bta_opc_st_disabled,
    bta_opc_st_idle,
    bta_opc_st_w4_conn,
    bta_opc_st_connected,
    bta_opc_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* OPC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_OPC_CB  bta_opc_cb;
#endif

#if BTA_OPC_DEBUG == TRUE
static char *opc_evt_code(tBTA_OPC_INT_EVT evt_code);
static char *opc_state_code(tBTA_OPC_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_opc_sm_execute
**
** Description      State machine event handling function for OPC
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_sm_execute(tBTA_OPC_CB *p_cb, UINT16 event, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_ST_TBL     state_table;
    UINT8               action;
    int                 i;

    /* look up the state table for the current state */
    state_table = bta_opc_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_OPC_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_OPC_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_OPC_IGNORE)
        {
            (*bta_opc_action[action])(p_cb, p_data);
        }
        else
        {
            break;
        }
    }
}

/*******************************************************************************
**
** Function         bta_opc_hdl_event
**
** Description      File transfer server main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_opc_hdl_event(BT_HDR *p_msg)
{
    tBTA_OPC_CB *p_cb = &bta_opc_cb;

#if BTA_OPC_DEBUG == TRUE
    tBTA_OPC_STATE in_state = p_cb->state;
    APPL_TRACE_DEBUG3("OPC Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                      opc_state_code(in_state),
                      opc_evt_code(p_msg->event));
#endif

    bta_opc_sm_execute(p_cb, p_msg->event, (tBTA_OPC_DATA *) p_msg);

    if ( p_cb->state == BTA_OPC_CONN_ST )
    {
        if (( p_cb->pm_state == BTA_OPC_PM_IDLE )
          &&( p_cb->obx_oper != OPC_OP_NONE ))
        {
            /* inform power manager */
            APPL_TRACE_DEBUG0("BTA OPC informs DM/PM busy state");
            bta_sys_busy( BTA_ID_OPC, p_cb->app_id, p_cb->bd_addr );
            p_cb->pm_state = BTA_OPC_PM_BUSY;
        }
        else if (( p_cb->pm_state == BTA_OPC_PM_BUSY )
               &&( p_cb->obx_oper == OPC_OP_NONE ))
        {
            /* inform power manager */
            APPL_TRACE_DEBUG0("BTA OPC informs DM/PM idle state");
            bta_sys_idle( BTA_ID_OPC ,p_cb->app_id, p_cb->bd_addr);
            p_cb->pm_state = BTA_OPC_PM_IDLE;
        }
    }
    else if ( p_cb->state == BTA_OPC_IDLE_ST )
    {
        /* initialize power management state */
        p_cb->pm_state = BTA_OPC_PM_BUSY;
    }

#if BTA_OPC_DEBUG == TRUE
    if (in_state != p_cb->state)
    {
        APPL_TRACE_DEBUG3("OPC State Change: [%s] -> [%s] after Event [%s]",
                      opc_state_code(in_state),
                      opc_state_code(p_cb->state),
                      opc_evt_code(p_msg->event));
    }
#endif

    return (TRUE);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_OPC_DEBUG == TRUE

/*******************************************************************************
**
** Function         opc_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *opc_evt_code(tBTA_OPC_INT_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_OPC_API_ENABLE_EVT:
        return "BTA_OPC_API_ENABLE_EVT";
    case BTA_OPC_API_DISABLE_EVT:
        return "BTA_OPC_API_DISABLE_EVT";
    case BTA_OPC_API_CLOSE_EVT:
        return "BTA_OPC_API_CLOSE_EVT";
    case BTA_OPC_API_PUSH_EVT:
        return "BTA_OPC_API_PUSH_EVT";
    case BTA_OPC_API_PULL_EVT:
        return "BTA_OPC_API_PULL_EVT";
    case BTA_OPC_API_EXCH_EVT:
        return "BTA_OPC_API_EXCH_EVT";
    case BTA_OPC_SDP_OK_EVT:
        return "BTA_OPC_SDP_OK_EVT";
    case BTA_OPC_SDP_FAIL_EVT:
        return "BTA_OPC_SDP_FAIL_EVT";
    case BTA_OPC_CI_WRITE_EVT:
        return "BTA_OPC_CI_WRITE_EVT";
    case BTA_OPC_CI_READ_EVT:
        return "BTA_OPC_CI_READ_EVT";
    case BTA_OPC_CI_OPEN_EVT:
        return "BTA_OPC_CI_OPEN_EVT";
    case BTA_OPC_OBX_CONN_RSP_EVT:
        return "BTA_OPC_OBX_CONN_RSP_EVT";
    case BTA_OPC_OBX_PASSWORD_EVT:
        return "BTA_OPC_OBX_PASSWORD_EVT";
    case BTA_OPC_OBX_CLOSE_EVT:
        return "BTA_OPC_OBX_CLOSE_EVT";
    case BTA_OPC_OBX_PUT_RSP_EVT:
        return "BTA_OPC_OBX_PUT_RSP_EVT";
    case BTA_OPC_OBX_GET_RSP_EVT:
        return "BTA_OPC_OBX_GET_RSP_EVT";
    case BTA_OPC_OBX_CMPL_EVT:
        return "BTA_OPC_OBX_CMPL_EVT";
    case BTA_OPC_CLOSE_CMPL_EVT:
        return "BTA_OPC_CLOSE_CMPL_EVT";
    case BTA_OPC_DISABLE_CMPL_EVT:
        return "BTA_OPC_DISABLE_CMPL_EVT";
    default:
        return "unknown OPC event code";
    }
}

/*******************************************************************************
**
** Function         opc_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *opc_state_code(tBTA_OPC_STATE state_code)
{
    switch(state_code)
    {
    case BTA_OPC_DISABLED_ST:
        return "BTA_OPC_DISABLED_ST";
    case BTA_OPC_IDLE_ST:
        return "BTA_OPC_IDLE_ST";
    case BTA_OPC_W4_CONN_ST:
        return "BTA_OPC_W4_CONN_ST";
    case BTA_OPC_CONN_ST:
        return "BTA_OPC_CONN_ST";
    case BTA_OPC_CLOSING_ST:
        return "BTA_OPC_CLOSING_ST";
    default:
        return "unknown OPC state code";
    }
}

#endif  /* Debug Functions */
