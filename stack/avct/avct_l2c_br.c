/*****************************************************************************
**
**  Name:           avct_l2c_br.c
**
**  Description:    This AVCTP module interfaces to L2CAP
**
**  Copyright (c) 2008, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include "data_types.h"
#include "bt_target.h"
#include "avct_api.h"
#include "avct_int.h"
#include "l2c_api.h"
#include "l2cdefs.h"

/* Configuration flags. */
#define AVCT_L2C_CFG_IND_DONE   (1<<0)
#define AVCT_L2C_CFG_CFM_DONE   (1<<1)

#if (AVCT_BROWSE_INCLUDED == TRUE)
/* callback function declarations */
void avct_l2c_br_connect_ind_cback(BD_ADDR bd_addr, UINT16 lcid, UINT16 psm, UINT8 id);
void avct_l2c_br_connect_cfm_cback(UINT16 lcid, UINT16 result);
void avct_l2c_br_config_cfm_cback(UINT16 lcid, tL2CAP_CFG_INFO *p_cfg);
void avct_l2c_br_config_ind_cback(UINT16 lcid, tL2CAP_CFG_INFO *p_cfg);
void avct_l2c_br_disconnect_ind_cback(UINT16 lcid, BOOLEAN ack_needed);
void avct_l2c_br_disconnect_cfm_cback(UINT16 lcid, UINT16 result);
void avct_l2c_br_congestion_ind_cback(UINT16 lcid, BOOLEAN is_congested);
void avct_l2c_br_data_ind_cback(UINT16 lcid, BT_HDR *p_buf);

/* L2CAP callback function structure */
const tL2CAP_APPL_INFO avct_l2c_br_appl = {
    avct_l2c_br_connect_ind_cback,
    avct_l2c_br_connect_cfm_cback,
    NULL,
    avct_l2c_br_config_ind_cback,
    avct_l2c_br_config_cfm_cback,
    avct_l2c_br_disconnect_ind_cback,
    avct_l2c_br_disconnect_cfm_cback,
    NULL,
    avct_l2c_br_data_ind_cback,
    avct_l2c_br_congestion_ind_cback,
    NULL                                /* tL2CA_TX_COMPLETE_CB */
};

/* Browsing channel eL2CAP default options */
const tL2CAP_FCR_OPTS avct_l2c_br_fcr_opts_def = {
    L2CAP_FCR_ERTM_MODE,                /* Mandatory for Browsing channel */
    AVCT_BR_FCR_OPT_TX_WINDOW_SIZE,     /* Tx window size */
    AVCT_BR_FCR_OPT_MAX_TX_B4_DISCNT,   /* Maximum transmissions before disconnecting */
    AVCT_BR_FCR_OPT_RETX_TOUT,          /* Retransmission timeout (2 secs) */
    AVCT_BR_FCR_OPT_MONITOR_TOUT,       /* Monitor timeout (12 secs) */
    L2CAP_DEFAULT_ERM_MPS               /* MPS segment size */
};

