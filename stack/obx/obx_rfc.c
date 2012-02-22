/*****************************************************************************
**
**  Name:         obx_rfc.c
**
**  File:         OBEX  interface to the RFCOMM module
**
**  Copyright (c) 2003-2008, Broadcom Corp., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "wcassert.h"

#include "bt_target.h"
#include "obx_int.h"

#include "port_api.h"
#include "sdpdefs.h"
#include "btu.h"

/*******************************************************************************
** Function     obx_rfc_snd_evt
** Description  Sends an rfcomm event to OBX through the BTU task.
*******************************************************************************/
static void obx_rfc_snd_evt (tOBX_PORT_CB *p_pcb, UINT32 code)
{
    BT_HDR          *p_msg;
    tOBX_PORT_EVT   *p_evt;
    UINT16           event;

    if (!p_pcb)
        return;

    p_msg = (BT_HDR*)GKI_getbuf(BT_HDR_SIZE + sizeof(tOBX_PORT_EVT));
    WC_ASSERT(p_msg);

    if (p_pcb->handle & OBX_CL_HANDLE_MASK)
        event = BT_EVT_TO_OBX_CL_MSG;
    else
        event = BT_EVT_TO_OBX_SR_MSG;

    p_msg->event  = event;
    p_msg->len    = sizeof(tOBX_PORT_EVT);
    p_msg->offset = 0;
    p_evt = (tOBX_PORT_EVT *)(p_msg + 1);
    p_evt->code = code;
    p_evt->p_pcb  = p_pcb;

    GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_msg);
}

/*******************************************************************************
** Function     obx_rfc_cback
** Description  find the port control block and post an event to BTU task.
**              NOTE: This callback does not handle connect up/down events.
**                  obx_rfc_mgmt_cback is used for these events.
*******************************************************************************/
static void obx_rfc_cback (UINT32 code, UINT16 port_handle)
{
    tOBX_PORT_CB    *p_pcb = obx_port_handle_2cb(port_handle);

    if (p_pcb)
    {
        obx_rfc_snd_evt (p_pcb, code);
    }
    else
    {
        OBX_TRACE_WARNING0("Can not find control block");
    }
}

/*******************************************************************************
** Function     obx_rfc_mgmt_cback
** Callback registered with the PORT entity's Management Callback so that OBX
** can be notified when the connection has come up or gone down.
********************************************************************************/
void obx_rfc_mgmt_cback(UINT32 port_status, UINT16 port_handle)
{
    tOBX_PORT_CB    *p_pcb = obx_port_handle_2cb(port_handle);
    UINT32           code;

#if (OBX_CLIENT_INCLUDED == TRUE)
    if (!p_pcb && port_status != PORT_SUCCESS)
    {
        /* See if error called within RFCOMM_CreateConnection */
        if (obx_cb.p_temp_pcb)
        {
            p_pcb = obx_cb.p_temp_pcb;
            obx_cb.p_temp_pcb = NULL;
        }
    }
#endif

    if (p_pcb)
    {
        code = (port_status == PORT_SUCCESS) ? PORT_EV_CONNECTED : PORT_EV_CONNECT_ERR;
        obx_rfc_snd_evt (p_pcb, code);
    }
    else
    {
        OBX_TRACE_WARNING0("mgmt cback: Can not find control block");
    }
}


