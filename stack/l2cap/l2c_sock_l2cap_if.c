/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/******************************************************************************
 *
 *    This file contains the callback functions to be registered with core L2CAP
 *
 ******************************************************************************/
#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

#include "gki.h"
#include "l2c_sock_int.h"
#include "l2c_api.h"

/*
** Define Callback functions to be called by L2CAP
*/
static void SOCK_L2C_ConnectInd (BD_ADDR bd_addr, UINT16 lcid, UINT16 psm, UINT8 id);
static void SOCK_L2C_ConnectCnf (UINT16  lcid, UINT16 err);
static void SOCK_L2C_ConfigInd (UINT16 lcid, tL2CAP_CFG_INFO *p_cfg);
static void SOCK_L2C_ConfigCnf (UINT16 lcid, tL2CAP_CFG_INFO *p_cfg);
static void SOCK_L2C_DisconnectInd (UINT16 lcid, BOOLEAN is_clear);
static void SOCK_L2C_QoSViolationInd (BD_ADDR bd_addr);
static void SOCK_L2C_BufDataInd (UINT16 lcid, BT_HDR *p_buf);
static void SOCK_L2C_CongestionStatusInd (UINT16 lcid, BOOLEAN is_congested);
static void SOCK_L2C_TxCompleteInd (UINT16 lcid, UINT16 sdu_sent);

/*******************************************************************************
**
** Function         l2c_sock_if_init
**
** Description      This function is called during the SOCK_L2C startup to
**                  register interface functions with L2CAP for Obex over L2CAP.
**
*******************************************************************************/
UINT16 l2c_sock_if_init (UINT16 psm)
{
    tL2CAP_APPL_INFO *p_l2c = &l2c_sock_mcb.reg_info;

    p_l2c->pL2CA_ConnectInd_Cb       = SOCK_L2C_ConnectInd;
    p_l2c->pL2CA_ConnectCfm_Cb       = SOCK_L2C_ConnectCnf;
    p_l2c->pL2CA_ConnectPnd_Cb       = NULL;
    p_l2c->pL2CA_ConfigInd_Cb        = SOCK_L2C_ConfigInd;
    p_l2c->pL2CA_ConfigCfm_Cb        = SOCK_L2C_ConfigCnf;
    p_l2c->pL2CA_DisconnectInd_Cb    = SOCK_L2C_DisconnectInd;
    p_l2c->pL2CA_DisconnectCfm_Cb    = NULL;
    p_l2c->pL2CA_QoSViolationInd_Cb  = SOCK_L2C_QoSViolationInd;
    p_l2c->pL2CA_DataInd_Cb          = SOCK_L2C_BufDataInd;
    p_l2c->pL2CA_CongestionStatus_Cb = SOCK_L2C_CongestionStatusInd;
    p_l2c->pL2CA_TxComplete_Cb       = SOCK_L2C_TxCompleteInd;

    return L2CA_Register (psm, p_l2c);
}


/*******************************************************************************
**
** Function         SOCK_L2C_ConnectInd
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_ConnectInd received.
*******************************************************************************/
void SOCK_L2C_ConnectInd (BD_ADDR bd_addr, UINT16 lcid, UINT16 psm, UINT8 id)
{
    int i = 0, xx;
    tL2C_SOCK_CB *p_scb = NULL;
    tL2C_SOCK_CB *p_listen_scb = NULL;
    BOOLEAN listen_scb_found = FALSE;
    BOOLEAN free_scb_found = FALSE;

    /* Look through each connection control block to get the listening and free scb  */
    for (xx = 0, p_scb = l2c_sock_mcb.l2c_sock; xx < MAX_L2C_SOCK_CONNECTIONS; xx++, p_scb++)
    {
        if ( (p_scb->in_use == TRUE)  && (p_scb->state == L2C_SOCK_STATE_IDLE) &&
             (p_scb->is_server == TRUE) && (p_scb->psm == psm))
        {
            p_listen_scb = p_scb;
            listen_scb_found = TRUE;
        }
        else if((p_scb->in_use == FALSE) && (free_scb_found == FALSE))
        {
            free_scb_found = TRUE;
        }
    }

    if((listen_scb_found == FALSE) || (free_scb_found == FALSE))
    {
        /* Disconnect because it is an unexpected connection */
        L2CA_DISCONNECT_REQ (lcid);
        return;
    }
    else
    {
        p_listen_scb->lcid = lcid;
        for (i = 0; i < BD_ADDR_LEN; i++)
            p_listen_scb->bd_addr[i] = bd_addr[i];
    }

    L2C_SOCK_TRACE_WARNING("SOCK_L2C_ConnectInd LCID:0x%x", lcid);

    l2c_sock_sm_execute (p_listen_scb, L2C_SOCK_EVT_CONN_IND, &id);
}


