/*****************************************************************************
**
**  Name:           bta_dg_int.h
**
**  Description:    This is the private interface file for the BTA data
**                  gateway.
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_DG_INT_H
#define BTA_DG_INT_H

#include "bta_sys.h"

/*****************************************************************************
**  Constants
*****************************************************************************/


/* DG events */
enum
{
    /* these events are handled by the state machine */
    BTA_DG_API_CLOSE_EVT = BTA_SYS_EVT_START(BTA_ID_DG),
    BTA_DG_API_SHUTDOWN_EVT,
    BTA_DG_API_LISTEN_EVT,
    BTA_DG_API_OPEN_EVT,
    BTA_DG_CI_TX_READY_EVT,
    BTA_DG_CI_RX_READY_EVT,
    BTA_DG_CI_TX_FLOW_EVT,
    BTA_DG_CI_RX_WRITEBUF_EVT,
    BTA_DG_CI_CONTROL_EVT,
    BTA_DG_RFC_OPEN_EVT,
    BTA_DG_RFC_CLOSE_EVT,
    BTA_DG_RFC_TX_READY_EVT,
    BTA_DG_RFC_RX_READY_EVT,
    BTA_DG_RFC_FC_EVT,
    BTA_DG_RFC_CONTROL_EVT,
    BTA_DG_DISC_RESULT_EVT,
    BTA_DG_DISC_OK_EVT,
    BTA_DG_DISC_FAIL_EVT,

    /* these events are handled outside of the state machine */
    BTA_DG_API_DISABLE_EVT,
    BTA_DG_API_ENABLE_EVT,
    BTA_DG_RFC_PORT_EVT
};

/*****************************************************************************
**  Data types
*****************************************************************************/

/* data type for BTA_DG_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;                /* Event header */
    tBTA_DG_CBACK       *p_cback;           /* DG callback function */
} tBTA_DG_API_ENABLE;

/* data type for BTA_DG_API_LISTEN_EVT */
typedef struct
{
    BT_HDR          hdr;                            /* Event header */
    char            name[BTA_SERVICE_NAME_LEN+1];   /* Service name */
    tBTA_SEC        sec_mask;                       /* Security mask */
    tBTA_SERVICE_ID service;                        /* Service ID */
    UINT8           app_id;                         /* Application ID */
} tBTA_DG_API_LISTEN;

/* data type for BTA_DG_API_OPEN_EVT */
typedef struct
{
    BT_HDR          hdr;                            /* Event header */
    char            name[BTA_SERVICE_NAME_LEN+1];   /* Service name */
    BD_ADDR         bd_addr;                        /* Peer address */
    tBTA_SEC        sec_mask;                       /* Security mask */
    tBTA_SERVICE_ID service;                        /* Service ID */
    UINT8           app_id;                         /* Application ID */
} tBTA_DG_API_OPEN;

/* data type for BTA_DG_CI_TX_FLOW_EVT */
typedef struct
{
    BT_HDR          hdr;                    /* Event header */
    BOOLEAN         enable;                 /* Flow control setting */
} tBTA_DG_CI_TX_FLOW;

/* data type for BTA_DG_CI_CONTROL_EVT */
typedef struct
{
    BT_HDR          hdr;                    /* Event header */
    UINT8           signals;                /* RS-232 signals */
    UINT8           values;                 /* RS-232 signal values */
} tBTA_DG_CI_CONTROL;

/* data type for BTA_DG_RFC_FC_EVT */
typedef struct
{
    BT_HDR          hdr;                    /* Event header */
    BOOLEAN         enable;                 /* TRUE if data flow enabled */
} tBTA_DG_RFC_FC;

/* data type for BTA_DG_RFC_PORT_EVT */
typedef struct
{
    BT_HDR          hdr;                    /* Event header */
    UINT32          code;                   /* RFCOMM port callback event code mask */
} tBTA_DG_RFC_PORT;

