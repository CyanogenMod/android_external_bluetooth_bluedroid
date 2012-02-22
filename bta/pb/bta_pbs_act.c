/*****************************************************************************
**
**  Name:           bta_pbs_act.c
**
**  Description:    This file contains the phone book access server
**                  functions for the state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_fs_api.h"
#include "bta_pbs_api.h"
#include "bta_pbs_int.h"
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
#if BTA_PBS_DEBUG == TRUE
static char *pbs_obx_evt_code(tOBX_EVENT evt_code);
#endif

/*****************************************************************************
**  Action Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_pbs_api_disable
**
** Description      Stop PBS server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_api_disable(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    /* Free any outstanding headers and control block memory */
    bta_pbs_clean_getput(p_cb, TRUE);

    /* Stop the OBEX server */
    OBX_StopServer(p_cb->obx_handle);

    /* Remove the PBS service from the SDP database */
    SDP_DeleteRecord(p_cb->sdp_handle);
    bta_sys_remove_uuid(UUID_SERVCLASS_PBAP_PSE);
    /* Free the allocated server channel number */
    BTM_FreeSCN(p_cb->scn);
    BTM_SecClrService(BTM_SEC_SERVICE_PBAP);

    utl_freebuf((void**)&p_cb->p_rootpath);  /* Free buffer containing root and working paths */
}

/*******************************************************************************
**
** Function         bta_pbs_api_authrsp
**
** Description      Pass the response to an authentication request back to the
**                  client.
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_api_authrsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
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
** Function         bta_pbs_api_close
**
** Description      Handle an api close event.  
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_api_close(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    BD_ADDR bd_addr;
    if (OBX_GetPeerAddr(p_cb->obx_handle, bd_addr) != 0)
    {
        /* resources will be freed at BTA_PBS_OBX_CLOSE_EVT */
        OBX_DisconnectRsp(p_cb->obx_handle, OBX_RSP_SERVICE_UNAVL, NULL);
    }
    else
    {
    p_cb->p_cback(BTA_PBS_CLOSE_EVT, 0);
}
}


