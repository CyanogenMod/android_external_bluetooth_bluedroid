/*****************************************************************************
**
**  Name:         obx_capi.c
**
**  File:         OBEX Client Application Programming Interface functions
**
**  Copyright (c) 2003-2010, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "wcassert.h"
#include "obx_int.h"
#include "port_api.h"
#include "l2c_api.h"
#include "btm_api.h"

/*******************************************************************************
**
** Function     OBX_ConnectReq
**
** Description  This function registers a client entity to OBEX and sends a
**              CONNECT request to the server specified by the API parameters.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_ConnectReq(BD_ADDR bd_addr, UINT8 scn, UINT16 mtu,
                           tOBX_CL_CBACK *p_cback, tOBX_HANDLE *p_handle, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_NO_RESOURCES;
    UINT8       msg[OBX_HDR_OFFSET + OBX_MAX_CONN_HDR_EXTRA];
    UINT8       *p = msg;
    tOBX_CL_CB  *p_cb;

    WC_ASSERT(p_handle);

    p_cb = obx_cl_get_cb(*p_handle);
    if (p_cb == NULL)
        p_cb = obx_cl_alloc_cb();

    if (p_cb)
    {
        if (p_cb->ll_cb.port.port_handle == 0)
        {
            WC_ASSERT(p_cback);
            /* port is not open yet- open one
             * this is the first CONNECT request */
            p_cb->ll_cb.comm.rx_mtu = mtu;
            p_cb->p_cback   = p_cback;
            p_cb->state     = OBX_CS_NOT_CONNECTED;
            status          = obx_open_port(&p_cb->ll_cb.port, bd_addr, scn);
            *p_handle       = p_cb->ll_cb.port.handle;
        }
        else
        {
            /* when called by other OBX functions */
            status  = OBX_SUCCESS;
        }

        if (status == OBX_SUCCESS)
        {
            /* Connect request packet always has the final bit set */
            *p++ = (OBX_REQ_CONNECT | OBX_FINAL);
            p += OBX_PKT_LEN_SIZE;

            *p++ = OBX_VERSION;
            *p++ = OBX_CONN_FLAGS;
            UINT16_TO_BE_STREAM(p, p_cb->ll_cb.port.rx_mtu);
            /* IrOBEX spec forbids connection ID in Connect Request */
            p_pkt = obx_cl_prepend_msg(p_cb, p_pkt, msg, (UINT16)(p - msg) );

            p_pkt->event    = OBX_CONNECT_REQ_EVT;
            obx_csm_event(p_cb, OBX_CONNECT_REQ_CEVT, p_pkt);
        }
        else
        {
            OBX_TRACE_ERROR1("Error opening port for scn: %d", scn);
            obx_cl_free_cb(p_cb);
        }
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_AllocSession
**
** Description  This function registers a client entity to OBEX.
**              If p_session_info is not NULL, it tries to find an suspended session
**                  with matching session_info.
**              If scn is not 0, it allocates a control block for this new session.
**              Otherwise, it allocates a control block for the given PSM.
**              The associated virtual PSM assigned by L2CAP is returned in p_psm
**              The allocated OBEX handle is returned in p_handle.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_AllocSession (UINT8 *p_session_info, UINT8 scn, UINT16 *p_psm,
                              tOBX_CL_CBACK *p_cback, tOBX_HANDLE *p_handle)
{
    tOBX_STATUS status = OBX_NO_RESOURCES;
    tOBX_CL_CB  *p_cb;

    WC_ASSERT(p_handle);
    WC_ASSERT(p_cback);

    OBX_TRACE_API2("OBX_AllocSession scn: %d, psm:0x%x", scn, *p_psm);

    if (p_session_info)
    {
        p_cb = obx_cl_get_suspended_cb(p_handle, p_session_info);
        if (p_cb)
        {
            p_cb->srm = p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX];
            status  = OBX_SUCCESS;
        }
    }
    else
    {
        p_cb = obx_cl_get_cb(*p_handle);
        if (p_cb == NULL)
            p_cb = obx_cl_alloc_cb();


    }

    if (p_cb)
    {
        p_cb->rsp_code  = 0;
        p_cb->psm       = 0;

        if (p_cb->sess_st != OBX_SESS_SUSPENDED)
            p_cb->sess_st = OBX_SESS_NONE;

        if (p_psm && L2C_IS_VALID_PSM(*p_psm))
        {
            obx_register_l2c(p_cb, *p_psm);
            /* obx_register_l2c puts the virtual psm in p_cb->psm */
            if (p_cb->psm)
            {
                *p_psm  = p_cb->psm;
                status  = OBX_SUCCESS;
            }
        }

        /* check SCN only when a virtual PSM is not allocated */
        if (!p_cb->psm)
        {
            if (scn)
            {
                /* borrow this data member temporarily */
                p_cb->rsp_code  = scn;
                status  = OBX_SUCCESS;
            }
        }
    }

    if (status != OBX_SUCCESS)
    {
        obx_cl_free_cb(p_cb);
        p_cb = NULL;
    }

    if (p_cb)
    {
        *p_handle       = p_cb->ll_cb.comm.handle;
        p_cb->p_cback   = p_cback;
        p_cb->state     = OBX_CS_NOT_CONNECTED;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_CreateSession
**
** Description  This function registers a client entity to OBEX.
**              It may send a CreateSession request and wait for CreateSession response.
**              It sends a CONNECT request to the server specified by the API parameters.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_CreateSession (BD_ADDR bd_addr, UINT16 mtu, BOOLEAN srm, UINT32 nonce,
                                          tOBX_HANDLE handle, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_NO_RESOURCES;
    tOBX_CL_CB  *p_cb;
    UINT8       *p;
    UINT8       *pn;
    BT_HDR      *p_req;
    UINT8       data[20];
    tOBX_TRIPLET triplet[4];
    UINT8       num_trip = 0;

    OBX_TRACE_API1("OBX_CreateSession handle: 0x%x", handle);
    p_cb = obx_cl_get_cb(handle);

    if (p_cb)
    {
        if (p_cb->state != OBX_CS_NOT_CONNECTED || p_cb->sess_st != OBX_SESS_NONE)
        {
            OBX_TRACE_ERROR2("bad state: %d, or sess_st:%d", p_cb->state, p_cb->sess_st);
            return status;
        }

        if (p_cb->ll_cb.comm.id == 0)
        {
            p_cb->ll_cb.comm.rx_mtu = mtu;

            OBX_TRACE_DEBUG2("scn: %d, psm:0x%x", p_cb->rsp_code, p_cb->psm);

            if (p_cb->psm)
            {
                /* L2CAP channel is not open yet- open one
                 * this is the first CONNECT request */
                status = obx_open_l2c(p_cb, bd_addr);
            }
            else if (p_cb->rsp_code) /* p_cb->rsp_code is used as the scn */
            {
                /* port is not open yet- open one
                 * this is the first CONNECT request */
                status = obx_open_port(&p_cb->ll_cb.port, bd_addr, p_cb->rsp_code);
            }
        }
        else
        {
            /* when called by other OBX functions */
            status  = OBX_SUCCESS;
        }

        if (status == OBX_SUCCESS)
        {
            /* OBEX 1.5 */
            p_cb->srm       = OBX_SRM_NO;
            if (srm)
                p_cb->srm   = OBX_SRM_ENABLE;
            p_cb->nonce     = nonce;
            p_cb->sess_st   = OBX_SESS_NONE;
            if (nonce)
            {
                if ( (p_req = OBX_HdrInit(handle, OBX_MIN_MTU)) != NULL)
                {
                    p = (UINT8 *) (p_req + 1) + p_req->offset;
                    /* Session request packet always has the final bit set */
                    *p++ = (OBX_REQ_SESSION | OBX_FINAL);
                    p_req->len = 3;
                    p = data;
                    /* add session opcode */
                    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_OP;
                    triplet[num_trip].len = OBX_LEN_SESS_PARAM_SESS_OP;
                    triplet[num_trip].p_array = p;
                    *p = OBX_SESS_OP_CREATE;
                    p += OBX_LEN_SESS_PARAM_SESS_OP;
                    num_trip++;

                    /* add device addr */
                    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_ADDR;
                    triplet[num_trip].len = BD_ADDR_LEN;
                    triplet[num_trip].p_array = p;
                    BTM_GetLocalDeviceAddr (p);
                    p += BD_ADDR_LEN;
                    num_trip++;

                    /* add nonce 4 - 16 bytes */
                    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_NONCE;
                    triplet[num_trip].len = OBX_LOCAL_NONCE_SIZE;
                    pn = &p_cb->sess_info[OBX_SESSION_ID_SIZE];
                    triplet[num_trip].p_array = pn;
                    UINT32_TO_BE_STREAM(pn, nonce);
                    num_trip++;

                    /* add timeout */
                    triplet[num_trip].p_array = p;
                    if (obx_add_timeout (&triplet[num_trip], obx_cb.sess_tout_val, &p_cb->param.sess))
                    {
                        num_trip ++;
                        p = &p_cb->sess_info[OBX_SESSION_INFO_TO_IDX];
                        UINT32_TO_BE_STREAM(p, obx_cb.sess_tout_val);
                        p_cb->param.sess.timeout = obx_cb.sess_tout_val;
                    }

                    OBX_AddTriplet(p_req, OBX_HI_SESSION_PARAM, triplet, num_trip);
                    if (p_pkt)
                    {
                        /* assume that these headers are to be added to the connect req */
                        p_cb->p_next_req = p_pkt;
                    }
                    /* adjust the packet len */
                    p = (UINT8 *) (p_req + 1) + p_req->offset + 1;
                    UINT16_TO_BE_STREAM(p, p_req->len);
                    p_req->event    = OBX_SESSION_REQ_EVT;
                    p_cb->sess_st   = OBX_SESS_CREATE;
                    obx_csm_event(p_cb, OBX_SESSION_REQ_CEVT, p_req);
                }
                else
                    status = OBX_NO_RESOURCES;
            }
            else /* legacy */
            {
                obx_ca_connect_req (p_cb, p_pkt);
            }
        }
        if (status != OBX_SUCCESS)
        {
            obx_cl_free_cb(p_cb);
        }
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_ResumeSession
**
** Description  This function registers a client entity to OBEX and resumes
**              a previously interrupted reliable session.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_ResumeSession (BD_ADDR bd_addr, UINT8 ssn, UINT32 offset, tOBX_HANDLE handle)
{
    tOBX_STATUS status = OBX_NO_RESOURCES;
    UINT8       *p;
    tOBX_CL_CB  *p_cb;
    BT_HDR      *p_req;
    tOBX_TRIPLET triplet[6];
    UINT8       data[13];
    UINT8       num_trip = 0;
    UINT8       *pn;

    OBX_TRACE_API3("OBX_ResumeSession handle: 0x%x ssn:%d offset:%d", handle, ssn, offset);
    p_cb = obx_cl_get_cb(handle);

    if (p_cb)
    {
        OBX_TRACE_DEBUG3("OBX_ResumeSession, sess_st:%d srm:0x%x, saved state:0x%x", p_cb->sess_st,
            p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX], p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX]);
        if (p_cb->sess_st == OBX_SESS_SUSPENDED)
        {
            if ((p_req = OBX_HdrInit(OBX_HANDLE_NULL, OBX_MIN_MTU)) != NULL)
            {
                p = (UINT8 *) (p_req + 1) + p_req->offset;
                /* Session request packet always has the final bit set */
                *p++ = (OBX_REQ_SESSION | OBX_FINAL);
                p_req->len = 3;

                /* add session opcode */
                p = data;
                triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_OP;
                triplet[num_trip].len = OBX_LEN_SESS_PARAM_SESS_OP;
                triplet[num_trip].p_array = p;
                *p = OBX_SESS_OP_RESUME;
                p += OBX_LEN_SESS_PARAM_SESS_OP;
                num_trip++;

                /* add device addr */
                triplet[num_trip].tag = OBX_TAG_SESS_PARAM_ADDR;
                triplet[num_trip].len = BD_ADDR_LEN;
                triplet[num_trip].p_array = p;
                BTM_GetLocalDeviceAddr (p);
                p += BD_ADDR_LEN;
                num_trip++;

                /* add nonce 4 - 16 bytes */
                triplet[num_trip].tag = OBX_TAG_SESS_PARAM_NONCE;
                triplet[num_trip].len = OBX_LOCAL_NONCE_SIZE;
                pn = &p_cb->sess_info[OBX_SESSION_ID_SIZE];
                triplet[num_trip].p_array = pn;
                num_trip++;

                /* add session id */
                triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_ID;
                triplet[num_trip].len = OBX_SESSION_ID_SIZE;
                triplet[num_trip].p_array = p_cb->sess_info;
                num_trip++;

                if (p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX] & OBX_SRM_ENGAGE)
                {
                    /* add ssn */
                    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_NSEQNUM;
                    triplet[num_trip].len = 1;
                    triplet[num_trip].p_array = p;
                    *p++ = ssn;
                    num_trip++;

                    if (offset)
                    {
                    /* add object offset */
                    p_cb->param.sess.obj_offset = offset;
                    triplet[num_trip].tag = OBX_TAG_SESS_PARAM_OBJ_OFF;
                    triplet[num_trip].len = OBX_LEN_SESS_PARAM_OBJ_OFF;
                    triplet[num_trip].p_array = p;
                    UINT32_TO_BE_STREAM(p, offset);
                    num_trip++;
                }
                }

                p_cb->sess_st   = OBX_SESS_RESUME;
                OBX_AddTriplet(p_req, OBX_HI_SESSION_PARAM, triplet, num_trip);
                /* adjust the packet len */
                p = (UINT8 *) (p_req + 1) + p_req->offset + 1;
                UINT16_TO_BE_STREAM(p, p_req->len);
                p_req->event    = OBX_SESSION_REQ_EVT;
                status = OBX_SUCCESS;

                if (p_cb->ll_cb.comm.id == 0 || p_cb->ll_cb.comm.p_send_fn == 0)
                {
                    /* the transport is closed. open it again */
                    OBX_TRACE_DEBUG2("scn: %d, psm:0x%x", p_cb->rsp_code, p_cb->psm);
                    p_cb->ll_cb.comm.rx_mtu = OBX_MAX_MTU;

                    if (p_cb->psm)
                    {
                        /* L2CAP channel is not open yet- open one
                         * this is the first CONNECT request */
                        status      = obx_open_l2c(p_cb, bd_addr);
                    }
                    else if (p_cb->rsp_code) /* p_cb->rsp_code is used as the scn */
                    {
                        /* port is not open yet- open one
                         * this is the first CONNECT request */
                        status      = obx_open_port(&p_cb->ll_cb.port, bd_addr, p_cb->rsp_code);
                    }
                }

                if (status == OBX_SUCCESS)
                {
                    pn = &p_cb->sess_info[OBX_SESSION_INFO_ID_IDX];
                    BE_STREAM_TO_UINT32(p_cb->conn_id, pn);
                    p_cb->ssn = ssn;
                    p_cb->param.sess.ssn = ssn;
                    obx_csm_event(p_cb, OBX_SESSION_REQ_CEVT, p_req);
                }
            }
        }
        else
        {
            OBX_TRACE_ERROR1("Handle is not in a right state: %d for RESUME", p_cb->sess_st);
            status = OBX_BAD_HANDLE;
        }
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_SessionReq
**
** Description  This function is used to Suspend/Close a session or update the
**              timeout value of the session.
**              If timeout is 0, OBX_SESS_TIMEOUT_VALUE is the value used
**              THe timeout value is not added for the CloseSession request
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_SessionReq (tOBX_HANDLE handle, tOBX_SESS_OP opcode, UINT32 timeout)
{
    tOBX_STATUS status = OBX_NO_RESOURCES;
    UINT8       *p;
    tOBX_CL_CB  *p_cb;
    BT_HDR      *p_req;
    tOBX_TRIPLET triplet[3];
    UINT8       data[12];
    UINT8       num_trip = 0;
    tOBX_SESS_ST    old_sess_st;

    p_cb = obx_cl_get_cb(handle);

    if (p_cb)
    {
        OBX_TRACE_API2("OBX_SessionReq st:%d opcode:%d", p_cb->sess_st, opcode);
        old_sess_st = p_cb->sess_st;
        if ((p_req = OBX_HdrInit(OBX_HANDLE_NULL, OBX_MIN_MTU)) != NULL)
        {
            status = OBX_SUCCESS;
            p = (UINT8 *) (p_req + 1) + p_req->offset;
            /* Session request packet always has the final bit set */
            *p++ = (OBX_REQ_SESSION | OBX_FINAL);
            p_req->len = 3;

            /* add session opcode */
            p = data;
            triplet[num_trip].tag = OBX_TAG_SESS_PARAM_SESS_OP;
            triplet[num_trip].len = OBX_LEN_SESS_PARAM_SESS_OP;
            triplet[num_trip].p_array = p;
            *p = opcode;
            p += OBX_LEN_SESS_PARAM_SESS_OP;
            num_trip++;
            if (timeout == 0)
            {
                timeout = obx_cb.sess_tout_val;
                if (p_cb->srm & OBX_SRM_ENABLE)
                    timeout += obx_cb.sess_tout_val;
            }
            triplet[num_trip].p_array = p;
            switch (opcode)
            {
            case OBX_SESS_OP_CLOSE:
                /* do not need any other session parameters */
                if (p_cb->sess_st != OBX_SESS_NONE)
                    p_cb->sess_st  = OBX_SESS_CLOSE;
                else
                {
                    OBX_TRACE_ERROR1("Handle is not in a right state: %d for CLOSE", p_cb->sess_st);
                    status = OBX_BAD_HANDLE;
                }
                break;

            case OBX_SESS_OP_SUSPEND:
                /* do not need any other session parameters */
                if (p_cb->sess_st == OBX_SESS_ACTIVE)
                {
                    /* add timeout value */
                    num_trip += obx_add_timeout (&triplet[num_trip], timeout, &p_cb->param.sess);
                    p_cb->sess_st  = OBX_SESS_SUSPEND;
                }
                else
                {
                    OBX_TRACE_ERROR1("Handle is not in a right state: %d for SUSPEND", p_cb->sess_st);
                    status = OBX_BAD_HANDLE;
                }
                break;

            case OBX_SESS_OP_SET_TIME:
                if (p_cb->sess_st == OBX_SESS_ACTIVE)
                {
                    /* add timeout value */
                    num_trip += obx_add_timeout (&triplet[num_trip], timeout, &p_cb->param.sess);
                    p_cb->sess_st   = OBX_SESS_TIMEOUT;
                }
                else
                {
                    OBX_TRACE_ERROR1("Handle is not in a right state: %d for SET_TIME", p_cb->sess_st);
                    status = OBX_BAD_HANDLE;
                }
                break;
            default:
                OBX_TRACE_ERROR1("bad session opcode :%d", opcode);
                status = OBX_BAD_PARAMS;
            }

            OBX_TRACE_DEBUG4("OBX_SessionReq, sess_st:%d->%d opcode:%d status:%d", old_sess_st, p_cb->sess_st, opcode, status);
            if (status != OBX_SUCCESS)
                GKI_freebuf (p_req);
            else
            {
                OBX_AddTriplet(p_req, OBX_HI_SESSION_PARAM, triplet, num_trip);
                if (p_cb->sess_st == OBX_SESS_SUSPEND)
                {
                    p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] = p_cb->state;
                    if (p_cb->state == OBX_CS_PARTIAL_SENT)
                        p_cb->sess_info[OBX_SESSION_INFO_ST_IDX] = p_cb->prev_state;
                    p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX] = p_cb->srm;
                    OBX_TRACE_DEBUG2("suspend saved st:%d, srm:0x%x", p_cb->sess_info[OBX_SESSION_INFO_ST_IDX], p_cb->sess_info[OBX_SESSION_INFO_SRM_IDX]);
                }
                /* adjust the packet len */
                p = (UINT8 *) (p_req + 1) + p_req->offset + 1;
                UINT16_TO_BE_STREAM(p, p_req->len);
                p_req->event    = OBX_SESSION_REQ_EVT;
                obx_csm_event(p_cb, OBX_SESSION_REQ_CEVT, p_req);
            }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_GetPortHandle
**
** Description  This function is called to get RFCOMM port handle for the obex connection.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if no existing connection.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_GetPortHandle(tOBX_HANDLE handle, UINT16 *port_handle)
{
    BD_ADDR bd_addr;
    UINT16      lcid;
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_CL_CB  *p_cb = obx_cl_get_cb(handle);

    if (p_cb)
    {
        if (PORT_CheckConnection(p_cb->ll_cb.port.port_handle, bd_addr, &lcid) != PORT_SUCCESS)
        {
            status = OBX_NO_RESOURCES;
        } 
        else
        {
            *port_handle = p_cb->ll_cb.port.port_handle;
        }
    }
    else
        status = OBX_BAD_HANDLE;

    return status;
}

/*******************************************************************************
**
** Function     OBX_SetPathReq
**
** Description  This function sends a Set Path request to the connected server.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_SetPathReq(tOBX_HANDLE handle, UINT8 flags, BT_HDR *p_pkt)
{
    tOBX_CL_CB  *p_cb = obx_cl_get_cb(handle);
    tOBX_STATUS status = OBX_BAD_HANDLE;
    UINT8       msg[OBX_HDR_OFFSET];
    UINT8       *p = msg;
    UINT8       good_flags = (OBX_SPF_BACKUP | OBX_SPF_NO_CREATE);

    if (p_cb)
    {
        /* SetPath request packet always has the final bit set */
        *p++    = (OBX_REQ_SETPATH | OBX_FINAL);
        p       += OBX_PKT_LEN_SIZE;
        *p++    = (flags & good_flags); /* send only good flags */
        *p++    = OBX_SETPATH_CONST;

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
        p_pkt   = obx_cl_prepend_msg(p_cb, p_pkt, msg, (UINT16)(p - msg) );
        p_pkt->event    = OBX_SETPATH_REQ_EVT;
        obx_csm_event(p_cb, OBX_SETPATH_REQ_CEVT, p_pkt);
        status = OBX_SUCCESS;
    }
    return status;
}

/*******************************************************************************
**
** Function     obx_prepend_req_msg
**
** Description  This function is called to add request code and connection ID
**              to the given OBEX message 
** Returns      void
**
*******************************************************************************/
tOBX_STATUS obx_prepend_req_msg(tOBX_HANDLE handle, tOBX_CL_EVENT event, UINT8 req_code, BT_HDR *p_pkt)
{
    tOBX_STATUS status = OBX_SUCCESS;
    tOBX_CL_CB *p_cb = obx_cl_get_cb(handle);
    UINT8       msg[OBX_HDR_OFFSET];
    UINT8       *p = msg;
    UINT8       srm = 0;
    UINT8       num_hdrs, num_body;

    if (p_cb)
    {
        *p++ = req_code;
        p += OBX_PKT_LEN_SIZE;

        /* add session sequence number, if session is active */
        if (p_cb->sess_st == OBX_SESS_ACTIVE)
        {
            *p++ = OBX_HI_SESSION_SN;
            *p++ = p_cb->ssn;
        }

        req_code &= ~OBX_FINAL;

        /* add connection ID, if needed */
        if ((p_cb->conn_id != OBX_INVALID_CONN_ID) && 
            /* always use connection ID in CONNECTED state or being challenged on operation */
            ((p_cb->state == OBX_CS_CONNECTED) || (p_cb->state == OBX_CS_OP_UNAUTH) ||
            /* always use connection ID for abort and disconnect. they may be out of sequence */
            (req_code == OBX_REQ_ABORT) || (req_code == OBX_REQ_DISCONNECT)))
        {
            *p++    = OBX_HI_CONN_ID;
            UINT32_TO_BE_STREAM(p, p_cb->conn_id);
        }

        /* add SRM header, if SRM is enabled */
        if (p_cb->srm & OBX_SRM_ENABLE)
        {
            if (p_cb->state == OBX_CS_CONNECTED)
            {
                if(event == OBX_PUT_REQ_CEVT)
                {
                    num_hdrs = OBX_ReadNumHdrs(p_pkt, &num_body);
                    OBX_TRACE_DEBUG2("num_hdrs:%d num_body:%d", num_hdrs, num_body);
                    if (num_hdrs == num_body)
                    {
                        OBX_TRACE_DEBUG0("it is left-over, drop it");
                        if (p_pkt)
                            GKI_freebuf (p_pkt);
                        return OBX_BAD_PARAMS;
                    }
                    srm = OBX_SRM_REQING | OBX_SRM_WAIT;
                }
                else if (event == OBX_GET_REQ_CEVT)
                {
                    srm = OBX_SRM_REQING;
                }

                OBX_TRACE_DEBUG2("cb srm: 0x%x/0x%x", p_cb->srm, srm );
                if (srm)
                {
                    p_cb->srm |= srm;
                    *p++ = OBX_HI_SRM;
                    *p++ = OBX_HV_SRM_ENABLE;
                }
            }
        }

        p_pkt = obx_cl_prepend_msg(p_cb, p_pkt, msg, (UINT16)(p - msg) );
        p_pkt->event    = obx_sm_evt_to_api_evt[event];
        obx_csm_event(p_cb, event, p_pkt);
    }
    else
        status = OBX_BAD_HANDLE;

    return status;
}

/*******************************************************************************
**
** Function     OBX_PutReq
**
** Description  This function sends a Put request to the connected server.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_PutReq(tOBX_HANDLE handle, BOOLEAN final, BT_HDR *p_pkt)
{
    UINT8 req_code = OBX_REQ_PUT;
    if (final)
        req_code |= OBX_FINAL;
    return obx_prepend_req_msg(handle, OBX_PUT_REQ_CEVT, req_code, p_pkt);

}


/*******************************************************************************
**
** Function     OBX_GetReq
**
** Description  This function sends a Get request to the connected server.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_GetReq(tOBX_HANDLE handle, BOOLEAN final, BT_HDR *p_pkt)
{
    UINT8 req_code = OBX_REQ_GET;
    OBX_TRACE_API1("OBX_GetReq final: 0x%x", final );
    if (final)
        req_code |= OBX_FINAL;
    return obx_prepend_req_msg(handle, OBX_GET_REQ_CEVT, req_code, p_pkt);
}


/*******************************************************************************
**
** Function     OBX_AbortReq
**
** Description  This function sends an Abort request to the connected server.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_AbortReq(tOBX_HANDLE handle, BT_HDR *p_pkt)
{
    /* Disconnect request always has the final bit set */
    UINT8 req_code = (OBX_REQ_ABORT|OBX_FINAL);
    return obx_prepend_req_msg(handle, OBX_ABORT_REQ_CEVT, req_code, p_pkt);
}


