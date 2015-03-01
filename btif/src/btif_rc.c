/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/


/*****************************************************************************
 *
 *  Filename:      btif_rc.c
 *
 *  Description:   Bluetooth AVRC implementation
 *
 *****************************************************************************/
#include <hardware/bluetooth.h>
#include <fcntl.h>
#include "bta_api.h"
#include "bta_av_api.h"
#include "avrc_defs.h"
#include "bd.h"
#include "gki.h"

#define LOG_TAG "BTIF_RC"
#include "btif_common.h"
#include "btif_util.h"
#include "btif_av.h"
#include "hardware/bt_rc.h"
#include "uinput.h"

/*****************************************************************************
**  Constants & Macros
******************************************************************************/

/* cod value for Headsets */
#define COD_AV_HEADSETS        0x0404
/* for AVRC 1.4 need to change this */
#define MAX_RC_NOTIFICATIONS AVRC_EVT_VOLUME_CHANGE
//#define TEST_BROWSE_RESPONSE
#define MAX_FOLDER_RSP_SUPPORT 3

#define IDX_GET_PLAY_STATUS_RSP    0
#define IDX_LIST_APP_ATTR_RSP      1
#define IDX_LIST_APP_VALUE_RSP     2
#define IDX_GET_CURR_APP_VAL_RSP   3
#define IDX_SET_APP_VAL_RSP        4
#define IDX_GET_APP_ATTR_TXT_RSP   5
#define IDX_GET_APP_VAL_TXT_RSP    6
#define IDX_GET_ELEMENT_ATTR_RSP   7
#define IDX_GET_FOLDER_ITEMS_RSP   8
#define IDX_SET_FOLDER_ITEM_RSP    9
#define IDX_SET_ADDRESS_PLAYER_RSP 10
#define IDX_SET_BROWSE_PLAYER_RSP  11
#define IDX_CHANGE_PATH_RSP        12
#define IDX_PLAY_ITEM_RSP          13
#define IDX_GET_ITEM_ATTR_RSP      14
#define MAX_VOLUME 128
#define MAX_LABEL 16
#define MAX_TRANSACTIONS_PER_SESSION 16
#define PLAY_STATUS_PLAYING 1
#define MAX_CMD_QUEUE_LEN 15
#define ERR_PLAYER_NOT_ADDRESED 0x13

#define CHECK_RC_CONNECTED                                                                  \
    BTIF_TRACE_DEBUG("## %s ##", __FUNCTION__);                                            \
    if(btif_rc_cb.rc_connected == FALSE)                                                    \
    {                                                                                       \
        BTIF_TRACE_WARNING("Function %s() called when RC is not connected", __FUNCTION__); \
        return BT_STATUS_NOT_READY;                                                         \
    }

#define FILL_PDU_QUEUE(index, ctype, label, pending)        \
{                                                           \
    btif_rc_cb.rc_pdu_info[index].ctype = ctype;            \
    btif_rc_cb.rc_pdu_info[index].label = label;            \
    btif_rc_cb.rc_pdu_info[index].is_rsp_pending = pending; \
}

#define SEND_METAMSG_RSP(index, avrc_rsp)                                                      \
{                                                                                              \
    if(btif_rc_cb.rc_pdu_info[index].is_rsp_pending == FALSE)                                  \
    {                                                                                          \
        BTIF_TRACE_WARNING("%s Not sending response as no PDU was registered", __FUNCTION__); \
        return BT_STATUS_UNHANDLED;                                                            \
    }                                                                                          \
    send_metamsg_rsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_pdu_info[index].label,                \
        btif_rc_cb.rc_pdu_info[index].ctype, avrc_rsp);                                        \
    btif_rc_cb.rc_pdu_info[index].ctype = 0;                                                   \
    btif_rc_cb.rc_pdu_info[index].label = 0;                                                   \
    btif_rc_cb.rc_pdu_info[index].is_rsp_pending = FALSE;                                      \
}


#define SEND_BROWSEMSG_RSP(index , avrc_rsp)                                                   \
{                                                                                              \
    if(btif_rc_cb.rc_pdu_info[index].is_rsp_pending == FALSE)                                  \
    {                                                                                          \
        BTIF_TRACE_WARNING("%s Not sending response as no PDU was registered", __FUNCTION__); \
        return BT_STATUS_UNHANDLED;                                                            \
    }                                                                                          \
    send_browsemsg_rsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_pdu_info[index].label,              \
        btif_rc_cb.rc_pdu_info[index].ctype, avrc_rsp);                                        \
    btif_rc_cb.rc_pdu_info[index].ctype = 0;                                                   \
    btif_rc_cb.rc_pdu_info[index].label = 0;                                                   \
    btif_rc_cb.rc_pdu_info[index].is_rsp_pending = FALSE;                                      \
}

/*****************************************************************************
**  Local type definitions
******************************************************************************/
typedef struct {
    UINT8 bNotify;
    UINT8 label;
} btif_rc_reg_notifications_t;

typedef struct
{
    UINT8   label;
    UINT8   ctype;
    BOOLEAN is_rsp_pending;
} btif_rc_cmd_ctxt_t;

/* TODO : Merge btif_rc_reg_notifications_t and btif_rc_cmd_ctxt_t to a single struct */
typedef struct {
    BOOLEAN                     rc_connected;
    UINT8                       rc_handle;
    tBTA_AV_FEAT                rc_features;
    BD_ADDR                     rc_addr;
    UINT16                      rc_pending_play;
    btif_rc_cmd_ctxt_t          rc_pdu_info[MAX_CMD_QUEUE_LEN];
    btif_rc_reg_notifications_t rc_notif[MAX_RC_NOTIFICATIONS];
    unsigned int                rc_volume;
    uint8_t                     rc_vol_label;
} btif_rc_cb_t;

typedef struct {
    BOOLEAN in_use;
    UINT8 lbl;
    UINT8 handle;
} rc_transaction_t;

typedef struct
{
    pthread_mutex_t lbllock;
    rc_transaction_t transaction[MAX_TRANSACTIONS_PER_SESSION];
} rc_device_t;


rc_device_t device;

#define MAX_UINPUT_PATHS 3
static const char* uinput_dev_path[] =
                       {"/dev/uinput", "/dev/input/uinput", "/dev/misc/uinput" };
static int uinput_fd = -1;

static int  send_event (int fd, uint16_t type, uint16_t code, int32_t value);
static void send_key (int fd, uint16_t key, int pressed);
static int  uinput_driver_check();
static int  uinput_create(char *name);
static int  init_uinput (void);
static void close_uinput (void);
static BOOLEAN dev_blacklisted_for_absolute_volume(BD_ADDR peer_dev);
#if (AVRC_CTLR_INCLUDED == TRUE)
static BOOLEAN conn_status = FALSE;
#endif
static const struct {
    const char *name;
    uint8_t avrcp;
    uint16_t mapped_id;
    uint8_t release_quirk;
} key_map[] = {
    { "PLAY",         AVRC_ID_PLAY,     KEY_PLAYCD,       1 },
    { "STOP",         AVRC_ID_STOP,     KEY_STOPCD,       0 },
    { "PAUSE",        AVRC_ID_PAUSE,    KEY_PAUSECD,      1 },
    { "FORWARD",      AVRC_ID_FORWARD,  KEY_NEXTSONG,     0 },
    { "BACKWARD",     AVRC_ID_BACKWARD, KEY_PREVIOUSSONG, 0 },
    { "REWIND",       AVRC_ID_REWIND,   KEY_REWIND,       0 },
    { "FAST FORWARD", AVRC_ID_FAST_FOR, KEY_FAST_FORWARD, 0 },
    { NULL,           0,                0,                0 }
};

/* the rc_black_addr_prefix and rc_white_addr_prefix are used to correct
 * IOP issues of absolute volume feature
 * We encoutered A2DP headsets/carkits advertising absolute volume but buggy.
 * We would like to blacklist those devices.
 * But we donot have a full list of the bad devices. So as a temp fix, we
 * are blacklisting all the devices except the devices we have well tested,
 * the ones in the whitelist.
 *
 * For now, only the rc_white_addr_prefix is used in the code while
 * rc_black_addr_prefix is kept here for future long term solution.
 */
static const UINT8 rc_black_addr_prefix[][3] = {
    {0x0, 0x18, 0x6B}, // LG HBS-730
    {0x0, 0x26, 0x7E}  // VW Passat
};

static const UINT8 rc_white_addr_prefix[][3] = {
    {0x94, 0xCE, 0x2C}, // Sony SBH50
    {0x30, 0x17, 0xC8}, // Sony wm600
    {0x00, 0x15, 0x83}, // BlueSoleil dongle
    {0x00, 0x80, 0x98}, // PTS dongle
    {0x48, 0xC1, 0xAC}, // Plantronics Backbeat Go
    {0x00, 0x1B, 0xDC}, // PTS dongle 2
    {0x00, 0x19, 0x8E}, // Demant
    {0x04, 0x88, 0xE2}, // Apple
    {0x00, 0x0C, 0x8A}, // Bose
    {0x1C, 0x48, 0xF9}  // Jabra Pulse
};

static const char* rc_white_name[] = {
    "SBH50",
    "MW600"
};

static void send_reject_response (UINT8 rc_handle, UINT8 label,
    UINT8 pdu, UINT8 status);
static UINT8 opcode_from_pdu(UINT8 pdu);
static void send_metamsg_rsp (UINT8 rc_handle, UINT8 label,
    tBTA_AV_CODE code, tAVRC_RESPONSE *pmetamsg_resp);
static void register_volumechange(UINT8 label);
static void lbl_init();
static void lbl_destroy();
static void init_all_transactions();
static bt_status_t  get_transaction(rc_transaction_t **ptransaction);
static void release_transaction(UINT8 label);
static rc_transaction_t* get_transaction_by_lbl(UINT8 label);
static void handle_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg);
static void btif_rc_upstreams_evt(UINT16 event, tAVRC_COMMAND* p_param, UINT8 ctype, UINT8 label);
static void btif_rc_upstreams_rsp_evt(UINT16 event, tAVRC_RESPONSE *pavrc_resp, UINT8 ctype, UINT8 label);
static bt_status_t set_addrplayer_rsp(btrc_status_t status_code);

/*Added for Browsing Message Response */
static void send_browsemsg_rsp (UINT8 rc_handle, UINT8 label,
    tBTA_AV_CODE code, tAVRC_RESPONSE *pmetamsg_resp);


/*****************************************************************************
**  Static variables
******************************************************************************/
static btif_rc_cb_t btif_rc_cb;
static btrc_callbacks_t *bt_rc_callbacks = NULL;
static btrc_ctrl_callbacks_t *bt_rc_ctrl_callbacks = NULL;

/*****************************************************************************
**  Static functions
******************************************************************************/

/*****************************************************************************
**  Externs
******************************************************************************/
extern BOOLEAN btif_hf_call_terminated_recently();
extern BOOLEAN check_cod(const bt_bdaddr_t *remote_bdaddr, uint32_t cod);
extern BOOLEAN btif_hf_is_call_idle();

/*****************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**   Local uinput helper functions
******************************************************************************/
int send_event (int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct uinput_event event;
    BTIF_TRACE_DEBUG("%s type:%u code:%u value:%d", __FUNCTION__,
        type, code, value);
    memset(&event, 0, sizeof(event));
    event.type  = type;
    event.code  = code;
    event.value = value;

    return write(fd, &event, sizeof(event));
}

void send_key (int fd, uint16_t key, int pressed)
{
    BTIF_TRACE_DEBUG("%s fd:%d key:%u pressed:%d", __FUNCTION__,
        fd, key, pressed);

    if (fd < 0)
    {
        return;
    }

    BTIF_TRACE_IMP("AVRCP: Send key %d (%d) fd=%d", key, pressed, fd);
    send_event(fd, EV_KEY, key, pressed);
    send_event(fd, EV_SYN, SYN_REPORT, 0);
}

/************** uinput related functions **************/
int uinput_driver_check()
{
    uint32_t i;
    for (i=0; i < MAX_UINPUT_PATHS; i++)
    {
        if (access(uinput_dev_path[i], O_RDWR) == 0) {
           return 0;
        }
    }
    BTIF_TRACE_ERROR("%s ERROR: uinput device is not in the system", __FUNCTION__);
    return -1;
}

int uinput_create(char *name)
{
    struct uinput_dev dev;
    int fd, err, x = 0;

    for(x=0; x < MAX_UINPUT_PATHS; x++)
    {
        fd = open(uinput_dev_path[x], O_RDWR);
        if (fd < 0)
            continue;
        break;
    }
    if (x == MAX_UINPUT_PATHS) {
        BTIF_TRACE_ERROR("%s ERROR: uinput device open failed", __FUNCTION__);
        return -1;
    }
    memset(&dev, 0, sizeof(dev));
    if (name)
        strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE);

    dev.id.bustype = BUS_BLUETOOTH;
    dev.id.vendor  = 0x0000;
    dev.id.product = 0x0000;
    dev.id.version = 0x0000;

    if (write(fd, &dev, sizeof(dev)) < 0) {
        BTIF_TRACE_ERROR("%s Unable to write device information", __FUNCTION__);
        close(fd);
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    for (x = 0; key_map[x].name != NULL; x++)
        ioctl(fd, UI_SET_KEYBIT, key_map[x].mapped_id);

    for(x = 0; x < KEY_MAX; x++)
        ioctl(fd, UI_SET_KEYBIT, x);

    if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
        BTIF_TRACE_ERROR("%s Unable to create uinput device", __FUNCTION__);
        close(fd);
        return -1;
    }
    return fd;
}

int init_uinput (void)
{
    char *name = "AVRCP";

    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    uinput_fd = uinput_create(name);
    if (uinput_fd < 0) {
        BTIF_TRACE_ERROR("%s AVRCP: Failed to initialize uinput for %s (%d)",
                          __FUNCTION__, name, uinput_fd);
    } else {
        BTIF_TRACE_DEBUG("%s AVRCP: Initialized uinput for %s (fd=%d)",
                          __FUNCTION__, name, uinput_fd);
    }
    return uinput_fd;
}

void close_uinput (void)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    if (uinput_fd > 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);

        close(uinput_fd);
        uinput_fd = -1;
    }
}

