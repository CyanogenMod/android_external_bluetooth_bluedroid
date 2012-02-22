/*****************************************************************************
**
**  Name:           bta_ops_act.c
**
**  Description:    This file contains the object transfer action 
**                  functions for the state machine.
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
#include "obx_api.h"
#include "bta_op_api.h"
#include "bta_ops_int.h"
#include "bta_fs_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "rfcdefs.h"    /* BT_PSM_RFCOMM */
#include "btm_api.h"
#include "utl.h"
#include "goep_util.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

#define BTA_OPS_NUM_FMTS        7
#define BTA_OPS_PROTOCOL_COUNT  3

/* object format lookup table */
const tBTA_OP_FMT bta_ops_obj_fmt[] =
{
    BTA_OP_VCARD21_FMT,
    BTA_OP_VCARD30_FMT,
    BTA_OP_VCAL_FMT,    
    BTA_OP_ICAL_FMT,   
    BTA_OP_VNOTE_FMT,   
    BTA_OP_VMSG_FMT,
    BTA_OP_OTHER_FMT
};    

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if BTA_OPS_DEBUG == TRUE
static char *ops_obx_evt_code(tOBX_EVENT evt_code);
#endif

/*****************************************************************************
**  Action Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_ops_enable
**
** Description      Perform necessary operations to enable object push.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_enable(tBTA_OPS_CB *p_cb, tBTA_OPS_API_ENABLE *p_data)
{
    tOBX_StartParams    start_params;
    UINT16      servclass = UUID_SERVCLASS_OBEX_OBJECT_PUSH;
    int         i, j;
    tBTA_UTL_COD cod;
    tOBX_STATUS status;
    UINT8       desc_type[BTA_OPS_NUM_FMTS];
    UINT8       type_len[BTA_OPS_NUM_FMTS];
    UINT8       *type_value[BTA_OPS_NUM_FMTS];
    UINT16      mtu = OBX_MAX_MTU;
    UINT8       temp[4], *p;
    UINT16      version = GOEP_ENHANCED_VERSION;
    
    /* allocate scn for opp */
    p_cb->scn = BTM_AllocateSCN();
    p_cb->app_id = p_data->app_id;
    
    /* set security level */
    BTM_SetSecurityLevel (FALSE, (char *) "", BTM_SEC_SERVICE_OBEX,
                          p_data->sec_mask, BT_PSM_RFCOMM,
                          BTM_SEC_PROTO_RFCOMM, p_cb->scn);

    p_cb->psm = L2CA_AllocatePSM();
    BTM_SetSecurityLevel (FALSE, (char *) "", BTM_SEC_SERVICE_OBEX,
                          p_data->sec_mask, p_cb->psm, 0, 0);

    memset (&start_params, 0, sizeof(tOBX_StartParams));
    start_params.p_target = NULL;
    start_params.p_cback = &bta_ops_obx_cback;
    start_params.mtu = mtu;
    start_params.scn = p_cb->scn;
    start_params.psm = p_cb->psm;
    start_params.srm = p_cb->srm;
    start_params.nonce = 0;
    start_params.authenticate = FALSE;
    start_params.auth_option = OBX_AO_NONE;
    start_params.realm_charset = OBX_RCS_ASCII;
    start_params.p_realm = NULL;
    start_params.realm_len = 0;
    
    if ((status = OBX_StartServer (&start_params, &p_cb->obx_handle)) == OBX_SUCCESS)
    {
        status = GOEP_Register (p_data->name, &p_cb->sdp_handle, p_cb->scn, 1, &servclass,
            servclass, version);

        /* add the psm */
        p = temp;
        UINT16_TO_BE_STREAM(p, p_cb->psm);
        SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_OBX_OVR_L2CAP_PSM, UINT_DESC_TYPE,
                  (UINT32)2, (UINT8*)temp);

        /* add sequence for supported types */
        for (i = 0, j = 0; i < BTA_OPS_NUM_FMTS; i++)
        {
            if ((p_data->formats >> i) & 1)
            {
                type_value[j] = (UINT8 *) &bta_ops_obj_fmt[i];
                desc_type[j] = UINT_DESC_TYPE;
                type_len[j++] = 1;
            }
        }
        
        SDP_AddSequence(p_cb->sdp_handle, (UINT16) ATTR_ID_SUPPORTED_FORMATS_LIST,
            (UINT8) j, desc_type, type_len, type_value);
        
        /* set class of device */
        cod.service = BTM_COD_SERVICE_OBJ_TRANSFER;
        utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);
        
        /* store formats value */
        p_cb->formats = p_data->formats;
    }
    bta_sys_add_uuid(servclass); /* UUID_SERVCLASS_OBEX_OBJECT_PUSH */

    p_cb->p_cback(BTA_OPS_ENABLE_EVT, NULL);
}