/*******************************************************************************
**
** Function         bta_pbs_api_accessrsp
**
** Description      Process the access API event.
**                  If permission had been granted, continue the operation,
**                  otherwise stop the operation.
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_api_accessrsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    UINT8             rsp_code = OBX_RSP_OK;
    tBTA_PBS_CO_STATUS status = BTA_PBS_CO_EACCES;
    tBTA_PBS_ACCESS_TYPE    access = p_data->access_rsp.flag;
    tBTA_PBS_OPER      old_acc_active = p_cb->acc_active;
    tBTA_PBS_OBX_RSP  *p_rsp = NULL;
    tBTA_PBS_OBJECT   objevt;

    if(p_cb->acc_active != p_data->access_rsp.oper )
    {
        APPL_TRACE_WARNING2("PBS ACCRSP: not match active:%d, rsp:%d",
            p_cb->acc_active, p_data->access_rsp.oper);
        return;
    }

    p_cb->acc_active = 0;
    /* Process the currently active access response */
    switch (old_acc_active)
    {
    case BTA_PBS_OPER_PULL_PB:
    case BTA_PBS_OPER_PULL_VCARD_ENTRY:
        if (access == BTA_PBS_ACCESS_TYPE_ALLOW)
        {
            p_cb->cout_active = TRUE;
            bta_pbs_co_open(p_cb->p_path, p_cb->obx_oper, &p_cb->pullpb_app_params);
        }

        /* Case where application determined requested vCard handle has been modified:  Requests a new listing to clear */
        else if (access == BTA_PBS_ACCESS_TYPE_PRECONDITION && old_acc_active == BTA_PBS_OPER_PULL_VCARD_ENTRY)
            bta_pbs_get_file_rsp(OBX_RSP_PRECONDTN_FAILED, 0);
        else /* Denied */
            bta_pbs_get_file_rsp(OBX_RSP_UNAUTHORIZED, 0);
        break;
    case BTA_PBS_OPER_PULL_VCARD_LIST:
        if (access == BTA_PBS_ACCESS_TYPE_ALLOW)
        {
            /* continue with get vlist */
            bta_pbs_getvlist(p_cb->p_name);
        }
        else 
            bta_pbs_get_file_rsp(OBX_RSP_UNAUTHORIZED, 0);
        break;

    case BTA_PBS_OPER_SET_PB:       /* Request is a Change Folder */
        p_rsp = OBX_SetPathRsp;
        if (access == BTA_PBS_ACCESS_TYPE_ALLOW)
        {
            status = BTA_PBS_CO_OK;
            /* set obj type based on path */
            if (!memcmp(p_cb->p_name, BTA_PBS_PBFOLDER_NAME, sizeof(BTA_PBS_PBFOLDER_NAME)))
                     p_cb->obj_type = BTA_PBS_PB_OBJ;
            if (!memcmp(p_cb->p_name, BTA_PBS_ICHFOLDER_NAME, sizeof(BTA_PBS_ICHFOLDER_NAME)))
                     p_cb->obj_type = BTA_PBS_ICH_OBJ;
            if (!memcmp(p_cb->p_name, BTA_PBS_OCHFOLDER_NAME, sizeof(BTA_PBS_OCHFOLDER_NAME)))
                     p_cb->obj_type = BTA_PBS_OCH_OBJ;
            if (!memcmp(p_cb->p_name, BTA_PBS_MCHFOLDER_NAME, sizeof(BTA_PBS_MCHFOLDER_NAME)))
                     p_cb->obj_type = BTA_PBS_MCH_OBJ;
            if (!memcmp(p_cb->p_name, BTA_PBS_CCHFOLDER_NAME, sizeof(BTA_PBS_CCHFOLDER_NAME)))
                     p_cb->obj_type = BTA_PBS_CCH_OBJ;
            BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_path, p_bta_fs_cfg->max_path_len);
            p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';
            APPL_TRACE_DEBUG1("PBS: SET NEW PATH [%s]", p_cb->p_workdir);
        }
        break;

    default:
        p_cb->acc_active = old_acc_active;
        APPL_TRACE_WARNING1("PBS ACCRSP: Unknown tBTA_PBS_OPER value (%d)",
                            p_cb->acc_active);
        break;
    }
    
    /* Set Path Done */
    if(p_rsp && old_acc_active == BTA_PBS_OPER_SET_PB)
    {
        switch (status)
        {
        case BTA_PBS_CO_OK:
            rsp_code = OBX_RSP_OK;
            break;
        case BTA_PBS_CO_EACCES:
            rsp_code = OBX_RSP_UNAUTHORIZED;
            break;
        default:
            rsp_code = OBX_RSP_SERVICE_UNAVL;
            break;
        }
        if (p_cb->p_cback)
        {
            objevt.operation = p_cb->obx_oper;
            objevt.p_name = p_cb->p_path;
            objevt.status = (status == BTA_PBS_CO_OK ? BTA_PBS_OK: BTA_PBS_FAIL);
            (p_cb->p_cback) (BTA_PBS_OPER_CMPL_EVT, (tBTA_PBS *) &objevt);
        }
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
        p_cb->obx_oper = BTA_PBS_OPER_NONE;
        (*p_rsp)(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
}


/*******************************************************************************
**
** Function         bta_pbs_ci_vlist_act
**
** Description      Continue getting the current vlist entry operation
**                  
** Returns          void
**
*******************************************************************************/
void bta_pbs_ci_vlist_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    UINT8   rsp_code;
    tBTA_PBS_CI_VLIST_EVT *p_revt = &p_data->vlist_evt;
    
    p_cb->cout_active = FALSE;
    
    /* Process vcard listing call-in event if operation is still active */
    if (p_cb->obx_oper == BTA_PBS_OPER_PULL_VCARD_LIST)
    {
        switch (p_revt->status)
        {
        case BTA_PBS_CO_OK:
            p_cb->num_vlist_idxs++;

            /* Valid new entry */
            if (!strcmp(p_cb->vlist.handle, ".") || !strcmp(p_cb->vlist.handle, "..")
                                 || p_cb->vlist.handle[0] == '\0') {
                /* don't count this entry */
                p_cb->num_vlist_idxs--;

                /* continue get */
                if (p_revt->final) {
                    /* if it is final */
                    bta_pbs_end_of_list(OBX_RSP_OK);
                } else {
                    p_cb->cout_active = TRUE;
                    bta_pbs_co_getvlist(p_cb->p_path, &p_cb->getvlist_app_params,
                                    FALSE, &p_cb->vlist);
                }
            } else if ((rsp_code = bta_pbs_add_list_entry()) == OBX_RSP_OK) {
                /* if we can add the entry */
                if (p_revt->final) {
                    /* if it is final */
                    bta_pbs_end_of_list(rsp_code);
                } else {
                    /* continue get */
                    p_cb->cout_active = TRUE;
                    bta_pbs_co_getvlist(p_cb->p_path, &p_cb->getvlist_app_params,
                                        FALSE, &p_cb->vlist);
                }
            } else if (rsp_code == OBX_RSP_CONTINUE) {
                /* do not have enough data buffer, send obex continue to client */
                if (p_revt->final) {
                /* if this is the last VCard list entry */
                    p_cb->obx.final_pkt = TRUE;
                }
                bta_pbs_end_of_list(rsp_code);
            } else {
                bta_pbs_end_of_list(rsp_code);
            }
            break;
                   
        case BTA_PBS_CO_FAIL:   /* Error occurred */
            bta_pbs_end_of_list(OBX_RSP_SERVICE_UNAVL);
            break;
        }
    }
}

