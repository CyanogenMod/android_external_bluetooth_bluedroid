/*****************************************************************************
**
**  Name:         obx_int.h
**
**  File:         OBEX Internal header file
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef OBX_INT_H
#define OBX_INT_H

#include "bt_target.h"
#include "obx_api.h"
#include "gki.h"

#define OBX_DEFAULT_TARGET_LEN      0xFF
#define OBX_INITIAL_CONN_ID         0x1

/* the rules for OBX handles: (some of the definitions are in obx_api.h)
 *
 * tOBX_HANDLE is UINT16
 * It was UINT8, the support for multiple clients on the same SCN for MAP is required
 *
 * LSB (0x00FF) is the same as the old definition.
 * The 0x80 bit (OBX_CL_HANDLE_MASK) is set for client connections.
 * The 0x40 bit (OBX_HANDLE_RX_MTU_MASK) is used internally for RFCOMM to allocate a buffer to receive data
 *
 * The MSB (0xFF00) is used for enhancements add for BTE release 3.15
 * This byte is the session index; used for server only.
 */

#define OBX_CL_HANDLE_MASK          0x80
#define OBX_CL_CB_IND_MASK          0x007F
#define OBX_HANDLE_RX_MTU_MASK      0x40
#define OBX_LOCAL_NONCE_SIZE        OBX_MIN_NONCE_SIZE  /* 4 - 16 per IrOBEX spec. local nonce is always 4 */

#define OBX_MAX_EVT_MAP_NUM     (OBX_ABORT_REQ_SEVT+1)

#define OBX_PORT_EVENT_MASK  (PORT_EV_RXCHAR | PORT_EV_TXEMPTY | \
                              PORT_EV_FC | PORT_EV_FCS)

#define OBX_BAD_SM_EVT              0xFF

/* for wait_auth flag */
#define OBX_WAIT_AUTH_FAIL          2

enum
{
    OBX_CS_NULL,                /* 0    0 */
    OBX_CS_NOT_CONNECTED,       /* 1    1 */
    OBX_CS_SESSION_REQ_SENT,    /* 2      */
    OBX_CS_CONNECT_REQ_SENT,    /* 3    2 */
    OBX_CS_UNAUTH,              /* 4    3 */
    OBX_CS_CONNECTED,           /* 5    4 */
    OBX_CS_DISCNT_REQ_SENT,     /* 6    5 */
    OBX_CS_OP_UNAUTH,           /* 7    6 */
    OBX_CS_SETPATH_REQ_SENT,    /* 8    7 */
    OBX_CS_ACTION_REQ_SENT,     /* 9      */
    OBX_CS_ABORT_REQ_SENT,      /*10    8 */
    OBX_CS_PUT_REQ_SENT,        /*11    9 */
    OBX_CS_GET_REQ_SENT,        /*12   10 */
    OBX_CS_PUT_TRANSACTION,     /*13   11 */
    OBX_CS_GET_TRANSACTION,     /*14   12 */
    OBX_CS_PUT_SRM,             /*15      */
    OBX_CS_GET_SRM,             /*16      */
    OBX_CS_PARTIAL_SENT,        /*17   13 */
    OBX_CS_MAX
};

typedef UINT8 tOBX_CL_STATE;
#define OBX_CL_STATE_DROP   0x80 /* this is used only in session_info[] to mark link drop suspend*/

enum
{
    OBX_CONNECT_REQ_CEVT,   /* API call to send a CONNECT request. */
    OBX_SESSION_REQ_CEVT,   /* API call to send a SESSION request. */
    OBX_DISCNT_REQ_CEVT,    /* API call to send a DISCONNECT request. */
    OBX_PUT_REQ_CEVT,       /* API call to send a PUT request. */
    OBX_GET_REQ_CEVT,       /* API call to send a GET request.*/
    OBX_SETPATH_REQ_CEVT,   /* API call to send a SETPATH request. */
    OBX_ACTION_REQ_CEVT,    /* API call to send an ACTION request. */
    OBX_ABORT_REQ_CEVT,     /* API call to send an ABORT request. */
    OBX_OK_CFM_CEVT,        /* Received success response from server. */
    OBX_CONT_CFM_CEVT,      /* Received continue response from server.  */
    OBX_FAIL_CFM_CEVT,      /* Received failure response from server. */
    OBX_PORT_CLOSE_CEVT,    /* Transport is closed. */
    OBX_TX_EMPTY_CEVT,      /* Transmit Queue Empty */
    OBX_FCS_SET_CEVT,       /* Data flow enable */
    OBX_STATE_CEVT,         /* Change state. */
    OBX_TIMEOUT_CEVT,       /* Timeout occurred. */
    OBX_MAX_CEVT
};
typedef UINT8 tOBX_CL_EVENT;