/*******************************************************************************
** Function     obx_read_data
** Description  This functions reads data from FRCOMM. Return a message if the
**              whole packet is read.
**              The following defines how BT_HDR is used in this function
**              event:          response or request event code
**              len:            the length read so far.
**              offset:         offset to the beginning of the actual data.
**              layer_specific: left
*******************************************************************************/
static BT_HDR * obx_read_data (tOBX_PORT_CB *p_pcb, tOBX_VERIFY_OPCODE p_verify_opcode)
{
    BT_HDR  *p_ret = NULL;
    UINT8   *p;
    UINT16  ask_len;
    UINT16  got_len;
    int     rc;
    tOBX_RX_HDR *p_rxh;
    tOBX_SR_SESS_CB  *p_scb;
    UINT8       opcode;
    UINT16      pkt_len;
    BOOLEAN     failed = FALSE;

    OBX_TRACE_DEBUG1("obx_read_data port_handle:%d", p_pcb->port_handle );
    for (;;)
    {
        if (p_pcb->p_rxmsg == NULL)
        {
            p_pcb->p_rxmsg = OBX_HdrInit((tOBX_HANDLE)(p_pcb->handle|OBX_HANDLE_RX_MTU_MASK),
                                         OBX_LRG_DATA_POOL_SIZE);
            memset((p_pcb->p_rxmsg + 1), 0, sizeof(tOBX_RX_HDR));
        }
        /* we use this header to keep the status of this packet (instead of in control block) */
        p_rxh   = (tOBX_RX_HDR *)(p_pcb->p_rxmsg + 1);

        ask_len = 0;
        if (p_rxh->code == 0)
        {
            if (p_pcb->p_rxmsg->len == 0) /* we need this if statement in case of "throw away" */
                ask_len = 1;
        }
        else if (p_pcb->p_rxmsg->len < (OBX_PKT_LEN_SIZE + 1) )
        {
            /* if we do not know the packet len yet, read from port */
            ask_len = OBX_PKT_LEN_SIZE + 1 - p_pcb->p_rxmsg->len;
        }
        else
        {
            /* we already know the packet len.
             * determine how many more bytes we need for this packet */
            ask_len = p_rxh->pkt_len - p_pcb->p_rxmsg->len;
        }

        /* the position of next byte to read */
        p = (UINT8 *)(p_pcb->p_rxmsg + 1) + p_pcb->p_rxmsg->offset + p_pcb->p_rxmsg->len;

        if (ask_len)
        {
            rc = PORT_ReadData( p_pcb->port_handle, (char*)p, ask_len, &got_len);
            if (rc != PORT_SUCCESS)
            {
                OBX_TRACE_WARNING2("Error %d returned from PORT_Read_Data, len:%d", rc, got_len);
            }

            OBX_TRACE_DEBUG2("ask_len: %d, got_len:%d", ask_len, got_len );
            if (got_len == 0)
            {
                /* If we tried to read but did not get anything, */
                /* there is nothing more to read at this time */
                break;
            }
            p_pcb->p_rxmsg->len             += got_len;
            p_pcb->p_rxmsg->layer_specific  -= got_len;
        }

        /* process the response/opcode, if not yet */
        if (p_rxh->code == 0 && p_pcb->p_rxmsg->len)
        {
            opcode  = *((UINT8 *)(p_pcb->p_rxmsg + 1) + p_pcb->p_rxmsg->offset);
            if ( (p_verify_opcode)(opcode, p_rxh) == OBX_BAD_SM_EVT)
            {
                OBX_TRACE_WARNING1("bad opcode:0x%x - Disconnecting", opcode );
                /* received data with bad length. */

                /*bad length disconnect */
                failed = TRUE;
                break;
            }
            continue;
        }

        /* process the packet len */
        if (p_rxh->pkt_len == 0 && p_pcb->p_rxmsg->len >= (OBX_PKT_LEN_SIZE + 1) )
        {
            p = (UINT8 *)(p_pcb->p_rxmsg + 1) + p_pcb->p_rxmsg->offset + 1;
            BE_STREAM_TO_UINT16(pkt_len, p);

            if ( (pkt_len > p_pcb->rx_mtu) ||
                (pkt_len < 3) ||
                (pkt_len == 4) )
            {
                /* received data with bad length. */
                OBX_TRACE_WARNING2("Received bad packet len -Disconnecting: %d RX MTU: %x",
                    pkt_len, p_pcb->rx_mtu);
                /*bad length disconnect */
                failed = TRUE;
                break;
            }
            else
            {
                /* keep the packet len in the header */
                p_rxh->pkt_len  = pkt_len;
            }
            continue;
        }

        if (p_pcb->p_rxmsg->len == p_rxh->pkt_len)
        {
            /* received a whole packet */
            OBX_TRACE_DEBUG1("got a packet. opcode:0x%x", p_rxh->code );
            p_ret = p_pcb->p_rxmsg;
            p_pcb->p_rxmsg = NULL;
            break;
        }

    }

    if (failed)
    {
        if (p_pcb->handle & OBX_CL_HANDLE_MASK)
        {
            obx_close_port(p_pcb->port_handle);
        }
        else
        {
            if ((p_scb = obx_sr_get_scb(p_pcb->handle)) != NULL)
                obx_ssm_event(p_scb, OBX_PORT_CLOSE_SEVT, NULL);
            
        }
        p_ret = NULL;
    }

    return p_ret;
}


