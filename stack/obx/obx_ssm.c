/*****************************************************************************
**
**  Name:         obx_ssm.c
**
**  File:         OBEX Server State Machine and Control Block Access Functions
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "btu.h"    /* for timer */
#include "obx_int.h"

/* OBEX Server Action Functions Enums (must match obx_sr_action [below] */
enum
{
    OBX_SA_CLOSE_PORT,
    OBX_SA_CONNECTION_ERROR,
    OBX_SA_STATE,
    OBX_SA_CONNECT_IND,
    OBX_SA_WC_CONN_IND,
    OBX_SA_NC_TO,
    OBX_SA_CONNECT_RSP,
    OBX_SA_AUTH_IND,
    OBX_SA_SND_RSP,
    OBX_SA_SAVE_REQ,
    OBX_SA_SND_PART,
    OBX_SA_REJ_REQ,
    OBX_SA_ABORT_RSP,
    OBX_SA_OP_RSP,
    OBX_SA_GET_IND,
    OBX_SA_GET_REQ,
    OBX_SA_SESSION_IND,
    OBX_SA_SESS_CONN_IND,
    OBX_SA_WC_SESS_IND,
    OBX_SA_SESSION_RSP,
    OBX_SA_PUT_IND,
    OBX_SA_SRM_PUT_REQ,
    OBX_SA_SRM_PUT_RSP,
    OBX_SA_SRM_GET_FCS,
    OBX_SA_SRM_GET_RSP,
    OBX_SA_SRM_GET_REQ,
    OBX_SA_CLEAN_PORT
};

/* OBEX Server Action Functions */
static const tOBX_SR_ACT obx_sr_action[] = 
{
    obx_sa_close_port,
    obx_sa_connection_error,
    obx_sa_state,
    obx_sa_connect_ind,
    obx_sa_wc_conn_ind,
    obx_sa_nc_to,
    obx_sa_connect_rsp,
    obx_sa_auth_ind,
    obx_sa_snd_rsp,
    obx_sa_save_req,
    obx_sa_snd_part,
    obx_sa_rej_req,
    obx_sa_abort_rsp,
    obx_sa_op_rsp,
    obx_sa_get_ind,
    obx_sa_get_req,
    obx_sa_session_ind,
    obx_sa_sess_conn_ind,
    obx_sa_wc_sess_ind,
    obx_sa_session_rsp,
    obx_sa_put_ind,
    obx_sa_srm_put_req,
    obx_sa_srm_put_rsp,
    obx_sa_srm_get_fcs,
    obx_sa_srm_get_rsp,
    obx_sa_srm_get_req,
    obx_sa_clean_port
};

/************ OBX Server FSM State/Event Indirection Table **************/
/* obx_ssm_event() first looks at obx_ssm_entry_map[][] to get an entry of the event of a particular state
 * 0 means the event in the current state is ignored.
 * a number with 0x80 bit set, use obx_sr_all_table[][] as the "state table".
 * other numbers, look up obx_sr_main_state_table[] for the state table of current state.
 *
 * once the state table is determined,
 * look up the "action" column to find the associated action function
 * and the "next state" column to find the "next state" candidate.
 *
 * The actual next state could be either the state in the "next state" column
 * or the state returned from the action function.
 */
