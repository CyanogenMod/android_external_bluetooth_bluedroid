/*****************************************************************************
**
**  Name:         obx_csm.c
**
**  File:         OBEX Client State Machine and Control Block Access Functions
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "btu.h"    /* for timer */
#include "obx_int.h"

/* OBEX Client Action Functions Enums (must match obx_cl_action [below] */
enum
{
    OBX_CA_SND_REQ,
    OBX_CA_NOTIFY,
    OBX_CA_CONNECT_ERROR,
    OBX_CA_STATE,
    OBX_CA_CLOSE_PORT,
    OBX_CA_CONNECT_FAIL,
    OBX_CA_DISCNT_REQ,
    OBX_CA_START_TIMER,
    OBX_CA_FAIL_RSP,
    OBX_CA_SND_PART,
    OBX_CA_CONNECT_OK,
    OBX_CA_SESSION_OK,
    OBX_CA_SESSION_CONT,
    OBX_CA_SESSION_GET,
    OBX_CA_SESSION_FAIL,
    OBX_CA_ABORT,
    OBX_CA_SND_PUT_REQ,
    OBX_CA_SND_GET_REQ,
    OBX_CA_SRM_SND_REQ,
    OBX_CA_SRM_PUT_REQ,
    OBX_CA_SRM_GET_REQ,
    OBX_CA_SRM_PUT_NOTIFY,
    OBX_CA_SRM_GET_NOTIFY,
    OBX_CA_SAVE_RSP,
    OBX_CA_SAVE_REQ
};

/* OBEX Client Action Functions */
static const tOBX_CL_ACT obx_cl_action[] = 
{
    obx_ca_snd_req,
    obx_ca_notify,
    obx_ca_connect_error,
    obx_ca_state,
    obx_ca_close_port,
    obx_ca_connect_fail,
    obx_ca_discnt_req,
    obx_ca_start_timer,
    obx_ca_fail_rsp,
    obx_ca_snd_part,
    obx_ca_connect_ok,
    obx_ca_session_ok,
    obx_ca_session_cont,
    obx_ca_session_get,
    obx_ca_session_fail,
    obx_ca_abort,
    obx_ca_snd_put_req,
    obx_ca_snd_get_req,
    obx_ca_srm_snd_req,
    obx_ca_srm_put_req,
    obx_ca_srm_get_req,
    obx_ca_srm_put_notify,
    obx_ca_srm_get_notify,
    obx_ca_save_rsp,
    obx_ca_save_req
};

/************ OBX Client FSM State/Event Indirection Table **************/
/* obx_csm_event() first looks at obx_csm_entry_map[][] to get an entry of the event of a particular state
 * 0 means the event in the current state is ignored.
 * a number with 0x80 bit set, use obx_cl_all_table[][] as the "state table".
 * other numbers, look up obx_cl_main_state_table[] for the state table of current state.
 *
 * once the state table is determined,
 * look up the "action" column to find the associated action function
 * and the "next state" column to find the "next state" candidate.
 *
 * The actual next state could be either the state in the "next state" column
 * or the state returned from the action function.
 */