#if (OBX_CLIENT_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_cl_proc_evt
** Description  This is called to process BT_EVT_TO_OBX_CL_MSG
**              Process events from RFCOMM. Get the associated client control
**              block. If this is a response packet, stop timer. Call
**              obx_csm_event() with event OK_CFM, FAIL_CFM or CONT_CFM.
*******************************************************************************/
void obx_cl_proc_evt(tOBX_PORT_EVT *p_evt)
{
    tOBX_PORT_CB *p_pcb = p_evt->p_pcb;
    tOBX_CL_CB  *p_cb = obx_cl_get_cb(p_pcb->handle);
    BT_HDR      *p_pkt;

    if (p_cb == NULL)
    {
        /* probably already close the port and deregistered from OBX */
        OBX_TRACE_ERROR1("Could not find control block for handle: 0x%x", p_pcb->handle);
        return;
    }

    if (p_evt->code & PORT_EV_CONNECT_ERR)
    {
        obx_csm_event(p_cb, OBX_PORT_CLOSE_CEVT, NULL);
        return;
    } /* PORT_EV_CONNECT_ERR */

    if (p_evt->code & PORT_EV_TXEMPTY)
    {
        obx_csm_event(p_cb, OBX_TX_EMPTY_CEVT, NULL);
    } /* PORT_EV_TXEMPTY */

    if (p_evt->code & PORT_EV_RXCHAR)
    {
        while( (p_pkt = obx_read_data(p_pcb, obx_verify_response)) != NULL )
        {
            if (GKI_queue_is_empty(&p_pcb->rx_q))
            {
                if (p_pkt->event != OBX_BAD_SM_EVT)
                {
                    obx_cl_proc_pkt (p_cb, p_pkt);
                }
                else
                {
                    OBX_TRACE_ERROR0("bad SM event" );
                }
            }
            else if (p_pkt->event != OBX_BAD_SM_EVT)
            {
                GKI_enqueue (&p_pcb->rx_q, p_pkt);
                if (p_pcb->rx_q.count > obx_cb.max_rx_qcount)
                {
                    p_pcb->stopped = TRUE;
                    PORT_FlowControl(p_pcb->port_handle, FALSE);
                }
            }
        } /* while received a packet */
    } /* PORT_EV_RXCHAR */

    if (p_evt->code & PORT_EV_FC)
    {
        if (p_evt->code & PORT_EV_FCS)
        {
            OBX_TRACE_EVENT0("cl flow control event - FCS SET ----" );
            obx_csm_event(p_cb, OBX_FCS_SET_CEVT, NULL);
        }
    } /* PORT_EV_FC */
}
#endif

