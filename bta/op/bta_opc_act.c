/*****************************************************************************
**
**  Name:           bta_opc_act.c
**
**  Description:    This file contains the file transfer action 
**                  functions for the state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include "gki.h"
#include "bta_sys.h"
#include "bd.h"
#include "port_api.h"
#include "obx_api.h"
#include "goep_util.h"
#include "sdp_api.h"
#include "bta_fs_api.h"
#include "bta_op_api.h"
#include "bta_opc_int.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "btm_api.h"
#include "rfcdefs.h"
#include "utl.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
/* sdp discovery database size */
#define BTA_OPC_DISC_SIZE       450


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if BTA_OPC_DEBUG == TRUE
static char *opc_obx_evt_code(tOBX_EVENT evt_code);
#endif

static void bta_opc_sdp_cback(UINT16 status);


/*****************************************************************************
**  Action Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_opc_enable
**
** Description      Handle an api enable event.  This function enables the OP
**                  Client by opening an Obex/Rfcomm channel with a peer device.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_enable(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{

    /* store parameters */
    p_cb->p_cback = p_data->api_enable.p_cback;
    p_cb->sec_mask = p_data->api_enable.sec_mask;
    p_cb->app_id = p_data->api_enable.app_id;
    p_cb->srm = p_data->api_enable.srm;
    p_cb->single_op = p_data->api_enable.single_op;
    p_cb->fd = BTA_FS_INVALID_FD;

    /* callback with enable event */
    (*p_cb->p_cback)(BTA_OPC_ENABLE_EVT, NULL);
}

/*******************************************************************************
**
** Function         bta_opc_init_push
**
** Description      Push an object to the OPP server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_init_push(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    p_cb->status = BTA_OPC_FAIL;
    p_cb->exch_status = BTA_OPC_OK;

    if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        p_cb->to_do = BTA_OPC_PUSH_MASK;
        p_cb->format = p_data->api_push.format;
        BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len+1, p_data->api_push.p_name, p_bta_fs_cfg->max_path_len);
        p_cb->p_name[p_bta_fs_cfg->max_path_len] = '\0';
    }
    else
        bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, (tBTA_OPC_DATA *)NULL);
}

/*******************************************************************************
**
** Function         bta_opc_init_pull
**
** Description      Pull an object off the server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_init_pull(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    p_cb->status = BTA_OPC_FAIL;
    p_cb->exch_status = BTA_OPC_OK;

    if ((p_cb->p_name = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_file_len +
                                           p_bta_fs_cfg->max_path_len + 2))) != NULL)
    {
        p_cb->p_rcv_path = p_cb->p_name + p_bta_fs_cfg->max_file_len + 1;
        p_cb->to_do = BTA_OPC_PULL_MASK;
        p_cb->first_get_pkt = TRUE;
        BCM_STRNCPY_S(p_cb->p_rcv_path, p_bta_fs_cfg->max_path_len+1, p_data->api_pull.p_path, p_bta_fs_cfg->max_path_len);
        p_cb->p_rcv_path[p_bta_fs_cfg->max_path_len] = '\0';
    }
    else
        bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, (tBTA_OPC_DATA *)NULL);
}

/*******************************************************************************
**
** Function         bta_opc_init_exch
**
** Description      Exchange business cards with a server.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_init_exch(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    p_cb->status = BTA_OPC_FAIL;
    p_cb->exch_status = BTA_OPC_OK;

    /* Need room to hold the receive path, along with path and file name to push */
    if ((p_cb->p_name =
        (char *)GKI_getbuf((UINT16)((p_bta_fs_cfg->max_path_len + 1) * 2))) != NULL)
    {
        p_cb->p_rcv_path = p_cb->p_name + p_bta_fs_cfg->max_path_len + 1;
        p_cb->to_do =  BTA_OPC_PULL_MASK | BTA_OPC_PUSH_MASK;
        p_cb->first_get_pkt = TRUE;
        BCM_STRNCPY_S(p_cb->p_name, p_bta_fs_cfg->max_path_len+1, p_data->api_exch.p_send, p_bta_fs_cfg->max_path_len);
        p_cb->p_name[p_bta_fs_cfg->max_path_len] = '\0';
        BCM_STRNCPY_S(p_cb->p_rcv_path, p_bta_fs_cfg->max_path_len+1, p_data->api_exch.p_rcv_path, p_bta_fs_cfg->max_path_len);
        p_cb->p_rcv_path[p_bta_fs_cfg->max_path_len] = '\0';
    }
    else
        bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, (tBTA_OPC_DATA *)NULL);
}

