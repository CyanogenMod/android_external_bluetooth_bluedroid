/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed 
 *  pursuant to the terms and conditions of a separate, written license 
 *  agreement executed between you and Broadcom (an "Authorized License").  
 *  Except as set forth in an Authorized License, Broadcom grants no license 
 *  (express or implied), right to use, or waiver of any kind with respect to 
 *  the Software, and Broadcom expressly reserves all rights in and to the 
 *  Software and all intellectual property rights therein.  
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS 
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
 *  ALL USE OF THE SOFTWARE.  
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization, 
 *         constitutes the valuable trade secrets of Broadcom, and you shall 
 *         use all reasonable efforts to protect the confidentiality thereof, 
 *         and to use this information only in connection with your use of 
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR 
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  Filename:      btif_av.c
 *
 *  Description:   Bluedroid AV implementation
 * 
 *****************************************************************************/
#include <hardware/bluetooth.h>
#include "hardware/bt_av.h"

#define LOG_TAG "BTIF_AV"
#include "btif_common.h"
#include "btif_sm.h"
#include "bta_api.h"
#include "bta_av_api.h"
#include "gki.h"
#include "bd.h"
#include "btu.h"

/*****************************************************************************
**  Constants & Macros
******************************************************************************/
#define BTIF_AV_SERVICE_NAME "Advanced Audio"

#define BTIF_TIMEOUT_AV_OPEN_ON_RC  2 /* 2 seconds */

typedef enum {
    BTIF_AV_STATE_IDLE = 0x0,
    BTIF_AV_STATE_OPENING,
    BTIF_AV_STATE_OPENED,
    BTIF_AV_STATE_STARTED
} btif_av_state_t;

typedef enum {
    /* Reuse BTA_AV_XXX_EVT - No need to redefine them here */
    BTIF_AV_CONNECT_REQ_EVT = BTA_AV_MAX_EVT,
    BTIF_AV_DISCONNECT_REQ_EVT,
    BTIF_AV_START_STREAM_REQ_EVT,
    BTIF_AV_STOP_STREAM_REQ_EVT,
    BTIF_AV_SUSPEND_STREAM_REQ_EVT,
    BTIF_AV_RECONFIGURE_REQ_EVT,
} btif_av_sm_event_t;
/*****************************************************************************
**  Local type definitions
******************************************************************************/
typedef struct
{
    tBTA_AV_HNDL bta_handle;
    bt_bdaddr_t peer_bda;
    btif_sm_handle_t sm_handle;
} btif_av_cb_t;
/*****************************************************************************
**  Static variables
******************************************************************************/
static btav_callbacks_t *bt_av_callbacks = NULL;
static btif_av_cb_t btif_av_cb;

static TIMER_LIST_ENT tle_av_open_on_rc;

#define CHECK_BTAV_INIT() if (bt_av_callbacks == NULL)\
{\
     BTIF_TRACE_WARNING1("%s: BTAV not initialized", __FUNCTION__);\
     return BT_STATUS_NOT_READY;\
}\
else\
{\
     BTIF_TRACE_EVENT1("%s", __FUNCTION__);\
}

/* Helper macro to avoid code duplication in the state machine handlers */
#define CHECK_RC_EVENT(e, d) \
    case BTA_AV_RC_OPEN_EVT: \
    case BTA_AV_RC_CLOSE_EVT: \
    case BTA_AV_REMOTE_CMD_EVT: \
    case BTA_AV_VENDOR_CMD_EVT: \
    case BTA_AV_META_MSG_EVT: \
    case BTA_AV_RC_FEAT_EVT: \
    { \
         btif_rc_handler(e, d);\
    }break; \

static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *data);

static const btif_sm_handler_t btif_av_state_handlers[] =
{
    btif_av_state_idle_handler,
    btif_av_state_opening_handler,
    btif_av_state_opened_handler,
    btif_av_state_started_handler
};


/*************************************************************************
** Extern functions
*************************************************************************/
extern void btif_rc_init(void);
extern void btif_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data);
extern BOOLEAN btif_rc_get_connected_peer(BD_ADDR peer_addr);

