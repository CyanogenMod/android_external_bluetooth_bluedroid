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
 *    This file contains L2CAP socket interface functions to the BTA layer
 *
 ******************************************************************************/

#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

#include <string.h>
#include "btm_api.h"
#include "l2c_sock_api.h"
#include "l2c_sock_int.h"

/* Control channel eL2CAP default options */
tL2CAP_FCR_OPTS ertm_fcr_opts_def = {
    L2CAP_FCR_ERTM_MODE,
    OBX_OVER_L2C_TX_WINDOW_SIZE, /* Tx window size */
    OBX_OVER_L2C_MAX_TX_B4_DISCNT, /* Maximum transmissions before disconnecting */
    OBX_OVER_L2C_RETX_TOUT, /* Retransmission timeout (2 secs) */
    OBX_OVER_L2C_MONITOR_TOUT, /* Monitor timeout (12 secs) */
    OBX_OVER_L2C_MPS_SIZE  /* MPS segment size */
};


/*******************************************************************************
**
** Function         L2C_SOCK_Init
**
** Description      This function is called to initialize L2C SOCK layer of
**                  core stack
**
*******************************************************************************/
void L2C_SOCK_Init (void)
{
    memset (&l2c_sock_mcb, 0, sizeof (tL2C_SOCK_MCB));

    /* register callbacks with core L2CAP for OPP PSM */
    if(l2c_sock_if_init(BT_PSM_OPP_1_2) == 0)
    {
        L2C_SOCK_TRACE_ERROR("%s:l2c_sock_if_init Failed for PSM %d", __FUNCTION__,
                                 BT_PSM_OPP_1_2);
    }
}

