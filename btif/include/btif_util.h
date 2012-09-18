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
 *  Filename:      btif_util.h
 *
 *  Description:
 *
 ***********************************************************************************/

#ifndef BTIF_UTIL_H
#define BTIF_UTIL_H

#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>
#include <utils/Log.h>

#include "data_types.h"
#include "bt_types.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define CASE_RETURN_STR(const) case const: return #const;

/************************************************************************************
**  Type definitions for callback functions
************************************************************************************/

typedef char bdstr_t[18];

/************************************************************************************
**  Type definitions and return values
************************************************************************************/


/************************************************************************************
**  Functions
************************************************************************************/

const char* dump_bt_status(bt_status_t status);
const char* dump_dm_search_event(UINT16 event);
const char* dump_dm_event(UINT16 event);
const char* dump_hf_event(UINT16 event);
const char* dump_hh_event(UINT16 event);
const char* dump_hf_conn_state(UINT16 event);
const char* dump_hf_call_state(bthf_call_state_t call_state);
const char* dump_property_type(bt_property_type_t type);
const char* dump_hf_audio_state(UINT16 event);
const char* dump_adapter_scan_mode(bt_scan_mode_t mode);
const char* dump_thread_evt(bt_cb_thread_evt evt);

const char* dump_av_conn_state(UINT16 event);
const char* dump_av_audio_state(UINT16 event);

int str2bd(char *str, bt_bdaddr_t *addr);
char *bd2str(bt_bdaddr_t *addr, bdstr_t *bdstr);

UINT32 devclass2uint(DEV_CLASS dev_class);
void uint2devclass(UINT32 dev, DEV_CLASS dev_class);
void uuid16_to_uuid128(uint16_t uuid16, bt_uuid_t* uuid128);

void uuid_to_string(bt_uuid_t *p_uuid, char *str);
void string_to_uuid(char *str, bt_uuid_t *p_uuid);
int ascii_2_hex (char *p_ascii, int len, UINT8 *p_hex);

#endif /* BTIF_UTIL_H */
