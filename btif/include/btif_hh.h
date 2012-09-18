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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/

#ifndef BTIF_HH_H
#define BTIF_HH_H

#include <hardware/bluetooth.h>
#include <hardware/bt_hh.h>
#include <stdint.h>
#include "bta_hh_api.h"

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define BTIF_HH_MAX_HID         8
#define BTIF_HH_MAX_ADDED_DEV   32

#define BTIF_HH_MAX_KEYSTATES            3
#define BTIF_HH_KEYSTATE_MASK_NUMLOCK    0x01
#define BTIF_HH_KEYSTATE_MASK_CAPSLOCK   0x02
#define BTIF_HH_KEYSTATE_MASK_SCROLLLOCK 0x04


/*******************************************************************************
**  Type definitions and return values
********************************************************************************/

typedef enum
{
    BTIF_HH_DISABLED =   0,
    BTIF_HH_ENABLED,
    BTIF_HH_DISABLING,
    BTIF_HH_DEV_UNKNOWN,
    BTIF_HH_DEV_CONNECTING,
    BTIF_HH_DEV_CONNECTED,
    BTIF_HH_DEV_DISCONNECTED
} BTIF_HH_STATUS;

typedef struct
{
    bthh_connection_state_t       dev_status;
    UINT8                         dev_handle;
    bt_bdaddr_t                   bd_addr;
    tBTA_HH_ATTR_MASK             attr_mask;
    UINT8                         sub_class;
    UINT8                         app_id;
    int                           fd;
    BT_HDR                        *p_buf;
} btif_hh_device_t;

/* Control block to maintain properties of devices */
typedef struct
{
    UINT8             dev_handle;
    bt_bdaddr_t       bd_addr;
    tBTA_HH_ATTR_MASK attr_mask;
} btif_hh_added_device_t;

/**
 * BTIF-HH control block to maintain added devices and currently
 * connected hid devices
 */
typedef struct
{
    BTIF_HH_STATUS          status;
    btif_hh_device_t        devices[BTIF_HH_MAX_HID];
    UINT32                  device_num;
    btif_hh_added_device_t  added_devices[BTIF_HH_MAX_ADDED_DEV];
    btif_hh_device_t        *p_curr_dev;
} btif_hh_cb_t;


/*******************************************************************************
**  Functions
********************************************************************************/

extern btif_hh_cb_t btif_hh_cb;

extern btif_hh_device_t *btif_hh_find_connected_dev_by_handle(UINT8 handle);
extern void btif_hh_remove_device(bt_bdaddr_t bd_addr);
extern bt_status_t btif_hh_virtual_unplug(bt_bdaddr_t *bd_addr);
extern void btif_hh_disconnect(bt_bdaddr_t *bd_addr);

BOOLEAN btif_hh_add_added_dev(bt_bdaddr_t bd_addr, tBTA_HH_ATTR_MASK attr_mask);

#endif
