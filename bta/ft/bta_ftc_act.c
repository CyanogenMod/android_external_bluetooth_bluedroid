/*****************************************************************************
**
**  Name:           bta_ftc_act.c
**
**  Description:    This file contains the file transfer action 
**                  functions for the state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "bd.h"
#include "port_api.h"
#include "obx_api.h"
#include "goep_util.h"
#include "sdp_api.h"
#include "bta_fs_api.h"
#include "bta_ft_api.h"
#include "bta_ftc_int.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "btm_api.h"
#include "rfcdefs.h"
#include "utl.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
/* sdp discovery database size */
#define BTA_FTC_DISC_SIZE       400


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if BTA_FTC_DEBUG == TRUE
static char *ftc_obx_evt_code(tOBX_EVENT evt_code);
#endif

static void ftc_reset_cb (tBTA_FTC_CB *p_cb);
static void bta_ftc_sdp_cback(UINT16 status);

/*****************************************************************************
**  Action Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_ftc_init_open
**
** Description      Initiate a connection with a peer device's service.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_init_open(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    p_cb->obx_oper = FTC_OP_NONE;
    p_cb->sec_mask = p_data->api_open.sec_mask;
    p_cb->services = p_data->api_open.services;
    p_cb->srm = p_data->api_open.srm;
    p_cb->nonce = p_data->api_open.nonce;
    bdcpy(p_cb->bd_addr, p_data->api_open.bd_addr);
}

/*******************************************************************************
**
** Function         bta_ftc_abort
**
** Description      Abort an active Get or Put operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_abort(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    /* Abort an active request */
    if (p_cb->obx_oper != FTC_OP_NONE)
    {
        p_cb->aborting = FTC_ABORT_REQ_NOT_SENT;

        /* Issue the abort request only if no request pending.
         * some devices do not like out of sequence aborts even though
         * the spec allows it */
        if ( !p_cb->req_pending || p_cb->first_req_pkt ) /* allow request for the first packet */
        {

            p_cb->aborting = FTC_ABORT_REQ_SENT;    /* protection agains sending AbortReq twice in
                                                       certain conditions */
            OBX_AbortReq(p_cb->obx_handle, (BT_HDR *)NULL);
            /* Start abort response timer */
            p_cb->timer_oper = FTC_TIMER_OP_ABORT;
            bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT,
                                p_bta_ft_cfg->abort_tout);
        }
    }
}

/*******************************************************************************
**
** Function         bta_ftc_api_action
**
** Description      Send an Action command to the connected server
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_api_action(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_PKT    *p_obx = &p_cb->obx;
    tBTA_FTC_STATUS      status = BTA_FTC_FAIL;
    tBTA_FTC_EVT         event = BTA_FTC_COPY_EVT + p_data->api_action.action;

    if (p_cb->obx_oper == FTC_OP_NONE)
    {
        /* TODO: we need to check the connected profile here */
        p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_CMD_POOL_SIZE);
        OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_data->api_action.p_src);
        if (p_data->api_action.action == BTA_FT_ACT_PERMISSION)
        {
            OBX_AddPermissionHdr(p_obx->p_pkt, p_data->api_action.permission[0],
                p_data->api_action.permission[1], p_data->api_action.permission[2]);
        }
        else
        {
            OBX_AddUtf8DestNameHdr(p_obx->p_pkt, (UINT8 *)p_data->api_action.p_dest);
        }

        if (OBX_ActionReq(p_cb->obx_handle, p_data->api_action.action, p_obx->p_pkt) == OBX_SUCCESS)
        {
            status = BTA_FTC_OK;
            p_cb->req_pending = TRUE;
            p_cb->obx_oper = FTC_OP_COPY + p_data->api_action.action;
            p_obx->p_pkt = NULL;    /* OBX will free the packet */
        }
    }

    if (status != BTA_FTC_OK)   /* Send an error response to the application */
    {
        utl_freebuf((void**)&p_obx->p_pkt);
        p_cb->p_cback(event, (tBTA_FTC *)&status);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_action_rsp
**
** Description      Process the response to an action command
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_action_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT  *p_evt = &p_data->obx_evt;
    tBTA_FTC_STATUS     status = BTA_FTC_FAIL;
    tBTA_FTC_EVT        event = BTA_FTC_COPY_EVT + p_cb->obx_oper - FTC_OP_COPY;

    p_cb->req_pending = FALSE;
    
    if (p_evt->rsp_code == OBX_RSP_OK)
        status = BTA_FTC_OK;

    APPL_TRACE_DEBUG2("bta_ftc_obx_action_rsp rsp_code:0x%x, status:%d", p_evt->rsp_code, status);
    p_cb->obx_oper = FTC_OP_NONE;

    utl_freebuf((void**)&p_evt->p_pkt); /* Done with Obex packet */
    p_cb->p_cback(event, (tBTA_FTC *)&status);

    /* If successful, initiate a directory listing */
    if (event != BTA_FTC_PERMISSION_EVT &&
        p_evt->rsp_code == OBX_RSP_OK && p_bta_ft_cfg->auto_file_list &&
        (p_cb->sdp_service == UUID_SERVCLASS_OBEX_FILE_TRANSFER) )
    {
        bta_ftc_get_listing(p_cb, ".", NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_get_srm_rsp
**
** Description      Process the response to an get command after suspend is sent
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_get_srm_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_ACTION p_act;
    /* TODO remove
    if (FTC_SUSPEND_REQ_NOT_SENT == p_cb->aborting)
    {
        p_cb->aborting = 0;    
        OBX_SessionReq (p_cb->obx_handle, OBX_SESS_OP_SUSPEND, 0);
        p_cb->timer_oper = FTC_TIMER_OP_SUSPEND;
        bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT, p_bta_ft_cfg->suspend_tout );
    }
    */

    if (p_cb->timer_oper == FTC_TIMER_OP_SUSPEND)
    {
        p_act = bta_ftc_obx_get_rsp;
    }
    else
        p_act = bta_ftc_ignore_obx;
    (p_act)(p_cb, p_data);
}

/*******************************************************************************
**
** Function         bta_ftc_ci_write_srm
**
** Description      Continue with the current write operation after suspend is sent
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ci_write_srm(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    if (p_cb->timer_oper == FTC_TIMER_OP_SUSPEND)
    {
        bta_ftc_ci_write(p_cb, p_data);
    }
}


/*******************************************************************************
**
** Function         bta_ftc_init_putfile
**
** Description      Push a file to the OPP or FTP server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_init_putfile(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FS_CO_STATUS status;
    tBTA_FTC          data;
    BOOLEAN           is_dir;
    UINT8             rsp_code = OBX_RSP_FAILED;

    data.status = BTA_FTC_FAIL;
    if (p_cb->obx_oper == FTC_OP_NONE)
    {
        if (p_cb->sdp_service != UUID_SERVCLASS_PBAP_PSE)
        {
            p_cb->obx_oper = FTC_OP_PUT_FILE;

            status = bta_fs_co_access(p_data->api_put.p_name, BTA_FS_ACC_READ,
                                      &is_dir, p_cb->app_id);
            /* Verify that the file exists, and get (optional) file length */
            if (is_dir || status != BTA_FS_CO_OK)
            {
                /* Local file is not accessible or does not exist */
                if (is_dir || status == BTA_FS_CO_EACCES)
                    rsp_code = OBX_RSP_UNAUTHORIZED;
                else
                    rsp_code = OBX_RSP_NOT_FOUND;
            }
            else
            {
                if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
                {
                    BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len+1, p_data->api_put.p_name, p_bta_fs_cfg->max_path_len);
                    p_cb->cout_active = TRUE;
                    bta_fs_co_open(p_cb->p_name, BTA_FS_O_RDONLY, 0, BTA_FTC_CI_OPEN_EVT,
                                   p_cb->app_id);
                    rsp_code = OBX_RSP_OK;
                }
            }
            if (rsp_code != OBX_RSP_OK)
            {
                p_data->obx_evt.rsp_code = rsp_code;
                bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
            }
            return;
        }
        else
        {
            /* PBAP does not allow PUT */
            data.status = BTA_FTC_NO_PERMISSION;
        }
    }

    p_cb->p_cback(BTA_FTC_PUTFILE_EVT, &data);
}

