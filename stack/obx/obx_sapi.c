/*****************************************************************************
**
**  Name:         obx_sapi.c
**
**  File:         OBEX Server Application Programming Interface functions
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bt_target.h"

#if defined(OBX_INCLUDED) && (OBX_INCLUDED == TRUE)

#include "wcassert.h"
#include "btu.h"
#include "port_api.h"
#include "obx_int.h"
#include "l2c_api.h"
#include "btm_api.h"

/*******************************************************************************
**
** Function     OBX_StartServer
**
** Description  This function is to register a server entity to OBEX.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not have resources.
**
*******************************************************************************/
tOBX_STATUS OBX_StartServer(tOBX_StartParams *p_params, tOBX_HANDLE *p_handle)
{
    tOBX_SR_CB *p_cb = NULL;
    tOBX_STATUS status = OBX_NO_RESOURCES;
// btla-specific ++
    tOBX_SR_SESS_CB *p_scb = NULL;
// btla-specific --
    tOBX_HANDLE obx_handle;
    UINT8       size;

    WC_ASSERT(p_params);
    WC_ASSERT(p_params->p_cback);
    WC_ASSERT(p_handle);

    if (p_params->max_sessions > OBX_MAX_SR_SESSION)
    {
        OBX_TRACE_ERROR2("OBX_StartServer bad max_sessions:%d (1-%d)",
            p_params->max_sessions, OBX_MAX_SR_SESSION);
        return OBX_BAD_PARAMS;
    }

    if (p_params->scn == 0 && L2C_INVALID_PSM(p_params->psm))
    {
        OBX_TRACE_ERROR2("OBX_StartServer bad scn:%d and psm:0x%x", p_params->scn, p_params->psm);
        return OBX_BAD_PARAMS;
    }

    if (p_params->max_sessions == 0)
        p_params->max_sessions = 1;

    /* allocate a server control block */
    obx_handle = obx_sr_alloc_cb(p_params);
    if (obx_handle)
        p_cb = &obx_cb.server[obx_handle-1];

    if (p_cb != NULL)
    {
        p_scb = &obx_cb.sr_sess[p_cb->sess[0]-1];
        p_scb->ll_cb.port.rx_mtu   = p_params->mtu;
        if (p_cb->scn)
        {
            /* open an RFCOMM port to listen for incoming messages */
            /* allocate the port for the first session now. The others will be allocated when needed */
            status = obx_open_port(&p_scb->ll_cb.port, BT_BD_ANY, p_cb->scn);
        }
        else
        {
            status      = OBX_SUCCESS;
        }

        if (status == OBX_SUCCESS)
        {
            /* If authentication is needed for this server, save the parameters in control block */
            if (p_params->authenticate)
            {
                p_cb->p_auth = (tOBX_AUTH_PARAMS *)GKI_getbuf(sizeof(tOBX_AUTH_PARAMS)+OBX_MAX_REALM_LEN+1);
                if (p_cb->p_auth)
                {
                    p_cb->p_auth->auth_option   = p_params->auth_option;
                    /* adjust realm len, if the given realm is too big */
                    size    = (p_params->realm_len>OBX_MAX_REALM_LEN) ? OBX_MAX_REALM_LEN : p_params->realm_len;
                    p_cb->p_auth->realm_len     = size;
                    p_cb->p_auth->realm[0]      = p_params->realm_charset;
                    if (p_params->realm_len && p_params->p_realm)
                        memcpy(&p_cb->p_auth->realm[1], p_params->p_realm, size);
                }
                else
                    status = OBX_NO_RESOURCES;
            }
        }

        if (status == OBX_SUCCESS)
        {
            /* if everything is OK, save the other parameters for this server */
            memset(p_cb->target.target, 0, OBX_MAX_TARGET_LEN);
            if (p_params->p_target)
            {
                if (p_params->p_target->len)
                {
                    /* OBX handles who, connection ID headers */
                    p_cb->target.len = p_params->p_target->len;
                    memcpy(p_cb->target.target, p_params->p_target->target, p_params->p_target->len);
                }
                else
                {
                    /* the regular default server */
                    /* the user handles target, who headers.
                     * OBX handles connection ID header */
                    p_cb->target.len = OBX_DEFAULT_TARGET_LEN;
                }
            }
            else
            {
                /* the one and only default server */
                /* no target, who, connection id headers for this case */
                p_cb->target.len = 0;
            }

// btla-specific ++
            if (p_scb) 
			{
                OBX_TRACE_DEBUG3("OBX_StartServer target len:%d, authenticate:%d handle:0x%x",
                    p_cb->target.len, p_params->authenticate, p_scb->ll_cb.port.handle);
                p_cb->p_cback       = p_params->p_cback;
                p_scb->state         = OBX_SS_NOT_CONNECTED;

                /* give the handle to application */
                *p_handle = p_scb->ll_cb.port.handle;
            }
// btla-specific --
        }
        else
        {
            /* otherwise, free the control block */
            obx_sr_free_cb(obx_handle);
        }
    }

    return status;
}


