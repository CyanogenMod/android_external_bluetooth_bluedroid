/*****************************************************************************
**
**  Name:         obx_md5.c
**
**  File:         OBEX Authentication related functions
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "bt_target.h"
#include "btu.h"
#include "obx_int.h"
#include "wcassert.h"



/*
* This code implements the MD5 message-digest algorithm.
* The algorithm is due to Ron Rivest. This code was
* written by Colin Plumb in 1993, no copyright is claimed.
* This code is in the public domain; do with it what you wish.
*
* Equivalent code is available from RSA Data Security, Inc.
* This code has been tested against that, and is equivalent,
* except that you don't need to include two pages of legalese
* with every copy.
*
* To compute the message digest of a chunk of bytes, declare an
* MD5Context structure, pass it to MD5Init, call MD5Update as
* needed on buffers full of bytes, and then call MD5Final, which
* will fill a supplied 16-byte array with the digest.
*/
typedef UINT32 word32;
typedef UINT8  byte;
struct xMD5Context {
    word32 buf[4];
    word32 bytes[2];
    word32 in[16];
};

static void obx_md5_transform(word32 buf[4], word32 const in[16]);

/*
* Shuffle the bytes into little-endian order within words, as per the
* MD5 spec. Note: this code works regardless of the byte order.
*/
static void obx_byte_swap(word32 *buf, unsigned words)
{
    byte *p = (byte *)buf;
    do {
        *buf++ = (word32)((unsigned)p[3] << 8 | p[2]) << 16
               | (word32)((unsigned)p[1] << 8 | p[0]);
        p += 4;
    } while (--words);
}

/*
* Start MD5 accumulation. Set bit count to 0 and buffer to mysterious
* initialization constants.
*/
static void obx_md5_init(struct xMD5Context *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;
    ctx->bytes[0] = 0;
    ctx->bytes[1] = 0;
}

/*
* Update context to reflect the concatenation of another buffer full
* of bytes.
*/
static void obx_md5_update(struct xMD5Context *ctx, byte const *buf, int len)
{
    word32 t;
    /* Update byte count */
    t = ctx->bytes[0];
    if ((ctx->bytes[0] = t + len) < t)
        ctx->bytes[1]++; /* Carry from low to high */
    t = 64 - (t & 0x3f); /* Space avail in ctx->in (at least 1) */
    if ((unsigned)t > (unsigned)len)
    {
        memcpy ((byte *)ctx->in + 64 - (unsigned)t, buf, len);
        return;
    }
    /* First chunk is an odd size */
    memcpy (ctx->in + (64 - (unsigned)t), buf, (unsigned)t);
    obx_byte_swap(ctx->in, 16);
    obx_md5_transform(ctx->buf, ctx->in);
    buf += (unsigned)t;
    len -= (unsigned)t;
    /* Process data in 64-byte chunks */
    while (len >= 64)
    {
        /* coverity[access_dbuff_const] */
        memcpy (ctx->in, buf, 64);
        obx_byte_swap(ctx->in, 16);
        obx_md5_transform(ctx->buf, ctx->in);
        buf += 64;
        len -= 64;
    }
    /* Handle any remaining bytes of data. */
    memcpy (ctx->in, buf, len);
}

/*
* Final wrapup - pad to 64-byte boundary with the bit pattern
* 1 0* (64-bit count of bits processed, MSB-first)
*/
static void obx_md5_final(byte digest[16], struct xMD5Context *ctx)
{
    int count = (int)(ctx->bytes[0] & 0x3f); /* Bytes in ctx->in */
    byte *p = (byte *)ctx->in + count; /* First unused byte */
    /* Set the first char of padding to 0x80. There is always room.*/
    *p++ = 0x80;
    /* Bytes of padding needed to make 56 bytes (-8..55) */
    count = 56 - 1 - count;
    if (count < 0) /* Padding forces an extra block */
    {
        memset (p, 0, count+8);
        obx_byte_swap(ctx->in, 16);
        obx_md5_transform(ctx->buf, ctx->in);
        p = (byte *)ctx->in;
        count = 56;
    }
    memset (p, 0, count+8);
    obx_byte_swap(ctx->in, 14);
    /* Append length in bits and transform */
    ctx->in[14] = ctx->bytes[0] << 3;
    ctx->in[15] = ctx->bytes[1] << 3 | ctx->bytes[0] >> 29;
    obx_md5_transform(ctx->buf, ctx->in);
    obx_byte_swap(ctx->buf, 4);
    memcpy (digest, ctx->buf, 16);
    memset (ctx, 0, sizeof(ctx));
}

