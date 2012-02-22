/*****************************************************************************/
/*                                                                           */
/*  Name:          hidd_conn.c                                               */
/*                                                                           */
/*  Description:   this file contains the connection interface functions     */
/*                 for device role                                           */
/*  Copyright (c) 2002-2011, Broadcom Corp., All Rights Reserved.            */
/*  Broadcom Bluetooth Core. Proprietary and confidential.                   */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "gki.h"
#include "bt_types.h"

#include "l2cdefs.h"
#include "l2c_api.h"

#include "btu.h"
#include "btm_api.h"
#include "btm_int.h"

#include "hiddefs.h"

#include "hidd_api.h"
#include "hidd_int.h"

/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static void hidd_l2cif_connect_ind (BD_ADDR  bd_addr, UINT16 l2cap_cid, UINT16 psm,
                                    UINT8 l2cap_id);
static void hidd_l2cif_connect_cfm (UINT16 l2cap_cid, UINT16 result);
static void hidd_l2cif_config_ind (UINT16 l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void hidd_l2cif_config_cfm (UINT16 l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void hidd_l2cif_disconnect_ind (UINT16 l2cap_cid, BOOLEAN ack_needed);
static void hidd_l2cif_data_ind (UINT16 l2cap_cid, BT_HDR *p_msg);
static void hidd_l2cif_disconnect_cfm (UINT16 l2cap_cid, UINT16 result);
static void hidd_l2cif_cong_ind (UINT16 l2cap_cid, BOOLEAN congested);

static const tL2CAP_APPL_INFO reg_info = 
{
    hidd_l2cif_connect_ind,
    hidd_l2cif_connect_cfm,
    NULL,
    hidd_l2cif_config_ind,
    hidd_l2cif_config_cfm,
    hidd_l2cif_disconnect_ind,
    hidd_l2cif_disconnect_cfm,
    NULL,
    hidd_l2cif_data_ind,
    hidd_l2cif_cong_ind,
    NULL                        /* tL2CA_TX_COMPLETE_CB */
} ;


/*******************************************************************************
**
** Function         hidd_conn_reg
**
** Description      This function registers with L2CAP.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS hidd_conn_reg (void)
{
    /* Now, register with L2CAP for control and interrupt PSMs*/
    if (!L2CA_Register (HID_PSM_CONTROL, (tL2CAP_APPL_INFO *) &reg_info))
    {
        HIDD_TRACE_ERROR0 ("HID Control Registration failed");
        return( HID_ERR_L2CAP_FAILED );
    }

    if (!L2CA_Register (HID_PSM_INTERRUPT, (tL2CAP_APPL_INFO *) &reg_info))
    {
        L2CA_Deregister( HID_PSM_CONTROL ) ;
        HIDD_TRACE_ERROR0 ("HID Interrupt Registration failed");
        return( HID_ERR_L2CAP_FAILED );
    }

    /* Set up / initialize the l2cap configuration data */
    hd_cb.l2cap_ctrl_cfg.flush_to_present = TRUE;
    hd_cb.l2cap_ctrl_cfg.flush_to = HID_DEV_FLUSH_TO;
    hd_cb.l2cap_int_cfg.flush_to_present = TRUE;
    hd_cb.l2cap_int_cfg.flush_to = HID_DEV_FLUSH_TO;

    if( hd_cb.use_qos_flg == TRUE )
    {
        memcpy( &(hd_cb.l2cap_ctrl_cfg.qos), &(hd_cb.qos_info.ctrl_ch), sizeof( FLOW_SPEC ) ) ;
        hd_cb.l2cap_ctrl_cfg.qos_present = TRUE ;

        memcpy( &(hd_cb.l2cap_int_cfg.qos), &(hd_cb.qos_info.int_ch), sizeof( FLOW_SPEC ) ) ;
        hd_cb.l2cap_int_cfg.qos_present = TRUE ;
    }
    else
    {
        hd_cb.l2cap_ctrl_cfg.qos_present = FALSE ;
        hd_cb.l2cap_int_cfg.qos_present = FALSE ;
    }

    hd_cb.conn.conn_state = HID_CONN_STATE_UNUSED ;

    return( HID_SUCCESS );
}

/*******************************************************************************
**
** Function         hidd_conn_dereg
**
** Description      This function deregisters with L2CAP.
**
** Returns          void
**
*******************************************************************************/
void hidd_conn_dereg( void )
{
    L2CA_Deregister (HID_PSM_CONTROL);
    L2CA_Deregister (HID_PSM_INTERRUPT);
}

/*******************************************************************************
**
** Function         hid_conn_disconnect
**
** Description      This function disconnects a connection.
**
** Returns          TRUE if disconnect started, FALSE if already disconnected
**
*******************************************************************************/
void hidd_conn_disconnect ()
{
    tHID_CONN   *p_hcon = &hd_cb.conn;
#if HID_DEV_PM_INCLUDED == TRUE
    tHID_DEV_PM_PWR_MD act_pm = { 0, 0, 0, 0, HCI_MODE_ACTIVE } ;
#endif

    HIDD_TRACE_EVENT0 ("HID - disconnect");

#if HID_DEV_PM_INCLUDED == TRUE
    hidd_pm_stop(); /* This will stop the idle timer if running */

    /* Need to go to to active mode to be able to send disconnect */
    hidd_pm_set_power_mode( &act_pm ); 
#endif

    if ((p_hcon->ctrl_cid != 0) || (p_hcon->intr_cid != 0))
    {
        p_hcon->conn_state = HID_CONN_STATE_DISCONNECTING;
        /* Disconnect both control and interrupt channels */
        if (p_hcon->intr_cid)
            L2CA_DisconnectReq (p_hcon->intr_cid);

        if (p_hcon->ctrl_cid)
            L2CA_DisconnectReq (p_hcon->ctrl_cid);
    }
    else
    {
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
    }
}

/*******************************************************************************
**
** Function         hidd_conn_initiate
**
** Description      This function is called by the management to create a connection.
**
** Returns          void
**
*******************************************************************************/
tHID_STATUS hidd_conn_initiate (void)
{
    tHID_CONN   *p_hcon = &hd_cb.conn;
    UINT8   service_id = BTM_SEC_SERVICE_HID_NOSEC_CTRL;
    UINT32  mx_chan_id = HIDD_NOSEC_CHN;

#if HID_DEV_NORMALLY_CONN == TRUE
    if( p_hcon->conn_state != HID_CONN_STATE_UNUSED )
        return HID_ERR_CONN_IN_PROCESS ;
#endif

    HIDD_TRACE_EVENT0 ("HID - Originate started");

    p_hcon->ctrl_cid = 0;
    p_hcon->intr_cid = 0;

    /* We are the originator of this connection */
    p_hcon->conn_flags = HID_CONN_FLAGS_IS_ORIG;
    if(hd_cb.sec_mask)
    {
        service_id = BTM_SEC_SERVICE_HID_SEC_CTRL;
        mx_chan_id = HIDD_SEC_CHN;
    }
    BTM_SetOutService (hd_cb.host_addr, service_id, mx_chan_id);

    /* Check if L2CAP started the connection process */
    if ((p_hcon->ctrl_cid = L2CA_ConnectReq (HID_PSM_CONTROL, hd_cb.host_addr)) == 0)
    {
        HIDD_TRACE_WARNING0 ("HID - Originate failed");
        return (HID_ERR_L2CAP_FAILED);
    }
    else
    {
        /* Transition to the next appropriate state, waiting for connection confirm on control channel. */
        p_hcon->conn_state = HID_CONN_STATE_CONNECTING_CTRL;

        return (HID_SUCCESS);
    }
}

/*******************************************************************************
**
** Function         hidd_sec_check_complete_term
**
** Description      HID device security check complete callback function.
**
** Returns          When security check succeed, send L2Cap connect response with
**                  OK code and send L2C configuration requret; otherwise send
**                  L2C connection response with security block code.
**
**
*******************************************************************************/
void hidd_sec_check_complete_term (BD_ADDR bd_addr, void *p_ref_data, UINT8 res)
{
    tHID_CONN    *p_hcon = &hd_cb.conn;

    if( res == BTM_SUCCESS && p_hcon->conn_state == HID_CONN_STATE_SECURITY )
    {
        p_hcon->conn_state = HID_CONN_STATE_CONNECTING_INTR;

        /* Send response to the L2CAP layer. */
        L2CA_ConnectRsp (bd_addr, p_hcon->ctrl_id, p_hcon->ctrl_cid, L2CAP_CONN_OK, L2CAP_CONN_OK); 

        /* Send a Configuration Request. */
        L2CA_ConfigReq (p_hcon->ctrl_cid, &hd_cb.l2cap_ctrl_cfg);
        
    }
    /* security check fail */
    else if (res != BTM_SUCCESS) 
    {
        L2CA_ConnectRsp (bd_addr, p_hcon->ctrl_id, p_hcon->ctrl_cid, L2CAP_CONN_SECURITY_BLOCK, L2CAP_CONN_OK); 
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
    }
}

/*******************************************************************************
**
** Function         hidd_l2cif_connect_ind
**
** Description      This function handles an inbound connection indication
**                  from L2CAP. This is the case where we are acting as a
**                  server. 
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_connect_ind (BD_ADDR  bd_addr, UINT16 l2cap_cid, UINT16 psm, UINT8 l2cap_id)
{
    tHID_CONN    *p_hcon = &hd_cb.conn;
    BOOLEAN      bAccept = TRUE;
    tL2CAP_CFG_INFO *p_l2cfg = NULL;

    HIDD_TRACE_EVENT2 ("HID - Rcvd L2CAP conn ind, PSM: 0x%04x  CID 0x%x", psm, l2cap_cid);

    /* If host address provided during registration does not match, reject connection */  
    if( hd_cb.virtual_cable && hd_cb.host_known && 
        memcmp( hd_cb.host_addr, bd_addr, BD_ADDR_LEN) )
        bAccept = FALSE;
    else
    {
        /* Check we are in the correct state for this */
        if (psm == HID_PSM_INTERRUPT)
        {
            if (p_hcon->ctrl_cid == 0)
            {
                HIDD_TRACE_WARNING0 ("HID - Rcvd INTR L2CAP conn ind, but no CTL channel");
                bAccept = FALSE;
            }
            if (p_hcon->conn_state != HID_CONN_STATE_CONNECTING_INTR)
            {
                HIDD_TRACE_WARNING1 ("HID - Rcvd INTR L2CAP conn ind, wrong state: %d", p_hcon->conn_state);
                bAccept = FALSE;
            }
            p_l2cfg = &hd_cb.l2cap_int_cfg;
        }
        else
        {
            if (p_hcon->conn_state != HID_CONN_STATE_UNUSED)
            {
                HIDD_TRACE_WARNING1 ("HID - Rcvd CTL L2CAP conn ind, wrong state: %d", p_hcon->conn_state);
                bAccept = FALSE;
            }
            p_l2cfg = &hd_cb.l2cap_ctrl_cfg;
        }
    }

    if (!bAccept)
    {
        L2CA_ConnectRsp (bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_NO_RESOURCES, 0);
        return;
    }

    if (psm == HID_PSM_CONTROL)
    {
        p_hcon->conn_flags = 0;
        p_hcon->ctrl_cid   = l2cap_cid;
        p_hcon->ctrl_id   = l2cap_id;

        p_hcon->conn_state = HID_CONN_STATE_SECURITY;

        if(btm_sec_mx_access_request (bd_addr, HID_PSM_CONTROL, 
                FALSE, BTM_SEC_PROTO_HID, 
                (hd_cb.sec_mask == 0) ?  HIDD_NOSEC_CHN : HIDD_SEC_CHN,
                &hidd_sec_check_complete_term, NULL) == BTM_CMD_STARTED)
        {
            L2CA_ConnectRsp (bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_PENDING, L2CAP_CONN_OK); 
        }
        return;
    }
    else
    {
        /* Transition to the next appropriate state, configuration */
        p_hcon->conn_state = HID_CONN_STATE_CONFIG;
        p_hcon->intr_cid   = l2cap_cid;
    }

    /* Send response to the L2CAP layer. */
    L2CA_ConnectRsp (bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_OK, L2CAP_CONN_OK); 

    /* Send a Configuration Request. */
    L2CA_ConfigReq (l2cap_cid, p_l2cfg);

    memcpy( hd_cb.host_addr, bd_addr, BD_ADDR_LEN ) ;

    HIDD_TRACE_EVENT2 ("HID - Rcvd L2CAP conn ind, sent config req, PSM: 0x%04x  CID 0x%x", psm, l2cap_cid);
}

