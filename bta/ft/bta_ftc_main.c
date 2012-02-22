/*****************************************************************************
**
**  Name:           bta_ftc_main.c
**
**  Description:    This file contains the file transfer client main functions 
**                  and state machine.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bt_target.h"

#if defined(BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)

#include "bta_ftc_int.h"
#include "gki.h"
#include "obx_api.h"


/*****************************************************************************
** Constants and types
*****************************************************************************/


/* state machine action enumeration list */
enum
{
    BTA_FTC_INIT_OPEN,
    BTA_FTC_START_CLIENT,
    BTA_FTC_STOP_CLIENT,
    BTA_FTC_INIT_GETFILE,
    BTA_FTC_INIT_PUTFILE,
    BTA_FTC_LISTDIR,
    BTA_FTC_CHDIR,
    BTA_FTC_MKDIR,
    BTA_FTC_REMOVE,
    BTA_FTC_SEND_AUTHRSP,
    BTA_FTC_CI_WRITE,
    BTA_FTC_CI_READ,
    BTA_FTC_CI_OPEN,
    BTA_FTC_CI_RESUME,
    BTA_FTC_OBX_SESS_RSP,
    BTA_FTC_OBX_GET_SRM_RSP,
    BTA_FTC_CI_WRITE_SRM,
    BTA_FTC_SUSPENDED,
    BTA_FTC_OBX_CONN_RSP,
    BTA_FTC_CLOSE,
    BTA_FTC_OPEN_FAIL,
    BTA_FTC_OBX_ABORT_RSP,
    BTA_FTC_OBX_PASSWORD,
    BTA_FTC_OBX_TIMEOUT,
    BTA_FTC_OBX_PUT_RSP,
    BTA_FTC_OBX_GET_RSP,
    BTA_FTC_OBX_SETPATH_RSP,
    BTA_FTC_TRANS_CMPL,
    BTA_FTC_FREE_DB,
    BTA_FTC_IGNORE_OBX,
    BTA_FTC_FIND_SERVICE,
    BTA_FTC_INITIALIZE,
    BTA_FTC_CLOSE_COMPLETE,
    BTA_FTC_BIC_PUT,
    BTA_FTC_BIC_ABORT,
    BTA_FTC_SET_DISABLE,
    BTA_FTC_ABORT,
    BTA_FTC_API_ACTION,
    BTA_FTC_OBX_ACTION_RSP,
    BTA_FTC_RSP_TIMEOUT,
    BTA_FTC_IGNORE
};

/* action function list */
const tBTA_FTC_ACTION bta_ftc_action[] =
{
    bta_ftc_init_open,
    bta_ftc_start_client,
    bta_ftc_stop_client,
    bta_ftc_init_getfile,
    bta_ftc_init_putfile,
    bta_ftc_listdir,
    bta_ftc_chdir,
    bta_ftc_mkdir,
    bta_ftc_remove,
    bta_ftc_send_authrsp,
    bta_ftc_ci_write,
    bta_ftc_ci_read,
    bta_ftc_ci_open,
    bta_ftc_ci_resume,
    bta_ftc_obx_sess_rsp,
    bta_ftc_obx_get_srm_rsp,
    bta_ftc_ci_write_srm,
    bta_ftc_suspended,
    bta_ftc_obx_conn_rsp,
    bta_ftc_close,
    bta_ftc_open_fail,
    bta_ftc_obx_abort_rsp,
    bta_ftc_obx_password,
    bta_ftc_obx_timeout,
    bta_ftc_obx_put_rsp,
    bta_ftc_obx_get_rsp,
    bta_ftc_obx_setpath_rsp,
    bta_ftc_trans_cmpl,
    bta_ftc_free_db,
    bta_ftc_ignore_obx,
    bta_ftc_find_service,
    bta_ftc_initialize,
    bta_ftc_close_complete,
    bta_ftc_bic_put,
    bta_ftc_bic_abort,
    bta_ftc_set_disable,
    bta_ftc_abort,
    bta_ftc_api_action,
    bta_ftc_obx_action_rsp,
    bta_ftc_rsp_timeout
};


