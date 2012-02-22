/*****************************************************************************
**
**  Name:           bta_ftc_utils.c
**
**  Description:    This file implements object store functions for the
**                  file transfer server.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"


#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include <stdio.h>
#include <string.h>
#include "bta_ftc_int.h"
#include "bta_fs_api.h"
#include "bta_fs_co.h"
#include "gki.h"
#include "utl.h"

/*******************************************************************************
**  Constants
*******************************************************************************/

/*******************************************************************************
**  Local Function Prototypes
*******************************************************************************/

/*******************************************************************************
*  Macros for FTC 
*******************************************************************************/
#define BTA_FTC_XML_EOL              "\n"
#define BTA_FTC_FOLDER_LISTING_START ( "<?xml version=\"1.0\"?>\n" \
                                   "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n" \
                                   "<folder-listing version=\"1.0\">\n" )

#define BTA_FTC_FOLDER_LISTING_END   ( "</folder-listing>" )

#define BTA_FTC_FILE_ELEM            "file"
#define BTA_FTC_FOLDER_ELEM          "folder"
#define BTA_FTC_PARENT_FOLDER_ELEM   "parent-folder"
#define BTA_FTC_NAME_ATTR            "name"
#define BTA_FTC_SIZE_ATTR            "size"
#define BTA_FTC_TYPE_ATTR            "type"
#define BTA_FTC_MODIFIED_ATTR        "modified"
#define BTA_FTC_CREATED_ATTR         "created"
#define BTA_FTC_ACCESSED_ATTR        "accessed"
#define BTA_FTC_USER_PERM_ATTR       "user-perm"
#define BTA_FTC_GROUP_PERM_ATTR      "group-perm"
#define BTA_FTC_OTHER_PERM_ATTR      "other-perm"
#define BTA_FTC_GROUP_ATTR           "group"
#define BTA_FTC_OWNER_ATTR           "owner"
#define BTA_FTC_LANG_ATTR            "xml:lang"

/*******************************************************************************
**  Local Function 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_ftc_send_abort_req
**
** Description      Send an abort request.
**
** Parameters       p_cb      - Pointer to the FTC control block
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_send_abort_req(tBTA_FTC_CB *p_cb)
{
    if (FTC_ABORT_REQ_NOT_SENT == p_cb->aborting)
    {
        p_cb->aborting = FTC_ABORT_REQ_SENT;
        OBX_AbortReq(p_cb->obx_handle, (BT_HDR *)NULL);

        /* Start abort response timer */
        p_cb->timer_oper = FTC_TIMER_OP_ABORT;
        bta_sys_start_timer(&p_cb->rsp_timer, BTA_FTC_RSP_TOUT_EVT,
                            p_bta_ft_cfg->abort_tout);
    }
}
/*******************************************************************************
**
** Function         bta_ftc_proc_pbap_param
**
** Description      read PBAP app parameter header.
**
** Parameters       
**
** Returns          
**
*******************************************************************************/
tBTA_FTC_PB_PARAM * bta_ftc_proc_pbap_param(tBTA_FTC_PB_PARAM *p_param, BT_HDR *p_pkt)
{
    UINT8   *p_data = NULL, aph;
    UINT16  data_len = 0;
    int     left, len;
    if(OBX_ReadByteStrHdr(p_pkt, OBX_HI_APP_PARMS, &p_data, &data_len, 0))
    {
        memset(p_param, 0, sizeof(tBTA_FTC_PB_PARAM));
        left    = data_len;
        while(left > 0)
        {
            aph = *p_data++;
            len = *p_data++;
            left -= len;
            left -= 2;
            switch(aph)
            {
            case BTA_FTC_APH_PB_SIZE:
                BE_STREAM_TO_UINT16(p_param->phone_book_size, p_data);
                p_param->pbs_exist = TRUE;
                break;
            case BTA_FTC_APH_NEW_MISSED_CALL:
                p_param->new_missed_calls = *p_data++;
                p_param->nmc_exist = TRUE;
                break;
            default:
                p_data += len;
            }
        }
    }
    else
        p_param = NULL;
    return p_param;
}