#define OBX_MAX_API_CEVT    OBX_ABORT_REQ_CEVT

enum
{
    OBX_CONNECT_REQ_SEVT,   /* 1  1 Received CONNECT request from client. */
    OBX_SESSION_REQ_SEVT,   /* 2    Received SESSION request from client. */
    OBX_DISCNT_REQ_SEVT,    /* 3  2 Received DISCONNECT request from client. */
    OBX_PUT_REQ_SEVT,       /* 4  3 Received PUT request from client. */
    OBX_GET_REQ_SEVT,       /* 5  4 Received GET request from client. */
    OBX_SETPATH_REQ_SEVT,   /* 6  5 Received SETPATH request from client. */
    OBX_ACTION_REQ_SEVT,    /* 7    Received ACTION request  from client. */
    OBX_ABORT_REQ_SEVT,     /* 8  6 Received ABORT request from client. */
    OBX_CONNECT_CFM_SEVT,   /* 9  7 API call to send a CONNECT response. */
    OBX_SESSION_CFM_SEVT,   /*10    API call to send a SESSION response. */
    OBX_DISCNT_CFM_SEVT,    /*11  8 API call to send a DISCONNECT response or close the connection to the client. */
    OBX_PUT_CFM_SEVT,       /*12  9 API call to send a PUT response. */
    OBX_GET_CFM_SEVT,       /*13 10 API call to send a GET response. */
    OBX_SETPATH_CFM_SEVT,   /*14 11 API call to send a SETPATH response. */
    OBX_ACTION_CFM_SEVT,    /*15    API call to send an ACTION response. */
    OBX_ABORT_CFM_SEVT,     /*16 12 API call to send an ABORT response. */
    OBX_PORT_CLOSE_SEVT,    /*17 13 Transport is closed. */
    OBX_FCS_SET_SEVT,       /*18 14 Data flow enable */
    OBX_STATE_SEVT,         /*19 15 Change state. */
    OBX_TIMEOUT_SEVT,       /*20 16 Timeout has occurred. */
    OBX_BAD_REQ_SEVT,       /*21 17 Received a bad request from client. */
    OBX_TX_EMPTY_SEVT,      /*22 18 Transmit Queue Empty */
    OBX_MAX_SEVT
};
typedef UINT8 tOBX_SR_EVENT;

#define OBX_SEVT_DIFF_REQ_CFM       (OBX_CONNECT_CFM_SEVT - OBX_CONNECT_REQ_SEVT) /* the index difference between *REQ_SEVT and *CFM_SEVT */
#define OBX_SEVT_MAX_REQ            OBX_ACTION_REQ_SEVT /* last *REQ_SEVT */

enum
{
    OBX_SS_NULL,                /* 0   0 */
    OBX_SS_NOT_CONNECTED,       /* 1   1 */
    OBX_SS_SESS_INDICATED,      /* 2     */
    OBX_SS_CONN_INDICATED,      /* 3   2 */
    OBX_SS_WAIT_AUTH,           /* 4   3 */
    OBX_SS_AUTH_INDICATED,      /* 5   4 */
    OBX_SS_CONNECTED,           /* 6   5 */
    OBX_SS_DISCNT_INDICATED,    /* 7   6 */
    OBX_SS_SETPATH_INDICATED,   /* 8   7 */
    OBX_SS_ACTION_INDICATED,    /* 9    */
    OBX_SS_ABORT_INDICATED,     /*10   8 */
    OBX_SS_PUT_INDICATED,       /*11   9 */
    OBX_SS_GET_INDICATED,       /*12  10 */
    OBX_SS_PUT_TRANSACTION,     /*13  11 */
    OBX_SS_GET_TRANSACTION,     /*14  12 */
    OBX_SS_PUT_SRM,             /*15    */
    OBX_SS_GET_SRM,             /*16    */
    OBX_SS_PARTIAL_SENT,        /*17  13 */
    OBX_SS_WAIT_CLOSE,          /*18  14 */
    OBX_SS_MAX
};
typedef UINT8 tOBX_SR_STATE;

