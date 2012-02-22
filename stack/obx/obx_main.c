/*****************************************************************************
**
**  Name:         obx_main.c
**
**  File:         OBEX common API and interface to other Bluetooth modules
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bt_target.h"
#include "obx_int.h"
#include "btu.h"    /* for timer */
#include "l2c_api.h"
#include "l2cdefs.h"

#if (defined (BT_USE_TRACES) && BT_USE_TRACES == TRUE)
const char * const obx_cl_state_name [] = 
{
    "NULL",
    "NOT_CONN",
    "SESS_RS",
    "CONN_RS",
    "UNAUTH",
    "CONN",
    "DISCNT_RS",
    "OP_UNAUTH",
    "SETPATH_RS",
    "ACT_RS",
    "ABORT_RS",
    "PUT_RS",
    "GET_RS",
    "PUT",
    "GET",
    "PUT_S",
    "GET_S",
    "PART",
    "Unknown"
};

const char * const obx_cl_event_name [] = 
{
    "CONN_R",
    "SESS_R",
    "DISCNT_R",
    "PUT_R",
    "GET_R",
    "SETPATH_R",
    "ACT_R",
    "ABORT_R",
    "OK_C",
    "CONT_C",
    "FAIL_C",
    "PORT_CLS",
    "TX_EMPTY",
    "FCS_SET",
    "STATE",
    "TIMEOUT",
    "Unknown"
};

const char * const obx_sr_state_name [] = 
{
    "NULL",
    "NOT_CONN",
    "SESS_I",
    "CONN_I",
    "WAIT_AUTH",
    "AUTH_I",
    "CONN",
    "DISCNT_I",
    "SETPATH_I",
    "ACT_I",
    "ABORT_I",
    "PUT_I",
    "GET_I",
    "PUT",
    "GET",
    "PUT_S",
    "GET_S",
    "PART",
    "WAIT_CLS",
    "Unknown"
};

const char * const obx_sr_event_name [] = 
{
    "CONN_R",
    "SESS_R",
    "DISCNT_R",
    "PUT_R",
    "GET_R",
    "SETPATH_R",
    "ACT_R",
    "ABORT_R",
    "CONN_C",
    "SESS_C",
    "DISCNT_C",
    "PUT_C",
    "GET_C",
    "SETPATH_C",
    "ACT_C",
    "ABORT_C",
    "PORT_CLS",
    "FCS_SET",
    "STATE",
    "TIMEOUT",
    "BAD_REQ",
    "TX_EMPTY",
    "Unknown"
};
#endif

#if OBX_DYNAMIC_MEMORY == FALSE
OBX_API tOBX_CB obx_cb;
#endif


#if (defined (BT_USE_TRACES) && BT_USE_TRACES == TRUE)
#if (OBX_CLIENT_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_cl_get_state_name
** Returns      The client state name.
*******************************************************************************/
const char *obx_cl_get_state_name(tOBX_CL_STATE state)
{
    const char * p_str = obx_cl_state_name[OBX_CS_MAX];
    if (state < OBX_CS_MAX)
    {
        p_str = obx_cl_state_name[state];
    }
    return p_str;
}

/*******************************************************************************
** Function     obx_cl_get_event_name
** Returns      The client event name.
*******************************************************************************/
const char *obx_cl_get_event_name(tOBX_CL_EVENT event)
{
    const char * p_str = obx_cl_event_name[OBX_MAX_CEVT];
    if (event < OBX_MAX_CEVT)
    {
        p_str = obx_cl_event_name[event];
    }
    return p_str;
}
#endif

#if (OBX_SERVER_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_sr_get_state_name
** Returns      The server state name.
*******************************************************************************/
const char *obx_sr_get_state_name(tOBX_SR_STATE state)
{
    const char * p_str = obx_sr_state_name[OBX_SS_MAX];
    if (state < OBX_SS_MAX)
    {
        p_str = obx_sr_state_name[state];
    }
    return p_str;
}

/*******************************************************************************
** Function     obx_sr_get_event_name
** Returns      The server event name.
*******************************************************************************/
const char *obx_sr_get_event_name(tOBX_SR_EVENT event)
{
    const char * p_str = obx_sr_event_name[OBX_MAX_SEVT];
    if (event < OBX_MAX_SEVT)
    {
        p_str = obx_sr_event_name[event];
    }
    return p_str;
}
#endif
#endif

