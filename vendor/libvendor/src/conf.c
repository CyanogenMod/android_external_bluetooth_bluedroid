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
 *  Filename:      conf.c
 *
 *  Description:   Contains functions to conduct run-time module configuration
 *                 based on entries present in the .conf file
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_conf"

#include <utils/Log.h>
#include <string.h>
#include "bt_vendor_brcm.h"

/******************************************************************************
**  Externs
******************************************************************************/
int userial_set_port(char *p_conf_name, char *p_conf_value, int param);
int hw_set_patch_file_path(char *p_conf_name, char *p_conf_value, int param);
int btsnoop_set_logfile(char *p_conf_name, char *p_conf_value, int param);
int btsnoop_enable_logging(char *p_conf_name, char *p_conf_value, int param);
int debug_cfg(char *p_conf_name, char *p_conf_value, int param);


/******************************************************************************
**  Local type definitions
******************************************************************************/

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t"
#define CONF_MAX_LINE_LEN 255

typedef int (conf_action_t)(char *p_conf_name, char *p_conf_value, int param);

typedef struct {
    const char *conf_entry;
    conf_action_t *p_action;
    int param;
} conf_entry_t;

/******************************************************************************
**  Static variables
******************************************************************************/

/*
 * Current supported entries and corresponding action functions
 */
static const conf_entry_t conf_table[] = {
    {"UartPort", userial_set_port, 0},
    {"FwPatchFilePath", hw_set_patch_file_path, 0},
    {"EnableDebug", debug_cfg, DEBUG_ON},
    {"BtSnoopLogOutput", btsnoop_enable_logging, 0},
    {"BtSnoopFileName", btsnoop_set_logfile, 0},
    {"VndDebug", debug_cfg, TRACE_VND},
    {"HwDebug", debug_cfg, TRACE_HW},
    {"UserialDebug", debug_cfg, TRACE_USERIAL},
    {"HciDebug", debug_cfg, TRACE_HCI},
    {"UpioDebug", debug_cfg, TRACE_UPIO},
    {"BtSnoopDebug", debug_cfg, TRACE_BTSNOOP},
    {(const char *) NULL, NULL, 0}
};

int debug_cfg(char *p_conf_name, char *p_conf_value, int param)
{
    uint8_t enabled = (strcmp(p_conf_value, "true") == 0) ? 1 : 0;

    if (param == DEBUG_ON)
    {
        dbg_mode = (enabled == 1) ? DEBUG_ON : DEBUG_OFF;
    }
    else
    {
        if (enabled == 1)
            traces |= (1 << param);
        else
            traces &= ~(1 << param);
    }

    return 0;
}

/*****************************************************************************
**   CONF INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        vnd_load_conf
**
** Description     Read conf entry from p_path file one by one and call
**                 the corresponding config function
**
** Returns         None
**
*******************************************************************************/
void vnd_load_conf(const char *p_path)
{
    FILE    *p_file;
    char    *p_name;
    char    *p_value;
    conf_entry_t    *p_entry;
    char    line[CONF_MAX_LINE_LEN+1]; /* add 1 for \0 char */

    LOGI("Attempt to load conf from %s", p_path);

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

            p_value = strtok(NULL, CONF_DELIMITERS);

            if (NULL == p_value)
            {
                LOGW("vnd_load_conf: missing value for name: %s", p_name);
                continue;
            }

            p_entry = (conf_entry_t *)conf_table;

            while (p_entry->conf_entry != NULL)
            {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0)
                {
                    p_entry->p_action(p_name, p_value, p_entry->param);
                    break;
                }

                p_entry++;
            }
        }

        fclose(p_file);
    }
    else
    {
        LOGI( "vnd_load_conf file >%s< not found", p_path);
    }
}