/*******************************************************************************
**
** Function         bta_ftc_init_getfile
**
** Description      Pull a file off the server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_init_getfile(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_API_GET *p_get = &p_data->api_get;
    tBTA_FTC_GET_PARAM *p_param = p_get->p_param;
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FTC_STATUS   status = BTA_FTC_FAIL;
    tBTA_FTC          data;
    UINT16            buflen;
    UINT16            rem_name_len;
    BOOLEAN is_ok;
    UINT8   *p = (UINT8 *)BTA_FTC_PULL_VCARD_ENTRY_TYPE;
    UINT8   *p_start;
    UINT16  len    = 0;

    /* Available for FTP only */
    if (p_cb->obx_oper == FTC_OP_NONE &&
        (p_cb->sdp_service == UUID_SERVCLASS_OBEX_FILE_TRANSFER ||
         p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE) )
    {
        p_cb->first_req_pkt = TRUE;
        p_obx->offset = 0;
        if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_CMD_POOL_SIZE/*p_cb->peer_mtu*/)) != NULL)
        {
            /* Add the Name Header to the request */
            is_ok = OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_get->p_rem_name);

            if (p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE && is_ok)
            {
                /* add type header */
                if (p_get->obj_type == BTA_FTC_GET_PB)
                    p = (UINT8 *)BTA_FTC_PULL_PB_TYPE;
                is_ok = OBX_AddTypeHdr(p_obx->p_pkt, (char *)p);

                /* add app params for PCE */
                p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                p = p_start;
                if (p_get->obj_type != BTA_FTC_GET_PB || p_param->max_list_count != 0)
                {
                    if (p_param->filter)
                    {
                        *p++    = BTA_FTC_APH_FILTER;
                        *p++    = 8;
                        /* set proprietary filter to 0 */
                        UINT32_TO_BE_STREAM(p, 0);
                        UINT32_TO_BE_STREAM(p, p_param->filter);
                    }
                    if (p_param->format < BTA_FTC_FORMAT_MAX)
                    {
                        *p++    = BTA_FTC_APH_FORMAT;
                        *p++    = 1;
                        *p++    = p_param->format;
                    }
                }

                if (p_get->obj_type == BTA_FTC_GET_PB)
                {
                    /* this is mandatory */
                    *p++    = BTA_FTC_APH_MAX_LIST_COUNT;
                    *p++    = 2;
                    UINT16_TO_BE_STREAM(p, p_param->max_list_count);

                    if (p_param->list_start_offset)
                    {
                        *p++    = BTA_FTC_APH_LIST_STOFF;
                        *p++    = 2;
                        UINT16_TO_BE_STREAM(p, p_param->list_start_offset);
                    }
                }

                /* If any of the app param header is added */
                if (p != p_start)
                {
                    OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
                }
            }
        }

        rem_name_len = (UINT16)(strlen(p_get->p_rem_name) + 1);
        /* Need a buffer that can hold the local path and UTF-8 filename */
        buflen = p_bta_fs_cfg->max_path_len + 2 + rem_name_len;

        /* Save the UNICODE name of the remote file, and local filename and path */
        if ((p_cb->p_name = (char *)GKI_getbuf(buflen)) != NULL)
        {
            memset(p_cb->p_name, 0, buflen);
            BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len, p_get->p_name, p_bta_fs_cfg->max_path_len-1);

            p_cb->obj_type = p_get->obj_type;
            p_cb->obx_oper = FTC_OP_GET_FILE;
            if (p_cb->obj_type == BTA_FTC_GET_PB && p_param->max_list_count == 0)
            {
                /* getting the phone book size only. do not need to open a file */
                bta_ftc_send_get_req(p_cb);
            }
            else
            {
                p_cb->cout_active = TRUE;
                bta_fs_co_open(p_cb->p_name,
                               (BTA_FS_O_RDWR | BTA_FS_O_CREAT | BTA_FS_O_TRUNC),
                               0, BTA_FTC_CI_OPEN_EVT, p_cb->app_id);
            }
            status = BTA_FTC_OK;
        }
    }

    if (status != BTA_FTC_OK)
    {
        data.status = status;
        p_cb->p_cback(BTA_FTC_GETFILE_EVT, &data);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_chdir
**
** Description      Change Directory and get a listing of it.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_chdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_API_CHDIR  *p_chdir = &p_data->api_chdir;
    tBTA_FTC_OBX_PKT    *p_obx = &p_cb->obx;
    tOBX_SETPATH_FLAG    obx_flags = OBX_SPF_NO_CREATE;
    tBTA_FTC_STATUS      status = BTA_FTC_FAIL;
    UINT16               len = OBX_CMD_POOL_SIZE;    /* Don't need special OBX buffer pool */

    /* Only process if no other OBX operation is active and connected to FTP */
    if (p_cb->obx_oper == FTC_OP_NONE)
    {
        if (p_cb->sdp_service == UUID_SERVCLASS_OBEX_FILE_TRANSFER ||
            p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE )
        {
            /* Allocate an OBX packet */
            if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle, len)) != NULL)
            {
                status = BTA_FTC_OK;

                /* Add the name header if not backing up */
                if (p_chdir->flag != BTA_FTC_FLAG_BACKUP)
                {
                    if (!OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_chdir->p_dir))
                        status = BTA_FTC_FAIL;
                }
                else
                    obx_flags |= OBX_SPF_BACKUP;
            }
        }
    }

    if (status == BTA_FTC_OK)
    {
        if (OBX_SetPathReq(p_cb->obx_handle, obx_flags, p_obx->p_pkt))
        {
            status = BTA_FTC_FAIL;
        }
        else
        {
            p_cb->req_pending = TRUE;
            p_cb->obx_oper = FTC_OP_CHDIR;
            p_obx->p_pkt = NULL;    /* OBX will free the packet */
        }

    }

    if (status != BTA_FTC_OK)   /* Send an error response to the application */
    {
        utl_freebuf((void**)&p_obx->p_pkt);
        p_cb->p_cback(BTA_FTC_CHDIR_EVT, (tBTA_FTC *)&status);
    }
}

/*******************************************************************************
**
** Function         ftp_clnt_listdir
**
** Description      List the specified directory contents.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_listdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    /* Only process if no other OBX operation is active and connected to FTP */
    if (p_cb->obx_oper == FTC_OP_NONE)
        bta_ftc_get_listing(p_cb, p_data->api_list.p_dir, p_data->api_list.p_param);
    else
        bta_ftc_listing_err(&p_cb->obx.p_pkt, BTA_FTC_FAIL);
}