void handle_rc_features()
{
    if (bt_rc_callbacks != NULL)
    {
        /*Enabling Absolute volume and other avrcp TG specific features only if A2dp Src and
        Avrcp TG role is activated along with Avrcp CT. Check for bt_rc_callbacks not equal to
        null assures that avrcp TG service is up
        */
        btrc_remote_features_t rc_features = BTRC_FEAT_NONE;
        bt_bdaddr_t rc_addr;
        bt_bdaddr_t avdtp_addr;
        bdstr_t addr1, addr2;

        bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
        avdtp_addr = btif_av_get_addr();

        BTIF_TRACE_DEBUG("AVDTP Address : %s AVCTP address: %s",
                           bd2str(&avdtp_addr, &addr1), bd2str(&rc_addr, &addr2));

        if (dev_blacklisted_for_absolute_volume(btif_rc_cb.rc_addr) ||
            bdcmp(avdtp_addr.address, rc_addr.address))
        {
            btif_rc_cb.rc_features &= ~BTA_AV_FEAT_ADV_CTRL;
        }

        if (btif_rc_cb.rc_features & BTA_AV_FEAT_BROWSE)
        {
            rc_features |= BTRC_FEAT_BROWSE;
        }
        if ( (btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL) &&
             (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG))
        {
            rc_features |= BTRC_FEAT_ABSOLUTE_VOLUME;
        }
        if (btif_rc_cb.rc_features & BTA_AV_FEAT_METADATA)
        {
            rc_features |= BTRC_FEAT_METADATA;
        }
        BTIF_TRACE_IMP("%s: rc_features=0x%x", __FUNCTION__, rc_features);

        HAL_CBACK(bt_rc_callbacks, remote_features_cb, &rc_addr, rc_features)

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        BTIF_TRACE_DEBUG("Checking for feature flags in btif_rc_handler with label %d",
                        btif_rc_cb.rc_vol_label);
        // Register for volume change on connect
        if(btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL &&
         btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG)
        {
            rc_transaction_t *p_transaction=NULL;
            bt_status_t status = BT_STATUS_NOT_READY;
            if(MAX_LABEL==btif_rc_cb.rc_vol_label)
            {
                status=get_transaction(&p_transaction);
            }
            else
            {
                p_transaction=get_transaction_by_lbl(btif_rc_cb.rc_vol_label);
                if(NULL!=p_transaction)
                {
                    BTIF_TRACE_DEBUG("register_volumechange already in progress for label %d",
                                      btif_rc_cb.rc_vol_label);
                    return;
                }
                else
                {
                    status=get_transaction(&p_transaction);
                }
            }

            if(BT_STATUS_SUCCESS == status && NULL!=p_transaction)
            {
                btif_rc_cb.rc_vol_label=p_transaction->lbl;
                register_volumechange(btif_rc_cb.rc_vol_label);
            }
        }
#endif
    }
    else
    {
    /*Disable all TG related bits if AVRCP TG feature is not enabled*/
        BTIF_TRACE_WARNING("Avrcp TG role not enabled, disabling TG specific featuremask");
        btif_rc_cb.rc_features &= ~BTA_AV_FEAT_ADV_CTRL;
        btif_rc_cb.rc_features &= ~BTA_AV_FEAT_BROWSE;
        btif_rc_cb.rc_features &= ~BTA_AV_FEAT_METADATA;
    }
}


/***************************************************************************
 *  Function       handle_rc_connect
 *
 *  - Argument:    tBTA_AV_RC_OPEN  RC open data structure
 *
 *  - Description: RC connection event handler
 *
 ***************************************************************************/
void handle_rc_connect (tBTA_AV_RC_OPEN *p_rc_open)
{
    BTIF_TRACE_IMP("%s: rc_handle: %d", __FUNCTION__, p_rc_open->rc_handle);
    bt_status_t result = BT_STATUS_SUCCESS;
    int i;
    char bd_str[18];
#if (AVRC_CTLR_INCLUDED == TRUE)
    bt_bdaddr_t rc_addr;
#endif

    if(p_rc_open->status == BTA_AV_SUCCESS)
    {
        //check if already some RC is connected
        if (btif_rc_cb.rc_connected)
        {
            BTIF_TRACE_ERROR("Got RC OPEN in connected state, Connected RC: %d \
                and Current RC: %d", btif_rc_cb.rc_handle,p_rc_open->rc_handle );
            if ((btif_rc_cb.rc_handle != p_rc_open->rc_handle)
                && (bdcmp(btif_rc_cb.rc_addr, p_rc_open->peer_addr)))
            {
                BTIF_TRACE_DEBUG("Got RC connected for some other handle");
                BTA_AvCloseRc(p_rc_open->rc_handle);
                return;
            }
        }
        memcpy(btif_rc_cb.rc_addr, p_rc_open->peer_addr, sizeof(BD_ADDR));
        btif_rc_cb.rc_features = p_rc_open->peer_features;
        btif_rc_cb.rc_vol_label=MAX_LABEL;
        btif_rc_cb.rc_volume=MAX_VOLUME;

        btif_rc_cb.rc_connected = TRUE;
        btif_rc_cb.rc_handle = p_rc_open->rc_handle;

        /* on locally initiated connection we will get remote features as part of connect */
        if (btif_rc_cb.rc_features != 0)
            handle_rc_features();
        if (bt_rc_callbacks)
        {
            result = uinput_driver_check();
            if(result == BT_STATUS_SUCCESS)
            {
                init_uinput();
            }
        }
        else
        {
            BTIF_TRACE_WARNING("Avrcp TG role not enabled, not initializing UInput");
        }
#if (AVRC_CTLR_INCLUDED == TRUE)
        bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
        /* report connection state if device is AVRCP target */
        if (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG)
        {
            HAL_CBACK(bt_rc_ctrl_callbacks, connection_state_cb, TRUE, &rc_addr);
            conn_status = TRUE;
        }
        else
        {
            BTIF_TRACE_ERROR("RC connection state not updated to upper layer");
            conn_status = FALSE;
        }
#endif
    }
    else
    {
        BTIF_TRACE_ERROR("%s Connect failed with error code: %d",
            __FUNCTION__, p_rc_open->status);
        btif_rc_cb.rc_connected = FALSE;
    }
}

/***************************************************************************
 *  Function       handle_rc_disconnect
 *
 *  - Argument:    tBTA_AV_RC_CLOSE     RC close data structure
 *
 *  - Description: RC disconnection event handler
 *
 ***************************************************************************/
void handle_rc_disconnect (tBTA_AV_RC_CLOSE *p_rc_close)
{
#if (AVRC_CTLR_INCLUDED == TRUE)
    bt_bdaddr_t rc_addr;
    tBTA_AV_FEAT features;
#endif
    BTIF_TRACE_IMP("%s: rc_handle: %d", __FUNCTION__, p_rc_close->rc_handle);
    if ((p_rc_close->rc_handle != btif_rc_cb.rc_handle)
        && (bdcmp(btif_rc_cb.rc_addr, p_rc_close->peer_addr)))
    {
        BTIF_TRACE_ERROR("Got disconnect of unknown device");
        return;
    }
    btif_rc_cb.rc_handle = 0;
    btif_rc_cb.rc_connected = FALSE;
    memset(btif_rc_cb.rc_addr, 0, sizeof(BD_ADDR));
    memset(btif_rc_cb.rc_notif, 0, sizeof(btif_rc_cb.rc_notif));
#if (AVRC_CTLR_INCLUDED == TRUE)
    features = btif_rc_cb.rc_features;
#endif
    btif_rc_cb.rc_features = 0;
    btif_rc_cb.rc_vol_label=MAX_LABEL;
    btif_rc_cb.rc_volume=MAX_VOLUME;
    init_all_transactions();
    if (bt_rc_callbacks != NULL)
    {
        close_uinput();
    }
    else
    {
        BTIF_TRACE_WARNING("Avrcp TG role not enabled, not closing UInput");
    }
#if (AVRC_CTLR_INCLUDED == TRUE)
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
#endif
    memset(btif_rc_cb.rc_addr, 0, sizeof(BD_ADDR));
#if (AVRC_CTLR_INCLUDED == TRUE)
    /* report connection state if device is AVRCP target */
    if (features & BTA_AV_FEAT_RCTG)
    {
        HAL_CBACK(bt_rc_ctrl_callbacks, connection_state_cb, FALSE, &rc_addr);
        conn_status = FALSE;
    }
#endif
}

/***************************************************************************
 *  Function       handle_rc_passthrough_cmd
 *
 *  - Argument:    tBTA_AV_RC rc_id   remote control command ID
 *                 tBTA_AV_STATE key_state status of key press
 *
 *  - Description: Remote control command handler
 *
 ***************************************************************************/
void handle_rc_passthrough_cmd ( tBTA_AV_REMOTE_CMD *p_remote_cmd)
{
    const char *status;
    int pressed, i;

    BTIF_TRACE_DEBUG("%s: p_remote_cmd->rc_id=%d", __FUNCTION__, p_remote_cmd->rc_id);
    if (p_remote_cmd == NULL)
        return;

    /* If AVRC is open and peer sends PLAY but there is no AVDT, then we queue-up this PLAY */
    if (p_remote_cmd)
    {
        /* queue AVRC PLAY if GAVDTP Open notification to app is pending (2 second timer) */
        if ((p_remote_cmd->rc_id == BTA_AV_RC_PLAY) && (!btif_av_is_connected()))
        {
            if (p_remote_cmd->key_state == AVRC_STATE_PRESS)
            {
                APPL_TRACE_WARNING("%s: AVDT not open, queuing the PLAY command", __FUNCTION__);
                btif_rc_cb.rc_pending_play = TRUE;
            }
            return;
        }

        if ((p_remote_cmd->rc_id == BTA_AV_RC_PAUSE) && (btif_rc_cb.rc_pending_play))
        {
            APPL_TRACE_WARNING("%s: Clear the pending PLAY on PAUSE received", __FUNCTION__);
            btif_rc_cb.rc_pending_play = FALSE;
            return;
        }
    }

    if(!btif_av_is_connected())
    {
        APPL_TRACE_WARNING("%s: AVDT not open, discarding pass-through command: %d",
                                                        __FUNCTION__, p_remote_cmd->rc_id);
        return;
    }

    if ((p_remote_cmd->rc_id == BTA_AV_RC_STOP) && (!btif_av_stream_started_ready()))
    {
        APPL_TRACE_WARNING("%s: Stream suspended, ignore STOP cmd",__FUNCTION__);
        return;
    }

    if (p_remote_cmd->key_state == AVRC_STATE_RELEASE) {
        status = "released";
        pressed = 0;
    } else {
        status = "pressed";
        pressed = 1;
    }

    if (p_remote_cmd->rc_id == BTA_AV_RC_FAST_FOR || p_remote_cmd->rc_id == BTA_AV_RC_REWIND) {
        HAL_CBACK(bt_rc_callbacks, passthrough_cmd_cb, p_remote_cmd->rc_id, pressed);
        return;
    }

    for (i = 0; key_map[i].name != NULL; i++) {
        if (p_remote_cmd->rc_id == key_map[i].avrcp) {
            BTIF_TRACE_DEBUG("%s: %s %s", __FUNCTION__, key_map[i].name, status);

           /* MusicPlayer uses a long_press_timeout of 1 second for PLAYPAUSE button
            * and maps that to autoshuffle. So if for some reason release for PLAY/PAUSE
            * comes 1 second after the press, the MediaPlayer UI goes into a bad state.
            * The reason for the delay could be sniff mode exit or some AVDTP procedure etc.
            * The fix is to generate a release right after the press and drown the 'actual'
            * release.
            */
            if ((key_map[i].release_quirk == 1) && (pressed == 0))
            {
                BTIF_TRACE_DEBUG("%s: AVRC %s Release Faked earlier, drowned now",
                                  __FUNCTION__, key_map[i].name);
                return;
            }
            send_key(uinput_fd, key_map[i].mapped_id, pressed);
            if ((key_map[i].release_quirk == 1) && (pressed == 1))
            {
                GKI_delay(30); // 30ms
                BTIF_TRACE_DEBUG("%s: AVRC %s Release quirk enabled, send release now",
                                  __FUNCTION__, key_map[i].name);
                send_key(uinput_fd, key_map[i].mapped_id, 0);
            }
            break;
        }
    }

    if (key_map[i].name == NULL)
        BTIF_TRACE_ERROR("%s AVRCP: unknown button 0x%02X %s", __FUNCTION__,
                        p_remote_cmd->rc_id, status);
}

/***************************************************************************
 *  Function       handle_rc_passthrough_rsp
 *
 *  - Argument:    tBTA_AV_REMOTE_RSP passthrough command response
 *
 *  - Description: Remote control passthrough response handler
 *
 ***************************************************************************/
void handle_rc_passthrough_rsp ( tBTA_AV_REMOTE_RSP *p_remote_rsp)
{
#if (AVRC_CTLR_INCLUDED == TRUE)
    const char *status;
    if (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG)
    {
        int key_state;
        if (p_remote_rsp->key_state == AVRC_STATE_RELEASE)
        {
            status = "released";
            key_state = 1;
        }
        else
        {
            status = "pressed";
            key_state = 0;
        }

        BTIF_TRACE_DEBUG("%s: rc_id=%d status=%s", __FUNCTION__, p_remote_rsp->rc_id, status);

        release_transaction(p_remote_rsp->label);
        HAL_CBACK(bt_rc_ctrl_callbacks, passthrough_rsp_cb, p_remote_rsp->rc_id, key_state);
    }
    else
    {
        BTIF_TRACE_ERROR("%s DUT does not support AVRCP controller role", __FUNCTION__);
    }
#else
    BTIF_TRACE_ERROR("%s AVRCP controller role is not enabled", __FUNCTION__);
#endif
}

void handle_uid_changed_notification(tBTA_AV_META_MSG *pmeta_msg, tAVRC_COMMAND *pavrc_command)
{
    tAVRC_RESPONSE avrc_rsp = {0};
    avrc_rsp.rsp.pdu = pavrc_command->pdu;
    avrc_rsp.rsp.status = AVRC_STS_NO_ERROR;
    avrc_rsp.rsp.opcode = pavrc_command->cmd.opcode;

    avrc_rsp.reg_notif.event_id = pavrc_command->reg_notif.event_id;
    avrc_rsp.reg_notif.param.uid_counter = 0;

    send_metamsg_rsp(pmeta_msg->rc_handle, pmeta_msg->label, AVRC_RSP_INTERIM, &avrc_rsp);
    send_metamsg_rsp(pmeta_msg->rc_handle, pmeta_msg->label, AVRC_RSP_CHANGED, &avrc_rsp);

}


/***************************************************************************
 *  Function       handle_rc_metamsg_cmd
 *
 *  - Argument:    tBTA_AV_VENDOR Structure containing the received
 *                          metamsg command
 *
 *  - Description: Remote control metamsg command handler (AVRCP 1.3)
 *
 ***************************************************************************/
