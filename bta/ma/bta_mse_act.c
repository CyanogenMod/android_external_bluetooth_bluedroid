/*****************************************************************************
**
**  Name:           bta_mse_act.c
**
**  Description:    This file contains the message access server action 
**                  functions for the state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "sdp_api.h"
#include "bta_sys.h"
#include "port_api.h"
#include "obx_api.h"
#include "sdp_api.h"
#include "bta_fs_api.h"
#include "bta_mse_api.h"
#include "bta_mse_int.h"
#include "bta_fs_co.h"
#include "utl.h"
#include "bd.h"
#include "bta_ma_def.h"


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
void bta_mse_req_app_access(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
static void bta_mse_mn_sdp_cback0(UINT16 status);
static void bta_mse_mn_sdp_cback1(UINT16 status);
static void bta_mse_mn_sdp_cback2(UINT16 status);
static void bta_mse_mn_sdp_cback3(UINT16 status);
static void bta_mse_mn_sdp_cback4(UINT16 status);
static void bta_mse_mn_sdp_cback5(UINT16 status);
static void bta_mse_mn_sdp_cback6(UINT16 status);

static tSDP_DISC_CMPL_CB * const bta_mse_mn_sdp_cback_arr[] = {
    bta_mse_mn_sdp_cback0,
    bta_mse_mn_sdp_cback1,
    bta_mse_mn_sdp_cback2,
    bta_mse_mn_sdp_cback3,
    bta_mse_mn_sdp_cback4,
    bta_mse_mn_sdp_cback5,
    bta_mse_mn_sdp_cback6
};

#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
static char *bta_mse_obx_evt_code(UINT16 evt_code);
#endif


/*******************************************************************************
** Message Access Server (MAS) Action functions
**
*******************************************************************************/

/*******************************************************************************
**
** Function         bta_mse_ma_int_close
**
** Description      Porcesses the Internal MAS session close event                 
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_int_close(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    BD_ADDR                 bd_addr;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_int_close inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif

    if (OBX_GetPeerAddr(p_cb->obx_handle, bd_addr) != 0)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT1("Send Obx Discon rsp obx session id=%d",
                          p_cb->obx_handle); 
#endif
        /* resources will be freed at BTA_MSE_MA_OBX_CLOSE_EVT */
        OBX_DisconnectRsp(p_cb->obx_handle, OBX_RSP_SERVICE_UNAVL, NULL);
    }
    else
    {
        /* OBX close already */
        bta_mse_ma_sm_execute(inst_idx, sess_idx, BTA_MSE_MA_OBX_CLOSE_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_api_upd_ibx_rsp
**
** Description      Processes the API update inbox response event.
**                  If permission had been granted, continue the operation,
**                  otherwise stop the operation.
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_api_upd_ibx_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_UNAUTHORIZED;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_api_upd_ibx_rsp inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif

    /* Process the currently active access response */
    if (p_cb->oper == BTA_MSE_OPER_UPDATE_INBOX)
    {
        bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);
        if (p_data->api_upd_ibx_rsp.rsp == BTA_MSE_UPDATE_INBOX_ALLOW)
        {
            bta_mse_co_update_inbox(p_cb->obx_handle, bta_mse_cb.app_id);
            rsp_code = OBX_RSP_OK;
        }
        OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }
    else
    {
        APPL_TRACE_WARNING1("MSE UPDIBXRSP: Unknown tBTA_MSE_OPER value (%d)",
                            p_cb->oper);
    }
}


/*******************************************************************************
**
** Function         bta_mse_ma_api_set_notif_reg_rsp
**
** Description      Processes the API set notification registration response event.
**                  If permission had been granted, continue the operation,
**                  otherwise stop the operation.
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_api_set_notif_reg_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb    = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_SERVICE_UNAVL;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_api_set_notif_reg_rsp inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif

    /* Process the currently active access response */
    if ( p_cb->oper == BTA_MSE_OPER_NOTIF_REG)
    {
        bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);
        if (p_data->api_set_notif_reg_rsp.rsp == BTA_MSE_SET_NOTIF_REG_ALLOW)
        {
            bta_mse_proc_notif_reg_status(p_cb->notif_reg_req.notif_status,
                                          inst_idx,  sess_idx);
        }
        else
        {
            OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        }
    }
    else
    {
        APPL_TRACE_WARNING1("MSE SETNOTIFREGRSP: Unknown tBTA_MSE_OPER value (%d)",
                            p_cb->oper);
    }
}


/*******************************************************************************
**
** Function         bta_mse_ma_api_accessrsp
**
** Description      Processes the API access response event.
**                  If permission had been granted, continue the operation,
**                  otherwise stop the operation.
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**
** Returns          void
**
*******************************************************************************/

void bta_mse_ma_api_accessrsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_OK;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_ma_api_accessrsp inst idx=%d sess idx=%d access_rsp=%d",
                      inst_idx, sess_idx,
                      p_data->api_access_rsp.rsp);
#endif

    if (p_cb->oper != p_data->api_access_rsp.oper )
    {
        APPL_TRACE_WARNING2("MSE MA ACCRSP: not match active:%d, rsp:%d",
                            p_cb->oper, p_data->api_access_rsp.oper);
        return;
    }

    /* Process the currently active access response */
    switch (p_cb->oper)
    {
        case BTA_MSE_OPER_SETPATH:

            if (p_data->api_access_rsp.rsp == BTA_MA_ACCESS_TYPE_ALLOW)
            {
                bta_mse_co_set_folder( p_cb->obx_handle, p_cb->sp.p_path, bta_mse_cb.app_id);
                /* updat the working dir */
                BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->sp.p_path, p_bta_fs_cfg->max_path_len);
            }
            else
            {
                rsp_code = OBX_RSP_UNAUTHORIZED;
            }

            utl_freebuf((void**)&(p_cb->sp.p_path));
            utl_freebuf((void**)&(p_cb->sp.p_name));
            OBX_SetPathRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NONE);
            break;

        case BTA_MSE_OPER_GET_MSG_LIST:

            if (p_data->api_access_rsp.rsp == BTA_MA_ACCESS_TYPE_ALLOW)
            {
                bta_mse_getmsglist(inst_idx,sess_idx, TRUE); 
            }
            else
            {
                OBX_GetRsp(p_cb->obx_handle, OBX_RSP_UNAUTHORIZED, (BT_HDR *)NULL);
                bta_mse_clean_msg_list(inst_idx,sess_idx);
            }
            break;

        case BTA_MSE_OPER_GET_MSG:

            if (p_data->api_access_rsp.rsp == BTA_MA_ACCESS_TYPE_ALLOW)
            {
                bta_mse_getmsg(inst_idx,sess_idx, TRUE);
            }
            else
            {
                OBX_GetRsp(p_cb->obx_handle, OBX_RSP_UNAUTHORIZED, (BT_HDR *)NULL);
                bta_mse_clean_msg(inst_idx,sess_idx);
            }
            break;

        case BTA_MSE_OPER_PUSH_MSG:

            if (p_data->api_access_rsp.rsp == BTA_MA_ACCESS_TYPE_ALLOW)
            {
                bta_mse_pushmsg(inst_idx, sess_idx, TRUE);
            }
            else
            {
                OBX_PutRsp(p_cb->obx_handle, OBX_RSP_UNAUTHORIZED, (BT_HDR *)NULL);
                bta_mse_clean_push_msg(inst_idx,sess_idx);
            }
            break;

        case BTA_MSE_OPER_DEL_MSG:

            if (p_data->api_access_rsp.rsp == BTA_MA_ACCESS_TYPE_ALLOW)
            {
                p_cb->cout_active = TRUE;
                bta_mse_co_set_msg_delete_status((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                                 p_cb->set_msg_sts.handle,
                                                 p_cb->set_msg_sts.sts_val,
                                                 BTA_MSE_CI_DEL_MSG_EVT,
                                                 bta_mse_cb.app_id);
            }
            else
            {
                OBX_PutRsp(p_cb->obx_handle, OBX_RSP_UNAUTHORIZED, (BT_HDR *)NULL);
                bta_mse_clean_set_msg_sts(inst_idx,sess_idx);
            }
            break;

        default:
            APPL_TRACE_WARNING1("MSE ACCRSP: Unknown tBTA_MSE_OPER value (%d)",
                                p_cb->oper);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_ci_get_folder_entry
**
** Description      Proceses the get folder entry call-in event
** 
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                 
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_get_folder_entry(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_PART_CONTENT;
    BOOLEAN                 free_pkt = TRUE;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_get_folder_entry inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
        return;
    }

    /* Process dirent listing call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_GET_FOLDER_LIST)
    {
        switch (p_data->ci_get_fentry.status)
        {
            case BTA_MA_STATUS_OK:     /* Valid new entry */
                free_pkt = FALSE;
                rsp_code = bta_mse_add_list_entry(inst_idx, sess_idx);
                break;

            case BTA_MA_STATUS_EODIR:  /* End of list (entry not valid) */
                free_pkt = FALSE;
                rsp_code = OBX_RSP_OK;
                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                rsp_code = OBX_RSP_NOT_FOUND;
                break;

            default:
                APPL_TRACE_ERROR1("bta_mse_ma_ci_get_folder_entry Unknown status=%d ",
                                  p_data->ci_get_fentry.status );
                rsp_code = OBX_RSP_NOT_FOUND;
                break;
        }
    }

    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_list(inst_idx, sess_idx,rsp_code);

    if (free_pkt)
        utl_freebuf((void **)&p_cb->obx.p_pkt);
}


