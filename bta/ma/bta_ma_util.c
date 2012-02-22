/*****************************************************************************
**
**  Name:           bta_ma_util.c
**
**  Description:    This is the implementation file for the Message Access 
**                  Profile (MAP) utility functions.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bta_ma_api.h"
#include "bta_ma_util.h"

/* 
** Static constant data declarations
*/
/* Charset */static const char * bmsg_body_charset[] = 
{
    "native",       /* BMSG_CHARSET_NATIVE (native) */
    "UTF-8"         /* BMSG_CHARSET_UTF-8  (UTF-8) */
};

static const int num_bmsg_body_charset = sizeof(bmsg_body_charset) / sizeof(char*);


/* vCard property names (the order is dicated by the tBTA_MA_BMSG_VCARD_PROP enumeration in bmsg_cnt.h) */
static const char * vcard_prop_name[] = 
{
    "N",            /* VCARD_PROP_N */
    "FN",           /* VCARD_PROP_FN */
    "TEL",          /* VCARD_PROP_TEL */
    "EMAIL"         /* VCARD_PROP_EMAIL */
};

/* bMessage encoding names (the order is dictated by the tBTA_MA_BMSG_ENCODING enumeration in bmsg_cnt.h) */
static const char * bmsg_body_encoding[] = 
{
    "8BIT",         /* BMSG_ENC_8BIT (8-Bit-Clean encoding) */
    "G-7BIT",       /* BMSG_ENC_G7BIT (GSM 7 bit Default Alphabet) */
    "G-7BITEXT",    /* BMSG_ENC_G7BITEXT (GSM 7 bit Alphabet with national language extension) */
    "G-UCS2" ,      /* BMSG_ENC_GUCS2 */
    "G-8BIT",       /* BMSG_ENC_G8BIT */
    "C-8BIT",       /* BMSG_ENC_C8BIT (Octet, unspecified) */
    "C-EPM",        /* BMSG_ENC_CEPM (Extended Protocol Message) */
    "C-7ASCII",     /* BMSG_ENC_C7ASCII (7-bit ASCII) */
    "C-IA5",        /* BMSG_ENC_CIA5 (IA5) */
    "C-UNICODE",    /* BMSG_ENC_CUNICODE (UNICODE) */
    "C-SJIS",       /* BMSG_ENC_CSJIS (Shift-JIS) */
    "C-KOREAN",     /* BMSG_ENC_CKOREAN (Korean) */
    "C-LATINHEB",   /* BMSG_ENC_CLATINHEB (Latin/Hebrew) */
    "C-LATIN"       /* BMSG_ENC_CLATIN (Latin) */
};

static const int num_bmsg_body_encoding = sizeof(bmsg_body_encoding) / sizeof(char*);

static const char * bmsg_body_language[] =
{
    "",             /* BMSG_LANG_UNSPECIFIED (not provided in bBody */
    "UNKNOWN",      /* BMSG_LANG_UNKNOWN */
    "SPANISH",      /* BMSG_LANG_SPANISH */
    "TURKISH",      /* BMSG_LANG_TURKISH */
    "PORTUGUESE",   /* BMSG_LANG_PORTUGUESE */
    "ENGLISH",      /* BMSG_LANG_ENGLISH */
    "FRENCH",       /* BMSG_LANG_FRENCH */
    "JAPANESE",     /* BMSG_LANG_JAPANESE */
    "KOREAN",       /* BMSG_LANG_KOREAN */
    "CHINESE",      /* BMSG_LANG_CHINESE */
    "HEBREW"        /* BMSG_LANG_HEBREW */
};

static const int num_bmsg_body_language = sizeof(bmsg_body_language) / sizeof(char*);

#include "bta_ma_co.h"

#define BTA_MSE_NOTIF_TYPE_NEW_MSG_STR              "NewMessage"
#define BTA_MSE_NOTIF_TYPE_DELIVERY_SUCCESS_STR     "DeliverySuccess"
#define BTA_MSE_NOTIF_TYPE_SENDING_SUCCESS_STR      "SendingSuccess"
#define BTA_MSE_NOTIF_TYPE_DELIVERY_FAILURE_STR     "DeliveryFailure"
#define BTA_MSE_NOTIF_TYPE_SENDING_FAILURE_STR      "SendingFailure"
#define BTA_MSE_NOTIF_TYPE_MEMORY_FULL_STR          "MemoryFull"
#define BTA_MSE_NOTIF_TYPE_MEMORY_AVAILABLE_STR     "MemoryAvailable"
#define BTA_MSE_NOTIF_TYPE_MESSAGE_DELETED_STR      "MessageDeleted"
#define BTA_MSE_NOTIF_TYPE_MESSAGE_SHIFT_STR        "MessageShift"

#define BTA_MSE_MSG_TYPE_EMAIL                      "EMAIL"
#define BTA_MSE_MSG_TYPE_SMS_GSM                    "SMS_GSM"
#define BTA_MSE_MSG_TYPE_SMS_CDMA                   "SMS_CDMA"
#define BTA_MSE_MSG_TYPE_MMS                        "MMS"

#define BTA_MSE_RCV_STATUS_COMPLETE                 "complete"
#define BTA_MSE_RCV_STATUS_FRACTIONED               "fractioned"
#define BTA_MSE_RCV_STATUS_NOTIFICATION             "notification"

#define BTA_MSE_BOOLEAN_YES                         "yes"
#define BTA_MSE_BOOLEAN_NO                          "no"

/*******************************************************************************
**
** Function         bta_ma_evt_typ_to_string
**
** Description      Utility function to return a pointer to a string holding
**                  the notifiction "type" for the MAP-Event-Report object.
**                             
** Parameters       notif_type - Notification type
**
** Returns          Pointer to static string representing notification type.
**
*******************************************************************************/

