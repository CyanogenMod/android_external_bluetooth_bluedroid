/*****************************************************************************
**
**  Name:           bta_ops_int.h
**
**  Description:    This is the private file for the object transfer
**                  server (OPS).
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_OPS_INT_H
#define BTA_OPS_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_op_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/


/* OPS active obex operation (Valid in connected state) */
#define OPS_OP_NONE         0
#define OPS_OP_PULL_OBJ     1
#define OPS_OP_PUSH_OBJ     2

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_OPS_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_OPS),

    BTA_OPS_API_ACCESSRSP_EVT,      /* Response to an access request */
    BTA_OPS_API_CLOSE_EVT,          /* Close API */
    BTA_OPS_CI_OPEN_EVT,            /* Response to File Open request */
    BTA_OPS_CI_WRITE_EVT,           /* Response to Write request */
    BTA_OPS_CI_READ_EVT,            /* Response to Read request */
    BTA_OPS_OBX_CONN_EVT,           /* OBX Channel Connect Request */
    BTA_OPS_OBX_DISC_EVT,           /* OBX Channel Disconnect */
    BTA_OPS_OBX_ABORT_EVT,          /* OBX_operation aborted */
    BTA_OPS_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_OPS_OBX_PUT_EVT,            /* Write file data or delete */
    BTA_OPS_OBX_GET_EVT,            /* Read file data or folder listing */
    BTA_OPS_CLOSE_CMPL_EVT,         /* Finished closing channel */

    /* these events are handled outside the state machine */
    BTA_OPS_API_ENABLE_EVT
};

typedef UINT16 tBTA_OPS_INT_EVT;

typedef UINT8 tBTA_OPS_STATE;

/* data type for BTA_OPS_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char                name[BTA_SERVICE_NAME_LEN + 1];
    tBTA_OPS_CBACK      *p_cback;
    tBTA_OP_FMT_MASK    formats;
    UINT8               sec_mask;
    BOOLEAN             srm;
    UINT8               app_id;
} tBTA_OPS_API_ENABLE;

/* data type for BTA_OPS_API_ACCESSRSP_EVT */
typedef struct
{
    BT_HDR          hdr;
    char            *p_name;
    tBTA_OP_OPER    oper;
    tBTA_OP_ACCESS  flag;
} tBTA_OPS_API_ACCESSRSP;

/* data type for all obex events
    hdr.event contains the OPS event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_OPS_OBX_EVENT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_OPS_API_ENABLE     api_enable;
    tBTA_OPS_API_ACCESSRSP  api_access;
    tBTA_OPS_OBX_EVENT      obx_evt;
    tBTA_FS_CI_OPEN_EVT     open_evt;
    tBTA_FS_CI_READ_EVT     read_evt;
    tBTA_FS_CI_WRITE_EVT    write_evt;
} tBTA_OPS_DATA;

/* OBX Response Packet Structure - Holds current response packet info */
typedef struct
{
    BT_HDR      *p_pkt;         /* (Pull/Push) Holds the current OBX hdr for Push or Pull */
    UINT8       *p_start;       /* (Pull/Push) Start of the Body of the packet */
    UINT16       offset;        /* (Pull/Push) Contains the current offset into the Body (p_start) */
    UINT16       bytes_left;    /* (Pull/Push) Holds bytes available left in Obx packet */
    BOOLEAN      final_pkt;     /* (Push)      Holds the final bit of the Push packet */
} tBTA_OPS_OBX_PKT;

/* Power management state for OPS */
#define BTA_OPS_PM_BUSY     0
#define BTA_OPS_PM_IDLE     1

/* OPS control block */
typedef struct
{
    tBTA_OPS_CBACK  *p_cback;       /* pointer to application callback function */
    char            *p_name;        /* Holds name of current operation */
    char            *p_path;        /* Holds path of current operation */
    tBTA_OPS_OBX_PKT obx;           /* Holds the current OBX packet information */
    UINT32           sdp_handle;    /* SDP record handle */
    UINT32           file_length;   /* length of file being Push/Pull */
    int              fd;            /* File Descriptor of opened file */
    BD_ADDR          bd_addr;       /* Device currently connected to */
    tOBX_HANDLE      obx_handle;
    UINT16           peer_mtu;
    UINT16           psm;           /* PSM for Obex Over L2CAP */
    BOOLEAN          srm;           /* TRUE, to use SIngle Response Mode */
    UINT8            scn;           /* SCN of the OPP server */
    tBTA_OP_FMT_MASK formats;       /* supported object formats */
    tBTA_OPS_STATE   state;         /* state machine state */
    tBTA_OP_FMT      obj_fmt;       /* file format of received object */
    UINT8            obx_oper;      /* current active OBX operation PUSH OBJ, or PULL OBJ */
    UINT8            app_id;
    tBTA_OP_OPER     acc_active;    /* op code when waiting for an access rsp (API) (0 not active) */
    BOOLEAN          cout_active;   /* TRUE when waiting for a call-in function */
    BOOLEAN          aborting;      /* TRUE when waiting for a call-in function */
    UINT8            pm_state;
} tBTA_OPS_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* OPS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_OPS_CB  bta_ops_cb;
#else
extern tBTA_OPS_CB *bta_ops_cb_ptr;
#define bta_ops_cb (*bta_ops_cb_ptr)
#endif


/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_ops_hdl_event(BT_HDR *p_msg);
extern void     bta_ops_sm_execute(tBTA_OPS_CB *p_cb, UINT16 event, tBTA_OPS_DATA *p_data);
extern void     bta_ops_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   tOBX_EVT_PARAM param, BT_HDR *p_pkt);

/* action functions */
extern void bta_ops_api_disable(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_api_accessrsp(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_api_close(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_ci_write(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_ci_read(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_ci_open(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_connect(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_disc(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_close(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_abort(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_put(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_obx_get(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_gasp_err_rsp(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_close_complete(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);

/* object store */
extern void bta_ops_init_get_obj(tBTA_OPS_CB *p_cb, tBTA_OPS_DATA *p_data);
extern void bta_ops_proc_get_obj(tBTA_OPS_CB *p_cb);
extern void bta_ops_proc_put_obj(BT_HDR *p_pkt);

/* miscellaneous functions */
extern void bta_ops_enable(tBTA_OPS_CB *p_cb, tBTA_OPS_API_ENABLE *p_data);
extern void bta_ops_get_obj_rsp(UINT8 rsp_code, UINT16 num_read);
extern void bta_ops_put_obj_rsp(UINT8 rsp_code);
extern void bta_ops_clean_getput(tBTA_OPS_CB *p_cb, BOOLEAN is_aborted);
extern void bta_ops_discard_data(UINT16 event, tBTA_OPS_DATA *p_data);
extern void bta_ops_req_app_access (tBTA_OP_OPER oper, tBTA_OPS_CB *p_cb);
extern tBTA_OP_FMT bta_ops_fmt_supported(char *p, tBTA_OP_FMT_MASK fmt_mask);

#endif /* BTA_OPS_INT_H */