/*******************************************************************************
**
** Function         bta_ftc_remove
** Description      Remove the specified directory or a file.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_remove(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_PKT     *p_obx = &p_cb->obx;
    tBTA_FTC_STATUS      status = BTA_FTC_OK;

    APPL_TRACE_DEBUG1("bta_ftc_remove status:%d", p_cb->obx_oper);

    if (p_data->api_mkdir.p_dir[0] != '\0')
    {

        /* Only process if no other OBX operation is active and connected to FTP */
        if (p_cb->obx_oper == FTC_OP_NONE)
        {
            if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_CMD_POOL_SIZE)) != NULL)
            {
                /* Add the Name Header to the request */
                status = BTA_FTC_OK;
                OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_data->api_remove.p_name);
            }
            else
                status = BTA_FTC_FAIL;
        }
        else
            status = BTA_FTC_BUSY;
    }
    else
        status = BTA_FTC_NOT_FOUND;

    if (status == BTA_FTC_OK)
    {
        if ((OBX_PutReq(p_cb->obx_handle, TRUE, p_obx->p_pkt)) == OBX_SUCCESS)
        {
            p_cb->req_pending = TRUE;
            p_cb->obx_oper = FTC_OP_DELETE;
            p_obx->p_pkt = NULL;    /* OBX will free the packet */
        }
        else
            status = BTA_FTC_FAIL;
    }

    if (status != BTA_FTC_OK)   /* Send an error response to the application */
    {
        utl_freebuf((void**)&p_obx->p_pkt);
        p_cb->p_cback(BTA_FTC_REMOVE_EVT, (tBTA_FTC *)&status);
    }
}
/*******************************************************************************
**
** Function         bta_ftc_mkdir
**
** Description      Create the specified directory.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_mkdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_PKT    *p_obx = &p_cb->obx;
    tBTA_FTC_STATUS      status = BTA_FTC_OK;

    if (p_data->api_mkdir.p_dir[0] != '\0')
    {
        /* Only process if no other OBX operation is active and connected to FTP */
        if (p_cb->obx_oper == FTC_OP_NONE)
        {
            if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_CMD_POOL_SIZE)) != NULL)
            {
                /* Add the Name Header to the request */
                status = BTA_FTC_OK;
                OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_data->api_mkdir.p_dir);
            }
        }
        else
            status = BTA_FTC_BUSY;
    }
    else
        status = BTA_FTC_NOT_FOUND;

    if (status == BTA_FTC_OK )
    {
        if ((OBX_SetPathReq(p_cb->obx_handle, 0, p_obx->p_pkt)) == OBX_SUCCESS)
        {
            p_cb->req_pending = TRUE;
            p_cb->obx_oper = FTC_OP_MKDIR;
            p_obx->p_pkt = NULL;    /* OBX will free the packet */
        }
        else
            status = BTA_FTC_FAIL;
    }

    if (status != BTA_FTC_OK)   /* Send an error response to the application */
    {
        utl_freebuf((void**)&p_obx->p_pkt);
        p_cb->p_cback(BTA_FTC_MKDIR_EVT, (tBTA_FTC *)&status);
    }
}



/*******************************************************************************
**
** Function         bta_ftc_send_authrsp
**
** Description      Pass the response to an authentication request back to the
**                  client.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_send_authrsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    UINT8 *p_pwd = NULL;
    UINT8 *p_userid = NULL;

    if (p_bta_ft_cfg->p_bi_action != NULL && 
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_AUTHRSP_IDX] &&
        p_cb->sdp_service == UUID_SERVCLASS_IMAGING_RESPONDER)
    {
        (p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_AUTHRSP_IDX])(p_cb, p_data);
        return;
    }
    else
    {
        if (p_data->auth_rsp.key_len > 0)
            p_pwd = (UINT8 *)p_data->auth_rsp.key;
        if (p_data->auth_rsp.userid_len > 0)
            p_userid = (UINT8 *)p_data->auth_rsp.userid;

        OBX_AuthResponse(p_cb->obx_handle, p_pwd, p_data->auth_rsp.key_len, 
                         p_userid, p_data->auth_rsp.userid_len, FALSE);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_trans_cmpl
**
** Description      push/pull complete
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_trans_cmpl(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC param;

    /* Close the opened file */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;
    }

    param.status = bta_ftc_convert_obx_to_ftc_status(p_data->obx_evt.rsp_code);

    if (p_cb->obx_oper == FTC_OP_PUT_FILE)
        p_cb->p_cback(BTA_FTC_PUTFILE_EVT, &param);
    else if (p_cb->obx_oper == FTC_OP_GET_FILE)
    {
        if (param.status != BTA_FTC_OK)
        {
            bta_fs_co_unlink(p_cb->p_name, p_cb->app_id);
            APPL_TRACE_WARNING2("FTC: Get File Operation Aborted or Error [%s], status 0x%02x",
                                p_cb->p_name, param.status);
        }

        p_cb->p_cback(BTA_FTC_GETFILE_EVT, &param);
    }

    /* Clean up control block */
    ftc_reset_cb(p_cb);
}

