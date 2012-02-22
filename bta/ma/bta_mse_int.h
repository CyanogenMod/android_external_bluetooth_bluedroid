/*****************************************************************************
**
**  Name:           bta_mse_int.h
**
**  Description:    This is the private file for the message access
**                  equipment (MSE) subsystem.
**
**  Copyright (c) 1998-2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_MSE_INT_H
#define BTA_MSE_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_ma_def.h"
#include "bta_mse_api.h"
#include "bta_mse_co.h"
#include "bta_mse_ci.h"
#include "bta_ma_util.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
typedef tOBX_STATUS (tBTA_MA_OBX_RSP) (tOBX_HANDLE handle, UINT8 rsp_code, BT_HDR *p_pkt);

#ifndef BTA_MSE_MN_DISC_SIZE
    #define BTA_MSE_MN_DISC_SIZE                500
#endif

#define BTA_MSE_64BIT_HEX_STR_SIZE          (16+1)
#define BTA_MSE_32BIT_HEX_STR_SIZE          (8+1)

#define BTA_MSE_MN_MAX_MSG_EVT_OBECT_SIZE   1536 /* 1.5 k */
/* mode field in tBTA_MSE_CO_FOLDER_ENTRY (OR'd together) */
#define BTA_MSE_A_RDONLY         GOEP_A_RDONLY
#define BTA_MSE_A_DIR            GOEP_A_DIR      /* Entry is a sub directory */

enum
{
    BTA_MSE_MA_GET_ACT_NEW_FOLDER_LIST = 0,
    BTA_MSE_MA_GET_ACT_CON_FOLDER_LIST,
    BTA_MSE_MA_GET_ACT_ERR_FOLDER_LIST,
    BTA_MSE_MA_GET_ACT_NEW_MSG_LIST,
    BTA_MSE_MA_GET_ACT_CON_MSG_LIST,
    BTA_MSE_MA_GET_ACT_ERR_MSG_LIST,
    BTA_MSE_MA_GET_ACT_NEW_MSG,
    BTA_MSE_MA_GET_ACT_CON_MSG,
    BTA_MSE_MA_GET_ACT_ERR_MSG,
    BTA_MSE_MA_GET_ACT_ERR,
    BTA_MSE_MA_GET_ACT_NONE,
    BTA_MSE_MA_GET_ACT_MAX
};
typedef UINT8 tBTA_MSE_MA_GET_ACT;

enum
{
    BTA_MSE_MA_GET_TYPE_FOLDER_LIST = 0,
    BTA_MSE_MA_GET_TYPE_MSG_LIST,
    BTA_MSE_MA_GET_TYPE_MSG,
    BTA_MSE_MA_GET_TYPE_CON_FOLDER_LIST,
    BTA_MSE_MA_GET_TYPE_CON_MSG_LIST,
    BTA_MSE_MA_GET_TYPE_CON_MSG,
    BTA_MSE_MA_GET_TYPE_NONE,
    BTA_MSE_MA_GET_TYPE_MAX
};
typedef UINT8 tBTA_MSE_MA_GET_TYPE;

enum
{
    BTA_MSE_MA_PUT_TYPE_NOTIF_REG = 0,
    BTA_MSE_MA_PUT_TYPE_EVT_RPT,
    BTA_MSE_MA_PUT_TYPE_NONE,
    BTA_MSE_MA_PUT_TYPE_MAX
};
typedef UINT8 tBTA_MSE_MA_PUT_TYPE;

enum
{
    BTA_MSE_MN_NOTIF_REG_STS_OFF = 0,
    BTA_MSE_MN_NOTIF_REG_STS_ON,
    BTA_MSE_MN_NOTIF_REG_STS_NONE,
    BTA_MSE_MN_NOTIF_REG_STS_MAX
};
typedef UINT8 tBTA_MSE_MN_NOTIF_REG_STS;

enum
{
    BTA_MSE_MN_ACT_TYPE_OPEN_CONN = 0,
    BTA_MSE_MN_ACT_TYPE_OPEN_CONN_NONE,
    BTA_MSE_MN_ACT_TYPE_OPEN_CONN_ERR,
    BTA_MSE_MN_ACT_TYPE_CLOSE_CONN,
    BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_NONE,
    BTA_MSE_MN_ACT_TYPE_CLOSE_CONN_ERR,
    BTA_MSE_MN_ACT_TYPE_NONE,
    BTA_MSE_MN_ACT_TYPE_MAX
};
typedef UINT8 tBTA_MSE_MN_ACT_TYPE;

