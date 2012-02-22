/*****************************************************************************
**
**  Name:           bta_ops_api.c
**
**  Description:    This is the implementation of the API for the object
**                  push server subsystem of BTA, Widcomm's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
    
#include "bt_target.h"
    
#if defined(BTA_OP_INCLUDED) && (BTA_OP_INCLUDED)

#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "bta_fs_api.h"
#include "bta_op_api.h"
#include "bta_ops_int.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_ops_reg =
{
    bta_ops_hdl_event,
    BTA_OpsDisable
};

/*******************************************************************************
**
** Function         BTA_OpsEnable
**
** Description      Enable the object push server.  This function must be
**                  called before any other functions in the OPS API are called.
**
** Returns          void
**
*******************************************************************************/
void BTA_OpsEnable(tBTA_SEC sec_mask, tBTA_OP_FMT_MASK formats,
                   char *p_service_name, tBTA_OPS_CBACK *p_cback, BOOLEAN srm, UINT8 app_id)
{
    tBTA_OPS_API_ENABLE     *p_buf;

    GKI_sched_lock();
    /* register with BTA system manager */
    bta_sys_register(BTA_ID_OPS, &bta_ops_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_OPS_API_ENABLE *) GKI_getbuf(sizeof(tBTA_OPS_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event = BTA_OPS_API_ENABLE_EVT;
        BCM_STRNCPY_S(p_buf->name, sizeof(p_buf->name), p_service_name, BTA_SERVICE_NAME_LEN);
        p_buf->name[BTA_SERVICE_NAME_LEN] = '\0';
        p_buf->p_cback = p_cback;
        p_buf->app_id = app_id;
        p_buf->formats = formats;
        p_buf->sec_mask = sec_mask;
        p_buf->srm = srm;

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_OpsDisable
**
** Description      Disable the object push server.  If the server is currently
**                  connected to a peer device the connection will be closed.
**
** Returns          void
**
*******************************************************************************/
void BTA_OpsDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_OPS);
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_OPS_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_OpsClose
**
** Description      Close the current connection.  This function is called if
**                  the phone wishes to close the connection before the object
**                  push is completed.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_OpsClose(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_OPS_API_CLOSE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_OpsAccessRsp
**
** Description      Sends a reply to an access request event (BTA_OPS_ACCESS_EVT).
**                  This call MUST be made whenever the event occurs.
**
** Parameters       oper    - operation being accessed.
**                  access  - BTA_OP_ACCESS_ALLOW, BTA_OP_ACCESS_FORBID, or
**                            BTA_OP_ACCESS_NONSUP.
**                  p_name  - Full path of file to read from (pull) or write to
**                            (push).
**
** Returns          void
**
*******************************************************************************/
void BTA_OpsAccessRsp(tBTA_OP_OPER oper, tBTA_OP_ACCESS access, char *p_name)
{
    tBTA_OPS_API_ACCESSRSP *p_buf;

    if ((p_buf = (tBTA_OPS_API_ACCESSRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_OPS_API_ACCESSRSP)
                                       + p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        p_buf->flag = access;
        p_buf->oper = oper;
        p_buf->p_name = (char *)(p_buf + 1);
        if (p_name)
        {
            BCM_STRNCPY_S(p_buf->p_name, p_bta_fs_cfg->max_path_len+1, p_name, p_bta_fs_cfg->max_path_len);
            p_buf->p_name[p_bta_fs_cfg->max_path_len] = '\0';
        }
        else
            *p_buf->p_name = '\0';

        p_buf->hdr.event = BTA_OPS_API_ACCESSRSP_EVT;
        bta_sys_sendmsg(p_buf);
    }
}
#endif /* BTA_OP_INCLUDED */
