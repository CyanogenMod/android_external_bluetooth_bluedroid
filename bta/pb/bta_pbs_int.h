/*****************************************************************************
**
**  Name:           bta_pbs_int.h
**
**  Description:    This is the private file for the phone book access
**                  server (PBS).
**
**  Copyright (c) 2003-2010, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_PBS_INT_H
#define BTA_PBS_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_pbs_api.h"
#include "bta_pbs_co.h"
#include "bta_pbs_ci.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_PBS_TARGET_UUID "\x79\x61\x35\xf0\xf0\xc5\x11\xd8\x09\x66\x08\x00\x20\x0c\x9a\x66"
#define BTA_PBS_UUID_LENGTH                 16
#define BTA_PBS_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */

#define BTA_PBS_DEFAULT_VERSION             0x0101  /* for PBAP PSE version 1.1 */

#define BTA_PBS_GETVCARD_LISTING_TYPE       "x-bt/vcard-listing"
#define BTA_PBS_GETFILE_TYPE                "x-bt/phonebook"
#define BTA_PBS_GETVARD_ENTRY_TYPE          "x-bt/vcard"
#define BTA_PBS_PULLPB_NAME                 "telecom/pb.vcf"
#define BTA_PBS_PULLICH_NAME                "telecom/ich.vcf"
#define BTA_PBS_PULLOCH_NAME                "telecom/och.vcf"
#define BTA_PBS_PULLMCH_NAME                "telecom/mch.vcf"
#define BTA_PBS_PULLCCH_NAME                "telecom/cch.vcf"
#define BTA_PBS_PBFOLDER_NAME                 "pb"
#define BTA_PBS_ICHFOLDER_NAME                "ich"
#define BTA_PBS_OCHFOLDER_NAME                "och"
#define BTA_PBS_MCHFOLDER_NAME                "mch"
#define BTA_PBS_CCHFOLDER_NAME                "cch"

/* Tags for application parameter obex headers */
/* application parameter len: number of bytes + 2 (tag&len) */
#define BTA_PBS_TAG_ORDER          1    /* UINT8 */
#define BTA_PBS_TAG_SEARCH_VALUE   2    /* string */
#define BTA_PBS_TAG_SEARCH_ATTRIBUTE 3  /* UINT8 */
#define BTA_PBS_TAG_MAX_LIST_COUNT 4    /* UINT16 */
#define BTA_PBS_TAG_LIST_START_OFFSET  5    /* UINT16 */
#define BTA_PBS_TAG_FILTER         6    /* UINT32 */
#define BTA_PBS_TAG_FORMAT         7    /* UINT8 */
#define BTA_PBS_TAG_PB_SIZE        8    /* UINT16 */
#define BTA_PBS_TAG_NEW_MISSED_CALLS   9    /* UINT8 */

#define BTA_PBS_MAX_LIST_COUNT   65535

typedef tOBX_STATUS (tBTA_PBS_OBX_RSP) (tOBX_HANDLE handle, UINT8 rsp_code, BT_HDR *p_pkt);

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_PBS_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_PBS),

    BTA_PBS_API_AUTHRSP_EVT,        /* Response to password request */
    BTA_PBS_API_ACCESSRSP_EVT,      /* Response to an access request */
    BTA_PBS_API_CLOSE_EVT,          /* Response to a close request */
    BTA_PBS_CI_READ_EVT,            /* Response to Read request */
    BTA_PBS_CI_OPEN_EVT,            /* Response to File Open request */
    BTA_PBS_CI_VLIST_EVT,           /* Response to Get Vcard Entry request */
    BTA_PBS_OBX_CONN_EVT,           /* OBX Channel Connect Request */
    BTA_PBS_OBX_DISC_EVT,           /* OBX Channel Disconnect */
    BTA_PBS_OBX_ABORT_EVT,          /* OBX_operation aborted */
    BTA_PBS_OBX_PASSWORD_EVT,       /* OBX password requested */
    BTA_PBS_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_PBS_OBX_GET_EVT,            /* Read file data or folder listing */
    BTA_PBS_OBX_SETPATH_EVT,        /* Make or Change Directory */
    BTA_PBS_APPL_TOUT_EVT,          /* Timeout waiting for application */
    BTA_PBS_DISC_ERR_EVT,           /* Sends OBX_DisconnectRsp with error code */
    BTA_PBS_GASP_ERR_EVT,           /* Sends Err Resp to Get, Abort, Setpath */
    BTA_PBS_CLOSE_CMPL_EVT,         /* Finished closing channel */

    /* these events are handled outside the state machine */
    BTA_PBS_API_ENABLE_EVT
};