typedef UINT8 tOBX_STATE;   /* this must be the same type as tOBX_SR_STATE and tOBX_CL_STATE */

typedef struct
{
    UINT16  pkt_len;/* the packet length */
    UINT8   code;   /* the response/request code with the final bit */
    UINT8   sm_evt; /* The state machine event*/
} tOBX_RX_HDR;

typedef void (tOBX_CLOSE_FN) (UINT16);
typedef BOOLEAN (tOBX_SEND_FN) (void *p_cb);
typedef UINT8 (*tOBX_VERIFY_OPCODE)(UINT8 opcode, tOBX_RX_HDR *p_rxh);

#define OBX_SRM_NO          0x00    /* SRM is not enabled and not engaged */
#define OBX_SRM_ENABLE      0x01    /* SRM is enabled. */
#define OBX_SRM_PARAM_AL    0x02    /* SRMP is allowed only in the beginning of a transaction */
#define OBX_SRM_REQING      0x04    /* requesting/requested to enable SRM. */
#define OBX_SRM_ABORT       0x08    /* (server) abort/reject at SRM GET/PUT rsp API*/
#define OBX_SRM_ENGAGE      0x10    /* SRM is engaged. */
#define OBX_SRM_NEXT        0x20    /* peer is ready for next packet. */
#define OBX_SRM_WAIT        0x40    /* wait for peer. */
#define OBX_SRM_WAIT_UL     0x80    /* wait for upper layer. */
typedef UINT8 tOBX_SRM;

#define OBX_SRMP_WAIT       0x40    /* wait for peer  */
#define OBX_SRMP_NONF       0x80    /* handle GET non-final (used by server only) */
#define OBX_SRMP_NONF_EVT   0x20    /* report GET non-final req event (used by server only) */
#define OBX_SRMP_SESS_FST   0x01    /* mark the session resume. The SSN on first req might not match */
typedef UINT8 tOBX_SRMP;

/* continue to define tOBX_SESS_ST for internal use */
enum
{
    OBX_SESS_TIMEOUT = OBX_SESS_EXT_MAX, /* 0x03 session is requested/set timeout. */
    OBX_SESS_CREATE,                     /* 0x04 session is requested/create. */
    OBX_SESS_SUSPEND,                    /* 0x05 session is requested/suspend. */
    OBX_SESS_RESUME,                     /* 0x06 session is requested/resume. */
    OBX_SESS_CLOSE,                      /* 0x07 session is requested/close. */
    OBX_SESS_SUSPENDING                  /* 0x08 session is requested/suspend: server has not reported the suspend event */
};
#define OBX_SESS_DROP       (0x80|OBX_SESS_SUSPENDED)   /* the session as suspended by link drop */

/* port control block */
typedef struct
{
    tOBX_HANDLE     handle;             /* The handle of the client or session handle for server */
    UINT16          port_handle;        /* Port handle of connection       */
    UINT16          tx_mtu;             /* The MTU of the connected peer */
    UINT16          rx_mtu;             /* The MTU of this instance */
    TIMER_LIST_ENT  tle;                /* This session's Timer List Entry */
    tOBX_CLOSE_FN   *p_close_fn;        /* the close connection function */
    tOBX_SEND_FN    *p_send_fn;         /* the send message function */
    BT_HDR          *p_txmsg;           /* The message to send to peer */
    BUFFER_Q        rx_q;               /* received data buffer queue       */
    BOOLEAN         stopped;            /* TRUE, if flow control the peer (stop peer from sending more data). */
    BT_HDR          *p_rxmsg;           /* The message received from peer */
} tOBX_PORT_CB;