#if (OBX_SERVER_INCLUDED == TRUE)
/*******************************************************************************
** Function     obx_build_dummy_rsp
** Description  make up a dummy response if the app does not call response API
**              yet and AbortRsp is called
*******************************************************************************/
BT_HDR * obx_build_dummy_rsp(tOBX_SR_SESS_CB *p_scb, UINT8 rsp_code)
{
    BT_HDR  *p_pkt;
    UINT8   *p;
    UINT16  size = 3;

    p_pkt   = OBX_HdrInit(p_scb->ll_cb.comm.handle, OBX_CMD_POOL_SIZE);
    p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
    *p++    = (rsp_code|OBX_FINAL);
    if (p_scb->conn_id)
    {
        size += 5;
        UINT16_TO_BE_STREAM(p, size);
        *p++    = OBX_HI_CONN_ID;
        UINT32_TO_BE_STREAM(p, p_scb->conn_id);
    }
    else
    {
        UINT16_TO_BE_STREAM(p, size);
    }
    p_pkt->len   = size;
    p_pkt->event = OBX_PUT_RSP_EVT; /* or OBX_GET_RSP_EVT: for tracing purposes */
    return p_pkt;
}

/*******************************************************************************
** Function     obx_add_port
** Description  check if this server has aother un-used port to open
**              
**              
** Returns      void
*******************************************************************************/
void obx_add_port(tOBX_HANDLE obx_handle)
{
    tOBX_SR_CB * p_cb = obx_sr_get_cb(obx_handle);
    tOBX_SR_SESS_CB *p_scb, *p_scb0;
    int xx;
    tOBX_STATUS status = OBX_NO_RESOURCES;
    BOOLEAN found;

    OBX_TRACE_DEBUG1("obx_add_port handle:0x%x", obx_handle );
    if (p_cb && p_cb->scn)
    {
        OBX_TRACE_DEBUG2("num_sess:%d scn:%d", p_cb->num_sess, p_cb->scn );
        p_scb0 = &obx_cb.sr_sess[p_cb->sess[0]-1];
        found = FALSE;
        /* find an RFCOMM port that is not connected yet */
        for (xx=0; xx < p_cb->num_sess && p_cb->sess[xx]; xx++)
        {
            p_scb = &obx_cb.sr_sess[p_cb->sess[xx]-1];
            OBX_TRACE_DEBUG3("[%d] id:0x%x, state:%d", xx, p_scb->ll_cb.comm.id, p_scb->state );

            if (p_scb->ll_cb.comm.p_send_fn == (tOBX_SEND_FN *)obx_rfc_snd_msg
                && p_scb->state == OBX_SS_NOT_CONNECTED)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            for (xx=0; xx < p_cb->num_sess && p_cb->sess[xx]; xx++)
            {
                p_scb = &obx_cb.sr_sess[p_cb->sess[xx]-1];
                OBX_TRACE_DEBUG2("[%d] port_handle:%d", xx, p_scb->ll_cb.port.port_handle );
                if (!p_scb->ll_cb.comm.id)
                {
                    status = obx_open_port(&p_scb->ll_cb.port, BT_BD_ANY, p_cb->scn);
                    if (status == OBX_SUCCESS)
                    {
                        p_scb->ll_cb.port.rx_mtu= p_scb0->ll_cb.port.rx_mtu;
                        p_scb->state            = OBX_SS_NOT_CONNECTED;
                    }
                    break;
                }
            }
        }
    }
}

