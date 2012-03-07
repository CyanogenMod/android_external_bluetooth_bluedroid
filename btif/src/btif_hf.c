/************************************************************************************
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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR 
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

/************************************************************************************
 *
 *  Filename:      btif_hf.c
 *
 *  Description:   Handsfree Profile Bluetooth Interface
 * 
 * 
 ***********************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>

#define LOG_TAG "BTIF_HF"
#include "btif_common.h"
#include "btif_util.h"

#include "bd.h"
#include "bta_ag_api.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/
/* TODO: Should these be moved to a conf file? */
#define BTIF_HF_SERVICES    (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK)
#define BTIF_HF_SECURITY    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#define BTIF_HF_FEATURES   (BTA_AG_FEAT_ECNR   | \
                             BTA_AG_FEAT_REJECT | \
                             BTA_AG_FEAT_ECS    | \
                             BTA_AG_FEAT_ECC    | \
                             BTA_AG_FEAT_EXTERR | \
                             BTA_AG_FEAT_BTRH   | \
                             BTA_AG_FEAT_VREC)
#define BTIF_HSAG_SERVICE_NAME ("Headset Gateway")
#define BTIF_HFAG_SERVICE_NAME ("Handsfree Gateway")

#define BTIF_HF_ID_1        0

/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/
static bthf_callbacks_t *bt_hf_callbacks = NULL;

#define CHECK_BTHF_INIT() if (bt_hf_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING1("BTHF: %s: BTHF not initialized", __FUNCTION__);\
        return BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT1("BTHF: %s", __FUNCTION__);\
    }


/* BTIF-HF control block to map bdaddr to BTA handle */
typedef struct _btif_hf_cb
{
    UINT16                  handle;
    bt_bdaddr_t             connected_bda;
    bthf_connection_state_t state;
    bthf_vr_state_t         vr_state;
    tBTA_AG_PEER_FEAT       peer_feat;
    int                     num_active;
    int                     num_held;
    bthf_call_state_t       call_setup_state;
} btif_hf_cb_t;

static btif_hf_cb_t btif_hf_cb;


/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

/*******************************************************************************
**
** Function         is_connected
**
** Description      Internal function to check if HF is connected
**
** Returns          TRUE if connected
**
*******************************************************************************/
static BOOLEAN is_connected(bt_bdaddr_t *bd_addr)
{
    if ((btif_hf_cb.state == BTHF_CONNECTION_STATE_CONNECTED) &&
        ((bd_addr == NULL) || (bdcmp(bd_addr->address, btif_hf_cb.connected_bda.address) == 0)))
        return TRUE;
    else
        return FALSE;
}

/*******************************************************************************
**
** Function         callstate_to_callsetup
**
** Description      Converts HAL call state to BTA call setup indicator value
**
** Returns          BTA call indicator value
**
*******************************************************************************/
static UINT8 callstate_to_callsetup(bthf_call_state_t call_state)
{
    UINT8 call_setup = 0;
    if (call_state == BTHF_CALL_STATE_INCOMING)
        call_setup = 1;
    if (call_state == BTHF_CALL_STATE_DIALING)
        call_setup = 2;
    if (call_state == BTHF_CALL_STATE_ALERTING)
        call_setup = 3;

    return call_setup;
}

/*******************************************************************************
**
** Function         send_at_result
**
** Description      Send AT result code (OK/ERROR)
**
** Returns          void
**
*******************************************************************************/
static void send_at_result(UINT8 ok_flag, UINT16 errcode)
{
    tBTA_AG_RES_DATA    ag_res;
    memset (&ag_res, 0, sizeof (ag_res));

    ag_res.ok_flag = ok_flag;
    if (ok_flag == BTA_AG_OK_ERROR)
    {
        ag_res.errcode = errcode;
    }

    BTA_AgResult (btif_hf_cb.handle, BTA_AG_UNAT_RES, &ag_res);
}

/*******************************************************************************
**
** Function         send_indicator_update
**
** Description      Send indicator update (CIEV)
**
** Returns          void
**
*******************************************************************************/
static void send_indicator_update (UINT16 indicator, UINT16 value)
{
    tBTA_AG_RES_DATA ag_res;

    memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));
    ag_res.ind.id = indicator;
    ag_res.ind.value = value;

    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IND_RES, &ag_res);
}

/*****************************************************************************
**   Section name (Group of functions)
*****************************************************************************/

/*****************************************************************************
**
**   btif hf api functions (no context switch)
**
*****************************************************************************/


