/*****************************************************************************
**
**  Name:           bta_pbs_utils.c
**
**  Description:    This file implements utils functions for phone book access
**                  server.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
    
#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include <stdio.h>
#include <string.h>
#include "bta_fs_api.h"
#include "bta_pbs_int.h"
#include "bta_fs_co.h"
#include "gki.h"
#include "utl.h"

/*******************************************************************************
**  Constants
*******************************************************************************/

/*******************************************************************************
**  Local Function Prototypes
*******************************************************************************/
static void bta_pbs_translate_special_character(char *buffer, const char *str);
/*******************************************************************************
*  Macros for PBS 
*******************************************************************************/
#define BTA_PBS_XML_EOL              "\n"
#define BTA_PBS_FOLDER_LISTING_START ( "<?xml version=\"1.0\"?>\n" \
                                   "<!DOCTYPE vcard-listing SYSTEM \"vcard-listing.dtd\">\n" \
                                   "<vCard-listing version=\"1.0\">\n" )

#define BTA_PBS_FOLDER_LISTING_END   ( "</vCard-listing>" )
#define BTA_PBS_PARENT_FOLDER        (" <parent-folder/>\n")

#define BTA_PBS_CARD_ELEM            "card handle"
#define BTA_PBS_NAME_ATTR            "name"