/*******************************************************************************
**
** Function         bta_mse_ma_ci_get_ml_info
**
** Description      Proceses the get message list info call-in event
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_get_ml_info(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_PART_CONTENT;
    BOOLEAN                 free_pkt = TRUE;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_get_ml_info inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
        return;
    }

    /* Process dirent listing call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_GET_MSG_LIST)
    {
        switch (p_data->ci_get_ml_info.status)
        {
            case BTA_MA_STATUS_OK:     /* Valid new entry */
                free_pkt = FALSE;
                rsp_code = bta_mse_add_msg_list_info(inst_idx, sess_idx);
                break;

            case BTA_MA_STATUS_EODIR:  /* End of list (entry not valid) */
                free_pkt = FALSE;
                rsp_code = OBX_RSP_OK;
                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                rsp_code = OBX_RSP_NOT_FOUND;
                break;

            default:
                APPL_TRACE_ERROR1("bta_mse_ma_ci_get_ml_info Unknown status=%d ",
                                  p_data->ci_get_ml_info.status );
                rsp_code = OBX_RSP_NOT_FOUND;
                break;
        }
    }

    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_msg_list(inst_idx, sess_idx,rsp_code);

    if (free_pkt)
        utl_freebuf((void **)&p_cb->obx.p_pkt);
}

/*******************************************************************************
**
** Function         bta_mse_ma_ci_get_msg_entry
**
** Description      Proceses the get message entry call-in event
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_get_ml_entry(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                   rsp_code = OBX_RSP_PART_CONTENT;
    BOOLEAN                 free_pkt = TRUE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_get_msg_entry inst idx=%d sess idx=%d",inst_idx, sess_idx);
    APPL_TRACE_EVENT1(" status=%d",p_data->ci_get_ml_entry.status);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
        return;
    }

    /* Process dirent listing call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_GET_MSG_LIST)
    {
        switch (p_data->ci_get_ml_entry.status)
        {
            case BTA_MA_STATUS_OK:     /* Valid new entry */
                free_pkt = FALSE;
                rsp_code = bta_mse_add_msg_list_entry(inst_idx, sess_idx);
                break;

            case BTA_MA_STATUS_EODIR:  /* End of list (entry not valid) */
                free_pkt = FALSE;
                rsp_code = OBX_RSP_OK;
                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                rsp_code = OBX_RSP_NOT_FOUND;
                break;

            default:
                APPL_TRACE_ERROR1("bta_mse_ma_ci_get_ml_entry Unknown status=%d ",
                                  p_data->ci_get_ml_entry.status);
                rsp_code = OBX_RSP_NOT_FOUND;
                break;
        }
    }

    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_mse_end_of_msg_list(inst_idx, sess_idx,rsp_code);

    if (free_pkt)
        utl_freebuf((void **)&p_cb->obx.p_pkt);
}


/*******************************************************************************
**
** Function         bta_mse_ma_ci_get_msg
**
** Description      Proceses the get message call-in event
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_get_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OPER_MSG_PARAM *p_param = &p_cb->msg_param;
    tBTA_MSE_CI_GET_MSG     *p_ci_get_msg = &p_data->ci_get_msg;
    UINT8                   rsp_code = OBX_RSP_NOT_FOUND; 
    BOOLEAN                 free_pkt = TRUE, end_of_msg= TRUE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_get_msg inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
    APPL_TRACE_EVENT1("status=%d",p_data->ci_get_ml_entry.status);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
        return;
    }

    /* Process get msg call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_GET_MSG)
    {
        switch (p_data->ci_get_msg.status)
        {
            case BTA_MA_STATUS_OK:     /* Valid new entry */
                free_pkt = FALSE;
                p_param->frac_deliver_status = p_ci_get_msg->frac_deliver_status;
                p_param->filled_buff_size = p_ci_get_msg->filled_buff_size;

                if ( p_ci_get_msg->multi_pkt_status == BTA_MA_MPKT_STATUS_MORE)
                {
                    p_param->byte_get_cnt += p_param->filled_buff_size;
                    rsp_code = OBX_RSP_CONTINUE;
                }
                else
                    rsp_code = OBX_RSP_OK;

                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                break;

            default:
                end_of_msg = FALSE;
                break;
        }
    }

    if (end_of_msg)
        bta_mse_end_of_msg(inst_idx, sess_idx,rsp_code);

    if (free_pkt)
        utl_freebuf((void **)&p_cb->obx.p_pkt);
}


/*******************************************************************************
**
** Function         bta_mse_ma_ci_push_msg
**
** Description      Proceses the push message call-in event
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_push_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_PKT        *p_obx  = &p_cb->obx;
    UINT8                   rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    BOOLEAN                 free_pkt = TRUE;
    char                    handle_buf[BTA_MSE_64BIT_HEX_STR_SIZE];
    tBTA_MA_STREAM      strm;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_push_msg inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
    APPL_TRACE_EVENT4(" status=%d ci_last_pkt=%d obx_final=%d oper=%d",
                      p_data->ci_push_msg.status, 
                      p_data->ci_push_msg.last_packet,
                      p_obx->final_pkt,
                      p_cb->oper);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
        return;
    }

    /* Process get msg call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_PUSH_MSG)
    {
        switch (p_data->ci_push_msg.status)
        {
            case BTA_MA_STATUS_OK:     /* Valid new entry */
                if (p_obx->final_pkt)
                {
                    APPL_TRACE_EVENT2("final pkt: status=%d oper=%d",
                                      p_data->ci_push_msg.status, p_cb->oper);

                    utl_freebuf((void**)&p_obx->p_pkt);

                    rsp_code = OBX_RSP_OK;

                    p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                                                         /* p_cb->peer_mtu */ HCI_CMD_POOL_BUF_SIZE);
                    if (p_obx->p_pkt)
                    {
                        memset(handle_buf, 0,BTA_MSE_64BIT_HEX_STR_SIZE);
                        BTA_MaInitMemStream(&strm,(UINT8 *) handle_buf, BTA_MSE_64BIT_HEX_STR_SIZE);
                        bta_ma_stream_handle(&strm, p_data->ci_push_msg.handle);
                        if (OBX_AddUtf8NameHdr(p_obx->p_pkt, (UINT8 *)handle_buf))
                        {
                            rsp_code = OBX_RSP_OK;
                            free_pkt = FALSE;
                        }
                        else
                        {
                            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                        }
                    }
                    else
                    {
                        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                    }
                }
                else
                    rsp_code = OBX_RSP_CONTINUE;
                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                rsp_code = OBX_RSP_UNAUTHORIZED;
                break;
            default:
                break;
        }
    }

    if (rsp_code==OBX_RSP_OK)
    {
        OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt); 
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
    }
    else
    {
        OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
    }


    if (rsp_code == OBX_RSP_CONTINUE)
    {
        bta_mse_send_push_msg_in_prog_evt(inst_idx,sess_idx);
    }
    else
    {
        if ((rsp_code != OBX_RSP_OK) || 
            ((p_obx->final_pkt) &&(rsp_code == OBX_RSP_OK)))
        {
            bta_mse_send_oper_cmpl_evt(inst_idx,sess_idx, p_data->ci_push_msg.status);
            bta_mse_clean_push_msg(inst_idx,sess_idx);
        }
    }

    if (free_pkt) utl_freebuf((void**)&p_obx->p_pkt);

}