typedef struct
{
    tOBX_PORT_CB   *p_pcb;              /* the port control block */
    UINT32          code;               /* the event code from RFCOMM */
} tOBX_PORT_EVT;

/* l2cap control block */
typedef struct
{
    tOBX_HANDLE     handle;             /* The handle of the client or session handle for server */
    UINT16          lcid;               /* l2cap lcid of connection       */
    UINT16          tx_mtu;             /* The MTU of the connected peer */
    UINT16          rx_mtu;             /* The MTU of this instance */
    TIMER_LIST_ENT  tle;                /* This session's Timer List Entry */
    tOBX_CLOSE_FN   *p_close_fn;        /* the close connection function */
    tOBX_SEND_FN    *p_send_fn;         /* the send message function */
    BT_HDR          *p_txmsg;           /* The message to send to peer */
    BUFFER_Q        rx_q;               /* received data buffer queue       */
    BOOLEAN         stopped;            /* TRUE, if flow control the peer (stop peer from sending more data). */
    UINT8           ch_state;           /* L2CAP channel state */
    UINT8           ch_flags;           /* L2CAP configuration flags */
    BOOLEAN         cong;               /* TRUE, if L2CAP is congested. */
} tOBX_L2C_CB;

enum
{
    OBX_L2C_EVT_CONG,
    OBX_L2C_EVT_CLOSE,
    OBX_L2C_EVT_CONN_IND,
    OBX_L2C_EVT_DATA_IND,
    OBX_L2C_EVT_RESUME
};
typedef UINT8 tOBX_L2C_EVT;

typedef struct
{
    BD_ADDR         bd_addr;
    UINT16          lcid;
    UINT16          psm;
    UINT8           id;
} tOBX_L2C_IND;

typedef union
{
    tOBX_L2C_IND    conn_ind;
    BOOLEAN         is_cong;
    BT_HDR          *p_pkt;
    UINT8           any;
    BD_ADDR         remote_addr;
} tOBX_L2C_EVT_PARAM;

typedef struct
{
    tOBX_L2C_CB         *p_l2cb;        /* the L2CAP control block */
    tOBX_L2C_EVT_PARAM  param;
    tOBX_L2C_EVT        l2c_evt;        /* the event code from L2CAP */
} tOBX_L2C_EVT_MSG;

/* common of port & l2cap control block */
typedef struct
{
    tOBX_HANDLE     handle;             /* The handle of the client or session handle for server */
    UINT16          id;                 /* l2cap lcid or port handle     */
    UINT16          tx_mtu;             /* The MTU of the connected peer */
    UINT16          rx_mtu;             /* The MTU of this instance */
    TIMER_LIST_ENT  tle;                /* This session's Timer List Entry */
    tOBX_CLOSE_FN   *p_close_fn;        /* the close connection function */
    tOBX_SEND_FN    *p_send_fn;         /* the send message function */
    BT_HDR          *p_txmsg;           /* The message to send to peer */
    BUFFER_Q        rx_q;               /* received data buffer queue       */
    BOOLEAN         stopped;            /* TRUE, if flow control the peer (stop peer from sending more data). */
} tOBX_COMM_CB;

/* lower layer control block */
typedef union
{
    tOBX_PORT_CB    port;
    tOBX_L2C_CB     l2c;
    tOBX_COMM_CB    comm;
} tOBX_LL_CB;