/*******************************************************************************
**
** Function         hidd_sec_check_complete_orig
**
** Description      This function checks to see if security procedures are being
**                  carried out or not..
**
** Returns          void
**
*******************************************************************************/
void hidd_sec_check_complete_orig (BD_ADDR bd_addr, void *p_ref_data, UINT8 res)
{
    UINT32 reason;
    tHID_CONN    *p_hcon = &hd_cb.conn;

    if( res == BTM_SUCCESS && p_hcon->conn_state == HID_CONN_STATE_SECURITY )
    {
        HIDD_TRACE_EVENT0 ("HID Device - Originator security pass.");

        /* Send connect request for interrupt channel */
        if ((p_hcon->intr_cid = L2CA_ConnectReq (HID_PSM_INTERRUPT, hd_cb.host_addr)) == 0)
        {
            HIDD_TRACE_WARNING0 ("HID - INTR Originate failed");
            hidd_conn_disconnect();/* Disconnects the ctrl channel if setting of int chnl failed */
            reason = HID_L2CAP_REQ_FAIL ;
            hd_cb.conn_tries = HID_DEV_MAX_CONN_RETRY+1;
            hidd_mgmt_process_evt( HOST_CONN_FAIL, &reason ) ;
            return;
        }
        else
        {
            /* Transition to the next appropriate state, waiting for connection confirm on control channel. */
            p_hcon->conn_state = HID_CONN_STATE_CONNECTING_INTR;
        }
    }

    if( res != BTM_SUCCESS && p_hcon->conn_state == HID_CONN_STATE_SECURITY )
    {
        hidd_conn_disconnect();    
    }

}