/*******************************************************************************
**
** Function         bta_mse_ma_ci_del_msg
**
** Description      Proceses the delete message call-in event
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_ci_del_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    UINT8                       rsp_code = OBX_RSP_OK;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_ci_del_msg inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
    APPL_TRACE_EVENT2(" status=%d oper=%d",p_data->ci_del_msg.status, p_cb->oper);
#endif

    p_cb->cout_active = FALSE;

    if (p_cb->aborting)
    {
        /* too late to abort now*/
        bta_mse_abort_too_late(inst_idx,sess_idx);
    }

    /* Process get msg call-in event if operation is still active */
    if (p_cb->oper == BTA_MSE_OPER_DEL_MSG)
    {
        switch (p_data->ci_del_msg.status)
        {
            case BTA_MA_STATUS_OK:  
                rsp_code = OBX_RSP_OK;  
                break;

            case BTA_MA_STATUS_FAIL:   /* Error occurred */
                rsp_code = OBX_RSP_FORBIDDEN;
                break;
            default:
                break;
        }
    }

    OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL); 
    bta_mse_clean_set_msg_sts(inst_idx,sess_idx);

}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_connect
**
** Description      Proceses the obx connect request to open a MAS session
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_connect(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_CB          *p_scb  = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_EVT        *p_evt = &p_data->obx_evt;
    tBTA_MSE                cback_evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_connect inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif

    p_cb->peer_mtu = p_evt->param.conn.mtu;
    p_cb->obx_handle = p_evt->handle;
    memcpy(p_cb->bd_addr, p_evt->param.conn.peer_addr, BD_ADDR_LEN);

    OBX_ConnectRsp(p_evt->handle, OBX_RSP_OK, (BT_HDR *)NULL);

    /* Reset to the root directory */
    BCM_STRNCPY_S(p_cb->p_workdir, p_bta_fs_cfg->max_path_len+1, p_scb->p_rootpath, p_bta_fs_cfg->max_path_len);

    /* inform role manager */
    bta_mse_pm_conn_open(p_cb->bd_addr);

    bdcpy(cback_evt_data.ma_open.bd_addr, p_cb->bd_addr); 
    BCM_STRNCPY_S((char *)cback_evt_data.ma_open.dev_name, sizeof(cback_evt_data.ma_open.dev_name),
            "", BTM_MAX_REM_BD_NAME_LEN);
    cback_evt_data.ma_open.mas_instance_id = p_scb->mas_inst_id;
    cback_evt_data.ma_open.mas_session_id = p_cb->obx_handle;

    bta_mse_cb.p_cback(BTA_MSE_MA_OPEN_EVT, (tBTA_MSE *) &cback_evt_data);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_disc
**
** Description      Proceses the obx disconnect request to close a MAS session
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_disc(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_OBX_EVT        *p_evt = &p_data->obx_evt;
    UINT8                   rsp_code;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_disc inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif
    rsp_code = (p_evt->obx_event == OBX_DISCONNECT_REQ_EVT) ? OBX_RSP_OK
               : OBX_RSP_BAD_REQUEST;
    OBX_DisconnectRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_close
**
** Description      Proceses the obx close indication to close a MAS connection
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_close(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_close inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif
    /* finished if not waiting on a call-in function */
    if (!p_cb->cout_active)
        bta_mse_ma_sm_execute(inst_idx, sess_idx, BTA_MSE_CLOSE_CMPL_EVT, p_data);

}

/*******************************************************************************
**
** Function         bta_mse_ma_obx_abort
**
** Description      Proceses the obx abort request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_abort(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_EVT        *p_evt = &p_data->obx_evt;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT3("bta_mse_ma_obx_abort inst idx=%d sess idx=%d oper=%d",
                      inst_idx, sess_idx, p_cb->oper);
#endif

    utl_freebuf((void**)&p_evt->p_pkt);

    switch (p_cb->oper)
    {
        case BTA_MSE_OPER_SETPATH:
        case BTA_MSE_OPER_NONE:
            OBX_AbortRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
            return;
        default:
            break;

    }

    if (!p_cb->cout_active)
    {
        bta_mse_clean_getput(inst_idx,sess_idx, BTA_MA_STATUS_ABORTED);
    }
    else    /* Delay the response if a call-out function is active */
        p_cb->aborting = TRUE;

}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_put
**
** Description      Proceses the obx put request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_put(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_EVT            *p_evt  = &p_data->obx_evt;
    tBTA_MSE_OBX_PKT            *p_obx  = &p_cb->obx;
    tBTA_MSE_OPER_PUSH_MSG      *p_push_msg = &p_cb->push_msg;
    tBTA_MSE_OPER_SET_MSG_STS   *p_set_msg_sts = &p_cb->set_msg_sts;
    tBTA_MA_SESS_HANDLE         mas_session_id;
    UINT16                      len;
    UINT8                       *p_type;
    UINT8                       *p_param;
    UINT8                       rsp_code = OBX_RSP_OK;
    UINT8                       num_hdrs;
    BOOLEAN                     free_pkt = TRUE;
    BOOLEAN                     send_rsp = TRUE;
    BOOLEAN                     send_ok_rsp = FALSE;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_put inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif

    if (OBX_ReadTypeHdr(p_evt->p_pkt, &p_type, &len))
    {
        if (!memcmp(p_type, BTA_MA_HDR_TYPE_NOTIF_REG, len))
        {
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_NOTIF_REG);
        }
        else if (!memcmp(p_type, BTA_MA_HDR_TYPE_MSG, len))
        {
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_PUSH_MSG);
            bta_mse_init_push_msg(inst_idx, sess_idx, &rsp_code);
            if (rsp_code != OBX_RSP_OK)
            {
                OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
                bta_mse_clean_push_msg(inst_idx, sess_idx);
                return;
            }
        }
        else if (!memcmp(p_type, BTA_MA_HDR_TYPE_MSG_UPDATE, len))
        {
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_UPDATE_INBOX);
        }
        else if (!memcmp(p_type, BTA_MA_HDR_TYPE_MSG_STATUS, len))
        {
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_SET_MSG_STATUS);
            bta_mse_init_set_msg_sts(inst_idx, sess_idx, &rsp_code);
            if (rsp_code != OBX_RSP_OK)
            {
                OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
                bta_mse_clean_set_msg_sts(inst_idx, sess_idx);
                return;
            }
        }
    }

    switch ( p_cb->oper)
    {
        case BTA_MSE_OPER_NOTIF_REG:

            p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_NOTIF_STATUS, &len);
            if (p_param)
            {
                p_cb->notif_reg_req.notif_status_rcv = TRUE;
                BE_STREAM_TO_UINT8(p_cb->notif_reg_req.notif_status, p_param);
            }

            if (p_evt->param.put.final)
            {
                /* check end of body is included or not */ 

                if ((num_hdrs = OBX_ReadBodyHdr(p_evt->p_pkt, &p_obx->p_start, &p_obx->offset, &p_obx->final_pkt)) && 
                    p_obx->final_pkt)
                {
                    if (p_cb->notif_reg_req.notif_status_rcv)
                    {
                        if (!bta_mse_send_set_notif_reg(p_cb->notif_reg_req.notif_status,
                                                        inst_idx, sess_idx))
                        {
                            OBX_PutRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                        }
                        else
                        {
                            /* wait for the set notif reg response from the application*/
                            break;
                        }
                    }
                    else
                    {
                        OBX_PutRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                    }
                }
                else
                {
                    OBX_PutRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                }
                /* if reach here this is a bad request case so clean up*/
                bta_mse_clean_set_notif_reg(inst_idx,sess_idx);
            }
            else
            {
                OBX_PutRsp(p_evt->handle, OBX_RSP_CONTINUE, (BT_HDR *)NULL);
            }

            break;
        case BTA_MSE_OPER_PUSH_MSG:

            if ((!p_push_msg->rcv_folder_name) && 
                (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_push_msg->param.p_folder, 
                                     p_bta_mse_cfg->max_name_len)))
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT2("Rcv folder len=%d, name=%s ",
                                  strlen( p_push_msg->param.p_folder),
                                  p_push_msg->param.p_folder);  
#endif
                p_push_msg->rcv_folder_name = TRUE;
            }

            if (!p_push_msg->rcv_charset)
            {
                p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_CHARSET, &len);
                if (p_param)
                {

                    p_push_msg->rcv_charset = TRUE;
                    BE_STREAM_TO_UINT8(p_push_msg->param.charset, p_param);
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT1("Rcv Charset=%d(0-native 1-UTF8)",p_push_msg->param.charset);
#endif
                }
            }

            if (!p_push_msg->rcv_transparent)
            {
                p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_TRANSPARENT, &len);
                if (p_param)
                {
                    p_push_msg->rcv_transparent = TRUE;
                    BE_STREAM_TO_UINT8(p_push_msg->param.transparent, p_param);
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT1("Rcv Transparent=%d",p_push_msg->param.transparent);
#endif
                }
            }

            if (!p_push_msg->rcv_retry)
            {
                p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_RETRY, &len);
                if (p_param)
                {
                    p_push_msg->rcv_retry = TRUE;
                    BE_STREAM_TO_UINT8(p_push_msg->param.retry, p_param);
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT1("Rcv Retry=%d",p_push_msg->param.retry);
#endif
                }
            }

            /* Read the body header from the obx packet */
            num_hdrs = OBX_ReadBodyHdr(p_evt->p_pkt, &p_obx->p_start, &p_obx->offset,
                                       &p_obx->final_pkt);
            /* process body or end of body header */
            if (num_hdrs >= 1 )
            {

#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT3("Rcv Body or EndOfBody body_hdr=%d len=%d final=%d",
                                  num_hdrs,
                                  p_obx->offset,
                                  p_obx->final_pkt);