typedef tOBX_STATUS (tBTA_MSE_OBX_RSP) (tOBX_HANDLE handle, UINT8 rsp_code, BT_HDR *p_pkt);

/* MSE Active MA obex operation (Valid in connected state) */
#define BTA_MSE_MA_OP_NONE                  0
#define BTA_MSE_MA_OP_SETPATH               1
#define BTA_MSE_MA_OP_GET_FOLDER_LIST       2
#define BTA_MSE_MA_OP_GET_MSG_LIST          3
#define BTA_MSE_MA_OP_GET_MSG               4   
#define BTA_MSE_MA_OP_SET_STATUS            5
#define BTA_MSE_MA_OP_PUSH_MSG              6

/* MAS Active MN obex operation (Valid in connected state) */
#define BTA_MSE_MN_OP_NONE         0
#define BTA_MSE_MN_OP_PUT_EVT_RPT  1


/* MN Abort state */
#define BTA_MSE_MN_ABORT_NONE          0x0
#define BTA_MSE_MN_ABORT_REQ_NOT_SENT  0x1
#define BTA_MSE_MN_ABORT_REQ_SENT      0x2
#define BTA_MSE_MN_ABORT_RSP_RCVD      0x4


/* Response Timer Operations */
#define BTA_MSE_TIMER_OP_NONE   0
#define BTA_MSE_TIMER_OP_STOP   1
#define BTA_MSE_TIMER_OP_ABORT  2
#define BTA_MSE_TIMER_OP_ALL    0xFF   /* use to stop all timers */

/* State Machine Events */
enum
{
    /* these events are handled by the state machine */
    BTA_MSE_INT_CLOSE_EVT = BTA_SYS_EVT_START(BTA_ID_MSE),
    BTA_MSE_API_ACCESSRSP_EVT, 
    BTA_MSE_API_UPD_IBX_RSP_EVT,
    BTA_MSE_API_SET_NOTIF_REG_RSP_EVT,
    BTA_MSE_INT_START_EVT,
    BTA_MSE_CI_GET_FENTRY_EVT,
    BTA_MSE_CI_GET_ML_INFO_EVT,
    BTA_MSE_CI_GET_ML_ENTRY_EVT,
    BTA_MSE_CI_GET_MSG_EVT,
    BTA_MSE_CI_PUSH_MSG_EVT,
    BTA_MSE_CI_DEL_MSG_EVT,
    BTA_MSE_MA_OBX_CONN_EVT,        /* OBX Channel Connect Request */
    BTA_MSE_MA_OBX_DISC_EVT,        /* OBX Channel Disconnect */
    BTA_MSE_MA_OBX_ABORT_EVT,       /* OBX_operation aborted */
    BTA_MSE_MA_OBX_CLOSE_EVT,       /* OBX Channel Disconnected (Link Lost) */
    BTA_MSE_MA_OBX_PUT_EVT,         /* Write file data or delete */
    BTA_MSE_MA_OBX_GET_EVT,         /* Read file data or folder listing */
    BTA_MSE_MA_OBX_SETPATH_EVT,     /* Make or Change Directory */
    BTA_MSE_CLOSE_CMPL_EVT,         /* Finished closing channel */

    /* Message Notification Client events */
    BTA_MSE_MN_INT_OPEN_EVT,
    BTA_MSE_MN_INT_CLOSE_EVT,
    BTA_MSE_MN_SDP_OK_EVT,
    BTA_MSE_MN_SDP_FAIL_EVT,
    BTA_MSE_MN_OBX_CONN_RSP_EVT,
    BTA_MSE_MN_OBX_PUT_RSP_EVT,
    BTA_MSE_MN_OBX_CLOSE_EVT,
    BTA_MSE_MN_OBX_TOUT_EVT,
    BTA_MSE_MN_CLOSE_CMPL_EVT,
    BTA_MSE_API_SEND_NOTIF_EVT,
    BTA_MSE_API_MN_ABORT_EVT,
    BTA_MSE_MN_OBX_ABORT_RSP_EVT,
    BTA_MSE_MN_RSP_TOUT_EVT,

