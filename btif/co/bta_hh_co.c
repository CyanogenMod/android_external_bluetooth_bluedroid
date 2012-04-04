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

//#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "btif_hh.h"

#include "bta_api.h"
#include "bta_hh_api.h"



#define UINT8 uint8_t
#define UINT16 uint16_t

#define BTHID_HID_INFO  1
#define LOG_TAG "BTA_HH_CO"
#define BTHID_MAX_DEV_NAME_LEN  128
#define BTHID_MAX_DSCP_BUF_LEN  884

typedef struct BTHID_CONTROL
{
    int   dscp_len;
    char  dscp_buf[BTHID_MAX_DSCP_BUF_LEN];
    char  dev_name[BTHID_MAX_DEV_NAME_LEN];
    unsigned short vendor_id;
    unsigned short product_id;
    unsigned short version;
    unsigned short ctry_code;
} tBTHID_CONTROL;


static UINT8 HID_REPORT_START[] = {1,0,0};
#define HID_REPORT_CAPSLOCK  0x39
#define HID_REPORT_NUMLOCK   0x53
#define HID_REPORT_SCROLLLOCK 0x47
static int saved_keyevents =0;

static void process_rpt_keys_with_state(UINT8 dev_handle, UINT8 *p_rpt, UINT16 len)
{
/*
 --TODO
   */
}

/*******************************************************************************
**
** Function      bta_hh_co_open
**
** Description   When connection is opened, this call-out function is executed
**               by HH to do platform specific initialization.
**
** Returns       void.
*******************************************************************************/
void bta_hh_co_open(UINT8 dev_handle, UINT8 sub_class, tBTA_HH_ATTR_MASK attr_mask,
                    UINT8 app_id)
{
    UINT32 i;
    btif_hh_device_t *p_dev = NULL;

    BTIF_TRACE_WARNING5("%s: dev_handle = %d, subclass = 0x%02X, attr_mask = 0x%04X, app_id = %d",
         __FUNCTION__, dev_handle, sub_class, attr_mask, app_id);

    if (dev_handle == BTA_HH_INVALID_HANDLE) {
        BTIF_TRACE_WARNING2("%s: Oops, dev_handle (%d) is invalid...", __FUNCTION__, dev_handle);
        return;
    }

    for (i = 0; i < BTIF_HH_MAX_HID; i++) {
        p_dev = &btif_hh_cb.devices[i];
        if (p_dev->dev_status != BTHH_CONN_STATE_UNKNOWN && p_dev->dev_handle == dev_handle) {
            // We found a device with the same handle. Must be a device reconnected.
            BTIF_TRACE_WARNING2("%s: Found an existing device with the same handle. dev_status = %d",
                 __FUNCTION__, p_dev->dev_status);
            BTIF_TRACE_WARNING6("%s:     bd_addr = [%02X:%02X:%02X:%02X:%02X:]", __FUNCTION__,
                 p_dev->bd_addr.address[0], p_dev->bd_addr.address[1], p_dev->bd_addr.address[2],
                 p_dev->bd_addr.address[3], p_dev->bd_addr.address[4]);
                 BTIF_TRACE_WARNING4("%s:     attr_mask = 0x%04x, sub_class = 0x%02x, app_id = %d", __FUNCTION__,
                 p_dev->attr_mask, p_dev->sub_class, p_dev->app_id);

            if(p_dev->fd<0) {
                p_dev->fd = open("/dev/bthid", O_RDWR);
                BTIF_TRACE_WARNING2("%s: bthid fd = %d", __FUNCTION__, p_dev->fd);
            }

            break;
        }
        p_dev = NULL;
    }

    if (p_dev == NULL) {
        // Did not find a device reconnection case. Find an empty slot now.
        for (i = 0; i < BTIF_HH_MAX_HID; i++) {
            if (btif_hh_cb.devices[i].dev_status == BTHH_CONN_STATE_UNKNOWN) {
                p_dev = &btif_hh_cb.devices[i];
                p_dev->dev_handle = dev_handle;
                p_dev->attr_mask  = attr_mask;
                p_dev->sub_class  = sub_class;
                p_dev->app_id     = app_id;

                btif_hh_cb.device_num++;
                // This is a new device,open the bthid driver now.
                p_dev->fd = open("/dev/bthid", O_RDWR);
                BTIF_TRACE_WARNING3("%s: bthid fd = %d, errno=%d", __FUNCTION__, p_dev->fd,errno);
                break;
            }
        }
    }

    if (p_dev == NULL) {
        BTIF_TRACE_WARNING1("%s: Error: too many HID devices are connected", __FUNCTION__);
        return;
    }

    p_dev->dev_status = BTHH_CONN_STATE_CONNECTED;
    BTIF_TRACE_WARNING2("%s: Return device status %d", __FUNCTION__, p_dev->dev_status);
}


/*******************************************************************************
**
** Function      bta_hh_co_close
**
** Description   When connection is closed, this call-out function is executed
**               by HH to do platform specific finalization.
**
** Parameters    dev_handle  - device handle
**                  app_id      - application id
**
** Returns          void.
*******************************************************************************/
void bta_hh_co_close(UINT8 dev_handle, UINT8 app_id)
{
    BTIF_TRACE_WARNING3("%s: dev_handle = %d, app_id = %d", __FUNCTION__, dev_handle, app_id);
}