/*******************************************************************************
*  Exported Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_ftc_send_get_req
**
** Description      Processes a Get File Operation.
**
** Parameters       p_cb      - Pointer to the FTC control block
**
** Returns          (UINT8) OBX response code
**
*******************************************************************************/
UINT8 bta_ftc_send_get_req(tBTA_FTC_CB *p_cb)
{
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code = OBX_RSP_FAILED;

    /* Do not start another request if currently aborting */
    if (!p_cb->aborting)
    {
        /* OBX header are added in bta_ftc_init_getfile */
        if ((OBX_GetReq(p_cb->obx_handle, TRUE, p_obx->p_pkt)) == OBX_SUCCESS)
        {
            p_cb->req_pending = TRUE;
            rsp_code = OBX_RSP_OK;
            p_obx->p_pkt = NULL;
        }
    }
    else
    {
        bta_ftc_send_abort_req(p_cb);
    }

    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_ftc_proc_get_rsp
**
** Description      Processes a Get File response packet.
**                  Initiates a file write if no errors.
**
** Parameters       
**
**                  
** Returns          void
**
*******************************************************************************/
void bta_ftc_proc_get_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_OBX_EVT *p_evt = &p_data->obx_evt;
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    UINT16            body_size;
    BOOLEAN           final;
    BOOLEAN           done = TRUE;
    BOOLEAN           send_request = TRUE;
    UINT8             num_hdrs;
    tBTA_FTC          data;

    do
    {
        if (p_evt->rsp_code == OBX_RSP_OK || p_evt->rsp_code == OBX_RSP_CONTINUE)
        {
            /* Only continue if not aborting */
            if (p_cb->aborting)
            {
                /* Aborting: done with current packet */
                utl_freebuf((void**)&p_evt->p_pkt);

                /* If aborting, and this response is not last packet send abort */
                if(p_evt->rsp_code == OBX_RSP_CONTINUE)
                {
                    bta_ftc_send_abort_req(p_cb);
                    return;
                }
                else /* Last packet - abort and remove file (p_evt->rsp_code == OBX_RSP_OK) */
                {
                    p_evt->rsp_code = (!p_cb->int_abort) ? OBX_RSP_GONE : OBX_RSP_INTRNL_SRVR_ERR;
                    bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
                    return;
                }
            }

            p_obx->final_pkt = (p_evt->rsp_code == OBX_RSP_OK) ? TRUE : FALSE;

            /* If length header exists, save the file length */
            if (p_cb->first_req_pkt == TRUE)
            {
                p_cb->first_req_pkt = FALSE;

                /* if the obj_type is  */
                if(p_cb->obj_type == BTA_FTC_GET_PB)
                {
                    bta_ftc_proc_pbap_param(&data.pb, p_obx->p_pkt);
                    bta_ftc_cb.p_cback(BTA_FTC_PHONEBOOK_EVT, &data);
                    if (p_cb->fd == BTA_FS_INVALID_FD)
                    {
                        /* if the obj_type is get pb && the file id not open,
                         * must be getting the phonebook size only
                         * - end of transaction */
                        break;
                    }
                }
                if (!OBX_ReadLengthHdr(p_evt->p_pkt, &p_cb->file_size))
                    p_cb->file_size = BTA_FS_LEN_UNKNOWN;
            }

            /* Read the body header from the obx packet */
            num_hdrs = OBX_ReadBodyHdr(p_evt->p_pkt, &p_obx->p_start, &body_size,
                                       &final);
            /* process body header */
            if (num_hdrs == 1)
            {
                if (body_size)
                {
                    /* Data to be written */
                    p_obx->p_pkt = p_evt->p_pkt;
                    p_obx->offset = body_size;  /* Save write size for comparison */
                    p_cb->cout_active = TRUE;
                    bta_fs_co_write(p_cb->fd, p_obx->p_start, body_size,
                                    BTA_FTC_CI_WRITE_EVT, 0, p_cb->app_id);
                    done = FALSE;
                    send_request = FALSE;
                }
                /* If body header is zero in length but not final, request next packet */
                else if (!final)
            	{
                    done = FALSE;
                } 
            }
            else if (num_hdrs > 1)  /* Cannot handle more than a single body header */
            {
                p_evt->rsp_code = OBX_RSP_BAD_REQUEST;
            }
            /* Empty Body; send next request or finished */
            else if (!p_obx->final_pkt)
                done = FALSE;
        }
    } while (0);

    if (done)
    {
        bta_ftc_sm_execute(p_cb, BTA_FTC_OBX_CMPL_EVT, p_data);
        utl_freebuf((void**)&p_evt->p_pkt);
    }
    else if (send_request)
    {
        /* Free current packet and send a new request */
        utl_freebuf((void**)&p_evt->p_pkt);
        bta_ftc_send_get_req(p_cb);
    }
}