const char * bta_ma_evt_typ_to_string(tBTA_MSE_NOTIF_TYPE notif_type)
{
    switch (notif_type)
    {
        case BTA_MSE_NOTIF_TYPE_NEW_MSG:
            return(BTA_MSE_NOTIF_TYPE_NEW_MSG_STR);
        case BTA_MSE_NOTIF_TYPE_DELIVERY_SUCCESS:
            return(BTA_MSE_NOTIF_TYPE_DELIVERY_SUCCESS_STR);
        case BTA_MSE_NOTIF_TYPE_SENDING_SUCCESS:
            return(BTA_MSE_NOTIF_TYPE_SENDING_SUCCESS_STR);
        case BTA_MSE_NOTIF_TYPE_DELIVERY_FAILURE:
            return(BTA_MSE_NOTIF_TYPE_DELIVERY_FAILURE_STR);
        case BTA_MSE_NOTIF_TYPE_SENDING_FAILURE:
            return(BTA_MSE_NOTIF_TYPE_SENDING_FAILURE_STR);
        case BTA_MSE_NOTIF_TYPE_MEMORY_FULL:
            return(BTA_MSE_NOTIF_TYPE_MEMORY_FULL_STR);
        case BTA_MSE_NOTIF_TYPE_MEMORY_AVAILABLE:
            return(BTA_MSE_NOTIF_TYPE_MEMORY_AVAILABLE_STR);
        case BTA_MSE_NOTIF_TYPE_MESSAGE_DELETED:
            return(BTA_MSE_NOTIF_TYPE_MESSAGE_DELETED_STR);
        case BTA_MSE_NOTIF_TYPE_MESSAGE_SHIFT:
            return(BTA_MSE_NOTIF_TYPE_MESSAGE_SHIFT_STR);
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_ma_msg_typ_to_string
**
** Description      Utility function to return a pointer to a string holding
**                  the "msg_type" for the MAP-Event-Report object or or the
**                  the type for the MAP-msg-listing object
**                             
** Parameters       msg_typ - Message type
**
** Returns          Pointer to static string representing message type.
**
*******************************************************************************/

const char * bta_ma_msg_typ_to_string(tBTA_MA_MSG_TYPE msg_typ)
{
    switch (msg_typ)
    {
        case BTA_MA_MSG_TYPE_EMAIL:    return(BTA_MSE_MSG_TYPE_EMAIL);
        case BTA_MA_MSG_TYPE_SMS_GSM:  return(BTA_MSE_MSG_TYPE_SMS_GSM);
        case BTA_MA_MSG_TYPE_SMS_CDMA: return(BTA_MSE_MSG_TYPE_SMS_CDMA);
        case BTA_MA_MSG_TYPE_MMS:      return(BTA_MSE_MSG_TYPE_MMS);
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_ma_rcv_status_to_string
**
** Description      Utility function to return a pointer to a string holding
**                  the "reception_status" for the MAP-msg-listing object 
**                             
** Parameters       rcv_status - Reception status
**
** Returns          Pointer to static string representing message type.
**
*******************************************************************************/
const char * bta_ma_rcv_status_to_string(tBTA_MSE_CO_RCV_STATUS rcv_status)
{
    switch (rcv_status)
    {
        case BTA_MSE_CO_RCV_STATUS_COMPLETE:    
            return(BTA_MSE_RCV_STATUS_COMPLETE);
        case BTA_MSE_CO_RCV_STATUS_FRACTIONED:  
            return(BTA_MSE_RCV_STATUS_FRACTIONED);
        case BTA_MSE_CO_RCV_STATUS_NOTIFICATION: 
            return(BTA_MSE_RCV_STATUS_NOTIFICATION);
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_ma_stream_str
**
** Description      Input a string into the stream.
**                 
** Parameters       p_stream - pointer to stream information.
**                  p_str - pointer to string to be put in the buffer. Only the
**                      string characters, and not the NULL, are put into the
**                      buffer.
**
** Returns          TRUE if the string was successfully added into the stream.
**
*******************************************************************************/
BOOLEAN bta_ma_stream_str(tBTA_MA_STREAM * p_stream, const char * p_str)
{
    UINT16  buf_size;
    UINT16  str_size;
    UINT16  size = 0;

    /* ensure stream and string are not NULL */
    if ( !p_str || !p_stream )
        return(FALSE);

    /* get length of string */
    str_size = strlen(p_str);

    switch ( p_stream->type )
    {
        case STRM_TYPE_MEMORY:
            /* ensure buffer is not full */
            if ( p_stream->status == STRM_SUCCESS )
            {
                /* get amount of size left in buffer */
                buf_size = p_stream->u.mem.size - bta_ma_stream_used_size(p_stream);

                /* calculate the size to copy (the minimum of string and buffer size */
                if ( str_size > buf_size )
                {
                    size = buf_size;
                    p_stream->status = STRM_ERROR_OVERFLOW;
                }
                else
                    size = str_size;

                /* copy the data and move the pointer */
                memcpy(p_stream->u.mem.p_next, p_str, size);
                p_stream->u.mem.p_next += size;
            }
            break;

        case STRM_TYPE_FILE:
            /* write string */
            bta_ma_co_write(p_stream->u.file.fd, p_str, str_size);
            break;
    }

    /* return TRUE if stream is OK */
    return(p_stream->status == STRM_SUCCESS);
}

/*******************************************************************************
**
** Function         bta_ma_stream_buf
**
** Description      Stream a buffer into the buffer.
**                 
** Parameters       p_stream - pointer to stream information.
**                  len - length of buffer
**                  p_buf - pointer to buffer to stream.
**
** Returns          TRUE if the buffer was large enough to hold the data.
**
*******************************************************************************/
extern BOOLEAN bta_ma_stream_buf(tBTA_MA_STREAM * p_stream,
                                 UINT16 len,
                                 UINT8 * p_buf)
{
    UINT16  buf_size;
    UINT16  size = 0;

    /* ensure stream and buffer pointer are not NULL */
    if ( !p_buf || !p_stream )
        return(FALSE);

    switch ( p_stream->type )
    {
        case STRM_TYPE_MEMORY:
            /* ensure buffer is not full */
            if ( p_stream->status == STRM_SUCCESS )
            {
                /* get amount of size left in buffer */
                buf_size = p_stream->u.mem.size - bta_ma_stream_used_size(p_stream);

                /* calculate the size to copy (the minimum of string and buffer size */
                if ( len > buf_size )
                {
                    size = buf_size;
                    p_stream->status = STRM_ERROR_OVERFLOW;
                }
                else
                    size = len;

                /* copy the data and move the pointer */
                memcpy(p_stream->u.mem.p_next, p_buf, len);
                p_stream->u.mem.p_next += size;
            }
            break;
        case STRM_TYPE_FILE:
            /* write string */
            bta_ma_co_write(p_stream->u.file.fd, p_buf, len);
            break;
    }

    /* return TRUE if stream is OK */
    return(p_stream->status == STRM_SUCCESS);
}


/*******************************************************************************
**
** Function         bta_ma_stream_boolean_yes_no
**
** Description      Stream a yes/no string into the buffer.
**                 
** Parameters       p_stream - pointer to stream information.
**                  val - a boolean value to indicate yes or no
**
** Returns          TRUE if the yes/no string was successfully added into 
**                      the stream.
**
*******************************************************************************/
BOOLEAN bta_ma_stream_boolean_yes_no(tBTA_MA_STREAM * p_stream, BOOLEAN val)
{
    return bta_ma_stream_str(p_stream, 
                             val == FALSE ? BTA_MSE_BOOLEAN_NO : BTA_MSE_BOOLEAN_YES);
}

/*******************************************************************************
**
** Function         bta_ma_stream_value
**
** Description      Stream an UINT32 value string into the buffer.
**                 
** Parameters       p_stream - pointer to stream information.
**                  val - a UINT32 value
**
** Returns          TRUE if the buffer was large enough to hold the data.
**
*******************************************************************************/
BOOLEAN bta_ma_stream_value(tBTA_MA_STREAM * p_stream, UINT32 val)
{
    char    temp[50];

    sprintf(temp, "%lu", val);
    return bta_ma_stream_str(p_stream, temp);
}

/*******************************************************************************
**
** Function         bta_ma_stream_handle
**
** Description      Stream a message handle into the buffer.
**                 
** Parameters       p_stream - pointer to stream information.
**                  handle - handle to be put in the buffer.
**
** Returns          TRUE if the buffer was large enough to hold the data.
**
*******************************************************************************/

BOOLEAN bta_ma_stream_handle(tBTA_MA_STREAM * p_stream, tBTA_MA_MSG_HANDLE handle)
{
    char    temp[5];
    int     x;
    BOOLEAN value_yet = FALSE;

    /* ensure buffer is not full */
    if ( p_stream->status == STRM_SUCCESS )
    {
        for ( x=0; x < BTA_MA_HANDLE_SIZE; x++ )
        {
            /* Skip any leading 0's */
            if ( (! value_yet) && (handle[x] == 0) )
                continue;

            value_yet = TRUE;

            sprintf(temp, "%02x", handle[x]);

            if ( bta_ma_stream_str(p_stream, temp) == FALSE )
                return(FALSE);
        }
    }

    /* return TRUE if stream is OK */
    return(p_stream->status == STRM_SUCCESS);
}


/*******************************************************************************
**
** Function         bta_ma_stream_used_size
**
** Description      Returns the used byte count.
**                 
** Parameters       p_stream - pointer to stream information.
**
** Returns          Number of bytes used in the buffer.
**
*******************************************************************************/

UINT16 bta_ma_stream_used_size(tBTA_MA_STREAM * p_stream)
{
    return(p_stream->u.mem.p_next - p_stream->u.mem.p_buffer);
}

/*******************************************************************************
**
** Function         bta_ma_stream_ok
**
** Description      Determines if the stream is ok.
**
** Parameters       p_stream - pointer to stream information.
**
** Returns          TRUE if stream status is OK.
**
*******************************************************************************/

BOOLEAN bta_ma_stream_ok(tBTA_MA_STREAM * p_stream)
{
    return(p_stream && p_stream->status == STRM_SUCCESS);
}

/*******************************************************************************
**
** Function         bta_ma_convert_hex_str_to_64bit_handle
**
** Description      Convert a hex string to a 64 bit message handle in Big Endian  
**                  format
**
** Returns          void
**
*******************************************************************************/
void bta_ma_convert_hex_str_to_64bit_handle(char *p_hex_str, tBTA_MA_MSG_HANDLE handle)
{
    UINT32 ul1, ul2;
    UINT8  *p;
    char   tmp[BTA_MA_32BIT_HEX_STR_SIZE];
    UINT8   str_len;

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT0("bta_mse_convert_hex_str_to_64bit_handle");
#endif

    str_len = strlen(p_hex_str);
    memset(handle,0,sizeof(tBTA_MA_MSG_HANDLE));

    if (str_len >= 8)
    {
        /* most significant 4 bytes */
        memcpy(tmp,p_hex_str,(str_len-8));
        tmp[str_len-8]='\0';
        ul1 = strtoul(tmp,0,16);
        p=handle;
        UINT32_TO_BE_STREAM(p, ul1);

        /* least significant 4 bytes */
        memcpy(tmp,&(p_hex_str[str_len-8]),8);
        tmp[8]='\0';
        ul2 = strtoul(tmp,0,16);
        p=&handle[4];
        UINT32_TO_BE_STREAM(p, ul2);
    }
    else
    {
        /* least significant 4 bytes */
        ul1 = strtoul(p_hex_str,0,16);
        p=&handle[4];
        UINT32_TO_BE_STREAM(p, ul1);
    }
}

/*******************************************************************************
**
** Function         bta_ma_get_char
**
** Description      Gets one character from the stream.
**
** Parameters       p_stream - pointer to stream information.
**                  p_char - pointer to where to receive the character.
**
** Returns          TRUE if character was read OK.
**
*******************************************************************************/
BOOLEAN bta_ma_get_char(tBTA_MA_STREAM * p_stream, char * p_char)
{
    BOOLEAN bStatus = FALSE;

    if ( p_char )
    {
        *p_char = 0;

        if ( p_stream )
        {
            switch ( p_stream->type )
            {
                case STRM_TYPE_MEMORY:
                    if ( (p_stream->u.mem.p_next-p_stream->u.mem.p_buffer) < p_stream->u.mem.size )
                    {
                        *p_char = *p_stream->u.mem.p_next;
                        p_stream->u.mem.p_next++;

                        bStatus = TRUE;
                    }
                    break;
                case STRM_TYPE_FILE:
                    /* read character */
                    bStatus = bta_ma_co_read(p_stream->u.file.fd, (void *) p_char, 1) == 1;
                    break;
            }
        }
    }

    return( bStatus );
}


/*******************************************************************************
**
** Function         bta_ma_get_tag
**
** Description      Parses a tag from the stream.  Basically this returns any text
**                  before a ':' character, ignoring leading whitespace.
**                             
** Parameters       p_stream - Input stream.
**                  psz - buffer to receive the tag
**                  max_size - size of the receiving buffer (including space
**                      for the NULL character.
**
** Returns          Size of tag, or 0 if there was an error.
**
*******************************************************************************/
UINT16 bta_ma_get_tag(tBTA_MA_STREAM * p_stream, char * psz, UINT16 max_size)
{
    char    c;
    UINT16  count = 0;

    /* handle bad arguments */
    if ( p_stream && psz && (max_size > 0) )
    {
        /* reserve last byte for NULL terminator */
        max_size--;

        while ( bta_ma_get_char(p_stream, &c) 
                && (c != ':')
                && (count < max_size) )
        {
            /* ignore leading whitespace */
            if ( !count && ((c == ' ') || (c == '\t')) )
                continue;

            /* if we hit a CR, return 0 to indicate an error */
            if ( c == '\r' )
                return( 0 );

            psz[count++] = c;
        }

        /* Either we hit a problem reading from the stream
        ** or the buffer was not large enough
        */
        if ( c != ':' )
            return( 0 );

        /* terminate string */
        psz[count] = '\0';
    }

    return( count );
}

/*******************************************************************************
**
** Function         bta_ma_get_value
**
** Description      Parses a value from the stream.  Basically this any text
**                  up to (but not including) the CR LF sequence.
**                             
** Parameters       p_stream - Input stream.
**                  psz - buffer to receive the value
**                  max_size - size of the receiving buffer (including space
**                      for the NULL character.
**
** Returns          Size of value, or 0 if there was an error.
**
*******************************************************************************/
UINT16 bta_ma_get_value(tBTA_MA_STREAM * p_stream, char * psz, UINT16 max_size)
{
    char    c;
    UINT16  count = 0;

    /* handle bad arguments */
    if ( p_stream && psz && (max_size > 0) )
    {
        /* reserve last byte for NULL terminator */
        max_size--;

        while ( bta_ma_get_char(p_stream, &c) 
                && (c != '\r')
                && (count < max_size) )
        {
            psz[count++] = c;
        }

        /* Either we hit a problem reading from the stream
        ** or the buffer was not large enough
        */
        if ( c != '\r' )
            return( 0 );

        /* burn the next character which must be LF */
        if ( ! bta_ma_get_char(p_stream, &c) )
            return( 0 );

        /* terminate string */
        psz[count] = '\0';
    }

    return( count );
}

/*******************************************************************************
**
** Function         bta_ma_get_param
**
** Description      Parses the parameter from the source string.  
**                             
** Parameters       p_src - source paramter string.
**                  psz - buffer to receive the value
**
** Returns          Size of param, or 0 if there was an error.
**
*******************************************************************************/
UINT16 bta_ma_get_param(char *p_src, char *psz ) 
{
    char    c;
    UINT16  count = 0;
	BOOLEAN first_semicolon_found=FALSE;

    /* handle bad arguments */
    if ( p_src && psz  )
    {
        while ( (c = *p_src++) )
        {

			/* throw away the first ';' */
            if ( !count && (c==';') )
			{
                first_semicolon_found = TRUE;
                continue;
			}

			/* first char should be ';' otherwise return 0 */
			if(!count && !first_semicolon_found && (c!=';'))
				return (0);

            /* if we hit a CR, return 0 to indicate an error */
            if ( c == '\r' )
                return( 0 );

            psz[count++] = c;
        }

        if ( !count )
            return( 0 );

        /* terminate string */
        psz[count] = '\0';
    }

    return( count );
}




/*******************************************************************************
**
** Function         bta_ma_parse_vcard
**
** Description      Parses a vCard from the stream into a generic tBTA_MA_BMSG_VCARD
**                  structure.
**                             
** Parameters       p_vcard - pointer to generic vCard structure.
**                  p_stream - Input stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS bta_ma_parse_vcard(tBTA_MA_BMSG_VCARD * p_vcard,
                                  tBTA_MA_STREAM * p_stream)
{
    char sz[BTA_MA_MAX_SIZE];
    tBTA_MA_STATUS status = BTA_MA_STATUS_FAIL;
    tBTA_MA_VCARD_VERSION version;
    char * psztoken_tel = "TEL";
// btla-specific ++
    char * psztoken_name = "N;";
// btla-specific --
    char param[BTA_MA_MAX_SIZE];
    char *p_src;
	UINT16 len;


    while ( bta_ma_get_tag(p_stream, sz, BTA_MA_MAX_SIZE) )
    {
        if ( strcmp(sz, "VERSION") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            if ( strcmp(sz, "3.0") == 0 )
                version = BTA_MA_VCARD_VERSION_30;
            else if ( strcmp(sz, "2.1") == 0 )
                version = BTA_MA_VCARD_VERSION_21;
            else
            {
                APPL_TRACE_ERROR1("bta_ma_parse_vcard - Invalid vcard version: '%s'", sz);
                break;
            }

            BTA_MaBmsgSetVcardVersion(p_vcard, version);
        }
        else if ( strcmp(sz, "N") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_N, sz, NULL);
// btla-specific ++
        }
        else if ( strstr(sz, psztoken_name) == sz  )
        {
            p_src = sz + strlen(psztoken_name) - 1; // move to (first) semicolon, not past it
            len = strlen(p_src);
            if ( (len < BTA_MA_MAX_SIZE) && bta_ma_get_param(p_src, param))
            {
                bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_N, sz, param);
            }
// btla-specific --
        }
        else if ( strcmp(sz, "FN") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_FN, sz, NULL);
        }
        else if (  strcmp(sz, psztoken_tel ) == 0  )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_TEL, sz, NULL);
        }
        else if ( strstr(sz, psztoken_tel) == sz  )
        {
            p_src = sz + strlen(psztoken_tel);
			len = strlen(p_src);
            if ( (len < BTA_MA_MAX_SIZE) && bta_ma_get_param(p_src, param))
            {
                bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_TEL, sz, param);
            }
        }
        else if ( strcmp(sz, "EMAIL") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            BTA_MaBmsgAddVcardProp(p_vcard, BTA_MA_VCARD_PROP_EMAIL, sz, NULL);
        }
        else if ( strcmp(sz, "END") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            status = BTA_MA_STATUS_OK;
            break;
        }
        else
        {
            APPL_TRACE_ERROR1("bta_ma_parse_vcard - Invalid tag: '%s'", sz);
        }
    }

    return( status );
}

/*******************************************************************************
**
** Function         bta_ma_parse_content
**
** Description      Parses a <bmessage-body-content> from the stream into a 
**                  generic tBTA_MA_BMSG_CONTENT structure. This will parse text until
**                  we see "END:MSG" at the start of a line.
**                             
** Parameters       p_content - pointer to generic content structure.
**                  p_stream - Input stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS bta_ma_parse_content(tBTA_MA_BMSG_CONTENT * p_content,
                                    tBTA_MA_STREAM * p_stream)
{
/* This constant defines the size of the work buffer used for parsing.
** It MUST be larger than the 'END:MSG<CRLF>" string size.  The larger
** the buffer the more efficient this parser will be.
*/
#define BTA_MA_PARSE_BUF_SIZE   BTA_MA_MAX_SIZE

/* These constants define the four states the parser can be in.
*/
#define STATE_WS            (0)     /* checking for leading whitespace */
#define STATE_END           (1)     /* checking for END:MSG */
#define STATE_CR            (2)     /* checking for CRLF */
#define STATE_TEXT          (3)     /* copying text */

    static const char * END_MSG = "END:MSG\r\n";

    char sz[BTA_MA_PARSE_BUF_SIZE+1];
    char c;
    int state = STATE_WS;           /* start in the 'whitespace' state */
    int idx_commit = 0;
    int idx_trial = 0;
    int idx_end = 0;
    int x;

    /* NOTES - There are 3 primary indices used during parsing:
    **
    ** 'idx_commit' these are characters that are commited to being
    **      in the message text.  We need to be able to save characters
    **      (such as <CR><LF>, "END..", etc.) but not actually 'commit' them.
    **
    ** 'idx_trial' these are characters that we are saving on a trial
    **      basis until we know what to do with them.  For example, if
    **      we get a sequence "<CR>+<LF>+E+N", we don't want to commit
    **      them until we know it is not "END:MSG<CR><LF>".
    **
    ** 'idx_end' is used to index through the "END:MSG<CR><LF> string.
    */

    /* Handle bad arguments */
    if ( p_stream && p_content )
    {
        /* Get one character from the stream */
        while ( bta_ma_get_char(p_stream, &c) )
        {
            switch (state)
            {
                case STATE_WS:
                    /* totally ignore leading whitespace */
                    if ( (c == ' ') || (c == '\t') )
                        continue;

                    /* Otherwise intentionaly fall through after resetting the
                    ** 'end' index so we start comparing from the beginning. 
                    */
                    idx_end = 0;

                case STATE_END:
                    /* Is the character in the "END:MSG<CR><LF> sequence? */
                    if ( c == END_MSG[idx_end] )
                    {
                        /* Yes.  Did we get to the end of "END:MSG<cr><lf>"? */
                        if ( ! END_MSG[++idx_end] )
                        {
                            /* Yes. Commit any characters and get out. */
                            if ( idx_commit )
                            {
                                sz[idx_commit] = '\0';
                                BTA_MaBmsgAddMsgContent(p_content, sz);
                            }

                            return( BTA_MA_STATUS_OK );
                        }

                        state = STATE_END;
                        break;
                    }
                    /* If we fell through from the whitespace state 
                    ** then we should commit all chars at this point.
                    ** It handles the case where we get consecutive CRLF.
                    */
                    if ( state == STATE_WS )
                        idx_commit = idx_trial;

                    /* And intentionally fall through */

                case STATE_CR:
                    /* We got <CR>, is this <LF>? */
                    if ( c == '\n' )
                    {
                        /* Now look for any whitespace */
                        state = STATE_WS;
                        break;
                    }

                    /* otherwise intentionally fall through */

                case STATE_TEXT:
                    /* is a CR? */
                    if ( c == '\r' )
                        state = STATE_CR;   /* Look for <LF> */
                    else
                        state = STATE_TEXT; /* Copy the text */
                    break;
            }

            /* All (non-whitespace) characters are copied to 
            ** the buffer as 'trial' characters possibly
            ** committed later.
            */
            sz[idx_trial++] = c;

            /* If we are in the text copy state, then 
            ** commit all characters to this point.
            */
            if ( state == STATE_TEXT )
                idx_commit = idx_trial;

            /* The buffer is full.  Commit the good characters
            ** to the message content, and rearrange the rest
            ** of the text to make room for more.
            */
            if ( idx_trial == BTA_MA_PARSE_BUF_SIZE )
            {
                /* Do we have characters to commit?
                ** If we don't we are in trouble.
                */
                if ( idx_commit )
                {
                    /* Save the last character so we can put a NULL there. */
                    c = sz[idx_commit];
                    sz[idx_commit] = '\0';
                    BTA_MaBmsgAddMsgContent(p_content, sz);

                    /* Do we need to rearrange uncommited text? */
                    if ( idx_commit != idx_trial )
                    {
                        /* Restore character */
                        sz[idx_commit] = c;

                        /* Copy the 'trial' characters to the beginning of buffer */
                        idx_trial -= idx_commit;
                        for ( x=0; x < idx_trial; x++)
                            sz[x] = sz[x+idx_commit];
                        idx_commit = 0;
                    }
                    else
                    {
                        idx_trial = idx_commit = 0;
                    }
                }
                else
                {
                    /* ERROR - no space to shuffle things around */
                    APPL_TRACE_ERROR0("bta_ma_parse_content - work buffer too small");
                    break;
                }
            }
        }
    }

    return( BTA_MA_STATUS_FAIL );
}

/*******************************************************************************
**
** Function         bta_ma_parse_body
**
** Description      Parses a <bmessage-content> (BBODY) from the stream into a 
**                  generic tBTA_MA_BMSG_BODY structure.  This will parse text until
**                  we see "END:BODY" at the start of a line.
**                             
** Parameters       p_body - pointer to generic content body structure.
**                  p_stream - Input stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS bta_ma_parse_body(tBTA_MA_BMSG_BODY * p_body,
                                 tBTA_MA_STREAM * p_stream)
{
    char sz[BTA_MA_MAX_SIZE];
    tBTA_MA_STATUS          status = BTA_MA_STATUS_FAIL;
    tBTA_MA_BMSG_CONTENT    *p_content = NULL;
    tBTA_MA_BMSG_ENCODING   encoding;
    tBTA_MA_BMSG_LANGUAGE   language;
    tBTA_MA_CHARSET         charset;

    while ( bta_ma_get_tag(p_stream, sz, BTA_MA_MAX_SIZE) )
    {
        if ( strcmp(sz, "PARTID") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            BTA_MaBmsgSetBodyPartid(p_body, (UINT16) atoi(sz));
        }
        else if ( strcmp(sz, "ENCODING") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            if ( bta_ma_str_to_encoding(sz, &encoding) )
                BTA_MaBmsgSetBodyEncoding(p_body, encoding);
            else
                APPL_TRACE_ERROR1("bta_ma_parse_body - Invalid ENCODING: '%s'", sz);
        }
        else if ( strcmp(sz, "CHARSET") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            if ( bta_ma_str_to_charset(sz, &charset))
                BTA_MaBmsgSetBodyCharset(p_body, charset);
            else
                APPL_TRACE_ERROR1("bta_ma_parse_body - invalid CHARSET: '%s'", sz);
        }
        else if ( strcmp(sz, "LANGUAGE") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            if ( bta_ma_str_to_language(sz, &language) )
                BTA_MaBmsgSetBodyLanguage(p_body, language);
            else
                APPL_TRACE_ERROR1("bta_ma_parse_body - Invalid LANGUAGE: '%s'", sz);
        }
        else if ( strcmp(sz, "LENGTH") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            /* we don't really care about the length */
        }
        else if ( strcmp(sz, "BEGIN") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);

            if ( strcmp(sz, "MSG") == 0 )
            {
                p_content = BTA_MaBmsgAddContentToBody(p_body);
                bta_ma_parse_content(p_content, p_stream);
            }
            else
            {
                APPL_TRACE_ERROR1("bta_ma_parse_body - Invalid BEGIN: '%s'", sz);
            }
        }
        else if ( strcmp(sz, "END") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            status = BTA_MA_STATUS_OK;
            break;
        }
        else
        {
            APPL_TRACE_ERROR1("bta_ma_parse_body - Invalid tag: '%s'", sz);
        }
    }

    return( status );
}

/*******************************************************************************
**
** Function         bta_ma_parse_envelope
**
** Description      Parses a <bmessage-envelope> from the stream into a 
**                  generic tBTA_MA_BMSG_ENVELOPE structure.  This will parse text 
**                  until we see "END:BENV" at the start of a line.
**                             
** Parameters       p_envelope - pointer to generic envelope structure.
**                  p_stream - Input stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS bta_ma_parse_envelope(tBTA_MA_BMSG_ENVELOPE * p_envelope,
                                     tBTA_MA_STREAM * p_stream)
{
    char sz[BTA_MA_MAX_SIZE];
    tBTA_MA_STATUS status = BTA_MA_STATUS_FAIL;
    tBTA_MA_BMSG_VCARD * p_vcard = NULL;
    tBTA_MA_BMSG_ENVELOPE * p_new_envelope = NULL;
    tBTA_MA_BMSG_BODY * p_body = NULL;

    while ( bta_ma_get_tag(p_stream, sz, BTA_MA_MAX_SIZE) )
    {
        if ( strcmp(sz, "BEGIN") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);

            if ( strcmp(sz, "VCARD") == 0 )
            {
                p_vcard = BTA_MaBmsgAddRecipToEnv(p_envelope);
                bta_ma_parse_vcard(p_vcard, p_stream);
            }
            else if ( strcmp(sz, "BENV") == 0 )
            {
                p_new_envelope = BTA_MaBmsgAddEnvToEnv(p_envelope);
                bta_ma_parse_envelope(p_new_envelope, p_stream);
            }
            else if ( strcmp(sz, "BBODY") == 0 )
            {
                p_body = BTA_MaBmsgAddBodyToEnv(p_envelope);
                bta_ma_parse_body(p_body, p_stream);
            }
            else
            {
                APPL_TRACE_ERROR1("bta_ma_parse_envelope - Invalid BEGIN: '%s'", sz);
            }

        }
        else if ( strcmp(sz, "END") == 0 )
        {
            bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
            status = BTA_MA_STATUS_OK;
            break;
        }
        else
        {
            APPL_TRACE_ERROR1("bta_ma_parse_envelope - Invalid tag: '%s'", sz);
        }
    }

    return( status );
}

/*******************************************************************************
**
** Function         bta_ma_stream_vcards
**
** Description      Builds vCards into a stream.
**                             
** Parameters       p_stream - Output stream.
**                  p_vcard - pointer to single vCard that may be linked to
**                      additional vCards.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ma_stream_vcards(tBTA_MA_STREAM * p_stream, 
                          tBTA_MA_BMSG_VCARD * p_vcard)
{
    int x;

    /* vCards are formatted one after another */
    while ( p_stream && p_vcard )
    {
        bta_ma_stream_str(p_stream, "\r\nBEGIN:VCARD");

        /* version */
        bta_ma_stream_str(p_stream, "\r\nVERSION:");
        bta_ma_stream_str(p_stream, 
                          p_vcard->version == BTA_MA_VCARD_VERSION_21 ? "2.1" : "3.0");

        /* vcard properties */
        for (x=0; x < BTA_MA_VCARD_PROP_MAX; x++)
            bta_ma_stream_vcard_prop(p_stream, p_vcard,(tBTA_MA_VCARD_PROP) x);

        bta_ma_stream_str(p_stream, "\r\nEND:VCARD");

        /* Get the next vCard and repeat */
        p_vcard = BTA_MaBmsgGetNextVcard(p_vcard);
    }
}

