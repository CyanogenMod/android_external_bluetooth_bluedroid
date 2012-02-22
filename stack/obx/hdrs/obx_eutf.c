/*****************************************************************************
**
**  Name:         obx_eutf.c
**
**  File:         OBEX Headers Encode Utility functions
**                - Unicode conversion functions
**
**  Copyright (c) 2003-2005, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "gki.h"
#include "obx_api.h"


/*******************************************************************************
**
** Function     OBX_AddUtf8NameHdr
**
** Description  This function is called to add an OBEX Name header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddUtf8NameHdr(BT_HDR *p_pkt, UINT8 *p_name)
{
    BOOLEAN     status = FALSE;
    UINT16      utf16_len = 0;
    UINT16      *p_utf16 = NULL;

    if (p_name)
    {
        utf16_len = strlen((char *)p_name) * 2 + 2;
        p_utf16 = (UINT16 *)GKI_getbuf(utf16_len);
    }

    if (p_utf16)
        utf16_len = utfc_8_to_16(p_utf16, utf16_len, p_name);
    else
        utf16_len = 0;

    status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_NAME, p_utf16, utf16_len);

    if (p_utf16)
        GKI_freebuf(p_utf16);

    return status;
}

/*******************************************************************************
**
** Function     OBX_AddUtf8DescrHdr
**
** Description  This function is called to add an OBEX Description header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddUtf8DescrHdr(BT_HDR *p_pkt, UINT8 *p_descr)
{
    BOOLEAN     status = FALSE;
    UINT16      utf16_len;
    UINT16      *p_utf16;

    utf16_len = strlen((char *)p_descr) * 2 + 2;
    p_utf16 = (UINT16 *)GKI_getbuf(utf16_len);
    if(p_utf16)
    {
        utf16_len = utfc_8_to_16(p_utf16, utf16_len, p_descr);
        status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_utf16, utf16_len);
        GKI_freebuf(p_utf16);
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_AddUtf8DestNameHdr
**
** Description  This function is called to add an OBEX DestName header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddUtf8DestNameHdr(BT_HDR *p_pkt, UINT8 *p_dest)
{
    BOOLEAN     status = FALSE;
    UINT16      utf16_len;
    UINT16      *p_utf16;

    utf16_len = strlen((char *)p_dest) * 2 + 2;
    p_utf16 = (UINT16 *)GKI_getbuf(utf16_len);
    if(p_utf16)
    {
        utf16_len = utfc_8_to_16(p_utf16, utf16_len, p_dest);
        status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_utf16, utf16_len);
        GKI_freebuf(p_utf16);
    }
    return status;
}



