/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utils/Log.h>
#include <pthread.h>

#include <hardware/wipower.h>
#include <hardware/bluetooth.h>

#include "bta_api.h"
#include "btm_api.h"
#include "btif_common.h"
#include "wipower_const.h"

pthread_mutex_t signal_mutex;
pthread_cond_t signal_cv;

/* #define LOG_NDDEBUG 0 */

#define WIP_LOG_TAG "wipower"
#define HCI_EVENT_WAIT_TIMEOUT 2

wipower_callbacks_t *wipower_hal_cbacks = NULL;
wipower_dyn_data_t wipower_dyn_data;

unsigned char gStatus;
wipower_state_t gState;
unsigned char gCurrentLimit;







void dispatch_enable_event (UINT16 event, char* p_param) {
    ALOGI("%s->", __func__);
    wipower_state_t state = OFF;

    unsigned char stat =  *p_param;
    ALOGI("%s: %x", __func__, stat);

    if (stat == 0x00) state = ON;
    else  state = OFF;

    if (wipower_hal_cbacks) {
         wipower_hal_cbacks->wipower_state_changed_cb(state);
    } else {
         ALOGE("wipower_hal_cbacks not registered");
    }
}


void enable_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    unsigned char status = 0xFF;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-2];

        ALOGI("%s: %x", __func__, status);

    }

    btif_transfer_context(dispatch_enable_event, 0/*Event is not used*/,
          (char*)&status, sizeof(status), NULL);
}

void set_current_limit_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    gStatus = 0xFF;
    unsigned char status = 0x00;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-2];

        ALOGI("%s: %x", __func__, status);

        if (status == 0) {
                gStatus = 0x00;
        }
    }
}

void get_current_limit_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    gStatus = 0xFF;
    unsigned char status = 0x00;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-2];


        ALOGI("%s: %x", __func__, status);

        if (status == 0) {
            gStatus = 0x00;
            gCurrentLimit = p1->p_param_buf[p1->param_len-2];
        }
    }

    ALOGI("%s->%x", __func__, gCurrentLimit);
}

void get_state_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    gStatus = 0xFF;
    unsigned char status = 0x00;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-3];

        ALOGI("%s: %x", __func__, status);

        if (status == 0) {
            gStatus = 0x00;
            gState =  p1->p_param_buf[p1->param_len-1];
        }
    }
}

void enable_alert_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    gStatus = 0xFF;
    unsigned char status = 0x00;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-2];

        ALOGI("%s: %x", __func__, status);

        if (status == 0) {
                gStatus = 0x00;
        }
    }
}

void enable_data_cb(tBTM_VSC_CMPL *p1) {
    ALOGI("%s->", __func__);
    gStatus = 0xFF;
    unsigned char status = 0x00;

    if (p1->param_len) {
        status = p1->p_param_buf[p1->param_len-2];

        ALOGI("%s: %x", __func__, status);

        if (status == 0) {
                gStatus = 0x00;
        }
    }
}

void dispatch_wp_events (UINT16 len, char* p_param) {
    ALOGI("%s->", __func__);

    unsigned char event = p_param[0];
    switch(event) {
    case WP_HCI_EVENT_ALERT: {
        if (len != 2) {
            ALOGE("WP_HCI_EVENT_ALERT:Error! Length"); return;
        } else {
            unsigned char alert = p_param[1];
            if (wipower_hal_cbacks) {
                wipower_hal_cbacks->wipower_alert(alert);
                ALOGE("alert event forwarded to app layer");
            } else {
                ALOGE("wipower_hal_cbacks not registered");
            }
        }
    } break;
    case WP_HCI_EVENT_DATA: {
        if (len != 21) {
            ALOGE("WP_HCI_EVENT_DATA: Error! Length"); return;
        } else {
            memcpy(&wipower_dyn_data, &p_param[1], 20);
            if (wipower_hal_cbacks) {
                wipower_hal_cbacks->wipower_data(&wipower_dyn_data);
                ALOGE("wp data event forwarded to app layer");
            } else {
                ALOGE("wipower_hal_cbacks not registered");
            }
        }

    } break;
    case WP_HCI_EVENT_POWER_ON: {
        if (len != 2) {
            ALOGE("WP_HCI_EVENT_POWER_ON: Error! Length"); return;
        } else {
            unsigned char alert = p_param[1];
            if (wipower_hal_cbacks) {
                //wipower_hal_cbacks->wipower_alert(alert);
                ALOGE("wp power-on event forwarded to app layer");
            } else {
                ALOGE("wipower_hal_cbacks not registered");
            }
        }

    } break;
    }

}