/*******************************************************************************
**
** Function     OBX_DisconnectReq
**
** Description  This function sends a Disconnect request to the connected server.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_DisconnectReq(tOBX_HANDLE handle, BT_HDR *p_pkt)
{
    /* Disconnect request always has the final bit set */
    UINT8 req_code = (OBX_REQ_DISCONNECT|OBX_FINAL);
    return obx_prepend_req_msg(handle, OBX_DISCNT_REQ_CEVT, req_code, p_pkt);
}

/*******************************************************************************
**
** Function     OBX_ActionReq
**
** Description  This function sends a Action request to the connected server.
**              The Name Header and DestName Header must be in p_pkt for 
**                  the Copy and Move Object action.
**              The Name header and Permission Header must be in p_pkt for
**                  the Set Object Permission action.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_BAD_HANDLE, if the handle is not valid.
**
*******************************************************************************/
tOBX_STATUS OBX_ActionReq(tOBX_HANDLE handle, tOBX_ACTION action_id, BT_HDR *p_pkt)
{
    /* Disconnect request always has the final bit set */
    UINT8 req_code = (OBX_REQ_ACTION|OBX_FINAL);
    UINT8       *p;

    if (p_pkt == NULL)
    {
        OBX_TRACE_ERROR0("OBX_ActionReq must include Name & DestName Header for Copy/Move action" );
        OBX_TRACE_ERROR0("OBX_ActionReq must include Name & Permission Header for Set Object Permission action" );
        return OBX_BAD_PARAMS;
    }

    /* add the Action ID header before the other headers */
    p_pkt->offset   -= 2;
    p_pkt->len      += 2;
    p       = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    *p++    = OBX_HI_ACTION_ID;
    *p++    = action_id;

    return obx_prepend_req_msg(handle, OBX_ACTION_REQ_CEVT, req_code, p_pkt);
}