static const UINT8 obx_ssm_entry_map[][OBX_SS_MAX-1] =
{
/* state name:  NtCon SesIn CntIn WtAut AutIn Conn  DscIn StpIn ActIn AbtIn PutIn GetIn Put   Get   PutS  GetS  Part WtCls */
/* CONN_R   */{ 1,    0x82, 0x82, 1,    0x82, 0x86, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82 ,1    },
/* SESS_R   */{ 3,    0x82, 0x82, 0x82, 0x82, 0x88, 3,    0x85, 0x85, 0x85, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 1    ,3    },
/* DISCNT_R */{ 0x86, 0x82, 0x82, 0x82, 0x82, 0x85, 0x82, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 1    ,0x86 },
/* PUT_R    */{ 0x86, 0x82, 0x82, 0x82, 0x82, 1,    0x87, 0x87, 0x87, 0x82, 0x87, 0x87, 1,    0x82, 1,    0x82, 0x82 ,0x86 },
/* GET_R    */{ 0x86, 0x82, 0x82, 0x82, 0x82, 2,    0x87, 0x87, 0x87, 0x82, 0x87, 0x87, 0x82, 1,    0x82, 1,    0x82 ,0x86 },
/* SETPATH_R*/{ 0x86, 0x82, 0x82, 0x82, 0x82, 3,    0x87, 0x87, 0x87, 0x82, 0x87, 0x87, 0x82, 0x82, 0x82, 0x82, 0x82 ,0x86 },
/* ACTION_R */{ 0x86, 0x82, 0x82, 0x82, 0x82, 4,    0x87, 0x87, 0x87, 0x82, 0x87, 0x87, 0x82, 0x82, 0x82, 0x82, 0x82 ,0x86 },
/* ABORT_R  */{ 0x86, 0x82, 0x82, 0x82, 0x82, 0x86, 0x82, 0x82, 0x82, 0x82, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 1    ,0x86 },
/* CONN_C   */{ 0,    0,    1,    0,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0    ,0    },
/* SESS_C   */{ 0,    1,    0,    0,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0    ,0    },
/* DISCNT_C */{ 0,    0x82, 0x82, 0x82, 0x82, 0x82, 1,    0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82 ,0    },
/* PUT_C    */{ 0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    1,    0,    0,    0,    2,    0,    0    ,0    },
/* GET_C    */{ 0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    1,    0,    0,    0,    2,    0    ,0    },
/* SETPATH_C*/{ 0,    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0    ,0    },
/* ACTION_C */{ 0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0,    0,    0    ,0    },
/* ABORT_C  */{ 0,    0,    0,    0,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0    ,0    },
/* PORT_CLS */{ 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83 ,0x83 },
/* FCS_SET  */{ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    2    ,0    },
/* STATE    */{ 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84 ,0x84 },
/* TIMEOUT  */{ 2,    0,    0,    2,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0    ,2    },
/* BAD_REQ  */{ 0x86, 0x82, 0x82, 0x82, 0x82, 0x86, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82 ,0x86 },
/* TX_EMPTY */{ 2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0    ,0    }
};

static const UINT8 obx_sr_all_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* ABORT_R  */{OBX_SM_NO_ACTION,        OBX_SS_ABORT_INDICATED },
/* misc     */{OBX_SA_CLOSE_PORT,       OBX_SS_NOT_CONNECTED },
/* PORT_CLS */{OBX_SA_CONNECTION_ERROR, OBX_SS_NOT_CONNECTED },
/* STATE    */{OBX_SA_STATE,            OBX_SS_NULL },
/* DISCNT_R */{OBX_SM_NO_ACTION,        OBX_SS_DISCNT_INDICATED },
/* misc     */{OBX_SA_REJ_REQ,          OBX_SS_NULL },
/* illegalop*/{OBX_SA_CLEAN_PORT,       OBX_SS_NOT_CONNECTED },
/* SESS_R   */{OBX_SA_SESSION_IND,      OBX_SS_SESS_INDICATED }
};

static const UINT8 obx_sr_not_conn_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_R   */{OBX_SA_CONNECT_IND,      OBX_SS_CONN_INDICATED },
/* TIMEOUT  */{OBX_SA_NC_TO,            OBX_SS_NOT_CONNECTED},
/* SESS_R   */{OBX_SA_SESS_CONN_IND,    OBX_SS_SESS_INDICATED }
};

static const UINT8 obx_sr_connect_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_C   */{OBX_SA_CONNECT_RSP,      OBX_SS_CONNECTED }
};

static const UINT8 obx_sr_session_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* SESS_R   */{OBX_SA_SESSION_RSP,      OBX_SS_NOT_CONNECTED }
};

static const UINT8 obx_sr_wait_auth_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_R   */{OBX_SA_AUTH_IND,         OBX_SS_AUTH_INDICATED },
/* TIMEOUT  */{OBX_SA_CLOSE_PORT,       OBX_SS_NOT_CONNECTED}
};

static const UINT8 obx_sr_conn_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_SA_PUT_IND,          OBX_SS_PUT_INDICATED },
/* GET_R    */{OBX_SA_GET_IND,          OBX_SS_GET_INDICATED },
/* SETPATH_R*/{OBX_SM_NO_ACTION,        OBX_SS_SETPATH_INDICATED },
/* ACTION_R */{OBX_SM_NO_ACTION,        OBX_SS_ACTION_INDICATED }
};

