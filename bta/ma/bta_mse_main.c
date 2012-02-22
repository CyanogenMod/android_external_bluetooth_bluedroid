/*****************************************************************************
**
**  Name:           bta_mse_main.c
**
**  Description:    This file contains the Message Access Server main functions 
**                  and state machine.
**
**  Copyright (c) 1998-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED == TRUE)

#include <string.h>

#include "bta_fs_api.h"
#include "bta_mse_int.h"
#include "gki.h"
#include "utl.h"
#include "obx_api.h"

/*****************************************************************************
** Message Access Server State Table
*****************************************************************************/
/*****************************************************************************
** Constants and types
*****************************************************************************/
/* state machine action enumeration list for MAS */
enum
{
    BTA_MSE_MA_INT_CLOSE,
    BTA_MSE_MA_API_ACCESSRSP,
    BTA_MSE_MA_API_UPD_IBX_RSP,
    BTA_MSE_MA_API_SET_NOTIF_REG_RSP,
    BTA_MSE_MA_CI_GET_FENTRY,
    BTA_MSE_MA_CI_GET_ML_INFO,
    BTA_MSE_MA_CI_GET_ML_ENTRY,
    BTA_MSE_MA_CI_GET_MSG,
    BTA_MSE_MA_CI_PUSH_MSG,
    BTA_MSE_MA_CI_DEL_MSG,
    BTA_MSE_MA_OBX_CONNECT,
    BTA_MSE_MA_OBX_DISC,
    BTA_MSE_MA_OBX_CLOSE,
    BTA_MSE_MA_OBX_ABORT,
    BTA_MSE_MA_OBX_PUT,
    BTA_MSE_MA_OBX_GET,
    BTA_MSE_MA_OBX_SETPATH,
    BTA_MSE_MA_CONN_ERR_RSP,
    BTA_MSE_MA_CLOSE_COMPLETE,
    BTA_MSE_MA_IGNORE
};

/* type for action functions */
typedef void (*tBTA_MSE_MA_ACTION)(UINT8 inst_idx, UINT8 sess_idx, tBTA_MSE_DATA *p_data);

/* action function list for MAS */
const tBTA_MSE_MA_ACTION bta_mse_ma_action[] =
{
    bta_mse_ma_int_close,
    bta_mse_ma_api_accessrsp,
    bta_mse_ma_api_upd_ibx_rsp,
    bta_mse_ma_api_set_notif_reg_rsp,
    bta_mse_ma_ci_get_folder_entry,
    bta_mse_ma_ci_get_ml_info,
    bta_mse_ma_ci_get_ml_entry,
    bta_mse_ma_ci_get_msg,
    bta_mse_ma_ci_push_msg,
    bta_mse_ma_ci_del_msg,
    bta_mse_ma_obx_connect,
    bta_mse_ma_obx_disc,
    bta_mse_ma_obx_close,
    bta_mse_ma_obx_abort,
    bta_mse_ma_obx_put,
    bta_mse_ma_obx_get,
    bta_mse_ma_obx_setpath,
    bta_mse_ma_conn_err_rsp,
    bta_mse_ma_close_complete,
};


/* state table information */
#define BTA_MSE_MA_ACTIONS             1       /* number of actions */
#define BTA_MSE_MA_NEXT_STATE          1       /* position of next state */
#define BTA_MSE_MA_NUM_COLS            2       /* number of columns in state tables */


/* state table for MAS idle state */
static const UINT8 bta_mse_ma_st_idle[][BTA_MSE_MA_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_MSE_INT_CLOSE_EVT     */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_API_ACCESSRSP_EVT */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_API_UPD_IBX_RSP_EVT*/       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_API_SET_NOTIF_REG_RSP_EVT*/ {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_INT_START_EVT  */           {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},

/* BTA_MSE_CI_GET_FENTRY_EVT  */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CI_GET_ML_INFO_EVT */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CI_GET_ML_ENTRY_EVT*/       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CI_GET_MSG_EVT     */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CI_PUSH_MSG_EVT    */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CI_DEL_MSG_EVT     */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},

/* BTA_MSE_MA_OBX_CONN_EVT   */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_DISC_EVT   */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_ABORT_EVT  */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_CLOSE_EVT  */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_PUT_EVT    */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_GET_EVT    */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_MA_OBX_SETPATH_EVT*/        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
/* BTA_MSE_CLOSE_CMPL_EVT    */        {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_IDLE_ST},
};