    /* these events are handled outside the state machine */
    BTA_MSE_MN_RSP0_TOUT_EVT,   /* timeout event for MN control block index 0*/
    BTA_MSE_MN_RSP1_TOUT_EVT,    
    BTA_MSE_MN_RSP2_TOUT_EVT,  
    BTA_MSE_MN_RSP3_TOUT_EVT,  
    BTA_MSE_MN_RSP4_TOUT_EVT,  
    BTA_MSE_MN_RSP5_TOUT_EVT,  
    BTA_MSE_MN_RSP6_TOUT_EVT,   /* Bluetooth limit for upto 7 devices*/  
    BTA_MSE_API_ENABLE_EVT,
    BTA_MSE_API_DISABLE_EVT,
    BTA_MSE_API_START_EVT,
    BTA_MSE_API_STOP_EVT,
    BTA_MSE_API_CLOSE_EVT,
    BTA_MSE_API_MA_CLOSE_EVT,
    BTA_MSE_API_MN_CLOSE_EVT

};
typedef UINT16 tBTA_MSE_INT_EVT;

#define BTA_MSE_MN_EVT_MIN      BTA_MSE_MN_INT_OPEN_EVT
#define BTA_MSE_MN_EVT_MAX      BTA_MSE_MN_RSP_TOUT_EVT


/* state machine states */
enum
{
    BTA_MSE_MA_IDLE_ST = 0,      /* Idle  */
    BTA_MSE_MA_LISTEN_ST,        /* Listen - waiting for OBX/RFC connection */
    BTA_MSE_MA_CONN_ST,          /* Connected - MA Session is active */
    BTA_MSE_MA_CLOSING_ST        /* Closing is in progress */
};
typedef UINT8 tBTA_MSE_MA_STATE;

/* data type for BTA_MSE_MA_CI_GET_FENTRY_EVT note:sdh */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
} tBTA_MSE_CI_GET_FENTRY;

/* data type for BTA_MSE_MA_CI_GET_FENTRY_EVT note:sdh */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
} tBTA_MSE_CI_GET_ML_INFO;



typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
} tBTA_MSE_CI_GET_ML_ENTRY;


typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
    BOOLEAN             last_packet;
    tBTA_MA_MSG_HANDLE  handle;
} tBTA_MSE_CI_PUSH_MSG;


typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
    UINT16              filled_buff_size; 
    tBTA_MA_MPKT_STATUS multi_pkt_status;
    tBTA_MA_FRAC_DELIVER frac_deliver_status;
} tBTA_MSE_CI_GET_MSG;

typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id; 
    tBTA_MA_STATUS      status;
} tBTA_MSE_CI_DEL_MSG;



/* data type for BTA_MSE_API_ENABLE_EVT note:sdh */
typedef struct
{
    BT_HDR             hdr;
    tBTA_MSE_CBACK     *p_cback;        /* pointer to application callback function */
    UINT8              app_id;
} tBTA_MSE_API_ENABLE;


/* data type for BTA_MSE_API_DISABLE_EVT note:sdh */
typedef struct
{
    BT_HDR             hdr;
    UINT8              app_id;
} tBTA_MSE_API_DISABLE;

/* data type for BTA_MSE_API_START_EVT note:sdh */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_INST_ID     mas_inst_id;
    char                servicename[BTA_SERVICE_NAME_LEN + 1];
    char                *p_root_path;
    tBTA_SEC            sec_mask;
    tBTA_MA_MSG_TYPE    sup_msg_type;
} tBTA_MSE_API_START;

/* data type for BTA_MSE_API_STOP_EVT note:sdh */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_INST_ID     mas_inst_id;

} tBTA_MSE_API_STOP;

/* data type for BTA_MSE_API_CLOSE_EVT  */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_INST_ID     mas_instance_id;

} tBTA_MSE_API_CLOSE;

/* data type for BTA_MSE_API_MA_CLOSE_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    tBTA_MA_INST_ID     mas_instance_id;

} tBTA_MSE_API_MA_CLOSE;

/* data type for BTA_MSE_INT_CLOSE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE    mas_session_id;

} tBTA_MSE_INT_CLOSE;


/* data type for BTA_MSE_API_MN_CLOSE_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
} tBTA_MSE_API_MN_CLOSE;


typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_SESS_HANDLE mas_session_id;
    tBTA_MSE_OPER       oper;
    tBTA_MA_ACCESS_TYPE rsp; 
    char                *p_path;
} tBTA_MSE_API_ACCESSRSP;

typedef struct
{
    BT_HDR                      hdr;
    tBTA_MA_SESS_HANDLE         mas_session_id;
    tBTA_MSE_UPDATE_INBOX_TYPE  rsp; 
    char                        *p_path;
} tBTA_MSE_API_UPDINBRSP;

typedef struct
{
    BT_HDR                      hdr;
    tBTA_MA_SESS_HANDLE         mas_session_id;
    tBTA_MSE_SET_NOTIF_REG_TYPE rsp; 
} tBTA_MSE_API_SETNOTIFREGRSP;

/* data type for all obex events
    hdr.event contains the MAS event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_MSE_OBX_EVT;

/* OBX Response Packet Structure - Holds current response packet info */
typedef struct
{
    BT_HDR   *p_pkt;            /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8    *p_start;          /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available left in Obx packet */
    BOOLEAN  final_pkt;         /* (Put)     Holds the final bit of the Put packet */
} tBTA_MSE_OBX_PKT;