/*******************************************************************************
*  Exported Functions 
*******************************************************************************/
/*******************************************************************************
**
** Function         bta_pbs_getdirlist
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
void bta_pbs_getvlist(char *p_name)
{
    tBTA_PBS_CB      *p_cb = &bta_pbs_cb;
    tBTA_PBS_OBX_PKT *p_obx = &p_cb->obx;
    UINT16            temp_len;
    UINT8             rsp_code = OBX_RSP_OK;
    UINT16 pb_size, new_missed_call, len=0;
    UINT8        *p, *p_start;
    char *p_sep = NULL;


    /* if this the first time asking for access */    
    if (!p_cb->p_path)
    {
        p_cb->p_path = (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1));
        /* If not specified, use the current work directory */
        if (!p_name || p_name[0] == '\0')
        {
            BCM_STRNCPY_S(p_cb->p_path, p_bta_fs_cfg->max_path_len+1, p_cb->p_workdir, p_bta_fs_cfg->max_path_len);
            p_sep = strchr((char *)p_cb->p_path, '/');
            if (p_sep)
                *p_sep = p_bta_fs_cfg->path_separator;
        }
        else
        {
            if ((strlen(p_cb->p_workdir) + strlen(p_name) + 2) <= p_bta_fs_cfg->max_path_len)
            {
                sprintf(p_cb->p_path, "%s%c%s", p_cb->p_workdir,
                        p_bta_fs_cfg->path_separator, p_name);
                if (p_bta_fs_cfg->path_separator == 0x5c)
                {
                    while ((p_sep = strchr((char *)p_cb->p_path, '/')) != NULL) 
                    {
                       if (p_sep)
                           *p_sep = p_bta_fs_cfg->path_separator;
                    }
                }
            }
            else 
                rsp_code = OBX_RSP_BAD_REQUEST;
        }
        if (rsp_code == OBX_RSP_OK)
        {
            p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                        /* p_cb->peer_mtu */ OBX_LRG_DATA_POOL_SIZE);
            if (p_obx->p_pkt)
            {
                /* Notify the application that a get file has been requested */
                bta_pbs_req_app_access (BTA_PBS_OPER_PULL_VCARD_LIST, p_cb);
                return;
            }
            else
                rsp_code = OBX_RSP_BAD_REQUEST;
        }
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_pbs_clean_getput(p_cb, TRUE);
    }
    else if (p_cb->p_path)
    {   
        /* Build the listing */
        if (rsp_code == OBX_RSP_OK)
        {
            if (!p_obx->p_pkt)
                 p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
                        /* p_cb->peer_mtu */ OBX_LRG_DATA_POOL_SIZE);                 
            if (p_obx->p_pkt)
            {
                /* Is this a new request or continuation? */
                if ((p_cb->obx_oper == BTA_PBS_OPER_NONE))
                {
                    p_cb->get_only_indexes = FALSE;

                    if (p_cb->getvlist_app_params.max_count == 0 || 
                        p_cb->obj_type == BTA_PBS_MCH_OBJ)
                    {
                        p_start = OBX_AddByteStrStart(p_obx->p_pkt, &len);
                        bta_pbs_co_getpbinfo(p_cb->obx_oper, p_cb->obj_type, &pb_size, &new_missed_call);
                        p = p_start;
                        /* Just looking for number of indexes only ? */
                        if (p_cb->getvlist_app_params.max_count == 0)
                        {
                            *p++    = BTA_PBS_TAG_PB_SIZE;
                            *p++    = 2;
                            p_cb->p_stream_indexes = p;   /* save location to store entries later */
                            UINT16_TO_BE_STREAM(p, p_cb->getvlist_app_params.max_count);

                            /* overwrite appl params max count so entries will be retrieved */
                            p_cb->getvlist_app_params.max_count = BTA_PBS_MAX_LIST_COUNT;
                            /* ignore the list start offset */
                            p_cb->getvlist_app_params.start_offset = 0;
                            p_cb->get_only_indexes = TRUE;
                            p_cb->num_vlist_idxs = 0;
                            APPL_TRACE_EVENT1("PBS Get Vcard List: Name [p_stream = 0x%07x] (Indexes Only)", /*p_cb->p_path*/(UINT32)p_cb->p_stream_indexes);
                        }

                        if (p_cb->obj_type == BTA_PBS_MCH_OBJ)
                        {
                            *p++    = BTA_PBS_TAG_NEW_MISSED_CALLS;
                            *p++    = 1;
                            *p++    = (UINT8) new_missed_call;
                        }
                        OBX_AddByteStrHdr(p_obx->p_pkt, OBX_HI_APP_PARMS, NULL, (UINT16)(p - p_start));
                    }

                    /* By pass Body Header if only requesting number of entries */
                    if (!p_cb->get_only_indexes)
                    {
                        /* Add the start of the Body Header */
                        p_obx->offset = 0;
                        p_obx->bytes_left = 0;
                        p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

                        temp_len = strlen(BTA_PBS_FOLDER_LISTING_START);

                        /* Add the beginning label of vard list */
                        memcpy(p_obx->p_start, BTA_PBS_FOLDER_LISTING_START, temp_len);
                        p_obx->bytes_left -= (UINT16)(temp_len + strlen(BTA_PBS_FOLDER_LISTING_END));
                        p_obx->offset += temp_len;

                        APPL_TRACE_EVENT1("PBS Get Vcard List: Name [%s]", p_cb->p_path);
                    }

                    p_cb->obx_oper = BTA_PBS_OPER_PULL_VCARD_LIST;
                    p_cb->cout_active = TRUE;

                    bta_pbs_co_getvlist(p_cb->p_path, &p_cb->getvlist_app_params,
                                        TRUE, &p_cb->vlist);

                    /* List is not complete, so don't send the response yet */
                    rsp_code = OBX_RSP_PART_CONTENT;
                }
                else /* continue case */
                {
                    /* By pass Body Header if only requesting number of entries */
                    if (!p_cb->get_only_indexes)
                    {
                        /* Add the start of the Body Header */
                        p_obx->offset = 0;
                        p_obx->bytes_left = 0;
                        p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);
                        APPL_TRACE_EVENT1("PBS Get Vcard List: Name [%s] continue", p_cb->p_path);
                    }

                    /* Add the entry previously retrieved */
                    rsp_code = bta_pbs_add_list_entry();

                    if (!p_obx->final_pkt) {
                        p_cb->cout_active = TRUE;

                        bta_pbs_co_getvlist(p_cb->p_path, &p_cb->getvlist_app_params,
                                        FALSE, &p_cb->vlist);
                        /* List is not complete, so don't send the response yet */
                        rsp_code = OBX_RSP_PART_CONTENT;
                    } else
                        p_obx->final_pkt = FALSE;
                }
            }
            else
                rsp_code = OBX_RSP_SERVICE_UNAVL;
        }
    }
    else    /* Error occurred */
        rsp_code = OBX_RSP_SERVICE_UNAVL;

    /* Response goes out if complete or error occurred */
    if (rsp_code != OBX_RSP_PART_CONTENT)
        bta_pbs_end_of_list(rsp_code);
}