/*******************************************************************************
** Function     obx_start_timer
** Description  start BTU timer.
*******************************************************************************/
void obx_start_timer(tOBX_COMM_CB *p_pcb)
{
    UINT16 type = 0;
    UINT32 timeout = obx_cb.timeout_val;

    if (timeout)
    {
        if (p_pcb->handle & OBX_CL_HANDLE_MASK)
        {
            if (p_pcb->p_txmsg && p_pcb->p_txmsg->event == OBX_DISCONNECT_REQ_EVT)
            {
                timeout = OBX_DISC_TOUT_VALUE;
            }
            type = BTU_TTYPE_OBX_CLIENT_TO;
        }
        else
        {
            if (p_pcb->p_txmsg && p_pcb->p_txmsg->event == (OBX_DISCNT_CFM_SEVT + 1))
            {
                timeout = OBX_DISC_TOUT_VALUE;
            }
            type = BTU_TTYPE_OBX_SERVER_TO;
        }
        btu_start_timer(&p_pcb->tle, type, timeout);
    }
    OBX_TRACE_DEBUG3("obx_start_timer type: %d, val:%d, p_tle:0x%x", type, timeout, &p_pcb->tle);
}

/*******************************************************************************
** Function     obx_stop_timer
** Description  Stop BTU timer
*******************************************************************************/
void obx_stop_timer(TIMER_LIST_ENT *p_tle)
{
    btu_stop_timer(p_tle);
    OBX_TRACE_DEBUG1("obx_stop_timer p_tle:0x%x", p_tle);
}

#if (OBX_CLIENT_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_cl_timeout
** Description  Get client control block from timer param. Start BTU timer again.
**              Call application callback routine with OBX_TIMEOUT_EVT event.
*******************************************************************************/
void obx_cl_timeout(TIMER_LIST_ENT *p_tle)
{
    tOBX_CL_CB      *p_cb = (tOBX_CL_CB *) p_tle->param;
    tOBX_EVT_PARAM  evtp;
    tOBX_HANDLE     handle = p_cb->ll_cb.comm.handle;
    tOBX_CL_CBACK   *p_cback = p_cb->p_cback;

    memset(&evtp, 0, sizeof(tOBX_EVT_PARAM));
    if (obx_cb.timeout_val)
        btu_start_timer(p_tle, BTU_TTYPE_OBX_CLIENT_TO, obx_cb.timeout_val);
    obx_csm_event(p_cb, OBX_TIMEOUT_CEVT, NULL);
    (*p_cback) (handle, OBX_TIMEOUT_EVT, OBX_RSP_DEFAULT, evtp, NULL);
}

/*******************************************************************************
**
** Function     obx_cl_alloc_cb
**
** Description  allocate a client control block.
**
*******************************************************************************/
tOBX_CL_CB *obx_cl_alloc_cb(void)
{
    int         xx, yy;
    tOBX_CL_CB  *p_cb = NULL;

    /* allocate a client control block */
    for (xx=0, yy=obx_cb.next_ind; xx<obx_cb.num_client; xx++, yy++)
    {
        if (yy >= obx_cb.num_client)
            yy = 0;
        p_cb = &obx_cb.client[yy];

        if (p_cb->ll_cb.comm.handle == OBX_HANDLE_NULL)
        {
            p_cb->ll_cb.comm.handle   = OBX_CL_HANDLE_MASK | (yy + 1);
            obx_cb.next_ind     = yy+1; /* it will be adjusted, so we do not need to check the range now */
            OBX_TRACE_DEBUG3("obx_cl_alloc_cb obx handle:0x%x, yy:%d, next: %d",
                p_cb->ll_cb.comm.handle, yy, obx_cb.next_ind);
            p_cb->ll_cb.comm.tx_mtu = 0;
            p_cb->conn_id     = OBX_INVALID_CONN_ID;

            p_cb->ll_cb.comm.tle.param = (UINT32)p_cb;
            p_cb->psm         = 0;
            p_cb->srm         = 0;
            break;
        }
    }

    if (xx == obx_cb.num_client)
        p_cb = NULL;
    return p_cb;
}