/*******************************************************************************
**
** Function         SOCK_L2C_ConnectCnf
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_ConnectCnf received.  Save L2CAP handle and dispatch
**                  event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_ConnectCnf (UINT16 lcid, UINT16 result)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    l2c_sock_sm_execute (p_cb, L2C_SOCK_EVT_CONN_CNF, &result);
}


/*******************************************************************************
**
** Function         SOCK_L2C_ConfigInd
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_ConfigInd received.  Save parameters in the control
**                  block and dispatch event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_ConfigInd (UINT16 lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    l2c_sock_sm_execute (p_cb, L2C_SOCK_EVT_CONF_IND, (void *)p_cfg);
}


/*******************************************************************************
**
** Function         SOCK_L2C_ConfigCnf
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_ConfigCnf received.  Save L2CAP handle and dispatch
**                  event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_ConfigCnf (UINT16 lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    l2c_sock_sm_execute (p_cb, L2C_SOCK_EVT_CONF_CNF, (void *)p_cfg);
}


/*******************************************************************************
**
** Function         SOCK_L2C_QoSViolationInd
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_QoSViolationIndInd received.  Dispatch event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_QoSViolationInd (BD_ADDR bd_addr)
{
}


/*******************************************************************************
**
** Function         SOCK_L2C_DisconnectInd
**
** Description      This is a callback function called by L2CAP when
**                  L2CA_DisconnectInd received.  Dispatch event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_DisconnectInd (UINT16 lcid, BOOLEAN is_conf_needed)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    if (is_conf_needed)
    {
        L2CA_DisconnectRsp (lcid);
    }

    l2c_sock_sm_execute (p_cb, L2C_SOCK_EVT_DISC_IND, NULL);
}


/*******************************************************************************
**
** Function         SOCK_L2C_BufDataInd
**
** Description      This is a callback function called by L2CAP when
**                  data frame is received.  Dispatch event to the FSM.
**
*******************************************************************************/
void SOCK_L2C_BufDataInd (UINT16 lcid, BT_HDR *p_buf)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    l2c_sock_sm_execute (p_cb, L2C_SOCK_EVT_DATA_IN, p_buf);
}

/*******************************************************************************
**
** Function         SOCK_L2C_CongestionStatusInd
**
** Description      This is a callback function called by L2CAP when
**                  data SOCK_L2C L2CAP congestion status changes
**
*******************************************************************************/
void SOCK_L2C_CongestionStatusInd (UINT16 lcid, BOOLEAN is_congested)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    UINT16       event;
    BT_HDR      *p_buf;
    UINT8        status;

    if(p_cb == NULL)
        return;

    p_cb->is_congested = is_congested;
    event = (is_congested) ? L2C_SOCK_CONGESTED : L2C_SOCK_UNCONGESTED;

    p_cb->p_sock_mgmt_cback(p_cb->inx, event);

    L2C_SOCK_TRACE_WARNING ("SOCK_L2C_CongestionStatusInd is_congested %d", is_congested);

    if (!is_congested)
    {
        while ((p_buf = (BT_HDR *)GKI_dequeue (&p_cb->tx_queue)) != NULL)
        {
            L2C_SOCK_TRACE_WARNING ("SOCK_L2C_CongestionStatusInd sending the buffs now");
            status = L2CA_DATA_WRITE (p_cb->lcid, p_buf);

            if (status == L2CAP_DW_CONGESTED)
            {
                p_cb->is_congested = TRUE;
                break;
            }
            else if (status != L2CAP_DW_SUCCESS)
                break;
        }
    }
}

/*******************************************************************************
**
** Function         SOCK_L2C_TxCompleteInd
**
** Description      This is a callback function called by L2CAP when
**                  packets are sent or flushed.
**
*******************************************************************************/
void SOCK_L2C_TxCompleteInd (UINT16 lcid, UINT16 sdu_sent)
{
    tL2C_SOCK_CB *p_cb = l2c_sock_find_scb_by_cid (lcid);

    if(p_cb == NULL)
        return;

    L2C_SOCK_TRACE_DEBUG ("SOCK_L2C_TxCompleteInd ");

    if(sdu_sent == 0xFFFF)
    {
        L2C_SOCK_TRACE_DEBUG ("SOCK_L2C_TxCompleteInd: L2C_SOCK_TX_EMPTY \n");
        p_cb->p_sock_mgmt_cback(p_cb->inx, L2C_SOCK_TX_EMPTY);
    }
}

#endif
