/*****************************************************************************
**
**  Name:           bta_ftc_int.h
**
**  Description:    This is the private file for the file transfer
**                  client (FTC).
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_FTC_INT_H
#define BTA_FTC_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_ft_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
#include "bta_bi_api.h"
#endif

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_FTC_PB_ACCESS_TARGET_UUID       "\x79\x61\x35\xF0\xF0\xC5\x11\xD8\x09\x66\x08\x00\x20\x0C\x9A\x66"
#define BTA_FTC_FOLDER_BROWSING_TARGET_UUID "\xF9\xEC\x7B\xC4\x95\x3C\x11\xD2\x98\x4E\x52\x54\x00\xDC\x9E\x09"
#define BTA_FTC_UUID_LENGTH                 16
#define BTA_FTC_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */
#define BTA_FTC_DEFAULT_VERSION             0x0100  /* for PBAP PCE */

#define BTA_FTC_FOLDER_LISTING_TYPE         "x-obex/folder-listing"

#define BTA_FTC_PULL_PB_TYPE                "x-bt/phonebook"
#define BTA_FTC_PULL_VCARD_LISTING_TYPE     "x-bt/vcard-listing"
#define BTA_FTC_PULL_VCARD_ENTRY_TYPE       "x-bt/vcard"

/* FTC Active ftp obex operation (Valid in connected state) */
#define FTC_OP_NONE         0
#define FTC_OP_GET_FILE     1
#define FTC_OP_PUT_FILE     2
#define FTC_OP_DELETE       3   /* Folder or File */
#define FTC_OP_GET_LIST     4
#define FTC_OP_MKDIR        5
#define FTC_OP_CHDIR        6
/* note: the following 3 OPs need to be continuous and the order can not change */
#define FTC_OP_COPY         7    /* Copy object */
#define FTC_OP_MOVE         8    /* Move/rename object */
#define FTC_OP_PERMISSION   9    /* Set object permission */
#define FTC_OP_RESUME       0x80 /* to mark the operation to resume */

enum
{
    BTA_FTC_GET_FILE,   /* get file */
    BTA_FTC_GET_CARD,   /* PBAP PullvCardEntry */
    BTA_FTC_GET_PB      /* PBAP PullPhoneBook */
};
typedef UINT8 tBTA_FTC_TYPE;

/* Response Timer Operations */
#define FTC_TIMER_OP_STOP   0
#define FTC_TIMER_OP_ABORT  1
#define FTC_TIMER_OP_SUSPEND    2

/* File abort mask states */
/* Abort must receive cout and response before abort completed */
#define FTC_ABORT_REQ_NOT_SENT  0x1
#define FTC_ABORT_REQ_SENT      0x2
#define FTC_ABORT_RSP_RCVD      0x4
#define FTC_ABORT_COUT_DONE     0x8

#define FTC_ABORT_COMPLETED     (FTC_ABORT_REQ_SENT | FTC_ABORT_RSP_RCVD | FTC_ABORT_COUT_DONE)

