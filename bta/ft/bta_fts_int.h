/*****************************************************************************
**
**  Name:           bta_fts_int.h
**
**  Description:    This is the private file for the file transfer
**                  server (FTS).
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_FTS_INT_H
#define BTA_FTS_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_ft_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "bta_ftc_int.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/



#define BTA_FTS_FOLDER_BROWSING_TARGET_UUID "\xF9\xEC\x7B\xC4\x95\x3C\x11\xD2\x98\x4E\x52\x54\x00\xDC\x9E\x09"
#define BTA_FTS_UUID_LENGTH                 16
#define BTA_FTS_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */

#define BTA_FTS_DEFAULT_VERSION             0x0100
#define BTA_FTS_FOLDER_LISTING_TYPE         "x-obex/folder-listing"

typedef tOBX_STATUS (tBTA_FTS_OBX_RSP) (tOBX_HANDLE handle, UINT8 rsp_code, BT_HDR *p_pkt);

/* FTS Active ftp obex operation (Valid in connected state) */
#define FTS_OP_NONE         0
#define FTS_OP_LISTING      1
#define FTS_OP_GET_FILE     2
#define FTS_OP_PUT_FILE     3
#define FTS_OP_DELETE       4   /* Folder or File */
#define FTS_OP_CHDIR        5
#define FTS_OP_MKDIR        6
#define FTS_OP_RESUME       0x10

/* state machine states */
enum
{
  BTA_FTS_IDLE_ST = 0,      /* Idle  */
  BTA_FTS_LISTEN_ST,        /* Listen - waiting for OBX/RFC connection */
  BTA_FTS_W4_AUTH_ST,       /* Wait for Authentication -  (optional) */
  BTA_FTS_CONN_ST,          /* Connected - FTP Session is active */
  BTA_FTS_CLOSING_ST        /* Closing is in progress */
};

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_FTS_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_FTS),

    BTA_FTS_API_AUTHRSP_EVT,        /* Response to password request */
    BTA_FTS_API_ACCESSRSP_EVT,      /* Response to an access request */
    BTA_FTS_API_CLOSE_EVT,          /* Disconnect the OBEX channel  */
    BTA_FTS_CI_WRITE_EVT,           /* Response to Write request */
    BTA_FTS_CI_READ_EVT,            /* Response to Read request */
    BTA_FTS_CI_OPEN_EVT,            /* Response to File Open request */
    BTA_FTS_CI_DIRENTRY_EVT,        /* Response to a directory entry request */
    BTA_FTS_OBX_CONN_EVT,           /* OBX Channel Connect Request */
    BTA_FTS_OBX_DISC_EVT,           /* OBX Channel Disconnect */
    BTA_FTS_OBX_ABORT_EVT,          /* OBX_operation aborted */
    BTA_FTS_OBX_PASSWORD_EVT,       /* OBX password requested */
    BTA_FTS_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_FTS_OBX_PUT_EVT,            /* Write file data or delete */
    BTA_FTS_OBX_GET_EVT,            /* Read file data or folder listing */
    BTA_FTS_OBX_SETPATH_EVT,        /* Make or Change Directory */
    BTA_FTS_CI_SESSION_EVT,         /* Call-in response to session requests */
    BTA_FTS_OBX_ACTION_EVT,         /* Action command */
    BTA_FTS_APPL_TOUT_EVT,          /* Timeout waiting for application */
    BTA_FTS_DISC_ERR_EVT,           /* Sends OBX_DisconnectRsp with error code */
    BTA_FTS_GASP_ERR_EVT,           /* Sends Err Resp to Get, Abort, Setpath, and Put */
    BTA_FTS_CLOSE_CMPL_EVT,         /* Finished closing channel */
    BTA_FTS_DISABLE_CMPL_EVT,       /* Finished disabling server */

    /* these events are handled outside the state machine */
    BTA_FTS_API_ENABLE_EVT
};

typedef UINT16 tBTA_FTS_INT_EVT;

typedef UINT8 tBTA_FTS_STATE;

/* data type for BTA_FTS_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_FTS_CBACK     *p_cback;
    char                servicename[BTA_SERVICE_NAME_LEN + 1];
    char               *p_root_path;
    UINT8               realm [OBX_MAX_REALM_LEN];    /* The realm is intended to be
                                                         displayed to users so they know
                                                         which userid and password to use. 
                                                         The first byte of the string is
                                                         the character set of the string.
                                                       */
    UINT8               realm_len;
    UINT8               sec_mask;
    UINT8               app_id;
    BOOLEAN             auth_enabled;
} tBTA_FTS_API_ENABLE;

/* data type for BTA_FTS_API_AUTHRSP_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   key [BTA_FTS_MAX_AUTH_KEY_SIZE];      /* The authentication key.*/
    UINT8   key_len;
    UINT8   userid [OBX_MAX_REALM_LEN];      /* The authentication user id.*/
    UINT8   userid_len;
} tBTA_FTS_API_AUTHRSP;

/* data type for BTA_FTS_API_ACCESSRSP_EVT */
typedef struct
{
    BT_HDR          hdr;
    char           *p_name;
    tBTA_FT_OPER    oper;
    tBTA_FT_ACCESS  flag;
} tBTA_FTS_API_ACCESSRSP;

