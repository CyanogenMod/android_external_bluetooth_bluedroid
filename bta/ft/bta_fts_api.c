/*****************************************************************************
**
**  Name:           bta_fts_api.c
**
**  Description:    This is the implementation of the API for the file
**                  transfer server subsystem of BTA, Widcomm's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bta_fs_api.h"
#include "bta_fts_int.h"

// btla-specific ++
//todo sdh

/* Maximum path length supported by MMI */
#ifndef BTA_FS_PATH_LEN
#define BTA_FS_PATH_LEN         294
#endif
// btla-specific --


/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_fts_reg =
{
    bta_fts_hdl_event,
    BTA_FtsDisable
};

/*******************************************************************************
**
** Function         BTA_FtsEnable
**
** Description      Enable the file transfer server.  This function must be
**                  called before any other functions in the FTS API are called.
**                  When the enable operation is complete the callback function
**                  will be called with an BTA_FTS_ENABLE_EVT event.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_FtsEnable(tBTA_SEC sec_mask, const char *p_service_name,
                   const char *p_root_path, BOOLEAN enable_authen,
                   UINT8 realm_len, UINT8 *p_realm,
                   tBTA_FTS_CBACK *p_cback, UINT8 app_id)
{
    tBTA_FTS_API_ENABLE *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_FTS, &bta_fts_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_FTS_API_ENABLE *)GKI_getbuf((UINT16)(sizeof(tBTA_FTS_API_ENABLE) +
                    p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        memset(p_buf, 0, sizeof(tBTA_FTS_API_ENABLE));

        p_buf->p_root_path = (char *)(p_buf + 1);
        p_buf->p_root_path[p_bta_fs_cfg->max_path_len] = '\0';

        p_buf->hdr.event = BTA_FTS_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        p_buf->sec_mask = sec_mask;
        p_buf->app_id = app_id;
        p_buf->auth_enabled = enable_authen;

        p_buf->realm_len = (realm_len < OBX_MAX_REALM_LEN) ? realm_len :
                            OBX_MAX_REALM_LEN;
        if (p_realm)
            memcpy(p_buf->realm, p_realm, p_buf->realm_len);

        if (p_service_name)
            BCM_STRNCPY_S(p_buf->servicename, sizeof(p_buf->servicename), p_service_name, BTA_SERVICE_NAME_LEN);

        if (p_root_path)
        {
            BCM_STRNCPY_S(p_buf->p_root_path, p_bta_fs_cfg->max_path_len+1, p_root_path,
                    p_bta_fs_cfg->max_path_len);
            p_buf->p_root_path[p_bta_fs_cfg->max_path_len] = '\0';
        }

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtsDisable
**
** Description      Disable the file transfer server.  If the server is currently
**                  connected to a peer device the connection will be closed.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtsDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_FTS);
    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_FTS_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtsClose
**
** Description      Close the current connection.  This function is called if
**                  the phone wishes to close the connection before the FT
**                  client disconnects.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_FtsClose(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_FTS_API_CLOSE_EVT;

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtsUnauthRsp
**
** Description      Sends an OBEX authentication challenge to the connected
**                  OBEX client. Called in response to an BTA_FTS_AUTH_EVT event.
**                  Used when "enable_authen" is set to TRUE in BTA_FtsEnable().
**
**                  Note: If the "userid_required" is TRUE in the BTA_FTS_AUTH_EVT
**                        event, then p_userid is required, otherwise it is optional.
**
**                  p_password  must be less than BTA_FTS_MAX_AUTH_KEY_SIZE
**                  p_userid    must be less than OBX_MAX_REALM_LEN
**
** Returns          void
**
*******************************************************************************/
void BTA_FtsAuthRsp (char *p_password, char *p_userid)
{
    tBTA_FTS_API_AUTHRSP *p_auth_rsp;

    if ((p_auth_rsp = (tBTA_FTS_API_AUTHRSP *)GKI_getbuf(sizeof(tBTA_FTS_API_AUTHRSP))) != NULL)
    {
        memset(p_auth_rsp, 0, sizeof(tBTA_FTS_API_AUTHRSP));

        p_auth_rsp->hdr.event = BTA_FTS_API_AUTHRSP_EVT;

        if (p_password)
        {
            p_auth_rsp->key_len = strlen(p_password);
            if (p_auth_rsp->key_len > BTA_FTS_MAX_AUTH_KEY_SIZE)
                p_auth_rsp->key_len = BTA_FTS_MAX_AUTH_KEY_SIZE;
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
** Function         BTA_FtsAccessRsp
**
** Description      Sends a reply to an access request event (BTA_FTS_ACCESS_EVT).
**                  This call MUST be made whenever the event occurs.
**
** Parameters       oper    - operation being accessed.
**                  access  - BTA_FT_ACCESS_ALLOW or BTA_FT_ACCESS_FORBID
**                  p_name  - Full path of file to pulled or pushed.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtsAccessRsp(tBTA_FT_OPER oper, tBTA_FT_ACCESS access, char *p_name)
{
    tBTA_FTS_API_ACCESSRSP *p_acc_rsp;

    if ((p_acc_rsp = (tBTA_FTS_API_ACCESSRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_FTS_API_ACCESSRSP)
                                       + p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        p_acc_rsp->flag = access;
        p_acc_rsp->oper = oper;
        p_acc_rsp->p_name = (char *)(p_acc_rsp + 1);
        if (p_name)
        {
            BCM_STRNCPY_S(p_acc_rsp->p_name, BTA_FS_PATH_LEN, p_name, p_bta_fs_cfg->max_path_len-1);
            p_acc_rsp->p_name[p_bta_fs_cfg->max_path_len-1] = '\0';
        }
        else
            *p_acc_rsp->p_name = '\0';

        p_acc_rsp->hdr.event = BTA_FTS_API_ACCESSRSP_EVT;
        bta_sys_sendmsg(p_acc_rsp);
    }
}
#endif /* BTA_FT_INCLUDED */
