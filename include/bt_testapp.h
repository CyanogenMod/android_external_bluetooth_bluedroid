/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
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

#ifdef TEST_APP_INTERFACE
#ifndef ANDROID_INCLUDE_BT_TESTAPP_H
#define ANDROID_INCLUDE_BT_TESTAPP_H
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <linux/capability.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <hardware/bluetooth.h>
#include "l2c_api.h"
#include "sdp_api.h"
#include "gatt_api.h"
#include "gap_api.h"
#include <hardware/hardware.h>
#include "data_types.h"

__BEGIN_DECLS

typedef void (tREMOTE_DEVICE_NAME_CB) (void *p1);

enum {
    SUCCESS,
    FAIL
};

typedef enum {
    DUMMY,
    ALL,
    SPP,
    FTP,
    OPP,
    MAP,
    PBAP,
    DUN,
    NOT_SUPPORTED,
}profileName;
typedef enum {
    TEST_APP_L2CAP,
    TEST_APP_SDP,
    TEST_APP_GATT,
    TEST_APP_GAP,
    TEST_APP_SMP,
    TEST_APP_RFCOMM
} test_app_profile;
typedef struct {

    /** set to sizeof(Btl2capInterface) */
    size_t          size;
    /** Register the L2cap callbacks  */
    bt_status_t (*Init)(tL2CAP_APPL_INFO* callbacks);
    bt_status_t (*RegisterPsm)(UINT16 psm, BOOLEAN conn_type, UINT16 sec_level);
    bt_status_t (*Deregister)(UINT16 psm);
    UINT16      (*AllocatePsm)(void);
    UINT16      (*Connect)(UINT16 psm, bt_bdaddr_t *bd_addr);
    BOOLEAN     (*ConnectRsp)(BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid, UINT16 result, UINT16 status);
    UINT16      (*ErtmConnectReq)(UINT16 psm, BD_ADDR p_bd_addr, tL2CAP_ERTM_INFO *p_ertm_info);
    BOOLEAN     (*ErtmConnectRsp)(BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid,
                                             UINT16 result, UINT16 status,
                                             tL2CAP_ERTM_INFO *p_ertm_info);
    BOOLEAN     (*ConfigReq)(UINT16 cid, tL2CAP_CFG_INFO *p_cfg);
    BOOLEAN     (*ConfigRsp)(UINT16 cid, tL2CAP_CFG_INFO *p_cfg);
    BOOLEAN     (*DisconnectReq)(UINT16 cid);
    BOOLEAN     (*DisconnectRsp)(UINT16 cid);
    UINT8       (*DataWrite)(UINT16 cid, char *p_data, UINT32 len);
    BOOLEAN     (*Ping)(BD_ADDR p_bd_addr, tL2CA_ECHO_RSP_CB *p_cb);
    BOOLEAN     (*Echo)(BD_ADDR p_bd_addr, BT_HDR *p_data, tL2CA_ECHO_DATA_CB *p_callback);
    BOOLEAN     (*SetIdleTimeout)(UINT16 cid, UINT16 timeout, BOOLEAN is_global);
    BOOLEAN     (*SetIdleTimeoutByBdAddr)(BD_ADDR bd_addr, UINT16 timeout);
    UINT8       (*SetDesireRole)(UINT8 new_role);
    UINT16      (*LocalLoopbackReq)(UINT16 psm, UINT16 handle, BD_ADDR p_bd_addr);
    UINT16      (*FlushChannel)(UINT16 lcid, UINT16 num_to_flush);
    BOOLEAN     (*SetAclPriority)(BD_ADDR bd_addr, UINT8 priority);
    BOOLEAN     (*FlowControl)(UINT16 cid, BOOLEAN data_enabled);
    BOOLEAN     (*SendTestSFrame)(UINT16 cid, BOOLEAN rr_or_rej, UINT8 back_track);
    BOOLEAN     (*SetTxPriority)(UINT16 cid, tL2CAP_CHNL_PRIORITY priority);
    BOOLEAN     (*RegForNoCPEvt)(tL2CA_NOCP_CB *p_cb, BD_ADDR p_bda);
    BOOLEAN     (*SetChnlDataRate)(UINT16 cid, tL2CAP_CHNL_DATA_RATE tx, tL2CAP_CHNL_DATA_RATE rx);
    BOOLEAN     (*SetFlushTimeout)(BD_ADDR bd_addr, UINT16 flush_tout);
    UINT8       (*DataWriteEx)(UINT16 cid, BT_HDR *p_data, UINT16 flags);
    BOOLEAN     (*SetChnlFlushability)(UINT16 cid, BOOLEAN is_flushable);
    BOOLEAN     (*GetPeerFeatures)(BD_ADDR bd_addr, UINT32 *p_ext_feat, UINT8 *p_chnl_mask);
    BOOLEAN     (*GetBDAddrbyHandle)(UINT16 handle, BD_ADDR bd_addr);
    UINT8       (*GetChnlFcrMode)(UINT16 lcid);
    UINT16      (*SendFixedChnlData)(UINT16 fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf);
    void  (*Cleanup)(void);
} btl2cap_interface_t;

