/*****************************************************************************
**
**  Name:           bta_ops_main.c
**
**  Description:    This file contains the file transfer server main functions 
**                  and state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_OP_INCLUDED) && (BTA_OP_INCLUDED)

#include <string.h>

#include "bta_fs_api.h"
#include "bta_ops_int.h"
#include "gki.h"
#include "obx_api.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/

/* state machine states */
enum
{
  BTA_OPS_IDLE_ST = 0,      /* Idle  */
  BTA_OPS_LISTEN_ST,        /* Listen - waiting for OBX/RFC connection */
  BTA_OPS_CONN_ST,          /* Connected - OPP Session is active */
  BTA_OPS_CLOSING_ST        /* Closing is in progress */
};

/* state machine action enumeration list */
enum
{
    BTA_OPS_API_DISABLE,
    BTA_OPS_API_ACCESSRSP,
    BTA_OPS_API_CLOSE,
    BTA_OPS_CI_WRITE,
    BTA_OPS_CI_READ,
    BTA_OPS_CI_OPEN,
    BTA_OPS_OBX_CONNECT,
    BTA_OPS_OBX_DISC,
    BTA_OPS_OBX_CLOSE,
    BTA_OPS_OBX_ABORT,
    BTA_OPS_OBX_PUT,
    BTA_OPS_OBX_GET,
    BTA_OPS_CLOSE_COMPLETE,
    BTA_OPS_IGNORE
};

/* type for action functions */
typedef void (*tBTA_OPS_ACTION)(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);

/* action function list */
const tBTA_OPS_ACTION bta_ops_action[] =
{
    bta_ops_api_disable,
    bta_ops_api_accessrsp,
    bta_ops_api_close,
    bta_ops_ci_write,
    bta_ops_ci_read,
    bta_ops_ci_open,
    bta_ops_obx_connect,
    bta_ops_obx_disc,
    bta_ops_obx_close,
    bta_ops_obx_abort,
    bta_ops_obx_put,
    bta_ops_obx_get,
    bta_ops_close_complete
};


/* state table information */
#define BTA_OPS_ACTIONS             1       /* number of actions */
#define BTA_OPS_NEXT_STATE          1       /* position of next state */
#define BTA_OPS_NUM_COLS            2       /* number of columns in state tables */

/* state table for idle state */
static const UINT8 bta_ops_st_idle[][BTA_OPS_NUM_COLS] =
{
/* Event                          Action 1               Next state */
/* BTA_OPS_API_DISABLE_EVT   */   {BTA_OPS_IGNORE,       BTA_OPS_IDLE_ST},
/* BTA_OPS_API_ACCESSRSP_EVT */   {BTA_OPS_IGNORE,       BTA_OPS_IDLE_ST},
/* BTA_OPS_API_CLOSE_EVT     */   {BTA_OPS_IGNORE,       BTA_OPS_IDLE_ST},
};

/* state table for obex/rfcomm connection state */
static const UINT8 bta_ops_st_listen[][BTA_OPS_NUM_COLS] =
{
/* Event                          Action 1               Next state */
/* BTA_OPS_API_DISABLE_EVT   */   {BTA_OPS_API_DISABLE,  BTA_OPS_IDLE_ST},
/* BTA_OPS_API_ACCESSRSP_EVT */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_API_CLOSE_EVT     */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_CI_OPEN_EVT       */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_CI_WRITE_EVT      */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_CI_READ_EVT       */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_CONN_EVT      */   {BTA_OPS_OBX_CONNECT,  BTA_OPS_CONN_ST},
/* BTA_OPS_OBX_DISC_EVT      */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_ABORT_EVT     */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_CLOSE_EVT     */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_PUT_EVT       */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_GET_EVT       */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST},
/* BTA_OPS_CLOSE_CMPL_EVT    */   {BTA_OPS_IGNORE,       BTA_OPS_LISTEN_ST}
};

