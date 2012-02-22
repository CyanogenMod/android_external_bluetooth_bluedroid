/*****************************************************************************
**
**  Name:         obx_cact.c
**
**  File:         OBEX Client State Machine Action Functions
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "obx_int.h"
#include "btm_api.h"

const tOBX_EVENT obx_cl_state_2_event_map[] = 
{
    OBX_DISCONNECT_RSP_EVT, /* OBX_CS_NOT_CONNECTED */
    OBX_SESSION_RSP_EVT,    /* OBX_CS_SESSION_REQ_SENT */
    OBX_CONNECT_RSP_EVT,    /* OBX_CS_CONNECT_REQ_SENT */
    OBX_NULL_EVT,           /* OBX_CS_UNAUTH */
    OBX_ABORT_RSP_EVT,      /* OBX_CS_CONNECTED */
    OBX_DISCONNECT_RSP_EVT, /* OBX_CS_DISCNT_REQ_SENT */
    OBX_NULL_EVT,           /* OBX_CS_OP_UNAUTH */
    OBX_SETPATH_RSP_EVT,    /* OBX_CS_SETPATH_REQ_SENT */
    OBX_ACTION_RSP_EVT,     /* OBX_CS_ACTION_REQ_SENT */
    OBX_ABORT_RSP_EVT,      /* OBX_CS_ABORT_REQ_SENT */
    OBX_PUT_RSP_EVT,        /* OBX_CS_PUT_REQ_SENT */
    OBX_GET_RSP_EVT,        /* OBX_CS_GET_REQ_SENT */
    OBX_PUT_RSP_EVT,        /* OBX_CS_PUT_TRANSACTION */
    OBX_GET_RSP_EVT,        /* OBX_CS_GET_TRANSACTION */
    OBX_PUT_RSP_EVT,        /* OBX_CS_PUT_SRM */
    OBX_GET_RSP_EVT         /* OBX_CS_GET_SRM */
};

/*******************************************************************************
** Function     obx_ca_close_sess_req
** Description  send close session request
*******************************************************************************/
BT_HDR * obx_ca_close_sess_req(tOBX_CL_CB *p_cb)
{
    BT_HDR      *p_req;
    UINT8       *p;
    UINT8       num_trip = 0;
    tOBX_TRIPLET triplet[4];
    UINT8       data[2];

    p_req   = OBX_HdrInit(OBX_HANDLE_NULL, OBX_MIN_MTU);
    p = (UINT8 *) (p_req + 1) + p_req->offset;
    /* Session request packet always has the final bit set */
    *p++ = (OBX_REQ_SESSION | OBX_FINAL);
    p_req->len = 3;
    p = data;

    /* add session opcode */
    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_OP;
    triplet[num_trip].len = OBX_LEN_SESS_PARAM_SESS_OP;
    triplet[num_trip].p_array = p;
    *p = OBX_SESS_OP_CLOSE;
    p += OBX_LEN_SESS_PARAM_SESS_OP;
    num_trip++;

    /* add session id */
    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_ID;
    triplet[num_trip].len = OBX_SESSION_ID_SIZE;
    triplet[num_trip].p_array = p_cb->sess_info;
    num_trip++;

    OBX_AddTriplet(p_req, OBX_HI_SESSION_PARAM, triplet, num_trip);
    /* adjust the packet len */
    p = (UINT8 *) (p_req + 1) + p_req->offset + 1;
    UINT16_TO_BE_STREAM(p, p_req->len);
    p_req->event    = OBX_SESSION_REQ_EVT;
    p_cb->sess_st   = OBX_SESS_CLOSE;
    OBX_TRACE_DEBUG2("obx_ca_close_sess_req shandle:0x%x, sess_st:%d", p_cb->ll_cb.comm.handle, p_cb->sess_st);
    return p_req;
}

/*******************************************************************************
** Function     obx_ca_connect_req
** Description  send connect request
*******************************************************************************/
void obx_ca_connect_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    UINT8       msg[OBX_HDR_OFFSET + OBX_MAX_CONN_HDR_EXTRA];
    UINT8       *p = msg;

    /* Connect request packet always has the final bit set */
    *p++ = (OBX_REQ_CONNECT | OBX_FINAL);
    p += OBX_PKT_LEN_SIZE;

    *p++ = OBX_VERSION;
    *p++ = OBX_CONN_FLAGS;
    UINT16_TO_BE_STREAM(p, p_cb->ll_cb.port.rx_mtu);

    /* add session sequence number, if session is active */
    if (p_cb->sess_st == OBX_SESS_ACTIVE)
    {
        *p++ = OBX_HI_SESSION_SN;
        *p++ = p_cb->ssn;
    }
    if (p_cb->srm)
    {
        p_cb->srm   = OBX_SRM_ENABLE;
    }

    /* IrOBEX spec forbids connection ID in Connect Request */
    p_pkt = obx_cl_prepend_msg(p_cb, p_pkt, msg, (UINT16)(p - msg) );

    p_pkt->event    = OBX_CONNECT_REQ_EVT;
    obx_csm_event(p_cb, OBX_CONNECT_REQ_CEVT, p_pkt);
}

/*******************************************************************************
** Function     obx_ca_state
** Description  change state
*******************************************************************************/
tOBX_CL_STATE obx_ca_state(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    /* p_pkt should be NULL here */
    return p_cb->next_state;
}