/* data type for BTA_MSE_MN_INT_OPEN_EVT */
typedef struct
{
    BT_HDR                  hdr;
    UINT8                   ccb_idx;        
    BD_ADDR                 bd_addr; 
    tBTA_SEC                sec_mask;
}tBTA_MSE_MN_INT_OPEN;

/* data type for BTA_MSE_MN_INT_OPEN_EVT */
typedef struct
{
    BT_HDR                  hdr;
    UINT8                   ccb_idx;     
}tBTA_MSE_MN_INT_CLOSE;

/* data type for BTA_MSE_MN_SDP_OK_EVT */
typedef struct
{
    BT_HDR                  hdr;
    UINT8                   ccb_idx;  
    UINT8                   scn;
}tBTA_MSE_MN_SDP_OK;


/* data type for BTA_MSE_MN_SDP_FAIL_EVT */
typedef struct
{
    BT_HDR                  hdr;
    UINT8                   ccb_idx;     
}tBTA_MSE_MN_SDP_FAIL;


/* data type for BTA_MSE_MN_API_SEND_NOTIF_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_INST_ID     mas_instance_id; 
    tBTA_MSE_NOTIF_TYPE notif_type; 
    tBTA_MA_MSG_HANDLE  handle; 
    char                *p_folder;     
    char                *p_old_folder; 
    tBTA_MA_MSG_TYPE    msg_type;
    BD_ADDR             except_bd_addr;
} tBTA_MSE_MN_API_SEND_NOTIF;

/* data type for BTA_MSE_API_MN_ABORT_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_INST_ID     mas_instance_id; 
}tBTA_MSE_MN_API_ABORT;


/* union of all state machine event data types */
typedef union
{
    BT_HDR                      hdr;
    tBTA_MSE_API_ENABLE         api_enable; /* data for BTA_MSE_API_ENABLE_EVT */
    tBTA_MSE_API_DISABLE        api_disable; 
    tBTA_MSE_API_START          api_start;
    tBTA_MSE_API_STOP           api_stop;
    tBTA_MSE_API_CLOSE          api_close;
    tBTA_MSE_INT_CLOSE          int_close;
    tBTA_MSE_API_MA_CLOSE       api_ma_close;
    tBTA_MSE_API_MN_CLOSE       api_mn_close;
    tBTA_MSE_API_ACCESSRSP      api_access_rsp;
    tBTA_MSE_API_UPDINBRSP      api_upd_ibx_rsp;
    tBTA_MSE_API_SETNOTIFREGRSP api_set_notif_reg_rsp;
    tBTA_MSE_OBX_EVT            obx_evt;
    tBTA_MSE_CI_GET_FENTRY      ci_get_fentry;
    tBTA_MSE_CI_GET_ML_INFO     ci_get_ml_info;
    tBTA_MSE_CI_GET_ML_ENTRY    ci_get_ml_entry;
    tBTA_MSE_CI_GET_MSG         ci_get_msg;
    tBTA_MSE_CI_PUSH_MSG        ci_push_msg;
    tBTA_MSE_CI_DEL_MSG         ci_del_msg;
    tBTA_MSE_MN_INT_OPEN        mn_int_open;
    tBTA_MSE_MN_INT_CLOSE       mn_int_close;
    tBTA_MSE_MN_SDP_OK          mn_sdp_ok;
    tBTA_MSE_MN_SDP_FAIL        mn_sdp_fail;
    tBTA_MSE_MN_API_SEND_NOTIF  mn_send_notif;
    tBTA_MSE_MN_API_ABORT       mn_abort;

} tBTA_MSE_DATA;

typedef struct
{
    char             *p_path;
    char             *p_name; 
    tBTA_MA_DIR_NAV  flags;
} tBTA_MSE_OPER_SETPATH;

/* Directory Listing bufer pointer */
typedef struct
{
    tBTA_MSE_CO_FOLDER_ENTRY    *p_entry;     /* Holds current directory entry */
    BOOLEAN                     is_root;     /* TRUE if path is root directory */
} tBTA_MSE_DIRLIST;

