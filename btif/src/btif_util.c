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
 *  Filename:      btif_util.c
 *
 *  Description:   Miscellaneous helper functions
 *
 *
 ***********************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>
#include <hardware/bt_av.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define LOG_TAG "BTIF_UTIL"
#include "btif_common.h"
#include "bta_api.h"
#include "gki.h"
#include "btu.h"
#include "bte.h"
#include "bd.h"
#include "btif_dm.h"
#include "btif_util.h"
#include "bta_ag_api.h"
#include "bta_hh_api.h"



/************************************************************************************
**  Constants & Macros
************************************************************************************/
#define ISDIGIT(a)  ((a>='0') && (a<='9'))
#define ISXDIGIT(a) (((a>='0') && (a<='9'))||((a>='A') && (a<='F'))||((a>='a') && (a<='f')))

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

/*****************************************************************************
**   Logging helper functions
*****************************************************************************/

int str2bd(char *str, bt_bdaddr_t *addr)
{
    int32_t i = 0;
    for (i = 0; i < 6; i++) {
       addr->address[i] = (uint8_t) strtoul(str, (char **)&str, 16);
       str++;
    }
    return 0;
}

char *bd2str(bt_bdaddr_t *bdaddr, bdstr_t *bdstr)
{
    char *addr = (char *) bdaddr->address;

    sprintf((char*)bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
                       (int)addr[0],(int)addr[1],(int)addr[2],
                       (int)addr[3],(int)addr[4],(int)addr[5]);
    return (char *)bdstr;
}

UINT32 devclass2uint(DEV_CLASS dev_class)
{
    UINT32 cod = 0;

    /* if COD is 0, irrespective of the device type set it to Unclassified device */
    cod = (dev_class[2]) | (dev_class[1] << 8) | (dev_class[0] << 16);

    return cod;
}
void uint2devclass(UINT32 cod, DEV_CLASS dev_class)
{
    dev_class[2] = (UINT8)cod;
    dev_class[1] = (UINT8)(cod >> 8);
    dev_class[0] = (UINT8)(cod >> 16);
}

static const UINT8  sdp_base_uuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                       0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

void uuid16_to_uuid128(uint16_t uuid16, bt_uuid_t* uuid128)
{
    uint16_t uuid16_bo;
    memset(uuid128, 0, sizeof(bt_uuid_t));

    memcpy(uuid128->uu, sdp_base_uuid, MAX_UUID_SIZE);
    uuid16_bo = ntohs(uuid16);
    memcpy(uuid128->uu + 2, &uuid16_bo, sizeof(uint16_t));
}

void string_to_uuid(char *str, bt_uuid_t *p_uuid)
{
    uint32_t uuid0, uuid4;
    uint16_t uuid1, uuid2, uuid3, uuid5;

    sscanf(str, "%08x-%04hx-%04hx-%04hx-%08x%04hx",
                &uuid0, &uuid1, &uuid2, &uuid3, &uuid4, &uuid5);

    uuid0 = htonl(uuid0);
    uuid1 = htons(uuid1);
    uuid2 = htons(uuid2);
    uuid3 = htons(uuid3);
    uuid4 = htonl(uuid4);
    uuid5 = htons(uuid5);

    memcpy(&(p_uuid->uu[0]), &uuid0, 4);
    memcpy(&(p_uuid->uu[4]), &uuid1, 2);
    memcpy(&(p_uuid->uu[6]), &uuid2, 2);
    memcpy(&(p_uuid->uu[8]), &uuid3, 2);
    memcpy(&(p_uuid->uu[10]), &uuid4, 4);
    memcpy(&(p_uuid->uu[14]), &uuid5, 2);

    return;

}

void uuid_to_string(bt_uuid_t *p_uuid, char *str)
{
    uint32_t uuid0, uuid4;
    uint16_t uuid1, uuid2, uuid3, uuid5;

    memcpy(&uuid0, &(p_uuid->uu[0]), 4);
    memcpy(&uuid1, &(p_uuid->uu[4]), 2);
    memcpy(&uuid2, &(p_uuid->uu[6]), 2);
    memcpy(&uuid3, &(p_uuid->uu[8]), 2);
    memcpy(&uuid4, &(p_uuid->uu[10]), 4);
    memcpy(&uuid5, &(p_uuid->uu[14]), 2);

    sprintf((char *)str, "%.8x-%.4x-%.4x-%.4x-%.8x%.4x",
            ntohl(uuid0), ntohs(uuid1),
            ntohs(uuid2), ntohs(uuid3),
            ntohl(uuid4), ntohs(uuid5));
    return;
}

