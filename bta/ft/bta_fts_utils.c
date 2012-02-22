/*****************************************************************************
**
**  Name:           bta_fts_utils.c
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
#include "bta_fs_api.h"
#include "bta_fts_int.h"
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
*  Macros for FTS 
*******************************************************************************/
#define BTA_FTS_XML_EOL              "\n"
#define BTA_FTS_FOLDER_LISTING_START ( "<?xml version=\"1.0\"?>\n" \
                                   "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n" \
                                   "<folder-listing version=\"1.0\">\n" )

#define BTA_FTS_FOLDER_LISTING_END   ( "</folder-listing>" )
#define BTA_FTS_PARENT_FOLDER        (" <parent-folder/>\n")

#define BTA_FTS_FILE_ELEM            "file"
#define BTA_FTS_FOLDER_ELEM          "folder"
#define BTA_FTS_NAME_ATTR            "name"
#define BTA_FTS_SIZE_ATTR            "size"
#define BTA_FTS_TYPE_ATTR            "type"
#define BTA_FTS_MODIFIED_ATTR        "modified"
#define BTA_FTS_CREATED_ATTR         "created"
#define BTA_FTS_ACCESSED_ATTR        "accessed"
#define BTA_FTS_USER_PERM_ATTR       "user-perm"

/*******************************************************************************
*  Exported Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_fts_getdirlist
**
** Description      Processes the retrieval of a directory listing.
**
** Parameters       p_pkt - Pointer to the OBX Get request
**                  name directory to list.
**
**                  
** Returns          UINT8 - OBX response code.  OBX_RSP_OK if initiated.
**
*******************************************************************************/
void bta_fts_getdirlist(char *p_name)
{
    tBTA_FTS_CB      *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FTS_DIRLIST *p_dir = &p_cb->dir;
    UINT16            temp_len;
    BOOLEAN           is_dir;
    UINT8             rsp_code = OBX_RSP_OK;
    
    if (!p_cb->p_path)
        p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1));
    if (p_cb->p_path)
    {
        /* If not specified, use the current work directory */
        if (!p_name || p_name[0] == '\0')
        {
            p_cb->p_path[p_bta_fs_cfg->max_path_len] = 0;
            BCM_STRNCPY_S(p_cb->p_path, (p_bta_fs_cfg->max_path_len + 1), p_cb->p_workdir, p_bta_fs_cfg->max_path_len);
        }
        else
        {
            if ((strlen(p_cb->p_workdir) + strlen(p_name) + 2) <= p_bta_fs_cfg->max_path_len)
            {
                sprintf(p_cb->p_path, "%s%c%s", p_cb->p_workdir,
                        p_bta_fs_cfg->path_separator, p_name);

                /* Make sure the Name is a directory and accessible */
                if (((bta_fs_co_access(p_cb->p_path, BTA_FS_ACC_EXIST,
                                       &is_dir, p_cb->app_id))!= BTA_FS_CO_OK)
                                       || !is_dir)
                    rsp_code = OBX_RSP_NOT_FOUND;
            }
            else
                rsp_code = OBX_RSP_BAD_REQUEST;
        }
        
        /* Build the listing */
        if (rsp_code == OBX_RSP_OK)
        {
            if (!(strcmp(p_cb->p_path, p_cb->p_rootpath)))
                p_dir->is_root = TRUE;
            else
                p_dir->is_root = FALSE;

            p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle, p_cb->peer_mtu);
            
            if (!p_dir->p_entry)
            {
                /* Allocate enough space for the structure and the file name */
                if ((p_dir->p_entry = (tBTA_FS_DIRENTRY *)
                        GKI_getbuf((UINT16)(sizeof(tBTA_FS_DIRENTRY) +
                                    p_bta_fs_cfg->max_file_len + 1))) != NULL)
                    p_dir->p_entry->p_name = (char *)(p_dir->p_entry + 1);
            }
            
            if (p_dir->p_entry && p_obx->p_pkt)
            {
                /* Add the start of the Body Header */
                p_obx->offset = 0;
                p_obx->bytes_left = 0;
                p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

                /* Is this a new request or continuation? */
                if ((p_cb->obx_oper == FTS_OP_NONE))
                {
                    p_cb->obx_oper = FTS_OP_LISTING;
                    APPL_TRACE_EVENT1("FTS List Directory: Name [%s]", p_cb->p_path);

                    temp_len = strlen(BTA_FTS_FOLDER_LISTING_START);

                    /* Add the beginning label of http */
                    memcpy(p_obx->p_start, BTA_FTS_FOLDER_LISTING_START, temp_len);
                    p_obx->bytes_left -= (UINT16)(temp_len + strlen(BTA_FTS_FOLDER_LISTING_END));
                    p_obx->offset += temp_len;

                    /* Add the parent directory if not the root */
                    if (strcmp(p_cb->p_path, p_cb->p_rootpath))
                    {
                        temp_len = strlen(BTA_FTS_PARENT_FOLDER);
                        memcpy(p_obx->p_start + p_obx->offset,
                               BTA_FTS_PARENT_FOLDER, temp_len);
                        p_obx->bytes_left -= temp_len;
                        p_obx->offset += temp_len;
                    }

                    p_cb->cout_active = TRUE;
                    bta_fs_co_getdirentry (p_cb->p_path, TRUE, p_dir->p_entry,
                                           BTA_FTS_CI_DIRENTRY_EVT, p_cb->app_id);

                    /* List is not complete, so don't send the response yet */
                    rsp_code = OBX_RSP_PART_CONTENT;
                }
                else /* Add the entry previously retrieved */
                    rsp_code = bta_fts_add_list_entry();
            }
            else
                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
        }
    }
    else    /* Error occurred */
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    /* Response goes out if complete or error occurred */
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_fts_end_of_list(rsp_code);
}

