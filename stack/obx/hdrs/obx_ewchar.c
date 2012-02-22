/*****************************************************************************
**
**  Name:         obx_ewchar.c
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
** Function     OBX_AddAsciiNameHdr
**
** Description  This function is called to add an OBEX Name header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddAsciiNameHdr(BT_HDR *p_pkt, char *p_name)
{
    BOOLEAN status;
    UINT16  len = 0;
    UINT16  *p_unicode = NULL;

    if(p_name)
    {
        len = strlen(p_name) + 1;
        p_unicode = (UINT16 *)GKI_getbuf((UINT16)(len*2));
    }

    if(p_unicode)
        len = OBX_CharToWchar(p_unicode, p_name, len);
    else
        len = 0;

    status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_NAME, p_unicode, len);

    if(p_unicode)
        GKI_freebuf(p_unicode);
    return status;
}


/*******************************************************************************
**
** Function     OBX_AddAsciiDescrHdr
**
** Description  This function is called to add an OBEX Description header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddAsciiDescrHdr(BT_HDR *p_pkt, char *p_descr)
{
    BOOLEAN status;
    UINT16  len = strlen(p_descr) + 1;
    UINT16  *p_unicode = (UINT16 *)GKI_getbuf((UINT16)(len*2));

    len = OBX_CharToWchar(p_unicode, p_descr, len);
    status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_unicode, len);
    GKI_freebuf(p_unicode);
    return status;
}

/*******************************************************************************
**
** Function     OBX_AddAsciiDestNameHdr
**
** Description  This function is called to add an OBEX DestName header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddAsciiDestNameHdr(BT_HDR *p_pkt, char *p_dest)
{
    BOOLEAN status;
    UINT16  len = strlen(p_dest) + 1;
    UINT16  *p_unicode = (UINT16 *)GKI_getbuf((UINT16)(len*2));

    len = OBX_CharToWchar(p_unicode, p_dest, len);
    status = OBX_AddUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_unicode, len);
    GKI_freebuf(p_unicode);
    return status;
}



