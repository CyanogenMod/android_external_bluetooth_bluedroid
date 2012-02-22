/*****************************************************************************
**
**  Name:           bta_ops_utils.c
**
**  Description:    This file implements object store functions for the
**                  object transfer server.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_OP_INCLUDED) && (BTA_OP_INCLUDED)

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bta_fs_api.h"
#include "bta_ops_int.h"
#include "bta_fs_co.h"
#include "gki.h"
#include "utl.h"
#include "bd.h"

/*******************************************************************************
**  Constants
*******************************************************************************/

/*******************************************************************************
**  Local Function Prototypes
*******************************************************************************/
static int bta_ops_stricmp (const char *p_str1, const char *p_str2);

/*******************************************************************************
*  Exported Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_ops_init_get_obj
**
** Description      Processes a begin object pull request.
**
** Returns          void
**
*******************************************************************************/
void bta_ops_init_get_obj(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_FORBIDDEN;
    UINT16               len;
    UINT8               *p_type;
    UINT16               name_len;

    /* Type Hdr must be present */
    if (OBX_ReadTypeHdr(p_evt->p_pkt, &p_type, &len))
    {
        /* Type Hdr must be correct */
        if (!bta_ops_stricmp((const char *)p_type, "text/x-vcard"))
        {
            /* Erratum 385 - original OPP spec says "Name Header must not be used." 
             *               errara says "Name Header must be empty"
             * Be a forgiving OPP server, allow either way */
            name_len = OBX_ReadHdrLen(p_evt->p_pkt, OBX_HI_NAME);
            if((name_len == OBX_INVALID_HDR_LEN) ||/* no name header */
               (name_len == 3) /* name header is empty */ )
            {
                p_cb->obx.p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                                        /*p_cb->peer_mtu*/OBX_CMD_POOL_SIZE);
                p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1));
            
                if (p_cb->p_path && p_cb->obx.p_pkt)
                {
                    p_cb->p_path[0] = '\0';
                    p_cb->p_name = p_cb->p_path;
                    p_cb->obx_oper = OPS_OP_PULL_OBJ;
                    p_cb->obj_fmt = BTA_OP_VCARD21_FMT;
                    p_cb->file_length = BTA_FS_LEN_UNKNOWN;
                
                    /* Notify the appl that a pull default card has been requested */
                    bta_ops_req_app_access (BTA_OP_OPER_PULL, p_cb);
                    rsp_code = OBX_RSP_CONTINUE;
                }
                else
                    rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
            }
        }
    }
    
    /* Error has been detected; respond with error code */
    if (rsp_code != OBX_RSP_CONTINUE)
    {
        utl_freebuf((void**)&p_cb->obx.p_pkt);
        utl_freebuf((void**)&p_cb->p_path);
        p_cb->p_name = NULL;
        OBX_GetRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ops_proc_get_obj
**
** Description      Processes a continuation packet for pulling a vcard.
**                  Builds a response packet and initiates a read into it.
**
** Returns          void
**
*******************************************************************************/
void bta_ops_proc_get_obj(tBTA_OPS_CB *p_cb)
{
    tBTA_OPS_OBX_PKT *p_obx = &p_cb->obx;
    
    /* Allocate a new OBX packet */
    if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                /*p_cb->peer_mtu*/OBX_LRG_DATA_POOL_SIZE)) != NULL)
    {
        /* Add the start of the Body Header */
        p_obx->offset = 0;
        p_obx->bytes_left = 0;
        p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
        
        p_cb->cout_active = TRUE;
        bta_fs_co_read(p_cb->fd, &p_obx->p_start[p_obx->offset],
            p_obx->bytes_left, BTA_OPS_CI_READ_EVT, 0, p_cb->app_id);
    }
    else
        bta_ops_get_obj_rsp(OBX_RSP_INTRNL_SRVR_ERR, 0);
}

/*******************************************************************************
**
** Function         bta_ops_proc_put_obj
**
** Description      Processes a Push Object Operation.
**                  Initiates the opening of an object for writing, or continues 
**                  with a new Obx packet of data (continuation).
**
** Parameters       p_pkt - Pointer to the OBX Put request
**
** Returns          void
**
*******************************************************************************/
void bta_ops_proc_put_obj(BT_HDR *p_pkt)
{
    tBTA_OPS_CB      *p_cb = &bta_ops_cb;
    tBTA_OPS_OBX_PKT *p_obx = &p_cb->obx;
    UINT8             num_hdrs;
    BOOLEAN           endpkt;
    
    p_obx->p_pkt = p_pkt;
    
    p_obx->offset = 0;  /* Initial offset into OBX data */
    /* Read in start of body if there is a body header */
    num_hdrs = OBX_ReadBodyHdr(p_obx->p_pkt, &p_obx->p_start, &p_obx->bytes_left,
                               &endpkt);
    if (num_hdrs == 1)
    {
        p_cb->cout_active = TRUE;
        bta_fs_co_write(p_cb->fd, &p_obx->p_start[p_obx->offset],
            p_obx->bytes_left, BTA_OPS_CI_WRITE_EVT, 0, p_cb->app_id);
    }
    else
    {
        bta_ops_clean_getput(p_cb, TRUE);
        bta_ops_put_obj_rsp(OBX_RSP_BAD_REQUEST);
    }
}

