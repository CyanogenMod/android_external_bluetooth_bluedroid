/*****************************************************************************
**
**  Name:           bta_fts_main.c
**
**  Description:    This file contains the file transfer server main functions 
**                  and state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bt_target.h"
#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include "bta_fs_api.h"
#include "bta_fts_int.h"
#include "gki.h"
#include "utl.h"
#include "obx_api.h"
#include "rfcdefs.h"    /* BT_PSM_RFCOMM */

/*****************************************************************************
** Constants and types
*****************************************************************************/


/* state machine action enumeration list */
enum
{
    BTA_FTS_API_DISABLE,
    BTA_FTS_API_AUTHRSP,
    BTA_FTS_API_ACCESSRSP,
    BTA_FTS_API_CLOSE,
    BTA_FTS_CI_WRITE,
    BTA_FTS_CI_READ,
    BTA_FTS_CI_OPEN,
    BTA_FTS_CI_DIRENTRY,
    BTA_FTS_OBX_CONNECT,
    BTA_FTS_OBX_DISC,
    BTA_FTS_OBX_CLOSE,
    BTA_FTS_OBX_ABORT,
    BTA_FTS_OBX_PASSWORD,
    BTA_FTS_OBX_PUT,
    BTA_FTS_OBX_GET,
    BTA_FTS_OBX_SETPATH,
    BTA_FTS_CI_RESUME,
    BTA_FTS_OBX_ACTION,
    BTA_FTS_SESSION_REQ,
    BTA_FTS_APPL_TOUT,
    BTA_FTS_CONN_ERR_RSP,
    BTA_FTS_DISC_ERR_RSP,
    BTA_FTS_GASP_ERR_RSP,
    BTA_FTS_CLOSE_COMPLETE,
    BTA_FTS_DISABLE_CMPL,
    BTA_FTS_IGNORE
};

/* type for action functions */
typedef void (*tBTA_FTS_ACTION)(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data);

/* action function list */
const tBTA_FTS_ACTION bta_fts_action[] =
{
    bta_fts_api_disable,
    bta_fts_api_authrsp,
    bta_fts_api_accessrsp,
    bta_fts_api_close,
    bta_fts_ci_write,
    bta_fts_ci_read,
    bta_fts_ci_open,
    bta_fts_ci_direntry,
    bta_fts_obx_connect,
    bta_fts_obx_disc,
    bta_fts_obx_close,
    bta_fts_obx_abort,
    bta_fts_obx_password,
    bta_fts_obx_put,
    bta_fts_obx_get,
    bta_fts_obx_setpath,
    bta_fts_ci_resume,
    bta_fts_obx_action,
    bta_fts_session_req,
    bta_fts_appl_tout,
    bta_fts_conn_err_rsp,
    bta_fts_disc_err_rsp,
    bta_fts_gasp_err_rsp,
    bta_fts_close_complete,
    bta_fts_disable_cmpl
};


/* state table information */
#define BTA_FTS_ACTIONS             1       /* number of actions */
#define BTA_FTS_NEXT_STATE          1       /* position of next state */
#define BTA_FTS_NUM_COLS            2       /* number of columns in state tables */

/* state table for idle state */
static const UINT8 bta_fts_st_idle[][BTA_FTS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_FTS_API_ENABLE_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_API_AUTHRSP_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_API_ACCESSRSP_EVT */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_API_CLOSE_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CI_WRITE_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CI_READ_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CI_OPEN_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CI_DIRENTRY_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_CONN_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_DISC_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_ABORT_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_PASSWORD_EVT  */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_CLOSE_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_PUT_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_GET_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_OBX_SETPATH_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CI_SESSION_EVT    */   {BTA_FTS_CI_RESUME,    BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_ACTION_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_APPL_TOUT_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_DISC_ERR_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_GASP_ERR_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_CLOSE_CMPL_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST},
/* BTA_FTS_DISABLE_CMPL_EVT  */   {BTA_FTS_IGNORE,       BTA_FTS_IDLE_ST}
};