/* state table for open state */
static const UINT8 bta_ops_st_connected[][BTA_OPS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_OPS_API_DISABLE_EVT   */   {BTA_OPS_API_DISABLE,     BTA_OPS_IDLE_ST},
/* BTA_OPS_API_ACCESSRSP_EVT */   {BTA_OPS_API_ACCESSRSP,   BTA_OPS_CONN_ST},
/* BTA_OPS_API_CLOSE_EVT     */   {BTA_OPS_API_CLOSE,       BTA_OPS_CLOSING_ST},
/* BTA_OPS_CI_OPEN_EVT       */   {BTA_OPS_CI_OPEN,         BTA_OPS_CONN_ST},
/* BTA_OPS_CI_WRITE_EVT      */   {BTA_OPS_CI_WRITE,        BTA_OPS_CONN_ST},
/* BTA_OPS_CI_READ_EVT       */   {BTA_OPS_CI_READ,         BTA_OPS_CONN_ST},
/* BTA_OPS_OBX_CONN_EVT      */   {BTA_OPS_IGNORE,          BTA_OPS_CONN_ST},
/* BTA_OPS_OBX_DISC_EVT      */   {BTA_OPS_OBX_DISC,        BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_ABORT_EVT     */   {BTA_OPS_OBX_ABORT,       BTA_OPS_CONN_ST},
/* BTA_OPS_OBX_CLOSE_EVT     */   {BTA_OPS_OBX_CLOSE,       BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_PUT_EVT       */   {BTA_OPS_OBX_PUT,         BTA_OPS_CONN_ST},
/* BTA_OPS_OBX_GET_EVT       */   {BTA_OPS_OBX_GET,         BTA_OPS_CONN_ST},
/* BTA_OPS_CLOSE_CMPL_EVT    */   {BTA_OPS_IGNORE,          BTA_OPS_CONN_ST}
};

/* state table for closing state */
static const UINT8 bta_ops_st_closing[][BTA_OPS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_OPS_API_DISABLE_EVT   */   {BTA_OPS_API_DISABLE,     BTA_OPS_IDLE_ST},
/* BTA_OPS_API_ACCESSRSP_EVT */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_API_CLOSE_EVT     */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_CI_OPEN_EVT       */   {BTA_OPS_CLOSE_COMPLETE,  BTA_OPS_LISTEN_ST},
/* BTA_OPS_CI_WRITE_EVT      */   {BTA_OPS_CLOSE_COMPLETE,  BTA_OPS_LISTEN_ST},
/* BTA_OPS_CI_READ_EVT       */   {BTA_OPS_CLOSE_COMPLETE,  BTA_OPS_LISTEN_ST},
/* BTA_OPS_OBX_CONN_EVT      */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_DISC_EVT      */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_ABORT_EVT     */   {BTA_OPS_OBX_ABORT,       BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_CLOSE_EVT     */   {BTA_OPS_OBX_CLOSE,       BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_PUT_EVT       */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_OBX_GET_EVT       */   {BTA_OPS_IGNORE,          BTA_OPS_CLOSING_ST},
/* BTA_OPS_CLOSE_CMPL_EVT    */   {BTA_OPS_CLOSE_COMPLETE,  BTA_OPS_LISTEN_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_OPS_ST_TBL)[BTA_OPS_NUM_COLS];

/* state table */
const tBTA_OPS_ST_TBL bta_ops_st_tbl[] =
{
    bta_ops_st_idle,
    bta_ops_st_listen,
    bta_ops_st_connected,
    bta_ops_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* OPS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_OPS_CB  bta_ops_cb;
#endif

#if BTA_OPS_DEBUG == TRUE
static char *ops_evt_code(tBTA_OPS_INT_EVT evt_code);
static char *ops_state_code(tBTA_OPS_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_ops_sm_execute
**
** Description      State machine event handling function for OPS
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_sm_execute(tBTA_OPS_CB *p_cb, UINT16 event, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_ST_TBL     state_table;
    UINT8               action;
    int                 i;
#if BTA_OPS_DEBUG == TRUE
    tBTA_OPS_STATE in_state = bta_ops_cb.state;
    UINT16         in_event = event;
    APPL_TRACE_EVENT3("OPS Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                      ops_state_code(in_state),
                      ops_evt_code(event));
#endif

    /* look up the state table for the current state */
    state_table = bta_ops_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_OPS_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_OPS_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_OPS_IGNORE)
        {
            (*bta_ops_action[action])(p_cb, p_data);
        }
        else
        {
            /* discard ops data */
            bta_ops_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }

#if BTA_OPS_DEBUG == TRUE
    if (in_state != bta_ops_cb.state)
    {
        APPL_TRACE_DEBUG3("OPS State Change: [%s] -> [%s] after Event [%s]",
                      ops_state_code(in_state),
                      ops_state_code(bta_ops_cb.state),
                      ops_evt_code(in_event));
    }
#endif
}

/*******************************************************************************
**
** Function         bta_ops_api_enable
**
** Description      Handle an api enable event.  This function enables the OP
**                  Server by opening an Obex/Rfcomm channel and placing it into
**                  listen mode.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_ops_api_enable(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_API_ENABLE *p_enable = &p_data->api_enable;
    
    /* initialize control block */
    memset(p_cb, 0, sizeof(tBTA_OPS_CB));
    p_cb->p_cback = p_enable->p_cback;
    p_cb->app_id = p_enable->app_id;
    p_cb->srm = p_enable->srm;
    p_cb->fd = BTA_FS_INVALID_FD;

    bta_ops_cb.state = BTA_OPS_LISTEN_ST;
    
    /* call enable action function */
    bta_ops_enable(p_cb, p_enable);
}

/*******************************************************************************
**
** Function         bta_ops_hdl_event
**
** Description      File transfer server main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_ops_hdl_event(BT_HDR *p_msg)
{
#if BTA_OPS_DEBUG == TRUE
    tBTA_OPS_STATE in_state = bta_ops_cb.state;
#endif

    switch (p_msg->event)
    {
        case BTA_OPS_API_ENABLE_EVT:
#if BTA_OPS_DEBUG == TRUE
            APPL_TRACE_EVENT3("OPS Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                              ops_state_code(in_state),
                              ops_evt_code(p_msg->event));
#endif
            bta_ops_api_enable(&bta_ops_cb, (tBTA_OPS_DATA *) p_msg);

#if BTA_OPS_DEBUG == TRUE
            if (in_state != bta_ops_cb.state)
            {
                APPL_TRACE_DEBUG3("OPS State Change: [%s] -> [%s] after Event [%s]",
                              ops_state_code(in_state),
                              ops_state_code(bta_ops_cb.state),
                              ops_evt_code(p_msg->event));
            }
#endif
            break;

        default:

            bta_ops_sm_execute(&bta_ops_cb, p_msg->event, (tBTA_OPS_DATA *) p_msg);

            if ( bta_ops_cb.state == BTA_OPS_CONN_ST )
            {
                if (( bta_ops_cb.pm_state == BTA_OPS_PM_IDLE )
                  &&( bta_ops_cb.obx_oper != OPS_OP_NONE ))
                {
                    /* inform power manager */
                    APPL_TRACE_DEBUG0("BTA OPS informs DM/PM busy state");
                    bta_sys_busy( BTA_ID_OPS ,bta_ops_cb.app_id, bta_ops_cb.bd_addr);
                    bta_ops_cb.pm_state = BTA_OPS_PM_BUSY;
                }
                else if (( bta_ops_cb.pm_state == BTA_OPS_PM_BUSY )
                       &&( bta_ops_cb.obx_oper == OPS_OP_NONE ))
                {
                    /* inform power manager */
                    APPL_TRACE_DEBUG0("BTA OPS informs DM/PM idle state");
                    bta_sys_idle( BTA_ID_OPS ,bta_ops_cb.app_id, bta_ops_cb.bd_addr);
                    bta_ops_cb.pm_state = BTA_OPS_PM_IDLE;
                }
            }
            else if ( bta_ops_cb.state == BTA_OPS_LISTEN_ST )
            {
                /* initialize power management state */
                bta_ops_cb.pm_state = BTA_OPS_PM_BUSY;
            }

            break;
    }

    return (TRUE);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_OPS_DEBUG == TRUE

/*******************************************************************************
**
** Function         ops_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ops_evt_code(tBTA_OPS_INT_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_OPS_API_DISABLE_EVT:
        return "BTA_OPS_API_DISABLE_EVT";
    case BTA_OPS_API_ACCESSRSP_EVT:
        return "BTA_OPS_API_ACCESSRSP_EVT";
    case BTA_OPS_API_CLOSE_EVT:
        return "BTA_OPS_API_CLOSE_EVT";
    case BTA_OPS_CI_OPEN_EVT:
        return "BTA_OPS_CI_OPEN_EVT";
    case BTA_OPS_CI_WRITE_EVT:
        return "BTA_OPS_CI_WRITE_EVT";
    case BTA_OPS_CI_READ_EVT:
        return "BTA_OPS_CI_READ_EVT";
    case BTA_OPS_OBX_CONN_EVT:
        return "BTA_OPS_OBX_CONN_EVT";
    case BTA_OPS_OBX_DISC_EVT:
        return "BTA_OPS_OBX_DISC_EVT";
    case BTA_OPS_OBX_ABORT_EVT:
        return "BTA_OPS_OBX_ABORT_EVT";
    case BTA_OPS_OBX_CLOSE_EVT:
        return "BTA_OPS_OBX_CLOSE_EVT";
    case BTA_OPS_OBX_PUT_EVT:
        return "BTA_OPS_OBX_PUT_EVT";
    case BTA_OPS_OBX_GET_EVT:
        return "BTA_OPS_OBX_GET_EVT";
    case BTA_OPS_CLOSE_CMPL_EVT:
        return "BTA_OPS_CLOSE_CMPL_EVT";
    case BTA_OPS_API_ENABLE_EVT:
        return "BTA_OPS_API_ENABLE_EVT";
    default:
        return "unknown OPS event code";
    }
}

/*******************************************************************************
**
** Function         ops_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ops_state_code(tBTA_OPS_STATE state_code)
{
    switch(state_code)
    {
    case BTA_OPS_IDLE_ST:
        return "BTA_OPS_IDLE_ST";
    case BTA_OPS_LISTEN_ST:
        return "BTA_OPS_LISTEN_ST";
    case BTA_OPS_CONN_ST:
        return "BTA_OPS_CONN_ST";
    case BTA_OPS_CLOSING_ST:
        return "BTA_OPS_CLOSING_ST";
    default:
        return "unknown OPS state code";
    }
}

#endif  /* Debug Functions */
#endif /* BTA_OP_INCLUDED */