/*******************************************************************************
**
** Function         bta_opc_trans_cmpl
**
** Description      push/pull complete
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_trans_cmpl(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBJECT param;
    tBTA_OPC_EVT    evt = BTA_OPC_OBJECT_EVT;
    tBTA_FS_CO_STATUS status;

    if (p_data)
    {
        p_cb->status = bta_opc_convert_obx_to_opc_status(p_data->obx_evt.rsp_code);
    }
    else
    {
        /* some action functions send the event to SM with NULL p_data */
        p_cb->status = BTA_OPC_FAIL;
    }

    if (p_cb->obx_oper == OPC_OP_PUSH_OBJ)
        evt = BTA_OPC_OBJECT_PSHD_EVT;

    /* rearrange the code a little bit to make sure p_cb->status is for the current op
     * (previously, we might use the ststus from the PUSH op, if this is an exchange card) */
    param.status = p_cb->status;

    /* If exchange, and push received an error use this error */
    if (p_cb->exch_status != BTA_OPC_OK)
        param.status = p_cb->exch_status;

    /* Notify appl the result of the pull or push */
    param.p_name = p_cb->p_name;
    p_cb->p_cback(evt, (tBTA_OPC *)&param);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;
 
        /* Delete an aborted unfinished get vCard operation */
        if (p_cb->obx_oper == OPC_OP_PULL_OBJ && p_cb->status != BTA_OPC_OK)
        {
            status = bta_fs_co_unlink(p_cb->p_name, p_cb->app_id);
            APPL_TRACE_WARNING2("OPC: Remove ABORTED Get File Operation [%s], status 0x%02x",
                                 p_cb->p_name, status);
        }
    }

    utl_freebuf((void**)&p_cb->p_name);

    p_cb->obx_oper = OPC_OP_NONE;

}