/* The four core functions - F1 is optimized somewhat */
/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
(w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/*
* The core of the MD5 algorithm, this alters an existing MD5 hash to
* reflect the addition of 16 longwords of new data. MD5Update blocks
* the data and converts bytes into longwords for this routine.
*/
static const word32 obx_md5_f1 [] = 
{
    0xd76aa478,
    0xe8c7b756,
    0x242070db,
    0xc1bdceee,

    0xf57c0faf,
    0x4787c62a,
    0xa8304613,
    0xfd469501,

    0x698098d8,
    0x8b44f7af,
    0xffff5bb1,
    0x895cd7be,

    0x6b901122,
    0xfd987193,
    0xa679438e,
    0x49b40821
};

static const word32 obx_md5_f2 [] = 
{
    0xf61e2562,
    0xc040b340,
    0x265e5a51,
    0xe9b6c7aa,

    0xd62f105d,
    0x02441453,
    0xd8a1e681,
    0xe7d3fbc8,

    0x21e1cde6,
    0xc33707d6,
    0xf4d50d87,
    0x455a14ed,

    0xa9e3e905,
    0xfcefa3f8,
    0x676f02d9,
    0x8d2a4c8a
};

static const word32 obx_md5_f3 [] = 
{
    0xfffa3942,
    0x8771f681,
    0x6d9d6122,
    0xfde5380c,

    0xa4beea44,
    0x4bdecfa9,
    0xf6bb4b60,
    0xbebfbc70,

    0x289b7ec6,
    0xeaa127fa,
    0xd4ef3085,
    0x04881d05,

    0xd9d4d039,
    0xe6db99e5,
    0x1fa27cf8,
    0xc4ac5665
};

static const word32 obx_md5_f4 [] = 
{
    0xf4292244,
    0x432aff97,
    0xab9423a7,
    0xfc93a039,

    0x655b59c3,
    0x8f0ccc92,
    0xffeff47d,
    0x85845dd1,

    0x6fa87e4f,
    0xfe2ce6e0,
    0xa3014314,
    0x4e0811a1,

    0xf7537e82,
    0xbd3af235,
    0x2ad7d2bb,
    0xeb86d391
};

static const UINT8 obx_md5_a [] = 
{
    1,
    2,
    3,
    0,
    1,
    2,
    3
};

static const word32 obx_md5_var1 [] =
{
    7,
    12,
    17,
    22
};

static const word32 obx_md5_var2 [] =
{
    5,
    9,
    14,
    20
};

static const word32 obx_md5_var3 [] =
{
    4,
    11,
    16,
    23
};

static const word32 obx_md5_var4 [] =
{
    6,
    10,
    15,
    21
};

static void obx_md5_transform(word32 buf[4], word32 const in[16])
{
    int     xx, yy, zz, i, j, k;
    word32  a[4];

    a[0] = buf[0];
    a[1] = buf[1];
    a[2] = buf[2];
    a[3] = buf[3];

    yy = 0;
    for(xx=0; xx<4; xx++)
    {
        j  = 3;
        for(i=0; i < 4; i++)
        {
            k = j--;
            /*
            OBX_TRACE_DEBUG4( "f1 a: %d, yy: %d, inc: 0x%x, var: %d",
                obx_md5_a[k], yy, obx_md5_f1[yy], obx_md5_var1[i]);
             */
            MD5STEP(F1, a[obx_md5_a[k]], a[obx_md5_a[k+1]], a[obx_md5_a[k+2]], a[obx_md5_a[k+3]],
                    in[yy] + obx_md5_f1[yy], obx_md5_var1[i]);
            yy++;
        }
    }

    yy = 1;
    zz = 0;
    for(xx=0; xx<4; xx++)
    {
        j  = 3;
        for(i=0; i < 4; i++)
        {
            k = j--;
            /*
            OBX_TRACE_DEBUG4( "f2 a: %d, yy: %d, inc: 0x%x, var: %d",
                obx_md5_a[k], yy, obx_md5_f2[zz], obx_md5_var2[i]);
             */
            MD5STEP(F2, a[obx_md5_a[k]], a[obx_md5_a[k+1]], a[obx_md5_a[k+2]], a[obx_md5_a[k+3]],
                    in[yy] + obx_md5_f2[zz++], obx_md5_var2[i]);
            yy += 5;
            yy %= 16;
        }
    }

    yy = 5;
    zz = 0;
    for(xx=0; xx<4; xx++)
    {
        j  = 3;
        for(i=0; i < 4; i++)
        {
            k = j--;
            /*
            OBX_TRACE_DEBUG4( "f3 a: %d, yy: %d, inc: 0x%x, var: %d",
                obx_md5_a[k], yy, obx_md5_f3[zz], obx_md5_var3[i]);
             */
            MD5STEP(F3, a[obx_md5_a[k]], a[obx_md5_a[k+1]], a[obx_md5_a[k+2]], a[obx_md5_a[k+3]],
                    in[yy] + obx_md5_f3[zz++], obx_md5_var3[i]);
            yy += 3;
            yy %= 16;
        }
    }


    yy = 0;
    zz = 0;
    for(xx=0; xx<4; xx++)
    {
        j  = 3;
        for(i=0; i < 4; i++)
        {
            k = j--;
            /*
            OBX_TRACE_DEBUG4( "f4 a: %d, yy: %d, inc: 0x%x, var: %d",
                obx_md5_a[k], yy, obx_md5_f4[zz], obx_md5_var4[i]);
             */
            MD5STEP(F4, a[obx_md5_a[k]], a[obx_md5_a[k+1]], a[obx_md5_a[k+2]], a[obx_md5_a[k+3]],
                    in[yy] + obx_md5_f4[zz++], obx_md5_var4[i]);
            yy += 7;
            yy %= 16;
        }
    }

    buf[0] += a[0];
    buf[1] += a[1];
    buf[2] += a[2];
    buf[3] += a[3];
}

/*******************************************************************************
**
** Function     OBX_MD5
**
** Description  This function is called to run the MD5 algorithm.
**
** Returns      void
**
*******************************************************************************/
static void OBX_MD5(void *digest, UINT8 *nonce, UINT8 * password, int password_len)
{
    struct xMD5Context context;
    UINT8  before[OBX_NONCE_SIZE + OBX_MAX_AUTH_KEY_SIZE + 4];

    memcpy(before, nonce, OBX_NONCE_SIZE);
    before[OBX_NONCE_SIZE] = ':';
    memcpy(before + OBX_NONCE_SIZE + 1, password, password_len);
    /*
    scru_dump_hex (before, "before", OBX_NONCE_SIZE + 1 + password_len, TRACE_LAYER_OBEX, TRACE_TYPE_GENERIC);
    */

    obx_md5_init(&context);
/* coverity[overrun-buffer-val] */
/*
FALSE-POSITIVE: coverity says "Overrun of static array "before" of size 36 bytes by passing it as an argument to a function which indexes it at byte position 63"
obx_md5_update() only goes into that condition when (len >= 64). In this case, len is (OBX_NONCE_SIZE + 1 + password_len) which is less than or equal to 33.
password_len is less than or equal to OBX_MAX_AUTH_KEY_SIZE. The size of before[] is more than enough */
    obx_md5_update(&context, (byte const *)before, OBX_NONCE_SIZE + 1 + password_len);
    obx_md5_final((byte *)digest, &context);
    /*
    scru_dump_hex (digest, "after", 16, TRACE_LAYER_OBEX, TRACE_TYPE_GENERIC);
    */
}

/*******************************************************************************
**
** Function     obx_session_id
**
** Description  This function is called to run the MD5 algorithm to create a
**              session id.
**
** Returns      void
**
*******************************************************************************/
void obx_session_id(UINT8 *p_sess_id, UINT8 *p_cl_addr, UINT8 * p_cl_nonce, int cl_nonce_len,
                    UINT8 *p_sr_addr, UINT8 * p_sr_nonce, int sr_nonce_len)
{
    struct xMD5Context context;
    UINT8  before[(OBX_NONCE_SIZE + BD_ADDR_LEN) * 2 + 4];
    UINT8  *p = before;
    UINT8  len;

    memcpy(p, p_cl_addr, BD_ADDR_LEN);
    p += BD_ADDR_LEN;
    memcpy(p, p_cl_nonce, cl_nonce_len);
    p += cl_nonce_len;
    memcpy(p, p_sr_addr, BD_ADDR_LEN);
    p += BD_ADDR_LEN;
    memcpy(p, p_sr_nonce, sr_nonce_len);
    p += sr_nonce_len;
    /*
    scru_dump_hex (before, "before", OBX_NONCE_SIZE + 1 + password_len, TRACE_LAYER_OBEX, TRACE_TYPE_GENERIC);
    */

    len = BD_ADDR_LEN + cl_nonce_len + BD_ADDR_LEN + sr_nonce_len;
    obx_md5_init(&context);
/* coverity [overrun-buffer-val] */ 
/* Overrun of static array "before" of size 48 bytes by passing it as an argument to a function which indexes it at byte position 63*/
/* FALSE-POSITIVE: obx_md5_update() only goes into that condition when (len >= 64). In this case, len is (OBX_NONCE_SIZE + 1 + password_len) which is less than or equal to 33.
password_len is less than or equal to OBX_MAX_AUTH_KEY_SIZE. The size of before[] is more than enough */
    obx_md5_update(&context, (byte const *)before, len);
    obx_md5_final((byte *)p_sess_id, &context);
    /*
    scru_dump_hex (p_sess_id, "after", 16, TRACE_LAYER_OBEX, TRACE_TYPE_GENERIC);
    */
}

#if (OBX_SERVER_INCLUDED == TRUE)
/*******************************************************************************
**
** Function     obx_copy_and_rm_auth_hdrs
**
** Description  This function is used to copy the given OBEX packet to a new packet
**              with the authentication headers removed.
**
** Returns      tOBX_STATUS
** 
*******************************************************************************/
tOBX_STATUS obx_copy_and_rm_auth_hdrs(BT_HDR *p_pkt, BT_HDR *p_new, UINT8 * p_hi)
{
    tOBX_STATUS status = OBX_SUCCESS;
    UINT8   *pn, *po, *p_nlen = NULL;
    int     hsize, nsize;
    UINT16  nlen, olen;
    UINT8   size;
    int     xx;

    WC_ASSERT(p_new);


    po  = (UINT8 *)(p_pkt + 1) + p_pkt->offset + 1;
    BE_STREAM_TO_UINT16(olen, po);
    OBX_TRACE_DEBUG3( "obx_copy_and_rm_auth_hdrs event: %d, new len:%d, olen:%d",
        p_pkt->event, p_new->len, olen);

    po  = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    if(p_new->len == 0)
    {
        p_new->offset   = p_pkt->offset;
        p_nlen          = (UINT8 *)(p_new + 1) + p_new->offset + 1;
    }
    pn  = (UINT8 *)(p_new + 1) + p_new->offset;

    /* Get the number of bytes before headers start */
    size = obx_hdr_start_offset[p_pkt->event - 1];
    if(p_new->len)
    {
        /* for OBX_AuthResponse
         * - do not need to copy the stuff before headers
         * there's already some header, skip them */
        pn  += p_new->len;
        nlen = p_new->len;
    }
    else
    {
        /* for obx_unauthorize_rsp
         * - needs to copy the stuff before headers */
        memcpy(pn, po, size);
        pn  += size;
        nlen = size;
    }
    po  += size;

    olen    -= size;
    while(olen)
    {
        OBX_TRACE_DEBUG1( "olen:%d", olen);
        hsize = (int)obx_read_header_len(po);
        if(olen > hsize)
            olen -= hsize;
        else
            olen = 0;

        xx  = 0;
        nsize = hsize;
        while(p_hi[xx] != OBX_HI_NULL)
        {
            /*
            OBX_TRACE_DEBUG2( "po:0x%x, hix:0x%x", *po, p_hi[xx]);
            */
            if(*po == p_hi[xx++])
            {
                nsize = 0;
                break;
            }
        }
        OBX_TRACE_DEBUG2( "hsize:%d, nsize:%d", hsize, nsize);
        if(nsize)
        {
            /* skip auth challenge and auth response */
            if((nlen+hsize) < p_pkt->layer_specific)
            {
                /* copy other headers */
                memcpy(pn, po, hsize);
                pn      += hsize;
                nlen    += hsize;
            }
            else
            {
                OBX_TRACE_WARNING1( "obx_copy_and_rm_auth_hdrs Not enough room: %d", olen);
                /* no more room in the new packet */
                status = OBX_NO_RESOURCES;
                break;
            }
        }
        po += hsize;
    }

    if(status == OBX_SUCCESS)
    {
        if(p_nlen)
        {
            UINT16_TO_BE_STREAM(p_nlen, nlen);
        }
        p_new->len  = nlen;
        p_new->layer_specific   = GKI_get_buf_size(p_new) - BT_HDR_SIZE - p_new->offset;
        p_new->event            = p_pkt->event;
    }

    return status;
}

/*******************************************************************************
**
** Function     obx_unauthorize_rsp
**
** Description  This function is used to add authentication challenge triplet
**
** Returns      void.
** Note:        this function assumes that all data can fit in the MTU
*******************************************************************************/
BT_HDR * obx_unauthorize_rsp(tOBX_SR_CB *p_cb, tOBX_SR_SESS_CB *p_scb, BT_HDR *p_pkt)
{
    UINT8   *p;
    UINT8   nonce[OBX_NONCE_SIZE+2];
    UINT8   option[2];
    UINT8   *p_target = NULL;
    tOBX_TRIPLET    triplet[OBX_MAX_NUM_AUTH_TRIPLET];
    UINT8           num_trip    = 0;
    BT_HDR          *p_old;
    UINT8           hi[] = {OBX_HI_CHALLENGE, OBX_HI_AUTH_RSP, OBX_HI_NULL};

    OBX_TRACE_DEBUG1( "obx_unauthorize_rsp: %d", p_cb->target.len);
    if(OBX_CheckHdr(p_pkt, OBX_HI_CHALLENGE) || OBX_CheckHdr(p_pkt, OBX_HI_AUTH_RSP))
    {
        p_old   = p_pkt;
        p_pkt   = OBX_HdrInit(p_scb->ll_cb.comm.handle, (UINT16)(p_old->len + BT_HDR_SIZE + p_old->offset));
        obx_copy_and_rm_auth_hdrs(p_old, p_pkt, hi);
        GKI_freebuf(p_old);
    }

    if(p_cb->target.len)
    {
        /* There must be the target header, change it to be WHO header */
        if( (p_target = OBX_CheckHdr(p_pkt, OBX_HI_TARGET)) != NULL)
        {
            *p_target = OBX_HI_WHO;
        }
    }
    OBX_TRACE_DEBUG1( "p_target: %x", p_target);

    if(p_cb->p_auth)
    {
        /* server registers for authentication */
        /* prepare the triplet byte sequence */
        /* add nonce: tag, length, value */
        p_cb->p_auth->nonce = GKI_get_tick_count();
        sprintf ((char *)nonce, "%016lu", p_cb->p_auth->nonce);

        num_trip    = 0;
        triplet[num_trip].tag       = OBX_NONCE_CHLNG_TAG;
        triplet[num_trip].len       = OBX_NONCE_SIZE;
        triplet[num_trip].p_array   = nonce;
        num_trip++;

        if(p_cb->p_auth->auth_option)
        {
            /* add option */
            triplet[num_trip].tag       = OBX_OPTIONS_CHLNG_TAG;
            triplet[num_trip].len       = 1;
            option[0]                   = p_cb->p_auth->auth_option;
            triplet[num_trip].p_array   = option;
            num_trip++;
        }
        if(p_cb->p_auth->realm_len)
        {
            /* add realm */
            triplet[num_trip].tag       = OBX_REALM_CHLNG_TAG;
            triplet[num_trip].len       = p_cb->p_auth->realm_len+1;
            triplet[num_trip].p_array   = p_cb->p_auth->realm;
            num_trip++;
        }

        /* add the sequence to header */
        OBX_AddTriplet(p_pkt, OBX_HI_CHALLENGE, triplet, num_trip);
    }

    p = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    /* change the opcode to unauthorized response */
    *p++    = OBX_RSP_UNAUTHORIZED | OBX_FINAL;
    /* adjust the packet len */
    UINT16_TO_BE_STREAM(p, p_pkt->len);

    /* add session sequence number, if session is active */
    if ((p_scb->sess_st == OBX_SESS_ACTIVE) && ((p_target = OBX_CheckHdr(p_pkt, OBX_HI_SESSION_SN)) != NULL))
    {
        p_target++;
        (*p_target)++;
    }

    p_pkt->event    = OBX_CONNECT_RSP_EVT;

    return p_pkt;
}

/*******************************************************************************
**
** Function     OBX_Password
**
** Description  This function is called by the server to respond to an 
**              OBX_PASSWORD_EVT event.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_Password(tOBX_HANDLE shandle, UINT8 *p_password, UINT8 password_len,
                                        UINT8 *p_userid, UINT8 userid_len)
{
    tOBX_STATUS status = OBX_SUCCESS;
    BT_HDR     *p_pkt;
    BT_HDR     *p_rsp;
    UINT8      temp_digest[OBX_DIGEST_SIZE];
    UINT8      nonce[OBX_NONCE_SIZE + 1];
    BOOLEAN    pass = FALSE;
    BOOLEAN    digest_done = FALSE;
    BOOLEAN    option_userid = FALSE;
    tOBX_TRIPLET    triplet[OBX_MAX_NUM_AUTH_TRIPLET];
    UINT8           num_trip = OBX_MAX_NUM_AUTH_TRIPLET;
    UINT8           xx;
    UINT8           *p_target;
    UINT16          tlen;
    tOBX_SR_SESS_CB *p_scb = obx_sr_get_scb(shandle);
    tOBX_SR_CB      *p_cb = obx_sr_get_cb(shandle);
    UINT8           *p;
    tOBX_EVT_PARAM  param;              /* The event parameter. */

    WC_ASSERT(password_len < OBX_MAX_AUTH_KEY_SIZE);

    if(p_scb == NULL || p_scb->p_saved_msg == NULL)
        return OBX_NO_RESOURCES;

     /* The coverity complaints on this function is not correct. 
      * The value in triplet[] is set/initialized by OBX_ReadTriplet if num_trip returns TRUE.
      * leave this unnecessary memset here */
    memset(triplet,0,sizeof(triplet));

    p_pkt = p_scb->p_saved_msg;

    if(p_cb->p_auth == NULL)
    {
        pass    = TRUE;
    }
    else if(OBX_ReadTriplet(p_pkt, OBX_HI_AUTH_RSP, triplet, &num_trip) == TRUE)
    {
        if(p_password && password_len )
        {
            /* if given password, verify it */
            for(xx=0; xx<num_trip; xx++)
            {
                if(triplet[xx].tag == OBX_DIGEST_RSP_TAG)
                {
                    sprintf((char *)nonce, "%016lu", p_cb->p_auth->nonce);
                    OBX_MD5 (temp_digest, nonce, p_password, password_len);
                    if (memcmp (temp_digest, triplet[xx].p_array, OBX_DIGEST_SIZE) == 0)
                        pass    = TRUE;
                    break;
                }
            }
        }
    }

    /* check if challenged by client */
    num_trip = OBX_MAX_NUM_AUTH_TRIPLET;
    if(pass == TRUE && OBX_ReadTriplet(p_pkt, OBX_HI_CHALLENGE, triplet, &num_trip) == TRUE)
    {
        /* Make sure password was passed in and not empty */
        if (p_password && password_len )
        {
            for(xx=0; xx<num_trip; xx++)
            {
                if(triplet[xx].tag == OBX_NONCE_CHLNG_TAG)
                {
                    /* nonce tag is mandatory */
                    OBX_MD5 (temp_digest, triplet[xx].p_array, p_password, password_len);
                    digest_done = TRUE;
                }
                else if(triplet[xx].tag == OBX_OPTIONS_CHLNG_TAG)
                {
                    /* user ID bit is set */
                    if(OBX_AO_USR_ID & triplet[xx].p_array[0])
                        option_userid   = TRUE;
                }
            }

            if (option_userid && (userid_len == 0 || p_userid == NULL) )
            {
                status  = OBX_BAD_PARAMS;
            }
        }
    }

    if(status == OBX_SUCCESS)
    {
        p_scb->p_saved_msg = NULL;
        if(pass == TRUE)
        {
            p_rsp   = OBX_HdrInit(shandle, OBX_MIN_MTU);

            /* add the authentication response if been challenged */
            if(digest_done)
            {
                num_trip = 1;
                triplet[0].tag      = OBX_DIGEST_RSP_TAG;
                triplet[0].len      = OBX_DIGEST_SIZE;
                triplet[0].p_array  = temp_digest;
                if(option_userid)
                {
                    num_trip++;
                    triplet[1].tag  = OBX_USERID_RSP_TAG;
                    triplet[1].len  = userid_len;
                    triplet[1].p_array  = p_userid;
                }
                OBX_AddTriplet(p_rsp, OBX_HI_AUTH_RSP, triplet, num_trip);
            }

            if( p_cb->target.len == OBX_DEFAULT_TARGET_LEN &&
                OBX_ReadTargetHdr(p_pkt, &p_target, &tlen, 0) == TRUE)
            {
                /* API user is supposed to handle WHO header
                 * for this special case, we add it for the API user */
                OBX_AddWhoHdr(p_rsp, p_target, tlen);
            }

            /* use OBX_ConnectRsp to fill the common data */
            status = OBX_ConnectRsp(shandle, OBX_RSP_OK, p_rsp);
            if(status == OBX_SUCCESS)
            {
                /* If authentication is successful, need to update session info */
                p = &p_scb->sess_info[OBX_SESSION_INFO_ID_IDX];
                UINT32_TO_BE_STREAM(p, p_scb->conn_id);
                param.sess.p_sess_info   = p_scb->sess_info;
                param.sess.sess_op       = OBX_SESS_OP_CREATE;
                param.sess.sess_st       = p_scb->sess_st;
                param.sess.nssn          = p_scb->param.ssn;
                param.sess.ssn           = p_scb->param.ssn;
                param.sess.obj_offset    = 0;
                p = &p_scb->sess_info[OBX_SESSION_INFO_MTU_IDX];
                UINT16_TO_BE_STREAM(p, p_scb->param.conn.mtu);
                memcpy(param.sess.peer_addr, p_scb->peer_addr, BD_ADDR_LEN);
                (*p_cb->p_cback)(p_scb->ll_cb.comm.handle, OBX_SESSION_REQ_EVT, param, p_pkt);
                (*p_cb->p_cback)(p_scb->ll_cb.comm.handle, OBX_CONNECT_REQ_EVT, p_scb->param, p_pkt);
            }
            else
                GKI_freebuf(p_pkt);
        }
        else
        {
            /* server can not find the password, send unauthorized response */
            p_pkt = obx_unauthorize_rsp(p_cb, p_scb, p_pkt);
            obx_ssm_event(p_scb, OBX_CONNECT_CFM_SEVT, p_pkt);
        }
    }
    /*
    else p_scb->p_saved_msg is still valid.
     */

    return status;
}
#endif