/*******************************************************************************
**
** Function         bta_ma_stream_vcard_prop
**
** Description      Builds a property and values into a stream.  This will
**                  build all of the property/values for one property (i.e. 
**                  can be multiple EMAIL propeties set).  It will only
**                  format the property if it has a value (except the N/name
**                  if a 2.1 vCard and FN/fullname property of a 3.0 vCard
**                  will always be output).
**                             
** Parameters       p_stream - Output stream.
**                  p_vcard - pointer to vCard.
**                  prop - property.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ma_stream_vcard_prop(tBTA_MA_STREAM * p_stream, 
                              tBTA_MA_BMSG_VCARD * p_vcard, 
                              tBTA_MA_VCARD_PROP prop)
{
    tBTA_MA_VCARD_PROPERTY * p_prop;
    tBTA_MA_VCARD_VERSION version;
    char * p_param;
    char * p_value;

    if ( p_vcard && prop < BTA_MA_VCARD_PROP_MAX )
    {
        p_prop = BTA_MaBmsgGetVcardProp(p_vcard, prop);

        do
        {
            p_param = BTA_MaBmsgGetVcardPropParam(p_prop);
            p_value = BTA_MaBmsgGetVcardPropValue(p_prop);
            version = BTA_MaBmsgGetVcardVersion(p_vcard);

            if ( (p_value && strlen(p_value))
                 || ((version == BTA_MA_VCARD_VERSION_21) && (prop == BTA_MA_VCARD_PROP_N))
                 || ((version == BTA_MA_VCARD_VERSION_30) && (prop <= BTA_MA_VCARD_PROP_FN)) )
            {
                /* property name */
                bta_ma_stream_str(p_stream, "\r\n");
                bta_ma_stream_str(p_stream, vcard_prop_name[prop]);


                /* property parameter */
                if ( p_param )
                {
                    bta_ma_stream_str(p_stream, ";");
                    bta_ma_stream_str(p_stream, p_param);
                }

                /* property value */
                bta_ma_stream_str(p_stream, ":");
                bta_ma_stream_str(p_stream, p_value);
            }

            /* There may be multiple instances of a property (e.g. 2 TEL numbers */
            p_prop = BTA_MaBmsgGetNextVcardProp(p_prop);

        } while ( p_prop );
    }
}