/*******************************************************************************
**
** Function         bta_ops_api_disable
**
** Description      Perform necessary operations to disable object push.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_api_disable(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    /* Free any outstanding headers and control block memory */
    bta_ops_clean_getput(p_cb, TRUE);

    /* Stop the OBEX server */
    OBX_StopServer(p_cb->obx_handle);

    /* Remove the OPP service from the SDP database */
    SDP_DeleteRecord(p_cb->sdp_handle);
    bta_sys_remove_uuid(UUID_SERVCLASS_OBEX_OBJECT_PUSH);

    /* Free the allocated server channel number */
    BTM_FreeSCN(p_cb->scn);
    BTM_SecClrService(BTM_SEC_SERVICE_OBEX);
}

/*******************************************************************************
**
** Function         bta_ops_api_accessrsp
**
** Description      Process the access API event.
**                  If permission had been granted, continue the push or pull
**                  operation.
**
** Returns          void
**
*******************************************************************************/
void bta_ops_api_accessrsp(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    UINT8 rsp_code;

    /* Process the currently active access response */
    switch (p_cb->acc_active)
    {
    case BTA_OP_OPER_PUSH:
        if (p_data->api_access.oper == BTA_OP_OPER_PUSH)
        {
            if (p_data->api_access.flag == BTA_OP_ACCESS_ALLOW)
            {
                /* Save the file name with path prepended */
                BCM_STRCPY_S(p_cb->p_path, p_bta_fs_cfg->max_path_len+1, p_data->api_access.p_name);

                APPL_TRACE_DEBUG2("OPS PUSH OBJ: Name [%s], Length = 0x%0x (0 = n/a)",
                                  p_cb->p_path, p_cb->file_length);
                    
                p_cb->cout_active = TRUE;
                bta_fs_co_open (p_cb->p_path,
                                (BTA_FS_O_CREAT | BTA_FS_O_TRUNC | BTA_FS_O_RDWR),
                                p_cb->file_length, BTA_OPS_CI_OPEN_EVT,
                                p_cb->app_id);
            }
            else    /* Access denied or Unsupported */
            {
                rsp_code = (p_data->api_access.flag == BTA_OP_ACCESS_NONSUP)
                         ? OBX_RSP_UNSUPTD_TYPE : OBX_RSP_UNAUTHORIZED;
                bta_ops_clean_getput(p_cb, TRUE);
                OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
            }
            p_cb->acc_active = 0;
        }

        break;

    case BTA_OP_OPER_PULL:
        if (p_data->api_access.oper == BTA_OP_OPER_PULL)
        {
            if (p_data->api_access.flag == BTA_OP_ACCESS_ALLOW)
            {
                /* Save the file name with path prepended */
                BCM_STRCPY_S(p_cb->p_path, p_bta_fs_cfg->max_path_len+1, p_data->api_access.p_name);

                APPL_TRACE_DEBUG1("OPS PULL VCARD: Name [%s]", p_cb->p_path);
                    
                p_cb->cout_active = TRUE;
                bta_fs_co_open (p_cb->p_path, BTA_FS_O_RDONLY, 0,
                                        BTA_OPS_CI_OPEN_EVT, p_cb->app_id);
            }
            else    /* Denied */
                bta_ops_get_obj_rsp(OBX_RSP_UNAUTHORIZED, 0);

            p_cb->acc_active = 0;
        }
        break;

    default:
        APPL_TRACE_WARNING1("OPS ACCRSP: Unknown tBTA_OP_OPER value (%d)",
                            p_cb->acc_active);
    }
}

/*******************************************************************************
**
** Function         bta_ops_api_close
**
** Description      Handle an api close event.  
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_api_close(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    /* resources will be freed at BTA_OPS_OBX_CLOSE_EVT */
    OBX_DisconnectRsp(p_cb->obx_handle, OBX_RSP_SERVICE_UNAVL, NULL);
}