void wp_events(UINT8 len, UINT8 *p) {

    ALOGI("%s-> %d", __func__, len);


    /*Event is part of payload, Length used as Event*/
    btif_transfer_context(dispatch_wp_events, (UINT16)len,
          (char*)p, sizeof(p), NULL);
}

/** Enable Bluetooth. */
int enable(wipower_callbacks_t *wp_callbacks, bool enable)
{
    UINT8 en[2];
    ALOGI("enable: %d", enable);

    en[0] = WP_HCI_CMD_SET_CHARGE_OUTPUT;

    if (enable) {
        en[1] = 1;
    } else {
        en[1]= 0;
    }
    wipower_hal_cbacks = wp_callbacks;

    //Register VS Event
    tBTM_STATUS res = BTM_RegisterForVSEvents(wp_events, enable);
    ALOGI("res of BTM_RegisterForVSEvents %d", res);

    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 2, en, enable_cb);

    return 0;
}


int set_current_limit(short value)
{
    UINT8 curr_limit[2];
    ALOGI("set_current_limit: %x", value);
    int status = -1;

    curr_limit[0] = WP_HCI_CMD_SET_CURRENT_LIMIT;
    curr_limit[1] = value;

    gStatus = 0xFF;
    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 2, curr_limit, set_current_limit_cb);

    if (gStatus == 0) {
        status = 0;
    }

    ALOGI("set_current_limit: Status %x", status);
    return status;
}

unsigned char get_current_limit(void)
{
    ALOGI("%s", __func__);
    unsigned char val =  0;
    UINT8 get_limit = 0;
    get_limit = WP_HCI_CMD_GET_CURRENT_LIMIT;
    gStatus = 0xFF;
    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 1, &get_limit, get_current_limit_cb);

    val = gCurrentLimit;

    ALOGI("get_current_limit: value %x", val);
    return val;
}

wipower_state_t get_state(void)
{
    ALOGI("%s", __func__);
    wipower_state_t state = OFF;
    UINT8 get_state = 0;
    get_state = WP_HCI_CMD_GET_CHARGE_OUTPUT;
    gStatus = 0xFF;
    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 1, &get_state, get_state_cb);

    if (gStatus == 0x00) {
        state = gState;
    }

    ALOGI("%s: %x", __func__, state);
    return state;
}

int enable_alerts(bool enable)
{
    UINT8 en[2];
    int status = -1;
    ALOGI("%s:%d", __func__, enable);

    en[0] = WP_HCI_CMD_ENABLE_ALERT;
    if (enable) {
        en[1] = 1;
    } else {
        en[1] = 0;
    }

    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 2, en, enable_alert_cb);


    status = gStatus;

    ALOGI("%s: Status %x", __func__, status);
    return status;
}

int enable_data_notify(bool enable)
{
    UINT8 en[2];
    int status = -1;
    ALOGI("%s:%d", __func__, enable);

    en[0] = WP_HCI_CMD_ENABLE_DATA;
    if (enable) {
        en[1] = 1;
    } else {
        en[1] = 0;
    }

    BTA_DmVendorSpecificCommand(WP_HCI_VS_CMD, 2, en, enable_data_cb);

    status = gStatus;

    ALOGI("enable_data_notify: Status %x", status);
    return status;
}

static const wipower_interface_t wipowerInterface = {
    sizeof(wipowerInterface),
    /** Enable Bluetooth. */
    enable,
    /** Disable Bluetooth. */
    set_current_limit,
    get_current_limit,
    get_state,
    enable_alerts,
    enable_data_notify
};

const wipower_interface_t* get_wipower_interface ()
{
    ALOGE("get wp interface>>");
    return &wipowerInterface;
}