/*******************************************************************************
**
** Function     obx_cl_get_cb
**
** Description  Returns the pointer to the client control block with given handle.
**
*******************************************************************************/
tOBX_CL_CB *obx_cl_get_cb(tOBX_HANDLE handle)
{
    tOBX_CL_CB *p_cb = NULL;
    UINT8       ind  = (handle & OBX_CL_CB_IND_MASK);

    if (handle & OBX_CL_HANDLE_MASK)
    {
        if (ind <= obx_cb.num_client && ind > 0)
        {
            if (obx_cb.client[--ind].ll_cb.comm.handle == handle)
                p_cb = &obx_cb.client[ind];
        }
    }

    return p_cb;
}

/*******************************************************************************
**
** Function     obx_cl_get_suspended_cb
**
** Description  Returns the pointer to the client control block with given handle.
**
*******************************************************************************/
tOBX_CL_CB *obx_cl_get_suspended_cb(tOBX_HANDLE *p_handle, UINT8 *p_session_info)
{
    tOBX_HANDLE handle = *p_handle;
    tOBX_CL_CB *p_cb = NULL;
    tOBX_CL_CB *p_ccb = NULL;
    UINT8       ind  = (handle & OBX_CL_CB_IND_MASK);

    OBX_TRACE_DEBUG1("obx_cl_get_suspended_cb handle: 0x%x", handle);
    if (handle & OBX_CL_HANDLE_MASK)
    {
        if (ind <= obx_cb.num_client && ind > 0)
        {
            if (obx_cb.client[--ind].ll_cb.comm.handle == handle)
            {
                p_ccb = &obx_cb.client[ind];
                if (p_ccb->sess_st == OBX_SESS_SUSPENDED && 
                    ((p_session_info == p_ccb->sess_info) || (memcmp(p_session_info, p_ccb->sess_info, OBX_SESSION_INFO_SIZE) == 0)))
                {
                    OBX_TRACE_DEBUG0("found a suspended session");
                    p_cb = p_ccb;
                }
            }
        }
    }
    if (p_cb == NULL) /* if the handle is NULL, assume the system was power cycled. attempt to allocate a control block */
    {
        p_cb = obx_cl_alloc_cb();
        if (p_cb)
        {
            *p_handle       = p_cb->ll_cb.comm.handle;
            p_cb->sess_st   = OBX_SESS_SUSPENDED;
            memcpy(p_cb->sess_info, p_session_info, OBX_SESSION_INFO_SIZE);
            OBX_TRACE_DEBUG1("allocated a suspended session handle: 0x%x", *p_handle);
        }
    }

    return p_cb;
}

/*******************************************************************************
**
** Function     obx_cl_free_cb
**
** Description  Free the given client control block.
**
** Returns      void
**
*******************************************************************************/
void obx_cl_free_cb(tOBX_CL_CB * p_cb)
{
    if (p_cb)
    {
        OBX_TRACE_DEBUG2("obx_cl_free_cb id: %d, sess_st:%d", p_cb->ll_cb.comm.id, p_cb->sess_st);

        if (p_cb->ll_cb.comm.id>0)
        {
            if (p_cb->ll_cb.comm.p_send_fn == (tOBX_SEND_FN *)obx_rfc_snd_msg)
                obx_cb.hdl_map[p_cb->ll_cb.port.port_handle - 1] = 0;
            else
                obx_cb.l2c_map[p_cb->ll_cb.l2c.lcid - L2CAP_BASE_APPL_CID] = 0;
        }

        /* make sure the GKI buffers are freed */
        if (p_cb->p_next_req)
            GKI_freebuf(p_cb->p_next_req);

        if (p_cb->p_saved_req)
            GKI_freebuf(p_cb->p_saved_req);

        if (p_cb->p_auth)
            GKI_freebuf(p_cb->p_auth);

        obx_free_buf (&p_cb->ll_cb);

        if (p_cb->psm)
            L2CA_DEREGISTER (p_cb->psm);

        /* make sure the timer is stopped */
        btu_stop_timer(&p_cb->ll_cb.comm.tle);

        memset(p_cb, 0, sizeof(tOBX_CL_CB) );
    }
}
#endif /* OBX_CLIENT_INCLUDED */