/* data type for all obex events
    hdr.event contains the FTS event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_FTS_OBX_EVENT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_FTS_API_ENABLE     api_enable;
    tBTA_FTS_API_AUTHRSP    auth_rsp;
    tBTA_FTS_API_ACCESSRSP  access_rsp;
    tBTA_FTS_OBX_EVENT      obx_evt;
    tBTA_FS_CI_GETDIR_EVT   getdir_evt;
    tBTA_FS_CI_OPEN_EVT     open_evt;
    tBTA_FS_CI_RESUME_EVT   resume_evt;
    tBTA_FS_CI_READ_EVT     read_evt;
    tBTA_FS_CI_WRITE_EVT    write_evt;
} tBTA_FTS_DATA;


/* OBX Response Packet Structure - Holds current response packet info */
typedef struct
{
    BT_HDR  *p_pkt;             /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8   *p_start;           /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available left in Obx packet */
    BOOLEAN  final_pkt;         /* (Put)     Holds the final bit of the Put packet */
} tBTA_FTS_OBX_PKT;

/* Directory Listing Information */
typedef struct
{
    tBTA_FS_DIRENTRY *p_entry;     /* Holds current directory entry */
    BOOLEAN           is_root;     /* TRUE if path is root directory */
} tBTA_FTS_DIRLIST;

/* Power management state for FTS */
#define BTA_FTS_PM_BUSY     0
#define BTA_FTS_PM_IDLE     1

/* FTS control block */
typedef struct
{
    tBTA_FTS_CBACK  *p_cback;       /* pointer to application callback function */
    char            *p_name;        /* Holds name of current operation */
    char            *p_dest;        /* Holds p_dest_name of current operation */
    char            *p_path;        /* Holds path of current operation */
    char            *p_rootpath;
    char            *p_workdir;     /* Current working directory */
    tBTA_FTS_OBX_PKT obx;           /* Holds the current OBX packet information */
    tBTA_FTS_DIRLIST dir;           /* Holds current directory list information */
    UINT32           sdp_handle;    /* SDP record handle */
    UINT32           file_length;   /* length of file being PUT/GET */
    UINT8            sess_id[OBX_SESSION_ID_SIZE]; /* session id */
    int              fd;            /* File Descriptor of opened file */
    BD_ADDR          bd_addr;       /* Device currently connected to */
    tOBX_HANDLE      obx_handle;
    UINT16           peer_mtu;
    UINT16           psm;           /* PSM for Obex Over L2CAP */
    UINT8            perms[BTA_FS_PERM_SIZE]; /* the permission */
    UINT8            scn;           /* SCN of the FTP server */
    tBTA_FTS_STATE   state;         /* state machine state */
    UINT8            obx_oper;      /* current active OBX operation PUT FILE, GET FILE, LISTING, etc */
    UINT8            app_id;
    BOOLEAN          auth_enabled;  /* Is OBEX authentication enabled */
    BOOLEAN          cout_active;   /* TRUE when waiting for a call-in function */
    tBTA_FT_OPER     acc_active;    /* op code when waiting for an access rsp (API) (0 not active) */
    BOOLEAN          suspending;    /* TRUE when suspending session */
    UINT8            resume_ssn;    /* the ssn for resume session */
    BOOLEAN          disabling;     /* TRUE when disabling server */
    UINT8            pm_state;      /* power management state */
} tBTA_FTS_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* FTS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_FTS_CB  bta_fts_cb;
#else
extern tBTA_FTS_CB *bta_fts_cb_ptr;
#define bta_fts_cb (*bta_fts_cb_ptr)
#endif

/* FT configuration constants */
extern tBTA_FT_CFG * p_bta_ft_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_fts_hdl_event(BT_HDR *p_msg);
extern void     bta_fts_sm_execute(tBTA_FTS_CB *p_cb, UINT16 event, tBTA_FTS_DATA *p_data);
extern void     bta_fts_sdp_register (tBTA_FTS_CB *p_cb, char *p_service_name);
extern void     bta_fts_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   tOBX_EVT_PARAM param, BT_HDR *p_pkt);

/* action functions */
extern void bta_fts_api_disable(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_api_authrsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_api_accessrsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_api_close(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_ci_write(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_ci_read(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_ci_open(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_ci_resume(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_ci_direntry(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_connect(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_disc(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_close(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_abort(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_password(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_put(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_get(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_setpath(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_obx_action(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_appl_tout(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_conn_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_disc_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_gasp_err_rsp(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_close_complete(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_disable_cmpl(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);
extern void bta_fts_session_req(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);

/* object store */
extern UINT8 bta_fts_mkdir(BT_HDR *p_pkt, tBTA_FT_OPER *p_op);
extern UINT8 bta_fts_chdir(BT_HDR *p_pkt, BOOLEAN backup_flag, tBTA_FT_OPER *p_op);
extern void  bta_fts_getdirlist(char *p_name);
extern void  bta_fts_proc_get_file(char *p_name);
extern void  bta_fts_proc_put_file(BT_HDR *p_pkt, char *p_name, BOOLEAN final_pkt, UINT8 oper);
extern void  bta_fts_delete(tBTA_FTS_OBX_EVENT *p_evt, const char *p);
extern void bta_fts_req_app_access (tBTA_FT_OPER oper, tBTA_FTS_CB *p_cb);

/* miscellaneous functions */
extern void    bta_fts_get_file_rsp(UINT8 rsp_code, UINT16 num_read);
extern void    bta_fts_put_file_rsp(UINT8 rsp_code);
extern void    bta_fts_end_of_list(UINT8 rsp_code);
extern UINT8   bta_fts_add_list_entry(void);
extern void    bta_fts_clean_list(tBTA_FTS_CB *p_cb);
extern void    bta_fts_clean_getput(tBTA_FTS_CB *p_cb, BOOLEAN is_aborted);
extern void    bta_fts_disable_cleanup(tBTA_FTS_CB *p_cb);
extern void    bta_fts_discard_data(UINT16 event, tBTA_FTS_DATA *p_data);

#endif /* BTA_FTS_INT_H */
