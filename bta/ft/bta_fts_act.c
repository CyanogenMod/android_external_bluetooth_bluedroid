/*****************************************************************************
**
**  Name:           bta_fts_act.c
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

#include <stdio.h>
#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_ft_api.h"
#include "bta_fts_int.h"
#include "bta_fs_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "btm_api.h"
#include "utl.h"
#include "bd.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
#define FLAGS_ARE_MASK      (OBX_SPF_BACKUP | OBX_SPF_NO_CREATE)
#define FLAGS_ARE_ILLEGAL   0x1 /* 'Backup and Create flag combo is BAD */

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if BTA_FTS_DEBUG == TRUE
static char *fts_obx_evt_code(tOBX_EVENT evt_code);
#endif

/*****************************************************************************
**  Action Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_fts_api_disable
**
** Description      Stop FTP server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_api_disable(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{

    /* If callout is active, wait till finished before shutting down */
    if (p_cb->cout_active)
        p_cb->disabling = TRUE;
    else
        bta_fts_sm_execute(p_cb, BTA_FTS_DISABLE_CMPL_EVT, p_data);

    bta_sys_remove_uuid(UUID_SERVCLASS_OBEX_FILE_TRANSFER);
    BTM_SecClrService(BTM_SEC_SERVICE_OBEX_FTP);
}

/*******************************************************************************
**
** Function         bta_fts_api_authrsp
**
** Description      Pass the response to an authentication request back to the
**                  client.
**
** Returns          void
**
*******************************************************************************/
void bta_fts_api_authrsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    UINT8 *p_pwd = NULL;
    UINT8 *p_userid = NULL;

    if (p_data->auth_rsp.key_len > 0)
        p_pwd = (UINT8 *)p_data->auth_rsp.key;
    if (p_data->auth_rsp.userid_len > 0)
        p_userid = (UINT8 *)p_data->auth_rsp.userid;

    OBX_Password(p_cb->obx_handle, p_pwd, p_data->auth_rsp.key_len, 
                 p_userid, p_data->auth_rsp.userid_len);
}