static const UINT8 obx_csm_entry_map[][OBX_CS_MAX-1] =
{
/* state name:  NtCon SesRs ConRs UnAut Conn  DscRs OpUna StpRs ActRs AbtRs PutRs GetRs Put   Get   PutS  GetS  Part */
/* CONN_R   */{ 1,    0,    0,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
/* SESS_R   */{ 2,    0,    0,    0,    5,    0,    0,    0,    0,    0,    2,    2,    2,    2,    5,    5,    1 },
/* DISCNT_R */{ 0,    0x87, 2,    0x87, 0x81, 0x87, 0x87, 0x81, 0x81, 0x81, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87 },
/* PUT_R    */{ 0,    0,    0,    0,    1,    0,    1,    0,    0,    0,    0,    0,    1,    0,    2,    0,    0 },
/* GET_R    */{ 0,    3,    0,    0,    2,    0,    2,    0,    0,    0,    0,    0,    0,    1,    0,    2,    0 },
/* SETPATH_R*/{ 0,    0,    0,    0,    3,    0,    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
/* ACT_R    */{ 0,    0,    0,    0,    6,    0,    5,    0,    0,    0,    0x82, 0x82, 0x82, 0x82, 3,    3,    1 },
/* ABORT_R  */{ 0,    0,    0,    0,    4,    0,    4,    0,    0,    0,    0x82, 0x82, 0x82, 0x82, 3,    3,    1 },
/* OK_C     */{ 0,    1,    3,    0,    0,    1,    0,    0x83, 0x83, 0x83, 0x83, 0x83, 0,    0,    0x83, 0x83, 3 },
/* CONT_C   */{ 0,    2,    0,    0,    0,    2,    0,    0,    0,    1,    1,    1,    0,    0,    4,    4,    3 },
/* FAIL_C   */{ 0,    4,    1,    0,    0,    0x87, 0,    0x86, 0x86, 2,    0x86, 0x86, 0,    0,    0x86, 0x86, 3 },
/* PORT_CLS */{ 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84 },
/* TX_EMPTY */{ 0x87, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
/* FCS_SET  */{ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    2 },
/* STATE    */{ 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85 },
/* TIMEOUT  */{ 3,    2,    0x87, 0,    0,    0x87, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};

static const UINT8 obx_cl_all_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* DISCNT_R */{OBX_CA_SND_REQ,          OBX_CS_DISCNT_REQ_SENT },
/* ABORT_R  */{OBX_CA_SND_REQ,          OBX_CS_ABORT_REQ_SENT },
/* OK_C     */{OBX_CA_NOTIFY,           OBX_CS_NULL },
/* PORT_CLS */{OBX_CA_CONNECT_ERROR,    OBX_CS_NOT_CONNECTED },
/* STATE    */{OBX_CA_STATE,            OBX_CS_NULL },
/* FAIL_C   */{OBX_CA_FAIL_RSP,         OBX_CS_NULL },
/* end      */{OBX_CA_CLOSE_PORT,       OBX_CS_NOT_CONNECTED }
};

static const UINT8 obx_cl_not_conn_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_R   */{OBX_CA_SND_REQ,          OBX_CS_CONNECT_REQ_SENT },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT },
/* TIMEOUT  */{OBX_CA_SESSION_FAIL,     OBX_CS_NOT_CONNECTED }
};

static const UINT8 obx_cl_sess_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* OK_C     */{OBX_CA_SESSION_OK,       OBX_CS_NOT_CONNECTED },
/* CONT_C   */{OBX_CA_SESSION_CONT,     OBX_CS_SESSION_REQ_SENT },
/* GET_R    */{OBX_CA_SESSION_GET,      OBX_CS_SESSION_REQ_SENT },
/* FAIL_C   */{OBX_CA_SESSION_FAIL,     OBX_CS_NOT_CONNECTED }
};

static const UINT8 obx_cl_conn_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* FAIL_C   */{OBX_CA_CONNECT_FAIL,     OBX_CS_UNAUTH },
/* DISCNT_R */{OBX_CA_DISCNT_REQ,       OBX_CS_NULL },
/* OK_C     */{OBX_CA_CONNECT_OK,       OBX_CS_NULL }
};

static const UINT8 obx_cl_unauth_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_R   */{OBX_CA_SND_REQ,          OBX_CS_CONNECT_REQ_SENT }
};

static const UINT8 obx_cl_conn_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_CA_SND_PUT_REQ,      OBX_CS_PUT_REQ_SENT },
/* GET_R    */{OBX_CA_SND_GET_REQ,      OBX_CS_GET_REQ_SENT },
/* SETPATH_R*/{OBX_CA_SND_REQ,          OBX_CS_SETPATH_REQ_SENT },
/* ABORT_R  */{OBX_CA_ABORT,            OBX_CS_CONNECTED },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT },
/* ACT_R    */{OBX_CA_SND_REQ,          OBX_CS_ACTION_REQ_SENT }
};

static const UINT8 obx_cl_discnt_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* OK_C     */{OBX_CA_NOTIFY,           OBX_CS_NOT_CONNECTED },  /* and close port */
/* CONT_C   */{OBX_CA_START_TIMER,      OBX_CS_DISCNT_REQ_SENT }
};

static const UINT8 obx_cl_op_unauth_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_CA_SND_REQ,          OBX_CS_PUT_REQ_SENT },
/* GET_R    */{OBX_CA_SND_REQ,          OBX_CS_GET_REQ_SENT },
/* SETPATH_R*/{OBX_CA_SND_REQ,          OBX_CS_SETPATH_REQ_SENT },
/* ABORT_R  */{OBX_SM_NO_ACTION,        OBX_CS_CONNECTED },
/* ACT_R    */{OBX_CA_SND_REQ,          OBX_CS_ACTION_REQ_SENT }
};

