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
#include "gki.h"
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_ag_api.h"
#include "bta_ag_co.h"
#include "bte_appl.h"

#define LOG_TAG "BTA_AG_CO: "

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/*******************************************************************************
**
** Function         bta_ag_co_init
**
** Description      This callout function is executed by AG when it is
**                  started by calling BTA_AgEnable().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.  
**                  
**
** Returns          Void.
**
*******************************************************************************/
void bta_ag_co_init(void)
{
    BTM_WriteVoiceSettings(AG_VOICE_SETTINGS);
}


/*******************************************************************************
**
** Function         bta_ag_co_audio_state
**
** Description      This function is called by the AG before the audio connection
**                  is brought up, after it comes up, and after it goes down.
**                   
** Parameters       handle - handle of the AG instance
**                  state - Audio state
**                      BTA_AG_CO_AUD_STATE_OFF     - Audio has been turned off
**                      BTA_AG_CO_AUD_STATE_OFF_XFER - Audio has been turned off (xfer)
**                      BTA_AG_CO_AUD_STATE_ON      - Audio has been turned on
**                      BTA_AG_CO_AUD_STATE_SETUP   - Audio is about to be turned on
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state)
{
    BTIF_TRACE_DEBUG2("bta_ag_co_audio_state: handle %d, state %d", handle, state);
}


/*******************************************************************************
**
** Function         bta_ag_co_data_open
**
** Description      This function is executed by AG when a service level connection
**                  is opened.  The phone can use this function to set
**                  up data paths or perform any required initialization or
**                  set up particular to the connected service.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_open(UINT16 handle, tBTA_SERVICE_ID service)
{
    BTIF_TRACE_DEBUG2("bta_ag_co_data_open handle:%d service:%d", handle, service);
}

/*******************************************************************************
**
** Function         bta_ag_co_data_close
**
** Description      This function is called by AG when a service level
**                  connection is closed
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_close(UINT16 handle)
{
    BTIF_TRACE_DEBUG1("bta_ag_co_data_close handle:%d", handle);
}


/*******************************************************************************
 **
 ** Function         bta_ag_co_tx_write
 **
 ** Description      This function is called by the AG to send data to the
 **                  phone when the AG is configured for AT command pass-through.
 **                  The implementation of this function must copy the data to
 **                  the phones memory.
 **
 ** Returns          void
 **
 *******************************************************************************/
void bta_ag_co_tx_write(UINT16 handle, UINT8 * p_data, UINT16 len)
{
   BTIF_TRACE_DEBUG2( "bta_ag_co_tx_write: handle: %d, len: %d", handle, len );
}

