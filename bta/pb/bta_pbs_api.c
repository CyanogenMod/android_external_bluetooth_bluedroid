/*****************************************************************************
**
**  Name:           bta_pbs_api.c
**
**  Description:    This is the implementation of the API for the phone book 
**                  access server subsystem of BTA, Widcomm's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bta_fs_api.h"
#include "bta_pbs_api.h"
#include "bta_pbs_int.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_pbs_reg =
{
    bta_pbs_hdl_event,
    BTA_PbsDisable
};

/*******************************************************************************
**
** Function         BTA_PbsEnable
**
** Description      Enable the phone book access server.  This function must be
**                  called before any other functions in the PB Server API are called.
**                  When the enable operation is complete the callback function
**                  will be called with an BTA_PBS_ENABLE_EVT event.
**                  Note: Pbs always enable (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT) 
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_PbsEnable(tBTA_SEC sec_mask, const char *p_service_name,
                   const char *p_root_path, BOOLEAN enable_authen,
                   UINT8 realm_len, UINT8 *p_realm,
                   tBTA_PBS_CBACK *p_cback, UINT8 app_id)
{
    tBTA_PBS_API_ENABLE *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_PBS, &bta_pbs_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_PBS_API_ENABLE *)GKI_getbuf((UINT16)(sizeof(tBTA_PBS_API_ENABLE) +
                    p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        memset(p_buf, 0, sizeof(tBTA_PBS_API_ENABLE));

        p_buf->p_root_path = (char *)(p_buf + 1);
        p_buf->p_root_path[0] = '\0';

        p_buf->hdr.event = BTA_PBS_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        p_buf->sec_mask = (sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
        p_buf->app_id = app_id;
        p_buf->auth_enabled = enable_authen;

        p_buf->realm_len = (realm_len < OBX_MAX_REALM_LEN) ? realm_len :
                            OBX_MAX_REALM_LEN;
        if (p_realm)
            memcpy(p_buf->realm, p_realm, p_buf->realm_len);

        if (p_service_name)
        {
            BCM_STRNCPY_S(p_buf->servicename, sizeof(p_buf->servicename), p_service_name, BTA_SERVICE_NAME_LEN);
            p_buf->servicename[BTA_SERVICE_NAME_LEN] = '\0';
        }

        if (p_root_path)
        {
            BCM_STRNCPY_S(p_buf->p_root_path, p_bta_fs_cfg->max_path_len+1, p_root_path, p_bta_fs_cfg->max_path_len);
            p_buf->p_root_path[p_bta_fs_cfg->max_path_len] = '\0';
        }

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_PbsDisable
**
** Description      Disable the Phone book access server.  If the server is currently
**                  connected to a peer device the connection will be closed.
**
** Returns          void
**
*******************************************************************************/
void BTA_PbsDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_PBS);
    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_PBS_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}
/*******************************************************************************
**
** Function         BTA_PbsAuthRsp
**
** Description      Respond to obex client authenticate repond by sending back password to
**                  BTA. Called in response to an BTA_PBS_AUTH_EVT event.
**                  Used when "enable_authen" is set to TRUE in BTA_PbapsEnable().
**
**                  Note: If the "userid_required" is TRUE in the BTA_PBS_AUTH_EVT
**                        event, then p_userid is required, otherwise it is optional.
**
**                  p_password  must be less than BTA_PBS_MAX_AUTH_KEY_SIZE (16 bytes)
**                  p_userid    must be less than OBX_MAX_REALM_LEN (defined in target.h)
**
** Returns          void
**
*******************************************************************************/
void BTA_PbsAuthRsp (char *p_password, char *p_userid)
{
    tBTA_PBS_API_AUTHRSP *p_auth_rsp;

    if ((p_auth_rsp = (tBTA_PBS_API_AUTHRSP *)GKI_getbuf(sizeof(tBTA_PBS_API_AUTHRSP))) != NULL)
    {
        memset(p_auth_rsp, 0, sizeof(tBTA_PBS_API_AUTHRSP));

        p_auth_rsp->hdr.event = BTA_PBS_API_AUTHRSP_EVT;

        if (p_password)
        {
            p_auth_rsp->key_len = strlen(p_password);
            if (p_auth_rsp->key_len > BTA_PBS_MAX_AUTH_KEY_SIZE)
                p_auth_rsp->key_len = BTA_PBS_MAX_AUTH_KEY_SIZE;
            memcpy(p_auth_rsp->key, p_password, p_auth_rsp->key_len);
        }

        if (p_userid)
        {
            p_auth_rsp->userid_len = strlen(p_userid);
            if (p_auth_rsp->userid_len > OBX_MAX_REALM_LEN)
                p_auth_rsp->userid_len = OBX_MAX_REALM_LEN;
            memcpy(p_auth_rsp->userid, p_userid, p_auth_rsp->userid_len);
        }

        bta_sys_sendmsg(p_auth_rsp);
    }
}

/*******************************************************************************
**
** Function         BTA_PbsAccessRsp
**
** Description      Sends a reply to an access request event (BTA_PBS_ACCESS_EVT).
**                  This call MUST be made whenever the event occurs.
**
** Parameters       oper    - operation being accessed.
**                  access  - BTA_PBS_ACCESS_ALLOW or BTA_PBS_ACCESS_FORBID
**                  p_name  - path of file or directory to be accessed.
**
** Returns          void
**
*******************************************************************************/
void BTA_PbsAccessRsp(tBTA_PBS_OPER oper, tBTA_PBS_ACCESS_TYPE access, char *p_name)
{
    tBTA_PBS_API_ACCESSRSP *p_acc_rsp;
    UINT16 max_full_name = p_bta_fs_cfg->max_path_len + p_bta_fs_cfg->max_file_len + 1;

    if ((p_acc_rsp = (tBTA_PBS_API_ACCESSRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_PBS_API_ACCESSRSP)
                                       + max_full_name + 1))) != NULL)
    {
        p_acc_rsp->flag = access;
        p_acc_rsp->oper = oper;
        p_acc_rsp->p_name = (char *)(p_acc_rsp + 1);
        if (p_name)
        {
            BCM_STRNCPY_S(p_acc_rsp->p_name, max_full_name+1, p_name, max_full_name);
            p_acc_rsp->p_name[max_full_name] = '\0';
        }
        else
            p_acc_rsp->p_name[0] = '\0';

        p_acc_rsp->hdr.event = BTA_PBS_API_ACCESSRSP_EVT;
        bta_sys_sendmsg(p_acc_rsp);
    }
}

/*******************************************************************************
**
** Function         BTA_PbsClose
**
** Description      Close the current connection.  
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_PbsClose(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_PBS_API_CLOSE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

#endif /* BTA_PBS_INCLUDED */