#if (OBX_CLIENT_INCLUDED == TRUE)
/*******************************************************************************
**
** Function     OBX_AuthResponse
**
** Description  This function is called by the client to respond to an 
**              OBX_PASSWORD_EVT event.
**
** Returns      OBX_SUCCESS, if successful.
**              OBX_NO_RESOURCES, if OBX does not resources
**
*******************************************************************************/
tOBX_STATUS OBX_AuthResponse(tOBX_HANDLE handle,
                            UINT8 *p_password, UINT8 password_len,
                            UINT8 *p_userid, UINT8 userid_len,
                            BOOLEAN authenticate)
{
    tOBX_CL_CB  *p_cb = obx_cl_get_cb(handle);
    tOBX_STATUS status = OBX_NO_RESOURCES;
    BT_HDR      *p_pkt;
    BT_HDR      *p_saved_req;
    UINT8       num_trip, flags = 0;
    UINT16      nlen, xx;
    UINT8       temp_digest[OBX_DIGEST_SIZE];
    UINT8       nonce[OBX_NONCE_SIZE + 1];
    BOOLEAN     digest_done = FALSE;
    BOOLEAN     option_userid = FALSE;
    UINT8       rsp_code;
    BOOLEAN     final;
    UINT8       *p_target;
    tOBX_TRIPLET    triplet[OBX_MAX_NUM_AUTH_TRIPLET];
    UINT8           hi[] = {OBX_HI_CONN_ID, OBX_HI_TARGET, OBX_HI_CHALLENGE, OBX_HI_AUTH_RSP,
                            OBX_HI_SESSION_SN, /* prevent SSN from being copied */
                            OBX_HI_NULL};
    WC_ASSERT(password_len < OBX_MAX_AUTH_KEY_SIZE);
    WC_ASSERT(p_password);

    if(p_cb == NULL || p_cb->p_auth == NULL || p_cb->p_saved_req == NULL)
        return OBX_NO_RESOURCES;

    p_saved_req     = p_cb->p_saved_req;
    p_pkt   = OBX_HdrInit(handle, GKI_MAX_BUF_SIZE);    /* make sure added length will fit */
    WC_ASSERT(p_pkt);
    OBX_TRACE_DEBUG2("OBX_AuthResponse p_saved_req:%d, p_saved_req->len %d ",
                     p_saved_req->event, p_saved_req->len);

    /* if the save_req contains SSN, add it now */
    if ((p_target = OBX_CheckHdr(p_saved_req, OBX_HI_SESSION_SN)) != NULL)
    {
        p_target++;
        OBX_Add1ByteHdr(p_pkt, OBX_HI_SESSION_SN, (UINT8)((*p_target) + 1));
    }

     /* The coverity complaints on this function is not correct. 
      * The value in triplet[] is set/initialized by OBX_ReadTriplet if num_trip returns TRUE.
      * leave this unnecessary memset here */
    memset(triplet,0,sizeof(triplet));
    
    /* if target header exists in the original request
     * and the Connection ID has not been assigned yet,
     * copy the target header over */
    if(OBX_ReadTargetHdr(p_saved_req, &p_target, &nlen, 0) == TRUE)
    {
        OBX_AddTargetHdr(p_pkt, p_target, nlen);
    }

    /* read the challenge from received packet  */
    num_trip = OBX_MAX_NUM_AUTH_TRIPLET;
    if(OBX_ReadTriplet(p_cb->p_auth, OBX_HI_CHALLENGE, triplet, &num_trip) == TRUE)
    {
        status = OBX_SUCCESS;
        for(xx=0; xx<num_trip; xx++)
        {
            if(triplet[xx].tag == OBX_NONCE_CHLNG_TAG)
            {
                /* nonce tag is mandatory */
                OBX_MD5 (temp_digest, triplet[xx].p_array, p_password, password_len);
                digest_done = TRUE;
            }
            else if(triplet[xx].tag == OBX_OPTIONS_CHLNG_TAG)
            {
                /* user ID bit is set */
                if(OBX_AO_USR_ID & triplet[xx].p_array[0])
                    option_userid   = TRUE;
            }
        }

        if(option_userid && (userid_len == 0 || p_userid == NULL) )
        {
            status  = OBX_BAD_PARAMS;
        }
        OBX_TRACE_DEBUG2( "num_trip:%d, option_userid:%d", num_trip, option_userid);

        if(digest_done && status == OBX_SUCCESS)
        {
            /* Compose and add the authentication Response header */
            num_trip = 1;
            triplet[0].tag      = OBX_DIGEST_RSP_TAG;
            triplet[0].len      = OBX_DIGEST_SIZE;
            triplet[0].p_array  = temp_digest;
            if(option_userid)
            {
                /* add user id */
                num_trip++;
                triplet[1].tag  = OBX_USERID_RSP_TAG;
                triplet[1].len  = userid_len;
                triplet[1].p_array  = p_userid;
            }
            OBX_AddTriplet(p_pkt, OBX_HI_AUTH_RSP, triplet, num_trip);

            /* if we want to authenticate the server, add the challenge header here */
            /* Note: we only do it for the CONNECT request */
            if(authenticate && p_saved_req->event == OBX_CONNECT_REQ_EVT)
            {
                /* Indicate in the control block to verify the response */
                p_cb->wait_auth     = TRUE;

                /* add the challenge nonce */
                num_trip = 1;
                triplet[0].tag      = OBX_NONCE_CHLNG_TAG;
                triplet[0].len      = OBX_NONCE_SIZE;
                triplet[0].p_array  = nonce;
                sprintf ((char *)nonce, "%016lu", GKI_get_tick_count());
                OBX_AddTriplet(p_pkt, OBX_HI_CHALLENGE, triplet, num_trip);
                OBX_MD5 ((UINT8 *)(p_cb->p_auth), nonce, p_password, password_len);
            }

            /* copy non-(target, conn_id, authentication) headers from p_saved_req. */
            status = obx_copy_and_rm_auth_hdrs(p_saved_req, p_pkt, hi);
            OBX_TRACE_DEBUG4( "status:%d, save:%d, pkt:%d, off:%d",
                status, p_saved_req->event, p_pkt->event, p_pkt->offset);

            if(status == OBX_SUCCESS)
            {
                /* get the final from the saved request */
                rsp_code = OBX_RSP_DEFAULT;
                obx_access_rsp_code(p_saved_req, &rsp_code);
                final = (rsp_code & OBX_FINAL) ? TRUE : FALSE;
                OBX_TRACE_DEBUG1( "saved final:%d", final);

                /* call the associated API function to send the request again */
                switch(p_pkt->event)
                {
                case OBX_CONNECT_REQ_EVT:
                    status = OBX_ConnectReq((BD_ADDR_PTR)BT_BD_ANY, 0, 0, NULL, &p_cb->ll_cb.comm.handle, p_pkt);
                    break;
                case OBX_PUT_REQ_EVT:
                    status = OBX_PutReq(handle, final, p_pkt);
                    break;
                case OBX_GET_REQ_EVT:
                    status = OBX_GetReq(handle, final, p_pkt);
                    break;
                case OBX_SETPATH_REQ_EVT:
                    /* get the flags from old request - if SetPath */
                    flags = *((UINT8 *)(p_saved_req + 1) + p_saved_req->offset + 3);
                    status = OBX_SetPathReq(handle, flags, p_pkt);
                    break;
                default:
                    /* it does not make sense to authenticate on Abort and Disconnect */
                    OBX_TRACE_WARNING1( "Authenticate on bad request: %d", p_pkt->event);
                    status = OBX_NO_RESOURCES;
                } /* switch (event) */
            }
        } /* digest done */
    } /* challenge heaer exists */

    if(status != OBX_SUCCESS)
    {
        GKI_freebuf(p_pkt);
    }

    if(p_cb->wait_auth == FALSE)
    {
        GKI_freebuf(p_cb->p_auth);
        p_cb->p_auth = NULL;
    }
    return status;
}

#if (OBX_MD5_TEST_INCLUDED == TRUE)
/*******************************************************************************
**
** Function     OBX_VerifyResponse
**
** Description  This function is called by the client to verify the challenge
**              response.
**
** Returns      TRUE, if successful.
**              FALSE, if authentication failed
**
*******************************************************************************/
BOOLEAN OBX_VerifyResponse(UINT32 nonce_u32, UINT8 *p_password, UINT8 password_len, UINT8 *p_response)
{
    BOOLEAN     status = FALSE;
    UINT8       nonce[OBX_NONCE_SIZE + 1];
    UINT8       temp_digest[OBX_DIGEST_SIZE];

    OBX_TRACE_API0( "OBX_VerifyResponse");
    if (p_password && password_len)
    {
        sprintf((char *)nonce, "%016lu", nonce_u32);
        OBX_MD5 (temp_digest, nonce, p_password, password_len);
        if (memcmp (temp_digest, p_response, OBX_DIGEST_SIZE) == 0)
            status = TRUE;
    }
    return status;
}
#endif /* OBX_MD5_TEST_INCLUDED */


#endif