/*******************************************************************************
**
** Function         bta_pbs_proc_get_file
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
void bta_pbs_proc_get_file(char *p_name, tBTA_PBS_OPER operation)
{
    tBTA_PBS_CB      *p_cb = &bta_pbs_cb;
    tBTA_PBS_OBX_PKT *p_obx = &p_cb->obx;
    UINT8 rsp_code = OBX_RSP_SERVICE_UNAVL;
    char *p_sep;
    
    if (operation != BTA_PBS_OPER_PULL_PB && operation != BTA_PBS_OPER_PULL_VCARD_ENTRY)
        return;
    /* Allocate an OBX packet */
    if (p_obx->p_pkt != NULL || (p_obx->p_pkt = (BT_HDR *)OBX_HdrInit(p_cb->obx_handle,
            /*p_cb->peer_mtu*/OBX_LRG_DATA_POOL_SIZE)) != NULL)
    {
        /* Is this a new request or continuation? */
        if ((p_cb->obx_oper == BTA_PBS_OPER_NONE))
        {
            /* Validate the name */
            if (p_name)
            {
                if ((p_cb->p_path =
                    (char *)GKI_getbuf((UINT16)(p_bta_fs_cfg->max_path_len + 1))) != NULL)
                {
                    /* Build a fully qualified path for Pull Vcard entry */
                    if (operation == BTA_PBS_OPER_PULL_VCARD_ENTRY) 
                    {
                       if ((strlen(p_cb->p_workdir) + strlen(p_name) + 2)
                            <= p_bta_fs_cfg->max_path_len)
                       {
                           rsp_code = OBX_RSP_OK;
                           sprintf(p_cb->p_path, "%s%c%s", p_cb->p_workdir,
                                   p_bta_fs_cfg->path_separator, p_name);
                           if (p_bta_fs_cfg->path_separator == 0x5c) {
                               while ((p_sep = strchr((char *)p_cb->p_path, '/')) != NULL)
                               {
                                  if (p_sep)
                                      *p_sep = p_bta_fs_cfg->path_separator;
                               }
                           }
                        
                           APPL_TRACE_EVENT1("PBS PULL VCARD ENTRY: Name [%s]", p_cb->p_path);
                        
                           p_cb->obx_oper = BTA_PBS_OPER_PULL_VCARD_ENTRY;

                           /* Notify the application that a get file has been requested */
                           bta_pbs_req_app_access (BTA_PBS_OPER_PULL_VCARD_ENTRY, p_cb);
                       }
                    }

                    /* Build a fully qualified path for Pull PBentry */
                    if (operation == BTA_PBS_OPER_PULL_PB) 
                    {
                       if ((strlen(p_cb->p_rootpath) + strlen(p_name) + 2)
                            <= p_bta_fs_cfg->max_path_len)
                       {
                           rsp_code = OBX_RSP_OK;
                           sprintf(p_cb->p_path, "%s%c%s", p_cb->p_rootpath,
                                   p_bta_fs_cfg->path_separator, p_name);
                           if (p_bta_fs_cfg->path_separator == 0x5c) {
                               while ((p_sep = strchr((char *)p_cb->p_path, '/')) != NULL)
                               {
                                    if (p_sep)
                                        *p_sep = p_bta_fs_cfg->path_separator;
                               }
                           }
                        
                           APPL_TRACE_EVENT1("PBS PULL PB : Name [%s]", p_cb->p_path);
                        
                           p_cb->obx_oper = BTA_PBS_OPER_PULL_PB;

                           /* Notify the application that a get file has been requested */
                           bta_pbs_req_app_access (BTA_PBS_OPER_PULL_PB, p_cb);
                       }
                    }

                }
            }
        }
        else    /* Continue reading from the file */
        {
            /* Add the start of the Body Header */
            p_obx->offset = 0;
            p_obx->bytes_left = 0;
            p_obx->p_start = OBX_AddBodyStart(p_obx->p_pkt, &p_obx->bytes_left);

            rsp_code = OBX_RSP_OK;
            p_cb->cout_active = TRUE;
            bta_pbs_co_read(p_cb->fd, p_cb->obx_oper, &p_obx->p_start[p_obx->offset],
                           p_obx->bytes_left);
        }
    }
    if (rsp_code != OBX_RSP_OK)
        bta_pbs_get_file_rsp(rsp_code, 0);
}