/*******************************************************************************
**
** Function         bta_opc_ci_write
**
** Description      Continue with the current write operation
**                  (Get File processing)
**
** Returns          void
**
*******************************************************************************/
void bta_opc_ci_write(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_OPC_PROGRESS param;
    UINT8             rsp_code = OBX_RSP_FAILED;

    p_cb->cout_active = FALSE;

    /* Process write call-in event if operation is still active */
    if (p_cb->obx_oper == OPC_OP_PULL_OBJ)
    {
        if (p_data->write_evt.status == BTA_FS_CO_OK)
        {
            param.bytes = p_obx->offset;
            param.obj_size = p_cb->obj_size;
            param.operation = BTA_OP_OPER_PULL;
            p_cb->p_cback(BTA_OPC_PROGRESS_EVT, (tBTA_OPC *)&param);

            /* Send another Get request if not finished */
            if (!p_obx->final_pkt)
            {
                /* Free current packet and send a new request */
                bta_opc_send_get_req(p_cb);
                rsp_code = OBX_RSP_CONTINUE;
            }
            else
                rsp_code = OBX_RSP_OK;
        }

        if (rsp_code != OBX_RSP_CONTINUE)
        {
            p_data->obx_evt.rsp_code = rsp_code;
            bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_opc_ci_read
**
** Description      Handles the response to a read call-out request.
**                  This is called within the OBX get object request.  The
**                  operation has completed, send the OBX packet out.
**
** Returns          void
**
*******************************************************************************/
void bta_opc_ci_read(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_PKT    *p_obx = &p_cb->obx;
    tBTA_FS_CI_READ_EVT *p_revt = &p_data->read_evt;
    tBTA_OPC_PROGRESS    param;
    BOOLEAN              is_final;

    p_cb->cout_active = FALSE;

    /* Process read call-in event if operation is still active */
    if (p_cb->obx_oper == OPC_OP_PUSH_OBJ)
    {
        if (p_revt->status != BTA_FS_CO_OK && p_revt->status != BTA_FS_CO_EOF)
        {
            p_data->obx_evt.rsp_code = OBX_RSP_FAILED;
/* if abort added to OPC use ->  bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, p_data); */
            bta_opc_sm_execute(p_cb, BTA_OPC_API_CLOSE_EVT, p_data);
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
            param.bytes = p_revt->num_read;
            param.obj_size = p_cb->obj_size;
            param.operation = BTA_OP_OPER_PUSH;
            p_cb->p_cback(BTA_OPC_PROGRESS_EVT, (tBTA_OPC *)&param);
        }
    }
}

/*******************************************************************************
**
** Function         bta_opc_ci_open
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_ci_open(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FS_CI_OPEN_EVT *p_open = &p_data->open_evt;
    UINT8 rsp_code = OBX_RSP_FAILED;
   
    p_cb->cout_active = FALSE;

    /* if file is accessible read/write the first buffer of data */
    if (p_open->status == BTA_FS_CO_OK)
    {
        p_cb->fd = p_open->fd;
            
        if (p_cb->obx_oper == OPC_OP_PUSH_OBJ)
        {
            p_cb->obj_size = p_open->file_size;

            rsp_code = bta_opc_send_put_req(p_cb, TRUE);
        }
        else if (p_cb->obx_oper == OPC_OP_PULL_OBJ)
        {
            rsp_code = OBX_RSP_OK;

            /* Initiate the first OBX GET request */
            p_obx->offset = 0;

            /* Continue processing GET rsp */
            bta_opc_cont_get_rsp(p_cb);
        }
    }
    else
    {
        if (p_open->status == BTA_FS_CO_EACCES)
            rsp_code = OBX_RSP_UNAUTHORIZED;
        else    /* File could not be found */
            rsp_code = OBX_RSP_NOT_FOUND;
    }

    if (rsp_code != OBX_RSP_OK)
    {
        p_data->obx_evt.rsp_code = rsp_code;
        bta_opc_sm_execute(p_cb, BTA_OPC_API_CLOSE_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_opc_obx_conn_rsp
**
** Description      Process the OBX connect event.
**                  If OPP service, get directory listing.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_obx_conn_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_EVT  *p_evt = &p_data->obx_evt;
    
    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
    
    p_cb->peer_mtu = p_data->obx_evt.param.conn.mtu;
    
    /* inform role manager */
    bta_sys_conn_open( BTA_ID_OPC ,p_cb->app_id, bta_opc_cb.bd_addr);

    p_cb->p_cback(BTA_OPC_OPEN_EVT, NULL);
    
    /* start the first operation. For card exchange PUSH is done first */
    if(p_cb->to_do &  BTA_OPC_PUSH_MASK)
        bta_opc_start_push(p_cb);
    else
        bta_opc_send_get_req(p_cb);
}

/*******************************************************************************
**
** Function         bta_opc_obx_put_rsp
**
** Description      Process the OBX file put and delete file/folder events
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_obx_put_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_EVT    *p_evt = &p_data->obx_evt;
    tBTA_OPC_OBJECT      param;

    p_cb->req_pending = FALSE;
    APPL_TRACE_DEBUG1("bta_opc_obx_put_rsp to_do 0x%02x",p_cb->to_do);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

    if (p_cb->obx_oper == OPC_OP_PUSH_OBJ)
    {
        /* If not finished with Put, start another read */
        if (p_evt->rsp_code == OBX_RSP_CONTINUE)
            bta_opc_send_put_req(p_cb, FALSE);

        /* Start the Pull if this is card exchange */
        else if (p_cb->to_do & BTA_OPC_PULL_MASK)
        {
            /* Close the current file and initiate a pull vcard */
            bta_fs_co_close(p_cb->fd, p_cb->app_id);
            p_cb->fd = BTA_FS_INVALID_FD;

            /* If error occurred during push save it */
            if (p_evt->rsp_code != OBX_RSP_OK)
                p_cb->exch_status = BTA_OPC_FAIL;

            /* Notify application with status of push operation */
            param.status = p_cb->exch_status;
            param.p_name = p_cb->p_name;
            p_cb->p_cback(BTA_OPC_OBJECT_PSHD_EVT, (tBTA_OPC *)&param);

            /* Initiate the Pull operation */
            bta_opc_send_get_req(p_cb);
        }
        else    /* Finished or an error occurred */
        {
            p_data->obx_evt.rsp_code = p_evt->rsp_code;
            bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_opc_obx_get_rsp
**
** Description      Process the OBX file get and folder listing events
**                  If the type header is not folder listing, then pulling a file.
**
** Returns          void
**
*******************************************************************************/
void bta_opc_obx_get_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_EVT *p_evt = &p_data->obx_evt;
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    BOOLEAN           free_pkt = FALSE;

    p_obx->final_pkt = (p_evt->rsp_code == OBX_RSP_OK) ? TRUE : FALSE;
    p_obx->p_pkt = p_evt->p_pkt;
    p_obx->rsp_code = p_evt->rsp_code;
    p_cb->req_pending = FALSE;

    if (p_cb->obx_oper == OPC_OP_PULL_OBJ)
    {
        /* Open file for writing */
        if (p_cb->first_get_pkt == TRUE)
        {
            p_cb->first_get_pkt = FALSE;
            free_pkt = bta_opc_proc_get_rsp(p_cb, p_data);
        }
        else    /* Continuation of the object transfer */
            bta_opc_cont_get_rsp(p_cb);
    }
    else
        free_pkt = TRUE;

    if (free_pkt)    /* Release the OBX response packet */
        utl_freebuf((void**)&p_obx->p_pkt);
}

/*******************************************************************************
**
** Function         bta_opc_initialize
**
** Description      Initialize the control block.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_initialize(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_FS_CO_STATUS   status;

    utl_freebuf((void**)&p_cb->obx.p_pkt);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;

        /* Delete an aborted unfinished get vCard operation */
        if (p_cb->obx_oper == OPC_OP_PULL_OBJ && p_cb->status != BTA_OPC_OK)
        {
            status = bta_fs_co_unlink(p_cb->p_name, p_cb->app_id);
            APPL_TRACE_WARNING2("OPC: Remove ABORTED Get File Operation [%s], status 0x%02x",
                                 p_cb->p_name, status);
        }
    }

    utl_freebuf((void**)&p_cb->p_name);

    /* Clean up control block */
    p_cb->obx_oper = OPC_OP_NONE;
    p_cb->req_pending = FALSE;
    p_cb->sdp_pending = FALSE;
    p_cb->to_do = 0;
    p_cb->p_rcv_path = NULL;
    p_cb->first_get_pkt = FALSE;

    if (p_cb->disabling)
    {
        p_cb->disabling = FALSE;
        bta_opc_sm_execute(p_cb, BTA_OPC_DISABLE_CMPL_EVT, NULL);
    }
}

/*******************************************************************************
**
** Function         bta_opc_stop_client
**
** Description      Stop OBX client.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_stop_client(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    if (!p_cb->sdp_pending)
    {
// btla-specific ++
#if defined (BTA_OPC_SENDING_ABORT) && (BTA_OPC_SENDING_ABORT == TRUE) 
        APPL_TRACE_WARNING3("bta_opc_stop_client p_cb->req_pending = %d, p_cb->sdp_pending = %d,handle = %d",
                             p_cb->req_pending, p_cb->sdp_pending,p_cb->obx_handle);
        /* Abort an active request */
        if (p_cb->req_pending)
            OBX_AbortReq(p_cb->obx_handle, (BT_HDR *)NULL);
#endif
// btla-specific --
        /* do not free p_cb->obx.p_pkt here, just in case cout_active.
         * bta_opc_close_complete would handle it */
        OBX_DisconnectReq(p_cb->obx_handle, NULL);
    }
}