/* state table for obex/rfcomm connection state */
static const UINT8 bta_fts_st_listen[][BTA_FTS_NUM_COLS] =
{
/* Event                          Action 1               Next state */
/* BTA_FTS_API_DISABLE_EVT   */   {BTA_FTS_API_DISABLE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_API_AUTHRSP_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_API_ACCESSRSP_EVT */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_API_CLOSE_EVT     */   {BTA_FTS_API_CLOSE,    BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_WRITE_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_READ_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_OPEN_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_DIRENTRY_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_CONN_EVT      */   {BTA_FTS_OBX_CONNECT,  BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_DISC_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_ABORT_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_PASSWORD_EVT  */   {BTA_FTS_OBX_PASSWORD, BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_CLOSE_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_PUT_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_GET_EVT       */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_SETPATH_EVT   */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_SESSION_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_ACTION_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_APPL_TOUT_EVT     */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_DISC_ERR_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_GASP_ERR_EVT      */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_CLOSE_CMPL_EVT    */   {BTA_FTS_IGNORE,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_DISABLE_CMPL_EVT  */   {BTA_FTS_DISABLE_CMPL, BTA_FTS_IDLE_ST}
};

/* state table for wait for authentication response state */
static const UINT8 bta_fts_st_w4_auth[][BTA_FTS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_FTS_API_DISABLE_EVT   */   {BTA_FTS_API_DISABLE,     BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_API_AUTHRSP_EVT   */   {BTA_FTS_API_AUTHRSP,     BTA_FTS_LISTEN_ST},
/* BTA_FTS_API_ACCESSRSP_EVT */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_API_CLOSE_EVT     */   {BTA_FTS_API_CLOSE,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_CI_WRITE_EVT      */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_CI_READ_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_CI_OPEN_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_CI_DIRENTRY_EVT   */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_CONN_EVT      */   {BTA_FTS_CONN_ERR_RSP,    BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_DISC_EVT      */   {BTA_FTS_OBX_DISC,        BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_ABORT_EVT     */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_PASSWORD_EVT  */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_CLOSE_EVT     */   {BTA_FTS_OBX_CLOSE,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_PUT_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_GET_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_SETPATH_EVT   */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_CI_SESSION_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_ACTION_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_APPL_TOUT_EVT     */   {BTA_FTS_APPL_TOUT,       BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_DISC_ERR_EVT      */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_GASP_ERR_EVT      */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_CLOSE_CMPL_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_DISABLE_CMPL_EVT  */   {BTA_FTS_DISABLE_CMPL,    BTA_FTS_IDLE_ST}
};

/* state table for open state */
static const UINT8 bta_fts_st_connected[][BTA_FTS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_FTS_API_DISABLE_EVT   */   {BTA_FTS_API_DISABLE,     BTA_FTS_CONN_ST},
/* BTA_FTS_API_AUTHRSP_EVT   */   {BTA_FTS_IGNORE,          BTA_FTS_CONN_ST},
/* BTA_FTS_API_ACCESSRSP_EVT */   {BTA_FTS_API_ACCESSRSP,   BTA_FTS_CONN_ST},
/* BTA_FTS_API_CLOSE_EVT     */   {BTA_FTS_API_CLOSE,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_CI_WRITE_EVT      */   {BTA_FTS_CI_WRITE,        BTA_FTS_CONN_ST},
/* BTA_FTS_CI_READ_EVT       */   {BTA_FTS_CI_READ,         BTA_FTS_CONN_ST},
/* BTA_FTS_CI_OPEN_EVT       */   {BTA_FTS_CI_OPEN,         BTA_FTS_CONN_ST},
/* BTA_FTS_CI_DIRENTRY_EVT   */   {BTA_FTS_CI_DIRENTRY,     BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_CONN_EVT      */   {BTA_FTS_SESSION_REQ,     BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_DISC_EVT      */   {BTA_FTS_OBX_DISC,        BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_ABORT_EVT     */   {BTA_FTS_OBX_ABORT,       BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_PASSWORD_EVT  */   {BTA_FTS_IGNORE,          BTA_FTS_W4_AUTH_ST},
/* BTA_FTS_OBX_CLOSE_EVT     */   {BTA_FTS_OBX_CLOSE,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_PUT_EVT       */   {BTA_FTS_OBX_PUT,         BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_GET_EVT       */   {BTA_FTS_OBX_GET,         BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_SETPATH_EVT   */   {BTA_FTS_OBX_SETPATH,     BTA_FTS_CONN_ST},
/* BTA_FTS_CI_SESSION_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_CONN_ST},
/* BTA_FTS_OBX_ACTION_EVT    */   {BTA_FTS_OBX_ACTION,      BTA_FTS_CONN_ST},
/* BTA_FTS_APPL_TOUT_EVT     */   {BTA_FTS_APPL_TOUT,       BTA_FTS_LISTEN_ST},
/* BTA_FTS_DISC_ERR_EVT      */   {BTA_FTS_IGNORE,          BTA_FTS_CONN_ST},
/* BTA_FTS_GASP_ERR_EVT      */   {BTA_FTS_GASP_ERR_RSP,    BTA_FTS_CONN_ST},
/* BTA_FTS_CLOSE_CMPL_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_CONN_ST},
/* BTA_FTS_DISABLE_CMPL_EVT  */   {BTA_FTS_DISABLE_CMPL,    BTA_FTS_IDLE_ST}
};

/* state table for closing state */
static const UINT8 bta_fts_st_closing[][BTA_FTS_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_FTS_API_DISABLE_EVT   */   {BTA_FTS_API_DISABLE,     BTA_FTS_CLOSING_ST},
/* BTA_FTS_API_AUTHRSP_EVT   */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_API_ACCESSRSP_EVT */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_API_CLOSE_EVT     */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_CI_WRITE_EVT      */   {BTA_FTS_CLOSE_COMPLETE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_READ_EVT       */   {BTA_FTS_CLOSE_COMPLETE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_OPEN_EVT       */   {BTA_FTS_CLOSE_COMPLETE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_CI_DIRENTRY_EVT   */   {BTA_FTS_CLOSE_COMPLETE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_OBX_CONN_EVT      */   {BTA_FTS_SESSION_REQ,     BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_DISC_EVT      */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_ABORT_EVT     */   {BTA_FTS_OBX_ABORT,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_PASSWORD_EVT  */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_CLOSE_EVT     */   {BTA_FTS_OBX_CLOSE,       BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_PUT_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_GET_EVT       */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_SETPATH_EVT   */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_CI_SESSION_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_OBX_ACTION_EVT    */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_APPL_TOUT_EVT     */   {BTA_FTS_IGNORE,          BTA_FTS_CLOSING_ST},
/* BTA_FTS_DISC_ERR_EVT      */   {BTA_FTS_DISC_ERR_RSP,    BTA_FTS_CONN_ST},
/* BTA_FTS_GASP_ERR_EVT      */   {BTA_FTS_GASP_ERR_RSP,    BTA_FTS_CLOSING_ST},
/* BTA_FTS_CLOSE_CMPL_EVT    */   {BTA_FTS_CLOSE_COMPLETE,  BTA_FTS_LISTEN_ST},
/* BTA_FTS_DISABLE_CMPL_EVT  */   {BTA_FTS_DISABLE_CMPL,    BTA_FTS_IDLE_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_FTS_ST_TBL)[BTA_FTS_NUM_COLS];

/* state table */
const tBTA_FTS_ST_TBL bta_fts_st_tbl[] =
{
    bta_fts_st_idle,
    bta_fts_st_listen,
    bta_fts_st_w4_auth,
    bta_fts_st_connected,
    bta_fts_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* FTS control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_FTS_CB  bta_fts_cb;
#endif

#if BTA_FTS_DEBUG == TRUE
static char *fts_evt_code(tBTA_FTS_INT_EVT evt_code);
static char *fts_state_code(tBTA_FTS_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_fts_sm_execute
**
** Description      State machine event handling function for FTS
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_fts_sm_execute(tBTA_FTS_CB *p_cb, UINT16 event, tBTA_FTS_DATA *p_data)
{
    tBTA_FTS_ST_TBL     state_table;
    UINT8               action;
    int                 i;
#if BTA_FTS_DEBUG == TRUE
    tBTA_FTS_STATE in_state = bta_fts_cb.state;
    UINT16         in_event = event;
    APPL_TRACE_EVENT3("FTS Event        : State 0x%02x [%s], Event [%s]", in_state,
                      fts_state_code(in_state),
                      fts_evt_code(event));
#endif

    /* look up the state table for the current state */
    state_table = bta_fts_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_FTS_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_FTS_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_FTS_IGNORE)
        {
            (*bta_fts_action[action])(p_cb, p_data);
        }
        else
        {
            /* discard fts data */
            bta_fts_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }

#if BTA_FTS_DEBUG == TRUE
    if (in_state != bta_fts_cb.state)
    {
        APPL_TRACE_DEBUG3("FTS State Change: [%s] -> [%s] after Event [%s]",
                      fts_state_code(in_state),
                      fts_state_code(bta_fts_cb.state),
                      fts_evt_code(in_event));
    }
#endif
}

/*******************************************************************************
**
** Function         bta_fts_api_enable
**
** Description      Handle an api enable event.  This function enables the FT
**                  Server by opening an Obex/Rfcomm channel and placing it into
**                  listen mode.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_fts_api_enable(tBTA_FTS_CB *p_cb, tBTA_FTS_DATA *p_data)
{
    tOBX_StartParams     start_msg;
    tBTA_FTS_API_ENABLE *p_api = &p_data->api_enable;
    tOBX_TARGET          target;
    UINT16               len;
    tOBX_STATUS          status;
    UINT16               mtu = OBX_MAX_MTU;

    /* initialize control block */
    memset(p_cb, 0, sizeof(tBTA_FTS_CB));
    
    /* Allocate an aligned memory buffer to hold the root path and working directory */
    /* Add 1 byte for '\0' */
    len = p_bta_fs_cfg->max_path_len + 1;
    if ((p_cb->p_rootpath = (char *)GKI_getbuf((UINT16)(len * 2))) != NULL)
    {
        p_cb->p_workdir = p_cb->p_rootpath + len;
        memcpy(target.target, BTA_FTS_FOLDER_BROWSING_TARGET_UUID, BTA_FTS_UUID_LENGTH);
        target.len = BTA_FTS_UUID_LENGTH;
        
        /* store parameters */
        p_cb->app_id = p_api->app_id;
        p_cb->p_cback = p_api->p_cback;
        p_cb->scn = BTM_AllocateSCN();

        if (p_bta_ft_cfg->over_l2cap)
        {
            p_cb->psm = L2CA_AllocatePSM();
            BTM_SetSecurityLevel (FALSE, p_api->servicename, BTM_SEC_SERVICE_OBEX_FTP,
                                  p_api->sec_mask, p_cb->psm,
                                  0, 0);
        }

        p_cb->auth_enabled = p_api->auth_enabled;
        
        p_cb->fd = BTA_FS_INVALID_FD;
        /* Initialize the current working directory to be the root directory */
        BCM_STRNCPY_S(p_cb->p_rootpath, len, p_api->p_root_path, len-1);
        BCM_STRNCPY_S(p_cb->p_workdir, len, p_api->p_root_path, len-1);
        
        /* Register FTP security requirements with BTM */
        BTM_SetSecurityLevel(FALSE, p_api->servicename, BTM_SEC_SERVICE_OBEX_FTP,
            p_api->sec_mask, BT_PSM_RFCOMM,
            BTM_SEC_PROTO_RFCOMM, (UINT32)p_cb->scn);
        
        /* Start up the FTP service */
        memset (&start_msg, 0, sizeof(tOBX_StartParams));
        start_msg.p_target = &target;

        /* Make the MTU fit into one RFC frame */
        start_msg.mtu = mtu;
        start_msg.scn = p_cb->scn;
        start_msg.psm = p_cb->psm;
        start_msg.srm = p_bta_ft_cfg->srm;
        start_msg.nonce = p_bta_ft_cfg->nonce;
        start_msg.max_suspend = p_bta_ft_cfg->max_suspend;
        start_msg.authenticate = p_cb->auth_enabled;

        start_msg.auth_option = (p_bta_ft_cfg->userid_req) ? OBX_AO_USR_ID : OBX_AO_NONE;
        start_msg.p_cback = bta_fts_obx_cback;
        
        start_msg.realm_len = p_api->realm_len;
        start_msg.p_realm = p_api->realm;
        start_msg.realm_charset = (tOBX_CHARSET) p_bta_ft_cfg->realm_charset;
 
        if ((status = OBX_StartServer (&start_msg, &p_cb->obx_handle)) == OBX_SUCCESS)
        {
            /* Set up the SDP record for file transfer service */
            bta_fts_sdp_register(p_cb, p_api->servicename);

            if (start_msg.nonce)
            {
                bta_fs_co_resume (BTA_FTS_CI_SESSION_EVT, p_cb->app_id);
            }
            else
            {
                p_data->resume_evt.status = BTA_FS_CO_FAIL;
                bta_fts_ci_resume(p_cb, p_data);
            }
        }
        else
            APPL_TRACE_ERROR1("OBX_StartServer returns error (%d)", status);
    }
    else    /* Cannot allocate resources to run Server */
        APPL_TRACE_ERROR0("Not enough Resources to run FTP Server");

    p_cb->p_cback(BTA_FTS_ENABLE_EVT, 0);
}

/*******************************************************************************
**
** Function         bta_fts_hdl_event
**
** Description      File transfer server main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_fts_hdl_event(BT_HDR *p_msg)
{
#if BTA_FTS_DEBUG == TRUE
    tBTA_FTS_STATE in_state = bta_fts_cb.state;
#endif

    switch (p_msg->event)
    {
        case BTA_FTS_API_ENABLE_EVT:
#if BTA_FTS_DEBUG == TRUE
            APPL_TRACE_EVENT3("FTS Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                              fts_state_code(in_state),
                              fts_evt_code(p_msg->event));
#endif
            bta_fts_api_enable(&bta_fts_cb, (tBTA_FTS_DATA *) p_msg);

#if BTA_FTS_DEBUG == TRUE
            if (in_state != bta_fts_cb.state)
            {
                APPL_TRACE_DEBUG3("FTS State Change: [%s] -> [%s] after Event [%s]",
                              fts_state_code(in_state),
                              fts_state_code(bta_fts_cb.state),
                              fts_evt_code(p_msg->event));
            }
#endif
            break;

        default:

            bta_fts_sm_execute(&bta_fts_cb, p_msg->event, (tBTA_FTS_DATA *) p_msg);

            if ( bta_fts_cb.state == BTA_FTS_CONN_ST )
            {
                if (( bta_fts_cb.pm_state == BTA_FTS_PM_IDLE )
                  &&( bta_fts_cb.obx_oper != FTS_OP_NONE ))
                {
                    /* inform power manager */
                    APPL_TRACE_DEBUG0("BTA FTS informs DM/PM busy state");
                    bta_sys_busy( BTA_ID_FTS ,bta_fts_cb.app_id, bta_fts_cb.bd_addr);
                    bta_fts_cb.pm_state = BTA_FTS_PM_BUSY;
                }
                else if (( bta_fts_cb.pm_state == BTA_FTS_PM_BUSY )
                       &&( bta_fts_cb.obx_oper == FTS_OP_NONE ))
                {
                    /* inform power manager */
                    APPL_TRACE_DEBUG0("BTA FTS informs DM/PM idle state");
                    bta_sys_idle( BTA_ID_FTS ,bta_fts_cb.app_id, bta_fts_cb.bd_addr);
                    bta_fts_cb.pm_state = BTA_FTS_PM_IDLE;
                }
            }
            else if ( bta_fts_cb.state == BTA_FTS_LISTEN_ST )
            {
                /* initialize power management state */
                bta_fts_cb.pm_state = BTA_FTS_PM_BUSY;
            }
            
            break;
    }

    return (TRUE);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_FTS_DEBUG == TRUE

/*******************************************************************************
**
** Function         fts_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *fts_evt_code(tBTA_FTS_INT_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_FTS_API_DISABLE_EVT:
        return "BTA_FTS_API_DISABLE_EVT";
    case BTA_FTS_API_AUTHRSP_EVT:
        return "BTA_FTS_API_AUTHRSP_EVT";
    case BTA_FTS_API_ACCESSRSP_EVT:
        return "BTA_FTS_API_ACCESSRSP_EVT";
    case BTA_FTS_API_CLOSE_EVT:
        return "BTA_FTS_API_CLOSE_EVT";
    case BTA_FTS_CI_WRITE_EVT:
        return "BTA_FTS_CI_WRITE_EVT";
    case BTA_FTS_CI_READ_EVT:
        return "BTA_FTS_CI_READ_EVT";
    case BTA_FTS_CI_OPEN_EVT:
        return "BTA_FTS_CI_OPEN_EVT";
    case BTA_FTS_CI_DIRENTRY_EVT:
        return "BTA_FTS_CI_DIRENTRY_EVT";
    case BTA_FTS_OBX_CONN_EVT:
        return "BTA_FTS_OBX_CONN_EVT";
    case BTA_FTS_OBX_DISC_EVT:
        return "BTA_FTS_OBX_DISC_EVT";
    case BTA_FTS_OBX_ABORT_EVT:
        return "BTA_FTS_OBX_ABORT_EVT";
    case BTA_FTS_OBX_PASSWORD_EVT:
        return "BTA_FTS_OBX_PASSWORD_EVT";
    case BTA_FTS_OBX_CLOSE_EVT:
        return "BTA_FTS_OBX_CLOSE_EVT";
    case BTA_FTS_OBX_PUT_EVT:
        return "BTA_FTS_OBX_PUT_EVT";
    case BTA_FTS_OBX_GET_EVT:
        return "BTA_FTS_OBX_GET_EVT";
    case BTA_FTS_OBX_SETPATH_EVT:
        return "BTA_FTS_OBX_SETPATH_EVT";
    case BTA_FTS_OBX_ACTION_EVT:
        return "BTA_FTS_OBX_ACTION_EVT";
    case BTA_FTS_CI_SESSION_EVT:
        return "BTA_FTS_CI_SESSION_EVT";
    case BTA_FTS_APPL_TOUT_EVT:
        return "BTA_FTS_APPL_TOUT_EVT";
    case BTA_FTS_DISC_ERR_EVT:
        return "BTA_FTS_DISC_ERR_EVT";
    case BTA_FTS_GASP_ERR_EVT:
        return "BTA_FTS_GASP_ERR_EVT";
    case BTA_FTS_API_ENABLE_EVT:
        return "BTA_FTS_API_ENABLE_EVT";
    case BTA_FTS_CLOSE_CMPL_EVT:
        return "BTA_FTS_CLOSE_CMPL_EVT";
    case BTA_FTS_DISABLE_CMPL_EVT:
        return "BTA_FTS_DISABLE_CMPL_EVT";
    default:
        return "unknown FTS event code";
    }
}

/*******************************************************************************
**
** Function         fts_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *fts_state_code(tBTA_FTS_STATE state_code)
{
    switch(state_code)
    {
    case BTA_FTS_IDLE_ST:
        return "BTA_FTS_IDLE_ST";
    case BTA_FTS_LISTEN_ST:
        return "BTA_FTS_LISTEN_ST";
    case BTA_FTS_W4_AUTH_ST:
        return "BTA_FTS_W4_AUTH_ST";
    case BTA_FTS_CONN_ST:
        return "BTA_FTS_CONN_ST";
    case BTA_FTS_CLOSING_ST:
        return "BTA_FTS_CLOSING_ST";
    default:
        return "unknown FTS state code";
    }
}

#endif  /* Debug Functions */
#endif /* BTA_FT_INCLUDED */
