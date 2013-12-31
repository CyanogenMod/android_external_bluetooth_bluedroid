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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hardware/bluetooth.h>

#define LOG_NDDEBUG 0
#define LOG_TAG "bluedroid"

#include "gki.h"
#include "btif_api.h"
#include "bt_utils.h"
#include "data_types.h"
#include "l2cdefs.h"
#include "l2c_api.h"
#if TEST_APP_INTERFACE == TRUE
#include <bt_testapp.h>

static tL2CAP_APPL_INFO    *pl2test_l2c_appl = NULL;
static bt_status_t L2cap_Init (tL2CAP_APPL_INFO *p);
static bt_status_t L2cap_Register (UINT16 psm, BOOLEAN ConnType, UINT16 SecLevel);
static bt_status_t L2cap_DeRegister (UINT16 psm);
static UINT16 L2cap_AllocatePSM(void);
static UINT16 L2cap_Connect(UINT16 psm, bt_bdaddr_t *bd_addr);
static BOOLEAN L2cap_ConnectRsp(BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid,
                                        UINT16 result, UINT16 status);
static UINT16 L2cap_ErtmConnect(UINT16 psm, BD_ADDR p_bd_addr, tL2CAP_ERTM_INFO *p_ertm_info);
static BOOLEAN  L2cap_ErtmConnectRsp (BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid,
                                              UINT16 result, UINT16 status,
                                              tL2CAP_ERTM_INFO *p_ertm_info);
static BOOLEAN L2cap_ConfigReq(UINT16 cid, tL2CAP_CFG_INFO *p_cfg);
static BOOLEAN L2cap_ConfigRsp(UINT16 cid, tL2CAP_CFG_INFO *p_cfg);
static BOOLEAN L2cap_DisconnectReq (UINT16 cid);
static BOOLEAN L2cap_DisconnectRsp (UINT16 cid);
static UINT8 L2cap_DataWrite (UINT16 cid, char *p_data, UINT32 len);
static BOOLEAN L2cap_Ping (BD_ADDR p_bd_addr, tL2CA_ECHO_RSP_CB *p_cb);
static BOOLEAN  L2cap_Echo (BD_ADDR p_bd_addr, BT_HDR *p_data, tL2CA_ECHO_DATA_CB *p_callback);
static BOOLEAN L2cap_SetIdleTimeout (UINT16 cid, UINT16 timeout, BOOLEAN is_global);
static BOOLEAN L2cap_SetIdleTimeoutByBdAddr(BD_ADDR bd_addr, UINT16 timeout);
static UINT8 L2cap_SetDesireRole (UINT8 new_role);
static UINT16 L2cap_LocalLoopbackReq (UINT16 psm, UINT16 handle, BD_ADDR p_bd_addr);
static UINT16   L2cap_FlushChannel (UINT16 lcid, UINT16 num_to_flush);
static BOOLEAN L2cap_SetAclPriority (BD_ADDR bd_addr, UINT8 priority);
static BOOLEAN L2cap_FlowControl (UINT16 cid, BOOLEAN data_enabled);
static BOOLEAN L2cap_SendTestSFrame (UINT16 cid, BOOLEAN rr_or_rej, UINT8 back_track);
static BOOLEAN L2cap_SetTxPriority (UINT16 cid, tL2CAP_CHNL_PRIORITY priority);
static BOOLEAN L2cap_RegForNoCPEvt(tL2CA_NOCP_CB *p_cb, BD_ADDR p_bda);
static BOOLEAN L2cap_SetChnlDataRate (UINT16 cid, tL2CAP_CHNL_DATA_RATE tx, tL2CAP_CHNL_DATA_RATE rx);
static BOOLEAN L2cap_SetFlushTimeout (BD_ADDR bd_addr, UINT16 flush_tout);
static UINT8 L2cap_DataWriteEx (UINT16 cid, BT_HDR *p_data, UINT16 flags);
static BOOLEAN L2cap_SetChnlFlushability (UINT16 cid, BOOLEAN is_flushable);
static BOOLEAN L2cap_GetPeerFeatures (BD_ADDR bd_addr, UINT32 *p_ext_feat, UINT8 *p_chnl_mask);
static BOOLEAN L2cap_GetBDAddrbyHandle (UINT16 handle, BD_ADDR bd_addr);
static UINT8 L2cap_GetChnlFcrMode (UINT16 lcid);
static UINT16 L2cap_SendFixedChnlData (UINT16 fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf);