/*******************************************************************************
**
** Function         bta_ops_get_obj_rsp
**
** Description      Finishes up the end body of the object get, and sends out the
**                  OBX response
**
** Returns          void
**
*******************************************************************************/
void bta_ops_get_obj_rsp(UINT8 rsp_code, UINT16 num_read)
{
    tBTA_OPS_CB       *p_cb = &bta_ops_cb;
    tBTA_OPS_OBX_PKT  *p_obx = &p_cb->obx;
    tBTA_OPS_PROGRESS  param;
    BOOLEAN            done = TRUE;
    
    /* Send the response packet if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        p_obx->offset += num_read;
        
        /* More to be sent */
        if (rsp_code == OBX_RSP_CONTINUE)
        {
            if (p_obx->bytes_left != num_read)
                APPL_TRACE_WARNING2("OPS Read: Requested (0x%04x), Read In (0x%04x)",
                                     p_obx->bytes_left, num_read);
            done = FALSE;
        }

        OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, done);

        /* Notify application with progress */
        if (num_read)
        {
            param.bytes = num_read;
            param.obj_size = p_cb->file_length;
            param.operation = BTA_OP_OPER_PULL;
            p_cb->p_cback(BTA_OPS_PROGRESS_EVT, (tBTA_OPS *)&param);
        }
    }
    else
        p_cb->obx_oper = OPS_OP_NONE;

    OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
    p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */

    /* Final response packet sent out */
    if (done)
        bta_ops_clean_getput(p_cb, FALSE);
}

/*******************************************************************************
**
** Function         bta_ops_put_obj_rsp
**
** Description      Responds to a put request, and closes the object if finished
**
** Returns          void
**
*******************************************************************************/
void bta_ops_put_obj_rsp(UINT8 rsp_code)
{
    tBTA_OPS_CB       *p_cb = &bta_ops_cb;
    tBTA_OPS_OBX_PKT  *p_obx = &p_cb->obx;
    tBTA_OPS_PROGRESS  param;
    tBTA_OPS_OBJECT    object;
    
    /* Finished with input packet */
    utl_freebuf((void**)&p_obx->p_pkt);

    if (rsp_code == OBX_RSP_OK)
    {
        /* Update application if object data was transferred */
        if (p_obx->bytes_left)
        {
            param.bytes = p_obx->bytes_left;
            param.obj_size = p_cb->file_length;
            param.operation = BTA_OP_OPER_PUSH;
            p_cb->p_cback(BTA_OPS_PROGRESS_EVT, (tBTA_OPS *)&param);
        }

        /* If not end of object put, set the continue response */
        if (!p_obx->final_pkt)
            rsp_code = OBX_RSP_CONTINUE;
        else    /* Done - free the allocated memory */
        {
            /* callback to app with object */
            object.format = p_cb->obj_fmt;
            object.p_name = p_cb->p_name;

            bta_ops_clean_getput(p_cb, FALSE);
            p_cb->p_cback(BTA_OPS_OBJECT_EVT, (tBTA_OPS *) &object);

        }
    }
    else
        p_cb->obx_oper = OPS_OP_NONE;

    OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_ops_req_app_access
**
** Description      Sends an access request event to the application.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ops_req_app_access (tBTA_OP_OPER oper, tBTA_OPS_CB *p_cb)
{
    tBTA_OPS_ACCESS *p_acc_evt;
    char            *p_devname;
    UINT16           len;

    /* Notify the application that a put or get file has been requested */
    if ((p_acc_evt = (tBTA_OPS_ACCESS *)GKI_getbuf(sizeof(tBTA_OPS_ACCESS))) != NULL)
    {
        memset(p_acc_evt, 0, sizeof(tBTA_OPS_ACCESS));
        p_acc_evt->p_name = p_cb->p_name;
        p_acc_evt->size = p_cb->file_length;
        p_acc_evt->oper = p_cb->acc_active = oper;
        p_acc_evt->format = p_cb->obj_fmt;
        bdcpy(p_acc_evt->bd_addr, p_cb->bd_addr);

        if ((p_devname = BTM_SecReadDevName(p_cb->bd_addr)) != NULL)
            BCM_STRNCPY_S((char *)p_acc_evt->dev_name, sizeof(tBTM_BD_NAME), p_devname, BTM_MAX_REM_BD_NAME_LEN);

        /* Only pass the object type if Push operation */
        if (oper == BTA_OP_OPER_PUSH)
        {
            if (!OBX_ReadTypeHdr(p_cb->obx.p_pkt, (UINT8 **)&p_acc_evt->p_type, &len))
                p_acc_evt->p_type = NULL;
        }

        if (p_acc_evt->p_type)
        {
            APPL_TRACE_EVENT3("OPS Access Request...Name [%s], Oper [%d], Type [%s]",
                p_cb->p_name, oper, p_acc_evt->p_type);
        }
        else
        {
            APPL_TRACE_EVENT2("OPS Access Request...Name [%s], Oper [%d]",
                p_cb->p_name, oper);
        }

        p_cb->p_cback(BTA_OPS_ACCESS_EVT, (tBTA_OPS *)p_acc_evt);
        GKI_freebuf(p_acc_evt);
    }
}

/*******************************************************************************
**
** Function         bta_ops_clean_getput
**
** Description      Cleans up the get/put resources and control block
**
** Returns          void
**
*******************************************************************************/
void bta_ops_clean_getput(tBTA_OPS_CB *p_cb, BOOLEAN is_aborted)
{
    tBTA_FS_CO_STATUS status;

    /* Clean up control block */
    utl_freebuf((void**)&p_cb->obx.p_pkt);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;

        /* Delete an aborted unfinished push file operation */
        if (is_aborted && p_cb->obx_oper == OPS_OP_PUSH_OBJ)
        {
            status = bta_fs_co_unlink(p_cb->p_path, p_cb->app_id);
            APPL_TRACE_WARNING2("OPS: Remove ABORTED Push File Operation [%s], status 0x%02x", p_cb->p_path, status);
        }
    }

    utl_freebuf((void**)&p_cb->p_path);
    p_cb->p_name = NULL;

    p_cb->obx_oper = OPS_OP_NONE;
    p_cb->obx.bytes_left = 0;
    p_cb->file_length = BTA_FS_LEN_UNKNOWN;
    p_cb->acc_active = 0;
    p_cb->aborting = FALSE;
}

