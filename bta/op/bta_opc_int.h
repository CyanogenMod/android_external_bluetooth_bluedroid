/*****************************************************************************
**
**  Name:           bta_opc_int.h
**
**  Description:    This is the private header file for the Object Push
**                  Client (OPC).
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_OPC_INT_H
#define BTA_OPC_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_op_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/


#define OPC_DEF_NAME        "default_opc.vcf"

/* if the pulled vCard does not have a Name Header associated with it,
 * this is the default name used to received the file.
 * Please make sure that this is of different file name than the local
 * default vCard file name */
#ifndef OPC_DEF_RCV_NAME
#define OPC_DEF_RCV_NAME    "opc_def_rcv.vcf"
#endif

/* OPC Active opp obex operation (Valid in connected state) */
#define OPC_OP_NONE         0
#define OPC_OP_PULL_OBJ     1
#define OPC_OP_PUSH_OBJ     2

/* Constants used for "to_do" list */
#define BTA_OPC_PUSH_MASK 0x01
#define BTA_OPC_PULL_MASK 0x02

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_OPC_API_ENABLE_EVT = BTA_SYS_EVT_START(BTA_ID_OPC),
    BTA_OPC_API_DISABLE_EVT,
    BTA_OPC_API_CLOSE_EVT,          /* Close connection event */
    BTA_OPC_API_PUSH_EVT,           /* Push Object request */
    BTA_OPC_API_PULL_EVT,           /* Pull Object request */
    BTA_OPC_API_EXCH_EVT,           /* Exchange business Cards */
    BTA_OPC_SDP_OK_EVT,             /* Service search was successful */
    BTA_OPC_SDP_FAIL_EVT,           /* Service search failed */
    BTA_OPC_CI_WRITE_EVT,           /* Call-in response to Write request */
    BTA_OPC_CI_READ_EVT,            /* Call-in response to Read request */
    BTA_OPC_CI_OPEN_EVT,            /* Call-in response to File Open request */
    BTA_OPC_OBX_CONN_RSP_EVT,       /* OBX Channel Connect Request */
    BTA_OPC_OBX_PASSWORD_EVT,       /* OBX password requested */
    BTA_OPC_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_OPC_OBX_PUT_RSP_EVT,        /* Write file data or delete */
    BTA_OPC_OBX_GET_RSP_EVT,        /* Read file data or folder listing */
    BTA_OPC_OBX_CMPL_EVT,           /* operation has completed */
    BTA_OPC_CLOSE_CMPL_EVT,         /* Finish the closing of the channel */
    BTA_OPC_DISABLE_CMPL_EVT        /* Transition to disabled state */
};

typedef UINT16 tBTA_OPC_INT_EVT;

typedef UINT8 tBTA_OPC_STATE;

/* data type for BTA_OPC_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_OPC_CBACK     *p_cback;
    UINT8               sec_mask;
    BOOLEAN             single_op;
    BOOLEAN             srm;
    UINT8               app_id;
} tBTA_OPC_API_ENABLE;

/* data type for BTA_OPC_API_PUSH_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    char                *p_name;
    tBTA_OP_FMT         format;
} tBTA_OPC_API_PUSH;

/* data type for BTA_OPC_API_PULL_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    char                *p_path;
} tBTA_OPC_API_PULL;

/* data type for BTA_OPC_API_EXCH_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    char                *p_send;
    char                *p_rcv_path;
} tBTA_OPC_API_EXCH;

/* data type for BTA_OPC_SDP_OK_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   scn;
    UINT16  psm;
    UINT16  version;
} tBTA_OPC_SDP_OK_EVT;

/* data type for all obex events
    hdr.event contains the OPC event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_OPC_OBX_EVT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_OPC_API_ENABLE     api_enable;
    tBTA_OPC_API_PUSH       api_push;
    tBTA_OPC_API_PULL       api_pull;
    tBTA_OPC_API_EXCH       api_exch;
    tBTA_OPC_SDP_OK_EVT     sdp_ok;
    tBTA_OPC_OBX_EVT        obx_evt;
    tBTA_FS_CI_OPEN_EVT     open_evt;
    tBTA_FS_CI_READ_EVT     read_evt;
    tBTA_FS_CI_WRITE_EVT    write_evt;
} tBTA_OPC_DATA;


/* OBX Response Packet Structure - Holds current command/response packet info */
typedef struct
{
    BT_HDR  *p_pkt;             /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8   *p_start;           /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available leop in Obx packet */
    BOOLEAN  final_pkt;         /* (Get)     Holds the final bit of the Put packet */
    UINT8    rsp_code;
} tBTA_OPC_OBX_PKT;