/*****************************************************************************
** Local helper functions
******************************************************************************/
const char *dump_av_sm_state_name(btif_av_state_t state)
{
   switch (state) {
      case BTIF_AV_STATE_IDLE: return "BTIF_AV_STATE_IDLE";
      case BTIF_AV_STATE_OPENING: return "BTIF_AV_STATE_OPENING";
      case BTIF_AV_STATE_OPENED: return "BTIF_AV_STATE_OPENED";
      case BTIF_AV_STATE_STARTED: return "BTIF_AV_STATE_STARTED";
      default: return "UNKNOWN_STATE";
   }
}

const char *dump_av_sm_event_name(btif_av_sm_event_t event)
{
   switch(event) {
        case BTA_AV_ENABLE_EVT: return "BTA_AV_ENABLE_EVT";
        case BTA_AV_REGISTER_EVT: return "BTA_AV_REGISTER_EVT";
        case BTA_AV_OPEN_EVT: return "BTA_AV_OPEN_EVT";
        case BTA_AV_CLOSE_EVT: return "BTA_AV_CLOSE_EVT";
        case BTA_AV_START_EVT: return "BTA_AV_START_EVT";
        case BTA_AV_STOP_EVT: return "BTA_AV_STOP_EVT";
        case BTA_AV_PROTECT_REQ_EVT: return "BTA_AV_PROTECT_REQ_EVT";
        case BTA_AV_PROTECT_RSP_EVT: return "BTA_AV_PROTECT_RSP_EVT";
        case BTA_AV_RC_OPEN_EVT: return "BTA_AV_RC_OPEN_EVT";
        case BTA_AV_RC_CLOSE_EVT: return "BTA_AV_RC_CLOSE_EVT";
        case BTA_AV_REMOTE_CMD_EVT: return "BTA_AV_REMOTE_CMD_EVT";
        case BTA_AV_REMOTE_RSP_EVT: return "BTA_AV_REMOTE_RSP_EVT";
        case BTA_AV_VENDOR_CMD_EVT: return "BTA_AV_VENDOR_CMD_EVT";
        case BTA_AV_VENDOR_RSP_EVT: return "BTA_AV_VENDOR_RSP_EVT";
        case BTA_AV_RECONFIG_EVT: return "BTA_AV_RECONFIG_EVT";
        case BTA_AV_SUSPEND_EVT: return "BTA_AV_SUSPEND_EVT";
        case BTA_AV_PENDING_EVT: return "BTA_AV_PENDING_EVT";
        case BTA_AV_META_MSG_EVT: return "BTA_AV_META_MSG_EVT";
        case BTA_AV_REJECT_EVT: return "BTA_AV_REJECT_EVT";
        case BTA_AV_RC_FEAT_EVT: return "BTA_AV_RC_FEAT_EVT";

        case BTIF_SM_ENTER_EVT: return "BTIF_SM_ENTER_EVT";
        case BTIF_SM_EXIT_EVT: return "BTIF_SM_EXIT_EVT";
        case BTIF_AV_CONNECT_REQ_EVT: return "BTIF_AV_CONNECT_REQ_EVT";
        case BTIF_AV_DISCONNECT_REQ_EVT: return "BTIF_AV_DISCONNECT_REQ_EVT";
        case BTIF_AV_START_STREAM_REQ_EVT: return "BTIF_AV_START_STREAM_REQ_EVT";
        case BTIF_AV_STOP_STREAM_REQ_EVT: return "BTIF_AV_STOP_STREAM_REQ_EVT";
        case BTIF_AV_SUSPEND_STREAM_REQ_EVT: return "BTIF_AV_SUSPEND_STREAM_REQ_EVT";
        case BTIF_AV_RECONFIGURE_REQ_EVT: return "BTIF_AV_RECONFIGURE_REQ_EVT";
        default: return "UNKNOWN_EVENT";
   }
}


/****************************************************************************
**  Local helper functions
*****************************************************************************/
/*******************************************************************************
**
** Function         btif_initiate_av_open_tmr_hdlr
**
** Description      Timer to trigger AV open if the remote headset establishes
**                  RC connection w/o AV connection. The timer is needed to IOP
**                  with headsets that do establish AV after RC connection.
**
** Returns          void
**
*******************************************************************************/
static void btif_initiate_av_open_tmr_hdlr(TIMER_LIST_ENT *tle)
{
    BD_ADDR peer_addr;

    /* is there at least one RC connection - There should be */
    if (btif_rc_get_connected_peer(peer_addr)) {
       BTIF_TRACE_DEBUG1("%s Issuing connect to the remote RC peer", __FUNCTION__);
       btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CONNECT_REQ_EVT, (void*)&peer_addr);
    }
    else
    {
        BTIF_TRACE_ERROR1("%s No connected RC peers", __FUNCTION__);
    }
}