/*******************************************************************************
** Function     obx_sr_proc_evt
** Description  This is called to process BT_EVT_TO_OBX_SR_MSG
**              Process events from RFCOMM. Get the associated server control
**              block. If this is a request packet, stop timer. Find the 
**              associated API event and save it in server control block 
**              (api_evt). Fill the event parameter (param).
**              Call obx_ssm_event() with the associated events.If the associated
**              control block is not found (maybe the target header does not
**              match) or busy, compose a service unavailable response and call
**              obx_rfc_snd_msg().
*******************************************************************************/
void obx_sr_proc_evt(tOBX_PORT_EVT *p_evt)
{
    tOBX_SR_SESS_CB *p_scb;
    BT_HDR      *p_pkt;
    tOBX_RX_HDR *p_rxh;
    tOBX_PORT_CB *p_pcb = p_evt->p_pcb;


    OBX_TRACE_DEBUG2("obx_sr_proc_evt handle: 0x%x, port_handle:%d", p_evt->p_pcb->handle, p_evt->p_pcb->port_handle);
    if (p_pcb->handle == 0 || p_pcb->p_send_fn != (tOBX_SEND_FN*)obx_rfc_snd_msg)
        return;

    if ((p_scb = obx_sr_get_scb(p_pcb->handle)) == NULL)
    {
        /* probably already close the port and deregistered from OBX */
        OBX_TRACE_ERROR1("Could not find control block for handle: 0x%x", p_pcb->handle);
        return;
    }

    if (p_evt->code & PORT_EV_CONNECTED)
    {
        p_scb->ll_cb.port.tx_mtu = OBX_MIN_MTU;
        obx_start_timer(&p_scb->ll_cb.comm);
        /* Get the Bd_Addr */
        PORT_CheckConnection (p_scb->ll_cb.port.port_handle,
                              p_scb->param.conn.peer_addr,
                              NULL);
         memcpy(p_scb->peer_addr, p_scb->param.conn.peer_addr, BD_ADDR_LEN);
   }

    if (p_evt->code & PORT_EV_CONNECT_ERR)
    {
        obx_ssm_event(p_scb, OBX_PORT_CLOSE_SEVT, NULL);
        return;
    } /* PORT_EV_CONNECT_ERR */

    if (p_evt->code & PORT_EV_RXCHAR)
    {
        while( (p_pkt = obx_read_data(p_pcb, obx_verify_request)) != NULL)
        {
            p_rxh = (tOBX_RX_HDR *)(p_pkt + 1);
            p_pkt->event = obx_sm_evt_to_api_evt[p_rxh->sm_evt];
            if (GKI_queue_is_empty(&p_pcb->rx_q))
            {
                if (p_pkt->event != OBX_BAD_SM_EVT)
                {
                    obx_sr_proc_pkt (p_scb, p_pkt);
                }
                else
                {
                    OBX_TRACE_ERROR0("bad SM event" );
                }
            }
            else
            {
                GKI_enqueue (&p_pcb->rx_q, p_pkt);
                if (p_pcb->rx_q.count > obx_cb.max_rx_qcount)
                {
                    p_pcb->stopped = TRUE;
                    PORT_FlowControl(p_pcb->port_handle, FALSE);
                }
            }
        } /* while a packet */
    } /* PORT_EV_RXCHAR */

    /* The server does not need to handle this event
    */
    if (p_evt->code & PORT_EV_TXEMPTY)
    {
        obx_ssm_event(p_scb, OBX_TX_EMPTY_SEVT, NULL);
    }

    if (p_evt->code & PORT_EV_FC)
    {
        if (p_evt->code & PORT_EV_FCS)
        {
            OBX_TRACE_EVENT0("sr flow control event - FCS SET ----" );
            obx_ssm_event(p_scb, OBX_FCS_SET_SEVT, NULL);
        }
    } /* PORT_EV_FC */
}
#endif /* OBX_SERVER_INCLUDED */