void handle_rc_metamsg_cmd (tBTA_AV_META_MSG *pmeta_msg)
{
    /* Parse the metamsg command and pass it on to BTL-IFS */
    UINT8             scratch_buf[512] = {0};
    tAVRC_COMMAND    avrc_command = {0};
    tAVRC_STS status;
    int param_len;

    BTIF_TRACE_EVENT("+ %s", __FUNCTION__);

    if (pmeta_msg->p_msg->hdr.opcode != AVRC_OP_VENDOR)
    {
        BTIF_TRACE_WARNING("Invalid opcode: %x", pmeta_msg->p_msg->hdr.opcode);
        return;
    }
    if (pmeta_msg->len < 3)
    {
        BTIF_TRACE_WARNING("Invalid length.Opcode: 0x%x, len: 0x%x", pmeta_msg->p_msg->hdr.opcode,
            pmeta_msg->len);
        return;
    }

    if (pmeta_msg->code >= AVRC_RSP_NOT_IMPL)
    {
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
{
     rc_transaction_t *transaction=NULL;
     transaction=get_transaction_by_lbl(pmeta_msg->label);
     if(NULL!=transaction)
     {
        handle_rc_metamsg_rsp(pmeta_msg);
     }
     else
     {
         BTIF_TRACE_DEBUG("%s:Discard vendor dependent rsp. code: %d label:%d.",
             __FUNCTION__, pmeta_msg->code, pmeta_msg->label);
     }
     return;
}
#else
{
        BTIF_TRACE_DEBUG("%s:Received vendor dependent rsp. code: %d len: %d. Not processing it.",
            __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
        return;
}
#endif
      }

    status=AVRC_ParsCommand(pmeta_msg->p_msg, &avrc_command, scratch_buf, sizeof(scratch_buf));
    BTIF_TRACE_DEBUG("Received vendor command.code,PDU and label: %d, %d,%d",pmeta_msg->code,
                       avrc_command.cmd.pdu, pmeta_msg->label);

    if (status != AVRC_STS_NO_ERROR)
    {
        /* return error */
        BTIF_TRACE_WARNING("%s: Error in parsing received metamsg command. status: 0x%02x",
            __FUNCTION__, status);
        send_reject_response(pmeta_msg->rc_handle, pmeta_msg->label, avrc_command.pdu, status);
    }
    else
    {
        /* if RegisterNotification, add it to our registered queue */

        if (avrc_command.cmd.pdu == AVRC_PDU_REGISTER_NOTIFICATION)
        {
            UINT8 event_id = avrc_command.reg_notif.event_id;
            param_len = sizeof(tAVRC_REG_NOTIF_CMD);
            BTIF_TRACE_EVENT("%s:New register notification received.event_id:%s,label:0x%x,code:%x",
            __FUNCTION__,dump_rc_notification_event_id(event_id), pmeta_msg->label,pmeta_msg->code);
            btif_rc_cb.rc_notif[event_id-1].bNotify = TRUE;
            btif_rc_cb.rc_notif[event_id-1].label = pmeta_msg->label;

            if(event_id == AVRC_EVT_UIDS_CHANGE)
            {
                handle_uid_changed_notification(pmeta_msg, &avrc_command);
                return;
            }

        }

        BTIF_TRACE_EVENT("%s: Passing received metamsg command to app. pdu: %s",
            __FUNCTION__, dump_rc_pdu(avrc_command.cmd.pdu));

        /* Since handle_rc_metamsg_cmd() itself is called from
            *btif context, no context switching is required. Invoke
            * btif_rc_upstreams_evt directly from here. */
        btif_rc_upstreams_evt((uint16_t)avrc_command.cmd.pdu, &avrc_command, pmeta_msg->code,
                               pmeta_msg->label);
    }
}

/************************************************************************************
*  Function                 handle_get_folder_item_mediaplyerlist_cmd
*
* - Argument:               tBTA_AV_BROWSE_MSG  structure containing  the received
*                           browse message
*                           tAVRC_COMMAND       structure containing the commands
*                           to be updated
*                           UINT8 event,        variable having value of event.
*
*  - Description:           Handler for get media player list command
*
************************************************************************************/
UINT8 handle_get_folder_item_mediaplyerlist_cmd (tBTA_AV_BROWSE_MSG *pbrowse_msg,
                                                        tAVRC_COMMAND *cmd, UINT8 *event)
{
    UINT8 *p_length, *start_item, *end_item;
    UINT8 length, xx;
    *event = pbrowse_msg->p_msg->browse.p_browse_data[0] ;
    cmd->get_items.pdu = *event;
    //Check length
    p_length = &pbrowse_msg->p_msg->browse.p_browse_data[1];
    BE_STREAM_TO_UINT16(length, p_length);
    if (length < 10)
    {
        BTIF_TRACE_ERROR("GET_FOLDER_ITEMS: length error: =%d", length);
        return TRUE;
    }
    else
    {
        start_item = &pbrowse_msg->p_msg->browse.p_browse_data[4];
        BE_STREAM_TO_UINT32(cmd->get_items.start_item ,start_item);
        BTIF_TRACE_EVENT("pbrowse_msg start_item :%x",cmd->get_items.start_item);
        end_item  =  &pbrowse_msg->p_msg->browse.p_browse_data[8];
        BE_STREAM_TO_UINT32(cmd->get_items.end_item,end_item);
        BTIF_TRACE_EVENT("pbrowse_msg start_item :%x",cmd->get_items.end_item);
        cmd->get_items.attr_count = 0xff; /* in MediaPlayerList we don't have attr_id */
        //Update OPCODE
        cmd->get_items.opcode  = AVRC_OP_BROWSE;
        cmd->get_items.scope   = pbrowse_msg->p_msg->browse.p_browse_data[3] ;
        cmd->get_items.status = AVRC_STS_NO_ERROR ;
        for (xx = 0; xx < BTRC_MAX_ELEM_ATTR_SIZE ; xx++)
        {
            cmd->get_items.attrs[xx] = 0;
        }
        return FALSE;
    }
}

/************************************************************************************
*  Function                 handle_get_folder_item_filesystem_cmd
*
* - Argument:               tBTA_AV_BROWSE_MSG  structure containing  the received
*                           browse message
*                           tAVRC_COMMAND       structure containing the commands
*                           to be updated
*                           UINT8 event,        variable having value of event.
*
*  - Description:           Handler for get folder item command
*
************************************************************************************/
UINT8 handle_get_folder_item_filesystem_cmd (tBTA_AV_BROWSE_MSG *pbrowse_msg, tAVRC_COMMAND *cmd,
                                                 UINT8 *event)
{
    UINT8 *p_length, *start_item, *end_item, *p_data;
    UINT8 length, attr_count = 0, xx;
    *event = pbrowse_msg->p_msg->browse.p_browse_data[0] ;
    cmd->get_items.pdu = *event;
    //Check length
    p_length = &pbrowse_msg->p_msg->browse.p_browse_data[1];
    BE_STREAM_TO_UINT16(length, p_length);
    attr_count = pbrowse_msg->p_msg->browse.p_browse_data[12];
    BTIF_TRACE_ERROR("GET_FOLDER_ITEMS: attr_count: =%d", attr_count);
    switch (attr_count)
    {
        case 0xff:
            if (length != 10)
            {
                BTIF_TRACE_ERROR("GET_FOLDER_ITEMS: length error: =%d", length);
                return TRUE;
            }
            break;
        default:
            if (length != ((attr_count * 4) + 10))
            {
                BTIF_TRACE_ERROR("GET_FOLDER_ITEMS: length error: =%d", length);
                return TRUE;
            }
    }

    start_item = &pbrowse_msg->p_msg->browse.p_browse_data[4];
    BE_STREAM_TO_UINT32(cmd->get_items.start_item ,start_item);
    BTIF_TRACE_EVENT("pbrowse_msg start_item :%x",cmd->get_items.start_item);
    end_item  =  &pbrowse_msg->p_msg->browse.p_browse_data[8];
    BE_STREAM_TO_UINT32(cmd->get_items.end_item,end_item);
    BTIF_TRACE_EVENT("pbrowse_msg start_item :%x",cmd->get_items.end_item);
    cmd->get_items.attr_count = attr_count;
    if (attr_count == 0)
    {
        for (xx = 0; xx < BTRC_MAX_ELEM_ATTR_SIZE; xx++)
        {
            cmd->get_items.attrs[xx] = xx + 1;
        }
    }
    else if (attr_count == 0xff) /* no attribute requested */
    {
        BTIF_TRACE_DEBUG("No attribute requested");
    }
    else if (attr_count <= AVRC_MAX_ELEM_ATTR_SIZE)
    {
        p_data = &pbrowse_msg->p_msg->browse.p_browse_data[13];
        for (xx = 0; xx < attr_count; xx++)
            BE_STREAM_TO_UINT32(cmd->get_items.attrs[xx], p_data)
    }
    //Update OPCODE
    cmd->get_items.opcode  = AVRC_OP_BROWSE;
    cmd->get_items.scope   = pbrowse_msg->p_msg->browse.p_browse_data[3] ;
    cmd->get_items.status = AVRC_STS_NO_ERROR ;
    return FALSE;
}

/************************************************************************************
*  Function                 handle_rc_browsemsg_cmd
*
* - Argument:               tBTA_AV_BROWSE_MSG  structure containing  the recieved
*                           browse message
*
*  - Description:           Remote Control browse message handler
*
************************************************************************************/
void handle_rc_browsemsg_cmd (tBTA_AV_BROWSE_MSG *pbrowse_msg)
{
    UINT8 event;
    UINT16 length;
    tAVRC_COMMAND cmd;
    UINT8 *start_item, *p_length, *p_data;
    UINT8 *end_item;
    tAVRC_RESPONSE avrc_rsp;
    UINT8  dropmsg = TRUE;

    BTIF_TRACE_EVENT("+ %s", __FUNCTION__);
    BTIF_TRACE_EVENT("pbrowse_msg PDU_ID :%x",pbrowse_msg->p_msg->browse.p_browse_data[0]);
    BTIF_TRACE_EVENT("pbrowse_msg length :%x",pbrowse_msg->p_msg->browse.browse_len);
    switch(pbrowse_msg->p_msg->browse.p_browse_data[0])
    {
        case AVRC_PDU_GET_FOLDER_ITEMS:
        {
            UINT8 scope = pbrowse_msg->p_msg->browse.p_browse_data[3];
            switch (scope)
            {
                case AVRC_SCOPE_PLAYER_LIST:
                    dropmsg = handle_get_folder_item_mediaplyerlist_cmd(pbrowse_msg, &cmd, &event);
                break;
                case AVRC_SCOPE_FILE_SYSTEM:
                case AVRC_SCOPE_NOW_PLAYING:
                    dropmsg = handle_get_folder_item_filesystem_cmd(pbrowse_msg, &cmd, &event);
                break;
            }
            if (dropmsg == FALSE)
            {
                btif_rc_upstreams_evt(event,&cmd,0,pbrowse_msg->label);
            }
        }
        break;

        case AVRC_PDU_SET_BROWSED_PLAYER:
            event  = pbrowse_msg->p_msg->browse.p_browse_data[0] ;
            cmd.br_player.pdu     = event;
            //Check for length
            p_length = &pbrowse_msg->p_msg->browse.p_browse_data[1];
            BE_STREAM_TO_UINT16(length, p_length);
            if (length != 0x0002)
            {
                BTIF_TRACE_ERROR("SET_BROWSED_PLAYERlength error: = %d", length);
            }
            else
            {
                p_length = &pbrowse_msg->p_msg->browse.p_browse_data[3];
                BE_STREAM_TO_UINT16(cmd.br_player.player_id, p_length);
                cmd.br_player.opcode = AVRC_OP_BROWSE;
                btif_rc_upstreams_evt(event, &cmd, 0, pbrowse_msg->label);
                dropmsg = FALSE;
            }
        break;

        case AVRC_PDU_CHANGE_PATH:
            event  = pbrowse_msg->p_msg->browse.p_browse_data[0] ;
            cmd.chg_path.pdu = event;
            p_data = &pbrowse_msg->p_msg->browse.p_browse_data[1];
            BE_STREAM_TO_UINT16(length, p_data);
            if (length != 11)
            {
                BTIF_TRACE_ERROR("CHANGE_PATH length error: = %d",length);
            }
            else
            {
                p_data = &pbrowse_msg->p_msg->browse.p_browse_data[3];
                BE_STREAM_TO_UINT16(cmd.chg_path.uid_counter, p_data);
                cmd.chg_path.direction = pbrowse_msg->p_msg->browse.p_browse_data[5];
                cmd.chg_path.opcode = AVRC_OP_BROWSE;
                cmd.chg_path.status = AVRC_STS_NO_ERROR;
                p_data = &pbrowse_msg->p_msg->browse.p_browse_data[6];
                BE_STREAM_TO_UINT64(cmd.chg_path.folder_uid, p_data);
                btif_rc_upstreams_evt(event, &cmd, 0, pbrowse_msg->label);
                dropmsg = FALSE;
            }
        break;

        case AVRC_PDU_GET_ITEM_ATTRIBUTES:
        {
            UINT16 packet_len;
            UINT8  num_attr, index;
            event  = pbrowse_msg->p_msg->browse.p_browse_data[0] ;
            cmd.get_attrs.pdu = event;
            p_data = &pbrowse_msg->p_msg->browse.p_browse_data[1];
            BE_STREAM_TO_UINT16(packet_len, p_data);
            p_data = &pbrowse_msg->p_msg->browse.p_browse_data[14];
            BE_STREAM_TO_UINT8(num_attr, p_data);
            if (packet_len != ((num_attr * 4) + 12))
            {
                BTIF_TRACE_ERROR("Get Item Attributes length error: = %d",packet_len);
            }
            else
            {
                cmd.get_attrs.status = AVRC_STS_NO_ERROR;
                cmd.get_attrs.opcode = AVRC_OP_BROWSE;
                cmd.get_attrs.scope =  pbrowse_msg->p_msg->browse.p_browse_data[3];
                p_data = &pbrowse_msg->p_msg->browse.p_browse_data[4];
                BE_STREAM_TO_UINT64(cmd.get_attrs.uid, p_data);
                p_data = &pbrowse_msg->p_msg->browse.p_browse_data[12];
                BE_STREAM_TO_UINT16(cmd.get_attrs.uid_counter, p_data);
                cmd.get_attrs.attr_count = num_attr;
                if (num_attr == 0)
                {
                    /* remote requested all Attribute ID*/
                    for (index = 0; index < BTRC_MAX_ELEM_ATTR_SIZE; index++)
                    {
                        cmd.get_attrs.attrs[index] = index + 1;
                    }
                }
                else
                {
                    p_data = &pbrowse_msg->p_msg->browse.p_browse_data[15];
                    BTIF_TRACE_ERROR("GetItemAttr num_attr: = %d", cmd.get_attrs.attr_count);
                    for (index = 0; index < num_attr ; index++)
                    {
                        BE_STREAM_TO_UINT32(cmd.get_attrs.attrs[index], p_data);
                        BTIF_TRACE_ERROR("GetItemAttr attrid: = %d", cmd.get_attrs.attrs[index]);
                    }
                }
                btif_rc_upstreams_evt(event, &cmd, 0, pbrowse_msg->label);
                dropmsg = FALSE;
            }
        }
        break;

        default:
            BTIF_TRACE_ERROR("pbrowse_msg ERROR");
        break;
    }
    if (dropmsg == TRUE)
    {
        avrc_rsp.rsp.pdu    = pbrowse_msg->p_msg->browse.p_browse_data[0];
        avrc_rsp.rsp.status = AVRC_STS_BAD_CMD;
        avrc_rsp.rsp.opcode = AVRC_OP_BROWSE;
        BTIF_TRACE_ERROR("handle_rc_browsemsg_cmd: pbrowse_msg ERROR: %x", avrc_rsp.rsp.pdu);
        send_browsemsg_rsp(btif_rc_cb.rc_handle, pbrowse_msg->label, 0, &avrc_rsp);
    }

}


/***************************************************************************
 **
 ** Function       btif_rc_handler
 **
 ** Description    RC event handler
 **
 ***************************************************************************/
void btif_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data)
{
    BTIF_TRACE_IMP ("%s event:%s", __FUNCTION__, dump_rc_event(event));
    switch (event)
    {
        case BTA_AV_RC_OPEN_EVT:
        {
            BTIF_TRACE_DEBUG("Peer_features:%x", p_data->rc_open.peer_features);
            handle_rc_connect( &(p_data->rc_open) );
        }break;

        case BTA_AV_RC_CLOSE_EVT:
        {
            handle_rc_disconnect( &(p_data->rc_close) );
        }break;

        case BTA_AV_REMOTE_CMD_EVT:
        {
            if (bt_rc_callbacks != NULL)
            {
                BTIF_TRACE_DEBUG("rc_id:0x%x key_state:%d",
                    p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
                /** In race conditions just after 2nd AVRCP is connected
                 *  remote might send pass through commands, so check for
                 *  Rc handle before processing pass through commands
                 **/
                if (btif_rc_cb.rc_handle == p_data->remote_cmd.rc_handle)
                {
                    handle_rc_passthrough_cmd( (&p_data->remote_cmd) );
                }
                else
                {
                    BTIF_TRACE_DEBUG("Pass-through command for Invalid rc handle");
                }
            }
            else
            {
                BTIF_TRACE_ERROR("AVRCP TG role not up, drop passthrough commands");
            }
        }
        break;
#if (AVRC_CTLR_INCLUDED == TRUE)
        case BTA_AV_REMOTE_RSP_EVT:
        {
            BTIF_TRACE_DEBUG("RSP: rc_id:0x%x key_state:%d", p_data->remote_rsp.rc_id,
                               p_data->remote_rsp.key_state);
            handle_rc_passthrough_rsp( (&p_data->remote_rsp) );
        }
        break;
#endif
        case BTA_AV_RC_FEAT_EVT:
        {
            BTIF_TRACE_DEBUG("Peer_features:%x", p_data->rc_feat.peer_features);
            btif_rc_cb.rc_features = p_data->rc_feat.peer_features;
            handle_rc_features();
#if (AVRC_CTLR_INCLUDED == TRUE)
            bt_bdaddr_t rc_addr;
            bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
            if (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG &&
                btif_rc_cb.rc_connected == TRUE && conn_status == FALSE)
            {
                BTIF_TRACE_DEBUG("Update RC Connection State");
                HAL_CBACK(bt_rc_ctrl_callbacks, connection_state_cb, TRUE, &rc_addr);
                conn_status = TRUE;
            }
#endif
        }
        break;
        case BTA_AV_META_MSG_EVT:
        {
            if (bt_rc_callbacks != NULL)
            {
                BTIF_TRACE_DEBUG("BTA_AV_META_MSG_EVT  code:%d label:%d",
                                                p_data->meta_msg.code,
                                                p_data->meta_msg.label);
                BTIF_TRACE_DEBUG("  company_id:0x%x len:%d handle:%d",
                                            p_data->meta_msg.company_id,
                                            p_data->meta_msg.len,
                                            p_data->meta_msg.rc_handle);
                /* handle the metamsg command */
                handle_rc_metamsg_cmd(&(p_data->meta_msg));
                /* Free the Memory allocated for tAVRC_MSG */
            }
            else
            {
                BTIF_TRACE_ERROR("AVRCP TG role not up, drop meta commands");
            }
            GKI_freebuf(p_data->meta_msg.p_msg);
        }
        break;
        case BTA_AV_BROWSE_MSG_EVT:
        {
            if (bt_rc_callbacks != NULL)
            {
                BTIF_TRACE_DEBUG("BTA_AV_BROWSE_MSG_EVT  label:%d handle:%d",
                                                p_data->browse_msg.label,
                                                p_data->browse_msg.rc_handle);
                handle_rc_browsemsg_cmd(&(p_data->browse_msg));
            }
            else
            {
                BTIF_TRACE_ERROR("AVRCP TG role not up, drop browse commands");
            }
            GKI_freebuf(p_data->browse_msg.p_msg);
        }
        break;
        default:
            BTIF_TRACE_DEBUG("Unhandled RC event : 0x%x", event);
    }
}

/***************************************************************************
 **
 ** Function       btif_rc_get_connected_peer
 **
 ** Description    Fetches the connected headset's BD_ADDR if any
 **
 ***************************************************************************/
BOOLEAN btif_rc_get_connected_peer(BD_ADDR peer_addr)
{
    if (btif_rc_cb.rc_connected == TRUE) {
        bdcpy(peer_addr, btif_rc_cb.rc_addr);
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************
 **
 ** Function       btif_rc_check_handle_pending_play
 **
 ** Description    Clears the queued PLAY command. if bSend is TRUE, forwards to app
 **
 ***************************************************************************/

/* clear the queued PLAY command. if bSend is TRUE, forward to app */
void btif_rc_check_handle_pending_play (BD_ADDR peer_addr, BOOLEAN bSendToApp)
{
    UNUSED(peer_addr);

    BTIF_TRACE_DEBUG("%s: bSendToApp=%d", __FUNCTION__, bSendToApp);
    if (btif_rc_cb.rc_pending_play)
    {
        if (bSendToApp)
        {
            tBTA_AV_REMOTE_CMD remote_cmd;
            APPL_TRACE_DEBUG("%s: Sending queued PLAYED event to app", __FUNCTION__);

            memset (&remote_cmd, 0, sizeof(tBTA_AV_REMOTE_CMD));
            remote_cmd.rc_handle  = btif_rc_cb.rc_handle;
            remote_cmd.rc_id      = AVRC_ID_PLAY;
            remote_cmd.hdr.ctype  = AVRC_CMD_CTRL;
            remote_cmd.hdr.opcode = AVRC_OP_PASS_THRU;

            /* delay sending to app, else there is a timing issue in the framework,
             ** which causes the audio to be on th device's speaker. Delay between
             ** OPEN & RC_PLAYs
            */
            GKI_delay (200);
            /* send to app - both PRESSED & RELEASED */
            remote_cmd.key_state  = AVRC_STATE_PRESS;
            handle_rc_passthrough_cmd( &remote_cmd );

            GKI_delay (100);

            remote_cmd.key_state  = AVRC_STATE_RELEASE;
            handle_rc_passthrough_cmd( &remote_cmd );
        }
        btif_rc_cb.rc_pending_play = FALSE;
    }
}

/* Generic reject response */
static void send_reject_response (UINT8 rc_handle, UINT8 label, UINT8 pdu, UINT8 status)
{
    UINT8 ctype = AVRC_RSP_REJ;
    tAVRC_RESPONSE avrc_rsp;
    BT_HDR *p_msg = NULL;
    memset (&avrc_rsp, 0, sizeof(tAVRC_RESPONSE));

    avrc_rsp.rsp.opcode = opcode_from_pdu(pdu);
    avrc_rsp.rsp.pdu    = pdu;
    avrc_rsp.rsp.status = status;

    if (AVRC_STS_NO_ERROR == (status = AVRC_BldResponse(rc_handle, &avrc_rsp, &p_msg)) )
    {
        BTIF_TRACE_DEBUG("%s:Sending error notification to handle:%d. pdu:%s,status:0x%02x",
            __FUNCTION__, rc_handle, dump_rc_pdu(pdu), status);
        BTA_AvMetaRsp(rc_handle, label, ctype, p_msg);
    }
}

/***************************************************************************
 *  Function       send_metamsg_rsp
 *
 *  - Argument:
 *                  rc_handle     RC handle corresponding to the connected RC
 *                  label            Label of the RC response
 *                  code            Response type
 *                  pmetamsg_resp    Vendor response
 *
 *  - Description: Remote control metamsg response handler (AVRCP 1.3)
 *
 ***************************************************************************/
static void send_metamsg_rsp (UINT8 rc_handle, UINT8 label, tBTA_AV_CODE code,
    tAVRC_RESPONSE *pmetamsg_resp)
{
    UINT8 ctype;
    tAVRC_STS status;

    if (!pmetamsg_resp)
    {
        BTIF_TRACE_WARNING("%s: Invalid response received from application", __FUNCTION__);
        return;
    }

    BTIF_TRACE_EVENT("+%s: rc_handle: %d, label: %d, code: 0x%02x, pdu: %s", __FUNCTION__,
        rc_handle, label, code, dump_rc_pdu(pmetamsg_resp->rsp.pdu));

    if (pmetamsg_resp->rsp.status != AVRC_STS_NO_ERROR)
    {
        ctype = AVRC_RSP_REJ;
    }
    else
    {
        if ( code < AVRC_RSP_NOT_IMPL)
        {
            if (code == AVRC_CMD_NOTIF)
            {
               ctype = AVRC_RSP_INTERIM;
            }
            else if (code == AVRC_CMD_STATUS)
            {
               ctype = AVRC_RSP_IMPL_STBL;
            }
            else
            {
               ctype = AVRC_RSP_ACCEPT;
            }
        }
        else
        {
            ctype = code;
        }
    }
    /* if response is for register_notification, make sure the rc has
    actually registered for this */
    if((pmetamsg_resp->rsp.pdu == AVRC_PDU_REGISTER_NOTIFICATION) && (code == AVRC_RSP_CHANGED))
    {
        BOOLEAN bSent = FALSE;
        UINT8   event_id = pmetamsg_resp->reg_notif.event_id;
        BOOLEAN bNotify = (btif_rc_cb.rc_connected) && (btif_rc_cb.rc_notif[event_id-1].bNotify);

        /* de-register this notification for a CHANGED response */
        btif_rc_cb.rc_notif[event_id-1].bNotify = FALSE;
        BTIF_TRACE_DEBUG("%s rc_handle: %d. event_id: 0x%02d bNotify:%u", __FUNCTION__,
             btif_rc_cb.rc_handle, event_id, bNotify);
        if (bNotify)
        {
            BT_HDR *p_msg = NULL;
            tAVRC_STS status;

            if (AVRC_STS_NO_ERROR == (status = AVRC_BldResponse(btif_rc_cb.rc_handle,
                pmetamsg_resp, &p_msg)) )
            {
                BTIF_TRACE_DEBUG("%s Sending notification to rc_handle: %d. event_id: 0x%02d",
                     __FUNCTION__, btif_rc_cb.rc_handle, event_id);
                bSent = TRUE;
                BTA_AvMetaRsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_notif[event_id-1].label,
                    ctype, p_msg);
            }
            else
            {
                BTIF_TRACE_WARNING("%s failed to build metamsg response. status: 0x%02x",
                    __FUNCTION__, status);
            }

        }

        if (!bSent)
        {
            BTIF_TRACE_DEBUG("%s: Notification not sent, as there are no RC connections or the \
                CT has not subscribed for event_id: %s", __FUNCTION__, dump_rc_notification_event_id(event_id));
        }
    }
    else
    {
        /* All other commands go here */

        BT_HDR *p_msg = NULL;
        tAVRC_STS status;

        status = AVRC_BldResponse(rc_handle, pmetamsg_resp, &p_msg);

        if (status == AVRC_STS_NO_ERROR)
        {
            BTA_AvMetaRsp(rc_handle, label, ctype, p_msg);
        }
        else
        {
            BTIF_TRACE_ERROR("%s: failed to build metamsg response. status: 0x%02x",
                __FUNCTION__, status);
        }
    }
}

static UINT8 opcode_from_pdu(UINT8 pdu)
{
    UINT8 opcode = 0;

    switch (pdu)
    {
    case AVRC_PDU_NEXT_GROUP:
    case AVRC_PDU_PREV_GROUP: /* pass thru */
        opcode  = AVRC_OP_PASS_THRU;
        break;

    default: /* vendor */
        opcode  = AVRC_OP_VENDOR;
        break;
    }

    return opcode;
}

/****************************************************************************
* Function         send_browsemsg_rsp
*
* Arguments    -   rc_handle     RC handle corresponding to the connected RC
*                  label         Label of the RC response
*                  code          Response type---->Not needed for Browsing
*                  pmetamsg_resp Vendor response
*
* Description  -   Remote control browse Message Rsp
*
*******************************************************************************/
static void send_browsemsg_rsp (UINT8 rc_handle, UINT8 label, tBTA_AV_CODE code,
                                                       tAVRC_RESPONSE *pbrowsemsg_resp)
{
    tAVRC_STS status;
    BT_HDR *p_msg = NULL;

    if (!pbrowsemsg_resp)
    {
        BTIF_TRACE_WARNING("%s: Invalid response received from application", __FUNCTION__);
        return;
    }

    BTIF_TRACE_EVENT("+%s:rc_handle: %d, label: %d, code: 0x%02x, pdu: %s", __FUNCTION__,\
                      rc_handle, label, code, dump_rc_pdu(pbrowsemsg_resp->rsp.pdu));
    if (pbrowsemsg_resp->rsp.status != AVRC_STS_NO_ERROR)
    {
        BTIF_TRACE_ERROR("send_browsemsg_rsp **Error**");
    }
    /*Browse Command and Response structure are different
     *as comapared to Meta data response ,opcode and c-type
     *not part of browse response hence handling browse response
     *in seprate function
    */
    status = AVRC_BldBrowseResponse(rc_handle, pbrowsemsg_resp, &p_msg);
    if (status == AVRC_STS_NO_ERROR)
    {
        BTA_AvMetaRsp(rc_handle, label, 0, p_msg);
    }
    else
    {
        BTIF_TRACE_ERROR("%s: failed to build metamsg response. status: 0x%02x",
                __FUNCTION__, status);
    }
}

/****************************************************************************
* Function         app_sendbrowsemsg
*
* Arguments    -   index         Of array stored while recieving command
*                  avrc_rsp      Avrcp response from application
*
* Description  -   Send Browse message
*
*******************************************************************************/
int app_sendbrowsemsg(UINT8 index ,tAVRC_RESPONSE *avrc_rsp)
{
   SEND_BROWSEMSG_RSP(index ,avrc_rsp);
   return 0;
}


/*******************************************************************************
**
** Function         btif_rc_upstreams_evt
**
** Description      Executes AVRC UPSTREAMS events in btif context.
**
** Returns          void
**
*******************************************************************************/
static void btif_rc_upstreams_evt(UINT16 event, tAVRC_COMMAND *pavrc_cmd, UINT8 ctype, UINT8 label)
{
    BTIF_TRACE_IMP("%s pdu: %s handle: 0x%x ctype:%x label:%x", __FUNCTION__,
        dump_rc_pdu(pavrc_cmd->pdu), btif_rc_cb.rc_handle, ctype, label);

    switch (event)
    {
        case AVRC_PDU_GET_PLAY_STATUS:
        {
            BTIF_TRACE_DEBUG("AVRC_PDU_GET_PLAY_STATUS ");
            FILL_PDU_QUEUE(IDX_GET_PLAY_STATUS_RSP, ctype, label, TRUE)
            HAL_CBACK(bt_rc_callbacks, get_play_status_cb);
        }
        break;
        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
        {
            BTIF_TRACE_DEBUG("AVRC_PDU_LIST_PLAYER_APP_ATTR ");
            FILL_PDU_QUEUE(IDX_LIST_APP_ATTR_RSP, ctype, label, TRUE)
            HAL_CBACK(bt_rc_callbacks, list_player_app_attr_cb);
        }
        break;
        case AVRC_PDU_LIST_PLAYER_APP_VALUES:
        {
            BTIF_TRACE_DEBUG("AVRC_PDU_LIST_PLAYER_APP_VALUES =%d" ,pavrc_cmd->list_app_values.attr_id);
            if (pavrc_cmd->list_app_values.attr_id == 0)
            {
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                break;
            }
            FILL_PDU_QUEUE(IDX_LIST_APP_VALUE_RSP, ctype, label, TRUE)
            HAL_CBACK(bt_rc_callbacks, list_player_app_values_cb ,pavrc_cmd->list_app_values.attr_id);
        }
        break;
        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
        {
            btrc_player_attr_t player_attr[BTRC_MAX_ELEM_ATTR_SIZE];
            UINT8 player_attr_num;
            BTIF_TRACE_DEBUG("PLAYER_APP_VALUE PDU 0x13 = %d",pavrc_cmd->get_cur_app_val.num_attr);
            if ((pavrc_cmd->get_cur_app_val.num_attr == 0) ||
                  (pavrc_cmd->get_cur_app_val.num_attr > BTRC_MAX_ELEM_ATTR_SIZE))
            {
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                break;
            }
            memset( player_attr, 0, sizeof(player_attr));
            for (player_attr_num = 0 ; player_attr_num < pavrc_cmd->get_cur_app_val.num_attr;
                                                                            ++player_attr_num)
            {
                player_attr[player_attr_num] = pavrc_cmd->get_cur_app_val.attrs[player_attr_num];
            }
            FILL_PDU_QUEUE(IDX_GET_CURR_APP_VAL_RSP, ctype, label, TRUE)
            HAL_CBACK(bt_rc_callbacks, get_player_app_value_cb ,
                                               pavrc_cmd->get_cur_app_val.num_attr, player_attr );
        }
        break;
        case AVRC_PDU_SET_PLAYER_APP_VALUE:
        {
            btrc_player_settings_t attr;
            UINT8 count;
            tAVRC_RESPONSE avrc_rsp;
            if ((pavrc_cmd->set_app_val.num_val== 0) ||
                              (pavrc_cmd->set_app_val.num_val > BTRC_MAX_ELEM_ATTR_SIZE))
            {
                send_reject_response (btif_rc_cb.rc_handle, label,
                                       pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                break;
            }
            else
            {
                for(count = 0; count < pavrc_cmd->set_app_val.num_val ; ++count)
                {
                    attr.attr_ids[count] = pavrc_cmd->set_app_val.p_vals[count].attr_id ;
                    attr.attr_values[count]= pavrc_cmd->set_app_val.p_vals[count].attr_val;
                }
                attr.num_attr  =  pavrc_cmd->set_app_val.num_val ;
                FILL_PDU_QUEUE(IDX_SET_APP_VAL_RSP, ctype, label, TRUE)
                HAL_CBACK(bt_rc_callbacks, set_player_app_value_cb, &attr );
            }
        }
        break;
        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
        {
            btrc_player_attr_t player_attr_txt [BTRC_MAX_ELEM_ATTR_SIZE];
            UINT8 count_txt = 0 ;
            if ((pavrc_cmd->get_app_attr_txt.num_attr == 0) ||
                   (pavrc_cmd->get_app_attr_txt.num_attr > BTRC_MAX_ELEM_ATTR_SIZE))
            {
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
            }
            else
            {
                for (count_txt = 0;count_txt < pavrc_cmd->get_app_attr_txt.num_attr ; ++count_txt)
                {
                    player_attr_txt[count_txt] = pavrc_cmd->get_app_attr_txt.attrs[count_txt];
                }
                FILL_PDU_QUEUE(IDX_GET_APP_ATTR_TXT_RSP, ctype, label, TRUE)
                HAL_CBACK(bt_rc_callbacks, get_player_app_attrs_text_cb,
                            pavrc_cmd->get_app_attr_txt.num_attr, player_attr_txt );
            }
        }
        break;
        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
        {
            if (pavrc_cmd->get_app_val_txt.attr_id == 0 ||
                     pavrc_cmd->get_app_val_txt.attr_id > AVRC_PLAYER_VAL_GROUP_REPEAT)
            {
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                break;
            }
            if (pavrc_cmd->get_app_val_txt.num_val == 0)
            {
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
            }
            else
            {
                FILL_PDU_QUEUE(IDX_GET_APP_VAL_TXT_RSP, ctype, label, TRUE)
                HAL_CBACK(bt_rc_callbacks, get_player_app_values_text_cb,
                          pavrc_cmd->get_app_val_txt.attr_id, pavrc_cmd->get_app_val_txt.num_val,
                          pavrc_cmd->get_app_val_txt.vals);
            }
        }
        break;
        case AVRC_PDU_GET_ELEMENT_ATTR:
        {
            btrc_media_attr_t element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];
            UINT8 num_attr;
             memset(&element_attrs, 0, sizeof(element_attrs));
            if (pavrc_cmd->get_elem_attrs.num_attr == 0)
            {
                /* CT requests for all attributes */
                int attr_cnt;
                num_attr = BTRC_MAX_ELEM_ATTR_SIZE;
                for (attr_cnt = 0; attr_cnt < BTRC_MAX_ELEM_ATTR_SIZE; attr_cnt++)
                {
                    element_attrs[attr_cnt] = attr_cnt + 1;
                }
            }
            else if (pavrc_cmd->get_elem_attrs.num_attr == 0xFF)
            {
                /* 0xff indicates, no attributes requested - reject */
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu,
                    AVRC_STS_BAD_PARAM);
                return;
            }
            else
            {
                int attr_cnt, filled_attr_count;

                num_attr = 0;
                /* Attribute IDs from 1 to AVRC_MAX_NUM_MEDIA_ATTR_ID are only valid,
                 * hence HAL definition limits the attributes to AVRC_MAX_NUM_MEDIA_ATTR_ID.
                 * Fill only valid entries.
                 */
                for (attr_cnt = 0; (attr_cnt < pavrc_cmd->get_elem_attrs.num_attr) &&
                    (num_attr < AVRC_MAX_NUM_MEDIA_ATTR_ID); attr_cnt++)
                {
                    if ((pavrc_cmd->get_elem_attrs.attrs[attr_cnt] > 0) &&
                        (pavrc_cmd->get_elem_attrs.attrs[attr_cnt] <= AVRC_MAX_NUM_MEDIA_ATTR_ID))
                    {
                        /* Skip the duplicate entries : PTS sends duplicate entries for Fragment cases
                         */
                        for (filled_attr_count = 0; filled_attr_count < num_attr; filled_attr_count++)
                        {
                            if (element_attrs[filled_attr_count] == pavrc_cmd->get_elem_attrs.attrs[attr_cnt])
                                break;
                        }
                        if (filled_attr_count == num_attr)
                        {
                            element_attrs[num_attr] = pavrc_cmd->get_elem_attrs.attrs[attr_cnt];
                            num_attr++;
                        }
                    }
                }
            }
            FILL_PDU_QUEUE(IDX_GET_ELEMENT_ATTR_RSP, ctype, label, TRUE);
            HAL_CBACK(bt_rc_callbacks, get_element_attr_cb, num_attr, element_attrs);
        }
        break;
        case AVRC_PDU_REGISTER_NOTIFICATION:
        {
            if (pavrc_cmd->reg_notif.event_id == BTRC_EVT_PLAY_POS_CHANGED &&
                pavrc_cmd->reg_notif.param == 0)
            {
                BTIF_TRACE_WARNING("%s Device registering position changed with illegal param 0.",
                    __FUNCTION__);
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                /* de-register this notification for a rejected response */
                btif_rc_cb.rc_notif[BTRC_EVT_PLAY_POS_CHANGED - 1].bNotify = FALSE;
                return;
            }
            HAL_CBACK(bt_rc_callbacks, register_notification_cb, pavrc_cmd->reg_notif.event_id,
                pavrc_cmd->reg_notif.param);
        }
        break;
        case AVRC_PDU_INFORM_DISPLAY_CHARSET:
        {
            tAVRC_RESPONSE avrc_rsp;
            BTIF_TRACE_EVENT("%s() AVRC_PDU_INFORM_DISPLAY_CHARSET", __FUNCTION__);
            if(btif_rc_cb.rc_connected == TRUE)
            {
                memset(&(avrc_rsp.inform_charset), 0, sizeof(tAVRC_RSP));
                avrc_rsp.inform_charset.opcode=opcode_from_pdu(AVRC_PDU_INFORM_DISPLAY_CHARSET);
                avrc_rsp.inform_charset.pdu=AVRC_PDU_INFORM_DISPLAY_CHARSET;
                avrc_rsp.inform_charset.status=AVRC_STS_NO_ERROR;
                send_metamsg_rsp(btif_rc_cb.rc_handle, label, ctype, &avrc_rsp);
            }
        }
        break;
        case AVRC_PDU_SET_ADDRESSED_PLAYER:
        {
            btrc_status_t status_code = AVRC_STS_NO_ERROR;
            BTIF_TRACE_EVENT("%s() AVRC_PDU_SET_ADDRESSED_PLAYER", __FUNCTION__);
            if (!btif_hf_is_call_idle())
            {
                set_addrplayer_rsp(ERR_PLAYER_NOT_ADDRESED); // send reject if call is in progress
                return;
            }
            if (btif_rc_cb.rc_connected == TRUE)
            {
                FILL_PDU_QUEUE(IDX_SET_ADDRESS_PLAYER_RSP, ctype, label, TRUE);
                HAL_CBACK(bt_rc_callbacks, set_addrplayer_cb, pavrc_cmd->addr_player.player_id);
            }
        }
        break;
        case AVRC_PDU_GET_FOLDER_ITEMS:
        {
            tAVRC_RESPONSE avrc_rsp;
            btrc_getfolderitem_t getfolder;
            btrc_browse_folderitem_t scope;
            UINT8 player[] = "MusicPlayer1";
            tAVRC_ITEM tem[1];
            UINT8  index, numAttr;
            BTIF_TRACE_EVENT("%s()AVRC_PDU_GET_FOLDER_ITEMS", __FUNCTION__);
            FILL_PDU_QUEUE(IDX_GET_FOLDER_ITEMS_RSP,ctype, label, TRUE);
            BTIF_TRACE_EVENT("rc_connected: %d",btif_rc_cb.rc_connected);
            if (btif_rc_cb.rc_connected == TRUE)
            {
                getfolder.start_item = pavrc_cmd->get_items.start_item;
                getfolder.end_item   = pavrc_cmd->get_items.end_item;
                getfolder.size       = AVCT_GetBrowseMtu(btif_rc_cb.rc_handle);
                getfolder.attr_count = pavrc_cmd->get_items.attr_count;
                scope                = (btrc_browse_folderitem_t)pavrc_cmd->get_items.scope;
                if (getfolder.attr_count == 255)
                {
                    numAttr = 0;
                }
                else
                {
                    if (getfolder.attr_count == 0)
                    {
                        numAttr = 7;
                        for (index = 0; index < BTRC_MAX_ELEM_ATTR_SIZE; index++)
                        {
                            getfolder.attrs[index] = index + 1;
                        }
                    }
                    else
                    {
                        numAttr = getfolder.attr_count;
                        for (index = 0; index < numAttr; index++)
                        {
                            getfolder.attrs[index] = pavrc_cmd->get_items.attrs[index];
                            BTIF_TRACE_ERROR("getfolder[%d] = %d", index, getfolder.\
                                                                        attrs[index]);
                            BTIF_TRACE_ERROR("pavrc_cmd->get_items.attrs[%d] = %d",\
                                            index, pavrc_cmd->get_items.attrs[index]);
                        }
                    }
                }
                HAL_CBACK(bt_rc_callbacks, get_folderitems_cb, scope, &getfolder);
            }
        }
        break;
        case AVRC_PDU_SET_BROWSED_PLAYER:
        {
            BTIF_TRACE_EVENT("%s() AVRC_PDU_SET_BROWSED_PLAYER", __FUNCTION__);
            if (btif_rc_cb.rc_connected == TRUE)
            {
                FILL_PDU_QUEUE(IDX_SET_BROWSE_PLAYER_RSP, ctype, label, TRUE);
                HAL_CBACK(bt_rc_callbacks, set_browsed_player_cb, pavrc_cmd->br_player.player_id);
            }
        }
        break;
        case AVRC_PDU_CHANGE_PATH:
        {
            BTIF_TRACE_EVENT("%s() AVRC_PDU_CHANGE_PATH", __FUNCTION__);
            if (btif_rc_cb.rc_connected == TRUE)
            {
                FILL_PDU_QUEUE(IDX_CHANGE_PATH_RSP, ctype, label, TRUE);
                HAL_CBACK(bt_rc_callbacks, change_path_cb, pavrc_cmd->chg_path.direction, \
                                                            pavrc_cmd->chg_path.folder_uid);
            }
        }
        break;
        case AVRC_PDU_GET_ITEM_ATTRIBUTES:
        {
            UINT8 num_attr =  pavrc_cmd->get_attrs.attr_count;
            UINT8 index, num_attr_requested = 0;
            BTIF_TRACE_EVENT("%s() AVRC_PDU_GET_ITEM_ATTRIBUTES", __FUNCTION__);
            btrc_media_attr_t element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];
            memset(&element_attrs, 0, sizeof(element_attrs));
            if (num_attr == 0)
            {
                /* CT requests for all attributes */
                for (index = 0; index < BTRC_MAX_ELEM_ATTR_SIZE; index++)
                {
                    element_attrs[index] = index + 1;
                }
                num_attr_requested = 7; /* get all seven */
            }
            else if (num_attr == 0xFF)
            {
                /* 0xff indicates, no attributes requested - reject */
                send_reject_response (btif_rc_cb.rc_handle, label, pavrc_cmd->pdu,
                    AVRC_STS_BAD_PARAM);
                return;
            }
            else
            {
                /* Attribute IDs from 1 to BTRC_MAX_ELEM_ATTR_SIZE are only valid,
                 * hence HAL definition limits the attributes to BTRC_MAX_ELEM_ATTR_SIZE.
                 * Fill only valid entries.
                 */
                for (index = 0; (index < num_attr) && (num_attr <= BTRC_MAX_ELEM_ATTR_SIZE); index++)
                {
                    if ((pavrc_cmd->get_attrs.attrs[index] > 0) &&
                        (pavrc_cmd->get_attrs.attrs[index] <= BTRC_MAX_ELEM_ATTR_SIZE))
                    {
                        element_attrs[index] = pavrc_cmd->get_attrs.attrs[index];
                        BTIF_TRACE_ERROR("element_attrs[%d]: %d", index, element_attrs[index]);
                    }
                }
                num_attr_requested = index;
                BTIF_TRACE_ERROR("num_attr_requested: %d", num_attr_requested);
            }

            if (btif_rc_cb.rc_connected == TRUE)
            {
                FILL_PDU_QUEUE(IDX_GET_ITEM_ATTR_RSP, ctype, label, TRUE);
                HAL_CBACK(bt_rc_callbacks, get_item_attr_cb, pavrc_cmd->get_attrs.scope,
                        pavrc_cmd->get_attrs.uid, num_attr_requested, element_attrs);
            }
        }
        break;
        case AVRC_PDU_PLAY_ITEM:
        {
            BTIF_TRACE_EVENT("%s() AVRC_PDU_PLAY_ITEM", __FUNCTION__);
            if (btif_rc_cb.rc_connected == TRUE)
            {
                FILL_PDU_QUEUE(IDX_PLAY_ITEM_RSP, ctype, label, TRUE);
                HAL_CBACK(bt_rc_callbacks, play_item_cb, pavrc_cmd->play_item.scope,
                        pavrc_cmd->play_item.uid);
            }
        }
        break;
        default:
        {
            send_reject_response(btif_rc_cb.rc_handle, label, pavrc_cmd->pdu,
                 (pavrc_cmd->pdu == AVRC_PDU_SEARCH)? AVRC_STS_SEARCH_NOT_SUP:
                                                                AVRC_STS_BAD_CMD);
        }
        break;
    }

}