/* data type for BTA_DG_DISC_RESULT_EVT */
typedef struct
{
    BT_HDR          hdr;                    /* Event header */
    UINT16          status;                 /* SDP status */
} tBTA_DG_DISC_RESULT;

/* union of all data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_DG_API_ENABLE      api_enable;
    tBTA_DG_API_LISTEN      api_listen;
    tBTA_DG_API_OPEN        api_open;
    tBTA_DG_CI_TX_FLOW      ci_tx_flow;
    tBTA_DG_CI_CONTROL      ci_control;
    tBTA_DG_RFC_FC          rfc_fc;
    tBTA_DG_RFC_PORT        rfc_port;
    tBTA_DG_DISC_RESULT     disc_result;
} tBTA_DG_DATA;

/* type used to store temporary sdp data */
typedef struct
{
    char                name[BTA_SERVICE_NAME_LEN+1];   /* Service name */
    tSDP_DISCOVERY_DB   db;                             /* Discovery database */
} tBTA_DG_SDP_DB;

/* state machine control block */
typedef struct
{
    char                    name[BTA_SERVICE_NAME_LEN+1];   /* Service name */
    BD_ADDR                 peer_addr;      /* BD address of peer device */
    tBTA_DG_SDP_DB          *p_disc;        /* pointer to sdp discovery db */
    UINT32                  sdp_handle;     /* SDP record handle */
    UINT16                  port_handle;    /* RFCOMM port handle */
    UINT16                  mtu;            /* RFCOMM MTU */
    tBTA_SERVICE_ID         service_id;     /* Profile service ID */
    tBTA_SEC                sec_mask;       /* Security mask */
    BOOLEAN                 rfc_enable;     /* RFCOMM flow control state */
    BOOLEAN                 app_enable;     /* Application flow control state */
    BOOLEAN                 in_use;         /* TRUE if SCB in use */
    BOOLEAN                 dealloc;        /* TRUE if service shutting down */
    BOOLEAN                 is_server;      /* TRUE if scb is for server */
    UINT8                   state;          /* State machine state */
    UINT8                   app_id;         /* Application ID */
    UINT8                   scn;            /* Server channel number */
    UINT8                   flow_mask;      /* Data flow mask */
} tBTA_DG_SCB;

/* main control block */
typedef struct
{
    tBTA_DG_SCB     scb[BTA_DG_NUM_CONN];       /* state machine control blocks */
    UINT8           hdl_to_scb[MAX_RFC_PORTS+1];/* port to state machine lookup table */
    tBTA_DG_CBACK   *p_cback;                   /* DG callback function */
} tBTA_DG_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* DG control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_DG_CB  bta_dg_cb;
#else
extern tBTA_DG_CB *bta_dg_cb_ptr;
#define bta_dg_cb (*bta_dg_cb_ptr)
#endif

/* DG configuration constants */
extern tBTA_DG_CFG * p_bta_dg_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern void bta_dg_scb_dealloc(tBTA_DG_SCB *p_scb);
extern UINT8 bta_dg_scb_to_idx(tBTA_DG_SCB *p_scb);
extern tBTA_DG_SCB *bta_dg_scb_by_idx(UINT16 idx);
extern tBTA_DG_SCB *bta_dg_scb_by_handle(UINT16 handle);
extern tBTA_DG_SCB *bta_dg_server_match(tBTA_DG_SCB *p_srv);
extern BOOLEAN bta_dg_hdl_event(BT_HDR *p_msg);
extern void bta_dg_sm_execute(tBTA_DG_SCB *p_scb, UINT16 event, tBTA_DG_DATA *p_data);

/* action functions */
extern void bta_dg_setup(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_listen(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_del_record(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_shutdown(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_close(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_set_dealloc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_rx_path(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_tx_path(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_fc_state(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_writebuf(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_control(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_rfc_control(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_rfc_open(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_rfc_close(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_dealloc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_disc_result(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_do_disc(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_open(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_free_db(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);
extern void bta_dg_setup_server(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);


#endif /* BTA_DG_INT_H */
