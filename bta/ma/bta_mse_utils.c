/*********************************************a********************************
**
**  Name:           bta_mse_utils.c
**
**  Description:    This file implements utility functions for the
**                  file transfer server.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED == TRUE)

#include <stdio.h>
#include <string.h>
#include "gki.h"
#include "utl.h"
#include "bd.h"
#include "bta_fs_api.h"
#include "bta_mse_int.h"
#include "bta_fs_co.h"
#include "bta_mse_co.h"

#include "bta_ma_util.h"


/*******************************************************************************
*  Macros for MSE 
*******************************************************************************/
#define BTA_MSE_XML_EOL              "\n"
#define BTA_MSE_FOLDER_LISTING_START ( "<?xml version=\"1.0\"?>\n" \
                                   "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n" \
                                   "<folder-listing version=\"1.0\">\n" )

#define BTA_MSE_FOLDER_LISTING_END   ( "</folder-listing>" )
#define BTA_MSE_PARENT_FOLDER        (" <parent-folder/>\n")

#define BTA_MSE_FILE_ELEM            "file"
#define BTA_MSE_FOLDER_ELEM          "folder"
#define BTA_MSE_NAME_ATTR            "name"
#define BTA_MSE_SIZE_ATTR            "size"
#define BTA_MSE_TYPE_ATTR            "type"
#define BTA_MSE_MODIFIED_ATTR        "modified"
#define BTA_MSE_CREATED_ATTR         "created"
#define BTA_MSE_ACCESSED_ATTR        "accessed"
#define BTA_MSE_USER_PERM_ATTR       "user-perm"

#define BTA_MSE_MSG_LISTING_START ( "<MAP-msg-listing version=\"1.0\">\n" )
#define BTA_MSE_MSG_LISTING_END   ( "</MAP-msg-listing>" )


// btla-specific ++
#define BTA_MSE_ENABLE_FS_CO FALSE
// btla-specific --

/*******************************************************************************
**
** Function         bta_mse_send_push_msg_in_prog_evt
**
** Description      send the push message in progress event  
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/

void bta_mse_send_push_msg_in_prog_evt(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_PUSH_MSG      *p_push_msg = &p_cb->push_msg;
    tBTA_MSE                    cback_evt_data;
    tBTA_MSE                    *p_data = &cback_evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_send_push_msg_in_prog_evt inst_idx=%d sess_idx=%d",
                      inst_idx,sess_idx);
#endif

    p_data->push_msg_in_prog.mas_session_id = p_cb->obx_handle;
    p_data->push_msg_in_prog.bytes = p_cb->obx.offset;
    p_data->push_msg_in_prog.obj_size = p_push_msg->push_byte_cnt; 
    bta_mse_cb.p_cback(BTA_MSE_PUSH_MSG_IN_PROG_EVT, (tBTA_MSE *) p_data);
}
/*******************************************************************************
**
** Function         bta_mse_send_get_msg_in_prog_evt
**
** Description      Sends the get message in progress event  
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_send_get_msg_in_prog_evt(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_MSG_PARAM     *p_param = &p_cb->msg_param;
    tBTA_MSE                    cback_evt_data;
    tBTA_MSE                    *p_data = &cback_evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_send_get_msg_in_prog_evt inst_idx=%d sess_idx=%d",
                      inst_idx,sess_idx);
#endif

    p_data->get_msg_in_prog.mas_session_id = (tBTA_MA_SESS_HANDLE) p_cb->obx_handle;
    p_data->get_msg_in_prog.bytes = p_param->filled_buff_size;
    p_data->get_msg_in_prog.obj_size = p_param->byte_get_cnt; 
    bta_mse_cb.p_cback(BTA_MSE_GET_MSG_IN_PROG_EVT, (tBTA_MSE *) p_data);
}
/*******************************************************************************
**
** Function         bta_mse_send_oper_cmpl_evt
**
** Description      Sends the operartion complete event based on the specified status 
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  status      - MA status code
**
** Returns          void
**
*******************************************************************************/
void bta_mse_send_oper_cmpl_evt(UINT8 inst_idx, UINT8 sess_idx, tBTA_MA_STATUS status)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_MSG_PARAM     *p_param = &p_cb->msg_param;
    tBTA_MSE_OPER_PUSH_MSG      *p_push_msg = &p_cb->push_msg;
    tBTA_MSE                    cback_evt_data;
    tBTA_MSE                    *p_data = &cback_evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_send_oper_cmpl_evt inst_idx=%d sess_idx=%d status=%d",
                      inst_idx, sess_idx, status);
#endif

    p_data->oper_cmpl.mas_session_id = p_cb->obx_handle;
    p_data->oper_cmpl.operation      = p_cb->oper;
    p_data->oper_cmpl.status         = status;
    switch (p_cb->oper)
    {
        case BTA_MSE_OPER_GET_MSG:
            p_data->oper_cmpl.obj_size = p_param->byte_get_cnt;
            break;
        case BTA_MSE_OPER_PUSH_MSG:
            p_data->oper_cmpl.obj_size = p_push_msg->push_byte_cnt;
            break;
        default:
            p_data->oper_cmpl.obj_size = 0;
            break;
    }
    bta_mse_cb.p_cback(BTA_MSE_OPER_CMPL_EVT, (tBTA_MSE *) p_data);
}

/*******************************************************************************
**
** Function         bta_mse_pushmsg
**
** Description     Process the push message request 
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**                 first_pkt   - first packet of the push message request
**
** Returns         void
**
*******************************************************************************/
void bta_mse_pushmsg(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN first_pkt)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_PUSH_MSG      *p_push_msg = &p_cb->push_msg;
    tBTA_MSE_OBX_PKT            *p_obx  = &p_cb->obx;
    tBTA_MA_MPKT_STATUS         mpkt_status;

    p_push_msg->push_byte_cnt += p_obx->offset;
    p_cb->cout_active = TRUE;
    mpkt_status = BTA_MA_MPKT_STATUS_LAST;      
    if (!p_obx->final_pkt) mpkt_status = BTA_MA_MPKT_STATUS_MORE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT5("bta_mse_pushmsg i_idx=%d s_idx=%d first=%d final=%d cnt=%d",
                      inst_idx, sess_idx, first_pkt,  p_obx->final_pkt, 
                      p_push_msg->push_byte_cnt);
#endif


    bta_mse_co_push_msg((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,  &p_push_msg->param,
                        p_obx->offset,  p_obx->p_start, first_pkt,  
                        mpkt_status, BTA_MSE_CI_PUSH_MSG_EVT, bta_mse_cb.app_id);
}

/*******************************************************************************
**
** Function         bta_mse_clean_set_msg_sts
**
** Description      Cleans up the set message status resources and cotrol block 
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_set_msg_sts(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB    *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_SET_MSG_STS *p_set_msg_sts = &p_cb->set_msg_sts;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_set_msg_sts");
#endif

    utl_freebuf((void**)&(p_set_msg_sts->p_msg_handle));   
    bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);

}

/*******************************************************************************
**
** Function         bta_mse_clean_set_notif_reg
**
** Description      Cleans up the set notification registration resources and cotrol block 
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_set_notif_reg(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB    *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_set_notif_reg");
#endif

    p_cb->notif_reg_req.notif_status_rcv = FALSE;
    bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);
}

/*******************************************************************************
**
** Function         bta_mse_clean_push_msg
**
** Description      Cleans up the push message resources and cotrol block 
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_push_msg(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB    *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_PUSH_MSG *p_push_msg = &p_cb->push_msg;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_push_msg");
#endif

    utl_freebuf((void**)&(p_push_msg->param.p_folder));   
    bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);

}

/*******************************************************************************
**
** Function         bta_mse_init_set_msg_sts
**
** Description      Initializes the set message status resources and cotrol block 
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**                 p_rsp_code  - (output) pointer to the obex response code
**
** Returns         void
**
*******************************************************************************/
void bta_mse_init_set_msg_sts(UINT8 inst_idx, UINT8 sess_idx, UINT8 *p_rsp_code)
{
    tBTA_MSE_MA_SESS_CB    *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_SET_MSG_STS *p_set_msg_sts = &p_cb->set_msg_sts;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_init_set_msg_sts");
#endif
    *p_rsp_code                     = OBX_RSP_OK;



    p_set_msg_sts->rcv_msg_handle      = FALSE;
    p_set_msg_sts->rcv_sts_ind         = FALSE;
    p_set_msg_sts->rcv_sts_val         = FALSE;

    if (!p_set_msg_sts->p_msg_handle)
    {
        if ((p_set_msg_sts->p_msg_handle = (char *)GKI_getbuf((UINT16)(BTA_MSE_64BIT_HEX_STR_SIZE))) == NULL )
        {
            *p_rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
    }
}

/*******************************************************************************
**
** Function         bta_mse_init_push_msg
**
** Description      initializes the push message resources and control block
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**                 p_rsp_code  - (output) pointer to the obex response code
**
** Returns         void
**
*******************************************************************************/
void bta_mse_init_push_msg(UINT8 inst_idx, UINT8 sess_idx, UINT8 *p_rsp_code)
{
    tBTA_MSE_MA_SESS_CB    *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_PUSH_MSG *p_push_msg = &p_cb->push_msg;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_init_push_msg");
#endif
    *p_rsp_code                     = OBX_RSP_OK;


    p_push_msg->push_byte_cnt       = 0;
    p_push_msg->first_req_pkt       = TRUE;
    p_push_msg->rcv_charset         = FALSE;
    p_push_msg->rcv_folder_name     = FALSE;
    p_push_msg->rcv_retry           = FALSE;
    p_push_msg->rcv_transparent     = FALSE;
    p_push_msg->param.transparent   = BTA_MA_TRANSP_OFF;
    p_push_msg->param.retry         = BTA_MA_RETRY_OFF;

    if (!p_push_msg->param.p_folder)
    {
        if ((p_push_msg->param.p_folder = (char *)GKI_getbuf((UINT16)(p_bta_mse_cfg->max_name_len + 1))) == NULL )
        {
            *p_rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
    }
}


/*******************************************************************************
**
** Function         bta_mse_remove_uuid
**
** Description      Remove UUID and clear service 
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_mse_remove_uuid(void)
{
    bta_sys_remove_uuid(UUID_SERVCLASS_MESSAGE_ACCESS);
    BTM_SecClrService(BTM_SEC_SERVICE_MAP);
}

/*******************************************************************************
**
** Function         bta_mse_clean_mas_service
**
** Description      Cleans up the MAS service resources and control block
**
** Parameters      inst_idx    - Index to the MA instance control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_mas_service(UINT8 inst_idx)
{
    tBTA_MSE_MA_CB         *p_scb   = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE_MA_SESS_CB    *p_cb;
    int i, num_act_mas =0;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_mas_service");
#endif
    OBX_StopServer(p_scb->obx_handle);
    BTM_FreeSCN(p_scb->scn);
    if (p_scb->sdp_handle) SDP_DeleteRecord(p_scb->sdp_handle);

    utl_freebuf((void**)&p_scb->p_rootpath);     
    for (i=0; i < BTA_MSE_NUM_SESS; i++  )
    {
        p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, i);
        utl_freebuf((void**)&p_cb->p_workdir);   
    }
    p_scb->in_use = FALSE;

    /* check all other MAS instances */
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        p_scb = BTA_MSE_GET_INST_CB_PTR(i);
        if (p_scb->in_use) num_act_mas++;
    }

    if (!num_act_mas) bta_mse_remove_uuid();
}


/*******************************************************************************
**
** Function         bta_mse_abort_too_late
**
** Description      It is too late to abort the operation
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_abort_too_late(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_abort_too_late oper=%d  ",p_cb->oper  );
#endif
    OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_FORBIDDEN, (BT_HDR *)NULL);
    p_cb->aborting = FALSE;
}

/*******************************************************************************
**
** Function         bta_mse_clean_getput
**
** Description      Cleans up get and put resources and control blocks
**
** Parameters      inst_idx    - Index to the MA instance control block
**                 sess_idx    - Index to the MA session control block
**                 status      - operation status
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_getput(UINT8 inst_idx, UINT8 sess_idx, tBTA_MA_STATUS status)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_clean_getput oper=%d status=%d",p_cb->oper, status );
#endif
    if (status == BTA_MA_STATUS_ABORTED)
    {
        OBX_AbortRsp(p_cb->obx_handle, OBX_RSP_OK, (BT_HDR *)NULL);
        p_cb->aborting = FALSE;
    }
    switch (p_cb->oper)
    {
        case BTA_MSE_OPER_UPDATE_INBOX:
            bta_mse_set_ma_oper(inst_idx,sess_idx, BTA_MSE_OPER_NONE);
            break;
        case BTA_MSE_OPER_GET_FOLDER_LIST:
            bta_mse_clean_list(inst_idx,sess_idx);
            break;
        case BTA_MSE_OPER_GET_MSG_LIST:
            bta_mse_clean_msg_list(inst_idx,sess_idx);
            break;
        case BTA_MSE_OPER_GET_MSG:
            bta_mse_send_oper_cmpl_evt(inst_idx,sess_idx, status);
            bta_mse_clean_msg(inst_idx,sess_idx);
            break;
        case BTA_MSE_OPER_PUSH_MSG:
            bta_mse_send_oper_cmpl_evt(inst_idx,sess_idx, status);
            bta_mse_clean_push_msg(inst_idx,sess_idx);
            break;
        case BTA_MSE_OPER_DEL_MSG:
        case BTA_MSE_OPER_SET_MSG_STATUS:
            bta_mse_clean_set_msg_sts(inst_idx,sess_idx);
            break;

        default:
            break;
    }

    utl_freebuf((void**)&p_cb->obx.p_pkt);
    p_cb->obx.bytes_left = 0;

}


/*******************************************************************************
**
** Function         bta_mse_clean_msg
**
** Description      Cleans up the get message resources and control block
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_msg(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    /* Clean up control block */
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_msg");
#endif
    utl_freebuf((void **)&p_cb->msg_param.p_msg_handle);
    bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);

}