/* TODO remove: Reliable session suspend mask states */
/* suspend must receive cout and response before suspend completed
#define FTC_SUSPEND_REQ_NOT_SENT  0x10 */

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_FTC_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_FTC),

    BTA_FTC_API_OPEN_EVT,           /* Open a connection request */
    BTA_FTC_API_CLOSE_EVT,          /* Close an open connection request */
    BTA_FTC_API_PUTFILE_EVT,        /* Put File request */
    BTA_FTC_API_GETFILE_EVT,        /* Get File request */
    BTA_FTC_API_LISTDIR_EVT,          /* List Directory request */
    BTA_FTC_API_CHDIR_EVT,          /* Change Directory request */
    BTA_FTC_API_MKDIR_EVT,          /* make Directory request */
    BTA_FTC_API_REMOVE_EVT,          /* Remove Directory request */
    BTA_FTC_API_AUTHRSP_EVT,        /* Response to password request */
    BTA_FTC_API_ABORT_EVT,          /* Abort request */
    BTA_FTC_API_ACTION_EVT,         /* Action request */
    BTA_FTC_OBX_ACTION_RSP_EVT,     /* Copy/Move File or Set Permission */
    BTA_FTC_CI_SESSION_EVT,         /* Call-in response to session requests */
    BTA_FTC_SDP_OK_EVT,             /* Service search was successful */
    BTA_FTC_SDP_FAIL_EVT,           /* Service search failed */
    BTA_FTC_SDP_NEXT_EVT,           /* Try another service search (OPP) */
    BTA_FTC_CI_WRITE_EVT,           /* Call-in response to Write request */
    BTA_FTC_CI_READ_EVT,            /* Call-in response to Read request */
    BTA_FTC_CI_OPEN_EVT,            /* Call-in response to File Open request */
    BTA_FTC_OBX_CONN_RSP_EVT,       /* OBX Channel Connect Request */
    BTA_FTC_OBX_ABORT_RSP_EVT,      /* OBX_operation aborted */
    BTA_FTC_OBX_TOUT_EVT,           /* OBX Operation Timeout */
    BTA_FTC_OBX_PASSWORD_EVT,       /* OBX password requested */
    BTA_FTC_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_FTC_OBX_PUT_RSP_EVT,        /* Write file data or delete */
    BTA_FTC_OBX_GET_RSP_EVT,        /* Read file data or folder listing */
    BTA_FTC_OBX_SETPATH_RSP_EVT,    /* Make or Change Directory */
    BTA_FTC_OBX_CMPL_EVT,           /* operation has completed */
    BTA_FTC_CLOSE_CMPL_EVT,         /* Finish the closing of the channel */
    BTA_FTC_DISABLE_CMPL_EVT,       /* Finished disabling system */
    BTA_FTC_RSP_TOUT_EVT,           /* Timeout waiting for response from server */

    /* these events are handled outside the state machine */
    BTA_FTC_API_ENABLE_EVT
};


/* state machine states */
enum
{
  BTA_FTC_IDLE_ST = 0,      /* Idle  */
  BTA_FTC_W4_CONN_ST,       /* Waiting for an Obex connect response */
  BTA_FTC_CONN_ST,          /* Connected - FTP Session is active */
  BTA_FTC_SUSPENDING_ST,    /* suspend is in progress */
  BTA_FTC_CLOSING_ST        /* Closing is in progress */
};
typedef UINT16 tBTA_FTC_INT_EVT;

typedef UINT8 tBTA_FTC_STATE;

/* Application Parameters Header
Tag IDs used in the Application Parameters header:
*/
                                            /*  Tag ID          Length      Possible Values */
#define BTA_FTC_APH_ORDER           0x01    /* Order            1 bytes     0x0 to 0x2 */
#define BTA_FTC_APH_SEARCH_VALUE    0x02    /* SearchValue      variable    text */
#define BTA_FTC_APH_SEARCH_ATTR     0x03    /* SearchAttribute  1 byte      0x0 to 0x2 */
#define BTA_FTC_APH_MAX_LIST_COUNT  0x04    /* MaxListCount     2 bytes     0x0000 to 0xFFFF */
#define BTA_FTC_APH_LIST_STOFF      0x05    /* ListStartOffset  2 bytes     0x0000 to 0xFFFF */
#define BTA_FTC_APH_FILTER          0x06    /* Filter           4 bytes     0x00000000 to 0xFFFFFFFF */
#define BTA_FTC_APH_FORMAT          0x07    /* Format           1 byte      0x00(2.1), 0x01(3.0) */
#define BTA_FTC_APH_PB_SIZE         0x08    /* PhoneBookSize    2 byte      0x0000 to 0xFFFF */
#define BTA_FTC_APH_NEW_MISSED_CALL 0x09    /* NewMissedCall    1 bytes     0x00 to 0xFF */
#define BTA_FTC_APH_MAX_TAG         BTA_FTC_APH_NEW_MISSED_CALL

