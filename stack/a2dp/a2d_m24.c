/*****************************************************************************
**
**  Name:       a2d_m24.c
**
**  Description:utility functions to help build and parse MPEG-2, 4 AAC 
**              Codec Information Element and Media Payload.
**  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if (A2D_M24_INCLUDED == TRUE)
#include "a2d_api.h"
#include "a2d_int.h"
#include "a2d_m24.h"


/******************************************************************************
**
** Function         A2D_BldM24Info
**
** Description      This function is called by an application to build  
**                  the MPEG-2, 4 AAC Media Codec Capabilities byte sequence
**                  beginning from the LOSC octet.
**                  Input Parameters:
**                      media_type:  Indicates Audio, or Multimedia.
**
**                      p_ie:  MPEG-2, 4 AAC Codec Information Element information.
**
**                  Output Parameters:
**                      p_result:  the resulting codec info byte sequence.
**
** Returns          A2D_SUCCESS if function execution succeeded.
**                  Error status code, otherwise.
******************************************************************************/
tA2D_STATUS A2D_BldM24Info(UINT8 media_type, tA2D_M24_CIE *p_ie, UINT8 *p_result)
{
    tA2D_STATUS status;
    UINT16      bitrate45;

    if( p_ie == NULL || p_result == NULL ||
        (p_ie->obj_type & ~A2D_M24_IE_OBJ_MSK) ||
        (p_ie->samp_freq & ~A2D_M24_IE_SAMP_FREQ_MSK) ||
        (p_ie->chnl & ~A2D_M24_IE_CHNL_MSK) ||
        (p_ie->bitrate & ~A2D_M24_IE_BITRATE_MSK))
    {
        /* if any unused bit is set */
        status = A2D_INVALID_PARAMS;
    }
    else
    {
        status = A2D_SUCCESS;
        *p_result++ = A2D_M24_INFO_LEN;
        *p_result++ = media_type;
        *p_result++ = A2D_MEDIA_CT_M24;

        /* Media Codec Specific Information Element */
        *p_result++ = p_ie->obj_type;

        UINT16_TO_BE_STREAM(p_result, p_ie->samp_freq);
        p_result--;
        *p_result++ |= p_ie->chnl;

        *p_result = (p_ie->bitrate & A2D_M24_IE_BITRATE3_MSK) >> 16;
        if(p_ie->vbr)
            *p_result |= A2D_M24_IE_VBR_MSK;
        p_result++;
        bitrate45 = (UINT16)(p_ie->bitrate & A2D_M24_IE_BITRATE45_MSK);
        UINT16_TO_BE_STREAM(p_result, bitrate45);
    }
    return status;
}

/******************************************************************************
**
** Function         A2D_ParsM24Info
**
** Description      This function is called by an application to parse  
**                  the MPEG-2, 4 AAC Media Codec Capabilities byte sequence
**                  beginning from the LOSC octet.
**                  Input Parameters:
**                      p_info:  the byte sequence to parse.
**
**                      for_caps:  TRUE, if the byte sequence is for get capabilities response.
**
**                  Output Parameters:
**                      p_ie:  MPEG-2, 4 AAC Codec Information Element information.
**
** Returns          A2D_SUCCESS if function execution succeeded.
**                  Error status code, otherwise.
******************************************************************************/
tA2D_STATUS A2D_ParsM24Info(tA2D_M24_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps)
{
    tA2D_STATUS status;
    UINT8   vbr;
    UINT16  u16;
    UINT8   losc;
    UINT8   mt;

    if( p_ie == NULL || p_info == NULL)
        status = A2D_INVALID_PARAMS;
    else
    {
        losc    = *p_info++;
        mt      = *p_info++;
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if(losc != A2D_M24_INFO_LEN || *p_info != A2D_MEDIA_CT_M24)
            status = A2D_WRONG_CODEC;
        else
        {
            p_info++;
            p_ie->obj_type  = *p_info++;
            BE_STREAM_TO_UINT16(p_ie->samp_freq, p_info);
            p_ie->chnl      = p_ie->samp_freq & A2D_M24_IE_CHNL_MSK;
            p_ie->samp_freq &= A2D_M24_IE_SAMP_FREQ_MSK;
            vbr             = *p_info++;
            BE_STREAM_TO_UINT16(u16, p_info);      /* u16 contains the octect4, 5 of bterate */

            p_ie->vbr       = (vbr & A2D_M24_IE_VBR_MSK) >> 7;
            vbr            &= ~A2D_M24_IE_VBR_MSK; /* vbr has become the octect3 of bitrate */
            p_ie->bitrate   = vbr;
            p_ie->bitrate <<= 16;
            p_ie->bitrate  += u16;

            status = A2D_SUCCESS;
            if(for_caps == FALSE)
            {
                if(A2D_BitsSet(p_ie->obj_type) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_OBJ_TYPE;

                if((A2D_BitsSet((UINT8)(p_ie->samp_freq&0xFF)) +
                    A2D_BitsSet((UINT8)((p_ie->samp_freq&0xFF00)>>8)))!= A2D_SET_ONE_BIT)
                    status = A2D_BAD_SAMP_FREQ;

                if(A2D_BitsSet(p_ie->chnl) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_CHANNEL;

                /* BitRate must be greater than zero when specifying AAC configuration */
                if (p_ie->bitrate == 0)
                    status = A2D_BAD_BIT_RATE;
            }
        }
    }
    return status;
}
#endif /* (A2D_M24_INCLUDED == TRUE) */

