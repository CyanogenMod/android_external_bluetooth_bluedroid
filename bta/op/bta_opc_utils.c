/*****************************************************************************
**
**  Name:           bta_opc_utils.c
**
**  Description:    This file implements object store functions for the
**                  file transfer server.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bta_opc_int.h"
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
*  Macros for OPC 
*******************************************************************************/

/*******************************************************************************
*  Exported Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_opc_send_get_req
**
** Description      Processes a Get File Operation.
**
** Parameters       p_cb      - Pointer to the OPC control block
**
** Returns          (UINT8) OBX response code
**
*******************************************************************************/
UINT8 bta_opc_send_get_req(tBTA_OPC_CB *p_cb)
{
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code = OBX_RSP_FAILED;
    
    utl_freebuf((void**)&p_obx->p_pkt);
    if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, p_cb->peer_mtu)) != NULL)
    {
        /* If first request add the Type Header to the request */
        if (p_cb->first_get_pkt == TRUE)
        {
            p_cb->obx_oper = OPC_OP_PULL_OBJ;
            p_cb->to_do &=  ~BTA_OPC_PULL_MASK;
            OBX_AddTypeHdr(p_obx->p_pkt, "text/x-vcard");
        }

        if ((OBX_GetReq(p_cb->obx_handle, TRUE, p_obx->p_pkt)) == OBX_SUCCESS)
        {
            rsp_code = OBX_RSP_OK;
            p_obx->p_pkt = NULL;
            p_cb->req_pending = TRUE;
        }
    }
    
    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_opc_cont_get_rsp
**
** Description      Continues an obex get response packet.
**                  This function is called upon completion of a file open or
**                  a file write event.
**
**                  
** Returns          void
**
*******************************************************************************/
void bta_opc_cont_get_rsp(tBTA_OPC_CB *p_cb)
{
    tBTA_OPC_OBX_EVT  obx_evt;
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    UINT16            body_size;
    BOOLEAN           end;
    
    /* Read the body header from the obx packet if it exists */
    if (OBX_ReadBodyHdr(p_obx->p_pkt, &p_obx->p_start, &body_size, &end))
    {
        p_obx->final_pkt = end;
        if (body_size)
        {
            /* Data to be written */
            p_obx->offset = body_size;  /* Save write size for comparison */
            p_cb->cout_active = TRUE;
            bta_fs_co_write(p_cb->fd, p_obx->p_start, body_size,
                BTA_OPC_CI_WRITE_EVT, 0, p_cb->app_id);
            return;
        }
    }
    
    /* Empty Body; send next request or finished */
    if (!p_obx->final_pkt)
    {
        bta_opc_send_get_req(p_cb);
    }
    else    /* Done getting object */
    {
        memset(&obx_evt, 0, sizeof(tBTA_OPC_OBX_EVT));
        obx_evt.rsp_code = OBX_RSP_OK;
        bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, (tBTA_OPC_DATA *) &obx_evt);
        utl_freebuf((void**)&p_obx->p_pkt);
    }
}

/*******************************************************************************
**
** Function         bta_opc_proc_get_rsp
**
** Description      Processes an obex get response packet.
**                  Initiates a file open if no errors.
**
**                  
** Returns          BOOLEAN - TRUE if obex packet is to be freed
**
*******************************************************************************/
BOOLEAN bta_opc_proc_get_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_EVT *p_evt = &p_data->obx_evt;
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    char             *p_filename;
    
    if (p_evt->rsp_code == OBX_RSP_OK || p_evt->rsp_code == OBX_RSP_CONTINUE)
    {
        /* The get the file name */
        if (!OBX_ReadUtf8NameHdr(p_obx->p_pkt, (UINT8 *) p_cb->p_name,
                                   p_bta_fs_cfg->max_file_len))
        BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len+1, OPC_DEF_RCV_NAME, sizeof(OPC_DEF_RCV_NAME)+1);
        
        /* If length header exists, save the file length */
        if (!OBX_ReadLengthHdr(p_obx->p_pkt, &p_cb->obj_size))
            p_cb->obj_size = BTA_FS_LEN_UNKNOWN;
        
        /* Build the file name with a fully qualified path */
        if ((p_filename = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
        {
            sprintf(p_filename, "%s%c%s", p_cb->p_rcv_path,
                p_bta_fs_cfg->path_separator,
                p_cb->p_name);

            p_cb->cout_active = TRUE;
            bta_fs_co_open(p_filename,
                (BTA_FS_O_RDWR | BTA_FS_O_CREAT | BTA_FS_O_TRUNC),
                p_cb->obj_size, BTA_OPC_CI_OPEN_EVT, p_cb->app_id);

            GKI_freebuf(p_filename);
            return FALSE;   /* Do not free the obex packet until data is written */
        }
        else
            p_evt->rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    }
    
    /* Server returned an error or out of memory to process request */
    bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, p_data);
    
    return TRUE;
}