typedef struct
{
    UINT16      max_list_cnt;    
    UINT16      start_offset;
    UINT16      list_cnt;
} tBTA_MSE_OPER_FLIST_PARAM;

/* Message Listing buffer pointers */
typedef struct
{
    tBTA_MSE_CO_MSG_LIST_ENTRY *p_entry;
    tBTA_MSE_CO_MSG_LIST_INFO  *p_info;
    BOOLEAN                     pending_ml_frag;
    char                        *p_xml_buf;
    UINT16                      offset;
    UINT16                      remaing_size;
} tBTA_MSE_MSGLIST;

typedef struct
{
    tBTA_MA_MSG_LIST_FILTER_PARAM   filter;
    char                            *p_name;
    char                            *p_path;
    BOOLEAN                         w4info;   
} tBTA_MSE_OPER_MLIST_PARAM;


typedef struct
{
    BOOLEAN                         notif_status_rcv;
    tBTA_MA_NOTIF_STATUS            notif_status;  
} tBTA_NOTIF_REG;

typedef struct
{
    tBTA_MA_GET_MSG_PARAM data;
    char                  *p_msg_handle;
    UINT8                 *p_frac_delivery;
    UINT16                byte_get_cnt;
    UINT16                filled_buff_size;
    BOOLEAN               add_frac_del_hdr;   
    tBTA_MA_FRAC_DELIVER  frac_deliver_status;
} tBTA_MSE_OPER_MSG_PARAM;

typedef struct
{
    tBTA_MA_PUSH_MSG_PARAM  param;
    UINT16                  push_byte_cnt;
    BOOLEAN                 rcv_folder_name;
    BOOLEAN                 rcv_charset;
    BOOLEAN                 rcv_transparent;
    BOOLEAN                 rcv_retry;
    BOOLEAN                 first_req_pkt;
} tBTA_MSE_OPER_PUSH_MSG;

typedef struct
{
    char                    *p_msg_handle;
    BOOLEAN                 rcv_msg_handle;
    BOOLEAN                 rcv_sts_ind;
    BOOLEAN                 rcv_sts_val;
    tBTA_MA_MSG_HANDLE      handle;
    tBTA_MA_STS_INDCTR      sts_ind;
    tBTA_MA_STS_VALUE       sts_val;
} tBTA_MSE_OPER_SET_MSG_STS;


/* MAS control block */
typedef struct
{
    tBTA_NOTIF_REG              notif_reg_req;
    tBTA_MSE_OPER_PUSH_MSG      push_msg;
    tBTA_MSE_OPER_SET_MSG_STS   set_msg_sts;
    tBTA_MSE_OPER_SETPATH       sp;
    tBTA_MSE_OPER_FLIST_PARAM   fl_param;
    tBTA_MSE_DIRLIST            dir;            
    tBTA_MSE_OPER_MLIST_PARAM   ml_param;
    tBTA_MSE_MSGLIST            ml;            /* point to the Message-Listing object buffer */  
    tBTA_MSE_OPER_MSG_PARAM     msg_param;
    tBTA_MSE_OBX_PKT            obx; 
    char                        *p_workdir;    /* Current working folder */
    tOBX_HANDLE                 obx_handle;    /* OBEX handle,used as MAS session ID as well */
    UINT16                      peer_mtu;
    BOOLEAN                     cout_active;   /* TRUE when waiting for a call-in function */
    BOOLEAN                     aborting;
    BD_ADDR                     bd_addr;       /* Device currently connected to */
    tBTA_MSE_OPER               oper;      
    tBTA_MSE_MA_STATE           state;         /* state machine state */

} tBTA_MSE_MA_SESS_CB;

typedef struct
{
    tBTA_MSE_MA_SESS_CB sess_cb[BTA_MSE_NUM_SESS];
    UINT32              sdp_handle;    /* SDP record handle */
    char                *p_rootpath;
    tOBX_HANDLE         obx_handle;    /* OBEX handle ID */
    BOOLEAN             stopping;
    BOOLEAN             in_use;
    tBTA_SEC            sec_mask; 
    tBTA_MA_MSG_TYPE    sup_msg_type;  
    char                servicename[BTA_SERVICE_NAME_LEN + 1];
    UINT8               scn;           /* SCN of the MA server */
    tBTA_MA_INST_ID     mas_inst_id;   /* MAS instance id */
}tBTA_MSE_MA_CB;