/*******************************************************************************
**
** Function         bta_fts_proc_get_file
**
** Description      Processes a Get File Operation.
**                  If first OBX request, the file is opened, otherwise if it is
**                  a continuation the next read is initiated.
**
** Parameters       p_pkt - Pointer to the OBX Get request
**                  name of file to read.
**
**                  
** Returns          void
**
*******************************************************************************/
void bta_fts_proc_get_file(char *p_name)
{
    tBTA_FTS_CB      *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code = OBX_RSP_INTRNL_SRVR_ERR;

    p_obx->offset = 0;

    /* Allocate an OBX packet */
    if ((p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
            p_cb->peer_mtu)) != NULL)
    {
        /* Is this a new request or continuation? */
        if ((p_cb->obx_oper == FTS_OP_NONE))
        {
            /* Validate the name */
            if (p_name)
            {
                if ((p_cb->p_path =
                    (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
                {
                    /* Build a fully qualified path */
                    if ((strlen(p_cb->p_workdir) + strlen(p_name) + 2)
                        <= p_bta_fs_cfg->max_path_len)
                    {
                        rsp_code = OBX_RSP_OK;
                        sprintf(p_cb->p_path, "%s%c%s", p_cb->p_workdir,
                            p_bta_fs_cfg->path_separator, p_name);
                        
                        APPL_TRACE_EVENT1("FTS GET FILE: Name [%s]", p_cb->p_path);
                        
                        p_cb->obx_oper = FTS_OP_GET_FILE;

                        /* Notify the application that a get file has been requested */
                        bta_fts_req_app_access (BTA_FT_OPER_GET, p_cb);
                    }
                }
            }
        }
        else    /* Continue reading from the file */
        {
            /* Add the start of the Body Header */
            p_obx->bytes_left = 0;
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

            rsp_code = OBX_RSP_OK;
            p_cb->cout_active = TRUE;
            bta_fs_co_read(p_cb->fd, &p_obx->p_start[p_obx->offset],
                           p_obx->bytes_left, BTA_FTS_CI_READ_EVT, 0, p_cb->app_id);
        }
    }
    if (rsp_code != OBX_RSP_OK)
        bta_fts_get_file_rsp(rsp_code, 0);
}

/*******************************************************************************
**
** Function         bta_fts_proc_put_file
**
** Description      Processes a Put File Operation.
**                  Initiates the opening of a file for writing, or continues 
**                  with a new Obx packet of data (continuation).
**
** Parameters       p_pkt - Pointer to the OBX Put request
**                  name of file to write out.
**
**                  
** Returns          void
**
*******************************************************************************/
void bta_fts_proc_put_file(BT_HDR *p_pkt, char *p_name, BOOLEAN final_pkt, UINT8 oper)
{
    tBTA_FTS_CB      *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FS_CO_STATUS status;
    UINT8             rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    UINT8             num_hdrs;
    BOOLEAN           is_dir;
    BOOLEAN           endpkt;
    
    p_obx->final_pkt = final_pkt;
    p_obx->p_pkt = p_pkt;
    p_obx->offset = 0;  /* Initial offset into OBX data */
    APPL_TRACE_DEBUG2("bta_fts_proc_put_file len:%d p_pkt->offset:%d", p_pkt->len, p_pkt->offset);

    /* Is this a new request or continuation? */
    if ((p_cb->obx_oper == FTS_OP_NONE))
    {
        /* See if the folder permissions are writable in the current folder */
        if (((status = bta_fs_co_access(p_cb->p_workdir, BTA_FS_ACC_RDWR,
            &is_dir, p_cb->app_id)) == BTA_FS_CO_OK) && is_dir)
        {
            /* Initialize the start of data and length */
            if ((p_cb->p_path =
                (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
            {
                /* Build a fully qualified path */
                if ((strlen(p_cb->p_workdir) + strlen(p_name) + 2)
                    <= p_bta_fs_cfg->max_path_len)
                {
                    /* Read the file length if header exists */
                    if (!OBX_ReadLengthHdr(p_pkt, &p_cb->file_length))
                        p_cb->file_length = BTA_FS_LEN_UNKNOWN;

                    rsp_code = OBX_RSP_PART_CONTENT;  /* Do not send OBX response yet */
                    sprintf(p_cb->p_path, "%s%c%s", p_cb->p_workdir,
                        p_bta_fs_cfg->path_separator, p_name);
                    
                    APPL_TRACE_DEBUG2("FTS PUT FILE: Name [%s], Length = 0x%0x (0 = n/a)",
                                      p_cb->p_path, p_cb->file_length);
                    
                    p_cb->obx_oper = FTS_OP_PUT_FILE;

                    /* Get permission before proceeding */
                    bta_fts_req_app_access(BTA_FT_OPER_PUT, p_cb);
                }
            }
        }
        else
            rsp_code = OBX_RSP_UNAUTHORIZED;
    }
    else    /* Continue writing to the open file */
    {
        /* Read in start of body if there is a body header */
        num_hdrs = OBX_ReadBodyHdr(p_obx->p_pkt, &p_obx->p_start,
                                   &p_obx->bytes_left, &endpkt);
        if (num_hdrs == 1)
        {
            if (p_obx->bytes_left)
            {
                rsp_code = OBX_RSP_PART_CONTENT;   /* Do not send OBX response yet */
                p_cb->cout_active = TRUE;
                bta_fs_co_write(p_cb->fd, &p_obx->p_start[p_obx->offset],
                    p_obx->bytes_left, BTA_FTS_CI_WRITE_EVT, 0, p_cb->app_id);
            }
            else
            {
                rsp_code = OBX_RSP_OK;
            }

        }
        else if (oper & FTS_OP_RESUME)
        {
            rsp_code = OBX_RSP_OK;
        }
        else
        {
            bta_fts_clean_getput(p_cb, TRUE);
            rsp_code = OBX_RSP_BAD_REQUEST;
        }
    }
    
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_fts_put_file_rsp(rsp_code);
}

/*******************************************************************************
**
** Function         bta_fts_mkdir
**
** Description      make a new directory and sets the new path to this directory
**                  if successful.
**
** Parameters       p_pkt - Pointer to the OBX packet
**                  
** Returns          UINT8 - OBX response code
**
*******************************************************************************/
UINT8 bta_fts_mkdir(BT_HDR *p_pkt, tBTA_FT_OPER *p_op)
{
    tBTA_FTS_CB         *p_cb = &bta_fts_cb;
    tBTA_FS_CO_STATUS    status = BTA_FS_CO_FAIL;
    UINT8                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    char                *p_newpath;
    char                *p_name;
    BOOLEAN              is_dir;
    
    p_newpath = p_cb->p_path;
    p_name = p_cb->p_name;
    
    /* the name of the directory being created */
    if (p_cb->p_name && p_cb->p_name[0])
    {
        /* Make sure the new path is not too big */
        if ((strlen(p_name) + strlen(p_cb->p_workdir) + 1) <= p_bta_fs_cfg->max_path_len)
        {
            /* create a temporary path for creation attempt */
            sprintf(p_newpath, "%s%c%s", p_cb->p_workdir,
                    p_bta_fs_cfg->path_separator, p_name);
            
            /* If the directory already exists, we're done */
            if (((status = bta_fs_co_access(p_newpath, BTA_FS_ACC_EXIST,
                &is_dir, p_cb->app_id)) == BTA_FS_CO_OK) && is_dir)
            {
                /* If directory exists, skip mkdir and just issue chdir- Note OBX_RSP_CONTINUE used internally by callee */
                 rsp_code = OBX_RSP_CONTINUE;
            }
            
            /* See if the folder permissions are writable in the current folder */
            else if (((status = bta_fs_co_access(p_cb->p_workdir, BTA_FS_ACC_RDWR,
                        &is_dir, p_cb->app_id)) == BTA_FS_CO_OK))
            {
                *p_op = BTA_FT_OPER_MK_DIR;
            }
            else if (status == BTA_FS_CO_EACCES)
                rsp_code = OBX_RSP_UNAUTHORIZED;  /* Read only folder, cannot create a new one */
        }
        else
            APPL_TRACE_WARNING0 ("bta_fts_mkdir: path too long!!!");
    }
    else
        rsp_code = OBX_RSP_BAD_REQUEST;
        
    
    APPL_TRACE_DEBUG2("bta_fts_mkdir: co_status [%d], obx_rsp_code [0x%02x]",
        status, rsp_code);
    
    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_fts_chdir
**
** Description      Changes the current path to the specified directory.
**
** Parameters       p_pkt - Pointer to the OBX packet
**                  backup_flag - if TRUE, path adjusted up one level.
**                  
** Returns          UINT8 - OBX response code
**
*******************************************************************************/
UINT8 bta_fts_chdir(BT_HDR *p_pkt, BOOLEAN backup_flag, tBTA_FT_OPER *p_op)
{
    tBTA_FTS_CB *p_cb = &bta_fts_cb;
    char        *p_path = p_cb->p_path;
    char        *p_name = p_cb->p_name;
    char        *p_workdir = p_cb->p_workdir;
    UINT8        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    BOOLEAN      is_dir;          
    
    if (!backup_flag)
    {
        
        /* If No Name header, or empty name header, set to root path */
        if (p_name == NULL || (p_name && p_name[0] == '\0'))
        {
            BCM_STRNCPY_S(p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
            p_workdir[p_bta_fs_cfg->max_path_len] = '\0';

            rsp_code = OBX_RSP_OK;
            APPL_TRACE_DEBUG0("FTS: Setting current path to ROOT");
        }
// btla-specific ++
	#if defined (FTS_REJECT_INVALID_OBEX_SET_PATH_REQ) && (FTS_REJECT_INVALID_OBEX_SET_PATH_REQ == TRUE)
		/* Reject invalid OBEX set path reqeust - DOS or unix/linux like change directory*/
		else if(strncmp("/", p_name, 1) == 0 || strncmp("..", p_name, 2) == 0)
		{
			APPL_TRACE_ERROR0("FTS: Rejecting invalid chdir request start with / or ..");
			rsp_code = OBX_RSP_NOT_FOUND;
		}
	#endif
// btla-specific --
        /* Make sure the new path is not too big */
        else if ((strlen(p_name) + strlen(p_workdir) + 2)
                  <= p_bta_fs_cfg->max_path_len)
        {
            /* create a temporary path for creation attempt */
            sprintf(p_path, "%s%c%s", p_workdir,
                p_bta_fs_cfg->path_separator, p_name);
            
            if (((bta_fs_co_access(p_path, BTA_FS_ACC_EXIST,
                &is_dir, p_cb->app_id)) == BTA_FS_CO_OK) && is_dir)
            {
                *p_op = BTA_FT_OPER_CHG_DIR;
            }
            else
                rsp_code = OBX_RSP_NOT_FOUND;
        }
    }
    else    /* Backing up a directory */
    {
        /* Backup unless already at root */
        if (strcmp(p_workdir, p_cb->p_rootpath))
        {
            /* if an empty name header exist(although illegal), goes to root */
            if (p_name && p_name[0] == '\0')
            {
                BCM_STRNCPY_S(p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
                p_workdir[p_bta_fs_cfg->max_path_len] = '\0';

                APPL_TRACE_DEBUG0("FTS: Setting current path to ROOT");
            }
            /* otherwise back up one level */
            /* Find the last occurrence of separator and replace with '\0' */
            else if((p_path = strrchr(p_workdir, (int)p_bta_fs_cfg->path_separator)) != NULL)
                *p_path = '\0';
            APPL_TRACE_DEBUG1("FTS: SET NEW PATH [%s]", p_cb->p_workdir);
            
            rsp_code = OBX_RSP_OK;
        }
        else
            rsp_code = OBX_RSP_NOT_FOUND;
    }

    if (rsp_code == OBX_RSP_OK)
        bta_fs_co_setdir(p_workdir, p_cb->app_id);
    
    return (rsp_code);
}

/*******************************************************************************
**
** Function         bta_fts_delete
**
** Description      remove a file or directory
**
** Parameters       
**                  p - Pointer to the name of the object store.  It is
**                      converted into a fully qualified path before call-out
**                      function is called (if not a RAM object).
**
** Returns          void
**
**                  Obex packet is responded to with:
**                      OBX_RSP_OK if successful,
**                      OBX_RSP_PRECONDTN_FAILED if a directory and not empty,
**                      OBX_RSP_UNAUTHORIZED if read only (access problem),
**                      OBX_RSP_INTRNL_SRVR_ERR otherwise.
**
*******************************************************************************/
void bta_fts_delete(tBTA_FTS_OBX_EVENT *p_evt, const char *p_name)
{
    tBTA_FTS_CB         *p_cb = &bta_fts_cb;
    char                *p_path;
    tBTA_FS_CO_STATUS    status;
    UINT8                rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    BOOLEAN              is_dir;
    tBTA_FT_OPER         ft_op = BTA_FT_OPER_DEL_FILE;
    BOOLEAN              rsp_now = TRUE;
    
    if ((p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 2))) != NULL)
    {
        p_path = p_cb->p_path;
        sprintf(p_path, "%s%c%s", p_cb->p_workdir, p_bta_fs_cfg->path_separator,
                p_name);

        /* Access the object to see if it exists */
        status = bta_fs_co_access(p_path, BTA_FS_ACC_RDWR, &is_dir, p_cb->app_id);
        if (status == BTA_FS_CO_OK)
        {
            if (is_dir)
                ft_op = BTA_FT_OPER_DEL_DIR;
            rsp_now = FALSE;
            bta_fts_req_app_access(ft_op, p_cb);
        }
        else if (status == BTA_FS_CO_EACCES)
            rsp_code = OBX_RSP_UNAUTHORIZED;
        else
            rsp_code = OBX_RSP_NOT_FOUND;

    }
    if(rsp_now)
    {
        utl_freebuf((void**)&p_cb->p_path);
        OBX_PutRsp(p_evt->handle, rsp_code, (BT_HDR *)NULL);
    }
}

/*******************************************************************************
**
** Function         bta_fts_end_of_list
**
** Description      Finishes up the end body of the listing, and sends out the
**                  OBX response
**
** Returns          void
**
*******************************************************************************/
void bta_fts_end_of_list(UINT8 rsp_code)
{
    tBTA_FTS_CB       *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT  *p_obx = &p_cb->obx;
    UINT16             temp_len;
    
    /* Add the end of folder listing string if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        /* If listing has completed, add on end string (http) */
        if (rsp_code == OBX_RSP_OK)
        {
            temp_len = strlen(BTA_FTS_FOLDER_LISTING_END);
            memcpy(&p_obx->p_start[p_obx->offset], BTA_FTS_FOLDER_LISTING_END, temp_len);
            p_obx->offset += temp_len;
        
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
            
            /* Clean up control block */
            bta_fts_clean_list(p_cb);
        }
        else    /* More listing data to be sent */
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, FALSE);

        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
    }
    else    /* An error occurred */
    {
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_fts_clean_list(p_cb);
    }
}

/*******************************************************************************
**
** Function         bta_fts_get_file_rsp
**
** Description      Finishes up the end body of the file get, and sends out the
**                  OBX response
**
** Returns          void
**
*******************************************************************************/
void bta_fts_get_file_rsp(UINT8 rsp_code, UINT16 num_read)
{
    tBTA_FTS_CB       *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT  *p_obx = &p_cb->obx;
    tBTA_FTS           param;
    BOOLEAN            done = TRUE;
    
    /* Send the response packet if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        p_obx->offset += num_read;
        
        /* More to be sent */
        if (rsp_code == OBX_RSP_CONTINUE)
        {
            if (p_obx->bytes_left != num_read)
                APPL_TRACE_WARNING2("FTS Read: Requested (0x%04x), Read In (0x%04x)",
                                     p_obx->bytes_left, num_read);
            done = FALSE;
        }

        OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, done);

        /* Notify application with progress */
        if (num_read)
        {
            param.prog.bytes = num_read;
            param.prog.file_size = p_cb->file_length;
            p_cb->p_cback(BTA_FTS_PROGRESS_EVT, &param);
        }
    }
    else
        p_cb->obx_oper = FTS_OP_NONE;

    OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
    p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */

    /* Final response packet sent out */
    if (done)
        bta_fts_clean_getput(p_cb, FALSE);
}

/*******************************************************************************
**
** Function         bta_fts_put_file_rsp
**
** Description      Responds to a put request, and closes the file if finished
**
** Returns          void
**
*******************************************************************************/
void bta_fts_put_file_rsp(UINT8 rsp_code)
{
    tBTA_FTS_CB       *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT  *p_obx = &p_cb->obx;
    tBTA_FTS           param;
    
    /* Finished with input packet */
    utl_freebuf((void**)&p_obx->p_pkt);

    if (rsp_code == OBX_RSP_OK)
    {
        /* Update application if file data was transferred */
        if (p_obx->bytes_left)
        {
            param.prog.bytes = p_obx->bytes_left;
            param.prog.file_size = p_cb->file_length;
            p_cb->p_cback(BTA_FTS_PROGRESS_EVT, &param);
        }

        /* If not end of file put, set the continue response */
        if (!p_obx->final_pkt)
            rsp_code = OBX_RSP_CONTINUE;
        else    /* Done - free the allocated memory */
            bta_fts_clean_getput(p_cb, FALSE);
    }
    else
        p_cb->obx_oper = FTS_OP_NONE;

    OBX_PutRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
}

/*******************************************************************************
**
** Function         bta_fts_add_list_entry
**
** Description      used by bta_fts_getdirlist to write a list entry to an
**                  obex packet (byte array).
**
** Returns          UINT8 - OBX response code
**                      OBX_RSP_PART_CONTENT if not finished yet.
**                      OBX_RSP_CONTINUE [packet done]
**                      Others send error response out
**
*******************************************************************************/
UINT8 bta_fts_add_list_entry(void)
{
    tBTA_FTS_CB      *p_cb = &bta_fts_cb;
    tBTA_FTS_OBX_PKT *p_obx = &p_cb->obx;
    tBTA_FTS_DIRLIST *p_dir = &p_cb->dir;
    tBTA_FS_DIRENTRY *p_file = p_dir->p_entry;
    char             *p_buf;
    UINT16             size;
    UINT8              rsp_code = OBX_RSP_PART_CONTENT;
    
    if ((p_buf = (char *)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL)
    {
        p_buf[0] = '\0';
        
        APPL_TRACE_DEBUG2("bta_fts_add_list_entry: attr:0x%02x, name:%s",
            p_file->mode, p_file->p_name);
        
        if(p_file->mode & BTA_FS_A_DIR)  /* Subdirectory */
        {
            /* ignore "." and ".." */
            if (strcmp(p_file->p_name, ".") && strcmp(p_file->p_name, ".."))
            {
                sprintf(p_buf, " <" BTA_FTS_FOLDER_ELEM " "
                        BTA_FTS_NAME_ATTR "=\"%s\"/>" BTA_FTS_XML_EOL,
                        p_file->p_name);
            }
        }
        else /* treat anything else as file */
        {
            /* Add the creation time if valid */
            if (p_file->crtime[0] != '\0')
            {
                sprintf(p_buf, " <" BTA_FTS_FILE_ELEM " "
                    BTA_FTS_NAME_ATTR "=\"%s\" "
                    BTA_FTS_SIZE_ATTR "=\"%lu\" "
                    BTA_FTS_USER_PERM_ATTR "=\"R%s\" "
                    BTA_FTS_CREATED_ATTR "=\"%s\"/>" BTA_FTS_XML_EOL,
                    p_file->p_name,
                    p_file->filesize,
                    p_file->mode & BTA_FS_A_RDONLY ? "" : "WD",
                    p_file->crtime);
            }
            else
            {
                sprintf(p_buf, " <" BTA_FTS_FILE_ELEM " "
                    BTA_FTS_NAME_ATTR "=\"%s\" "
                    BTA_FTS_SIZE_ATTR "=\"%lu\" "
                    BTA_FTS_USER_PERM_ATTR "=\"R%s\"/>" BTA_FTS_XML_EOL,
                    p_file->p_name,
                    p_file->filesize,
                    p_file->mode & BTA_FS_A_RDONLY ? "" : "WD");
            }
        }
        
        /* Make sure the entry fits into the current obx packet */
        size = strlen(p_buf);
        if (size <= p_obx->bytes_left)
        {
            if (size > 0)
            {
                memcpy (&p_obx->p_start[p_obx->offset], p_buf, size);
                p_obx->offset += size;
                p_obx->bytes_left -= size;
            }
            /* Get the next directory entry */
            p_cb->cout_active = TRUE;
            bta_fs_co_getdirentry (p_cb->p_path, FALSE, p_dir->p_entry,
                                   BTA_FTS_CI_DIRENTRY_EVT, p_cb->app_id);
        }
        else  /* entry did not fit in current obx packet; try to add entry in next obx req */
            rsp_code = OBX_RSP_CONTINUE;
        
        /* Done with temporary buffer */
        GKI_freebuf(p_buf);
    }
    else
        rsp_code = OBX_RSP_INTRNL_SRVR_ERR;
    return (rsp_code);
}


/*******************************************************************************
*  Static Functions 
*******************************************************************************/

/*******************************************************************************
**
** Function         bta_fts_clean_list
**
** Description      Cleans up the get directory list memory and control block
**
** Returns          void
**
*******************************************
************************************/
void bta_fts_clean_list(tBTA_FTS_CB *p_cb)
{
    tBTA_FTS_DIRLIST *p_dir = &p_cb->dir;
    /* Clean up control block */
    p_cb->obx_oper = FTS_OP_NONE;
    utl_freebuf((void**)&p_cb->p_name);
    utl_freebuf((void**)&p_dir->p_entry);
    utl_freebuf((void**)&p_cb->p_path);
}

/*******************************************************************************
**
** Function         bta_fts_clean_getput
**
** Description      Cleans up the get/put resources and control block
**
** Returns          void
**
*******************************************************************************/
void bta_fts_clean_getput(tBTA_FTS_CB *p_cb, BOOLEAN is_aborted)
{
    tBTA_FS_CO_STATUS status;
    tBTA_FTS_OBJECT   objdata;
    tBTA_FTS_EVT      evt = 0;

    /* Clean up control block */
    utl_freebuf((void**)&p_cb->obx.p_pkt);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;

        /* Notify the application */
        objdata.p_name = p_cb->p_path;

        if (is_aborted)
            objdata.status = BTA_FTS_FAIL;
        else
            objdata.status = BTA_FTS_OK;

        if (p_cb->obx_oper == FTS_OP_PUT_FILE)
        {
            /* Delete an aborted unfinished push file operation */
            if (is_aborted && p_cb->suspending == FALSE)
            {
                status = bta_fs_co_unlink(p_cb->p_path, p_cb->app_id);
                APPL_TRACE_WARNING2("FTS: Remove ABORTED Push File Operation [%s], status 0x%02x",
                    p_cb->p_path, status);
            }

            evt = BTA_FTS_PUT_CMPL_EVT;
        }
        else if (p_cb->obx_oper == FTS_OP_GET_FILE)
        {
            evt = BTA_FTS_GET_CMPL_EVT;
        }

        if (evt)
        {
            /* Notify application of operation complete */
            p_cb->p_cback(evt, (tBTA_FTS *)&objdata);
        }
    }

    p_cb->suspending = FALSE;

    utl_freebuf((void**)&p_cb->p_name);
    utl_freebuf((void**)&p_cb->p_path);

    p_cb->obx_oper = FTS_OP_NONE;
    p_cb->obx.bytes_left = 0;
    p_cb->file_length = BTA_FS_LEN_UNKNOWN;
    p_cb->acc_active = 0;
}

/*******************************************************************************
**
** Function         bta_fts_disable_cleanup
**
** Description      Cleans up the resources and control block
**
** Returns          void
**
*******************************************************************************/
void bta_fts_disable_cleanup(tBTA_FTS_CB *p_cb)
{
    tBTA_FS_CO_STATUS status;

    /* Stop the OBEX server */
    OBX_StopServer(p_cb->obx_handle);

    /* Clean up control block */
    utl_freebuf((void **)&p_cb->obx.p_pkt);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_fs_co_close(p_cb->fd, p_cb->app_id);
        p_cb->fd = BTA_FS_INVALID_FD;

        /* Delete an aborted unfinished push file operation */
        if (p_cb->obx_oper == FTS_OP_PUT_FILE)
        {
            status = bta_fs_co_unlink(p_cb->p_path, p_cb->app_id);
            APPL_TRACE_WARNING2("FTS: bta_fts_disable_cleanup() ->Remove ABORTED Push File Operation [%s], status 0x%02x",
                p_cb->p_path, status);
        }
    }

    /* Free any used memory buffers */
    utl_freebuf((void **)&p_cb->dir.p_entry);
    utl_freebuf((void **)&p_cb->p_name);
    utl_freebuf((void **)&p_cb->p_path);

    /* Remove the FTP service from the SDP database */
    SDP_DeleteRecord(p_cb->sdp_handle);

    /* Free the allocated server channel number */
    BTM_FreeSCN(p_cb->scn);

    GKI_freebuf(p_cb->p_rootpath);  /* Free buffer containing root and working paths */

    p_cb->obx_oper = FTS_OP_NONE;
    p_cb->obx.bytes_left = 0;
    p_cb->file_length = BTA_FS_LEN_UNKNOWN;
    p_cb->acc_active = 0;

    if (p_cb->p_cback)
    {
        /* Notify the application */
        p_cb->p_cback(BTA_FTS_DISABLE_EVT, 0);
        p_cb->p_cback = NULL;
    }
}

/*******************************************************************************
**
** Function         bta_fts_discard_data
**
** Description      frees the data
**
** Returns          void
**
*******************************************************************************/
void bta_fts_discard_data(UINT16 event, tBTA_FTS_DATA *p_data)
{
    switch(event)
    {
    case BTA_FTS_OBX_CONN_EVT:
    case BTA_FTS_OBX_DISC_EVT:
    case BTA_FTS_OBX_ABORT_EVT:
    case BTA_FTS_OBX_PASSWORD_EVT:
    case BTA_FTS_OBX_CLOSE_EVT:
    case BTA_FTS_OBX_PUT_EVT:
    case BTA_FTS_OBX_GET_EVT:
    case BTA_FTS_OBX_SETPATH_EVT:
        utl_freebuf((void**)&p_data->obx_evt.p_pkt);
        break;

    default:
        /*Nothing to free*/
        break;
    }
}

#endif /* BTA_FT_INCLUDED */
