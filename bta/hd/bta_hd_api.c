/******************************************************************************
**
**  File Name:   bta_hd_api.c
**
**  Description: This is the API functions for the HID Device
**               service.
**
**  Copyright (c) 2002-2011, Broadcom Corp., All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
**
******************************************************************************/

#include "bta_api.h"
#include "bd.h"
#include "bta_sys.h"
#include "bta_hd_api.h"
#include "bta_hd_int.h"
#include "gki.h"
#include <string.h>

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_hd_reg =
{
    bta_hd_hdl_event,
    BTA_HdDisable
};

/*******************************************************************************
**
** Function         BTA_HdEnable
**
** Description      Enable the HID Device service. When the enable 
**                  operation is complete the callback function will be
**                  called with a BTA_HD_ENABLE_EVT. This function must
**                  be called before other function in the HD API are
**                  called.
**
**                  If all bytes of the specified bd_addr are 0xff, the 
**                  peer address is considered as unknown. The HID device listens
**                  for incoming connection request.
**                  Otherwise, The HID device initiates a connection toward the
**                  specified bd_addr when BTA_HdOpen() is called.
**
** Returns          void
**
*******************************************************************************/
void BTA_HdEnable(BD_ADDR bd_addr, tBTA_SEC sec_mask, const char *p_service_name,
                  tBTA_HD_CBACK *p_cback, UINT8 app_id)
{
    tBTA_HD_API_ENABLE  *p_buf;


    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_HD, &bta_hd_reg);
    GKI_sched_unlock();

    if ((p_buf = (tBTA_HD_API_ENABLE *) GKI_getbuf(sizeof(tBTA_HD_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event = BTA_HD_API_ENABLE_EVT;
        p_buf->app_id    = app_id;
        if(p_service_name)
        {
            BCM_STRNCPY_S(p_buf->service_name, sizeof(p_buf->service_name), p_service_name, BTA_SERVICE_NAME_LEN);
            p_buf->service_name[BTA_SERVICE_NAME_LEN] = 0;
        }
        else
        {
            p_buf->service_name[0] = 0;
        }
        p_buf->p_cback = p_cback;
        bdcpy(p_buf->bd_addr, bd_addr);
        p_buf->sec_mask = sec_mask;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_HdDisable
**
** Description      Disable the HID Device service.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_HdDisable(void)
{
    BT_HDR  *p_buf;

    bta_sys_deregister(BTA_ID_HD);
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_HD_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_HdOpen
**
** Description      Opens an HID Device connection to a peer device.
**                  When connection is open, callback function is called
**                  with a BTA_HD_OPEN_EVT.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_HdOpen(tBTA_SEC sec_mask)
{
    tBTA_HD_API_OPEN  *p_buf;

    if ((p_buf = (tBTA_HD_API_OPEN *) GKI_getbuf(sizeof(tBTA_HD_API_OPEN))) != NULL)
    {
        p_buf->hdr.event = BTA_HD_API_OPEN_EVT;
        p_buf->sec_mask = sec_mask;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_HdClose
**
** Description      Close the current connection a peer device.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_HdClose(void)
{
    BT_HDR  *p_buf;

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_HD_API_CLOSE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}


/*******************************************************************************
**
** Function         BTA_HdSendRegularKey
**
** Description      Send a key report to the connected host.
**                  If auto_release is TRUE, assume the keyboard report must be
**                  a key press. An associated key release report is also sent.
**
** Returns          void
**
*******************************************************************************/
void BTA_HdSendRegularKey (UINT8 modifier, UINT8 key_code, BOOLEAN auto_release)
{
    tBTA_HD_API_INPUT  *p_buf;

    if ((p_buf = (tBTA_HD_API_INPUT *) GKI_getbuf(sizeof(tBTA_HD_API_INPUT))) != NULL)
    {
        p_buf->hdr.event    = BTA_HD_API_INPUT_EVT;
        p_buf->rid          = BTA_HD_REPT_ID_KBD;
        p_buf->keyCode      = key_code;
        p_buf->buttons      = modifier;
        p_buf->release      = auto_release;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_HdSendSpecialKey
**
** Description      Send a special key report to the connected host.
**                  The report is sent as a keyboard report.
**                  If auto_release is TRUE, assume the keyboard report must be
**                  a key press. An associated key release report is also sent.
**                  If key_len is less than BTA_HD_KBD_REPT_SIZE, the key_seq
**                  is padded with 0 until BTA_HD_KBD_REPT_SIZE.
**
** Returns          void
**
*******************************************************************************/
void BTA_HdSendSpecialKey (UINT8 key_len, UINT8 * key_seq, BOOLEAN auto_release)
{
    tBTA_HD_API_INPUT_SPEC  *p_buf;

    if ((p_buf = (tBTA_HD_API_INPUT_SPEC *) GKI_getbuf(
                    (UINT16)(sizeof(tBTA_HD_API_INPUT_SPEC)+key_len))) != NULL)
    {
        p_buf->hdr.event    = BTA_HD_API_INPUT_EVT;
        p_buf->rid          = BTA_HD_REPT_ID_SPEC;
        p_buf->len          = key_len;
        p_buf->seq          = (UINT8 *)(p_buf+1);
        p_buf->release      = auto_release;
        memcpy(p_buf->seq, key_seq, key_len);
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_HdSendMouseReport
**
** Description      Send a mouse report to the connected host
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_HdSendMouseReport (UINT8 is_left, UINT8 is_right, UINT8 is_middle,
                            INT8 delta_x, INT8 delta_y, INT8 delta_wheel)
{
    tBTA_HD_API_INPUT  *p_buf;

    if ((p_buf = (tBTA_HD_API_INPUT *) GKI_getbuf(sizeof(tBTA_HD_API_INPUT))) != NULL)
    {
        p_buf->hdr.event    = BTA_HD_API_INPUT_EVT;
        p_buf->rid          = BTA_HD_REPT_ID_MOUSE;
        p_buf->keyCode      = (UINT8)delta_x;
        p_buf->keyCode2     = (UINT8)delta_y;
        p_buf->buttons      = (is_left ? 0x01 : 0x00) |
                              (is_right ? 0x02 : 0x00) |
                              (is_middle ? 0x04 : 0x00);
        p_buf->wheel        = (UINT8)delta_wheel;
        bta_sys_sendmsg(p_buf);
    }
}