/*******************************************************************************
**
** Function         avct_l2c_br_connect_ind_cback
**
** Description      This is the L2CAP connect indication callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_connect_ind_cback(BD_ADDR bd_addr, UINT16 lcid, UINT16 psm, UINT8 id)
{
    tAVCT_LCB       *p_lcb;
    UINT16          result = L2CAP_CONN_NO_RESOURCES;
    tL2CAP_CFG_INFO cfg;
    tAVCT_BCB       *p_bcb;
    tL2CAP_ERTM_INFO ertm_info;

    memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
    cfg.mtu_present = TRUE;

    if ((p_lcb = avct_lcb_by_bd(bd_addr)) != NULL)
    {
        /* control channel exists */
        p_bcb = avct_bcb_by_lcb(p_lcb);

        if (!p_bcb->allocated)
        {
            /* browsing channel does not exist yet and the browsing channel is registered
             * - accept connection */
            p_bcb->allocated = p_lcb->allocated; /* copy the index from lcb */

            result = L2CAP_CONN_OK;
            cfg.mtu = avct_cb.mtu_br;

            cfg.fcr_present = TRUE;
            cfg.fcr         = avct_l2c_br_fcr_opts_def;
        }
    }
    /* else no control channel yet, reject */

    /* Set the FCR options: Browsing channel mandates ERTM */
    ertm_info.preferred_mode  = cfg.fcr.mode;
    ertm_info.allowed_modes = L2CAP_FCR_CHAN_OPT_ERTM;
    ertm_info.user_rx_pool_id = AVCT_BR_USER_RX_POOL_ID;
    ertm_info.user_tx_pool_id = AVCT_BR_USER_TX_POOL_ID;
    ertm_info.fcr_rx_pool_id = AVCT_BR_FCR_RX_POOL_ID;
    ertm_info.fcr_tx_pool_id = AVCT_BR_FCR_TX_POOL_ID;

    /* Send L2CAP connect rsp */
    L2CA_ErtmConnectRsp(bd_addr, id, lcid, result, 0, &ertm_info); 

    /* if result ok, proceed with connection */
    if (result == L2CAP_CONN_OK)
    {
        /* store LCID */
        p_bcb->ch_lcid = lcid;

        /* transition to configuration state */
        p_bcb->ch_state = AVCT_CH_CFG;

        /* Send L2CAP config req */
        L2CA_ConfigReq(lcid, &cfg);
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_connect_cfm_cback
**
** Description      This is the L2CAP connect confirm callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_connect_cfm_cback(UINT16 lcid, UINT16 result)
{
    tAVCT_BCB       *p_lcb;
    tL2CAP_CFG_INFO cfg;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        /* if in correct state */
        if (p_lcb->ch_state == AVCT_CH_CONN)
        {
            /* if result successful */
            if (result == L2CAP_CONN_OK)
            {
                /* set channel state */
                p_lcb->ch_state = AVCT_CH_CFG;

                /* Send L2CAP config req */
                memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));

                cfg.mtu_present = TRUE;
                cfg.mtu = avct_cb.mtu_br;

                cfg.fcr_present  = TRUE;
                cfg.fcr          = avct_l2c_br_fcr_opts_def;

                L2CA_ConfigReq(lcid, &cfg);
            }
            /* else failure */
            else
            {
                avct_bcb_event(p_lcb, AVCT_LCB_LL_CLOSE_EVT, (tAVCT_LCB_EVT *) &result);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_config_cfm_cback
**
** Description      This is the L2CAP config confirm callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_config_cfm_cback(UINT16 lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tAVCT_BCB       *p_lcb;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        /* if in correct state */
        if (p_lcb->ch_state == AVCT_CH_CFG)
        {
            /* if result successful */
            if (p_cfg->result == L2CAP_CFG_OK)
            {
                /* update flags */
                p_lcb->ch_flags |= AVCT_L2C_CFG_CFM_DONE;

                /* if configuration complete */
                if (p_lcb->ch_flags & AVCT_L2C_CFG_IND_DONE)
                {
                    p_lcb->ch_state = AVCT_CH_OPEN;
                    avct_bcb_event(p_lcb, AVCT_LCB_LL_OPEN_EVT, NULL);
                }
            }
            /* else failure */
            else
            {
                /* store result value */
                p_lcb->ch_result = p_cfg->result;

                /* Send L2CAP disconnect req */
                L2CA_DisconnectReq(lcid);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_config_ind_cback
**
** Description      This is the L2CAP config indication callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_config_ind_cback(UINT16 lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tAVCT_BCB       *p_lcb;
    UINT16          max_mtu = GKI_MAX_BUF_SIZE - L2CAP_MIN_OFFSET - BT_HDR_SIZE;

    /* Don't include QoS nor flush timeout in the response since we
       currently always accept these values.  Note: fcr_present is left
       untouched since l2cap negotiates this internally
    */
    p_cfg->flush_to_present = FALSE;
    p_cfg->qos_present = FALSE;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        /* store the mtu in tbl */
        if (p_cfg->mtu_present)
        {
            p_lcb->peer_mtu = p_cfg->mtu;
        }
        else
        {
            p_lcb->peer_mtu = L2CAP_DEFAULT_MTU;
        }

        if (p_lcb->peer_mtu > max_mtu)
        {
            p_lcb->peer_mtu = p_cfg->mtu = max_mtu;

            /* Must tell the peer what the adjusted value is */
            p_cfg->mtu_present = TRUE;
        }
        else    /* Don't include in the response */
            p_cfg->mtu_present = FALSE;
        AVCT_TRACE_DEBUG2 ("avct_l2c_br_config_ind_cback peer_mtu:%d use:%d", p_lcb->peer_mtu, max_mtu);

        if (p_lcb->peer_mtu >= AVCT_MIN_BROWSE_MTU)
            p_cfg->result = L2CAP_CFG_OK;
        else
        {
            p_cfg->result       = L2CAP_CFG_UNACCEPTABLE_PARAMS;
            p_cfg->mtu_present  = TRUE;
            p_cfg->mtu          = AVCT_MIN_BROWSE_MTU;
        }

        /* send L2CAP configure response */
        L2CA_ConfigRsp(lcid, p_cfg);

        if (p_cfg->result != L2CAP_CFG_OK)
        {
            return;
        }

        /* if first config ind */
        if ((p_lcb->ch_flags & AVCT_L2C_CFG_IND_DONE) == 0)
        {        
            /* update flags */
            p_lcb->ch_flags |= AVCT_L2C_CFG_IND_DONE;

            /* if configuration complete */
            if (p_lcb->ch_flags & AVCT_L2C_CFG_CFM_DONE)
            {
                p_lcb->ch_state = AVCT_CH_OPEN;
                avct_bcb_event(p_lcb, AVCT_LCB_LL_OPEN_EVT, NULL);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_disconnect_ind_cback
**
** Description      This is the L2CAP disconnect indication callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_disconnect_ind_cback(UINT16 lcid, BOOLEAN ack_needed)
{
    tAVCT_BCB       *p_lcb;
    UINT16          result = AVCT_RESULT_FAIL;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        if (ack_needed)
        {
            /* send L2CAP disconnect response */
            L2CA_DisconnectRsp(lcid);
        }
  
        avct_bcb_event(p_lcb, AVCT_LCB_LL_CLOSE_EVT, (tAVCT_LCB_EVT *) &result);
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_disconnect_cfm_cback
**
** Description      This is the L2CAP disconnect confirm callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_disconnect_cfm_cback(UINT16 lcid, UINT16 result)
{
    tAVCT_BCB       *p_lcb;
    UINT16          res;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        /* result value may be previously stored */
        res = (p_lcb->ch_result != 0) ? p_lcb->ch_result : result;
        p_lcb->ch_result = 0;

        avct_bcb_event(p_lcb, AVCT_LCB_LL_CLOSE_EVT, (tAVCT_LCB_EVT *) &res);
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_congestion_ind_cback
**
** Description      This is the L2CAP congestion indication callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_congestion_ind_cback(UINT16 lcid, BOOLEAN is_congested)
{
    tAVCT_BCB       *p_lcb;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        avct_bcb_event(p_lcb, AVCT_LCB_LL_CONG_EVT, (tAVCT_LCB_EVT *) &is_congested);
    }
}

/*******************************************************************************
**
** Function         avct_l2c_br_data_ind_cback
**
** Description      This is the L2CAP data indication callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void avct_l2c_br_data_ind_cback(UINT16 lcid, BT_HDR *p_buf)
{
    tAVCT_BCB       *p_lcb;

    /* look up lcb for this channel */
    if ((p_lcb = avct_bcb_by_lcid(lcid)) != NULL)
    {
        avct_bcb_event(p_lcb, AVCT_LCB_LL_MSG_EVT, (tAVCT_LCB_EVT *) &p_buf);
    }
    else /* prevent buffer leak */
        GKI_freebuf(p_buf);
}
#endif /* (AVCT_BROWSE_INCLUDED == TRUE) */