#if (OBX_SERVER_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_find_suspended_session
** Description  if p_triplet is NULL,
**                  check if there's still room for a reliable session
**              else check if the given session is still in the suspended list
*******************************************************************************/
tOBX_SPND_CB *obx_find_suspended_session (tOBX_SR_SESS_CB *p_scb, tOBX_TRIPLET *p_triplet, UINT8 num)
{
    UINT8   ind;
    BOOLEAN found = FALSE;
    BOOLEAN infinite = FALSE;
    UINT8   *p, xx;
    tOBX_SR_CB      *p_cb = obx_sr_get_cb(p_scb->handle);
    tOBX_SPND_CB    *p_spndcb, *p_ret = NULL;

    OBX_TRACE_DEBUG0("obx_find_suspended_session ");
    if (p_triplet == NULL)
    {
        if (p_cb->p_suspend)
        {
            for (xx=0, p_spndcb=p_cb->p_suspend; xx<p_cb->max_suspend; xx++, p_spndcb++)
            {
                if ((p_spndcb->state == OBX_SS_NULL) || (memcmp(p_spndcb->peer_addr, p_scb->peer_addr, BD_ADDR_LEN)==0))
                {
                    OBX_TRACE_DEBUG3("[%d] state: %d, BDA: %08x", xx, p_spndcb->state,
                   (p_spndcb->peer_addr[2]<<24)+(p_spndcb->peer_addr[3]<<16)+(p_spndcb->peer_addr[4]<<8)+p_spndcb->peer_addr[5]);
                    /* this entry is not used yet or overwriting the entry with the same address */
                    found = TRUE;
                    break;
                }
                else if (p_spndcb->stle.param == 0)
                {
                    infinite = TRUE;
                }
            }
            OBX_TRACE_DEBUG2("found: %d infinite:%d", found, infinite);
            if (found == FALSE)
                found = infinite;
        }
        else if (p_cb->max_suspend > 0)
        {
            found = TRUE;
        }
        p_ret = (tOBX_SPND_CB *)p_scb;
    }
    else if (p_cb->p_suspend)
    {
        ind = obx_read_triplet(p_triplet, num, OBX_TAG_SESS_PARAM_SESS_ID);
        if (ind < num && p_triplet[ind].len == OBX_SESSION_ID_SIZE)
        {
            p = p_triplet[ind].p_array;
            for (xx=0, p_spndcb=p_cb->p_suspend; xx<p_cb->max_suspend; xx++, p_spndcb++)
            {
                OBX_TRACE_DEBUG5("[%d] state: %d/%d, ssn:%d offset:x%x", xx, p_spndcb->state,
                    p_spndcb->sess_info[OBX_SESSION_INFO_ST_IDX], p_spndcb->ssn, p_spndcb->offset);
                if ((p_spndcb->state != OBX_SS_NULL) &&
                    (memcmp (p, p_spndcb->sess_info, OBX_SESSION_ID_SIZE) == 0))
                {
                    obxu_dump_hex (p_spndcb->sess_info, "sess info", OBX_SESSION_INFO_SIZE);
                    /* prepare p_scb to the proper state for resume */
                    p_ret = p_spndcb;
                    break;
                }
            }
        }
    }
    return p_ret;
}

/*******************************************************************************
** Function     obx_sr_sess_timeout
** Description  Get suspended session control block from timer param. 
**              mark the suspended session as NULL
*******************************************************************************/
void obx_sr_sess_timeout(TIMER_LIST_ENT *p_tle)
{
    tOBX_SPND_CB    *p_spndcb = (tOBX_SPND_CB *) p_tle->param;

    OBX_TRACE_DEBUG0("obx_sr_sess_timeout");
    p_spndcb->state = OBX_SS_NULL;
}