typedef UINT16 tBTA_PBS_INT_EVT;

typedef UINT8 tBTA_PBS_STATE;

/* data type for BTA_PBS_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_PBS_CBACK     *p_cback;
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
} tBTA_PBS_API_ENABLE;

/* data type for BTA_PBS_API_AUTHRSP_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   key [BTA_PBS_MAX_AUTH_KEY_SIZE];      /* The authentication key.*/
    UINT8   key_len;
    UINT8   userid [OBX_MAX_REALM_LEN];      /* The authentication user id.*/
    UINT8   userid_len;
} tBTA_PBS_API_AUTHRSP;

/* data type for BTA_PBS_API_ACCESSRSP_EVT */
typedef struct
{
    BT_HDR          hdr;
    char            *p_name;
    tBTA_PBS_OPER    oper;
    tBTA_PBS_ACCESS_TYPE  flag;
} tBTA_PBS_API_ACCESSRSP;

typedef struct
{
    BT_HDR            hdr;
    UINT32            file_size;
    int               fd;
    tBTA_PBS_CO_STATUS status;
} tBTA_PBS_CI_OPEN_EVT;

/* Read Ready Event */
typedef struct
{
    BT_HDR            hdr;
    int               fd;
    UINT16            num_read;
    tBTA_PBS_CO_STATUS status;
    BOOLEAN           final;
} tBTA_PBS_CI_READ_EVT;

/* Get Vlist Entry Ready Event */
typedef struct
{
    BT_HDR            hdr;
    tBTA_PBS_CO_STATUS status;
    BOOLEAN           final;
} tBTA_PBS_CI_VLIST_EVT;


/* data type for all obex events
    hdr.event contains the PBS event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_PBS_OBX_EVENT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_PBS_API_ENABLE     api_enable;
    tBTA_PBS_API_AUTHRSP    auth_rsp;
    tBTA_PBS_API_ACCESSRSP  access_rsp;
    tBTA_PBS_OBX_EVENT      obx_evt;
    tBTA_PBS_CI_OPEN_EVT    open_evt;
    tBTA_PBS_CI_READ_EVT    read_evt;
    tBTA_PBS_CI_VLIST_EVT   vlist_evt;
} tBTA_PBS_DATA;


/* OBX Response Packet Structure - Holds current response packet info */
typedef struct
{
    BT_HDR  *p_pkt;             /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8   *p_start;           /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available left in Obx packet */
    BOOLEAN  final_pkt;         /* (Put)     Holds the final bit of the Put packet */
} tBTA_PBS_OBX_PKT;