/*******************************************************************************
**
** Function         bta_ftc_cont_put_file
**
** Description      Continues a Put File Operation.
**                  Builds a new OBX packet, and initiates a read operation.
**
** Parameters       p_cb - pointer to the client's control block.
**                  first_pkt - TRUE if initial PUT request to server.
**
**                  
** Returns          UINT8 OBX response code
**
*******************************************************************************/
UINT8 bta_ftc_cont_put_file(tBTA_FTC_CB *p_cb, BOOLEAN first_pkt)
{
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code;
    char *p_ch;
    
    /* Do not start another request if currently aborting */
    if (p_cb->aborting)
    {
        bta_ftc_send_abort_req(p_cb);
        return (OBX_RSP_OK);
    }

    rsp_code = OBX_RSP_FAILED;

    if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, p_cb->peer_mtu)) != NULL)
    {
        p_obx->offset = 0;

        /* Add length header if it exists; No body in first packet */
        if (first_pkt)
        {
            /* Add the Name Header to the request */
            /* Find the beginning of the name (excluding the path) */
            p_ch = strrchr(p_cb->p_name, (int) p_bta_fs_cfg->path_separator);
            if (p_ch && p_ch[1] != '\0')
            {
                OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)&p_ch[1]);

                /* Add the length header if known */
                if (p_cb->file_size != BTA_FS_LEN_UNKNOWN)
                {
                    OBX_AddLengthHdr(p_obx->p_pkt, p_cb->file_size);

                }
                
                OBX_PutReq(p_cb->obx_handle, FALSE, p_obx->p_pkt);
                p_cb->req_pending = TRUE;
                p_obx->p_pkt = NULL;
                rsp_code = OBX_RSP_OK;
            }
        }
        else    /* A continuation packet so read file data */
        {
            /* Add the start of the Body Header */
            p_obx->bytes_left = 0;
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
            
            /* Read in the first packet's worth of data */
            p_cb->cout_active = TRUE;
            bta_fs_co_read(p_cb->fd, &p_obx->p_start[p_obx->offset],
                p_obx->bytes_left, BTA_FTC_CI_READ_EVT, 0, p_cb->app_id);
            rsp_code = OBX_RSP_OK;
        }
    }
    
    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_ftc_proc_list_data