/*****************************************************************************
**  Function        ascii_2_hex
**
**  Description     This function converts an ASCII string into HEX
**
**  Returns         the number of hex bytes filled.
*/
int ascii_2_hex (char *p_ascii, int len, UINT8 *p_hex)
{
    int     x;
    UINT8   c;

    for (x = 0; (x < len) && (*p_ascii); x++)
    {
        if (ISDIGIT (*p_ascii))
            c = (*p_ascii - '0') << 4;
        else
            c = (toupper(*p_ascii) - 'A' + 10) << 4;

        p_ascii++;
        if (*p_ascii)
        {
            if (ISDIGIT (*p_ascii))
                c |= (*p_ascii - '0');
            else
                c |= (toupper(*p_ascii) - 'A' + 10);

            p_ascii++;
        }
        *p_hex++ = c;
    }

    return (x);
}


const char* dump_dm_search_event(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTA_DM_INQ_RES_EVT)
        CASE_RETURN_STR(BTA_DM_INQ_CMPL_EVT)
        CASE_RETURN_STR(BTA_DM_DISC_RES_EVT)
        CASE_RETURN_STR(BTA_DM_DISC_BLE_RES_EVT)
        CASE_RETURN_STR(BTA_DM_DISC_CMPL_EVT)
        CASE_RETURN_STR(BTA_DM_DI_DISC_CMPL_EVT)
        CASE_RETURN_STR(BTA_DM_SEARCH_CANCEL_CMPL_EVT)

        default:
            return "UNKNOWN MSG ID";
     }
}


const char* dump_property_type(bt_property_type_t type)
{
    switch(type)
    {
        CASE_RETURN_STR(BT_PROPERTY_BDNAME)
        CASE_RETURN_STR(BT_PROPERTY_BDADDR)
        CASE_RETURN_STR(BT_PROPERTY_UUIDS)
        CASE_RETURN_STR(BT_PROPERTY_CLASS_OF_DEVICE)
        CASE_RETURN_STR(BT_PROPERTY_TYPE_OF_DEVICE)
        CASE_RETURN_STR(BT_PROPERTY_REMOTE_RSSI)
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT)
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_BONDED_DEVICES)
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_SCAN_MODE)
        CASE_RETURN_STR(BT_PROPERTY_REMOTE_FRIENDLY_NAME)

        default:
            return "UNKNOWN PROPERTY ID";
    }
}


const char* dump_hf_event(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTA_AG_ENABLE_EVT)
        CASE_RETURN_STR(BTA_AG_REGISTER_EVT)
        CASE_RETURN_STR(BTA_AG_OPEN_EVT)
        CASE_RETURN_STR(BTA_AG_CLOSE_EVT)
        CASE_RETURN_STR(BTA_AG_CONN_EVT)
        CASE_RETURN_STR(BTA_AG_AUDIO_OPEN_EVT)
        CASE_RETURN_STR(BTA_AG_AUDIO_CLOSE_EVT)
        CASE_RETURN_STR(BTA_AG_SPK_EVT)
        CASE_RETURN_STR(BTA_AG_MIC_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CKPD_EVT)
        CASE_RETURN_STR(BTA_AG_DISABLE_EVT)

        CASE_RETURN_STR(BTA_AG_AT_A_EVT)
        CASE_RETURN_STR(BTA_AG_AT_D_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CHLD_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CHUP_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CIND_EVT)
        CASE_RETURN_STR(BTA_AG_AT_VTS_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BINP_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BLDN_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BVRA_EVT)
        CASE_RETURN_STR(BTA_AG_AT_NREC_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CNUM_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BTRH_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CLCC_EVT)
        CASE_RETURN_STR(BTA_AG_AT_COPS_EVT)
        CASE_RETURN_STR(BTA_AG_AT_UNAT_EVT)
        CASE_RETURN_STR(BTA_AG_AT_CBC_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BAC_EVT)
        CASE_RETURN_STR(BTA_AG_AT_BCS_EVT)

        default:
            return "UNKNOWN MSG ID";
     }
}

