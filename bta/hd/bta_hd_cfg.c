/*****************************************************************************
**
**  Name:           bta_hd_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for HID Device
**
**  Copyright (c) 2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "gki.h"
#include "bta_api.h"
#include "bd.h"
#include "bta_hd_api.h"

const UINT8 bta_hd_cfg_report[] =
{
    0x05, 0x01,         /* Usage page Desktop 01 */
    0x09, 0x06,         /* Usage Keyboard 06 */
    0xa1, 0x01,         /* Collection appliction */
        0x05, 0x07,        /* Usage page Keyboard */
        0x85, BTA_HD_REPT_ID_KBD,        /* Report ID 1 */
        0x19, 0xe0,        /* Usage minimum e0 (leftControl) */
        0x29, 0xe7,        /* Usage maximum e7 (right gui) */
        0x15, 0x00,        /* Logical mimumum 0 */
        0x25, 0x01,        /* Logical Maximum 1 */
        0x75, 0x01,        /* Report size 1 */
        0x95, 0x08,        /* Report count 8 */
        0x81, 0x02,        /* Input Variable Abs */
        0x95, 0x01,        /* Report count 1 */
        0x75, 0x08,        /* Report size 8 */
        0x81, 0x01,        /* Iput constant variable   */
        0x95, 0x05,        /* report count 5 */
        0x75, 0x01,        /* Report size 1 */
        0x05, 0x08,        /* LED page */
        0x19, 0x01,        /* Usage minimum 1 Num lock */
        0x29, 0x05,        /* Usage maximum 5 Kana  */
        0x91, 0x02,        /* Output Data, Variable, Absolute */
        0x95, 0x01,        /* Report Count 1 */
        0x75, 0x03,        /* Report Size 3 */
        0x91, 0x01,        /* Output constant, Absolute  */
        0x95, 0x06,        /* Report Count 6 */
        0x75, 0x08,        /* Report size 8 */
        0x15, 0x00,        /* Logical mimumum 0 */
        0x26, 0xa4, 0x00,  /* Logical Maximum 00a4 */
        0x05, 0x07,        /* Usage page Keyboard */
        0x19, 0x00,        /* Usage minimum 0 */
        0x29, 0xa4,        /* Usage maximum a4 */
        0x81, 0x00,        /* Input data array absolute */
    0xc0,
    
    0x05, 0x01,             /* Usage page Desktop 01 */
    0x09, 0x02,             /* Usage 2 Mouse */
    0xa1, 0x01,             /* Collection appliction */
        0x09, 0x01,         /* Usage 1 pointer */
        0xa1, 0x00,         /* Collection physical */
            0x85, BTA_HD_REPT_ID_MOUSE,     /* report id 2 */
            0x05, 0x09,     /* Usage page button */
            0x19, 0x01,     /* Usage minimum 1 */
            0x29, 0x03,     /* Usage maximum 3 */
            0x15, 0x00,     /* Logical mimumum 0 */
            0x25, 0x01,     /* Logical Maximum 1 */
            0x95, 0x03,     /* Report Count 3 */
            0x75, 0x01,     /* Report size 1 */
            0x81, 0x02,     /* Input Variable Abs */
            0x95, 0x01,     /* Report Count 1 */
            0x75, 0x05,     /* Report size 5 */
            0x81, 0x03,     /* Input const var Abs */

            0x05, 0x01,     /* Usage page Desktop 01 */
            0x09, 0x30,     /* Usage X */
            0x09, 0x31,     /* Usage Y */
            0x09, 0x38,     /* Usage Wheel */
            0x15, 0x81,     /* Logical mimumum -127 */
            0x25, 0x7f,     /* Logical Maximum 127 */
            0x75, 0x08,     /* Report size 8 */
            0x95, 0x03,     /* Report Count 3 */            
            0x81, 0x06,     /* Input Variable relative */ 
        0xc0,
    0xc0,
    
    0x05, 0x0c,            /* Usage page (consumer page) */
    0x09, 0x01,            /* Usage Consumer control  */
    0xa1, 0x01,            /* Collection appliction */
        0x85, BTA_HD_REPT_ID_CONSUMER,        /* Report ID 3 */
        0x75, 0x10,        /* report size 16 */
        0x95, 0x02,        /* report count 2 */
        0x15, 0x01,        /* Logical mimumum 1 */
        0x26, 0x8c, 0x02,  /* Logical Maximum 028c */
        0x19, 0x01,        /* Usage minimum 1 */
        0x2a, 0x8c, 0x02,  /* Usage maximum 028c */
        0x81, 0x60,        /* Input Data array absolute No preferred, Null state */
    0xc0,       
};


const tBTA_HD_CFG bta_hd_cfg =
{
    {
        {
            0,          /* qos_flags */
            1,          /* service type */
            800,        /* token rate (bytes/second) */
            8,          /* token_bucket_size (bytes) */
            0,          /* peak_bandwidth (bytes/second) */
            0xffffffff, /* latency(microseconds) */
            0xffffffff  /* delay_variation(microseconds) */
        },  /* ctrl_ch */
        {
            0,          /* qos_flags */
            1,          /* service type */
            300,        /* token rate (bytes/second) */
            4,          /* token_bucket_size (bytes) */
            300,        /* peak_bandwidth (bytes/second) */
            10000,      /* latency(microseconds) */
            10000       /* delay_variation(microseconds) */
        },  /* int_ch */
        {
            0,          /* qos_flags */
            2,          /* service type */
            400,        /* token rate (bytes/second) */
            8,          /* token_bucket_size (bytes) */
            800,        /* peak_bandwidth (bytes/second) */
            10000,      /* latency(microseconds) */
            10000       /* delay_variation(microseconds) */
        }   /* hci */
    },  /* qos */
    {
        "BTA HID Device",   /* Service name */
        "Remote Control",   /* Service Description */
        "Broadcom Corp",    /* Provider Name.*/
        0x0100,             /* HID device release number */
        0x0111,             /* HID Parser Version.*/
        HID_SSR_PARAM_INVALID,             /* SSR max latency */
        HID_SSR_PARAM_INVALID,             /* SSR min timeout */
        BTM_COD_MINOR_COMBO,/*Device Subclass.*/
        0,                  /* Country Code. (0 for not localized) */
        0,                  /* Supervisory Timeout */
        {
            sizeof(bta_hd_cfg_report), /* size of report descriptor */ 
            (UINT8 *)bta_hd_cfg_report,/* config report descriptor */
        },  /* dscp_info */
        0   /* p_sdp_layer_rec */
    },  /* sdp info */
    1                   /* use QoS */
};

tBTA_HD_CFG *p_bta_hd_cfg = (tBTA_HD_CFG *) &bta_hd_cfg;