/*******************************************************************************
**
** Function         bta_mse_end_of_msg
**
** Description      Complete the end body of the get message response, and 
**                  sends out the OBX get response
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  rsp_code    - Obex response code
**
** Returns          void
**
*******************************************************************************/
void bta_mse_end_of_msg(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_OPER_MSG_PARAM     *p_param = &p_cb->msg_param;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_end_of_msg");
#endif
    /* Add the end of folder listing string if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        /* If get message has completed, update the fraction delivery status */

        if (p_param->add_frac_del_hdr)
        {
            *(p_param->p_frac_delivery) = p_param->frac_deliver_status;
            p_param->add_frac_del_hdr = FALSE;
        }

        p_obx->offset += p_param->filled_buff_size;

        if (rsp_code == OBX_RSP_OK)
        {
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
        }
        else    /* More data to be sent */
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, FALSE);

        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */

        if (rsp_code == OBX_RSP_CONTINUE)
        {
            bta_mse_send_get_msg_in_prog_evt(inst_idx, sess_idx);
        }
        else
        {
            bta_mse_clean_getput(inst_idx, sess_idx, BTA_MA_STATUS_OK);
        }
    }
    else    /* An error occurred */
    {
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_mse_clean_getput(inst_idx, sess_idx, BTA_MA_STATUS_FAIL);
    }
}

/*******************************************************************************
**
** Function         bta_mse_getmsg
**
** Description      Processes the get message request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns         void
**
*******************************************************************************/
void bta_mse_getmsg(UINT8 inst_idx, UINT8 sess_idx,  BOOLEAN new_req)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_OPER_MSG_PARAM     *p_param  = &p_cb->msg_param;
    UINT8                       rsp_code = OBX_RSP_OK;
    UINT8                       *p, *p_start;
    UINT16                      len = 0;
    BOOLEAN                     first_get=FALSE;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_getmsg");
#endif

    p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                                         /* p_cb->peer_mtu */ OBX_LRG_DATA_POOL_SIZE);
    if (p_obx->p_pkt)
    {
        /* Is this a new request or continuation? */
        if (new_req)
        {
            first_get = TRUE;
            if (p_param->add_frac_del_hdr)
            {
                /* Add Fraction Deliver Header */
                p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                p = p_start;
                *p++    = BTA_MA_APH_FRAC_DELVR;
                *p++    = 1;
                p_param->p_frac_delivery = p;
                /* use Last as default and it can be changed to More if application indicates more */
                UINT8_TO_BE_STREAM(p, BTA_MA_FRAC_DELIVER_LAST);
                OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
            }
        }

        /* Add the start of the Body Header */
        p_obx->offset = 0;
        p_obx->bytes_left = 0;
        p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
        p_cb->cout_active = TRUE;
        bta_mse_co_get_msg((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                           &p_param->data,
                           first_get,
                           p_obx->bytes_left,
                           p_obx->p_start,
                           BTA_MSE_CI_GET_MSG_EVT,
                           bta_mse_cb.app_id);

        /* List is not complete, so don't send the response yet */
        rsp_code = OBX_RSP_PART_CONTENT;
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    /* Response goes out if complete or error occurred */
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_msg(inst_idx, sess_idx,rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_clean_msg_list
**
** Description      Cleans up the get message list resources and control block
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_msg_list(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_MSGLIST            *p_ml = &p_cb->ml;
    /* Clean up control block */
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_msg_list");
#endif
    bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);
    p_ml->remaing_size =0;
    p_ml->offset =0;
    p_ml->pending_ml_frag = FALSE;

    utl_freebuf((void**)&p_ml->p_entry);
    utl_freebuf((void**)&p_ml->p_info);
    utl_freebuf((void**)&p_ml->p_xml_buf);
    utl_freebuf((void**)&p_cb->ml_param.p_name);
    utl_freebuf((void**)&p_cb->ml_param.p_path);
}

/*******************************************************************************
**
** Function         bta_mse_end_of_msg_list
**
** Description      Complete the end body of the get message listing response, and 
**                  sends out the OBX get response
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  rsp_code    - Obex response code
**
** Returns          void
**
*******************************************************************************/
void bta_mse_end_of_msg_list(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_OPER_MLIST_PARAM   *p_ml_param = &p_cb->ml_param;
    UINT16             len;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_end_of_msg_list");
#endif
    /* Add the end of folder listing string if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        /* If listing has completed, check the max list count */
        if (rsp_code == OBX_RSP_OK)
        {
            if (p_ml_param->filter.max_list_cnt)
            {
                len = strlen(BTA_MSE_MSG_LISTING_END);
                memcpy(&p_obx->p_start[p_obx->offset], BTA_MSE_MSG_LISTING_END, len);
                p_obx->offset += len;

                OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
            }

            /* Clean up control block */
            bta_mse_clean_msg_list(inst_idx, sess_idx);
        }
        else    /* More listing data to be sent */
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, FALSE);

        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
    }
    else    /* An error occurred */
    {
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_mse_clean_msg_list(inst_idx, sess_idx);
    }
}

/*******************************************************************************
**
** Function         bta_mse_add_msg_list_info
**
** Description      Adds applications paramter headers for new message and 
**                  message list size 
** 
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                 
** Returns          UINT8 - OBX response code
**                      OBX_RSP_PART_CONTENT if not finished yet.
**
*******************************************************************************/
UINT8 bta_mse_add_msg_list_info(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB             *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT                *p_obx = &p_cb->obx;
    tBTA_MSE_MSGLIST                *p_ml = &p_cb->ml;
    tBTA_MSE_CO_MSG_LIST_INFO       *p_info = p_ml->p_info;
    tBTA_MSE_CO_MSG_LIST_ENTRY      *p_entry = p_ml->p_entry;
    tBTA_MA_MSG_LIST_FILTER_PARAM   *p_filter = &p_cb->ml_param.filter;
    UINT8                           rsp_code = OBX_RSP_PART_CONTENT;
    UINT8                           *p, *p_start;
    UINT16                          len = 0;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_add_msg_list_info: new msg=%d, list size=%d",
                      p_info->new_msg, p_info->msg_list_size );