/* client control block */
typedef struct
{
    tOBX_CL_CBACK   *p_cback;           /* Application callback function to receive events */
    BT_HDR          *p_next_req;        /* This is used when the session is flow controlled by peer
                                         * and a DISCONNECT or ABORT request is sent.*/
    BT_HDR          *p_saved_req;       /* Client saves a copy of the request sent to the server
                                         * Just in case the operation is challenged by the server */
    BT_HDR          *p_auth;            /* The request-digest string, if challenging the server.
                                         * The received OBEX packet, if waiting for password */
    tOBX_LL_CB      ll_cb;              /* lower layer control block for this client */
    UINT32          conn_id;            /* Connection ID for this connection */
    UINT32          nonce;              /* This is converted to UINT8[16] internally before adding to the OBEX header. This value is copied to the server control block and is increased after each use. 0, if only legacy OBEX (unreliable) session is desired. */
    BD_ADDR         peer_addr;          /* peer address */
    tOBX_EVT_PARAM  param;              /* The event parameter. */
    UINT8           sess_info[OBX_SESSION_INFO_SIZE]; /* session id + local nonce */
    tOBX_EVENT      api_evt;            /* Set the API event, if need to notify user outside of action function. */
    tOBX_CL_STATE   state;              /* The current state */
    tOBX_STATE      next_state;         /* Use by PART state to return to regular states */
    tOBX_CL_STATE   prev_state;         /* The previous state */
    UINT16          psm;                /* L2CAP virtual psm */
    tOBX_SRM        srm;                /* Single Response Mode  */
    tOBX_SRMP       srmp;               /* Single Response Mode Parameter */
    UINT8           ssn;                /* Session Sequence number */
    tOBX_SESS_ST    sess_st;            /* Session state */
    UINT8           rsp_code;           /* The response code of the response packet */
    BOOLEAN         final;              /* The final bit status of last request */
    BOOLEAN         wait_auth;          /* TRUE, if challenges the server and is waiting for the response */
} tOBX_CL_CB;

/* Authentication Control block */
typedef struct
{
    UINT32          nonce;              /* Timestamp used as nonce */
    tOBX_AUTH_OPT   auth_option;        /* Authentication option */
    UINT8           realm_len;          /* The length of p_realm */
    UINT8           realm[1];           /* The realm of this server. Charset is the first byte */
} tOBX_AUTH_PARAMS;

/* suspend control block */
typedef struct
{
    TIMER_LIST_ENT  stle;               /* The timer for suspended session timeout */
    BD_ADDR         peer_addr;          /* peer address */
    UINT8           sess_info[OBX_SESSION_INFO_SIZE]; /* session id + local nonce + conn id + state + srm */
    UINT32          offset;             /* the file offset */
    tOBX_SR_STATE   state;              /* The current state */
    UINT8           ssn;                /* the last ssn */
} tOBX_SPND_CB;

/* server control block */
typedef struct
{
    tOBX_SR_CBACK   *p_cback;           /* Application callback function to receive events */
    tOBX_TARGET     target;             /* target header of this server */
    tOBX_AUTH_PARAMS *p_auth;           /* A GKI buffer that holds the authentication related parameters */
    UINT8           sess[OBX_MAX_SR_SESSION]; /* index + 1 of sr_sess[]. 0, if not used. */
    tOBX_SPND_CB    *p_suspend;         /* the information for suspended sessions (GKI buffer) */
    UINT32          nonce;              /* This is converted to UINT8[16] internally before adding to the OBEX header. This value is copied to the server control block and is increased after each use. 0, if only legacy OBEX (unreliable) session is desired. */
    UINT16          psm;                /* PSM for this server         */
    UINT8           max_suspend;        /* the max number of tOBX_SPD_CB[] in p_suspend. must be less than OBX_MAX_SUSPEND_SESSIONS */
    UINT8           scn;                /* SCN for this server         */
    UINT8           num_sess;           /* Max number of session control blocks used by this server*/
} tOBX_SR_CB;

/* server session control block */
typedef struct
{
    BT_HDR          *p_saved_msg;       /* This is a message saved for authentication or GetReq-non-final */
    BT_HDR          *p_next_req;        /* This is a message saved for flow control reasons */
    tOBX_LL_CB      ll_cb;              /* lower layer control block for this session */
    UINT32          conn_id;            /* Connection ID for this connection */
    BD_ADDR         peer_addr;          /* peer address */
    UINT8           sess_info[OBX_SESSION_INFO_SIZE]; /* session id + local nonce */
    tOBX_SR_STATE   state;              /* The current state */
    tOBX_SR_STATE   prev_state;         /* The previous state */
    tOBX_STATE      next_state;         /* Use by PART state to return to regular states */
    tOBX_HANDLE     handle;             /* The obx handle for server */
    tOBX_EVENT      api_evt;            /* The event to notify the API user. */
    tOBX_EVT_PARAM  param;              /* The event parameter. */
    tOBX_SRMP       srmp;               /* Single Response Mode Parameter */
    tOBX_SRM        srm;                /* Single Response Mode  */
    tOBX_SESS_ST    sess_st;            /* Session state */
    UINT8           ssn;                /* Next Session Sequence number */
    UINT8           cur_op;             /* The op code for the current transaction (keep this for Abort reasons) */
                                        /* set to the current OP (non-abort) in rfc.c, set it to abort when a response is sent */
} tOBX_SR_SESS_CB;

