/*****************************************************************************
**
**  Name:         obx_dunic.c
**
**  File:         OBEX Headers Decode Utility functions
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
** Function     OBX_ReadNameHdr
**
** Description  This function is called to get the Name Header in the given 
**              OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadNameHdr(BT_HDR *p_pkt, UINT16 *p_name, UINT16 *p_len)
{
    return OBX_ReadUnicodeHdr(p_pkt, OBX_HI_NAME, p_name, p_len);
}

/*******************************************************************************
**
** Function     OBX_ReadDescrHdr
**
** Description  This function is called to get the Description Header in the
**              given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadDescrHdr(BT_HDR *p_pkt, UINT16 *p_descr, UINT16 *p_len)
{
    return OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DESCRIPTION, p_descr, p_len);
}

/*******************************************************************************
**
** Function     OBX_ReadDestNameHdr
**
** Description  This function is called to get the DestName Header in the
**              given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadDestNameHdr(BT_HDR *p_pkt, UINT16 *p_dest, UINT16 *p_len)
{
    return OBX_ReadUnicodeHdr(p_pkt, OBX_HI_DEST_NAME, p_dest, p_len);
}