/*******************************************************************************
** Function     obx_ca_start_timer
** Description  start timer
*******************************************************************************/
tOBX_CL_STATE obx_ca_start_timer(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    if (p_pkt)
        GKI_freebuf(p_pkt);
    obx_start_timer(&p_cb->ll_cb.comm);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_connect_ok
** Description  process the connect OK response from server
*******************************************************************************/
tOBX_CL_STATE obx_ca_connect_ok(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    UINT8   *p;
    tOBX_EVT_PARAM  param;              /* The event parameter. */

    if (p_cb->sess_st == OBX_SESS_ACTIVE)
    {
        /* reliable session was established - need to report the session event first */
        p = &p_cb->sess_info[OBX_SESSION_INFO_ID_IDX];
        UINT32_TO_BE_STREAM(p, p_cb->conn_id);
        param.sess.p_sess_info= p_cb->sess_info;
        param.sess.sess_st    = p_cb->sess_st;
        param.sess.nssn       = p_cb->ssn;
        param.sess.obj_offset = 0;
        p = &p_cb->sess_info[OBX_SESSION_INFO_MTU_IDX];
        UINT16_TO_BE_STREAM(p, p_cb->param.conn.mtu);
        param.sess.sess_op    = OBX_SESS_OP_CREATE;
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_SESSION_RSP_EVT, p_cb->rsp_code, param, NULL);
    }
    return obx_ca_notify(p_cb, p_pkt);
}

