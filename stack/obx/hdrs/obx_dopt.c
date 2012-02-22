/*****************************************************************************
**
**  Name:         obx_dopt.c
**
**  File:         OBEX Headers Decode Utility functions
**                - optional functions
**
**  Copyright (c) 2003-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "obx_api.h"






/*******************************************************************************
**
** Function     OBX_ReadTimeHdr
**
** Description  This function is called to get the Time Header in the given 
**              OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadTimeHdr(BT_HDR *p_pkt, UINT8 **p_time, UINT16 *p_len)
{
    return OBX_ReadByteStrHdr(p_pkt, OBX_HI_TIME, p_time, p_len, 0);
}


/*******************************************************************************
**
** Function     OBX_ReadHttpHdr
**
** Description  This function is called to get the HTTP Header in the
**              given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadHttpHdr(BT_HDR *p_pkt, UINT8 **p_http, UINT16 *p_len, UINT8 next)
{
    return OBX_ReadByteStrHdr(p_pkt, OBX_HI_HTTP, p_http, p_len, next);
}