/* Power management state for OPC */
#define BTA_OPC_PM_BUSY     0
#define BTA_OPC_PM_IDLE     1

/* OPC control block */
typedef struct
{
    tBTA_OPC_CBACK    *p_cback;       /* pointer to application callback function */
    char              *p_name;        /* Holds the local path and file name of pushed item */
    char              *p_rcv_path;    /* Holds the local path and file name of received item (card exch only) */
    tSDP_DISCOVERY_DB *p_db;          /* pointer to discovery database */
    tBTA_OPC_OBX_PKT   obx;           /* Holds the current OBX packet information */
    int                fd;            /* File Descriptor of opened file */
    UINT32             obj_size;      /* (Push/Pull) file length */
    tOBX_HANDLE        obx_handle;
    UINT16             peer_mtu;
    BD_ADDR            bd_addr;
    BOOLEAN            single_op;     /* if TRUE, close OBX connection when OP finishes */
    UINT8              sec_mask;
    tBTA_OPC_STATE     state;         /* state machine state */
    tBTA_OP_FMT        format;
    UINT8              obx_oper;      /* current active OBX operation PUT FILE or GET FILE */
    UINT8              to_do;         /* actions to be done (push,pull) */
    UINT8              app_id;
    tBTA_OPC_STATUS    status;
    tBTA_OPC_STATUS    exch_status;
    BOOLEAN            first_get_pkt; /* TRUE if retrieving the first packet of GET object */
    BOOLEAN            cout_active;   /* TRUE if call-out is currently active */
    BOOLEAN            req_pending;   /* TRUE if an obx request to peer is in progress */
    BOOLEAN            disabling;     /* TRUE if an disabling client */
    BOOLEAN            sdp_pending;   /* TRUE when waiting for SDP to complete */
    BOOLEAN          srm;             /* TRUE, to use SIngle Response Mode */
    UINT8              pm_state;
} tBTA_OPC_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* OPC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_OPC_CB  bta_opc_cb;
#else
extern tBTA_OPC_CB *bta_opc_cb_ptr;
#define bta_opc_cb (*bta_opc_cb_ptr)
#endif


/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_opc_hdl_event(BT_HDR *p_msg);
extern void     bta_opc_sm_execute(tBTA_OPC_CB *p_cb, UINT16 event,
                                   tBTA_OPC_DATA *p_data);
extern void     bta_opc_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   UINT8 rsp_code, tOBX_EVT_PARAM param,
                                   BT_HDR *p_pkt);

/* action functions */
extern void bta_opc_init_close(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_init_push(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_init_pull(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_init_exch(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_send_authrsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_ci_write(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_ci_read(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_ci_open(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_obx_conn_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_obx_put_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_obx_get_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_initialize(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_trans_cmpl(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_stop_client(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_start_client(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_free_db(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_ignore_obx(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_find_service(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_close(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_close_complete(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_set_disable(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_chk_close(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_do_pull(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_do_push(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void bta_opc_enable(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);

/* miscellaneous functions */
extern UINT8    bta_opc_send_get_req(tBTA_OPC_CB *p_cb);
extern BOOLEAN  bta_opc_proc_get_rsp(tBTA_OPC_CB *p_cb, tBTA_OPC_DATA *p_data);
extern void     bta_opc_cont_get_rsp(tBTA_OPC_CB *p_cb);
extern UINT8    bta_opc_send_put_req(tBTA_OPC_CB *p_cb, BOOLEAN first_pkt);
extern void     bta_opc_start_push(tBTA_OPC_CB *p_cb);
extern void     bta_opc_listing_err(BT_HDR **p_pkt, tBTA_OPC_STATUS status);

extern tBTA_OPC_STATUS bta_opc_convert_obx_to_opc_status(tOBX_STATUS obx_status);

#endif /* BTA_OPC_INT_H */