/* state machine states */
enum
{
    BTA_MSE_MN_IDLE_ST = 0,      /* Idle  */
    BTA_MSE_MN_W4_CONN_ST,       /* wait for connection state */
    BTA_MSE_MN_CONN_ST,          /* Connected - MAS Session is active */
    BTA_MSE_MN_CLOSING_ST        /* Closing is in progress */
};
typedef UINT8 tBTA_MSE_MN_STATE;

/* notification registration status */
typedef struct
{
    BOOLEAN                     status;
    tBTA_MA_INST_ID             mas_inst_id; /* only valid when status == TRUE*/
} tBTA_MSE_MN_REG_STATUS;

/* Message notification report */
typedef struct
{
    UINT16              buffer_len;
    UINT16              bytes_sent; 
    BOOLEAN             final_pkt;
    UINT8               pkt_cnt;
    tBTA_MA_INST_ID     mas_instance_id; 
    UINT8               *p_buffer;
}tBTA_MSE_MN_MSG_NOTIF;

typedef struct
{
    tBTA_MSE_MN_REG_STATUS      notif_reg[BTA_MSE_NUM_INST];   
    tBTA_MSE_OBX_PKT            obx;           /* Holds the current OBX packet information */
    tBTA_MSE_MN_MSG_NOTIF       msg_notif;
    TIMER_LIST_ENT              rsp_timer;     /* response timer */
    tSDP_DISCOVERY_DB           *p_db;          /* pointer to discovery database */
    tSDP_DISC_CMPL_CB           *sdp_cback;
    tOBX_HANDLE                 obx_handle;
    UINT16                      sdp_service;
    UINT16                      peer_mtu;
    BOOLEAN                     in_use;
    BOOLEAN                     req_pending;   /* TRUE when waiting for an obex response  */
    BOOLEAN                     cout_active;   /* TRUE if call-out is currently active */
    BOOLEAN                     sdp_pending;
    BD_ADDR                     bd_addr;       /* Peer device MNS server address */
    tBTA_MSE_MN_STATE           state;         /* state machine state */
    tBTA_SEC                    sec_mask;
    UINT8                       timer_oper;    /* current active response timer action (abort or close) */
    UINT8                       obx_oper;      /* current active OBX operation PUT or GET  operations */
    UINT8                       aborting;
}tBTA_MSE_MN_CB;

typedef struct
{
    BOOLEAN        in_use;
    BD_ADDR        bd_addr;
    BOOLEAN        opened;
    BOOLEAN        busy;
}tBTA_MSE_PM_CB;


typedef struct
{
    tBTA_MSE_MA_CB            scb[BTA_MSE_NUM_INST]; /* MA Server Control Blocks */
    tBTA_MSE_MN_CB            ccb[BTA_MSE_NUM_MN];   /* MN Client Control Blocks */
    tBTA_MSE_PM_CB            pcb[BTA_MSE_NUM_MN];
    tBTA_MSE_CBACK            *p_cback;        /* pointer to application event callback function */
    BOOLEAN                   enable;
    BOOLEAN                   disabling; 
    UINT8                     app_id;
}tBTA_MSE_CB;

/******************************************************************************
**  Configuration Definitions
*******************************************************************************/
/* Configuration structure */
typedef struct
{
    INT32       abort_tout;             /* Timeout in milliseconds to wait for abort OBEX response (client only) */
    INT32       stop_tout;              /* Timeout in milliseconds to wait for close OBEX response (client only) */

} tBTA_MSE_MA_CFG;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* MAS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_MSE_CB  bta_mse_cb;
#else
extern tBTA_MSE_CB *bta_mse_cb_ptr;
    #define bta_mse_cb (*bta_mse_cb_ptr)
#endif

#define BTA_MSE_GET_INST_CB_PTR(inst_idx) &(bta_mse_cb.scb[(inst_idx)])
#define BTA_MSE_GET_SESS_CB_PTR(inst_idx, sess_idx )  &(bta_mse_cb.scb[(inst_idx)].sess_cb[(sess_idx)])
#define BTA_MSE_GET_MN_CB_PTR(ccb_idx) &(bta_mse_cb.ccb[(ccb_idx)])
#define BTA_MSE_GET_PM_CB_PTR(pcb_idx) &(bta_mse_cb.pcb[(pcb_idx)])
/* MAS configuration constants */
extern tBTA_MSE_MA_CFG * p_bta_ms_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
/* MSE event handler for MA and MN  */

