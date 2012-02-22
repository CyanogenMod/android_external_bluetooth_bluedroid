/*****************************************************************************
**
**  Name:         obx_gen.c
**
**  File:         OBEX Headers Utility functions
**                - common/generic functions
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "bt_target.h"
#include "wcassert.h"
#include "obx_api.h"
#include "gki.h"
#include "obx_int.h"

const UINT8 obx_hdr_start_offset[] =
{
    OBX_CONN_HDRS_OFFSET,       /* OBX_CONNECT_REQ_EVT */
    OBX_SESS_HDRS_OFFSET,       /* OBX_SESSION_REQ_EVT */
    OBX_DISCON_HDRS_OFFSET,     /* OBX_DISCONNECT_REQ_EVT */
    OBX_PUT_HDRS_OFFSET,        /* OBX_PUT_REQ_EVT */
    OBX_GET_HDRS_OFFSET,        /* OBX_GET_REQ_EVT */
    OBX_SETPATH_REQ_HDRS_OFFSET,/* OBX_SETPATH_REQ_EVT */
    OBX_ABORT_HDRS_OFFSET,      /* OBX_ABORT_REQ_EVT */
    OBX_ACTION_HDRS_OFFSET,     /* OBX_ACTION_REQ_EVT */
    OBX_CONN_HDRS_OFFSET,       /* OBX_CONNECT_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_SESSION_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_DISCONNECT_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_PUT_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_GET_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_SETPATH_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET,   /* OBX_ABORT_RSP_EVT */
    OBX_RESPONSE_HDRS_OFFSET    /* OBX_ACTION_RSP_EVT */
};

/*******************************************************************************
**
** Function     obx_access_rsp_code
**
** Description  This function is used to read/change response code
**
** Returns      void.
**
*******************************************************************************/
void obx_access_rsp_code(BT_HDR *p_pkt, UINT8 *p_rsp_code)
{
    UINT8   *p = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    if(*p_rsp_code == OBX_RSP_DEFAULT)
        *p_rsp_code = *p;
    else
        *p = *p_rsp_code;
}

/*******************************************************************************
**
** Function     obx_adjust_packet_len
**
** Description  Adjust the packet length in the OBEX packet
**
** Returns      void.
**
*******************************************************************************/
void obx_adjust_packet_len(BT_HDR *p_pkt)
{
    UINT8   *p = (UINT8 *)(p_pkt + 1) + p_pkt->offset + 1;
    UINT16_TO_BE_STREAM(p, p_pkt->len);
}

/*******************************************************************************
**
** Function     obx_read_header_len
**
** Description  ph is the beginning of an OBEX header
**
** Returns      total length of the header
**
*******************************************************************************/
UINT16 obx_read_header_len(UINT8 *ph)
{
    UINT16  len = 0;

    /*
    OBX_TRACE_DEBUG1( "obx_read_header_len: 0x%x", *ph);
    */
    switch(*ph&OBX_HI_TYPE_MASK)
    {
    case OBX_HI_TYPE_BYTE:
        len = 2;
        break;
    case OBX_HI_TYPE_INT:
        len = 5;
        break;
    case OBX_HI_TYPE_ARRAY:
    case OBX_HI_TYPE_UNIC:
        ph++;
        BE_STREAM_TO_UINT16(len, ph);
        break;
    }
    /*
    OBX_TRACE_DEBUG1( "len:%d", len);
    */
    return len;
}

/*******************************************************************************
**
** Function     obx_dup_pkt
**
** Description  This function duplicate the OBEX message
**
** Returns      BT_HDR *.
**
*******************************************************************************/
BT_HDR * obx_dup_pkt(BT_HDR *p_pkt)
{
    BT_HDR *p_new;
    UINT16 size = p_pkt->len + p_pkt->offset + BT_HDR_SIZE;

    if (size < GKI_MAX_BUF_SIZE)
    {
        /* Use the largest general pool to allow challenge tags appendage */
        p_new = (BT_HDR *)GKI_getbuf(GKI_MAX_BUF_SIZE);
    }
    else
    {
        p_new = (BT_HDR *) GKI_getpoolbuf(OBX_LRG_DATA_POOL_ID);
    }

    if (p_new)
        memcpy(p_new, p_pkt, size );

    return p_new;
}