/*******************************************************************************
**
** Function         bta_hh_co_data
**
** Description      This function is executed by BTA when HID host receive a data
**                  report.
**
** Parameters       dev_handle  - device handle
**                  *p_rpt      - pointer to the report data
**                  len         - length of report data
**                  mode        - Hid host Protocol Mode
**                  sub_clas    - Device Subclass
**                  app_id      - application id
**
** Returns          void
*******************************************************************************/
void bta_hh_co_data(UINT8 dev_handle, UINT8 *p_rpt, UINT16 len, tBTA_HH_PROTO_MODE mode,
                    UINT8 sub_class, UINT8 ctry_code, BD_ADDR peer_addr, UINT8 app_id)
{
    btif_hh_device_t *p_dev;
    //tBTA_HH_BOOT_RPT  rpt;

    BTIF_TRACE_WARNING6("%s: dev_handle = %d, subclass = 0x%02X, mode = %d, "
         "ctry_code = %d, app_id = %d",
         __FUNCTION__, dev_handle, sub_class, mode, ctry_code, app_id);

    p_dev = btif_hh_find_connected_dev_by_handle(dev_handle);
    if (p_dev == NULL) {
        BTIF_TRACE_WARNING2("%s: Error: unknown HID device handle %d", __FUNCTION__, dev_handle);
        return;
    }
    // Send the HID report to the kernel.
    if (p_dev->fd >= 0) {
        BTIF_TRACE_WARNING3("%s: fd = %d, len = %d", __FUNCTION__, p_dev->fd, len);
        /* TODO: keystate
        */
        write(p_dev->fd, p_rpt, len);
    }
    else {
        BTIF_TRACE_WARNING3("%s: Error: fd = %d, len = %d", __FUNCTION__, p_dev->fd, len);
    }
}


/*******************************************************************************
**
** Function         bta_hh_co_send_hid_info
**
** Description      This function is called in btif_hh.c to process DSCP received.
**
** Parameters       dev_handle  - device handle
**                  dscp_len    - report descriptor length
**                  *p_dscp     - report descriptor
**
** Returns          void
*******************************************************************************/
void bta_hh_co_send_hid_info(btif_hh_device_t *p_dev, char *dev_name, UINT16 vendor_id,
                             UINT16 product_id, UINT16 version, UINT8 ctry_code,
                             int dscp_len, UINT8 *p_dscp)
{
    int result;
    tBTHID_CONTROL  ctrl;

    /*
    int i;
    for (i = 0; i < dscp_len; i += 16) {
        LOGI("%02X %02X %02X %02X %02X %02X %02X %02X "
             "%02X %02X %02X %02X %02X %02X %02X %02X",
             p_dscp[i],    p_dscp[i+1],  p_dscp[i+2],  p_dscp[i+3],
             p_dscp[i+4],  p_dscp[i+5],  p_dscp[i+6],  p_dscp[i+7],
             p_dscp[i+8],  p_dscp[i+9],  p_dscp[i+10], p_dscp[i+11],
             p_dscp[i+12], p_dscp[i+13], p_dscp[i+14], p_dscp[i+15]);
    }
    */
    if (p_dev->fd < 0) {
        BTIF_TRACE_WARNING3("%s: Error: fd = %d, dscp_len = %d", __FUNCTION__, p_dev->fd, dscp_len);
        return;
    }

    if (dscp_len > BTHID_MAX_DSCP_BUF_LEN) {
        BTIF_TRACE_WARNING2("%s: Error: HID report descriptor is too large. dscp_len = %d", __FUNCTION__, dscp_len);
        return;
    }

    BTIF_TRACE_WARNING4("%s: fd = %d, name = [%s], dscp_len = %d", __FUNCTION__, p_dev->fd, dev_name, dscp_len);
    BTIF_TRACE_WARNING5("%s: vendor_id = 0x%04x, product_id = 0x%04x, version= 0x%04x, ctry_code=0x%02x",
         __FUNCTION__, vendor_id, product_id, version, ctry_code);

    memset(&ctrl, 0, sizeof(tBTHID_CONTROL));

    ctrl.dscp_len = dscp_len;
    memcpy(ctrl.dscp_buf, p_dscp, dscp_len);
    strncpy(ctrl.dev_name, dev_name, BTHID_MAX_DEV_NAME_LEN - 1);
    ctrl.vendor_id  = vendor_id;
    ctrl.product_id = product_id;
    ctrl.version    = version;
    ctrl.ctry_code  = ctry_code;

    BTIF_TRACE_WARNING1("%s: send ioctl", __FUNCTION__);

    result = ioctl(p_dev->fd, BTHID_HID_INFO, &ctrl);

    BTIF_TRACE_WARNING4("%s: fd = %d, dscp_len = %d, result = %d", __FUNCTION__, p_dev->fd, dscp_len, result);

    if (result != 0) {
        BTIF_TRACE_WARNING2("%s: Error: failed to send DSCP, result = %d", __FUNCTION__, result);

        /* The HID report descriptor is corrupted. Close the driver. */
        close(p_dev->fd);
        p_dev->fd = -1;
    }
}