/*******************************************************************************
**
** Function         hid_l2cif_connect_cfm
**
** Description      This function handles the connect confirm events
**                  from L2CAP. This is the case when we are acting as a
**                  client and have sent a connect request.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_connect_cfm (UINT16 l2cap_cid, UINT16 result)
{
    UINT32 reason;
    tHID_CONN    *p_hcon = &hd_cb.conn;
    tL2CAP_CFG_INFO *p_l2cfg;

    /* Verify we are in a state to accept this message */
    if (((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
     || (!(p_hcon->conn_flags & HID_CONN_FLAGS_IS_ORIG))
     || ((l2cap_cid == p_hcon->ctrl_cid) && (p_hcon->conn_state != HID_CONN_STATE_CONNECTING_CTRL))
     || ((l2cap_cid == p_hcon->intr_cid) && (p_hcon->conn_state != HID_CONN_STATE_CONNECTING_INTR)))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd unexpected conn cnf, CID 0x%x ", l2cap_cid);
        return;
    }

    if (result != L2CAP_CONN_OK)
    {
        if (l2cap_cid == p_hcon->ctrl_cid)
            p_hcon->ctrl_cid = 0;
        else
            p_hcon->intr_cid = 0;

        reason = HID_L2CAP_CONN_FAIL | ((UINT32) result) ;
        hidd_conn_disconnect(); /* Disconnects the ctrl channel if setting of int chnl failed */

        /* If connection failed due to bad air link, retry... */
        if (result != HCI_ERR_CONNECTION_TOUT && result != HCI_ERR_UNSPECIFIED 
             && result != HCI_ERR_PAGE_TIMEOUT) 
            hd_cb.conn_tries = HID_DEV_MAX_CONN_RETRY+1;
        hidd_mgmt_process_evt( HOST_CONN_FAIL, &reason ) ;
        return;
    }

    if (l2cap_cid == p_hcon->ctrl_cid)
    {
        /* let HID Device handle security itself */
        p_hcon->conn_state = HID_CONN_STATE_SECURITY;

        btm_sec_mx_access_request (hd_cb.host_addr, HID_PSM_CONTROL, 
            TRUE, BTM_SEC_PROTO_HID, 
            (hd_cb.sec_mask == 0) ?  HIDD_NOSEC_CHN : HIDD_SEC_CHN,
            &hidd_sec_check_complete_orig, NULL);

        p_l2cfg = &hd_cb.l2cap_int_cfg;

    }
    else
    {
        p_hcon->conn_state = HID_CONN_STATE_CONFIG;
        p_l2cfg = &hd_cb.l2cap_ctrl_cfg;
    }

    /* Send a Configuration Request. */
    L2CA_ConfigReq (l2cap_cid, p_l2cfg);

    HIDD_TRACE_EVENT1 ("HID - got CTRL conn cnf, sent cfg req, CID: 0x%x", l2cap_cid);
    return;
}

