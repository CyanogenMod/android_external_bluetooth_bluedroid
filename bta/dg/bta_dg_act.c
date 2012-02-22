/*****************************************************************************
**
**  Name:           bta_dg_act.c
**
**  Description:    This file contains the data gateway action functions
**                  for the state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)

#include <string.h>
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_dg_api.h"
#include "bta_dg_int.h"
#include "bta_dg_co.h"
#include "btm_api.h"
#include "sdp_api.h"
#include "dun_api.h"
#include "gki.h"
#include "bd.h"

/* Event mask for RfCOMM port callback */
#define BTA_DG_PORT_EV_MASK         (PORT_EV_FC | PORT_EV_FCS | PORT_EV_RXCHAR | \
                                     PORT_EV_TXEMPTY | PORT_EV_CTS | PORT_EV_DSR | \
                                     PORT_EV_RING | PORT_EV_CTSS | PORT_EV_DSRS)

/* RX and TX data flow mask */
#define BTA_DG_RX_MASK              0x0F
#define BTA_DG_TX_MASK              0xF0

/* size of database for service discovery */
#define BTA_DG_DISC_BUF_SIZE        450

/* size of discovery db taking into account tBTA_DG_SDP_DB */
#define BTA_DG_SDP_DB_SIZE          (BTA_DG_DISC_BUF_SIZE - (UINT32) &((tBTA_DG_SDP_DB *) 0)->db)

/* UUID lookup table */
const UINT16 bta_dg_uuid[] =
{
    UUID_SERVCLASS_SERIAL_PORT,         /* BTA_SPP_SERVICE_ID */
    UUID_SERVCLASS_DIALUP_NETWORKING,   /* BTA_DUN_SERVICE_ID */
    UUID_SERVCLASS_FAX,                 /* BTA_FAX_SERVICE_ID */
    UUID_SERVCLASS_LAN_ACCESS_USING_PPP /* BTA_LAP_SERVICE_ID */
};

/* BTM service ID lookup table */
const UINT8 bta_dg_sec_id[] =
{
    BTM_SEC_SERVICE_SERIAL_PORT,         /* BTA_SPP_SERVICE_ID */
    BTM_SEC_SERVICE_DUN,                 /* BTA_DUN_SERVICE_ID */
    BTM_SEC_SERVICE_FAX,                 /* BTA_FAX_SERVICE_ID */
    BTM_SEC_SERVICE_SERIAL_PORT          /* BTA_LAP_SERVICE_ID */
};

/* RS-232 signal lookup table */
const UINT8 bta_dg_sig[2][4] = 
{
    {PORT_CLR_DTRDSR, PORT_CLR_CTSRTS, PORT_CLR_RI, PORT_CLR_DCD},
    {PORT_SET_DTRDSR, PORT_SET_CTSRTS, PORT_SET_RI, PORT_SET_DCD}
};

/* declare sdp callback functions */
void bta_dg_sdp_cback_1(UINT16 status);
void bta_dg_sdp_cback_2(UINT16 status);
void bta_dg_sdp_cback_3(UINT16 status);
void bta_dg_sdp_cback_4(UINT16 status);
void bta_dg_sdp_cback_5(UINT16 status);
void bta_dg_sdp_cback_6(UINT16 status);
void bta_dg_sdp_cback_7(UINT16 status);
void bta_dg_sdp_cback_8(UINT16 status);

/* SDP callback function table */
typedef tSDP_DISC_CMPL_CB *tBTA_DG_SDP_CBACK;
const tBTA_DG_SDP_CBACK bta_dg_sdp_cback_tbl[] =
{
    bta_dg_sdp_cback_1,
    bta_dg_sdp_cback_2,
    bta_dg_sdp_cback_3,
    bta_dg_sdp_cback_4,
    bta_dg_sdp_cback_5,
    bta_dg_sdp_cback_6,
    bta_dg_sdp_cback_7,
    bta_dg_sdp_cback_8
};