**
** Description      Processes responses to directory listing requests
**                  Loops through returned data generating application
**                  listing data events.  If needed, issues a new request
**                  to the server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_proc_list_data(tBTA_FTC_CB *p_cb, tBTA_FTC_OBX_EVT *p_evt)
{
    tBTA_FTC app_evt;
    tBTA_FTC_PB_PARAM param;
    BOOLEAN           is_ok = FALSE;
    BOOLEAN           first_req_pkt = p_cb->first_req_pkt;

    app_evt.list.status = bta_ftc_convert_obx_to_ftc_status(p_evt->rsp_code);

    if (p_evt->rsp_code == OBX_RSP_OK || p_evt->rsp_code == OBX_RSP_CONTINUE)
    {
        app_evt.list.p_param = NULL;
        if (p_cb->first_req_pkt == TRUE)
        {
            p_cb->first_req_pkt = FALSE;
            if(p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE)
                app_evt.list.p_param = bta_ftc_proc_pbap_param(&param, p_evt->p_pkt);
        }
        APPL_TRACE_EVENT1("first_req_pkt: %d ", first_req_pkt);

        if (OBX_ReadBodyHdr(p_evt->p_pkt, &app_evt.list.data, &app_evt.list.len,
                        &app_evt.list.final))
        {
            /* len > 0 or is final packet */
            if(app_evt.list.len || app_evt.list.final) 

            {
                bta_ftc_cb.p_cback(BTA_FTC_LIST_EVT, &app_evt);
            }
            is_ok = TRUE;
        }
        else if(first_req_pkt && p_evt->rsp_code == OBX_RSP_CONTINUE)
        {
            /* no body header is OK, if this is the first packet and is continue response */
            is_ok = TRUE;
        }

        if(is_ok)
        {
            utl_freebuf((void**)&p_evt->p_pkt);
            /* Initiate another request if not finished */
            if (p_evt->rsp_code == OBX_RSP_CONTINUE)
            {
                if (p_cb->aborting)
                    bta_ftc_send_abort_req(p_cb);
                else
                    bta_ftc_get_listing(p_cb, NULL, NULL);
            }
            else
                p_cb->obx_oper = FTC_OP_NONE;  /* Finished with directory listing */
        }
        else
        {
            /* Missing body header & not the first packet */
            bta_ftc_listing_err(&p_evt->p_pkt, OBX_RSP_NO_CONTENT);
        }
    }
    else    /* Issue an error list entry */
        bta_ftc_listing_err(&p_evt->p_pkt, app_evt.list.status);
}