/*******************************************************************************
**
** Function         bta_opc_close
**
** Description      If not waiting for a call-in function, complete the closing
**                  of the channel.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_close(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    /* finished if not waiting on a call-in function */
    if (!p_cb->cout_active)
        bta_opc_sm_execute(p_cb, BTA_OPC_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_opc_start_client
**
** Description      Start an OPP operation.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_start_client(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tBTA_OPC_OBX_PKT *p_obx = &p_cb->obx;
    tOBX_STATUS      status;
    BOOLEAN          srm = p_cb->srm;

    /* Allocate an OBX packet */
    if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(OBX_HANDLE_NULL, OBX_MAX_MTU)) != NULL)
    {
        status = OBX_AllocSession (NULL, p_data->sdp_ok.scn, &p_data->sdp_ok.psm,
                              bta_opc_obx_cback, &p_cb->obx_handle);

        /* set security level */
        if (p_data->sdp_ok.scn)
        {
            BTM_SetSecurityLevel (TRUE, "BTA_OPC", BTM_SEC_SERVICE_OBEX,
                                  p_cb->sec_mask, BT_PSM_RFCOMM,
                                  BTM_SEC_PROTO_RFCOMM, p_data->sdp_ok.scn);
            srm = 0;
        }
        else
        {
            BTM_SetSecurityLevel (TRUE, "BTA_OPC", BTM_SEC_SERVICE_OBEX,
                                  p_cb->sec_mask, p_data->sdp_ok.psm, 0, 0);
        }

        if (status == OBX_SUCCESS)
        {
            OBX_CreateSession (p_cb->bd_addr, OBX_MAX_MTU, srm, 0,
                               p_cb->obx_handle, p_obx->p_pkt);
            p_obx->p_pkt = NULL;    /* OBX will free the memory */
            return;
        }
    }
    else
    {
        p_data->obx_evt.rsp_code = OBX_RSP_FAILED;
        bta_opc_sm_execute(p_cb, BTA_OPC_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_opc_free_db
**
** Description      Free buffer used for service discovery database.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_free_db(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    utl_freebuf((void**)&p_cb->p_db);
    p_cb->sdp_pending = FALSE;
}

/*******************************************************************************
**
** Function         bta_opc_ignore_obx
**
** Description      Free OBX packet for ignored OBX events.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_ignore_obx(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    utl_freebuf((void**)&p_data->obx_evt.p_pkt);
}

/*******************************************************************************
**
** Function         bta_opc_find_service
**
** Description      Perform service discovery to find the OPP service on the
**                  peer device.
**                  
** Returns          void
**
*******************************************************************************/
void bta_opc_find_service(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    tSDP_UUID   uuid_list;
    UINT16      attr_list[3];
    
    if (!p_cb->to_do)
        return;
    
    bdcpy(p_cb->bd_addr, p_data->api_push.bd_addr);
    if ((p_cb->p_db = (tSDP_DISCOVERY_DB *) GKI_getbuf(BTA_OPC_DISC_SIZE)) != NULL)
    {
        attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
        attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;
        attr_list[2] = ATTR_ID_OBX_OVR_L2CAP_PSM;
        
        uuid_list.len = LEN_UUID_16;
        uuid_list.uu.uuid16 = UUID_SERVCLASS_OBEX_OBJECT_PUSH;
        p_cb->sdp_pending = TRUE;
        SDP_InitDiscoveryDb(p_cb->p_db, BTA_OPC_DISC_SIZE, 1, &uuid_list, 3, attr_list);
        if(!SDP_ServiceSearchAttributeRequest(p_cb->bd_addr, p_cb->p_db, bta_opc_sdp_cback))
        {
            bta_opc_sm_execute(p_cb, BTA_OPC_SDP_FAIL_EVT, (tBTA_OPC_DATA *)NULL);
        }
    }
    else
        bta_opc_sm_execute(p_cb, BTA_OPC_OBX_CMPL_EVT, (tBTA_OPC_DATA *)NULL);
}

/*******************************************************************************
**
** Function         bta_opc_close_complete
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_close_complete(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    p_cb->cout_active = FALSE;
    bta_opc_initialize(p_cb, p_data);
    
    /* inform role manager */
    bta_sys_conn_close( BTA_ID_OPC ,p_cb->app_id, bta_opc_cb.bd_addr);

    p_cb->p_cback(BTA_OPC_CLOSE_EVT, (tBTA_OPC *)&p_cb->status);
}

/*******************************************************************************
**
** Function         bta_opc_set_disable
**
** Description      Sets flag to disable.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_set_disable(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    p_cb->disabling = TRUE;
}

/*******************************************************************************
**
** Function         bta_opc_chk_close
**
** Description      Check if we need to stop the client now.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_chk_close(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    if (p_cb->single_op)
    {
        bta_opc_sm_execute(p_cb, BTA_OPC_API_CLOSE_EVT, (tBTA_OPC_DATA *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_opc_do_pull
**
** Description      
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_do_pull(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    APPL_TRACE_DEBUG1("bta_opc_do_pull to_do 0x%02x",p_cb->to_do);
    bta_opc_send_get_req(p_cb);
}

/*******************************************************************************
**
** Function         bta_opc_do_push
**
** Description      
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_do_push(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data)
{
    APPL_TRACE_DEBUG1("bta_opc_do_push to_do 0x%02x",p_cb->to_do);
    bta_opc_start_push(p_cb);
}

/*****************************************************************************
**  Callback Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_opc_obx_cback
**
** Description      OBX callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_opc_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event, UINT8 rsp_code,
                        tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_OPC_OBX_EVT *p_obx_msg;
    UINT16            event = 0;

#if BTA_OPC_DEBUG == TRUE
    APPL_TRACE_DEBUG1("OBX Event Callback: obx_event [%s]", opc_obx_evt_code(obx_event));
#endif

    switch(obx_event)
    {
    case OBX_CONNECT_RSP_EVT:
        if (rsp_code == OBX_RSP_OK)
        {
            event = BTA_OPC_OBX_CONN_RSP_EVT;
        }
        else    /* Obex will disconnect underneath BTA */
        {
            APPL_TRACE_WARNING1("OPC_CBACK: Bad connect response 0x%02x", rsp_code);
            if (p_pkt)
                GKI_freebuf(p_pkt);
            return;
        }
        break;
    case OBX_PUT_RSP_EVT:
        event = BTA_OPC_OBX_PUT_RSP_EVT;
        break;
    case OBX_GET_RSP_EVT:
        event = BTA_OPC_OBX_GET_RSP_EVT;
        break;
    case OBX_CLOSE_IND_EVT:
        event = BTA_OPC_OBX_CLOSE_EVT;
        break;
    case OBX_PASSWORD_EVT:
        event = BTA_OPC_OBX_PASSWORD_EVT;
        break;
        
    default:
/*  case OBX_ABORT_RSP_EVT: */
/*  case OBX_DISCONNECT_RSP_EVT: Handled when OBX_CLOSE_IND_EVT arrives */
        if (p_pkt)
            GKI_freebuf(p_pkt);
        return;
    }
    
    /* send event to BTA, if any */
    if ((p_obx_msg = (tBTA_OPC_OBX_EVT *) GKI_getbuf(sizeof(tBTA_OPC_OBX_EVT))) != NULL)
    {
        p_obx_msg->hdr.event = event;
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
        p_obx_msg->rsp_code = rsp_code;
        p_obx_msg->param = param;
        p_obx_msg->p_pkt = p_pkt;

        bta_sys_sendmsg(p_obx_msg);
    }
}