static const btl2cap_interface_t btl2capInterface = {
    sizeof(btl2cap_interface_t),
    L2cap_Init,
    L2cap_Register,
    L2cap_DeRegister,
    L2cap_AllocatePSM,
    L2cap_Connect,
    L2cap_ConnectRsp,
    L2cap_ErtmConnect,
    L2cap_ErtmConnectRsp,
    L2cap_ConfigReq,
    L2cap_ConfigRsp,
    L2cap_DisconnectReq,
    L2cap_DisconnectRsp,
    L2cap_DataWrite,
    L2cap_Ping,
    L2cap_Echo,
    L2cap_SetIdleTimeout,
    L2cap_SetIdleTimeoutByBdAddr,
    L2cap_SetDesireRole,
    L2cap_LocalLoopbackReq,
    L2cap_FlushChannel,
    L2cap_SetAclPriority,
    L2cap_FlowControl,
    L2cap_SendTestSFrame,
    L2cap_SetTxPriority,
    L2cap_RegForNoCPEvt,
    L2cap_SetChnlDataRate,
    L2cap_SetFlushTimeout,
    L2cap_DataWriteEx,
    L2cap_SetChnlFlushability,
    L2cap_GetPeerFeatures,
    L2cap_GetBDAddrbyHandle,
    L2cap_GetChnlFcrMode,
    L2cap_SendFixedChnlData,
    NULL, // cleanup,
};

const btl2cap_interface_t *btif_l2cap_get_interface(void)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    return &btl2capInterface;
}


/*
- Take PSM only once during the Register func call.
- Rest of the functions (connect, dereg) uses the same PSM. This way user need not pass it again.
  This will also avoid additional error checks like unregistered psm is passed etc.
 */

static UINT16 g_Psm = 0;
static UINT16 g_lcid = 0;
static BD_ADDR g_bd_addr = {0};

static bt_status_t L2cap_Init (tL2CAP_APPL_INFO *p)
{
    pl2test_l2c_appl = p;
    return BT_STATUS_SUCCESS;
}
/*******************************************************************************
**
** Function         L2cap_Register
**
** Description      This function is called during the task startup
**                  to register interface functions with L2CAP.
**
*******************************************************************************/
static bt_status_t L2cap_Register (UINT16 psm, BOOLEAN ConnType, UINT16 SecLevel)
{

    BTIF_TRACE_DEBUG1("L2cap_Register :: psm=%d", psm);
    if (!BTM_SetSecurityLevel (ConnType, "l2test", /*BTM_SEC_SERVICE_SDP_SERVER*/ BTM_SEC_PROTO_L2CAP,
                               SecLevel, psm, 0, 0))
    {
        BTIF_TRACE_DEBUG0("Error:: BTM_SetSecurityLevel failed");
        return BT_STATUS_FAIL;
}
#if 1
    if(4113 == psm) {
        if (!BTM_SetSecurityLevel (ConnType, "l2test 4113", /*BTM_SEC_SERVICE_SDP_SERVER*/ BTM_SEC_PROTO_L2CAP,
                                   SecLevel, psm, 0, 0)) {
            BTIF_TRACE_DEBUG0("Error:: BTM_SetSecurityLevel failed");
            return BT_STATUS_FAIL;
        }
    }
#endif
    g_Psm = L2CA_Register (psm, pl2test_l2c_appl);
    if(0 == g_Psm) {
        BTIF_TRACE_DEBUG0("Error:: L2CA_Register failed");
        return BT_STATUS_FAIL;
    }
   return BT_STATUS_SUCCESS;
}