/*******************************************************************************
**
** Function         bta_fts_api_accessrsp
**
** Description      Process the access API event.
**                  If permission had been granted, continue the PUT operation,
**                  otherwise stop the operation.
**
** Returns          void
**
*******************************************************************************/
void bta_fts_api_accessrsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    UINT8             rsp_code = OBX_RSP_OK;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_EACCES;
    tBTA_FT_ACCESS    access = p_data->access_rsp.flag;
    tBTA_FT_OPER      old_acc_active = p_cb->acc_active;
    tBTA_FTS_OBX_RSP  *p_rsp = NULL;
    tBTA_FTS_OBJECT   objevt;

    APPL_TRACE_DEBUG3("bta_fts_api_accessrsp op:%d/%d access:%d", old_acc_active, p_data->access_rsp.oper, access);
    if(p_cb->acc_active != p_data->access_rsp.oper )
    {
        APPL_TRACE_WARNING2("FTS ACCRSP: not match active:%d, rsp:%d",
            p_cb->acc_active, p_data->access_rsp.oper);
        return;
    }

    p_cb->acc_active = 0;
    /* Process the currently active access response */
    switch (old_acc_active)
    {
    case BTA_FT_OPER_PUT:
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            /* Save the file name with path prepended */
            BCM_STRNCPY_S(p_cb->p_path, p_bta_fs_cfg->max_file_len+1, p_data->access_rsp.p_name, p_bta_fs_cfg->max_file_len);
            p_cb->p_path[p_bta_fs_cfg->max_file_len] = '\0';

            p_cb->cout_active = TRUE;
            bta_fs_co_open (p_cb->p_path,
                            (BTA_FS_O_CREAT | BTA_FS_O_TRUNC | BTA_FS_O_RDWR),
                            p_cb->file_length, BTA_FTS_CI_OPEN_EVT,
                            p_cb->app_id);
        }
        else    /* Access denied */
        {
            bta_fts_clean_getput(p_cb, TRUE);
            OBX_PutRsp(p_cb->obx_handle, OBX_RSP_UNAUTHORIZED, (BT_HDR *)NULL);
        }
        break;

    case BTA_FT_OPER_GET:
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            p_cb->cout_active = TRUE;
            bta_fs_co_open (p_cb->p_path, BTA_FS_O_RDONLY, 0,
                            BTA_FTS_CI_OPEN_EVT, p_cb->app_id);
        }
        else    /* Denied */
            bta_fts_get_file_rsp(OBX_RSP_UNAUTHORIZED, 0);
        break;

    case BTA_FT_OPER_DEL_FILE:   /* Request is a DELETE file */
        p_rsp = OBX_PutRsp;
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            objevt.p_name = p_cb->p_path;
            objevt.status = BTA_FTS_OK;
            if ((status = bta_fs_co_unlink(p_cb->p_path, p_cb->app_id)) != BTA_FS_CO_OK)
            {
                objevt.status = BTA_FTS_FAIL;
            }

            /* Notify application of delete attempt */
            p_cb->p_cback(BTA_FTS_DEL_CMPL_EVT, (tBTA_FTS *)&objevt);
        }
        break;

    case BTA_FT_OPER_DEL_DIR:    /* Request is a DELETE folder */
        p_rsp = OBX_PutRsp;
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            objevt.p_name = p_cb->p_path;
            objevt.status = BTA_FTS_OK;
            if ((status = bta_fs_co_rmdir(p_cb->p_path, p_cb->app_id)) != BTA_FS_CO_OK)
            {
                objevt.status = BTA_FTS_FAIL;
            }

            /* Notify application of delete attempt */
            p_cb->p_cback(BTA_FTS_DEL_CMPL_EVT, (tBTA_FTS *)&objevt);
        }
        break;

    case BTA_FT_OPER_CHG_DIR:       /* Request is a Change Folder */
        p_rsp = OBX_SetPathRsp;
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            status = BTA_FS_CO_OK;
            BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_path, p_bta_fs_cfg->max_path_len);
            p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';
            APPL_TRACE_DEBUG1("FTS: SET NEW PATH [%s]", p_cb->p_workdir);
            bta_fs_co_setdir(p_cb->p_workdir, p_cb->app_id);
        }
        break;

    case BTA_FT_OPER_MK_DIR:        /* Request is a Make Folder */
        p_rsp = OBX_SetPathRsp;
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            if ((status = bta_fs_co_mkdir(p_cb->p_path, p_cb->app_id)) == BTA_FS_CO_OK)
            {
                BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_path, p_bta_fs_cfg->max_path_len);
                p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';
                APPL_TRACE_DEBUG1("FTS: SET NEW PATH [%s]", p_cb->p_workdir);
                bta_fs_co_setdir(p_cb->p_workdir, p_cb->app_id);
            }
        }
        break;

    case BTA_FT_OPER_COPY_ACT:
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            p_cb->cout_active = TRUE;
            bta_fs_co_copy(p_cb->p_path, p_cb->p_dest, p_cb->perms, BTA_FTS_CI_OPEN_EVT, p_cb->app_id);
        }
        else
            p_rsp = OBX_ActionRsp;
        break;

    case BTA_FT_OPER_MOVE_ACT:
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            p_cb->cout_active = TRUE;
            bta_fs_co_rename(p_cb->p_path, p_cb->p_dest, p_cb->perms, BTA_FTS_CI_OPEN_EVT, p_cb->app_id);
        }
        else
            p_rsp = OBX_ActionRsp;
        break;

    case BTA_FT_OPER_SET_PERM:
        if (access == BTA_FT_ACCESS_ALLOW)
        {
            p_cb->cout_active = TRUE;
            bta_fs_co_set_perms(p_cb->p_path, p_cb->perms, BTA_FTS_CI_OPEN_EVT, p_cb->app_id);
        }
        else
            p_rsp = OBX_ActionRsp;
        break;

    default:
        p_cb->acc_active = old_acc_active;
        APPL_TRACE_WARNING1("FTS ACCRSP: Unknown tBTA_FT_OPER value (%d)",
                            p_cb->acc_active);
    }
    if(p_rsp)
    {
        switch (status)
        {
        case BTA_FS_CO_OK:
            rsp_code = OBX_RSP_OK;
            break;
        case BTA_FS_CO_ENOTEMPTY:
            rsp_code = OBX_RSP_PRECONDTN_FAILED;
            break;
        case BTA_FS_CO_EACCES:
            rsp_code = OBX_RSP_UNAUTHORIZED;
            break;
        default:
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
        utl_freebuf((void**)&p_cb->p_dest);

        p_cb->obx_oper = FTS_OP_NONE;
        (*p_rsp)(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_fts_api_close
**
** Description      Handle an api close event.  
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_api_close(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    BD_ADDR bd_addr;
    if (OBX_GetPeerAddr(p_cb->obx_handle, bd_addr) != 0)
    {
        /* resources will be freed at BTA_PBS_OBX_CLOSE_EVT */
        OBX_DisconnectRsp(p_cb->obx_handle, OBX_RSP_SERVICE_UNAVL, NULL);
    }
    else
    {
        p_cb->p_cback(BTA_FTS_CLOSE_EVT, 0);
    }
}

/*******************************************************************************
**
** Function         bta_fts_ci_write
**
** Description      Continue with the current write operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_ci_write(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    UINT8 rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    
    p_cb->cout_active = FALSE;
    
    if (p_cb->disabling)
    {
        bta_fts_sm_execute(p_cb,BTA_FTS_DISABLE_CMPL_EVT, p_data);
    }
    
    /* Process write call-in event if operation is still active */
    if (p_cb->obx_oper == FTS_OP_PUT_FILE)
    {
        if (p_data->write_evt.status == BTA_FS_CO_OK)
            rsp_code = OBX_RSP_OK;
        else
        {
            if (p_data->write_evt.status == BTA_FS_CO_ENOSPACE)
                rsp_code = OBX_RSP_DATABASE_FULL;
            bta_fts_clean_getput(p_cb, TRUE);
        }
        
        /* Process response to OBX client */
        bta_fts_put_file_rsp(rsp_code);
    }
}

/*******************************************************************************
**
** Function         bta_fts_ci_read
**
** Description      Handles the response to a read call-out request.
**                  This is called within the OBX get file request.  If the
**                  operation has completed, the OBX response is sent out;
**                  otherwise a read for additional data is made.
**
** Returns          void
**
*******************************************************************************/
void bta_fts_ci_read(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FS_CI_READ_EVT *p_revt = &p_data->read_evt;
    UINT8 rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    p_cb->cout_active = FALSE;

    if (p_cb->disabling)
    {
        bta_fts_sm_execute(p_cb,BTA_FTS_DISABLE_CMPL_EVT, p_data);
    }

    /* Process read call-in event if operation is still active */
    if (p_cb->obx_oper == FTS_OP_GET_FILE && p_revt->fd == p_cb->fd)
    {
        /* Read was successful, not finished yet */
        if (p_revt->status == BTA_FS_CO_OK)
            rsp_code = OBX_RSP_CONTINUE;

        /* Read was successful, end of file has been detected */
        else if (p_revt->status == BTA_FS_CO_EOF)
            rsp_code = OBX_RSP_OK;

        /* Process response to OBX client */
        bta_fts_get_file_rsp(rsp_code, p_revt->num_read);
    }
}

/*******************************************************************************
**
** Function         bta_fts_ci_resume
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_ci_resume(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_UTL_COD         cod;

    p_cb->state = BTA_FTS_LISTEN_ST;
    APPL_TRACE_EVENT1("bta_fts_ci_resume status:%d", p_data->resume_evt.status);
    /* Set the File Transfer service class bit */
    cod.service = BTM_COD_SERVICE_OBJ_TRANSFER;
    utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);
    if (p_data->resume_evt.status == BTA_FS_CO_OK)
    {
        OBX_AddSuspendedSession (p_cb->obx_handle, p_data->resume_evt.p_addr, 
            p_data->resume_evt.p_sess_info, p_data->resume_evt.timeout,
            p_data->resume_evt.ssn, p_data->resume_evt.offset);
    }
}