/*******************************************************************************
**
** Function         SOCK_L2C_CreateConnection
**
** Description      SOCK_L2C_CreateConnection function is used from the application
**                  to establish direct L2cap onnection to the peer device,
**                  or to accept a l2cap connection from the peer device
**                  for the given PSM.
**
** Parameters:      psm          - PSM of the remote device to connect or
**                                 PSM of local device for server
**
**                  is_server    - TRUE if requesting application is a server
**
**                  bd_addr      - BD_ADDR of the peer (client)
**
**                  p_handle     - OUT pointer to the handle.
**
**                  p_l2c_sock_mgmt_cback
**                               - pointer to callback function to receive
**                                 connection up/down events.
**
*******************************************************************************/
int SOCK_L2C_CreateConnection (UINT16 psm, BOOLEAN is_server, BD_ADDR bd_addr,
                               UINT16 *p_handle, tL2C_SOCK_CBACK *p_sock_mgmt_cback)
{
    int        i;
    UINT8      dlci;
    tL2C_SOCK_CB *p_scb;
    UINT16     rfcomm_mtu;

    L2C_SOCK_TRACE_API ("SOCK_L2C_CreateConnection()  BDA: %02x-%02x-%02x-%02x-%02x-%02x",
                       bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    *p_handle = 0;

    /* allocate the socket control block for client and server */
    p_scb = l2c_sock_allocate_scb();
    if( p_scb == NULL)
    {
        L2C_SOCK_TRACE_ERROR ("SOCK_L2C_CreateConnection Not able to allocate SCB");
        return L2C_SOCK_INVALID_HANDLE;
    }

    /* Register the PSM with L2CAP incase if it's not already registered */
    /* PSM check would have done in BTA layer */
    if(psm != BT_PSM_OPP_1_2)
    {
        if(l2c_sock_if_init(psm) == 0)
        {
            L2C_SOCK_TRACE_ERROR("%s:l2c_sock_if_init Failed for PSM %d", __FUNCTION__,
                                     psm);
            l2c_sock_release_scb(p_scb);
            return L2C_SOCK_INVALID_HANDLE;
        }
    }

    *p_handle = p_scb->inx;

    /* initialize all params */
    p_scb->state        = L2C_SOCK_STATE_IDLE;
    p_scb->in_use       = TRUE;
    p_scb->is_server    = is_server;
    p_scb->psm          = psm;
    p_scb->p_sock_mgmt_cback = p_sock_mgmt_cback;
    for (i = 0; i < BD_ADDR_LEN; i++)
        p_scb->bd_addr[i] = bd_addr[i];

    /* reset the cfg and initialize cfg */
    memset(&p_scb->cfg, 0, sizeof(tL2CAP_CFG_INFO));

    // initialize FCR, ERTM variables */
    p_scb->cfg.mtu_present = TRUE;
    p_scb->cfg.mtu = OBX_OVER_L2C_MAX_MTU;
    p_scb->cfg.flush_to_present = TRUE;
    p_scb->cfg.flush_to = 0xffff;

    p_scb->cfg.fcr_present = TRUE;
    p_scb->cfg.fcs = 0;
    p_scb->cfg.fcs_present = 1;
    p_scb->cfg.fcr = ertm_fcr_opts_def;

    /* optional FCR channel modes */
    p_scb->ertm_info.allowed_modes     = L2CAP_FCR_CHAN_OPT_ERTM;

    /* Fill in eL2CAP parameter data */
    p_scb->ertm_info.preferred_mode = L2CAP_FCR_ERTM_MODE;
    p_scb->ertm_info.user_rx_pool_id = GKI_POOL_ID_10;
    p_scb->ertm_info.user_tx_pool_id = GKI_POOL_ID_10;
    p_scb->ertm_info.fcr_rx_pool_id = GKI_POOL_ID_10;
    p_scb->ertm_info.fcr_tx_pool_id = GKI_POOL_ID_10;

    /* If this is not initiator of the connection, return success otherwise
        initiate a start req */
    if (p_scb->is_server)
    {
        return (L2C_SOCK_SUCCESS);
    }
    else
    {
        l2c_sock_sm_execute (p_scb, L2C_SOCK_EVT_START_REQ, NULL);
        return L2C_SOCK_SUCCESS;
    }
}

/*******************************************************************************
**
** Function         SOCK_L2C_RemoveConnection
**
** Description      This function is called to close the specified connection.
**
** Parameters:      handle - Handle returned in the SOCK_L2C_RemoveConnection
**
*******************************************************************************/
int SOCK_L2C_RemoveConnection (UINT16 sock_handle)
{
    tL2C_SOCK_CB *p_scb;

    /* Check if sock_handle is valid and with in the range */
    if (sock_handle >= MAX_L2C_SOCK_CONNECTIONS)
    {
        return (L2C_SOCK_INVALID_HANDLE);
    }

    p_scb = l2c_sock_find_scb_by_handle(sock_handle);

    if(p_scb)
    {
        if (p_scb->state == L2C_SOCK_STATE_IDLE)
        {
            /* in this case need not to send the close req */
            l2c_sock_release_scb(p_scb);
        }
        else
        {
            l2c_sock_sm_execute (p_scb, L2C_SOCK_EVT_CLOSE_REQ, NULL);
        }
        return L2C_SOCK_SUCCESS;
    }

    return (L2C_SOCK_INVALID_HANDLE);
}

/*******************************************************************************
**
** Function         SOCK_L2C_ConnGetRemoteAddr
**
** Description      This function is called to get the remote BD address
**                  of a connection.
**
** Parameters:      handle      - Handle of the connection returned by
**                                SOCK_L2C_CreateConnection
**
*******************************************************************************/
UINT8 *SOCK_L2C_ConnGetRemoteAddr (UINT16 sock_handle)
{

    tL2C_SOCK_CB *p_scb;

    L2C_SOCK_TRACE_DEBUG("SOCK_L2C_ConnGetRemoteAddr sock_handle = %d", sock_handle);

    /* Check if sock_handle is valid and with in the range */
    if (sock_handle >= MAX_L2C_SOCK_CONNECTIONS)
    {
        return NULL;
    }

    p_scb = l2c_sock_find_scb_by_handle(sock_handle);

    if(p_scb)
    {
        L2C_SOCK_TRACE_DEBUG("BDA :0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n", \
                         p_scb->bd_addr[0],p_scb->bd_addr[1],p_scb->bd_addr[2],
                         p_scb->bd_addr[3],p_scb->bd_addr[4],p_scb->bd_addr[5]);
        return (p_scb->bd_addr);
    }
    else
    {
        return NULL;
    }
}

/*******************************************************************************
**
** Function         SOCK_L2C_SetDataCallback
**
** Description      This function is when a data packet is received
**
** Parameters:      handle     - Handle returned in the SOCK_L2C_CreateConnection
**
**                  p_l2c_sock_cb - address of the callback function which should
**                                  be called from the L2C Socket when data packet
**                                  is received.
**
**
*******************************************************************************/
int SOCK_L2C_SetDataCallback (UINT16 sock_handle, tL2C_SOCK_DATA_CBACK *p_l2c_sock_cb)
{
    tL2C_SOCK_CB *p_scb = l2c_sock_find_scb_by_handle(sock_handle);

    if( (!p_scb) || (sock_handle >= MAX_L2C_SOCK_CONNECTIONS))
    {
        return L2C_SOCK_INVALID_HANDLE;
    }

    p_scb->p_l2c_sock_data_co_cback = p_l2c_sock_cb;

    return (L2C_SOCK_SUCCESS);
}

/*******************************************************************************
**
** Function         SOCK_L2C_WriteData
**
** Description      This function is used to write data to l2cap socket which
**                  will write the data to the L2cap directly.
**
** Parameters:      handle     - Handle returned in the SOCK_L2C_CreateConnection
**                  p_len      - Byte count returned
**
*******************************************************************************/
int SOCK_L2C_WriteData (UINT16 sock_handle, int* p_len)
{
    BT_HDR     *p_buf;
    UINT16     length;
    int available = 0;

    tL2C_SOCK_CB *p_scb = l2c_sock_find_scb_by_handle(sock_handle);

    if( (!p_scb) || (sock_handle >= MAX_L2C_SOCK_CONNECTIONS))
    {
        return L2C_SOCK_INVALID_HANDLE;
    }
    else if (p_scb->state != L2C_SOCK_STATE_CONNECTED)
    {
        return (L2C_SOCK_NOT_CONNECTED);
    }

    /* first get the available outgoing size */
    if(p_scb->p_l2c_sock_data_co_cback(sock_handle, (UINT8*)&available, sizeof(available),
                                L2C_SOCK_DATA_CBACK_TYPE_OUTGOING_SIZE) == FALSE)
    {
        L2C_SOCK_TRACE_ERROR (" L2C_SOCK_DATA_CBACK_TYPE_OUTGOING_SIZE: failed, available:%d",
                                                    available);
        return (L2C_SOCK_UNKNOWN_ERROR);
    }

    /* If No data available return success */
    if(available == 0)
    {
        return L2C_SOCK_SUCCESS;
    }

    /* Length for each buffer is the smaller of GKI buffer, peer MTU, or available */

    while (available)
    {
        length = GKI_BUF10_SIZE - (UINT16)(sizeof(BT_HDR)) - L2CAP_MIN_OFFSET - 4;

        if ((p_buf = (BT_HDR *)GKI_getpoolbuf (p_scb->ertm_info.user_tx_pool_id)) == NULL)
        {
            L2C_SOCK_TRACE_ERROR ("SOCK_L2C_WriteData Error in GKI_getpoolbuf\n");
            return (L2C_SOCK_CONGESTED);
        }

        if (p_scb->rem_mtu_size < length)
            length = p_scb->rem_mtu_size;
        if (available < (int)length)
            length = (UINT16)available;

        p_buf->offset = L2CAP_MIN_OFFSET;
        p_buf->len = length;
        p_buf->event = BT_EVT_TO_BTU_SP_DATA;

        if(p_scb->p_l2c_sock_data_co_cback(sock_handle, (UINT8 *)(p_buf + 1) + p_buf->offset, length,
                                      L2C_SOCK_DATA_CBACK_TYPE_OUTGOING) == FALSE)
        {
            L2C_SOCK_TRACE_ERROR ("SOCK_L2C_WriteData: err while receiving the data from APP\n");
            return (L2C_SOCK_UNKNOWN_ERROR);
        }

        *p_len  += length;
        available -= (int) length;

        GKI_enqueue (&p_scb->tx_queue, p_buf);
    }

    if (p_scb->is_congested)
    {
        L2C_SOCK_TRACE_EVENT ("SOCK_L2C_WriteData: Already congested \n");
        return (L2C_SOCK_SUCCESS);
    }

    while ((p_buf = (BT_HDR *)GKI_dequeue (&p_scb->tx_queue)) != NULL)
    {
        UINT8 status = L2CA_DATA_WRITE (p_scb->lcid, p_buf);

        if (status == L2CAP_DW_CONGESTED)
        {
            L2C_SOCK_TRACE_EVENT ("SOCK_L2C_WriteData: L2CAP_DW_CONGESTED \n");
            p_scb->is_congested = TRUE;
            break;
        }
        else if (status != L2CAP_DW_SUCCESS)
            return (L2C_SOCK_UNKNOWN_ERROR);
    }

    return L2C_SOCK_SUCCESS;
}

/*******************************************************************************
**
** Function         l2c_sock_find_scb_by_cid
**
** Description      This function searches the SCB table for an entry with the
**                  passed CID.
**
** Returns          the SCB address, or NULL if not found.
**
*******************************************************************************/
tL2C_SOCK_CB *l2c_sock_find_scb_by_cid (UINT16 cid)
{
    UINT16       xx;
    tL2C_SOCK_CB     *p_scb;

    /* Look through each socket control block */
    for (xx = 0, p_scb = l2c_sock_mcb.l2c_sock; xx < MAX_L2C_SOCK_CONNECTIONS;
                                                                xx++, p_scb++)
    {
        if ((p_scb->in_use == TRUE) && (p_scb->lcid == cid))
        {
            return (p_scb);
        }
    }

    /* If here, not found */
    return (NULL);
}


/*******************************************************************************
**
** Function         l2c_sock_find_scb_by_handle
**
** Description      This function searches the SCB table for an entry with the
**                  passed handle.
**
** Returns          the SCB address, or NULL if not found.
**
*******************************************************************************/
tL2C_SOCK_CB *l2c_sock_find_scb_by_handle (UINT16 handle)
{
    tL2C_SOCK_CB     *p_scb  = NULL;
    int xx;

    /* Check that handle is valid */
    if (handle < MAX_L2C_SOCK_CONNECTIONS)
    {
        for (xx = 0, p_scb = l2c_sock_mcb.l2c_sock; xx < MAX_L2C_SOCK_CONNECTIONS;
                                                                   xx++, p_scb++)
        {
            if ( (p_scb->in_use == TRUE) && (p_scb->inx == handle))
            {
                L2C_SOCK_TRACE_DEBUG ("l2c_sock_find_scb_by_handle: found %p\n", p_scb);
                return (p_scb);
            }
        }
    }

    /* If here, handle points to invalid connection */
    return (NULL);
}

/*******************************************************************************
**
** Function         l2c_sock_allocate_scb
**
** Description      This function allocates a new SCB.
**
** Returns          SCB address, or NULL if none available.
**
*******************************************************************************/
tL2C_SOCK_CB *l2c_sock_allocate_scb (void)
{
    UINT16         xx;
    tL2C_SOCK_CB  *p_scb;

    /* Look through each socket control block for a free one */
    for (xx = 0, p_scb = l2c_sock_mcb.l2c_sock; xx < MAX_L2C_SOCK_CONNECTIONS;
                                                                xx++, p_scb++)
    {
        if (p_scb->in_use == FALSE)
        {
            memset (p_scb, 0, sizeof (tL2C_SOCK_CB));

            p_scb->inx   = xx;
            p_scb->in_use   = TRUE;
            p_scb->rem_mtu_size = L2CAP_MTU_SIZE;

            L2C_SOCK_TRACE_DEBUG("l2c_sock_allocate_scb: BTA handle %d\n",p_scb->inx);

            return (p_scb);
        }
    }

    /* If here, no free CCB found */
    return (NULL);
}


/*******************************************************************************
**
** Function         l2c_sock_release_scb
**
** Description      This function releases a SCB.
**
** Returns          void
**
*******************************************************************************/
void l2c_sock_release_scb (tL2C_SOCK_CB *p_scb)
{
    UINT16       xx;
    UINT16      psm = p_scb->psm;

    while (p_scb->tx_queue.p_first)
       GKI_freebuf (GKI_dequeue (&p_scb->tx_queue));

    p_scb->state = L2C_SOCK_STATE_IDLE;
    p_scb->in_use   = FALSE;

    L2C_SOCK_TRACE_DEBUG("l2c_sock_release_scb: BTA handle %d lcid  %d psm %d\n",
                                            p_scb->inx, p_scb->lcid, p_scb->psm);

    /* If no-one else is using the PSM, deregister from L2CAP */
    for (xx = 0, p_scb = l2c_sock_mcb.l2c_sock; xx < MAX_L2C_SOCK_CONNECTIONS;
                                                               xx++, p_scb++)
    {
        if ((p_scb->state != L2C_SOCK_STATE_IDLE) && (p_scb->psm == psm))
            return;
    }

    /* Deregister the PSM with L2CAP */
    if(psm != BT_PSM_OPP_1_2)
    {
        L2CA_DEREGISTER (psm);
    }
}

#endif
