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
 *   This file contains state machine and action routines for L2cap Socket
 *
 ******************************************************************************/

#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bt_types.h"
#include "l2c_sock_api.h"
#include "l2c_sock_int.h"

tL2C_SOCK_MCB l2c_sock_mcb;


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static void l2c_sock_sm_state_idle (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data);
static void l2c_sock_sm_state_wait_conn_cnf (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data);
static void l2c_sock_sm_state_configure (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data);
static void l2c_sock_sm_state_connected (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data);
static void l2c_sock_sm_state_wait_disc_cnf (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data);

static void l2c_sock_conf_ind (tL2C_SOCK_CB *p_l2c_cb, tL2CAP_CFG_INFO *p_cfg);
static void l2c_sock_conf_cnf (tL2C_SOCK_CB *p_l2c_cb, tL2CAP_CFG_INFO *p_cfg);



/*******************************************************************************
 **
 ** Function         l2c_sock_sm_execute
 **
 ** Description      This function sends BTA layer and Core L2cap events through
 **                  the state machine.
 **
 ** Returns          void
 **
 *******************************************************************************/
void l2c_sock_sm_execute (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    switch (p_l2c_cb->state)
    {
        case L2C_SOCK_STATE_IDLE:
            l2c_sock_sm_state_idle (p_l2c_cb, event, p_data);
            break;

        case L2C_SOCK_STATE_WAIT_CONN_CNF:
            l2c_sock_sm_state_wait_conn_cnf (p_l2c_cb, event, p_data);
            break;

        case L2C_SOCK_STATE_CONFIGURE:
            l2c_sock_sm_state_configure (p_l2c_cb, event, p_data);
            break;

        case L2C_SOCK_STATE_CONNECTED:
            l2c_sock_sm_state_connected (p_l2c_cb, event, p_data);
            break;

        case L2C_SOCK_STATE_WAIT_DISC_CNF:
            l2c_sock_sm_state_wait_disc_cnf (p_l2c_cb, event, p_data);
            break;

    }
}


/*******************************************************************************
 **
 ** Function         l2c_sock_sm_state_idle
 **
 ** Description      This function handles events when the L2cap socket is in
 **                  IDLE state.
 ** Returns          void
 **
 *******************************************************************************/
static void l2c_sock_sm_state_idle (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    L2C_SOCK_TRACE_EVENT ("l2c_sock_sm_state_idle - evt:%d", event);

    switch (event)
    {
        case L2C_SOCK_EVT_START_REQ:
            /* send the L2cap connect request */
            if ((p_l2c_cb->lcid = L2CA_ErtmConnectReq (p_l2c_cb->psm, p_l2c_cb->bd_addr,
                                                          &p_l2c_cb->ertm_info)) == 0)
            {
                /* Intimate the upper layer if there is a callback */
                if (p_l2c_cb->p_sock_mgmt_cback)
                    (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);
                l2c_sock_release_scb(p_l2c_cb);
                return;
            }
            p_l2c_cb->state = L2C_SOCK_STATE_WAIT_CONN_CNF;
            break;
        case L2C_SOCK_EVT_START_RSP:
        case L2C_SOCK_EVT_CONN_CNF:
        case L2C_SOCK_EVT_CONF_IND:
        case L2C_SOCK_EVT_CONF_CNF:
        case L2C_SOCK_EVT_DISC_IND:
            L2C_SOCK_TRACE_ERROR (" error state %d event %d", p_l2c_cb->state, event);
            break;
        case L2C_SOCK_EVT_CONN_IND:
            {
                UINT8 id = *((UINT8 *)p_data);
                /* Transition to the next appropriate state, waiting for config setup. */
                p_l2c_cb->state = L2C_SOCK_STATE_CONFIGURE;

                /* Send response to the L2CAP layer. */
                L2CA_ErtmConnectRsp (p_l2c_cb->bd_addr, id, p_l2c_cb->lcid, L2CAP_CONN_OK,
                        L2CAP_CONN_OK, &p_l2c_cb->ertm_info);

                /* Send a Configuration Request. */
                L2CA_CONFIG_REQ (p_l2c_cb->lcid, &p_l2c_cb->cfg);
            }
            break;

    }
}


/*******************************************************************************
 **
 ** Function         l2c_sock_sm_state_wait_conn_cnf
 **
 ** Description      This function handles events when the L2C socket is
 **                  waiting for Connection Confirm from L2CAP.
 **
 ** Returns          void
 **
 *******************************************************************************/