/******************************************************************************
**
** Function         bta_opc_sdp_cback
**
** Description      This is the SDP callback function used by OPC.
**                  This function will be executed by SDP when the service
**                  search is completed.  If the search is successful, it
**                  finds the first record in the database that matches the
**                  UUID of the search.  Then retrieves the scn from the
**                  record.
**
** Returns          Nothing.
**
******************************************************************************/
static void bta_opc_sdp_cback(UINT16 status)
{
    tBTA_OPC_SDP_OK_EVT *p_buf;
    tSDP_DISC_REC       *p_rec = NULL;
    tSDP_PROTOCOL_ELEM  pe;
    UINT8               scn = 0;
    BOOLEAN             found = FALSE;
    UINT16              version = GOEP_LEGACY_VERSION;
    UINT16               psm = 0;
    tSDP_DISC_ATTR      *p_attr;

    APPL_TRACE_DEBUG1("bta_opc_sdp_cback status:%d", status);

    if ( (status == SDP_SUCCESS) || (status == SDP_DB_FULL) )
    {
        status = SDP_SUCCESS;
        /* loop through all records we found */
        do
        {
            /* get next record; if none found, we're done */
            if ((p_rec = SDP_FindServiceInDb(bta_opc_cb.p_db, 
                            UUID_SERVCLASS_OBEX_OBJECT_PUSH, p_rec)) == NULL)
                break;

            /* this is an optional attribute */
            SDP_FindProfileVersionInRec (p_rec, UUID_SERVCLASS_OBEX_OBJECT_PUSH, &version);

            /* get psm from proto desc list alternative; if not found, go to next record */
            if ((p_attr = SDP_FindAttributeInRec(p_rec,
                            ATTR_ID_OBX_OVR_L2CAP_PSM)) != NULL)
            {
                psm = p_attr->attr_value.v.u16;
                if ((SDP_DISC_ATTR_LEN(p_attr->attr_len_type) == 2) && L2C_IS_VALID_PSM(psm))
                {
                    found = TRUE;
                    if (version == GOEP_LEGACY_VERSION)
                    {
                        APPL_TRACE_ERROR0("Lacking mandatory attribute/version");
                        version = GOEP_ENHANCED_VERSION;
                    }
                    break;
                }
            }

            /* If no OBEX over L2CAP look for RFCOMM SCN */
            if (!found)
            {
                /* get scn from proto desc list; if not found, go to next record */
                if (SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe))
                {
                    scn = (UINT8) pe.params[0];

                    /* we've got everything, we're done */
                    found = TRUE;
                    break;
                }
                else
                {
                    continue;
                }
            }
        } while (TRUE);
    }

    /* send result in event back to BTA */
    if ((p_buf = (tBTA_OPC_SDP_OK_EVT *) GKI_getbuf(sizeof(tBTA_OPC_SDP_OK_EVT))) != NULL)
    {
        if ((status == SDP_SUCCESS) && (found == TRUE))
        {
            p_buf->hdr.event = BTA_OPC_SDP_OK_EVT;
            p_buf->scn = scn;
            p_buf->psm = psm;
            p_buf->version = version;
        }
        else
            p_buf->hdr.event = BTA_OPC_SDP_FAIL_EVT;

        bta_sys_sendmsg(p_buf);
    }
}

/*****************************************************************************
**  Local OPP Event Processing Functions
*****************************************************************************/

/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_OPC_DEBUG == TRUE

/*******************************************************************************
**
** Function         opc_obx_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *opc_obx_evt_code(tOBX_EVENT evt_code)
{
    switch(evt_code)
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
    default:
        return "unknown OBX event code";
    }
}
#endif  /* Debug Functions */