/*******************************************************************************
**
** Function         bta_ops_ci_write
**
** Description      Continue with the current write operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_ci_write(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    UINT8 rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    
    p_cb->cout_active = FALSE;
    
    if (!p_cb->aborting)
    {
        /* Process write call-in event if operation is still active */
        if (p_cb->obx_oper == OPS_OP_PUSH_OBJ)
        {
            if (p_data->write_evt.status == BTA_FS_CO_OK)
                rsp_code = OBX_RSP_OK;
            else
            {
                if (p_data->write_evt.status == BTA_FS_CO_ENOSPACE)
                    rsp_code = OBX_RSP_DATABASE_FULL;
                bta_ops_clean_getput(p_cb, TRUE);
            }
            
            /* Process response to OBX client */
            bta_ops_put_obj_rsp(rsp_code);
        }
    }
    else    /* Finish aborting */
    {
        bta_ops_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ops_ci_read
**
** Description      Handles the response to a read call-out request.
**                  This is called within the OBX get file request.  If the
**                  operation has completed, the OBX response is sent out;
**                  otherwise a read for additional data is made.
**
** Returns          void
**
*******************************************************************************/
void bta_ops_ci_read(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_FS_CI_READ_EVT *p_revt = &p_data->read_evt;
    UINT8 rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    
    p_cb->cout_active = FALSE;
    
    if (!p_cb->aborting)
    {
        /* Process read call-in event if operation is still active */
        if (p_cb->obx_oper == OPS_OP_PULL_OBJ && p_revt->fd == p_cb->fd)
        {
            /* Read was successful, not finished yet */
            if (p_revt->status == BTA_FS_CO_OK)
                rsp_code = OBX_RSP_CONTINUE;
            
            /* Read was successful, end of file has been detected */
            else if (p_revt->status == BTA_FS_CO_EOF)
                rsp_code = OBX_RSP_OK;
            
            /* Process response to OBX client */
            bta_ops_get_obj_rsp(rsp_code, p_revt->num_read);
        }
    }
    else    /* Finish aborting */
    {
        bta_ops_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
        APPL_TRACE_ERROR0("OPS PUSH OBJ: Finished ABORTING!!!");
    }
}

/*******************************************************************************
**
** Function         bta_ops_ci_open
**
** Description      Continue with the current file open operation
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_ci_open(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_PKT    *p_obx = &p_cb->obx;
    tBTA_FS_CI_OPEN_EVT *p_open = &p_data->open_evt;
    UINT8                rsp_code = OBX_RSP_OK;
    UINT8                num_hdrs;
    BOOLEAN              endpkt;
    char                *p_name;
    
    p_cb->cout_active = FALSE;
    
    if (p_cb->aborting)
    {
        bta_ops_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
        return;
    }

    /* Only process file get or put operations */
    if (p_cb->obx_oper == OPS_OP_PULL_OBJ)
    {
        /* if file is accessible read/write the first buffer of data */
        if (p_open->status == BTA_FS_CO_OK)
        {
            p_cb->fd = p_open->fd;
            p_cb->file_length = p_open->file_size;

            /* Add the name and length headers */
            p_name = strrchr(p_cb->p_path, (int)p_bta_fs_cfg->path_separator);
            if (p_name == NULL)
                p_name = p_cb->p_path;
            else
                p_name++;   /* increment past the file separator */

            OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)p_name);

            if (p_cb->file_length != BTA_FS_LEN_UNKNOWN)
            {
                OBX_AddLengthHdr(p_obx->p_pkt, p_cb->file_length);
                if (p_cb->file_length > 0)
                {
                    rsp_code = OBX_RSP_CONTINUE;
                }
            }

            /* Send continuation response with the length of the file and no body */
            OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
            p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
        }
        else
        {
            if (p_open->status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else    /* File could not be found */
                rsp_code = OBX_RSP_NOT_FOUND;
            
            /* Send OBX response if an error occurred */
            bta_ops_get_obj_rsp(rsp_code, 0);
        }
    }
    else if (p_cb->obx_oper == OPS_OP_PUSH_OBJ)
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
                                p_obx->bytes_left, BTA_OPS_CI_WRITE_EVT,
                                0, p_cb->app_id);
            }
            else if (num_hdrs > 1)  /* Too many body headers to handle */
            {
                rsp_code = OBX_RSP_BAD_REQUEST;
                bta_ops_clean_getput(p_cb, TRUE);
            }
            else    /* No body: respond with an OK so client can start sending the data */
                p_obx->bytes_left = 0;
        }
        else
        {
            if (p_open->status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;
            else if (p_open->status == BTA_FS_CO_ENOSPACE)
                rsp_code = OBX_RSP_DATABASE_FULL;
            else
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }

        /* Send OBX response now if an error occurred or no body */
        if (rsp_code != OBX_RSP_PART_CONTENT)
            bta_ops_put_obj_rsp(rsp_code);
    }
}