/*******************************************************************************
**
** Function     OBX_HdrInit
**
** Description  This function is called to initialize an OBEX packet. This
**              function takes a GKI buffer and sets the offset in BT_HDR as
**              OBX_HDR_OFFSET, the len as 0. The layer_specific is set to the
**              length still available. This function compares the given
**              (pkt_size - BT_HDR_SIZE) with the peer MTU to get the lesser
**              of the two and set the layer_specific to
**              (lesser_size - OBX_HDR_OFFSET).
**              If composing a header for the CONNECT request (there is no
**              client handle yet), use OBX_HANDLE_NULL as the handle.
**
**              If the pkt_size is larger than the largest public pool size,
**              GKI_MAX_BUF_SIZE, then an attempt to grab a buffer from the reserved OBX 
**              data pool will be made.
** Returns      BT_HDR *.
**
*******************************************************************************/
BT_HDR *OBX_HdrInit(tOBX_HANDLE handle, UINT16 pkt_size)
{
    UINT16  mtu     = OBX_HandleToMtu(handle);
    BT_HDR  *p_pkt = NULL;
    UINT16  buf_size;
#if (BT_USE_TRACES == TRUE)
    UINT16  req_size = pkt_size;
#endif

    WC_ASSERT(pkt_size > (BT_HDR_SIZE + OBX_HDR_OFFSET));

    pkt_size -= BT_HDR_SIZE;
    if(pkt_size > mtu )
        pkt_size = mtu;
    pkt_size    += (BT_HDR_SIZE + OBX_HDR_OFFSET);

    OBX_TRACE_DEBUG4( "OBX_HdrInit: checking req_size %d, pkt_size:%d, max:%d, offset:%d",
        req_size, pkt_size, GKI_MAX_BUF_SIZE, OBX_HDR_OFFSET);
    /* See if packet will fit in regular public pool */
    if ((pkt_size) < GKI_MAX_BUF_SIZE)
    {
        p_pkt = (BT_HDR *) GKI_getbuf(pkt_size);
    }
    else    /* Must use the reserved OBX buffer pool */
    {
        p_pkt = (BT_HDR *) GKI_getpoolbuf(OBX_LRG_DATA_POOL_ID);
        if (!p_pkt)
        {
            OBX_TRACE_DEBUG1( "Out of Large buffers. Trying pkt_size:%d", GKI_MAX_BUF_SIZE);
            p_pkt = (BT_HDR *) GKI_getbuf(GKI_MAX_BUF_SIZE);
        }
    }

    if(p_pkt)
    {
        buf_size    = GKI_get_buf_size(p_pkt);
        buf_size    -= BT_HDR_SIZE;
        if(buf_size > mtu)
            buf_size = mtu;

        OBX_TRACE_DEBUG4( "OBX_HdrInit: req_size %d, pkt_size = %d, gki_size %d, buf_size %d",
                            req_size, pkt_size, GKI_get_buf_size(p_pkt), buf_size);

        p_pkt->offset   = OBX_HDR_OFFSET;
        p_pkt->len      = 0;
        p_pkt->event    = 0;

        /* layer specific contains remaining space in packet */
        p_pkt->layer_specific = buf_size - OBX_HDR_OFFSET ;
        p_pkt->layer_specific -= 2;

        OBX_TRACE_DEBUG2( "buf size: %d, ls:%d", buf_size, p_pkt->layer_specific);
    }
    else
    {
        OBX_TRACE_ERROR1("OBX_HdrInit: No buffers for size (%d)", pkt_size);
    }

    return p_pkt;
}


