/******************************************************************************

Copyright (c) 2013, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************/
#ifndef BTC_COMMON_H
#define BTC_COMMON_H

#include "bt_hci_bdroid.h"
/*****************************************************************************/

typedef unsigned short uint16_t;

#define HCI_COMMAND                                     0x01
#define HCI_EVT                                         0x04
#define OCF_INQUIRY                                     0x01
#define OCF_INQUIRY_CANCEL                              0x02
#define OCF_PERIODIC_INQUIRY                            0x03
#define OCF_OCF_CREATE_CONN                             0x05
#define OCF_READ_BD_ADDR                                0x09
#define OCF_READ_REMOTE_FEATURES                        0x1B
#define OCF_READ_REMOTE_VERSION                         0x1D
#define OCF_SET_AFH_HOST_CHANNEL_CLASSIFICATION         0x3F
#define OCF_RESET                                       0x03
#define OCF_ROLE_DISCOVERY                              0x09
#define EVT_INQUIRY_COMPLETE                            0x01
#define EVT_CONN_COMPLETE                               0x03
#define EVT_CONN_REQUEST                                0x04
#define EVT_DISCONN_COMPLETE                            0x05
#define EVT_READ_REMOTE_FEATURES_COMPLETE               0x0B
#define EVT_READ_REMOTE_VERSION_COMPLETE                0x0C
#define EVT_CMD_COMPLETE                                0x0E
#define EVT_ROLE_CHANGE                                 0x12
#define EVT_PIN_CODE_REQ                                0x16
#define EVT_LINK_KEY_NOTIFY                             0x18
#define EVT_SYNC_CONN_COMPLETE                          0x2C
#define OGF_LINK_CTL                                    0x04
#define OGF_LINK_POLICY                                 0x08
#define OGF_HOST_CTL                                    0x0c
#define OGF_INFO_PARAM                                  0x10
#define BTC_STACK_TO_HC_HCI_CMD                         0x2000
#define BTC_HC_TO_STACK_HCI_EVT                         0x1000
#define BTC_EVT_MASK                                    0xFF00
#define BTC_MAX_LEN                                     0xFF

#define EVT_OGF_OFFSET                                  0x05
#define EVT_OCF_OFFSET                                  0x04
#define EVT_CODE_OFFSET                                 0x01
#define CMD_OGF_OFFSET                                  0x02
#define CMD_OCF_OFFSET                                  0x01


typedef void (*t_HciPacketCallback)(HC_BT_HDR *p_buf);
extern void btsnoop_reg(t_HciPacketCallback p_callback);
void btc_process_client_command(char *hci_cmd, int len);
void btc_process_stack_command(char *hci_cmd, int len);
void btc_send_cmd_complete(char *hci_evt, int len);
void btc_init(void);
void btc_deinit(void);
int btc_close_serv_socket(const char* name);
void btc_cmd_evt_handler(HC_BT_HDR *p_buf);
static void btc_thread(void *param);
static int btc_accept(char *name);
#endif /*BTC_COMMON_H*/