static const UINT8 obx_sr_disconnect_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* DISCNT_C */{OBX_SA_SND_RSP,          OBX_SS_WAIT_CLOSE },
/* TIMEOUT  */{OBX_SA_CLOSE_PORT,       OBX_SS_NOT_CONNECTED},
/* SESS_R   */{OBX_SA_SESSION_IND,      OBX_SS_DISCNT_INDICATED}
};

static const UINT8 obx_sr_setpath_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* SETPATH_C*/{OBX_SA_SND_RSP,          OBX_SS_CONNECTED }
};

static const UINT8 obx_sr_abort_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* GET/PUT_C*/{OBX_SA_OP_RSP,           OBX_SS_ABORT_INDICATED },
/* ABORT_C  */{OBX_SA_ABORT_RSP,        OBX_SS_CONNECTED }
};

static const UINT8 obx_sr_put_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_C    */{OBX_SA_SND_RSP,          OBX_SS_PUT_TRANSACTION }
};

static const UINT8 obx_sr_get_ind_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* GET_C    */{OBX_SA_SND_RSP,          OBX_SS_GET_TRANSACTION }
};

static const UINT8 obx_sr_put_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_SM_NO_ACTION,        OBX_SS_PUT_INDICATED }
};

static const UINT8 obx_sr_get_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* GET_R    */{OBX_SA_GET_REQ,          OBX_SS_GET_INDICATED }
};

static const UINT8 obx_sr_put_s_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* PUT_R    */{OBX_SA_SRM_PUT_REQ,      OBX_SS_PUT_SRM },
/* PUT_C    */{OBX_SA_SRM_PUT_RSP,      OBX_SS_PUT_SRM }
};

static const UINT8 obx_sr_get_s_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* GET_R    */{OBX_SA_SRM_GET_REQ,      OBX_SS_GET_SRM },
/* GET_C    */{OBX_SA_SRM_GET_RSP,      OBX_SS_GET_SRM },
/* FCS_SET  */{OBX_SA_SRM_GET_FCS,      OBX_SS_GET_SRM }
};

static const UINT8 obx_sr_part_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* ABORT_R  */{OBX_SA_SAVE_REQ,         OBX_SS_NULL },/* and DISCNT_R */
/* FCS_SET  */{OBX_SA_SND_PART,         OBX_SS_NULL }
};

static const UINT8 obx_sr_wait_close_table[][OBX_SM_NUM_COLS] = {
/* Event       Action                   Next State */
/* CONN_R   */{OBX_SA_WC_CONN_IND,      OBX_SS_CONN_INDICATED },
/* TIMEOUT  */{OBX_SA_NC_TO,            OBX_SS_NOT_CONNECTED},
/* SESS_R   */{OBX_SA_WC_SESS_IND,      OBX_SS_SESS_INDICATED }
};

static const tOBX_SM_TBL obx_sr_main_state_table[] = {
    obx_sr_not_conn_table,
    obx_sr_session_ind_table,
    obx_sr_connect_ind_table,
    obx_sr_wait_auth_table,
    obx_sr_connect_ind_table, /* same table for auth ind */
    obx_sr_conn_table,
    obx_sr_disconnect_ind_table,
    obx_sr_setpath_ind_table,
    obx_sr_setpath_ind_table, /* same action table for action_ind */
    obx_sr_abort_ind_table,
    obx_sr_put_ind_table,
    obx_sr_get_ind_table,
    obx_sr_put_table,
    obx_sr_get_table,
    obx_sr_put_s_table,
    obx_sr_get_s_table,
    obx_sr_part_table,
    obx_sr_wait_close_table
};