#endif

    /* add app params for GetMessageListing */
    p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
    p = p_start;

    *p++    = BTA_MA_APH_NEW_MSG;
    *p++    = 1;
    UINT8_TO_BE_STREAM(p, p_info->new_msg);

    *p++    = BTA_MA_APH_MSG_LST_SIZE;
    *p++    = 2;
    UINT16_TO_BE_STREAM(p, p_info->msg_list_size);

    *p++    = BTA_MA_APH_MSE_TIME;
    *p++    = p_info->mse_time_len;
    memcpy(p, p_info->mse_time, p_info->mse_time_len);
    p +=  p_info->mse_time_len;

    if (p != p_start)
    {
        OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
    }

    if (p_filter->max_list_cnt)
    {
        /* Add the start of the Body Header */
        p_obx->offset = 0;
        p_obx->bytes_left = 0;
        p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

        len = strlen(BTA_MSE_MSG_LISTING_START);
        memcpy(&p_obx->p_start[p_obx->offset], BTA_MSE_MSG_LISTING_START, len);
        p_obx->bytes_left -= (UINT16)(len + strlen(BTA_MSE_MSG_LISTING_END)); 
        p_obx->offset += len;
        p_cb->cout_active = TRUE;
        bta_mse_co_get_msg_list_entry((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                      p_cb->ml_param.p_name, 
                                      p_filter, TRUE,
                                      p_entry,
                                      BTA_MSE_CI_GET_ML_ENTRY_EVT,
                                      bta_mse_cb.app_id);
    }
    else
    {
        /* done with get msg list request */
        rsp_code = OBX_RSP_OK;
    }
    return(rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_add_msg_list_entry
**
** Description     Adds one entry of the message list to the message list object
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          UINT8 - OBX response code
**                      OBX_RSP_PART_CONTENT if not finished yet.
**                      OBX_RSP_CONTINUE [packet done]
**                      Others send error response out
**
*******************************************************************************/
UINT8 bta_mse_add_msg_list_entry(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB             *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT                *p_obx = &p_cb->obx;
    tBTA_MSE_MSGLIST                *p_ml = &p_cb->ml;
    tBTA_MSE_CO_MSG_LIST_ENTRY      *p_entry = p_ml->p_entry;
    tBTA_MA_MSG_LIST_FILTER_PARAM   *p_filter = &p_cb->ml_param.filter;
    char                            *p_buf;
    UINT16                          size;
    UINT8                           rsp_code = OBX_RSP_PART_CONTENT;
    tBTA_MA_STATUS                  status; 
    BOOLEAN                         release_xml_buf = TRUE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_add_msg_list_entry");
#endif

    if (p_ml->pending_ml_frag)
    {
        if (p_ml->remaing_size <= p_obx->bytes_left)
        {
            size = p_ml->remaing_size;
            p_ml->pending_ml_frag = FALSE;
        }
        else
        {
            rsp_code = OBX_RSP_CONTINUE;
            size = p_obx->bytes_left;
        }

        p_buf = p_ml->p_xml_buf + p_ml->offset;
        memcpy (&p_obx->p_start[p_obx->offset], p_buf, size);
        p_obx->offset += size;
        p_obx->bytes_left -= size;
        p_ml->remaing_size -= size;
        p_ml->offset += size;

        if ( !p_ml->pending_ml_frag )
        {
            utl_freebuf((void **) &p_ml->p_xml_buf);
        }


        if (rsp_code == OBX_RSP_PART_CONTENT)
        {
            /* Get the next msg list entry */
            p_cb->cout_active = TRUE;
            bta_mse_co_get_msg_list_entry((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                          p_cb->ml_param.p_name, 
                                          p_filter, FALSE,
                                          p_entry,
                                          BTA_MSE_CI_GET_ML_ENTRY_EVT,
                                          bta_mse_cb.app_id);
        }
        return rsp_code;
    }

    if ((p_buf = (char *)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL)
    {
        p_buf[0] = '\0';
        size = GKI_MAX_BUF_SIZE;
        status = bta_mse_build_msg_listing_obj( p_entry, &size, p_buf  );

        if (status == BTA_MA_STATUS_OK)
        {
            size = strlen(p_buf);

            if (size <= p_obx->bytes_left)
            {
                if ( size > 0)
                {
                    memcpy (&p_obx->p_start[p_obx->offset], p_buf, size);
                    p_obx->offset += size;
                    p_obx->bytes_left -= size;
                }

                /* Get the next msg list entry */
                p_cb->cout_active = TRUE;
                bta_mse_co_get_msg_list_entry((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                              p_cb->ml_param.p_name, 
                                              p_filter, FALSE,
                                              p_entry,
                                              BTA_MSE_CI_GET_ML_ENTRY_EVT,
                                              bta_mse_cb.app_id);
            }
            else  /* entry did not fit in current obx packet; try to add entry in next obx req */
            {
                p_ml->pending_ml_frag = TRUE;
                p_ml->p_xml_buf= p_buf;
                p_ml->offset =0;
                p_ml->remaing_size = size - p_obx->bytes_left;
                p_ml->offset += p_obx->bytes_left;
                release_xml_buf = FALSE;
                memcpy (&p_obx->p_start[p_obx->offset], p_buf, p_obx->bytes_left);
                p_obx->offset += p_obx->bytes_left;
                p_obx->bytes_left = 0;

                APPL_TRACE_EVENT2("1st msg list fragment peer_mtu=%d msg_list_size=%d",
                                  p_cb->peer_mtu, size);
                APPL_TRACE_EVENT3("pending_flag=%d offset=%d remaining_size=%d", 
                                  p_ml->pending_ml_frag, p_ml->offset, p_ml->remaing_size);
                APPL_TRACE_EVENT2("obx offset=%d byte_left=%d",
                                  p_obx->offset, p_obx->bytes_left );

                rsp_code = OBX_RSP_CONTINUE;

            }
        }
        else
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        /* Done with temporary buffer */
        if (release_xml_buf) utl_freebuf((void **) &p_buf);
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    return(rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_getmsglist
**
** Description      Process the retrieval of a msg listing.
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  
** Returns         None
**
*******************************************************************************/
void bta_mse_getmsglist(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN new_req)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_MSGLIST            *p_ml  = &p_cb->ml;
    UINT8                       rsp_code = OBX_RSP_OK;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_getmsglist ");
#endif

    p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                                         /* p_cb->peer_mtu */ OBX_LRG_DATA_POOL_SIZE);
    if (!p_ml->p_entry)
    {
        /* Allocate enough space for the structure  */
        p_ml->p_entry = (tBTA_MSE_CO_MSG_LIST_ENTRY *)
                        GKI_getbuf((UINT16)(sizeof(tBTA_MSE_CO_MSG_LIST_ENTRY)));
    }

    if (p_ml->p_entry && p_obx->p_pkt)
    {
        /* Is this a new request or continuation? */
        if (new_req)
        {
            if (!p_ml->p_info)
            {
                /* Allocate enough space for the structure  */
                p_ml->p_info = (tBTA_MSE_CO_MSG_LIST_INFO *)
                               GKI_getbuf((UINT16)(sizeof(tBTA_MSE_CO_MSG_LIST_INFO)));
            }

            if (p_ml->p_info)
            {
                p_cb->ml_param.w4info =TRUE;

                p_cb->cout_active = TRUE;
                bta_mse_co_get_msg_list_info((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                             p_cb->ml_param.p_name,
                                             &(p_cb->ml_param.filter),
                                             p_ml->p_info,
                                             BTA_MSE_CI_GET_ML_INFO_EVT,
                                             bta_mse_cb.app_id);

                /* List is not complete, so don't send the response yet */
                rsp_code = OBX_RSP_PART_CONTENT;
            }
            else
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
        else /* Add the entry previously retrieved */
        {
            /* Add the start of the Body Header */
            p_obx->offset = 0;
            p_obx->bytes_left = 0;
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
            rsp_code = bta_mse_add_msg_list_entry(inst_idx, sess_idx);
        }
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    /* Response goes out if complete or error occurred */
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_msg_list(inst_idx, sess_idx,rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_read_app_params
**
** Description      Read the specified application parameter from the given OBX packet
**
** Parameters       p_pkt       - obex packet pointer
**                  tag         - application parameter tag
**                  param_len   - (output) pointer to the length of application paramter
**
** Returns          pointer to the application parameter found
**                  NULL - not found
**
*******************************************************************************/

UINT8 * bta_mse_read_app_params(BT_HDR *p_pkt, UINT8 tag, UINT16 *param_len)
{
    UINT8   *p_data = NULL, *p = NULL;
    UINT16  data_len = 0;
    int     left, len;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_read_app_params");
#endif

    if (OBX_ReadByteStrHdr(p_pkt, OBX_HI_APP_PARMS, &p_data, &data_len, 0))
    {
        left    = data_len;
        while (left > 0)
        {
            len = *(p_data + 1);
            if (*p_data == tag)
            {
                p_data += 2;
                p = p_data;
                *param_len = (UINT16) len;
                break;
            }
            p_data     += (len+2);
            left       -= (len+2);
        }
    }
    return p;
}

/*******************************************************************************
**
** Function         bta_mse_clean_list
**
** Description      Cleans up the get folder list resources and control block
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
** 
** Returns          void
**
*******************************************************************************/
void bta_mse_clean_list(UINT8 inst_idx, UINT8 sess_idx)
{

    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_DIRLIST            *p_dir = &p_cb->dir;
    /*tBTA_MSE_CO_FOLDER_ENTRY    *p_entry = p_dir->p_entry; */
    /* Clean up control block */
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_clean_list");
#endif
    bta_mse_set_ma_oper(inst_idx,sess_idx, BTA_MSE_OPER_NONE);
    utl_freebuf((void**)&(p_dir->p_entry));
}

/*******************************************************************************
**
** Function         bta_mse_end_of_list
**
** Description      Finishes up the end body of the get folder listing, and sends out the
**                  OBX response
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  rsp_code    - obex response code
**
** Returns          void
**
*******************************************************************************/
void bta_mse_end_of_list(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    UINT16             len = 0;
    UINT8 *p, *p_start;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_end_of_list");
#endif
    /* Add the end of folder listing string if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        /* If listing has completed, add on end string (http) */
        if (rsp_code == OBX_RSP_OK)
        {
            if (p_cb->fl_param.max_list_cnt)
            {
                len = strlen(BTA_MSE_FOLDER_LISTING_END);
                memcpy(&p_obx->p_start[p_obx->offset], BTA_MSE_FOLDER_LISTING_END, len);
                p_obx->offset += len;
                OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
            }
            else
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT1("list_cnt=%d",p_cb->fl_param.list_cnt);
#endif

                /* send the folder list size header only*/
                p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                p = p_start;

                *p++    = BTA_MA_APH_FOLDER_LST_SIZE;
                *p++    = 2;
                UINT16_TO_BE_STREAM(p, p_cb->fl_param.list_cnt);
                if (p != p_start)
                {
                    OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
                }
            }
            /* Clean up control block */
            bta_mse_clean_list(inst_idx, sess_idx);
        }
        else    /* More listing data to be sent */
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, FALSE);

        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
    }
    else    /* An error occurred */
    {
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_mse_clean_list(inst_idx, sess_idx);
    }
}

/*******************************************************************************
**
** Function         bta_mse_add_list_entry
**
** Description      Adds an subfolder entry to the folder list object
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**
** Returns          UINT8 - OBX response code
**                      OBX_RSP_PART_CONTENT if not finished yet.
**                      OBX_RSP_CONTINUE [packet done]
**                      Others send error response out
**
*******************************************************************************/
UINT8 bta_mse_add_list_entry(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_DIRLIST            *p_dir = &p_cb->dir;
    tBTA_MSE_CO_FOLDER_ENTRY    *p_entry = p_dir->p_entry;
    char                        *p_buf;
    UINT16                      size;
    UINT8                       rsp_code = OBX_RSP_PART_CONTENT;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_add_list_entry");
#endif
    if ((p_buf = (char *)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL)
    {
        p_buf[0] = '\0';

        APPL_TRACE_DEBUG2("bta_mse_add_list_entry: attr:0x%02x, name:%s",
                          p_entry->mode, p_entry->p_name);

        if (p_entry->mode & BTA_MA_A_DIR)  /* only process Subdirectory ignore files */
        {
            /* ignore "." and ".." */
            if (strcmp(p_entry->p_name, ".") && strcmp(p_entry->p_name, ".."))
            {
                p_cb->fl_param.list_cnt++;
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT1("list_cnt=%d",p_cb->fl_param.list_cnt);
#endif
                if (p_cb->fl_param.max_list_cnt)
                {
                    if (p_cb->fl_param.list_cnt <= p_cb->fl_param.max_list_cnt)
                    {
                        if (p_cb->fl_param.list_cnt > p_cb->fl_param.start_offset )
                        {
                            sprintf(p_buf, " <" BTA_MSE_FOLDER_ELEM " "
                                    BTA_MSE_NAME_ATTR "=\"%s\"/>" BTA_MSE_XML_EOL,
                                    p_entry->p_name);
                        }
                    }
                } /* if max_list_cnt==0 only count the list size - but no body should be included*/
            }
        }/* ignore files i.e. non-folder items */

        size = strlen(p_buf);
        if (size <= p_obx->bytes_left)
        {
            if ( size > 0)
            {
                memcpy (&p_obx->p_start[p_obx->offset], p_buf, size);
                p_obx->offset += size;
                p_obx->bytes_left -= size;
            }

            if ((p_cb->fl_param.list_cnt < p_cb->fl_param.max_list_cnt) ||
                (p_cb->fl_param.max_list_cnt == 0) )
            {
                /* Get the next directory entry */
                p_cb->cout_active = TRUE;
                bta_mse_co_get_folder_entry((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                            p_cb->p_workdir, FALSE, p_dir->p_entry,
                                            BTA_MSE_CI_GET_FENTRY_EVT,
                                            bta_mse_cb.app_id);
            }
            else
            {
                /* reach the max allowed */
                rsp_code = OBX_RSP_OK;
            }
        }
        else  /* entry did not fit in current obx packet; try to add entry in next obx req */
        {
            p_cb->fl_param.list_cnt--;
            rsp_code = OBX_RSP_CONTINUE;
        }

        /* Done with temporary buffer */
        GKI_freebuf(p_buf);
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    return(rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_getfolderlist
**
** Description      Processes the retrieval of a folder listing.
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_getfolderlist(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN new_req)
{
    tBTA_MSE_MA_CB              *p_scb  = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT            *p_obx = &p_cb->obx;
    tBTA_MSE_DIRLIST            *p_dir = &p_cb->dir;
    UINT16                      temp_len;
    UINT8                       rsp_code = OBX_RSP_OK;

#if BTA_MSE_ENABLE_FS_CO == TRUE
    BOOLEAN             is_dir;
#endif

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_getfolderlist");
#endif

#if BTA_MSE_ENABLE_FS_CO == TRUE
    /* Make sure the Name is a directory and accessible */
    if (((bta_fs_co_access(p_cb->p_workdir, BTA_FS_ACC_EXIST,
                           &is_dir, bta_mse_cb.app_id))!= BTA_FS_CO_OK)
        || !is_dir)
        rsp_code = OBX_RSP_NOT_FOUND;
#endif
    /* Build the listing */
    if (rsp_code == OBX_RSP_OK)
    {
        p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                                             /* p_cb->peer_mtu */ OBX_LRG_DATA_POOL_SIZE);
        if (!(strcmp(p_cb->p_workdir, p_scb->p_rootpath)))
            p_dir->is_root = TRUE;
        else
            p_dir->is_root = FALSE;

        if (!p_dir->p_entry)
        {
            /* Allocate enough space for the structure and the folder name */
            if ((p_dir->p_entry = (tBTA_MSE_CO_FOLDER_ENTRY *)
                 GKI_getbuf((UINT16)(sizeof(tBTA_MSE_CO_FOLDER_ENTRY) +
                                     p_bta_mse_cfg->max_name_len + 1))) != NULL)
                p_dir->p_entry->p_name = (char *)(p_dir->p_entry + 1);
        }

        if (p_dir->p_entry && p_obx->p_pkt)
        {
            if (p_cb->fl_param.max_list_cnt)
            {
                /* Add the start of the Body Header */
                p_obx->offset = 0;
                p_obx->bytes_left = 0;
                p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
            }

            /* Is this a new request or continuation? */
            if (new_req)
            {
                APPL_TRACE_EVENT1("Folder List Directory:  [%s]", p_cb->p_workdir);
                p_cb->fl_param.list_cnt =0;

                /* add body header if max_list_cnt is not 0
                   if max_list_cnt =0 then only report the actual number
                   accessible folders. Use FolderListingSize header only
                */

                if (p_cb->fl_param.max_list_cnt)
                {
                    temp_len = strlen(BTA_MSE_FOLDER_LISTING_START);

                    /* Add the beginning label of http */
                    memcpy(p_obx->p_start, BTA_MSE_FOLDER_LISTING_START, temp_len);
                    p_obx->bytes_left -= (UINT16)(temp_len + strlen(BTA_MSE_FOLDER_LISTING_END));
                    p_obx->offset += temp_len;

                    /* Add the parent directory if not the root */
                    if (strcmp(p_cb->p_workdir, p_scb->p_rootpath))
                    {
                        APPL_TRACE_EVENT0("Add parent folder");

                        temp_len = strlen(BTA_MSE_PARENT_FOLDER);
                        memcpy(p_obx->p_start + p_obx->offset,
                               BTA_MSE_PARENT_FOLDER, temp_len);
                        p_obx->bytes_left -= temp_len;
                        p_obx->offset += temp_len;
                    }
                }
                p_cb->cout_active = TRUE;
                bta_mse_co_get_folder_entry((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                            p_cb->p_workdir, TRUE, p_dir->p_entry,
                                            BTA_MSE_CI_GET_FENTRY_EVT,
                                            bta_mse_cb.app_id);

                /* List is not complete, so don't send the response yet */
                rsp_code = OBX_RSP_PART_CONTENT;
            }
            else /* Add the entry previously retrieved */
                rsp_code = bta_mse_add_list_entry(inst_idx, sess_idx);
        }
        else
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    }

    /* Response goes out if complete or error occurred */
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_list(inst_idx, sess_idx,rsp_code);
}

/*******************************************************************************
**
** Function         bta_mse_chdir
**
** Description      Changes the current path based on received setpath paramters
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_oper      - (output) pointer to the MSE operation code
**
** Returns          UINT8 - OBX response code
**                  output *p_oper set to BTA_MSE_OPER_SETPATH if the 
**                         the resulting path is a valid path 
*******************************************************************************/
UINT8 bta_mse_chdir(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_OPER *p_oper)
{
    tBTA_MSE_MA_CB      *p_scb = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE_MA_SESS_CB *p_cb  = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    char                *p_path = p_cb->sp.p_path;
    char                *p_name = p_cb->sp.p_name;
    char                *p_tmp;

    char                *p_workdir = p_cb->p_workdir;
    UINT8               rsp_code = OBX_RSP_OK;

#if BTA_MSE_ENABLE_FS_CO == TRUE
    BOOLEAN             is_dir;
#endif

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_chdir flag=%d name=%s",p_cb->sp.flags,p_cb->sp.p_name );
#endif
    *p_oper = BTA_MSE_OPER_NONE;
    switch (p_cb->sp.flags)
    {
        case BTA_MA_DIR_NAV_ROOT_OR_DOWN_ONE_LVL:
            p_path  = p_cb->sp.p_path;
            p_name  = p_cb->sp.p_name;
            rsp_code = OBX_RSP_OK;
            if (*p_name == '\0')
            {
                /* backup to root */
                if (strcmp(p_workdir, p_scb->p_rootpath))
                {
                    BCM_STRNCPY_S(p_path, p_bta_fs_cfg->max_path_len+1, p_scb->p_rootpath, p_bta_fs_cfg->max_path_len);
                    /* go back up to the root folder*/
                    *p_oper = BTA_MSE_OPER_SETPATH;
                }
            }
            /* Make sure the new path is not too big */
            /* +1 is for the separator */
            else if ((strlen(p_workdir)+1+strlen(p_name)) <= p_bta_fs_cfg->max_path_len)
            {
                /* create a temporary path for creation attempt */
                sprintf(p_path, "%s%c%s", p_workdir,
                        p_bta_fs_cfg->path_separator, p_name);
#if BTA_MSE_ENABLE_FS_CO == TRUE
                if (((bta_fs_co_access(p_path, BTA_FS_ACC_EXIST,
                                       &is_dir, bta_mse_cb.app_id)) == BTA_FS_CO_OK) && is_dir)
                {
                    /* go back down one level to the name folder*/
                    *p_oper = BTA_MSE_OPER_SETPATH;
                }
                else
                    rsp_code = OBX_RSP_NOT_FOUND;
#else
                *p_oper = BTA_MSE_OPER_SETPATH;
#endif
            }
            else
            {
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
            }
            break;
        case BTA_MA_DIR_NAV_UP_ONE_LVL:

            if (strcmp(p_workdir, p_scb->p_rootpath))
            {
                /* Find the last occurrence of separator and replace with '\0' */
                BCM_STRNCPY_S(p_path, p_bta_fs_cfg->max_path_len+1, p_workdir, p_bta_fs_cfg->max_path_len);
                if ((p_tmp = strrchr(p_path, (int)p_bta_fs_cfg->path_separator)) != NULL)
                {
                    *p_tmp = '\0';
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT1("path=[%s]",p_path );
                    APPL_TRACE_EVENT1("name=[%s]", p_name );
#endif
                    /* now check we need to go down one level if name is not empty*/
                    if (*p_name !='\0')
                    {
                        if ((strlen(p_workdir)+1+strlen(p_name)) <= p_bta_fs_cfg->max_path_len)
                        {
                            sprintf(p_path, "%s%c%s", p_path,
                                    p_bta_fs_cfg->path_separator, p_name);
#if BTA_MSE_DEBUG == TRUE
                            APPL_TRACE_EVENT0("Up one level and then down one" );
                            APPL_TRACE_EVENT1("path=[%s]",p_path );
#endif
#if BTA_MSE_ENABLE_FS_CO == TRUE
                            if (((bta_fs_co_access(p_path, BTA_FS_ACC_EXIST,
                                                   &is_dir, bta_mse_cb.app_id)) == BTA_FS_CO_OK) && is_dir)
                            {
                                /* go up one level and then go down one level to the name folder */
                                *p_oper = BTA_MSE_OPER_SETPATH;
                            }
                            else
                            {
                                rsp_code = OBX_RSP_NOT_FOUND;
                            }
#else
                            *p_oper = BTA_MSE_OPER_SETPATH;
#endif
                        }
                        else
                        {
                            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                        }
                    }
                    else
                    {
                        /* just go up one level to the parent directory */
                        *p_oper = BTA_MSE_OPER_SETPATH;
                    }
                }
                else
                {
                    rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                }
            }
            else
            {
                rsp_code = OBX_RSP_NOT_FOUND;
            }
            break;
        default:
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
            break;
    }

    return(rsp_code);
}


/*******************************************************************************
**
** Function         bta_mse_send_set_notif_reg
**
** Description      Send a set notification registration event to application
**                  so application can decide whether the request is allowed or not
**
** Parameters       status      - (output) pointer to the MSE operation code
**                  inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block                  
**                  
** Returns          TRUE - request is sent FALSE - requestr is not sent due to 
**                                                 error in the request         
*******************************************************************************/
BOOLEAN bta_mse_send_set_notif_reg(UINT8 status,
                                   UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB         *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_MA_CB              *p_scb = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE                    cback_evt_data;
    tBTA_MA_NOTIF_STATUS        notif_sts = BTA_MA_NOTIF_OFF;
    BOOLEAN                     send_status = TRUE;
    UINT8                       ccb_idx;

    if (status & BTA_MA_NOTIF_STS_ON) notif_sts = BTA_MA_NOTIF_ON;

    if (notif_sts == BTA_MA_NOTIF_OFF)
    {
        if (!bta_mse_find_bd_addr_match_mn_cb_index(p_cb->bd_addr, &ccb_idx))
        {
            send_status = FALSE;
        }
    }

    if (send_status)
    {
        cback_evt_data.set_notif_reg.mas_session_id = p_cb->obx_handle;
        cback_evt_data.set_notif_reg.mas_instance_id = p_scb->mas_inst_id;
        cback_evt_data.set_notif_reg.notif_status = notif_sts;
        bdcpy(cback_evt_data.set_notif_reg.bd_addr, p_cb->bd_addr);
        bta_mse_cb.p_cback(BTA_MSE_SET_NOTIF_REG_EVT, (tBTA_MSE *) &cback_evt_data);
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_send_set_notif_reg send_status=%d",send_status );
#endif


    return send_status;

}

/*******************************************************************************
**
** Function         bta_mse_proc_notif_reg_status
**
** Description      Process the notification registration status to determine
**                  whether a MN conenction should be opened or closed
**
** Parameters       status      - (output) pointer to the MSE operation code
**                  inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block                  
**                  
** Returns          None         
**
*******************************************************************************/
void bta_mse_proc_notif_reg_status(UINT8 status,
                                   UINT8 inst_idx, UINT8 sess_idx )
{
    tBTA_MSE_MA_SESS_CB         *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_MA_CB              *p_scb = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MA_NOTIF_STATUS        notif_sts = BTA_MA_NOTIF_OFF;
    tBTA_MSE_MN_CB              *p_ccb;  
    UINT8                       ccb_idx;
    tBTA_MSE_MN_ACT_TYPE        mn_act_type = BTA_MSE_MN_ACT_TYPE_NONE; 
    tOBX_RSP_CODE               rsp_code = OBX_RSP_OK;

    tBTA_MSE                    cback_evt_data;
    tBTA_MSE_MN_INT_OPEN        *p_open_evt;
    tBTA_MSE_MN_INT_CLOSE       *p_close_evt;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_proc_notif_reg_status");
#endif

    if (status & BTA_MA_NOTIF_STS_ON) notif_sts = BTA_MA_NOTIF_ON;

    switch (notif_sts)
    {
        case BTA_MA_NOTIF_ON: 

            if (!bta_mse_find_bd_addr_match_mn_cb_index(p_cb->bd_addr, &ccb_idx))
            {
                if (bta_mse_find_avail_mn_cb_idx(&ccb_idx))
                    mn_act_type = BTA_MSE_MN_ACT_TYPE_OPEN_CONN;
                else
                    mn_act_type = BTA_MSE_MN_ACT_TYPE_OPEN_CONN_ERR;
            }
            else
            {
                /* it is connected already */
                mn_act_type = BTA_MSE_MN_ACT_TYPE_OPEN_CONN_NONE;
            }
            break;

        case BTA_MA_NOTIF_OFF:

            if (!bta_mse_find_bd_addr_match_mn_cb_index(p_cb->bd_addr, &ccb_idx))
            {
                mn_act_type  = BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_ERR;
                break;
            }

            p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

            if ((p_ccb->state !=BTA_MSE_MN_W4_CONN_ST) && 
                (p_ccb->state !=BTA_MSE_MN_CONN_ST))
            {
                /* MN is either idle or to be closed shortly so do nothing*/
                mn_act_type = BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_NONE; 
            }
            else
            {
                if (bta_mse_mn_is_ok_to_close_mn(p_cb->bd_addr, p_scb->mas_inst_id))
                {
                    /* This is the last active MN session using this conncection*/
                    mn_act_type  = BTA_MSE_MN_ACT_TYPE_CLOSE_CONN;
                }
                else
                {
                    mn_act_type  = BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_NONE;
                }
            }
            break;
    }

    switch (mn_act_type)
    {
        case BTA_MSE_MN_ACT_TYPE_OPEN_CONN:

            if ((p_open_evt = (tBTA_MSE_MN_INT_OPEN *) GKI_getbuf(sizeof(tBTA_MSE_MN_INT_OPEN))) != NULL)
            {
                bta_mse_mn_add_inst_id(ccb_idx, p_scb->mas_inst_id);
                p_open_evt->hdr.event = BTA_MSE_MN_INT_OPEN_EVT;
                p_open_evt->ccb_idx = ccb_idx;
                p_open_evt->sec_mask = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                memcpy(p_open_evt->bd_addr, p_cb->bd_addr, sizeof(BD_ADDR));
                bta_sys_sendmsg(p_open_evt);
            }
            else
            {
                rsp_code = OBX_RSP_FAILED;
            }

            break;

        case BTA_MSE_MN_ACT_TYPE_CLOSE_CONN:

            if ((p_close_evt = (tBTA_MSE_MN_INT_CLOSE *) GKI_getbuf(sizeof(tBTA_MSE_MN_INT_CLOSE))) != NULL)
            {
                bta_mse_mn_remove_inst_id(ccb_idx, p_scb->mas_inst_id);
                p_close_evt->hdr.event = BTA_MSE_MN_INT_CLOSE_EVT;
                p_close_evt->ccb_idx = ccb_idx;
                bta_sys_sendmsg(p_close_evt);
            }
            else
            {
                rsp_code = OBX_RSP_FAILED;
            }
            break;

        case BTA_MSE_MN_ACT_TYPE_OPEN_CONN_ERR:
            rsp_code = OBX_RSP_FAILED;
            break;

        case BTA_MSE_MN_ACT_TYPE_OPEN_CONN_NONE:
            bta_mse_mn_add_inst_id(ccb_idx, p_scb->mas_inst_id);
            break;
        case BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_NONE:
            bta_mse_mn_remove_inst_id(ccb_idx, p_scb->mas_inst_id);
            break;
        default:
            break;
    }

    OBX_PutRsp(p_cb->obx_handle , rsp_code, NULL);
    if (rsp_code == OBX_RSP_OK)
        cback_evt_data.notif_reg.status = BTA_MA_STATUS_OK;
    else
        cback_evt_data.notif_reg.status = BTA_MA_STATUS_FAIL;
    cback_evt_data.notif_reg.mas_session_id = p_cb->obx_handle;
    cback_evt_data.notif_reg.mas_instance_id = p_scb->mas_inst_id;
    cback_evt_data.notif_reg.notif_status = notif_sts;
    bdcpy(cback_evt_data.notif_reg.bd_addr, p_cb->bd_addr);
    bta_mse_clean_set_notif_reg(inst_idx,sess_idx); 

    bta_mse_cb.p_cback(BTA_MSE_NOTIF_REG_EVT, (tBTA_MSE *) &cback_evt_data);
}

/*******************************************************************************
**
** Function         bta_mse_discard_data
**
** Description      frees the data
**
** Parameters       event       - MSE event                
**                  p_data      - Pointer to the MSE event data
** 
** Returns          void
**
*******************************************************************************/
void bta_mse_discard_data(UINT16 event, tBTA_MSE_DATA *p_data)
{

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_discard_data");
#endif

    switch (event)
    {
        case BTA_MSE_MA_OBX_CONN_EVT:
        case BTA_MSE_MA_OBX_DISC_EVT:
        case BTA_MSE_MA_OBX_ABORT_EVT:
        case BTA_MSE_MA_OBX_CLOSE_EVT:
        case BTA_MSE_MA_OBX_PUT_EVT:
        case BTA_MSE_MA_OBX_GET_EVT:
        case BTA_MSE_MA_OBX_SETPATH_EVT:
            utl_freebuf((void**)&p_data->obx_evt.p_pkt);
            break;

        default:
            /*Nothing to free*/
            break;
    }
}



/*******************************************************************************
**
** Function       bta_mse_find_mas_inst_id_match_cb_idx  
**
** Description    Finds the MAS instance control block index based on the specified 
**                MAS instance ID    
**
** Parameters     mas_inst_id   -  MAS instance ID               
**                p_idx         - (output) pointer to the MA control block index
**  
** Returns          BOOLEAN - TRUE found
**                            FALSE not found 
*******************************************************************************/
BOOLEAN bta_mse_find_mas_inst_id_match_cb_idx(tBTA_MA_INST_ID mas_inst_id, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (bta_mse_cb.scb[i].in_use)
        {
            if (bta_mse_cb.scb[i].mas_inst_id == mas_inst_id)
            {
                found = TRUE;
                *p_idx = i;
                break;
            }
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_find_mas_inst_id_match_cb_idx found=%d, inst_id=%d inst_idx=%d",
                      found, mas_inst_id, i);
#endif
    return found;
}


/*******************************************************************************
**
** Function       bta_mse_find_bd_addr_match_sess_cb_idx  
**
** Description    Finds the Session control block index based on the specified 
**                MAS instance control block index and BD address   
** 
** Parameters     bd_addr       - BD address
**                inst_idx      - MA control block index             
**                p_idx         - (output) pointer to the MA server control block 
**                                index
**                  
** 
** Returns          BOOLEAN - TRUE found
**                            FALSE not found 
*******************************************************************************/
BOOLEAN bta_mse_find_bd_addr_match_sess_cb_idx(BD_ADDR bd_addr, UINT8 inst_idx, 
                                               UINT8 *p_idx)
{
    tBTA_MSE_MA_SESS_CB     *p_cb;
    BOOLEAN                 found=FALSE;
    UINT8                   i;

    for (i=0; i < BTA_MSE_NUM_SESS ; i ++)
    {
        p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, i); 
        if ((p_cb->state ==  BTA_MSE_MA_CONN_ST) &&
            !memcmp (p_cb->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_find_bd_addr_match_sess_cb_idx found=%d, inst_idx=%d p_idx=%d",
                      found, inst_idx, i);
#endif
    return found;
}


/*******************************************************************************
**
** Function       bta_mse_find_handle_match_mas_inst_cb_idx  
**
** Description   Finds the MAS instance control block index based on the specified Obx handle   
** 
** Parameters     obx_handle    - Obex session handle
**                p_idx         - (output) pointer to the MA server control block index
**                   
** 
** Returns          BOOLEAN - TRUE found
**                            FALSE not found         
**
*******************************************************************************/
BOOLEAN bta_mse_find_handle_match_mas_inst_cb_idx(tOBX_HANDLE obx_handle, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (bta_mse_cb.scb[i].in_use)
        {
            if (bta_mse_cb.scb[i].obx_handle == obx_handle)
            {
                found = TRUE;
                *p_idx = i;
                break;
            }
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_find_handle_match_mas_inst_cb_idx found=%d idx=%d",found, i);
#endif
    return found;
}
/*******************************************************************************
**
** Function       bta_mse_find_mas_sess_cb_idx  
**
** Description   Finds the MAS instance and session control block indexes 
**               based on Obx handle   
**
** Parameters     obx_handle        - Obex session handle
**                p_mas_inst_idx    - (output) pointer to the MA server control 
**                                     block index
**                p_mas_sess_idx    - (output) pointer to the MA session control 
**                                     block index   
**                  
** Returns      BOOLEAN - TRUE found
**                        FALSE not found         
**
*******************************************************************************/
BOOLEAN bta_mse_find_mas_sess_cb_idx(tOBX_HANDLE obx_handle, 
                                     UINT8 *p_mas_inst_idx, UINT8 *p_mas_sess_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i, j;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_mas_sess_cb_idx");
#endif
    for (i=0; i< BTA_MSE_NUM_INST; i++)
    {
        if (bta_mse_cb.scb[i].in_use)
        {
            for (j=0; j < BTA_MSE_NUM_SESS; j++  )
            {
                if ( (bta_mse_cb.scb[i].sess_cb[j].state != BTA_MSE_MA_LISTEN_ST) &&
                     (bta_mse_cb.scb[i].sess_cb[j].obx_handle == obx_handle) )
                {
                    found = TRUE;
                    *p_mas_inst_idx = i;
                    *p_mas_sess_idx = j;
                    return found;
                }
            }
        }
    }

    return found;
}


/*******************************************************************************
**
** Function       bta_mse_find_ma_cb_indexes  
**
** Description   Finds the MAS instance and session control block indexes 
**               based on the received internal event   
**
** Parameters     p_msg             - Pointer to MSE msg data
**                p_mas_inst_idx    - (output) pointer to the MA server control 
**                                     block index
**                p_mas_sess_idx    - (output) pointer to the MA session control 
**                                     block index   
**                  
** Returns      BOOLEAN - TRUE found
**                        FALSE not found  
**
*******************************************************************************/
BOOLEAN bta_mse_find_ma_cb_indexes(tBTA_MSE_DATA *p_msg, 
                                   UINT8 *p_inst_idx, UINT8  *p_sess_idx)
{
    BOOLEAN found = FALSE;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_ma_cb_indexes");
#endif
    switch (p_msg->hdr.event)
    {
        case BTA_MSE_MA_OBX_CONN_EVT:

            if (bta_mse_find_handle_match_mas_inst_cb_idx( p_msg->obx_evt.param.conn.handle, p_inst_idx))
            {
                if (bta_mse_find_avail_mas_sess_cb_idx(&(bta_mse_cb.scb[*p_inst_idx]), p_sess_idx))
                    found = TRUE;
            }
            break;

        case BTA_MSE_API_ACCESSRSP_EVT:  

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->api_access_rsp.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_API_UPD_IBX_RSP_EVT:  

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->api_upd_ibx_rsp.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_API_SET_NOTIF_REG_RSP_EVT:  

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->api_set_notif_reg_rsp.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_INT_CLOSE_EVT:  

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->int_close.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_CI_GET_FENTRY_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_get_fentry.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;
        case BTA_MSE_CI_GET_ML_INFO_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_get_ml_info.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;
        case BTA_MSE_CI_GET_ML_ENTRY_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_get_ml_entry.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;
        case BTA_MSE_CI_GET_MSG_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_get_msg.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_CI_PUSH_MSG_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_push_msg.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_CI_DEL_MSG_EVT:

            if (bta_mse_find_sess_id_match_ma_cb_indexes(p_msg->ci_del_msg.mas_session_id,
                                                         p_inst_idx,p_sess_idx))
                found = TRUE;

            break;

        case BTA_MSE_MA_OBX_DISC_EVT:       
        case BTA_MSE_MA_OBX_ABORT_EVT:      
        case BTA_MSE_MA_OBX_CLOSE_EVT:      
        case BTA_MSE_MA_OBX_PUT_EVT:        
        case BTA_MSE_MA_OBX_GET_EVT:        
        case BTA_MSE_MA_OBX_SETPATH_EVT: 

            if (bta_mse_find_mas_sess_cb_idx( p_msg->obx_evt.handle, p_inst_idx, p_sess_idx))
                found = TRUE;
            break;
        default:
            break;

    }

    return found;
}


/*******************************************************************************
**
** Function       bta_mse_ma_cleanup  
**
** Description    Free resources if unable to find MA control block indexes  
**
** Parameters     p_msg - Pointer to MSE msg data
**                  
** Returns        none  
**
*******************************************************************************/
void bta_mse_ma_cleanup(tBTA_MSE_DATA *p_msg)
{
    tBTA_MSE_OBX_EVT        *p_evt = &p_msg->obx_evt;
    tBTA_MA_OBX_RSP         *p_rsp = NULL;
    UINT8                   rsp_code = OBX_RSP_BAD_REQUEST;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_cleanup");
#endif
    switch (p_msg->hdr.event)
    {
        case BTA_MSE_MA_OBX_CONN_EVT:
            p_rsp = OBX_ConnectRsp;
            rsp_code = OBX_RSP_SERVICE_UNAVL;
            break;
        case BTA_MSE_MA_OBX_DISC_EVT:   
            p_rsp = OBX_DisconnectRsp;
            break;
        case BTA_MSE_MA_OBX_ABORT_EVT:   
            p_rsp = OBX_AbortRsp;
            break;
        case BTA_MSE_MA_OBX_PUT_EVT: 
            p_rsp = OBX_PutRsp;
            break;
        case BTA_MSE_MA_OBX_GET_EVT:  
            p_rsp = OBX_GetRsp;
            break;
        case BTA_MSE_MA_OBX_SETPATH_EVT: 
            p_rsp = OBX_SetPathRsp;
            break;
        default:
            break;
    }

    if (p_rsp)
    {
        (*p_rsp)(p_evt->handle, rsp_code, (BT_HDR *)NULL);
        /* Done with Obex packet */
        utl_freebuf((void**)&p_evt->p_pkt);
    }
}



/*******************************************************************************
**
** Function      bta_mse_is_a_duplicate_id  
**
** Description   Determine the MAS instance ID has been used or not by other MAS instance 
**  
** Parameters     mas_inst_id - MAS instance ID          
**                   
** Returns      BOOLEAN - TRUE the MAS isntance is a duplicate ID
**                        FALSE not a duplicate ID
*******************************************************************************/
BOOLEAN bta_mse_is_a_duplicate_id(tBTA_MA_INST_ID mas_inst_id)
{
    BOOLEAN is_duplicate=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (bta_mse_cb.scb[i].in_use && 
            (bta_mse_cb.scb[i].mas_inst_id == mas_inst_id))
        {
            is_duplicate = TRUE;

            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_is_a_duplicate_id inst_id=%d status=%d",
                      mas_inst_id, is_duplicate);
#endif

    return is_duplicate;
}


/*******************************************************************************
**
** Function      bta_mse_find_avail_mas_inst_cb_idx  
**
** Description   Finds a not in used MAS instance control block index 
** 
** Parameters     p_idx    - (output) pointer to the MA server control 
**                           block index
**                 
** Returns      BOOLEAN - TRUE found
**                        FALSE not found          
**
*******************************************************************************/
BOOLEAN bta_mse_find_avail_mas_inst_cb_idx(UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;


    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (!bta_mse_cb.scb[i].in_use)
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_find_avail_mas_inst_cb_idx found=%d inst_idx=%d",
                      found, i);
#endif
    return found;
}
/*******************************************************************************
**
** Function      bta_mse_find_avail_mas_sess_cb_idx  
**
** Description   Finds a not in used MAS session control block index 
**  
** Parameters    p_scb    - Pointer to the MA control block
**               p_idx    - (output) pointer to the MA session control 
**                           block index
**                                 
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found   
*******************************************************************************/
BOOLEAN bta_mse_find_avail_mas_sess_cb_idx(tBTA_MSE_MA_CB *p_scb, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_SESS ; i ++)
    {
        if (p_scb->sess_cb[i].state == BTA_MSE_MA_LISTEN_ST)
        {
            if ((p_scb->sess_cb[i].p_workdir = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
            {
                found = TRUE;
                *p_idx = i;
            }
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_find_avail_mas_sess_cb_idx found=%d idx=%d", found, i);
#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_mse_find_avail_mn_cb_idx  
**
** Description   Finds a not in use MN control block index
** 
** Parameters    p_idx    - (output) pointer to the MN control block index
**  
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found       
**                            
*******************************************************************************/
BOOLEAN bta_mse_find_avail_mn_cb_idx(UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_avail_mn_cb_idx");
#endif
    for (i=0; i < BTA_MSE_NUM_MN ; i ++)
    {
        if (!bta_mse_cb.ccb[i].in_use)
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }
    return found;
}



/*******************************************************************************
**
** Function      bta_mse_find_bd_addr_match_mn_cb_index  
**
** Description   Find the MN control block index based on the specified BD address
**   
** Parameters   p_bd_addr   - Pointer to the BD address
**              p_idx       - (output) pointer to the MN control block index
**  
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found               
**
*******************************************************************************/

BOOLEAN bta_mse_find_bd_addr_match_mn_cb_index(BD_ADDR p_bd_addr, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_MN ; i ++)
    {
        if ((bta_mse_cb.ccb[i].in_use) &&
            (!memcmp (bta_mse_cb.ccb[i].bd_addr, p_bd_addr, BD_ADDR_LEN)))
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_find_bd_addr_match_mn_cb_index found=%d index=%d", found, i);
#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_mse_find_bd_addr_match_mn_cb_index  
**
** Description   Find the MN control block index based on the specified obx handle
**
** Parameters   obx_hdl     - Obex session handle
**              p_idx       - (output) pointer to the MN control block index
**    
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found           
**
*******************************************************************************/

BOOLEAN bta_mse_find_obx_hdl_match_mn_cb_index(tOBX_HANDLE obx_hdl, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_obx_hdl_match_mn_cb_index");
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if ((bta_mse_cb.ccb[i].in_use) &&
            (bta_mse_cb.ccb[i].obx_handle == obx_hdl))
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

    return found;
}

/*******************************************************************************
**
** Function      bta_mse_find_sess_id_match_ma_cb_indexes  
**
** Description   Finds the MAS instance and session control block indexes 
**               based on the specified MAS session ID
**   
** Parameters     mas_session_id    - MAS instance ID
**                p_inst_idx        - (output) pointer to the MA server control 
**                                     block index
**                p_sess_idx        - (output) pointer to the MA session control 
**                                     block index   
**
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found     
**
*******************************************************************************/
BOOLEAN bta_mse_find_sess_id_match_ma_cb_indexes(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                 UINT8 *p_inst_idx, UINT8 *p_sess_idx)
{
    BOOLEAN found=FALSE;
    UINT8       i,j;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_sess_id_match_ma_cb_indexes");
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        for (j=0; j<BTA_MSE_NUM_SESS; j++ )
        {
            if ((bta_mse_cb.scb[i].in_use) &&
                (bta_mse_cb.scb[i].sess_cb[j].obx_handle == (tOBX_HANDLE) mas_session_id ))
            {
                found = TRUE;
                *p_inst_idx = i;
                *p_sess_idx = j;
                return found;
            }
        }
    }

    return found;
}

/*******************************************************************************
**
** Function      bta_mse_find_sess_id_match_mn_cb_index  
**
** Description   Finds the MN control block index 
**               based on the specified MAS session ID
**
** Parameters     mas_session_id    - MAS instance ID
**                p_idx             - (output) pointer to the MN control 
**                                     block index  
** 
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found         
**
*******************************************************************************/
BOOLEAN bta_mse_find_sess_id_match_mn_cb_index(tBTA_MA_SESS_HANDLE mas_session_id, 
                                               UINT8 *p_idx)
{
    BOOLEAN found=FALSE;

    UINT8       i,j;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_sess_id_match_mn_cb_index");
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        for (j=0; j<BTA_MSE_NUM_SESS; j++ )
        {
            if ((bta_mse_cb.scb[i].in_use) &&
                (bta_mse_cb.scb[i].sess_cb[j].obx_handle == (tOBX_HANDLE) mas_session_id ))
            {
                found = TRUE;
                break;
            }
        }
        if (found) break;
    }

    /* found session index now need to match BD address*/
    if (found)
    {
        found = FALSE;
        if ( bta_mse_find_bd_addr_match_mn_cb_index(bta_mse_cb.scb[i].sess_cb[j].bd_addr, p_idx))
        {
            found = TRUE;
        }
    }
    return found;
}


/*******************************************************************************
**
** Function      bta_mse_find_mn_cb_index  
**
** Description   Finds the MN control block index 
**               based on the specified event
**
** Parameters     p_msg        - Pointer to MSE msg data
**                p_ccb_idx    - (output) pointer to the MN control block index 
**   
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found          
**
*******************************************************************************/
BOOLEAN bta_mse_find_mn_cb_index(tBTA_MSE_DATA *p_msg, UINT8 *p_ccb_idx)
{
    BOOLEAN found = TRUE;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_find_mn_cb_index");
#endif
    switch (p_msg->hdr.event)
    {
        case BTA_MSE_MN_INT_OPEN_EVT:
            *p_ccb_idx= p_msg->mn_int_open.ccb_idx;
            break;

        case BTA_MSE_MN_INT_CLOSE_EVT:
            *p_ccb_idx= p_msg->mn_int_close.ccb_idx;
            break;

        case BTA_MSE_MN_OBX_CONN_RSP_EVT:

            if (!bta_mse_find_bd_addr_match_mn_cb_index(
                                                       p_msg->obx_evt.param.conn.peer_addr,
                                                       p_ccb_idx))
            {
                found = FALSE;
            }
            break;

        case BTA_MSE_MN_OBX_TOUT_EVT:
        case BTA_MSE_MN_OBX_CLOSE_EVT:
        case BTA_MSE_MN_OBX_PUT_RSP_EVT:
            if (!bta_mse_find_obx_hdl_match_mn_cb_index(
                                                       p_msg->obx_evt.handle,
                                                       p_ccb_idx))
            {
                found = FALSE;
            }

            break;

        case BTA_MSE_MN_SDP_OK_EVT:

            *p_ccb_idx = p_msg->mn_sdp_ok.ccb_idx;
            break;

        case BTA_MSE_MN_SDP_FAIL_EVT:

            *p_ccb_idx = p_msg->mn_sdp_fail.ccb_idx;
            break;

        default:
            found = FALSE;
            break;
    }

    return found;
}


/*******************************************************************************
**
** Function      bta_mse_mn_cleanup  
**
** Description   Free resources if unable to find MN control block index  
**
** Parameters     p_msg        - Pointer to MSE msg data
**   
** Returns      none        
**
*******************************************************************************/
void bta_mse_mn_cleanup(tBTA_MSE_DATA *p_msg)
{
    tBTA_MSE_OBX_EVT        *p_evt = &p_msg->obx_evt;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_cleanup");
#endif
    switch (p_msg->hdr.event)
    {
        case BTA_MSE_MN_OBX_CONN_RSP_EVT:
        case BTA_MSE_MN_OBX_PUT_RSP_EVT:
            /* Done with Obex packet */
            utl_freebuf((void**)&p_evt->p_pkt);
            break;

        default:
            break;
    }
}
/*******************************************************************************
**
** Function         bta_mse_build_map_event_rpt_obj
**
** Description      Create a MAP-Event-Report object in the
**                  specified buffer.
**                             
** Parameters       notif_type - Notification type
**                  handle (input only) - handle of the message that the "type"
**                      indication refers to.  Ignored when the event "type" is 
**                      "MemoryFull" or "MemoryAvailable".
**                  p_folder - name of the folder in which the corresponding
**                      message has been filed by the MSE.  NULL when the event
**                      "type" is "MemoryFull" or "MemoryAvailable".
**                  p_old_folder - Used only in case of a message shift to
**                      indicate the folder on the MSE from which the message
**                      has been shifted out.  
**                  msg_typ - Gives the type of the message.  Ignored when the
**                      event "type" is "MemoryFull" or "MemoryAvailable".
**                  p_len - Pointer to value containing the size of 
**                      the buffer (p_buffer).  Receives the output size of
**                      filled XML object.
**                  p_buffer - Pointer to buffer to receive the XML object.
**
** Returns          BTA_MA_STATUS_FAIL if buffer was not large enough, otherwise
**                      returns BTA_MA_STATUS_OK.
**
*******************************************************************************/
tBTA_MA_STATUS bta_mse_build_map_event_rpt_obj(tBTA_MSE_NOTIF_TYPE notif_type, 
                                               tBTA_MA_MSG_HANDLE handle, 
                                               char * p_folder,
                                               char * p_old_folder, 
                                               tBTA_MA_MSG_TYPE msg_typ,
                                               UINT16 * p_len,
                                               UINT8 * p_buffer)
{
    tBTA_MA_STREAM      strm;

    memset(p_buffer, 0, *p_len);
    BTA_MaInitMemStream(&strm, p_buffer, *p_len);

    /* stream event attribute and event type */
    bta_ma_stream_str(&strm, "<MAP-event-report version=\"1.0\">\n"
                      "<event type = \"");
    bta_ma_stream_str(&strm, bta_ma_evt_typ_to_string(notif_type));

    /* Some things are not used for "MemoryFull" and "MemoryAvailable" */
    if ( (BTA_MSE_NOTIF_TYPE_MEMORY_FULL != notif_type)
         && (BTA_MSE_NOTIF_TYPE_MEMORY_AVAILABLE != notif_type) )
    {
        /* stream handle label and value */
        bta_ma_stream_str(&strm, "\" handle = \"");
        bta_ma_stream_handle(&strm, handle);

        if (p_folder && (*p_folder !='\0'))
        {
            /* stream folder */
            bta_ma_stream_str(&strm, "\" folder = \"");
            bta_ma_stream_str(&strm, p_folder);
        }

        /* stream old_folder if it is a "MessageShift" */
        if ( (BTA_MSE_NOTIF_TYPE_MESSAGE_SHIFT == notif_type) &&
             p_old_folder && (*p_old_folder !='\0'))
        {
            /* stream folder */
            bta_ma_stream_str(&strm, "\" old_folder = \"");
            bta_ma_stream_str(&strm, p_old_folder);
        }

        /* stream message type */
        bta_ma_stream_str(&strm, "\" msg_type = \"");
        bta_ma_stream_str(&strm, bta_ma_msg_typ_to_string(msg_typ));
    }

    /* we are done with this evnet */
    bta_ma_stream_str(&strm, "\" />\n</MAP-event-report>");

    /* set the output length (i.e. amount of buffer that got used) */
    *p_len = bta_ma_stream_used_size(&strm);

    /* return status based on the stream status */
    return(bta_ma_stream_ok(&strm) 
           ? BTA_MA_STATUS_OK
           : BTA_MA_STATUS_FAIL);
}


/*******************************************************************************
**
** Function         bta_mse_build_msg_listing_obj
**
** Description      Build the message listing object in the specified buffer    
**
** Parameters       p_entry - Pointer to the message listing entry
**                  p_size  - input: pointer to the available buffer size
**                            output: pointer to the filled buffer size
**                  p_buf   - pointer to the buffer for building the msg listing
**                            object
**
** Returns          status - BTA_MA_STATUS_OK  - build the object successfully 
**                           BTA_MA_STATUS_FAIL - failed to build the object
**
*******************************************************************************/
tBTA_MA_STATUS bta_mse_build_msg_listing_obj(tBTA_MSE_CO_MSG_LIST_ENTRY *p_entry, 
                                             UINT16 *p_size, char *p_buf  )
{
    tBTA_MA_STREAM      strm;

    memset(p_buf, 0, *p_size);
    BTA_MaInitMemStream(&strm, (UINT8 *)p_buf, *p_size);

    /* stream msg element */
    bta_ma_stream_str(&strm, "<msg ");
    /* stream msg attributes */
    bta_ma_stream_str(&strm, "handle = \"");
    bta_ma_stream_handle(&strm, p_entry->msg_handle);

    if (p_entry->parameter_mask &BTA_MA_ML_MASK_SUBJECT)
    {
        bta_ma_stream_str(&strm, "\" subject = \"");
        bta_ma_stream_str(&strm, p_entry->subject);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_DATETIME)
    {
        bta_ma_stream_str(&strm, "\" datetime = \"");
        bta_ma_stream_str(&strm, p_entry->date_time);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_SENDER_NAME)
    {
        bta_ma_stream_str(&strm, "\" sender_name = \"");
        bta_ma_stream_str(&strm, p_entry->sender_name);
    }


    if (p_entry->parameter_mask & BTA_MA_ML_MASK_SENDER_ADDRESSING)
    {
        bta_ma_stream_str(&strm, "\" sender_addressing = \"");
        bta_ma_stream_str(&strm, p_entry->sender_addressing);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_REPLYTO_ADDRESSING)
    {
        bta_ma_stream_str(&strm, "\" replyto_addressing = \"");
        bta_ma_stream_str(&strm, p_entry->replyto_addressing);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_RECIPIENT_NAME)
    {
        bta_ma_stream_str(&strm, "\" recipient_name = \"");
        bta_ma_stream_str(&strm, p_entry->recipient_name);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_RECIPIENT_ADDRESSING)
    {
        bta_ma_stream_str(&strm, "\" recipient_addressing = \"");
        bta_ma_stream_str(&strm, p_entry->recipient_addressing);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_TYPE)
    {
        bta_ma_stream_str(&strm, "\" type = \"");
        if (!bta_ma_stream_str(&strm, bta_ma_msg_typ_to_string(p_entry->type)))
        {
            return  BTA_MA_STATUS_FAIL;
        }
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_SIZE)
    {
        bta_ma_stream_str(&strm, "\" size = \"");
        bta_ma_stream_value(&strm,  p_entry->org_msg_size);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_TEXT)
    {
        bta_ma_stream_str(&strm, "\" text = \"");
        bta_ma_stream_boolean_yes_no(&strm,  p_entry->text);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_RECEPTION_STATUS)
    {
        bta_ma_stream_str(&strm, "\" reception_status = \"");
        if (!bta_ma_stream_str(&strm,   
                               bta_ma_rcv_status_to_string(p_entry->reception_status)))
        {
            return BTA_MA_STATUS_FAIL;
        }
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_ATTACHMENT_SIZE)
    {
        bta_ma_stream_str(&strm, "\" attachment_size = \"");
        bta_ma_stream_value(&strm,  p_entry->attachment_size);
    }

    if (p_entry->parameter_mask & BTA_MA_ML_MASK_PRIORITY)
    {
        bta_ma_stream_str(&strm, "\" priority = \"");
        bta_ma_stream_boolean_yes_no(&strm,  p_entry->high_priority);
    }
    if (p_entry->parameter_mask & BTA_MA_ML_MASK_READ)
    {
        bta_ma_stream_str(&strm, "\" read = \"");
        bta_ma_stream_boolean_yes_no(&strm,  p_entry->read);
    }
    if (p_entry->parameter_mask & BTA_MA_ML_MASK_SENT)
    {
        bta_ma_stream_str(&strm, "\" sent = \"");
        bta_ma_stream_boolean_yes_no(&strm,  p_entry->sent);
    }
    if (p_entry->parameter_mask & BTA_MA_ML_MASK_PROTECTED)
    {
        bta_ma_stream_str(&strm, "\" protected = \"");
        bta_ma_stream_boolean_yes_no(&strm,  p_entry->is_protected);
    }

    /* stream msg element end tag*/
    bta_ma_stream_str(&strm, "\"/> ");

    /* set the output length (i.e. amount of buffer that got used) */
    *p_size = bta_ma_stream_used_size(&strm);

    /* return status based on the stream status */
    return(bta_ma_stream_ok(&strm) 
           ? BTA_MA_STATUS_OK
           : BTA_MA_STATUS_FAIL);
}

/*******************************************************************************
**
** Function      bta_mse_mn_start_timer  
**
** Description   Start a wait for obx response timer
**
** Parameters    ccb_inx  - MN control block index
**               timer_id - indicating this timer is for which operation 
**   
** Returns      None         
**
*******************************************************************************/
void bta_mse_mn_start_timer(UINT8 ccb_idx, UINT8 timer_id)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    UINT16 event_id;
    p_cb->rsp_timer.param = (UINT32) (ccb_idx+1); 
    event_id = (UINT16) BTA_MSE_MN_RSP0_TOUT_EVT + ccb_idx; 
    bta_sys_start_timer(&p_cb->rsp_timer, event_id,
                        p_bta_mse_cfg->obx_rsp_tout);

    p_cb->timer_oper = timer_id;
}
/*******************************************************************************
**
** Function      bta_mse_mn_start_stop_timer  
**
** Description   Stop a wait for obx response timer
**
** Parameters    ccb_inx - MN control block index
**               timer_id - indicating this timer is for which operation 
**    
** Returns      None      
**
*******************************************************************************/
void bta_mse_mn_stop_timer(UINT8 ccb_idx, UINT8 timer_id)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

    if ((p_cb->timer_oper == timer_id) || (timer_id == BTA_MSE_TIMER_OP_ALL))
    {
        p_cb->rsp_timer.param = 0;
        bta_sys_stop_timer(&p_cb->rsp_timer );
        p_cb->timer_oper = BTA_MSE_TIMER_OP_NONE;
    }
}

/*******************************************************************************
**
** Function      bta_mse_mn_add_inst_id  
**
** Description   Add mas_inst_id to the MN notif_reg data base
** 
** Parameters    ccb_inx        - MN control block index
**               mas_inst_id    - MAS instance ID
**    
** Returns      BOOLEAN - TRUE OK        
**                        FALSE not OK       
**                            
*******************************************************************************/
BOOLEAN bta_mse_mn_add_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id)
{
    BOOLEAN found=FALSE;
    BOOLEAN add_status=FALSE;
    UINT8 i;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_add_inst_id ccb_idx=%d mas_inst_id=%d",
                      ccb_idx, mas_inst_id);
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status && 
            (p_ccb->notif_reg[i].mas_inst_id  == mas_inst_id))
        {
            found = TRUE;
            add_status = TRUE;
            break;
        }
    }

    if (!found)
    {
        for (i=0; i < BTA_MSE_NUM_INST ; i ++)
        {
            if (!p_ccb->notif_reg[i].status)
            {
                /* find an available entry to add */
                p_ccb->notif_reg[i].mas_inst_id  = mas_inst_id;
                p_ccb->notif_reg[i].status = TRUE;
                add_status = TRUE;
                break;
            }
        }
    }


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("add_status=%d", add_status);
#endif

    return add_status;
}

/*******************************************************************************
**
** Function      bta_mse_mn_remove_inst_id  
**
** Description   Remove mas_inst_id from the MN notif_reg data base
**
** Parameters    ccb_inx        - MN control block index
**               mas_inst_id    - MAS instance ID
**    
** Returns      BOOLEAN - TRUE OK        
**                        FALSE not OK       
**                            
*******************************************************************************/
BOOLEAN bta_mse_mn_remove_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id)
{
    BOOLEAN remove_status=FALSE;
    UINT8 i;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_remove_inst_id ccb_idx=%d mas_inst_id=%d",
                      ccb_idx, mas_inst_id);
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status && 
            (p_ccb->notif_reg[i].mas_inst_id  == mas_inst_id))
        {
            p_ccb->notif_reg[i].status = FALSE;
            p_ccb->notif_reg[i].mas_inst_id  =(tBTA_MA_INST_ID )0;

            remove_status = TRUE;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("remove_status=%d", remove_status);
#endif

    return remove_status;
}

/*******************************************************************************
**
** Function      bta_mse_mn_remove_all_inst_ids  
**
** Description   Remove all mas_inst_ids from the MN notif_reg data base
**
** Parameters    ccb_inx        - MN control block index
**    
** Returns      None      
**                            
*******************************************************************************/
void bta_mse_mn_remove_all_inst_ids(UINT8 ccb_idx)
{
    UINT8 i;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_remove_all_inst_ids ccb_idx=%d ",
                      ccb_idx);
#endif
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status)
        {
            p_ccb->notif_reg[i].status = FALSE;
            p_ccb->notif_reg[i].mas_inst_id  =(tBTA_MA_INST_ID) 0;
        }
    }
}

/*******************************************************************************
**
** Function      bta_mse_mn_find_num_of_act_inst_id  
**
** Description   fin the number of Mas Instance IDs with registration status on
**
** Parameters    ccb_inx - MN control block index
**    
** Returns      UINT8 - Number of active Mas Instance ID    
**                            
*******************************************************************************/
UINT8 bta_mse_mn_find_num_of_act_inst_id(UINT8 ccb_idx)
{
    UINT8 i,cnt;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);; 

    cnt =0;
    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status) cnt++;
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_find_num_of_act_inst_id ccb_idx=%d cnt=%d",
                      ccb_idx, cnt);
#endif

    return cnt;
}

/*******************************************************************************
**
** Function      bta_mse_mn_is_inst_id_exist  
**
** Description   Check whether the specified mas_inst_id is in the
**               MN notif_reg data base
** 
** Parameters    ccb_inx        - MN control block index
**               mas_inst_id    - MAS instance ID
**
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found         
**                            
**
*******************************************************************************/
BOOLEAN bta_mse_mn_is_inst_id_exist(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id )
{
    BOOLEAN found = FALSE;
    UINT8 i;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);; 

    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status && 
            (p_ccb->notif_reg[i].mas_inst_id  == mas_inst_id))
        {
            found = TRUE;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_mn_is_inst_id_exist ccb_idx=%d mas_inst_id=%d found=%d",
                      ccb_idx, mas_inst_id, found);
#endif

    return found;
}

/*******************************************************************************
**
** Function      bta_mse_mn_is_ok_to_close_mn  
**
** Description   Determine is ok to close MN connection
**
** Parameters    bd_addr        - BD address
**               mas_inst_id    - MAS instance ID
**   
** Returns      BOOLEAN - TRUE OK        
**                        FALSE not OK       
**                            
*******************************************************************************/
BOOLEAN bta_mse_mn_is_ok_to_close_mn(BD_ADDR bd_addr, tBTA_MA_INST_ID mas_inst_id )
{
    UINT8 ccb_idx;
    BOOLEAN ok_status= FALSE;


    if (bta_mse_find_bd_addr_match_mn_cb_index(bd_addr, &ccb_idx) &&
        (bta_mse_mn_find_num_of_act_inst_id(ccb_idx) == 1) &&
        (bta_mse_mn_is_inst_id_exist(ccb_idx, mas_inst_id)))
    {
        ok_status = TRUE;
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_is_ok_to_close_mn mas_inst_id=%d ok_status=%d",
                      mas_inst_id, ok_status);
#endif

    return ok_status;
}


/*******************************************************************************
**
** Function      bta_mse_mn_get_first_inst_id  
**
** Description   Get the first active mas_inst_id from the MN notif_reg data base
**
** Parameters    ccb_inx        - MN control block index
**               mas_inst_id    - MAS instance ID
**   
** Returns      BOOLEAN - TRUE OK        
**                        FALSE not OK       
**                            
*******************************************************************************/
BOOLEAN bta_mse_mn_get_first_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID *p_mas_inst_id)
{
    BOOLEAN found=FALSE;
    UINT8 i;
    tBTA_MSE_MN_CB *p_ccb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);


    for (i=0; i < BTA_MSE_NUM_INST ; i ++)
    {
        if (p_ccb->notif_reg[i].status )
        {
            *p_mas_inst_id = p_ccb->notif_reg[i].mas_inst_id;

            found = TRUE;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_mn_get_inst_id ccb_idx=%d found status =%d mas_inst_id=%d",
                      ccb_idx, found, *p_mas_inst_id);
#endif

    return found;
}


/*******************************************************************************
**
** Function         bta_mse_mn_send_abort_req
**
** Description      Send an abort request.
**
** Parameters       ccb_inx  - MN control block index
**
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_send_abort_req(UINT8 ccb_idx)
{
    tBTA_MSE_MN_CB          *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

    if (BTA_MSE_MN_ABORT_REQ_NOT_SENT == p_cb->aborting)
    {
        bta_mse_mn_start_timer(ccb_idx, BTA_MSE_TIMER_OP_ABORT);
        OBX_AbortReq(p_cb->obx_handle, (BT_HDR *)NULL);
        p_cb->aborting = BTA_MSE_MN_ABORT_REQ_SENT;
    }
}

/*******************************************************************************
**
** Function      bta_mse_mn_cont_send_notif  
**
** Description   Continues the send notification operation. Builds a new OBX packet
** 
** Parameters    ccb_idx    - MN control block index 
**               first_pkt  - first obex packet indicator
**    
** Returns       tBTA_MA_STATUS : BTA_MA_STATUS_OK if msg notification sent is ok             
**                                otherwise BTA_MA_STATUS_FAIL
*******************************************************************************/
tBTA_MA_STATUS bta_mse_mn_cont_send_notif(UINT8 ccb_idx, BOOLEAN first_pkt)
{
    tBTA_MSE_MN_CB          *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_OBX_PKT        *p_obx = &p_cb->obx;
    tBTA_MSE_MN_MSG_NOTIF   *p_msg_notif = &p_cb->msg_notif;
    tOBX_TRIPLET            app_param[1];
    UINT16                  body_len;
    BOOLEAN                 final_pkt = FALSE;
    tBTA_MA_STATUS          status = BTA_MA_STATUS_OK;

    /* Do not start another request if currently aborting */
    if (p_cb->aborting)
    {
        bta_mse_mn_send_abort_req(ccb_idx);
        return status;
    }

    if ((p_obx->p_pkt = OBX_HdrInit(p_cb->obx_handle, p_cb->peer_mtu)) != NULL)
    {
        if (first_pkt)
        {
            OBX_AddTypeHdr(p_obx->p_pkt, BTA_MA_HDR_TYPE_EVENT_RPT);

            app_param[0].tag = BTA_MA_NAS_INST_ID_TAG_ID;
            app_param[0].len = BTA_MA_NAS_INST_ID_LEN;
            app_param[0].p_array = &(p_msg_notif->mas_instance_id);
            OBX_AddAppParamHdr(p_obx->p_pkt, app_param, 1);
        }
        else
        {
            p_obx->offset     =
            p_obx->bytes_left = 0; /* 0 -length available */
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

            body_len =  p_obx->bytes_left;
            final_pkt = ( (p_msg_notif->buffer_len - p_msg_notif->bytes_sent) < body_len) ?
                        TRUE : FALSE ;
            if (final_pkt) body_len = (p_msg_notif->buffer_len - p_msg_notif->bytes_sent);

            memcpy(&p_obx->p_start[p_obx->offset], 
                   &(p_msg_notif->p_buffer[p_msg_notif->bytes_sent]), body_len);
            p_msg_notif->bytes_sent += body_len;
            p_obx->offset           += body_len;
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
        }

        OBX_PutReq(p_cb->obx_handle, final_pkt , p_obx->p_pkt);
        p_obx->p_pkt = NULL;
        p_cb->req_pending = TRUE;
        bta_mse_set_mn_oper(ccb_idx, BTA_MSE_MN_OP_PUT_EVT_RPT);
        p_msg_notif->final_pkt  = final_pkt;
        p_msg_notif->pkt_cnt++;
    }
    else
    {
        status = BTA_MA_STATUS_FAIL;
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT4("bta_mse_mn_cont_send_notif ccb_idx=%d first_pkt=%d send_status=%d final=%d",
                      ccb_idx, first_pkt, status, final_pkt);
#endif

    return status;
}
/*******************************************************************************
**
** Function      bta_mse_mn_send_notif_evt  
**
** Description   Issue a send notification event
**
** Parameters    mas_instance_id - MAS instance ID
**               status          - MS sttaus
**               bd_addr         - BD address          
** 
**   
** Returns       UINT8 OBX response code             
**                            
*******************************************************************************/
void bta_mse_mn_send_notif_evt(tBTA_MA_INST_ID mas_instance_id, tBTA_MA_STATUS status,
                               BD_ADDR bd_addr )
{
    tBTA_MSE            param;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_send_notif_evt mas_instance_id=%d status=%d",
                      mas_instance_id,
                      status);
#endif

    param.send_notif.mas_instance_id = mas_instance_id;
    param.send_notif.status = status;
    bdcpy(param.send_notif.bd_addr, bd_addr);
    bta_mse_cb.p_cback(BTA_MSE_SEND_NOTIF_EVT, &param);
}

/*******************************************************************************
**
** Function         bta_mse_mn_clean_send_notif
**
** Description      Clean up send notif resources and cotrol block 
**
** Parameters       ccb_idx    - MN control block index 
** 
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_clean_send_notif(UINT8 ccb_idx)
{
    tBTA_MSE_MN_CB      *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_clean_send_notif");
#endif

    utl_freebuf((void**)&(p_cb->msg_notif.p_buffer));   
    bta_mse_set_mn_oper(ccb_idx, BTA_MSE_MN_OP_NONE);
    p_cb->req_pending = FALSE;
    p_cb->aborting = BTA_MSE_MN_ABORT_NONE;
    memset(&(p_cb->msg_notif), 0, sizeof(tBTA_MSE_MN_MSG_NOTIF));
}

/*******************************************************************************
**
** Function         bta_mse_ma_fl_read_app_params
**
** Description      Read application parameters for the get folder list requst
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_pkt       - Pointer to the obex packet
** 
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_fl_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt)

{          
    tBTA_MSE_MA_SESS_CB *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8               *p_param;
    UINT16              len;

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_MAX_LIST_COUNT, &len);

    if (p_param)
    {
        BE_STREAM_TO_UINT16(p_cb->fl_param.max_list_cnt, p_param);
    }
    else
        p_cb->fl_param.max_list_cnt = BTA_MA_DEFAULT_MAX_LIST_CNT;

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_START_STOFF, &len);

    if (p_param)
    {
        BE_STREAM_TO_UINT16(p_cb->fl_param.start_offset, p_param);
    }
    else
        p_cb->fl_param.start_offset = 0;

} 

/*******************************************************************************
**
** Function         bta_mse_ma_ml_read_app_params
**
** Description      Read application parameters for the get message list requst
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_pkt       - Pointer to the obex packet
** 
** Returns          void
**
*******************************************************************************/

void bta_mse_ma_ml_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt)

