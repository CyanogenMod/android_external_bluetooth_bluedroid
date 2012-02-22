/*****************************************************************************
**
**  Name:         obx_eopt.c
**
**  File:         OBEX Headers Encode Utility functions
**                - optional functions
**
**  Copyright (c) 2003-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "obx_api.h"



/*******************************************************************************
**
** Function     OBX_AddTimeHdr
**
** Description  This function is called to add an OBEX Time header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddTimeHdr(BT_HDR *p_pkt, char *p_time)
{
    return OBX_AddByteStrHdr(p_pkt, OBX_HI_TIME, (UINT8 *)p_time, (UINT16)strlen(p_time));
}


/*******************************************************************************
**
** Function     OBX_AddHttpHdr
**
** Description  This function is called to add an OBEX HTTP header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddHttpHdr(BT_HDR *p_pkt, UINT8 *p_http, UINT16 len)
{
    return OBX_AddByteStrHdr(p_pkt, OBX_HI_HTTP, p_http, len);
}


