/*****************************************************************************
**
**  Name:           avct_bcb_act.c
**
**  Description:    This module contains action functions of the browsing control
**                  state machine.
**
**  Copyright (c) 2003-2008, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include "data_types.h"
#include "bt_target.h"
#include "avct_api.h"
#include "avct_int.h"
#include "gki.h"


#include "btm_api.h"

#if (AVCT_BROWSE_INCLUDED == TRUE)
/* action function list */
const tAVCT_BCB_ACTION avct_bcb_action[] = {
    avct_bcb_chnl_open,     /* AVCT_LCB_CHNL_OPEN */
    avct_bcb_chnl_disc,     /* AVCT_LCB_CHNL_DISC */
    avct_bcb_send_msg,      /* AVCT_LCB_SEND_MSG */
    avct_bcb_open_ind,      /* AVCT_LCB_OPEN_IND */
    avct_bcb_open_fail,     /* AVCT_LCB_OPEN_FAIL */
    avct_bcb_close_ind,     /* AVCT_LCB_CLOSE_IND */
    avct_bcb_close_cfm,     /* AVCT_LCB_CLOSE_CFM */
    avct_bcb_msg_ind,       /* AVCT_LCB_MSG_IND */
    avct_bcb_cong_ind,      /* AVCT_LCB_CONG_IND */
    avct_bcb_bind_conn,     /* AVCT_LCB_BIND_CONN */
    avct_bcb_bind_fail,     /* AVCT_LCB_BIND_FAIL */
    avct_bcb_unbind_disc,   /* AVCT_LCB_UNBIND_DISC */
    avct_bcb_chk_disc,      /* AVCT_LCB_CHK_DISC */
    avct_bcb_discard_msg,   /* AVCT_LCB_DISCARD_MSG */
    avct_bcb_dealloc,       /* AVCT_LCB_DEALLOC */
    avct_bcb_free_msg_ind   /* AVCT_LCB_FREE_MSG_IND */
};


/*******************************************************************************
**
** Function         avct_bcb_msg_asmbl
**
** Description      Reassemble incoming message.
**                  
**
** Returns          Pointer to reassembled message;  NULL if no message
**                  available.
**
*******************************************************************************/
static BT_HDR *avct_bcb_msg_asmbl(tAVCT_BCB *p_bcb, BT_HDR *p_buf)
{
    UINT8   *p;
    UINT8   pkt_type;
    BT_HDR  *p_ret = NULL;

    /* parse the message header */
    p = (UINT8 *)(p_buf + 1) + p_buf->offset;
    AVCT_PRS_PKT_TYPE(p, pkt_type);

    /* must be single packet - can not fragment */
    if (pkt_type == AVCT_PKT_TYPE_SINGLE)
    {
        p_ret = p_buf;
    }
    else
    {
        GKI_freebuf(p_buf);
        AVCT_TRACE_WARNING1("Pkt type=%d - fragmentation not allowed. drop it", pkt_type);
    }
    return p_ret;
}


/*******************************************************************************
**
** Function         avct_bcb_chnl_open
**
** Description      Open L2CAP channel to peer
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_chnl_open(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    UINT16      result = AVCT_RESULT_FAIL;
    tAVCT_LCB   *p_lcb = avct_lcb_by_bcb(p_bcb);
    tL2CAP_ERTM_INFO ertm_info;

    BTM_SetOutService(p_lcb->peer_addr, BTM_SEC_SERVICE_AVCTP_BROWSE, 0);

    /* Set the FCR options: Browsing channel mandates ERTM */
    ertm_info.preferred_mode  = avct_l2c_br_fcr_opts_def.mode;
    ertm_info.allowed_modes = L2CAP_FCR_CHAN_OPT_ERTM;
    ertm_info.user_rx_pool_id = AVCT_BR_USER_RX_POOL_ID;
    ertm_info.user_tx_pool_id = AVCT_BR_USER_TX_POOL_ID;
    ertm_info.fcr_rx_pool_id = AVCT_BR_FCR_RX_POOL_ID;
    ertm_info.fcr_tx_pool_id = AVCT_BR_FCR_TX_POOL_ID;

    /* call l2cap connect req */
    p_bcb->ch_state = AVCT_CH_CONN;
    if ((p_bcb->ch_lcid = L2CA_ErtmConnectReq(AVCT_BR_PSM, p_lcb->peer_addr, &ertm_info)) == 0)
    {
        /* if connect req failed, send ourselves close event */
        avct_bcb_event(p_bcb, AVCT_LCB_LL_CLOSE_EVT, (tAVCT_LCB_EVT *) &result);
    }
}