{  
    tBTA_MSE_MA_SESS_CB *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8               *p_param;
    UINT16              len;

    memset(&(p_cb->ml_param.filter), 0x00, sizeof(tBTA_MA_MSG_LIST_FILTER_PARAM));

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_MAX_LIST_COUNT, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT16(p_cb->ml_param.filter.max_list_cnt, p_param);
    }
    else
    {
        p_cb->ml_param.filter.max_list_cnt = BTA_MA_DEFAULT_MAX_LIST_CNT;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_START_STOFF, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT16(p_cb->ml_param.filter.list_start_offset, p_param);
    }
    else
    {
        p_cb->ml_param.filter.list_start_offset = 0;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_SUBJ_LEN, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->ml_param.filter.subject_length, p_param);
    }
    else
    {
        p_cb->ml_param.filter.subject_length = 0xff;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_PARAM_MASK, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT32(p_cb->ml_param.filter.parameter_mask, p_param);
    }
    else
    {
        p_cb->ml_param.filter.parameter_mask = 0;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_MSG_TYPE, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->ml_param.filter.msg_mask, p_param);
    }
    else
    {
        p_cb->ml_param.filter.msg_mask = 0;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_PRD_BEGIN, &len);

    if (p_param)
    {
        p_cb->ml_param.filter.period_begin[BTA_MA_LTIME_LEN]='\0';
        if (len < BTA_MA_LTIME_LEN)
        {
            p_cb->ml_param.filter.period_begin[0] = '\0';
        }
        else
        {
            BCM_STRNCPY_S((char *)p_cb->ml_param.filter.period_begin, sizeof(p_cb->ml_param.filter.period_begin),
                    (const char *)p_param, BTA_MA_LTIME_LEN);  
        }
    }
    else
    {
        p_cb->ml_param.filter.period_begin[0] = '\0';
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_PRD_END, &len);
    if (p_param)
    {
        p_cb->ml_param.filter.period_end[BTA_MA_LTIME_LEN] = '\0';
        if (len < BTA_MA_LTIME_LEN)
        {
            p_cb->ml_param.filter.period_end[0] = '\0';
        }
        else
        {
            BCM_STRNCPY_S((char *)p_cb->ml_param.filter.period_end, sizeof(p_cb->ml_param.filter.period_end),
                    (const char *)p_param, BTA_MA_LTIME_LEN); 
        }
    }
    else
    {
        p_cb->ml_param.filter.period_end[0] = '\0';
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_READ_STS, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->ml_param.filter.read_status, p_param);
    }
    else
    {
        p_cb->ml_param.filter.read_status = 0;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_RECEIP, &len);
    p_cb->ml_param.filter.recipient[0] = '\0';
    if (p_param && len)
    {
        if (len >= BTA_MA_MAX_FILTER_TEXT_SIZE)
        {
            p_cb->ml_param.filter.recipient[BTA_MA_MAX_FILTER_TEXT_SIZE] = '\0';
            len = BTA_MA_MAX_FILTER_TEXT_SIZE;
        }
        BCM_STRNCPY_S((char *)p_cb->ml_param.filter.recipient, sizeof(p_cb->ml_param.filter.recipient),
                (const char *)p_param, len);
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_ORIGIN, &len);
    p_cb->ml_param.filter.originator[0] = '\0';
    if (p_param && len)
    {
        if (len >= BTA_MA_MAX_FILTER_TEXT_SIZE)
        {
            p_cb->ml_param.filter.originator[BTA_MA_MAX_FILTER_TEXT_SIZE] = '\0';
            len = BTA_MA_MAX_FILTER_TEXT_SIZE;
        }
        BCM_STRNCPY_S((char *)p_cb->ml_param.filter.originator, sizeof(p_cb->ml_param.filter.originator),
                (const char *)p_param, len);
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FILTER_PRIORITY, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->ml_param.filter.pri_status, p_param);
    }
    else
    {
        p_cb->ml_param.filter.pri_status = 0;
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_msg_read_app_params
**
** Description      Read application parameters for the get message list requst
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_pkt       - Pointer to the obex packet
** 
** Returns          BOOLEAN TRUE - operation is successful
**
*******************************************************************************/

BOOLEAN bta_mse_ma_msg_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt)