/*******************************************************************************
** Function     obx_sr_timeout
** Description  Get server control block from timer param. Start BTU timer again.
**              Call application callback routine with OBX_TIMEOUT_EVT event.
*******************************************************************************/
void obx_sr_timeout(TIMER_LIST_ENT *p_tle)
{
    tOBX_SR_SESS_CB *p_scb = (tOBX_SR_SESS_CB *) p_tle->param;
    tOBX_EVT_PARAM  evtp;
    tOBX_HANDLE     handle = p_scb->ll_cb.comm.handle;
    tOBX_SR_CBACK   *p_cback;
    tOBX_SR_CB      *p_cb;

    memset(&evtp, 0, sizeof(tOBX_EVT_PARAM));
    if (obx_cb.timeout_val)
        btu_start_timer(p_tle, BTU_TTYPE_OBX_SERVER_TO, obx_cb.timeout_val);
    p_cb = &obx_cb.server[p_scb->handle - 1];
    p_cback = p_cb->p_cback;
    obx_ssm_event(p_scb, OBX_TIMEOUT_SEVT, NULL);
    (*p_cback) (handle, OBX_TIMEOUT_EVT, evtp, NULL);
}

/*******************************************************************************
**
** Function     obx_sr_alloc_cb
**
** Description  allocate a server control block.
**
*******************************************************************************/
tOBX_HANDLE obx_sr_alloc_cb(tOBX_StartParams *p_params)
{
    int         xx, yy, zz;
    tOBX_SR_CB  *p_cb = &obx_cb.server[0];
    tOBX_SR_SESS_CB *p_scb = &obx_cb.sr_sess[0];
    tOBX_HANDLE obx_handle = OBX_HANDLE_NULL;

    OBX_TRACE_DEBUG1("obx_sr_alloc_cb num sess: %d", p_params->max_sessions);
    /* allocate a server control block */
    for (xx=0; xx<obx_cb.num_server; xx++, p_cb++)
    {
        if (p_cb->scn == 0 && p_cb->psm == 0)
        {
            obx_handle  = xx + 1;
            p_cb->scn   = p_params->scn;
            p_cb->psm   = p_params->psm;
            break;
        }
    }

    if (xx != obx_cb.num_server)
    {
        /* allocate session control blocks */
        zz = 0;
        for (yy=0; yy<obx_cb.num_sr_sess && zz < p_params->max_sessions; yy++, p_scb++)
        {
            if (p_scb->ll_cb.comm.handle == OBX_HANDLE_NULL)
            {
                p_scb->handle   = obx_handle;
                if (p_params->get_nonf)
                    p_scb->srmp = OBX_SRMP_NONF_EVT;
                p_cb->sess[zz]  = yy + 1;
                p_scb->srm      = p_params->srm;
                p_scb->ll_cb.comm.handle = OBX_ENC_SESS_HANDLE((obx_handle), zz);
                p_scb->ll_cb.comm.tx_mtu = 0;
                p_scb->ll_cb.comm.tle.param = (UINT32)p_scb;
                OBX_TRACE_DEBUG2("[%d]: 0x%x", zz, p_scb->ll_cb.comm.handle);
                zz++;
            }
        }

        if (zz == p_params->max_sessions && L2C_IS_VALID_PSM(p_params->psm))
        {
            if (obx_l2c_sr_register(p_cb) != OBX_SUCCESS)
            {
                OBX_TRACE_ERROR0("Cannot register to L2CAP");
                zz = 0; /* let it fail */
            }
        }

        if (zz != p_params->max_sessions)
        {
            OBX_TRACE_ERROR0("not enough resources: release the allocated ones");
            p_cb->scn   = 0;
            p_cb->psm   = 0;
            obx_handle  = OBX_HANDLE_NULL;
            p_scb = &obx_cb.sr_sess[0];
            for (yy=0; yy<obx_cb.num_sr_sess; yy++, p_scb++)
            {
                if (p_scb->handle == obx_handle)
                {
                    p_scb->ll_cb.comm.handle = OBX_HANDLE_NULL;
                }
            }
        }
        else
        {
            p_cb->num_sess  = p_params->max_sessions;
            p_cb->nonce     = p_params->nonce;
            p_cb->max_suspend       = p_params->max_suspend;
            if (p_cb->nonce && (p_cb->max_suspend == 0))
                p_cb->max_suspend = 1;
            if (p_cb->max_suspend > OBX_MAX_SUSPEND_SESSIONS)
                p_cb->max_suspend = OBX_MAX_SUSPEND_SESSIONS;
            if ((p_cb->max_suspend * sizeof (tOBX_SPND_CB)) > GKI_MAX_BUF_SIZE)
            {
                OBX_TRACE_ERROR1("OBX_MAX_SUSPEND_SESSIONS:%d is too big", OBX_MAX_SUSPEND_SESSIONS);
            }
        }
    }
    return obx_handle;
}

