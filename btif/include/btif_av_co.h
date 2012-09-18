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

#ifndef BTIF_AV_CO_H
#define BTIF_AV_CO_H

#include "btif_media.h"

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

enum
{
    BTIF_SV_AV_AA_SBC_INDEX = 0,
    BTIF_SV_AV_AA_SEP_INDEX  /* Last index */
};


/*******************************************************************************
**  Functions
********************************************************************************/

/*******************************************************************************
 **
 ** Function         bta_av_co_cp_is_active
 **
 ** Description      Get the current configuration of content protection
 **
 ** Returns          TRUE if the current streaming has CP, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN bta_av_co_cp_is_active(void);

/*******************************************************************************
 **
 ** Function         bta_av_co_cp_get_flag
 **
 ** Description      Get content protection flag
 **                  BTA_AV_CP_SCMS_COPY_NEVER
 **                  BTA_AV_CP_SCMS_COPY_ONCE
 **                  BTA_AV_CP_SCMS_COPY_FREE
 **
 ** Returns          The current flag value
 **
 *******************************************************************************/
UINT8 bta_av_co_cp_get_flag(void);

/*******************************************************************************
 **
 ** Function         bta_av_co_cp_set_flag
 **
 ** Description      Set content protection flag
 **                  BTA_AV_CP_SCMS_COPY_NEVER
 **                  BTA_AV_CP_SCMS_COPY_ONCE
 **                  BTA_AV_CP_SCMS_COPY_FREE
 **
 ** Returns          TRUE if setting the SCMS flag is supported else FALSE
 **
 *******************************************************************************/
BOOLEAN bta_av_co_cp_set_flag(UINT8 cp_flag);

/*******************************************************************************
 **
 ** Function         bta_av_co_audio_codec_reset
 **
 ** Description      Reset the current codec configuration
 **
 ** Returns          void
 **
 *******************************************************************************/
void bta_av_co_audio_codec_reset(void);

/*******************************************************************************
 **
 ** Function         bta_av_co_audio_codec_supported
 **
 ** Description      Check if all opened connections are compatible with a codec
 **                  configuration
 **
 ** Returns          TRUE if all opened devices support this codec, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN bta_av_co_audio_codec_supported(tBTIF_STATUS *p_status);

/*******************************************************************************
 **
 ** Function         bta_av_co_audio_set_codec
 **
 ** Description      Set the current codec configuration from the feeding type.
 **                  This function is starting to modify the configuration, it
 **                  should be protected.
 **
 ** Returns          TRUE if successful, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN bta_av_co_audio_set_codec(const tBTIF_AV_MEDIA_FEEDINGS *p_feeding, tBTIF_STATUS *p_status);

/*******************************************************************************
 **
 ** Function         bta_av_co_audio_get_sbc_config
 **
 ** Description      Retrieves the SBC codec configuration.  If the codec in use
 **                  is not SBC, return the default SBC codec configuration.
 **
 ** Returns          TRUE if codec is SBC, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN bta_av_co_audio_get_sbc_config(tA2D_SBC_CIE *p_sbc_config, UINT16 *p_minmtu);

/*******************************************************************************
 **
 ** Function         bta_av_co_audio_discard_config
 **
 ** Description      Discard the codec configuration of a connection
 **
 ** Returns          Nothing
 **
 *******************************************************************************/
void bta_av_co_audio_discard_config(tBTA_AV_HNDL hndl);

/*******************************************************************************
 **
 ** Function         bta_av_co_init
 **
 ** Description      Initialization
 **
 ** Returns          Nothing
 **
 *******************************************************************************/
void bta_av_co_init(void);


/*******************************************************************************
 **
 ** Function         bta_av_co_peer_cp_supported
 **
 ** Description      Checks if the peer supports CP
 **
 ** Returns          TRUE if the peer supports CP
 **
 *******************************************************************************/
BOOLEAN bta_av_co_peer_cp_supported(tBTA_AV_HNDL hndl);

/*******************************************************************************
 **
 ** Function         bta_av_co_get_remote_bitpool_pref
 **
 ** Description      Check if remote side did a setconfig within the limits
 **                  of our exported bitpool range. If set we will set the
 **                  remote preference.
 **
 ** Returns          TRUE if config set, FALSE otherwize
 **
 *******************************************************************************/
BOOLEAN bta_av_co_get_remote_bitpool_pref(UINT8 *min, UINT8 *max);

#endif