/*******************************************************************************
** Function     obx_ca_session_ok
** Description  process the session OK response from server
*******************************************************************************/
tOBX_CL_STATE obx_ca_session_ok(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_TRIPLET    triplet[OBX_MAX_SESS_PARAM_TRIP];
    UINT8           num = OBX_MAX_SESS_PARAM_TRIP;
    UINT8           *p_nonce = NULL, *p_addr = NULL, *p_sess_id = NULL;
    UINT8           *p, *p_cl_nonce;
    UINT8           ind, nonce_len = 0;
    UINT8           ind_sess_id;
    BD_ADDR         cl_addr;
    tOBX_STATUS     status = OBX_SUCCESS;
    UINT8           nssn;
#if (BT_USE_TRACES == TRUE)
    tOBX_SESS_ST    old_sess_st = p_cb->sess_st;
#endif
    tOBX_SESS_OP    sess_op = OBX_SESS_OP_SET_TIME;
    tOBX_CL_STATE   new_state = OBX_CS_NULL;
    tOBX_CL_EVENT   sm_evt = OBX_BAD_SM_EVT;
    UINT32          obj_offset = p_cb->param.sess.obj_offset;
    UINT32          timeout = p_cb->param.sess.timeout;
    tOBX_EVT_PARAM  param;              /* The event parameter. */
    UINT8           dropped = 0;

    OBX_TRACE_DEBUG4("obx_ca_session_ok sess_st: %d ssn:%d obj_offset:%d prev_state:%d", p_cb->sess_st, p_cb->ssn, obj_offset, p_cb->prev_state);
    OBX_ReadTriplet(p_pkt, OBX_HI_SESSION_PARAM, triplet, &num);
    obx_read_timeout (triplet, num, &timeout, &p_cb->sess_info[OBX_SESSION_INFO_TO_IDX]);
    p_cb->param.sess.timeout = timeout;
    if (p_cb->sess_st == OBX_SESS_SUSPEND)
    {
        p_cb->sess_st   = OBX_SESS_SUSPENDED;
        sess_op         = OBX_SESS_OP_SUSPEND;
        p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX] = p_cb->srm;
        nssn            = p_cb->ssn;
        /* send a tx_empty event to close port */
        sm_evt  = OBX_TX_EMPTY_CEVT;
        OBX_TRACE_DEBUG2("suspend saved st:%d, srm:0x%x", p_cb->sess_info[OBX_SESSION_INFO_ST_IDX], p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX]);
    }
    else if (p_cb->sess_st == OBX_SESS_CLOSE)
    {
        sess_op         = OBX_SESS_OP_CLOSE;
        p_cb->sess_st   = OBX_SESS_NONE;
        /* send a tx_empty event to close port */
        sm_evt  = OBX_TX_EMPTY_CEVT;
    }
    else if (num)
    {
        ind_sess_id = obx_read_triplet(triplet, num, OBX_TAG_SESS_PARAM_SESS_ID);
        if ((ind_sess_id != num) && (triplet[ind_sess_id].len == OBX_SESSION_ID_SIZE))
        {
            p_sess_id = triplet[ind_sess_id].p_array;
        }
        switch (p_cb->sess_st)
        {
        case OBX_SESS_CREATE:
            sess_op = OBX_SESS_OP_CREATE;
            status = OBX_BAD_PARAMS;
            ind = obx_read_triplet(triplet, num, OBX_TAG_SESS_PARAM_ADDR);
            if ((ind != num) && (triplet[ind].len == BD_ADDR_LEN))
            {
                p_addr = triplet[ind].p_array;
            }
            ind = obx_read_triplet(triplet, num, OBX_TAG_SESS_PARAM_NONCE);
            if ((ind != num) && (triplet[ind].len >= OBX_MIN_NONCE_SIZE) && (triplet[ind].len <= OBX_NONCE_SIZE))
            {
                p_nonce = triplet[ind].p_array;
                nonce_len = triplet[ind].len;
            }

            if (p_nonce && p_addr && p_sess_id)
            {
                OBX_TRACE_DEBUG0("verify session id");
                BTM_GetLocalDeviceAddr (cl_addr);
                p_cl_nonce = &p_cb->sess_info[OBX_SESSION_ID_SIZE];
                p = p_cl_nonce;
                UINT32_TO_BE_STREAM(p, p_cb->nonce);
                /* calculate client copy of session id */
                obx_session_id (p_cb->sess_info, cl_addr, p_cl_nonce, OBX_LOCAL_NONCE_SIZE, p_addr, p_nonce, nonce_len);
                obxu_dump_hex (p_cb->sess_info, "cl sess id", OBX_SESSION_ID_SIZE);
                obxu_dump_hex (p_sess_id, "sr sess id", OBX_SESSION_ID_SIZE);
                /* verify that the server copy is the same */
                if (memcmp (p_sess_id, p_cb->sess_info, OBX_SESSION_ID_SIZE) == 0)
                {
                    p_cb->sess_st   = OBX_SESS_ACTIVE;
                    p_cb->ssn       = 0;
                    /* do we want a timer here */
                    status = OBX_SUCCESS;
                    OBX_TRACE_DEBUG0("freeing received packet");
                    GKI_freebuf (p_pkt) ;
                    p_pkt  = p_cb->p_next_req;
                    p_cb->p_next_req = NULL;
                    obx_ca_connect_req (p_cb, p_pkt);
                    /*
                    p_cb->param.sess.p_sess_info= p_cb->sess_info;
                    p_cb->param.sess.sess_st    = p_cb->sess_st;
                    p_cb->param.sess.nssn       = p_cb->ssn;
                    p_cb->param.sess.sess_op    = sess_op;
                    (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_SESSION_RSP_EVT, p_cb->rsp_code, p_cb->param, NULL);
                    */
                    return new_state;
                }
            }
            break;

        case OBX_SESS_RESUME:
            status = OBX_BAD_PARAMS;
            dropped = p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] & OBX_CL_STATE_DROP;
            ind = obx_read_triplet(triplet, num, OBX_TAG_SESS_PARAM_NSEQNUM);
            if ((ind == num) || (triplet[ind].len != 1))
            {
                OBX_TRACE_ERROR0("RESUME:do not have valid NSSN tag");
                break;
            }

            nssn = *(triplet[ind].p_array);
            /* if SRM is enaged; make sure object offset TAG exists */
            if ((p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX] & OBX_SRM_ENGAGE) != 0)
            {
                obj_offset = obx_read_obj_offset(triplet, num);
                OBX_TRACE_DEBUG2("RESUME:SRM is engaged and object offset:0x%x (0x%x)",
                    obj_offset, p_cb->param.sess.obj_offset);
                
                /* client always takes the offset and ssn from the response since adjustment was done at server side */
                p_cb->param.sess.obj_offset = obj_offset;
                p_cb->ssn = nssn;
                status = OBX_SUCCESS;
            }
            /* otherwise make sure NSSN from server is OK */
            else if (nssn == p_cb->ssn)
            {
                OBX_TRACE_DEBUG0("RESUME:nssn matches expected ssn");
                status = OBX_SUCCESS;
            }
            else if (dropped != 0)
            {
                OBX_TRACE_DEBUG2("RESUME:link drop suspend nssn:%d cb ssn:%d", nssn, p_cb->ssn);
                if ((UINT8)(nssn+1) == p_cb->ssn)
                {
                    OBX_TRACE_DEBUG0("RESUME:nssn matches expected(ssn-1)");
                    p_cb->ssn -= 1;
                    status = OBX_SUCCESS;
                }
                else if (nssn == (UINT8)(p_cb->ssn+1))
                {
                    OBX_TRACE_DEBUG0("RESUME:nssn matches expected(ssn+1)");
                    nssn -= 1;
                    status = OBX_SUCCESS;
                }
            }
            else
            {
                OBX_TRACE_ERROR2("RESUME:bad NSSN:%d (%d)", nssn, p_cb->ssn);
                break;
            }
            p_cb->sess_st   = OBX_SESS_ACTIVE;
            sess_op         = OBX_SESS_OP_RESUME;
            OBX_TRACE_DEBUG2("RESUME:info new_state:0x%x, srm:0x%x", p_cb->sess_info[OBX_SESSION_INFO_ST_IDX], p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX]);
            p_cb->srm = p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX];
            p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] &= ~OBX_CL_STATE_DROP;
            if (p_cb->srm & OBX_SRM_ENGAGE)
            {
                new_state = p_cb->sess_info[OBX_SESSION_INFO_ST_IDX];
                if (new_state == OBX_CS_GET_SRM)
                {
                    p_cb->srm |= OBX_SRM_WAIT_UL;
                    /* Adjust snn in the control block since it is off by one with nssn in resume request */
                    p_cb->ssn--;
                }
            }
            else
            {
                new_state    = OBX_CS_CONNECTED;
                p_cb->srmp  |= OBX_SRMP_SESS_FST;
            }
            OBX_TRACE_DEBUG2("RESUME:new_state:%d, srm:0x%x", new_state, p_cb->srm);
            break;

        default:
            status = OBX_BAD_PARAMS;
        }
    }
    else
    {
        status = OBX_BAD_PARAMS;
    }
    OBX_TRACE_DEBUG5("obx_ca_session_ok prev:%d, sess_st:%d->%d obj_offset:%d status:%d", p_cb->prev_state, old_sess_st, p_cb->sess_st, obj_offset, status);

    if (sess_op == OBX_SESS_OP_SET_TIME)
        new_state = p_cb->prev_state;

    if (status != OBX_SUCCESS)
    {
        if (p_cb->sess_st == OBX_SESS_CLOSE)
            p_cb->sess_st = OBX_SESS_NONE;
        obx_csm_event(p_cb, OBX_TIMEOUT_CEVT, NULL);
        return OBX_CS_NULL;
    }
    p_cb->param.sess.p_sess_info= p_cb->sess_info;
    p_cb->param.sess.sess_st    = p_cb->sess_st;
    p_cb->param.sess.nssn       = nssn;
    p_cb->param.sess.ssn        = nssn;
    p_cb->param.sess.sess_op    = sess_op;
    p_cb->param.sess.obj_offset = obj_offset;
    p_cb->param.sess.timeout    = timeout;
    (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_SESSION_RSP_EVT, p_cb->rsp_code, p_cb->param, NULL);

    if ((sess_op == OBX_SESS_OP_RESUME) && (p_cb->sess_st == OBX_SESS_ACTIVE))
    {
        param.conn.ssn      = p_cb->ssn;
        memcpy (param.conn.peer_addr, p_cb->peer_addr, BD_ADDR_LEN);
        p = &p_cb->sess_info[OBX_SESSION_INFO_MTU_IDX];
        BE_STREAM_TO_UINT16(param.conn.mtu, p);
        p_cb->ll_cb.comm.tx_mtu = param.conn.mtu;
        param.conn.handle = p_cb->ll_cb.comm.handle;
        OBX_TRACE_DEBUG1("RESUME: tx_mtu: %d", p_cb->ll_cb.comm.tx_mtu);
        /* report OBX_CONNECT_RSP_EVT to let the client know the MTU */
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_CONNECT_RSP_EVT, OBX_RSP_OK, param, NULL);
        sm_evt = OBX_STATE_CEVT;
        p_cb->next_state = OBX_CS_CONNECTED;
    }

    if (sm_evt != OBX_BAD_SM_EVT)
    {
        /* send an event to csm */
        obx_csm_event(p_cb, sm_evt, NULL);
    }
    return new_state;
}