/*******************************************************************************
**
** Function         avct_bcb_unbind_disc
**
** Description      call callback with disconnect event.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_unbind_disc(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    p_data->p_ccb->p_bcb = NULL;
    (*p_data->p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_data->p_ccb), AVCT_BROWSE_DISCONN_CFM_EVT, 0, NULL);
}

/*******************************************************************************
**
** Function         avct_bcb_open_ind
**
** Description      Handle an LL_OPEN event.  For each allocated ccb already
**                  bound to this lcb, send a connect event.  For each
**                  unbound ccb with a new PID, bind that ccb to this lcb and
**                  send a connect event.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_open_ind(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB   *p_ccb = &avct_cb.ccb[0];
    tAVCT_CCB   *p_ccb_bind = NULL;
    int         i;
    BOOLEAN     bind = FALSE;
    tAVCT_UL_MSG    ul_msg;

    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        /* if ccb allocated and */
        if (p_ccb->allocated)
        {
            /* if bound to this lcb send connect confirm event */
            if (p_ccb->p_bcb == p_bcb)
            {
                bind = TRUE;
                p_ccb_bind = p_ccb;
                p_ccb->cc.p_ctrl_cback(avct_ccb_to_idx(p_ccb), AVCT_BROWSE_CONN_CFM_EVT,
                                       0, p_ccb->p_lcb->peer_addr);
            }
            /* if unbound acceptor and lcb e */
            else if ((p_ccb->p_bcb == NULL) && (p_ccb->cc.role == AVCT_ACP) &&
                     (p_ccb->p_lcb != NULL))
            {
                /* bind ccb to lcb and send connect ind event */
                bind = TRUE;
                p_ccb_bind = p_ccb;
                p_ccb->p_bcb = p_bcb;
                p_ccb->cc.p_ctrl_cback(avct_ccb_to_idx(p_ccb), AVCT_BROWSE_CONN_IND_EVT,
                                    0, p_ccb->p_lcb->peer_addr);
            }
        }
    }

    /* if no ccbs bound to this lcb, disconnect */
    if (bind == FALSE)
    {
        avct_bcb_event(p_bcb, AVCT_LCB_INT_CLOSE_EVT, p_data);
    }
    else if (p_bcb->p_tx_msg)
    {
        if (p_ccb_bind)
        {
            ul_msg.p_buf = p_bcb->p_tx_msg;
            ul_msg.p_ccb = p_ccb_bind;
            ul_msg.label = (UINT8)(p_bcb->p_tx_msg->layer_specific & 0xFF);
            ul_msg.cr = (UINT8)((p_bcb->p_tx_msg->layer_specific & 0xFF00) >> 8);
            p_bcb->p_tx_msg->layer_specific = AVCT_DATA_BROWSE;
            p_bcb->p_tx_msg = NULL;

            /* send msg event to bcb */
            avct_bcb_event(p_bcb, AVCT_LCB_UL_MSG_EVT, (tAVCT_LCB_EVT *) &ul_msg);
        }
        else
        {
            GKI_freebuf (p_bcb->p_tx_msg);
            p_bcb->p_tx_msg = NULL;
        }

    }
}