/*******************************************************************************
**
** Function         bta_dg_sdp_cback
**
** Description      SDP callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_sdp_cback(UINT16 status, UINT8 idx)
{
    tBTA_DG_DISC_RESULT  *p_buf;

    APPL_TRACE_DEBUG1("bta_dg_sdp_cback status:0x%x", status);

    if ((p_buf = (tBTA_DG_DISC_RESULT *) GKI_getbuf(sizeof(tBTA_DG_DISC_RESULT))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_DISC_RESULT_EVT;
        p_buf->hdr.layer_specific = idx;
        p_buf->status = status;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_dg_sdp_cback_1 to 8
**
** Description      SDP callback functions.  Since there is no way to
**                  distinguish scb from the callback we need separate 
**                  callbacks for each scb.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_sdp_cback_1(UINT16 status) {bta_dg_sdp_cback(status, 1);}
void bta_dg_sdp_cback_2(UINT16 status) {bta_dg_sdp_cback(status, 2);}
void bta_dg_sdp_cback_3(UINT16 status) {bta_dg_sdp_cback(status, 3);}
void bta_dg_sdp_cback_4(UINT16 status) {bta_dg_sdp_cback(status, 4);}
void bta_dg_sdp_cback_5(UINT16 status) {bta_dg_sdp_cback(status, 5);}
void bta_dg_sdp_cback_6(UINT16 status) {bta_dg_sdp_cback(status, 6);}
void bta_dg_sdp_cback_7(UINT16 status) {bta_dg_sdp_cback(status, 7);}
void bta_dg_sdp_cback_8(UINT16 status) {bta_dg_sdp_cback(status, 8);}

/*******************************************************************************
**
** Function         bta_dg_port_cback
**
** Description      RFCOMM Port callback
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_port_cback(UINT32 code, UINT16 port_handle)
{
    tBTA_DG_RFC_PORT    *p_buf;
    tBTA_DG_SCB         *p_scb;

    /* set flow control state directly */
    if (code & PORT_EV_FC)
    {
        if ((p_scb = bta_dg_scb_by_handle(port_handle)) != NULL)
        {
            p_scb->rfc_enable = ((code & PORT_EV_FCS) == PORT_EV_FCS);

            if ((p_scb->flow_mask & BTA_DG_RX_MASK) == BTA_DG_RX_PUSH_BUF)
            {
                bta_dg_co_rx_flow(port_handle, p_scb->app_id, p_scb->rfc_enable);
            }
        }
    }

    if ((p_buf = (tBTA_DG_RFC_PORT *) GKI_getbuf(sizeof(tBTA_DG_RFC_PORT))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_RFC_PORT_EVT;
        p_buf->hdr.layer_specific = port_handle;
        p_buf->code = code;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_dg_mgmt_cback
**
** Description      RFCOMM management callback
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_mgmt_cback(UINT32 code, UINT16 port_handle)
{
    BT_HDR      *p_buf;
    tBTA_DG_SCB *p_scb;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->layer_specific = port_handle;

        if (code == PORT_SUCCESS)
        {
            p_buf->event = BTA_DG_RFC_OPEN_EVT;
            bta_sys_sendmsg(p_buf);
        }
        else
        {
            if ((p_scb = bta_dg_scb_by_handle(port_handle)) != NULL)
            {
/*                APPL_TRACE_EVENT6("DG Port Close: handle %d, scb %d, app_id %d, in_use %d, scn %d, state %d",
                    port_handle, bta_dg_scb_to_idx(p_scb), p_scb->app_id, p_scb->in_use, p_scb->scn, p_scb->state);
*/
                p_buf->event = BTA_DG_RFC_CLOSE_EVT;

                /* Execute close event before exiting RFC callback to avoid a race
                   condition where RFC could reallocate port_handle before DG can
                   process the event queue
                 */
                bta_dg_sm_execute(p_scb, p_buf->event, (tBTA_DG_DATA *) p_buf);
            }
            else
            {
                APPL_TRACE_WARNING1("DG received RFC Close for unknown handle (%d)",
                                    port_handle);
            }

            GKI_freebuf(p_buf);
        }
    }
    else
    {
        APPL_TRACE_ERROR0("DG RFC callback: No Resources!");
    }
}

/*******************************************************************************
**
** Function         bta_dg_data_cback
**
** Description      RFCOMM data callback.  This callback is used when a server
**                  TX data path is configured for BTA_DG_TX_PUSH to transfer
**                  data directly from RFCOMM to the phone.
**                  
**
** Returns          void
**
*******************************************************************************/
static int bta_dg_data_cback(UINT16 port_handle, void *p_data, UINT16 len)
{
    tBTA_DG_SCB     *p_scb;

    if ((p_scb = bta_dg_scb_by_handle(port_handle)) != NULL)
    {
        bta_dg_co_tx_write(port_handle, p_scb->app_id, (UINT8 *) p_data, len);
    }
    return 0;
}

/*******************************************************************************
**
** Function         bta_dg_setup_port
**
** Description      Setup RFCOMM port for use by DG.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_setup_port(tBTA_DG_SCB *p_scb)
{
    /* store scb in handle lookup table */
    bta_dg_cb.hdl_to_scb[p_scb->port_handle - 1] = bta_dg_scb_to_idx(p_scb);

    /* call application init call-out */
    p_scb->flow_mask = bta_dg_co_init(p_scb->port_handle, p_scb->app_id);

    /* set up port and data callbacks */
    if ((p_scb->flow_mask & BTA_DG_TX_MASK) == BTA_DG_TX_PUSH)
    {
        PORT_SetDataCallback(p_scb->port_handle, bta_dg_data_cback);
    }
    PORT_SetEventMask(p_scb->port_handle, BTA_DG_PORT_EV_MASK);
    PORT_SetEventCallback(p_scb->port_handle, bta_dg_port_cback);
}