/*******************************************************************************
**
** Function     obx_sr_get_cb
**
** Description  Returns the pointer to the server control block with given handle.
**
*******************************************************************************/
tOBX_SR_CB * obx_sr_get_cb(tOBX_HANDLE handle)
{
    tOBX_SR_CB *p_cb = NULL;
    tOBX_HANDLE obx_handle = OBX_DEC_HANDLE(handle);

    /* check range */
    if (obx_handle <= obx_cb.num_server && obx_handle > 0 && obx_cb.server[obx_handle-1].p_cback)
    {
        p_cb = &obx_cb.server[obx_handle-1];
    }

    return p_cb;
}

/*******************************************************************************
**
** Function     obx_sr_get_scb
**
** Description  Returns the pointer to the server control block with given handle.
**
*******************************************************************************/
tOBX_SR_SESS_CB * obx_sr_get_scb(tOBX_HANDLE handle)
{
    tOBX_SR_CB      *p_cb;
    tOBX_SR_SESS_CB *p_scb = NULL;
    tOBX_HANDLE obx_handle;
    UINT16      sess_ind;

    /* check range */
    obx_handle = OBX_DEC_HANDLE(handle) - 1;
    if (obx_handle < obx_cb.num_server)
    {
        /* make sure the handle is a valid one */
        p_cb = &obx_cb.server[obx_handle];
        sess_ind = OBX_DEC_SESS_IND(handle);
        if ((sess_ind < p_cb->num_sess))
        {
            if ((obx_cb.sr_sess[p_cb->sess[sess_ind]-1].ll_cb.comm.handle) == handle)
                p_scb = &obx_cb.sr_sess[p_cb->sess[sess_ind]-1];
        }
    }

    return p_scb;
}

/*******************************************************************************
**
** Function     obx_sr_free_cb
**
** Description  Free the given server control block.
**
** Returns      void
**
*******************************************************************************/
void obx_sr_free_cb(tOBX_HANDLE handle)
{
    tOBX_SR_CB * p_cb = obx_sr_get_cb(handle);
    tOBX_SR_SESS_CB *p_scb;
    int yy;

    OBX_TRACE_DEBUG1("obx_sr_free_cb handle:0x%x", handle);
    /* check range */
    if (p_cb)
    {
        p_scb = &obx_cb.sr_sess[0];
        for (yy=0; yy<obx_cb.num_sr_sess; yy++, p_scb++)
        {
            if (OBX_DEC_HANDLE(p_scb->handle) == OBX_DEC_HANDLE(handle))
            {
                obx_sr_free_scb(p_scb);
                if (p_scb->ll_cb.comm.id>0)
                {
                    if (p_scb->ll_cb.comm.p_send_fn == (tOBX_SEND_FN *)obx_rfc_snd_msg)
                        obx_cb.hdl_map[p_scb->ll_cb.port.port_handle - 1] = 0;
                    else
                        obx_cb.l2c_map[p_scb->ll_cb.l2c.lcid - L2CAP_BASE_APPL_CID] = 0;
                }
                memset(p_scb, 0, sizeof(tOBX_SR_SESS_CB) );
            }
        }

        if (p_cb->p_auth)
        {
            GKI_freebuf(p_cb->p_auth);
            p_cb->p_auth = 0;
        }

        memset(p_cb, 0, sizeof(tOBX_SR_CB) );
    }
}