/*******************************************************************************
**
** Function         hidd_l2c_connected
**
** Description      This function is called when both control and interrupt channels
**                  have been created.
**
** Returns          void
**
*******************************************************************************/
void hidd_l2c_connected( tHID_CONN    *p_hcon )
{
    p_hcon->conn_state = HID_CONN_STATE_CONNECTED;

    /* Set HCI QoS */
    if( hd_cb.use_qos_flg )     
        BTM_SetQoS( hd_cb.host_addr, &(hd_cb.qos_info.hci), NULL ) ;

    hidd_mgmt_process_evt( HOST_CONN_OPEN, hd_cb.host_addr) ;
#if HID_DEV_PM_INCLUDED == TRUE
    hidd_pm_init();
    hidd_pm_start(); /* This kicks in the idle timer */
#endif
}

/*******************************************************************************
**
** Function         hidd_l2cif_config_ind
**
** Description      This function processes the L2CAP configuration indication
**                  event.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_config_ind (UINT16 l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    tHID_CONN    *p_hcon = &hd_cb.conn;

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDD_TRACE_EVENT1 ("HID - Rcvd cfg ind, sent cfg cfm, CID: 0x%x", l2cap_cid);

    /* Remember the remote MTU size */
    if ((!p_cfg->mtu_present) || (p_cfg->mtu > HID_DEV_MTU_SIZE))
        p_hcon->rem_mtu_size = HID_DEV_MTU_SIZE;
    else
        p_hcon->rem_mtu_size = p_cfg->mtu;

    /* For now, always accept configuration from the other side */
    p_cfg->flush_to_present = FALSE;
    p_cfg->mtu_present      = FALSE;
    p_cfg->result           = L2CAP_CFG_OK;

    L2CA_ConfigRsp (l2cap_cid, p_cfg);

    if (l2cap_cid == p_hcon->ctrl_cid)
        p_hcon->conn_flags |= HID_CONN_FLAGS_HIS_CTRL_CFG_DONE;
    else
        p_hcon->conn_flags |= HID_CONN_FLAGS_HIS_INTR_CFG_DONE;

    /* If all configuration is complete, change state and tell management we are up */
    if (((p_hcon->conn_flags & HID_CONN_FLAGS_ALL_CONFIGURED) == HID_CONN_FLAGS_ALL_CONFIGURED)
     && (p_hcon->conn_state == HID_CONN_STATE_CONFIG))
    {
        hidd_l2c_connected( p_hcon );
    }
}