/*****************************************************************************
**  Static functions
******************************************************************************/

void btif_a2dp_set_busy_level(UINT8 level);
void btif_a2dp_upon_init(void);
void btif_a2dp_upon_idle(void);
void btif_a2dp_upon_start_req(void);
void btif_a2dp_upon_started(void);
int btif_a2dp_start_media_task(void);
void btif_a2dp_stop_media_task(void);

static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *p_data)
{
    BTIF_TRACE_DEBUG2("%s event:%s", __FUNCTION__, dump_av_sm_event_name(event));

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
        {
            /* clear the peer_bda */
            memset(&btif_av_cb.peer_bda, 0, sizeof(bt_bdaddr_t));
            btif_a2dp_upon_idle();
        } break;
 
        case BTIF_SM_EXIT_EVT:
            break;

        case BTA_AV_ENABLE_EVT:
        {
            BTA_AvRegister(BTA_AV_CHNL_AUDIO, BTIF_AV_SERVICE_NAME, 0);
        } break;

        case BTA_AV_REGISTER_EVT:
        {
            btif_av_cb.bta_handle = ((tBTA_AV*)p_data)->registr.hndl;
        } break;
        case BTA_AV_PENDING_EVT:
        case BTIF_AV_CONNECT_REQ_EVT:
        {
             if (event == BTIF_AV_CONNECT_REQ_EVT)
             {
             memcpy(&btif_av_cb.peer_bda, (bt_bdaddr_t*)p_data, sizeof(bt_bdaddr_t));
             BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                        TRUE, BTA_SEC_NONE);
             }
             else if (event == BTA_AV_PENDING_EVT)
             {
                  bdcpy(btif_av_cb.peer_bda.address, ((tBTA_AV*)p_data)->pend.bd_addr);
             }
             btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_OPENING);
        } break;
        case BTA_AV_RC_OPEN_EVT:
        {
            /* IOP_FIX: Jabra 620 only does RC open without AV open whenever it connects. So
             * as per the AV WP, an AVRC connection cannot exist without an AV connection. Therefore,
             * we initiate an AV connection if an RC_OPEN_EVT is received when we are in AV_CLOSED state.
             * We initiate the AV connection after a small 3s timeout to avoid any collisions from the
             * headsets, as some headsets initiate the AVRC connection first and then
             * immediately initiate the AV connection
             *
             * TODO: We may need to do this only on an AVRCP Play. FixMe
             */
            BTIF_TRACE_DEBUG0("BTA_AV_RC_OPEN_EVT received w/o AV");
            memset(&tle_av_open_on_rc, 0, sizeof(tle_av_open_on_rc));
            tle_av_open_on_rc.param = (UINT32)btif_initiate_av_open_tmr_hdlr;
            btu_start_timer(&tle_av_open_on_rc, BTU_TTYPE_USER_FUNC,
                            BTIF_TIMEOUT_AV_OPEN_ON_RC);
            btif_rc_handler(event, p_data);
        }break;
        case BTA_AV_REMOTE_CMD_EVT:
        case BTA_AV_VENDOR_CMD_EVT:
        case BTA_AV_META_MSG_EVT:
        case BTA_AV_RC_FEAT_EVT:
        {
            btif_rc_handler(event, (tBTA_AV*)p_data);
        }break;
        case BTA_AV_RC_CLOSE_EVT:
        {
            if (tle_av_open_on_rc.in_use) {
                BTIF_TRACE_DEBUG0("BTA_AV_RC_CLOSE_EVT: Stopping AV timer.");
                btu_stop_timer(&tle_av_open_on_rc);
            }
            btif_rc_handler(event, p_data);
        }break;

        default:
            BTIF_TRACE_WARNING2("%s Unhandled event:%s", __FUNCTION__,
                                dump_av_sm_event_name(event));
    }
    return TRUE;
}