/*******************************************************************************
**
** Function         bta_dg_setup_server
**
** Description      This function initializes values of the DG scb and sets up
**                  the SDP record for the server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_setup_server(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    tBTA_DG_LISTEN  listen;
    tBTA_DG_SCB     *p_match;

    /* initialize control block */
    p_scb->service_id = p_data->api_listen.service;
    p_scb->sec_mask = p_data->api_listen.sec_mask;
    p_scb->app_id = p_data->api_listen.app_id;
    p_scb->is_server = TRUE;
    BCM_STRNCPY_S(p_scb->name, BTA_SERVICE_NAME_LEN+1, p_data->api_listen.name, BTA_SERVICE_NAME_LEN);
    p_scb->name[BTA_SERVICE_NAME_LEN] = '\0';

    /* if existing matching server */
    if ((p_match = bta_dg_server_match(p_scb)) != NULL)
    {
        /* set SCN same as existing matching server */
        p_scb->scn = p_match->scn;

        /* copy sdp handle in order to unregister it from any server instance (being last) */
        p_scb->sdp_handle = p_match->sdp_handle;
    }
    else
    {
        /* allocate SDP record and SCN */
        p_scb->sdp_handle = SDP_CreateRecord();
        p_scb->scn = BTM_AllocateSCN();
        APPL_TRACE_DEBUG2( "bta_dg_setup_server: x%x, scn:%d", p_scb->sdp_handle, p_scb->scn);

        /* set up sdp record */
        DUN_AddRecord(bta_dg_uuid[p_scb->service_id - BTA_SPP_SERVICE_ID],
                      p_data->api_listen.name, p_scb->scn, 0, p_scb->sdp_handle);

        bta_sys_add_uuid(bta_dg_uuid[p_scb->service_id - BTA_SPP_SERVICE_ID]);
    }

    /* Listen on RFCOMM port */
    bta_dg_listen(p_scb, p_data);

    /* call app callback with listen event */
    listen.handle = bta_dg_scb_to_idx(p_scb);
    listen.app_id = p_scb->app_id;
    (*bta_dg_cb.p_cback)(BTA_DG_LISTEN_EVT, (tBTA_DG *) &listen);
}

/*******************************************************************************
**
** Function         bta_dg_listen
**
** Description      Call DUN_Listen to set up a server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_listen(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    UINT8           lkup_id = p_scb->service_id - BTA_SPP_SERVICE_ID;

    /* Listen on RFCOMM port */
    if (DUN_Listen(bta_dg_uuid[lkup_id], p_scb->name, p_scb->scn, p_bta_dg_cfg->mtu[lkup_id],
                   p_scb->sec_mask, &p_scb->port_handle, bta_dg_mgmt_cback)
        == DUN_SUCCESS)
    {
        bta_dg_setup_port(p_scb);
    }
}

/*******************************************************************************
**
** Function         bta_dg_del_record
**
** Description      Delete SDP record.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_del_record(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    /* if no existing matching server */
    if (bta_dg_server_match(p_scb) == NULL)
    {
        APPL_TRACE_DEBUG1( "bta_dg_del_record: x%x", p_scb->sdp_handle);
        if(p_scb->sdp_handle)
            SDP_DeleteRecord(p_scb->sdp_handle);
        bta_sys_remove_uuid( bta_dg_uuid[p_scb->service_id - BTA_SPP_SERVICE_ID] );
        BTM_FreeSCN(p_scb->scn);
        BTM_SecClrService(bta_dg_sec_id[p_scb->service_id - BTA_SPP_SERVICE_ID]);
    }
}