/*******************************************************************************
**
** Function         btif_hf_upstreams_evt
**
** Description      Executes HF UPSTREAMS events in btif context
**
** Returns          void                  
**
*******************************************************************************/
static void btif_hf_upstreams_evt(UINT16 event, char* p_param)
{
    tBTA_AG *p_data = (tBTA_AG *)p_param;
    bdstr_t bdstr;    

    BTIF_TRACE_DEBUG2("%s: event=%s", __FUNCTION__, dump_hf_event(event));

    switch (event)
    {
        case BTA_AG_ENABLE_EVT:
        case BTA_AG_DISABLE_EVT:
            break;

        case BTA_AG_REGISTER_EVT:
            btif_hf_cb.handle = p_data->reg.hdr.handle;
            break;

        case BTA_AG_OPEN_EVT:
            if (p_data->open.status == BTA_AG_SUCCESS)
            {
                bdcpy(btif_hf_cb.connected_bda.address, p_data->open.bd_addr);
                btif_hf_cb.state = BTHF_CONNECTION_STATE_CONNECTED;
                btif_hf_cb.peer_feat = 0;
            }
            else if (btif_hf_cb.state == BTHF_CONNECTION_STATE_CONNECTING)
            {
                btif_hf_cb.state = BTHF_CONNECTION_STATE_DISCONNECTED;
            }
            else
            {
                BTIF_TRACE_WARNING4("%s: AG open failed, but another device connected. status=%d state=%d connected device=%s",
                        __FUNCTION__, p_data->open.status, btif_hf_cb.state, bd2str(&btif_hf_cb.connected_bda, &bdstr));
                break;
            }

            CHECK_CALL_CBACK(bt_hf_callbacks, connection_state_cb, btif_hf_cb.state, &btif_hf_cb.connected_bda);

            if (p_data->open.status != BTA_AG_SUCCESS)
                bdsetany(btif_hf_cb.connected_bda.address);
            break;

        case BTA_AG_CLOSE_EVT:
            btif_hf_cb.state = BTHF_CONNECTION_STATE_DISCONNECTED;
            CHECK_CALL_CBACK(bt_hf_callbacks, connection_state_cb, btif_hf_cb.state, &btif_hf_cb.connected_bda);
            bdsetany(btif_hf_cb.connected_bda.address);
            btif_hf_cb.peer_feat = 0;
            break;

        case BTA_AG_CONN_EVT:
            btif_hf_cb.peer_feat = p_data->conn.peer_feat;
            break;

        case BTA_AG_AUDIO_OPEN_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, audio_state_cb, BTHF_AUDIO_STATE_CONNECTED, &btif_hf_cb.connected_bda);
            break;

        case BTA_AG_AUDIO_CLOSE_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, audio_state_cb, BTHF_AUDIO_STATE_DISCONNECTED, &btif_hf_cb.connected_bda);
            break;

        /* BTA auto-responds, silently discard */
        case BTA_AG_SPK_EVT:
        case BTA_AG_MIC_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, volume_cmd_cb, 
                (event == BTA_AG_SPK_EVT) ? BTHF_VOLUME_TYPE_SPK : BTHF_VOLUME_TYPE_MIC, p_data->val.num);
            break;

        case BTA_AG_AT_A_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, answer_call_cmd_cb);
            break;

        /* Java needs to send OK/ERROR for these commands */
        case BTA_AG_AT_BLDN_EVT:
        case BTA_AG_AT_D_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, dial_call_cmd_cb,
                (event == BTA_AG_AT_D_EVT) ? p_data->val.str : NULL);
            break;

        case BTA_AG_AT_CHUP_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, hangup_call_cmd_cb);
            break;

        case BTA_AG_AT_CIND_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, cind_cmd_cb);
            break;

        case BTA_AG_AT_VTS_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, dtmf_cmd_cb, p_data->val.str[0]);
            break;

        case BTA_AG_AT_BVRA_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, vr_cmd_cb,
                (p_data->val.num == 1) ? BTHF_VR_STATE_STARTED : BTHF_VR_STATE_STOPPED);
            break;

        case BTA_AG_AT_NREC_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, nrec_cmd_cb,
                (p_data->val.num == 1) ? BTHF_NREC_START : BTHF_NREC_STOP);
            break;

        /* TODO: Add a callback for CBC */
        case BTA_AG_AT_CBC_EVT:
            break;

        case BTA_AG_AT_CKPD_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, key_pressed_cmd_cb);
            break;

        /* Java needs to send OK/ERROR for these commands */
        case BTA_AG_AT_CHLD_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, chld_cmd_cb, p_data->val.num);
            break;

        case BTA_AG_AT_CLCC_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, clcc_cmd_cb, p_data->val.num);
            break;

        case BTA_AG_AT_COPS_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, cops_cmd_cb);
            break;

        case BTA_AG_AT_UNAT_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, unknown_at_cmd_cb,
                             p_data->val.str);
            break;

        case BTA_AG_AT_CNUM_EVT:
            CHECK_CALL_CBACK(bt_hf_callbacks, cnum_cmd_cb);
            break;

        /* TODO: Some of these commands may need to be sent to app. For now respond with error */
        case BTA_AG_AT_BINP_EVT:
        case BTA_AG_AT_BTRH_EVT:
            send_at_result(BTA_AG_OK_ERROR, BTA_AG_ERR_OP_NOT_SUPPORTED);
            break;


        default:
            BTIF_TRACE_WARNING2("%s: Unhandled event: %d", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         bte_hf_evt
**
** Description      Switches context from BTE to BTIF for all HF events
**
** Returns          void                  
**
*******************************************************************************/

static void bte_hf_evt(tBTA_AG_EVT event, tBTA_AG *p_data)
{
    bt_status_t status;
    int param_len = 0;

    /* TODO: BTA sends the union members and not tBTA_AG. If using param_len=sizeof(tBTA_AG), we get a crash on memcpy */
    if (BTA_AG_REGISTER_EVT == event)
        param_len = sizeof(tBTA_AG_REGISTER);
    else if (BTA_AG_OPEN_EVT == event)
        param_len = sizeof(tBTA_AG_OPEN);
    else if (BTA_AG_CONN_EVT == event)
        param_len = sizeof(tBTA_AG_CONN);
    else if ( (BTA_AG_CLOSE_EVT == event) || (BTA_AG_AUDIO_OPEN_EVT == event) || (BTA_AG_AUDIO_CLOSE_EVT == event))
        param_len = sizeof(tBTA_AG_HDR);
    else if (p_data)
        param_len = sizeof(tBTA_AG_VAL);
    
    /* switch context to btif task context (copy full union size for convenience) */
    status = btif_transfer_context(btif_hf_upstreams_evt, (uint16_t)event, (void*)p_data, param_len, NULL);

    /* catch any failed context transfers */
    ASSERTC(status == BT_STATUS_SUCCESS, "context transfer failed", status);
}


/*******************************************************************************
**
** Function         btif_hf_init
**
** Description     initializes the hf interface
**
** Returns         bt_status_t
**
*******************************************************************************/
static bt_status_t init( bthf_callbacks_t* callbacks )
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    bt_hf_callbacks = callbacks;

    /* Invoke the enable service API to the core to set the appropriate service_id
     * Internally, the HSP_SERVICE_ID shall also be enabled */
    btif_enable_service(BTA_HFP_SERVICE_ID);

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         connect
**
** Description     connect to headset
**
** Returns         bt_status_t
**
*******************************************************************************/
static bt_status_t connect( bt_bdaddr_t *bd_addr )
{
    CHECK_BTHF_INIT();

    if (!is_connected(bd_addr))
    {
        btif_hf_cb.state = BTHF_CONNECTION_STATE_CONNECTING;
        bdcpy(btif_hf_cb.connected_bda.address, bd_addr->address);

        BTA_AgOpen(btif_hf_cb.handle, btif_hf_cb.connected_bda.address, 
                   BTIF_HF_SECURITY, BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_BUSY;
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      disconnect from headset
**
** Returns         bt_status_t
**
*******************************************************************************/
static bt_status_t disconnect( bt_bdaddr_t *bd_addr )
{
    CHECK_BTHF_INIT();

    if (is_connected(bd_addr))
    {
        BTA_AgClose(btif_hf_cb.handle);
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         connect_audio
**
** Description     create an audio connection
**
** Returns         bt_status_t
**
*******************************************************************************/
static bt_status_t connect_audio( bt_bdaddr_t *bd_addr )
{
    CHECK_BTHF_INIT();

    if (is_connected(bd_addr))
    {
        BTA_AgAudioOpen(btif_hf_cb.handle);
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         disconnect_audio
**
** Description      close the audio connection
**
** Returns         bt_status_t
**
*******************************************************************************/
static bt_status_t disconnect_audio( bt_bdaddr_t *bd_addr )
{
    CHECK_BTHF_INIT();

    if (is_connected(bd_addr))
    {
        BTA_AgAudioClose(btif_hf_cb.handle);
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         start_voice_recognition
**
** Description      start voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t start_voice_recognition()
{
    CHECK_BTHF_INIT();
    if (is_connected(NULL))
    {
        tBTA_AG_RES_DATA ag_res;
        memset(&ag_res, 0, sizeof(ag_res));
        ag_res.state = 1;
        BTA_AgResult (btif_hf_cb.handle, BTA_AG_BVRA_RES, &ag_res);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         stop_voice_recognition
**
** Description      stop voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t stop_voice_recognition()
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        tBTA_AG_RES_DATA ag_res;
        memset(&ag_res, 0, sizeof(ag_res));
        ag_res.state = 0;
        BTA_AgResult (btif_hf_cb.handle, BTA_AG_BVRA_RES, &ag_res);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         volume_control
**
** Description      volume control
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t volume_control(bthf_volume_type_t type, int volume)
{
    CHECK_BTHF_INIT();

    tBTA_AG_RES_DATA ag_res;
    memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));
    if (is_connected(NULL))
    {
        ag_res.num = volume;
        BTA_AgResult(btif_hf_cb.handle,
                     (type == BTHF_VOLUME_TYPE_SPK) ? BTA_AG_SPK_RES : BTA_AG_MIC_RES,
                     &ag_res);
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         device_status_notification
**
** Description      Combined device status change notification
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t device_status_notification(bthf_network_state_t ntk_state,
                          bthf_service_type_t svc_type, int signal, int batt_chg)
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        /* send all indicators to BTA.
        ** BTA will make sure no duplicates are sent out
        */
        send_indicator_update(BTA_AG_IND_SERVICE,
                              (ntk_state == BTHF_NETWORK_STATE_AVAILABLE) ? 1 : 0);
        send_indicator_update(BTA_AG_IND_ROAM,
                             (svc_type == BTHF_SERVICE_TYPE_HOME) ? 0 : 1);
        send_indicator_update(BTA_AG_IND_SIGNAL, signal);
        send_indicator_update(BTA_AG_IND_BATTCHG, batt_chg);
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         cops_response
**
** Description      Response for COPS command
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t cops_response(const char *cops)
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        tBTA_AG_RES_DATA    ag_res;

        /* Format the response */
        sprintf (ag_res.str, "0,0,\"%s\"", cops);
        ag_res.ok_flag = BTA_AG_OK_DONE;

        BTA_AgResult (btif_hf_cb.handle, BTA_AG_COPS_RES, &ag_res);
        return BT_STATUS_SUCCESS;
    }
    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         cind_response
**
** Description      Response for CIND command
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t cind_response(int svc, int num_active, int num_held,
                                     bthf_call_state_t call_setup_state,
                                     int signal, int roam, int batt_chg)
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        tBTA_AG_RES_DATA    ag_res;
        
        memset (&ag_res, 0, sizeof (ag_res));
        sprintf (ag_res.str, "%d,%d,%d,%d,%d,%d,%d",
                (num_active ? 1 : 0),           /* Call state */
                callstate_to_callsetup(call_setup_state), /* Callsetup state */
                svc,                            /* network service */
                signal,                         /* Signal strength */
                roam,                           /* Roaming indicator */
                batt_chg,                       /* Battery level */
                (num_held ? 1 : 0));            /* Call held */
        
        BTA_AgResult (btif_hf_cb.handle, BTA_AG_CIND_RES, &ag_res);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         formatted_at_response
**
** Description      Pre-formatted AT response, typically in response to unknown AT cmd
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t formatted_at_response(const char *rsp)
{
    CHECK_BTHF_INIT();
    tBTA_AG_RES_DATA    ag_res;

    if (is_connected(NULL))
    {
        /* Format the response and send */
        memset (&ag_res, 0, sizeof (ag_res));
        strncpy(ag_res.str, rsp, BTA_AG_AT_MAX_LEN);
        BTA_AgResult (btif_hf_cb.handle, BTA_AG_UNAT_RES, &ag_res);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         at_response
**
** Description      ok/error response
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t at_response(bthf_at_response_t response_code)
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        /* TODO: Fix the errcode */
        send_at_result((response_code == BTHF_AT_RESPONSE_OK) ? BTA_AG_OK_DONE
                        : BTA_AG_OK_ERROR, 0);
    }


    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         clcc_response
**
** Description      response for CLCC command
**                  Can be iteratively called for each call index. Call index
**                  of 0 will be treated as NULL termination (Completes response)
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t clcc_response(int index, bthf_call_direction_t dir,
                                bthf_call_state_t state, bthf_call_mode_t mode,
                                bthf_call_mpty_type_t mpty, const char *number,
                                bthf_call_addrtype_t type)
{
    CHECK_BTHF_INIT();

    if (is_connected(NULL))
    {
        tBTA_AG_RES_DATA    ag_res;
        int                 xx;

        memset (&ag_res, 0, sizeof (ag_res));

        /* Format the response */
        if (index == 0)
        {
            ag_res.ok_flag = BTA_AG_OK_DONE;
        }
        else
        {
            BTIF_TRACE_EVENT6("clcc_response: [%d] dir %d state %d mode %d number = %s type = %d",
                          index, dir, state, mode, number, type);
            xx = sprintf (ag_res.str, "%d,%d,%d,%d,%d",
                         index, dir, state, mode, mpty);

            if (number)
            {
                if ((type == BTHF_CALL_ADDRTYPE_INTERNATIONAL) && (*number != '+'))
                    sprintf (&ag_res.str[xx], ",\"+%s\",%d", number, type);
                else
                    sprintf (&ag_res.str[xx], ",\"%s\",%d", number, type);
            }
        }
        BTA_AgResult (btif_hf_cb.handle, BTA_AG_CLCC_RES, &ag_res);

        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         phone_state_change
**
** Description      notify of a call state change
**                  number & type: valid only for incoming & waiting call
**
** Returns          bt_status_t
**
*******************************************************************************/

static bt_status_t phone_state_change(int num_active, int num_held, bthf_call_state_t call_setup_state,
                                            const char *number, bthf_call_addrtype_t type)
{
    tBTA_AG_RES res = 0xff;
    tBTA_AG_RES_DATA ag_res;
    bt_status_t status = BT_STATUS_SUCCESS;

    CHECK_BTHF_INIT();

    BTIF_TRACE_DEBUG6("phone_state_change: num_active=%d[%d]  num_held=%d[%d]"\
                      " call_setup=%d [%d]", num_active, btif_hf_cb.num_active,
                       num_held, btif_hf_cb.num_held,
                       call_setup_state, btif_hf_cb.call_setup_state);

    /* Check what has changed and update the corresponding indicators.
    ** In ad
    */

    /* if all indicators are 0, send end call and return */
    if (num_active == 0 && num_held == 0 && call_setup_state == BTHF_CALL_STATE_IDLE)
    {
        BTIF_TRACE_DEBUG1("%s: Phone on hook", __FUNCTION__);
        BTA_AgResult (BTA_AG_HANDLE_ALL, BTA_AG_END_CALL_RES, NULL);

        /* if held call was present, reset that as well */
        if (btif_hf_cb.num_held)
            send_indicator_update(BTA_AG_IND_CALLHELD, 0);

        goto update_call_states;
    }

    /* Ringing call changed? */
    if (call_setup_state != btif_hf_cb.call_setup_state)
    {
        BTIF_TRACE_DEBUG3("%s: Call setup states changed. old: %s new: %s",
            __FUNCTION__, dump_hf_call_state(btif_hf_cb.call_setup_state),
            dump_hf_call_state(call_setup_state));
        memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));

        switch (call_setup_state)
        {
            case BTHF_CALL_STATE_IDLE:
            {
                switch (btif_hf_cb.call_setup_state)
                {
                    case BTHF_CALL_STATE_INCOMING:
                        if (num_active > btif_hf_cb.num_active)
                        {
                            res = BTA_AG_IN_CALL_CONN_RES;
                            ag_res.audio_handle = btif_hf_cb.handle;
                        }
                        else if (num_held > btif_hf_cb.num_held)
                            res = BTA_AG_IN_CALL_HELD_RES;
                        else
                            res = BTA_AG_CALL_CANCEL_RES;
                        break;
                    case BTHF_CALL_STATE_DIALING:
                    case BTHF_CALL_STATE_ALERTING:
                        if (num_active > btif_hf_cb.num_active)
                        {
                            ag_res.audio_handle = btif_hf_cb.handle;
                            res = BTA_AG_OUT_CALL_CONN_RES;
                        }
                        else
                            res = BTA_AG_CALL_CANCEL_RES;
                        break;
                    default:
                        BTIF_TRACE_ERROR1("%s: Incorrect Call setup state transition", __FUNCTION__);
                        status = BT_STATUS_PARM_INVALID;
                        break;
                }
            } break;

            case BTHF_CALL_STATE_INCOMING:
                if (num_active || num_held)
                    res = BTA_AG_CALL_WAIT_RES;
                else
                    res = BTA_AG_IN_CALL_RES;
                if (number)
                {
                    strcpy(ag_res.str, number);
                    ag_res.num = type;
                }
                break;
            case BTHF_CALL_STATE_DIALING:
                ag_res.audio_handle = btif_hf_cb.handle;
                res = BTA_AG_OUT_CALL_ORIG_RES;
                break;
            case BTHF_CALL_STATE_ALERTING:
                ag_res.audio_handle = btif_hf_cb.handle;
                res = BTA_AG_OUT_CALL_ALERT_RES;
                break;
            default:
                BTIF_TRACE_ERROR1("%s: Incorrect new ringing call state", __FUNCTION__);
                status = BT_STATUS_PARM_INVALID;
                break;
        }
        BTIF_TRACE_DEBUG3("%s: Call setup state changed. res=%d, audio_handle=%d", __FUNCTION__, res, ag_res.audio_handle);

        if (res)
            BTA_AgResult(BTA_AG_HANDLE_ALL, res, &ag_res);

        /* if call setup is idle, we have already updated call indicator, jump out */
        if (call_setup_state == BTHF_CALL_STATE_IDLE)
        {
            /* check & update callheld */
            if ((num_held > 0) && (num_active > 0))
                send_indicator_update(BTA_AG_IND_CALLHELD, 1);
            goto update_call_states;
        }
    }

    /* Active Changed? */
    if (num_active != btif_hf_cb.num_active)
    {
        BTIF_TRACE_DEBUG3("%s: Active call states changed. old: %d new: %d", __FUNCTION__, btif_hf_cb.num_active, num_active);
        send_indicator_update(BTA_AG_IND_CALL, (num_active ? 1 : 0));
    }


    /* Held Changed? */
    if (num_held != btif_hf_cb.num_held)
    {
        BTIF_TRACE_DEBUG3("%s: Held call states changed. old: %d new: %d", __FUNCTION__, btif_hf_cb.num_held, num_held);
        send_indicator_update(BTA_AG_IND_CALLHELD, ((num_held == 0) ? 0 : ((num_active == 0) ? 2 : 1)));
    }

    /* Calls Swapped? */
    if ( (call_setup_state == btif_hf_cb.call_setup_state) &&
         (num_active && num_held) &&
         (num_active == btif_hf_cb.num_active) &&
         (num_held == btif_hf_cb.num_held) )
    {
        BTIF_TRACE_DEBUG1("%s: Calls swapped", __FUNCTION__);
        send_indicator_update(BTA_AG_IND_CALLHELD, 1);
    }

update_call_states:
    btif_hf_cb.num_active = num_active;
    btif_hf_cb.num_held = num_held;
    btif_hf_cb.call_setup_state = call_setup_state;

    return status;
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Closes the HF interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static void  cleanup( void )
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    if (bt_hf_callbacks)
    {
        btif_disable_service(BTA_HFP_SERVICE_ID);
        bt_hf_callbacks = NULL;
    }
}

static const bthf_interface_t bthfInterface = {
    sizeof(bt_interface_t),
    init,
    connect,
    disconnect,
    connect_audio,
    disconnect_audio,
    start_voice_recognition,
    stop_voice_recognition,
    volume_control,
    device_status_notification,
    cops_response,
    cind_response,
    formatted_at_response,
    at_response,
    clcc_response,
    phone_state_change,
    cleanup,
};

/*******************************************************************************
**
** Function         btif_hf_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btif_hf_execute_service(BOOLEAN b_enable)
{
    char * p_service_names[] = {BTIF_HSAG_SERVICE_NAME, BTIF_HFAG_SERVICE_NAME};
     if (b_enable)
     {
          /* Enable and register with BTA-AG */
          BTA_AgEnable (BTA_AG_PARSE, bte_hf_evt);
          BTA_AgRegister(BTIF_HF_SERVICES, BTIF_HF_SECURITY, BTIF_HF_FEATURES,
                         p_service_names, BTIF_HF_ID_1);
     }
     else {
         /* De-register AG */
         BTA_AgDeregister(btif_hf_cb.handle);
         /* Disable AG */
         BTA_AgDisable();
     }
     return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_hf_get_interface
**
** Description      Get the hf callback interface
**
** Returns          bthf_interface_t
**
*******************************************************************************/
const bthf_interface_t *btif_hf_get_interface()
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    return &bthfInterface;
}