/*******************************************************************************
**
** Function         bta_ma_stream_envelopes
**
** Description      Builds a envelope <bmessage-envelope> (or series of 
**                  envelopes) into a stream.
**                             
** Parameters       p_stream - Output stream.
**                  p_envelope - pointer to envelope structure.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ma_stream_envelopes(tBTA_MA_STREAM * p_stream, 
                             tBTA_MA_BMSG_ENVELOPE * p_envelope)
{
    tBTA_MA_BMSG_BODY * p_body;

    if ( p_stream && p_envelope )
    {
        bta_ma_stream_str(p_stream, "\r\nBEGIN:BENV");

        /* Recipients */
        bta_ma_stream_vcards(p_stream, BTA_MaBmsgGetRecipFromEnv(p_envelope));

        /* It will either be another (nested) envelope or the body */
        p_body = BTA_MaBmsgGetBodyFromEnv(p_envelope);

        if ( p_body )
            bta_ma_stream_body(p_stream, p_body);
        else
            bta_ma_stream_envelopes(p_stream, BTA_MaBmsgGetNextEnv(p_envelope));

        bta_ma_stream_str(p_stream, "\r\nEND:BENV");
    }
}

/*******************************************************************************
**
** Function         bta_ma_stream_body
**
** Description      Builds a bMessage content <bmessage-content> into  a stream.
**                             
** Parameters       p_stream - Output stream.
**                  p_body - pointer to bBody structure.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ma_stream_body(tBTA_MA_STREAM * p_stream, tBTA_MA_BMSG_BODY * p_body)
{
    UINT16 part_id = 0;
    tBTA_MA_BMSG_LANGUAGE language;
    tBTA_MA_CHARSET charset;

    if ( p_stream && p_body )
    {
        bta_ma_stream_str(p_stream, "\r\nBEGIN:BBODY");

        /* Part ID (optional) */
        part_id = BTA_MaBmsgGetBodyPartid(p_body);
        if ( part_id != 0 )
        {
            bta_ma_stream_str(p_stream, "\r\nPARTID:");
            bta_ma_stream_value(p_stream, part_id);
        }

        /* Character set */
        charset = BTA_MaBmsgGetBodyCharset(p_body);
        switch ( charset)
        {
            case BTA_MA_CHARSET_UTF_8:
                bta_ma_stream_str(p_stream, "\r\nCHARSET:UTF-8");
                break;
            case BTA_MA_CHARSET_NATIVE:
                bta_ma_stream_str(p_stream, "\r\nCHARSET:NATIVE");
                /* Encoding */
                bta_ma_stream_str(p_stream, "\r\nENCODING:");
                bta_ma_stream_str(p_stream, bmsg_body_encoding[BTA_MaBmsgGetBodyEncoding(p_body)]);
                break;
            default:
                break;
        }

        /* Language */
        language = BTA_MaBmsgGetBodyLanguage(p_body);
        if ( language != BTA_MA_BMSG_LANG_UNSPECIFIED )
        {
            bta_ma_stream_str(p_stream, "\r\nLANGUAGE:");
            bta_ma_stream_str(p_stream, bmsg_body_language[language]);
        }

        /* Body content length */
        bta_ma_stream_str(p_stream, "\r\nLENGTH:");
        bta_ma_stream_value(p_stream, bta_ma_get_body_length(p_body));

        /* Content */
        bta_ma_stream_body_content(p_stream, BTA_MaBmsgGetContentFromBody(p_body));

        bta_ma_stream_str(p_stream, "\r\nEND:BBODY");
    }
}