/*******************************************************************************
**
** Function     OBX_StopServer
**
** Description  This function is to stop this OBEX server from receiving any
**              more incoming requests.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_StopServer(tOBX_HANDLE handle)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_SR_CB *p_cb = obx_sr_get_cb(handle);
    tOBX_SR_SESS_CB *p_scb;
    int xx;
    tOBX_SPND_CB    *p_spndcb;

    if (p_cb)
    {
        /* Process suspended session if necessary */
        if (p_cb->p_suspend)
        {
            for (xx=0, p_spndcb=p_cb->p_suspend; xx<p_cb->max_suspend; xx++, p_spndcb++)
            {
                if (p_spndcb->state)
                {
                    btu_stop_timer (&p_spndcb->stle);
                }
            }
            GKI_freebuf (p_cb->p_suspend);
        }

        for (xx=0; xx < p_cb->num_sess && p_cb->sess[xx]; xx ++)
        {
            p_scb = &obx_cb.sr_sess[p_cb->sess[xx]-1];
            if (p_scb->ll_cb.comm.id)
            {
                OBX_DisconnectRsp(handle, OBX_RSP_SERVICE_UNAVL, NULL);
                if (p_scb->ll_cb.comm.p_send_fn == (tOBX_SEND_FN *)obx_rfc_snd_msg)
                    RFCOMM_RemoveServer(p_scb->ll_cb.port.port_handle);
            }
        }

        if (p_cb->psm)
            L2CA_DEREGISTER (p_cb->psm);

        obx_sr_free_cb (handle);
    }
    else
        status = OBX_BAD_HANDLE;
    return status;
}

/*******************************************************************************
**
** Function     OBX_AddSuspendedSession
**
** Description  This function is to add the session information for a previously
**				suspended reliable session to the server control block
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_AddSuspendedSession(tOBX_HANDLE shandle, BD_ADDR peer_addr, UINT8 *p_sess_info,
                                    UINT32 timeout, UINT8 ssn, UINT32 offset)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_SR_CB  *p_cb = obx_sr_get_cb(shandle);
    UINT16      size;
    UINT8       xx;
    tOBX_SPND_CB    *p_spndcb;
    INT32       ticks = 0x7FFFFFFF, remain_ticks;
    BOOLEAN     added = FALSE;
    UINT8       saved_xx = 0;

    OBX_TRACE_DEBUG2("OBX_AddSuspendedSession BDA: %06x%06x",
                    (peer_addr[0]<<16)+(peer_addr[1]<<8)+peer_addr[2],
                    (peer_addr[3]<<16)+(peer_addr[4]<<8)+peer_addr[5]);
    if (p_cb && p_sess_info && p_cb->max_suspend)
    {
        if (p_cb->p_suspend == NULL)
        {
            size = p_cb->max_suspend * sizeof (tOBX_SPND_CB);
            p_cb->p_suspend = (tOBX_SPND_CB *)GKI_getbuf( size);
            memset (p_cb->p_suspend, 0, size);
        }

        if (p_cb->p_suspend)
        {
            for (xx=0, p_spndcb=p_cb->p_suspend; xx<p_cb->max_suspend; xx++, p_spndcb++)
            {
                OBX_TRACE_DEBUG4("[%d] state: %d, ssn:%d BDA: %08x", xx, p_spndcb->state, p_spndcb->ssn,
                   (p_spndcb->peer_addr[2]<<24)+(p_spndcb->peer_addr[3]<<16)+(p_spndcb->peer_addr[4]<<8)+p_spndcb->peer_addr[5]);
                if (p_spndcb->state == OBX_SS_NULL || memcmp(p_spndcb->peer_addr, peer_addr, BD_ADDR_LEN) == 0)
                {
                    added = TRUE;
                    break;
                }
                else if (p_spndcb->state != OBX_SS_NULL && ticks)
                {
                    if (p_spndcb->stle.param == 0)
                    {
                        /* this entry has infinite timeout; just use it */
                        ticks = 0;
                        saved_xx = xx;
                        OBX_TRACE_DEBUG1("[%d] infinite timeout", xx );
                    }
                    /* find the entry the expires in the shortest time  */
                    else
                    {
                        remain_ticks = btu_remaining_time(&p_spndcb->stle);
                        OBX_TRACE_DEBUG2("[%d] remain_ticks: %d", xx, remain_ticks );
                        if (remain_ticks < ticks)
                        {
                            ticks = remain_ticks;
                            saved_xx = xx;
                        }
                    }
                }
            }

            if (!added)
            {
                /* if cannot use an empty/or reuse an existing entry, use the one expires soon */
                added = TRUE;
                xx = saved_xx; /* this is for debug trace; don't optimize */
                p_spndcb = &p_cb->p_suspend[xx];
                OBX_TRACE_DEBUG1("reuse entry [%d]", xx );
            }

            if (added)
            {
		        memcpy (p_spndcb->sess_info, p_sess_info, OBX_SESSION_INFO_SIZE);
                p_spndcb->state = p_sess_info[OBX_SESSION_INFO_ST_IDX];
                p_spndcb->ssn = ssn;
                p_spndcb->offset = offset;
                OBX_TRACE_DEBUG6("[%d] timeout: %d state:%d ssn:%d offset:%d, BDA: %08x",
                    xx, timeout, p_spndcb->state, ssn, offset,
                   (peer_addr[2]<<24)+(peer_addr[3]<<16)+(peer_addr[4]<<8)+peer_addr[5]);
                memcpy(p_spndcb->peer_addr, peer_addr, BD_ADDR_LEN);
                if (timeout != OBX_INFINITE_TIMEOUT)
                {
                    p_spndcb->stle.param = (UINT32)p_spndcb;
                    btu_start_timer(&p_spndcb->stle, BTU_TTYPE_OBX_SVR_SESS_TO, timeout);
                    OBX_TRACE_DEBUG2("timeout: %d ticks:%d", timeout, p_spndcb->stle.ticks);
                }
                else
                    p_spndcb->stle.param = 0;
            }
        }
    }
    else
        status = OBX_BAD_HANDLE;
    return status;
}