/*******************************************************************************
**
** Function         bta_fts_ci_open
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_ci_open(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FS_CI_OPEN_EVT *p_open = &p_data->open_evt;
    UINT8                rsp_code = OBX_RSP_OK;
    tBTA_FTS_OBX_PKT    *p_obx = &p_cb->obx;
    UINT8                num_hdrs;
    BOOLEAN              endpkt;

    p_cb->cout_active = FALSE;
    
    if (p_cb->disabling)
    {
        bta_fts_sm_execute(p_cb,BTA_FTS_DISABLE_CMPL_EVT, p_data);
    }

    /* If using OBEX 1.5 */
    if ( p_cb->obx_oper & FTS_OP_RESUME)
    {
        /* clear the FTS_OP_RESUME when the first request is received. 
         * We may need to adjust ssn/offset */
        if ((p_open->fd >= 0) && (p_cb->obx_oper == FTS_OP_RESUME))
        {
            /* file is open successfully when the resumed obex op is FTS_OP_NONE
             * close this file */
            bta_fs_co_close(p_open->fd, p_cb->app_id);
            p_open->fd = BTA_FS_INVALID_FD;
        }
        p_cb->fd = p_open->fd;
        return;
    }

    /* Only process file get or put operations */
    if (p_cb->obx_oper == FTS_OP_GET_FILE)
    {
        /* if file is accessible read/write the first buffer of data */
        if (p_open->status == BTA_FS_CO_OK)
        {
            p_cb->file_length = p_open->file_size;
            p_cb->fd = p_open->fd;

            /* Add the length header if available */
            if (p_cb->file_length != BTA_FS_LEN_UNKNOWN)
            {
                OBX_AddLengthHdr(p_obx->p_pkt, p_cb->file_length);
                if (p_cb->file_length > 0)
                    rsp_code = OBX_RSP_CONTINUE;
                else
                    OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
            }

            /* Send continuation response with the length of the file and no body */
            OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
            p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */

            if (p_cb->file_length == 0)
               bta_fts_clean_getput(p_cb, FALSE);
        }
        else
        {
            if (p_open->status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else    /* File could not be found */
                rsp_code = OBX_RSP_NOT_FOUND;
            
            /* Send OBX response if an error occurred */
            bta_fts_get_file_rsp(rsp_code, 0);
        }
    }
    else if (p_cb->obx_oper == FTS_OP_PUT_FILE)
    {
        /* if file is accessible read/write the first buffer of data */
        if (p_open->status == BTA_FS_CO_OK)
        {
            p_cb->fd = p_open->fd;

            /* Read in start of body if there is a body header */
            num_hdrs = OBX_ReadBodyHdr(p_obx->p_pkt, &p_obx->p_start,
                                       &p_obx->bytes_left, &endpkt);
            if (num_hdrs == 1)
            {
                rsp_code = OBX_RSP_PART_CONTENT;   /* Do not send OBX response yet */
                /* Initiate the writing out of the data */
                p_cb->cout_active = TRUE;
                bta_fs_co_write(p_cb->fd, &p_obx->p_start[p_obx->offset],
                    p_obx->bytes_left, BTA_FTS_CI_WRITE_EVT, 0,
                    p_cb->app_id);
            }
            else if (num_hdrs > 1)  /* Cannot handle multiple body headers */
            {
                rsp_code = OBX_RSP_BAD_REQUEST;
                bta_fts_clean_getput(p_cb, TRUE);
            }
            else    /* No body: respond with an OK so client can start sending the data */
                p_obx->bytes_left = 0;
            
            if (rsp_code != OBX_RSP_PART_CONTENT)
            {
                bta_fts_put_file_rsp(rsp_code); 
            }
        }
        else
        {
            if (p_open->status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else if (p_open->status == BTA_FS_CO_ENOSPACE)
                rsp_code = OBX_RSP_DATABASE_FULL;
            else
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

            /* Send OBX response now if an error occurred */
            bta_fts_put_file_rsp(rsp_code);
        }
    }
    /* If using OBEX 1.5 */
    else
    {
        /* must be action commands BTA_FT_OPER_COPY_ACT - BTA_FT_OPER_SET_PERM*/
        switch (p_open->status)
        {
        case BTA_FS_CO_OK:
            rsp_code = OBX_RSP_OK;
            break;
        case BTA_FS_CO_EACCES:
            rsp_code = OBX_RSP_UNAUTHORIZED;
            break;
        case BTA_FS_CO_EIS_DIR:
            rsp_code = OBX_RSP_FORBIDDEN;
        default:
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
        utl_freebuf((void**)&p_cb->p_dest);
        p_cb->obx_oper = FTS_OP_NONE;
        OBX_ActionRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_fts_ci_direntry
**
** Description      Continue getting the current directory entry operation
**                  
** Returns          void
**
*******************************************************************************/
void bta_fts_ci_direntry(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    UINT8   rsp_code;
    BOOLEAN free_pkt = TRUE;
    
    p_cb->cout_active = FALSE;
    
    if (p_cb->disabling)
    {
        bta_fts_sm_execute(p_cb,BTA_FTS_DISABLE_CMPL_EVT, p_data);
    }

    /* Process dirent listing call-in event if operation is still active */
    if (p_cb->obx_oper == FTS_OP_LISTING)
    {
        switch (p_data->getdir_evt.status)
        {
        case BTA_FS_CO_OK:     /* Valid new entry */
            free_pkt = FALSE;
            if ((rsp_code = bta_fts_add_list_entry()) != OBX_RSP_PART_CONTENT)
                bta_fts_end_of_list(rsp_code);
            break;
            
        case BTA_FS_CO_EODIR:  /* End of list (entry not valid) */
            free_pkt = FALSE;
            bta_fts_end_of_list(OBX_RSP_OK);
            break;
            
        case BTA_FS_CO_FAIL:   /* Error occurred */
            bta_fts_end_of_list(OBX_RSP_NOT_FOUND);
            break;
        }
    }

    if (free_pkt)
        utl_freebuf((void **)&p_cb->obx.p_pkt);
}

/*******************************************************************************
**
** Function         bta_fts_obx_connect
**
** Description      Process the OBX connect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_connect(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tOBX_SESS_EVT       *p_sess = &p_data->obx_evt.param.sess;

    APPL_TRACE_DEBUG1("bta_fts_obx_connect obx_event=%d", p_evt->obx_event);
    if (p_evt->obx_event == OBX_SESSION_REQ_EVT)
    {
        APPL_TRACE_EVENT3("sess_op:%d obj_offset:x%x ssn:%d", p_sess->sess_op, p_sess->obj_offset, p_sess->ssn);

        BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
        p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';

        bta_fs_co_session_info(p_cb->bd_addr, p_sess->p_sess_info, p_sess->ssn,
            BTA_FS_CO_SESS_ST_ACTIVE, p_cb->p_workdir, &p_cb->obx_oper, p_cb->app_id);
        APPL_TRACE_DEBUG1("obx_oper:%d", p_cb->obx_oper);
        if (p_sess->sess_op == OBX_SESS_OP_CREATE)
        {
            p_cb->state = BTA_FTS_LISTEN_ST;
            p_cb->obx_oper = FTS_OP_NONE;
        }
        else if (p_sess->sess_op == OBX_SESS_OP_RESUME)
        {
            p_cb->state = BTA_FTS_LISTEN_ST;
            p_cb->resume_ssn = p_sess->ssn;
            APPL_TRACE_EVENT1("resume ssn:%d", p_sess->ssn);
            if (p_cb->obx_oper)
            {
                bta_fs_co_resume_op(p_sess->obj_offset, BTA_FTS_CI_OPEN_EVT, p_cb->app_id);
                p_cb->obx_oper |= FTS_OP_RESUME;
            }
        }
        return;
    }

    p_cb->peer_mtu = p_evt->param.conn.mtu;
    memcpy(p_cb->bd_addr, p_evt->param.conn.peer_addr, BD_ADDR_LEN);
    APPL_TRACE_EVENT2("FTS Connect: peer mtu 0x%04x handle:0x%x", p_cb->peer_mtu, p_evt->handle);

    if (!p_evt->param.conn.no_rsp)
    {
        OBX_ConnectRsp(p_evt->handle, OBX_RSP_OK, (BT_HDR *)NULL);
    
        /* Reset to the root directory */
        BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
        p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';

        bta_fs_co_setdir(p_cb->p_workdir, p_cb->app_id);
    }
    
    /* inform role manager */
    bta_sys_conn_open( BTA_ID_FTS, p_cb->app_id, p_cb->bd_addr);

    /* Notify the MMI that a connection has been opened */
    p_cb->p_cback(BTA_FTS_OPEN_EVT, (tBTA_FTS*)p_cb->bd_addr);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_fts_obx_disc
**
** Description      Process the OBX disconnect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_disc(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code;

    rsp_code = (p_evt->obx_event == OBX_DISCONNECT_REQ_EVT) ? OBX_RSP_OK
                                                            : OBX_RSP_BAD_REQUEST;
    OBX_DisconnectRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_fts_obx_close
**
** Description      Process the OBX link lost event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_close(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    /* finished if not waiting on a call-in function */
    if (!p_cb->cout_active)
        bta_fts_sm_execute(p_cb, BTA_FTS_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_fts_obx_abort
**
** Description      Process the OBX abort event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_abort(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_OK;

    utl_freebuf((void**)&p_evt->p_pkt);

    switch (p_cb->obx_oper)
    {
    case FTS_OP_LISTING:
        utl_freebuf((void**)&p_cb->obx.p_pkt);
        bta_fts_clean_list(p_cb);
        break;

    case FTS_OP_GET_FILE:
    case FTS_OP_PUT_FILE:
        bta_fts_clean_getput(p_cb, TRUE);
        break;

    default:    /* Reply OK to the client */
        rsp_code = OBX_RSP_BAD_REQUEST;
    }

    OBX_AbortRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_fts_obx_password
**
** Description      Process the OBX password request
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_password(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tBTA_FTS_AUTH       *p_auth;
    BOOLEAN              is_challenged;
    tOBX_AUTH_OPT        options;
    
    if ((p_auth = (tBTA_FTS_AUTH *)GKI_getbuf(sizeof(tBTA_FTS_AUTH))) != NULL)
    {
        memset(p_auth, 0, sizeof(tBTA_FTS_AUTH));
        
        /* Extract user id from packet (if available) */
        if (OBX_ReadAuthParams(p_data->obx_evt.p_pkt, &p_auth->p_userid,
            &p_auth->userid_len,
            &is_challenged, &options))
        {
            if (options & OBX_AO_USR_ID)
                p_auth->userid_required = TRUE;
        }
        
        /* Don't need OBX packet any longer */
        utl_freebuf((void**)&p_evt->p_pkt);
        
        /* Notify application */
        p_cb->p_cback(BTA_FTS_AUTH_EVT, (tBTA_FTS *)p_auth);
        
        GKI_freebuf(p_auth);
    }
}

/*******************************************************************************
**
** Function         bta_fts_obx_put
**
** Description      Process the OBX file put and delete file/folder events
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_put(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8    rsp_code = OBX_RSP_OK;
    BOOLEAN  rsp_now = TRUE;
    BOOLEAN  free_in_pkt = TRUE;
    UINT8    obx_oper = bta_fts_cb.obx_oper;
    tBTA_FTS_OBX_PKT *p_obx = &p_cb->obx;
    UINT8             num_hdrs;
    BOOLEAN           endpkt;
    
    APPL_TRACE_EVENT1("bta_fts_obx_put oper=x%x", p_cb->obx_oper);

    /* If using OBEX 1.5 */
    if ( p_cb->obx_oper & FTS_OP_RESUME)
    {
        APPL_TRACE_EVENT2("bta_fts_obx_put ssn:%d resume ssn:%d", p_evt->param.get.ssn, p_cb->resume_ssn);
        p_cb->obx_oper &= ~FTS_OP_RESUME;
        num_hdrs = OBX_ReadBodyHdr(p_obx->p_pkt, &p_obx->p_start,
                                   &p_obx->bytes_left, &endpkt);
        if ((p_evt->param.get.ssn != p_cb->resume_ssn) && (p_cb->fd >= 0))
        {
            if ((p_cb->obx_oper == FTS_OP_PUT_FILE) && (num_hdrs == 0) && ((p_evt->param.get.ssn + 1) == p_cb->resume_ssn))
            {
                APPL_TRACE_EVENT0("client did not get the last PUT response. Just want another response pkt");
            }
            else
                bta_fs_co_sess_ssn(p_cb->fd, p_evt->param.put.ssn, p_cb->app_id);
        }
    }

    /* If currently processing a PUT, use the current name */
    if (bta_fts_cb.obx_oper == FTS_OP_PUT_FILE)
    {
        free_in_pkt = FALSE;    /* Hang on to Obx packet until done */
        bta_fts_proc_put_file(p_evt->p_pkt, p_cb->p_name, p_evt->param.put.final, obx_oper);
        rsp_now = FALSE;    /* Response sent after processing the request */
    }
    else  /* This is a new request */
    {
        /* Pull out the name header if it exists */
        if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1))) != NULL)
        {
            if (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *) p_cb->p_name, p_bta_fs_cfg->max_file_len))
            {
                /* Is Put operation Delete, Create, or Put */
                if (p_evt->param.put.type == OBX_PT_DELETE)
                {
                    APPL_TRACE_EVENT1("FTS File/Folder Delete: Name [%s]", p_cb->p_name);
                    bta_fts_delete(p_evt, p_cb->p_name);
                    utl_freebuf((void**)&p_cb->p_name);
                }
                else    /* File Put or File Create */
                {
                    free_in_pkt = FALSE;    /* Hang on to Obx packet until done */
                    APPL_TRACE_EVENT1("FTS File Put: Name [%s]", p_cb->p_name);
                    bta_fts_proc_put_file(p_evt->p_pkt, p_cb->p_name, p_evt->param.put.final, obx_oper);
                }

                rsp_now = FALSE;    /* Response sent after processing the request */
            }
            else    /* Must have a name header */
            {
                utl_freebuf((void**)&p_cb->p_name);
                rsp_code = OBX_RSP_BAD_REQUEST;
            }
        }
        else
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    }

    /* Respond right away if an error has been detected processing request */
    if (rsp_now)
        OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);

    /* Done with Obex packet */
    if (free_in_pkt)
        utl_freebuf((void**)&p_evt->p_pkt);

}