typedef struct {
    /** set to sizeof(BtsdpInterface) */
    size_t    size;
    int       (*Init)(tSDP_DISC_CMPL_CB* callback);
    int       (*GetRemoteDeviceName)(UINT8 *p_bd_addr, tREMOTE_DEVICE_NAME_CB* rmd_name_callback);
    int       (*SearchServices)(UINT8 *p_bd_addr);
    UINT32    (*CreateNewRecord)(void);
    int       (*AddRecord)(UINT32 handle,profileName profile);
    void      (*Cleanup)(void);
    void      (*printSearchedServices)(void);
} btsdp_interface_t;

typedef struct
{
    size_t    size;
    //GATT common APIs (Both client and server)
    tGATT_IF (*Register) (tBT_UUID *p_app_uuid128, tGATT_CBACK *p_cb_info);
    void (*Deregister) (tGATT_IF gatt_if);
    void (*StartIf) (tGATT_IF gatt_if);
    BOOLEAN (*Connect) (tGATT_IF gatt_if, BD_ADDR bd_addr, BOOLEAN is_direct,tBT_TRANSPORT transport);
    tGATT_STATUS (*Disconnect) (UINT16 conn_id);
    BOOLEAN (*Listen) (tGATT_IF gatt_if, BOOLEAN start, BD_ADDR_PTR bd_addr);

    //GATT Client APIs
    tGATT_STATUS (*cConfigureMTU) (UINT16 conn_id, UINT16  mtu);
    tGATT_STATUS (*cDiscover) (UINT16 conn_id, tGATT_DISC_TYPE disc_type, tGATT_DISC_PARAM *p_param );
    tGATT_STATUS (*cRead) (UINT16 conn_id, tGATT_READ_TYPE type, tGATT_READ_PARAM *p_read);
    tGATT_STATUS (*cWrite) (UINT16 conn_id, tGATT_WRITE_TYPE type, tGATT_VALUE *p_write);
    tGATT_STATUS (*cExecuteWrite) (UINT16 conn_id, BOOLEAN is_execute);
    tGATT_STATUS (*cSendHandleValueConfirm) (UINT16 conn_id, UINT16 handle);
    void (*cSetIdleTimeout)(BD_ADDR bd_addr, UINT16 idle_tout);

    //GATT Server APIs
    //TODO - Add api on the need basis

}btgatt_test_interface_t;

typedef struct
{
    size_t    size;
    void (*init)(void);
    BOOLEAN (*Register) (tSMP_CALLBACK *p_cback);
    tSMP_STATUS (*Pair) (BD_ADDR bd_addr);
    BOOLEAN (*PairCancel) (BD_ADDR bd_addr);
    void (*SecurityGrant)(BD_ADDR bd_addr, UINT8 res);
    void (*PasskeyReply) (BD_ADDR bd_addr, UINT8 res, UINT32 passkey);
    BOOLEAN (*Encrypt) (UINT8 *key, UINT8 key_len, UINT8 *plain_text, UINT8 pt_len, tSMP_ENC *p_out);
}btsmp_interface_t;
typedef struct
{
    size_t    size;
    void (*Gap_AttrInit)();
    void (*Gap_BleAttrDBUpdate)(BD_ADDR bd_addr, UINT16 int_min, UINT16 int_max, UINT16 latency, UINT16 sp_tout);
    void (*Gap_SetDiscoverableMode)(UINT16 mode, UINT16 duration, UINT16 interval);
    void (*Gap_SetConnectableMode) (UINT16 mode, UINT16 duration, UINT16 interval);
}btgap_interface_t;

/** Bluetooth RFC tool commands */
typedef enum {
    RFC_TEST_CLIENT =1,
    RFC_TEST_FRAME_ERROR,
    RFC_TEST_ROLE_SWITCH,
    RFC_TEST_SERVER,
    RFC_TEST_DISCON,
    RFC_TEST_CLIENT_TEST_MSC_DATA, //For PTS test case BV 21 and 22
    RFC_TEST_WRITE_DATA
}rfc_test_cmd_t;


typedef struct {
    bt_bdaddr_t bdadd;
    uint8_t     scn; //Server Channel Number
}bt_rfc_conn_t;

typedef struct {
    bt_bdaddr_t bdadd;
    uint8_t     role; //0x01 for master
}bt_role_sw;

typedef union {
    bt_rfc_conn_t  conn;
    uint8_t        server;
    bt_role_sw     role_switch;
}tRfcomm_test;

typedef struct {
    rfc_test_cmd_t param;
    tRfcomm_test   data;
}tRFC;

typedef struct {
    size_t          size;
    bt_status_t (*init)( tL2CAP_APPL_INFO* callbacks );
    void  (*rdut_rfcomm)( BOOLEAN server );
    void  (*rdut_rfcomm_test_interface)( tRFC *input);
    bt_status_t (*connect)( bt_bdaddr_t *bd_addr );
    void  (*cleanup)( void );
} btrfcomm_interface_t;

#endif

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_TESTAPP_H */