typedef struct
{
#if (OBX_SERVER_INCLUDED == TRUE)
    tOBX_SR_CB      server[OBX_NUM_SERVERS];/* The server control blocks */
    UINT32          next_cid;               /* Next OBEX connection ID for server */
    tOBX_SR_SESS_CB sr_sess[OBX_NUM_SR_SESSIONS];
    tOBX_L2C_CB     sr_l2cb;                /* for obx_l2c_connect_ind_cback */
#endif

#if (OBX_CLIENT_INCLUDED == TRUE)
    tOBX_CL_CB      client[OBX_NUM_CLIENTS];/* The client control blocks */
    tOBX_PORT_CB   *p_temp_pcb;         /* Valid only during client RFCOMM_CreateConnection call */
    UINT8           next_ind;           /* The index to the next client control block */
#endif

    tOBX_HANDLE     hdl_map[MAX_RFC_PORTS]; /* index of this array is the port_handle,
                                         * the value is the OBX handle */
    UINT16          l2c_map[MAX_L2CAP_CHANNELS]; /* index of this array is (lcid - L2CAP_BASE_APPL_CID) */
    UINT32          timeout_val;        /* The timeout value to wait for activity from peer */
    UINT32          sess_tout_val;      /* The timeout value for reliable sessions to remain in suspend */
    UINT8           trace_level;        /* The default trace level */
    UINT8           num_client;         /* Number of client control blocks */
    UINT8           num_server;         /* Number of server control blocks */
    UINT8           num_sr_sess;        /* Number of server session control blocks */
    UINT8           max_rx_qcount;      /* Max Number of rx_q count */
} tOBX_CB;

/*****************************************************************************
**  Definition for State Machine
*****************************************************************************/

/* Client Action functions are of this type */
typedef tOBX_CL_STATE (*tOBX_CL_ACT)(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);

/* Server Action functions are of this type */
typedef tOBX_SR_STATE (*tOBX_SR_ACT)(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);

#define OBX_SM_IGNORE           0
#define OBX_SM_NO_ACTION        0xFF
#define OBX_SM_ALL              0x80 /* for events like close confirm, abort request, close request where event handling is the same for most of the states */
#define OBX_SM_ENTRY_MASK       0x7F /* state machine entry mask */

#define OBX_SME_ACTION          0
#define OBX_SME_NEXT_STATE      1
#define OBX_SM_NUM_COLS         2

typedef const UINT8 (*tOBX_SM_TBL)[OBX_SM_NUM_COLS];

/*****************************************************************************
**  External global data
*****************************************************************************/
#if OBX_DYNAMIC_MEMORY == FALSE
OBX_API extern tOBX_CB  obx_cb;
#else
OBX_API extern tOBX_CB *obx_cb_ptr;
#define obx_cb (*obx_cb_ptr)
#endif
extern const tOBX_EVENT obx_sm_evt_to_api_evt[];
extern const UINT8 obx_hdr_start_offset[];
/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
/* from obx_main.c */

#if (defined (BT_USE_TRACES) && BT_USE_TRACES == TRUE && OBX_CLIENT_INCLUDED == TRUE)
extern const char * obx_cl_get_state_name(tOBX_CL_STATE state);
extern const char * obx_cl_get_event_name(tOBX_CL_EVENT event);
#else
#define obx_cl_get_state_name(state_num)    ""
#define obx_cl_get_event_name(event_num)    ""
#endif

