/*****************************************************************************
**
**  Name:         obx_dbtp.c
**
**  File:         OBEX Headers Decode Utility functions
**                - for current Bluetooth profiles
**
**  Copyright (c) 2003-2008, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "obx_api.h"
#include "obx_int.h"




/*******************************************************************************
**
** Function     OBX_ReadTypeHdr
**
** Description  This function is called to get the Type Header in the given 
**              OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadTypeHdr(BT_HDR *p_pkt, UINT8 **p_type, UINT16 *p_len)
{
    return OBX_ReadByteStrHdr(p_pkt, OBX_HI_TYPE, p_type, p_len, 0);
}

/*******************************************************************************
**
** Function     OBX_ReadLengthHdr
**
** Description  This function is called to get the Length Header in the given 
**              OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadLengthHdr(BT_HDR *p_pkt, UINT32 *p_len)
{
    return OBX_Read4ByteHdr(p_pkt, OBX_HI_LENGTH, p_len);
}

/*******************************************************************************
**
** Function     OBX_ReadTargetHdr
**
** Description  This function is called to get the Target Header in the
**              given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadTargetHdr(BT_HDR *p_pkt, UINT8 **p_target, UINT16 *p_len, UINT8 next)
{
    return OBX_ReadByteStrHdr(p_pkt, OBX_HI_TARGET, p_target, p_len, next);
}

/*******************************************************************************
**
** Function     OBX_ReadBodyHdr
**
** Description  This function is called to get the Body Header in the
**              given OBEX packet.
**
** Returns      1, if a single header is in the OBEX packet.
**              2, if a end of body header is in the OBEX packet.
**              0, (FALSE) otherwise.
**
*******************************************************************************/
UINT8 OBX_ReadBodyHdr(BT_HDR *p_pkt, UINT8 **p_body, UINT16 *p_len, BOOLEAN *p_end)
{
    BOOLEAN status;
    UINT8   *p_body2;
    UINT16  len2;
    UINT8   num;

    *p_end  = FALSE;
    num     = OBX_ReadByteStrHdr(p_pkt, OBX_HI_BODY, p_body, p_len, 0);
    status  = OBX_ReadByteStrHdr(p_pkt, OBX_HI_BODY_END, &p_body2, &len2, 0);
    if(num == FALSE && status == TRUE)
    {
        *p_body = p_body2;
        *p_len  = len2;
        *p_end  = TRUE;
    }
    num += status;

    return num;
}

/*******************************************************************************
**
** Function     OBX_ReadWhoHdr
**
** Description  This function is called to get the Who Header in the
**              given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadWhoHdr(BT_HDR *p_pkt, UINT8 **p_who, UINT16 *p_len)
{
    return OBX_ReadByteStrHdr(p_pkt, OBX_HI_WHO, p_who, p_len, 0);
}

/*******************************************************************************
**
** Function     OBX_ReadAppParamHdr
**
** Description  This function is called to get the Application Parameter Header
**              in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadAppParamHdr(BT_HDR *p_pkt, UINT8 *p_tag, UINT8 **p_app_param, UINT8 *p_len, UINT8 next)
{
    BOOLEAN         status;
    tOBX_TRIPLET    triplet[OBX_MAX_TRIPLET];
    UINT8           num = OBX_MAX_TRIPLET;

     /* The coverity complaints on this function is not correct. 
      * The value in triplet[] is set/initialized by OBX_ReadTriplet if it returns TRUE.
      * leave this unnecessary memset here */
    memset( triplet, 0, sizeof(triplet ));

    status = OBX_ReadTriplet(p_pkt, OBX_HI_APP_PARMS, triplet, &num);
    if(status == TRUE)
    {
        if(next >= num)
        {
            status      = FALSE;
        }
        else
        {
            *p_tag      = triplet[next].tag;
            *p_app_param = triplet[next].p_array;
            *p_len      = triplet[next].len;
            OBX_TRACE_DEBUG3( "OBX_ReadAppParamHdr: next: %d, tag: %x, len: %d",
                next, *p_tag, *p_len);
        }
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadPermissionHdr
**
** Description  This function is called to get the Application Parameter Header
**              in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadPermissionHdr(BT_HDR *p_pkt, UINT8 *p_user, UINT8 *p_group, UINT8 *p_other)
{
    UINT32 permission;
    BOOLEAN status = FALSE;

    if (OBX_Read4ByteHdr(p_pkt, OBX_HI_PERMISSION, &permission))
    {
        *p_other    = (permission & 0xFF);
        permission >>= 8;
        *p_group    = (permission & 0xFF);
        permission >>= 8;
        *p_user     = (permission & 0xFF);
        status = TRUE;
    }

    return status;
}