/*******************************************************************************
** Function     obx_ca_session_cont
** Description  process the continue response from server for SRM after
**              a suspend session request is sent
*******************************************************************************/
tOBX_CL_STATE obx_ca_session_cont(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    BOOLEAN free = TRUE;

    OBX_TRACE_DEBUG3("obx_ca_session_cont sess_st:%d prev_state:%d, srm:0x%x", p_cb->sess_st, p_cb->prev_state, p_cb->srm);
    if (p_cb->sess_st == OBX_SESS_SUSPEND)
    {
        if (p_cb->prev_state == OBX_CS_GET_SRM)
        {
            if ((p_cb->srm & OBX_SRM_WAIT_UL) == 0)
            {
                p_cb->srm |= OBX_SRM_WAIT_UL;
                p_cb->api_evt = OBX_GET_RSP_EVT;
            }
            else
            {
                GKI_enqueue_head  (&p_cb->ll_cb.comm.rx_q, p_pkt);
                OBX_TRACE_DEBUG1("obx_ca_session_cont rx_q.count:%d", p_cb->ll_cb.comm.rx_q.count);
            }
            free = FALSE;
        }
        else if (p_cb->prev_state == OBX_CS_GET_REQ_SENT)
        {
            p_cb->api_evt = OBX_GET_RSP_EVT;
            free = FALSE;
        }

    }
    if (free && p_pkt)
        GKI_freebuf(p_pkt);
    OBX_TRACE_DEBUG1("obx_ca_session_cont srm: 0x%x(e)", p_cb->srm );
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_session_get
** Description  process the get req api for SRM after
**              a suspend session request is sent (to clean out the received buffers)
*******************************************************************************/
tOBX_CL_STATE obx_ca_session_get(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    OBX_TRACE_DEBUG3("obx_ca_session_get sess_st:%d prev_state: %d, srm:0x%x", p_cb->sess_st, p_cb->prev_state, p_cb->srm );
    if (p_cb->sess_st == OBX_SESS_SUSPEND && p_cb->prev_state == OBX_CS_GET_SRM)
        return obx_ca_srm_get_req(p_cb, p_pkt);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_session_fail
** Description  process the session failed response from server
*******************************************************************************/
tOBX_CL_STATE obx_ca_session_fail(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SESS_ST    old_sess_st = p_cb->sess_st;

    p_cb->sess_st = OBX_SESS_NONE;
    OBX_TRACE_DEBUG2("obx_ca_session_fail, sess_st:%d->%d", old_sess_st, p_cb->sess_st);
    if (old_sess_st == OBX_SESS_CREATE && p_cb->rsp_code != OBX_RSP_OK)
    {
        /* peer device does not support session. Continue with regular session */
        /* report session failure */
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_SESSION_RSP_EVT, p_cb->rsp_code, p_cb->param, NULL);
        OBX_TRACE_DEBUG0("freeing received packet");
        if (p_pkt)
            GKI_freebuf (p_pkt) ;
        p_pkt  = p_cb->p_next_req;
        p_cb->p_next_req = NULL;
        obx_ca_connect_req (p_cb, p_pkt);
    }
    else
    {
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_SESSION_RSP_EVT, OBX_RSP_FAILED, p_cb->param, NULL);
        obx_ca_close_port (p_cb, p_pkt);
    }
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_abort
** Description  process the abort request in connected state
*******************************************************************************/
tOBX_CL_STATE obx_ca_abort(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    OBX_TRACE_DEBUG2("obx_ca_abort srm:0x%x srmp:0x%x", p_cb->srm, p_cb->srmp);
    if ( p_cb->srmp & OBX_SRMP_SESS_FST)
    {
        /* the first request after a session is resume. 
         * We may need to abort the previous request */
        if (p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] != OBX_CS_CONNECTED)
        {
            /* set the state here, just in case the result of obx_ca_snd_req is partial_sent */
            p_cb->state = OBX_CS_ABORT_REQ_SENT;
            obx_ca_snd_req(p_cb, p_pkt);
            return OBX_CS_NULL;
        }
        p_cb->srmp &= ~OBX_SRMP_SESS_FST;
    }
    obx_ca_notify (p_cb, p_pkt);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_snd_put_req
** Description  send put request in connected state
*******************************************************************************/
tOBX_CL_STATE obx_ca_snd_put_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE state = OBX_CS_NULL;

    OBX_TRACE_DEBUG1("obx_ca_snd_put_req, srm:0x%x", p_cb->srm);
    state = obx_ca_snd_req (p_cb, p_pkt);
    if (p_cb->srm & OBX_SRM_ENGAGE)
    {
        if (state == OBX_CS_PARTIAL_SENT)
        {
            p_cb->next_state   = OBX_CS_PUT_SRM;
        }
        else
        {
            p_cb->state = OBX_CS_PUT_SRM;
            if ((p_cb->srm & OBX_SRM_WAIT) == 0)
            {
                p_cb->rsp_code = OBX_RSP_CONTINUE;
                p_cb->param.put.final = FALSE;
                p_cb->param.put.type = OBX_PT_PUT;
                p_cb->param.put.ssn = 0;
                state = obx_ca_notify (p_cb, NULL);
            }
        }
    }
    return state;
}