#endif

                if (p_push_msg->rcv_charset && p_push_msg->rcv_folder_name)
                {
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT0("Rcv all Required params");
#endif

                    free_pkt = FALSE;
                    p_obx->p_pkt = p_evt->p_pkt;
                    if (p_push_msg->first_req_pkt)
                    {
                        p_push_msg->first_req_pkt = FALSE;
                        bta_mse_req_app_access( inst_idx,  sess_idx,  p_data);
                    }
                    else
                    {
                        bta_mse_pushmsg(inst_idx, sess_idx, FALSE);
                    }
                    send_rsp = FALSE;
                }
                else
                {
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_ERROR0("Not all Mandatory params are rcv before body/EndofBody: bad request");
#endif
                    rsp_code = OBX_RSP_BAD_REQUEST;
                }
            }
            else
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT0("No Body or End of Body in this push msg pkt");
#endif
            }

            if (!p_obx->final_pkt)
            {
                if (rsp_code != OBX_RSP_BAD_REQUEST)
                {
#if BTA_MSE_DEBUG == TRUE
                    APPL_TRACE_EVENT0("Not a final pkt and no error set rsp_code=Continue");
#endif
                    rsp_code = OBX_RSP_CONTINUE;
                }
            }

            if (send_rsp) OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
            if (rsp_code == OBX_RSP_BAD_REQUEST )
            {
                if (!p_push_msg->first_req_pkt)
                    bta_mse_send_oper_cmpl_evt(inst_idx,sess_idx, BTA_MA_STATUS_FAIL);
                bta_mse_clean_push_msg(inst_idx,sess_idx);
            }
            break;
        case BTA_MSE_OPER_UPDATE_INBOX:

            if (p_evt->param.put.final)
            {
                /* check enod of body is included or not */ 

                if ((num_hdrs = OBX_ReadBodyHdr(p_evt->p_pkt, &p_obx->p_start, &p_obx->offset, &p_obx->final_pkt)) && 
                    p_obx->final_pkt)
                {

                    mas_session_id  = (tBTA_MA_SESS_HANDLE) p_cb->obx_handle;
                    bta_mse_cb.p_cback(BTA_MSE_UPDATE_INBOX_EVT, (tBTA_MSE *)&mas_session_id);
                }
                else
                {
                    OBX_PutRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
                }
            }
            else
            {
                OBX_PutRsp(p_evt->handle, OBX_RSP_CONTINUE, (BT_HDR *)NULL);
            }


            break;
        case BTA_MSE_OPER_SET_MSG_STATUS:


            if ((!p_set_msg_sts->rcv_msg_handle) && 
                (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_set_msg_sts->p_msg_handle, 
                                     BTA_MSE_64BIT_HEX_STR_SIZE)))
            {
                bta_ma_convert_hex_str_to_64bit_handle(p_set_msg_sts->p_msg_handle,
                                                       p_set_msg_sts->handle); 
                p_set_msg_sts->rcv_msg_handle = TRUE;
            }

            if (!p_set_msg_sts->rcv_sts_ind)
            {
                p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_STS_INDCTR, &len);
                if (p_param)
                {
                    p_set_msg_sts->rcv_sts_ind = TRUE;
                    BE_STREAM_TO_UINT8(p_set_msg_sts->sts_ind, p_param);
                }
            }

            if (!p_set_msg_sts->rcv_sts_val)
            {
                p_param = bta_mse_read_app_params(p_evt->p_pkt, BTA_MA_APH_STS_VALUE, &len);
                if (p_param)
                {
                    p_set_msg_sts->rcv_sts_val = TRUE;
                    BE_STREAM_TO_UINT8(p_set_msg_sts->sts_val, p_param);
                }
            }

            /* Read the body header from the obx packet */
            num_hdrs = OBX_ReadBodyHdr(p_evt->p_pkt, &p_obx->p_start, &p_obx->offset,
                                       &p_obx->final_pkt);
            /* process body or end of body header */
            if ((num_hdrs >= 1 && p_obx->final_pkt ) )
            {
                if (p_set_msg_sts->rcv_msg_handle && p_set_msg_sts->rcv_sts_ind && 
                    p_set_msg_sts->rcv_sts_val)
                {
                    if (p_set_msg_sts->sts_ind == BTA_MA_STS_INDTR_DELETE)
                    {
                        p_cb->oper = BTA_MSE_OPER_DEL_MSG;
                        bta_mse_req_app_access( inst_idx,  sess_idx,  p_data);
                    }
                    else
                    {
                        if (bta_mse_co_set_msg_read_status((tBTA_MA_SESS_HANDLE) p_cb->obx_handle,
                                                           p_set_msg_sts->handle,
                                                           p_set_msg_sts->sts_val,
                                                           bta_mse_cb.app_id) != BTA_MA_STATUS_OK)
                        {
                            rsp_code = OBX_RSP_FORBIDDEN;
                        }
                        send_ok_rsp = TRUE;
                    }
                }
                else
                {
                    rsp_code = OBX_RSP_BAD_REQUEST;
                }
            }

            if (!p_obx->final_pkt)
            {
                if (rsp_code != OBX_RSP_BAD_REQUEST)
                {
                    rsp_code = OBX_RSP_CONTINUE;
                }
            }

            if (rsp_code != OBX_RSP_OK)
            {
                OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
                if (rsp_code == OBX_RSP_BAD_REQUEST )
                {
                    bta_mse_clean_set_msg_sts(inst_idx,sess_idx);
                }
            }
            else
            {
                if (send_ok_rsp)
                {
                    OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
                    bta_mse_clean_set_msg_sts(inst_idx,sess_idx);
                }
            }
            break;
        default:
            OBX_PutRsp(p_evt->handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
            break;
    }

    /* Done with Obex packet */
    if (free_pkt) utl_freebuf((void**)&p_evt->p_pkt);
}

/*******************************************************************************
**
** Function         bta_mse_ma_obx_get
**
** Description      Proceses the obx get request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_get(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_EVT            *p_evt  = &p_data->obx_evt;
    UINT16                      len;
    UINT8                       *p_type;
    tBTA_MSE_MA_GET_ACT         get_act = BTA_MSE_MA_GET_ACT_NONE;
    tBTA_MSE_MA_GET_TYPE        get_type = BTA_MSE_MA_GET_TYPE_NONE;
    UINT8                       rsp_code = OBX_RSP_BAD_REQUEST;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_get inst idx=%d sess idx=%d",inst_idx, sess_idx);
    APPL_TRACE_EVENT1(" oper=%d",p_cb->oper );
#endif
    switch (p_cb->oper)
    {
        case BTA_MSE_OPER_GET_FOLDER_LIST:
            get_type = BTA_MSE_MA_GET_TYPE_CON_FOLDER_LIST;
            break;
        case BTA_MSE_OPER_GET_MSG_LIST:
            get_type = BTA_MSE_MA_GET_TYPE_CON_MSG_LIST;
            break;
        case BTA_MSE_OPER_GET_MSG:
            get_type = BTA_MSE_MA_GET_TYPE_CON_MSG;
            break;
        case BTA_MSE_OPER_NONE:
            if (OBX_ReadTypeHdr(p_evt->p_pkt, &p_type, &len))
            {
                if (!memcmp(p_type, BTA_MA_HDR_TYPE_FOLDER_LIST, len))
                {
                    get_type = BTA_MSE_MA_GET_TYPE_FOLDER_LIST;
                }
                else if (!memcmp(p_type, BTA_MA_HDR_TYPE_MSG_LIST, len) )
                {
                    get_type = BTA_MSE_MA_GET_TYPE_MSG_LIST;
                }
                else if (!memcmp(p_type, BTA_MA_HDR_TYPE_MSG, len) )
                {

                    get_type = BTA_MSE_MA_GET_TYPE_MSG;
                }
            }
            break;
        default:
            break;
    }
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_ma_obx_get get_type=%d",get_type);
#endif
    switch (get_type)
    {
        case BTA_MSE_MA_GET_TYPE_FOLDER_LIST:
            bta_mse_ma_fl_read_app_params(inst_idx, sess_idx, p_evt->p_pkt);
            get_act = BTA_MSE_MA_GET_ACT_NEW_FOLDER_LIST;
            break;

        case BTA_MSE_MA_GET_TYPE_MSG_LIST:
            if (!p_cb->ml_param.p_name)
            {
                p_cb->ml_param.p_name = (char *)GKI_getbuf((UINT16)(p_bta_mse_cfg->max_name_len + 1));
            }

            if (!p_cb->ml_param.p_path)
            {
                p_cb->ml_param.p_path =  (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1));
            }

            if ((p_cb->ml_param.p_name != NULL) && (p_cb->ml_param.p_path != NULL))
            {
                if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->ml_param.p_name, 
                                         p_bta_mse_cfg->max_name_len))
                {
                    rsp_code = OBX_RSP_BAD_REQUEST;
                    break;
                }
                else
                {
                    if (!bta_mse_get_msglist_path(inst_idx, sess_idx))
                    {
                        rsp_code = OBX_RSP_NOT_FOUND;
                        break;
                    }
                }
            }
            else
            {
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                break;
            }

            bta_mse_ma_ml_read_app_params(inst_idx, sess_idx,p_evt->p_pkt);
            get_act = BTA_MSE_MA_GET_ACT_NEW_MSG_LIST;
            break;

        case BTA_MSE_MA_GET_TYPE_MSG:

            if (!p_cb->msg_param.p_msg_handle)
            {
                if ((p_cb->msg_param.p_msg_handle = (char *)GKI_getbuf((UINT16)(BTA_MSE_64BIT_HEX_STR_SIZE))) == NULL )
                {
                    rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
                    get_act = BTA_MSE_MA_GET_ACT_ERR;
                    break;
                }
            }

            if (OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->msg_param.p_msg_handle, 
                                    BTA_MSE_64BIT_HEX_STR_SIZE))
            {
                bta_ma_convert_hex_str_to_64bit_handle(p_cb->msg_param.p_msg_handle,
                                                       p_cb->msg_param.data.handle);
            }
            else
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT0("Unable to decode or find Name Header for Message Handle");
#endif

                get_act = BTA_MSE_MA_GET_ACT_ERR;
                break;
            }

            if (!bta_mse_ma_msg_read_app_params(inst_idx, sess_idx, p_evt->p_pkt))
            {
                get_act = BTA_MSE_MA_GET_ACT_ERR;
                break;
            }
            get_act = BTA_MSE_MA_GET_ACT_NEW_MSG;
            break;

        case BTA_MSE_MA_GET_TYPE_CON_FOLDER_LIST:
            get_act = BTA_MSE_MA_GET_ACT_CON_FOLDER_LIST;
            break;

        case BTA_MSE_MA_GET_TYPE_CON_MSG_LIST:
            get_act = BTA_MSE_MA_GET_ACT_CON_MSG_LIST;
            break;

        case BTA_MSE_MA_GET_TYPE_CON_MSG:
            get_act = BTA_MSE_MA_GET_ACT_CON_MSG;
            break;
        default:
            break;    
    }
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1(" get_act=%d",get_act);
#endif
    switch (get_act)
    {
        case BTA_MSE_MA_GET_ACT_NEW_FOLDER_LIST:
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_GET_FOLDER_LIST);
            bta_mse_getfolderlist(inst_idx,sess_idx, TRUE);
            break;
        case BTA_MSE_MA_GET_ACT_CON_FOLDER_LIST:
            bta_mse_getfolderlist(inst_idx,sess_idx, FALSE);
            break;

        case BTA_MSE_MA_GET_ACT_NEW_MSG_LIST:
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_GET_MSG_LIST);
            bta_mse_req_app_access( inst_idx,  sess_idx,  p_data);
            break;
        case BTA_MSE_MA_GET_ACT_CON_MSG_LIST:
            bta_mse_getmsglist(inst_idx,sess_idx, FALSE);
            break;

        case BTA_MSE_MA_GET_ACT_NEW_MSG:
            bta_mse_set_ma_oper(inst_idx, sess_idx, BTA_MSE_OPER_GET_MSG);
            if (p_cb->msg_param.data.fraction_request == BTA_MA_FRAC_REQ_FIRST ||
                p_cb->msg_param.data.fraction_request  == BTA_MA_FRAC_REQ_NEXT)
            {
                p_cb->msg_param.add_frac_del_hdr = TRUE;
            }
            p_cb->msg_param.byte_get_cnt    = 0;
            bta_mse_req_app_access( inst_idx,  sess_idx,  p_data);
            break;
        case BTA_MSE_MA_GET_ACT_CON_MSG:
            bta_mse_getmsg(inst_idx,sess_idx, FALSE);
            break;

        case BTA_MSE_MA_GET_ACT_ERR:
        default:
            OBX_GetRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
            utl_freebuf((void**)&p_cb->ml_param.p_name);
            utl_freebuf((void**)&p_cb->ml_param.p_path);
            break;
    }
    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_setpath
**
** Description      Proceses the obx setpath request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_setpath(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{

    tBTA_MSE_MA_SESS_CB  *p_cb  = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_OBX_EVT     *p_evt = &p_data->obx_evt;
    tBTA_MA_DIR_NAV      flags  = (tBTA_MA_DIR_NAV)p_evt->param.sp.flag;
    UINT8                rsp_code    = OBX_RSP_BAD_REQUEST;
    tBTA_MSE_OPER        mse_oper = BTA_MSE_OPER_NONE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_obx_setpath inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
    APPL_TRACE_EVENT1(" flags=%d ", flags);
#endif

    /* Verify flags and handle before accepting */
    if ((flags == BTA_MA_DIR_NAV_ROOT_OR_DOWN_ONE_LVL) ||
        (flags == BTA_MA_DIR_NAV_UP_ONE_LVL))
    {

        p_cb->sp.p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1));
        p_cb->sp.p_name = (char *)GKI_getbuf((UINT16)(p_bta_mse_cfg->max_name_len + 1));
        if (p_cb->sp.p_name != NULL )
        {
            p_cb->sp.flags = flags;

            if (!OBX_ReadUtf8NameHdr(p_evt->p_pkt, (UINT8 *)p_cb->sp.p_name, 
                                     p_bta_fs_cfg->max_path_len))
            {
                p_cb->sp.p_name[0] = '\0';
            }

            rsp_code = bta_mse_chdir( inst_idx,  sess_idx,  &mse_oper);

        }
        else
        {
            rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
    }

    bta_mse_set_ma_oper(inst_idx, sess_idx, mse_oper);
    if (mse_oper != BTA_MSE_OPER_NONE)
    {
        bta_mse_req_app_access( inst_idx,  sess_idx,  p_data);
    }
    else
    {
        OBX_SetPathRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
        utl_freebuf((void**)&p_cb->sp.p_name);
    }

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}
/*******************************************************************************
**
** Function         bta_mse_ma_conn_err_rsp
**
** Description      Proceses the inavlid obx connection request
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_conn_err_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_conn_err_rsp inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif

    OBX_ConnectRsp(p_data->obx_evt.handle, OBX_RSP_BAD_REQUEST, (BT_HDR *)NULL);
    /* Done with Obex packet */
    utl_freebuf((void**)&(p_data->obx_evt.p_pkt));
}

