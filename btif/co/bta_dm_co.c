/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
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
#include <stdio.h>
#include <stdlib.h>

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_dm_co.h"
#include "bta_dm_ci.h"


/*******************************************************************************
**
** Function         bta_dm_co_get_compress_memory
**
** Description      This callout function is executed by DM to get memory for compression

** Parameters       id  -  BTA SYS ID
**                  memory_p - memory return by callout
**                  memory_size - memory size
**
** Returns          TRUE for success, FALSE for fail.
**
*******************************************************************************/
BOOLEAN bta_dm_co_get_compress_memory(tBTA_SYS_ID id, UINT8 **memory_p, UINT32 *memory_size)
{
    return TRUE;
}

/*******************************************************************************
**
** Function         bta_dm_co_io_req
**
** Description      This callout function is executed by DM to get IO capabilities
**                  of the local device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - TRUE, if MITM protection is required. 
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                      tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig)
{
}

/*******************************************************************************
**
** Function         bta_dm_co_io_rsp
**
** Description      This callout function is executed by DM to report IO capabilities
**                  of the peer device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  io_cap - The remote Input/Output capabilities
**                  oob_data - TRUE, if OOB data is available for the peer device.
**                  auth_req - TRUE, if MITM protection is required. 
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap,
                      tBTA_OOB_DATA oob_data, tBTA_AUTH_REQ auth_req)
{
}

/*******************************************************************************
**
** Function         bta_dm_co_lk_upgrade
**
** Description      This callout function is executed by DM to check if the
**                  platform wants allow link key upgrade
**
** Parameters       bd_addr  - The peer device
**                  *p_upgrade - TRUE, if link key upgrade is desired. 
**
** Returns          void.
**
*******************************************************************************/
void  bta_dm_co_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade )
{
}

#if (BTM_OOB_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_loc_oob
**
** Description      This callout function is executed by DM to report the OOB
**                  data of the local device for the Simple Pairing process
**
** Parameters       valid - TRUE, if the local OOB data is retrieved from LM
**                  c     - Simple Pairing Hash C
**                  r     - Simple Pairing Randomnizer R
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r)
{
}

/*******************************************************************************
**
** Function         bta_dm_co_rmt_oob
**
** Description      This callout function is executed by DM to request the OOB
**                  data for the remote device for the Simple Pairing process
**                  Need to call bta_dm_ci_rmt_oob() in response
**
** Parameters       bd_addr  - The peer device
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_rmt_oob(BD_ADDR bd_addr)
{
}

#endif /* BTM_OOB_INCLUDED */


// REMOVE FOR BLUEDROID ?

#if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         btui_sco_codec_callback
**
** Description      Callback for btui codec.  
**                  
**
** Returns          void
**
*******************************************************************************/
static void btui_sco_codec_callback(UINT16 event, UINT16 sco_handle)
{
    bta_dm_sco_ci_data_ready(event, sco_handle);
}
/*******************************************************************************
**
** Function         bta_dm_sco_co_init
**
** Description      This function can be used by the phone to initialize audio 
**                  codec or for other initialization purposes before SCO connection
**                  is opened.  
**                  
**
** Returns          tBTA_DM_SCO_ROUTE_TYPE: SCO routing configuration type.
**
*******************************************************************************/
tBTA_DM_SCO_ROUTE_TYPE bta_dm_sco_co_init(UINT32 rx_bw, UINT32 tx_bw, 
                                          tBTA_CODEC_INFO * p_codec_type, UINT8 app_id)
{
    tBTM_SCO_ROUTE_TYPE route = BTA_DM_SCO_ROUTE_PCM;

    BTIF_TRACE_DEBUG0("bta_dm_sco_co_init");

    /* set up SCO routing configuration if SCO over HCI app ID is used and run time
        configuration is set to SCO over HCI */
    /* HS invoke this call-out */
    if (
#if (BTA_HS_INCLUDED == TRUE ) && (BTA_HS_INCLUDED == TRUE)
       (app_id == BTUI_DM_SCO_4_HS_APP_ID && btui_cfg.hs_sco_over_hci) ||
#endif
       /* AG invoke this call-out */
       (app_id != BTUI_DM_SCO_4_HS_APP_ID && btui_cfg.ag_sco_over_hci ))
    {
        route = btui_cb.sco_hci = BTA_DM_SCO_ROUTE_HCI;
    }
    /* no codec is is used for the SCO data */
    if (p_codec_type->codec_type == BTA_SCO_CODEC_PCM && route == BTA_DM_SCO_ROUTE_HCI)
    {
        /* initialize SCO codec */
        if (!btui_sco_codec_init(rx_bw, tx_bw))
        {
            BTIF_TRACE_ERROR0("codec initialization exception!");
        }
    }

    return route;
}