/*******************************************************************************
**
** Function         avct_bcb_open_fail
**
** Description      L2CAP channel open attempt failed.  Mark the ccbs
**                  as NULL bcb.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_open_fail(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB           *p_ccb = &avct_cb.ccb[0];
    int                 i;

    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        if (p_ccb->allocated && (p_ccb->p_bcb == p_bcb))
        {
            p_ccb->p_bcb = NULL;
        }
    }
}

/*******************************************************************************
**
** Function         avct_bcb_close_ind
**
** Description      L2CAP channel closed by peer.  Deallocate any initiator
**                  ccbs on this lcb and send disconnect ind event.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_close_ind(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB           *p_ccb = &avct_cb.ccb[0];
    tAVCT_LCB *p_lcb = avct_lcb_by_bcb(p_bcb);
    int                 i;

    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        if (p_ccb->allocated && (p_ccb->p_bcb == p_bcb))
        {
            if (p_ccb->cc.role == AVCT_INT)
            {
                (*p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_ccb), AVCT_BROWSE_DISCONN_CFM_EVT, 0, p_lcb->peer_addr);
            }
            else
            {
                (*p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_ccb), AVCT_BROWSE_DISCONN_IND_EVT, 0, NULL);
            }
            p_ccb->p_bcb = NULL;
        }
    }
}

/*******************************************************************************
**
** Function         avct_bcb_close_cfm
**
** Description      L2CAP channel closed by us.  Deallocate any initiator
**                  ccbs on this lcb and send disconnect ind or cfm event.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_close_cfm(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB           *p_ccb = &avct_cb.ccb[0];
    int                 i;
    UINT8               event = 0;
    BOOLEAN             ch_close = p_bcb->ch_close;           /* Whether BCB initiated channel close */
    tAVCT_CTRL_CBACK    *p_cback;

    p_bcb->ch_close = FALSE;
    p_bcb->allocated = 0;
    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        if (p_ccb->allocated && (p_ccb->p_bcb == p_bcb))
        {
            /* if this ccb initiated close send disconnect cfm otherwise ind */
            if (ch_close)
            {
                event = AVCT_BROWSE_DISCONN_CFM_EVT;
            }
            else
            {
                event = AVCT_BROWSE_DISCONN_IND_EVT;
            }

            p_cback = p_ccb->cc.p_ctrl_cback;
            p_ccb->p_bcb = NULL;
            if (p_ccb->p_lcb == NULL)
                avct_ccb_dealloc(p_ccb, AVCT_NO_EVT, 0, NULL);
            (*p_cback)(avct_ccb_to_idx(p_ccb), event,
                                       p_data->result, NULL);
        }
    }
}

/*******************************************************************************
**
** Function         avct_bcb_bind_conn
**
** Description      Bind ccb to lcb and send connect cfm event.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_bind_conn(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_LCB *p_lcb = avct_lcb_by_bcb(p_bcb);
    p_data->p_ccb->p_bcb = p_bcb;
    (*p_data->p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_data->p_ccb),
                                      AVCT_BROWSE_CONN_CFM_EVT, 0, p_lcb->peer_addr);
}

/*******************************************************************************
**
** Function         avct_bcb_chk_disc
**
** Description      A ccb wants to close; if it is the last ccb on this lcb, 
**                  close channel.  Otherwise just deallocate and call
**                  callback.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_chk_disc(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    AVCT_TRACE_WARNING0("avct_bcb_chk_disc");
    p_bcb->ch_close = avct_bcb_last_ccb(p_bcb, p_data->p_ccb);
    if (p_bcb->ch_close)
    {
        AVCT_TRACE_WARNING0("b closing");
        avct_bcb_event(p_bcb, AVCT_LCB_INT_CLOSE_EVT, p_data);
    }
    else
    {
        AVCT_TRACE_WARNING0("b unbind_disc");
        avct_bcb_unbind_disc(p_bcb, p_data);
    }
}

/*******************************************************************************
**
** Function         avct_bcb_chnl_disc
**
** Description      Disconnect L2CAP channel.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_chnl_disc(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    L2CA_DisconnectReq(p_bcb->ch_lcid);
}

/*******************************************************************************
**
** Function         avct_bcb_bind_fail
**
** Description      Deallocate ccb and call callback with connect event 
**                  with failure result.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_bind_fail(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    p_data->p_ccb->p_bcb = NULL;
    (*p_data->p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_data->p_ccb), AVCT_BROWSE_CONN_CFM_EVT, AVCT_RESULT_FAIL, NULL);
}

/*******************************************************************************
**
** Function         avct_bcb_cong_ind
**
** Description      Handle congestion indication from L2CAP.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_cong_ind(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB           *p_ccb = &avct_cb.ccb[0];
    int                 i;
    UINT8               event;
    tAVCT_LCB           *p_lcb = avct_lcb_by_bcb(p_bcb);

    /* set event */
    event = (p_data->cong) ? AVCT_BROWSE_CONG_IND_EVT : AVCT_BROWSE_UNCONG_IND_EVT;

    /* send event to all ccbs on this lcb */
    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        if (p_ccb->allocated && (p_ccb->p_bcb == p_bcb))
        {
            (*p_ccb->cc.p_ctrl_cback)(avct_ccb_to_idx(p_ccb), event, 0, p_lcb->peer_addr);
        }
    }

}

