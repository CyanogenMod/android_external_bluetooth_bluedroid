/*****************************************************************************
**
**  Name:           bta_opc_api.c
**
**  Description:    This is the implementation of the API for the object
**                  push client subsystem of BTA, Widcomm's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2003 - 2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "bta_fs_api.h"
#include "bta_op_api.h"
#include "bta_opc_int.h"
#include "bd.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_opc_reg =
{
    bta_opc_hdl_event,
    BTA_OpcDisable
};

/*******************************************************************************
**
** Function         BTA_OpcEnable
**
** Description      Enable the object push client.  This function must be
**                  called before any other functions in the DG API are called.
**                  When the enable operation is complete the callback function
**                  will be called with a BTA_OPC_ENABLE_EVT.
**
**                  If single_op is FALSE, the connection stays open after
**                  the operation finishes (until BTA_OpcClose is called).
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcEnable(tBTA_SEC sec_mask, tBTA_OPC_CBACK *p_cback,
                   BOOLEAN single_op, BOOLEAN srm, UINT8 app_id)
{
    tBTA_OPC_API_ENABLE     *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_OPC, &bta_opc_reg);
    GKI_sched_unlock();

    /* initialize control block */
    memset(&bta_opc_cb, 0, sizeof(tBTA_OPC_CB));

    if ((p_buf = (tBTA_OPC_API_ENABLE *) GKI_getbuf(sizeof(tBTA_OPC_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event = BTA_OPC_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        p_buf->sec_mask = sec_mask;
        p_buf->single_op = single_op;
        p_buf->srm = srm;
        p_buf->app_id = app_id;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_OpcDisable
**
** Description      Disable the object push client.  If the client is currently
**                  connected to a peer device the connection will be closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_OPC);
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_OPC_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_OpcPush
**
** Description      Push an object to a peer device.  p_name must point to 
**                  a fully qualified path and file name.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcPush(BD_ADDR bd_addr, tBTA_OP_FMT format, char *p_name)
{
    tBTA_OPC_DATA   *p_msg;

    if ((p_msg = (tBTA_OPC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_OPC_DATA) +
                                        p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        p_msg->api_push.p_name = (char *)(p_msg + 1);
        BCM_STRNCPY_S(p_msg->api_push.p_name, p_bta_fs_cfg->max_path_len+1, p_name, p_bta_fs_cfg->max_path_len);
        p_msg->api_push.p_name[p_bta_fs_cfg->max_path_len] = '\0';
        p_msg->hdr.event = BTA_OPC_API_PUSH_EVT;
        bdcpy(p_msg->api_push.bd_addr, bd_addr);
        p_msg->api_push.format = format;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_OpcPullCard
**
** Description      Pull default card from peer. p_path must point to a fully
**                  qualified path specifying where to store the pulled card.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcPullCard(BD_ADDR bd_addr, char *p_path)
{
    tBTA_OPC_DATA     *p_msg;
    tBTA_OPC_API_PULL *p_pull;

    if ((p_msg = (tBTA_OPC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_OPC_DATA) +
        (p_bta_fs_cfg->max_path_len + 1)))) != NULL)
    {
        p_pull = &p_msg->api_pull;
        p_pull->p_path = (char *)(p_msg + 1);
        BCM_STRNCPY_S(p_pull->p_path, p_bta_fs_cfg->max_path_len+1, p_path, p_bta_fs_cfg->max_path_len);
        p_pull->p_path[p_bta_fs_cfg->max_path_len] = '\0';
        bdcpy(p_pull->bd_addr, bd_addr);
        p_pull->hdr.event = BTA_OPC_API_PULL_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_OpcExchCard
**
** Description      Exchange business cards with a peer device. p_send points to 
**                  a fully qualified path and file name of vcard to push.
**                  p_recv_path points to a fully qualified path specifying
**                  where to store the pulled card.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcExchCard(BD_ADDR bd_addr, char *p_send, char *p_recv_path)
{
    tBTA_OPC_DATA     *p_msg;
    tBTA_OPC_API_EXCH *p_exch;

    if(!p_send || !p_recv_path)
        return;

    if ((p_msg = (tBTA_OPC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_OPC_DATA) +
        (p_bta_fs_cfg->max_path_len + 1) * 2))) != NULL)
    {
        p_exch = &p_msg->api_exch;
        p_exch->p_send = (char *)(p_msg + 1);
        p_exch->p_rcv_path = (char *)(p_exch->p_send + p_bta_fs_cfg->max_path_len + 1);
        BCM_STRNCPY_S(p_exch->p_send, p_bta_fs_cfg->max_path_len+1, p_send, p_bta_fs_cfg->max_path_len);
        p_exch->p_send[p_bta_fs_cfg->max_path_len] = '\0';
        BCM_STRNCPY_S(p_exch->p_rcv_path, p_bta_fs_cfg->max_path_len+1, p_recv_path, p_bta_fs_cfg->max_path_len);
        p_exch->p_rcv_path[p_bta_fs_cfg->max_path_len] = '\0';
        bdcpy(p_exch->bd_addr, bd_addr);
        p_exch->hdr.event = BTA_OPC_API_EXCH_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_OpcClose
**
** Description      Close the current connection.  This function is called if
**                  the phone wishes to close the connection before the object
**                  push is completed.  In a typical connection this function
**                  does not need to be called; the connection will be closed
**                  automatically when the object push is complete.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpcClose(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_OPC_API_CLOSE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}