/*******************************************************************************
**
** Function         bta_mse_ma_close_complete
**
** Description      Proceses the connection close complete event.  
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_close_complete(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_CB              *p_scb  = BTA_MSE_GET_INST_CB_PTR(inst_idx);
    tBTA_MSE_MA_SESS_CB         *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE                    cback_evt_data;
    tBTA_MSE                    *p_cevt = &cback_evt_data;
    UINT8                       num_act_mas=0, num_act_sess=0, i;
    tBTA_MA_STATUS              clean_getput_status = BTA_MA_STATUS_OK;
    UINT8                       ccb_idx;
    BD_ADDR                     bd_addr;
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ma_close_complete inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif


    p_cb->cout_active = FALSE;

    if (p_cb->aborting) clean_getput_status = BTA_MA_STATUS_ABORTED;
    bta_mse_clean_getput(inst_idx, sess_idx, clean_getput_status);
    utl_freebuf((void**)&p_cb->p_workdir); 

    /* inform role manager */

    bta_mse_pm_conn_close(p_cb->bd_addr);
    bdcpy(bd_addr, p_cb->bd_addr);
    memset(p_cb->bd_addr, 0, BD_ADDR_LEN);
    p_cb->peer_mtu = 0;

    /* We now done with the close this seesion so issue a BTA_MSE_MA_CLOSE_EVT*/
    p_cevt->ma_close.mas_session_id  = p_cb->obx_handle;
    p_cevt->ma_close.mas_instance_id = p_scb->mas_inst_id;
    p_cevt->ma_close.status           = BTA_MA_STATUS_OK ;
    bta_mse_cb.p_cback(BTA_MSE_MA_CLOSE_EVT, (tBTA_MSE *) p_cevt);


    /* turn off the registration status for the associated MAs instance
       and also check whether MN connection needs to be closed or not */
    if (bta_mse_find_bd_addr_match_mn_cb_index(bd_addr, &ccb_idx) &&
        bta_mse_mn_is_inst_id_exist(ccb_idx, p_scb->mas_inst_id))
    {
        bta_mse_mn_remove_inst_id(ccb_idx, p_scb->mas_inst_id);
        if (!bta_mse_mn_find_num_of_act_inst_id(ccb_idx))
        {
            bta_mse_mn_sm_execute(ccb_idx,BTA_MSE_MN_INT_CLOSE_EVT, NULL);  
        }
    }

    /* Check Any MAS instance need to be stopped */
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("stopping=%d MAS in use=%d", p_scb->stopping,p_scb->in_use);
#endif
    if (p_scb->stopping && p_scb->in_use)
    {
        num_act_sess = 0;
        for (i=0; i < BTA_MSE_NUM_SESS; i ++)
        {
            p_cb = BTA_MSE_GET_SESS_CB_PTR(inst_idx, i);
            if (p_cb->state != BTA_MSE_MA_LISTEN_ST)
                num_act_sess++;
        }
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT1("num_act_sess=%d ", num_act_sess);
#endif
        if (!num_act_sess)
        {
            p_cevt->stop.status = BTA_MA_STATUS_OK;
            p_cevt->stop.mas_instance_id = p_scb->mas_inst_id;
            bta_mse_cb.p_cback(BTA_MSE_STOP_EVT, (tBTA_MSE *) p_cevt);
            bta_mse_clean_mas_service(inst_idx);
        }
    }

    if (bta_mse_cb.disabling && bta_mse_cb.enable)
    {
        num_act_mas=0; 
        for (i=0; i < BTA_MSE_NUM_INST ; i ++)
        {
            p_scb = BTA_MSE_GET_INST_CB_PTR(i);
            if (p_scb->in_use )
                num_act_mas++;
        }

        if (!num_act_mas)
        {
            bta_mse_cb.enable = FALSE;
            p_cevt->disable.status = BTA_MA_STATUS_OK;
            p_cevt->disable.app_id = bta_mse_cb.app_id;
            bta_mse_cb.p_cback(BTA_MSE_DISABLE_EVT, (tBTA_MSE *) p_cevt);
#if BTA_MSE_DEBUG == TRUE
            APPL_TRACE_EVENT0("MSE Disabled location-2");
#endif
        }
    }
}

