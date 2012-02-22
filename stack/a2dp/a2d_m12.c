/*****************************************************************************
**
**  Name:       a2d_m12.c
**
**  Description:utility functions to help build and parse MPEG-1, 2 Audio 
**              Codec Information Element and Media Payload.
**  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"

#if (A2D_M12_INCLUDED == TRUE)
#include "a2d_api.h"
#include "a2d_int.h"
#include "a2d_m12.h"


/******************************************************************************
**
** Function         A2D_BldM12Info
**
** Description      This function is called by an application to build 
**                  the MPEG-1, 2 Audio Media Codec Capabilities byte sequence
**                  beginning from the LOSC octet.
**                      media_type:  Indicates Audio, or Multimedia.
**
**                      p_ie:  The MPEG-1, 2 Audio Codec Information Element information.
**
**                  Output Parameters:
**                      p_result:  the resulting codec info byte sequence.
**
** Returns          A2D_SUCCESS if function execution succeeded.
**                  Error status code, otherwise.
******************************************************************************/
tA2D_STATUS A2D_BldM12Info(UINT8 media_type, tA2D_M12_CIE *p_ie, UINT8 *p_result)
{
    tA2D_STATUS status;
    UINT8       *p_vbr;

    if( p_ie == NULL || p_result == NULL ||
        (p_ie->layer & ~A2D_M12_IE_LAYER_MSK) ||
        (p_ie->ch_mode & ~A2D_M12_IE_CH_MD_MSK) ||
        (p_ie->samp_freq & ~A2D_M12_IE_SAMP_FREQ_MSK) ||
        (p_ie->bitrate & ~A2D_M12_IE_BITRATE_MSK))
    {
        /* if any unused bit is set */
        status = A2D_INVALID_PARAMS;
    }
    else
    {
        status = A2D_SUCCESS;
        *p_result++ = A2D_M12_INFO_LEN;
        *p_result++ = media_type;
        *p_result++ = A2D_MEDIA_CT_M12;

        /* Media Codec Specific Information Element */
        *p_result = p_ie->layer | p_ie->ch_mode;
        if(p_ie->crc)
            *p_result |= A2D_M12_IE_CRC_MSK;
        p_result++;

        *p_result = p_ie->samp_freq;
        if(p_ie->mpf)
            *p_result |= A2D_M12_IE_MPF_MSK;
        p_result++;

        p_vbr = p_result;
        UINT16_TO_BE_STREAM(p_result, p_ie->bitrate);
        if(p_ie->vbr)
            *p_vbr |= A2D_M12_IE_VBR_MSK;
    }
    return status;
}

/******************************************************************************
**
** Function         A2D_ParsM12Info
**
** Description      This function is called by an application to parse 
**                  the MPEG-1, 2 Audio Media Codec Capabilities byte sequence
**                  beginning from the LOSC octet.
**                  Input Parameters:
**                      p_info:  the byte sequence to parse.
**
**                      for_caps:  TRUE, if the byte sequence is for get capabilities response.
**
**                  Output Parameters:
**                      p_ie:  The MPEG-1, 2 Audio Codec Information Element information.
**
** Returns          A2D_SUCCESS if function execution succeeded.
**                  Error status code, otherwise.
******************************************************************************/
tA2D_STATUS A2D_ParsM12Info(tA2D_M12_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps)
{
    tA2D_STATUS status;
    UINT8   losc;
    UINT8   mt;

    if( p_ie == NULL || p_info == NULL)
        status = A2D_INVALID_PARAMS;
    else
    {
        losc    = *p_info++;
        mt      = *p_info++;
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if(losc != A2D_M12_INFO_LEN || *p_info != A2D_MEDIA_CT_M12)
            status = A2D_WRONG_CODEC;
        else
        {
            p_info++;
            p_ie->layer     = *p_info & A2D_M12_IE_LAYER_MSK;
            p_ie->crc       = (*p_info & A2D_M12_IE_CRC_MSK) >> 4;
            p_ie->ch_mode   = *p_info & A2D_M12_IE_CH_MD_MSK;
            p_info++;
            p_ie->mpf       = (*p_info & A2D_M12_IE_MPF_MSK) >> 6;
            p_ie->samp_freq = *p_info & A2D_M12_IE_SAMP_FREQ_MSK;
            p_info++;
            p_ie->vbr       = (*p_info & A2D_M12_IE_VBR_MSK) >> 7;
            BE_STREAM_TO_UINT16(p_ie->bitrate, p_info);
            p_ie->bitrate  &= A2D_M12_IE_BITRATE_MSK;

            status = A2D_SUCCESS;
            if(for_caps == FALSE)
            {
                if(A2D_BitsSet(p_ie->layer) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_LAYER;
                if(A2D_BitsSet(p_ie->ch_mode) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_CH_MODE;
                if(A2D_BitsSet(p_ie->samp_freq) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_SAMP_FREQ;
                if((A2D_BitsSet((UINT8)(p_ie->bitrate&0xFF)) +
                    A2D_BitsSet((UINT8)((p_ie->bitrate&0xFF00)>>8)))!= A2D_SET_ONE_BIT)
                    status = A2D_BAD_BIT_RATE;
            }
        }
    }
    return status;
}

#endif /* A2D_M12_INCLUDED == TRUE */
