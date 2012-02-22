/******************************************************************************
**
**  File Name:   bta_hd_int.h
**
**  Description: This is the internal header file for the HID Device service.
**
**  Copyright (c) 2002-2009, Broadcom Corp., All Rights Reserved.              
**  WIDCOMM Bluetooth Core. Proprietary and confidential.                    
**
******************************************************************************/

#ifndef BTA_HD_INT_H
#define BTA_HD_INT_H

#include "bt_types.h"
#include "gki.h"
#include "bta_sys.h"
#include "bta_hd_api.h"


/*****************************************************************************/
/*                            C O N S T A N T S                              */
/*****************************************************************************/

#define BTA_HD_KEYBOARD_REPORT_SIZE     0x3F
#define BTA_HD_MOUSE_REPORT_SIZE        0x32

/* BTA_HD State machine events */
enum 
{
    /* these events are handled by the state machine */
    BTA_HD_API_OPEN_EVT = BTA_SYS_EVT_START(BTA_ID_HD),
    BTA_HD_API_CLOSE_EVT,
    BTA_HD_API_DISABLE_EVT,
    BTA_HD_API_INPUT_EVT,
    BTA_HD_CBACK_EVT,
    BTA_HD_CONNECTED_EVT,
    BTA_HD_DISCONNECTED_EVT,

    /* these events are handled outside of the state machine */
    BTA_HD_API_ENABLE_EVT
};
#define BTA_HD_INVALID_EVT      (BTA_HD_API_ENABLE_EVT + 1)

/*****************************************************************************
**  Data types
*****************************************************************************/

/* data type for BTA_HD_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char                service_name[BTA_SERVICE_NAME_LEN+1];
    tBTA_HD_CBACK       *p_cback;
    BD_ADDR             bd_addr;
    tBTA_SEC            sec_mask;
    UINT8               app_id;
} tBTA_HD_API_ENABLE;

/* data type for BTA_HD_API_OPEN_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_SEC            sec_mask;
} tBTA_HD_API_OPEN;


/* data type for BTA_HD_API_INPUT_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_HD_REPT_ID     rid;
    UINT8               keyCode;    /* if BTA_HD_REPT_ID_MOUSE, X */
    UINT8               keyCode2;   /* if BTA_HD_REPT_ID_MOUSE, Y */
    UINT8               wheel;      /* BTA_HD_REPT_ID_MOUSE only */
    UINT8               buttons;    /* BTA_HD_REPT_ID_MOUSE & BTA_HD_REPT_ID_KBD (modifier) only */
    BOOLEAN             release;    /* If TRUE, HD sends release key. not used by mouse report */
} tBTA_HD_API_INPUT;

typedef struct
{
    BT_HDR              hdr;
    tBTA_HD_REPT_ID     rid;
    UINT8               len;
    UINT8               *seq;
    BOOLEAN             release;    /* If TRUE, HD sends release key. */
} tBTA_HD_API_INPUT_SPEC;

/* data type for BTA_HD_CBACK_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT8               event;
    UINT32              data;
    tHID_DEV_CBACK_DATA *pdata;
} tBTA_HD_CBACK_DATA;

/* union of all event datatypes */
typedef union
{
    tBTA_HD_API_ENABLE      api_enable;
    tBTA_HD_API_OPEN        api_open;
    tBTA_HD_API_INPUT       api_input;
    tBTA_HD_API_INPUT_SPEC  api_spec;
    tBTA_HD_CBACK_DATA      cback_data;
} tBTA_HD_DATA;


/* type for HD control block */
typedef struct
{
    BD_ADDR             peer_addr;      /* peer BD address */
    UINT32              sdp_handle;     /* SDP record handle */
    tBTA_HD_CBACK       *p_cback;       /* application callback function */
    BUFFER_Q            out_q;          /* Queue for reports */
    tBTA_SEC            sec_mask;       /* security mask */
    UINT8               state;          /* state machine state */
    UINT8               proto;          /* protocol (boot or report) */
    UINT8               app_id;
#if (BTM_SSR_INCLUDED == TRUE)
    BOOLEAN             use_ssr;        /* TRUE, if SSR is supported on this link */
    TIMER_LIST_ENT      timer;          /* delay timer to check for SSR */
#endif
} tBTA_HD_CB;

/*****************************************************************************/
/*                            P U B L I C  D A T A                           */
/*****************************************************************************/

/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_HD_CB bta_hd_cb;
#else
extern tBTA_HD_CB *bta_hd_cb_ptr;
#define bta_hd_cb (*bta_hd_cb_ptr)
#endif

/* config struct */
extern tBTA_HD_CFG *p_bta_hd_cfg;

/* rc id config struct */
extern UINT16 *p_bta_hd_rc_id;

/*****************************************************************************/
/*                   F U N C T I O N     P R O T O T Y P E S                 */
/*****************************************************************************/

/* from bta_hd_main.c */
extern BOOLEAN bta_hd_hdl_event(BT_HDR *p_msg);
extern void bta_hd_sm_execute(tBTA_HD_CB *p_cb, UINT16 event, tBTA_HD_DATA *p_data);
/* from bta_hd_act.c */
extern void bta_hd_init_con_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_close_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_disable_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_open_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_opn_cb_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_input_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_discntd_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_discnt_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data);
extern void bta_hd_hidd_cback(UINT8 event, UINT32 data, tHID_DEV_CBACK_DATA *pdata);
#endif  /* BTA_HD_INT_H */

