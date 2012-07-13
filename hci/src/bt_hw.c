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
 *  Filename:      bt_hw.c
 *
 *  Description:   Bluedroid libbt-vendor callback functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_hw"

#include <dlfcn.h>
#include <utils/Log.h>
#include <pthread.h>
#include "bt_vendor_lib.h"
#include "bt_hci_bdroid.h"
#include "userial.h"

/******************************************************************************
**  Externs
******************************************************************************/

uint8_t hci_h4_send_int_cmd(uint16_t opcode, HC_BT_HDR *p_buf, \
                                  tINT_CMD_CBACK p_cback);
void lpm_vnd_cback(uint8_t vnd_result);
void hci_h4_get_acl_data_length(void);

/******************************************************************************
**  Variables
******************************************************************************/

bt_vendor_interface_t *bt_vnd_if=NULL;

/******************************************************************************
**  Functions
******************************************************************************/

/******************************************************************************
**
** Function         fwcfg_cb
**
** Description      HOST/CONTROLLER VENDOR LIB CALLBACK API - This function is
**                  called when the libbt-vendor completed firmware
**                  configuration process
**
** Returns          None
**
******************************************************************************/
static void fwcfg_cb(bt_vendor_op_result_t result)
{
    bt_hc_postload_result_t status = (result == BT_VND_OP_RESULT_SUCCESS) ? \
                                     BT_HC_PRELOAD_SUCCESS : BT_HC_PRELOAD_FAIL;

    if (bt_hc_cbacks)
        bt_hc_cbacks->preload_cb(NULL, status);
}

/******************************************************************************
**
** Function         scocfg_cb
**
** Description      HOST/CONTROLLER VENDOR LIB CALLBACK API - This function is
**                  called when the libbt-vendor completed vendor specific SCO
**                  configuration process
**
** Returns          None
**
******************************************************************************/
static void scocfg_cb(bt_vendor_op_result_t result)
{
    /* Continue rest of postload process*/
    hci_h4_get_acl_data_length();
}

/******************************************************************************
**
** Function         lpm_vnd_cb
**
** Description      HOST/CONTROLLER VENDOR LIB CALLBACK API - This function is
**                  called back from the libbt-vendor to indicate the result of
**                  previous LPM enable/disable request
**
** Returns          None
**
******************************************************************************/
static void lpm_vnd_cb(bt_vendor_op_result_t result)
{
    uint8_t status = (result == BT_VND_OP_RESULT_SUCCESS) ? 0 : 1;

    lpm_vnd_cback(status);
}

/******************************************************************************
**
** Function         alloc
**
** Description      HOST/CONTROLLER VENDOR LIB CALLOUT API - This function is
**                  called from the libbt-vendor to request for data buffer
**                  allocation
**
** Returns          NULL / pointer to allocated buffer
**
******************************************************************************/
static void *alloc(int size)
{
    HC_BT_HDR *p_hdr = NULL;

    if (bt_hc_cbacks)
        p_hdr = (HC_BT_HDR *) bt_hc_cbacks->alloc(size);

    return (p_hdr);
}

/******************************************************************************
**
** Function         dealloc
**
** Description      HOST/CONTROLLER VENDOR LIB CALLOUT API - This function is
**                  called from the libbt-vendor to release the data buffer
**                  allocated through the alloc call earlier
**
** Returns          None
**
******************************************************************************/
static void dealloc(void *p_buf)
{
    HC_BT_HDR *p_hdr = (HC_BT_HDR *) p_buf;

    if (bt_hc_cbacks)
        bt_hc_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_hdr+1));
}

/******************************************************************************
**
** Function         xmit_cb
**
** Description      HOST/CONTROLLER VEDNOR LIB CALLOUT API - This function is
**                  called from the libbt-vendor in order to send a prepared
**                  HCI command packet through HCI transport TX function.
**
** Returns          TRUE/FALSE
**
******************************************************************************/
static uint8_t xmit_cb(uint16_t opcode, void *p_buf, tINT_CMD_CBACK p_cback)
{
    return hci_h4_send_int_cmd(opcode, (HC_BT_HDR *)p_buf, p_cback);
}

/*****************************************************************************
**   The libbt-vendor Callback Functions Table
*****************************************************************************/
static const bt_vendor_callbacks_t vnd_callbacks = {
    sizeof(bt_vendor_callbacks_t),
    fwcfg_cb,
    scocfg_cb,
    lpm_vnd_cb,
    alloc,
    dealloc,
    xmit_cb
};

/******************************************************************************
**
** Function         init_vnd_if
**
** Description      Initialize vendor lib interface
**
** Returns          None
**
******************************************************************************/
void init_vnd_if(unsigned char *local_bdaddr)
{
    void *dlhandle;

    dlhandle = dlopen("libbt-vendor.so", RTLD_NOW);
    if (!dlhandle)
    {
        ALOGE("!!! Failed to load libbt-vendor.so !!!");
        return;
    }

    bt_vnd_if = (bt_vendor_interface_t *) dlsym(dlhandle, "BLUETOOTH_VENDOR_LIB_INTERFACE");
    if (!bt_vnd_if)
    {
        ALOGE("!!! Failed to get bt vendor interface !!!");
        return;
    }

    bt_vnd_if->init(&vnd_callbacks, local_bdaddr);
}