/* static const UINT8 obx_cl_setpath_rs_table[][OBX_SM_NUM_COLS] = { */
/* Event       Action                   Next State */
/* DISCNT_R   {OBX_CA_SND_REQ,          OBX_CS_DISCNT_REQ_SENT },*/
/* OK_C       {OBX_CA_NOTIFY,           OBX_CS_NULL },*/
/* FAIL_C     {OBX_CA_FAIL_RSP,         OBX_CS_NULL },*/
/* PORT_CLS   {OBX_CA_CONNECT_ERROR,    OBX_CS_NOT_CONNECTED },*/
/* STATE      {OBX_CA_STATE,            OBX_CS_NULL },*/
/* }; */

static const UINT8 obx_cl_abort_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONT_C   */{OBX_CA_START_TIMER,      OBX_CS_ABORT_REQ_SENT },
/* FAIL_C   */{OBX_CA_NOTIFY,           OBX_CS_CONNECTED }
};

static const UINT8 obx_cl_put_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONT_C   */{OBX_CA_NOTIFY,           OBX_CS_PUT_TRANSACTION },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_get_rs_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONT_C   */{OBX_CA_NOTIFY,           OBX_CS_GET_TRANSACTION },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_put_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_CA_SND_REQ,          OBX_CS_PUT_REQ_SENT },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_get_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* GET_R    */{OBX_CA_SND_REQ,          OBX_CS_GET_REQ_SENT },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_put_s_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* DISCNT_R */{OBX_CA_SRM_SND_REQ,      OBX_CS_DISCNT_REQ_SENT },
/* PUT_R    */{OBX_CA_SRM_PUT_REQ,      OBX_CS_PUT_SRM },
/* ABORT_R  */{OBX_CA_SRM_SND_REQ,      OBX_CS_ABORT_REQ_SENT },
/* CONT_C   */{OBX_CA_SRM_PUT_NOTIFY,   OBX_CS_PUT_SRM },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_get_s_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* DISCNT_R */{OBX_CA_SRM_SND_REQ,      OBX_CS_DISCNT_REQ_SENT },
/* GET_R    */{OBX_CA_SRM_GET_REQ,      OBX_CS_GET_SRM },
/* ABORT_R  */{OBX_CA_SRM_SND_REQ,      OBX_CS_ABORT_REQ_SENT },
/* CONT_C   */{OBX_CA_SRM_GET_NOTIFY,   OBX_CS_GET_SRM },
/* SESS_R   */{OBX_CA_SND_REQ,          OBX_CS_SESSION_REQ_SENT }
};

static const UINT8 obx_cl_part_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* ABORT_R  */{OBX_CA_SAVE_REQ,         OBX_CS_PARTIAL_SENT },
/* FCS_SET  */{OBX_CA_SND_PART,         OBX_CS_NULL },
/* FAIL_C   */{OBX_CA_SAVE_RSP,         OBX_CS_NULL }
};

static const tOBX_SM_TBL obx_cl_main_state_table[] = {
    obx_cl_not_conn_table,
    obx_cl_sess_rs_table,
    obx_cl_conn_rs_table,
    obx_cl_unauth_table,
    obx_cl_conn_table,
    obx_cl_discnt_rs_table,
    obx_cl_op_unauth_table,
    NULL, /* obx_cl_setpath_rs_table */
    NULL, /* obx_cl_action_rs_table */
    obx_cl_abort_rs_table,
    obx_cl_put_rs_table,
    obx_cl_get_rs_table,
    obx_cl_put_table,
    obx_cl_get_table,
    obx_cl_put_s_table,
    obx_cl_get_s_table,
    obx_cl_part_table
};

/*******************************************************************************
**
** Function     obx_csm_event
**
** Description  Handle events to the client state machine. It looks up the entry
**              in the obx_csm_entry_map array. If it is a valid entry, it gets
**              the state table. Set the next state, if not NULL state.Execute
**              the action function according to the state table. If the state
**              returned by action function is not NULL state, adjust the new
**              state to the returned state.If (api_evt != MAX), call callback
**              function.
**
** Returns      void.
**
*******************************************************************************/
void obx_csm_event(tOBX_CL_CB *p_cb, tOBX_CL_EVENT event, BT_HDR *p_msg)
{
    UINT8           curr_state = p_cb->state;
    tOBX_SM_TBL     state_table = NULL;
    UINT8           action, entry;
    tOBX_CL_STATE   act_state = OBX_CS_NULL;
    UINT8           prev_state = OBX_CS_NULL;

    if( curr_state == OBX_CS_NULL || curr_state >= OBX_CS_MAX)
    {
        OBX_TRACE_WARNING1( "Invalid state: %d", curr_state) ;
        if(p_msg)
            GKI_freebuf(p_msg);
        return;
    }
    OBX_TRACE_DEBUG4( "Client Handle 0x%x, State: %s, event: %s srm:0x%x",
        p_cb->ll_cb.comm.handle, obx_cl_get_state_name( p_cb->state ), obx_cl_get_event_name(event), p_cb->srm ) ;
    OBX_TRACE_DEBUG1("obx_csm_event csm offset:%d", p_cb->param.sess.obj_offset);

    /* look up the state table for the current state */
    /* lookup entry /w event & curr_state */
    /* If entry is ignore, return.
     * Otherwise, get state table (according to curr_state or all_state) */
    if( (entry = obx_csm_entry_map[event][curr_state-1]) != OBX_SM_IGNORE )
    {
        if(entry&OBX_SM_ALL)
        {
            entry &= OBX_SM_ENTRY_MASK;
            state_table = obx_cl_all_table;
        }
        else
            state_table = obx_cl_main_state_table[curr_state-1];
    }

    if( entry == OBX_SM_IGNORE || state_table == NULL)
    {
        OBX_TRACE_WARNING4( "Ignore event %s(%d) in state %s(%d)",
            obx_cl_get_event_name(event), event, obx_cl_get_state_name(curr_state), curr_state );
        if(p_msg)
            GKI_freebuf(p_msg);
        return;
    }

    /* Get possible next state from state table. */
    if( state_table[entry-1][OBX_SME_NEXT_STATE] != OBX_CS_NULL )
    {
        prev_state = p_cb->state;
        p_cb->state = state_table[entry-1][OBX_SME_NEXT_STATE];
        if (prev_state != p_cb->state)
        {
            p_cb->prev_state = prev_state;
            OBX_TRACE_DEBUG1( "saved state1:%s", obx_cl_get_state_name(p_cb->prev_state));
        }
    }
    OBX_TRACE_DEBUG1( "possible new state = %s", obx_cl_get_state_name( p_cb->state ) ) ;

    /* If action is not ignore, clear param, exec action and get next state.
     * The action function may set the Param for cback.
     * Depending on param, call cback or free buffer. */
    /* execute action */
    action = state_table[entry-1][OBX_SME_ACTION];
    if (action != OBX_SM_NO_ACTION)
    {
        act_state = (*obx_cl_action[action])(p_cb, p_msg);
    }

    /* adjust next state, if it needs to use the new state returned from action function */
    if( act_state != OBX_CS_NULL)
    {
        prev_state = p_cb->state;
        p_cb->state = act_state;
        OBX_TRACE_DEBUG1( "new state = %s (action)", obx_cl_get_state_name( p_cb->state )) ;
        if (prev_state != p_cb->state)
        {
            p_cb->prev_state = prev_state;
            OBX_TRACE_DEBUG1( "saved state2:%s", obx_cl_get_state_name(p_cb->prev_state));
        }
    }

    if(p_cb->api_evt)
    {
        (p_cb->p_cback) (p_cb->ll_cb.comm.handle, p_cb->api_evt, p_cb->rsp_code, p_cb->param, p_msg);
        p_cb->api_evt   = OBX_NULL_EVT;
        p_cb->rsp_code  = 0;
        memset(&p_cb->param, 0, sizeof (p_cb->param) );
    }
    else if(action == OBX_SM_NO_ACTION && p_msg)
            GKI_freebuf(p_msg);
    OBX_TRACE_DEBUG1("after csm offset:%d", p_cb->param.sess.obj_offset);

    OBX_TRACE_DEBUG2( "result state = %s/%d", obx_cl_get_state_name( p_cb->state ), p_cb->state ) ;
}