{  
    tBTA_MSE_MA_SESS_CB *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8               *p_param;
    UINT16              len;

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_CHARSET, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->msg_param.data.charset, p_param);
    }
    else
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT0("Unable to decode or find charset in application parameter ");
#endif
        return FALSE;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_ATTACH, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->msg_param.data.attachment, p_param);
    }
    else
    {
        p_cb->msg_param.data.attachment = FALSE;
    }

    p_param = bta_mse_read_app_params(p_pkt, BTA_MA_APH_FRAC_REQ, &len);
    if (p_param)
    {
        BE_STREAM_TO_UINT8(p_cb->msg_param.data.fraction_request, p_param);
    }
    else
    {
        p_cb->msg_param.data.fraction_request = BTA_MA_FRAC_REQ_NO;
    }


    return TRUE;
}

/*******************************************************************************
**
** Function         bta_mse_get_msglist_path
**
** Description      Get the path based on received folder name for the get
**                  message list
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_path      - (output) pointer to the folder path
**
** Returns          BOOLEAN TRUE-get path is successful 
*******************************************************************************/
BOOLEAN bta_mse_get_msglist_path(UINT8 inst_idx, UINT8 sess_idx)
{
    tBTA_MSE_MA_SESS_CB *p_cb  = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    char                *p_name = p_cb->ml_param.p_name;
    char                *p_path = p_cb->ml_param.p_path;
    char                *p_workdir = p_cb->p_workdir;
    BOOLEAN             status = TRUE;
#if BTA_MSE_ENABLE_FS_CO == TRUE
    BOOLEAN             is_dir;
#endif


    if (*p_name == '\0')
    {
        BCM_STRNCPY_S(p_path, p_bta_fs_cfg->max_path_len+1, p_workdir, p_bta_fs_cfg->max_path_len);
    }
    /* Make sure the new path is not too big */
    /* +1 is for the separator */
    else if ((strlen(p_workdir)+1+strlen(p_name)) <= p_bta_fs_cfg->max_path_len)
    {
        /* create a temporary path for creation attempt */
        sprintf(p_path, "%s%c%s", p_workdir,
                p_bta_fs_cfg->path_separator, p_name);
#if BTA_MSE_ENABLE_FS_CO == TRUE
        if (((bta_fs_co_access(p_path, BTA_FS_ACC_EXIST,
                               &is_dir, bta_mse_cb.app_id)) != BTA_FS_CO_OK) || !is_dir)
        {
            status = FALSE;
        }
#endif
    }
    else
    {
        status = FALSE;
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_get_msglist_path status=%d pth=%s",status, p_path );
#endif

    return status;
}

/*******************************************************************************
**
** Function      bta_mse_find_bd_addr_match_pm_cb_index  
**
** Description   Finds the PM control block index 
**               based on the specified BD address
**
** Parameters     app_id     - app_id 
**                p_bd_addr  - BD address
**                p_idx      - (output) pointer to the MN control 
**                              block index  
** 
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found         
**
*******************************************************************************/
BOOLEAN bta_mse_find_pm_cb_index(BD_ADDR p_bd_addr, UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_MN ; i ++)
    {
        if ((bta_mse_cb.pcb[i].in_use) &&
            (!memcmp (bta_mse_cb.pcb[i].bd_addr, p_bd_addr, BD_ADDR_LEN)))
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    if (!found) APPL_TRACE_DEBUG2("dbg bta_mse_find_pm_cb_index found=%d index=%d", found, i);
#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_mse_find_avail_pm_cb_idx  
**
** Description   Finds a not in use PM control block index
** 
** Parameters    p_idx    - (output) pointer to the PM control block index
**  
** Returns      BOOLEAN - TRUE found        
**                        FALSE not found       
**                            
*******************************************************************************/
BOOLEAN bta_mse_find_avail_pm_cb_idx(UINT8 *p_idx)
{
    BOOLEAN found=FALSE;
    UINT8 i;

    for (i=0; i < BTA_MSE_NUM_MN ; i ++)
    {
        if (!bta_mse_cb.pcb[i].in_use)
        {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_MSE_DEBUG == TRUE
    if (!found) APPL_TRACE_DEBUG2("bta_mse_find_avail_pm_cb_idx found=%d i=%d", found, i);
#endif

    return found;
}

/*******************************************************************************
**
** Function     bta_mse_pm_conn_open
**
** Description   Determine whether or not bta_sys_conn_open should be called
** 
** Parameters    bd_addr - peer BD address
**  
** Returns      None   
**                            
*******************************************************************************/
void bta_mse_pm_conn_open(BD_ADDR bd_addr)
{
    tBTA_MSE_PM_CB *p_pcb;
    UINT8  idx;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_DEBUG0("bta_mse_pm_conn_open");
#endif
    if (!bta_mse_find_pm_cb_index(bd_addr, &idx))
    {
        if (bta_mse_find_avail_pm_cb_idx(&idx))
        {
            p_pcb = BTA_MSE_GET_PM_CB_PTR(idx);

            p_pcb->in_use    = TRUE;
            p_pcb->opened    = TRUE;
            bdcpy(p_pcb->bd_addr, bd_addr);
            bta_sys_conn_open(BTA_ID_MSE , bta_mse_cb.app_id, bd_addr);
        }
    }
}

/*******************************************************************************
**
** Function      bta_mse_pm_conn_close
**
** Description   Determine whether or not bta_sys_conn_close should be called 
** 
** Parameters    bd_addr - peer BD address
**  
** Returns      None  
*******************************************************************************/
void bta_mse_pm_conn_close(BD_ADDR bd_addr)
{
    tBTA_MSE_PM_CB *p_pcb;
    UINT8  i, pm_idx, sess_idx, mn_idx;
    BOOLEAN found_bd_addr=FALSE;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_DEBUG0("bta_mse_pm_conn_close");
#endif
    if (bta_mse_find_pm_cb_index(bd_addr, &pm_idx))
    {
        p_pcb = BTA_MSE_GET_PM_CB_PTR(pm_idx);
        if (p_pcb->opened)
        {
            for (i=0; i<BTA_MSE_NUM_INST; i++)
            {
                if (bta_mse_find_bd_addr_match_sess_cb_idx(bd_addr, i, &sess_idx))
                {
                    found_bd_addr = TRUE;
                    break;
                }
            }

            if (!found_bd_addr)
            {
                if ( bta_mse_find_bd_addr_match_mn_cb_index(bd_addr, &mn_idx))
                {
                    found_bd_addr = TRUE;

                }
            }

            if (!found_bd_addr)
            {
                memset(p_pcb, 0, sizeof(tBTA_MSE_PM_CB));
                bta_sys_conn_close(BTA_ID_MSE ,  bta_mse_cb.app_id, bd_addr);
            }
        }
    }
}

/*******************************************************************************
**
** Function       bta_mse_set_ma_oper
**
** Description   Set MA operation and power management's busy/idle status based on
**               MA operation
** 
** Parameters    inst_idx    - Index to the MA instance control block
**               sess_idx    - Index to the MA session control block
**               oper        - MA operation
**                 
** Returns      None  
*******************************************************************************/
void bta_mse_set_ma_oper(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_OPER oper)
{
    tBTA_MSE_MA_SESS_CB         *p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_MA_SESS_CB         *p_scb;
    tBTA_MSE_MN_CB              *p_mcb;
    tBTA_MSE_PM_CB              *p_pcb;
    UINT8                       i, j, pm_idx, mn_idx;
    BOOLEAN                     still_busy = FALSE;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_DEBUG2("bta_mse_set_ma_oper old=%d new=%d", p_cb->oper, oper);
#endif

    if (p_cb->oper != oper)
    {
        p_cb->oper = oper;
        if (bta_mse_find_pm_cb_index(p_cb->bd_addr, &pm_idx))
        {
            p_pcb = BTA_MSE_GET_PM_CB_PTR(pm_idx);

            if (oper != BTA_MSE_OPER_NONE )
            {
                if (!p_pcb->busy)
                {
                    p_pcb->busy = TRUE;
                    bta_sys_busy(BTA_ID_MSE, bta_mse_cb.app_id, p_cb->bd_addr);
                }
            }
            else
            {
                if (p_pcb->busy)
                {
                    for (i=0; i<BTA_MSE_NUM_INST; i++)
                    {
                        if (bta_mse_find_bd_addr_match_sess_cb_idx( p_cb->bd_addr, i, &j) &&
                            (i != inst_idx) && (j != sess_idx))
                        {
                            p_scb = BTA_MSE_GET_SESS_CB_PTR(i, j);
                            if (p_scb->oper != BTA_MSE_OPER_NONE )
                            {
                                still_busy = TRUE;
                                break;
                            }
                        }
                    }

                    if ((!still_busy) &&
                        bta_mse_find_bd_addr_match_mn_cb_index( p_cb->bd_addr, &mn_idx))
                    {
                        p_mcb = BTA_MSE_GET_MN_CB_PTR(mn_idx);
                        if (p_mcb->obx_oper != BTA_MSE_MN_OP_NONE )
                        {
                            still_busy = TRUE;
                        }
                    }

                    if (!still_busy)
                    {
                        p_pcb->busy = FALSE;
                        bta_sys_idle(BTA_ID_MSE, bta_mse_cb.app_id, p_cb->bd_addr);
                    }
                }
            }
        }
    }
}


/*******************************************************************************
**
** Function      bta_mse_set_mn_oper
**
** Description   Set MN operation and power management's busy/idle status based on
**               MN operation
** 
** Parameters    ccb_idx      - MN control block index
**               oper         - MN operation
**                 
** Returns      None 
*******************************************************************************/
void bta_mse_set_mn_oper(UINT8 ccb_idx, UINT8 oper)
{
    tBTA_MSE_MN_CB              *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_MA_SESS_CB         *p_scb;
    tBTA_MSE_MN_CB              *p_mcb;
    tBTA_MSE_PM_CB              *p_pcb;
    UINT8                       i, j, pm_idx, mn_idx;
    BOOLEAN                     still_busy = FALSE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_DEBUG2("dbg bta_mse_set_mn_oper old=%d new=%d", p_cb->obx_oper, oper);
#endif
    if (p_cb->obx_oper != oper)
    {
        p_cb->obx_oper = oper;
        if (bta_mse_find_pm_cb_index(p_cb->bd_addr, &pm_idx))
        {
            p_pcb = BTA_MSE_GET_PM_CB_PTR(pm_idx);

            if (oper != BTA_MSE_MN_OP_NONE )
            {
                if (!p_pcb->busy)
                {
                    p_pcb->busy = TRUE;
                    bta_sys_busy(BTA_ID_MSE, bta_mse_cb.app_id, p_cb->bd_addr);
                }
            }
            else
            {
                if (p_pcb->busy)
                {
                    for (i=0; i<BTA_MSE_NUM_INST; i++)
                    {
                        if (bta_mse_find_bd_addr_match_sess_cb_idx( p_cb->bd_addr, i, &j))
                        {
                            p_scb = BTA_MSE_GET_SESS_CB_PTR(i, j);
                            if (p_scb->oper != BTA_MSE_OPER_NONE )
                            {
                                still_busy = TRUE;
                                break;
                            }
                        }
                    }

                    if ((!still_busy) &&
                        bta_mse_find_bd_addr_match_mn_cb_index(p_cb->bd_addr, &mn_idx) && 
                        (mn_idx != ccb_idx))
                    {
                        p_mcb = BTA_MSE_GET_MN_CB_PTR(mn_idx);
                        if (p_mcb->obx_oper != BTA_MSE_MN_OP_NONE )
                        {
                            still_busy = TRUE;
                        }
                    }

                    if (!still_busy)
                    {
                        p_pcb->busy = FALSE;
                        bta_sys_idle(BTA_ID_MSE, bta_mse_cb.app_id, p_cb->bd_addr);
                    }
                }
            }
        }
    }

}

#endif /* BTA_MSE_INCLUDED */