/*******************************************************************************
**
** Function         hid_l2cif_config_cfm
**
** Description      This function processes the L2CAP configuration confirmation
**                  event.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_config_cfm (UINT16 l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    UINT32  reason;
    tHID_CONN    *p_hcon = &hd_cb.conn;
    tL2CAP_CFG_INFO * p_l2cfg = NULL;

    HIDD_TRACE_EVENT2 ("HID - Rcvd cfg cfm, CID: 0x%x  Result: %d", l2cap_cid, p_cfg->result);

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    /* config fail for unaccepted param, send reconfiguration */
    if (p_cfg->result == L2CAP_CFG_UNACCEPTABLE_PARAMS)
    {
        /* remember the host prefered config param in control block */
        p_l2cfg = (l2cap_cid == p_hcon->ctrl_cid)? &hd_cb.l2cap_ctrl_cfg : &hd_cb.l2cap_int_cfg ;
        memcpy(p_l2cfg, p_cfg, sizeof(tL2CAP_CFG_INFO));

        /* Send a second time configuration request. */
        L2CA_ConfigReq (l2cap_cid, p_l2cfg);
        return;
    }
    /* If configuration failed for other reason, disconnect the channel(s) */
    else if (p_cfg->result != L2CAP_CFG_OK)
    {
        reason = HID_L2CAP_CFG_FAIL | ((UINT32) p_cfg->result) ;
        hidd_conn_disconnect();
        hd_cb.conn_tries = HID_DEV_MAX_CONN_RETRY+1;
        hidd_mgmt_process_evt( HOST_CONN_FAIL, &reason ) ;
        return;
    }

    if (l2cap_cid == p_hcon->ctrl_cid)
        p_hcon->conn_flags |= HID_CONN_FLAGS_MY_CTRL_CFG_DONE;
    else
        p_hcon->conn_flags |= HID_CONN_FLAGS_MY_INTR_CFG_DONE;

    /* If all configuration is complete, change state and tell management we are up */
    if (((p_hcon->conn_flags & HID_CONN_FLAGS_ALL_CONFIGURED) == HID_CONN_FLAGS_ALL_CONFIGURED)
     && (p_hcon->conn_state == HID_CONN_STATE_CONFIG))
    {
        hidd_l2c_connected( p_hcon );
    }
}