/*******************************************************************************
** Function     obx_ca_snd_get_req
** Description  send get request in connected state
*******************************************************************************/
tOBX_CL_STATE obx_ca_snd_get_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE state;

    OBX_TRACE_DEBUG1("obx_ca_snd_get_req srm:0x%x", p_cb->srm );
    state = obx_ca_snd_req (p_cb, p_pkt);
    OBX_TRACE_DEBUG1("srm:0x%x", p_cb->srm );
    if (p_cb->srm & OBX_SRM_ENABLE)
    {
        if (state == OBX_CS_PARTIAL_SENT)
        {
            p_cb->next_state   = OBX_CS_GET_SRM;
        }
        else
            state = OBX_CS_GET_SRM;
        p_cb->srm &= ~OBX_SRM_WAIT_UL;
    }
    OBX_TRACE_DEBUG1("srm:0x%x", p_cb->srm );
    return state;
}

/*******************************************************************************
** Function     obx_ca_srm_snd_req
** Description  send Abort or Disconnect request
*******************************************************************************/
tOBX_CL_STATE obx_ca_srm_snd_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state;
    tOBX_COMM_CB    *p_comm = &p_cb->ll_cb.comm;

    OBX_TRACE_DEBUG2("obx_ca_srm_snd_req rx_q.count: %d, srm:0x%x", p_comm->rx_q.count, p_cb->srm );
    p_cb->srm &= ~OBX_SRM_WAIT_UL;
    state = obx_ca_snd_req (p_cb, p_pkt);
    if ((p_pkt = (BT_HDR *)GKI_dequeue (&p_comm->rx_q)) != NULL)
    {
        GKI_freebuf(p_pkt);
    }
    OBX_TRACE_DEBUG2("                   rx_q.count: %d, srm:0x%x", p_comm->rx_q.count, p_cb->srm );
    return state;
}

/*******************************************************************************
** Function     obx_ca_srm_put_req
** Description  send a PUT request when SRM is engaged
*******************************************************************************/
tOBX_CL_STATE obx_ca_srm_put_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state;

    OBX_TRACE_DEBUG1("obx_ca_srm_put_req srm:0x%x", p_cb->srm );
    state = obx_ca_snd_req (p_cb, p_pkt);
    OBX_TRACE_DEBUG4("obx_ca_srm_put_req state:%d srm:0x%x, final:%d rsp_code:0x%x", state, p_cb->srm, p_cb->final, p_cb->rsp_code );
    if (state != OBX_CS_PARTIAL_SENT && p_cb->final != TRUE && (p_cb->srm & OBX_SRM_WAIT) == 0)
    {
        p_cb->rsp_code = OBX_RSP_CONTINUE;
        state = obx_ca_notify (p_cb, NULL);
    }
    return state;
}

/*******************************************************************************
** Function     obx_ca_srm_get_req
** Description  send a GET request when SRM is engaged
*******************************************************************************/
tOBX_CL_STATE obx_ca_srm_get_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state = OBX_CS_NULL;
    tOBX_COMM_CB    *p_comm = &p_cb->ll_cb.comm;

    OBX_TRACE_DEBUG3("obx_ca_srm_get_req rx_q.count: %d, srm:0x%x", p_cb->sess_st, p_comm->rx_q.count, p_cb->srm );

    obx_start_timer(&p_cb->ll_cb.comm);
    p_cb->srm &= ~OBX_SRM_WAIT_UL;
    if (p_cb->sess_st == OBX_SESS_ACTIVE)
    {
        p_cb->ssn++;
    }
    if (p_pkt)
        GKI_freebuf(p_pkt);

    if ((p_pkt = (BT_HDR *)GKI_dequeue (&p_comm->rx_q)) != NULL)
    {
        if (state != OBX_CS_NULL)
        {
            p_cb->prev_state = p_cb->state;
            p_cb->state = state;
            state = OBX_CS_NULL;
        }
        obx_cl_proc_pkt (p_cb, p_pkt);
        obx_flow_control(p_comm);
        OBX_TRACE_DEBUG1("obx_ca_srm_get_req rx_q.count: %d", p_cb->ll_cb.comm.rx_q.count );
    }
    OBX_TRACE_DEBUG1("obx_ca_srm_get_req srm:0x%x", p_cb->srm );

    return state;
}

/*******************************************************************************
** Function     obx_ca_srm_put_notify
** Description  Notify the OBX user OBX_PUT_RSP_EVT (OBX is ready for next req)
*******************************************************************************/
tOBX_CL_STATE obx_ca_srm_put_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state;

    OBX_TRACE_DEBUG2("obx_ca_srm_put_notify srm: 0x%x, srmp: 0x%x", p_cb->srm, p_cb->srmp );

    state = obx_ca_notify (p_cb, p_pkt);
    return state;
}

/*******************************************************************************
** Function     obx_ca_srm_get_notify
** Description  Notify the OBX user OBX_GET_RSP_EVT (OBX is ready for next req)
*******************************************************************************/
tOBX_CL_STATE obx_ca_srm_get_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state = OBX_CS_NULL;

    OBX_TRACE_DEBUG1("obx_ca_srm_get_notify srm: 0x%x", p_cb->srm );
    /* do not allow SRMP for now */
    p_cb->srm &= ~OBX_SRM_WAIT;

    if (p_cb->srm & OBX_SRM_ENGAGE)
    {
        if ((p_cb->srm & OBX_SRM_WAIT_UL) == 0)
        {
            p_cb->srm |= OBX_SRM_WAIT_UL;
            state = obx_ca_notify (p_cb, p_pkt);
        }
        else
        {
            GKI_enqueue_head  (&p_cb->ll_cb.comm.rx_q, p_pkt);
            OBX_TRACE_DEBUG1("obx_ca_srm_get_notify rx_q.count:%d", p_cb->ll_cb.comm.rx_q.count);
        }
    }
    else
    {
        state = obx_ca_notify (p_cb, p_pkt);
        if (state == OBX_CS_GET_SRM || state == OBX_CS_NULL)
            state = OBX_CS_GET_TRANSACTION;
    }
    OBX_TRACE_DEBUG2("obx_ca_srm_get_notify srm: 0x%x(e) state:%s", p_cb->srm, obx_sr_get_state_name(state) );
    return state;
}