/*******************************************************************************
**
** Function         bta_ma_stream_body_content
**
** Description      Builds a body content <bmessage-body-content> into a stream.
**                             
** Parameters       p_stream - Output stream.
**                  p_content - pointer to content structure.
**                  
** Returns          void
**
*******************************************************************************/
void bta_ma_stream_body_content(tBTA_MA_STREAM * p_stream, 
                                tBTA_MA_BMSG_CONTENT * p_content)
{
    char * p_text;

    APPL_TRACE_EVENT0("bta_ma_stream_body_content");
    while ( p_stream && p_content )
    {
        bta_ma_stream_str(p_stream, "\r\nBEGIN:MSG");

        p_text = BTA_MaBmsgGetMsgContent(p_content);
        if ( p_text )
        {
            bta_ma_stream_str(p_stream, "\r\n");

            while ( p_text )
            {
                bta_ma_stream_str(p_stream, p_text);
                p_text = BTA_MaBmsgGetNextMsgContent(p_content);
            }
        }

        bta_ma_stream_str(p_stream, "\r\nEND:MSG");

        p_content = BTA_MaBmsgGetNextContent(p_content);
    }
}

/*******************************************************************************
**
** Function         bta_ma_str_to_charset
**
** Description      Returns the charset enumeration (tBTA_MA_CHARSET) that
**                      corresponds to the provided string.
**                             
** Parameters       psz - Input string.
**                  p_charset - pointer to the charset value to be received.
**                  
** Returns          TRUE if there is a match, otherwise FALSE.
**
*******************************************************************************/
BOOLEAN bta_ma_str_to_charset(char * psz, tBTA_MA_CHARSET * p_charset)
{

    tBTA_MA_CHARSET e;

    if ( psz && p_charset )
    {
        for (e= BTA_MA_CHARSET_NATIVE; e < num_bmsg_body_charset; e++)
        {
            if ( strcmp(psz, bmsg_body_charset[e]) == 0 )
            {
                *p_charset = e;
                return( TRUE );
            }
        }
    }

    return( FALSE );
}