/* PBS control block */
typedef struct
{
    tBTA_PBS_CBACK  *p_cback;       /* pointer to application callback function */
    char            *p_name;        /* Holds name of current operation */
    char            *p_path;        /* Holds path of current operation */
    char            *p_rootpath;
    char            *p_workdir;     /* Current working directory */
    UINT8           *p_stream_indexes; /* Contains pointer to beginning of phonebook size area in ob hdr */
    tBTA_PBS_OBX_PKT obx;           /* Holds the current OBX packet information */
    tBTA_PBS_PULLPB_APP_PARAMS pullpb_app_params; /* PULLPB Application params */
    tBTA_PBS_VCARDLIST_APP_PARAMS getvlist_app_params; /* Get VLIST Application params */
    tBTA_PBS_VCARDLIST vlist;       /* Holds current directory list information */
    UINT32           sdp_handle;    /* SDP record handle */
    UINT32           file_length;   /* length of file being PUT/GET */
    int              fd;            /* File Descriptor of opened file */
    BD_ADDR          bd_addr;       /* Device currently connected to */
    tOBX_HANDLE      obx_handle;
    UINT16           peer_mtu;
    UINT16           num_vlist_idxs; /* keeps track of number of indexes in vCard listing */
    UINT8            scn;           /* SCN of the FTP server */
    tBTA_PBS_STATE   state;         /* state machine state */
    UINT8            obx_oper;      /* current active OBX operation GET FILE, LISTING, etc */
    UINT8            app_id;
    BOOLEAN          auth_enabled;  /* Is OBEX authentication enabled */
    BOOLEAN          cout_active;   /* TRUE when waiting for a call-in function */
    BOOLEAN          aborting;
    BOOLEAN          get_only_indexes; /* True if PCE only wants num indexes for vListing response */
    tBTA_PBS_OPER    acc_active;    /* op code when waiting for an access rsp (API) (0 not active) */
    tBTA_PBS_OBJ_TYPE obj_type;
    UINT8            realm [OBX_MAX_REALM_LEN];    /* The realm is intended to be
                                                         displayed to users so they know
                                                         which userid and password to use. 
                                                         The first byte of the string is
                                                         the character set of the string.
                                                       */
    UINT8            realm_len;
} tBTA_PBS_CB;



/* Configuration structure */
typedef struct
{
    UINT8       realm_charset;          /* Server only */
    BOOLEAN     userid_req;             /* TRUE if user id is required during obex authentication (Server only) */
    UINT8       supported_features;     /* Server supported features */
    UINT8       supported_repositories; /* Server supported repositories */

} tBTA_PBS_CFG;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* PBS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_PBS_CB  bta_pbs_cb;
#else
extern tBTA_PBS_CB *bta_pbs_cb_ptr;
#define bta_pbs_cb (*bta_pbs_cb_ptr)
#endif

/* PBS configuration constants */
extern tBTA_PBS_CFG * p_bta_pbs_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_pbs_hdl_event(BT_HDR *p_msg);
extern void     bta_pbs_sm_execute(tBTA_PBS_CB *p_cb, UINT16 event, tBTA_PBS_DATA *p_data);
extern void     bta_pbs_sdp_register (tBTA_PBS_CB *p_cb, char *p_service_name);
extern void     bta_pbs_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   tOBX_EVT_PARAM param, BT_HDR *p_pkt);

/* action functions */
extern void bta_pbs_api_disable(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_api_authrsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_api_accessrsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_api_close(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_connect(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_disc(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_close(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_abort(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_password(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_get(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_obx_setpath(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_appl_tout(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_conn_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_disc_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_gasp_err_rsp(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_close_complete(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_ci_open_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_ci_read_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
extern void bta_pbs_ci_vlist_act(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);
/* object store */
extern void  bta_pbs_proc_get_file(char *p_name, tBTA_PBS_OPER operation);
extern void  bta_pbs_req_app_access (tBTA_PBS_OPER oper, tBTA_PBS_CB *p_cb);
extern void  bta_pbs_getvlist(char *p_name);
/* miscellaneous functions */
extern void    bta_pbs_get_file_rsp(UINT8 rsp_code, UINT16 num_read);
extern void    bta_pbs_clean_getput(tBTA_PBS_CB *p_cb, BOOLEAN is_aborted);
extern void    bta_pbs_end_of_list(UINT8 rsp_code);
extern UINT8   bta_pbs_add_list_entry(void);
extern UINT8   bta_pbs_chdir(BT_HDR *p_pkt, BOOLEAN backup_flag, tBTA_PBS_OPER *p_op);
extern UINT8 * bta_pbs_read_app_params(BT_HDR *p_pkt, UINT8 tag, UINT16 *param_len);

#endif /* BTA_PBS_INT_H */