/*******************************************************************************
**
** Function         bta_opc_send_put_req
**
** Description      Initiates/Continues a Put Object Operation.
**                  Builds a new OBX packet, and initiates a read operation.
**
** Parameters       p_cb - pointer to the client's control block.
**                  first_pkt - TRUE if initial PUT request to server.
**
**                  
** Returns          UINT8 OBX response code
**
*******************************************************************************/
UINT8 bta_opc_send_put_req(tBTA_OPC_CB *p_cb, BOOLEAN first_pkt)
{
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code = OBX_RSP_FAILED;
    char *p_ch;
    
    utl_freebuf((void**)&p_obx->p_pkt);
    if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, OBX_LRG_DATA_POOL_SIZE)) != NULL)
    {
        /* Add length header if it exists; No body in first packet */
        if (first_pkt)
        {
            /* Add the Name Header to the request */
            /* Find the beginning of the name (excluding the path) */
            p_ch = strrchr(p_cb->p_name, (int) p_bta_fs_cfg->path_separator);
            if (p_ch == NULL)
                p_ch = p_cb->p_name;

            if (p_ch && p_ch[1] != '\0')
            {
                OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *) (&p_ch[1]));

                /* Add the length header if known */
                if (p_cb->obj_size != BTA_FS_LEN_UNKNOWN)
                {
                    OBX_AddLengthHdr(p_obx->p_pkt, p_cb->obj_size);
                }

                /* Add the type header if known */
                switch (p_cb->format)
                {
                case BTA_OP_VCARD30_FMT:
                case BTA_OP_VCARD21_FMT:
                    OBX_AddTypeHdr(p_obx->p_pkt, "text/x-vcard");
                    break;
                case BTA_OP_VCAL_FMT:
                case BTA_OP_ICAL_FMT:
                    OBX_AddTypeHdr(p_obx->p_pkt, "text/x-vcalendar");
                    break;
                case BTA_OP_VNOTE_FMT:
                    OBX_AddTypeHdr(p_obx->p_pkt, "text/x-vnote");
                    break;
                case BTA_OP_VMSG_FMT:
                    OBX_AddTypeHdr(p_obx->p_pkt, "text/x-vmessage");
                    break;
                }

                OBX_PutReq(p_cb->obx_handle, FALSE, p_obx->p_pkt);
                p_obx->p_pkt = NULL;
                p_cb->req_pending = TRUE;
                rsp_code = OBX_RSP_OK;
            }
            else
                APPL_TRACE_ERROR1("Invalid file name [%s] feed, PutReq NOT sent.", p_cb->p_name);

         }
        else    /* A continuation packet so read object data */
        {
            /* Add the start of the Body Header */
            p_obx->offset = 0;
            p_obx->bytes_left = 0;
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
            
            /* Read in the first packet's worth of data */
            p_cb->cout_active = TRUE;
            bta_fs_co_read(p_cb->fd, &p_obx->p_start[p_obx->offset],
                p_obx->bytes_left, BTA_OPC_CI_READ_EVT, 0, p_cb->app_id);
            rsp_code = OBX_RSP_OK;
        }
    }
    
    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_opc_start_push
**
** Description      Push an object to connected server.
**                  Opens the object to be transferred to the server.
**
** Returns          void
**
*******************************************************************************/
void bta_opc_start_push(tBTA_OPC_CB *p_cb)
{
    p_cb->to_do &= ~BTA_OPC_PUSH_MASK;
    p_cb->obx_oper = OPC_OP_PUSH_OBJ;
    p_cb->cout_active = TRUE;
        
    bta_fs_co_open(p_cb->p_name, BTA_FS_O_RDONLY, 0, BTA_OPC_CI_OPEN_EVT,
                   p_cb->app_id);
}

/*******************************************************************************
**
** Function         bta_opc_convert_obx_to_opc_status
**
** Description      Convert OBX response code into BTA OPC status code.
**
** Returns          void
**
*******************************************************************************/
tBTA_OPC_STATUS bta_opc_convert_obx_to_opc_status(tOBX_STATUS obx_status)
{
    tBTA_OPC_STATUS status;

    switch (obx_status)
    {
    case OBX_RSP_OK:
    case OBX_RSP_CONTINUE:
        status = BTA_OPC_OK;
        break;
    case OBX_RSP_UNAUTHORIZED:
        status = BTA_OPC_NO_PERMISSION;
        break;
    case OBX_RSP_NOT_FOUND:
        status = BTA_OPC_NOT_FOUND;
        break;
    case OBX_RSP_SERVICE_UNAVL:
        status = BTA_OPC_SRV_UNAVAIL;
        break;
    case OBX_RSP_FORBIDDEN:
        status = BTA_OPC_RSP_FORBIDDEN;
        break;
    case OBX_RSP_NOT_ACCEPTABLE:
        status = BTA_OPC_RSP_NOT_ACCEPTABLE;
        break;
    default:
        status = BTA_OPC_FAIL;
    }

    return (status);
}