/*******************************************************************************
**
** Function     OBX_ConnectRsp
**
** Description  This function is called to send the response to a Connect 
**              Request from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_ConnectRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_SR_SESS_CB *p_scb = obx_sr_get_scb(shandle);
    tOBX_SR_CB      *p_cb = obx_sr_get_cb(shandle);

    if (p_scb)
    {
        p_pkt = obx_conn_rsp(p_cb, p_scb, rsp_code, p_pkt);

        obx_ssm_event(p_scb, OBX_CONNECT_CFM_SEVT, p_pkt);
    }
    else
    {
        OBX_TRACE_DEBUG1("OBX_ConnectRsp Bad Handle: 0x%x", shandle);
        status = OBX_BAD_HANDLE;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_SessionRsp
**
** Description  This function is called to respond to a request to create a reliable session.
**              
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_SessionRsp(tOBX_HANDLE shandle, UINT8 rsp_code, UINT8 ssn, UINT32 offset, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_SR_SESS_CB *p_scb = obx_sr_get_scb(shandle);
    UINT8       *p;
    BT_HDR      *p_rsp = NULL;
    tOBX_TRIPLET triplet[5];
    UINT8       data[12];
    UINT8       num_trip = 0;
    UINT32      timeout;
    tOBX_SESS_ST    old_sess_st;

    OBX_TRACE_API0("OBX_SessionRsp");
    if (p_scb)
    {
        old_sess_st = p_scb->sess_st;
        p_rsp   = OBX_HdrInit(p_scb->handle, OBX_MIN_MTU);
        if (p_rsp)
        {
            p = (UINT8 *) (p_rsp + 1) + p_rsp->offset;
            /* response packet always has the final bit set */
            *p++ = (rsp_code | OBX_FINAL);
            p_rsp->len = 3;
            p = data;
            if (rsp_code == OBX_RSP_OK)
            {
                switch (p_scb->sess_st)
                {
                case OBX_SESS_CREATE:
                case OBX_SESS_RESUME:
                    GKI_freebuf (p_rsp);
                    if (p_pkt)
                        GKI_freebuf (p_pkt);
                    OBX_TRACE_DEBUG0("OBX_SessionRsp do not need to be called for CREATE and RESUME");
                    return OBX_SUCCESS;
                case OBX_SESS_SUSPEND:
                    p_scb->sess_st = OBX_SESS_SUSPENDED;
                    p = &p_scb->sess_info[OBX_SESSION_INFO_TO_IDX];
                    BE_STREAM_TO_UINT32(timeout, p);
                    OBX_AddSuspendedSession(p_scb->handle, p_scb->peer_addr, p_scb->sess_info, timeout, ssn, offset);
                    break;
                case OBX_SESS_CLOSE:
                    p_scb->sess_st = OBX_SESS_NONE;
                    break;
                }

                if (num_trip)
                    OBX_AddTriplet(p_rsp, OBX_HI_SESSION_PARAM, triplet, num_trip);
                if (p_pkt)
                {
                    p = (UINT8 *) (p_rsp + 1) + p_rsp->offset + p_rsp->len;
                    memcpy (p, ((UINT8 *) (p_pkt + 1) + p_pkt->offset), p_pkt->len);
                    p_rsp->len += p_pkt->len;
                }
            }
            p = (UINT8 *) (p_rsp + 1) + p_rsp->offset + 1;
            UINT16_TO_BE_STREAM(p, p_rsp->len);

            p_rsp->event = OBX_SESSION_CFM_SEVT + 1;
        }
        OBX_TRACE_DEBUG3("Rsp sess_st:%d->%d status:%d", old_sess_st, p_scb->sess_st, status);

        obx_ssm_event(p_scb, OBX_SESSION_CFM_SEVT, p_rsp);
        /* clear the "previous" session state as required by earlier comment */
        p_scb->param.sess.sess_st = 0;
    }
    else
        status = OBX_BAD_HANDLE;

    if (p_pkt)
    {
        GKI_freebuf (p_pkt);
    }
    return status;
}

