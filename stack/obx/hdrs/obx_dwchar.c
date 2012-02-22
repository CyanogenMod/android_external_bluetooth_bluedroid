/*****************************************************************************
**
**  Name:         obx_dwchar.c
**
**  File:         OBEX Headers Decode Utility functions
**                - Unicode conversion functions
**
**  Copyright (c) 2003-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "gki.h"
#include "obx_api.h"


/*******************************************************************************
**
** Function     OBX_ReadAsciiNameHdr
**
** Description  This function is called to get the Name Header in the given 
**              OBEX packet. If Name header exists in the given OBEX packet,
**              it is converted to ASCII format and copied into p_name.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadAsciiNameHdr(BT_HDR *p_pkt, char *p_name, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_NAME, p_unicode, &len);
    OBX_WcharToChar(p_name, p_unicode, max_len);
    GKI_freebuf(p_unicode);
    return status;
}



/*******************************************************************************
**
** Function     OBX_ReadAsciiDescrHdr
**
** Description  This function is called to get the Description Header in the
**              given OBEX packet. If Description header exists in the given
**              OBEX packet, it is converted to ASCII format and copied into
**              p_descr.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadAsciiDescrHdr(BT_HDR *p_pkt, char *p_descr, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_unicode, &len);
    OBX_WcharToChar(p_descr, p_unicode, max_len);
    GKI_freebuf(p_unicode);
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadAsciiDestNameHdr
**
** Description  This function is called to get the DestName Header in the
**              given OBEX packet. If DestName header exists in the given
**              OBEX packet, it is converted to ASCII format and copied into
**              p_descr.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadAsciiDestNameHdr(BT_HDR *p_pkt, char *p_dest, UINT16 max_len)
{
    BOOLEAN status;
    UINT16 *p_unicode = (UINT16 *)GKI_getbuf((UINT16)((max_len + 1) * 2));
    UINT16 len = max_len;

    status = OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_unicode, &len);
    OBX_WcharToChar(p_dest, p_unicode, max_len);
    GKI_freebuf(p_unicode);
    return status;
}