static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *p_data)
{
    BTIF_TRACE_DEBUG2("%s event:%s", __FUNCTION__, dump_av_sm_event_name(event));

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
        {
           /* inform the application that we are entering connecting state */
           CHECK_CALL_CBACK(bt_av_callbacks, connection_state_cb,
                      BTAV_CONNECTION_STATE_CONNECTING, &(btif_av_cb.peer_bda));
        } break;

        case BTIF_SM_EXIT_EVT:
            break;

        case BTA_AV_OPEN_EVT:
        {
            tBTA_AV *p_bta_data = (tBTA_AV*)p_data;
            btav_connection_state_t state;
            btif_sm_state_t av_state;
            BTIF_TRACE_DEBUG1("status:%d", p_bta_data->open.status);
            if (p_bta_data->open.status == BTA_AV_SUCCESS)
            {
                 state = BTAV_CONNECTION_STATE_CONNECTED;
                 av_state = BTIF_AV_STATE_OPENED;
            }
            else
            {
                BTIF_TRACE_WARNING1("BTA_AV_OPEN_EVT::FAILED status: %d",
                                     p_bta_data->open.status );
                state = BTAV_CONNECTION_STATE_DISCONNECTED;
                av_state  = BTIF_AV_STATE_IDLE;
            }
            /* inform the application of the event */
            CHECK_CALL_CBACK(bt_av_callbacks, connection_state_cb,
                             state, &(btif_av_cb.peer_bda));
            /* change state to open/idle based on the status */
            btif_sm_change_state(btif_av_cb.sm_handle, av_state);
        } break;

        CHECK_RC_EVENT(event, p_data);

        default:
            BTIF_TRACE_WARNING2("%s Unhandled event:%s", __FUNCTION__,
                                dump_av_sm_event_name(event));
   }
   return TRUE;
}


static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *p_data)
{
    BTIF_TRACE_DEBUG2("%s event:%s", __FUNCTION__, dump_av_sm_event_name(event));

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:

            BTIF_TRACE_DEBUG0("starting av...sleeping before that");

            /* Auto-starting on Open could introduce race conditions. So delaying
             * the start.
             *
             * This is anyway temporary and will be removed once the START/STOP stream
             * requests are processed.
             */
            GKI_delay(3000);

            btif_a2dp_upon_start_req();  

            /* autostart for now to test media task */
            BTA_AvStart();

            break;

        case BTIF_SM_EXIT_EVT:
            break;

        case BTIF_AV_START_STREAM_REQ_EVT:
            /* fixme */
            break;

        case BTA_AV_START_EVT:
        {
            tBTA_AV *p_bta_data = (tBTA_AV*)p_data;

            if (p_bta_data->start.status == BTA_AV_SUCCESS)
            {                 
                 /* change state to started  */
                 btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_STARTED);
            }
            else
            {
                /* fixme */
            }
        } break;

        case BTIF_AV_DISCONNECT_REQ_EVT:
        {
           BTA_AvClose(btif_av_cb.bta_handle);
           /* inform the application that we are disconnecting */
           CHECK_CALL_CBACK(bt_av_callbacks, connection_state_cb,
               BTAV_CONNECTION_STATE_DISCONNECTING, &(btif_av_cb.peer_bda));
        } break;

        case BTA_AV_CLOSE_EVT:
        {
            /* inform the application that we are disconnecting */
            CHECK_CALL_CBACK(bt_av_callbacks, connection_state_cb,
                BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
        } break;

        CHECK_RC_EVENT(event, p_data);

        default:
           BTIF_TRACE_WARNING2("%s Unhandled event:%s", __FUNCTION__,
                               dump_av_sm_event_name(event));
    }
    return TRUE;
}

static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *p_data)
{
    BTIF_TRACE_DEBUG2("%s event:%s", __FUNCTION__, dump_av_sm_event_name(event));

    switch (event)
    {
        case BTIF_SM_ENTER_EVT: 
            btif_a2dp_upon_started();
            break;
        
        case BTIF_SM_EXIT_EVT:
            break;
        case BTIF_AV_DISCONNECT_REQ_EVT:
        {
           BTA_AvClose(btif_av_cb.bta_handle);
           /* inform the application that we are disconnecting */
           CHECK_CALL_CBACK(bt_av_callbacks, connection_state_cb,
               BTAV_CONNECTION_STATE_DISCONNECTING, &(btif_av_cb.peer_bda));
        } break;

        case BTA_AV_STOP_EVT:
        {
            /* inform the application that we are disconnecting */
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_OPENED);
        } break;

        CHECK_RC_EVENT(event, p_data);

        default:
            BTIF_TRACE_WARNING2("%s Unhandled event:%s", __FUNCTION__,
                                 dump_av_sm_event_name(event));
    }
    return TRUE;
}