/* state table information */
#define BTA_FTC_ACTIONS             2       /* number of actions */
#define BTA_FTC_NEXT_STATE          2       /* position of next state */
#define BTA_FTC_NUM_COLS            3       /* number of columns in state tables */

/* state table for idle state */
static const UINT8 bta_ftc_st_idle[][BTA_FTC_NUM_COLS] =
{
/* Event                            Action 1                Action 2              Next state */
/* BTA_FTC_API_DISABLE_EVT     */   {BTA_FTC_SET_DISABLE,   BTA_FTC_INITIALIZE,   BTA_FTC_IDLE_ST},
/* BTA_FTC_API_OPEN_EVT        */   {BTA_FTC_INIT_OPEN,     BTA_FTC_FIND_SERVICE, BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_CLOSE_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_PUTFILE_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_GETFILE_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_LISTDIR_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_CHDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_MKDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_REMOVE_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_AUTHRSP_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_ABORT_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_API_ACTION_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_ACTION_RSP_EVT  */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_SESSION_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_SDP_OK_EVT          */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_SDP_FAIL_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_SDP_NEXT_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_WRITE_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_READ_EVT         */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_OPEN_EVT         */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_CONN_RSP_EVT    */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_ABORT_RSP_EVT   */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_TOUT_EVT        */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_PASSWORD_EVT    */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_CLOSE_EVT       */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_PUT_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_GET_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_SETPATH_RSP_EVT */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_CMPL_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_CLOSE_CMPL_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_DISABLE_CMPL_EVT    */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_RSP_TOUT_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST}
};