/*******************************************************************************
**
** Function         bta_ma_str_to_encoding
**
** Description      Returns the encoding enumeration (tBTA_MA_BMSG_ENCODING) that
**                      corresponds to the provided string.
**                             
** Parameters       psz - Input string.
**                  p_encoding - pointer to the encoding value to be received.
**                  
** Returns          TRUE if there is a match, otherwise FALSE.
**
*******************************************************************************/
BOOLEAN bta_ma_str_to_encoding(char * psz, tBTA_MA_BMSG_ENCODING * p_encoding)
{
    tBTA_MA_BMSG_ENCODING e;

    if ( psz && p_encoding )
    {
        for (e= BTA_MA_BMSG_ENC_8BIT; e < num_bmsg_body_encoding; e++)
        {
            if ( strcmp(psz, bmsg_body_encoding[e]) == 0 )
            {
                *p_encoding = e;
                return( TRUE );
            }
        }
    }

    return( FALSE );
}

/*******************************************************************************
**
** Function         bta_ma_str_to_language
**
** Description      Returns the language enumeration (tBTA_MA_BMSG_LANGUAGE) that
**                      corresponds to the provided string.
**                             
** Parameters       psz - Input string.
**                  p_language - pointer to the language value to be received.
**                  
** Returns          TRUE if there is a match, otherwise FALSE.
**
*******************************************************************************/
BOOLEAN bta_ma_str_to_language(char * psz, tBTA_MA_BMSG_LANGUAGE * p_language)
{
    tBTA_MA_BMSG_LANGUAGE l;

    if ( psz && p_language )
    {
        for (l=BTA_MA_BMSG_LANG_UNSPECIFIED; l < num_bmsg_body_language; l++)
        {
            if ( strcmp(psz, bmsg_body_language[l]) == 0 )
            {
                *p_language = l;
                return( TRUE );
            }
        }
    }

    return( FALSE );
}