/*******************************************************************************
**
** Function         btif_rc_upstreams_rsp_evt
**
** Description      Executes AVRC UPSTREAMS response events in btif context.
**
** Returns          void
**
*******************************************************************************/
static void btif_rc_upstreams_rsp_evt(UINT16 event, tAVRC_RESPONSE *pavrc_resp, UINT8 ctype, UINT8 label)
{
    BTIF_TRACE_IMP("%s pdu: %s handle: 0x%x ctype:%x label:%x", __FUNCTION__,
        dump_rc_pdu(pavrc_resp->pdu), btif_rc_cb.rc_handle, ctype, label);

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    switch (event)
    {
        case AVRC_PDU_REGISTER_NOTIFICATION:
        {
             if(AVRC_RSP_CHANGED==ctype)
                 btif_rc_cb.rc_volume=pavrc_resp->reg_notif.param.volume;
             HAL_CBACK(bt_rc_callbacks, volume_change_cb, pavrc_resp->reg_notif.param.volume,ctype)
        }
        break;

        case AVRC_PDU_SET_ABSOLUTE_VOLUME:
        {
            BTIF_TRACE_DEBUG("Set absolute volume change event received: volume %d,ctype %d",
                pavrc_resp->volume.volume,ctype);
            if(AVRC_RSP_ACCEPT==ctype)
                btif_rc_cb.rc_volume=pavrc_resp->volume.volume;
            HAL_CBACK(bt_rc_callbacks,volume_change_cb,pavrc_resp->volume.volume,ctype)
        }
        break;

        default:
            return;
    }
#endif
}