/*******************************************************************************
**
** Function         bta_mse_ignore_obx
**
** Description      Ignores the obx event                 
**
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  p_data      - Pointer to the event data 
**
** Returns          void
**
*******************************************************************************/
void bta_mse_ignore_obx(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_ignore_obx inst idx=%d sess idx=%d",inst_idx, sess_idx);
#endif

    utl_freebuf((void**)&p_data->obx_evt.p_pkt);
}
/*******************************************************************************
**
** Function         bta_mse_ma_obx_cback
**
** Description      OBX callback function for MA.
**
** Parameters       handle      - Handle for Obex session 
**                  obx_event   - Obex event
**                  param       - event parameters
**                  p_pkt       - pointer to Obex packet
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event,
                           tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_MSE_OBX_EVT    *p_obx_msg;
    UINT16              event = 0;

    switch (obx_event)
    {
        case OBX_CONNECT_REQ_EVT:
            event = BTA_MSE_MA_OBX_CONN_EVT;
            break;
        case OBX_DISCONNECT_REQ_EVT:
            event = BTA_MSE_MA_OBX_DISC_EVT;
            break;
        case OBX_PUT_REQ_EVT:
            event = BTA_MSE_MA_OBX_PUT_EVT;
            break;
        case OBX_GET_REQ_EVT:
            event = BTA_MSE_MA_OBX_GET_EVT;
            break;
        case OBX_SETPATH_REQ_EVT:
            event = BTA_MSE_MA_OBX_SETPATH_EVT;
            break;
        case OBX_ABORT_REQ_EVT:
            event = BTA_MSE_MA_OBX_ABORT_EVT;
            break;
        case OBX_CLOSE_IND_EVT:
            event = BTA_MSE_MA_OBX_CLOSE_EVT;
            break;
        case OBX_TIMEOUT_EVT:
            break;
        case OBX_PASSWORD_EVT:
            break;
        default:
            /* Unrecognized packet; disconnect the session */
            if (p_pkt)
                event = BTA_MSE_MA_OBX_DISC_EVT;
    }

    /* send event to BTA, if any */
    if (event && (p_obx_msg =
                  (tBTA_MSE_OBX_EVT *) GKI_getbuf(sizeof(tBTA_MSE_OBX_EVT))) != NULL)
    {
#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
        APPL_TRACE_DEBUG1("MA OBX Event Callback: mse_obx_event [%s]", bta_mse_obx_evt_code(event));
#endif
        p_obx_msg->hdr.event = event;
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
        p_obx_msg->param = param;
        p_obx_msg->p_pkt = p_pkt;
        p_obx_msg->hdr.layer_specific = 0xFFFF;

        bta_sys_sendmsg(p_obx_msg);
    }
}
/*******************************************************************************
** Message Notification Client (MNC) Action functions
**
*******************************************************************************/
/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback
**
** Description      This is the SDP callback function used by MSE.
**                  This function will be executed by SDP when the service
**                  search is completed.  If the search is successful, it
**                  finds the first record in the database that matches the
**                  UUID of the search.  Then retrieves the scn from the
**                  record.
**
** Parameters       ccb_idx - Index to the MN control block
**                  status  - status of the SDP callabck
**                 
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback(UINT8 ccb_idx, UINT16 status)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_MN_SDP_OK     *p_buf;
    tSDP_DISC_REC          *p_rec = NULL;
    tSDP_PROTOCOL_ELEM   pe;
    UINT8                scn = 0;
    BOOLEAN              found = FALSE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_sdp_cback status:%d", status);
#endif
    if (status == SDP_SUCCESS || status == SDP_DB_FULL)
    {
        /* loop through all records we found */
        do
        {
            /* get next record; if none found, we're done */
            if ((p_rec = SDP_FindServiceInDb(p_cb->p_db, p_cb->sdp_service,
                                             p_rec)) == NULL)
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT0("SDP Not Found");
#endif 
                break;
            }

            /* get scn from proto desc list; if not found, go to next record */
            if (SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe))
            {
                found = TRUE;
                scn = (UINT8) pe.params[0];

#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT1("SDP found scn=%d", scn);
#endif 
                /* we've got everything, we're done */
                break;
            }
            else
                continue;
        } while (TRUE);
    }

    /* send result in event back to BTA */
    if ((p_buf = (tBTA_MSE_MN_SDP_OK *) GKI_getbuf(sizeof(tBTA_MSE_MN_SDP_OK))) != NULL)
    {
        p_buf->hdr.event = BTA_MSE_MN_SDP_FAIL_EVT;
        p_buf->ccb_idx = ccb_idx;
        if (status == SDP_SUCCESS || status == SDP_DB_FULL)
        {
            if (found == TRUE)
            {
                p_buf->hdr.event = BTA_MSE_MN_SDP_OK_EVT;
                p_buf->scn = scn;
            }
            else
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT0("SDP 1 BTA_MSE_MN_SDP_FAIL_EVT");
#endif 
            }
        }
        else
        {

#if BTA_MSE_DEBUG == TRUE
            APPL_TRACE_EVENT0("SDP 2 BTA_MSE_MN_SDP_FAIL_EVT");
#endif 
        }

        bta_sys_sendmsg(p_buf);
    }
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback0
**
** Description      This is the SDP callback function used by MN indx = 0
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback0(UINT16 status)
{
    bta_mse_mn_sdp_cback(0, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback1
**
** Description      This is the SDP callback function used by MN indx = 1
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback1(UINT16 status)
{
    bta_mse_mn_sdp_cback(1, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback2
**
** Description      This is the SDP callback function used by MN indx = 2
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback2(UINT16 status)
{
    bta_mse_mn_sdp_cback(2, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback3
**
** Description      This is the SDP callback function used by MN indx = 3
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback3(UINT16 status)
{
    bta_mse_mn_sdp_cback(3, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback4
**
** Description      This is the SDP callback function used by MN indx = 4
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback4(UINT16 status)
{
    bta_mse_mn_sdp_cback(4, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback5
**
** Description      This is the SDP callback function used by MN indx = 5
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback5(UINT16 status)
{
    bta_mse_mn_sdp_cback(5, status);
}

/******************************************************************************
**
** Function         bta_mse_mn_sdp_cback6
**
** Description      This is the SDP callback function used by MN indx = 6
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_mse_mn_sdp_cback6(UINT16 status)
{
    bta_mse_mn_sdp_cback(6, status);
}
/*******************************************************************************
**
** Function         bta_mse_mn_init_sdp
**
** Description      Initialize SDP on message notification server device.                  
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_init_sdp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = &(bta_mse_cb.ccb[ccb_idx]);
    tBTA_MSE_MN_INT_OPEN *p_evt = &p_data->mn_int_open;

    tSDP_UUID   uuid_list;
    UINT16      attr_list[3];
    UINT16      num_attrs = 2;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_init_sdp");
#endif

    p_cb->in_use = TRUE;
    p_cb->sec_mask = p_evt->sec_mask;
    bdcpy(p_cb->bd_addr,p_evt->bd_addr);
    p_cb->sdp_pending = TRUE;
    p_cb->sdp_cback = bta_mse_mn_sdp_cback_arr[ccb_idx];

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT6("SDP INIT BD_ADDR= %x:%x:%x:%x:%x:%x",
                      p_cb->bd_addr[0],
                      p_cb->bd_addr[1],
                      p_cb->bd_addr[2],
                      p_cb->bd_addr[3],
                      p_cb->bd_addr[4],
                      p_cb->bd_addr[5]);
#endif

    if ((p_cb->p_db = (tSDP_DISCOVERY_DB *) GKI_getbuf(BTA_MSE_MN_DISC_SIZE)) != NULL)
    {
        attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
        attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;

        uuid_list.len = LEN_UUID_16;

        p_cb->sdp_service = UUID_SERVCLASS_MESSAGE_NOTIFICATION;
        uuid_list.uu.uuid16 = UUID_SERVCLASS_MESSAGE_NOTIFICATION;

        SDP_InitDiscoveryDb(p_cb->p_db, BTA_MSE_MN_DISC_SIZE, 1, &uuid_list, num_attrs, attr_list);
        if (!SDP_ServiceSearchAttributeRequest(p_cb->bd_addr, p_cb->p_db, p_cb->sdp_cback))
        {
#if BTA_MSE_DEBUG == TRUE
            APPL_TRACE_EVENT0("SDP Init failed ");
#endif
            bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_SDP_FAIL_EVT, p_data);
        }
        else
        {
#if BTA_MSE_DEBUG == TRUE
            APPL_TRACE_EVENT0("SDP Init Success ");
#endif
        }
    }
}

/*******************************************************************************
**
** Function         bta_mse_mn_start_client
**
** Description      Starts the OBEX client for Message Notification service.                  
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_start_client(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{

    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_OBX_PKT *p_obx = &p_cb->obx;
    BOOLEAN connect_fail = TRUE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_start_client ccb idx=%d ",ccb_idx);
#endif

    /* free discovery data base */
    utl_freebuf((void**)&p_cb->p_db);
    p_cb->sdp_pending = FALSE;
    /* Allocate an OBX packet */
    if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(OBX_HANDLE_NULL, OBX_CMD_POOL_SIZE)) != NULL)
    {
        /* set security level */
        BTM_SetSecurityLevel (TRUE, "BTA_MNC", BTM_SEC_SERVICE_MAP,
                              p_cb->sec_mask, BT_PSM_RFCOMM,
                              BTM_SEC_PROTO_RFCOMM, p_data->mn_sdp_ok.scn);

        if (p_cb->sdp_service == UUID_SERVCLASS_MESSAGE_NOTIFICATION)
        {
            OBX_AddTargetHdr(p_obx->p_pkt, (UINT8 *)BTA_MAS_MESSAGE_NOTIFICATION_TARGET_UUID,
                             BTA_MAS_UUID_LENGTH);
        }
        if (OBX_ConnectReq(p_cb->bd_addr, p_data->mn_sdp_ok.scn, OBX_MAX_MTU,
                           bta_mse_mn_obx_cback, &p_cb->obx_handle, p_obx->p_pkt) == OBX_SUCCESS)
        {
            p_obx->p_pkt = NULL;    /* OBX will free the memory */
            connect_fail = FALSE;
        }
        else
        {
            utl_freebuf((void**)&p_obx->p_pkt);
        }
    }

    if (connect_fail)
    {
        bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_OBX_CLOSE_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_mse_mn_rsp_timeout
**
** Description      Process the OBX response timeout event
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_rsp_timeout(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

    if (p_cb->timer_oper == BTA_MSE_TIMER_OP_ABORT)
    {
        /* Start stop response timer */
        bta_mse_mn_start_timer(ccb_idx, BTA_MSE_TIMER_OP_STOP);

        OBX_DisconnectReq(p_cb->obx_handle, NULL);
    }
    else    /* Timeout waiting for disconnect response */
    {
        bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_OBX_CLOSE_EVT, p_data);

    }
}
/*******************************************************************************
**
** Function         bta_mse_mn_stop_client
**
** Description      Stop the OBX client for Message Notification service.                  
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_stop_client(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_stop_client ccb idx=%d ",ccb_idx);
#endif

    if (!p_cb->sdp_pending)
    {
        /* Start stop response timer */
        bta_mse_mn_start_timer(ccb_idx, BTA_MSE_TIMER_OP_STOP);
        OBX_DisconnectReq(p_cb->obx_handle, NULL);
    }
    else
    {
        APPL_TRACE_WARNING0("bta_mase_mn_stop_client: Waiting for SDP to complete");
    }
}
/*******************************************************************************
**
** Function         bta_mse_mn_obx_conn_rsp
**
** Description      Process OBX connection response.                  
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_obx_conn_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB      *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_OBX_EVT    *p_evt = &p_data->obx_evt;
    tBTA_MSE            param;
    UINT8               mas_insatnce_id;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_obx_conn_rsp ccb idx=%d ",ccb_idx);
#endif

    p_cb->peer_mtu = p_data->obx_evt.param.conn.mtu;
    p_cb->obx_handle = p_evt->handle;

    if (p_cb->sdp_service == UUID_SERVCLASS_MESSAGE_NOTIFICATION)
    {
        bdcpy(param.mn_open.bd_addr ,p_cb->bd_addr);  
        BCM_STRNCPY_S((char *)param.mn_open.dev_name, sizeof(param.mn_open.dev_name), "",
                BTM_MAX_REM_BD_NAME_LEN);
        if (bta_mse_mn_get_first_inst_id(ccb_idx, &mas_insatnce_id))
        {
            param.mn_open.first_mas_instance_id = mas_insatnce_id;
        }
        else
        {
            APPL_TRACE_ERROR0("Unable to find the first instance ID");
        }
    }

    /* inform role manager */
    bta_mse_pm_conn_open(p_cb->bd_addr);

    bta_mse_cb.p_cback(BTA_MSE_MN_OPEN_EVT, &param);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);
}
/*******************************************************************************
**
** Function         bta_mse_mn_close
**
** Description      Process the connection close event.                  
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_close(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_close ccb idx=%d ",ccb_idx);
#endif

    bta_mse_mn_remove_all_inst_ids(ccb_idx);

    /* finished if not waiting on a call-in function */
    if (!p_cb->sdp_pending && !p_cb->cout_active)
        bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_CLOSE_CMPL_EVT, p_data);
}
/*******************************************************************************
**
** Function         bta_mse_mn_send_notif
**
** Description      Process send notification request              
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_send_notif(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB              *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_MN_API_SEND_NOTIF  *p_evt = &p_data->mn_send_notif;
    UINT8                       *p_buffer;
    UINT16                      buffer_len;
    tBTA_MA_STATUS              status = BTA_MA_STATUS_FAIL;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_send_notif ccb_idx=%d ",ccb_idx);
#endif

    if ( (p_cb->req_pending == TRUE) && 
         (p_cb->obx_oper == BTA_MSE_MN_OP_PUT_EVT_RPT))
    {
        /* only one pending request per Obex connection is allowed */
        bta_mse_mn_send_notif_evt(p_evt->mas_instance_id,
                                  BTA_MA_STATUS_NO_RESOURCE,
                                  p_cb->bd_addr);
        return;
    }

    memset(&(p_cb->msg_notif), 0, sizeof(tBTA_MSE_MN_MSG_NOTIF));

    if ((p_buffer = (UINT8 *)GKI_getbuf(BTA_MSE_MN_MAX_MSG_EVT_OBECT_SIZE)) != NULL)
    {
        /* Build the Message Event Object */
        buffer_len = BTA_MSE_MN_MAX_MSG_EVT_OBECT_SIZE;
        status = bta_mse_build_map_event_rpt_obj( p_evt->notif_type, 
                                                  p_evt->handle, 
                                                  p_evt->p_folder,
                                                  p_evt->p_old_folder, 
                                                  p_evt->msg_type,
                                                  &buffer_len,
                                                  p_buffer);
        if (status == BTA_MA_STATUS_OK)
        {
            p_cb->msg_notif.p_buffer        = p_buffer;
            p_cb->msg_notif.buffer_len      = buffer_len;
            p_cb->msg_notif.bytes_sent      = 0;
            p_cb->msg_notif.mas_instance_id = p_evt->mas_instance_id;
            status = bta_mse_mn_cont_send_notif(ccb_idx, TRUE);
        }
    }

    if ( status != BTA_MA_STATUS_OK)
    {
        bta_mse_mn_send_notif_evt(p_evt->mas_instance_id,
                                  BTA_MA_STATUS_NO_RESOURCE,
                                  p_cb->bd_addr);
        bta_mse_mn_clean_send_notif(ccb_idx);
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("status=%d ",status );
#endif
}
/*******************************************************************************
**
** Function         bta_mse_mn_put_rsp
**
** Description      Process the obx PUT response.              
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_put_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB          *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_OBX_EVT        *p_evt = &p_data->obx_evt;
    tBTA_MSE_MN_MSG_NOTIF   *p_msg_notif = &p_cb->msg_notif; 
    tBTA_MA_STATUS          status = BTA_MA_STATUS_FAIL;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_mn_put_rsp ccb idx=%d rsp_code=%d",
                      ccb_idx, p_evt->rsp_code);
#endif

    p_cb->req_pending = FALSE;
    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

    if (p_cb->obx_oper == BTA_MSE_MN_OP_PUT_EVT_RPT)
    {
        /* If not finished with Put, start another read */
        if (p_evt->rsp_code == OBX_RSP_CONTINUE)
            bta_mse_mn_cont_send_notif(ccb_idx, FALSE);
        else
        {
            /* done with obex operation */
            if ( p_evt->rsp_code == OBX_RSP_OK ) status = BTA_MA_STATUS_OK;
            bta_mse_mn_send_notif_evt(p_msg_notif->mas_instance_id, status,
                                      p_cb->bd_addr);
            bta_mse_mn_clean_send_notif(ccb_idx);
        }
    }
}
/*******************************************************************************
**
** Function         bta_mse_mn_close_compl
**
** Description      Process the close complete event.             
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_close_compl(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE       param;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_close_compl ccb idx=%d ",ccb_idx);
#endif

    /* free discovery data base */
    utl_freebuf((void**)&p_cb->p_db);
    p_cb->sdp_pending = FALSE;

    p_cb->cout_active = FALSE;
    bta_mse_mn_stop_timer(ccb_idx, BTA_MSE_TIMER_OP_ALL);
    bta_mse_mn_clean_send_notif(ccb_idx);
    /* inform role manager */
    bta_mse_pm_conn_close(p_cb->bd_addr);
    bdcpy(param.mn_close.bd_addr ,p_cb->bd_addr); 
    bta_mse_cb.p_cback(BTA_MSE_MN_CLOSE_EVT, &param);
    memset(p_cb, 0 , sizeof(tBTA_MSE_MN_CB));
}