/*******************************************************************************
**
** Function         bta_ma_str_to_msg_typ
**
** Description      Returns the message type enumeration (tBTA_MA_MSG_TYPE)
**                      that corresponds to the provided string.
**                             
** Parameters       psz - Input string.
**                  p_msg_type - pointer to the message type value to be 
**                      received.
**                  
** Returns          TRUE if there is a match, otherwise FALSE.
**
*******************************************************************************/
BOOLEAN bta_ma_str_to_msg_typ(char * psz, tBTA_MA_MSG_TYPE * p_msg_type)
{
    if ( psz && p_msg_type )
    {
        if ( strcmp(psz, "EMAIL") == 0 )
            *p_msg_type = BTA_MA_MSG_TYPE_EMAIL;
        else if ( strcmp(psz, "SMS_GSM") == 0 )
            *p_msg_type = BTA_MA_MSG_TYPE_SMS_GSM;
        else if ( strcmp(psz, "SMS_CDMA") == 0 )
            *p_msg_type = BTA_MA_MSG_TYPE_SMS_CDMA;
        else if ( strcmp(psz, "MMS") == 0 )
            *p_msg_type = BTA_MA_MSG_TYPE_MMS;
        else
            return FALSE;
    }

    return( TRUE );
}

/*******************************************************************************
**
** Function         bta_ma_get_body_length
**
** Description      Returns the combined length in characters of the message
**                      content.
**                             
** Parameters       p_body - pointer to bBody structure.
**                  
** Returns          Length of the body message text.
**
*******************************************************************************/
UINT32 bta_ma_get_body_length(tBTA_MA_BMSG_BODY * p_body)
{
    UINT32 length = 0, len=0;
    tBTA_MA_BMSG_CONTENT * p_content;
    char * p_text;

    APPL_TRACE_EVENT0("bta_ma_get_body_length");

    p_content = BTA_MaBmsgGetContentFromBody(p_body);

    while ( p_content )
    {
        p_text= BTA_MaBmsgGetMsgContent(p_content);

        while ( p_text )
        {
            len = strlen(p_text);
            length += (len + BTA_MA_BMSG_BODY_TAG_CTL_LENGTH); 

            APPL_TRACE_EVENT3("total=%d len=%d text=%s",length, len, p_text);

            p_text = BTA_MaBmsgGetNextMsgContent(p_content);
        }

        p_content = BTA_MaBmsgGetNextContent(p_content);
    }

    APPL_TRACE_EVENT1("bta_ma_get_body_length len=%d", length);
    return( length );


}