/*******************************************************************************
**
** Function         bta_pbs_chdir
**
** Description      Changes the current path to the specified directory.
**
** Parameters       p_pkt - Pointer to the OBX packet
**                  backup_flag - if TRUE, path adjusted up one level.
**                  
** Returns          UINT8 - OBX response code
**
*******************************************************************************/
UINT8 bta_pbs_chdir(BT_HDR *p_pkt, BOOLEAN backup_flag, tBTA_PBS_OPER *p_op)
{
    tBTA_PBS_CB *p_cb = &bta_pbs_cb;
    char        *p_path;
    char        *p_name;
    char        *p_workdir = p_cb->p_workdir;
    UINT8        rsp_code = OBX_RSP_SERVICE_UNAVL;
    BOOLEAN     is_dir;
    
    if (!backup_flag)
    {
        p_path = p_cb->p_path;
        p_name = p_cb->p_name;            
        
        /* If No Name header, or if it is NULL, set to root path */
        if (*p_name == '\0')
        {
            BCM_STRNCPY_S(p_workdir, p_bta_fs_cfg->max_path_len+1, p_cb->p_rootpath, p_bta_fs_cfg->max_path_len);
            p_workdir[p_bta_fs_cfg->max_path_len] = '\0';
            p_cb->obj_type = BTA_PBS_NONE_OBJ;
            rsp_code = OBX_RSP_OK;
            APPL_TRACE_DEBUG0("PBS: Setting current path to ROOT");
        }
        /* Make sure the new path is not too big */
        else if ((strlen(p_name) + strlen(p_workdir) + 2)
                  <= p_bta_fs_cfg->max_path_len)
        {
            if (!strcmp(p_name, "pb") || !strcmp(p_name, "telecom") ||
                !strcmp(p_name, "ich") || !strcmp(p_name, "och") ||
                !strcmp(p_name, "mch") || !strcmp(p_name, "cch") ||
                !strcmp(p_name, "SIM1"))
            {

               /* create a temporary path for creation attempt */
                sprintf(p_path, "%s%c%s", p_workdir,
                        p_bta_fs_cfg->path_separator, p_name);
                    
                if (((bta_fs_co_access(p_path, BTA_FS_ACC_EXIST,
                &is_dir, p_cb->app_id)) == BTA_FS_CO_OK) && is_dir)
                {
                *p_op = BTA_PBS_OPER_SET_PB;
                }
                else
                    rsp_code = OBX_RSP_NOT_FOUND;
            }
            else
                rsp_code = OBX_RSP_BAD_REQUEST;
        }
    }
    else    /* Backing up a directory */
    {
        /* Backup a level unless already at root */
        if (strcmp(p_workdir, p_cb->p_rootpath))
        {
            /* Find the last occurrence of separator and replace with '\0' */
            if ((p_path = strrchr(p_workdir, (int)p_bta_fs_cfg->path_separator)) != NULL)
                *p_path = '\0';
            p_cb->obj_type = BTA_PBS_NONE_OBJ;
            APPL_TRACE_DEBUG1("PBS: SET NEW PATH [%s]", p_cb->p_workdir);
            
            rsp_code = OBX_RSP_OK;
        }
        else
            rsp_code = OBX_RSP_NOT_FOUND;
    }
    
    return (rsp_code);
}