/*******************************************************************************
**
** Function         hidd_l2cif_disconnect_ind
**
** Description      This function handles a disconnect event from L2CAP. If
**                  requested to, we ack the disconnect before dropping the CCB
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_disconnect_ind (UINT16 l2cap_cid, BOOLEAN ack_needed)
{
    tHID_CONN    *p_hcon = &hd_cb.conn;
    UINT16       disc_res = HCI_PENDING ;
    UINT8        evt = HOST_CONN_CLOSE;

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP disc, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (ack_needed)
        L2CA_DisconnectRsp (l2cap_cid);

    HIDD_TRACE_EVENT1 ("HID - Rcvd L2CAP disc, CID: 0x%x", l2cap_cid);

    p_hcon->conn_state = HID_CONN_STATE_DISCONNECTING;

    if (l2cap_cid == p_hcon->ctrl_cid)
        p_hcon->ctrl_cid = 0;
    else
        p_hcon->intr_cid = 0;

    if ((p_hcon->ctrl_cid == 0) && (p_hcon->intr_cid == 0))
    {
        if (!ack_needed)
            disc_res = btm_get_acl_disc_reason_code();
        HIDD_TRACE_EVENT1 ("disc_res: 0x%x", disc_res);

        if( disc_res == HCI_ERR_CONNECTION_TOUT || disc_res == HCI_ERR_UNSPECIFIED )
            evt = HOST_CONN_LOST;

        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
        hidd_mgmt_process_evt( evt, &disc_res ) ; 
#if HID_DEV_PM_INCLUDED == TRUE
        hidd_pm_stop();
#endif
    }
}


/*******************************************************************************
**
** Function         hid_l2cif_disconnect_cfm
**
** Description      This function handles a disconnect confirm event from L2CAP.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_disconnect_cfm (UINT16 l2cap_cid, UINT16 result)
{
    UINT16       disc_res = HCI_SUCCESS;
    tHID_CONN    *p_hcon = &hd_cb.conn;

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP disc cfm, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDD_TRACE_EVENT1 ("HID - Rcvd L2CAP disc cfm, CID: 0x%x", l2cap_cid);

    if (l2cap_cid == p_hcon->ctrl_cid)
        p_hcon->ctrl_cid = 0;
    else
        p_hcon->intr_cid = 0;

    if ((p_hcon->ctrl_cid == 0) && (p_hcon->intr_cid == 0))
    {
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
        hidd_mgmt_process_evt( HOST_CONN_CLOSE, &disc_res ) ; 
#if HID_DEV_PM_INCLUDED == TRUE
        hidd_pm_stop();
#endif
    }
}


/*******************************************************************************
**
** Function         hidd_l2cif_cong_ind
**
** Description      This function handles a congestion status event from L2CAP.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_cong_ind (UINT16 l2cap_cid, BOOLEAN congested)
{
    tHID_CONN    *p_hcon = &hd_cb.conn;

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP congestion status, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDD_TRACE_EVENT2 ("HID - Rcvd L2CAP congestion status, CID: 0x%x  Cong: %d", l2cap_cid, congested);

    if (congested)
        p_hcon->conn_flags |= HID_CONN_FLAGS_CONGESTED;
    else
    {
        p_hcon->conn_flags &= ~HID_CONN_FLAGS_CONGESTED;
    }

    hd_cb.callback( HID_DEV_EVT_L2CAP_CONGEST, (p_hcon->ctrl_cid == l2cap_cid), (tHID_DEV_CBACK_DATA*) &congested ) ;
}


/*******************************************************************************
**
** Function         hid_l2cif_data_ind
**
** Description      This function is called when data is received from L2CAP.
**                  if we are the originator of the connection, we are the SDP
**                  client, and the received message is queued up for the client.
**
**                  If we are the destination of the connection, we are the SDP
**                  server, so the message is passed to the server processing
**                  function.
**
** Returns          void
**
*******************************************************************************/
static void hidd_l2cif_data_ind (UINT16 l2cap_cid, BT_HDR *p_msg)
{
    tHID_CONN       *p_hcon = &hd_cb.conn;
    UINT8           *p_data = (UINT8 *)(p_msg + 1) + p_msg->offset;
    UINT8           ttype, param, rep_type, idle_rate;
    tHID_DEV_GET_REP_DATA get_rep;
    BOOLEAN suspend = FALSE;

    /* Find CCB based on CID */
    if ((p_hcon->ctrl_cid != l2cap_cid) && (p_hcon->intr_cid != l2cap_cid))
    {
        HIDD_TRACE_WARNING1 ("HID - Rcvd L2CAP data, unknown CID: 0x%x", l2cap_cid);
        GKI_freebuf (p_msg);
        return;
    }

    ttype    = HID_GET_TRANS_FROM_HDR(*p_data);
    param    = HID_GET_PARAM_FROM_HDR(*p_data);
    rep_type = param & HID_PAR_REP_TYPE_MASK;
    p_data++;

    switch (ttype)
    {
    case HID_TRANS_CONTROL:
        switch (param)
        {
        case HID_PAR_CONTROL_NOP:
        case HID_PAR_CONTROL_HARD_RESET:
        case HID_PAR_CONTROL_SOFT_RESET:
        case HID_PAR_CONTROL_EXIT_SUSPEND:
            break;

        case HID_PAR_CONTROL_SUSPEND:
            suspend = TRUE;
#if HID_DEV_PM_INCLUDED == TRUE
            hidd_pm_suspend_evt() ;
#endif
            break;

        case HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG:
            hidd_mgmt_process_evt( HID_API_DISCONNECT, NULL ) ; 
            hd_cb.unplug_on = TRUE;
            break;

        default:
            GKI_freebuf (p_msg);
            HID_DevHandShake( HID_PAR_HANDSHAKE_RSP_ERR_INVALID_PARAM ) ;
            return;
        }
        hd_cb.callback( HID_DEV_EVT_CONTROL, param, NULL ) ;
        break;

    case HID_TRANS_GET_REPORT:
        if (param & HID_PAR_GET_REP_BUFSIZE_FOLLOWS)
        {
            STREAM_TO_UINT8 (get_rep.rep_id, p_data);
            STREAM_TO_UINT16 (hd_cb.get_rep_buf_sz, p_data);
        }
        else
            hd_cb.get_rep_buf_sz = 0;

        get_rep.rep_type = param & HID_PAR_REP_TYPE_MASK;

        hd_cb.callback( HID_DEV_EVT_GET_REPORT, param, 
            (tHID_DEV_CBACK_DATA*) &get_rep ) ;
        break;

    case HID_TRANS_SET_REPORT:
        hd_cb.callback( HID_DEV_EVT_SET_REPORT, rep_type, (tHID_DEV_CBACK_DATA*) &p_msg ) ;
        break;

    case HID_TRANS_DATA:
        hd_cb.callback( HID_DEV_EVT_DATA, rep_type, (tHID_DEV_CBACK_DATA*) &p_msg ) ;
        break;

    case HID_TRANS_DATAC:
        hd_cb.callback( HID_DEV_EVT_DATC, rep_type, (tHID_DEV_CBACK_DATA*) &p_msg ) ;
        break;

    case HID_TRANS_GET_PROTOCOL:
        /* Get current protocol */
        hd_cb.callback( HID_DEV_EVT_GET_PROTO, 0, NULL ) ;
        break;

    case HID_TRANS_SET_PROTOCOL:
        hd_cb.callback( HID_DEV_EVT_SET_PROTO, (UINT32) (param & HID_PAR_PROTOCOL_MASK), NULL ) ;
        break;

    /* for HID 1.0 device only */
    case HID_TRANS_GET_IDLE:
        hd_cb.callback( HID_DEV_EVT_GET_IDLE, 0, NULL ) ;
        break;

    /* for HID 1.0 device only */
    case HID_TRANS_SET_IDLE:
        STREAM_TO_UINT8 (idle_rate, p_data);
        hd_cb.callback( HID_DEV_EVT_SET_IDLE, idle_rate, NULL ) ;
        break;

    default:
        GKI_freebuf (p_msg);
        HID_DevHandShake( HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ ) ;
        return;
    }

#if HID_DEV_PM_INCLUDED == TRUE
    if( !suspend )
        hidd_pm_start();
#endif

    if( (ttype != HID_TRANS_SET_REPORT) && (ttype != HID_TRANS_DATA) && (ttype != HID_TRANS_DATAC) )
        GKI_freebuf (p_msg);
    /* Else application frees the buffer */
}

