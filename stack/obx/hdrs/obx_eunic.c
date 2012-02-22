/*****************************************************************************
**
**  Name:         obx_eunic.c
**
**  File:         OBEX Headers Encode Utility functions
**                - Unicode functions
**
**  Copyright (c) 2003-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "obx_api.h"


/*******************************************************************************
**
** Function     OBX_AddNameHdr
**
** Description  This function is called to add an OBEX Name header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddNameHdr(BT_HDR *p_pkt, UINT16 *p_name, UINT16 len)
{
    return OBX_AddUnicodeHdr(p_pkt, OBX_HI_NAME, p_name, len);
}

/*******************************************************************************
**
** Function     OBX_AddDescrHdr
**
** Description  This function is called to add an OBEX Description header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddDescrHdr(BT_HDR *p_pkt, UINT16 *p_descr, UINT16 len)
{
    return OBX_AddUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_descr, len);
}

/*******************************************************************************
**
** Function     OBX_AddDestNameHdr
**
** Description  This function is called to add an OBEX DestName header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddDestNameHdr(BT_HDR *p_pkt, UINT16 *p_dest, UINT16 len)
{
    return OBX_AddUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_dest, len);
}