/*******************************************************************************
**
** Function         bta_ops_obx_connect
**
** Description      Process the OBX connect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_connect(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    
    p_cb->peer_mtu = p_evt->param.conn.mtu;
    memcpy(p_cb->bd_addr, p_evt->param.conn.peer_addr, BD_ADDR_LEN);
    APPL_TRACE_EVENT1("OPS Connect: peer mtu 0x%04x", p_cb->peer_mtu);
    
    /* done with obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

    OBX_ConnectRsp(p_evt->handle, OBX_RSP_OK, (BT_HDR *)NULL);
    
    /* inform role manager */
    bta_sys_conn_open( BTA_ID_OPS ,p_cb->app_id, bta_ops_cb.bd_addr);

    /* Notify the MMI that a connection has been opened */
    p_cb->p_cback(BTA_OPS_OPEN_EVT, (tBTA_OPS*)bta_ops_cb.bd_addr);
}

/*******************************************************************************
**
** Function         bta_ops_obx_disc
**
** Description      Process the OBX disconnect event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_disc(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code;

    rsp_code = (p_evt->obx_event == OBX_DISCONNECT_REQ_EVT) ? OBX_RSP_OK
                                                            : OBX_RSP_BAD_REQUEST;

    /* Action operation is not supported in OPP, send reject rsp and free data */
    if(p_evt->obx_event == OBX_ACTION_REQ_EVT)
    {
        OBX_ActionRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
    }

    OBX_DisconnectRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_ops_obx_close
