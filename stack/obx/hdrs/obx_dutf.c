/*****************************************************************************
**
**  Name:         obx_dutf.c
**
**  File:         OBEX Headers Decode Utility functions
**                - Unicode conversion functions
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "gki.h"
#include "obx_api.h"


/*******************************************************************************
**
** Function     OBX_ReadUtf8NameHdr
**
** Description  This function is called to get the Name Header in the given 
**              OBEX packet. If Name header exists in the given OBEX packet,
**              it is converted to UTF8 format and copied into p_name.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadUtf8NameHdr(BT_HDR *p_pkt, UINT8 *p_name, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_NAME, p_unicode, &len);
    utfc_16_to_8(p_name, max_len, p_unicode, len);
    GKI_freebuf(p_unicode);
    return status;
}



/*******************************************************************************
**
** Function     OBX_ReadUtf8DescrHdr
**
** Description  This function is called to get the Description Header in the
**              given OBEX packet. If Description header exists in the given
**              OBEX packet, it is converted to UTF8 format and copied into
**              p_descr.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadUtf8DescrHdr(BT_HDR *p_pkt, UINT8 *p_descr, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_unicode, &len);
    utfc_16_to_8(p_descr, max_len, p_unicode, len);
    GKI_freebuf(p_unicode);
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadUtf8DestNameHdr
**
** Description  This function is called to get the DestName Header in the
**              given OBEX packet. 
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadUtf8DestNameHdr(BT_HDR *p_pkt, UINT8 *p_dest, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_unicode, &len);
    utfc_16_to_8(p_dest, max_len, p_unicode, len);
    GKI_freebuf(p_unicode);
    return status;
}