/*******************************************************************************
**
** Function         avct_bcb_discard_msg
**
** Description      Discard a message sent in from the API.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_discard_msg(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    AVCT_TRACE_WARNING0("save msg in control block");

    if (p_bcb->p_tx_msg)
    {
        GKI_freebuf(p_bcb->p_tx_msg);
        p_bcb->p_tx_msg = NULL;
    }

    /* if control channel is up, save the message and open the browsing channel */
    if (p_data->ul_msg.p_ccb->p_lcb)
    {
        p_bcb->p_tx_msg = p_data->ul_msg.p_buf;

        if (p_bcb->p_tx_msg)
        {
            p_bcb->p_tx_msg->layer_specific = (p_data->ul_msg.cr << 8) + p_data->ul_msg.label;

            /* the channel is closed, opening or closing - open it again */
            AVCT_TRACE_DEBUG2("ch_state: %d, allocated:%d", p_bcb->ch_state, p_bcb->allocated);
            p_bcb->allocated = p_data->ul_msg.p_ccb->p_lcb->allocated;
            AVCT_TRACE_DEBUG2("ch_state: %d, allocated:%d", p_bcb->ch_state, p_bcb->allocated);
            avct_bcb_event(p_bcb, AVCT_LCB_UL_BIND_EVT, (tAVCT_LCB_EVT *) p_data->ul_msg.p_ccb);
        }
    }
    else
        GKI_freebuf(p_data->ul_msg.p_buf);
}

/*******************************************************************************
**
** Function         avct_bcb_send_msg
**
** Description      Build and send an AVCTP message.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_send_msg(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    UINT16          curr_msg_len;
    UINT8           pkt_type = AVCT_PKT_TYPE_SINGLE;
    UINT8           hdr_len;
    BT_HDR          *p_buf;
    UINT8           *p;
        
    /* store msg len */
    curr_msg_len = p_data->ul_msg.p_buf->len;

    /* initialize packet type and other stuff */
    if (curr_msg_len > (p_bcb->peer_mtu - AVCT_HDR_LEN_SINGLE))
    {
        AVCT_TRACE_ERROR3 ("avct_bcb_send_msg msg len (%d) exceeds peer mtu(%d-%d)!!", curr_msg_len, p_bcb->peer_mtu, AVCT_HDR_LEN_SINGLE);
        GKI_freebuf(p_data->ul_msg.p_buf);
        return;
    }

    /* set header len */
    hdr_len = avct_lcb_pkt_type_len[pkt_type];
    p_buf = p_data->ul_msg.p_buf;

    /* set up to build header */
    p_buf->len += hdr_len;
    p_buf->offset -= hdr_len;
    p = (UINT8 *)(p_buf + 1) + p_buf->offset;

    /* build header */
    AVCT_BLD_HDR(p, p_data->ul_msg.label, pkt_type, p_data->ul_msg.cr);
    UINT16_TO_BE_STREAM(p, p_data->ul_msg.p_ccb->cc.pid);

    /* send message to L2CAP */
    L2CA_DataWrite(p_bcb->ch_lcid, p_buf);
    return;
}

