/*****************************************************************************
**
**  Name:           bta_ftc_api.c
**
**  Description:    This is the implementation of the API for the file
**                  transfer server subsystem of BTA, Widcomm's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
    
#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bta_fs_api.h"
#include "bta_ftc_int.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
static const tBTA_SYS_REG bta_ftc_reg =
{
    bta_ftc_hdl_event,
    BTA_FtcDisable
};

/*******************************************************************************
**
** Function         BTA_FtcEnable
**
** Description      Enable the file transfer client.  This function must be
**                  called before any other functions in the FTC API are called.
**                  When the enable operation is complete the callback function
**                  will be called with an BTA_FTC_ENABLE_EVT event.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcEnable(tBTA_FTC_CBACK *p_cback, UINT8 app_id)
{
    tBTA_FTC_API_ENABLE *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_FTC, &bta_ftc_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_FTC_API_ENABLE *)GKI_getbuf(sizeof(tBTA_FTC_API_ENABLE))) != NULL)
    {
        memset(p_buf, 0, sizeof(tBTA_FTC_API_ENABLE));

        p_buf->hdr.event = BTA_FTC_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        p_buf->app_id = app_id;

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcDisable
**
** Description      Disable the file transfer client.  If the client is currently
**                  connected to a peer device the connection will be closed.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_FTC);
    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_FTC_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcOpen
**
** Description      Open a connection to an FTP, PBAP, OPP or BIP server.  
**                  If parameter services is set to use both all services,
**                  the client will attempt to connect to the device using 
**                  FTP first and then PBAP, OPP, BIP.
**                  When the connection is open the callback function
**                  will be called with a BTA_FTC_OPEN_EVT.  If the connection
**                  fails or otherwise is closed the callback function will be
**                  called with a BTA_FTC_CLOSE_EVT.
**
**                  If the connection is opened with FTP profile and
**                  bta_ft_cfg.auto_file_list is TRUE , the callback
**                  function will be called with one or more BTA_FTC_LIST_EVT
**                  containing directory list information formatted in XML as
**                  described in the IrOBEX Spec, Version 1.2, section 9.1.2.3.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcOpen(BD_ADDR bd_addr, tBTA_SEC sec_mask, tBTA_SERVICE_MASK services,
                 BOOLEAN srm, UINT32 nonce)
{
    tBTA_FTC_API_OPEN *p_buf;

    if ((p_buf = (tBTA_FTC_API_OPEN *)GKI_getbuf(sizeof(tBTA_FTC_API_OPEN))) != NULL)
    {
        memset(p_buf, 0, sizeof(tBTA_FTC_API_OPEN));

        p_buf->hdr.event = BTA_FTC_API_OPEN_EVT;
        p_buf->services = services;
        p_buf->sec_mask = sec_mask;
        p_buf->srm = srm;
        p_buf->nonce = nonce;
        memcpy(p_buf->bd_addr, bd_addr, BD_ADDR_LEN);

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcSuspend
**
** Description      Suspend the current connection to the server.
**                  This is allowed only for the sessions created by 
**                  BTA_FtcConnect with nonce!=0
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcSuspend(void)
{
    tBTA_FTC_API_CLOSE  *p_buf;

    APPL_TRACE_DEBUG0("BTA_FtcSuspend");
    if ((p_buf = (tBTA_FTC_API_CLOSE *)GKI_getbuf(sizeof(tBTA_FTC_API_CLOSE))) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_API_CLOSE_EVT;
        p_buf->suspend = TRUE;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcClose
**
** Description      Close the current connection to the server.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcClose(void)
{
    tBTA_FTC_API_CLOSE  *p_buf;

    if ((p_buf = (tBTA_FTC_API_CLOSE *)GKI_getbuf(sizeof(tBTA_FTC_API_CLOSE))) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_API_CLOSE_EVT;
        p_buf->suspend = FALSE;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcCopyFile
**
** Description      Invoke a Copy action on the server.
**                  Create a copy of p_src and name it as p_dest
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcCopyFile(const char *p_src, const char *p_dest)
{
    tBTA_FTC_API_ACTION  *p_buf;
    UINT16                src_len = (p_src) ? strlen(p_src) : 0;
    UINT16                dest_len = (p_dest) ? strlen(p_dest) : 0;
    UINT16                total_len = sizeof(tBTA_FTC_API_ACTION) + src_len + dest_len + 2;

    APPL_TRACE_DEBUG2("BTA_FtcCopyFile src:%s, dest:%s", p_src, p_dest);

    if ((p_buf = (tBTA_FTC_API_ACTION *)GKI_getbuf( total_len)) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_API_ACTION_EVT;
        p_buf->action = BTA_FT_ACT_COPY;
        p_buf->p_src = (char *)(p_buf + 1);
        p_buf->p_dest = (char *)(p_buf->p_src + src_len + 1);
        /* copy the src name */
        if (p_src)
        {
            BCM_STRNCPY_S(p_buf->p_src, src_len+1, p_src, src_len);
            p_buf->p_src[src_len] = '\0';
        }
        else
            p_buf->p_src[0] = '\0';

        /* copy the dest name */
        if (p_dest)
        {
            BCM_STRNCPY_S(p_buf->p_dest, dest_len+1, p_dest, dest_len);
            p_buf->p_dest[dest_len] = '\0';
        }
        else
            p_buf->p_dest[0] = '\0';
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcMoveFile
**
** Description      Invoke a Move action on the server.
**                  Move/rename p_src to p_dest
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcMoveFile(const char *p_src, const char *p_dest)
{
    tBTA_FTC_API_ACTION  *p_buf;
    UINT16                src_len = (p_src) ? strlen(p_src) : 0;
    UINT16                dest_len = (p_dest) ? strlen(p_dest) : 0;
    UINT16                total_len = sizeof(tBTA_FTC_API_ACTION) + src_len + dest_len + 2;

    APPL_TRACE_DEBUG2("BTA_FtcMoveFile src:%s, dest:%s", p_src, p_dest);

    if ((p_buf = (tBTA_FTC_API_ACTION *)GKI_getbuf(total_len)) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_API_ACTION_EVT;
        p_buf->action = BTA_FT_ACT_MOVE;
        p_buf->p_src = (char *)(p_buf + 1);
        p_buf->p_dest = (char *)(p_buf->p_src + src_len + 1);

        /* copy the src name */
        if (p_src)
        {
            BCM_STRNCPY_S(p_buf->p_src, src_len+1, p_src, src_len);
            p_buf->p_src[src_len] = '\0';
        }
        else
            p_buf->p_src[0] = '\0';

        /* copy the dest name */
        if (p_dest)
        {
            BCM_STRNCPY_S(p_buf->p_dest, dest_len+1, p_dest, dest_len);
            p_buf->p_dest[dest_len] = '\0';
        }
        else
            p_buf->p_dest[0] = '\0';

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcSetPermission
**
** Description      Invoke a SetPermission action on the server.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcSetPermission(const char *p_src, UINT8 user, UINT8 group, UINT8 other)
{
    tBTA_FTC_API_ACTION  *p_buf;
    UINT16                src_len = (p_src) ? strlen(p_src) : 0;
    UINT16                total_len = sizeof(tBTA_FTC_API_ACTION) + src_len + 1;

    APPL_TRACE_DEBUG4("BTA_FtcSetPermission src:%s, user:0x%x, group:0x%x, other:0x%x",
        p_src, user, group, other);

    if ((p_buf = (tBTA_FTC_API_ACTION *)GKI_getbuf(total_len)) != NULL)
    {
        p_buf->hdr.event = BTA_FTC_API_ACTION_EVT;
        p_buf->action = BTA_FT_ACT_PERMISSION;
        p_buf->p_src = (char *)(p_buf + 1);

        /* copy the src name */
        if (p_src)
        {
            BCM_STRNCPY_S(p_buf->p_src, src_len+1, p_src, src_len);
            p_buf->p_src[src_len] = '\0';
        }
        else
            p_buf->p_src[0] = '\0';

        p_buf->permission[0] = user;
        p_buf->permission[1] = group;
        p_buf->permission[2] = other;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcPutFile
**
** Description      Send a file to the connected server.
**                  This function can only be used when the client is connected
**                  in FTP, OPP and BIP mode.
**
** Note:            File name is specified with a fully qualified path.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcPutFile(const char *p_name, tBTA_FTC_PARAM *p_param)
{
    tBTA_FTC_DATA *p_msg;  
    INT32          name_len = (INT32)((p_bta_fs_cfg->max_path_len + 1 + 3)/4)*4;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
                                        sizeof(tBTA_FTC_PARAM) + name_len))) != NULL)
    {
        p_msg->api_put.p_name = (char *)(p_msg + 1);

        if (p_name != NULL)
            BCM_STRNCPY_S(p_msg->api_put.p_name, name_len, p_name, p_bta_fs_cfg->max_path_len);
        else
            p_msg->api_put.p_name[0] = 0;
        
        if(p_param)
        {
            p_msg->api_put.p_param = (tBTA_FTC_PARAM *)(p_msg->api_put.p_name + name_len);
            memcpy(p_msg->api_put.p_param, p_param, sizeof(tBTA_FTC_PARAM));
        }
        else
            p_msg->api_put.p_param = NULL;

        p_msg->api_put.hdr.event = BTA_FTC_API_PUTFILE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcGetPhoneBook
**
** Description      Retrieve a PhoneBook from the peer device and copy it to the
**                  local file system.
**
**                  This function can only be used when the client is connected
**                  in PBAP mode.
**
** Note:            local file name is specified with a fully qualified path.
**                  Remote file name is absolute path in UTF-8 format.
**                  (telecom/pb.vcf or SIM1/telecom/pb.vcf).
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcGetPhoneBook(char *p_local_name, char *p_remote_name,
                         tBTA_FTC_FILTER_MASK filter, tBTA_FTC_FORMAT format,
                         UINT16 max_list_count, UINT16 list_start_offset)
{
    tBTA_FTC_DATA      *p_msg;
    tBTA_FTC_API_GET   *p_get; 
    tBTA_FTC_GET_PARAM *p_getp;
    UINT16            remote_name_length = (p_remote_name) ? strlen(p_remote_name) : 0;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
        (p_bta_fs_cfg->max_path_len + 1) + remote_name_length + 1))) != NULL)
    {
        p_get = &p_msg->api_get;
        p_get->p_param = (tBTA_FTC_GET_PARAM *)(p_msg + 1);
        p_getp = p_get->p_param;
        p_get->p_rem_name = (char *)(p_getp + 1);
        p_get->p_name = (char *)(p_get->p_rem_name + remote_name_length + 1);
        p_get->obj_type = BTA_FTC_GET_PB;
        
        /* copy the local name */
        if (p_local_name)
            BCM_STRNCPY_S(p_get->p_name, p_bta_fs_cfg->max_path_len + 1, p_local_name, p_bta_fs_cfg->max_path_len);
        else
            p_get->p_name[0] = '\0';
        
        /* copy remote name */
        if( p_remote_name)
            BCM_STRNCPY_S(p_get->p_rem_name, remote_name_length + 1, p_remote_name, remote_name_length);
        p_get->p_rem_name[remote_name_length] = '\0';

        p_getp->filter = filter;
        p_getp->format = format;
        p_getp->list_start_offset = list_start_offset;
        p_getp->max_list_count = max_list_count;

        p_get->hdr.event = BTA_FTC_API_GETFILE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcGetCard
**
** Description      Retrieve a vCard from the peer device and copy it to the
**                  local file system.
**
**                  This function can only be used when the client is connected
**                  in PBAP mode.
**
** Note:            local file name is specified with a fully qualified path.
**                  Remote file name is relative path in UTF-8 format.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcGetCard(char *p_local_name, char *p_remote_name,
                    tBTA_FTC_FILTER_MASK filter, tBTA_FTC_FORMAT format)
{
    tBTA_FTC_DATA      *p_msg;
    tBTA_FTC_API_GET   *p_get; 
    tBTA_FTC_GET_PARAM *p_getp;
    UINT16            remote_name_length = (p_remote_name) ? strlen(p_remote_name) : 0;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
        (p_bta_fs_cfg->max_path_len + 1) + remote_name_length + 1))) != NULL)
    {
        p_get = &p_msg->api_get;
        p_get->p_param = (tBTA_FTC_GET_PARAM *)(p_msg + 1);
        p_getp = p_get->p_param;
        p_get->p_rem_name = (char *)(p_getp + 1);
        p_get->p_name = (char *)(p_get->p_rem_name + remote_name_length + 1);
        p_get->obj_type = BTA_FTC_GET_CARD;
        
        /* copy the local name */
        if (p_local_name)
        {
            BCM_STRNCPY_S(p_get->p_name, p_bta_fs_cfg->max_path_len + 1, p_local_name, p_bta_fs_cfg->max_path_len);
            p_get->p_name[p_bta_fs_cfg->max_path_len] = '\0';
        }
        else
            p_get->p_name[0] = '\0';
        
        /* copy remote name */
        if( p_remote_name)
            BCM_STRNCPY_S(p_get->p_rem_name, remote_name_length+1, p_remote_name, remote_name_length);
        p_get->p_rem_name[remote_name_length] = '\0';

        p_getp->filter = filter;
        p_getp->format = format;

        p_get->hdr.event = BTA_FTC_API_GETFILE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcGetFile
**
** Description      Retrieve a file from the peer device and copy it to the
**                  local file system.
**
**                  This function can only be used when the client is connected
**                  in FTP mode.
**
** Note:            local file name is specified with a fully qualified path.
**                  Remote file name is specified in UTF-8 format.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcGetFile(char *p_local_name, char *p_remote_name)
{
    tBTA_FTC_DATA    *p_msg;
    tBTA_FTC_API_GET *p_get;
    UINT16            remote_name_length = (p_remote_name) ? strlen(p_remote_name) : 0;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
        (p_bta_fs_cfg->max_path_len + 1) + remote_name_length + 1))) != NULL)
    {
        p_get = &p_msg->api_get;
        p_get->obj_type = BTA_FTC_GET_FILE;
        p_get->p_param = NULL;
        p_get->p_rem_name = (char *)(p_msg + 1);
        p_get->p_name = (char *)(p_msg->api_get.p_rem_name + remote_name_length + 1);
        
        /* copy the local name */
        if (p_local_name)
        {
            BCM_STRNCPY_S(p_get->p_name, p_bta_fs_cfg->max_path_len + 1, p_local_name, p_bta_fs_cfg->max_path_len);
            p_get->p_name[p_bta_fs_cfg->max_path_len] = '\0';
        }
        else
            p_get->p_name[0] = '\0';
        
        /* copy remote name */
        if( p_remote_name)
            BCM_STRNCPY_S(p_get->p_rem_name, remote_name_length+1, p_remote_name, remote_name_length);
        p_get->p_rem_name[remote_name_length] = '\0';
        

        p_get->hdr.event = BTA_FTC_API_GETFILE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcChDir
**
** Description      Change directory on the peer device.
**
**                  This function can only be used when the client is connected
**                  in FTP and PBAP mode.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcChDir(char *p_dir, tBTA_FTC_FLAG flag)
{
    tBTA_FTC_DATA      *p_msg;
    tBTA_FTC_API_CHDIR *p_chdir;
    UINT16              dir_len;

    /* If directory is specified set the length */
    dir_len = (p_dir && *p_dir != '\0') ? (UINT16)(strlen(p_dir) + 1): 0;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
                                             dir_len))) != NULL)
    {
        p_chdir = &p_msg->api_chdir;
        p_chdir->flag = flag;
        if (dir_len)
        {
            p_chdir->p_dir = (char *)(p_msg + 1);
            BCM_STRNCPY_S(p_chdir->p_dir, dir_len, p_dir, dir_len);
        }
        else    /* Setting to root directory OR backing up a directory */
            p_chdir->p_dir = NULL;

        p_chdir->hdr.event = BTA_FTC_API_CHDIR_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcAuthRsp
**
** Description      Sends a response to an OBEX authentication challenge to the
**                  connected OBEX server. Called in response to an BTA_FTC_AUTH_EVT
**                  event.
**
** Note:            If the "userid_required" is TRUE in the BTA_FTC_AUTH_EVT event,
**                  then p_userid is required, otherwise it is optional.
**
**                  p_password  must be less than BTA_FTC_MAX_AUTH_KEY_SIZE (16 bytes)
**                  p_userid    must be less than OBX_MAX_REALM_LEN (defined in target.h)
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcAuthRsp (char *p_password, char *p_userid)
{
    tBTA_FTC_API_AUTHRSP *p_auth_rsp;

    if ((p_auth_rsp = (tBTA_FTC_API_AUTHRSP *)GKI_getbuf(sizeof(tBTA_FTC_API_AUTHRSP))) != NULL)
    {
        memset(p_auth_rsp, 0, sizeof(tBTA_FTC_API_AUTHRSP));

        p_auth_rsp->hdr.event = BTA_FTC_API_AUTHRSP_EVT;

        if (p_password)
        {
            p_auth_rsp->key_len = strlen(p_password);
            if (p_auth_rsp->key_len > BTA_FTC_MAX_AUTH_KEY_SIZE)
                p_auth_rsp->key_len = BTA_FTC_MAX_AUTH_KEY_SIZE;
            memcpy(p_auth_rsp->key, p_password, p_auth_rsp->key_len);
        }

        if (p_userid)
        {
            p_auth_rsp->userid_len = strlen(p_userid);
            if (p_auth_rsp->userid_len > OBX_MAX_REALM_LEN)
                p_auth_rsp->userid_len = OBX_MAX_REALM_LEN;
            memcpy(p_auth_rsp->userid, p_userid, p_auth_rsp->userid_len);
        }

        bta_sys_sendmsg(p_auth_rsp);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcListCards
**
** Description      Retrieve a directory listing from the peer device. 
**                  When the operation is complete the callback function will
**                  be called with one or more BTA_FTC_LIST_EVT events
**                  containing directory list information formatted as described
**                  in the PBAP Spec, Version 0.9, section 3.1.6.
**                  This function can only be used when the client is connected
**                  to a peer device.
**
**                  This function can only be used when the client is connected
**                  in PBAP mode.
**
** Parameters       p_dir - Name of directory to retrieve listing of.
**
** Returns          void
**
*******************************************************************************/

void BTA_FtcListCards(char *p_dir, tBTA_FTC_ORDER order, char *p_value,
                      tBTA_FTC_ATTR attribute, UINT16 max_list_count,
                      UINT16 list_start_offset)
{
    tBTA_FTC_DATA      *p_msg;
    tBTA_FTC_API_LIST  *p_list; 
    tBTA_FTC_LST_PARAM *p_param; 
    UINT16              dir_len = (p_dir == NULL) ? 0 : strlen(p_dir) ;
    UINT16              value_len = (p_value == NULL) ? 0 : strlen(p_value) ;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
                                             sizeof(tBTA_FTC_LST_PARAM) +
                                             dir_len + value_len + 2))) != NULL)
    {
        p_list = &p_msg->api_list;
        p_list->p_param       = (tBTA_FTC_LST_PARAM *)(p_msg + 1);
        p_param = p_list->p_param;
        p_list->p_dir = (char *)(p_param + 1);
        p_param->order               = order;
        p_param->attribute           = attribute;
        p_param->max_list_count      = max_list_count;
        p_param->list_start_offset   = list_start_offset;

        if (dir_len)
            BCM_STRNCPY_S(p_list->p_dir, dir_len+1, p_dir, dir_len);
        p_list->p_dir[dir_len] = 0;

        if (value_len)
        {
            p_param->p_value = (char *)(p_list->p_dir + dir_len + 1);
            BCM_STRNCPY_S(p_param->p_value, value_len+1, p_value, value_len);
            p_param->p_value[value_len] = 0;
        }
        else
            p_param->p_value = NULL;

        p_list->hdr.event = BTA_FTC_API_LISTDIR_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcListDir
**
** Description      Retrieve a directory listing from the peer device. 
**                  When the operation is complete the callback function will
**                  be called with one or more BTA_FTC_LIST_EVT events
**                  containing directory list information formatted as described
**                  in the IrOBEX Spec, Version 1.2, section 9.1.2.3.
**
**                  This function can only be used when the client is connected
**                  in FTP mode.
**
** Parameters       p_dir - Name of directory to retrieve listing of.  If NULL,
**                          the current working directory is retrieved.
**
** Returns          void
**
*******************************************************************************/

void BTA_FtcListDir(char *p_dir)
{
    tBTA_FTC_DATA      *p_msg;
    tBTA_FTC_API_LIST  *p_list; 
    UINT16              dir_len = (p_dir == NULL) ? 0 : strlen(p_dir) ;

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
                                             dir_len + 1))) != NULL)
    {
        p_list = &p_msg->api_list;
        p_list->p_dir = (char *)(p_msg + 1);
        p_list->p_param = NULL;

        if (dir_len)
            BCM_STRNCPY_S(p_list->p_dir, dir_len+1, p_dir, dir_len);

        p_list->p_dir[dir_len] = 0;

        p_list->hdr.event = BTA_FTC_API_LISTDIR_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcMkDir
**
** Description      Create a directory on the peer device. When the operation is
**                  complete the status is returned with the BTA_FTC_MKDIR_EVT
**                  event.
**
**                  This function can only be used when the client is connected
**                  in FTP mode.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcMkDir(char *p_dir)
{
    tBTA_FTC_API_MKDIR  *p_mkdir;
    UINT16              len = (p_dir == NULL) ? 0: strlen(p_dir);

    if ((p_mkdir = (tBTA_FTC_API_MKDIR *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_API_MKDIR) +
        len + 1))) != NULL)
    {
        p_mkdir->p_dir = (char *)(p_mkdir + 1);

        if (len > 0)
            BCM_STRNCPY_S(p_mkdir->p_dir, len+1, p_dir, len);

        p_mkdir->p_dir[len] = 0;

        p_mkdir->hdr.event = BTA_FTC_API_MKDIR_EVT;
        bta_sys_sendmsg(p_mkdir);
    }
}