/*******************************************************************************
**
** Function         bta_ftc_ci_write
**
** Description      Continue with the current write operation
**                  (Get File processing)
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ci_write(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FTC          param;
    UINT8             rsp_code = OBX_RSP_FAILED;

    p_cb->cout_active = FALSE;

    /* Process write call-in event if operation is still active */
    if (p_cb->obx_oper != FTC_OP_GET_FILE)
        return;

    /* Free current packet */
    utl_freebuf((void**)&p_obx->p_pkt);

    /* Process any pending abort */
    if (p_cb->aborting)
    {
        p_cb->aborting |= FTC_ABORT_COUT_DONE;

        /* If already have response to abort then notify appl and cleanup */
        if (FTC_ABORT_COMPLETED == p_cb->aborting)
        {
            /* APPL_TRACE_DEBUG0("Received WRITE CALL-IN function; continuing abort..."); */

            /* OBX_RSP_GONE indicates aborted; used internally when called from API */
            p_data->obx_evt.rsp_code = (!p_cb->int_abort) ? OBX_RSP_GONE : OBX_RSP_INTRNL_SRVR_ERR;
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
            return;
        }
    }

    if (p_data->write_evt.status == BTA_FS_CO_OK)
    {
        param.prog.bytes = p_obx->offset;
        param.prog.file_size = p_cb->file_size;
        p_cb->p_cback(BTA_FTC_PROGRESS_EVT, &param);

        /* Send another Get request if not finished */
        if (!p_obx->final_pkt)
        {
            /* send a new request */
            bta_ftc_send_get_req(p_cb);
            rsp_code = OBX_RSP_CONTINUE;
        }
        else
            rsp_code = OBX_RSP_OK;
    }

    if (rsp_code != OBX_RSP_CONTINUE)
    {
        p_data->obx_evt.rsp_code = rsp_code;
        bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_ci_read
**
** Description      Handles the response to a read call-out request.
**                  This is called within the OBX get file request.  The
**                  operation has completed, send the OBX packet out.
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ci_read(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_PKT    *p_obx = &p_cb->obx;
    tBTA_FS_CI_READ_EVT *p_revt = &p_data->read_evt;
    tBTA_FTC             param;
    BOOLEAN              is_final;

    p_cb->cout_active = FALSE;

    /* Process read call-in event if operation is still active */
    if (p_cb->obx_oper == FTC_OP_PUT_FILE)
    {
        if (!p_cb->aborting)
        {
            if (p_revt->status != BTA_FS_CO_OK && p_revt->status != BTA_FS_CO_EOF)
            {
                p_cb->int_abort = TRUE;
                bta_ftc_sm_execute(p_cb, BTA_FTC_API_ABORT_EVT, p_data);
            }
            else
            {
                is_final = (p_revt->status == BTA_FS_CO_EOF) ? TRUE: FALSE;

                /* Add the body header to the packet */
                OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_revt->num_read, is_final);

                p_cb->obx.bytes_left -= p_revt->num_read;
                p_cb->obx.offset += p_revt->num_read;

                /* Send out the data */
                OBX_PutReq(p_cb->obx_handle, is_final, p_obx->p_pkt);
                p_obx->p_pkt = NULL;
                p_cb->req_pending = TRUE;

                /* Give application the status */
                param.prog.bytes = p_revt->num_read;
                param.prog.file_size = p_cb->file_size;
                p_cb->p_cback(BTA_FTC_PROGRESS_EVT, &param);
            }
        }
        else
        {
            /* Process any pending abort */
            p_cb->aborting |= FTC_ABORT_COUT_DONE;

            /* If already have response to abort then notify appl and cleanup */
            if (FTC_ABORT_COMPLETED == p_cb->aborting)
            {
                /* APPL_TRACE_DEBUG0("Received READ CALL-IN function; continuing abort..."); */

                /* OBX_RSP_GONE indicates aborted; used internally when called from API */
                p_data->obx_evt.rsp_code = (!p_cb->int_abort) ? OBX_RSP_GONE : OBX_RSP_INTRNL_SRVR_ERR;
                bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
                return;
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_ftc_ci_open
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ci_open(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FS_CI_OPEN_EVT *p_open = &p_data->open_evt;
    UINT8 rsp_code = OBX_RSP_FAILED;

    p_cb->cout_active = FALSE;
    APPL_TRACE_DEBUG2("bta_ftc_ci_open status:%d, aborting:%d", p_open->status, p_cb->aborting);

    /* Process any pending abort */
    if (p_cb->aborting)
    {
        p_cb->aborting |= FTC_ABORT_COUT_DONE;

        /* If already have response to abort then notify appl and cleanup */
        if (FTC_ABORT_COMPLETED == p_cb->aborting)
        {
            /* APPL_TRACE_DEBUG0("Received OPEN CALL-IN function; continuing abort..."); */

            /* OBX_RSP_GONE indicates aborted; used internally when called from API */
            p_data->obx_evt.rsp_code = (!p_cb->int_abort) ? OBX_RSP_GONE : OBX_RSP_INTRNL_SRVR_ERR;
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
            return;
        }
    }

    /* if file is accessible read/write the first buffer of data */
    if (p_open->status == BTA_FS_CO_OK)
    {
        p_cb->fd = p_open->fd;

        /* If using OBEX 1.5 */
        if (p_open->p_file)
        {
            if ((p_cb->p_name != NULL) ||
                (p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
            {
                /* save the image file name */
                BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len+1, p_open->p_file, p_bta_fs_cfg->max_path_len);
                p_cb->p_name[p_bta_fs_cfg->max_path_len] = '\0';
                p_cb->first_req_pkt = FALSE;
            }
        }
        else
            p_cb->first_req_pkt = TRUE;

        APPL_TRACE_DEBUG2("first_req_pkt:%d obx_oper:%d", p_cb->first_req_pkt, p_cb->obx_oper);

        if (p_cb->obx_oper == FTC_OP_PUT_FILE)
        {
            p_cb->file_size = p_open->file_size;
            rsp_code = bta_ftc_cont_put_file(p_cb, p_cb->first_req_pkt);
        }
        else if (p_cb->obx_oper == FTC_OP_GET_FILE)
        {
            /* Initiate the first OBX GET request */
            rsp_code = bta_ftc_send_get_req(p_cb);
        }
    }
    else
    {
        /* If using OBEX 1.5 */
        if (p_open->status == BTA_FS_CO_NONE)
        {
            rsp_code = OBX_RSP_OK;
            if (p_cb->obx_oper == FTC_OP_GET_LIST)
            {
                bta_ftc_abort(p_cb, NULL);
            }
        }
        else
        {
            if (p_open->status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else    /* File could not be found */
                rsp_code = OBX_RSP_NOT_FOUND;
        }
    }

    if (rsp_code != OBX_RSP_OK)
    {
        p_data->obx_evt.rsp_code = rsp_code;
        bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_ci_resume
**
** Description      Continue with the resume session
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ci_resume(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FS_CI_RESUME_EVT   *p_resume = &p_data->resume_evt;

    APPL_TRACE_DEBUG3("bta_ftc_ci_resume status:%d, ssn:%d, info:%d",
        p_resume->status, p_resume->ssn, p_resume->info);

    if (p_resume->status == BTA_FS_CO_OK && 
        OBX_AllocSession (p_resume->p_sess_info, 0, &p_cb->psm,
                       bta_ftc_obx_cback, &p_cb->obx_handle) == OBX_SUCCESS)
    {
        BTM_SetSecurityLevel (TRUE, "BTA_FTC", BTM_SEC_SERVICE_OBEX_FTP,
                              p_cb->sec_mask, p_cb->psm,
                              0, 0);
        OBX_ResumeSession (p_cb->bd_addr, p_resume->ssn, p_resume->offset, p_cb->obx_handle);
    }
    else
    {
        p_cb->status = BTA_FTC_OBX_ERR;
        p_data->obx_evt.rsp_code = OBX_RSP_FAILED;
        bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CLOSE_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_conn_rsp
**
** Description      Process the OBX connect event.
**                  If FTP service, get directory listing.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_conn_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT    *p_evt = &p_data->obx_evt;
    tBTA_FTC            param;
    tOBX_SESS_EVT       *p_sess;
    UINT8               obx_oper, *p_obx_oper = &p_cb->obx_oper;

    APPL_TRACE_DEBUG1("bta_ftc_obx_conn_rsp obx_event=%d", p_evt->obx_event);

    /* If using OBEX 1.5 */
    if (p_evt->obx_event == OBX_SESSION_RSP_EVT)
    {
        p_sess = &p_evt->param.sess;
        APPL_TRACE_DEBUG2("bta_ftc_obx_conn_rsp sess_id:0x%x/0x%x", p_cb->p_sess_info, p_sess->p_sess_info);
        p_cb->p_sess_info = p_sess->p_sess_info;
        APPL_TRACE_DEBUG2("client cb p_sess_info:0x%x obj_offset:x%x", p_cb->p_sess_info, p_sess->obj_offset);
        if (p_cb->nonce != BTA_FTC_RESUME_NONCE)
        {
            p_obx_oper = &obx_oper;
            p_cb->obx_oper = FTC_OP_NONE;
        }
        bta_fs_co_session_info(p_cb->bd_addr, p_cb->p_sess_info, p_sess->ssn, BTA_FS_CO_SESS_ST_ACTIVE, NULL, p_obx_oper, p_cb->app_id);
        if ((p_sess->sess_op == OBX_SESS_OP_CREATE) || (p_sess->sess_op == OBX_SESS_OP_RESUME))
        {
            p_cb->state = BTA_FTC_W4_CONN_ST;
        }
        if (p_cb->obx_oper)
            bta_fs_co_resume_op(p_sess->obj_offset, BTA_FTC_CI_OPEN_EVT, p_cb->app_id);
        return;
    }

    p_cb->peer_mtu = p_data->obx_evt.param.conn.mtu;

    APPL_TRACE_DEBUG2("bta_ftc_obx_conn_rsp peer_mtu=%d obx_oper:0x%x", p_cb->peer_mtu, p_cb->obx_oper);

    /* OPP is connected and ready */
    if (p_cb->sdp_service == UUID_SERVCLASS_OBEX_OBJECT_PUSH)
    {
        param.open.service = BTA_OPP_SERVICE_ID;
        APPL_TRACE_EVENT0("[BTA FTC]OPP Service Id ");
    }
    /* PBAP is connected and ready */
    else if (p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE)
    {
        param.open.service = BTA_PBAP_SERVICE_ID;
        APPL_TRACE_EVENT0("[BTA FTC]PBAP Service Id ");
    }
    else /* Initiate directory listing if FTP service */
    {
        APPL_TRACE_EVENT0("[BTA FTC]FTP Service Id ");
        param.open.service = BTA_FTP_SERVICE_ID;
        if (p_bta_ft_cfg->auto_file_list && !p_cb->obx_oper)
        {
            bta_ftc_get_listing(p_cb, ".", NULL);
        }
    }

    if (p_cb->p_sess_info == NULL)
    {
        bta_fs_co_session_info(p_cb->bd_addr, p_cb->p_sess_info, 0, BTA_FS_CO_SESS_ST_NONE, NULL, NULL, p_cb->app_id);
    }
    p_cb->suspending = FALSE;

    /* inform role manager */
    bta_sys_conn_open( BTA_ID_FTC ,p_cb->app_id, p_cb->bd_addr);

    param.open.version = p_cb->version;
    p_cb->p_cback(BTA_FTC_OPEN_EVT, &param);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_ftc_obx_sess_rsp
**
** Description      Process the OBX session event.
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_sess_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT *p_evt = &p_data->obx_evt;
    tOBX_SESS_EVT    *p_sess = &p_data->obx_evt.param.sess;

    APPL_TRACE_DEBUG3("bta_ftc_obx_sess_rsp obx_event=%d sess_op:%d, ssn:%d",
        p_evt->obx_event, p_sess->sess_op, p_sess->ssn);
    if (p_evt->obx_event == OBX_SESSION_RSP_EVT && p_sess->sess_op == OBX_SESS_OP_SUSPEND)
    {
        p_cb->p_sess_info = p_sess->p_sess_info;
        APPL_TRACE_DEBUG1("client cb p_sess_info:0x%x", p_cb->p_sess_info);
        bta_fs_co_suspend(p_cb->bd_addr, p_cb->p_sess_info, p_sess->ssn, &p_sess->timeout, NULL, p_cb->obx_oper, p_cb->app_id);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_suspended
**
** Description      Process transport down suspend.
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_suspended(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT *p_evt = &p_data->obx_evt;
    tOBX_SESS_EVT    *p_sess = &p_data->obx_evt.param.sess;
    UINT32 offset = 0;

    APPL_TRACE_DEBUG4("bta_ftc_suspended obx_event=%d sess_op:%d, ssn:%d, obx_oper:%d",
        p_evt->obx_event, p_sess->sess_op, p_sess->ssn, p_cb->obx_oper);
    if (p_evt->obx_event == OBX_SESSION_INFO_EVT)
    {
        if ((p_cb->obx_oper != FTC_OP_GET_FILE) && (p_cb->obx_oper != FTC_OP_PUT_FILE) && (p_cb->obx_oper != FTC_OP_GET_LIST))
        {
            /* the other obx ops are single transaction.
             * If the link is dropped before the transaction ends, re-transmit the request when the session resumes */
            p_sess->ssn--;
            p_cb->obx_oper = FTC_OP_NONE;
        }
        p_cb->suspending = TRUE;
        bta_fs_co_suspend(p_cb->bd_addr, p_sess->p_sess_info, p_sess->ssn,
                          &p_sess->timeout, &offset, p_cb->obx_oper, p_cb->app_id);
        /* do not need to worry about active call-out here.
         * this is a result of OBX_SESSION_INFO_EVT
         * OBX would report OBX_CLOSE_IND_EVT, which will eventually call bta_ftc_close_complete */
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_abort_rsp
**
** Description      Process the OBX abort event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_abort_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    bta_sys_stop_timer(&p_cb->rsp_timer);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_data->obx_evt.p_pkt);

    if (p_cb->obx_oper != FTC_OP_NONE)
    {
        if (!p_cb->cout_active)
        {
            /* OBX_RSP_GONE indicates aborted; used internally when called from API */
            p_data->obx_evt.rsp_code = (!p_cb->int_abort) ? OBX_RSP_GONE : OBX_RSP_INTRNL_SRVR_ERR;
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
        }
        else /* must wait for cout function to complete before finishing the abort */
        {
            /* Mark the fact we have already received the response from the peer */
            p_cb->aborting |= FTC_ABORT_RSP_RCVD;
            /* APPL_TRACE_DEBUG1("ftc_obx_abort_rsp()->Rcvd abort_rsp; waiting for callin...",
                                p_cb->aborting); */
        }
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_password
**
** Description      Process the OBX password request
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_password(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT    *p_evt = &p_data->obx_evt;
    tBTA_FTC_AUTH        parms;
    tOBX_AUTH_OPT        options;

    memset(&parms, 0, sizeof(tBTA_FTC_AUTH));

    /* Extract user id from packet (if available) */
    if (OBX_ReadChallenge(p_evt->p_pkt, &parms.realm_charset,
                          &parms.p_realm,
                          &parms.realm_len, &options))
    {
        if (options & OBX_AO_USR_ID)
            parms.userid_required = TRUE;
    }

    /* Don't need OBX packet any longer */
    utl_freebuf((void**)&p_evt->p_pkt);

    /* Notify application */
    p_cb->p_cback(BTA_FTC_AUTH_EVT, (tBTA_FTC *)&parms);
}

/*******************************************************************************
**
** Function         bta_ftc_obx_timeout
**
** Description      Process the OBX timeout event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_timeout(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    utl_freebuf((void**)&p_data->obx_evt.p_pkt);

    /* If currently processing OBX request, peer is unresponsive so we're done */
    if (p_cb->aborting || p_cb->obx_oper != FTC_OP_NONE)
    {
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
        if (p_cb->bic_handle != BTA_BIC_INVALID_HANDLE)
        {
            BTA_BicDeRegister(p_cb->bic_handle);
            p_cb->bic_handle    = BTA_BIC_INVALID_HANDLE;
        }
        else
#endif
        {
            p_data->obx_evt.rsp_code = OBX_RSP_GONE;
            p_cb->status = BTA_FTC_OBX_TOUT;

            /* Start stop response timer */
            p_cb->timer_oper = FTC_TIMER_OP_STOP;
            bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT,
                                p_bta_ft_cfg->stop_tout);

            OBX_DisconnectReq(p_cb->obx_handle, NULL);
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CLOSE_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_ftc_obx_put_rsp
**
** Description      Process the OBX file put and delete file/folder events
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_put_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT  *p_evt = &p_data->obx_evt;
    tBTA_FTC_STATUS    status;

    p_cb->req_pending = FALSE;
    p_cb->first_req_pkt = FALSE;

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

    if (p_cb->obx_oper == FTC_OP_PUT_FILE)
    {
        /* If not finished with Put, start another read */
        if (p_evt->rsp_code == OBX_RSP_CONTINUE)
            bta_ftc_cont_put_file(p_cb, FALSE);
        else
        {
            p_data->obx_evt.rsp_code = p_evt->rsp_code;
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
        }
    }

    else if (p_cb->obx_oper == FTC_OP_DELETE)
    {
        status = bta_ftc_convert_obx_to_ftc_status(p_data->obx_evt.rsp_code);
        p_cb->obx_oper = FTC_OP_NONE;
        p_cb->p_cback(BTA_FTC_REMOVE_EVT, (tBTA_FTC *)&status);
    }

}

/*******************************************************************************
**
** Function         bta_ftc_obx_get_rsp
**
** Description      Process the OBX file get and folder listing events
**                  If the type header is not folder listing, then pulling a file.
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_get_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT  *p_evt = &p_data->obx_evt;

    APPL_TRACE_DEBUG2("bta_ftc_obx_get_rsp req_pending=%d obx_oper:%d", p_cb->req_pending, p_cb->obx_oper);
    p_cb->req_pending = FALSE;

    if (p_cb->obx_oper == FTC_OP_GET_FILE)
        bta_ftc_proc_get_rsp(p_cb, p_data);
    else if (p_cb->obx_oper == FTC_OP_GET_LIST)
        bta_ftc_proc_list_data(p_cb, p_evt);
    else    /* Release the unexpected OBX response packet */
        utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_ftc_obx_setpath_rsp
**
** Description      Process the response to a change or make directory requests
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_setpath_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT  *p_evt = &p_data->obx_evt;
    tBTA_FTC_STATUS     status = BTA_FTC_FAIL;
    tBTA_FTC_EVT        event = BTA_FTC_CHDIR_EVT;

    p_cb->req_pending = FALSE;

    if (p_cb->obx_oper == FTC_OP_MKDIR)
        event = BTA_FTC_MKDIR_EVT;

    if (p_evt->rsp_code == OBX_RSP_OK)
        status = BTA_FTC_OK;

    p_cb->obx_oper = FTC_OP_NONE;

    utl_freebuf((void**)&p_evt->p_pkt); /* Done with Obex packet */
    p_cb->p_cback(event, (tBTA_FTC *)&status);

    /* If successful, initiate a directory listing */
    if (p_evt->rsp_code == OBX_RSP_OK && p_bta_ft_cfg->auto_file_list &&
        (p_cb->sdp_service == UUID_SERVCLASS_OBEX_FILE_TRANSFER))
    {
        bta_ftc_get_listing(p_cb, ".", NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_rsp_timeout
**
** Description      Process the OBX response timeout event
**                  stop and abort
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_rsp_timeout(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    p_cb->timer_oper = FTC_TIMER_OP_STOP;
    if (p_cb->timer_oper == FTC_TIMER_OP_ABORT)
    {
        /* Start stop response timer */
        p_cb->status = BTA_FTC_ABORTED;
        bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT,
                            p_bta_ft_cfg->stop_tout);

        OBX_DisconnectReq(p_cb->obx_handle, NULL);
    }
    else    /* Timeout waiting for disconnect response */
    {
        p_cb->cout_active = FALSE;
        bta_ftc_initialize(p_cb, p_data);
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
        BTA_BicFreeXmlData(BTA_BIC_CAPABILITIES_EVT, (tBTA_BI_XML **)&p_cb->p_caps);
#endif
        /* force OBX to close port */
        OBX_DisconnectReq(p_cb->obx_handle, NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_initialize
**
** Description      Initialize the control block.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_initialize(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FS_CO_STATUS   status;

    bta_sys_stop_timer(&p_cb->rsp_timer);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;

        /* Delete an aborted unfinished get file operation */
        if (p_cb->obx_oper == FTC_OP_GET_FILE && p_cb->suspending == FALSE)
        {
            status = bta_fs_co_unlink(p_cb->p_name, p_cb->app_id);
            APPL_TRACE_WARNING2("FTC: Remove ABORTED Get File Operation [%s], status 0x%02x",
                                p_cb->p_name, status);
        }
    }

    /* Clean up control block */
    ftc_reset_cb(p_cb);
    p_cb->sdp_service = 0;
    p_cb->sdp_pending = FALSE;
    p_cb->suspending = FALSE;
    p_cb->p_sess_info = NULL;

    if (p_cb->disabling)
    {
        if (p_cb->sdp_handle)
        {
            SDP_DeleteRecord(p_cb->sdp_handle);
            p_cb->sdp_handle = 0;
            bta_sys_remove_uuid( UUID_SERVCLASS_PBAP_PCE );
        }
        p_cb->is_enabled = FALSE;
        p_cb->disabling = FALSE;
        bta_ftc_sm_execute(p_cb, BTA_FTC_DISABLE_CMPL_EVT, NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_stop_client
**
** Description      Stop OBX client.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_stop_client(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    INT32 timeout;

    APPL_TRACE_DEBUG1("bta_ftc_stop_client suspend:%d", p_data->api_close.suspend);
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
    if (p_bta_ft_cfg->p_bi_action && 
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_DEREGISTER_IDX] &&
        p_cb->bic_handle != BTA_BIC_INVALID_HANDLE)
    {
        (p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_DEREGISTER_IDX])(p_cb, p_data);
    }
    else if (!p_cb->sdp_pending)
#else
    if (!p_cb->sdp_pending)
#endif
    {
        if (p_data->api_close.suspend)
        {
            timeout = p_bta_ft_cfg->suspend_tout;
            p_cb->timer_oper = FTC_TIMER_OP_SUSPEND;
            p_cb->suspending = TRUE;
            OBX_SessionReq (p_cb->obx_handle, OBX_SESS_OP_SUSPEND, 0);
            p_cb->state = BTA_FTC_SUSPENDING_ST;

        }
        else
        {
            bta_fs_co_session_info(p_cb->bd_addr, p_cb->p_sess_info, 0, BTA_FS_CO_SESS_ST_NONE, NULL, NULL, p_cb->app_id);

            timeout = p_bta_ft_cfg->stop_tout;
            p_cb->timer_oper = FTC_TIMER_OP_STOP;
            OBX_DisconnectReq(p_cb->obx_handle, NULL);
        }
        /* Start stop response timer */
        bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT, timeout );
    }
}

/*******************************************************************************
**
** Function         bta_ftc_close
**
** Description      If not waiting for a call-in function, complete the closing
**                  of the channel.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_close(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    /* finished if not waiting on a call-in function */
    if (!p_cb->sdp_pending && !p_cb->cout_active)
        bta_ftc_sm_execute(p_cb, BTA_FTC_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_ftc_close
**
** Description      If not waiting for a call-in function, complete the closing
**                  of the channel.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_open_fail(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    bta_ftc_close(p_cb, p_data);
}

/*******************************************************************************
**
** Function         bta_ftc_start_client
**
** Description      Start an FTP or OPP client.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_start_client(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tOBX_STATUS     status;
    UINT16          mtu = OBX_MAX_MTU;
    BOOLEAN         srm = p_cb->srm;
    UINT32          nonce = p_cb->nonce;
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;

    p_cb->status = BTA_FTC_OK;
    p_cb->p_sess_info = NULL;

    if ( p_bta_ft_cfg->p_bi_action && 
         p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_REGISTER_IDX] &&
         p_cb->sdp_service == UUID_SERVCLASS_IMAGING_RESPONDER)
    {
        (p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_REGISTER_IDX])(p_cb, p_data);
        return;
    }

    /* If using OBEX 1.5 */
    if (p_cb->nonce == BTA_FTC_RESUME_NONCE)
    {
        APPL_TRACE_DEBUG1("bta_ftc_start_client psm:0x%x", p_data->sdp_ok.psm);
        p_cb->psm = p_data->sdp_ok.psm;
        bta_fs_co_resume (BTA_FTC_CI_SESSION_EVT, p_cb->app_id);
    }
    /* Allocate an OBX packet */
    else
    {
        if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(OBX_HANDLE_NULL, OBX_CMD_POOL_SIZE)) != NULL)
        {
            p_cb->version = p_data->sdp_ok.version;

            /* Add the target header if connecting to FTP service */
            if (p_cb->sdp_service == UUID_SERVCLASS_OBEX_FILE_TRANSFER)
            {
                OBX_AddTargetHdr(p_obx->p_pkt, (UINT8 *)BTA_FTC_FOLDER_BROWSING_TARGET_UUID,
                                 BTA_FTC_UUID_LENGTH);
            }
            /* Add the target header if connecting to PBAP service */
            else if (p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE)
            {
                OBX_AddTargetHdr(p_obx->p_pkt, (UINT8 *)BTA_FTC_PB_ACCESS_TARGET_UUID,
                                 BTA_FTC_UUID_LENGTH);
            }

            status = OBX_AllocSession (NULL, p_data->sdp_ok.scn, &p_data->sdp_ok.psm,
                                  bta_ftc_obx_cback, &p_cb->obx_handle);

            /* set security level */
            if (p_data->sdp_ok.scn)
            {
                BTM_SetSecurityLevel (TRUE, "BTA_FTC", BTM_SEC_SERVICE_OBEX_FTP,
                                      p_cb->sec_mask, BT_PSM_RFCOMM,
                                      BTM_SEC_PROTO_RFCOMM, p_data->sdp_ok.scn);
                srm = 0;
                nonce = 0;
            }
            else
            {
                BTM_SetSecurityLevel (TRUE, "BTA_FTC", BTM_SEC_SERVICE_OBEX_FTP,
                                      p_cb->sec_mask, p_data->sdp_ok.psm, 0, 0);
            }

            if (status == OBX_SUCCESS)
            {
                OBX_CreateSession (p_cb->bd_addr, mtu, srm, nonce,
                                   p_cb->obx_handle, p_obx->p_pkt);
                p_obx->p_pkt = NULL;    /* OBX will free the memory */
                return;
            }
            /* else stay in this function to report failure */
            return;
        }
        else
        {
            p_cb->status = BTA_FTC_OBX_ERR;
            p_data->obx_evt.rsp_code = OBX_RSP_FAILED;
            bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CLOSE_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_ftc_free_db
**
** Description      Free buffer used for service discovery database.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_free_db(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    utl_freebuf((void**)&p_cb->p_db);
    p_cb->sdp_pending = FALSE;
}

/*******************************************************************************
**
** Function         bta_ftc_ignore_obx
**
** Description      Free OBX packet for ignored OBX events.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_ignore_obx(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    utl_freebuf((void**)&p_data->obx_evt.p_pkt);
}

/*******************************************************************************
**
** Function         bta_ftc_find_service
**
** Description      Perform service discovery to find OPP and/or FTP services on
**                  peer device. If the service ID is both FTP and OPP then FTP 
**                  is tried first.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_find_service(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tSDP_UUID   uuid_list;
    UINT16      attr_list[5];
    UINT16      num_attrs = 4;
    tBTA_SERVICE_MASK   ftc_services = (BTA_FTP_SERVICE_MASK | BTA_OPP_SERVICE_MASK
                                        | BTA_BIP_SERVICE_MASK | BTA_PBAP_SERVICE_MASK);
    
    APPL_TRACE_DEBUG1("bta_ftc_find_service event:0x%x", BTA_FTC_API_OPEN_EVT);

    /* Make sure at least one service was specified */
    if (p_cb->services & ftc_services)
    {
        if ((p_cb->p_db = (tSDP_DISCOVERY_DB *) GKI_getbuf(BTA_FTC_DISC_SIZE)) != NULL)
        {
            p_cb->status = BTA_FTC_OK;

            attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
            attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;
            attr_list[2] = ATTR_ID_BT_PROFILE_DESC_LIST;
            attr_list[3] = ATTR_ID_OBX_OVR_L2CAP_PSM;

            uuid_list.len = LEN_UUID_16;

            /* Try FTP if its service is desired */
            if (p_cb->services & BTA_FTP_SERVICE_MASK)
            {
                p_cb->services &= ~BTA_FTP_SERVICE_MASK;
                p_cb->sdp_service = UUID_SERVCLASS_OBEX_FILE_TRANSFER;
                uuid_list.uu.uuid16 = UUID_SERVCLASS_OBEX_FILE_TRANSFER;
            }
            else if (p_cb->services & BTA_PBAP_SERVICE_MASK)
            {
                p_cb->services &= ~BTA_PBAP_SERVICE_MASK;
                p_cb->sdp_service = UUID_SERVCLASS_PBAP_PSE;
                uuid_list.uu.uuid16 = UUID_SERVCLASS_PBAP_PSE;
            }
            else if (p_cb->services & BTA_OPP_SERVICE_MASK)
            {
                p_cb->services &= ~BTA_OPP_SERVICE_MASK;
                p_cb->sdp_service = UUID_SERVCLASS_OBEX_OBJECT_PUSH;
                uuid_list.uu.uuid16 = UUID_SERVCLASS_OBEX_OBJECT_PUSH;
            }
            else if (p_bta_ft_cfg->p_bi_action)
            {
                /* Try the basic imaging service */
                p_cb->services &= ~BTA_BIP_SERVICE_MASK;
                p_cb->sdp_service = UUID_SERVCLASS_IMAGING_RESPONDER;
                uuid_list.uu.uuid16 = UUID_SERVCLASS_IMAGING_RESPONDER;
                attr_list[num_attrs++] = ATTR_ID_SUPPORTED_FEATURES;
            }

            SDP_InitDiscoveryDb(p_cb->p_db, BTA_FTC_DISC_SIZE, 1, &uuid_list, num_attrs, attr_list);
            if (!SDP_ServiceSearchAttributeRequest(p_cb->bd_addr, p_cb->p_db, bta_ftc_sdp_cback))
            {
                p_cb->status = BTA_FTC_SDP_ERR;
                bta_ftc_sm_execute(p_cb, BTA_FTC_SDP_FAIL_EVT, p_data);

            }

            p_cb->sdp_pending = TRUE;
        }
    }
    else    /* No services available */
    {
        p_cb->status = BTA_FTC_SERVICE_UNAVL;
        bta_ftc_sm_execute(p_cb, BTA_FTC_SDP_FAIL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_close_complete
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_close_complete(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC    param;

    param.status    = p_cb->status;
    p_cb->cout_active = FALSE;
    bta_ftc_initialize(p_cb, p_data);

    if (p_bta_ft_cfg->p_bi_action && p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_FREEXML_IDX])
        (p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_FREEXML_IDX])(p_cb, p_data);

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_FTC ,p_cb->app_id, p_cb->bd_addr);
    bta_fs_co_session_info(p_cb->bd_addr, p_cb->p_sess_info, 0, BTA_FS_CO_SESS_ST_NONE, NULL, NULL, p_cb->app_id);

    p_cb->p_cback(BTA_FTC_CLOSE_EVT, &param);
}

/*******************************************************************************
**
** Function         bta_ftc_set_disable
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_set_disable(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    p_cb->disabling = TRUE;
}

/*******************************************************************************
**
** Function         bta_ftc_bic_put
**
** Description      Initiate a connection with a peer device's service.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_bic_put(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    if (p_bta_ft_cfg->p_bi_action &&
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_PUT_IDX])
    {
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_PUT_IDX](p_cb, p_data);
    }
    return;
}

/*******************************************************************************
**
** Function         bta_ftc_bic_abort
**
** Description      Initiate a connection with a peer device's service.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_bic_abort(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    if (p_bta_ft_cfg->p_bi_action &&
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_ABORT_IDX])
    {
        p_bta_ft_cfg->p_bi_action[BTA_FTC_BI_ABORT_IDX](p_cb, p_data);
    }
    return;
}

/*****************************************************************************
**  Callback Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_ftc_obx_cback
**
** Description      OBX callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event, UINT8 rsp_code,
                        tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_FTC_OBX_EVT *p_obx_msg;
    UINT16              event = 0;
    UINT16              size = sizeof(tBTA_FTC_OBX_EVT);

#if BTA_FTC_DEBUG == TRUE
    APPL_TRACE_DEBUG2("OBX Event Callback: obx_event [%s] 0x%x", ftc_obx_evt_code(obx_event), p_pkt);
#endif

    switch (obx_event)
    {
    case OBX_SESSION_INFO_EVT:
        size += OBX_SESSION_INFO_SIZE;
        /* Falls through */

    case OBX_SESSION_RSP_EVT:
    case OBX_CONNECT_RSP_EVT:
        if (rsp_code == OBX_RSP_OK)
        {
            event = BTA_FTC_OBX_CONN_RSP_EVT;
        }
        else    /* Obex will disconnect underneath BTA */
        {
            APPL_TRACE_WARNING1("FTC_CBACK: Bad connect response 0x%02x", rsp_code);
            if (p_pkt)
                GKI_freebuf(p_pkt);
            return;
        }
        break;

    case OBX_ACTION_RSP_EVT:
        event = BTA_FTC_OBX_ACTION_RSP_EVT;
        break;

    case OBX_PUT_RSP_EVT:
        event = BTA_FTC_OBX_PUT_RSP_EVT;
        break;
    case OBX_GET_RSP_EVT:
        event = BTA_FTC_OBX_GET_RSP_EVT;
        break;
    case OBX_SETPATH_RSP_EVT:
        event = BTA_FTC_OBX_SETPATH_RSP_EVT;
        break;
    case OBX_ABORT_RSP_EVT:
        event = BTA_FTC_OBX_ABORT_RSP_EVT;
        break;
    case OBX_CLOSE_IND_EVT:
        event = BTA_FTC_OBX_CLOSE_EVT;
        break;
    case OBX_TIMEOUT_EVT:
        event = BTA_FTC_OBX_TOUT_EVT;
        break;
    case OBX_PASSWORD_EVT:
        event = BTA_FTC_OBX_PASSWORD_EVT;
        break;

    default:
    /*  case OBX_DISCONNECT_RSP_EVT: Handled when OBX_CLOSE_IND_EVT arrives */
        if (p_pkt)
            GKI_freebuf(p_pkt);
        return;

    }

    /* send event to BTA, if any */
    if ((p_obx_msg = (tBTA_FTC_OBX_EVT *) GKI_getbuf(size)) != NULL)
    {
        APPL_TRACE_DEBUG2("obx_event [%d] event:0x%x", obx_event, event);
        p_obx_msg->hdr.event = event;
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
        p_obx_msg->rsp_code = rsp_code;
        p_obx_msg->param = param;
        p_obx_msg->p_pkt = p_pkt;

        if (obx_event == OBX_SESSION_INFO_EVT)
        {
            p_obx_msg->param.sess.p_sess_info = (UINT8 *)(p_obx_msg + 1);
            memcpy (p_obx_msg->param.sess.p_sess_info, param.sess.p_sess_info, OBX_SESSION_INFO_SIZE);
        }

        bta_sys_sendmsg(p_obx_msg);
    }
}


/******************************************************************************
**
** Function         bta_ftc_sdp_cback
**
** Description      This is the SDP callback function used by FTC.
**                  This function will be executed by SDP when the service
**                  search is completed.  If the search is successful, it
**                  finds the first record in the database that matches the
**                  UUID of the search.  Then retrieves the scn from the
**                  record.
**
** Returns          Nothing.
**
******************************************************************************/
static void bta_ftc_sdp_cback(UINT16 status)
{
    tBTA_FTC_CB         *p_cb = &bta_ftc_cb;
    tBTA_FTC_SDP_OK_EVT *p_buf;
    tSDP_DISC_REC       *p_rec = NULL;
    tSDP_PROTOCOL_ELEM   pe;
    UINT8                scn = 0;
    UINT16               psm = 0;
    UINT16               version = GOEP_LEGACY_VERSION;
    BOOLEAN              found = FALSE;
    tSDP_DISC_ATTR      *p_attr;
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
    tBIP_FEATURE_FLAGS   features = 0;
#endif

    APPL_TRACE_DEBUG1("bta_ftc_sdp_cback status:%d", status);

    if ( (status == SDP_SUCCESS) || (status == SDP_DB_FULL) )
    {
        status = SDP_SUCCESS;
        /* loop through all records we found */
        do
        {
            /* get next record; if none found, we're done */
            if ((p_rec = SDP_FindServiceInDb(p_cb->p_db, p_cb->sdp_service,
                                             p_rec)) == NULL)
                break;

            /* this is an optional attribute */
            SDP_FindProfileVersionInRec (p_rec, p_cb->sdp_service, &version);

            /* get psm from proto desc list alternative; if not found, go to next record */
            if ((p_attr = SDP_FindAttributeInRec(p_rec,
                            ATTR_ID_OBX_OVR_L2CAP_PSM)) != NULL)
            {
                psm = p_attr->attr_value.v.u16;
                APPL_TRACE_DEBUG1("attr_len_type: x%x", p_attr->attr_len_type);
                if ((SDP_DISC_ATTR_LEN(p_attr->attr_len_type) == 2) && L2C_IS_VALID_PSM(psm))
                {
                    found = TRUE;
                    if (version == GOEP_LEGACY_VERSION)
                    {
                        APPL_TRACE_ERROR0("Lacking mandatory attribute/version");
                        version = BTA_FT_ENHANCED_VERSION;
                    }
                    break;
                }
            }

            if (!found)
            {
                /* get scn from proto desc list; if not found, go to next record */
                if (SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe))
                {
                    if ( p_bta_ft_cfg->p_bi_action && p_cb->sdp_service == UUID_SERVCLASS_IMAGING_RESPONDER)
                    {
                        /* Check if the Push feature is supported */
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
                        if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SUPPORTED_FEATURES)) != NULL)
                        {
                            features = p_attr->attr_value.v.u16;
                            APPL_TRACE_DEBUG1("supported BIP features: x%x", features);
                            if (features & BIP_FT_IMG_PUSH_FLAGS)
                            {
                                found = TRUE;
                            }
                        }
#endif
                    }
                    else
                    {
                        found = TRUE;
                    }
                    scn = (UINT8) pe.params[0];

                    /* we've got everything, we're done */
                    break;
                }
                else
                {
                    continue;
                }
            }
        } while (TRUE);
    }

    APPL_TRACE_DEBUG4("found: %d scn:%d, psm:0x%x version:0x%x",found , scn, psm, version);

    /* send result in event back to BTA */
    if ((p_buf = (tBTA_FTC_SDP_OK_EVT *) GKI_getbuf(sizeof(tBTA_FTC_SDP_OK_EVT))) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_SDP_FAIL_EVT;
        p_cb->status = BTA_FTC_SERVICE_UNAVL;

        if (status == SDP_SUCCESS)
        {
            if (found == TRUE)
            {
                p_buf->hdr.event = BTA_FTC_SDP_OK_EVT;
                p_buf->scn = scn;
                p_buf->psm = psm;
                p_buf->version = version;
            }
            /* See if OPP service needs to be searched for */
            else if (p_cb->services & (BTA_OPP_SERVICE_MASK|BTA_BIP_SERVICE_MASK))
            {
                p_buf->hdr.event = BTA_FTC_SDP_NEXT_EVT;
            }
        }

        bta_sys_sendmsg(p_buf);
    }
}

/*****************************************************************************
**  Local FTP Event Processing Functions
*****************************************************************************/
static void ftc_reset_cb (tBTA_FTC_CB *p_cb)
{
    /* Clean up control block */
    utl_freebuf((void**)&p_cb->obx.p_pkt);
    utl_freebuf((void**)(BT_HDR **)&p_cb->p_name);
    p_cb->obx_oper = FTC_OP_NONE;
    p_cb->aborting = 0;
    p_cb->int_abort = FALSE;
    p_cb->req_pending = FALSE;
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_FTC_DEBUG == TRUE

/*******************************************************************************
**
** Function         ftc_obx_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ftc_obx_evt_code(tOBX_EVENT evt_code)
{
    switch (evt_code)
    {
    case OBX_CONNECT_RSP_EVT:
        return "OBX_CONNECT_RSP_EVT";
    case OBX_DISCONNECT_RSP_EVT:
        return "OBX_DISCONNECT_RSP_EVT";
    case OBX_PUT_RSP_EVT:
        return "OBX_PUT_RSP_EVT";
    case OBX_GET_RSP_EVT:
        return "OBX_GET_RSP_EVT";
    case OBX_SETPATH_RSP_EVT:
        return "OBX_SETPATH_RSP_EVT";
    case OBX_ABORT_RSP_EVT:
        return "OBX_ABORT_RSP_EVT";
    case OBX_CLOSE_IND_EVT:
        return "OBX_CLOSE_IND_EVT";
    case OBX_TIMEOUT_EVT:
        return "OBX_TIMEOUT_EVT";
    case OBX_PASSWORD_EVT:
        return "OBX_PASSWORD_EVT";
    case OBX_SESSION_RSP_EVT:
        return "OBX_SESSION_RSP_EVT";
    case OBX_ACTION_RSP_EVT:
        return "OBX_ACTION_RSP_EVT";
    case OBX_SESSION_INFO_EVT:
        return "OBX_SESSION_INFO_EVT";
    default:
            return "unknown OBX event code";
    }
}
#endif  /* Debug Functions */
#endif /* BTA_FT_INCLUDED */