/*******************************************************************************
**
** Function         bta_pbs_ci_read
**
** Description      Handles the response to a read call-out request.
**                  This is called within the OBX get file request.  If the
**                  operation has completed, the OBX response is sent out;
**                  otherwise a read for additional data is made.
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_ci_read_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_CI_READ_EVT *p_revt = &p_data->read_evt;
    UINT8 rsp_code = OBX_RSP_SERVICE_UNAVL;

    p_cb->cout_active = FALSE;
    if (p_cb->aborting)
    {
        bta_pbs_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
        return;
    }

    /* Process read call-in event if operation is still active */
    if ((p_cb->obx_oper == BTA_PBS_OPER_PULL_PB || p_cb->obx_oper == BTA_PBS_OPER_PULL_VCARD_ENTRY)
         && p_revt->fd == p_cb->fd)
    {
        /* Read was successful, not finished yet */
        if (p_revt->status == BTA_PBS_CO_OK && !p_revt->final)
            rsp_code = OBX_RSP_CONTINUE;
        if (p_revt->status == BTA_PBS_CO_OK && p_revt->final)
            rsp_code = OBX_RSP_OK;

        /* Process response to OBX client */
        bta_pbs_get_file_rsp(rsp_code, p_revt->num_read);
    }
    else
    {
        bta_pbs_get_file_rsp(rsp_code, 0);
    }
}

/*******************************************************************************
**
** Function         bta_pbs_ci_open
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_ci_open_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_CI_OPEN_EVT *p_open = &p_data->open_evt;
    UINT8                rsp_code = OBX_RSP_OK;
    tBTA_PBS_OBX_PKT    *p_obx = &p_cb->obx;
    UINT16 pb_size = 0, new_missed_call = 0, len=0;
    UINT8        *p, *p_start;

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_pbs_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
        return;
    }
    
    /* Only process file get operations */
    if (p_cb->obx_oper == BTA_PBS_OPER_PULL_PB || p_cb->obx_oper == BTA_PBS_OPER_PULL_VCARD_ENTRY)
    {
        if (p_open->status == BTA_PBS_CO_OK)
        {
            p_cb->file_length = p_open->file_size;
            p_cb->fd = p_open->fd;

            /* add other application header */
            if (p_cb->obx_oper == BTA_PBS_OPER_PULL_PB)
            {
                    if (p_cb->pullpb_app_params.max_count == 0 || 
                        p_cb->obj_type == BTA_PBS_MCH_OBJ)
                    {
                        p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                        bta_pbs_co_getpbinfo(p_cb->obx_oper, p_cb->obj_type, &pb_size, &new_missed_call);
                        p = p_start;
                        if (p_cb->pullpb_app_params.max_count == 0)
                        {
                            *p++    = BTA_PBS_TAG_PB_SIZE;
                            *p++    = 2;
                            UINT16_TO_BE_STREAM(p, pb_size);
                            /* if max count = 0, client want to know the pb size */
                            p_cb->file_length = 0;
                        }
                        if (p_cb->obj_type == BTA_PBS_MCH_OBJ)
                        {
                            *p++    = BTA_PBS_TAG_NEW_MISSED_CALLS;
                            *p++    = 1;
                            *p++    = (UINT8) new_missed_call;
                        }
                        OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
                    }
            }

            if (p_cb->file_length > 0)
            {
                /* contiune to get file */
                bta_pbs_proc_get_file(p_cb->p_name, p_cb->obx_oper);
            }
            else
            {
                /* If file length is zero, add end body header */
                if (((p_cb->obx_oper == BTA_PBS_OPER_PULL_PB) && p_cb->pullpb_app_params.max_count) || p_cb->obx_oper == BTA_PBS_OPER_PULL_VCARD_ENTRY)   
                {
                    p_obx->offset = 0;
                    p_obx->bytes_left = 0;
                    p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);		
                    OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, 0, TRUE); 
                }
                bta_pbs_get_file_rsp(rsp_code, 0);
            }
        }
        else
        {
            if (p_open->status == BTA_PBS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else    /* File could not be found */
                rsp_code = OBX_RSP_NOT_FOUND;
            
            /* Send OBX response if an error occurred */
            bta_pbs_get_file_rsp(rsp_code, 0);
        }
    }

    return;
    
}