/*******************************************************************************
**
** Function         bta_ops_fmt_supported
**
** Description      This function determines the file type from a file name
**                  and checks if the file type is supported.
**                  
**
** Returns          Format type value or zero if format unsupported.
**
*******************************************************************************/
tBTA_OP_FMT bta_ops_fmt_supported(char *p, tBTA_OP_FMT_MASK fmt_mask)
{
    char        *p_suffix;
    tBTA_OP_FMT fmt = BTA_OP_OTHER_FMT;

    /* scan to find file suffix */
    if ((p_suffix = strrchr(p, '.')) != NULL)
    {
        p_suffix++;
        if (bta_ops_stricmp (p_suffix, "vcf") == 0)
        {
            fmt = BTA_OP_VCARD21_FMT;
        }
        else if (bta_ops_stricmp (p_suffix, "vcd") == 0)
        {
            fmt = BTA_OP_VCARD30_FMT;
        }
        else if (bta_ops_stricmp (p_suffix, "vcs") == 0)
        {
            fmt = BTA_OP_VCAL_FMT;
        }
        else if (bta_ops_stricmp (p_suffix, "ics") == 0)
        {
            fmt = BTA_OP_ICAL_FMT;
        }
        else if (bta_ops_stricmp (p_suffix, "vmg") == 0)
        {
            fmt = BTA_OP_VMSG_FMT;
        } 
        else if (bta_ops_stricmp (p_suffix, "vnt") == 0)
        {
            fmt = BTA_OP_VNOTE_FMT;
        }
    }

    /* see if supported */
    if (fmt == BTA_OP_OTHER_FMT)
    {
        if (!(fmt_mask & BTA_OP_ANY_MASK))
            fmt = 0;    /* Other types not supported */
    }
    else
    {
        if (!((1 << (fmt - 1)) & fmt_mask))
            fmt = 0;
    }
        
    return fmt;
}

/*******************************************************************************
**
** Function         bta_ops_discard_data
**
** Description      frees the data
**
** Returns          void
**
*******************************************************************************/
void bta_ops_discard_data(UINT16 event, tBTA_OPS_DATA *p_data)
{
    switch(event)
    {
    case BTA_OPS_OBX_CONN_EVT:
    case BTA_OPS_OBX_DISC_EVT:
    case BTA_OPS_OBX_ABORT_EVT:
    case BTA_OPS_OBX_CLOSE_EVT:
    case BTA_OPS_OBX_PUT_EVT:
    case BTA_OPS_OBX_GET_EVT:
        utl_freebuf((void**)&p_data->obx_evt.p_pkt);
        break;

    default:
        /*Nothing to free*/
        break;
    }
}

/*******************************************************************************
*  Static Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_ops_stricmp
**
** Description      Used to compare type header
**                  
**
** Returns          void
**
*******************************************************************************/
static int bta_ops_stricmp (const char *p_str1, const char *p_str2)
{
    UINT16 i;
    UINT16 cmplen;

    if (!p_str1 || !p_str2)
        return (1);

    cmplen = strlen(p_str1);
    if (cmplen != strlen(p_str2))
        return (1);

    for (i = 0; i < cmplen; i++)
    {
        if (toupper(p_str1[i]) != toupper(p_str2[i]))
            return (i+1);
    }

    return 0;
}
#endif /* BTA_OP_INCLUDED */