static void btif_av_handle_event(UINT16 event, char* p_param)
{
    btif_sm_dispatch(btif_av_cb.sm_handle, event, (void*)p_param);
}

static void bte_av_callback(tBTA_AV_EVT event, tBTA_AV *p_data)
{
    /* Switch to BTIF context */
    btif_transfer_context(btif_av_handle_event, event,
                          (char*)p_data, sizeof(tBTA_AV), NULL);
}
/*****************************************************************************
**  Externs
******************************************************************************/

/*****************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function         init
**
** Description      Initializes the AV interface
**
** Returns          bt_status_t
**
*******************************************************************************/

static bt_status_t init(btav_callbacks_t* callbacks )
{
    int status;

    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    status = btif_a2dp_start_media_task();

    if (status != GKI_SUCCESS)
        return BT_STATUS_FAIL;

    bt_av_callbacks = callbacks;

    btif_enable_service(BTA_A2DP_SERVICE_ID);
    /* Initialize the AVRC CB */
    btif_rc_init();
    /* Also initialize the AV state machine */
    btif_av_cb.sm_handle = btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE);

    btif_a2dp_upon_init();

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         connect
**
** Description      Establishes the AV signalling channel with the remote headset
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t connect(bt_bdaddr_t *bd_addr)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    BTIF_TRACE_ERROR1("callbacks is 0x%x", bt_av_callbacks);
    CHECK_BTAV_INIT();

    /* Switch to BTIF context */
    return btif_transfer_context(btif_av_handle_event, BTIF_AV_CONNECT_REQ_EVT,
                                 (char*)bd_addr, sizeof(bt_bdaddr_t), NULL);
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      Tears down the AV signalling channel with the remote headset
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t disconnect(bt_bdaddr_t *bd_addr)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    CHECK_BTAV_INIT();

    /* Switch to BTIF context */
    return btif_transfer_context(btif_av_handle_event, BTIF_AV_DISCONNECT_REQ_EVT,
                                 (char*)bd_addr, sizeof(bt_bdaddr_t), NULL);
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Shuts down the AV interface and does the cleanup
**
** Returns          None
**
*******************************************************************************/
static void cleanup(void)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    if (bt_av_callbacks)
    {
        btif_a2dp_stop_media_task();

        btif_disable_service(BTA_A2DP_SERVICE_ID);
        bt_av_callbacks = NULL;

        /* Also shut down the AV state machine */
        btif_sm_shutdown(btif_av_cb.sm_handle);
        btif_av_cb.sm_handle = NULL;
    }
    return;
}

static const btav_interface_t bt_av_interface = {
    sizeof(btav_interface_t),
    init,
    connect,
    disconnect,
    cleanup,
};

/*******************************************************************************
**
** Function         btif_av_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btif_av_execute_service(BOOLEAN b_enable)
{
     if (b_enable)
     {
#if (AVRC_METADATA_INCLUDED == TRUE)
         BTA_AvEnable(BTA_SEC_AUTHENTICATE|BTA_SEC_AUTHORIZE,
                      BTA_AV_FEAT_RCTG|BTA_AV_FEAT_METADATA|BTA_AV_FEAT_VENDOR,
                      bte_av_callback);
#else
         /* TODO: Removed BTA_SEC_AUTHORIZE since the Java/App does not
          * handle this request in order to allow incoming connections to succeed.
          * We need to put this back once support for this is added */
         BTA_AvEnable(BTA_SEC_AUTHENTICATE, BTA_AV_FEAT_RCTG,
                      bte_av_callback);
#endif
         BTA_AvRegister(BTA_AV_CHNL_AUDIO, BTIF_AV_SERVICE_NAME, 0);
     }
     else {
         BTA_AvDeregister(btif_av_cb.bta_handle);
         BTA_AvDisable();
     }
     return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_av_get_interface
**
** Description      Get the AV callback interface
**
** Returns          btav_interface_t
**
*******************************************************************************/
const btav_interface_t *btif_av_get_interface()
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    return &bt_av_interface;
}
