/************************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

/************************************************************************************
 *
 *  Filename:      unv.h
 *
 *  Description:   Universal NV Ram API
 *
 ***********************************************************************************/

#ifndef UNV_H
#define UNV_H

/************************************************************************************
**  Constants & Macros
************************************************************************************/

/* any keyfile line cannot exceed this length */
#define UNV_MAXLINE_LENGTH (2048)

/************************************************************************************
**  Type definitions for callback functions
************************************************************************************/
typedef int (*unv_iter_cb)(char *key, char *val, void *userdata);

/************************************************************************************
**  Type definitions and return values
************************************************************************************/

/************************************************************************************
**  Extern variables and functions
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

/******************************************************************************
**
** Function         unv_create_directory
**
** Description      Creates directory, if full path is not available it
**                  will construct it. Must be called from BTIF task context.
**
** Parameters
**                  path        : directory path to be created
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

int unv_create_directory(const char *path);


/*******************************************************************************
**
** Function         unv_create_file
**
** Description      Creates file
**                  Must be called from BTIF task context
**
** Parameters
**                  filename     : file path to be created
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

int unv_create_file(const char *filename);

/*******************************************************************************
**
** Function         unv_read_key
**
** Description      Reads keyvalue from file
**                  Path must be an existing absolute path
**                  Must be called from BTIF task context
**
** Parameters
**                  path        : path of keyfile
**                  key         : key string to query for
**                  p_out       : output line buffer supplied by caller
**                  out_len     : max length of output buffer
**
** Returns          Returns buffer of key value.
**
*******************************************************************************/

char* unv_read_key( const char *path,
                    const char *key,
                    char *p_out,
                    int out_len);

/*******************************************************************************
**
** Function         unv_read_key_iter
**
** Description      Reads keyvalue from file iteratively
**                  Path must be an existing absolute path
**                  cb is the callback that is invoked for each entry read
**                  Must be called from BTIF task context
**
** Parameters
**                  path        : path of keyfile
**                  cb          : iteration callback
**                  userdata    : context specific userdata passed onto callback
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

int unv_read_key_iter( const char *path,
                       unv_iter_cb cb,
                       void *userdata );


/*******************************************************************************
**
** Function         unv_write_key
**
** Description      Writes key to file. If key value exists it will be updated
**                  Path must be an existing absolute path
**                  Must be called from BTIF task context
**
** Parameters
**                  path        : path of keyfile
**                  key         : key string to write
**                  value       : value string to set for this key
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

int unv_write_key( const char *path,
                   const char *key,
                   const char *value );


/*******************************************************************************
**
** Function         unv_remove_key
**
** Description      Removes keyvalue from file
**                  Path must be an existing absolute path
**                  Must be called from BTIF task context
**
** Parameters
**                  path        : path of keyfile
**                  key         : key string to delete
**
** Returns         0 if successful, -1 if failure
**
*******************************************************************************/

int unv_remove_key( const char *path,
                    const char *key );
/*******************************************************************************
**
** Function         unv_read_hl_apps_cb
**
** Description      read HL applciation contorl block
**
** Parameters
**                  path        : path of file
**                  value       : pointer to value
**                  value_size  : size of value
**
** Returns         0 if successful, -1 if failure
**
******************************************************************************/
int unv_read_hl_apps_cb(const char *path, char *value, int value_size);
/*******************************************************************************
**
** Function         unv_write_hl_apps_cb
**
** Description      write HL applciation contorl block
**
** Parameters
**                  path        : path of file
**                  value       : pointer to value
**                  value_size  : size of value
**
** Returns         0 if successful, -1 if failure
**
******************************************************************************/
int unv_write_hl_apps_cb(const char *path, char *value, int value_size);
/*******************************************************************************
**
** Function         unv_read_hl_data
**
** Description      read HL applciation data
**
** Parameters
**                  path        : path of file
**                  value       : pointer to value
**                  value_size  : size of value
**
** Returns         0 if successful, -1 if failure
**
******************************************************************************/
int unv_read_hl_data(const char *path, char *value, int value_size);
/*******************************************************************************
**
** Function         unv_write_hl_data
**
** Description      write HL applciation data
**
** Parameters
**                  path        : path of file
**                  value       : pointer to value
**                  value_size  : size of value
**
** Returns         0 if successful, -1 if failure
**
******************************************************************************/
int unv_write_hl_data(const char *path, char *value, int value_size);
#endif /* UNV_H */