/* state table for wait for authentication response state */
static const UINT8 bta_ftc_st_w4_conn[][BTA_FTC_NUM_COLS] =
{
/* Event                            Action 1                Action 2              Next state */
/* BTA_FTC_API_DISABLE_EVT     */   {BTA_FTC_SET_DISABLE,   BTA_FTC_STOP_CLIENT,  BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_OPEN_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_CLOSE_EVT       */   {BTA_FTC_STOP_CLIENT,   BTA_FTC_IGNORE,       BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_PUTFILE_EVT     */   {BTA_FTC_BIC_PUT,       BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_GETFILE_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_LISTDIR_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_CHDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_MKDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_REMOVE_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_AUTHRSP_EVT     */   {BTA_FTC_SEND_AUTHRSP,  BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_ABORT_EVT       */   {BTA_FTC_BIC_ABORT,     BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_API_ACTION_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_ACTION_RSP_EVT  */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_CI_SESSION_EVT      */   {BTA_FTC_CI_RESUME,     BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_SDP_OK_EVT          */   {BTA_FTC_FREE_DB,       BTA_FTC_START_CLIENT, BTA_FTC_W4_CONN_ST},
/* BTA_FTC_SDP_FAIL_EVT        */   {BTA_FTC_FREE_DB,       BTA_FTC_CLOSE,        BTA_FTC_CLOSING_ST},
/* BTA_FTC_SDP_NEXT_EVT        */   {BTA_FTC_FREE_DB,       BTA_FTC_FIND_SERVICE, BTA_FTC_W4_CONN_ST},
/* BTA_FTC_CI_WRITE_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_CI_READ_EVT         */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_CI_OPEN_EVT         */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_CONN_RSP_EVT    */   {BTA_FTC_OBX_CONN_RSP,  BTA_FTC_IGNORE,       BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_ABORT_RSP_EVT   */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_TOUT_EVT        */   {BTA_FTC_OBX_TIMEOUT,   BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_PASSWORD_EVT    */   {BTA_FTC_OBX_PASSWORD,  BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_CLOSE_EVT       */   {BTA_FTC_OPEN_FAIL,     BTA_FTC_IGNORE,       BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_PUT_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_GET_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_SETPATH_RSP_EVT */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_OBX_CMPL_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_W4_CONN_ST},
/* BTA_FTC_CLOSE_CMPL_EVT      */   {BTA_FTC_CLOSE_COMPLETE,BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_DISABLE_CMPL_EVT    */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST},
/* BTA_FTC_RSP_TOUT_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,       BTA_FTC_IDLE_ST}

};

/* state table for connected state */
static const UINT8 bta_ftc_st_connected[][BTA_FTC_NUM_COLS] =
{
/* Event                            Action 1                  Action 2             Next state */
/* BTA_FTC_API_DISABLE_EVT     */   {BTA_FTC_SET_DISABLE,     BTA_FTC_STOP_CLIENT, BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_OPEN_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_CLOSE_EVT       */   {BTA_FTC_STOP_CLIENT,     BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_PUTFILE_EVT     */   {BTA_FTC_INIT_PUTFILE,    BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_GETFILE_EVT     */   {BTA_FTC_INIT_GETFILE,    BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_LISTDIR_EVT     */   {BTA_FTC_LISTDIR ,        BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_CHDIR_EVT       */   {BTA_FTC_CHDIR,           BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_MKDIR_EVT       */   {BTA_FTC_MKDIR,           BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_REMOVE_EVT      */   {BTA_FTC_REMOVE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_AUTHRSP_EVT     */   {BTA_FTC_SEND_AUTHRSP,    BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_ABORT_EVT       */   {BTA_FTC_ABORT,           BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_API_ACTION_EVT      */   {BTA_FTC_API_ACTION,      BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_ACTION_RSP_EVT  */   {BTA_FTC_OBX_ACTION_RSP,  BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_CI_SESSION_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_SDP_OK_EVT          */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_SDP_FAIL_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_SDP_NEXT_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_CI_WRITE_EVT        */   {BTA_FTC_CI_WRITE,        BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_CI_READ_EVT         */   {BTA_FTC_CI_READ,         BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_CI_OPEN_EVT         */   {BTA_FTC_CI_OPEN,         BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_CONN_RSP_EVT    */   {BTA_FTC_SUSPENDED,       BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_ABORT_RSP_EVT   */   {BTA_FTC_OBX_ABORT_RSP,   BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_TOUT_EVT        */   {BTA_FTC_OBX_TIMEOUT,     BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_PASSWORD_EVT    */   {BTA_FTC_OBX_PASSWORD,    BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_CLOSE_EVT       */   {BTA_FTC_CLOSE,           BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_PUT_RSP_EVT     */   {BTA_FTC_OBX_PUT_RSP,     BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_GET_RSP_EVT     */   {BTA_FTC_OBX_GET_RSP,     BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_SETPATH_RSP_EVT */   {BTA_FTC_OBX_SETPATH_RSP, BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_OBX_CMPL_EVT        */   {BTA_FTC_TRANS_CMPL,      BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_CLOSE_CMPL_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_CONN_ST},
/* BTA_FTC_DISABLE_CMPL_EVT    */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_RSP_TOUT_EVT        */   {BTA_FTC_RSP_TIMEOUT,     BTA_FTC_IGNORE,      BTA_FTC_CONN_ST}
};

/* state table for suspending state */
static const UINT8 bta_ftc_st_suspending[][BTA_FTC_NUM_COLS] =
{
/* Event                            Action 1                  Action 2             Next state */
/* BTA_FTC_API_DISABLE_EVT     */   {BTA_FTC_SET_DISABLE,     BTA_FTC_STOP_CLIENT, BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_OPEN_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_CLOSE_EVT       */   {BTA_FTC_STOP_CLIENT,     BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_PUTFILE_EVT     */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_GETFILE_EVT     */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_LISTDIR_EVT     */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_CHDIR_EVT       */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_MKDIR_EVT       */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_REMOVE_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_AUTHRSP_EVT     */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_ABORT_EVT       */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_API_ACTION_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_ACTION_RSP_EVT  */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_CI_SESSION_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_SDP_OK_EVT          */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_SDP_FAIL_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_SDP_NEXT_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_CI_WRITE_EVT        */   {BTA_FTC_CI_WRITE_SRM,    BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_CI_READ_EVT         */   {BTA_FTC_CI_READ,         BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_CI_OPEN_EVT         */   {BTA_FTC_CI_OPEN,         BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_CONN_RSP_EVT    */   {BTA_FTC_OBX_SESS_RSP,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_ABORT_RSP_EVT   */   {BTA_FTC_OBX_ABORT_RSP,   BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_TOUT_EVT        */   {BTA_FTC_OBX_TIMEOUT,     BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_PASSWORD_EVT    */   {BTA_FTC_IGNORE_OBX,      BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_CLOSE_EVT       */   {BTA_FTC_CLOSE,           BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_PUT_RSP_EVT     */   {BTA_FTC_OBX_PUT_RSP,     BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_GET_RSP_EVT     */   {BTA_FTC_OBX_GET_SRM_RSP, BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_SETPATH_RSP_EVT */   {BTA_FTC_IGNORE_OBX,      BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_OBX_CMPL_EVT        */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_CLOSE_CMPL_EVT      */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_SUSPENDING_ST},
/* BTA_FTC_DISABLE_CMPL_EVT    */   {BTA_FTC_IGNORE,          BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_RSP_TOUT_EVT        */   {BTA_FTC_RSP_TIMEOUT,     BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST}
};

/* state table for closing state */
static const UINT8 bta_ftc_st_closing[][BTA_FTC_NUM_COLS] =
{
/* Event                            Action 1                Action 2             Next state */
/* BTA_FTC_API_DISABLE_EVT     */   {BTA_FTC_SET_DISABLE,   BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_OPEN_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_CLOSE_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_PUTFILE_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_GETFILE_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_LISTDIR_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_CHDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_MKDIR_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_REMOVE_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_AUTHRSP_EVT     */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_ABORT_EVT       */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_API_ACTION_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_ACTION_RSP_EVT  */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_CI_SESSION_EVT      */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_SDP_OK_EVT          */   {BTA_FTC_FREE_DB,       BTA_FTC_CLOSE,       BTA_FTC_CLOSING_ST},
/* BTA_FTC_SDP_FAIL_EVT        */   {BTA_FTC_FREE_DB,       BTA_FTC_CLOSE,       BTA_FTC_CLOSING_ST},
/* BTA_FTC_SDP_NEXT_EVT        */   {BTA_FTC_FREE_DB,       BTA_FTC_CLOSE,       BTA_FTC_CLOSING_ST},
/* BTA_FTC_CI_WRITE_EVT        */   {BTA_FTC_CLOSE_COMPLETE,BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_READ_EVT         */   {BTA_FTC_CLOSE_COMPLETE,BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_CI_OPEN_EVT         */   {BTA_FTC_CLOSE_COMPLETE,BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_OBX_CONN_RSP_EVT    */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_ABORT_RSP_EVT   */   {BTA_FTC_OBX_ABORT_RSP, BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_TOUT_EVT        */   {BTA_FTC_OBX_TIMEOUT,   BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_PASSWORD_EVT    */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_CLOSE_EVT       */   {BTA_FTC_CLOSE,         BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_PUT_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_GET_RSP_EVT     */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_SETPATH_RSP_EVT */   {BTA_FTC_IGNORE_OBX,    BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_OBX_CMPL_EVT        */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_CLOSING_ST},
/* BTA_FTC_CLOSE_CMPL_EVT      */   {BTA_FTC_CLOSE_COMPLETE,BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_DISABLE_CMPL_EVT    */   {BTA_FTC_IGNORE,        BTA_FTC_IGNORE,      BTA_FTC_IDLE_ST},
/* BTA_FTC_RSP_TOUT_EVT        */   {BTA_FTC_RSP_TIMEOUT,   BTA_FTC_CLOSE_COMPLETE, BTA_FTC_IDLE_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_FTC_ST_TBL)[BTA_FTC_NUM_COLS];

/* state table */
const tBTA_FTC_ST_TBL bta_ftc_st_tbl[] =
{
    bta_ftc_st_idle,
    bta_ftc_st_w4_conn,
    bta_ftc_st_connected,
    bta_ftc_st_suspending,
    bta_ftc_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* FTC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_FTC_CB  bta_ftc_cb;
#endif

#if BTA_FTC_DEBUG == TRUE
static char *ftc_evt_code(tBTA_FTC_INT_EVT evt_code);
static char *ftc_state_code(tBTA_FTC_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_ftc_sm_execute
**
** Description      State machine event handling function for FTC
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ftc_sm_execute(tBTA_FTC_CB *p_cb, UINT16 event, tBTA_FTC_DATA *p_data)
{
    tBTA_FTC_ST_TBL     state_table;
    UINT8               action;
    int                 i;
#if BTA_FTC_DEBUG == TRUE
    tBTA_FTC_STATE in_state = p_cb->state;
    UINT16         in_event = event;
    APPL_TRACE_DEBUG4("bta_ftc_sm_execute: State 0x%02x [%s], Event 0x%x[%s]", in_state,
                      ftc_state_code(in_state),
                      in_event,
                      ftc_evt_code(in_event));
#endif

    /* look up the state table for the current state */
    state_table = bta_ftc_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_FTC_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_FTC_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_FTC_IGNORE)
        {
            (*bta_ftc_action[action])(p_cb, p_data);
        }
        else
        {
            break;
        }
    }

#if BTA_FTC_DEBUG == TRUE
    if (in_state != p_cb->state)
    {
        APPL_TRACE_DEBUG3("FTC State Change: [%s] -> [%s] after Event [%s]",
                      ftc_state_code(in_state),
                      ftc_state_code(p_cb->state),
                      ftc_evt_code(in_event));
    }
#endif
}

/*****************************************************************************
**
**  Function:    bta_ftc_sdp_register()
**
**  Purpose:     Registers the File Transfer service with SDP
**
**  Parameters:  
**
**
**  Returns:     void
**
*****************************************************************************/
static void bta_ftc_sdp_register (tBTA_FTC_CB *p_cb)
{
    UINT16              pbap_service = UUID_SERVCLASS_PBAP_PCE;
    UINT16              browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    BOOLEAN             status = FALSE;

    if ((p_cb->sdp_handle = SDP_CreateRecord()) == 0)
    {
        APPL_TRACE_WARNING0("FTC SDP: Unable to register PBAP PCE Service");
        return;
    }

    /* add service class */
    if (SDP_AddServiceClassIdList(p_cb->sdp_handle, 1, &pbap_service))
    {
        status = TRUE;  /* All mandatory fields were successful */

        /* optional:  if name is not "", add a name entry */
        if (p_bta_ft_cfg->pce_name && p_bta_ft_cfg->pce_name[0] != '\0')
            SDP_AddAttribute(p_cb->sdp_handle,
                             (UINT16)ATTR_ID_SERVICE_NAME,
                             (UINT8)TEXT_STR_DESC_TYPE,
                             (UINT32)(strlen(p_bta_ft_cfg->pce_name) + 1),
                             (UINT8 *)p_bta_ft_cfg->pce_name);

        /* Add in the Bluetooth Profile Descriptor List */
        SDP_AddProfileDescriptorList(p_cb->sdp_handle,
                                         UUID_SERVCLASS_PBAP_PCE,
                                         BTA_FTC_DEFAULT_VERSION);
    } /* end of setting mandatory service class */

    /* Make the service browseable */
    SDP_AddUuidSequence (p_cb->sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, &browse);

    if (!status)
    {
        SDP_DeleteRecord(p_cb->sdp_handle);
        APPL_TRACE_ERROR0("bta_ftc_sdp_register FAILED");
        p_cb->sdp_handle = 0;
    }
    else
    {
        bta_sys_add_uuid( pbap_service ); /* UUID_SERVCLASS_PBAP_PCE */
        APPL_TRACE_DEBUG1("FTC:  SDP Registered (handle 0x%08x)", p_cb->sdp_handle);
    }

    return;
}

/*******************************************************************************
**
** Function         bta_ftc_api_enable
**
** Description      Handle an api enable event.  This function enables the FT
**                  Client by opening an Obex/Rfcomm channel with a peer device.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_ftc_api_enable(tBTA_FTC_CB *p_cb, tBTA_FTC_DATA *p_data)
{
    if (!p_cb->is_enabled)
    {
        /* initialize control block */
        memset(p_cb, 0, sizeof(tBTA_FTC_CB));
    
        /* store parameters */
        p_cb->p_cback = p_data->api_enable.p_cback;
        p_cb->app_id = p_data->api_enable.app_id;
#if( defined BTA_BI_INCLUDED ) && (BTA_BI_INCLUDED == TRUE)
        p_cb->bic_handle = BTA_BIC_INVALID_HANDLE;
#endif
        p_cb->fd = BTA_FS_INVALID_FD;
        p_cb->is_enabled = TRUE;

        /* create SDP record for PCE */
        if(p_bta_ft_cfg->pce_features)
           bta_ftc_sdp_register(p_cb);
    }

    /* callback with enable event */
    (*p_cb->p_cback)(BTA_FTC_ENABLE_EVT, 0);
}

/*******************************************************************************
**
** Function         bta_ftc_hdl_event
**
** Description      File transfer server main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_ftc_hdl_event(BT_HDR *p_msg)
{
    tBTA_FTC_CB *p_cb = &bta_ftc_cb;
#if BTA_FTC_DEBUG == TRUE
    tBTA_FTC_STATE in_state = p_cb->state;
#endif

    switch (p_msg->event)
    {
        case BTA_FTC_API_ENABLE_EVT:
#if BTA_FTC_DEBUG == TRUE
            APPL_TRACE_DEBUG3("FTC Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                              ftc_state_code(in_state),
                              ftc_evt_code(p_msg->event));
#endif
            bta_ftc_api_enable(p_cb, (tBTA_FTC_DATA *) p_msg);
#if BTA_FTC_DEBUG == TRUE
            if (in_state != p_cb->state)
            {
                APPL_TRACE_DEBUG3("FTC State Change: [%s] -> [%s] after Event [%s]",
                              ftc_state_code(in_state),
                              ftc_state_code(p_cb->state),
                              ftc_evt_code(p_msg->event));
            }
#endif
            break;

        default:
            if (p_cb->is_enabled)
            {
                bta_ftc_sm_execute(p_cb, p_msg->event, (tBTA_FTC_DATA *) p_msg);

                if ( p_cb->state == BTA_FTC_CONN_ST )
                {
                    if (( p_cb->pm_state == BTA_FTC_PM_IDLE )
                      &&( p_cb->obx_oper != FTC_OP_NONE ))
                    {
                        /* inform power manager */
                        APPL_TRACE_DEBUG0("BTA FTC informs DM/PM busy state");
                        bta_sys_busy( BTA_ID_FTC, p_cb->app_id, p_cb->bd_addr );
                        p_cb->pm_state = BTA_FTC_PM_BUSY;
                    }
                    else if (( p_cb->pm_state == BTA_FTC_PM_BUSY )
                           &&( p_cb->obx_oper == FTC_OP_NONE ))
                    {
                        /* inform power manager */
                        APPL_TRACE_DEBUG0("BTA FTC informs DM/PM idle state");
                        bta_sys_idle( BTA_ID_FTC ,p_cb->app_id, p_cb->bd_addr);
                        p_cb->pm_state = BTA_FTC_PM_IDLE;
                    }
                }
                else if ( p_cb->state == BTA_FTC_IDLE_ST )
                {
                    /* initialize power management state */
                    p_cb->pm_state = BTA_FTC_PM_BUSY;
                }
            }
            break;
    }


    return (TRUE);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if BTA_FTC_DEBUG == TRUE

/*******************************************************************************
**
** Function         ftc_evt_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ftc_evt_code(tBTA_FTC_INT_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_FTC_API_DISABLE_EVT:
        return "BTA_FTC_API_DISABLE_EVT";
    case BTA_FTC_API_OPEN_EVT:
        return "BTA_FTC_API_OPEN_EVT";
    case BTA_FTC_API_CLOSE_EVT:
        return "BTA_FTC_API_CLOSE_EVT";
    case BTA_FTC_API_PUTFILE_EVT:
        return "BTA_FTC_API_PUTFILE_EVT";
    case BTA_FTC_API_GETFILE_EVT:
        return "BTA_FTC_API_GETFILE_EVT";
    case BTA_FTC_API_LISTDIR_EVT:
        return "BTA_FTC_API_LISTDIR_EVT";
    case BTA_FTC_API_CHDIR_EVT:
        return "BTA_FTC_API_CHDIR_EVT";
    case BTA_FTC_API_MKDIR_EVT:
        return "BTA_FTC_API_MKDIR_EVT";
    case BTA_FTC_API_REMOVE_EVT:
        return "BTA_FTC_API_REMOVE_EVT";
    case BTA_FTC_API_AUTHRSP_EVT:
        return "BTA_FTC_API_AUTHRSP_EVT";
    case BTA_FTC_API_ABORT_EVT:
        return "BTA_FTC_API_ABORT_EVT";
    case BTA_FTC_API_ACTION_EVT:
        return "BTA_FTC_API_ACTION_EVT";
    case BTA_FTC_OBX_ACTION_RSP_EVT:
        return "BTA_FTC_OBX_ACTION_RSP_EVT";
    case BTA_FTC_CI_SESSION_EVT:
        return "BTA_FTC_CI_SESSION_EVT";
    case BTA_FTC_SDP_OK_EVT:
        return "BTA_FTC_SDP_OK_EVT";
    case BTA_FTC_SDP_FAIL_EVT:
        return "BTA_FTC_SDP_FAIL_EVT";
    case BTA_FTC_SDP_NEXT_EVT:
        return "BTA_FTC_SDP_NEXT_EVT";
    case BTA_FTC_CI_WRITE_EVT:
        return "BTA_FTC_CI_WRITE_EVT";
    case BTA_FTC_CI_READ_EVT:
        return "BTA_FTC_CI_READ_EVT";
    case BTA_FTC_CI_OPEN_EVT:
        return "BTA_FTC_CI_OPEN_EVT";
    case BTA_FTC_OBX_CONN_RSP_EVT:
        return "BTA_FTC_OBX_CONN_RSP_EVT";
    case BTA_FTC_OBX_ABORT_RSP_EVT:
        return "BTA_FTC_OBX_ABORT_RSP_EVT";
    case BTA_FTC_OBX_TOUT_EVT:
        return "BTA_FTC_OBX_TOUT_EVT";
    case BTA_FTC_OBX_PASSWORD_EVT:
        return "BTA_FTC_OBX_PASSWORD_EVT";
    case BTA_FTC_OBX_CLOSE_EVT:
        return "BTA_FTC_OBX_CLOSE_EVT";
    case BTA_FTC_OBX_PUT_RSP_EVT:
        return "BTA_FTC_OBX_PUT_RSP_EVT";
    case BTA_FTC_CLOSE_CMPL_EVT:
        return "BTA_FTC_CLOSE_CMPL_EVT";
    case BTA_FTC_OBX_GET_RSP_EVT:
        return "BTA_FTC_OBX_GET_RSP_EVT";
    case BTA_FTC_OBX_SETPATH_RSP_EVT:
        return "BTA_FTC_OBX_SETPATH_RSP_EVT";
    case BTA_FTC_OBX_CMPL_EVT:
        return "BTA_FTC_OBX_CMPL_EVT";
    case BTA_FTC_DISABLE_CMPL_EVT:
        return "BTA_FTC_DISABLE_CMPL_EVT";
    case BTA_FTC_RSP_TOUT_EVT:
        return "BTA_FTC_RSP_TOUT_EVT";
    case BTA_FTC_API_ENABLE_EVT:
        return "BTA_FTC_API_ENABLE_EVT";
    default:
        return "unknown FTC event code";
    }
}

/*******************************************************************************
**
** Function         ftc_state_code
**
** Description      
**
** Returns          void
**
*******************************************************************************/
static char *ftc_state_code(tBTA_FTC_STATE state_code)
{
    switch(state_code)
    {
    case BTA_FTC_IDLE_ST:
        return "BTA_FTC_IDLE_ST";
    case BTA_FTC_W4_CONN_ST:
        return "BTA_FTC_W4_CONN_ST";
    case BTA_FTC_CONN_ST:
        return "BTA_FTC_CONN_ST";
    case BTA_FTC_SUSPENDING_ST:
        return "BTA_FTC_SUSPENDING_ST";
    case BTA_FTC_CLOSING_ST:
        return "BTA_FTC_CLOSING_ST";
    default:
        return "unknown FTC state code";
    }
}

#endif  /* Debug Functions */
#endif /* BTA_FT_INCLUDED */