/*******************************************************************************
** Function     obx_open_port
** Description  Call RFCOMM_CreateConnection() to get port_handle.
**              Call PORT_SetEventCallback() with given callback.
**              Call PORT_SetEventMask() with given event mask. Return port handle.
** Returns      port handle
*******************************************************************************/
tOBX_STATUS obx_open_port(tOBX_PORT_CB *p_pcb, const BD_ADDR bd_addr, UINT8 scn)
{
    tOBX_STATUS status = OBX_SUCCESS; /* successful */
	UINT16      port_rc;
    BOOLEAN     is_server = (p_pcb->handle & OBX_CL_HANDLE_MASK)?FALSE:TRUE;
    UINT16      max_mtu = OBX_MAX_MTU;

    OBX_TRACE_DEBUG2("obx_open_port rxmtu:%d, cbmtu:%d", p_pcb->rx_mtu, max_mtu );

    /* clear buffers from previous connection */
    obx_free_buf ((tOBX_LL_CB*)p_pcb);

    /* make sure the MTU is in registered range */
    if (p_pcb->rx_mtu > max_mtu)
        p_pcb->rx_mtu   = max_mtu;
    if (p_pcb->rx_mtu < OBX_MIN_MTU)
        p_pcb->rx_mtu   = OBX_MIN_MTU;

#if (OBX_CLIENT_INCLUDED == TRUE)
    /* There's a remote chance that an error can occur in L2CAP before the handle 
     * before the handle can be assigned (server side only).  We will save the 
     * client control block while the handle is not known */
    if (!is_server)
    {
        obx_cb.p_temp_pcb = p_pcb;
    }
#endif

    port_rc = RFCOMM_CreateConnection ( UUID_PROTOCOL_OBEX, scn, 
                                        is_server, (UINT16)(p_pcb->rx_mtu+1), (BD_ADDR_PTR)bd_addr, 
                                        &p_pcb->port_handle, obx_rfc_mgmt_cback);

    OBX_TRACE_DEBUG3("obx_open_port rxmtu:%d, port_handle:%d, port.handle:0x%x",
        p_pcb->rx_mtu, p_pcb->port_handle, p_pcb->handle );

#if (OBX_CLIENT_INCLUDED == TRUE)
    if (!is_server)
    {
        obx_cb.p_temp_pcb = NULL;
    }
#endif

    if (port_rc == PORT_SUCCESS)
    {
        obx_cb.hdl_map[p_pcb->port_handle - 1] = p_pcb->handle;
        PORT_SetEventCallback (p_pcb->port_handle, obx_rfc_cback);
        PORT_SetEventMask (p_pcb->port_handle, OBX_PORT_EVENT_MASK);
        p_pcb->p_send_fn = (tOBX_SEND_FN *)obx_rfc_snd_msg;
        p_pcb->p_close_fn = obx_close_port;
    }
    else
    {
        status = OBX_NO_RESOURCES;
    }

    return status;
}


/*******************************************************************************
** Function     obx_close_port
** Description  Clear the port event mask and callback. Close the port.
** Returns      void
*******************************************************************************/
void obx_close_port(UINT16 port_handle)
{
    RFCOMM_RemoveConnection(port_handle);
}

/*******************************************************************************
** Function     obx_rfc_snd_msg
** Description  Call PORT_WriteData() to send an OBEX message to peer. If 
**              all data is sent, free the GKI buffer that holds
**              the OBEX message.  If  only portion of data is
**              sent, adjust the BT_HDR for PART state.
** Returns      TRUE if all data is sent
*******************************************************************************/
BOOLEAN obx_rfc_snd_msg(tOBX_PORT_CB *p_pcb)
{
    BOOLEAN status = FALSE;
    UINT16  bytes_written = 0;

    obx_stop_timer(&p_pcb->tle);
    PORT_WriteData(p_pcb->port_handle, ((char*)(p_pcb->p_txmsg + 1)) + p_pcb->p_txmsg->offset,
               p_pcb->p_txmsg->len, &bytes_written);

    obx_start_timer ((tOBX_COMM_CB *)p_pcb);

    if (bytes_written == p_pcb->p_txmsg->len)
    {
        GKI_freebuf(p_pcb->p_txmsg);
        p_pcb->p_txmsg = NULL;
        status = TRUE;
    }
    else
    {
        /* packet not completely written to RFCOMM */
        p_pcb->p_txmsg->offset  += bytes_written;
        p_pcb->p_txmsg->len     -= bytes_written;
    }

    return status;
}