/*******************************************************************************
**
** Function         bta_fts_obx_get
**
** Description      Process the OBX file get and folder listing events
**                  If the type header is not folder listing, then pulling a file.
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_get(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT16   len;
    BOOLEAN  is_file_get = TRUE;    /* Set to false if folder listing */
    UINT8   *p_type;
    
    APPL_TRACE_EVENT1("bta_fts_obx_get:%d", bta_fts_cb.obx_oper);

    /* If using OBEX 1.5 */
    if ( p_cb->obx_oper & FTS_OP_RESUME)
    {
        p_cb->obx_oper &= ~FTS_OP_RESUME;
        APPL_TRACE_DEBUG3("ssn:%d resume ssn:%d, op:%d", p_evt->param.get.ssn, p_cb->resume_ssn, bta_fts_cb.obx_oper );
        if (bta_fts_cb.obx_oper == FTS_OP_LISTING)
        {
            /* abort the listing */
            bta_fts_end_of_list(OBX_RSP_INTRNL_SRVR_ERR);
            return;
        }
        else if ((p_evt->param.get.ssn != p_cb->resume_ssn) && (p_cb->fd >= 0))
        {
            APPL_TRACE_EVENT2("ssn:%d resume ssn:%d", p_evt->param.get.ssn, p_cb->resume_ssn);
            bta_fs_co_sess_ssn(p_cb->fd, p_evt->param.get.ssn, p_cb->app_id);
        }
    }

    /* If currently processing a GET, use the current name */
    if (bta_fts_cb.obx_oper == FTS_OP_LISTING)
        bta_fts_getdirlist(p_cb->p_name);
    else if (bta_fts_cb.obx_oper == FTS_OP_GET_FILE)
        bta_fts_proc_get_file(p_cb->p_name);

    else  /* This is a new request */
    {
        /* Pull out the name header if it exists */
        if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1))) != NULL)
        {
            if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len))
            {
                GKI_freebuf(p_cb->p_name);
                p_cb->p_name = NULL;
            }
        }
        
        /* See if getting a folder listing */
        if (OBX_ReadTypeHdr(p_evt->p_pkt, &p_type, &len))
        {
            if (!memcmp(p_type, BTA_FTS_FOLDER_LISTING_TYPE, len))
            {
                is_file_get = FALSE;
                bta_fts_getdirlist(p_cb->p_name);
            }
        }
        
        /* Initiate the Get File request if not folder listing */
        if (is_file_get)
        {
            if (p_cb->p_name)
            {
                APPL_TRACE_EVENT1("FTS File Get: Name [%s]", p_cb->p_name);
            }
            bta_fts_proc_get_file(p_cb->p_name);
        }
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_fts_obx_setpath
**
** Description      Process the FTP change or make directory requests
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_setpath(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_BAD_REQUEST;
    tOBX_SETPATH_FLAG   *p_flag = &p_evt->param.sp.flag;
    tBTA_FT_OPER         ft_op = 0;

    /* Verify flags and handle before accepting */
    if (p_evt->handle == p_cb->obx_handle &&
        (((*p_flag) & FLAGS_ARE_MASK) != FLAGS_ARE_ILLEGAL))
    {
        p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1));
        p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len +
                                p_bta_fs_cfg->max_file_len + 2));
        if (p_cb->p_name != NULL && p_cb->p_path != NULL)
        {
            if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len))
            {
                utl_freebuf((void **)&p_cb->p_name); /* no name header */
            }
            else if (!p_cb->p_name)  
            {
                /* empty name header set to null terminated string */
                p_cb->p_name[0] = '\0';
            }


            /* Determine if operation is mkdir */
            if (!((*p_flag) & OBX_SPF_NO_CREATE))
            {
                rsp_code = bta_fts_mkdir(p_evt->p_pkt, &ft_op);

                /* If the directory already exists, silently perform change directory */
                if(rsp_code == OBX_RSP_CONTINUE)
                {
                    /* Note BACKUP flag must be FALSE for mkdir operation */
                    rsp_code = bta_fts_chdir(p_evt->p_pkt, FALSE, &ft_op);
                }
            }
            else /* Operation is chdir */
                rsp_code = bta_fts_chdir(p_evt->p_pkt, (BOOLEAN)((*p_flag) & OBX_SPF_BACKUP), &ft_op);
        }
        else
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    }

    if (ft_op)
    {
        bta_fts_req_app_access(ft_op, p_cb);
    }
    else
    {
        OBX_SetPathRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/* Using OBEX 1.5 */
/*******************************************************************************
**
** Function         bta_fts_obx_action
**
** Description      Process the FTP action requests
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_action(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_BAD_REQUEST;
    tBTA_FT_OPER         ft_op = 0;
    UINT8                action;
    BOOLEAN              is_dir;
    BOOLEAN              has_perms = FALSE;
    char                *p_dest_name = NULL;
    char                *p_newpath = NULL;

    /* Verify flags and handle before accepting */
    if (p_evt->handle == p_cb->obx_handle )
    {
        if (OBX_ReadActionIdHdr(p_evt->p_pkt, &action))
        {
            p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1));
            p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len +
                                    p_bta_fs_cfg->max_file_len + 2));
            p_newpath = p_cb->p_path;
            if ((p_cb->p_name != NULL) && (p_cb->p_path != NULL) &&
                ((strlen(p_cb->p_name) + strlen(p_cb->p_workdir) + 1) <= p_bta_fs_cfg->max_path_len))

            {
                if (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len) &&
                    p_cb->p_name[0])
                {
                    /* the packet has non-empty name header */
                    sprintf(p_newpath, "%s%c%s", p_cb->p_workdir,
                            p_bta_fs_cfg->path_separator, p_cb->p_name);
                    if (((bta_fs_co_access(p_newpath, BTA_FS_ACC_EXIST,
                        &is_dir, p_cb->app_id)) == BTA_FS_CO_OK))
                    {
                        /* the src object exists */
                        ft_op = BTA_FT_OPER_COPY_ACT + action;
                    }
                    else
                        rsp_code = OBX_RSP_NOT_FOUND;
                }

                if (ft_op)
                {
                    memset (p_cb->perms, 0, BTA_FS_PERM_SIZE);
                    /* it's SetPermission -> need Permission Header */
                    if (OBX_ReadPermissionHdr(p_evt->p_pkt, &p_cb->perms[BTA_FS_PERM_USER],
                            &p_cb->perms[BTA_FS_PERM_GROUP], &p_cb->perms[BTA_FS_PERM_OTHER]))
                    {
                        has_perms = TRUE;
                    }
                    if (action != BTA_FT_ACT_PERMISSION)
                    {
                        /* it's Move or Copy -> need DestName Header */
                        p_dest_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1));
                        p_cb->p_dest = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len +
                                                p_bta_fs_cfg->max_file_len + 2));
                        p_newpath = p_cb->p_dest;
                        if (p_dest_name && p_cb->p_dest)
                        {
                            if (OBX_ReadUtf8DestNameHdr(p_evt->p_pkt, (UINT8 *)p_dest_name, p_bta_fs_cfg->max_file_len) &&
                                p_dest_name[0])
                            {
                                if (p_dest_name[0] == '/' || p_dest_name[0] == '\\')
                                {
                                    /* the packet has non-empty DestName header */
                                    sprintf(p_newpath, "%s%c%s", p_cb->p_rootpath,
                                            p_bta_fs_cfg->path_separator, p_dest_name);
                                }
                                else
                                {
                                    /* the packet has non-empty DestName header */
                                    sprintf(p_newpath, "%s%c%s", p_cb->p_workdir,
                                            p_bta_fs_cfg->path_separator, p_dest_name);
                                }
                                if (((bta_fs_co_access(p_newpath, BTA_FS_ACC_EXIST,
                                    &is_dir, p_cb->app_id)) == BTA_FS_CO_FAIL))
                                {
                                    /* the dest object does not exist */
                                    rsp_code = OBX_RSP_OK;
                                }
                            }
                        }
                        else
                            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                    }
                    else if (has_perms)
                    {
                        /* the SetPermission action must have the permission header */
                        rsp_code = OBX_RSP_OK;
                    }
                }

            }
            else
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
    }

    if(rsp_code == OBX_RSP_OK)
    {
        bta_fts_req_app_access(ft_op, p_cb);
    }
    else
    {
        OBX_ActionRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
        utl_freebuf((void**)&p_dest_name);
        utl_freebuf((void**)&p_cb->p_dest);
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}
/* end of using OBEX 1.5 */