/*******************************************************************************
**
** Function     OBX_Add1ByteHdr
**
** Description  This function is called to add a header with type as UINT8
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_Add1ByteHdr(BT_HDR *p_pkt, UINT8 id, UINT8 data)
{
    UINT8   *p;
    BOOLEAN status = FALSE;
    UINT16  size = 2;    /* total length added by this header - 1/hi & 1/hv */

    if(p_pkt)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
        /* verify that the HI is of correct type and the remaining length in the packet is good */
        if( ((id & OBX_HI_TYPE_MASK) == OBX_HI_TYPE_BYTE) && (p_pkt->layer_specific >= size) )
        {
            *p++        = id;
            *p++        = data;

            p_pkt->len  += size;
            p_pkt->layer_specific   -= size;
            status = TRUE;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function     OBX_Add4ByteHdr
**
** Description  This function is called to add a header with type as UINT32
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_Add4ByteHdr(BT_HDR *p_pkt, UINT8 id, UINT32 data)
{
    UINT8   *p;
    BOOLEAN status = FALSE;
    UINT16  size = 5;    /* total length added by this header - 1/hi & 4/hv */

    if(p_pkt)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
        /* verify that the HI is of correct type and the remaining length in the packet is good */
        if( ((id & OBX_HI_TYPE_MASK) == OBX_HI_TYPE_INT) && (p_pkt->layer_specific >= size) )
        {
            *p++        = id;
            UINT32_TO_BE_STREAM(p, data);

            p_pkt->len  += size;
            p_pkt->layer_specific   -= size;
            status = TRUE;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function     OBX_AddByteStrStart
**
** Description  This function is called to get the address to the beginning of
**              the byte sequence for an OBEX header in an OBEX packet.
**
** Returns      The address to add the byte sequence.
**
*******************************************************************************/
UINT8 *OBX_AddByteStrStart(BT_HDR *p_pkt, UINT16 *p_len)
{
    UINT8 *p = (UINT8 *)(p_pkt + 1) + p_pkt->offset + p_pkt->len + 3;

    WC_ASSERT(p_len);

    if(*p_len > (p_pkt->layer_specific - 3) || *p_len == 0)
        *p_len = p_pkt->layer_specific - 3;
    return p;
}

/*******************************************************************************
**
** Function     OBX_AddByteStrHdr
**
** Description  This function is called to add a header with type as byte sequence
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddByteStrHdr(BT_HDR *p_pkt, UINT8 id, UINT8 *p_data, UINT16 len)
{
    UINT8   *p;
    BOOLEAN status = FALSE;
    UINT16  size = (len+3); /* total length added by this header - 1/hi & len+2/hv */

    if(p_pkt)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
        /* verify that the HI is of correct type and the remaining length in the packet is good */
        if( ((id & OBX_HI_TYPE_MASK) == OBX_HI_TYPE_ARRAY) && (p_pkt->layer_specific >= size) )
        {
            *p++        = id;
            UINT16_TO_BE_STREAM(p, size);
            if(p_data)
                memcpy(p, p_data, len);

            p_pkt->len  += size;
            p_pkt->layer_specific   -= size;
            status = TRUE;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function     OBX_AddUnicodeHdr
**
** Description  This function is called to add a header with type as Unicode string
**              to an OBEX packet.
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddUnicodeHdr(BT_HDR *p_pkt, UINT8 id, UINT16 *p_data, UINT16 len)
{
    UINT8   *p;
    BOOLEAN status = FALSE;
    UINT16  size, xx;

    if(p_pkt)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
        size = (len*OBX_UNICODE_SIZE + 3); /* total length added by this header - 1/hi & len*OBX_UNICODE_SIZE+2/hv */
        OBX_TRACE_DEBUG4( "OBX_AddUnicodeHdr len: %d, size: %d, left: %d, id: 0x%x",
            len, size, p_pkt->layer_specific, id );

        /* verify that the HI is of correct type and the remaining length in the packet is good */
        if( ((id & OBX_HI_TYPE_MASK) == OBX_HI_TYPE_UNIC) && (p_pkt->layer_specific >= size) )
        {
            *p++        = id;
            UINT16_TO_BE_STREAM(p, size);
            for(xx=0; xx<len; xx++)
            {
                UINT16_TO_BE_STREAM(p, *p_data);
                p_data++;
            }

            p_pkt->len  += size;
            p_pkt->layer_specific   -= size;
            status = TRUE;
        }
    }

    return status;
}

/* Alternate Body header functions: for non-blocking scenario */
/*******************************************************************************
**
** Function     OBX_AddBodyStart
**
** Description  This function is called to get the address to the beginning of
**              the byte sequence for an OBEX body header in an OBEX packet.
**
** Returns      The address to add body content.
**
*******************************************************************************/
UINT8 *OBX_AddBodyStart(BT_HDR *p_pkt, UINT16 *p_len)
{
    UINT8 *p = (UINT8 *)(p_pkt + 1) + p_pkt->offset + p_pkt->len + 3;

    WC_ASSERT(p_len);

    if(*p_len > (p_pkt->layer_specific - 3) || *p_len == 0)
        *p_len = p_pkt->layer_specific - 3;
    return p;
}

/*******************************************************************************
**
** Function     OBX_AddBodyEnd
**
** Description  This function is called to add the HI and the length of HV of an
**              OBEX body header to an OBEX packet. If end is TRUE, HI is
**              OBX_HI_BODY_END. If FALSE, HI is OBX_HI_BODY. It is assumed that
**              the actual value of the body has been copied into the OBEX packet. 
**
** Returns      void
**
*******************************************************************************/
void OBX_AddBodyEnd(BT_HDR *p_pkt, UINT8 *p, UINT16 len, BOOLEAN end)
{
    UINT8 id = (end)?OBX_HI_BODY_END:OBX_HI_BODY;
    UINT8 *pb = (UINT8 *)(p_pkt + 1) + p_pkt->offset + p_pkt->len;
    *pb++   = id;
    len     += 3; /* 1/hi, 2/hv_len */
    UINT16_TO_BE_STREAM(pb, len);
    p_pkt->layer_specific   -= len;
    p_pkt->len              += len;
}

/*******************************************************************************
**
** Function     OBX_AddTriplet
**
** Description  This function is called to add a header with type as byte sequence
**              to an OBEX packet.
**
** Note:        The byte sequence uses a Tag-Length-Value encoding scheme
**              These headers include: Application Parameters header
**                                     Authenticate Challenge header
**                                     Authenticate Response header
**
** Returns      TRUE, if the header is added successfully.
**              FALSE, if the operation failed. p_pkt is not altered.
**
*******************************************************************************/
BOOLEAN OBX_AddTriplet(BT_HDR *p_pkt, UINT8 id, tOBX_TRIPLET *p_triplet, UINT8 num)
{
    UINT8   *p = (UINT8 *)(p_pkt+1)+p_pkt->offset+p_pkt->len;
    BOOLEAN status = FALSE;
    UINT16  size = 3;/* 1/hi & len+2/hv */
    UINT8   xx;

    /* calculate the total length added by this header */
    for(xx=0; xx< num; xx++)
        size += (p_triplet[xx].len + 2);

    /* verify that the HI is of correct type and the remaining length in the packet is good */
    if( ((id & OBX_HI_TYPE_MASK) == OBX_HI_TYPE_ARRAY) && (p_pkt->layer_specific >= size) )
    {
        *p++    = id;
        UINT16_TO_BE_STREAM(p, size);
        for(xx=0; xx< num; xx++)
        {
            *p++    = p_triplet[xx].tag;
            *p++    = p_triplet[xx].len;
            memcpy(p, p_triplet[xx].p_array, p_triplet[xx].len);
            p       += p_triplet[xx].len;
        }
        p_pkt->len  += size;
        p_pkt->layer_specific   -= size;
        status = TRUE;
    }

    return status;
}


/*******************************************************************************
**
** Function     OBX_CheckNext
**
** Description  This function is called to check if the given OBEX packet
**              contains the specified header.
**
** Returns      NULL, if the header is not in the OBEX packet.
**              The pointer to the specified header beginning from HI.
**
*******************************************************************************/
UINT8 * OBX_CheckNext(BT_HDR *p_pkt, UINT8 *p_start, UINT8 id)
{
    UINT8   *p;
    UINT8   *p_res = NULL;
    UINT16  len, start, skip;
    int     remain;

    if(p_pkt != NULL && p_start != NULL)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset;
        if(p_pkt->event <= OBX_MAX_OFFSET_IND)
        {
            start = obx_hdr_start_offset[p_pkt->event-1];
            p++;
            BE_STREAM_TO_UINT16(len, p);
            remain  = len - start;
            p   = p - 3 + start;

            while(remain >0)
            {
                if(*p != id || p < p_start)
                {
                    skip = obx_read_header_len(p);
                    p       += skip;
                    /* Just in case this is a bad packet, make sure that remain is >= 0 */
                    if(skip && (remain > skip))
                        remain  -= skip;
                    else
                        remain = 0;
                }
                else
                {
                    p_res = p;
                    break;
                }
            }
        }
    }
    
    if (p_pkt)
    {
        OBX_TRACE_DEBUG2( "OBX_CheckNext: remain: %d len:%d", remain, p_pkt->len);
    }

    return p_res;
}


/*******************************************************************************
**
** Function     OBX_CheckHdr
**
** Description  This function is called to check if the given OBEX packet
**              contains the specified header.
**
** Returns      NULL, if the header is not in the OBEX packet.
**              The pointer to the specified header beginning from HI.
**
*******************************************************************************/
UINT8 * OBX_CheckHdr(BT_HDR *p_pkt, UINT8 id)
{ 
    UINT8   *p;
    UINT8   *p_res = NULL;
    UINT16  len, start, skip;
    int     remain;

    if(p_pkt != NULL)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset;
        if(p_pkt->event <= OBX_MAX_OFFSET_IND)
        {
            start = obx_hdr_start_offset[p_pkt->event-1];
            p++;
            BE_STREAM_TO_UINT16(len, p);
            remain  = len - start;
            p   = p - 3 + start;

            while(remain >0)
            {
                if(*p != id)
                {
                    skip = obx_read_header_len(p);
                    p       += skip;
                    /* Just in case this is a bad packet, make sure that remain is >= 0 */
                    if(skip && (remain > skip))
                        remain  -= skip;
                    else
                        remain = 0;
                }
                else
                {
                    p_res = p;
                    break;
                }
            }
        }
    }

    return p_res;
}

/*******************************************************************************
**
** Function     OBX_ReadNumHdrs
**
** Description  This function is called to check the number of headers in the 
**              given OBEX packet
**
** Returns      number of headers.
**
*******************************************************************************/
UINT8 OBX_ReadNumHdrs(BT_HDR *p_pkt, UINT8 *p_num_body)
{
    UINT8   num_hdrs = 0, num_body = 0;
    UINT8   *p;
    UINT16  len, start, skip;
    int     remain = 0;

    if(p_pkt != NULL)
    {
        p = (UINT8 *)(p_pkt+1)+p_pkt->offset;
        if(p_pkt->event == 0)
        {
            /* GKI buffer just went through OBX_HdrInit; not processed by the state machine yet */
            remain = len = p_pkt->len;
        }
        else if(p_pkt->event <= OBX_MAX_OFFSET_IND)
        {
            start = obx_hdr_start_offset[p_pkt->event-1];
            p++;
            BE_STREAM_TO_UINT16(len, p);
            remain  = len - start;
            p   = p - 3 + start;
        }

        while(remain >0)
        {
            num_hdrs++;
            if(*p == OBX_HI_BODY || *p == OBX_HI_BODY_END)
                num_body++;

            skip = obx_read_header_len(p);
            p       += skip;
            /* Just in case this is a bad packet, make sure that remain is >= 0 */
            if(skip && (remain > skip))
                remain  -= skip;
            else
                remain = 0;

        }
    }
    if (p_num_body)
        *p_num_body = num_body;
    return num_hdrs;
}

/*******************************************************************************
**
** Function     OBX_ReadHdrLen
**
** Description  This function is called to check the length of the specified
**              header in the given OBEX packet.
**
** Returns      OBX_INVALID_HDR_LEN, if the header is not in the OBEX packet.
**              Otherwise the actual length of the header.
**
*******************************************************************************/
UINT16 OBX_ReadHdrLen(BT_HDR *p_pkt, UINT8 id)
{
    UINT8   *p;
    UINT16  len = OBX_INVALID_HDR_LEN;
 
    if( (p = OBX_CheckHdr(p_pkt, id)) != NULL)
        len = obx_read_header_len(p);
 
    return len;
}

/*******************************************************************************
**
** Function     OBX_Read1ByteHdr
**
** Description  This function is called to get the UINT8 HV of the given HI 
**              in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_Read1ByteHdr(BT_HDR *p_pkt, UINT8 id, UINT8 *p_data)
{
    BOOLEAN status = FALSE;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, id);

    if(p_start)
    {
        *p_data = *(++p_start);
        status = TRUE;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_Read4ByteHdr
**
** Description  This function is called to get the UINT32 HV of the given HI 
**              in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_Read4ByteHdr(BT_HDR *p_pkt, UINT8 id, UINT32 *p_data)
{
    BOOLEAN status = FALSE;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, id);

    if(p_start)
    {
        p_start++;
        BE_STREAM_TO_UINT32(*p_data, p_start);
        status = TRUE;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadByteStrHdr
**
** Description  This function is called to get the byte sequence HV of the given
**              HI in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadByteStrHdr(BT_HDR *p_pkt, UINT8 id, UINT8 **p_data, UINT16 *p_len, UINT8 next)
{
    BOOLEAN status = FALSE;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, id);

    if(p_start)
    {
        next += 1;
        while(next && (id == *p_start++))
        {
            next--;
            BE_STREAM_TO_UINT16(*p_len, p_start);
            if(next == 0)
            {
                status = TRUE;
                *p_len -= 3;    /* get rid of hi and hv_len */
                *p_data = p_start;
            }
            else
                p_start = p_start + *p_len - 3;
        }
    }
    else
    {
        *p_len = 0;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadUnicodeHdr
**
** Description  This function is called to get the Unicode HV of the given
**              HI in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadUnicodeHdr(BT_HDR *p_pkt, UINT8 id, UINT16 *p_data, UINT16 *p_len)
{
    BOOLEAN status = FALSE;
    UINT16  len, xx, max_len;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, id);

    if(p_start)
    {
        max_len = *p_len;
        p_start++; /* 1/hi*/
        BE_STREAM_TO_UINT16(len, p_start);
        len -= 3; /* 1/hi, 2/hv_len */
        len /= OBX_UNICODE_SIZE; /* size in UINT16 */
        /* only conver the provided size */
        if( len > max_len)
            len = max_len;
        for(xx=0; xx<len; xx++)
        {
            BE_STREAM_TO_UINT16(*p_data, p_start);
            p_data++;
        }
        *p_len = len;
        status = TRUE;
        max_len -= len;
        while ( (p_start = OBX_CheckNext(p_pkt, p_start, id)) != NULL && (max_len > 0))
        {
            p_start++; /* 1/hi*/
            BE_STREAM_TO_UINT16(len, p_start);
            len -= 3; /* 1/hi, 2/hv_len */
            len /= OBX_UNICODE_SIZE; /* size in UINT16 */
            /* only conver the provided size */
            if( len > max_len)
                len = max_len;
            for(xx=0; xx<len; xx++)
            {
                BE_STREAM_TO_UINT16(*p_data, p_start);
                p_data++;
            }
            *p_len += len;
            max_len -= len;
        }
    }
    else
    {
        *p_len = 0;
    }
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadTriplet
**
** Description  This function is called to get the Triplet HV of the given
**              HI in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadTriplet(BT_HDR *p_pkt, UINT8 id, tOBX_TRIPLET *p_triplet, UINT8 *p_num)
{
    BOOLEAN status = FALSE;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, id);
    UINT16  len;
    UINT8   count = 0;

    if(p_start)
    {
        p_start++; /* 1/hi*/
        BE_STREAM_TO_UINT16(len, p_start);
        len -= 3;   /* 1/hi, 2/hv_len */
        while(len && *p_num > count)
        {
            p_triplet[count].tag = *p_start++;
            p_triplet[count].len = *p_start++;
            OBX_TRACE_DEBUG3( "OBX_ReadTriplet: count: %d, tag: %x, len: %d",
                count, p_triplet[count].tag, p_triplet[count].len);
            p_triplet[count].p_array = p_start;
            p_start += p_triplet[count].len;
            if(len > (p_triplet[count].len + 2) )
                len -= (p_triplet[count].len + 2);
            else
                len = 0;
            count++;
        }
        status = TRUE;
    }
    *p_num = count;
    return status;
}

/*******************************************************************************
**
** Function     OBX_ReadActionIdHdr
**
** Description  This function is called to get the HV of the Action ID header 
**              in the given OBEX packet.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadActionIdHdr(BT_HDR *p_pkt, UINT8 *p_data)
{
    BOOLEAN status = FALSE;
    UINT8   *p_start = OBX_CheckHdr(p_pkt, OBX_HI_ACTION_ID);

    if(p_start)
    {
        p_start++;
        /* check for valid values: 0-2 */
        /* do not allow 0x80 - 0xFF (vendor extention) for now. */
        if (*p_start <= OBX_ACT_PERMISSION)
        {
            *p_data = *(p_start);
            status = TRUE;
        }
    }
    return status;
}