/*******************************************************************************
**
** Function      bta_ma_bmsg_free_vcards       
**
** Description   Free buffers used by vVards    
**                             
** Parameters    p_vcard - Pointer to the first vCard in the linked vCards     
**                  
** Returns       None   
**
*******************************************************************************/

void bta_ma_bmsg_free_vcards(tBTA_MA_BMSG_VCARD * p_vcard)
{
    int x;

    if ( p_vcard )
    {
        /* recursively free any linked vCards */
        bta_ma_bmsg_free_vcards((tBTA_MA_BMSG_VCARD *)p_vcard->p_next);

        /* Free properties */
        for (x=0; x < BTA_MA_VCARD_PROP_MAX; x++)
            bta_ma_bmsg_free_vcard_prop(p_vcard->p_prop[x]);

        /* free vcard structure itself */
        bta_ma_bmsg_free(p_vcard);
    }
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free_envelope       
**
** Description   Free buffers used by envelopes    
**                             
** Parameters    p_envelope - Pointer to the first envelope in the linked envelopes     
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free_envelope(tBTA_MA_BMSG_ENVELOPE * p_envelope)
{
    if ( p_envelope )
    {
        /* recursively free any linked envelopes */
        bta_ma_bmsg_free_envelope((tBTA_MA_BMSG_ENVELOPE *)p_envelope->p_next);

        /* free the body */
        bta_ma_bmsg_free_body(p_envelope->p_body);

        /* free recipients */
        bta_ma_bmsg_free_vcards(p_envelope->p_recip);

        /* free envelope structure itself */
        bta_ma_bmsg_free(p_envelope);
    }
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free_body       
**
** Description   Free buffers used by a message body    
**                             
** Parameters    p_body - Pointer to a message body   
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free_body(tBTA_MA_BMSG_BODY * p_body)
{
    if ( p_body )
    {
        bta_ma_bmsg_free_content(p_body->p_content);

        /* free body structure itself */
        bta_ma_bmsg_free(p_body);
    }
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free_content       
**
** Description   Free buffers used by message contents    
**                             
** Parameters    p_envelope - Pointer to the first message content in the 
**               linked message contents     
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free_content(tBTA_MA_BMSG_CONTENT * p_content)
{
    if ( p_content )
    {
        /* recursively free any linked content */
        bta_ma_bmsg_free_content((tBTA_MA_BMSG_CONTENT *)p_content->p_next);

        /* free all of the message text */
        bta_ma_bmsg_free_message_text(p_content->p_message);

        /* free content structure itself */
        bta_ma_bmsg_free(p_content);
    }
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free_message_text       
**
** Description   Free text string buffers used by a message    
**                             
** Parameters    p_envelope - Pointer to a message     
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free_message_text(tBTA_MA_BMSG_MESSAGE * p_message)
{
    tBTA_MA_BMSG_MESSAGE * p_temp;

    while ( p_message )
    {
        p_temp = (tBTA_MA_BMSG_MESSAGE *)p_message->p_next;

        /* free message string */
        bta_ma_bmsg_free(p_message->p_text);

        /* free message text structure */
        bta_ma_bmsg_free(p_message);

        /* now point to the next one */
        p_message = p_temp;
    }
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free_vcard_prop       
**
** Description   Free buffers used by a vCard property    
**                             
** Parameters    p_envelope - Pointer to a vCard property     
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free_vcard_prop(tBTA_MA_VCARD_PROPERTY * p_prop)
{
    if ( p_prop )
    {
        /* free the value and the parameter */
        if ( p_prop->p_value )
            bta_ma_bmsg_free(p_prop->p_value);

        if ( p_prop->p_param )
            bta_ma_bmsg_free(p_prop->p_param);

        /* recursively free any linked content */
        if ( p_prop->p_next )
            bta_ma_bmsg_free_vcard_prop((tBTA_MA_VCARD_PROPERTY *)p_prop->p_next);

        /* free property structure itself */
        bta_ma_bmsg_free(p_prop);
    }
}

/*******************************************************************************
**
** Function      bta_ma_bmsg_alloc       
**
** Description   Allocate buffer for the specified size    
**                             
** Parameters    cb - request buffer size   
**                  
** Returns       None   
**
*******************************************************************************/

void * bta_ma_bmsg_alloc(size_t cb) 
{
    void * p_buf;

    if ((p_buf = GKI_getbuf((UINT16) cb)) == NULL )
    {
        APPL_TRACE_ERROR1("Unable to allocate buffer for size=%", (UINT16) cb);
    }
    return(p_buf);
}
/*******************************************************************************
**
** Function      bta_ma_bmsg_free       
**
** Description   Free buffer   
**                             
** Parameters    p - pointer to a buffer    
**                  
** Returns       None   
**
*******************************************************************************/
void bta_ma_bmsg_free(void * p)
{
    if ( p )
        GKI_freebuf(p);
}


#endif /* BTA_MSE_INCLUDED */
