/******************************************************************************
 *
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2002-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains HID DEVICE internal definitions
 *
 ******************************************************************************/

#ifndef HIDD_INT_H
#define HIDD_INT_H

#include "hidd_api.h"
#include "hid_conn.h"
#include "l2c_api.h"

enum {
    HIDD_DEV_NO_CONN,
    HIDD_DEV_CONNECTED
};

typedef struct device_ctb
{
    BOOLEAN    in_use;
    BD_ADDR    addr;

    UINT8      state;

    tHID_CONN  conn;

    BOOLEAN    boot_mode;

    UINT8      idle_time;
} tHID_DEV_DEV_CTB;

typedef struct dev_ctb
{
    tHID_DEV_DEV_CTB        device;

    tHID_DEV_HOST_CALLBACK  *callback;
    tL2CAP_CFG_INFO         l2cap_cfg;
    tL2CAP_CFG_INFO         l2cap_intr_cfg;

    BOOLEAN                 use_in_qos;
    FLOW_SPEC               in_qos;

    BOOLEAN                 reg_flag;
    UINT8                   trace_level;

    BOOLEAN                 allow_incoming;

    BT_HDR                  *pending_data;

    BOOLEAN                 pending_vc_unplug;
} tHID_DEV_CTB;

extern tHID_STATUS hidd_conn_reg(void);
extern void hidd_conn_dereg(void);
extern tHID_STATUS hidd_conn_initiate(void);
extern tHID_STATUS hidd_conn_disconnect(void);
extern tHID_STATUS hidd_conn_send_data(UINT8 channel, UINT8 msg_type, UINT8 param,
                                            UINT8 data, UINT16 len, UINT8 *p_data);

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
** Main Control Block
*******************************************************************************/
#if HID_DYNAMIC_MEMORY == FALSE
HID_API extern tHID_DEV_CTB  hd_cb;
#else
HID_API extern tHID_DEV_CTB *hidd_cb_ptr;
#define hd_cb (*hidd_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif

