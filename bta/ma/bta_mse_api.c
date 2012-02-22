/*****************************************************************************
**
**  Name:           bta_mse_api.c
**
**  Description:    This is the implementation of the API for the Message
**                  Acess Server subsystem of BTA, Broadcom Corp's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2009-2011 Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bd.h"
#include "bta_ma_def.h"
#include "bta_mse_api.h"
#include "bta_fs_api.h"
#include "bta_mse_int.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
static const tBTA_SYS_REG bta_mse_reg =
{
    bta_mse_hdl_event,
    BTA_MseDisable
};

/*******************************************************************************
**
** Function         BTA_MseEnable
**
** Description      Enable the MSE subsystems.  This function must be
**                  called before any other functions in the MSE API are called.
**                  When the enable operation is completed the callback function
**                  will be called with an BTA_MSE_ENABLE_EVT event.
**
** Parameters       p_cback - MSE event call back function
**                  app_id  - Application ID                  
**
** Returns          void
**
*******************************************************************************/
void BTA_MseEnable(tBTA_MSE_CBACK *p_cback, UINT8 app_id)
{
    tBTA_MSE_API_ENABLE *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_MSE, &bta_mse_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_MSE_API_ENABLE *)GKI_getbuf(sizeof(tBTA_MSE_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event    = BTA_MSE_API_ENABLE_EVT;
        p_buf->p_cback      = p_cback;
        p_buf->app_id       = app_id;

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseDisable
**
** Description     Disable the MSE subsystem.                 
**
** Returns          void
**
*******************************************************************************/
void BTA_MseDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_MSE);
    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_MSE_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseStart
**
** Description      Start a MA server on the MSE 
**
**                 
** Parameters       mas_inst_id     - MAS instance ID
**                  sec_mask        - Security Setting Mask 
**                                     MSE always enables 
**                                    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT) 
**                  p_service_name  - Pointer to service name
**                  p_root_path     - Pointer to root path 
**                                    (one level above telecom)
**                  sup_msg_type    - supported message type(s)
**
** Returns          void
**
*******************************************************************************/
void BTA_MseStart( tBTA_MA_INST_ID   mas_inst_id,
                   tBTA_SEC sec_mask, const char *p_service_name,
                   const char *p_root_path,  
                   tBTA_MA_MSG_TYPE sup_msg_type)
{
    tBTA_MSE_API_START *p_buf;

    if ((p_buf = (tBTA_MSE_API_START *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_START) +
                                                           p_bta_fs_cfg->max_path_len + 1))) != NULL)
    {
        p_buf->p_root_path = (char *)(p_buf + 1);
        p_buf->hdr.event        = BTA_MSE_API_START_EVT;
        p_buf->mas_inst_id      = mas_inst_id;
        p_buf->sec_mask         = (sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
        p_buf->sup_msg_type     = sup_msg_type;


        if (p_service_name)
        {
            BCM_STRNCPY_S(p_buf->servicename, sizeof(p_buf->servicename), p_service_name, BTA_SERVICE_NAME_LEN);
            p_buf->servicename[BTA_SERVICE_NAME_LEN] = '\0';
        }
        else
            p_buf->servicename[0]= '\0';

        if (p_root_path)
        {
            BCM_STRNCPY_S(p_buf->p_root_path, p_bta_fs_cfg->max_path_len+1, p_root_path, p_bta_fs_cfg->max_path_len);
            p_buf->p_root_path[p_bta_fs_cfg->max_path_len] = '\0';
        }
        else
            p_buf->p_root_path[0] = '\0';

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseStop
**
** Description      Stop a MA server on the MSE  
**
** Parameters       mas_instance_id - MAS Instance ID       
**
** Returns          void
**
*******************************************************************************/
void BTA_MseStop (tBTA_MA_INST_ID mas_instance_id)
{
    tBTA_MSE_API_STOP *p_buf;

    if ((p_buf = (tBTA_MSE_API_STOP *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_STOP)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_STOP_EVT;
        p_buf->mas_inst_id      = mas_instance_id;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseClose
**
** Description      Close all MAS sessions on the specified MAS Instance ID
**
** Parameters       mas_instance_id - MAS Inatance ID    
**
** Returns          void
**
*******************************************************************************/
void BTA_MseClose(tBTA_MA_INST_ID mas_instance_id)
{

    tBTA_MSE_API_CLOSE *p_buf;

    if ((p_buf = (tBTA_MSE_API_CLOSE *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_CLOSE)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_CLOSE_EVT;
        p_buf->mas_instance_id   = mas_instance_id;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseMaClose
**
** Description      Close a MAS sessions on the specified BD address
**
** Parameters       bd_addr         - remote BD's address 
**                  mas_instance_id - MAS Inatance ID    
**
** Returns          void
**
*******************************************************************************/
void BTA_MseMaClose(BD_ADDR bd_addr, tBTA_MA_INST_ID mas_instance_id)
{
    tBTA_MSE_API_MA_CLOSE *p_buf;

    if ((p_buf = (tBTA_MSE_API_MA_CLOSE *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_MA_CLOSE)))) != NULL)
    {
        memset(p_buf, 0, sizeof(tBTA_MSE_API_MA_CLOSE));

        p_buf->hdr.event        = BTA_MSE_API_MA_CLOSE_EVT;
        bdcpy(p_buf->bd_addr, bd_addr); 
        p_buf->mas_instance_id   = mas_instance_id;
        bta_sys_sendmsg(p_buf);
    }
}
/*******************************************************************************
**
** Function         BTA_MseMnClose
**
** Description      Close a MN session
**
** Parameters       bd_addr - remote BD's address 
**
** Returns          void
**
*******************************************************************************/
void BTA_MseMnClose(BD_ADDR bd_addr)
{
    tBTA_MSE_API_MN_CLOSE *p_buf;

    if ((p_buf = (tBTA_MSE_API_MN_CLOSE *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_MN_CLOSE)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_MN_CLOSE_EVT;
        bdcpy(p_buf->bd_addr, bd_addr);  
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseAccessRsp
**
** Description      Send a response for the access request 
**   
** Parameters       mas_session_id  - MAS session ID  
**                  oper            - MAS operation type    
**                  access          - Access is allowed or not
**                  p_path          - pointer to a path if if the operation 
**                                    involves accessing a folder
** Returns          void
**
*******************************************************************************/
void BTA_MseAccessRsp(tBTA_MA_SESS_HANDLE mas_session_id, tBTA_MSE_OPER oper,
                      tBTA_MA_ACCESS_TYPE access, char *p_path)
{
    tBTA_MSE_API_ACCESSRSP *p_acc_rsp;
    UINT16                  path_len;

    /* If directory is specified set the length */
    path_len = (p_path && *p_path != '\0') ? (UINT16)(strlen(p_path) + 1): 1;

    if ((p_acc_rsp = (tBTA_MSE_API_ACCESSRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_ACCESSRSP)
                                                                   + path_len))) != NULL)
    {
        p_acc_rsp->mas_session_id   = mas_session_id;
        p_acc_rsp->rsp              = access;
        p_acc_rsp->oper             = oper;
        p_acc_rsp->p_path           = (char *)(p_acc_rsp + 1);
        if (p_path)
        {
            BCM_STRNCPY_S(p_acc_rsp->p_path, path_len, p_path, path_len-1);
            p_acc_rsp->p_path[path_len-1] = '\0';
        }
        else
            p_acc_rsp->p_path[0] = '\0';

        p_acc_rsp->hdr.event = BTA_MSE_API_ACCESSRSP_EVT;
        bta_sys_sendmsg(p_acc_rsp);
    }
}
/*******************************************************************************
**
** Function         BTA_MseUpdateInboxRsp
**
** Description      Send a response for the update inbox request      
**
**                 
** Parameters       mas_session_id  - MAS session ID  
**                  update_rsp      - update inbox is allowed or not    
**
** Returns          void
**
*******************************************************************************/
void BTA_MseUpdateInboxRsp(tBTA_MA_SESS_HANDLE mas_session_id,
                           tBTA_MSE_UPDATE_INBOX_TYPE update_rsp)
{
    tBTA_MSE_API_UPDINBRSP *p_buf;

    if ((p_buf = (tBTA_MSE_API_UPDINBRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_UPDINBRSP)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_UPD_IBX_RSP_EVT;
        p_buf->mas_session_id   = mas_session_id;
        p_buf->rsp              = update_rsp;
        bta_sys_sendmsg(p_buf);
    }
}
/*******************************************************************************
**
** Function         BTA_MseSetNotifRegRsp
**
** Description      Send a response for the set notification registration      
**                    
**                 
** Parameters       mas_session_id  - MAS session ID  
**                  set_notif_reg_rsp   - indicate whether the set notification    
**                                        registration is allowed or not   
**
** Returns          void
**
*******************************************************************************/
void BTA_MseSetNotifRegRsp(tBTA_MA_SESS_HANDLE mas_session_id,
                           tBTA_MSE_SET_NOTIF_REG_TYPE set_notif_reg_rsp)
{
    tBTA_MSE_API_SETNOTIFREGRSP *p_buf;

    if ((p_buf = (tBTA_MSE_API_SETNOTIFREGRSP *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_API_UPDINBRSP)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_SET_NOTIF_REG_RSP_EVT;
        p_buf->mas_session_id   = mas_session_id;
        p_buf->rsp              = set_notif_reg_rsp;
        bta_sys_sendmsg(p_buf);
    }
}
/*******************************************************************************
**
** Function         BTA_MseSendNotif
**
** Description      Send a Message notification report to all MCEs registered with
**                  the specified MAS instance ID 
**
** Parameters       mas_instance_id - MAS Instance ID  
**                  notif_type      - message notification type     
**                  handle          - message handle
**                  p_folder        - pointer to current folder
**                  p_old_folder    - pointer to older folder
**                  msg_type        - message type (E_MAIL, SMS_GSM, SMS_CDMA, MMS)
**                  except_bd_addr  - except to the MCE on this BD Address. 
**                                    (Note: notification will be not sent to 
**                                     this BD Addreess) 
**
** Returns          void
**  
*******************************************************************************/
void BTA_MseSendNotif(tBTA_MA_INST_ID mas_instance_id, 
                      tBTA_MSE_NOTIF_TYPE notif_type, 
                      tBTA_MA_MSG_HANDLE handle, 
                      char * p_folder, char *p_old_folder, 
                      tBTA_MA_MSG_TYPE msg_type,
                      BD_ADDR except_bd_addr)
{
    tBTA_MSE_MN_API_SEND_NOTIF *p_buf;
    UINT16              folder_len, old_folder_len;

    /* If directory is specified set the length */
    folder_len = (p_folder && *p_folder != '\0') ? (UINT16)(strlen(p_folder) + 1): 0;
    old_folder_len = (p_old_folder && *p_old_folder != '\0') ? (UINT16)(strlen(p_old_folder) + 1): 0;

    if ((p_buf = (tBTA_MSE_MN_API_SEND_NOTIF *)GKI_getbuf((UINT16) ((UINT16)sizeof(tBTA_MSE_MN_API_SEND_NOTIF) + 
                                                                    folder_len +  
                                                                    old_folder_len))) != NULL)
    {
        p_buf->hdr.event         = BTA_MSE_API_SEND_NOTIF_EVT;
        p_buf->mas_instance_id    = mas_instance_id;
        p_buf->notif_type        = notif_type;
        memcpy(p_buf->handle, handle, sizeof(tBTA_MA_MSG_HANDLE));

        if (folder_len)
        {
            p_buf->p_folder = (char *)(p_buf + 1);
            BCM_STRNCPY_S(p_buf->p_folder, folder_len+1, p_folder, folder_len);
        }
        else
            p_buf->p_folder = NULL;

        if (old_folder_len)
        {
            if (folder_len)
                p_buf->p_old_folder = p_buf->p_folder + folder_len;
            else
                p_buf->p_old_folder = (char *)(p_buf + 1);

            BCM_STRNCPY_S(p_buf->p_old_folder, old_folder_len+1, p_old_folder, old_folder_len);
        }
        else
            p_buf->p_old_folder = NULL;

        p_buf->msg_type          = msg_type;
        if (except_bd_addr != NULL)
        {
            bdcpy(p_buf->except_bd_addr, except_bd_addr); 
        }
        else
        {
            memset(p_buf->except_bd_addr, 0, sizeof(BD_ADDR));
        }
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_MseMnAbort
**
** Description      Abort the current OBEX multi-packt operation in MN
**
** Parameters       mas_instance_id - MAS Instance ID  
**
** Returns          void
**
*******************************************************************************/
void BTA_MseMnAbort(tBTA_MA_INST_ID mas_instance_id)
{
    tBTA_MSE_MN_API_ABORT *p_buf;

    if ((p_buf = (tBTA_MSE_MN_API_ABORT *)GKI_getbuf((UINT16)(sizeof(tBTA_MSE_MN_API_ABORT)))) != NULL)
    {
        p_buf->hdr.event        = BTA_MSE_API_MN_ABORT_EVT;
        p_buf->mas_instance_id  = mas_instance_id;
        bta_sys_sendmsg(p_buf);
    }
}


#endif /* BTA_MSE_INCLUDED */