/*******************************************************************************
**
** Function     obx_sr_free_scb
**
** Description  Free the given server session control block.
**
** Returns      void
**
*******************************************************************************/
void obx_sr_free_scb(tOBX_SR_SESS_CB *p_scb)
{
    OBX_TRACE_DEBUG2("obx_sr_free_scb shandle:0x%x, sess_st:%d", p_scb->ll_cb.comm.handle, p_scb->sess_st);

    /* make sure the GKI buffers are freed */
    if (p_scb->p_saved_msg)
    {
        GKI_freebuf(p_scb->p_saved_msg);
        p_scb->p_saved_msg = 0;
    }

    if (p_scb->p_next_req)
    {
        GKI_freebuf(p_scb->p_next_req);
        p_scb->p_next_req = 0;
    }
    obx_free_buf (&p_scb->ll_cb);

    /* make sure the timer is stopped */
    btu_stop_timer(&p_scb->ll_cb.comm.tle);
}

/*******************************************************************************
**
** Function     obx_sr_get_next_conn_id
**
** Description  assigns the next available connection id.  It will avoid using
**              active conn id instances as well so that we can work with the
**              IVT stack bugs.
**
*******************************************************************************/
UINT32 obx_sr_get_next_conn_id(void)
{
#if (OBX_CLIENT_INCLUDED == TRUE)
    tOBX_CL_CB  *p_ccb;
    int          xx;
    BOOLEAN      done;

    /* Make sure no client instances are using the value */
    do
    {
        obx_cb.next_cid++;

        /* Increment the value and make sure it is legal */
        if (obx_cb.next_cid == OBX_INVALID_CONN_ID)
            obx_cb.next_cid = OBX_INITIAL_CONN_ID;

        done = TRUE;
        for (xx=0, p_ccb = &obx_cb.client[0]; xx < obx_cb.num_client; xx++, p_ccb++)
        {
            /* If the handle is in use and same as proposed conn_id, increment and restart */
            if (p_ccb->ll_cb.comm.handle != OBX_HANDLE_NULL && p_ccb->conn_id == obx_cb.next_cid)
            {
                OBX_TRACE_WARNING1(" **** OBX CONN_ID Collision (0x%08x)  trying another...", obx_cb.next_cid);
                done = FALSE;
                break;
            }
        }
    } while (!done);

#else   /* Client code is not compiled in */

    obx_cb.next_cid++;

    /* Increment the value and make sure it is legal */
    if (obx_cb.next_cid == OBX_INVALID_CONN_ID)
        obx_cb.next_cid = OBX_INITIAL_CONN_ID;
#endif

    return obx_cb.next_cid;
}

#endif /* OBX_SERVER_INCLUDED */

/*******************************************************************************
**
** Function     obx_port_handle_2cb
**
** Description  Given a port handle, return the associated client or server
**              control block.
**
** Returns      
**
*******************************************************************************/
tOBX_PORT_CB * obx_port_handle_2cb(UINT16 port_handle)
{
    tOBX_PORT_CB    *p_pcb = NULL;
    tOBX_HANDLE     obx_handle = 0;
#if (OBX_SERVER_INCLUDED == TRUE)
    tOBX_SR_CB      *p_cb;
#endif

    /* this function is called by obx_rfc_cback() only.
     * assume that port_handle is within range */
    obx_handle  = obx_cb.hdl_map[port_handle-1];
    OBX_TRACE_DEBUG2("obx_port_handle_2cb port_handle:%d obx_handle:0x%x", port_handle, obx_handle);

    if (obx_handle > 0)
    {
        if (obx_handle & OBX_CL_HANDLE_MASK)
        {
#if (OBX_CLIENT_INCLUDED == TRUE)
            obx_handle &= OBX_CL_CB_IND_MASK;
            p_pcb = &obx_cb.client[obx_handle - 1].ll_cb.port;
#endif /* OBX_CLIENT_INCLUDED */
        }
        else
        {
#if (OBX_SERVER_INCLUDED == TRUE)
            p_cb = &obx_cb.server[OBX_DEC_HANDLE(obx_handle) - 1];
            p_pcb = &obx_cb.sr_sess[p_cb->sess[OBX_DEC_SESS_IND(obx_handle)]-1].ll_cb.port;
            OBX_TRACE_DEBUG1("p_pcb port_handle:%d", p_pcb->port_handle);
#endif /* OBX_SERVER_INCLUDED */
        }
    }

    return p_pcb;
}