/*******************************************************************************
** Function     obx_ca_save_rsp
** Description  save response in control block
*******************************************************************************/
tOBX_CL_STATE obx_ca_save_rsp(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    GKI_enqueue  (&p_cb->ll_cb.comm.rx_q, p_pkt);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_save_req
** Description  save request in control block
*******************************************************************************/
tOBX_CL_STATE obx_ca_save_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    if (p_cb->p_next_req)
    {
        /* this probably would not happen */
        /* this action only occurs when we are flow controlled by the peer
         * and the client wants to abort the operation */
        /* Just in case that the user keeps calling abort request.... */
        OBX_TRACE_WARNING1("free next req: 0x%x", p_cb->p_next_req );
        GKI_freebuf(p_cb->p_next_req);
    }

    p_cb->p_next_req = p_pkt;

    return OBX_CS_NULL;
}


/*******************************************************************************
** Function     obx_ca_snd_req
** Description  If (p_pkt), call p_send_fn() to send the message to the peer.
**              Start timer. Return NULL state.If data is partially sent, return
**              PART state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_snd_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE state = OBX_CS_NULL;
    UINT8   rsp_code = OBX_RSP_DEFAULT;

    obx_access_rsp_code(p_pkt, &rsp_code);
    p_cb->final = (rsp_code&OBX_FINAL) ? TRUE : FALSE;
    OBX_TRACE_DEBUG2("obx_ca_snd_req rsp_code: 0x%x final:%d", rsp_code, p_cb->final );

    /* save a copy of the request sent to the server */
    /* In case that we are challenged by the server,
     * we can send the same request with authentication response again */
    if (p_cb->p_saved_req)
        GKI_freebuf(p_cb->p_saved_req);

    p_cb->p_saved_req   = obx_dup_pkt(p_pkt);

    OBX_TRACE_DEBUG3( "event p_saved_req:%d, pkt:%d, final: %d", p_cb->p_saved_req->event, p_pkt->event,p_cb->final);

    p_cb->ll_cb.comm.p_txmsg  = p_pkt;
    /* debug: obxu_dump_hex ((UINT8 *)(p_pkt + 1) + p_pkt->offset, "conn req", p_pkt->len); */

    p_cb->srmp &= ~OBX_SRMP_SESS_FST;
    if (p_cb->sess_st == OBX_SESS_ACTIVE)
    {
        p_cb->ssn++;
    }

    if (p_cb->ll_cb.comm.p_send_fn(&p_cb->ll_cb) == FALSE)
    {
        p_cb->next_state   = p_cb->state;
        state = OBX_CS_PARTIAL_SENT;
    }

    return state;
}

/*******************************************************************************
** Function     obx_ca_close_port
** Description  Close the transport. Return NULL state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_close_port(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    if (p_pkt)
        GKI_freebuf(p_pkt);
    p_cb->ll_cb.comm.p_close_fn(p_cb->ll_cb.comm.id);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_snd_part
** Description  Call p_send_fn() to send the left-over OBEX message to the peer.
**              Start timer. If all the data is sent, call obx_csm_event() with
**              STATE event to next_state in the port control block. 
**              If (p_next_req), call obx_csm_event() to process the saved request.
**              Return NULL state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_snd_part(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_CL_STATE state = OBX_CS_NULL;

    OBX_TRACE_DEBUG1("obx_ca_snd_part sess_st:%d", p_cb->sess_st);

    /* p_pkt should be NULL here */
    if (p_cb->ll_cb.comm.p_send_fn(&p_cb->ll_cb) == TRUE)
    {
        /* data is all sent. change state to the appropriate state */
        obx_csm_event(p_cb, OBX_STATE_CEVT, NULL);
        if (p_cb->p_next_req && (p_cb->sess_st != OBX_SESS_CREATE))
        {
            /* abort request was issued - send it now */
            p_pkt = p_cb->p_next_req;
            p_cb->p_next_req = NULL;
            obx_csm_event(p_cb, (tOBX_CL_EVENT)(p_pkt->event-1), p_pkt);
        }

        OBX_TRACE_DEBUG2("obx_ca_snd_part state:%d, srm:0x%x", p_cb->state, p_cb->srm);
        if ((p_pkt = (BT_HDR *)GKI_dequeue (&p_cb->ll_cb.comm.rx_q)) != NULL)
        {
            obx_cl_proc_pkt (p_cb, p_pkt);
        }
        else if (p_cb->state == OBX_CS_PUT_SRM)
        {
            if (((p_cb->srm & OBX_SRM_WAIT) == 0) && (p_cb->final != TRUE))
            {
                state = obx_ca_notify (p_cb, NULL);
            }
        }
    }
    return state;
}

