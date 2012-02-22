/*****************************************************************************
**
**  Name:           bta_pbs_main.c
**
**  Description:    This file contains the phone book access server main functions 
**                  and state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include <string.h>
#include "bta_fs_api.h"
#include "bta_pbs_api.h"
#include "bta_pbs_int.h"
#include "gki.h"
#include "utl.h"
#include "obx_api.h"
#include "rfcdefs.h"    /* BT_PSM_RFCOMM */
#include "bta_fs_co.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/

/* state machine states */
enum
{
  BTA_PBS_IDLE_ST = 0,      /* Idle  */
  BTA_PBS_LISTEN_ST,        /* Listen - waiting for OBX/RFC connection */
  BTA_PBS_W4_AUTH_ST,       /* Wait for Authentication -  (optional) */
  BTA_PBS_CONN_ST,          /* Connected - PBS Session is active */
  BTA_PBS_CLOSING_ST        /* Closing is in progress */
};

/* state machine action enumeration list */
enum
{
    BTA_PBS_API_DISABLE,
    BTA_PBS_API_AUTHRSP,
    BTA_PBS_API_ACCESSRSP,
    BTA_PBS_API_CLOSE,
    BTA_PBS_CI_READ,
    BTA_PBS_CI_OPEN,
    BTA_PBS_CI_VLIST,
    BTA_PBS_OBX_CONNECT,
    BTA_PBS_OBX_DISC,
    BTA_PBS_OBX_CLOSE,
    BTA_PBS_OBX_ABORT,
    BTA_PBS_OBX_PASSWORD,
    BTA_PBS_OBX_GET,
    BTA_PBS_OBX_SETPATH,
    BTA_PBS_APPL_TOUT,
    BTA_PBS_CONN_ERR_RSP,
    BTA_PBS_DISC_ERR_RSP,
    BTA_PBS_GASP_ERR_RSP,
    BTA_PBS_CLOSE_COMPLETE,
    BTA_PBS_IGNORE
};

/* type for action functions */
typedef void (*tBTA_PBS_ACTION)(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data);

/* action function list */
const tBTA_PBS_ACTION bta_pbs_action[] =
{
    bta_pbs_api_disable,
    bta_pbs_api_authrsp,
    bta_pbs_api_accessrsp,
    bta_pbs_api_close,
    bta_pbs_ci_read_act,
    bta_pbs_ci_open_act,
    bta_pbs_ci_vlist_act,
    bta_pbs_obx_connect,
    bta_pbs_obx_disc,
    bta_pbs_obx_close,
    bta_pbs_obx_abort,
    bta_pbs_obx_password,
    bta_pbs_obx_get,
    bta_pbs_obx_setpath,
    bta_pbs_appl_tout,
    bta_pbs_conn_err_rsp,
    bta_pbs_disc_err_rsp,
    bta_pbs_gasp_err_rsp,
    bta_pbs_close_complete
};


/* state table information */
#define BTA_PBS_ACTIONS             1       /* number of actions */
#define BTA_PBS_NEXT_STATE          1       /* position of next state */
#define BTA_PBS_NUM_COLS            2       /* number of columns in state tables */

/* state table for idle state */
static const UINT8 bta_pbs_st_idle[][BTA_PBS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_PBS_API_ENABLE_EVT    */   {BTA_PBS_IGNORE,          BTA_PBS_LISTEN_ST}
};