/* data type for BTA_FTC_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_FTC_CBACK     *p_cback;
    UINT8               app_id;
} tBTA_FTC_API_ENABLE;

/* data type for BTA_FTC_API_OPEN_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_SERVICE_MASK   services;   /* FTP and/or OPP */
    BD_ADDR             bd_addr;
    UINT8               sec_mask;
    BOOLEAN             srm;
    UINT32              nonce;
} tBTA_FTC_API_OPEN;

/* data type for BTA_FTC_API_CLOSE_EVT */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             suspend;
} tBTA_FTC_API_CLOSE;

/* data type for BTA_FTC_API_ACTION_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_FTC_ACT        action;
    char               *p_src;      /* UTF-8 name from listing */
    char               *p_dest;     /* UTF-8 name  */
    UINT8               permission[3];
} tBTA_FTC_API_ACTION;

/* data type for BTA_FTC_API_PUTFILE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_name;
    tBTA_FTC_PARAM     *p_param;
} tBTA_FTC_API_PUT;

typedef struct
{
    tBTA_FTC_FILTER_MASK    filter;
    UINT16                  max_list_count;
    UINT16                  list_start_offset;
    tBTA_FTC_FORMAT         format;
} tBTA_FTC_GET_PARAM;

/* data type for BTA_FTC_API_GETFILE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_rem_name; /* UTF-8 name from listing */
    char               *p_name;
    tBTA_FTC_GET_PARAM *p_param;
    UINT8               obj_type;
} tBTA_FTC_API_GET;

/* data type for BTA_FTC_API_CHDIR_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_dir;    /* UTF-8 name from listing */
    tBTA_FTC_FLAG       flag;
} tBTA_FTC_API_CHDIR;

typedef struct
{
    char                *p_value;
    UINT16              max_list_count;
    UINT16              list_start_offset;
    tBTA_FTC_ORDER      order;
    tBTA_FTC_ATTR       attribute;
} tBTA_FTC_LST_PARAM;

/* data type for BTA_FTC_API_LISTDIR_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_dir;    /* UTF-8 name from listing */
    tBTA_FTC_LST_PARAM *p_param;
} tBTA_FTC_API_LIST;

/* data type for BTA_FTC_API_MKDIR_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_dir;      /* UTF-8 name directory */
} tBTA_FTC_API_MKDIR;

/* data type for BTA_FTC_API_REMOVE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_name;     /* UTF-8 name of file or directory */
} tBTA_FTC_API_REMOVE;


/* data type for BTA_FTC_API_AUTHRSP_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   key [BTA_FTC_MAX_AUTH_KEY_SIZE];      /* The authentication key.*/
    UINT8   key_len;
    UINT8   userid [OBX_MAX_REALM_LEN];      /* The authentication user id.*/
    UINT8   userid_len;
} tBTA_FTC_API_AUTHRSP;

/* data type for BTA_FTC_SDP_OK_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   scn;
    UINT16  psm;
    UINT16  version;
} tBTA_FTC_SDP_OK_EVT;

/* data type for all obex events
    hdr.event contains the FTC event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_FTC_OBX_EVT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_FTC_API_ENABLE     api_enable;
    tBTA_FTC_API_OPEN       api_open;
    tBTA_FTC_API_CLOSE      api_close;
    tBTA_FTC_API_PUT        api_put;
    tBTA_FTC_API_GET        api_get;
    tBTA_FTC_API_CHDIR      api_chdir;
    tBTA_FTC_API_MKDIR      api_mkdir;
    tBTA_FTC_API_REMOVE     api_remove;
    tBTA_FTC_API_AUTHRSP    auth_rsp;
    tBTA_FTC_API_LIST       api_list;
    tBTA_FTC_API_ACTION     api_action;
    tBTA_FTC_SDP_OK_EVT     sdp_ok;
    tBTA_FTC_OBX_EVT        obx_evt;
    tBTA_FS_CI_OPEN_EVT     open_evt;
    tBTA_FS_CI_RESUME_EVT   resume_evt;
    tBTA_FS_CI_READ_EVT     read_evt;
    tBTA_FS_CI_WRITE_EVT    write_evt;
} tBTA_FTC_DATA;


/* OBX Response Packet Structure - Holds current command/response packet info */
typedef struct
{
    BT_HDR  *p_pkt;             /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8   *p_start;           /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available left in Obx packet */
    BOOLEAN  final_pkt;         /* (Get)     Holds the final bit of the Put packet */
} tBTA_FTC_OBX_PKT;