/*******************************************************************************
**
** Function     OBX_Init
**
** Description  This function is called to initialize the control block for this
**              layer. It must be called before accessing any other of its API 
**              functions.  It is typically called once during the start up of
**              the stack.
**
** Returns      void.
**
*******************************************************************************/
void OBX_Init(void)
{
    memset(&obx_cb, 0, sizeof(tOBX_CB));

#if defined(OBX_INITIAL_TRACE_LEVEL)
    obx_cb.trace_level  = OBX_INITIAL_TRACE_LEVEL;
#else
    obx_cb.trace_level  = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif

#if (OBX_CLIENT_INCLUDED == TRUE)
    obx_cb.next_ind     = 0;
    obx_cb.num_client   = OBX_NUM_CLIENTS;
#endif

#if (OBX_SERVER_INCLUDED == TRUE)
    obx_cb.num_server   = OBX_NUM_SERVERS;
    obx_cb.num_sr_sess  = OBX_NUM_SR_SESSIONS;
    obx_cb.next_cid     = OBX_INITIAL_CONN_ID;
#endif
    obx_cb.timeout_val  = OBX_TIMEOUT_VALUE;
    obx_cb.sess_tout_val= OBX_SESS_TIMEOUT_VALUE;
    obx_cb.max_rx_qcount= OBX_MAX_RX_QUEUE_COUNT;
}

/*******************************************************************************
**
** Function         OBX_SetTraceLevel
**
** Description      This function sets the debug trace level for OBX.
**                  If 0xff is passed, the current trace level is returned.
**
**                  Input Parameters:
**                      level:  The level to set the OBX tracing to:
**                      0xff-returns the current setting.
**                      0-turns off tracing.
**                      >= 1-Errors.
**                      >= 2-Warnings.
**                      >= 3-APIs.
**                      >= 4-Events.
**                      >= 5-Debug.
**
** Returns          The new trace level or current trace level if
**                  the input parameter is 0xff.
**
*******************************************************************************/
UINT8 OBX_SetTraceLevel (UINT8 level)
{
    if (level != 0xFF)
        obx_cb.trace_level = level;

    return (obx_cb.trace_level);
}

/*******************************************************************************
** Function     OBX_HandleToMtu
**
** Description  Given an OBEX handle, return the associated peer MTU.
**
** Returns      MTU.
**
*******************************************************************************/
UINT16 OBX_HandleToMtu(tOBX_HANDLE handle)
{
    UINT16  mtu = OBX_MIN_MTU;
    BOOLEAN rx  = (handle & OBX_HANDLE_RX_MTU_MASK);
#if (OBX_CLIENT_INCLUDED == TRUE)
    int     xx;
#endif
#if (OBX_SERVER_INCLUDED == TRUE)
    tOBX_SR_SESS_CB *p_scb;
#endif

    handle &= ~OBX_HANDLE_RX_MTU_MASK;

    if (handle != OBX_HANDLE_NULL)
    {
        if (handle & OBX_CL_HANDLE_MASK)
        {
#if (OBX_CLIENT_INCLUDED == TRUE)
            /* a client handle */
            for (xx=0; xx<obx_cb.num_client; xx++)
            {
                if (handle == obx_cb.client[xx].ll_cb.comm.handle)
                {
                    mtu = (rx) ? obx_cb.client[xx].ll_cb.comm.rx_mtu : obx_cb.client[xx].ll_cb.comm.tx_mtu;
                    break;
                }
            }
#endif
        }
        else
        {
#if (OBX_SERVER_INCLUDED == TRUE)
            /* a server handle */
            p_scb = obx_sr_get_scb(handle);
            /* make sure the handle is a valid one */
            if (p_scb && p_scb->ll_cb.comm.handle != OBX_HANDLE_NULL)
            {
                mtu = (rx) ? p_scb->ll_cb.comm.rx_mtu : p_scb->ll_cb.comm.tx_mtu;
            }
#endif
        }
    }

    if (mtu < OBX_MIN_MTU)
        mtu = OBX_MIN_MTU;
    OBX_TRACE_DEBUG3("OBX_HandleToMtu handle: 0x%x, rx:%x, mtu:%d", handle, rx, mtu);
    return mtu;
}

