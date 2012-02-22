/*****************************************************************************
**
**  Name:           bta_mse_ci.c
**
**  Description:    This is the implementaion for the Message Server Equipment 
**                  (MSE) subsystem call-in functions.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bta_api.h"
#include "bta_ma_def.h"
#include "bta_mse_api.h"
#include "bta_mse_co.h"
#include "bta_mse_int.h"
#include "bta_mse_ci.h"

/*******************************************************************************
**
** Function         bta_mse_ci_get_folder_entry
**
** Description      This function is called in response to the
**                  bta_mse_co_get_folder_entry call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK if p_entry points to a valid entry.
**                           BTA_MA_STATUS_EODIR if no more entries (p_entry is ignored).
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_mse_ci_get_folder_entry(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                tBTA_MA_STATUS status, 
                                                UINT16 evt)
{
    tBTA_MSE_CI_GET_FENTRY  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_GET_FENTRY *)GKI_getbuf(sizeof(tBTA_MSE_CI_GET_FENTRY))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT2("bta_mse_ci_get_folder_entry sess_id=%d, status=%d \n",  
                          mas_session_id, status);
#endif
        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}
/*******************************************************************************
**
** Function         bta_mse_ci_get_msg_list_info
**
** Description      This function is called in response to the
**                  bta_mse_co_get_msg_list_info call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK operation is successful.
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_mse_ci_get_msg_list_info(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                 tBTA_MA_STATUS status, 
                                                 UINT16 evt)
{
    tBTA_MSE_CI_GET_ML_INFO  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_GET_ML_INFO *)GKI_getbuf(sizeof(tBTA_MSE_CI_GET_ML_INFO))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT2("bta_mse_ci_get_msg_list_info sess_id=%d, status=%d \n",  
                          mas_session_id, status);  
#endif
        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ci_get_msg_list_entry
**
** Description      This function is called in response to the
**                  bta_mse_co_get_msg_list_entry call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK if p_entry points to a valid entry.
**                           BTA_MA_STATUS_EODIR if no more entries (p_entry is ignored).
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_mse_ci_get_msg_list_entry(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                  tBTA_MA_STATUS status, 
                                                  UINT16 evt)
{
    tBTA_MSE_CI_GET_ML_ENTRY  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_GET_ML_ENTRY *)GKI_getbuf(sizeof(tBTA_MSE_CI_GET_ML_ENTRY))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT2("bta_mse_ci_get_msg_list_entry sess_id=%d, status=%d \n",  
                          mas_session_id, status);   
#endif
        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ci_get_msg
**
** Description      This function is called in response to the
**                  bta_mse_co_get_msg call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  filled_buff_size - size of the filled buffer  
**                  multi_pkt_status - BTA_MA_MPKT_STATUS_MORE - need to get more packets  
**                                     BTA_MA_MPKT_STATUS_LAST - last packet of the bMessage
**                  frac_deliver_status -  BTA_MA_FRAC_DELIVER_MORE - other fractions following 
**                                                                    this bMessage
**                                         BTA_MA_FRAC_DELIVER_LAST - Last fraction
**                  evt    - evt from the call-out function
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_mse_ci_get_msg(tBTA_MA_SESS_HANDLE  mas_session_id,
                                       tBTA_MA_STATUS status, 
                                       UINT16 filled_buff_size, 
                                       tBTA_MA_MPKT_STATUS multi_pkt_status,
                                       tBTA_MA_FRAC_DELIVER frac_deliver_status,
                                       UINT16 evt)
{
    tBTA_MSE_CI_GET_MSG  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_GET_MSG *)GKI_getbuf(sizeof(tBTA_MSE_CI_GET_MSG))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT4("bta_mse_ci_get_msg sess_id=%d, status=%d, multi-pkt status=%d frac=%d\n", 
                          mas_session_id, 
                          status,
                          multi_pkt_status,
                          frac_deliver_status);
#endif
        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        p_evt->filled_buff_size = filled_buff_size;
        p_evt->multi_pkt_status = multi_pkt_status;
        p_evt->frac_deliver_status = frac_deliver_status;
        bta_sys_sendmsg(p_evt);
    }
}
/*******************************************************************************
**
** Function         bta_mse_ci_set_msg_delete_status
**
** Description      This function is called in response to the
**                  bta_mse_co_set_msg_delete_status call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK if operation is successful.
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_mse_ci_set_msg_delete_status(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                     tBTA_MA_STATUS status, 
                                                     UINT16 evt)
{
    tBTA_MSE_CI_DEL_MSG  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_DEL_MSG *)GKI_getbuf(sizeof(tBTA_MSE_CI_DEL_MSG))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT3("bta_mse_ci_del_msg sess_id=%d, status=%d evt=%d\n",  
                          mas_session_id, status, evt);
#endif
        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ci_push_msg
**
** Description      This function is called in response to the
**                  bta_mse_co_push_msg call-out function.
**
** Parameters       mas_session_id - MAS session ID
**                  status - BTA_MA_STATUS_OK if the message upload is successful.
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  last_packet - last packet of a multi-packet message
**                  handle - message handle for the uploaded message if
**                           status is BTA_MA_OK and last_packet is TRUE
** Returns          void
**                   
*******************************************************************************/
BTA_API extern void bta_mse_ci_push_msg(tBTA_MA_SESS_HANDLE  mas_session_id,
                                        tBTA_MA_STATUS status, 
                                        BOOLEAN last_packet,
                                        tBTA_MA_MSG_HANDLE handle,
                                        UINT16 evt)
{
    tBTA_MSE_CI_PUSH_MSG  *p_evt;

    if ((p_evt = (tBTA_MSE_CI_PUSH_MSG *)GKI_getbuf(sizeof(tBTA_MSE_CI_PUSH_MSG))) != NULL)
    {
#if BTA_MSE_DEBUG == TRUE
        APPL_TRACE_EVENT4("bta_mse_ci_push_msg sess_id=%d, status=%d, last_pkt=%d evt=%d\n", 
                          mas_session_id, 
                          status,
                          last_packet,
                          evt);  
#endif

        p_evt->hdr.event = evt;
        p_evt->mas_session_id = mas_session_id;
        p_evt->status = status;
        memcpy(p_evt->handle,
               handle,
               sizeof(tBTA_MA_MSG_HANDLE));
        p_evt->last_packet = last_packet;
        bta_sys_sendmsg(p_evt);
    }
}


