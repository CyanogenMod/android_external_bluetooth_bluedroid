/*****************************************************************************
**
**  Name:         obx_ebtp.c
**
**  File:         OBEX Headers Encode Utility functions
**                - for current Bluetooth profiles
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
** Function     OBX_AddTypeHdr
**
** Description  This function is called to add an OBEX Type header
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddTypeHdr(BT_HDR *p_pkt, char *p_type)
{
    return OBX_AddByteStrHdr(p_pkt, OBX_HI_TYPE, (UINT8*)p_type, (UINT16)(strlen(p_type)+1));
}

/*******************************************************************************
**
** Function     OBX_AddLengthHdr
**
** Description  This function is called to add an OBEX Length header to an OBEX
**              packet. The Length header describes the total length in bytes of
**              the object.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddLengthHdr(BT_HDR *p_pkt, UINT32 len)
{
    return OBX_Add4ByteHdr(p_pkt, OBX_HI_LENGTH, len);
}

/*******************************************************************************
**
** Function     OBX_AddTargetHdr
**
** Description  This function is called to add an OBEX Target header to an OBEX
**              packet. This header is most commonly used in Connect packets. 
**
**              NOTE: The target header must be the first header in an OBEX message.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddTargetHdr(BT_HDR *p_pkt, UINT8 *p_target, UINT16 len)
{
    BOOLEAN add = FALSE;
    UINT8   *p;

    if (p_pkt->len == 0)
    {
        add = TRUE;
    }
    else if (p_pkt->len == 2)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset;
        if ((*p) == OBX_HI_SESSION_SN)
            add = TRUE;

    }
        /* allow SSN to preceed target header */
    if (add)
        return OBX_AddByteStrHdr(p_pkt, OBX_HI_TARGET, p_target, len);
    else
    {
        BT_ERROR_TRACE_1(TRACE_LAYER_OBEX, "Target header must be the first:%d", p_pkt->len) ;
        return FALSE;
    }
}

/*******************************************************************************
**
** Function     OBX_AddBodyHdr
**
** Description  This function is called to add an OBEX body header
**              to an OBEX packet.
**
**              NOTE: The body header must be the last header in an OBEX message.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddBodyHdr(BT_HDR *p_pkt, UINT8 *p_body, UINT16 len, BOOLEAN end)
{
    UINT8 id = (end)?OBX_HI_BODY_END:OBX_HI_BODY;
    return OBX_AddByteStrHdr(p_pkt, id, p_body, len);
}


/*******************************************************************************
**
** Function     OBX_AddWhoHdr
**
** Description  This function is called to add an OBEX Who header to an OBEX
**              packet.
**
** Note:        Who header is typically used in an OBEX CONNECT response packet
**              to indicate the UUID of the service that has accepted the
**              directed connection. If the server calls OBX_StartServer() with
**              specified target header, this OBEX implementation automatically
**              adds this WHO header to the CONNECT response packet. If
**              OBX_StartServer() is called with target header length as 0, the
**              OBEX API user is responsible to add the WHO header.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddWhoHdr(BT_HDR *p_pkt, UINT8 *p_who, UINT16 len)
{
    return OBX_AddByteStrHdr(p_pkt, OBX_HI_WHO, p_who, len);
}

/*******************************************************************************
**
** Function     OBX_AddAppParamHdr
**
** Description  This function is called to add an OBEX Application Parameter
**              header to an OBEX packet. This header is used by the application
**              layer above OBEX to convey application specific information.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddAppParamHdr(BT_HDR *p_pkt, tOBX_TRIPLET *p_triplet, UINT8 num)
{
    return OBX_AddTriplet(p_pkt, OBX_HI_APP_PARMS, p_triplet, num);
}

/*******************************************************************************
**
** Function     OBX_AddPermissionHdr
**
** Description  This function is called to add an OBEX Permission header to an OBEX
**              packet.
**              bit 0 is set for read permission
**              bit 1 is set for write permission
**              bit 2 is set for delete permission
**              bit 7 is set for modify permission
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddPermissionHdr(BT_HDR *p_pkt, UINT8 user, UINT8 group, UINT8 other)
{
    UINT32 permission = user;
    permission <<= 8;
    permission |= group;
    permission <<= 8;
    permission |= other;
    return OBX_Add4ByteHdr(p_pkt, OBX_HI_PERMISSION, permission);
}