/* state table for obex/rfcomm connection state */
static const UINT8 bta_mse_ma_st_listen[][BTA_MSE_MA_NUM_COLS] =
{
/* Event                          Action 1               Next state */
/* BTA_MSE_INT_CLOSE_EVT     */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_API_ACCESSRSP_EVT */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_API_UPD_IBX_RSP_EVT*/      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_API_SET_NOTIF_REG_RSP_EVT*/{BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_INT_START_EVT  */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_GET_FENTRY_EVT  */      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_GET_ML_INFO_EVT */      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_GET_ML_ENTRY_EVT*/      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_GET_MSG_EVT     */      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_PUSH_MSG_EVT    */      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CI_DEL_MSG_EVT     */      {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_CONN_EVT   */       {BTA_MSE_MA_OBX_CONNECT,  BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_DISC_EVT   */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_ABORT_EVT  */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_CLOSE_EVT  */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_PUT_EVT    */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_GET_EVT    */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_MA_OBX_SETPATH_EVT*/       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
/* BTA_MSE_CLOSE_CMPL_EVT    */       {BTA_MSE_MA_IGNORE,       BTA_MSE_MA_LISTEN_ST},
};

/* state table for open state */
static const UINT8 bta_mse_ma_st_connected[][BTA_MSE_MA_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_MSE_INT_CLOSE_EVT     */       {BTA_MSE_MA_INT_CLOSE,            BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_API_ACCESSRSP_EVT */       {BTA_MSE_MA_API_ACCESSRSP,        BTA_MSE_MA_CONN_ST},
/* BTA_MSE_API_UPD_IBX_RSP_EVT*/      {BTA_MSE_MA_API_UPD_IBX_RSP,      BTA_MSE_MA_CONN_ST},
/* BTA_MSE_API_SET_NOTIF_REG_RSP_EVT*/{BTA_MSE_MA_API_SET_NOTIF_REG_RSP,BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_INT_START_EVT  */       {BTA_MSE_MA_IGNORE,               BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_GET_FENTRY_EVT  */      {BTA_MSE_MA_CI_GET_FENTRY,        BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_GET_ML_INFO_EVT */      {BTA_MSE_MA_CI_GET_ML_INFO,       BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_GET_ML_ENTRY_EVT*/      {BTA_MSE_MA_CI_GET_ML_ENTRY,      BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_GET_MSG_EVT     */      {BTA_MSE_MA_CI_GET_MSG,           BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_PUSH_MSG_EVT    */      {BTA_MSE_MA_CI_PUSH_MSG,          BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CI_DEL_MSG_EVT     */      {BTA_MSE_MA_CI_DEL_MSG,           BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_CONN_EVT   */       {BTA_MSE_MA_CONN_ERR_RSP,         BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_DISC_EVT   */       {BTA_MSE_MA_OBX_DISC,             BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_ABORT_EVT  */       {BTA_MSE_MA_OBX_ABORT,            BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_CLOSE_EVT  */       {BTA_MSE_MA_OBX_CLOSE,            BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_PUT_EVT    */       {BTA_MSE_MA_OBX_PUT,              BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_GET_EVT    */       {BTA_MSE_MA_OBX_GET,              BTA_MSE_MA_CONN_ST},
/* BTA_MSE_MA_OBX_SETPATH_EVT*/       {BTA_MSE_MA_OBX_SETPATH,          BTA_MSE_MA_CONN_ST},
/* BTA_MSE_CLOSE_CMPL_EVT    */       {BTA_MSE_MA_IGNORE,               BTA_MSE_MA_CONN_ST},
};

/* state table for closing state */
static const UINT8 bta_mse_ma_st_closing[][BTA_MSE_MA_NUM_COLS] =
{
/* Event                          Action 1                  Next state */
/* BTA_MSE_INT_CLOSE_EVT     */       {BTA_MSE_MA_INT_CLOSE,       BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_API_ACCESSRSP_EVT */       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_API_UPD_IBX_RSP_EVT*/      {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_API_SET_NOTIF_REG_RSP_EVT*/{BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_INT_START_EVT  */       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_GET_FENTRY_EVT */       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_GET_ML_INFO_EVT*/       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_GET_ML_ENTRY_EVT*/      {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_GET_MSG_EVT    */       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_PUSH_MSG_EVT   */       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CI_DEL_MSG_EVT    */       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_CONN_EVT   */       {BTA_MSE_MA_CONN_ERR_RSP,    BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_DISC_EVT   */       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_ABORT_EVT  */       {BTA_MSE_MA_OBX_ABORT,       BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_CLOSE_EVT  */       {BTA_MSE_MA_OBX_CLOSE,       BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_PUT_EVT    */       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_GET_EVT    */       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_MA_OBX_SETPATH_EVT*/       {BTA_MSE_MA_IGNORE,          BTA_MSE_MA_CLOSING_ST},
/* BTA_MSE_CLOSE_CMPL_EVT    */       {BTA_MSE_MA_CLOSE_COMPLETE,  BTA_MSE_MA_LISTEN_ST},
};

/* type for state table MAS */
typedef const UINT8 (*tBTA_MSE_MA_ST_TBL)[BTA_MSE_MA_NUM_COLS];

/* MAS state table */
const tBTA_MSE_MA_ST_TBL bta_mse_ma_st_tbl[] =
{
    bta_mse_ma_st_idle,
    bta_mse_ma_st_listen,
    bta_mse_ma_st_connected,
    bta_mse_ma_st_closing
};

/*****************************************************************************
** Message Notification Client State Table
*****************************************************************************/
/*****************************************************************************
** Constants and types
*****************************************************************************/
/* state machine action enumeration list for MNC */
/* The order of this enumeration must be the same as bta_mse_mn_act_tbl[] */
enum
{
    BTA_MSE_MN_INIT_SDP,
    BTA_MSE_MN_START_CLIENT,
    BTA_MSE_MN_STOP_CLIENT,
    BTA_MSE_MN_OBX_CONN_RSP,
    BTA_MSE_MN_CLOSE,
    BTA_MSE_MN_SEND_NOTIF,
    BTA_MSE_MN_RSP_TIMEOUT,
    BTA_MSE_MN_PUT_RSP,
    BTA_MSE_MN_OBX_TOUT,
    BTA_MSE_MN_CLOSE_COMPL,
    BTA_MSE_MN_ABORT,
    BTA_MSE_MN_ABORT_RSP,
    BTA_MSE_MN_SDP_FAIL,
    BTA_MSE_MN_IGNORE_OBX,
    BTA_MSE_MN_IGNORE
};

typedef void (*tBTA_MSE_MN_ACTION)(UINT8 ccb_idx, tBTA_MSE_DATA *p_data);

static const tBTA_MSE_MN_ACTION bta_mse_mn_action[] = 
{
    bta_mse_mn_init_sdp,
    bta_mse_mn_start_client,
    bta_mse_mn_stop_client,
    bta_mse_mn_obx_conn_rsp,
    bta_mse_mn_close,
    bta_mse_mn_send_notif,
    bta_mse_mn_rsp_timeout,
    bta_mse_mn_put_rsp,
    bta_mse_mn_obx_tout,
    bta_mse_mn_close_compl,
    bta_mse_mn_abort,
    bta_mse_mn_abort_rsp,
    bta_mse_mn_sdp_fail,
    bta_mse_mn_ignore_obx
};


/* state table information */
#define BTA_MSE_MN_ACTIONS             1       /* number of actions */
#define BTA_MSE_MN_ACTION_COL          0       /* position of action */
#define BTA_MSE_MN_NEXT_STATE          1       /* position of next state */
#define BTA_MSE_MN_NUM_COLS            2       /* number of columns in state tables */

/* state table for idle state */
static const UINT8 bta_mse_mn_st_idle[][BTA_MSE_MN_NUM_COLS] =
{
/* Event                                Action 1                Next state */
/* BTA_MSE_MN_INT_OPEN_EVT      */   {BTA_MSE_MN_INIT_SDP,      BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_INT_CLOSE_EVT     */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_SDP_OK_EVT        */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_SDP_FAIL_EVT      */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_CONN_RSP_EVT  */   {BTA_MSE_MN_IGNORE_OBX,    BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_PUT_RSP_EVT   */   {BTA_MSE_MN_IGNORE_OBX,    BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_CLOSE_EVT     */   {BTA_MSE_MN_IGNORE_OBX,    BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_TOUT_EVT      */   {BTA_MSE_MN_IGNORE_OBX,    BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_CLOSE_CMPL_EVT    */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_API_SEND_NOTIF_EVT   */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_API_MN_ABORT_EVT     */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_ABORT_RSP_EVT */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_RSP_TOUT_EVT      */   {BTA_MSE_MN_IGNORE,        BTA_MSE_MN_IDLE_ST}
};


/* state table for wait for authentication state */
static const UINT8 bta_mse_mn_st_w4_conn[][BTA_MSE_MN_NUM_COLS] =
{
/* Event                                Action 1                Next state */
/* BTA_MSE_MN_INT_OPEN_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_INT_CLOSE_EVT     */   {BTA_MSE_MN_STOP_CLIENT,     BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_SDP_OK_EVT        */   {BTA_MSE_MN_START_CLIENT,    BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_SDP_FAIL_EVT      */   {BTA_MSE_MN_SDP_FAIL,        BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_OBX_CONN_RSP_EVT  */   {BTA_MSE_MN_OBX_CONN_RSP,    BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_OBX_PUT_RSP_EVT   */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_OBX_CLOSE_EVT     */   {BTA_MSE_MN_CLOSE,           BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_TOUT_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_CLOSE_CMPL_EVT    */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_API_SEND_NOTIF_EVT   */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_API_MN_ABORT_EVT     */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_OBX_ABORT_RSP_EVT */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST},
/* BTA_MSE_MN_RSP_TOUT_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_W4_CONN_ST}
};

/* state table for connected state */
static const UINT8 bta_mse_mn_st_connected[][BTA_MSE_MN_NUM_COLS] =
{
/* Event                                Action 1                Next state */
/* BTA_MSE_MN_INT_OPEN_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_INT_CLOSE_EVT     */   {BTA_MSE_MN_STOP_CLIENT,     BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_SDP_OK_EVT        */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_SDP_FAIL_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_OBX_CONN_RSP_EVT  */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_OBX_PUT_RSP_EVT   */   {BTA_MSE_MN_PUT_RSP,         BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_OBX_CLOSE_EVT     */   {BTA_MSE_MN_CLOSE,           BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_TOUT_EVT      */   {BTA_MSE_MN_OBX_TOUT,        BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_CLOSE_CMPL_EVT    */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CONN_ST},
/* BTA_MSE_API_SEND_NOTIF_EVT   */   {BTA_MSE_MN_SEND_NOTIF,      BTA_MSE_MN_CONN_ST},
/* BTA_MSE_API_MN_ABORT_EVT     */   {BTA_MSE_MN_ABORT,           BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_OBX_ABORT_RSP_EVT */   {BTA_MSE_MN_ABORT_RSP,       BTA_MSE_MN_CONN_ST},
/* BTA_MSE_MN_RSP_TOUT_EVT      */   {BTA_MSE_MN_RSP_TIMEOUT,     BTA_MSE_MN_CLOSING_ST}
};

/* state table for closing state */
static const UINT8 bta_mse_mn_st_closing[][BTA_MSE_MN_NUM_COLS] =
{
/* Event                                Action 1                Next state */
/* BTA_MSE_MN_INT_OPEN_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_INT_CLOSE_EVT     */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_SDP_OK_EVT        */   {BTA_MSE_MN_CLOSE_COMPL,     BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_SDP_FAIL_EVT      */   {BTA_MSE_MN_CLOSE_COMPL,     BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_MN_OBX_CONN_RSP_EVT  */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_PUT_RSP_EVT   */   {BTA_MSE_MN_IGNORE_OBX,      BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_CLOSE_EVT     */   {BTA_MSE_MN_CLOSE,           BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_TOUT_EVT      */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_CLOSE_CMPL_EVT    */   {BTA_MSE_MN_CLOSE_COMPL,     BTA_MSE_MN_IDLE_ST},
/* BTA_MSE_API_SEND_NOTIF_EVT   */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_API_MN_ABORT_EVT     */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_OBX_ABORT_RSP_EVT */   {BTA_MSE_MN_IGNORE,          BTA_MSE_MN_CLOSING_ST},
/* BTA_MSE_MN_RSP_TOUT_EVT      */   {BTA_MSE_MN_RSP_TIMEOUT,     BTA_MSE_MN_CLOSING_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_MSE_MN_ST_TBL)[BTA_MSE_MN_NUM_COLS];

/* state table */
const tBTA_MSE_MN_ST_TBL bta_mse_mn_st_tbl[] =
{
    bta_mse_mn_st_idle,
    bta_mse_mn_st_w4_conn,
    bta_mse_mn_st_connected,
    bta_mse_mn_st_closing
};
/*****************************************************************************
** Global data
*****************************************************************************/

/* MSE control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_MSE_CB  bta_mse_cb;
#endif

#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)

static char *bta_mse_evt_code(tBTA_MSE_INT_EVT evt_code);
static char *bta_mse_ma_state_code(tBTA_MSE_MA_STATE state_code);
static char *bta_mse_mn_state_code(tBTA_MSE_MN_STATE state_code);
#endif

/*******************************************************************************
**
** Function         bta_mse_ma_sm_execute
**
** Description      State machine event handling function for MA
** 
** Parameters       inst_idx    - Index to the MA instance control block
**                  sess_idx    - Index to the MA session control block
**                  event       - MA event
**                  p_data      - Pointer to the event data 
**                  
** Returns          void
**
*******************************************************************************/
void bta_mse_ma_sm_execute(UINT8 inst_idx, UINT8 sess_idx, UINT16 event, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MA_ST_TBL  state_table;
    UINT8               action;
    int                 i;
    tBTA_MSE_MA_SESS_CB *p_cb  = &(bta_mse_cb.scb[inst_idx].sess_cb[sess_idx]);

#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    tBTA_MSE_MA_STATE in_state = p_cb->state;
    UINT16             cur_evt = event;
    APPL_TRACE_EVENT3("MSE MA Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                      bta_mse_ma_state_code(in_state),
                      bta_mse_evt_code(cur_evt));
#endif

    /* look up the state table for the current state */
    state_table = bta_mse_ma_st_tbl[p_cb->state];

    event &= 0x00FF;

    /* set next state */
    p_cb->state = state_table[event][BTA_MSE_MA_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_MSE_MA_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_MSE_MA_IGNORE)
        {
            (*bta_mse_ma_action[action])(inst_idx, sess_idx, p_data);
        }
        else
        {
            /* discard mas data */
            bta_mse_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }
#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    if (in_state != p_cb->state)
    {
        APPL_TRACE_EVENT3("MSE MA State Change: [%s] -> [%s] after Event [%s]",
                          bta_mse_ma_state_code(in_state),
                          bta_mse_ma_state_code(p_cb->state),
                          bta_mse_evt_code(cur_evt));
    }
#endif

}

/*******************************************************************************
**
** Function         bta_mse_mn_sm_execute
**
** Description      State machine event handling function for MNC
**
** Parameters       mn_cb_idx   - Index to the MN control block
**                  event       - MN event
**                  p_data      - Pointer to the event data 
**                   
** Returns          void
**
*******************************************************************************/
void bta_mse_mn_sm_execute(UINT8 mn_cb_idx, UINT16 event, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_CB *p_cb = &(bta_mse_cb.ccb[mn_cb_idx]);
    tBTA_MSE_MN_ST_TBL     state_table;
    UINT8               action;
    int                 i;



#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    tBTA_MSE_MN_STATE in_state = p_cb->state;
    UINT16             cur_evt = event;
    APPL_TRACE_EVENT3("MSE MN Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                      bta_mse_mn_state_code(in_state),
                      bta_mse_evt_code(cur_evt));
#endif

    /* look up the state table for the current state */
    state_table = bta_mse_mn_st_tbl[p_cb->state];
    event -= BTA_MSE_MN_EVT_MIN;

    /* set next state */
    p_cb->state = state_table[event][BTA_MSE_MN_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_MSE_MN_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_MSE_MN_IGNORE)
        {
            (*bta_mse_mn_action[action])(mn_cb_idx, p_data);
        }
        else
        {
            /* discard mas data */
            bta_mse_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }


#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    if (in_state != p_cb->state)
    {
        APPL_TRACE_EVENT3("MSE MN State Change: [%s] -> [%s] after Event [%s]",
                          bta_mse_mn_state_code(in_state),
                          bta_mse_mn_state_code(p_cb->state),
                          bta_mse_evt_code(cur_evt));
    }
#endif
}
/*******************************************************************************
**
** Function         bta_mse_ma_api_enable
**
** Description      Process API enable request to enable MCE subsystem
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**                  
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_enable(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE    evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_enable" );
#endif
    /* If already enabled then reject this request */
    if (p_cb->enable)
    {
        APPL_TRACE_ERROR0("MSE is already enabled");
        evt_data.enable.app_id = p_data->api_enable.app_id; 
        evt_data.enable.status = BTA_MA_STATUS_FAIL; 
        p_data->api_enable.p_cback(BTA_MSE_ENABLE_EVT, (tBTA_MSE *) &evt_data);
        return;
    }

    /* Done with checking. now perform the enable oepration*/
    /* initialize control block */
    memset(p_cb, 0, sizeof(tBTA_MSE_CB));

    p_cb->p_cback = p_data->api_enable.p_cback;
    p_cb->app_id = p_data->api_enable.app_id;
    p_cb->enable = TRUE;

    evt_data.enable.app_id = p_cb->app_id;
    evt_data.enable.status = BTA_MA_STATUS_OK; 
    p_cb->p_cback(BTA_MSE_ENABLE_EVT, (tBTA_MSE *) &evt_data);
}
/*******************************************************************************
**
** Function         bta_mse_ma_api_disable
**
** Description      Process API disable request
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**                 
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_disable(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE                evt_data;
    tBTA_MA_STATUS          status = BTA_MA_STATUS_OK;
    UINT8                   app_id = p_data->api_disable.app_id;      
    UINT8                   i,j, num_act_mas=0, num_act_sess=0;
    tBTA_MSE_MA_CB          *p_scb;
    tBTA_MSE_MA_SESS_CB     *p_sess_cb;
    BOOLEAN                 send_disable_evt =FALSE;
    tBTA_MSE_MN_CB          *p_ccb;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_disable" );
#endif

    if (p_cb->enable)
    {
        /* close all MN connections */
        for (i=0; i < BTA_MSE_NUM_MN; i ++)
        {
            p_ccb = BTA_MSE_GET_MN_CB_PTR(i);
            if (p_ccb->in_use &&
                (p_ccb->state != BTA_MSE_MN_IDLE_ST))
            {
                bta_mse_mn_remove_all_inst_ids(i);
                bta_mse_mn_sm_execute(i,BTA_MSE_MN_INT_CLOSE_EVT, NULL);  
            }
        }

        for (i=0; i < BTA_MSE_NUM_INST ; i ++)
        {
            p_scb = BTA_MSE_GET_INST_CB_PTR(i);
            if (p_scb->in_use )
            {
                num_act_mas++;
                p_cb->disabling = TRUE;
                p_scb->stopping = TRUE;
                num_act_sess =0; 
                for (j=0; j<BTA_MSE_NUM_SESS; j++ )
                {
                    p_sess_cb = BTA_MSE_GET_SESS_CB_PTR(i, j);
                    if (p_sess_cb->state != BTA_MSE_MA_LISTEN_ST)
                    {
                        bta_mse_ma_sm_execute(i, j, BTA_MSE_INT_CLOSE_EVT, p_data); 
                        num_act_sess++;
                    }
                }

                if (!num_act_sess)
                {
                    evt_data.stop.status = BTA_MA_STATUS_OK;
                    evt_data.stop.mas_instance_id = p_scb->mas_inst_id;
                    p_cb->p_cback(BTA_MSE_STOP_EVT, (tBTA_MSE *) &evt_data);
                    bta_mse_clean_mas_service(i);
                    num_act_mas--;
                }
            }
        }

        if (!num_act_mas)
        {
            send_disable_evt = TRUE;
            p_cb->enable = FALSE;
        }
    }
    else
    {
        send_disable_evt = TRUE;
        status = BTA_MA_STATUS_FAIL; 
    }


    if (send_disable_evt)
    {
        evt_data.enable.app_id = app_id; 
        evt_data.enable.status = status; 
        p_cb->p_cback(BTA_MSE_DISABLE_EVT, (tBTA_MSE *) &evt_data);
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("MSE Disabled location-1");
#endif
    }

}
/*******************************************************************************
**
** Function         bta_mse_ma_api_start
**
** Description      Process API MA server start request
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**                    
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_start(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    UINT8               i = 0;
    tBTA_MSE            evt_data; /* event call back */
    tBTA_MSE_MA_CB      *p_scb = NULL;  /* MA instance control block*/
    tOBX_StartParams    start_msg;
    tBTA_MSE_API_START  *p_api = &p_data->api_start;
    tOBX_TARGET         target;
    tOBX_STATUS         status;
    tBTA_UTL_COD        cod;
    UINT8               mas_instance_idx;
    UINT16              root_folder_len;
    tBTA_MA_STATUS      ma_status = BTA_MA_STATUS_OK;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_start" );
#endif
    if (!p_cb->enable || ((p_cb->enable) && bta_mse_is_a_duplicate_id(p_api->mas_inst_id)) )
    {
        ma_status = BTA_MA_STATUS_DUPLICATE_ID;
    }

    if ((ma_status == BTA_MA_STATUS_OK) && bta_mse_find_avail_mas_inst_cb_idx(&mas_instance_idx))
    {
        APPL_TRACE_EVENT1("bta_mse_ma_api_start inst_idx=%d",mas_instance_idx );  

        /* initialize the MA Instance control block */
        p_scb = &(p_cb->scb[mas_instance_idx]);
        memset(p_scb, 0, sizeof(tBTA_MSE_MA_CB));

        p_scb->in_use = TRUE;
        BCM_STRNCPY_S(p_scb->servicename, sizeof(p_scb->servicename), p_api->servicename, BTA_SERVICE_NAME_LEN);
        p_scb->mas_inst_id = p_api->mas_inst_id;              
        p_scb->sec_mask = p_api->sec_mask;
        p_scb->sup_msg_type = p_api->sup_msg_type;

        /* If directory is specified set the length */
        root_folder_len = p_bta_fs_cfg->max_path_len + 1;

        /* Allocate an aligned memory buffer to hold the root path in the  */
        /* MAS instance cb as well as the session cbs                      */
        /* Add 1 byte for '\0' */
        if ((p_scb->p_rootpath = (char *)GKI_getbuf((UINT16)(root_folder_len+1))) != NULL)
        {
            memcpy(target.target, BTA_MAS_MESSAGE_ACCESS_TARGET_UUID, BTA_MAS_UUID_LENGTH);
            target.len = BTA_MAS_UUID_LENGTH;
            p_scb->scn = BTM_AllocateSCN();
            BCM_STRNCPY_S(p_scb->p_rootpath, root_folder_len+1, p_api->p_root_path, root_folder_len);
            p_scb->p_rootpath[root_folder_len] = '\0';

            /* Register MAP security requirements with BTM */
            BTM_SetSecurityLevel(FALSE, p_api->servicename, BTM_SEC_SERVICE_MAP,
                                 p_api->sec_mask, BT_PSM_RFCOMM,
                                 BTM_SEC_PROTO_RFCOMM, (UINT32)p_scb->scn);

            /* Start up the MSE service */
            memset(&start_msg, 0, sizeof(tOBX_StartParams));
            start_msg.p_target = &target;
            /* Make the MTU fit into one RFC frame */
            start_msg.mtu = OBX_MAX_MTU;
            start_msg.scn = p_scb->scn;
            start_msg.authenticate = FALSE; /* OBX authentication is disabled */
            start_msg.auth_option = (UINT8) NULL;
            start_msg.p_cback = bta_mse_ma_obx_cback;
            start_msg.realm_len =(UINT8) NULL;
            start_msg.p_realm = (UINT8) NULL;
            start_msg.realm_charset = (tOBX_CHARSET) NULL;
            start_msg.max_sessions = BTA_MSE_NUM_SESS; 

            if ((status = OBX_StartServer (&start_msg, &p_scb->obx_handle)) == OBX_SUCCESS)
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT1("Obx start for MAS server: obx_hdl=%d", p_scb->obx_handle);    
#endif
                /* initialze all session states */
                for (i=0; i<BTA_MSE_NUM_SESS; i++ )
                {
                    p_scb->sess_cb[i].state = BTA_MSE_MA_LISTEN_ST;
                }

                if (ma_status == BTA_MA_STATUS_OK)
                {
                    /* Set the File Transfer service class bit */
                    cod.service = BTM_COD_SERVICE_OBJ_TRANSFER;
                    utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);

                    /* Set up the SDP record for pbs service */
                    bta_mse_ma_sdp_register(p_scb, p_api->servicename, p_scb->mas_inst_id, p_scb->sup_msg_type );
                }
                else
                {
                    /* release resources */
                    OBX_StopServer(p_scb->obx_handle);
                    for (i=0; i < BTA_MSE_NUM_SESS; i++  )
                    {
                        utl_freebuf((void**)&p_scb->sess_cb[i].p_workdir );   
                    }
                }
            }
            else
            {
                /* release resources */
                utl_freebuf((void**)&p_scb->p_rootpath);     
                APPL_TRACE_ERROR1("OBX_StartServer returns error (%d)", status);
                ma_status = BTA_MA_STATUS_NO_RESOURCE;
            }
        }
        else    /* Cannot allocate resources to run Server */
        {
            APPL_TRACE_ERROR0("Not enough Resources to run MSE Server");
            ma_status = BTA_MA_STATUS_NO_RESOURCE;
        }

        if (ma_status != BTA_MA_STATUS_OK)
        {
            /* clearn up the contrl block*/
            memset(p_scb, 0, sizeof(tBTA_MSE_MA_CB));
        }
    }

    evt_data.start.mas_instance_id = p_api->mas_inst_id;
    evt_data.start.status = ma_status;
    p_cb->p_cback(BTA_MSE_START_EVT, (tBTA_MSE *) &evt_data);

}
/*******************************************************************************
**
** Function         bta_mse_ma_api_stop
**
** Description      Process API MA server stop request
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
** 
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_stop(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_API_STOP       *p_stop = &p_data->api_stop;
    BOOLEAN                 send_stop_evt = FALSE;
    tBTA_MA_STATUS          status = BTA_MA_STATUS_OK;
    tBTA_MSE_MA_CB          *p_scb;
    tBTA_MSE_MA_SESS_CB     *p_sess_cb;
    tBTA_MSE_MN_CB          *p_ccb;
    UINT8                   i, inst_idx;
    UINT8                   num_act_sess=0;
    tBTA_MSE                evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_stop" );
#endif


    if (bta_mse_find_mas_inst_id_match_cb_idx(p_stop->mas_inst_id, &inst_idx))
    {
        p_scb =  BTA_MSE_GET_INST_CB_PTR(inst_idx);
        p_scb->stopping = TRUE;

        for (i=0; i < BTA_MSE_NUM_MN; i ++)
        {
            p_ccb = BTA_MSE_GET_MN_CB_PTR(i);
            if (p_ccb->in_use &&
                (p_ccb->state != BTA_MSE_MN_IDLE_ST) &&
                bta_mse_mn_is_ok_to_close_mn(p_ccb->bd_addr,p_scb->mas_inst_id))
            {
                /* close the MN connection if mas_inst_id is the last active inst_id */
                bta_mse_mn_remove_inst_id(i, p_scb->mas_inst_id);
                bta_mse_mn_sm_execute(i,BTA_MSE_MN_INT_CLOSE_EVT, NULL);  
            }
        }
        /* close all active session */
        for (i=0; i < BTA_MSE_NUM_SESS; i ++)
        {
            p_sess_cb = &(p_scb->sess_cb[i]);
            if (p_sess_cb->state != BTA_MSE_MA_LISTEN_ST)
            {
                APPL_TRACE_EVENT2("Send API CLOSE to SM for STOP sess ind=%d state=%d",i,p_sess_cb->state);
                bta_mse_ma_sm_execute(inst_idx, i, BTA_MSE_INT_CLOSE_EVT, p_data); 
                num_act_sess++;
            }
        }

        if (!num_act_sess)
        {
            bta_mse_clean_mas_service(inst_idx);
            send_stop_evt = TRUE;
        }
    }
    else
    {
        send_stop_evt = TRUE;
        status = BTA_MA_STATUS_FAIL;
    }

    if (send_stop_evt)
    {
        evt_data.stop.status = status;
        evt_data.stop.mas_instance_id = p_stop->mas_inst_id;
        p_cb->p_cback(BTA_MSE_STOP_EVT, (tBTA_MSE *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_api_close
**
** Description      Process API close request. It will close all MA 
**                  sesions on the specified MAS instance ID 
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
** 
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_close(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_API_CLOSE      *p_close = &p_data->api_close;
    BOOLEAN                 send_close_evt = FALSE;
    tBTA_MA_STATUS          status = BTA_MA_STATUS_OK;
    tBTA_MSE_MA_CB          *p_scb;
    tBTA_MSE_MA_SESS_CB     *p_sess_cb;
    UINT8                   i, inst_idx;
    UINT8                   num_act_sess=0;
    tBTA_MSE                evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_close" );
#endif
    if (bta_mse_find_mas_inst_id_match_cb_idx(p_close->mas_instance_id, &inst_idx))
    {
        p_scb =  BTA_MSE_GET_INST_CB_PTR(inst_idx);
        /* close all active sessions for the specified MA Server */
        for (i=0; i < BTA_MSE_NUM_SESS; i ++)
        {
            p_sess_cb = &(p_scb->sess_cb[i]);
            if (p_sess_cb->state != BTA_MSE_MA_LISTEN_ST)
            {
#if BTA_MSE_DEBUG == TRUE
                APPL_TRACE_EVENT4("Disconnect inst_id=%d, sess_id=%d, inst_idx=%d, sess_idx=%d",
                                  p_close->mas_instance_id,
                                  p_sess_cb->obx_handle,
                                  inst_idx,
                                  i);  
#endif

                bta_mse_ma_sm_execute(inst_idx, i, BTA_MSE_INT_CLOSE_EVT, p_data); 
                num_act_sess++;
            }
        }

        if (!num_act_sess)
        {
            send_close_evt = TRUE;
        }
    }
    else
    {
        send_close_evt = TRUE;
        status = BTA_MA_STATUS_FAIL;
    }

    if (send_close_evt)
    {
        evt_data.ma_close.status = status;
        evt_data.ma_close.mas_session_id = 0;   
        evt_data.ma_close.mas_instance_id = p_close->mas_instance_id ;
        p_cb->p_cback(BTA_MSE_MA_CLOSE_EVT, (tBTA_MSE *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_api_ma_close
**
** Description      Process API MA close request. It will close the 
**                  specified MA session. 
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_mse_ma_api_ma_close(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_API_MA_CLOSE   *p_ma_close = &p_data->api_ma_close;
    UINT8                   inst_idx, sess_idx;
    tBTA_MSE                evt_data;


#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_ma_api_ma_close" );
#endif

    if (bta_mse_find_mas_inst_id_match_cb_idx (p_ma_close->mas_instance_id, &inst_idx)&&
        bta_mse_find_bd_addr_match_sess_cb_idx(p_ma_close->bd_addr,inst_idx,&sess_idx))
    {
        bta_mse_ma_sm_execute(inst_idx, sess_idx, BTA_MSE_INT_CLOSE_EVT, p_data); 
    }
    else
    {
        evt_data.ma_close.mas_session_id  = 0xFF;
        evt_data.ma_close.mas_instance_id = p_ma_close->mas_instance_id;
        evt_data.ma_close.status           = BTA_MA_STATUS_FAIL ;
        p_cb->p_cback(BTA_MSE_MA_CLOSE_EVT, (tBTA_MSE *) &evt_data);
    }
}


/*******************************************************************************
**
** Function         bta_mse_mn_api_close
**
** Description      Process API MN close request.   
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**
** Returns          void
**
*******************************************************************************/
static void bta_mse_mn_api_close(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_API_MN_CLOSE   *p_mn_close = &p_data->api_mn_close;
    UINT8                   ccb_idx;
    tBTA_MSE                evt_data;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_api_close" );
#endif

    if (bta_mse_find_bd_addr_match_mn_cb_index(p_mn_close->bd_addr, &ccb_idx))
    {
        bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_INT_CLOSE_EVT, p_data); 
    }
    else
    {
        evt_data.mn_close.bd_addr[0] = '\0';   
        evt_data.mn_close.dev_name[0] = '\0';
        p_cb->p_cback(BTA_MSE_MN_CLOSE_EVT, (tBTA_MSE *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_mse_mn_api_send_notif
**
** Description      Process API send message notification report to all MCEs 
**                  registered with the specified MAS instance ID                  
**
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**
** Returns          void
**
*******************************************************************************/
static void bta_mse_mn_api_send_notif(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_API_SEND_NOTIF      *p_mn_send_notif = &p_data->mn_send_notif;
    UINT8                           i;
    tBTA_MSE                        evt_data;
    tBTA_MSE_MN_CB                  *p_ccb;
    UINT8                           active_sess_cnt;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_api_send_notif" );
#endif

    active_sess_cnt =0 ;
    for (i=0; i < BTA_MSE_NUM_MN; i ++)
    {
        p_ccb = BTA_MSE_GET_MN_CB_PTR(i);

        if (p_ccb->in_use &&
            (p_ccb->state == BTA_MSE_MN_CONN_ST) &&
            (memcmp(p_ccb->bd_addr, p_mn_send_notif->except_bd_addr, BD_ADDR_LEN) != 0 ) &&
            bta_mse_mn_is_inst_id_exist(i,p_mn_send_notif->mas_instance_id))
        {
            bta_mse_mn_sm_execute(i, BTA_MSE_API_SEND_NOTIF_EVT, p_data);
            active_sess_cnt++;
        }
    }

    if (!active_sess_cnt)
    {
        evt_data.send_notif.mas_instance_id = p_mn_send_notif->mas_instance_id;  
        evt_data.send_notif.status = BTA_MA_STATUS_FAIL;
        memset(evt_data.send_notif.bd_addr,0, sizeof(evt_data.send_notif.bd_addr));
        p_cb->p_cback(BTA_MSE_SEND_NOTIF_EVT, (tBTA_MSE *) &evt_data);
    }

}


/*******************************************************************************
**
** Function         bta_mse_mn_api_abort
**
** Description      Abort the current OBEX multi-packet oepration
**                  
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**
** Returns          void
**
*******************************************************************************/
static void bta_mse_mn_api_abort(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    tBTA_MSE_MN_API_ABORT           *p_mn_abort = &p_data->mn_abort;
    UINT8                           i;
    tBTA_MSE_MN_CB                  *p_ccb;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_mn_api_abort" );
#endif

    for (i=0; i < BTA_MSE_NUM_MN; i ++)
    {
        p_ccb = BTA_MSE_GET_MN_CB_PTR(i);
        if (p_ccb->in_use &&
            (p_ccb->state == BTA_MSE_MN_CONN_ST) &&
            bta_mse_mn_is_inst_id_exist(i,p_mn_abort->mas_instance_id))
        {
            bta_mse_mn_sm_execute(i, BTA_MSE_API_MN_ABORT_EVT, p_data);
        }
    }

}

/*******************************************************************************
**
** Function         bta_mse_mn_rsp_tout
**
** Description      Process MN response timer timeout event
**                  
** Parameters       p_cb   - Pointer to MSE control block
**                  p_data - Pointer to MSE event data
**
** Returns          void
**
*******************************************************************************/
static void bta_mse_mn_rsp_tout(tBTA_MSE_CB *p_cb, tBTA_MSE_DATA *p_data)
{
    UINT8                           ccb_idx;

    ccb_idx = (UINT8)(p_data->hdr.event - BTA_MSE_MN_RSP0_TOUT_EVT);

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("bta_mse_mn_rsp_tout ccd_idx=%d",ccb_idx);
#endif
    bta_mse_mn_sm_execute(ccb_idx, BTA_MSE_MN_RSP_TOUT_EVT, p_data);
}
/*******************************************************************************
**
** Function         bta_mse_hdl_event
**
** Description      MSE main event handling function.
** 
** Parameters       p_msg  - Pointer to MSE event data
**                 
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_mse_hdl_event(BT_HDR *p_msg){
    UINT8 inst_idx, sess_idx, ccb_idx;
    BOOLEAN success = TRUE;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("MSE Event Handler: Event [%s]", 
                      bta_mse_evt_code(p_msg->event));
#endif

    switch (p_msg->event)
    {
        case BTA_MSE_API_ENABLE_EVT:
            bta_mse_ma_api_enable(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_START_EVT:           
            bta_mse_ma_api_start(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_STOP_EVT:           
            bta_mse_ma_api_stop(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_DISABLE_EVT:
            bta_mse_ma_api_disable(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_CLOSE_EVT:
            bta_mse_ma_api_close(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_MA_CLOSE_EVT:
            bta_mse_ma_api_ma_close(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_MN_CLOSE_EVT:
            bta_mse_mn_api_close(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);
            break;

        case BTA_MSE_API_SEND_NOTIF_EVT:
            bta_mse_mn_api_send_notif(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);  
            break;

        case BTA_MSE_API_MN_ABORT_EVT:
            bta_mse_mn_api_abort(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);  
            break;

        case BTA_MSE_MN_RSP0_TOUT_EVT:
        case BTA_MSE_MN_RSP1_TOUT_EVT:
        case BTA_MSE_MN_RSP2_TOUT_EVT:
        case BTA_MSE_MN_RSP3_TOUT_EVT:
        case BTA_MSE_MN_RSP4_TOUT_EVT:
        case BTA_MSE_MN_RSP5_TOUT_EVT:
        case BTA_MSE_MN_RSP6_TOUT_EVT:
            bta_mse_mn_rsp_tout(&bta_mse_cb, (tBTA_MSE_DATA *) p_msg);  
            break;

        default:

            if (p_msg->event < BTA_MSE_MN_EVT_MIN)
            {
                if (bta_mse_find_ma_cb_indexes((tBTA_MSE_DATA *) p_msg, &inst_idx, &sess_idx))
                {
                    bta_mse_ma_sm_execute( inst_idx , 
                                           sess_idx , 
                                           p_msg->event, (tBTA_MSE_DATA *) p_msg);
                }
                else
                {
                    APPL_TRACE_ERROR1("unable to find inst_idx and sess_idx: event=%s",  
                                      bta_mse_evt_code(p_msg->event));
                    success = FALSE;
                    bta_mse_ma_cleanup ((tBTA_MSE_DATA *) p_msg);
                }
            }
            else
            {
                if (bta_mse_find_mn_cb_index((tBTA_MSE_DATA *) p_msg, &ccb_idx))
                {
                    bta_mse_mn_sm_execute(  ccb_idx, 
                                            p_msg->event , 
                                            (tBTA_MSE_DATA *) p_msg);
                }
                else
                {
                    APPL_TRACE_ERROR1("unable to find ccb_idx: event=%s", 
                                      bta_mse_evt_code(p_msg->event));
                    success = FALSE;
                    bta_mse_mn_cleanup ((tBTA_MSE_DATA *) p_msg);
                }
            }
            break;
    }

    return(success);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
/*******************************************************************************
**
** Function         bta_mse_ma_evt_code
**
** Description      Maps MA event code to the corresponding event string 
**
** Parameters       evt_code  - MA event code
**  
** Returns          string pointer for the associated event name
**
*******************************************************************************/
static char *bta_mse_evt_code(tBTA_MSE_INT_EVT evt_code){
    switch (evt_code)
    {
        case BTA_MSE_API_CLOSE_EVT:
            return "BTA_MSE_API_CLOSE_EVT";
        case BTA_MSE_API_MA_CLOSE_EVT:
            return "BTA_MSE_API_MA_CLOSE_EVT";
        case BTA_MSE_API_MN_CLOSE_EVT:
            return "BTA_MSE_API_MN_CLOSE_EVT";
        case BTA_MSE_INT_CLOSE_EVT:
            return "BTA_MSE_INT_CLOSE_EVT";
        case BTA_MSE_API_ACCESSRSP_EVT:
            return "BTA_MSE_API_ACCESSRSP_EVT";
        case BTA_MSE_API_UPD_IBX_RSP_EVT:
            return "BTA_MSE_API_UPD_IBX_RSP_EVT";
        case BTA_MSE_API_SET_NOTIF_REG_RSP_EVT:
            return "BTA_MSE_API_SET_NOTIF_REG_RSP_EVT";
        case BTA_MSE_INT_START_EVT:
            return "BTA_MSE_INT_START_EVT";
        case BTA_MSE_CI_GET_FENTRY_EVT:
            return "BTA_MSE_CI_GET_FENTRY_EVT";
        case BTA_MSE_CI_GET_ML_INFO_EVT:
            return "BTA_MSE_CI_GET_ML_INFO_EVT";
        case BTA_MSE_CI_GET_ML_ENTRY_EVT:
            return "BTA_MSE_CI_GET_ML_ENTRY_EVT";
        case BTA_MSE_CI_GET_MSG_EVT:
            return "BTA_MSE_CI_GET_MSG_EVT";
        case BTA_MSE_CI_PUSH_MSG_EVT:
            return "BTA_MSE_CI_PUSH_MSG_EVT";
        case BTA_MSE_CI_DEL_MSG_EVT:
            return "BTA_MSE_CI_DEL_MSG_EVT";
        case BTA_MSE_MA_OBX_CONN_EVT:        
            return "BTA_MSE_MA_OBX_CONN_EVT";
        case BTA_MSE_MA_OBX_DISC_EVT:      
            return "BTA_MSE_MA_OBX_DISC_EVT";
        case BTA_MSE_MA_OBX_ABORT_EVT:     
            return "BTA_MSE_MA_OBX_ABORT_EVT";
        case BTA_MSE_MA_OBX_CLOSE_EVT:     
            return "BTA_MSE_MA_OBX_CLOSE_EVT";
        case BTA_MSE_MA_OBX_PUT_EVT:      
            return "BTA_MSE_MA_OBX_PUT_EVT";
        case BTA_MSE_MA_OBX_GET_EVT: 
            return "BTA_MSE_MA_OBX_GET_EVT";
        case BTA_MSE_MA_OBX_SETPATH_EVT:   
            return "BTA_MSE_MA_OBX_SETPATH_EVT";
        case BTA_MSE_CLOSE_CMPL_EVT:      
            return "BTA_MSE_CLOSE_CMPL_EVT";
        case BTA_MSE_MN_INT_OPEN_EVT:     
            return "BTA_MSE_MN_INT_OPEN_EVT";
        case BTA_MSE_MN_INT_CLOSE_EVT:
            return "BTA_MSE_MN_INT_CLOSE_EVT";
        case BTA_MSE_MN_SDP_OK_EVT:
            return "BTA_MSE_MN_SDP_OK_EVT";
        case BTA_MSE_MN_SDP_FAIL_EVT:
            return "BTA_MSE_MN_SDP_FAIL_EVT";
        case BTA_MSE_MN_OBX_CONN_RSP_EVT:
            return "BTA_MSE_MN_OBX_CONN_RSP_EVT";
        case BTA_MSE_MN_OBX_PUT_RSP_EVT: 
            return "BTA_MSE_MN_OBX_PUT_RSP_EVT";
        case BTA_MSE_MN_OBX_CLOSE_EVT:   
            return "BTA_MSE_MN_OBX_CLOSE_EVT";
        case BTA_MSE_MN_OBX_TOUT_EVT:       
            return "BTA_MSE_MN_OBX_TOUT_EVT";
        case BTA_MSE_MN_CLOSE_CMPL_EVT:     
            return "BTA_MSE_MN_CLOSE_CMPL_EVT";
        case BTA_MSE_API_SEND_NOTIF_EVT:    
            return "BTA_MSE_API_SEND_NOTIF_EVT";
        case BTA_MSE_API_MN_ABORT_EVT:    
            return "BTA_MSE_API_MN_ABORT_EVT";
        case BTA_MSE_MN_OBX_ABORT_RSP_EVT:    
            return "BTA_MSE_MN_OBX_ABORT_RSP_EVT";
        case BTA_MSE_API_ENABLE_EVT:        
            return "BTA_MSE_API_ENABLE_EVT";
        case BTA_MSE_API_START_EVT:         
            return "BTA_MSE_API_START_EVT";
        case BTA_MSE_API_STOP_EVT:           
            return "BTA_MSE_API_STOP_EVT";
        default:
            return "Unknown MSE event code";
    }
}

/*******************************************************************************
**
** Function         bta_mse_ma_state_code
**
** Description      Maps MA state code to the corresponding state string     
**
** Parameters       state_code  - MA state code
** 
** Returns          string pointer for the associated state name
**
*******************************************************************************/
static char *bta_mse_ma_state_code(tBTA_MSE_MA_STATE state_code){
    switch (state_code)
    {
        case BTA_MSE_MA_IDLE_ST:
            return "BTA_MSE_MA_IDLE_ST";
        case BTA_MSE_MA_LISTEN_ST:
            return "BTA_MSE_MA_LISTEN_ST";
        case BTA_MSE_MA_CONN_ST:
            return "BTA_MSE_MA_CONN_ST";
        case BTA_MSE_MA_CLOSING_ST:
            return "BTA_MSE_MA_CLOSING_ST";
        default:
            return "Unknown MA state code";
    }
}


/*******************************************************************************
**
** Function         bta_mse_mn_state_code
**
** Description      Maps MN state code to the corresponding state string     
**
** Parameters       state_code  - MN state code
** 
** Returns          string pointer for the associated state name
**
*******************************************************************************/
static char *bta_mse_mn_state_code(tBTA_MSE_MN_STATE state_code){
    switch (state_code)
    {
        case BTA_MSE_MN_IDLE_ST:
            return "BTA_MSE_MN_IDLE_ST";
        case BTA_MSE_MN_W4_CONN_ST:
            return "BTA_MSE_MN_W4_CONN_ST";
        case BTA_MSE_MN_CONN_ST:
            return "BTA_MSE_MN_CONN_ST";
        case BTA_MSE_MN_CLOSING_ST:
            return "BTA_MSE_MN_CLOSING_ST";
        default:
            return "Unknown MN state code";
    }
}

#endif  /* Debug Functions */














#endif /* BTA_MSE_INCLUDED */