const char* dump_hh_event(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTA_HH_ENABLE_EVT)
        CASE_RETURN_STR(BTA_HH_DISABLE_EVT)
        CASE_RETURN_STR(BTA_HH_OPEN_EVT)
        CASE_RETURN_STR(BTA_HH_CLOSE_EVT)
        CASE_RETURN_STR(BTA_HH_GET_DSCP_EVT)
        CASE_RETURN_STR(BTA_HH_GET_PROTO_EVT)
        CASE_RETURN_STR(BTA_HH_GET_RPT_EVT)
        CASE_RETURN_STR(BTA_HH_GET_IDLE_EVT)
        CASE_RETURN_STR(BTA_HH_SET_PROTO_EVT)
        CASE_RETURN_STR(BTA_HH_SET_RPT_EVT)
        CASE_RETURN_STR(BTA_HH_SET_IDLE_EVT)
        CASE_RETURN_STR(BTA_HH_VC_UNPLUG_EVT)
        CASE_RETURN_STR(BTA_HH_ADD_DEV_EVT)
        CASE_RETURN_STR(BTA_HH_RMV_DEV_EVT)
        CASE_RETURN_STR(BTA_HH_API_ERR_EVT)
        default:
            return "UNKNOWN MSG ID";
     }
}


const char* dump_hf_conn_state(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTHF_CONNECTION_STATE_DISCONNECTED)
        CASE_RETURN_STR(BTHF_CONNECTION_STATE_CONNECTING)
        CASE_RETURN_STR(BTHF_CONNECTION_STATE_CONNECTED)
        CASE_RETURN_STR(BTHF_CONNECTION_STATE_SLC_CONNECTED)
        CASE_RETURN_STR(BTHF_CONNECTION_STATE_DISCONNECTING)
        default:
            return "UNKNOWN MSG ID";
    }
}

const char* dump_hf_call_state(bthf_call_state_t call_state)
{
    switch(call_state)
    {
        CASE_RETURN_STR(BTHF_CALL_STATE_IDLE)
        CASE_RETURN_STR(BTHF_CALL_STATE_HELD)
        CASE_RETURN_STR(BTHF_CALL_STATE_DIALING)
        CASE_RETURN_STR(BTHF_CALL_STATE_ALERTING)
        CASE_RETURN_STR(BTHF_CALL_STATE_INCOMING)
        CASE_RETURN_STR(BTHF_CALL_STATE_WAITING)
        CASE_RETURN_STR(BTHF_CALL_STATE_ACTIVE)
        default:
            return "UNKNOWN CALL STATE";
    }
}

const char* dump_thread_evt(bt_cb_thread_evt evt)
{
    switch(evt)
    {
        CASE_RETURN_STR(ASSOCIATE_JVM)
        CASE_RETURN_STR(DISASSOCIATE_JVM)

        default:
            return "unknown thread evt";
    }
}


const char* dump_hf_audio_state(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTHF_AUDIO_STATE_DISCONNECTED)
        CASE_RETURN_STR(BTHF_AUDIO_STATE_CONNECTING)
        CASE_RETURN_STR(BTHF_AUDIO_STATE_CONNECTED)
        CASE_RETURN_STR(BTHF_AUDIO_STATE_DISCONNECTING)
        default:
           return "UNKNOWN MSG ID";

    }
}

const char* dump_av_conn_state(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTAV_CONNECTION_STATE_DISCONNECTED)
        CASE_RETURN_STR(BTAV_CONNECTION_STATE_CONNECTING)
        CASE_RETURN_STR(BTAV_CONNECTION_STATE_CONNECTED)
        CASE_RETURN_STR(BTAV_CONNECTION_STATE_DISCONNECTING)
        default:
            return "UNKNOWN MSG ID";
    }
}

const char* dump_av_audio_state(UINT16 event)
{
    switch(event)
    {
        CASE_RETURN_STR(BTAV_AUDIO_STATE_REMOTE_SUSPEND)
        CASE_RETURN_STR(BTAV_AUDIO_STATE_STOPPED)
        CASE_RETURN_STR(BTAV_AUDIO_STATE_STARTED)
        default:
            return "UNKNOWN MSG ID";
    }
}

const char* dump_adapter_scan_mode(bt_scan_mode_t mode)
{
    switch(mode)
    {
        CASE_RETURN_STR(BT_SCAN_MODE_NONE)
        CASE_RETURN_STR(BT_SCAN_MODE_CONNECTABLE)
        CASE_RETURN_STR(BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE)

        default:
            return "unknown scan mode";
    }
}

const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown scan mode";
    }
}