/* state table for obex/rfcomm connection state */
static const UINT8 bta_pbs_st_listen[][BTA_PBS_NUM_COLS] =
{
/* Event                          Action 1               Next state */
/* BTA_PBS_API_DISABLE_EVT   */   {BTA_PBS_API_DISABLE,  BTA_PBS_IDLE_ST},
/* BTA_PBS_API_AUTHRSP_EVT   */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_API_ACCESSRSP_EVT */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_API_CLOSE_EVT     */   {BTA_PBS_API_CLOSE,    BTA_PBS_LISTEN_ST},
/* BTA_PBS_CI_READ_EVT       */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_CI_OPEN_EVT       */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_CI_VLIST_EVT      */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_CONN_EVT      */   {BTA_PBS_OBX_CONNECT,  BTA_PBS_CONN_ST},
/* BTA_PBS_OBX_DISC_EVT      */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_ABORT_EVT     */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_PASSWORD_EVT  */   {BTA_PBS_OBX_PASSWORD, BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_CLOSE_EVT     */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_GET_EVT       */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_SETPATH_EVT   */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_APPL_TOUT_EVT     */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_DISC_ERR_EVT      */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_GASP_ERR_EVT      */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_CLOSE_CMPL_EVT    */   {BTA_PBS_IGNORE,       BTA_PBS_LISTEN_ST}
};

/* state table for wait for authentication response state */
static const UINT8 bta_pbs_st_w4_auth[][BTA_PBS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_PBS_API_DISABLE_EVT   */   {BTA_PBS_API_DISABLE,     BTA_PBS_IDLE_ST},
/* BTA_PBS_API_AUTHRSP_EVT   */   {BTA_PBS_API_AUTHRSP,     BTA_PBS_LISTEN_ST},
/* BTA_PBS_API_ACCESSRSP_EVT */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_API_CLOSE_EVT     */   {BTA_PBS_API_CLOSE,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_CI_READ_EVT       */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_CI_OPEN_EVT       */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_CI_VLIST_EVT      */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_CONN_EVT      */   {BTA_PBS_CONN_ERR_RSP,    BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_DISC_EVT      */   {BTA_PBS_OBX_DISC,        BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_ABORT_EVT     */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_PASSWORD_EVT  */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_CLOSE_EVT     */   {BTA_PBS_OBX_CLOSE,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_GET_EVT       */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_SETPATH_EVT   */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_APPL_TOUT_EVT     */   {BTA_PBS_APPL_TOUT,       BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_DISC_ERR_EVT      */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_GASP_ERR_EVT      */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_CLOSE_CMPL_EVT    */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST}
};

/* state table for open state */
static const UINT8 bta_pbs_st_connected[][BTA_PBS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_PBS_API_DISABLE_EVT   */   {BTA_PBS_API_DISABLE,     BTA_PBS_IDLE_ST},
/* BTA_PBS_API_AUTHRSP_EVT   */   {BTA_PBS_IGNORE,          BTA_PBS_CONN_ST},
/* BTA_PBS_API_ACCESSRSP_EVT */   {BTA_PBS_API_ACCESSRSP,   BTA_PBS_CONN_ST},
/* BTA_PBS_API_CLOSE_EVT     */   {BTA_PBS_API_CLOSE,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_CI_READ_EVT       */   {BTA_PBS_CI_READ,         BTA_PBS_CONN_ST},
/* BTA_PBS_CI_OPEN_EVT       */   {BTA_PBS_CI_OPEN,         BTA_PBS_CONN_ST},
/* BTA_PBS_CI_VLIST_EVT       */  {BTA_PBS_CI_VLIST,        BTA_PBS_CONN_ST},
/* BTA_PBS_OBX_CONN_EVT      */   {BTA_PBS_CONN_ERR_RSP,    BTA_PBS_CONN_ST},
/* BTA_PBS_OBX_DISC_EVT      */   {BTA_PBS_OBX_DISC,        BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_ABORT_EVT     */   {BTA_PBS_OBX_ABORT,       BTA_PBS_CONN_ST},
/* BTA_PBS_OBX_PASSWORD_EVT  */   {BTA_PBS_IGNORE,          BTA_PBS_W4_AUTH_ST},
/* BTA_PBS_OBX_CLOSE_EVT     */   {BTA_PBS_OBX_CLOSE,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_GET_EVT       */   {BTA_PBS_OBX_GET,         BTA_PBS_CONN_ST},
/* BTA_PBS_OBX_SETPATH_EVT   */   {BTA_PBS_OBX_SETPATH,     BTA_PBS_CONN_ST},
/* BTA_PBS_APPL_TOUT_EVT     */   {BTA_PBS_APPL_TOUT,       BTA_PBS_LISTEN_ST},
/* BTA_PBS_DISC_ERR_EVT      */   {BTA_PBS_IGNORE,          BTA_PBS_CONN_ST},
/* BTA_PBS_GASP_ERR_EVT      */   {BTA_PBS_GASP_ERR_RSP,    BTA_PBS_CONN_ST},
/* BTA_PBS_CLOSE_CMPL_EVT    */   {BTA_PBS_IGNORE,          BTA_PBS_CONN_ST}
};

/* state table for closing state */
static const UINT8 bta_pbs_st_closing[][BTA_PBS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_PBS_API_DISABLE_EVT   */   {BTA_PBS_API_DISABLE,     BTA_PBS_IDLE_ST},
/* BTA_PBS_API_AUTHRSP_EVT   */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_API_ACCESSRSP_EVT */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_API_CLOSE_EVT     */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_CI_READ_EVT       */   {BTA_PBS_CLOSE_COMPLETE,  BTA_PBS_LISTEN_ST},
/* BTA_PBS_CI_OPEN_EVT       */   {BTA_PBS_CLOSE_COMPLETE,  BTA_PBS_LISTEN_ST},
/* BTA_PBS_CI_VLIST_EVT      */   {BTA_PBS_CLOSE_COMPLETE,  BTA_PBS_LISTEN_ST},
/* BTA_PBS_OBX_CONN_EVT      */   {BTA_PBS_CONN_ERR_RSP,    BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_DISC_EVT      */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_ABORT_EVT     */   {BTA_PBS_OBX_ABORT,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_PASSWORD_EVT  */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_CLOSE_EVT     */   {BTA_PBS_OBX_CLOSE,       BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_GET_EVT       */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_OBX_SETPATH_EVT   */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_APPL_TOUT_EVT     */   {BTA_PBS_IGNORE,          BTA_PBS_CLOSING_ST},
/* BTA_PBS_DISC_ERR_EVT      */   {BTA_PBS_DISC_ERR_RSP,    BTA_PBS_CONN_ST},
/* BTA_PBS_GASP_ERR_EVT      */   {BTA_PBS_GASP_ERR_RSP,    BTA_PBS_CLOSING_ST},
/* BTA_PBS_CLOSE_CMPL_EVT    */   {BTA_PBS_CLOSE_COMPLETE,  BTA_PBS_LISTEN_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_PBS_ST_TBL)[BTA_PBS_NUM_COLS];

/* state table */
const tBTA_PBS_ST_TBL bta_pbs_st_tbl[] =
{
    bta_pbs_st_idle,
    bta_pbs_st_listen,
    bta_pbs_st_w4_auth,
    bta_pbs_st_connected,
    bta_pbs_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* PBS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_PBS_CB  bta_pbs_cb;
#endif

#if BTA_PBS_DEBUG == TRUE
static char *pbs_evt_code(tBTA_PBS_INT_EVT evt_code);
static char *pbs_state_code(tBTA_PBS_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_pbs_sm_execute
**
** Description      State machine event handling function for PBS
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_pbs_sm_execute(tBTA_PBS_CB *p_cb, UINT16 event, tBTA_PBS_DATA *p_data)
{
    tBTA_PBS_ST_TBL     state_table;
    UINT8               action;
    int                 i;
    tBTA_PBS_OBX_EVENT  *p_obex_event;
#if BTA_PBS_DEBUG == TRUE
    tBTA_PBS_STATE in_state = bta_pbs_cb.state;
    UINT16         in_event = event;
    APPL_TRACE_EVENT3("PBS Event        : State 0x%02x [%s], Event [%s]", in_state,
                      pbs_state_code(in_state),
                      pbs_evt_code(event));
#endif

    /* look up the state table for the current state */
    state_table = bta_pbs_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_PBS_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_PBS_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_PBS_IGNORE)
        {
            (*bta_pbs_action[action])(p_cb, p_data);
        }
        else
        {
            if (event >= BTA_PBS_OBX_CONN_EVT && event <= BTA_PBS_OBX_SETPATH_EVT) {
                p_obex_event = (tBTA_PBS_OBX_EVENT *) p_data;
                utl_freebuf((void**)&(p_obex_event->p_pkt));
            }
            break;
        }
    }

#if BTA_PBS_DEBUG == TRUE
    if (in_state != bta_pbs_cb.state)
    {
        APPL_TRACE_DEBUG3("PBS State Change: [%s] -> [%s] after Event [%s]",
                      pbs_state_code(in_state),
                      pbs_state_code(bta_pbs_cb.state),
                      pbs_evt_code(in_event));
    }
#endif
}

/*******************************************************************************
**
** Function         bta_pbs_api_enable
**
** Description      Handle an api enable event.  This function enables the PBS
**                  Server by opening an Obex/Rfcomm channel and placing it into
**                  listen mode.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_pbs_api_enable(tBTA_PBS_CB *p_cb, tBTA_PBS_DATA *p_data)
{
    tOBX_StartParams     start_msg;
    tBTA_PBS_API_ENABLE *p_api = &p_data->api_enable;
    tOBX_TARGET          target;
    UINT16               len;
    tOBX_STATUS          status;
    tBTA_UTL_COD         cod;
    
    /* initialize control block */
    memset(p_cb, 0, sizeof(tBTA_PBS_CB));
    
    /* Allocate an aligned memory buffer to hold the root path and working directory */
    /* Add 1 byte for '\0' */
    len = p_bta_fs_cfg->max_path_len + 1;
    if ((p_cb->p_rootpath = (char *)GKI_getbuf((UINT16)(len * 2))) != NULL)
    {
        p_cb->p_workdir = p_cb->p_rootpath + len;
        memcpy(target.target, BTA_PBS_TARGET_UUID, BTA_PBS_UUID_LENGTH);
        target.len = BTA_PBS_UUID_LENGTH;
        
        /* store parameters */
        p_cb->app_id = p_api->app_id;
        p_cb->p_cback = p_api->p_cback;
        p_cb->scn = BTM_AllocateSCN();
        p_cb->auth_enabled = p_api->auth_enabled;
        p_cb->fd = BTA_FS_INVALID_FD;
        p_cb->realm_len = p_api->realm_len;
        memcpy(p_cb->realm, p_api->realm, p_api->realm_len);
        
        /* Initialize the current working directory to be the root directory */
        BCM_STRNCPY_S(p_cb->p_rootpath, len, p_api->p_root_path, p_bta_fs_cfg->max_path_len);
        p_cb->p_rootpath[len-1] = '\0';
        BCM_STRNCPY_S(p_cb->p_workdir, len, p_api->p_root_path, p_bta_fs_cfg->max_path_len);
        p_cb->p_workdir[len-1] = '\0';
        
        /* Register PBS security requirements with BTM */
        BTM_SetSecurityLevel(FALSE, p_api->servicename, BTM_SEC_SERVICE_PBAP,
            p_api->sec_mask, BT_PSM_RFCOMM,
            BTM_SEC_PROTO_RFCOMM, (UINT32)p_cb->scn);
        
        /* Start up the PBS service */
        memset (&start_msg, 0, sizeof(tOBX_StartParams));
        start_msg.p_target = &target;

        /* Make the MTU fit into one RFC frame */
        start_msg.mtu = OBX_MAX_MTU;
        start_msg.scn = p_cb->scn;
        start_msg.authenticate = p_cb->auth_enabled;

        start_msg.auth_option = (p_bta_pbs_cfg->userid_req) ? OBX_AO_USR_ID : OBX_AO_NONE;
        start_msg.p_cback = bta_pbs_obx_cback;
        
        start_msg.realm_len = p_api->realm_len;
        start_msg.p_realm = p_api->realm;
        start_msg.realm_charset = (tOBX_CHARSET) p_bta_pbs_cfg->realm_charset;
 
        if ((status = OBX_StartServer (&start_msg, &p_cb->obx_handle)) == OBX_SUCCESS)
        {
            p_cb->state = BTA_PBS_LISTEN_ST;
            
            /* Set the File Transfer service class bit */
            cod.service = BTM_COD_SERVICE_OBJ_TRANSFER;
            utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);
            
            /* Set up the SDP record for pbs service */
            bta_pbs_sdp_register(p_cb, p_api->servicename);
        }
        else
            APPL_TRACE_ERROR1("OBX_StartServer returns error (%d)", status);
    }
    else    /* Cannot allocate resources to run Server */
        APPL_TRACE_ERROR0("Not enough Resources to run PBS Server");

    p_cb->p_cback(BTA_PBS_ENABLE_EVT, 0);
}

/*******************************************************************************
**
** Function         bta_pbs_hdl_event
**
** Description      File transfer server main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_pbs_hdl_event(BT_HDR *p_msg)
{
#if BTA_PBS_DEBUG == TRUE
    tBTA_PBS_STATE in_state = bta_pbs_cb.state;
#endif

    switch (p_msg->event)
    {
        case BTA_PBS_API_ENABLE_EVT:
#if BTA_PBS_DEBUG == TRUE
            APPL_TRACE_EVENT3("PBS Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                              pbs_state_code(in_state),
                              pbs_evt_code(p_msg->event));
#endif
            bta_pbs_api_enable(&bta_pbs_cb, (tBTA_PBS_DATA *) p_msg);

#if BTA_PBS_DEBUG == TRUE
            if (in_state != bta_pbs_cb.state)
            {
                APPL_TRACE_DEBUG3("PBS State Change: [%s] -> [%s] after Event [%s]",
                              pbs_state_code(in_state),
                              pbs_state_code(bta_pbs_cb.state),
                              pbs_evt_code(p_msg->event));
            }
#endif
            break;

        default:
            bta_pbs_sm_execute(&bta_pbs_cb, p_msg->event, (tBTA_PBS_DATA *) p_msg);
            break;
    }

    return (TRUE);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_PBS_DEBUG == TRUE

/*******************************************************************************
**
** Function         pbs_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *pbs_evt_code(tBTA_PBS_INT_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_PBS_API_DISABLE_EVT:
        return "BTA_PBS_API_DISABLE_EVT";
    case BTA_PBS_API_AUTHRSP_EVT:
        return "BTA_PBS_API_AUTHRSP_EVT";
    case BTA_PBS_API_ACCESSRSP_EVT:
        return "BTA_PBS_API_ACCESSRSP_EVT";
    case BTA_PBS_API_CLOSE_EVT:
        return "BTA_PBS_API_CLOSE_EVT";
    case BTA_PBS_CI_READ_EVT:
        return "BTA_PBS_CI_READ_EVT";
    case BTA_PBS_CI_OPEN_EVT:
        return "BTA_PBS_CI_OPEN_EVT";
    case BTA_PBS_CI_VLIST_EVT:
        return "BTA_PBS_CI_VLIST_EVT";
    case BTA_PBS_OBX_CONN_EVT:
        return "BTA_PBS_OBX_CONN_EVT";
    case BTA_PBS_OBX_DISC_EVT:
        return "BTA_PBS_OBX_DISC_EVT";
    case BTA_PBS_OBX_ABORT_EVT:
        return "BTA_PBS_OBX_ABORT_EVT";
    case BTA_PBS_OBX_PASSWORD_EVT:
        return "BTA_PBS_OBX_PASSWORD_EVT";
    case BTA_PBS_OBX_CLOSE_EVT:
        return "BTA_PBS_OBX_CLOSE_EVT";
    case BTA_PBS_OBX_GET_EVT:
        return "BTA_PBS_OBX_GET_EVT";
    case BTA_PBS_OBX_SETPATH_EVT:
        return "BTA_PBS_OBX_SETPATH_EVT";
    case BTA_PBS_APPL_TOUT_EVT:
        return "BTA_PBS_APPL_TOUT_EVT";
    case BTA_PBS_DISC_ERR_EVT:
        return "BTA_PBS_DISC_ERR_EVT";
    case BTA_PBS_GASP_ERR_EVT:
        return "BTA_PBS_GASP_ERR_EVT";
    case BTA_PBS_API_ENABLE_EVT:
        return "BTA_PBS_API_ENABLE_EVT";
    case BTA_PBS_CLOSE_CMPL_EVT:
        return "BTA_PBS_CLOSE_CMPL_EVT";
    default:
        APPL_TRACE_EVENT1("unknown PBS Event: %d", evt_code)
        return "unknown PBS event code";
    }
}

/*******************************************************************************
**
** Function         pbs_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *pbs_state_code(tBTA_PBS_STATE state_code)
{
    switch(state_code)
    {
    case BTA_PBS_IDLE_ST:
        return "BTA_PBS_IDLE_ST";
    case BTA_PBS_LISTEN_ST:
        return "BTA_PBS_LISTEN_ST";
    case BTA_PBS_W4_AUTH_ST:
        return "BTA_PBS_W4_AUTH_ST";
    case BTA_PBS_CONN_ST:
        return "BTA_PBS_CONN_ST";
    case BTA_PBS_CLOSING_ST:
        return "BTA_PBS_CLOSING_ST";
    default:
        return "unknown PBS state code";
    }
}

#endif  /* Debug Functions */
#endif /* BTA_PBS_INCLUDED */