/*******************************************************************************
**
** Function         bta_dg_shutdown
**
** Description      Shut down a server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_shutdown(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    UINT8 status;
    
    APPL_TRACE_DEBUG1( "bta_dg_shutdown: x%x", p_scb->sdp_handle);

    status = DUN_Shutdown(p_scb->port_handle);

    APPL_TRACE_DEBUG1("DUN shutdown status %d", status);

    bta_dg_del_record(p_scb, p_data);

    /* set shutdown flag */
    p_scb->dealloc = TRUE;
}

/*******************************************************************************
**
** Function         bta_dg_close
**
** Description      Close RFCOMM connection.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_close(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    if (p_scb->port_handle)
    {
        DUN_Close(p_scb->port_handle);
    }
}

/*******************************************************************************
**
** Function         bta_dg_set_dealloc
**
** Description      Set the dealloc flag.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_set_dealloc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    p_scb->dealloc = TRUE;
}

/*******************************************************************************
**
** Function         bta_dg_rx_path
**
** Description      Handle data on the RX path (data sent from the phone to
**                  BTA).
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_rx_path(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    /* if data path configured for rx pull */
    if ((p_scb->flow_mask & BTA_DG_RX_MASK) == BTA_DG_RX_PULL)
    {
        /* if RFCOMM can accept data */
        if (p_scb->rfc_enable == TRUE)
        {
          /* call application callout function for rx path */
          bta_dg_co_rx_path(p_scb->port_handle, p_scb->app_id, p_scb->mtu);
        }
    }
    /* else data path configured for rx push */
    else
    {
        
    }    
}