static bt_status_t L2cap_DeRegister (UINT16 psm)
{
    L2CA_Deregister(psm);
    return BT_STATUS_SUCCESS;
}

static UINT16 L2cap_AllocatePSM(void)
{
    BTIF_TRACE_DEBUG0("L2cap_AllocatePSM");
    return L2CA_AllocatePSM();
}

static UINT16 L2cap_Connect(UINT16 psm, bt_bdaddr_t *bd_addr)
{

    BTIF_TRACE_DEBUG6("L2cap_Connect:: %0x %0x %0x %0x %0x %0x", bd_addr->address[0],bd_addr->address[1],
                         bd_addr->address[2],bd_addr->address[3],bd_addr->address[4],bd_addr->address[5]);

    if (0 == (g_lcid = L2CA_ConnectReq (psm, bd_addr->address)))
    {
        BTIF_TRACE_DEBUG1("Error:: L2CA_ConnectReq failed for psm %d", psm);
    }
    return g_lcid;
}

static BOOLEAN L2cap_ConnectRsp(BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid,
                                        UINT16 result, UINT16 status)
{
    if (!L2CA_ConnectRsp (p_bd_addr, id, lcid, result, status)) {
        BTIF_TRACE_DEBUG0("L2CA_ConnectRsp:: error ");
        return  BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

static UINT16 L2cap_ErtmConnect(UINT16 psm, BD_ADDR address, tL2CAP_ERTM_INFO *p_ertm_info)
{
    BTIF_TRACE_DEBUG6("L2cap_ErtmConnect:: %0x %0x %0x %0x %0x %0x", address[0],address[1],address[2],address[3],address[4],address[5]);
    if (0 == (g_lcid = L2CA_ErtmConnectReq (psm, address, p_ertm_info))) {
        BTIF_TRACE_DEBUG1("Error:: L2CA_ErtmConnectReq failed for psm %d", psm);
    }
    return g_lcid;
}

static BOOLEAN  L2cap_ErtmConnectRsp (BD_ADDR p_bd_addr, UINT8 id, UINT16 lcid,
                                             UINT16 result, UINT16 status,
                                             tL2CAP_ERTM_INFO *p_ertm_info)
{
    if (!L2CA_ErtmConnectRsp (p_bd_addr, id, lcid, result, status, p_ertm_info)) {
        BTIF_TRACE_DEBUG0("L2CA_ErtmConnectRsp:: error ");
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

static BOOLEAN L2cap_ConfigReq(UINT16 cid, tL2CAP_CFG_INFO *p_cfg)
{
    BTIF_TRACE_DEBUG0("L2cap_ConfigReq:: Invoked\n");
    if (p_cfg->fcr_present)
    {
        BTIF_TRACE_DEBUG6("L2cap_ConfigReq::  mode %u, txwinsz %u, max_trans %u, rtrans_tout %u, mon_tout %u, mps %u\n",
                           p_cfg->fcr.mode, p_cfg->fcr.tx_win_sz, p_cfg->fcr.max_transmit,
                           p_cfg->fcr.rtrans_tout,p_cfg->fcr.mon_tout, p_cfg->fcr.mps);
    }
    return L2CA_ConfigReq (cid, p_cfg);
}

static BOOLEAN L2cap_ConfigRsp(UINT16 cid, tL2CAP_CFG_INFO *p_cfg)
{
    BTIF_TRACE_DEBUG0("L2cap_ConfigRsp:: Invoked");
    return L2CA_ConfigRsp (cid, p_cfg);
}

static BOOLEAN L2cap_DisconnectReq (UINT16 cid)
{
    BTIF_TRACE_DEBUG1("L2cap_DisconnectReq:: cid=%d", cid);
    return L2CA_DisconnectReq(cid);
}
static BOOLEAN L2cap_DisconnectRsp (UINT16 cid)
{
    BTIF_TRACE_DEBUG0("L2cap_DisconnectRsp:: Invoked");
    return L2CA_DisconnectRsp (cid);
}

static UINT8 L2cap_DataWrite (UINT16 cid, char *p_data, UINT32 len)
{
    BTIF_TRACE_DEBUG0("L2cap_DataWrite:: Invoked");
    BT_HDR  *p_msg = NULL;(BT_HDR *) GKI_getpoolbuf (GKI_POOL_ID_3);
    UINT8   *ptr, *p_start;

    p_msg = (BT_HDR *) GKI_getpoolbuf (GKI_POOL_ID_3);
    BTIF_TRACE_DEBUG0("GKI_getpoolbuf");
    if (!p_msg)
    {
        BTIF_TRACE_DEBUG0("No resource to allocate");
        return BT_STATUS_FAIL;
    }
    p_msg->offset = L2CAP_MIN_OFFSET;
    ptr = p_start = (UINT8 *)(p_msg + 1) + L2CAP_MIN_OFFSET;
    p_msg->len = len;    //Sends len bytes, irrespective of what you copy to the buffer
    memcpy(ptr, p_data, len);
    return L2CA_DataWrite(cid, p_msg);
}


static BOOLEAN L2cap_Ping (BD_ADDR p_bd_addr, tL2CA_ECHO_RSP_CB *p_cb)
{
    BTIF_TRACE_DEBUG0("L2cap_Ping:: Invoked");
    return L2CA_Ping (p_bd_addr, p_cb);
}

static BOOLEAN  L2cap_Echo (BD_ADDR p_bd_addr, BT_HDR *p_data, tL2CA_ECHO_DATA_CB *p_callback)
{
    BTIF_TRACE_DEBUG0("L2cap_Echo:: Invoked");
    return L2CA_Echo (p_bd_addr, p_data, p_callback);
}

static BOOLEAN L2cap_SetIdleTimeout (UINT16 cid, UINT16 timeout, BOOLEAN is_global)
{
    BTIF_TRACE_DEBUG0("L2cap_SetIdleTimeout:: Invoked");
    return L2CA_SetIdleTimeout (cid, timeout, is_global);
}


static BOOLEAN L2cap_SetIdleTimeoutByBdAddr(BD_ADDR bd_addr, UINT16 timeout)
{
    BTIF_TRACE_DEBUG0("L2cap_SetIdleTimeoutByBdAddr:: Invoked");
    return L2CA_SetIdleTimeoutByBdAddr(bd_addr, timeout);
}

static UINT8 L2cap_SetDesireRole (UINT8 new_role)
{
    BTIF_TRACE_DEBUG0("L2CA_SetDesireRole:: Invoked");
    return L2CA_SetDesireRole (new_role);
}


static UINT16 L2cap_LocalLoopbackReq (UINT16 psm, UINT16 handle, BD_ADDR p_bd_addr)
{
    BTIF_TRACE_DEBUG0("L2cap_LocalLoopbackReq:: Invoked");
    return L2CA_LocalLoopbackReq (psm, handle, p_bd_addr);
}

static UINT16   L2cap_FlushChannel (UINT16 lcid, UINT16 num_to_flush)
{
    BTIF_TRACE_DEBUG0("L2cap_FlushChannel:: Invoked");
    return L2CA_FlushChannel (lcid, num_to_flush);
}

static BOOLEAN L2cap_SetAclPriority (BD_ADDR bd_addr, UINT8 priority)
{
    BTIF_TRACE_DEBUG0("L2cap_SetAclPriority:: Invoked");
    return L2CA_SetAclPriority (bd_addr, priority);
}

static BOOLEAN L2cap_FlowControl (UINT16 cid, BOOLEAN data_enabled)
{
    BTIF_TRACE_DEBUG1("L2cap_FlowControl:: Invoked with LocalBusy=%s\n", (data_enabled)? "FALSE" :"TRUE");
    return L2CA_FlowControl (cid, data_enabled);
}

static BOOLEAN L2cap_SendTestSFrame (UINT16 cid, BOOLEAN rr_or_rej, UINT8 back_track)
{
    BTIF_TRACE_DEBUG0("L2cap_SendTestSFrame:: Invoked");
    return L2CA_SendTestSFrame (cid, rr_or_rej, back_track);
}

static BOOLEAN L2cap_SetTxPriority (UINT16 cid, tL2CAP_CHNL_PRIORITY priority)
{
    BTIF_TRACE_DEBUG0("L2cap_SetTxPriority:: Invoked");
    return L2CA_SetTxPriority (cid, priority);
}

static BOOLEAN L2cap_RegForNoCPEvt(tL2CA_NOCP_CB *p_cb, BD_ADDR p_bda)
{
    BTIF_TRACE_DEBUG0("L2cap_RegForNoCPEvt:: Invoked");
    return L2CA_RegForNoCPEvt(p_cb, p_bda);
}

static BOOLEAN L2cap_SetChnlDataRate (UINT16 cid, tL2CAP_CHNL_DATA_RATE tx, tL2CAP_CHNL_DATA_RATE rx)
{
    BTIF_TRACE_DEBUG0("L2cap_SetChnlDataRate:: Invoked");
    return L2CA_SetChnlDataRate (cid, tx, rx);
}

static BOOLEAN L2cap_SetFlushTimeout (BD_ADDR bd_addr, UINT16 flush_tout)
{
    BTIF_TRACE_DEBUG0("L2cap_SetFlushTimeout:: Invoked");
    return L2CA_SetFlushTimeout (bd_addr, flush_tout);
}

static UINT8 L2cap_DataWriteEx (UINT16 cid, BT_HDR *p_data, UINT16 flags)
{
    BTIF_TRACE_DEBUG0("L2cap_DataWriteEx:: Invoked");
    return L2CA_DataWriteEx (cid, p_data, flags);
}
static BOOLEAN L2cap_SetChnlFlushability (UINT16 cid, BOOLEAN is_flushable)
{
    BTIF_TRACE_DEBUG0("L2cap_SetChnlFlushability:: Invoked");
    return L2CA_SetChnlFlushability (cid, is_flushable);
}
static BOOLEAN L2cap_GetPeerFeatures (BD_ADDR bd_addr, UINT32 *p_ext_feat, UINT8 *p_chnl_mask)
{
    BTIF_TRACE_DEBUG0("L2cap_GetPeerFeatures:: Invoked");
    return L2CA_GetPeerFeatures (bd_addr, p_ext_feat, p_chnl_mask);
}
static BOOLEAN L2cap_GetBDAddrbyHandle (UINT16 handle, BD_ADDR bd_addr)
{
    BTIF_TRACE_DEBUG0("L2cap_GetBDAddrbyHandle:: Invoked");
    return L2CA_GetBDAddrbyHandle (handle, bd_addr);
}
static UINT8 L2cap_GetChnlFcrMode (UINT16 lcid)
{
    BTIF_TRACE_DEBUG0("L2cap_GetChnlFcrMode:: Invoked");
    return L2CA_GetChnlFcrMode (lcid);
}

//---------------------FIXED CHANNEL API ---------------------
static UINT16 L2cap_SendFixedChnlData (UINT16 fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf)
{
    BTIF_TRACE_DEBUG0("L2cap_SendFixedChnlData:: Invoked");
    p_buf->event = 20;
    return L2CA_SendFixedChnlData(fixed_cid, rem_bda, p_buf);
}
#endif //TEST_APP_INTERFACE