/*******************************************************************************
**
** Function         bta_fts_appl_tout
**
** Description      Process the FTS application timeout event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_appl_tout(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
}

/*******************************************************************************
**
** Function         bta_fts_conn_err_rsp
**
** Description      Process the OBX error response
**                  Connect request received in wrong state, or bad request
**                  from client
**                 
** Returns          void
**
*******************************************************************************/
void bta_fts_conn_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    OBX_ConnectRsp(p_data->obx_evt.handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
}

/* Using OBEX 1.5 */
/*******************************************************************************
**
** Function         bta_fts_session_req
**
** Description      Process the OBX session req in connected state
**                 
** Returns          void
**
*******************************************************************************/
void bta_fts_session_req(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tOBX_SESS_EVT       *p_sess = &p_data->obx_evt.param.sess;
    UINT8 rsp_code = OBX_RSP_OK;
    UINT32 offset = 0;

    if (p_evt->obx_event == OBX_SESSION_REQ_EVT)
    {
        APPL_TRACE_EVENT2("sess_op:%d ssn:%d", p_sess->sess_op, p_sess->ssn);
        switch (p_sess->sess_op)
        {
        case OBX_SESS_OP_SUSPEND:
            bta_fs_co_suspend(p_cb->bd_addr, p_sess->p_sess_info, p_sess->ssn, &p_sess->timeout, &offset, p_cb->obx_oper, p_cb->app_id);
            p_cb->state = BTA_FTS_CLOSING_ST;
            p_cb->suspending = TRUE;
            break;

        case OBX_SESS_OP_CLOSE:
            bta_fs_co_session_info(p_cb->bd_addr, p_sess->p_sess_info, 0, BTA_FS_CO_SESS_ST_NONE, NULL, NULL, p_cb->app_id);
            p_cb->state = BTA_FTS_CLOSING_ST;
            break;

        case OBX_SESS_OP_CREATE:
        case OBX_SESS_OP_RESUME:
            p_cb->state = BTA_FTS_CONN_ST;
            rsp_code = OBX_RSP_FORBIDDEN;
            break;
        }
        OBX_SessionRsp (p_evt->handle, rsp_code, p_sess->ssn, offset, NULL);
        return;
    }
    else if (p_evt->obx_event == OBX_SESSION_INFO_EVT)
    {
        if ((p_cb->obx_oper != FTS_OP_GET_FILE) && (p_cb->obx_oper != FTS_OP_PUT_FILE) && (p_cb->obx_oper != FTS_OP_LISTING))
        {
            /* the other obx ops are single transaction.
             * If the link is dropped before the transaction ends, let the client re-transmit the request */
            p_sess->ssn--;
            p_cb->obx_oper = FTS_OP_NONE;
        }
        bta_fs_co_suspend(p_cb->bd_addr, p_sess->p_sess_info, p_sess->ssn,
                          &p_sess->timeout, &offset, p_cb->obx_oper, p_cb->app_id);
        OBX_AddSuspendedSession (p_cb->obx_handle, p_cb->bd_addr, 
            p_sess->p_sess_info, p_sess->timeout,
            p_sess->ssn, offset);
        p_cb->suspending = TRUE;
    }
    else
    {
        p_cb->state = BTA_FTS_CONN_ST;
        bta_fts_conn_err_rsp (p_cb, p_data);
    }
}
/* End of using OBEX 1.5 */