/*******************************************************************************
**
** Function         bta_dg_tx_path
**
** Description      Handle the TX data path (data sent from BTA to the phone).
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_tx_path(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    BT_HDR          *p_buf;
    UINT16          port_errors;
    tPORT_STATUS    port_status;
    int             status;
    
    /* if data path configured for tx pull */
    if ((p_scb->flow_mask & BTA_DG_TX_MASK) == BTA_DG_TX_PULL)
    {
        /* call application callout function for tx path */
        bta_dg_co_tx_path(p_scb->port_handle, p_scb->app_id);
    }
    /* if configured for zero copy push */
    else if ((p_scb->flow_mask & BTA_DG_TX_MASK) == BTA_DG_TX_PUSH_BUF)
    {
        /* if app can accept data */
        while (p_scb->app_enable == TRUE)
        {
            /* read data from RFCOMM */
            if ((status = PORT_Read(p_scb->port_handle, &p_buf)) == PORT_SUCCESS)
            {
                if(p_buf == NULL)
                {
                    return;
                }
                /* send data to application */
                bta_dg_co_tx_writebuf(p_scb->port_handle, p_scb->app_id, p_buf);
            }
            else if (status == PORT_LINE_ERR)
            {
                PORT_ClearError(p_scb->port_handle, &port_errors, &port_status);
                return;
            }
            else
            {
                return;
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_fc_state
**
** Description      Set the application flow control state.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_fc_state(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    p_scb->app_enable = p_data->ci_tx_flow.enable;
    PORT_FlowControl(p_scb->port_handle, p_scb->app_enable);
}

/*******************************************************************************
**
** Function         bta_dg_writebuf
**
** Description      Handle a bta_dg_ci_rx_writebuf() and send data to RFCOMM.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_writebuf(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    if ((p_scb->flow_mask & BTA_DG_RX_MASK) == BTA_DG_RX_PUSH_BUF)
    {
        PORT_Write(p_scb->port_handle, (BT_HDR *) p_data);
    }
}

/*******************************************************************************
**
** Function         bta_dg_control
**
** Description      Pass RS-232 control signals from phone to RFCOMM.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_control(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    int         i;

    for (i = 0; i < 4; i++)
    {
        if (p_data->ci_control.signals & 1)
        {
            PORT_Control(p_scb->port_handle, bta_dg_sig[(p_data->ci_control.values & 1)][i]);
        }
        p_data->ci_control.signals >>= 1;
        p_data->ci_control.values >>= 1;
    }
}

/*******************************************************************************
**
** Function         bta_dg_rfc_control
**
** Description      Handle a change in RS-232 signals from RFCOMM
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_rfc_control(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    UINT8   signals = 0;
    UINT8   values = 0;
    UINT8   port_values = 0;

    PORT_GetModemStatus(p_scb->port_handle, &port_values);

    if (p_data->rfc_port.code & PORT_EV_CTS)
    {
        values |= (p_data->rfc_port.code & PORT_EV_CTSS) ? BTA_DG_RTSCTS_ON : BTA_DG_RTSCTS_OFF;
        signals |= BTA_DG_RTSCTS;
    }
    if (p_data->rfc_port.code & PORT_EV_DSR)
    {
        values |= (p_data->rfc_port.code & PORT_EV_DSRS) ? BTA_DG_DTRDSR_ON : BTA_DG_DTRDSR_OFF;
        signals |= BTA_DG_DTRDSR;
    }
    if (p_data->rfc_port.code & PORT_EV_RING)
    {
        values |= (port_values & PORT_RING_ON);
        signals |= BTA_DG_RI;
    }

    bta_dg_co_control(p_scb->port_handle, p_scb->app_id, signals, values);
}

/*******************************************************************************
**
** Function         bta_dg_rfc_open
**
** Description      Handle RFCOMM channel opening.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_rfc_open(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    tPORT_STATUS    status;
    UINT16          lcid;
    tBTA_DG_OPEN    open;

    /* reset flow control state */
    p_scb->rfc_enable = TRUE;
    p_scb->app_enable = TRUE;

    /* get mtu of connection */
    PORT_GetQueueStatus(p_scb->port_handle, &status);
    p_scb->mtu = status.mtu_size;

    /* call app call-out */
    bta_dg_co_open(p_scb->port_handle, p_scb->app_id, p_scb->service_id, p_scb->mtu);

    /* get bd addr of peer */
    PORT_CheckConnection(p_scb->port_handle, open.bd_addr, &lcid);

    /* call app callback with open event */
    open.handle = bta_dg_scb_to_idx(p_scb);
    open.service = p_scb->service_id;
    open.app_id = p_scb->app_id;

    bdcpy(p_scb->peer_addr, open.bd_addr);

    /* inform role manager */
    bta_sys_conn_open( BTA_ID_DG ,p_scb->app_id, open.bd_addr);

    (*bta_dg_cb.p_cback)(BTA_DG_OPEN_EVT, (tBTA_DG *) &open);    
}

/*******************************************************************************
**
** Function         bta_dg_rfc_close
**
** Description      
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_rfc_close(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    tBTA_DG_CLOSE   close;

    /* call app call-out */
    if (p_scb->port_handle != 0)
    {
        bta_dg_co_close(p_scb->port_handle, p_scb->app_id);
    }

    /* call app callback */
    close.handle = bta_dg_scb_to_idx(p_scb);
    close.app_id = p_scb->app_id;

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_DG ,p_scb->app_id, p_scb->peer_addr);

    (*bta_dg_cb.p_cback)(BTA_DG_CLOSE_EVT, (tBTA_DG *) &close);    

    /* if not shutting down, initialize the server */
    if (p_scb->dealloc == FALSE)
    {
        bta_dg_setup_port(p_scb);
    }
    /* else dealloc control block */
    else
    {
        bta_dg_scb_dealloc(p_scb);
    }
}

/*******************************************************************************
**
** Function         bta_dg_dealloc
**
** Description      
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_dealloc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    APPL_TRACE_DEBUG1( "bta_dg_dealloc: x%x", p_scb->sdp_handle);
    bta_dg_scb_dealloc(p_scb);
}
/*******************************************************************************
**
** Function         bta_dg_disc_result
**
** Description      Process a discovery result.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_disc_result(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    tSDP_DISC_REC       *p_rec = NULL;
    tSDP_DISC_ATTR      *p_attr;
    tSDP_PROTOCOL_ELEM  pe;
    UINT16              name_len;
    char                *p_name;
    UINT16              evt = BTA_DG_DISC_FAIL_EVT;

            
    if (p_data->disc_result.status == SDP_SUCCESS || p_data->disc_result.status == SDP_DB_FULL)
    {
        /* loop through all records we found */
        while (TRUE)
        {
            /* get next record; if none found, we're done */
            if ((p_rec = SDP_FindServiceInDb(&p_scb->p_disc->db,
                            bta_dg_uuid[p_scb->service_id - BTA_SPP_SERVICE_ID], p_rec)) == NULL)
            {
                break;
            }

            /* get scn from proto desc list; if not found, go to next record */
            if (SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe))
            {
                p_scb->scn = (UINT8) pe.params[0];
            }
            else
            {
                continue;
            }

            /* if service name provided for match */
            if (p_scb->p_disc->name[0] != 0)
            {
                /* get service name */
                if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SERVICE_NAME)) != NULL)
                {
                    /* check if name matches */
                    p_name = (char *) p_attr->attr_value.v.array;
                    name_len = SDP_DISC_ATTR_LEN(p_attr->attr_len_type);
                    if (strncmp(p_scb->p_disc->name, p_name, name_len) == 0)
                    {
                        /* name matches; we found it */
                        evt = BTA_DG_DISC_OK_EVT;
                        break;
                    }
                    else
                    {
                        /* name does not match; continue */
                        continue;
                    }
                }
            }
            /* else name match not required; we're done */
            else
            {
                evt = BTA_DG_DISC_OK_EVT;
                break;
            }
        }
    }

    /* send ourselves event to process disc ok or fail */
    bta_dg_sm_execute(p_scb, evt, NULL);
}