/*******************************************************************************
**
** Function         hidd_conn_snd_data
**
** Description      This function is called to send HID data.
**
** Returns          void
**
*******************************************************************************/
tHID_STATUS hidd_conn_snd_data (tHID_SND_DATA_PARAMS *p_data)
{
    tHID_CONN   *p_hcon = &hd_cb.conn;
    BT_HDR      *p_buf;
    UINT8       *p_out;
    UINT16      bytes_copied;
    BOOLEAN     seg_req = FALSE;
    UINT16      data_size;
    UINT16      cid;
    UINT8       pool_id;
    UINT16      rem_mtu;

#if HID_DEV_PM_INCLUDED == TRUE
    hidd_pm_start();
#endif

    /* Safety check */
    if (p_data)
    {
        pool_id = (p_data->ctrl_ch) ? HID_CONTROL_POOL_ID : HID_INTERRUPT_POOL_ID;
    }
    else
    {
        HIDD_TRACE_ERROR0 ("HID_ERR_INVALID_PARAM");
        return HID_ERR_INVALID_PARAM;
    }
 
    if (p_hcon->conn_flags & HID_CONN_FLAGS_CONGESTED)
    {
        return( HID_ERR_CONGESTED );
    }

    if( (p_data->trans_type == HID_TRANS_DATA) && p_data->ctrl_ch && hd_cb.get_rep_buf_sz )
    {
        rem_mtu = hd_cb.get_rep_buf_sz;
        hd_cb.get_rep_buf_sz = 0;
    }
    else
    {
        rem_mtu = p_hcon->rem_mtu_size;
    }

    do
    {
        /* If buf is null form the HID message with ttype and params alone */
        if ( p_data->buf == NULL )
        {
            if((p_buf = (BT_HDR *)GKI_getpoolbuf (pool_id)) == NULL)
                return (HID_ERR_NO_RESOURCES);

            p_buf->offset = L2CAP_MIN_OFFSET;
            seg_req = FALSE;
            data_size = 0;
            bytes_copied = 0;
        }
        /* Segmentation case */
        else if ( (p_data->buf->len > (rem_mtu - 1)))
        {
            if((p_buf = (BT_HDR *)GKI_getpoolbuf (pool_id)) == NULL)
                return (HID_ERR_NO_RESOURCES);

            p_buf->offset = L2CAP_MIN_OFFSET;
            seg_req = TRUE;
            data_size = p_data->buf->len;
            bytes_copied = rem_mtu - 1;
        }
        else /* NOTE: The initial buffer gets used last */
        {
            p_buf = p_data->buf ;
            p_buf->offset -= 1;
            seg_req = FALSE;
            data_size = p_data->buf->len;
            bytes_copied = p_data->buf->len;
        }

        p_out         = (UINT8 *)(p_buf + 1) + p_buf->offset;
        *p_out++      = HID_BUILD_HDR(p_data->trans_type, p_data->param);

        if (seg_req)
        {
            memcpy (p_out, (((UINT8 *)(p_data->buf+1)) + p_data->buf->offset), bytes_copied);
            p_data->buf->offset += bytes_copied;
            p_data->buf->len -= bytes_copied;
        }

        p_buf->len   = bytes_copied + 1;
        data_size    -= bytes_copied;

        cid = (p_data->ctrl_ch ? p_hcon->ctrl_cid : p_hcon->intr_cid);
        /* Send the buffer through L2CAP */
        if ((p_hcon->conn_flags & HID_CONN_FLAGS_CONGESTED) || (!L2CA_DataWrite (cid, p_buf)))
        {
            return (HID_ERR_CONGESTED);
        }

        if (data_size)
            p_data->trans_type = HID_TRANS_DATAC;

    } while (data_size != 0);

    if( bytes_copied == (rem_mtu - 1) )
    {
        tHID_SND_DATA_PARAMS datc;

        datc.buf = NULL;
        datc.ctrl_ch = p_data->ctrl_ch;
        datc.param = p_data->param ;
        datc.trans_type = HID_TRANS_DATAC ;

        hidd_conn_snd_data( &datc ) ;
    }

    return (HID_SUCCESS);
}