/************************************************************************************
**  AVRCP API Functions
************************************************************************************/

/*******************************************************************************
**
** Function         init
**
** Description      Initializes the AVRC interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t init(btrc_callbacks_t* callbacks )
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    bt_status_t result = BT_STATUS_SUCCESS;

    if (bt_rc_callbacks)
        return BT_STATUS_DONE;

    bt_rc_callbacks = callbacks;
    memset (&btif_rc_cb, 0, sizeof(btif_rc_cb));
    btif_rc_cb.rc_vol_label=MAX_LABEL;
    btif_rc_cb.rc_volume=MAX_VOLUME;
    lbl_init();

    return result;
}

/*******************************************************************************
**
** Function         init_ctrl
**
** Description      Initializes the AVRC interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t init_ctrl(btrc_ctrl_callbacks_t* callbacks )
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    bt_status_t result = BT_STATUS_SUCCESS;

    if (bt_rc_ctrl_callbacks)
        return BT_STATUS_DONE;

    bt_rc_ctrl_callbacks = callbacks;
    memset (&btif_rc_cb, 0, sizeof(btif_rc_cb));
    btif_rc_cb.rc_vol_label=MAX_LABEL;
    btif_rc_cb.rc_volume=MAX_VOLUME;
    lbl_init();

    return result;
}

/***************************************************************************
**
** Function         get_play_status_rsp
**
** Description      Returns the current play status.
**                      This method is called in response to
**                      GetPlayStatus request.
**
** Returns          bt_status_t
**
***************************************************************************/
static bt_status_t get_play_status_rsp(btrc_play_status_t play_status, uint32_t song_len,
    uint32_t song_pos)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;
    CHECK_RC_CONNECTED
    memset(&(avrc_rsp.get_play_status), 0, sizeof(tAVRC_GET_PLAY_STATUS_RSP));
    avrc_rsp.get_play_status.song_len = song_len;
    avrc_rsp.get_play_status.song_pos = song_pos;
    avrc_rsp.get_play_status.play_status = play_status;

    avrc_rsp.get_play_status.pdu = AVRC_PDU_GET_PLAY_STATUS;
    avrc_rsp.get_play_status.opcode = opcode_from_pdu(AVRC_PDU_GET_PLAY_STATUS);
    avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_PLAY_STATUS_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}