#ifdef __cplusplus
extern "C"
{
#endif

    extern BOOLEAN bta_mse_hdl_event(BT_HDR *p_msg);

/* State machine drivers  */
    extern void bta_mse_ma_sm_execute(UINT8 inst_idx, UINT8 sess_idx, UINT16 event, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_sm_execute(UINT8 ccb_idx, UINT16 event, tBTA_MSE_DATA *p_data);

    extern void bta_mse_ma_sdp_register (tBTA_MSE_MA_CB *p_scb, char *p_service_name, tBTA_MA_INST_ID inst_id, tBTA_MA_MSG_TYPE msg_type);

/* Obx callback functions  */
    extern void bta_mse_ma_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                      tOBX_EVT_PARAM param, BT_HDR *p_pkt);

    extern void bta_mse_mn_obx_cback (tOBX_HANDLE handle, tOBX_EVENT obx_event, UINT8 rsp_code,
                                      tOBX_EVT_PARAM param, BT_HDR *p_pkt);

/* action functions for MA */
    extern void bta_mse_ma_int_close(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_api_accessrsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_api_upd_ibx_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_api_set_notif_reg_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);

    extern void bta_mse_ma_ci_get_folder_entry(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_ci_get_ml_info(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_ci_get_ml_entry(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_ci_get_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_ci_push_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_ci_del_msg(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);

    extern void bta_mse_ma_obx_disc(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_connect(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_close(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_abort(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_put(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_get(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_obx_setpath(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_conn_err_rsp(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_ma_close_complete(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);

/* action function for MN */
    extern void bta_mse_mn_init_sdp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_start_client(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_stop_client(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_obx_conn_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_close(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_send_notif(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_rsp_timeout(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_put_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_obx_tout(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_close_compl(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_ignore_obx(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_sdp_fail(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_abort(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);
    extern void bta_mse_mn_abort_rsp(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);


/* miscellaneous functions */
    extern void bta_mse_discard_data(UINT16 event, tBTA_MSE_DATA *p_data);

/* utility functions */

    extern void bta_mse_send_push_msg_in_prog_evt(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_send_get_msg_in_prog_evt(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_send_oper_cmpl_evt(UINT8 inst_idx, UINT8 sess_idx, tBTA_MA_STATUS status);
    extern void bta_mse_pushmsg(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN first_pkt);
    extern void bta_mse_clean_set_msg_sts(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_clean_set_notif_reg(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_clean_push_msg(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_init_set_msg_sts(UINT8 inst_idx, UINT8 sess_idx, UINT8 *p_rsp_code);
    extern void bta_mse_init_push_msg(UINT8 inst_idx, UINT8 sess_idx, UINT8 *p_rsp_code);
    extern void bta_mse_remove_uuid(void);
    extern void bta_mse_clean_mas_service(UINT8 inst_idx);
    extern void bta_mse_abort_too_late(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_clean_getput(UINT8 inst_idx, UINT8 sess_idx, tBTA_MA_STATUS status);
    extern void bta_mse_clean_msg(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_end_of_msg(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code);
    extern void bta_mse_getmsg(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN new_req);
    extern tBTA_MA_STATUS bta_mse_build_msg_listing_obj(tBTA_MSE_CO_MSG_LIST_ENTRY *p_entry, 
                                                        UINT16 *p_size, char *p_buf  );
    extern  void bta_mse_clean_msg_list(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_end_of_msg_list(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code);
    extern UINT8 bta_mse_add_msg_list_info(UINT8 inst_idx, UINT8 sess_idx);
    extern UINT8 bta_mse_add_msg_list_entry(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_getmsglist(UINT8 inst_idx, UINT8 sess_idx,BOOLEAN new_req);
    extern UINT8 * bta_mse_read_app_params(BT_HDR *p_pkt, UINT8 tag, UINT16 *param_len);
    extern void bta_mse_clean_list(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_end_of_list(UINT8 inst_idx, UINT8 sess_idx, UINT8 rsp_code);
    extern UINT8 bta_mse_add_list_entry(UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_getfolderlist(UINT8 inst_idx, UINT8 sess_idx, BOOLEAN new_req);
    extern UINT8 bta_mse_chdir(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_OPER *p_oper);
    extern BOOLEAN bta_mse_send_set_notif_reg(UINT8 status,
                                              UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_proc_notif_reg_status(UINT8 status,
                                              UINT8 inst_idx, UINT8 sess_idx);
    extern void bta_mse_discard_data(UINT16 event, tBTA_MSE_DATA *p_data);
    extern BOOLEAN bta_mse_find_bd_addr_match_sess_cb_idx(BD_ADDR bd_addr, UINT8 inst_idx, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_mas_inst_id_match_cb_idx(tBTA_MA_INST_ID mas_inst_id, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_handle_match_mas_inst_cb_idx(tOBX_HANDLE obx_handle, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_mas_sess_cb_idx(tOBX_HANDLE obx_handle, UINT8 *p_mas_inst_idx, UINT8 *p_mas_sess_idx);
    extern BOOLEAN bta_mse_find_ma_cb_indexes(tBTA_MSE_DATA *p_msg, UINT8 *p_inst_idx, UINT8  *p_sess_idx);
    extern void bta_mse_ma_cleanup(tBTA_MSE_DATA *p_msg);
    extern BOOLEAN bta_mse_is_a_duplicate_id(tBTA_MA_INST_ID mas_inst_id);
    extern BOOLEAN bta_mse_find_avail_mas_inst_cb_idx(UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_avail_mas_sess_cb_idx(tBTA_MSE_MA_CB *p_scb, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_sess_id_match_ma_cb_indexes(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                            UINT8 *p_inst_idx, UINT8 *p_sess_idx);
    extern BOOLEAN bta_mse_find_avail_mn_cb_idx(UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_bd_addr_match_mn_cb_index(BD_ADDR p_bd_addr, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_sess_id_match_mn_cb_index(tBTA_MA_SESS_HANDLE mas_session_id, 
                                                          UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_mn_cb_index(tBTA_MSE_DATA *p_msg, UINT8 *p_ccb_idx);
    extern void bta_mse_mn_cleanup(tBTA_MSE_DATA *p_msg);
    extern tBTA_MA_STATUS bta_mse_build_map_event_rpt_obj(tBTA_MSE_NOTIF_TYPE notif_type, 
                                                          tBTA_MA_MSG_HANDLE handle, 
                                                          char * p_folder,
                                                          char * p_old_folder, 
                                                          tBTA_MA_MSG_TYPE msg_typ,
                                                          UINT16 * p_len,
                                                          UINT8 * p_buffer);
    extern void bta_mse_mn_start_timer(UINT8 ccb_idx, UINT8 timer_id);
    extern void bta_mse_mn_stop_timer(UINT8 ccb_idx, UINT8 timer_id);
    extern BOOLEAN bta_mse_mn_add_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id);
    extern BOOLEAN bta_mse_mn_remove_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id);
    extern void bta_mse_mn_remove_all_inst_ids(UINT8 ccb_idx);
    extern UINT8 bta_mse_mn_find_num_of_act_inst_id(UINT8 ccb_idx);
    extern BOOLEAN bta_mse_mn_is_inst_id_exist(UINT8 ccb_idx, tBTA_MA_INST_ID mas_inst_id );
    extern BOOLEAN bta_mse_mn_is_ok_to_close_mn(BD_ADDR bd_addr, tBTA_MA_INST_ID mas_inst_id );
    extern BOOLEAN bta_mse_mn_get_first_inst_id(UINT8 ccb_idx, tBTA_MA_INST_ID *p_mas_inst_id);
    extern tBTA_MA_STATUS bta_mse_mn_cont_send_notif(UINT8 ccb_idx, BOOLEAN first_pkt);
    extern void bta_mse_mn_send_notif_evt(tBTA_MA_INST_ID mas_instance_id, tBTA_MA_STATUS status,
                                          BD_ADDR bd_addr );
    extern void bta_mse_mn_clean_send_notif(UINT8 ccb_idx);
    extern void bta_mse_ma_fl_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt);
    extern void bta_mse_ma_ml_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt);
    extern BOOLEAN bta_mse_ma_msg_read_app_params(UINT8 inst_idx, UINT8 sess_idx, BT_HDR *p_pkt);
    extern BOOLEAN bta_mse_get_msglist_path(UINT8 inst_idx, UINT8 sess_idx);

    extern BOOLEAN bta_mse_find_pm_cb_index(BD_ADDR p_bd_addr, UINT8 *p_idx);
    extern BOOLEAN bta_mse_find_avail_pm_cb_idx(UINT8 *p_idx);
    extern void bta_mse_pm_conn_open(BD_ADDR bd_addr);
    extern void bta_mse_pm_conn_close(BD_ADDR bd_addr);
    extern void bta_mse_set_ma_oper(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_OPER oper);
    extern void bta_mse_set_mn_oper(UINT8 ccb_idx, UINT8 oper);
#ifdef __cplusplus
}
#endif
#endif /* BTA_MSE_INT_H */