/*******************************************************************************
**
** Function     obx_ssm_event
**
** Description  Handle events to the server state machine. It looks up the entry
**              in the obx_ssm_entry_map array. If it is a valid entry, it gets
**              the state table.Set the next state, if not NULL state.Execute
**              the action function according to the state table. If the state
**              returned by action function is not NULL state, adjust the new
**              state to the returned state.If (api_evt != MAX), call callback
**              function.
**
** Returns      void.
**
*******************************************************************************/
void obx_ssm_event(tOBX_SR_SESS_CB *p_scb, tOBX_SR_EVENT event, BT_HDR *p_msg)
{
    UINT8           curr_state = p_scb->state;
    tOBX_SM_TBL     state_table;
    UINT8           action, entry;
    tOBX_SR_STATE   act_state = OBX_SS_NULL;
    UINT8           *p_data;
    UINT16          len;
    BT_HDR          *p_dummy;
    tOBX_EVENT      api_evt;
    tOBX_SR_CB      *p_cb;
#if 0
    UINT8           srm;
#endif

    if( curr_state == OBX_SS_NULL || curr_state >= OBX_SS_MAX)
    {
        OBX_TRACE_WARNING1( "Invalid state: %d", curr_state) ;
        if(p_msg)
            GKI_freebuf(p_msg);
        return;
    }

    OBX_TRACE_DEBUG6( "For Server SHandle 0x%x, State: %s, Event: %s/%d srm:0x%x ssn:%d",
        p_scb->ll_cb.comm.handle, obx_sr_get_state_name( p_scb->state ),
        obx_sr_get_event_name(event), event, p_scb->srm, p_scb->ssn ) ;

    /* look up the state table for the current state */
    /* lookup entry /w event & curr_state */
    /* If entry is ignore, return.
     * Otherwise, get state table (according to curr_state or all_state) */
    /* coverity [index_parm] */ 
    if( (entry = obx_ssm_entry_map[event][curr_state-1]) != OBX_SM_IGNORE )
    {
        if(entry&OBX_SM_ALL)
        {
            entry &= OBX_SM_ENTRY_MASK;
            state_table = obx_sr_all_table;
        }
        else
            state_table = obx_sr_main_state_table[curr_state-1];
    }
    else
    {
        OBX_TRACE_WARNING2( "Ignore event %d in state %d", event, curr_state );
        if(p_msg)
            GKI_freebuf(p_msg);
        return;
    }

    /* Get possible next state from state table. */
    if( state_table[entry-1][OBX_SME_NEXT_STATE] != OBX_CS_NULL )
        p_scb->state = state_table[entry-1][OBX_SME_NEXT_STATE];
    p_scb->prev_state = curr_state;
    OBX_TRACE_DEBUG3( "possible new state = %s/%s/%d",
        obx_sr_get_state_name(p_scb->state), obx_sr_get_state_name( p_scb->prev_state ), p_scb->prev_state) ;

    /* If action is not ignore, clear param, exec action and get next state.
     * The action function may set the Param for cback.
     * Depending on param, call cback or free buffer. */
    /* execute action */
    action = state_table[entry-1][OBX_SME_ACTION];
    if (action != OBX_SM_NO_ACTION)
    {
        act_state = (*obx_sr_action[action])(p_scb, p_msg);
    }

    /* adjust next state, if it needs to use the new state returned from action function */
    if( act_state != OBX_CS_NULL)
    {
        p_scb->state = act_state;
        OBX_TRACE_DEBUG1( "new state = %s (action)", obx_sr_get_state_name( p_scb->state ) ) ;
    }

    if(p_scb->api_evt)
    {
        api_evt         = p_scb->api_evt;
        p_scb->api_evt   = OBX_NULL_EVT;
        /* we do not want the operation to be challenged by the client */
        if( event <= OBX_SEVT_MAX_REQ && event != OBX_CONNECT_REQ_SEVT &&
            OBX_ReadByteStrHdr(p_msg, OBX_HI_CHALLENGE, &p_data, &len, 0) == TRUE)
        {
            /* send bad request response */
            p_dummy = obx_build_dummy_rsp(p_scb, OBX_RSP_BAD_REQUEST);
            event += OBX_SEVT_DIFF_REQ_CFM;
            obx_ssm_event(p_scb, event, p_dummy);    
            GKI_freebuf(p_msg);
        }
        else
        {
            p_cb = &obx_cb.server[p_scb->handle - 1];
            (p_cb->p_cback) (p_scb->ll_cb.comm.handle, api_evt, p_scb->param, p_msg);
        }
        memset(&p_scb->param, 0, sizeof (p_scb->param) );
    }
    else if(action == OBX_SM_NO_ACTION && p_msg)
            GKI_freebuf(p_msg);


    OBX_TRACE_DEBUG2( "result state = %s ssn:%d", obx_sr_get_state_name( p_scb->state ), p_scb->ssn ) ;
}