/*******************************************************************************
**
** Function         avct_bcb_free_msg_ind
**
** Description      Discard an incoming AVCTP message.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_free_msg_ind(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    if (p_data)
        GKI_freebuf(p_data->p_buf);
    return;
}

/*******************************************************************************
**
** Function         avct_bcb_msg_ind
**
** Description      Handle an incoming AVCTP message.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_bcb_msg_ind(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    UINT8       *p;
    UINT8       label, type, cr_ipid;
    UINT16      pid;
    tAVCT_CCB   *p_ccb;
    BT_HDR      *p_buf;
    tAVCT_LCB   *p_lcb = avct_lcb_by_bcb(p_bcb);

    /* this p_buf is to be reported through p_msg_cback. The layer_specific
     * needs to be set properly to indicate that it is received through
     * broasing channel */
    p_data->p_buf->layer_specific = AVCT_DATA_BROWSE;

    /* reassemble message; if no message available (we received a fragment) return */
    if ((p_data->p_buf = avct_bcb_msg_asmbl(p_bcb, p_data->p_buf)) == NULL)
    {
        return;
    }

    p = (UINT8 *)(p_data->p_buf + 1) + p_data->p_buf->offset;

    /* parse header byte */
    AVCT_PRS_HDR(p, label, type, cr_ipid);

    /* check for invalid cr_ipid */
    if (cr_ipid == AVCT_CR_IPID_INVALID)
    {
        AVCT_TRACE_WARNING1("Invalid cr_ipid", cr_ipid);
        GKI_freebuf(p_data->p_buf);
        return;
    }

    /* parse and lookup PID */
    BE_STREAM_TO_UINT16(pid, p);
    if ((p_ccb = avct_lcb_has_pid(p_lcb, pid)) != NULL)
    {
        /* PID found; send msg up, adjust bt hdr and call msg callback */
        p_data->p_buf->offset += AVCT_HDR_LEN_SINGLE;
        p_data->p_buf->len -= AVCT_HDR_LEN_SINGLE;
        (*p_ccb->cc.p_msg_cback)(avct_ccb_to_idx(p_ccb), label, cr_ipid, p_data->p_buf);
    }
    else
    {
        /* PID not found; drop message */
        AVCT_TRACE_WARNING1("No ccb for PID=%x", pid);
        GKI_freebuf(p_data->p_buf);

        /* if command send reject */
        if (cr_ipid == AVCT_CMD)
        {
            if ((p_buf = (BT_HDR *) GKI_getpoolbuf(AVCT_CMD_POOL_ID)) != NULL)
            {
                p_buf->len = AVCT_HDR_LEN_SINGLE;
                p_buf->offset = AVCT_MSG_OFFSET - AVCT_HDR_LEN_SINGLE;
                p = (UINT8 *)(p_buf + 1) + p_buf->offset;
                AVCT_BLD_HDR(p, label, AVCT_PKT_TYPE_SINGLE, AVCT_REJ);
                UINT16_TO_BE_STREAM(p, pid);
                L2CA_DataWrite(p_bcb->ch_lcid, p_buf);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avct_bcb_dealloc
**
** Description      Deallocate a link control block.
**                  
**
** Returns          void.
**
*******************************************************************************/
void avct_bcb_dealloc(tAVCT_BCB *p_bcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_CCB   *p_ccb = &avct_cb.ccb[0];
    tAVCT_CCB   *p_ccb_found = NULL;
    BOOLEAN     found = FALSE;
    int         i;

    AVCT_TRACE_DEBUG1("avct_bcb_dealloc %d", p_bcb->allocated);

    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        /* if ccb allocated and */
        if (p_ccb->allocated)
        {
            if (p_ccb->p_bcb == p_bcb)
            {
                p_ccb_found = p_ccb;
                p_ccb->p_bcb = NULL;
                AVCT_TRACE_DEBUG1("avct_bcb_dealloc used by ccb: %d", i);
                found = TRUE;
                break;
            }
        }
    }

    /* the browsing channel is down. Check if we have pending messages */
    if (p_bcb->p_tx_msg != NULL)
    {
        GKI_freebuf(p_bcb->p_tx_msg);
    }
    memset(p_bcb, 0, sizeof(tAVCT_BCB));
}

/*******************************************************************************
**
** Function         avct_close_bcb
**
** Description      this function is called right before LCB disconnects.
**                  
**
** Returns          Nothing.
**
*******************************************************************************/
void avct_close_bcb(tAVCT_LCB *p_lcb, tAVCT_LCB_EVT *p_data)
{
    tAVCT_BCB *p_bcb = avct_bcb_by_lcb(p_lcb);
    if (p_bcb->allocated)
    {
        avct_bcb_event(p_bcb, AVCT_LCB_UL_UNBIND_EVT, p_data);
    }
}


/*******************************************************************************
**
** Function         avct_lcb_by_bcb
**
** Description      This lookup function finds the lcb for a bcb.
**
** Returns          pointer to the lcb.
**
*******************************************************************************/
tAVCT_LCB *avct_lcb_by_bcb(tAVCT_BCB *p_bcb)
{
    return &avct_cb.lcb[p_bcb->allocated - 1];
}

/*******************************************************************************
**
** Function         avct_bcb_by_lcb
**
** Description      This lookup function finds the bcb for a lcb.
**
** Returns          pointer to the lcb.
**
*******************************************************************************/
tAVCT_BCB *avct_bcb_by_lcb(tAVCT_LCB *p_lcb)
{
    return &avct_cb.bcb[p_lcb->allocated - 1];
}

/*******************************************************************************
**
** Function         avct_bcb_last_ccb
**
** Description      See if given ccb is only one on the bcb.
**                  
**
** Returns          0, if ccb is last,  (ccb index + 1) otherwise.
**
*******************************************************************************/
UINT8 avct_bcb_last_ccb(tAVCT_BCB *p_bcb, tAVCT_CCB *p_ccb_last)
{
    tAVCT_CCB   *p_ccb = &avct_cb.ccb[0];
    int         i;
    UINT8       idx = 0;

    for (i = 0; i < AVCT_NUM_CONN; i++, p_ccb++)
    {
        if (p_ccb->allocated && (p_ccb->p_bcb == p_bcb))
        {
            if (p_ccb != p_ccb_last)
                return 0;
            idx = (UINT8)(i + 1);
        }
    }
    return idx;
}

/*******************************************************************************
**
** Function         avct_bcb_by_lcid
**
** Description      Find the BCB associated with the L2CAP LCID
**                  
**
** Returns          pointer to the lcb, or NULL if none found.
**
*******************************************************************************/
tAVCT_BCB *avct_bcb_by_lcid(UINT16 lcid)
{
    tAVCT_BCB   *p_bcb = &avct_cb.bcb[0];
    int         i;

    for (i = 0; i < AVCT_NUM_LINKS; i++, p_bcb++)
    {
        if (p_bcb->allocated && (p_bcb->ch_lcid == lcid))
        {
            break;
        }
    }
    
    if (i == AVCT_NUM_LINKS)
    {
        p_bcb = NULL;
        /* out of lcbs */
        AVCT_TRACE_WARNING1("No bcb for lcid %x", lcid);
    }

    return p_bcb;
}
#endif /* (AVCT_BROWSE_INCLUDED == TRUE) */