/*******************************************************************************
**
** Function         bta_dm_sco_co_open
**
** Description      This function is executed when a SCO connection is open.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_open(UINT16 handle, UINT8 pkt_size, UINT16 event)
{
    tBTUI_SCO_CODEC_CFG cfg;

    if (btui_cb.sco_hci)
    {
        BTIF_TRACE_DEBUG2("bta_dm_sco_co_open handle:%d pkt_size:%d", handle, pkt_size);
        /* use dedicated SCO buffer pool for SCO TX data */
        cfg.pool_id = HCI_SCO_POOL_ID;
        cfg.p_cback = btui_sco_codec_callback;
        cfg.pkt_size = pkt_size;
        cfg.cb_event = event;
        /* open and start the codec */
        btui_sco_codec_open(&cfg);
        btui_sco_codec_start(handle);
    }
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_close
**
** Description      This function is called when a SCO connection is closed
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_close(void)
{
    if (btui_cb.sco_hci)
    {
        BTIF_TRACE_DEBUG0("bta_dm_sco_co_close close codec");
        /* close sco codec */
        btui_sco_codec_close();

        btui_cb.sco_hci = FALSE;
    }
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_in_data
**
** Description      This function is called to send incoming SCO data to application.                 
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_in_data(BT_HDR  *p_buf)
{
    if (btui_cfg.sco_use_mic)
        btui_sco_codec_inqdata (p_buf);
    else
        GKI_freebuf(p_buf);
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_out_data
**
** Description      This function is called to send SCO data over HCI.                 
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_out_data(BT_HDR  **p_buf)
{    
    btui_sco_codec_readbuf(p_buf);
}

#endif /* #if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)*/


#if (defined BLE_INCLUDED && BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_le_io_key_req
**
** Description      This callout function is executed by DM to get BLE key information
**                  before SMP pairing gets going.
**
** Parameters       bd_addr  - The peer device
**                  *p_max_key_size - max key size local device supported.
**                  *p_init_key - initiator keys.
**                  *p_resp_key - responder keys. 
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_le_io_key_req(BD_ADDR bd_addr, UINT8 *p_max_key_size, 
                             tBTA_LE_KEY_TYPE *p_init_key, 
                             tBTA_LE_KEY_TYPE  *p_resp_key )
{
    BTIF_TRACE_ERROR0("##################################");
    BTIF_TRACE_ERROR0("bta_dm_co_le_io_key_req: only setting max size to 16");
    BTIF_TRACE_ERROR0("##################################");
    *p_max_key_size = 16;
    *p_init_key = *p_resp_key = 
                  (BTA_LE_KEY_PENC|BTA_LE_KEY_PID|BTA_LE_KEY_PCSRK|BTA_LE_KEY_LENC|BTA_LE_KEY_LID|BTA_LE_KEY_LCSRK);
}


/*******************************************************************************
**
** Function         bta_dm_co_ble_local_key_reload
**
** Description      This callout function is to load the local BLE keys if available
**                  on the device.
**
** Parameters       none 
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_ble_load_local_keys(tBTA_DM_BLE_LOCAL_KEY_MASK *p_key_mask, BT_OCTET16 er,
                                   tBTA_BLE_LOCAL_ID_KEYS *p_id_keys)
{
    BTIF_TRACE_ERROR0("##################################");
    BTIF_TRACE_ERROR0("bta_dm_co_ble_load_local_keys: TBD Load local keys if any are persisted");
    BTIF_TRACE_ERROR0("##################################");
}

/*******************************************************************************
**
** Function         bta_dm_co_ble_io_req
**
** Description      This callout function is executed by DM to get BLE IO capabilities
**                  before SMP pairing gets going.
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req -  Auth request setting (Bonding and MITM required or not)  
**                  *p_max_key_size - max key size local device supported.
**                  *p_init_key - initiator keys.
**                  *p_resp_key - responder keys. 
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_ble_io_req(BD_ADDR bd_addr,  tBTA_IO_CAP *p_io_cap, 
                          tBTA_OOB_DATA *p_oob_data,
                          tBTA_LE_AUTH_REQ *p_auth_req,
                          UINT8 *p_max_key_size, 
                          tBTA_LE_KEY_TYPE *p_init_key, 
                          tBTA_LE_KEY_TYPE  *p_resp_key )
{
    /* if OOB is not supported, this call-out function does not need to do anything 
     * otherwise, look for the OOB data associated with the address and set *p_oob_data accordingly
     * If the answer can not be obtained right away,
     * set *p_oob_data to BTA_OOB_UNKNOWN and call bta_dm_ci_io_req() when the answer is available */

    /* *p_auth_req by default is FALSE for devices with NoInputNoOutput; TRUE for other devices. */
    BTIF_TRACE_ERROR2("bta_dm_co_ble_io_req. p_auth_req=%d ble_authereq=%d", *p_auth_req, bte_appl_cfg.ble_auth_req);
    BTIF_TRACE_ERROR2("bta_dm_co_ble_io_req. p_io_cap=%d ble_io_cap=%d", *p_io_cap, bte_appl_cfg.ble_io_cap);
    BTIF_TRACE_ERROR2("bta_dm_co_ble_io_req. p_init_key=%d ble_init_key=%d", *p_init_key, bte_appl_cfg.ble_init_key);
    BTIF_TRACE_ERROR2("bta_dm_co_ble_io_req. p_resp_key=%d ble_resp_key=%d", *p_resp_key, bte_appl_cfg.ble_resp_key );
    BTIF_TRACE_ERROR2("bta_dm_co_ble_io_req. p_max_key_size=%d ble_max_key_size=%d", *p_max_key_size, bte_appl_cfg.ble_max_key_size );

    *p_oob_data = FALSE;
    if (bte_appl_cfg.ble_auth_req)
        *p_auth_req = bte_appl_cfg.ble_auth_req | (bte_appl_cfg.ble_auth_req & 0x04) | ((*p_auth_req) & 0x04);

    if (bte_appl_cfg.ble_io_cap <=4)
        *p_io_cap = bte_appl_cfg.ble_io_cap;

    if (bte_appl_cfg.ble_init_key<=7)
        *p_init_key = bte_appl_cfg.ble_init_key;

    if (bte_appl_cfg.ble_resp_key<=7)
        *p_resp_key = bte_appl_cfg.ble_resp_key;

    if (bte_appl_cfg.ble_max_key_size<=16)
        *p_max_key_size = bte_appl_cfg.ble_max_key_size;

}


#endif