/*******************************************************************************
**
** Function         bta_pbs_obx_connect
**
** Description      Process the OBX connect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_connect(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tBTA_PBS_OPEN open_evt;
    char *p_devname;
    
    p_cb->peer_mtu = p_evt->param.conn.mtu;
    memcpy(p_cb->bd_addr, p_evt->param.conn.peer_addr, BD_ADDR_LEN);
    APPL_TRACE_EVENT1("PBS Connect: peer mtu 0x%04x", p_cb->peer_mtu);
    
    OBX_ConnectRsp(p_evt->handle, OBX_RSP_OK, (BT_HDR *)NULL);
    
    /* Reset to the root directory */
    BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
    p_cb->p_workdir[p_bta_fs_cfg->max_path_len] = '\0';
    
    /* inform role manager */
    bta_sys_conn_open(BTA_ID_PBS ,p_cb->app_id, p_cb->bd_addr);

    /* Notify the MMI that a connection has been opened */
    memcpy(open_evt.bd_addr, p_evt->param.conn.peer_addr, BD_ADDR_LEN);
    if ((p_devname = BTM_SecReadDevName(p_cb->bd_addr)) != NULL)
         BCM_STRNCPY_S((char *)open_evt.dev_name, sizeof(tBTM_BD_NAME), p_devname, BTM_MAX_REM_BD_NAME_LEN);

    p_cb->p_cback(BTA_PBS_OPEN_EVT, (tBTA_PBS *) &open_evt);
    
    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_pbs_obx_disc