/*******************************************************************************
** Function     obx_ca_connect_error
** Description  Call callback function with OBX_CLOSE_IND_EVT. Free the client
**              control block. Return NULL state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_connect_error(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_CL_CBACK   *p_cback = p_cb->p_cback;
    tOBX_HANDLE     handle   = p_cb->ll_cb.comm.handle;
    tOBX_EVT_PARAM  param    = p_cb->param;
    tOBX_SR_STATE   save_state;

    if (p_cb->sess_st == OBX_SESS_ACTIVE)
    {
        /* The transport is interrupted while a reliable session is active:
         * report a suspend event fot application to save the information in NV.
         * The only time this is called is for port close evt /w next state as not_connected
         * we need to use prev_state as the potential state to resume session
         */
        save_state = p_cb->prev_state;
        if (save_state == OBX_CS_PARTIAL_SENT)
            save_state = p_cb->next_state;
        /* marks link drop suspend only when SRM is not engaged */
        if ((p_cb->srm & OBX_SRM_ENGAGE) == 0)
            save_state |= OBX_CL_STATE_DROP;
        else if (save_state == OBX_CS_GET_SRM)
            p_cb->srm &= ~OBX_SRM_WAIT_UL;
        p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] = save_state;
        OBX_TRACE_DEBUG2("obx_ca_connect_error saved state:0x%x, srm:0x%x", save_state, p_cb->srm);
        p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX] = p_cb->srm;
        param.sess.p_sess_info   = p_cb->sess_info;
        param.sess.sess_op       = OBX_SESS_OP_TRANSPORT;
        param.sess.sess_st       = p_cb->sess_st;
        param.sess.nssn          = p_cb->ssn;
        param.sess.ssn           = p_cb->ssn;
        param.sess.obj_offset    = 0;
        param.sess.timeout       = OBX_SESS_TIMEOUT_VALUE;
        memcpy(param.sess.peer_addr, p_cb->peer_addr, BD_ADDR_LEN);
        p_cb->sess_st = OBX_SESS_NONE;
        (*p_cback)(handle, OBX_SESSION_INFO_EVT, OBX_RSP_OK, param, NULL);
    }

    obx_cl_free_cb(p_cb);
    (*p_cback)(handle, OBX_CLOSE_IND_EVT, OBX_RSP_DEFAULT, param, p_pkt);
    return OBX_CS_NULL;
}

/*******************************************************************************
** Function     obx_ca_connect_fail
** Description  If the response code is OBX_RSP_UNAUTHORIZED, save the OBEX
**              message in the client control block and call callback function
**              with OBX_PASSWORD_EVT. Return NULL state.Otherwise, call
**              obx_csm_event() with OBX_DISCNT_REQ_CEVT. (We do not need to
**              send disconnect request to the server, since we are not 
**              connected yet). Return NULL state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_connect_fail(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state = OBX_CS_NULL;

    if ( p_cb->rsp_code == OBX_RSP_UNAUTHORIZED &&
        OBX_CheckHdr(p_pkt, OBX_HI_CHALLENGE) != NULL)
    {
        p_cb->api_evt   = OBX_PASSWORD_EVT;
        p_cb->p_auth    = obx_dup_pkt(p_pkt);
    }
    else
    {
        /* Connect Request is rejected for reasons other than authentication
         * or if the client challenges the server, but the server does nt return good digest
         * - notify the user of the failure and .. */
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, OBX_CONNECT_RSP_EVT, p_cb->rsp_code, p_cb->param, p_pkt);
        p_cb->api_evt   = OBX_NULL_EVT;
        /* and close the RFCOMM port */
        obx_csm_event(p_cb, OBX_DISCNT_REQ_CEVT, NULL);
    }
    return state;
}

/*******************************************************************************
** Function     obx_ca_discnt_req
** Description  OBX_DISCNT_REQ_CEVT event is received in OBX_CS_CONNECT_REQ_SENT
**              state. In case that the server fails the authentication, client
**              needs to send Disconnect Req. Otherwise, just close port.
*******************************************************************************/
tOBX_CL_STATE obx_ca_discnt_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_CL_STATE state = OBX_CS_NOT_CONNECTED;
    UINT8       msg[OBX_HDR_OFFSET];
    UINT8       *p = msg;

    if (p_cb->wait_auth == OBX_WAIT_AUTH_FAIL)
    {
        /* server thinks the connection is up.
         * client needs to send disconnect req */
        state = OBX_CS_DISCNT_REQ_SENT;
        /* Disconnect request always has the final bit set */
        *p++ = (OBX_REQ_DISCONNECT|OBX_FINAL);
        p += OBX_PKT_LEN_SIZE;

        /* add session sequence number, if session is active */
        if (p_cb->sess_st == OBX_SESS_ACTIVE)
        {
            *p++ = OBX_HI_SESSION_SN;
            *p++ = p_cb->ssn;
        }

        /* add connection ID, if needed */
        if (p_cb->conn_id != OBX_INVALID_CONN_ID)
        {
            *p++    = OBX_HI_CONN_ID;
            UINT32_TO_BE_STREAM(p, p_cb->conn_id);
        }

        p_pkt = obx_cl_prepend_msg(p_cb, NULL, msg, (UINT16)(p - msg) );
        obx_ca_snd_req(p_cb, p_pkt);
    }
    else
    {
        /* connection is not officially up yet.
         * just close the port */
        obx_ca_close_port(p_cb, p_pkt);
    }
    p_cb->wait_auth = FALSE;

    return state;
}