/**************************************************************************
**
** Function         list_player_app_attr_rsp
**
** Description      ListPlayerApplicationSettingAttributes (PDU ID: 0x11)
**                  This method is callled in response to PDU 0x11
**
** Returns          bt_status_t
**
****************************************************************************/
static bt_status_t  list_player_app_attr_rsp( uint8_t num_attr, btrc_player_attr_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;

    CHECK_RC_CONNECTED
    memset(&(avrc_rsp.list_app_attr), 0, sizeof(tAVRC_LIST_APP_ATTR_RSP));
    if (num_attr == 0)
    {
        avrc_rsp.list_app_attr.status = AVRC_STS_BAD_PARAM;
    }
    else
    {
        avrc_rsp.list_app_attr.num_attr = num_attr;
        for (i = 0 ; i < num_attr ; ++i)
        {
            avrc_rsp.list_app_attr.attrs[i] = p_attrs[i];
        }
        avrc_rsp.list_app_attr.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.list_app_attr.pdu  = AVRC_PDU_LIST_PLAYER_APP_ATTR ;
    avrc_rsp.list_app_attr.opcode = opcode_from_pdu(AVRC_PDU_LIST_PLAYER_APP_ATTR);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_LIST_APP_ATTR_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function list_player_app_value_rsp
**
** Description      ListPlayerApplicationSettingValues (PDU ID: 0x12)
                    This method is called in response to PDU 0x12
************************************************************************/
static bt_status_t  list_player_app_value_rsp( uint8_t num_val, uint8_t *value)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;

    CHECK_RC_CONNECTED
    memset(&(avrc_rsp.list_app_values), 0, sizeof(tAVRC_LIST_APP_VALUES_RSP));
    if ((num_val == 0) || (num_val > AVRC_MAX_APP_ATTR_SIZE))
    {
        avrc_rsp.list_app_values.status = AVRC_STS_BAD_PARAM;
    }
    else
    {
        avrc_rsp.list_app_values.num_val = num_val;
        for (i = 0; i < num_val; ++i)
        {
            avrc_rsp.list_app_values.vals[i] = value[i];
        }
        avrc_rsp.list_app_values.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.list_app_values.pdu   = AVRC_PDU_LIST_PLAYER_APP_VALUES;
    avrc_rsp.list_app_attr.opcode  = opcode_from_pdu(AVRC_PDU_LIST_PLAYER_APP_VALUES);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_LIST_APP_VALUE_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}


/**********************************************************************
**
** Function  get_player_app_value_rsp
**
** Description  This methos is called in response to PDU ID 0x13
**
***********************************************************************/
static bt_status_t get_player_app_value_rsp(btrc_player_settings_t *p_vals)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;
    tAVRC_APP_SETTING app_sett[AVRC_MAX_APP_ATTR_SIZE];

    CHECK_RC_CONNECTED
    memset(&(avrc_rsp.get_cur_app_val) ,0 , sizeof(tAVRC_GET_CUR_APP_VALUE_RSP));
    avrc_rsp.get_cur_app_val.p_vals   = app_sett ;
    //Check for Error Condition
    if ((p_vals == NULL) || (p_vals->num_attr== 0) || (p_vals->num_attr > AVRC_MAX_APP_ATTR_SIZE))
    {
        avrc_rsp.get_cur_app_val.status = AVRC_STS_BAD_PARAM;
    }
    else if (p_vals->num_attr <= BTRC_MAX_APP_SETTINGS)
    {
        memset(app_sett, 0, sizeof(tAVRC_APP_SETTING)*p_vals->num_attr );
        //update num_val
        avrc_rsp.get_cur_app_val.num_val  = p_vals->num_attr ;
        avrc_rsp.get_cur_app_val.p_vals   = app_sett ;
        for (i = 0; i < p_vals->num_attr; ++i)
        {
            app_sett[i].attr_id  = p_vals->attr_ids[i] ;
            app_sett[i].attr_val = p_vals->attr_values[i];
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, num_element:%d",
                           __FUNCTION__, (unsigned int)app_sett[i].attr_id,
                              app_sett[i].attr_val ,p_vals->num_attr );
        }
        //Update PDU , status aind
        avrc_rsp.get_cur_app_val.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.get_cur_app_val.pdu = AVRC_PDU_GET_CUR_PLAYER_APP_VALUE;
    avrc_rsp.get_cur_app_val.opcode = opcode_from_pdu(AVRC_PDU_GET_CUR_PLAYER_APP_VALUE);
    SEND_METAMSG_RSP(IDX_GET_CURR_APP_VAL_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/********************************************************************
**
** Function     set_player_app_value_rsp
**
** Description  This method is called in response to
**              application value
**
** Return       bt_staus_t
**
*******************************************************************/
static bt_status_t set_player_app_value_rsp (btrc_status_t rsp_status )
{
    tAVRC_RESPONSE avrc_rsp;
    tAVRC_RSP    set_app_val;

    CHECK_RC_CONNECTED
    avrc_rsp.set_app_val.opcode = opcode_from_pdu(AVRC_PDU_SET_PLAYER_APP_VALUE);
    avrc_rsp.set_app_val.pdu    =  AVRC_PDU_SET_PLAYER_APP_VALUE ;
    avrc_rsp.set_app_val.status =  rsp_status ;
    SEND_METAMSG_RSP(IDX_SET_APP_VAL_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/********************************************************************
**
** Function      get_player_app_attr_text_rsp
**
** Description   This method is called in response to get player
**               applicaton attribute text response
**
**
*******************************************************************/
static bt_status_t get_player_app_attr_text_rsp(int num_attr, btrc_player_setting_text_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    tAVRC_APP_SETTING_TEXT attr_txt[AVRC_MAX_APP_ATTR_SIZE];
    int i;

    CHECK_RC_CONNECTED
    if (num_attr == 0)
    {
        avrc_rsp.get_app_attr_txt.status = AVRC_STS_BAD_PARAM;
    }
    else
    {
        for (i =0; i< num_attr; ++i)
        {
            attr_txt[i].charset_id = AVRC_CHARSET_ID_UTF8;
            attr_txt[i].attr_id = p_attrs[i].id ;
            attr_txt[i].str_len = (UINT8)strnlen((char *)p_attrs[i].text, BTRC_MAX_ATTR_STR_LEN);
            attr_txt[i].p_str       = p_attrs[i].text ;
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, str_len:%d, str:%s",
                                  __FUNCTION__, (unsigned int)attr_txt[i].attr_id,
                                  attr_txt[i].charset_id , attr_txt[i].str_len, attr_txt[i].p_str);
        }
        avrc_rsp.get_app_attr_txt.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.get_app_attr_txt.p_attrs = attr_txt ;
    avrc_rsp.get_app_attr_txt.num_attr = (UINT8)num_attr;
    avrc_rsp.get_app_attr_txt.pdu = AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT;
    avrc_rsp.get_app_attr_txt.opcode = opcode_from_pdu(AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_APP_ATTR_TXT_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/********************************************************************
**
** Function      get_player_app_value_text_rsp
**
** Description   This method is called in response to Player application
**               value text
**
** Return        bt_status_t
**
*******************************************************************/
static bt_status_t get_player_app_value_text_rsp(int num_attr, btrc_player_setting_text_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    tAVRC_APP_SETTING_TEXT attr_txt[AVRC_MAX_APP_ATTR_SIZE];
    int i;

    CHECK_RC_CONNECTED
    if (num_attr == 0)
    {
        avrc_rsp.get_app_val_txt.status = AVRC_STS_BAD_PARAM;
    }
    else
    {
        for (i =0; i< num_attr; ++i)
        {
            attr_txt[i].charset_id = AVRC_CHARSET_ID_UTF8;
            attr_txt[i].attr_id  = p_attrs[i].id ;
            attr_txt[i].str_len  = (UINT8)strnlen((char *)p_attrs[i].text ,BTRC_MAX_ATTR_STR_LEN );
            attr_txt[i].p_str    = p_attrs[i].text ;
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, str_len:%d, str:%s",
                                  __FUNCTION__, (unsigned int)attr_txt[i].attr_id,
                                 attr_txt[i].charset_id , attr_txt[i].str_len,attr_txt[i].p_str);
        }
        avrc_rsp.get_app_val_txt.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.get_app_val_txt.p_attrs = attr_txt;
    avrc_rsp.get_app_val_txt.num_attr = (UINT8)num_attr;
    avrc_rsp.get_app_val_txt.pdu = AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT;
    avrc_rsp.get_app_val_txt.opcode = opcode_from_pdu(AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_APP_VAL_TXT_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/***************************************************************************
**
** Function         get_element_attr_rsp
**
** Description      Returns the current songs' element attributes
**                  in text.
**
** Returns          bt_status_t
**
***************************************************************************/
static bt_status_t get_element_attr_rsp(uint8_t num_attr, btrc_element_attr_val_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;
    uint8_t j;
    tAVRC_ATTR_ENTRY element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];
    CHECK_RC_CONNECTED
    memset(element_attrs, 0, sizeof(tAVRC_ATTR_ENTRY) * num_attr);

    if (num_attr == 0)
    {
        avrc_rsp.get_play_status.status = AVRC_STS_BAD_PARAM;
    }
    else
    {
        for (i=0; i<num_attr; i++)
        {
            element_attrs[i].attr_id = p_attrs[i].attr_id;
            element_attrs[i].name.charset_id = AVRC_CHARSET_ID_UTF8;
            element_attrs[i].name.str_len = (UINT16)strlen((char *)p_attrs[i].text);
            element_attrs[i].name.p_str = p_attrs[i].text;
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, str_len:%d, str:%s",
                __FUNCTION__, (unsigned int)element_attrs[i].attr_id,
                element_attrs[i].name.charset_id, element_attrs[i].name.str_len,
                element_attrs[i].name.p_str);
        }
        avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.get_elem_attrs.num_attr = num_attr;
    avrc_rsp.get_elem_attrs.p_attrs = element_attrs;
    avrc_rsp.get_elem_attrs.pdu = AVRC_PDU_GET_ELEMENT_ATTR;
    avrc_rsp.get_elem_attrs.opcode = opcode_from_pdu(AVRC_PDU_GET_ELEMENT_ATTR);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_ELEMENT_ATTR_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/***************************************************************************
**
** Function         register_notification_rsp
**
** Description      Response to the register notification request.
**                      in text.
**
** Returns          bt_status_t
**
***************************************************************************/
static bt_status_t register_notification_rsp(btrc_event_id_t event_id,
    btrc_notification_type_t type, btrc_register_notification_t *p_param)
{
    tAVRC_RESPONSE avrc_rsp;
    CHECK_RC_CONNECTED
    BTIF_TRACE_IMP("## %s ## event_id:%s", __FUNCTION__, dump_rc_notification_event_id(event_id));
    if (btif_rc_cb.rc_notif[event_id-1].bNotify == FALSE)
    {
        BTIF_TRACE_ERROR("Avrcp Event id not registered: event_id = %x", event_id);
        return BT_STATUS_NOT_READY;
    }
    memset(&(avrc_rsp.reg_notif), 0, sizeof(tAVRC_REG_NOTIF_RSP));
    avrc_rsp.reg_notif.event_id = event_id;

    switch(event_id)
    {
        case BTRC_EVT_PLAY_STATUS_CHANGED:
            avrc_rsp.reg_notif.param.play_status = p_param->play_status;
            /* Clear remote suspend flag, as remote device issues
             * suspend within 3s after pause, and DUT within 3s
             * initiates Play
            */
            if (avrc_rsp.reg_notif.param.play_status == PLAY_STATUS_PLAYING)
                btif_av_clear_remote_suspend_flag();
            break;
        case BTRC_EVT_TRACK_CHANGE:
            memcpy(&(avrc_rsp.reg_notif.param.track), &(p_param->track), sizeof(btrc_uid_t));
            break;
        case BTRC_EVT_PLAY_POS_CHANGED:
            avrc_rsp.reg_notif.param.play_pos = p_param->song_pos;
            break;
        case BTRC_EVT_APP_SETTINGS_CHANGED:
            avrc_rsp.reg_notif.param.player_setting.num_attr = p_param->player_setting.num_attr;
            memcpy(&avrc_rsp.reg_notif.param.player_setting.attr_id,
                                       p_param->player_setting.attr_ids, 2);
            memcpy(&avrc_rsp.reg_notif.param.player_setting.attr_value,
                                       p_param->player_setting.attr_values, 2);
            break;
        case BTRC_EVT_ADDRESSED_PLAYER_CHANGED:
            avrc_rsp.reg_notif.param.addr_player.player_id = p_param->player_id;
            avrc_rsp.reg_notif.param.addr_player.uid_counter = 0;
            break;
        case BTRC_EVT_AVAILABLE_PLAYERS_CHANGED:
            avrc_rsp.reg_notif.param.evt  = 0x0a;
            break;
        case BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED:
            avrc_rsp.reg_notif.param.evt  = 0x09;
            break;
        default:
            BTIF_TRACE_WARNING("%s : Unhandled event ID : 0x%x", __FUNCTION__, event_id);
            return BT_STATUS_UNHANDLED;
    }

    avrc_rsp.reg_notif.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_rsp.reg_notif.opcode = opcode_from_pdu(AVRC_PDU_REGISTER_NOTIFICATION);
    if (type == BTRC_NOTIFICATION_TYPE_REJECT)
    {
        /* Spec AVRCP 1.5 ,section 6.9.2.2, on completion
         * of the addressed player changed notificatons the TG shall
         * complete all player specific notification with AV/C C-type
         * Rejected with error code Addressed Player changed.
         * This will happen in case when music player has changed
         * Application should take care of sending reject response.
        */
        avrc_rsp.get_play_status.status = AVRC_STS_ADDR_PLAYER_CHG;
    }
    else
    {
        avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    }

    /* Send the response. */
    send_metamsg_rsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_notif[event_id-1].label,
        ((type == BTRC_NOTIFICATION_TYPE_INTERIM)?AVRC_CMD_NOTIF:AVRC_RSP_CHANGED), &avrc_rsp);
    return BT_STATUS_SUCCESS;
}


/***************************************************************************
**
** Function         get_folderitem_rsp
**
** Description      Response to Get Folder Items , PDU 0x71
**
** Returns          bt_status_t
**
***************************************************************************/
static bt_status_t get_folderitem_rsp(btrc_folder_list_entries_t *rsp)
{
    tAVRC_RESPONSE avrc_rsp;
    CHECK_RC_CONNECTED
    tAVRC_ITEM item[MAX_FOLDER_RSP_SUPPORT]; //Number of players that could be supported
    UINT8  index, i, xx, media_attr_cnt;
    UINT8 *p_conversion;

    BTIF_TRACE_EVENT("%s() AVRC_PDU_GET_FOLDER_ITEMS", __FUNCTION__);
    index                             = IDX_GET_FOLDER_ITEMS_RSP ;
    avrc_rsp.get_items.pdu            = AVRC_PDU_GET_FOLDER_ITEMS;
    avrc_rsp.get_items.opcode         = AVRC_OP_BROWSE;
    avrc_rsp.get_items.uid_counter    = rsp->uid_counter;
    avrc_rsp.get_items.status         = rsp->status ;//4 means SUCCESS
    avrc_rsp.get_items.item_count     = 0;
    BTIF_TRACE_EVENT("status =%d, item_count =%d",rsp->status, rsp->item_count);

    for (i=0; (i < rsp->item_count && i < MAX_FOLDER_RSP_SUPPORT) ; ++i)
    {
        item[i].item_type = rsp->p_item_list[i].item_type;
        BTIF_TRACE_EVENT("item_type  = %d", rsp->p_item_list[i].item_type);
        switch (item[i].item_type)
        {
            case AVRC_ITEM_PLAYER:
                memcpy(item[i].u.player.features, rsp->p_item_list[i].u.player.features,
                            AVRC_FEATURE_MASK_SIZE);
                item[i].u.player.major_type      =  rsp->p_item_list[i].u.player.major_type;
                item[i].u.player.sub_type        =  rsp->p_item_list[i].u.player.sub_type;
                item[i].u.player.play_status     =  rsp->p_item_list[i].u.player.play_status;
                item[i].u.player.player_id       =  rsp->p_item_list[i].u.player.player_id;
                item[i].u.player.name.charset_id =  rsp->p_item_list[i].u.player.name.charset_id;
                item[i].u.player.name.str_len    =  rsp->p_item_list[i].u.player.name.str_len;
                item[i].u.player.name.p_str      =  rsp->p_item_list[i].u.player.name.p_str;
                ++avrc_rsp.get_items.item_count;
            break;

            case AVRC_ITEM_FOLDER:
                item[i].u.folder.type            =  rsp->p_item_list[i].u.folder.type;
                item[i].u.folder.playable        =  rsp->p_item_list[i].u.folder.playable;
                {
                    p_conversion = (UINT8*)&(rsp->p_item_list[i].u.folder.uid);
                    for (xx = 0; xx < AVRC_UID_SIZE; xx++)
                    {
                        ((UINT8 *) item[i].u.folder.uid)[AVRC_UID_SIZE - (xx + 1)] = \
                                                                        *p_conversion++;
                    }
                }

                item[i].u.folder.name.charset_id =  rsp->p_item_list[i].u.folder.name.charset_id;
                item[i].u.folder.name.str_len    =  rsp->p_item_list[i].u.folder.name.str_len;
                item[i].u.folder.name.p_str      =  rsp->p_item_list[i].u.folder.name.p_str;
                ++avrc_rsp.get_items.item_count;
            break;

            case AVRC_ITEM_MEDIA:
                item[i].u.media.type             =  rsp->p_item_list[i].u.media.type;
                {
                    p_conversion = (UINT8*)&(rsp->p_item_list[i].u.media.uid);
                    //BE_STREAM_TO_ARRAY(p_conversion, item[i].u.folder.uid, AVRC_UID_SIZE);
                    for (xx = 0; xx < AVRC_UID_SIZE; xx++)
                    {
                        ((UINT8 *) item[i].u.folder.uid)[AVRC_UID_SIZE - (xx + 1)] = \
                                                                        *p_conversion++;
                    }
                }
                item[i].u.media.name.charset_id  =  rsp->p_item_list[i].u.media.name.charset_id;
                item[i].u.media.name.str_len     =  rsp->p_item_list[i].u.media.name.str_len;
                item[i].u.media.name.p_str       =  rsp->p_item_list[i].u.media.name.p_str;
                media_attr_cnt                   =  rsp->p_item_list[i].u.media.attr_count;
                item[i].u.media.attr_count       =  rsp->p_item_list[i].u.media.attr_count;
                BTIF_TRACE_ERROR("attr count = %d", media_attr_cnt);
                if (media_attr_cnt > 0)
                {
                    if ((item[i].u.media.p_attr_list = \
                    (tAVRC_ATTR_ENTRY *)GKI_getbuf((UINT16)(media_attr_cnt * \
                                                        sizeof(tAVRC_ATTR_ENTRY)))) != NULL)
                    {
                        for (xx = 0; xx < media_attr_cnt; xx++)
                        {
                            item[i].u.media.p_attr_list[xx].attr_id = \
                                rsp->p_item_list[i].u.media.p_attr_list[xx].attr_id;
                            item[i].u.media.p_attr_list[xx].name.charset_id = \
                                rsp->p_item_list[i].u.media.p_attr_list[xx].name.charset_id;
                            item[i].u.media.p_attr_list[xx].name.str_len = \
                                rsp->p_item_list[i].u.media.p_attr_list[xx].name.str_len;
                            item[i].u.media.p_attr_list[xx].name.p_str = \
                                rsp->p_item_list[i].u.media.p_attr_list[xx].name.p_str;
                            BTIF_TRACE_ERROR("attr_id = %d", item[i].u.media.p_attr_list[xx].\
                                attr_id);
                            BTIF_TRACE_ERROR("str_len = %d", item[i].u.media.p_attr_list[xx].\
                                name.str_len);
                        }
                    }
                    else
                    {
                        BTIF_TRACE_ERROR("Not enough buffer allocated to accomodate attributes");
                        item[i].u.media.attr_count =  0;
                    }
                }
                ++avrc_rsp.get_items.item_count;
            break;

            default:
                return BT_STATUS_UNHANDLED;
            break;
        }
    }
    if (avrc_rsp.get_items.item_count == 0) {
        /*As per spec Send proper Error if no Music App is registered.*/
        avrc_rsp.get_items.status = AVRC_STS_BAD_RANGE;
    }
    avrc_rsp.get_items.p_item_list = item;
    app_sendbrowsemsg(IDX_GET_FOLDER_ITEMS_RSP ,&avrc_rsp);
    BTIF_TRACE_ERROR("free attr list");
    for (i=0; (i < rsp->item_count && i < MAX_FOLDER_RSP_SUPPORT) ; ++i)
    {
        if (item[i].item_type == AVRC_ITEM_MEDIA)
        {
            if (rsp->p_item_list[i].u.media.attr_count > 0)
            {
                GKI_freebuf(item[i].u.media.p_attr_list);
            }
        }
    }
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function        set_addrplayer_rsp
**
** Description     Response to Set Addressed Player , PDU 0x60
**
** Return          status
**
*********************************************************************/

static bt_status_t set_addrplayer_rsp(btrc_status_t status_code)
{
    tAVRC_RESPONSE avrc_rsp;
    CHECK_RC_CONNECTED
    avrc_rsp.addr_player.status = status_code;
    avrc_rsp.addr_player.opcode = opcode_from_pdu(AVRC_PDU_SET_ADDRESSED_PLAYER);
    avrc_rsp.addr_player.pdu    = AVRC_PDU_SET_ADDRESSED_PLAYER;
    /* Send the response */
    SEND_METAMSG_RSP(IDX_SET_ADDRESS_PLAYER_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function        set_browseplayer_rsp
**
** Description     Response to Set Browsed Player , PDU 0x70
**
** Return          status
**
*********************************************************************/

static bt_status_t set_browseplayer_rsp(btrc_set_browsed_player_rsp_t *p_param)
{
    tAVRC_RESPONSE avrc_rsp;

    CHECK_RC_CONNECTED
    avrc_rsp.br_player.pdu = AVRC_PDU_SET_BROWSED_PLAYER;
    avrc_rsp.br_player.folder_depth = p_param->folder_depth;
    avrc_rsp.br_player.charset_id = p_param->charset_id;
    avrc_rsp.br_player.num_items = p_param->num_items;
    avrc_rsp.br_player.opcode = opcode_from_pdu(AVRC_PDU_SET_BROWSED_PLAYER);
    avrc_rsp.br_player.status = p_param->status;
    avrc_rsp.br_player.uid_counter = p_param->uid_counter;
    avrc_rsp.br_player.p_folders = (tAVRC_NAME*)p_param->p_folders;
    /* Send the response */
    SEND_BROWSEMSG_RSP(IDX_SET_BROWSE_PLAYER_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function        changepath_rsp
**
** Description     Response to Change Path , PDU 0x60
**
** Return          status
**
*********************************************************************/

static bt_status_t changepath_rsp(uint8_t status_code, uint32_t item_count)
{
    tAVRC_RESPONSE avrc_rsp;

    CHECK_RC_CONNECTED
    avrc_rsp.chg_path.num_items = item_count;
    avrc_rsp.chg_path.opcode = opcode_from_pdu(AVRC_PDU_CHANGE_PATH);
    avrc_rsp.chg_path.pdu = AVRC_PDU_CHANGE_PATH;
    avrc_rsp.chg_path.status = status_code;
    /* Send the response */
    SEND_BROWSEMSG_RSP(IDX_CHANGE_PATH_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function        playitem_rsp
**
** Description     Response to Play Item , PDU 0x60
**
** Return          status
**
*********************************************************************/

static bt_status_t playitem_rsp(uint8_t status_code)
{
    tAVRC_RESPONSE avrc_rsp;

    CHECK_RC_CONNECTED
    avrc_rsp.play_item.status = status_code;
    avrc_rsp.play_item.opcode = opcode_from_pdu(AVRC_PDU_PLAY_ITEM);
    avrc_rsp.play_item.pdu    = AVRC_PDU_PLAY_ITEM;
    /* Send the response */
    SEND_METAMSG_RSP(IDX_PLAY_ITEM_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/**********************************************************************
**
** Function        get_itemattr_rsp
**
** Description     Response to Get Item , PDU 0x60
**
** Return          status
**
*********************************************************************/

static bt_status_t get_itemattr_rsp(uint8_t num_attr, btrc_element_attr_val_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    UINT32 i;
    uint8_t j;
    tAVRC_ATTR_ENTRY element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];

    CHECK_RC_CONNECTED
    memset(element_attrs, 0, sizeof(tAVRC_ATTR_ENTRY) * num_attr);

    if (num_attr == 0)
    {
        avrc_rsp.get_attrs.status = AVRC_STS_INTERNAL_ERR;
    }
    else
    {
        for (i=0; i<num_attr; i++)
        {
            element_attrs[i].attr_id = p_attrs[i].attr_id;
            element_attrs[i].name.charset_id = AVRC_CHARSET_ID_UTF8;
            element_attrs[i].name.str_len = (UINT16)strlen((char *)p_attrs[i].text);
            element_attrs[i].name.p_str = p_attrs[i].text;
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, str_len:%d, str:%s",
                __FUNCTION__, (unsigned int)element_attrs[i].attr_id,
                element_attrs[i].name.charset_id, element_attrs[i].name.str_len,
                element_attrs[i].name.p_str);
        }
        avrc_rsp.get_attrs.status = AVRC_STS_NO_ERROR;
    }
    avrc_rsp.get_attrs.attr_count = num_attr;
    avrc_rsp.get_attrs.p_attr_list = element_attrs;
    avrc_rsp.get_attrs.pdu = AVRC_PDU_GET_ITEM_ATTRIBUTES;
    avrc_rsp.get_attrs.opcode = opcode_from_pdu(AVRC_PDU_GET_ITEM_ATTRIBUTES);
    /* Send the response */
    SEND_BROWSEMSG_RSP(IDX_GET_ITEM_ATTR_RSP, &avrc_rsp);
    return BT_STATUS_SUCCESS;
}

/***************************************************************************
**
** Function         set_volume
**
** Description      Send current volume setting to remote side.
**                  Support limited to SetAbsoluteVolume
**                  This can be enhanced to support Relative Volume (AVRCP 1.0).
**                  With RelateVolume, we will send VOLUME_UP/VOLUME_DOWN
**                  as opposed to absolute volume level
** volume: Should be in the range 0-127. bit7 is reseved and cannot be set
**
** Returns          bt_status_t
**
***************************************************************************/
static bt_status_t set_volume(uint8_t volume)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    CHECK_RC_CONNECTED
    tAVRC_STS status = BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction=NULL;

    if(btif_rc_cb.rc_volume==volume)
    {
        status=BT_STATUS_DONE;
        BTIF_TRACE_ERROR("%s: volume value already set earlier: 0x%02x",__FUNCTION__, volume);
        return status;
    }

    if ((btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) &&
        (btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL))
    {
        tAVRC_COMMAND avrc_cmd = {0};
        BT_HDR *p_msg = NULL;

        BTIF_TRACE_DEBUG("%s: Peer supports absolute volume. newVolume=%d", __FUNCTION__, volume);
        avrc_cmd.volume.opcode = AVRC_OP_VENDOR;
        avrc_cmd.volume.pdu = AVRC_PDU_SET_ABSOLUTE_VOLUME;
        avrc_cmd.volume.status = AVRC_STS_NO_ERROR;
        avrc_cmd.volume.volume = volume;

        if (AVRC_BldCommand(&avrc_cmd, &p_msg) == AVRC_STS_NO_ERROR)
        {
            bt_status_t tran_status=get_transaction(&p_transaction);
            if(BT_STATUS_SUCCESS == tran_status && NULL!=p_transaction)
            {
                BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                                   __FUNCTION__,p_transaction->lbl);
                BTA_AvMetaCmd(btif_rc_cb.rc_handle,p_transaction->lbl, AVRC_CMD_CTRL, p_msg);
                status =  BT_STATUS_SUCCESS;
            }
            else
            {
                if(NULL!=p_msg)
                   GKI_freebuf(p_msg);
                BTIF_TRACE_ERROR("%s: failed to obtain transaction details. status: 0x%02x",
                                    __FUNCTION__, tran_status);
                status = BT_STATUS_FAIL;
            }
        }
        else
        {
            BTIF_TRACE_ERROR("%s: failed to build absolute volume command. status: 0x%02x",
                                __FUNCTION__, status);
            status = BT_STATUS_FAIL;
        }
    }
    else
        status=BT_STATUS_NOT_READY;
    return status;
}


/***************************************************************************
**
** Function         register_volumechange
**
** Description     Register for volume change notification from remote side.
**
** Returns          void
**
***************************************************************************/

static void register_volumechange (UINT8 lbl)
{
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tAVRC_STS BldResp=AVRC_STS_BAD_CMD;
    UINT16 rv = 0;
    bt_status_t tran_status;
    rc_transaction_t *p_transaction=NULL;

    BTIF_TRACE_DEBUG("%s called with label:%d",__FUNCTION__,lbl);

    avrc_cmd.cmd.opcode=0x00;
    avrc_cmd.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_cmd.reg_notif.event_id = AVRC_EVT_VOLUME_CHANGE;
    avrc_cmd.reg_notif.status = AVRC_STS_NO_ERROR;

    BldResp=AVRC_BldCommand(&avrc_cmd, &p_msg);
    if(AVRC_STS_NO_ERROR==BldResp && p_msg)
    {
         p_transaction=get_transaction_by_lbl(lbl);
         if(NULL!=p_transaction)
         {
             BTA_AvMetaCmd(btif_rc_cb.rc_handle,p_transaction->lbl, AVRC_CMD_NOTIF, p_msg);
             BTIF_TRACE_DEBUG("%s:BTA_AvMetaCmd called",__FUNCTION__);
         }
         else
         {
            if(NULL!=p_msg)
               GKI_freebuf(p_msg);
            BTIF_TRACE_ERROR("%s transaction not obtained with label: %d",__FUNCTION__,lbl);
         }
    }
    else
        BTIF_TRACE_ERROR("%s failed to build command:%d",__FUNCTION__,BldResp);
}


/***************************************************************************
**
** Function         handle_rc_metamsg_rsp
**
** Description      Handle RC metamessage response
**
** Returns          void
**
***************************************************************************/
static void handle_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg)
{
    tAVRC_RESPONSE    avrc_response = {0};
    UINT8             scratch_buf[512] = {0};
    tAVRC_STS status = BT_STATUS_UNSUPPORTED;

    if(AVRC_OP_VENDOR==pmeta_msg->p_msg->hdr.opcode &&(AVRC_RSP_CHANGED==pmeta_msg->code
      || AVRC_RSP_INTERIM==pmeta_msg->code || AVRC_RSP_ACCEPT==pmeta_msg->code
      || AVRC_RSP_REJ==pmeta_msg->code || AVRC_RSP_NOT_IMPL==pmeta_msg->code))
    {
        status=AVRC_ParsResponse(pmeta_msg->p_msg, &avrc_response, scratch_buf, sizeof(scratch_buf));
        BTIF_TRACE_DEBUG("%s: code %d,event ID %d,PDU %x,parsing status %d, label:%d",
          __FUNCTION__,pmeta_msg->code,avrc_response.reg_notif.event_id,avrc_response.reg_notif.pdu,
          status, pmeta_msg->label);

        if (status != AVRC_STS_NO_ERROR)
        {
            if(AVRC_PDU_REGISTER_NOTIFICATION==avrc_response.rsp.pdu
                && AVRC_EVT_VOLUME_CHANGE==avrc_response.reg_notif.event_id
                && btif_rc_cb.rc_vol_label==pmeta_msg->label)
            {
                btif_rc_cb.rc_vol_label=MAX_LABEL;
                release_transaction(btif_rc_cb.rc_vol_label);
            }
            else if(AVRC_PDU_SET_ABSOLUTE_VOLUME==avrc_response.rsp.pdu)
            {
                release_transaction(pmeta_msg->label);
            }
            return;
        }
        else if(AVRC_PDU_REGISTER_NOTIFICATION==avrc_response.rsp.pdu
            && AVRC_EVT_VOLUME_CHANGE==avrc_response.reg_notif.event_id
            && btif_rc_cb.rc_vol_label!=pmeta_msg->label)
            {
                // Just discard the message, if the device sends back with an incorrect label
                BTIF_TRACE_DEBUG("%s:Discarding register notfn in rsp.code: %d and label %d",
                __FUNCTION__, pmeta_msg->code, pmeta_msg->label);
                return;
            }
    }
    else
    {
        BTIF_TRACE_DEBUG("%s:Received vendor dependent in adv ctrl rsp. code: %d len: %d. Not processing it.",
        __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
        return;
    }

    if(AVRC_PDU_REGISTER_NOTIFICATION==avrc_response.rsp.pdu
        && AVRC_EVT_VOLUME_CHANGE==avrc_response.reg_notif.event_id
        && AVRC_RSP_CHANGED==pmeta_msg->code)
     {
         /* re-register for volume change notification */
         // Do not re-register for rejected case, as it might get into endless loop
         register_volumechange(btif_rc_cb.rc_vol_label);
     }
     else if(AVRC_PDU_SET_ABSOLUTE_VOLUME==avrc_response.rsp.pdu)
     {
          /* free up the label here */
          release_transaction(pmeta_msg->label);
     }

     BTIF_TRACE_EVENT("%s: Passing received metamsg response to app. pdu: %s",
             __FUNCTION__, dump_rc_pdu(avrc_response.pdu));
     btif_rc_upstreams_rsp_evt((uint16_t)avrc_response.rsp.pdu, &avrc_response, pmeta_msg->code,
                                pmeta_msg->label);
}


/***************************************************************************
**
** Function         cleanup
**
** Description      Closes the AVRC interface
**
** Returns          void
**
***************************************************************************/
static void cleanup()
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    close_uinput();
    if (bt_rc_callbacks)
    {
        bt_rc_callbacks = NULL;
    }
    memset(&btif_rc_cb, 0, sizeof(btif_rc_cb_t));
    lbl_destroy();
    BTIF_TRACE_EVENT("## %s ## completed", __FUNCTION__);
}

/***************************************************************************
**
** Function         cleanup_ctrl
**
** Description      Closes the AVRC Controller interface
**
** Returns          void
**
***************************************************************************/
static void cleanup_ctrl()
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);

    if (bt_rc_ctrl_callbacks)
    {
        bt_rc_ctrl_callbacks = NULL;
    }
    memset(&btif_rc_cb, 0, sizeof(btif_rc_cb_t));
    lbl_destroy();
    BTIF_TRACE_EVENT("## %s ## completed", __FUNCTION__);
}

static bt_status_t send_passthrough_cmd(bt_bdaddr_t *bd_addr, uint8_t key_code, uint8_t key_state)
{
    tAVRC_STS status = BT_STATUS_UNSUPPORTED;
#if (AVRC_CTLR_INCLUDED == TRUE)
    CHECK_RC_CONNECTED
    rc_transaction_t *p_transaction=NULL;
    BTIF_TRACE_DEBUG("%s: key-code: %d, key-state: %d", __FUNCTION__,
                                                    key_code, key_state);
    if (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG)
    {
        tAVRC_MSG_PASS avrc_cmd;
        bt_status_t tran_status = get_transaction(&p_transaction);
        if(BT_STATUS_SUCCESS == tran_status && NULL != p_transaction)
        {
            BTA_AvRemoteCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                (tBTA_AV_RC)key_code, (tBTA_AV_STATE)key_state);
            status =  BT_STATUS_SUCCESS;
            BTIF_TRACE_DEBUG("%s: succesfully sent passthrough command to BTA", __FUNCTION__);
        }
        else
        {
            status =  BT_STATUS_FAIL;
            BTIF_TRACE_DEBUG("%s: error in fetching transaction", __FUNCTION__);
        }
    }
    else
    {
        status =  BT_STATUS_FAIL;
        BTIF_TRACE_DEBUG("%s: feature not supported", __FUNCTION__);
    }
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

static const btrc_interface_t bt_rc_interface = {
    sizeof(bt_rc_interface),
    init,
    get_play_status_rsp,
    list_player_app_attr_rsp,     /* list_player_app_attr_rsp */
    list_player_app_value_rsp,    /* list_player_app_value_rsp */
    get_player_app_value_rsp,     /* get_player_app_value_rsp PDU 0x13*/
    get_player_app_attr_text_rsp, /* get_player_app_attr_text_rsp */
    get_player_app_value_text_rsp,/* get_player_app_value_text_rsp */
    get_element_attr_rsp,
    set_player_app_value_rsp,     /* set_player_app_value_rsp */
    register_notification_rsp,
    set_volume,
    get_folderitem_rsp,
    set_addrplayer_rsp,
    set_browseplayer_rsp,
    changepath_rsp,
    playitem_rsp,
    get_itemattr_rsp,
    cleanup,
};

static const btrc_ctrl_interface_t bt_rc_ctrl_interface = {
    sizeof(bt_rc_ctrl_interface),
    init_ctrl,
    send_passthrough_cmd,
    cleanup_ctrl,
};

/*******************************************************************************
**
** Function         btif_rc_get_interface
**
** Description      Get the AVRCP Target callback interface
**
** Returns          btav_interface_t
**
*******************************************************************************/
const btrc_interface_t *btif_rc_get_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_rc_interface;
}

/*******************************************************************************
**
** Function         btif_rc_ctrl_get_interface
**
** Description      Get the AVRCP Controller callback interface
**
** Returns          btav_interface_t
**
*******************************************************************************/
const btrc_ctrl_interface_t *btif_rc_ctrl_get_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_rc_ctrl_interface;
}

/*******************************************************************************
**      Function         initialize_transaction
**
**      Description    Initializes fields of the transaction structure
**
**      Returns          void
*******************************************************************************/
static void initialize_transaction(int lbl)
{
    pthread_mutex_lock(&device.lbllock);
    if(lbl < MAX_TRANSACTIONS_PER_SESSION)
    {
       device.transaction[lbl].lbl = lbl;
       device.transaction[lbl].in_use=FALSE;
       device.transaction[lbl].handle=0;
    }
    pthread_mutex_unlock(&device.lbllock);
}

/*******************************************************************************
**      Function         lbl_init
**
**      Description    Initializes label structures and mutexes.
**
**      Returns         void
*******************************************************************************/
void lbl_init()
{
    memset(&device,0,sizeof(rc_device_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&(device.lbllock), &attr);
    pthread_mutexattr_destroy(&attr);
    init_all_transactions();
}

/*******************************************************************************
**
** Function         init_all_transactions
**
** Description    Initializes all transactions
**
** Returns          void
*******************************************************************************/
void init_all_transactions()
{
    UINT8 txn_indx=0;
    for(txn_indx=0; txn_indx < MAX_TRANSACTIONS_PER_SESSION; txn_indx++)
    {
        initialize_transaction(txn_indx);
    }
}

/*******************************************************************************
**
** Function         get_transaction_by_lbl
**
** Description    Will return a transaction based on the label. If not inuse
**                     will return an error.
**
** Returns          bt_status_t
*******************************************************************************/
rc_transaction_t *get_transaction_by_lbl(UINT8 lbl)
{
    rc_transaction_t *transaction = NULL;
    pthread_mutex_lock(&device.lbllock);

    /* Determine if this is a valid label */
    if (lbl < MAX_TRANSACTIONS_PER_SESSION)
    {
        if (FALSE==device.transaction[lbl].in_use)
        {
            transaction = NULL;
        }
        else
        {
            transaction = &(device.transaction[lbl]);
            BTIF_TRACE_DEBUG("%s: Got transaction.label: %d",__FUNCTION__,lbl);
        }
    }

    pthread_mutex_unlock(&device.lbllock);
    return transaction;
}

/*******************************************************************************
**
** Function         get_transaction
**
** Description    Obtains the transaction details.
**
** Returns          bt_status_t
*******************************************************************************/

bt_status_t  get_transaction(rc_transaction_t **ptransaction)
{
    bt_status_t result = BT_STATUS_NOMEM;
    UINT8 i=0;
    pthread_mutex_lock(&device.lbllock);

    // Check for unused transactions
    for (i=0; i<MAX_TRANSACTIONS_PER_SESSION; i++)
    {
        if (FALSE==device.transaction[i].in_use)
        {
            BTIF_TRACE_DEBUG("%s:Got transaction.label: %d",__FUNCTION__,device.transaction[i].lbl);
            device.transaction[i].in_use = TRUE;
            *ptransaction = &(device.transaction[i]);
            result = BT_STATUS_SUCCESS;
            break;
        }
    }

    pthread_mutex_unlock(&device.lbllock);
    return result;
}


/*******************************************************************************
**
** Function         release_transaction
**
** Description    Will release a transaction for reuse
**
** Returns          bt_status_t
*******************************************************************************/
void release_transaction(UINT8 lbl)
{
    rc_transaction_t *transaction = get_transaction_by_lbl(lbl);

    /* If the transaction is in use... */
    if (transaction != NULL)
    {
        BTIF_TRACE_DEBUG("%s: lbl: %d", __FUNCTION__, lbl);
        initialize_transaction(lbl);
    }
}

/*******************************************************************************
**
** Function         lbl_destroy
**
** Description    Cleanup of the mutex
**
** Returns          void
*******************************************************************************/
void lbl_destroy()
{
    pthread_mutex_destroy(&(device.lbllock));
}

/*******************************************************************************
**      Function       dev_blacklisted_for_absolute_volume
**
**      Description    Blacklist Devices that donot handle absolute volume well
**                     We are blacklisting all the devices that are not in whitelist
**
**      Returns        True if the device is in the list
*******************************************************************************/
static BOOLEAN dev_blacklisted_for_absolute_volume(BD_ADDR peer_dev)
{
    int i;
    char *dev_name_str = NULL;
    int whitelist_size = sizeof(rc_white_addr_prefix)/sizeof(rc_white_addr_prefix[0]);

    for (i = 0; i < whitelist_size; i++) {
        if (rc_white_addr_prefix[i][0] == peer_dev[0] &&
            rc_white_addr_prefix[i][1] == peer_dev[1] &&
            rc_white_addr_prefix[i][2] == peer_dev[2]) {
            BTIF_TRACE_DEBUG("whitelist absolute volume for %02x:%02x:%02x",
                              peer_dev[0], peer_dev[1], peer_dev[2]);
            return FALSE;
        }
    }

    dev_name_str = BTM_SecReadDevName(peer_dev);
    whitelist_size = sizeof(rc_white_name)/sizeof(char*);
    if (dev_name_str != NULL) {
        for (i = 0; i < whitelist_size; i++) {
            if (strcmp(dev_name_str, rc_white_name[i]) == 0) {
                BTIF_TRACE_DEBUG("whitelist absolute volume for %s", dev_name_str);
                return FALSE;
            }
        }
    }

    BTIF_TRACE_WARNING("blacklist absolute volume for %02x:%02x:%02x, name = %s",
                        peer_dev[0], peer_dev[1], peer_dev[2], dev_name_str);
    return TRUE;
}