**
** Description      Process the OBX disconnect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_disc(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code;

    rsp_code = (p_evt->obx_event == OBX_DISCONNECT_REQ_EVT) ? OBX_RSP_OK
                                                            : OBX_RSP_BAD_REQUEST;
    OBX_DisconnectRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_pbs_obx_close
**
** Description      Process the OBX link lost event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_close(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    /* finished if not waiting on a call-in function */
    if (!p_cb->cout_active)
        bta_pbs_sm_execute(p_cb, BTA_PBS_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_pbs_obx_abort
**
** Description      Process the OBX abort event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_abort(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_OK;

    utl_freebuf((void**)&p_evt->p_pkt);

    if (!p_cb->cout_active)
    {
        bta_pbs_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
    else    /* Delay the response if a call-out function is active */
        p_cb->aborting = TRUE;
}

/*******************************************************************************
**
** Function         bta_pbs_obx_password
**
** Description      Process the OBX password request
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_password(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    tBTA_PBS_AUTH       *p_auth;
    BOOLEAN              is_challenged;
    tOBX_AUTH_OPT        options;
    
    if ((p_auth = (tBTA_PBS_AUTH *)GKI_getbuf(sizeof(tBTA_PBS_AUTH))) != NULL)
    {
        memset(p_auth, 0, sizeof(tBTA_PBS_AUTH));
        
        /* Extract user id from packet (if available) */
        if (OBX_ReadAuthParams(p_data->obx_evt.p_pkt, &p_auth->p_userid,
            &p_auth->userid_len,
            &is_challenged, &options))
        {
            if (options & OBX_AO_USR_ID)
                p_auth->userid_required = TRUE;
        }
        
        
        /* Notify application */
        p_cb->p_cback(BTA_PBS_AUTH_EVT, (tBTA_PBS *)p_auth);
        
        GKI_freebuf(p_auth);
    }
    /* Don't need OBX packet any longer */
    utl_freebuf((void**)&p_evt->p_pkt);
}


/*******************************************************************************
**
** Function         bta_pbs_obx_get
**
** Description      Process the OBX file get and folder listing events
**                  If the type header is not folder listing, then pulling a file.
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_get(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT16   len;
    UINT8   *p_type;
    UINT8   *p_param;
    UINT16  param_len;
    tBTA_PBS_OPER operation = 0;
    
    /* If currently processing a GET, use the current name */
    if (bta_pbs_cb.obx_oper == BTA_PBS_OPER_PULL_VCARD_LIST)
        bta_pbs_getvlist(p_cb->p_name);
    else if (bta_pbs_cb.obx_oper == BTA_PBS_OPER_PULL_PB || 
             bta_pbs_cb.obx_oper == BTA_PBS_OPER_PULL_VCARD_ENTRY)
        bta_pbs_proc_get_file(p_cb->p_name, bta_pbs_cb.obx_oper);
    else  /* This is a new request */
    {
        /* Pull out the name header if it exists */
        if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1))) != NULL)
        {
            if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len))
            {

                GKI_freebuf(p_cb->p_name);
                p_cb->p_name = NULL;

#if 0           /* Peer spec violation, but some platforms do this wrong, so relaxing requirement.
                   enable this if strict error checking needed */
                utl_freebuf((void**)&p_evt->p_pkt);
                OBX_GetRsp(p_cb->obx_handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                return;
#endif
            }
        } else {
            OBX_GetRsp(p_cb->obx_handle, OBX_RSP_SERVICE_UNAVL, (BT_HDR *)NULL);
            utl_freebuf((void**)&p_evt->p_pkt);
            return;
        }
        
        /* See what type of operations */
        if (OBX_ReadTypeHdr(p_evt->p_pkt, &p_type, &len))
        {
            if (!memcmp(p_type, BTA_PBS_GETVCARD_LISTING_TYPE, len))
            {

                if ((p_cb->p_name) && strlen(p_cb->p_name))
                {
                    APPL_TRACE_EVENT1("PBS VList Get: Name [%s]", p_cb->p_name);
                }
                else    /* This is a peer spec violation, but allowing for better IOP */
                {
                    APPL_TRACE_WARNING0("PBS OBX GET:  Missing Name Header...Assuming current directory");

                    /* Errata 1824: It is illegal to issue a PullvCardListing request with an empty 
                    name header from the "telecom" folder */
                    if (p_cb->p_workdir && !strcmp(&(p_cb->p_workdir[strlen(p_cb->p_workdir) - 7]), "telecom"))
                    {
                        utl_freebuf((void**)&p_evt->p_pkt);
                        OBX_GetRsp(p_cb->obx_handle, OBX_RSP_NOT_FOUND, (BT_HDR *)NULL);
                        return;
                    }
                }

                /* read application params */
                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_MAX_LIST_COUNT, &param_len);
                if (p_param)
                {
                    BE_STREAM_TO_UINT16(p_cb->getvlist_app_params.max_count, p_param);
                }
                else
                {
                    p_cb->getvlist_app_params.max_count = BTA_PBS_MAX_LIST_COUNT; 
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_LIST_START_OFFSET, &param_len);
                if (p_param)
                {
                    BE_STREAM_TO_UINT16(p_cb->getvlist_app_params.start_offset, p_param);
                }
                else
                {
                    p_cb->getvlist_app_params.start_offset = 0;
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_ORDER, &param_len);
                if (p_param)
                {
                    p_cb->getvlist_app_params.order = *p_param;
                }
                else
                {
                    p_cb->getvlist_app_params.order = BTA_PBS_ORDER_INDEX;
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_SEARCH_ATTRIBUTE, &param_len);
                if (p_param)
                {
                    p_cb->getvlist_app_params.attribute = *p_param;
                }
                else
                {
                    p_cb->getvlist_app_params.attribute = 0;
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_SEARCH_VALUE, &param_len);
                if (p_param)
                {
                    memcpy(p_cb->getvlist_app_params.p_value, p_param, param_len);
                    p_cb->getvlist_app_params.value_len = param_len;
                }
                else
                {
                    p_cb->getvlist_app_params.value_len = 0;
                }

                /* Assume downloading vCard Listing if no name */
                p_cb->obj_type = BTA_PBS_PB_OBJ;

                if (p_cb->p_name)
                {
/*                  if (!memcmp(p_cb->p_name, BTA_PBS_PBFOLDER_NAME, sizeof(BTA_PBS_PBFOLDER_NAME)))
                         p_cb->obj_type = BTA_PBS_PB_OBJ; <-- default type */
                    if (!memcmp(p_cb->p_name, BTA_PBS_ICHFOLDER_NAME, sizeof(BTA_PBS_ICHFOLDER_NAME)))
                         p_cb->obj_type = BTA_PBS_ICH_OBJ;
                    else if (!memcmp(p_cb->p_name, BTA_PBS_OCHFOLDER_NAME, sizeof(BTA_PBS_OCHFOLDER_NAME)))
                         p_cb->obj_type = BTA_PBS_OCH_OBJ;
                    else if (!memcmp(p_cb->p_name, BTA_PBS_MCHFOLDER_NAME, sizeof(BTA_PBS_MCHFOLDER_NAME)))
                         p_cb->obj_type = BTA_PBS_MCH_OBJ;
                    else if (!memcmp(p_cb->p_name, BTA_PBS_CCHFOLDER_NAME, sizeof(BTA_PBS_CCHFOLDER_NAME)))
                         p_cb->obj_type = BTA_PBS_CCH_OBJ;
                }
                bta_pbs_getvlist(p_cb->p_name);
            }
            else if (!memcmp(p_type, BTA_PBS_GETFILE_TYPE, len) || 
                     !memcmp(p_type, BTA_PBS_GETVARD_ENTRY_TYPE, len))
            {
                if (!memcmp(p_type, BTA_PBS_GETFILE_TYPE, len))
                    operation = BTA_PBS_OPER_PULL_PB;
                else
                    operation = BTA_PBS_OPER_PULL_VCARD_ENTRY;
                /* read application params */
                if (p_cb->p_name) 
                {
                    APPL_TRACE_EVENT2("PBS File Get: Name [%s], operation %d", p_cb->p_name, operation);
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_FILTER, &param_len);
                if (p_param)
                {
                    if (param_len == 8) 
                        p_param += 4; /* skip the first 4 bytes, we do not handle proprietary AttributesMask */
                    BE_STREAM_TO_UINT32(p_cb->pullpb_app_params.filter, p_param);
                }
                else
                {
                    p_cb->pullpb_app_params.filter = BTA_PBS_FILTER_ALL;
                }

                p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_FORMAT, &param_len);
                if (p_param)
                {
                    p_cb->pullpb_app_params.format = *p_param;
                }
                else
                {
                    p_cb->pullpb_app_params.format = BTA_PBS_VCF_FMT_21;
                }

                /* set the object type for pull pb */
                if (operation == BTA_PBS_OPER_PULL_PB)
                {
                   if (p_cb->p_name == NULL)
                        OBX_GetRsp(p_cb->obx_handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                   else
                   {
                       if (!memcmp(p_cb->p_name, BTA_PBS_PULLPB_NAME, sizeof(BTA_PBS_PULLPB_NAME)))
                         p_cb->obj_type = BTA_PBS_PB_OBJ;
                       if (!memcmp(p_cb->p_name, BTA_PBS_PULLICH_NAME, sizeof(BTA_PBS_PULLICH_NAME)))
                         p_cb->obj_type = BTA_PBS_ICH_OBJ;
                       if (!memcmp(p_cb->p_name, BTA_PBS_PULLOCH_NAME, sizeof(BTA_PBS_PULLOCH_NAME)))
                         p_cb->obj_type = BTA_PBS_OCH_OBJ;
                       if (!memcmp(p_cb->p_name, BTA_PBS_PULLMCH_NAME, sizeof(BTA_PBS_PULLMCH_NAME)))
                         p_cb->obj_type = BTA_PBS_MCH_OBJ;
                       if (!memcmp(p_cb->p_name, BTA_PBS_PULLCCH_NAME, sizeof(BTA_PBS_PULLCCH_NAME)))
                         p_cb->obj_type = BTA_PBS_CCH_OBJ;
                   }
                }
                /* for pull pb, read app params for max list count and start offset */
                if (operation == BTA_PBS_OPER_PULL_PB)
                {
                    p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_MAX_LIST_COUNT, &param_len);
                    if (p_param)
                        BE_STREAM_TO_UINT16(p_cb->pullpb_app_params.max_count, p_param);
                    p_param = bta_pbs_read_app_params(p_evt->p_pkt, BTA_PBS_TAG_LIST_START_OFFSET, &param_len);
                    if (p_param)
                    {
                        BE_STREAM_TO_UINT16(p_cb->pullpb_app_params.start_offset, p_param);
                    }
                    else
                    {
                        p_cb->pullpb_app_params.start_offset = 0;
                    }
                }
                bta_pbs_proc_get_file(p_cb->p_name, operation);
            }
            else {
                OBX_GetRsp(p_cb->obx_handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
            }
        }
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_pbs_obx_setpath
**
** Description      Process the PBS change directory requests
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_setpath(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_BAD_REQUEST;
    tOBX_SETPATH_FLAG   *p_flag = &p_evt->param.sp.flag;
    tBTA_PBS_OPER        pbs_op = 0;
    tBTA_PBS_OBJECT   objevt;

    /* Verify flags and handle before accepting */
    if (p_evt->handle == p_cb->obx_handle &&
        (((*p_flag) & FLAGS_ARE_MASK) != FLAGS_ARE_ILLEGAL))
    {
        p_cb->obx_oper = BTA_PBS_OPER_SET_PB;
        p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len + 1));
        p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len +
                                p_bta_fs_cfg->max_file_len + 2));
        if (p_cb->p_name != NULL && p_cb->p_path != NULL)
        {
            if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len))
            {
                p_cb->p_name[0] = 0;
            }
            
            rsp_code = bta_pbs_chdir(p_evt->p_pkt, (BOOLEAN)((*p_flag) & OBX_SPF_BACKUP), &pbs_op);
        }
        else
            rsp_code = OBX_RSP_SERVICE_UNAVL;
    }

    if(pbs_op)
    {
        bta_pbs_req_app_access(pbs_op, p_cb);
    }
    else
    {
        if (p_cb->p_cback)
        {
            objevt.operation = BTA_PBS_OPER_SET_PB;
            objevt.p_name = p_cb->p_path;
            objevt.status = (rsp_code == OBX_RSP_OK ? BTA_PBS_OK: BTA_PBS_FAIL);
            (p_cb->p_cback) (BTA_PBS_OPER_CMPL_EVT, (tBTA_PBS *) &objevt);
        }
        p_cb->obx_oper = BTA_PBS_OPER_NONE;
        OBX_SetPathRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
        utl_freebuf((void**)&p_cb->p_name);
        utl_freebuf((void**)&p_cb->p_path);
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_pbs_appl_tout
**
** Description      Process the PBS application timeout event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_appl_tout(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
}

/*******************************************************************************
**
** Function         bta_pbs_conn_err_rsp
**
** Description      Process the OBX error response
**                  Connect request received in wrong state, or bad request
**                  from client
**                 
** Returns          void
**
*******************************************************************************/
void bta_pbs_conn_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    OBX_ConnectRsp(p_data->obx_evt.handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_pbs_disc_err_rsp
**
** Description      Process the OBX error response
**                  Disconnect request received in wrong state, or bad request
**                  from client
**                 
** Returns          void
**
*******************************************************************************/
void bta_pbs_disc_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    OBX_DisconnectRsp(p_data->obx_evt.handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_pbs_gasp_err_rsp
**
** Description      Process the OBX error response for Get, Abort, Setpath.
**
**                  The rsp_code field of tBTA_PBS_DATA (obx_evt) contains the
**                  response code to be sent to OBEX, and the obx_event field
**                  contains the current OBEX event.
**                 
** Returns          void
**
*******************************************************************************/
void bta_pbs_gasp_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_OBX_EVENT  *p_evt = &p_data->obx_evt;

    switch (p_evt->obx_event)
    {
    case OBX_GET_REQ_EVT:
        OBX_GetRsp(p_evt->handle, p_evt->rsp_code, (BT_HDR *)NULL);
        break;
    case OBX_SETPATH_REQ_EVT:
        OBX_SetPathRsp(p_evt->handle, p_evt->rsp_code, (BT_HDR *)NULL);
        break;
    case OBX_ABORT_REQ_EVT:
        OBX_AbortRsp(p_evt->handle, p_evt->rsp_code, (BT_HDR *)NULL);
        break;
    }
}

/*******************************************************************************
**
** Function         bta_pbs_close_complete
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_close_complete(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    p_cb->cout_active = FALSE;
    bta_pbs_clean_getput(p_cb, TRUE);

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_PBS ,p_cb->app_id, p_cb->bd_addr);

    memset(p_cb->bd_addr, 0, BD_ADDR_LEN);

    /* Notify the MMI that a connection has been closed */
    p_cb->p_cback(BTA_PBS_CLOSE_EVT, 0);

    if (p_data->obx_evt.p_pkt)
        APPL_TRACE_WARNING0("PBS: OBX CLOSE CALLED WITH non-NULL Packet!!!");
}

/*****************************************************************************
**  Callback Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_pbs_obx_cback
**
** Description      OBX callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event,
                        tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_PBS_OBX_EVENT *p_obx_msg;
    UINT16              event = 0;

#if BTA_PBS_DEBUG == TRUE
    APPL_TRACE_DEBUG1("OBX Event Callback: obx_event [%s]", pbs_obx_evt_code(obx_event));
#endif

    switch(obx_event)
    {
    case OBX_CONNECT_REQ_EVT:
        event = BTA_PBS_OBX_CONN_EVT;
        break;
    case OBX_DISCONNECT_REQ_EVT:
        event = BTA_PBS_OBX_DISC_EVT;
        break;
    case OBX_GET_REQ_EVT:
        event = BTA_PBS_OBX_GET_EVT;
        break;
    case OBX_SETPATH_REQ_EVT:
        event = BTA_PBS_OBX_SETPATH_EVT;
        break;
    case OBX_ABORT_REQ_EVT:
        event = BTA_PBS_OBX_ABORT_EVT;
        break;
    case OBX_CLOSE_IND_EVT:
        event = BTA_PBS_OBX_CLOSE_EVT;
        break;
    case OBX_TIMEOUT_EVT:
        break;
    case OBX_PASSWORD_EVT:
        event = BTA_PBS_OBX_PASSWORD_EVT;
        break;
    /* send Bad Request for Obex put request */
    case OBX_PUT_REQ_EVT:
        OBX_PutRsp(handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
        if (p_pkt)
            utl_freebuf((void**)&p_pkt);
        return;
        break;
    default:
        /* Unrecognized packet; disconnect the session */
        if (p_pkt) 
        {
            event = BTA_PBS_OBX_DISC_EVT;
            utl_freebuf((void**)&p_pkt);
        }
        break;
    }
    
    /* send event to BTA, if any */
    if (event && (p_obx_msg =
        (tBTA_PBS_OBX_EVENT *) GKI_getbuf(sizeof(tBTA_PBS_OBX_EVENT))) != NULL)
    {
        p_obx_msg->hdr.event = event;
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
        p_obx_msg->param = param;
        p_obx_msg->p_pkt = p_pkt;

        bta_sys_sendmsg(p_obx_msg);
    }
}