/*******************************************************************************
**
** Function         bta_mse_mn_abort
**
** Description      Process the abort event.        
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_abort(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_abort ccb idx=%d ",ccb_idx);
#endif
    /* Abort an active request */
    if (p_cb->obx_oper != BTA_MSE_MN_OP_NONE)
    {
        p_cb->aborting = BTA_MSE_MN_ABORT_REQ_NOT_SENT;

        /* Issue the abort request only if no request pending.
         * some devices do not like out of sequence aborts even though
         * the spec allows it */
        if (!p_cb->req_pending)
        {
            /* Start abort response timer */
            bta_mse_mn_start_timer(ccb_idx, BTA_MSE_TIMER_OP_ABORT);
            OBX_AbortReq(p_cb->obx_handle, (BT_HDR *)NULL);
            p_cb->aborting = BTA_MSE_MN_ABORT_REQ_SENT;
        }
    }
}

/*******************************************************************************
**
** Function         bta_mse_mn_abort_rsp
**
** Description      Process the abort response event.        
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_abort_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB          *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);
    tBTA_MSE_OBX_EVT        *p_evt = &p_data->obx_evt;
    tBTA_MSE_MN_MSG_NOTIF   *p_msg_notif = &p_cb->msg_notif; 
    tBTA_MA_STATUS          status = BTA_MA_STATUS_FAIL;


    bta_mse_mn_stop_timer(ccb_idx,BTA_MSE_TIMER_OP_ABORT);

    /* Done with Obex packet */
    utl_freebuf((void**)&p_evt->p_pkt);

    if (p_cb->obx_oper != BTA_MSE_MN_OP_NONE)
    {
        if (p_cb->obx_oper == BTA_MSE_MN_OP_PUT_EVT_RPT )
        {
            bta_mse_mn_send_notif_evt(p_msg_notif->mas_instance_id, status,
                                      p_cb->bd_addr);
            bta_mse_mn_clean_send_notif(ccb_idx);
        }
    }
}

/*******************************************************************************
**
** Function         bta_mse_mn_sdp_fail
**
** Description      Process the SDP fail event.        
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_sdp_fail(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_sdp_fail ccb idx=%d ",ccb_idx);
#endif
    /* free discovery data base */
    utl_freebuf((void**)&p_cb->p_db);
    p_cb->sdp_pending = FALSE;
    bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_OBX_CLOSE_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_mse_mn_obx_tout