/*******************************************************************************
**
** Function         bta_ftc_get_listing
**
** Description      Initiates or Continues a GET Listing operation
**                  on the server's current directory.
**
** Parameters       p_cb - pointer to the client's control block.
**                  first_pkt - TRUE if initial GET request to server.
**
**                  
** Returns          void
**
*******************************************************************************/
void bta_ftc_get_listing(tBTA_FTC_CB *p_cb, char *p_name, tBTA_FTC_LST_PARAM *p_param)
{
    tBTA_FTC_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FTC_STATUS   status = BTA_FTC_FAIL;
    BOOLEAN           is_ok = TRUE;
    UINT8        *p, *p2, *p_start;
    UINT16       len    = 0;

    if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_CMD_POOL_SIZE)) != NULL)
    {
        if (p_name)
        {
            /* first packet */
            p_cb->first_req_pkt = TRUE;
            /* Add the Type Header */
            p = (UINT8 *) BTA_FTC_FOLDER_LISTING_TYPE;
            if(p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE)
                p = (UINT8 *) BTA_FTC_PULL_VCARD_LISTING_TYPE;

            is_ok = OBX_AddTypeHdr(p_obx->p_pkt, (char *)p);

            if(strcmp(p_name, ".") == 0 || p_name[0] == 0)
            {
                p_name = NULL;
            }

            if (p_name)
            {
                is_ok = OBX_AddUtf8NameHdr(p_obx->p_pkt, (unsigned char *) p_name);
            }

            if(p_param && p_cb->sdp_service == UUID_SERVCLASS_PBAP_PSE)
            {
                /* add app params for PCE */
                p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                p = p_start;
                if(p_param->order < BTA_FTC_ORDER_MAX)
                {
                    *p++    = BTA_FTC_APH_ORDER;
                    *p++    = 1;
                    *p++    = p_param->order;
                }
                if(p_param->p_value)
                {
                    *p++    = BTA_FTC_APH_SEARCH_VALUE;
                    *p      = strlen(p_param->p_value);
                    BCM_STRNCPY_S((char *) (p+1), strlen(p_param->p_value), p_param->p_value, *p);
                    p2 = p + 1 + *p;
                    p = p2;
                }
                if(p_param->attribute < BTA_FTC_ATTR_MAX)
                {
                    *p++    = BTA_FTC_APH_SEARCH_ATTR;
                    *p++    = 1;
                    *p++    = p_param->attribute;
                }
                if(p_param->max_list_count != 0xFFFF)
                {
                    *p++    = BTA_FTC_APH_MAX_LIST_COUNT;
                    *p++    = 2;
                    UINT16_TO_BE_STREAM(p, p_param->max_list_count);
                }
                if(p_param->list_start_offset)
                {
                    *p++    = BTA_FTC_APH_LIST_STOFF;
                    *p++    = 2;
                    UINT16_TO_BE_STREAM(p, p_param->list_start_offset);
                }

                /* If any of the app param header is added */
                if(p != p_start)
                {
                    OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
                }
            } /* if p_param: PBAP PCE need to keep AppParam in the first Get response */
        }

        if (is_ok)
        {
            if ((OBX_GetReq(p_cb->obx_handle, TRUE, p_obx->p_pkt)) == OBX_SUCCESS)
            {
                p_cb->req_pending = TRUE;
                p_obx->p_pkt = NULL;    /* OBX will free the memory */
                p_cb->obx_oper = FTC_OP_GET_LIST;
                status = BTA_FTC_OK;
            }
        }
    }

    if (status != BTA_FTC_OK)   /* Send an error response to the application */
        bta_ftc_listing_err(&p_obx->p_pkt, status);
}

/*******************************************************************************
**
** Function         bta_ftc_listing_err
**
** Description      Send a directory listing error event to the application
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_listing_err(BT_HDR **p_pkt, tBTA_FTC_STATUS status)
{
    tBTA_FTC err_rsp;
    
    if (bta_ftc_cb.obx_oper == FTC_OP_GET_LIST)
        bta_ftc_cb.obx_oper = FTC_OP_NONE;
    utl_freebuf((void**)p_pkt);
    
    err_rsp.list.len = 0;
    err_rsp.list.status = status;
    err_rsp.list.final = TRUE;
    err_rsp.list.data = NULL;
    bta_ftc_cb.p_cback(BTA_FTC_LIST_EVT, &err_rsp);
}

/*******************************************************************************
**
** Function         bta_ftc_convert_obx_to_ftc_status
**
** Description      Convert OBX response code into BTA FTC status code.
**
** Returns          void
**
*******************************************************************************/
tBTA_FTC_STATUS bta_ftc_convert_obx_to_ftc_status(tOBX_STATUS obx_status)
{
    tBTA_FTC_STATUS status;

    switch (obx_status)
    {
    case OBX_RSP_OK:
    case OBX_RSP_CONTINUE:
        status = BTA_FTC_OK;
        break;
    case OBX_RSP_UNAUTHORIZED:
        status = BTA_FTC_NO_PERMISSION;
        break;
    case OBX_RSP_NOT_FOUND:
        status = BTA_FTC_NOT_FOUND;
        break;
    case OBX_RSP_REQ_ENT_2_LARGE:
    case OBX_RSP_DATABASE_FULL:
        status = BTA_FTC_FULL;
        break;
    case OBX_RSP_GONE:
        status = BTA_FTC_ABORTED;
        break;
    case OBX_RSP_SERVICE_UNAVL: 
        status = BTA_FTC_SERVICE_UNAVL;
        break;
    default:
        status = BTA_FTC_FAIL;
    }

    return (status);
}
#endif /* BTA_FT_INCLUDED */