/*****************************************************************************
**  Local PBS Event Processing Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_pbs_req_app_access
**
** Description      Sends an access request event to the application.
**                  
** Returns          void
**
*******************************************************************************/
void bta_pbs_req_app_access (tBTA_PBS_OPER oper, tBTA_PBS_CB *p_cb)
{
    tBTA_PBS_ACCESS *p_acc_evt;
    char            *p_devname;

    /* Notify the application that a get file has been requested */
    if ((p_acc_evt = (tBTA_PBS_ACCESS *)GKI_getbuf(sizeof(tBTA_PBS_ACCESS))) != NULL)
    {
        memset(p_acc_evt, 0, sizeof(tBTA_PBS_ACCESS));

        APPL_TRACE_API1("ACCESS REQ: [%s]", p_cb->p_path);
        p_acc_evt->p_name = p_cb->p_path;
        p_acc_evt->oper = p_cb->acc_active = oper;
        bdcpy(p_acc_evt->bd_addr, p_cb->bd_addr);
        if ((p_devname = BTM_SecReadDevName(p_cb->bd_addr)) != NULL)
            BCM_STRNCPY_S((char *)p_acc_evt->dev_name, sizeof(tBTM_BD_NAME), p_devname, BTM_MAX_REM_BD_NAME_LEN);
    
        p_cb->p_cback(BTA_PBS_ACCESS_EVT, (tBTA_PBS *)p_acc_evt);
        GKI_freebuf(p_acc_evt);
    }
}

/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_PBS_DEBUG == TRUE

/*******************************************************************************
**
** Function         pbs_obx_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *pbs_obx_evt_code(tOBX_EVENT evt_code)
{
    switch(evt_code)
    {
    case OBX_CONNECT_REQ_EVT:
        return "OBX_CONNECT_REQ_EVT";
    case OBX_DISCONNECT_REQ_EVT:
        return "OBX_DISCONNECT_REQ_EVT";
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
    default:
        return "unknown OBX event code";
    }
}
#endif  /* Debug Functions */
#endif /* BTA_PBS_INCLUDED */