**
** Description      Process the obx timeout event.        
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_obx_tout(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB      *p_cb  = BTA_MSE_GET_MN_CB_PTR(ccb_idx);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_obx_tout ccb idx=%d ",ccb_idx);
#endif

    if (p_cb->req_pending)
    {
        if (p_cb->obx_oper == BTA_MSE_MN_OP_PUT_EVT_RPT)
        {
            bta_mse_mn_send_notif_evt(  p_cb->msg_notif.mas_instance_id,
                                        BTA_MA_STATUS_FAIL,
                                        p_cb->bd_addr);
            bta_mse_mn_clean_send_notif(ccb_idx);
        }
    }
}
/*******************************************************************************
**
** Function         bta_mse_mn_ignore_obx
**
** Description      Ignore OBX event.       
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_ignore_obx(UINT8 ccb_idx, tBTA_MSE_DATA *p_data)
{

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_ignore_obx ccb idx=%d ",ccb_idx);
#endif

    utl_freebuf((void **) &p_data->obx_evt.p_pkt);
}



/*****************************************************************************
**  MNC Callback Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_mse_mn_obx_cback
**
** Description      OBX callback function.
**
** Parameters       handle      - Handle for Obex session 
**                  obx_event   - Obex event
**                  rsp_code    - Obex response code
**                  param       - event parameters
**                  p_pkt       - pointer to Obex packet
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event, UINT8 rsp_code,
                           tOBX_EVT_PARAM param, BT_HDR *p_pkt)
{
    tBTA_MSE_OBX_EVT *p_obx_msg;
    UINT16              event = 0;



    switch (obx_event)
    {
        case OBX_CONNECT_RSP_EVT:
            if (rsp_code == OBX_RSP_OK)
            {
                event = BTA_MSE_MN_OBX_CONN_RSP_EVT;                 
            }
            else    /* Obex will disconnect underneath BTA */
            {
                APPL_TRACE_WARNING1("MN_OBX_CBACK: Bad connect response 0x%02x", rsp_code);
                if (p_pkt)
                    GKI_freebuf(p_pkt);
                return;
            }
            break;
        case OBX_PUT_RSP_EVT:
            event = BTA_MSE_MN_OBX_PUT_RSP_EVT;
            break;
        case OBX_CLOSE_IND_EVT:
            event = BTA_MSE_MN_OBX_CLOSE_EVT;
            break;
        case OBX_TIMEOUT_EVT:
            event = BTA_MSE_MN_OBX_TOUT_EVT;
            break;
        case OBX_ABORT_RSP_EVT:
            event = BTA_MSE_MN_OBX_ABORT_RSP_EVT;
            break;
        case OBX_PASSWORD_EVT:
        case OBX_GET_RSP_EVT:
        case OBX_SETPATH_RSP_EVT:
        default:
            /*  case OBX_DISCONNECT_RSP_EVT: Handled when OBX_CLOSE_IND_EVT arrives */
            if (p_pkt)
                GKI_freebuf(p_pkt);
            return;
    }



    if (event && (p_obx_msg =
                  (tBTA_MSE_OBX_EVT *) GKI_getbuf(sizeof(tBTA_MSE_OBX_EVT))) != NULL)
    {
#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
        APPL_TRACE_DEBUG1("MN OBX Event Callback: mn_obx_event [%s]", 
                          bta_mse_obx_evt_code(event));
#endif
        /* send event to BTA, if any */
        p_obx_msg->hdr.event = event;
        p_obx_msg->hdr.layer_specific = 0xFFFF; /* not used */
        p_obx_msg->obx_event = obx_event;
        p_obx_msg->handle = handle;
        p_obx_msg->rsp_code = rsp_code;
        p_obx_msg->param = param;
        p_obx_msg->p_pkt = p_pkt;

        bta_sys_sendmsg(p_obx_msg);
    }
}


/*****************************************************************************
**  Local MSE Event Processing Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_mse_req_app_access
**
** Description      Sends an access request event to the application.
**
** Parameters       ccb_idx - Index to the MN control block
**                  p_data  - Pointer to the MN event data
**                    
** Returns          void
**
*******************************************************************************/
void bta_mse_req_app_access (UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_SESS_CB     *p_cb   = BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx);
    tBTA_MSE_ACCESS         *p_acc_evt;
    char                    *p_devname;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT2("bta_mse_req_app_access inst idx=%d sess idx=%d",
                      inst_idx, sess_idx);
#endif

    /* Ask application for the operation's access permission */
    if ((p_acc_evt = (tBTA_MSE_ACCESS *)GKI_getbuf((UINT16) (sizeof(tBTA_MSE_ACCESS)+ 
                                                             (p_bta_fs_cfg->max_path_len + 1)))) != NULL)
    {
        memset(p_acc_evt, 0, sizeof(tBTA_MSE_ACCESS));
        p_acc_evt->p_path =  (char *) (p_acc_evt+1);
        p_acc_evt->p_path[0]='\0';

        APPL_TRACE_API1("MSE ACCESS REQ: Oper=%d", p_cb->oper);
        switch (p_cb->oper)
        {
            case BTA_MSE_OPER_SETPATH:
                BCM_STRNCPY_S(p_acc_evt->p_path, p_bta_fs_cfg->max_path_len+1,
                        p_cb->sp.p_path, p_bta_fs_cfg->max_path_len);
                break;

            case BTA_MSE_OPER_GET_MSG_LIST:
                BCM_STRNCPY_S(p_acc_evt->p_path, p_bta_fs_cfg->max_path_len+1,
                              p_cb->ml_param.p_path, p_bta_fs_cfg->max_path_len);
                break;

            case BTA_MSE_OPER_GET_MSG:
                memcpy(p_acc_evt->handle, p_cb->msg_param.data.handle, 
                       sizeof(tBTA_MA_MSG_HANDLE)); 
                break;
            case BTA_MSE_OPER_PUSH_MSG:
                memset(p_acc_evt->handle, 0, sizeof(tBTA_MA_MSG_HANDLE));
                BCM_STRNCPY_S(p_acc_evt->p_path, p_bta_fs_cfg->max_path_len+1,
                        p_cb->push_msg.param.p_folder, p_bta_fs_cfg->max_path_len);
                break;
            case BTA_MSE_OPER_DEL_MSG:
                memcpy(p_acc_evt->handle, p_cb->set_msg_sts.handle, 
                       sizeof(tBTA_MA_MSG_HANDLE)); 
                p_acc_evt->delete_sts = p_cb->set_msg_sts.sts_val;
                break;
            default:
                break;
        }

        p_acc_evt->oper = p_cb->oper;
        p_acc_evt->mas_session_id = (tBTA_MA_SESS_HANDLE) p_cb->obx_handle;
        bdcpy(p_acc_evt->bd_addr, p_cb->bd_addr);
        if ((p_devname = BTM_SecReadDevName(p_cb->bd_addr)) != NULL)
            BCM_STRNCPY_S((char *)p_acc_evt->dev_name, sizeof(p_acc_evt->dev_name), p_devname, BTM_MAX_REM_BD_NAME_LEN);

        bta_mse_cb.p_cback(BTA_MSE_ACCESS_EVT, (tBTA_MSE *)p_acc_evt);
        GKI_freebuf(p_acc_evt);
    }
}

/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)

/*******************************************************************************
**
** Function         bta_mse_obx_evt_code
**
** Description      get the obx event string pointer
**
** Parameters       evt_code - obex event code
**   
** Returns          char * - event string pointer
**
*******************************************************************************/
static char *bta_mse_obx_evt_code(UINT16 evt_code)
{
    switch (evt_code)
    {
        case BTA_MSE_MA_OBX_CONN_EVT:
            return "BTA_MSE_MA_OBX_CONN_EVT";
        case BTA_MSE_MA_OBX_DISC_EVT:
            return "BTA_MSE_MA_OBX_DISC_EVT";
        case BTA_MSE_MA_OBX_PUT_EVT:
            return "BTA_MSE_MA_OBX_PUT_EVT";
        case BTA_MSE_MA_OBX_GET_EVT:
            return "BTA_MSE_MA_OBX_GET_EVT";
        case BTA_MSE_MA_OBX_SETPATH_EVT:
            return "BTA_MSE_MA_OBX_SETPATH_EVT";
        case BTA_MSE_MA_OBX_ABORT_EVT:
            return "BTA_MSE_MA_OBX_ABORT_EVT";
        case BTA_MSE_MA_OBX_CLOSE_EVT:
            return "BTA_MSE_MA_OBX_CLOSE_EVT";
        case BTA_MSE_MN_OBX_TOUT_EVT:
            return "BTA_MSE_MN_OBX_TOUT_EVT";
        case BTA_MSE_MN_OBX_CONN_RSP_EVT:
            return "BTA_MSE_MN_OBX_CONN_RSP_EVT";                 
        case BTA_MSE_MN_OBX_PUT_RSP_EVT:
            return "BTA_MSE_MN_OBX_PUT_RSP_EVT";
        case BTA_MSE_MN_OBX_CLOSE_EVT:
            return "BTA_MSE_MN_OBX_CLOSE_EVT";
        case BTA_MSE_MN_OBX_ABORT_RSP_EVT:
            return"BTA_MSE_MN_OBX_ABORT_RSP_EVT";
        default:
            return "unknown MSE OBX event code";
    }
}
#endif  /* Debug Functions */















#endif /* BTA_MSE_INCLUDED */