/*******************************************************************************
**
** Function         bta_pbs_end_of_list
**
** Description      Finishes up the end body of the listing, and sends out the
**                  OBX response
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_end_of_list(UINT8 rsp_code)
{
    tBTA_PBS_CB       *p_cb = &bta_pbs_cb;
    tBTA_PBS_OBX_PKT  *p_obx = &p_cb->obx;
    UINT16             temp_len;
    
    /* Add the end of folder listing string if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        /* If only getting entries add the number to the response */
        if (p_cb->get_only_indexes)
        {
            if (rsp_code != OBX_RSP_OK)
            {
                rsp_code = OBX_RSP_OK;
                APPL_TRACE_WARNING0("bta_pbs_end_of_list: index ONLY, but received OBX_RSP_CONTINUE??");
            }
            APPL_TRACE_DEBUG2("bta_pbs_end_of_list: indexes = %d, p_stream_indexes = 0x%07x",
                        p_cb->num_vlist_idxs, p_cb->p_stream_indexes);
            UINT16_TO_BE_STREAM(p_cb->p_stream_indexes, p_cb->num_vlist_idxs);
        }
        else
        {
            /* If listing has completed, add on end string (http) */
            if (rsp_code == OBX_RSP_OK)
            {
                temp_len = strlen(BTA_PBS_FOLDER_LISTING_END);
                memcpy(&p_obx->p_start[p_obx->offset], BTA_PBS_FOLDER_LISTING_END, temp_len);
                p_obx->offset += temp_len;
        
                OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, TRUE);
            }
            else    /* More listing data to be sent */
                OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, FALSE);
        }

        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
        p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */
        if (rsp_code == OBX_RSP_OK)
            bta_pbs_clean_getput(p_cb, FALSE);
    }
    else    /* An error occurred */
    {
        OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)NULL);
        bta_pbs_clean_getput(p_cb, TRUE);
    }
   
}

/*******************************************************************************
**
** Function         bta_pbs_get_file_rsp
**
** Description      Finishes up the end body of the file get, and sends out the
**                  OBX response
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_get_file_rsp(UINT8 rsp_code, UINT16 num_read)
{
    tBTA_PBS_CB       *p_cb = &bta_pbs_cb;
    tBTA_PBS_OBX_PKT  *p_obx = &p_cb->obx;
    BOOLEAN            done = TRUE;
    
    /* Send the response packet if successful operation */
    if (rsp_code == OBX_RSP_OK || rsp_code == OBX_RSP_CONTINUE)
    {
        p_obx->offset += num_read;
        
        /* More to be sent */
        if (rsp_code == OBX_RSP_CONTINUE)
        {
            if (p_obx->bytes_left != num_read)
                APPL_TRACE_WARNING2("PBS Read: Requested (0x%04x), Read In (0x%04x)",
                                     p_obx->bytes_left, num_read);
            done = FALSE;
        }

        if (num_read)
            OBX_AddBodyEnd(p_obx->p_pkt, p_obx->p_start, p_obx->offset, done);
    }
    else
        p_cb->obx_oper = BTA_PBS_OPER_NONE;

    OBX_GetRsp(p_cb->obx_handle, rsp_code, (BT_HDR *)p_obx->p_pkt);
    p_obx->p_pkt = NULL;    /* Do not deallocate buffer; OBX will */

    /* Final response packet sent out */
    if (done && rsp_code == OBX_RSP_OK)
        bta_pbs_clean_getput(p_cb, FALSE);
    /* If there is error */
    if (rsp_code != OBX_RSP_OK && rsp_code != OBX_RSP_CONTINUE)
        bta_pbs_clean_getput(p_cb, TRUE);
}


/*******************************************************************************
**
** Function         bta_pbs_add_list_entry
**
** Description      used by bta_pbs_getdirlist to write a list entry to an
**                  obex packet (byte array).
**
** Returns          UINT8 - OBX response code
**                      OBX_RSP_OK 
**                      OBX_RSP_CONTINUE
**                      Others send error response out
**
*******************************************************************************/
UINT8 bta_pbs_add_list_entry(void)
{
    tBTA_PBS_CB      *p_cb = &bta_pbs_cb;
    tBTA_PBS_OBX_PKT *p_obx = &p_cb->obx;
    char             *p_buf;
    UINT16             size;
    UINT8              rsp_code = OBX_RSP_SERVICE_UNAVL;
    
    /* Skip filling in entry; just counting */
    if (p_cb->get_only_indexes)
    {
        return (OBX_RSP_OK);
    }

    if ((p_buf = (char *)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL)
    {
        p_buf[0] = '\0';
        
        APPL_TRACE_DEBUG2("bta_pbs_add_list_entry: handle:%s, name:%s",
            p_cb->vlist.handle, p_cb->vlist.name);
              
        sprintf(p_buf, " <" BTA_PBS_CARD_ELEM " = \"%s\" " BTA_PBS_NAME_ATTR " = \"",p_cb->vlist.handle);
        /* Need to translate special characters to XML format */
	    bta_pbs_translate_special_character(strlen(p_buf) + p_buf, p_cb->vlist.name);
	    sprintf(strlen(p_buf)+p_buf, "\"/>" BTA_PBS_XML_EOL);    
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
            rsp_code = OBX_RSP_OK;
        }
        else  /* entry did not fit in current obx packet; try to add entry in next obx req */
            rsp_code = OBX_RSP_CONTINUE;
        
        /* Done with temporary buffer */
        GKI_freebuf(p_buf);
    }
    else
        rsp_code = OBX_RSP_SERVICE_UNAVL;

    return (rsp_code);
}