#if (defined (BT_USE_TRACES) && BT_USE_TRACES == TRUE && OBX_SERVER_INCLUDED == TRUE)
extern const char * obx_sr_get_state_name(tOBX_SR_STATE state);
extern const char * obx_sr_get_event_name(tOBX_SR_EVENT event);
#else
#define obx_sr_get_state_name(state_num)    ""
#define obx_sr_get_event_name(event_num)    ""
#endif
/* client functions in obx_main.c */
extern void obx_cl_timeout(TIMER_LIST_ENT *p_tle);
extern tOBX_CL_CB * obx_cl_alloc_cb(void);
extern tOBX_CL_CB * obx_cl_get_cb(tOBX_HANDLE handle);
extern tOBX_CL_CB * obx_cl_get_suspended_cb(tOBX_HANDLE *p_handle, UINT8 *p_session_info);
extern void obx_cl_free_cb(tOBX_CL_CB * p_cb);

/* server functions in obx_main.c */
extern tOBX_SPND_CB * obx_find_suspended_session (tOBX_SR_SESS_CB *p_scb, tOBX_TRIPLET *p_triplet, UINT8 num);
extern void obx_l2c_snd_evt (tOBX_L2C_CB *p_l2cb, tOBX_L2C_EVT_PARAM  param, tOBX_L2C_EVT l2c_evt);
extern void obx_sr_timeout(TIMER_LIST_ENT *p_tle);
extern void obx_sr_sess_timeout(TIMER_LIST_ENT *p_tle);
extern tOBX_HANDLE obx_sr_alloc_cb(tOBX_StartParams *p_params);
extern tOBX_SR_CB * obx_sr_get_cb(tOBX_HANDLE handle);
extern tOBX_SR_SESS_CB * obx_sr_get_scb(tOBX_HANDLE handle);
extern void obx_sr_free_cb(tOBX_HANDLE handle);
extern void obx_sr_free_scb(tOBX_SR_SESS_CB *p_scb);
extern UINT32 obx_sr_get_next_conn_id(void);
/* common functions in obx_main.c */
extern tOBX_PORT_CB * obx_port_handle_2cb(UINT16 port_handle);
extern void obx_start_timer(tOBX_COMM_CB *p_pcb);
extern void obx_stop_timer(TIMER_LIST_ENT *p_tle);

/* from obx_rfc.c */
extern void obx_cl_proc_evt(tOBX_PORT_EVT *p_evt);
extern BT_HDR * obx_build_dummy_rsp(tOBX_SR_SESS_CB *p_scb, UINT8 rsp_code);
extern void obx_sr_proc_evt(tOBX_PORT_EVT *p_evt);
extern BOOLEAN obx_rfc_snd_msg(tOBX_PORT_CB *p_pcb);
extern tOBX_STATUS obx_open_port(tOBX_PORT_CB *p_pcb, const BD_ADDR bd_addr, UINT8 scn);
extern void obx_add_port(tOBX_HANDLE obx_handle);
extern void obx_close_port(UINT16 port_handle);

/* from obx_capi.c */
extern tOBX_STATUS obx_prepend_req_msg(tOBX_HANDLE handle, tOBX_CL_EVENT event, UINT8 req_code, BT_HDR *p_pkt);

/* from obx_csm.c */
extern void obx_csm_event(tOBX_CL_CB *p_cb, tOBX_EVENT event, BT_HDR *p_msg);