/*******************************************************************************
**
** Function     obx_prepend_rsp_msg
**
** Description  This function is called to add response code and connection ID
**              to the given OBEX message 
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS obx_prepend_rsp_msg(tOBX_HANDLE shandle, tOBX_SR_EVENT event, UINT8 rsp_code, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_SR_SESS_CB *p_scb = obx_sr_get_scb(shandle);
    UINT8       msg[OBX_HDR_OFFSET];
    UINT8       *p = msg;

    if (p_scb)
    {
        /* response packets always have the final bit set */
        *p++ = (rsp_code | OBX_FINAL);
        p += OBX_PKT_LEN_SIZE;

        /* add session sequence number, if session is active */
        if (p_scb->sess_st == OBX_SESS_ACTIVE || p_scb->sess_st == OBX_SESS_SUSPENDING)
        {
            *p++ = OBX_HI_SESSION_SN;
            *p++ = (p_scb->ssn+1);
        }

        if (event == OBX_DISCNT_CFM_SEVT)
            p_scb->conn_id = 0;

        if (p_scb->srm & OBX_SRM_REQING)
        {
            p_scb->srm &= ~OBX_SRM_REQING;
            if (rsp_code == OBX_RSP_CONTINUE)
            {
                p_scb->srm |= OBX_SRM_ENGAGE;
                *p++ = OBX_HI_SRM;
                *p++ = OBX_HV_SRM_ENABLE;

                if (event == OBX_PUT_CFM_SEVT)
                    p_scb->srm |= OBX_SRM_NEXT;
            }
        }

        p_pkt = obx_sr_prepend_msg(p_pkt, msg, (UINT16)(p - msg) );
        /* this event code needs to be set up properly for flow control reasons */
        p_pkt->event    = event+1;
        obx_ssm_event(p_scb, event, p_pkt);
    }
    else
        status = OBX_BAD_HANDLE;

    return status;
}

/*******************************************************************************
**
** Function     OBX_SetPathRsp
**
** Description  This function is called to send the response to a Set Path 
**              Request from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_SetPathRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_SETPATH_CFM_SEVT, rsp_code, p_pkt);
}


/*******************************************************************************
**
** Function     OBX_PutRsp
**
** Description  This function is called to send the response to a Put
**              Request from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_PutRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_PUT_CFM_SEVT, rsp_code, p_pkt);
}


/*******************************************************************************
**
** Function     OBX_GetRsp
**
** Description  This function is called to send the response to a Get
**              Request from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_GetRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_GET_CFM_SEVT, rsp_code, p_pkt);
}


/*******************************************************************************
**
** Function     OBX_AbortRsp
**
** Description  This function is called to send the response to an Abort
**              Request from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_AbortRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_ABORT_CFM_SEVT, rsp_code, p_pkt);
}

/*******************************************************************************
**
** Function     OBX_ActionRsp
**
** Description  This function is called to respond to an Action command Request
**              from an OBEX client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_ActionRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_ACTION_CFM_SEVT, rsp_code, p_pkt);
}

/*******************************************************************************
**
** Function     OBX_DisconnectRsp
**
** Description  This function is called to send the response to a Disconnect
**              Request from an OBEX client.
**              This function can also be used to force close the transport
**              to a connected client.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_DisconnectRsp(tOBX_HANDLE shandle, UINT8 rsp_code, BT_HDR *p_pkt)
{
    return obx_prepend_rsp_msg(shandle, OBX_DISCNT_CFM_SEVT, rsp_code, p_pkt);
}
#endif
