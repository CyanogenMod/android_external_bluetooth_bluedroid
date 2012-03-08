/*****************************************************************************
**
**  Name:           bta_ar_int.h
**
**  Description:    This is the private interface file for the BTA
**                  audio/video registration module.
**
**  Copyright (c) 2008-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_AR_INT_H
#define BTA_AR_INT_H

#include "bta_av_api.h"


#ifndef BTA_AR_DEBUG
#define BTA_AR_DEBUG    FALSE
#endif

#define BTA_AR_AV_MASK      0x01
#define BTA_AR_AVK_MASK     0x02

/* data associated with BTA_AR */
typedef struct
{
    tAVDT_CTRL_CBACK *p_av_conn_cback;       /* av connection callback function */
    tAVDT_CTRL_CBACK *p_avk_conn_cback;      /* avk connection callback function */
    UINT8           avdt_registered;
    UINT8           avct_registered;
	UINT32          sdp_tg_handle;
	UINT32          sdp_ct_handle;
    UINT16          ct_categories[2];
    UINT8           tg_registered;
    tBTA_AV_HNDL    hndl;       /* Handle associated with the stream that rejected the connection. */
} tBTA_AR_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_AR_CB bta_ar_cb;
#else
extern tBTA_AR_CB *bta_ar_cb_ptr;
#define bta_ar_cb (*bta_ar_cb_ptr)
#endif

#endif /* BTA_AR_INT_H */