/*******************************************************************************
**
** Function         BTA_FtcRemove
**
** Description      Remove a file or directory on the peer device.  When the
**                  operation is complete the status is returned with the
**                  BTA_FTC_REMOVE_EVT event.
**
**                  This function can only be used when the client is connected
**                  in FTP mode.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcRemove(char *p_name)

{
    tBTA_FTC_DATA           *p_msg;
    tBTA_FTC_API_REMOVE     *p_remove;
    UINT16                  len = (p_name == NULL) ? 0 :strlen(p_name);

    if ((p_msg = (tBTA_FTC_DATA *)GKI_getbuf((UINT16)(sizeof(tBTA_FTC_DATA) +
                                             len + 1))) != NULL)    
    {
        p_remove = &p_msg->api_remove;
        p_remove->p_name = (char *)(p_msg +1);
        if(len)
            BCM_STRNCPY_S(p_remove->p_name, len+1, p_name, len);
 
        p_remove->p_name[len] = 0;

        p_remove->hdr.event = BTA_FTC_API_REMOVE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}




/*******************************************************************************
**
** Function         BTA_FtcAbort
**
** Description      Aborts any active Put or Get file operation.
**
** Returns          void
**
*******************************************************************************/
void BTA_FtcAbort(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_FTC_API_ABORT_EVT;
        bta_sys_sendmsg(p_buf);
    }
}
#endif /* BTA_FT_INCLUDED */