/* Power management state for FTC */
#define BTA_FTC_PM_BUSY     0
#define BTA_FTC_PM_IDLE     1


/* FTC control block */
typedef struct
{
    tBTA_FTC_CBACK    *p_cback;       /* pointer to application callback function */
    char              *p_name;        /* Holds the local file name */
    tSDP_DISCOVERY_DB *p_db;          /* pointer to discovery database */
    UINT32             sdp_handle;    /* SDP record handle for PCE */
    tBTA_FTC_OBX_PKT   obx;           /* Holds the current OBX packet information */
    TIMER_LIST_ENT     rsp_timer;     /* response timer */
    tBTA_SERVICE_MASK  services;      /* FTP and/or OPP and/or BIP */
    int                fd;            /* File Descriptor of opened file */
    UINT32             file_size;     /* (Put/Get) length of file */
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
    tBIP_IMAGING_CAPS  *p_caps;       /* BIP imaging capabilities */
    tBIP_IMG_HDL_STR   img_hdl;       /* The image’s handle for when responder requests thumbnail */
#endif
    UINT8              *p_sess_info;
    UINT16             peer_mtu;
    UINT16             sdp_service;
    BD_ADDR            bd_addr;
    tOBX_HANDLE        obx_handle;
    UINT8              sec_mask;
    tBTA_FTC_STATE     state;         /* state machine state */
    UINT8              obx_oper;      /* current active OBX operation PUT FILE or GET FILE */
    UINT8              timer_oper;    /* current active response timer action (abort or close) */
    UINT8              app_id;
    tBTA_FTC_TYPE      obj_type;      /* type of get op */
    BOOLEAN            first_req_pkt; /* TRUE if retrieving the first packet of GET/PUT file */
    BOOLEAN            cout_active;   /* TRUE if call-out is currently active */
    BOOLEAN            disabling;     /* TRUE if client is in process of disabling */
    UINT8              aborting;      /* Non-zero if client is in process of aborting */
    BOOLEAN            int_abort;     /* TRUE if internal abort issued (Not API inititated) */
    BOOLEAN            is_enabled;    /* TRUE if client is enabled */
    BOOLEAN            req_pending;   /* TRUE when waiting for an obex response */
    BOOLEAN            sdp_pending;   /* TRUE when waiting for SDP to complete */
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
    UINT8              bic_handle;
#endif
    UINT8              scn;
    UINT16             version;

    /* OBEX 1.5 */
    UINT16             psm;
    UINT32             nonce;
    BOOLEAN            srm;
    BOOLEAN            suspending;    /* TRUE when suspending session */

    tBTA_FTC_STATUS    status;
    UINT8              pm_state;
} tBTA_FTC_CB;

/* type for action functions */
typedef void (*tBTA_FTC_ACTION)(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);

/******************************************************************************
**  Configuration Definitions
*******************************************************************************/
typedef const tBTA_FTC_ACTION  (*tBTA_FTC_BI_TBL);