**
** Description      Process the OBX link lost event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_close(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    /* finished if not waiting on a call-in function */
    if (!p_cb->cout_active)
        bta_ops_sm_execute(p_cb, BTA_OPS_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_ops_obx_abort
**
** Description      Process the OBX abort event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_abort(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8                rsp_code = OBX_RSP_OK;

    utl_freebuf((void**)&p_evt->p_pkt);

    if (!p_cb->cout_active)
    {
        bta_ops_clean_getput(p_cb, TRUE);
        OBX_AbortRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
    else    /* Delay the response if a call-out function is active */
        p_cb->aborting = TRUE;
}

/*******************************************************************************
**
** Function         bta_ops_obx_put
**
** Description      Process the OBX push object put event
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_put(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    tBTA_OPS_OBX_EVENT  *p_evt = &p_data->obx_evt;
    UINT8    rsp_code = OBX_RSP_CONTINUE;
    
    p_cb->obx.final_pkt = p_evt->param.put.final;

    /* If currently processing a push, use the current name */
    if (bta_ops_cb.obx_oper == OPS_OP_PUSH_OBJ)
    {
        bta_ops_proc_put_obj(p_evt->p_pkt);
    }

    /* This is a new request; allocate enough memory to hold the path (including file name) */
    else if ((p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len
                                        + p_bta_fs_cfg->max_file_len + 2))) != NULL)
    {
        p_cb->p_name = (p_cb->p_path + p_bta_fs_cfg->max_path_len + 1);
        p_cb->p_name[p_bta_fs_cfg->max_file_len] = '\0';
        p_cb->p_path[p_bta_fs_cfg->max_path_len] = '\0';

        /* read the name header if it exists */
        if (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->p_name, p_bta_fs_cfg->max_file_len))
        {
            /* get file type from file name; check if supported */
            if ((p_cb->obj_fmt = bta_ops_fmt_supported(p_cb->p_name,
                                                       p_cb->formats)) != 0)
            {
                if(!(OBX_ReadLengthHdr(p_evt->p_pkt, &p_cb->file_length)))
                    p_cb->file_length = BTA_FS_LEN_UNKNOWN;

                p_cb->obx.p_pkt = p_evt->p_pkt; /* save the packet for later use */
                p_cb->obx.offset = 0;  /* Initial offset into OBX data */
                p_cb->obx_oper = OPS_OP_PUSH_OBJ;

                /* request access from the app */
                bta_ops_req_app_access (BTA_OP_OPER_PUSH, p_cb);
            }
            else
                rsp_code = OBX_RSP_UNSUPTD_TYPE;
        }
        else
            rsp_code = OBX_RSP_BAD_REQUEST;
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    /* Error has been detected; respond with error code */
    if (rsp_code != OBX_RSP_CONTINUE)
    {
        utl_freebuf((void**)&p_evt->p_pkt); /* done with obex packet */
        utl_freebuf((void**)&p_cb->p_path);
        p_cb->p_name = NULL;
        OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_ops_obx_get
**
** Description      Process the OBX pull vCard object.
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_get(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{
    /* this is a new request; validate it */
    if (bta_ops_cb.obx_oper != OPS_OP_PULL_OBJ)
        bta_ops_init_get_obj(p_cb, p_data);
    else    /* this is a continuation request */
        bta_ops_proc_get_obj(p_cb);

    /* done with Obex packet */
    utl_freebuf((void**)&p_data->obx_evt.p_pkt);
}

/*******************************************************************************
**
** Function         bta_ops_close_complete
**
** Description      Finishes the memory cleanup after a channel is closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_close_complete(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data)
{

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_OPS ,p_cb->app_id, bta_ops_cb.bd_addr);

    p_cb->cout_active = FALSE;
    
    bta_ops_clean_getput(p_cb, TRUE);
    
    /* Notify the MMI that a connection has been closed */
    p_cb->p_cback(BTA_OPS_CLOSE_EVT, (tBTA_OPS*)p_cb->bd_addr);
    memset(p_cb->bd_addr, 0, BD_ADDR_LEN);

    if (p_data->obx_evt.p_pkt)
        APPL_TRACE_WARNING0("OPS: OBX CLOSE CALLED WITH non-NULL Packet!!!");
}

/*****************************************************************************
**  Callback Functions
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_ops_obx_cback
**
** Description      OBX callback function.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ops_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event,
                        tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_OPS_OBX_EVENT *p_obx_msg;
    UINT16              event = 0;

#if BTA_OPS_DEBUG == TRUE
    APPL_TRACE_DEBUG1("OBX Event Callback: ops_obx_event[%s]", ops_obx_evt_code(obx_event));
#endif

    switch(obx_event)
    {
    case OBX_CONNECT_REQ_EVT:
        event = BTA_OPS_OBX_CONN_EVT;
        break;
    case OBX_DISCONNECT_REQ_EVT:
        event = BTA_OPS_OBX_DISC_EVT;
        break;
    case OBX_PUT_REQ_EVT:
        event = BTA_OPS_OBX_PUT_EVT;
        break;
    case OBX_GET_REQ_EVT:
        event = BTA_OPS_OBX_GET_EVT;
        break;
    case OBX_ABORT_REQ_EVT:
        event = BTA_OPS_OBX_ABORT_EVT;
        break;
    case OBX_CLOSE_IND_EVT:
        event = BTA_OPS_OBX_CLOSE_EVT;
        break;
    case OBX_TIMEOUT_EVT:
        break;
    default:
        /* Unrecognized packet; disconnect the session */
        if (p_pkt)
            event = BTA_OPS_OBX_DISC_EVT;
    }
    
    /* send event to BTA, if any */
    if (event && (p_obx_msg =
        (tBTA_OPS_OBX_EVENT *) GKI_getbuf(sizeof(tBTA_OPS_OBX_EVENT))) != NULL)
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
**  Debug Functions
*****************************************************************************/
#if BTA_OPS_DEBUG == TRUE

/*******************************************************************************
**
** Function         ops_obx_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ops_obx_evt_code(tOBX_EVENT evt_code)
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
    default:
        return "unknown OBX event code";
    }
}
#endif  /* Debug Functions */
#endif /* BTA_OP_INCLUDED */
