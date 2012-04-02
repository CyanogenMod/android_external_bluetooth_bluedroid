/******************************************************************************
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
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bte_conf.c
 *
 *  Description:   Contains functions to conduct run-time module configuration
 *                 based on entries present in the .conf file
 *
 ******************************************************************************/

#define LOG_TAG "bte_conf"

#include <utils/Log.h>
#include <stdio.h>
#include <string.h>

#include "bt_target.h"

/******************************************************************************
**  Externs
******************************************************************************/
extern BOOLEAN trace_conf_enabled;
void bte_trace_conf(char *p_name, char *p_conf_value);
int device_name_cfg(char *p_conf_name, char *p_conf_value);
int device_class_cfg(char *p_conf_name, char *p_conf_value);
int trace_cfg_onoff(char *p_conf_name, char *p_conf_value);

BD_NAME local_device_default_name = BTM_DEF_LOCAL_NAME;
DEV_CLASS local_device_default_class = {0x40, 0x02, 0x0C};

/******************************************************************************
**  Local type definitions
******************************************************************************/

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "\"=\n\r\t"
#define CONF_COD_DELIMITERS " {,}\t"
#define CONF_MAX_LINE_LEN 255

typedef int (conf_action_t)(char *p_conf_name, char *p_conf_value);

typedef struct {
    const char *conf_entry;
    conf_action_t *p_action;
} conf_entry_t;

/******************************************************************************
**  Static variables
******************************************************************************/

/*
 * Current supported entries and corresponding action functions
 */
/* TODO: Name and Class are duplicated with NVRAM adapter_info. Need to be sorted out */
static const conf_entry_t conf_table[] = {
    /*{"Name", device_name_cfg},
    {"Class", device_class_cfg},*/
    {"TraceConf", trace_cfg_onoff},
    {(const char *) NULL, NULL}
};

/*****************************************************************************
**   FUNCTIONS
*****************************************************************************/

int device_name_cfg(char *p_conf_name, char *p_conf_value)
{
    strcpy((char *)local_device_default_name, p_conf_value);
    return 0;
}

int device_class_cfg(char *p_conf_name, char *p_conf_value)
{
    char *p_token;
    unsigned int x;

    p_token = strtok(p_conf_value, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[0] = (UINT8) x;
    p_token = strtok(NULL, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[1] = (UINT8) x;
    p_token = strtok(NULL, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[2] = (UINT8) x;

    return 0;
}

int trace_cfg_onoff(char *p_conf_name, char *p_conf_value)
{
    trace_conf_enabled = (strcmp(p_conf_value, "true") == 0) ? TRUE : FALSE;
    return 0;
}

/*****************************************************************************
**   CONF INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        bte_load_conf
**
** Description     Read conf entry from p_path file one by one and call
**                 the corresponding config function
**
** Returns         None
**
*******************************************************************************/
void bte_load_conf(const char *p_path)
{
    FILE    *p_file;
    char    *p_name;
    char    *p_value;
    conf_entry_t    *p_entry;
    char    line[CONF_MAX_LINE_LEN+1]; /* add 1 for \0 char */
    BOOLEAN name_matched;

    LOGI("Attempt to load stack conf from %s", p_path);

    if ((p_file = fopen(p_path, "r")) != NULL)
    {
        /* read line by line */
        while (fgets(line, CONF_MAX_LINE_LEN+1, p_file) != NULL)
        {
            if (line[0] == CONF_COMMENT)
                continue;

            p_name = strtok(line, CONF_DELIMITERS);

            if (NULL == p_name)
            {
                continue;
            }

            p_value = strtok(NULL, CONF_VALUES_DELIMITERS);

            if (NULL == p_value)
            {
                LOGW("bte_load_conf: missing value for name: %s", p_name);
                continue;
            }

            name_matched = FALSE;
            p_entry = (conf_entry_t *)conf_table;

            while (p_entry->conf_entry != NULL)
            {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0)
                {
                    name_matched = TRUE;
                    if (p_entry->p_action != NULL)
                        p_entry->p_action(p_name, p_value);
                    break;
                }

                p_entry++;
            }

            if ((name_matched == FALSE) && (trace_conf_enabled == TRUE))
            {
                /* Check if this is a TRC config item */
                bte_trace_conf(p_name, p_value);
            }
        }

        fclose(p_file);
    }
    else
    {
        LOGI( "bte_load_conf file >%s< not found", p_path);
    }
}