/*******************************************************************************
**
** Function         bta_pbs_clean_getput
**
** Description      Cleans up the get resources and control block
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_clean_getput(tBTA_PBS_CB *p_cb, BOOLEAN is_aborted)
{
    tBTA_PBS_OBJECT   objdata;

    /* Clean up control block */
    utl_freebuf((void**)&p_cb->obx.p_pkt);

    /* Close any open files */
    if (p_cb->fd >= 0)
    {
        bta_pbs_co_close(p_cb->fd);
        p_cb->fd = BTA_FS_INVALID_FD;
    }

    /* Notify the application */
    objdata.p_name = p_cb->p_path;

    if (is_aborted)
        objdata.status = BTA_PBS_FAIL;
    else
        objdata.status = BTA_PBS_OK;

    if (p_cb->p_cback && p_cb->obx_oper != 0)
    {
            objdata.operation = p_cb->obx_oper;
            /* Notify application of operation complete */
            p_cb->p_cback(BTA_PBS_OPER_CMPL_EVT, (tBTA_PBS *)&objdata);
    }

    utl_freebuf((void**)&p_cb->p_name);
    utl_freebuf((void**)&p_cb->p_path);

    p_cb->obx_oper = BTA_PBS_OPER_NONE;
    p_cb->obj_type = BTA_PBS_NONE_OBJ;
    p_cb->obx.bytes_left = 0;
    p_cb->file_length = BTA_FS_LEN_UNKNOWN;
    p_cb->acc_active = 0;
// btla-specific ++
    p_cb->aborting = FALSE;
// btla-specific --
    memset(&p_cb->getvlist_app_params, 0, sizeof(p_cb->getvlist_app_params));
    memset(&p_cb->pullpb_app_params, 0, sizeof(p_cb->pullpb_app_params));
    memset(&p_cb->vlist, 0, sizeof(p_cb->vlist));
}

/*****************************************************************************
* Function: bta_pbs_read_app_params
* Purpose:  Read the application parameters from the given OBX packet
*****************************************************************************/
UINT8 * bta_pbs_read_app_params(BT_HDR *p_pkt, UINT8 tag, UINT16 *param_len)
{
    UINT8   *p_data = NULL, *p = NULL;
    UINT16  data_len = 0;
    int     left, len;

    if(OBX_ReadByteStrHdr(p_pkt, OBX_HI_APP_PARMS, &p_data, &data_len, 0))
    {
        left    = data_len;
        while(left > 0)
        {
            len = *(p_data + 1);
            if(*p_data == tag)
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
** Function         bta_pbs_handle_special_character
**
** Description      Translate special characters to XML format
**                                    
**
** Returns          Void  
**
*******************************************************************************/
static void bta_pbs_translate_special_character(char *buffer, const char *str)
{
    char *buf=buffer;
    int slen = strlen(str);
    int i;
    for (i=0; i < slen;i++) 
    {
        
        char c = str[i];
        switch (c) 
        {
            case '<':
                BCM_STRCPY_S(buf,4,"&lt;");
                buf+=4;
                break;
            case '>':
                BCM_STRCPY_S(buf,4,"&gt;");
                buf+=4;
                break;
            case '\"':
                BCM_STRCPY_S(buf,6,"&quot;");
                buf+=6;
                break;
            case '\'':
                BCM_STRCPY_S(buf,6,"&apos;");
                buf+=6;
                break;
            case '&':
                BCM_STRCPY_S(buf,5,"&amp;");
                buf+=5;
                break;
            default:
                buf[0]=c;
                buf++;
        }
    }
    buf[0]='\0';

}



#endif /* BTA_PBS_INCLUDED */
