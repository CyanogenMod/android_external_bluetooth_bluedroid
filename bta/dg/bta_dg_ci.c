/*****************************************************************************
**
**  Name:           bta_dg_ci.c
**
**  Description:    This is the implementation file for data gateway call-in
**                  functions.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_dg_api.h"
#include "bta_dg_ci.h"
#include "bta_dg_int.h"
#include "gki.h"

/*******************************************************************************
**
** Function         bta_dg_ci_tx_ready
**
** Description      This function sends an event to DG indicating the phone is
**                  ready for more data and DG should call bta_dg_co_tx_path().
**                  This function is used when the TX data path is configured
**                  to use a pull interface.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_tx_ready(UINT16 handle)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_DG_CI_TX_READY_EVT;
        p_buf->layer_specific = handle;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_dg_ci_rx_ready
**
** Description      This function sends an event to DG indicating the phone
**                  has data available to send to DG and DG should call
**                  bta_dg_co_rx_path().  This function is used when the RX
**                  data path is configured to use a pull interface.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_rx_ready(UINT16 handle)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_DG_CI_RX_READY_EVT;
        p_buf->layer_specific = handle;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_dg_ci_tx_flow
**
** Description      This function is called to enable or disable data flow on
**                  the TX path.  The phone should call this function to
**                  disable data flow when it is congested and cannot handle
**                  any more data sent by bta_dg_co_tx_write() or
**                  bta_dg_co_tx_writebuf().  This function is used when the
**                  TX data path is configured to use a push interface.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_tx_flow(UINT16 handle, BOOLEAN enable)
{
    tBTA_DG_CI_TX_FLOW  *p_buf;

    if ((p_buf = (tBTA_DG_CI_TX_FLOW *) GKI_getbuf(sizeof(tBTA_DG_CI_TX_FLOW))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_CI_TX_FLOW_EVT;
        p_buf->hdr.layer_specific = handle;
        p_buf->enable = enable;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_dg_ci_rx_write
**
** Description      This function is called to send data to DG when the RX path
**                  is configured to use a push interface.  The function copies
**                  data to an event buffer and sends it to DG.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_rx_write(UINT16 handle, UINT8 *p_data, UINT16 len)
{

}

/*******************************************************************************
**
** Function         bta_dg_ci_rx_writebuf
**
** Description      This function is called to send data to the phone when
**                  the RX path is configured to use a push interface with
**                  zero copy.  The function sends an event to DG containing
**                  the data buffer.  The buffer must be allocated using
**                  functions GKI_getbuf() or GKI_getpoolbuf().  The buffer
**                  will be freed by BTA; the phone must not free the buffer.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_rx_writebuf(UINT16 handle, BT_HDR *p_buf)
{
    p_buf->event = BTA_DG_CI_RX_WRITEBUF_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_dg_ci_control
**
** Description      This function is called to send RS-232 signal information
**                  to DG to be propagated over RFCOMM.
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_dg_ci_control(UINT16 handle, UINT8 signals, UINT8 values)
{
    tBTA_DG_CI_CONTROL  *p_buf;

    if ((p_buf = (tBTA_DG_CI_CONTROL *) GKI_getbuf(sizeof(tBTA_DG_CI_CONTROL))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_CI_CONTROL_EVT;
        p_buf->hdr.layer_specific = handle;
        p_buf->signals = signals;
        p_buf->values = values;
        bta_sys_sendmsg(p_buf);
    }
}