/*******************************************************************************
** Function     obx_ca_notify
** Description  Use api_evt or look up the event according to the state. Fill
**              the event parameter. Call callback function with the event. 
**              Return NULL state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state = OBX_CS_CONNECTED;
    tOBX_EVENT      event = obx_cl_state_2_event_map[p_cb->state - 1];
    BOOLEAN         notify = FALSE;
    tOBX_CL_EVENT   sm_evt = OBX_BAD_SM_EVT;
    BT_HDR          *p_req = NULL;

    OBX_TRACE_DEBUG6( "obx_ca_notify state: %s, prev_state: %s, rsp:0x%x, sess_st:%d, event:%d srm:0x%x",
        obx_cl_get_state_name( p_cb->state ), obx_cl_get_state_name(p_cb->prev_state), p_cb->rsp_code, p_cb->sess_st, event, p_cb->srm);
    OBX_TRACE_DEBUG2( "ssn:0x%x/0x%x", p_cb->ssn, p_cb->param.ssn);

    if ( (p_cb->final == TRUE && p_cb->rsp_code == OBX_RSP_CONTINUE && p_cb->state == OBX_CS_PUT_TRANSACTION) ||
        (p_cb->final == FALSE && p_cb->rsp_code != OBX_RSP_CONTINUE) )
    {
        /* final bit on the request mismatch the responde code --- Error!! */
        OBX_TRACE_ERROR2( "final:%d on the request mismatch the responde code:0x%x",
            p_cb->final, p_cb->rsp_code) ;
        /* change the state to not connected state */
        p_cb->next_state   = OBX_CS_NOT_CONNECTED;
        obx_csm_event(p_cb, OBX_STATE_CEVT, NULL);
        notify  = TRUE;
        /* send a tx_empty event to close port */
        sm_evt  = OBX_TX_EMPTY_CEVT;
    }

    else if (event != OBX_NULL_EVT)
    {
        switch (p_cb->state)
        {
        case OBX_CS_PUT_TRANSACTION:
        case OBX_CS_GET_TRANSACTION:
        case OBX_CS_PUT_REQ_SENT:
        case OBX_CS_GET_REQ_SENT:
        case OBX_CS_PUT_SRM:
        case OBX_CS_GET_SRM:
            if (p_cb->rsp_code == OBX_RSP_CONTINUE )
            {
                /* notify the event in this function. the new state stays the same */
                notify = TRUE;

                if (p_cb->srm & OBX_SRM_ENGAGE)
                {
                    if (p_cb->state == OBX_CS_PUT_TRANSACTION)
                    {
                        p_cb->state = OBX_CS_PUT_SRM;
                    }
                    else if (p_cb->state == OBX_CS_GET_TRANSACTION)
                    {
                        p_cb->state = OBX_CS_GET_SRM;
                    }
                    else if (p_cb->state == OBX_CS_PUT_SRM && p_pkt && (p_cb->srm & OBX_SRM_WAIT) == 0)
                    {
                        OBX_TRACE_ERROR0 ("unexpected PUT response. disconnect now!!");
                        notify = FALSE;
                        event   = OBX_NULL_EVT;
                        obx_ca_close_port(p_cb, p_pkt);
                    }
                    /* clear the wait bit here to avoid the link being disconnected by accident */
                    p_cb->srm &= ~OBX_SRM_WAIT;
                    if (p_cb->srmp)
                    {
                        p_cb->srmp = 0;
                        p_cb->srm |= OBX_SRM_WAIT;
                    }
                }
            }
            /* else let obx_csm_event notify the event. the new state is OBX_CS_CONNECTED */
            else
            {
                /* dis-engage SRM */
                p_cb->srm &= OBX_SRM_ENABLE;
                OBX_TRACE_DEBUG1( "disengage srm:0x%x", p_cb->srm);
            }
            break;

        case OBX_CS_NOT_CONNECTED:
            if (p_cb->sess_st == OBX_SESS_ACTIVE && p_cb->prev_state == OBX_CS_DISCNT_REQ_SENT)
            {
                p_req   = obx_ca_close_sess_req (p_cb);
                sm_evt  = OBX_SESSION_REQ_CEVT;
                state   = OBX_CS_NULL;
            }
            else
            {
                notify  = TRUE;
                /* send a tx_empty event to close port */
                sm_evt  = OBX_TX_EMPTY_CEVT;
            }
            break;

        case OBX_CS_CONNECT_REQ_SENT:
            if (p_cb->rsp_code == OBX_RSP_FAILED)
            {
                /* client challenged the server and the server does not return a good digest */
                notify  = TRUE;
                /* send a disconnect req event to close port */
                sm_evt  = OBX_DISCNT_REQ_CEVT;
            }
            break;

        case OBX_CS_ABORT_REQ_SENT:
            p_cb->srm &= OBX_SRM_ENABLE;
            OBX_TRACE_DEBUG1( "(ab) disengage srm:0x%x", p_cb->srm);
            break;
        }
    }

    if (notify == TRUE )
    {
        (*p_cb->p_cback)(p_cb->ll_cb.comm.handle, event, p_cb->rsp_code, p_cb->param, p_pkt);
        event   = OBX_NULL_EVT;
    }

    if (sm_evt != OBX_BAD_SM_EVT)
    {
        /* send an event to csm */
        obx_csm_event(p_cb, sm_evt, p_req);
    }

    p_cb->api_evt   = event;
    if (event == OBX_NULL_EVT)
    {
        state = OBX_CS_NULL;
    }
    else
    {
        p_cb->ssn = p_cb->param.ssn;
        OBX_TRACE_DEBUG1( "ssn:0x%x", p_cb->ssn);
    }

    return state;
}

/*******************************************************************************
** Function     obx_ca_fail_rsp
** Description  Save the OBEX message in control block.If the response code is
**              OBX_RSP_UNAUTHORIZED, set api_evt to OBX_PASSWORD_EVT. Return
**              OP_UNAUTH state.Otherwise, set api_evt according to the
**               event/state table. Return CONN state.
*******************************************************************************/
tOBX_CL_STATE obx_ca_fail_rsp(tOBX_CL_CB *p_cb, BT_HDR *p_pkt)
{
    tOBX_SR_STATE   state = OBX_CS_CONNECTED;

    p_cb->srm &= OBX_SRM_ENABLE;

    if ( p_cb->rsp_code == OBX_RSP_UNAUTHORIZED &&
        OBX_CheckHdr(p_pkt, OBX_HI_CHALLENGE) != NULL)
    {
        state = OBX_CS_OP_UNAUTH;
        p_cb->api_evt   = OBX_PASSWORD_EVT;
        p_cb->p_auth =  obx_dup_pkt(p_pkt);
    }
    else
        p_cb->api_evt   = obx_cl_state_2_event_map[p_cb->state - 1];

    return state;
}
