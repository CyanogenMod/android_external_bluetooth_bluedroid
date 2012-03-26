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
 *  Filename:      unv_linux.c
 *
 *  Description:   Universal NV Ram API.
 *                 Manages all non volatile (file) database entries used by
 *                 bluetooth upper layer stack.
 *
 ***********************************************************************************/

#include "unv.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <pthread.h>

#define LOG_TAG "UNV_LINUX"
#include <utils/Log.h>

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define info(fmt, ...)  LOGI ("%s: " fmt,__FUNCTION__, ## __VA_ARGS__)
#define debug(fmt, ...) LOGD ("%s: " fmt,__FUNCTION__, ## __VA_ARGS__)
#define verbose(fmt, ...) //LOGD ("%s: " fmt, __FUNCTION__,  ## __VA_ARGS__)
#define error(fmt, ...) LOGE ("##### ERROR : %s: " fmt "#####", __FUNCTION__, ## __VA_ARGS__)

#define UNV_DELIM "\r\n"

/* fixme -- incorporate field delim definition */
#define UNV_FIELD_DELIM " "

/* keystring + space + valuestring + null terminate */
#define KEYVAL_SZ(key, val)  (strlen(key) + 1 + strlen(val) + strlen(UNV_DELIM) + 1)

#define DIR_MODE 0740
#define FILE_MODE 0644

#define BTIF_TASK_STR        "BTIF"

/************************************************************************************
**  Local type definitions
************************************************************************************/


/************************************************************************************
**  Static variables
************************************************************************************/

/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

/*******************************************************************************
**
** Function         check_caller_context
**
** Description      Checks pthread name of caller. Currently only BTIF thread
**                  is allowed to call in here to avoid multithreading issues
**
** Returns          1 if correct context, 0 otherwise
**
*******************************************************************************/

static int check_caller_context(void)
{
    char name[16];

    prctl(PR_GET_NAME, name, 0, 0, 0);

    if (strncmp(name, BTIF_TASK_STR, strlen(BTIF_TASK_STR)) != 0)
    {
        error("only btif context allowed (%s)", name);
        return 0;
    }
    return 1;
}

/*******************************************************************************
**
** Function         mk_dir
**
** Description      Create directory
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

static int mk_dir(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        if (!S_ISDIR(st.st_mode))
        {
            /* path is not a directory */
            error("directory path %s is not a directory (%s)", path, strerror(errno));
            return -1;
        }

        /* already exist */
        return 0;
    }

    /* no existing dir path, try creating it */
    if (mkdir(path, DIR_MODE) != 0)
    {
        error("failed to create dir [%s] (%s)", path, strerror(errno));
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         rm_dir
**
** Description      Removes directory. Assumes empty directory
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

static int rm_dir(const char *path)
{
    int status;

    status = rmdir(path);

    if (status < 0) {
        error("rmdir %s failed (%s)", path, strerror(errno));
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         write_keyval
**
** Description      Writes a key and value string to file descriptor
**
** Returns          Nbr bytes written, -1 if failure
**
*******************************************************************************/

static int write_keyval( int fd,
                         const char *key,
                         const char *val )
{
    int len = 0;
    int written = 0;
    int linesz = KEYVAL_SZ(key, val);
    char *line;

    /* check if written line would exceed the max read size of a line */
    if (linesz > UNV_MAXLINE_LENGTH)
        return -1;

    line = malloc(linesz);

    if (!line)
    {
        error("alloc failed (%s)", strerror(errno));
        return -1;
    }

    len = sprintf(line, "%s %s%s", key, val, UNV_DELIM);

    //info("write_keyval %s %s (%d bytes)", key, val, len);

    /* update line */
    written = write(fd, line, len);

    free(line);

    return written;
}

/*******************************************************************************
**
** Function         update_key
**
** Description      Updates existing keyvalue in file
**
**                  key       : key string to be updated
**                  value     : updated value string, NULL removes line
**                  pos_start : start offset of string to be updated
**                  pos_stop  : end offset of string to be updated
**
** Returns          returns bytes written, -1 if failure
**
*******************************************************************************/

static int update_key( int fd,
                       const char *key,
                       const char *value,
                       int pos_start,
                       int pos_stop )
{
    char *line;
    char *p_tail = NULL;
    int tail_sz;
    struct stat st;
    int len = 0;

    if (value)
    {
        verbose("update key [%s %s]", key, value);
    }
    else
    {
        verbose("remove key [%s]", key);
    }

    /* update file with new value for this key */

    if (fstat(fd, &st) != 0)
    {
        error("stat failed (%s)", strerror(errno));
        return -1;
    }

    tail_sz = st.st_size-pos_stop;

    if (tail_sz > 0)
    {
        /* allocate working area */
        p_tail = malloc(tail_sz);

        if (!p_tail)
        {
            error("malloc failed (%s)", strerror(errno));
            return -1;
        }

        /* copy anything after updated line */
        lseek(fd, pos_stop, SEEK_SET);

        /* read reamining portion into cache */
        if (read(fd, p_tail, tail_sz) < 0)
        {
            error("read failed %s", strerror(errno));
            return -1;
        }
    }

    /* rewind and update new line */
    lseek(fd, pos_start, SEEK_SET);
    len = pos_start;

    /* a null key means remove entry */
    if (value)
    {
        len += write_keyval(fd, key, value);
    }

    /* write tail content */
    if (p_tail)
    {
        len += write(fd, p_tail, tail_sz);
        free(p_tail);
    }

    /* finally truncate file to new length */
    ftruncate(fd, len);

    return len;
}

/******************************************************************************
**
** Function         get_keyval
**
** Description      Reads keyvalue from filedescriptor
**
** Parameters
**                  fd          : file descriptor of file to search
**                  key         : string to search for in keyfile
**                  p_out       : output buffer supplied by caller
**                  out_len     : max length of output line buffer
**                  pos_begin   : returns keyvalue start offset in file
**                  pos_end     : returns keyvalue end offset in file
**
** Returns          String of key value, NULL if not found
**
******************************************************************************/

static char *get_keyval( int fd,
                         const char *key,
                         char *p_out,
                         int out_len,
                         int *pos_begin,
                         int *pos_end)
{
    char *p_value = NULL;
    struct stat st;
    char *p_buf;
    char *line;
    char *p;

    if (fstat(fd, &st) != 0)
    {
        error("stat failed (%s)", strerror(errno));
        return NULL;
    }

    p_buf = malloc(st.st_size + 1);

    if (!p_buf)
        return NULL;

    p = p_buf;

    if (read(fd, p, st.st_size) < 0)
    {
        error("read failed %s", strerror(errno));
        return NULL;
    }
    *(p_buf + st.st_size) = 0;

    /* tokenize first line */
    line = strtok(p, UNV_DELIM);

    while (line && (p_value == NULL))
    {
        /* check for match */
        if (strncmp(line, key, strlen(key)) == 0)
        {
            if (pos_begin)
                *pos_begin = (line-p_buf);

            /* point line to key value */
            line += strlen(key) + 1;

            /* make sure supplied buffer isn't overflowed */
            if (out_len < (int)strlen(line))
            {
                error("output buffer too small (%d %d)", out_len, (int)strlen(line));
                info("line %s\n", line);
                return NULL;
            }

            p_value = p_out;
            /* should be ok to just strcpy from 'line' as
             * strrok shall null-terminate the token
             * in the above call */
            strcpy(p_value, line);

            verbose("found [%s=%s]", key, p_value);

            if (pos_end)
                *pos_end = (line-p_buf) + strlen(line) + strlen(UNV_DELIM);
        }

        /* check next line */
        line = strtok(NULL, UNV_DELIM);
    }

    free(p_buf);

    /* rewind */
    lseek(fd, 0, SEEK_SET);

    return p_value;
}


/*****************************************************************************
**
** MAIN API
**
******************************************************************************/





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

int unv_create_directory(const char *path)
{
    int status = 0;
    char tmpbuf[128];
    char *p_copy;
    char *p;

    verbose("CREATE DIR %s", path);

    if (check_caller_context() == 0)
        return -1;

    /* assumes absolute paths */
    if (strncmp(path, "./", 2) == 0)
    {
        error("%s not an absolute path", path);
        return -1;
    }

    /* try creating dir directly */
    if (mk_dir(path) == 0)
        return 0;

    /* directory does not exit, create it */

    /* first make sure we won't overflow the path buffer */
    if ((strlen(path)+1) > sizeof(tmpbuf))
        return -1;

    /* setup scratch buffer, make sure path is ended with / */
    sprintf(tmpbuf, "%s/", path);

    p_copy = tmpbuf;

    p = strchr(p_copy+1, '/'); /* skip root */

    while ((status == 0) && p)
    {
        /*
         * temporarily null terminate to allow creating
         * directories up to this point
         */
        *p= '\0';
        status = mk_dir(p_copy);
        *p= '/';

        /* find next */
        p = strchr(++p, '/');
    }

    return status;
}

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

int unv_create_file(const char *filename)
{
    int fd;
    char *p;
    char *path = strdup(filename); /* rw copy */

    if (check_caller_context() == 0)
        return -1;

    /* separate path from filename */
    p = strrchr(path, '/');

    if (p)
    {
        *p = '\0';
        if (unv_create_directory(path) < 0)
        {
            free(path);
            return -1;
        }
    }

    free(path);

    verbose("CREATE FILE %s", filename);

    fd = open(filename, O_RDWR|O_CREAT, FILE_MODE);

    if (fd < 0)
    {
        error("file failed to create %s errno: (%s)", filename, strerror(errno));
        return -1;
    }

    close(fd);

    return 0;
}


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
******************************************************************************/

char* unv_read_key( const char *path,
                    const char *key,
                    char *p_out,
                    int out_len)
{
    int fd;
    char *p_search;

    verbose("READ KEY [%s]", key);

    /* sanity check */
    if (key == NULL)
        return NULL;

    if (check_caller_context() == 0)
        return NULL;

    fd = open(path, O_RDONLY, FILE_MODE);

    if (fd < 0) {
       error("file failed to open %s (%s)", path, strerror(errno));
       return NULL;
    }

    p_search = get_keyval(fd, key, p_out, out_len, NULL, NULL);

    close(fd);

    return p_search;
}

/*******************************************************************************
**
** Function         unv_read_key_iter
**
** Description      Reads keyvalue from file iteratively
**                  Path must be an existing absolute path
**                  Must be called from BTIF task context
**
** Parameters
**                  path        : path of keyfile
**                  cb          : iteration callback invoked for each entry read
**                  userdata    : context specific userdata passed onto callback
**
** Returns          0 if successful, -1 if failure
**
*******************************************************************************/

int unv_read_key_iter( const char *path,
                       unv_iter_cb cb,
                       void *userdata )
{
    int fd;
    char *p, *p_buf;
    struct stat st;
    char *line;
    int cnt = 0;

    verbose("READ KEY ITER");

    if (check_caller_context() == 0)
        return -1;

    fd = open(path, O_RDONLY, FILE_MODE);

    if (fd < 0) {
       error("file failed to open %s (%s)", path, strerror(errno));
       return -1;
    }

    if (fstat(fd, &st) != 0)
    {
       error("stat failed (%s)", strerror(errno));
       return -1;
    }

    p_buf = malloc(st.st_size + 1);

    if (!p_buf)
       return -1;

    p = p_buf;

    if (read(fd, p, st.st_size) < 0)
    {
        error("read failed %s", strerror(errno));
        free(p_buf);
        return -1;
    }

    *(p + st.st_size) = 0;

    /* tokenize first line */
    line = strtok(p, UNV_DELIM);
    while (line)
    {
        char key[128], value[128];
        char *needle;

        needle = strchr(line, ' ');
        memset(key, 0, sizeof(key));
        memcpy(key, line, (needle-line));
        needle++;
        strcpy(value, needle);

        cb(key, value, userdata);
        cnt++;

        line = strtok(NULL, UNV_DELIM);
    }

    free(p_buf);

    close(fd);

    return cnt;
}

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
                   const char *value )
{
    int fd;
    int pos_start = 0;
    int pos_stop = 0;
    char *keyval = NULL;
    char *p_line = malloc(UNV_MAXLINE_LENGTH);

    verbose("WRITE KEY [%s %s]", key, value);

    /* sanity check */
    if (key == NULL)
        return -1;

    if (check_caller_context() == 0)
        return -1;

    fd = open(path, O_RDWR, FILE_MODE);

    if (fd < 0)
    {
       error("file failed to create %s (%s)", path, strerror(errno));
       return -1;
    }

    /* check if key already present */
    keyval = get_keyval(fd, key, p_line, UNV_MAXLINE_LENGTH, &pos_start, &pos_stop);

    if (keyval)
    {
        update_key(fd, key, value, pos_start, pos_stop);
    }
    else
    {
        /* append at end of file */
        lseek(fd, 0, SEEK_END);
        write_keyval(fd, key, value);
    }

    free(p_line);
    close(fd);

    return 0;
}

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
                    const char *key )
{
    int fd;
    char *p_search;
    int pos_begin = 0;
    int pos_stop = 0;
    char *p_line = malloc(UNV_MAXLINE_LENGTH);

    verbose("READ KEY [%s]", key);

    /* sanity check */
    if (key == NULL)
        return -1;

    if (check_caller_context() == 0)
        return -1;

    fd = open(path, O_RDWR, FILE_MODE);

    if (fd < 0) {
       error("file failed to open %s (%s)", path, strerror(errno));
       return -1;
    }

    p_search = get_keyval(fd, key, p_line, UNV_MAXLINE_LENGTH, &pos_begin, &pos_stop);

    if (p_search)
    {
        /* NULL value entry means remove key/val line in file */
        update_key(fd, key, NULL, pos_begin, pos_stop);
    }

    return (p_search ? 0 : -1);
}