/*******************************************************************************
**
** Function         bta_fts_disc_err_rsp
**
** Description      Process the OBX error response
**                  Disconnect request received in wrong state, or bad request
**                  from client
**                 
** Returns          void
**
*******************************************************************************/
void bta_fts_disc_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    OBX_DisconnectRsp(p_data->obx_evt.handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_fts_gasp_err_rsp
**
** Description      Process the OBX error response for Get, Abort, Setpath, and Put.
**
**                  The rsp_code field of tBTA_FTS_DATA (obx_evt) contains the
**                  response code to be sent to OBEX, and the obx_event field
**                  contains the current OBEX event.
**                 
** Returns          void
**
*******************************************************************************/
void bta_fts_gasp_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tBTA_FTS_OBX_RSP    *p_rsp = NULL;

    switch (p_evt->obx_event)
    {
    case OBX_PUT_REQ_EVT:
        p_rsp = OBX_PutRsp;
        break;
    case OBX_GET_REQ_EVT:
        p_rsp = OBX_GetRsp;
        break;
    case OBX_SETPATH_REQ_EVT:
        p_rsp = OBX_SetPathRsp;
        break;
    case OBX_ABORT_REQ_EVT:
        p_rsp = OBX_AbortRsp;
        break;
    case OBX_ACTION_REQ_EVT:
        p_rsp = OBX_ActionRsp;
        break;
    }
    if(p_rsp)
        (*p_rsp)(p_evt->handle, p_evt->rsp_code, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_fts_close_complete
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_close_complete(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    p_cb->cout_active = FALSE;
    bta_fts_clean_getput(p_cb, TRUE);
    bta_fts_clean_list(p_cb);

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_FTS ,p_cb->app_id, p_cb->bd_addr);

    


    /* Notify the MMI that a connection has been closed */
    p_cb->p_cback(BTA_FTS_CLOSE_EVT, (tBTA_FTS*)p_cb->bd_addr);
    memset(p_cb->bd_addr, 0, BD_ADDR_LEN);

    if (p_data->obx_evt.p_pkt)
        APPL_TRACE_WARNING0("FTS: OBX CLOSE CALLED WITH non-NULL Packet!!!");
}

/*******************************************************************************
**
** Function         bta_fts_disable_cmpl
**
** Description      Finishes the memory cleanup before shutting down server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_disable_cmpl(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    bta_fts_disable_cleanup(p_cb);
}

/*****************************************************************************
**  Callback Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_fts_obx_cback
**
** Description      OBX callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event,
                        tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_FTS_OBX_EVENT *p_obx_msg;
    UINT16              event = 0;
    UINT16              size = sizeof(tBTA_FTS_OBX_EVENT);

#if BTA_FTS_DEBUG == TRUE
    APPL_TRACE_DEBUG2("OBX Event Callback: obx_event [%s] %d", fts_obx_evt_code(obx_event), obx_event);
#endif

    switch(obx_event)
    {
    case OBX_SESSION_INFO_EVT:   /* the session information event to resume the session. */
        size += OBX_SESSION_INFO_SIZE;
        /* Falls through */

    case OBX_SESSION_REQ_EVT:
    case OBX_CONNECT_REQ_EVT:
        event = BTA_FTS_OBX_CONN_EVT;
        break;
    case OBX_ACTION_REQ_EVT:
        event = BTA_FTS_OBX_ACTION_EVT;
        break;
    case OBX_DISCONNECT_REQ_EVT:
        event = BTA_FTS_OBX_DISC_EVT;
        break;
    case OBX_PUT_REQ_EVT:
        event = BTA_FTS_OBX_PUT_EVT;
        break;
    case OBX_GET_REQ_EVT:
        event = BTA_FTS_OBX_GET_EVT;
        break;
    case OBX_SETPATH_REQ_EVT:
        event = BTA_FTS_OBX_SETPATH_EVT;
        break;
    case OBX_ABORT_REQ_EVT:
        event = BTA_FTS_OBX_ABORT_EVT;
        break;
    case OBX_CLOSE_IND_EVT:
        event = BTA_FTS_OBX_CLOSE_EVT;
        break;
    case OBX_TIMEOUT_EVT:
        break;
    case OBX_PASSWORD_EVT:
        event = BTA_FTS_OBX_PASSWORD_EVT;
        break;
    default:
        /* Unrecognized packet; disconnect the session */
        if (p_pkt)
            event = BTA_FTS_OBX_DISC_EVT;
    }
    
    /* send event to BTA, if any */
    if (event && (p_obx_msg =
        (tBTA_FTS_OBX_EVENT *) GKI_getbuf(size)) != NULL)
    {
        p_obx_msg->hdr.event = event;
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
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

/*****************************************************************************
**  Local FTP Event Processing Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_fts_req_app_access
**
** Description      Sends an access request event to the application.
**                  
** Returns          void
**
*******************************************************************************/
void bta_fts_req_app_access (tBTA_FT_OPER oper, tBTA_FTS_CB *p_cb)
{
    tBTA_FTS_ACCESS *p_acc_evt;
    char            *p_devname;

    /* Notify the application that a put or get file has been requested */
    if ((p_acc_evt = (tBTA_FTS_ACCESS *)GKI_getbuf(sizeof(tBTA_FTS_ACCESS))) != NULL)
    {
        memset(p_acc_evt, 0, sizeof(tBTA_FTS_ACCESS));

        APPL_TRACE_API1("ACCESS REQ: [%s]", p_cb->p_path);

        p_acc_evt->p_name = p_cb->p_path;
        p_acc_evt->p_dest_name = p_cb->p_dest;
        memcpy(p_acc_evt->perms, p_cb->perms, BTA_FS_PERM_SIZE);

        p_acc_evt->size = p_cb->file_length;
        p_acc_evt->oper = p_cb->acc_active = oper;
        bdcpy(p_acc_evt->bd_addr, p_cb->bd_addr);
        if ((p_devname = BTM_SecReadDevName(p_cb->bd_addr)) != NULL)
            BCM_STRNCPY_S((char *)p_acc_evt->dev_name, sizeof(p_acc_evt->dev_name), p_devname, BTM_MAX_REM_BD_NAME_LEN);
    
        p_cb->p_cback(BTA_FTS_ACCESS_EVT, (tBTA_FTS *)p_acc_evt);
        GKI_freebuf(p_acc_evt);
    }
}

/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_FTS_DEBUG == TRUE

/*******************************************************************************
**
** Function         fts_obx_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *fts_obx_evt_code(tOBX_EVENT evt_code)
{
    switch(evt_code)
    {
    case OBX_CONNECT_REQ_EVT:
        return "OBX_CONNECT_REQ_EVT";
    case OBX_DISCONNECT_REQ_EVT:
        return "OBX_DISCONNECT_REQ_EVT";
    case OBX_PUT_REQ_EVT:
        return "OBX_PUT_REQ_EVT";
    case OBX_GET_REQ_EVT:
        return "OBX_GET_REQ_EVT";
    case OBX_SETPATH_REQ_EVT:
        return "OBX_SETPATH_REQ_EVT";
    case OBX_ABORT_REQ_EVT:
        return "OBX_ABORT_REQ_EVT";
    case OBX_CLOSE_IND_EVT:
        return "OBX_CLOSE_IND_EVT";
    case OBX_TIMEOUT_EVT:
        return "OBX_TIMEOUT_EVT";
    case OBX_PASSWORD_EVT:
        return "OBX_PASSWORD_EVT";
    case OBX_SESSION_REQ_EVT:
        return "OBX_SESSION_REQ_EVT";
    case OBX_ACTION_REQ_EVT:
        return "OBX_ACTION_REQ_EVT";
    case OBX_SESSION_INFO_EVT:
        return "OBX_SESSION_INFO_EVT";

    default:
        return "unknown OBX event code";
    }
}
#endif  /* Debug Functions */
#endif /* BTA_FT_INCLUDED */