/* Configuration structure */
typedef struct
{
    tBTA_FTC_BI_TBL p_bi_action;        /* BI related action func used in FTC */
    UINT8       realm_charset;          /* Server only */
    BOOLEAN     userid_req;             /* TRUE if user id is required during obex authentication (Server only) */
    BOOLEAN     auto_file_list;         /* TRUE if automatic get the file listing from sever on OBEX connect response */
    UINT8       pce_features;           /* PBAP PCE supported features. If 0, PCE is not supported */
    char        *pce_name;              /* service name for PBAP PCE SDP record */
    INT32       abort_tout;             /* Timeout in milliseconds to wait for abort OBEX response (client only) */
    INT32       stop_tout;              /* Timeout in milliseconds to wait for close OBEX response (client only) */
    INT32       suspend_tout;           /* Timeout in milliseconds to wait for suspend OBEX response (client only) */
    UINT32      nonce;                  /* non-0 to allow reliable session (Server Only) */
    UINT8       max_suspend;            /* the maximum number of suspended session (Server Only) */
    BOOLEAN     over_l2cap;             /* TRUE to use Obex Over L2CAP (Server Only) */
    BOOLEAN     srm;                    /* TRUE to engage Single Response Mode (Server Only) */

} tBTA_FT_CFG;

enum
{
    BTA_FTC_BI_REGISTER_IDX = 0,
    BTA_FTC_BI_DEREGISTER_IDX,
    BTA_FTC_BI_AUTHRSP_IDX,
    BTA_FTC_BI_FREEXML_IDX,
    BTA_FTC_BI_PUT_IDX,
    BTA_FTC_BI_ABORT_IDX
};

extern const tBTA_FTC_ACTION bta_ftc_bi_action[]; 

/*****************************************************************************
**  Global data
*****************************************************************************/

/* FTC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_FTC_CB  bta_ftc_cb;
#else
extern tBTA_FTC_CB *bta_ftc_cb_ptr;
#define bta_ftc_cb (*bta_ftc_cb_ptr)
#endif

/* FT configuration constants */
extern tBTA_FT_CFG *p_bta_ft_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_ftc_hdl_event(BT_HDR *p_msg);
extern void     bta_ftc_sm_execute(tBTA_FTC_CB *p_cb, UINT16 event,
                                   tBTA_FTC_DATA *p_data);
extern void     bta_ftc_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   UINT8 rsp_code, tOBX_EVT_PARAM param,
                                   BT_HDR *p_pkt);

extern void bta_ftc_init_open(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_init_close(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_init_putfile(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_init_getfile(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_chdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_send_authrsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_abort(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_api_action(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_action_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_get_srm_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ci_write_srm(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ci_write(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ci_read(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ci_open(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ci_resume(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_conn_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_sess_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_suspended(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_abort_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_password(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_timeout(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_put_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_get_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_obx_setpath_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_initialize(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_trans_cmpl(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_stop_client(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_start_client(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_free_db(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_ignore_obx(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_find_service(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_close(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_open_fail(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_close_complete(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_set_disable(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_rsp_timeout(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_put(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_abort(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);

/* BI related action function used in FTC */
extern void bta_ftc_bic_register(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_deregister(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_authrsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_freexml(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_putact(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_bic_abortact(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);


extern void bta_ftc_listdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_remove(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern void bta_ftc_mkdir(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);

/* miscellaneous functions */
extern UINT8 bta_ftc_send_get_req(tBTA_FTC_CB *p_cb);
extern void  bta_ftc_proc_get_rsp(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data);
extern UINT8 bta_ftc_cont_put_file(tBTA_FTC_CB *p_cb, BOOLEAN first_pkt);
extern void  bta_ftc_proc_list_data(tBTA_FTC_CB *p_cb, tBTA_FTC_OBX_EVT *p_evt);
extern void  bta_ftc_get_listing(tBTA_FTC_CB *p_cb, char *p_name, tBTA_FTC_LST_PARAM *p_param);
extern void  bta_ftc_listing_err(BT_HDR **p_pkt, tBTA_FTC_STATUS status);


extern tBTA_FTC_STATUS bta_ftc_convert_obx_to_ftc_status(tOBX_STATUS obx_status);

#endif /* BTA_FTC_INT_H */