/* from obx_cact.c */
extern void obx_ca_connect_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_snd_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_close_port(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_snd_part(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_connect_error(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_connect_fail(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_discnt_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_fail_rsp(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_state(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_start_timer(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_connect_ok(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_session_ok(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_session_cont(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_session_get(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_session_fail(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_abort(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_snd_put_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_snd_get_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_srm_snd_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_srm_put_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_srm_get_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_srm_put_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_srm_get_notify(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_save_rsp(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern tOBX_CL_STATE obx_ca_save_req(tOBX_CL_CB *p_cb, BT_HDR *p_pkt);

/* from obx_ssm.c */
extern void obx_ssm_event(tOBX_SR_SESS_CB *p_scb, tOBX_SR_EVENT event, BT_HDR *p_msg);

/* from obx_sapi.c */
extern tOBX_STATUS obx_prepend_rsp_msg(tOBX_HANDLE shandle, tOBX_SR_EVENT event, UINT8 rsp_code, BT_HDR *p_pkt);

/* from obx_sact.c */
extern tOBX_SR_STATE obx_sa_snd_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_snd_part(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_abort_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_op_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern BT_HDR * obx_conn_rsp(tOBX_SR_CB *p_cb, tOBX_SR_SESS_CB *p_scb, UINT8 rsp_code, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_connect_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_wc_conn_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_auth_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_connect_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_connection_error(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_close_port(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_clean_port(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_state(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_nc_to(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_save_req(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_rej_req(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_session_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_sess_conn_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_wc_sess_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_session_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_put_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_get_ind(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_get_req(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_srm_put_req(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_srm_put_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_srm_get_fcs(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_srm_get_rsp(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern tOBX_SR_STATE obx_sa_srm_get_req(tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);

/* from obx_gen.c */
extern void obx_access_rsp_code(BT_HDR *p_pkt, UINT8 *p_rsp_code);
extern void obx_adjust_packet_len(BT_HDR *p_pkt);
extern UINT16 obx_read_header_len(UINT8 *ph);
extern BT_HDR * obx_dup_pkt(BT_HDR *p_pkt);

/* from obx_md5 */
extern BT_HDR * obx_unauthorize_rsp(tOBX_SR_CB *p_cb, tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern void obx_session_id(UINT8 *p_sess_id, UINT8 *p_cl_addr, UINT8 * p_cl_nonce, int cl_nonce_len,
                    UINT8 *p_sr_addr, UINT8 * p_sr_nonce, int sr_nonce_len);

/* from obx_l2c.c */
extern void obx_register_l2c(tOBX_CL_CB *p_cl_cb, UINT16 psm);
extern tOBX_STATUS obx_open_l2c(tOBX_CL_CB *p_cl_cb, const BD_ADDR bd_addr);
extern tOBX_STATUS obx_l2c_sr_register (tOBX_SR_CB  *p_cb);
extern void obx_close_l2c(UINT16 lcid);
extern BOOLEAN obx_l2c_snd_msg(tOBX_L2C_CB *p_l2cb);
extern void obx_sr_proc_l2c_evt (tOBX_L2C_EVT_MSG *p_msg);
extern void obx_cl_proc_l2c_evt (tOBX_L2C_EVT_MSG *p_msg);

/* from obx_utils.c */
extern UINT8 obx_verify_response(UINT8 opcode, tOBX_RX_HDR *p_rxh);
extern UINT8 obx_verify_request(UINT8 opcode, tOBX_RX_HDR *p_rxh);
extern BOOLEAN obx_sr_proc_pkt (tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt);
extern void obx_cl_proc_pkt (tOBX_CL_CB *p_cb, BT_HDR *p_pkt);
extern void obx_free_buf(tOBX_LL_CB *p_ll_cb);
extern UINT8 obx_add_timeout (tOBX_TRIPLET *p_trip, UINT32 timeout, tOBX_SESS_EVT *p_param);
extern void obx_read_timeout (tOBX_TRIPLET *p_trip, UINT8 num, UINT32 *p_timeout, UINT8 *p_toa);
#if (BT_USE_TRACES == TRUE)
extern void obxu_dump_hex (UINT8 *p, char *p_title, UINT16 len);
#else
#define obxu_dump_hex(p, p_title, len)
#endif
extern BT_HDR * obx_sr_prepend_msg(BT_HDR *p_pkt, UINT8 * p_data, UINT16 data_len);
extern BT_HDR * obx_cl_prepend_msg(tOBX_CL_CB *p_cb, BT_HDR *p_pkt, UINT8 * p_data, UINT16 data_len);
extern void obx_flow_control(tOBX_COMM_CB *p_comm);
extern UINT8 obx_read_triplet(tOBX_TRIPLET *p_trip, UINT8 num_trip, UINT8 tag);
extern UINT32 obx_read_obj_offset(tOBX_TRIPLET *p_trip, UINT8 num_trip);

#endif /* OBX_INT_H */