/*******************************************************************************
**
** Function         bta_dg_do_disc
**
** Description      Do service discovery.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_do_disc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    tBTA_DG_OPENING opening;
    tSDP_UUID       uuid_list;
    UINT16          attr_list[] = {ATTR_ID_SERVICE_CLASS_ID_LIST,
                                   ATTR_ID_PROTOCOL_DESC_LIST,
                                   ATTR_ID_SERVICE_NAME};

    /* allocate buffer for sdp database */
	if ((p_scb->p_disc = (tBTA_DG_SDP_DB *) GKI_getbuf(BTA_DG_DISC_BUF_SIZE)) != NULL)
	{
		/* store parameters */
		BCM_STRCPY_S(p_scb->p_disc->name, sizeof(p_scb->p_disc->name), p_data->api_open.name);
		bdcpy(p_scb->peer_addr, p_data->api_open.bd_addr);
		p_scb->sec_mask = p_data->api_open.sec_mask;
		p_scb->service_id = p_data->api_open.service;
		p_scb->app_id = p_data->api_open.app_id;

		/* initialize some scb parameters */
		p_scb->dealloc = TRUE;
		p_scb->port_handle = 0;


		bta_sys_app_open(BTA_ID_DG, p_scb->app_id, p_scb->peer_addr);

		/* set up service discovery database */
		uuid_list.len = LEN_UUID_16;
		uuid_list.uu.uuid16 = bta_dg_uuid[p_scb->service_id - BTA_SPP_SERVICE_ID];
		
		/* coverity[OVERRUN_STATIC] False-positive: All pointers are ok. len parameter size (in bytes) of the memory must be larger than sizeof(tSDP_DISCOVERY_DB) */
		SDP_InitDiscoveryDb(&p_scb->p_disc->db, BTA_DG_SDP_DB_SIZE, 1, &uuid_list, 3, attr_list);

		/* initiate service discovery */
		if (SDP_ServiceSearchAttributeRequest(p_scb->peer_addr, &p_scb->p_disc->db,
		                                  bta_dg_sdp_cback_tbl[bta_dg_scb_to_idx(p_scb) - 1]))
		{
			/* call callback with opening event */
		    opening.handle = bta_dg_scb_to_idx(p_scb);
		    opening.app_id = p_scb->app_id;
		    (*bta_dg_cb.p_cback)(BTA_DG_OPENING_EVT, (tBTA_DG *) &opening);
		}
		else
		{
		    bta_dg_sdp_cback(SDP_NO_RESOURCES, bta_dg_scb_to_idx(p_scb));
		}
	}
	else
	{
		APPL_TRACE_ERROR0("DG DISC: No Resources!");
	}

}

/*******************************************************************************
**
** Function         bta_dg_open
**
** Description      Open a client connection.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_open(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    UINT8           lkup_id = p_scb->service_id - BTA_SPP_SERVICE_ID;

    if (DUN_Connect(bta_dg_uuid[lkup_id], p_scb->peer_addr, p_scb->scn, p_bta_dg_cfg->mtu[lkup_id],
            p_scb->sec_mask, &p_scb->port_handle, bta_dg_mgmt_cback) == DUN_SUCCESS)
    {
        bta_dg_setup_port(p_scb);
    }
}

/*******************************************************************************
**
** Function         bta_dg_free_db
**
** Description      Free discovery database.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_free_db(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data)
{
    if (p_scb->p_disc != NULL)
    {
        GKI_freebuf(p_scb->p_disc);
        p_scb->p_disc = NULL;
    }
}

#endif /* BTA_DG_INCLUDED */