static void l2c_sock_sm_state_wait_conn_cnf (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    L2C_SOCK_TRACE_EVENT ("l2c_sock_sm_state_wait_conn_cnf - evt:%d", event);
    switch (event)
    {
        case L2C_SOCK_EVT_START_REQ:
            L2C_SOCK_TRACE_ERROR (" error state %d event %d", p_l2c_cb->state, event);
            break;
        case L2C_SOCK_EVT_CONF_IND:
            /* move to the state connected if local cfg sent and remote cfg is received */
            l2c_sock_conf_ind (p_l2c_cb, (tL2CAP_CFG_INFO *)p_data);
            break;
        case L2C_SOCK_EVT_CONN_CNF:
            {
                UINT16 result = *((UINT16 *)p_data);
                /* send the config request and make the state chnage */
                if (result == L2CAP_CONN_OK)
                {
                    p_l2c_cb->state = L2C_SOCK_STATE_CONFIGURE;
                    L2CA_CONFIG_REQ (p_l2c_cb->lcid, &p_l2c_cb->cfg);
                }
                else
                {
                    /* Intimate the upper layer if there is a callback */
                    if (p_l2c_cb->p_sock_mgmt_cback)
                        (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);
                    l2c_sock_release_scb (p_l2c_cb);
                }
            }
            break;
        case L2C_SOCK_EVT_DISC_IND:
            p_l2c_cb->state = L2C_SOCK_STATE_IDLE;
            /* Intimate the upper layer if there is a callback */
            if (p_l2c_cb->p_sock_mgmt_cback)
                (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);
            l2c_sock_release_scb(p_l2c_cb);
            break;
        case L2C_SOCK_EVT_CLOSE_REQ:
            /* send the L2CA_DISCONNECT_REQ and move the state to L2C_SOCK_STATE_WAIT_DISC_CNF */
            p_l2c_cb->state = L2C_SOCK_STATE_WAIT_DISC_CNF;
            L2CA_DISCONNECT_REQ (p_l2c_cb->lcid);
            l2c_sock_release_scb (p_l2c_cb);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         l2c_sock_sm_state_configure
 **
 ** Description      This function handles events when the L2C socket in the
 **                  configuration state.
 **
 ** Returns          void
 **
 *******************************************************************************/
static void l2c_sock_sm_state_configure (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    L2C_SOCK_TRACE_EVENT ("l2c_sock_sm_state_configure - evt:%d", event);
    switch (event)
    {
        case L2C_SOCK_EVT_START_REQ:
        case L2C_SOCK_EVT_CONN_CNF:
            L2C_SOCK_TRACE_ERROR (" error state %d event %d", p_l2c_cb->state, event);
            break;
        case L2C_SOCK_EVT_CONF_IND:
            /* move to the state connected if local cfg sent and remote cfg is received */
            l2c_sock_conf_ind (p_l2c_cb, (tL2CAP_CFG_INFO *)p_data);
            break;
        case L2C_SOCK_EVT_CONF_CNF:
            /* move to the state connected if local cfg sent and remote cfg is received */
            l2c_sock_conf_cnf (p_l2c_cb, (tL2CAP_CFG_INFO *)p_data);
            break;
        case L2C_SOCK_EVT_DISC_IND:
            p_l2c_cb->state = L2C_SOCK_STATE_IDLE;
            /* Intimate the upper layer if there is a callback */
            if (p_l2c_cb->p_sock_mgmt_cback)
                (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);
            l2c_sock_release_scb(p_l2c_cb);
            break;
        case L2C_SOCK_EVT_CLOSE_REQ:
            /* send the L2CA_DISCONNECT_REQ and move the state to L2C_SOCK_STATE_WAIT_DISC_CNF */
            p_l2c_cb->state = L2C_SOCK_STATE_WAIT_DISC_CNF;
            L2CA_DISCONNECT_REQ (p_l2c_cb->lcid);
            l2c_sock_release_scb (p_l2c_cb);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         l2c_sock_sm_state_connected
 **
 ** Description      This function handles events when the L2C socket is
 **                  in the CONNECTED state
 **
 ** Returns          void
 **
 *******************************************************************************/
static void l2c_sock_sm_state_connected (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    L2C_SOCK_TRACE_EVENT ("l2c_sock_sm_state_connected - evt:%d", event);

    switch (event)
    {
        case L2C_SOCK_EVT_CLOSE_REQ:
            /* send the L2CA_DISCONNECT_REQ and move the state to L2C_SOCK_STATE_WAIT_DISC_CNF */
            p_l2c_cb->state = L2C_SOCK_STATE_WAIT_DISC_CNF;
            L2CA_DISCONNECT_REQ (p_l2c_cb->lcid);
            l2c_sock_release_scb (p_l2c_cb);
            break;
        case L2C_SOCK_EVT_DISC_IND:
            p_l2c_cb->state = L2C_SOCK_STATE_IDLE;
            /* Intimate the upper layer if there is a callback */
            if (p_l2c_cb->p_sock_mgmt_cback)
                (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);
            l2c_sock_release_scb(p_l2c_cb);
            break;
        case L2C_SOCK_EVT_DATA_IN:
            /* incoming data from lower L2cap layers so sent this data to app layer */
            if (p_l2c_cb->p_l2c_sock_data_co_cback)
            {
                p_l2c_cb->p_l2c_sock_data_co_cback (p_l2c_cb->inx, (UINT8 *) p_data, -1,
                            DATA_CO_CALLBACK_TYPE_INCOMING);
            }
            break;
    }
}


/*******************************************************************************
 **
 ** Function         l2c_sock_sm_state_wait_disc_cnf
 **
 ** Description      This function handles events when the L2C socket sent
 **                  DISC and is waiting for disconnection confirmation.
 **
 ** Returns          void
 **
 *******************************************************************************/
static void l2c_sock_sm_state_wait_disc_cnf (tL2C_SOCK_CB *p_l2c_cb, UINT16 event, void *p_data)
{
    BT_HDR *p_buf;

    L2C_SOCK_TRACE_EVENT ("l2c_sock_sm_state_wait_disc_cnf - evt:%d", event);
    switch (event)
    {
        case L2C_SOCK_EVT_DISC_CNF:
            p_l2c_cb->p_sock_mgmt_cback(p_l2c_cb->inx, L2C_SOCK_CLOSED);
            p_l2c_cb->state = L2C_SOCK_STATE_IDLE;
            /* clean up the socket data */
            break;
        case L2C_SOCK_EVT_DISC_IND:
        case L2C_SOCK_EVT_START_REQ:
        case L2C_SOCK_EVT_CLOSE_REQ:
            break;
    }
}

/*******************************************************************************
 **
 ** Function         l2c_sock_conf_cnf
 **
 ** Description      This function handles L2CA_ConfigCnf message from the
 **                  L2CAP.  If result is not success tell upper layer that
 **                  start has not been accepted.
 **
 *******************************************************************************/
static void l2c_sock_conf_cnf (tL2C_SOCK_CB *p_l2c_cb, tL2CAP_CFG_INFO *p_cfg)
{
    L2C_SOCK_TRACE_EVENT ("l2c_sock_conf_cnf p_cfg:%08x res:%d ", p_cfg, (p_cfg) ? p_cfg->result : 0);

    if (p_cfg && p_cfg->result == L2CAP_CFG_OK)
    {
        p_l2c_cb->local_cfg_sent = TRUE;
        if ((p_l2c_cb->state == L2C_SOCK_STATE_CONFIGURE) && p_l2c_cb->peer_cfg_rcvd)
        {
            p_l2c_cb->state = L2C_SOCK_STATE_CONNECTED;
            p_l2c_cb->p_sock_mgmt_cback(p_l2c_cb->inx, L2C_SOCK_SUCCESS);
        }

        if (p_l2c_cb->cfg.fcr_present)
            p_l2c_cb->cfg.fcr.mode = p_cfg->fcr.mode;
        else
            p_l2c_cb->cfg.fcr.mode = L2CAP_FCR_BASIC_MODE;
    }
    else
    {
        if (p_l2c_cb->p_sock_mgmt_cback)
            (*p_l2c_cb->p_sock_mgmt_cback) (p_l2c_cb->inx, L2C_SOCK_UNKNOWN_ERROR);

        l2c_sock_release_scb (p_l2c_cb);
    }
}


/*******************************************************************************
 **
 ** Function         l2c_sock_conf_ind
 **
 ** Description      This function handles L2CA_ConfigInd message from the
 **                  L2CAP.  Send the L2CA_ConfigRsp message.
 **
 *******************************************************************************/
static void l2c_sock_conf_ind (tL2C_SOCK_CB *p_l2c_cb, tL2CAP_CFG_INFO *p_cfg)
{
    UINT16      local_mtu_size;

    if (p_l2c_cb->cfg.fcr.mode == L2CAP_FCR_ERTM_MODE)
    {
        local_mtu_size = GKI_get_pool_bufsize (p_l2c_cb->ertm_info.user_tx_pool_id)
            - sizeof(BT_HDR) - L2CAP_MIN_OFFSET;
    }
    else
        local_mtu_size = L2CAP_MTU_SIZE;

    if ((!p_cfg->mtu_present)||(p_cfg->mtu > local_mtu_size))
    {
        p_l2c_cb->rem_mtu_size = local_mtu_size;
    }
    else
        p_l2c_cb->rem_mtu_size = p_cfg->mtu;

    /* For now, always accept configuration from the other side */
    p_cfg->flush_to_present = FALSE;
    p_cfg->mtu_present      = FALSE;
    p_cfg->result           = L2CAP_CFG_OK;
    p_cfg->fcs_present      = FALSE;

    L2CA_CONFIG_RSP (p_l2c_cb->lcid, p_cfg);

    p_l2c_cb->peer_cfg_rcvd = TRUE;
    if ((p_l2c_cb->state == L2C_SOCK_STATE_CONFIGURE) && p_l2c_cb->local_cfg_sent)
    {
        p_l2c_cb->p_sock_mgmt_cback(p_l2c_cb->inx, L2C_SOCK_SUCCESS);
        p_l2c_cb->state = L2C_SOCK_STATE_CONNECTED;
    }
}

#endif
