/*****************************************************************************
**
**  Name:           bta_op_fmt.c
**
**  Description:    This file contains common functions and data structures
**                  used by the OPP object formatting functions.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "bta_op_api.h"
#include "bta_op_fmt.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define BTA_OP_ENCODING_LEN         9
#define BTA_OP_CHARSET_LEN          8
#define BTA_OP_PARAM_TYPE_HDR_LEN   5
#define BTA_OP_NUM_NONINLINE_MEDIA  2

const char bta_op_encoding[] = "ENCODING=";

const tBTA_OP_PARAM bta_op_encodings[] =
{/* the len is (BTA_OP_ENCODING_LEN + 1 + strlen(p_name)) */
    {NULL,                   0},
    {"QUOTED-PRINTABLE",    26},    /* BTA_OP_ENC_QUOTED_PRINTABLE */
    {"8BIT",                14},    /* BTA_OP_ENC_8BIT */
    {"b",                   11},    /* BTA_OP_ENC_BINARY */
// btla-specific ++
    {"BASE64",              16}     /* BTA_OP_ENC_BASE64 */
// btla-specific --
};

#define BTA_OP_NUM_ENCODINGS    3
#define BTA_OP_QP_IDX           1    /* array index for quoted printable in bta_op_encodings */

const char bta_op_charset[] = "CHARSET=";

const char bta_op_param_type_hdr[] = "TYPE=";

const tBTA_OP_PARAM bta_op_charsets[] =
{/* the len is (BTA_OP_CHARSET_LEN + 1 + strlen(p_name)) */
    {NULL,               0},
    {"BIG5",            13},   /* BTA_OP_CHAR_BIG5 */
    {"EUC-JP",          15},   /* BTA_OP_CHAR_EUC_JP */
    {"EUC-KR",          15},   /* BTA_OP_CHAR_EUC_KR */
    {"GB2312",          15},   /* BTA_OP_CHAR_GB2312 */
    {"ISO-2022-JP",     20},   /* BTA_OP_CHAR_ISO_2022_JP */
    {"ISO-8859-1",      19},   /* BTA_OP_CHAR_ISO_8859_1 */
    {"ISO-8859-2",      19},   /* BTA_OP_CHAR_ISO_8859_2 */
    {"ISO-8859-3",      19},   /* BTA_OP_CHAR_ISO_8859_3 */
    {"ISO-8859-4",      19},   /* BTA_OP_CHAR_ISO_8859_4 */
    {"ISO-8859-5",      19},   /* BTA_OP_CHAR_ISO_8859_5 */
    {"ISO-8859-6",      19},   /* BTA_OP_CHAR_ISO_8859_6 */
    {"ISO-8859-7",      19},   /* BTA_OP_CHAR_ISO_8859_7 */
    {"ISO-8859-8",      19},   /* BTA_OP_CHAR_ISO_8859_8 */
    {"KOI8-R",          15},   /* BTA_OP_CHAR_KOI8_R */
    {"SHIFT_JIS",       18},   /* BTA_OP_CHAR_SHIFT_JIS */
    {"UTF-8",           14}    /* BTA_OP_CHAR_UTF_8 */
};

#define BTA_OP_NUM_CHARSETS     16

/* Structure of the 32-bit parameters mask:
** (same comment is in bta_op_api.h)
**                  + property-specific
** +reserved        |        + character set
** |                |        |     + encoding
** |                |        |     |
** 0000000000000000 00000000 00000 000
*/
#define BTA_OP_GET_PARAM(param, encod, charset, specific) \
    encod = (UINT8) (param) & 0x00000007; \
    charset = (UINT8) ((param) >> 3) & 0x0000001F; \
    specific = (UINT8) ((param) >> 8) & 0x000000FF;

/* mask for properties default 0, filter all */
static UINT32 bta_op_prop_filter_mask = 0;

const tBTA_OP_PROP_MEDIA bta_op_media[] =
{
    {NULL,                   2},
    {"PHOTO",                5},
    {"SOUND",                5}
};

/* Place holder constant for safe string functions since there's no way to know how
** memory is remaining for input parameter to build property.
**  Note:  The BCM_STRCPY_S functions should be changed to know how much memory 
**      is remaining.  When completed this constant can be removed.  Also, if safe
**      string functions are not used then this parameter is ignored anyway!
*/
#ifndef BTA_OP_REM_MEMORY
#define BTA_OP_REM_MEMORY   8228
#endif

/*******************************************************************************
**
** Function         bta_op_strnicmp
**
** Description      Case insensitive strncmp.
**                  
**
** Returns          void
**
*******************************************************************************/
INT16 bta_op_strnicmp(const char *pStr1, const char *pStr2, size_t Count)
{
    char c1, c2;
    INT16 v;

    if (Count == 0)
        return 0;

    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* the casts are necessary when pStr1 is shorter & char is signed */
        v = (UINT16) tolower(c1) - (UINT16) tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (--Count > 0));

    return v;
}

/*******************************************************************************
**
** Function         bta_op_set_prop_mask
**
** Description      Set property mask
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_op_set_prop_filter_mask(UINT32 mask)
{
    bta_op_prop_filter_mask = mask;
    return;
}

/*******************************************************************************
**
** Function         bta_op_prop_len
**
** Description      Calculate the length of a property string through lookup
**                  tables.
**                  
**
** Returns          Length of string in bytes.
**
*******************************************************************************/
UINT16 bta_op_prop_len(const tBTA_OP_PROP_TBL *p_tbl, tBTA_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    UINT8   len_s = 0;
    int     i;

    /* parse parameters mask */
    BTA_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

    /* calculate length of property-specific parameters, if any */
    if (p_tbl[p_prop->name].p_param_tbl != NULL)
    {
        for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
        {
            if (i_s & 1)
            {
                len_s += p_tbl[p_prop->name].p_param_tbl[i].len;
            }
            i_s >>= 1;
        }
    }
    
    return (p_tbl[p_prop->name].len + len_s + bta_op_charsets[i_c].len + 
            bta_op_encodings[i_e].len + p_prop->len);
}

/*******************************************************************************
**
** Function         bta_op_param_conflict
**
** Description      Check if the parameters of the property for the format
**                  conflict/not allowed.
**                  
**
** Returns          TRUE if not allowed, else FALSE
**
*******************************************************************************/
BOOLEAN bta_op_param_conflict(tBTA_OP_SUP_FMT fmt, tBTA_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    BOOLEAN conflict = FALSE;

    if (p_prop->parameters != 0)
    {
        BTA_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

        if (fmt == BTA_OP_FMT_VCARD30)
        {
            /* VCard 3.0 does not support CHARSET. If it is present, we
               should ignore the whole property in the vCard build process */
            if (bta_op_charsets[i_c].p_name != NULL)
                conflict = TRUE;

            /* VCard 3.0 only supports 'b' encoding. If any other encoding
               ex. quoted-printable, is present, then whole property is ignored */
            if ((bta_op_encodings[i_e].p_name != NULL) && strcmp(bta_op_encodings[i_e].p_name, "b"))
                conflict = TRUE;
        }
    }

    return conflict;
}
/*******************************************************************************
**
** Function         bta_op_add_param
**
** Description      Add parameter strings to a property string.
**                  
**
** Returns          Pointer to the next byte after the end of the string.
**
*******************************************************************************/
UINT8 *bta_op_add_param(tBTA_OP_SUP_FMT fmt, const tBTA_OP_PROP_TBL *p_tbl, UINT8 *p, tBTA_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    int     i;
    UINT8   bta_op_param_type_delimit = 0;

    if (p_prop->parameters != 0)
    {
        BTA_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

        /* add encoding parameter */
        if (bta_op_encodings[i_e].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_encoding);
            p += BTA_OP_ENCODING_LEN;
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_encodings[i_e].p_name);
            p += bta_op_encodings[i_e].len - BTA_OP_ENCODING_LEN - 1;
        }

        /* add character set parameter */
        if (bta_op_charsets[i_c].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_charset);
            p += BTA_OP_CHARSET_LEN;
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_charsets[i_c].p_name);
            p += bta_op_charsets[i_c].len - BTA_OP_CHARSET_LEN - 1;
        }

        /* add any property-specific parameters */
        if (p_tbl[p_prop->name].p_param_tbl != NULL)
        {
            for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
            {
                if (i_s & 1)
                {
                    if (bta_op_param_type_delimit)
                    {
                        *p++ = ',';
                    }
                    else
                    {
                        *p++ = ';';

                        if (fmt == BTA_OP_FMT_VCARD30)
                        {
                            BCM_STRNCPY_S((char *) p, BTA_OP_PARAM_TYPE_HDR_LEN+1, bta_op_param_type_hdr, BTA_OP_PARAM_TYPE_HDR_LEN);
                            p += BTA_OP_PARAM_TYPE_HDR_LEN;
                            bta_op_param_type_delimit++;
                        }
                    }
                    BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, p_tbl[p_prop->name].p_param_tbl[i].p_name);
                    p += p_tbl[p_prop->name].p_param_tbl[i].len - 1;
                }
                i_s >>= 1;
            }
        }
    }
    else if (p_prop->p_param)
    {
        *p++ = ';';
        memcpy((char *) p, p_prop->p_param, p_prop->param_len);
        p += p_prop->param_len;
    }

    return p;
}

/*******************************************************************************
**
** Function         bta_op_add_media_param
**
** Description      Add parameter strings to a media property string.
**                  
**
** Returns          Pointer to the next byte after the end of the string.
**
*******************************************************************************/
UINT8 *bta_op_add_media_param(const tBTA_OP_PROP_TBL *p_tbl, UINT8 *p, tBTA_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    int     i;

    if (p_prop->parameters != 0)
    {
        BTA_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

        /* add encoding parameter */
        if (bta_op_encodings[i_e].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_encoding);
            p += BTA_OP_ENCODING_LEN;
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, bta_op_encodings[i_e].p_name);
            p += bta_op_encodings[i_e].len - BTA_OP_ENCODING_LEN - 1;
        }

        /* add any property-specific parameters */
        if (p_tbl[p_prop->name].p_param_tbl != NULL)
        {
            for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
            {
                if (i_s & 1)
                {
                    *p++ = ';';

                    /* Add "TYPE=" to non-referenced (inline) media */
                    if (i > BTA_OP_NUM_NONINLINE_MEDIA)
                    {
                        BCM_STRNCPY_S((char *) p, BTA_OP_PARAM_TYPE_HDR_LEN+1, bta_op_param_type_hdr, BTA_OP_PARAM_TYPE_HDR_LEN);
                        p += BTA_OP_PARAM_TYPE_HDR_LEN;
                    }

                    BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, p_tbl[p_prop->name].p_param_tbl[i].p_name);
                    p += p_tbl[p_prop->name].p_param_tbl[i].len - 1;
                    break;
                }
                i_s >>= 1;
            }
        }
    }
    else if (p_prop->p_param)
    {
        *p++ = ';';
        memcpy((char *) p, p_prop->p_param, p_prop->param_len);
        p += p_prop->param_len;
    }

    return p;
}

/*******************************************************************************
**
** Function         bta_op_get_property_by_name
**
** Description      Get the property user data by name.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if there is no property user data.
**
*******************************************************************************/
tBTA_OP_STATUS bta_op_get_property_by_name(const tBTA_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tBTA_OP_PROP *p_prop, UINT8 num_prop, UINT8 *p_data,
                                UINT16 *p_len)
{
    int             i, j;
    tBTA_OP_STATUS  result = BTA_OP_FAIL;

    /* for each property */
    for(i = 0; num_prop != 0; num_prop--, i++)
    {
            /* verify property is valid */
        if ((p_prop[i].name == 0) || (p_prop[i].name > p_tbl[0].len))
        {
             result = BTA_OP_FAIL;
             break;
        }

        j = p_prop[i].name;

        if (bta_op_strnicmp(p_tbl[j].p_name, (char *) p_name,
                (p_tbl[j].len - BTA_OP_PROP_OVHD)) == 0)
        {
            memcpy(p_data, p_prop[i].p_data, p_prop[i].len);
            *p_len = p_prop[i].len;
            result = BTA_OP_OK;
            break;
        }
    }
    return result;
}

/*******************************************************************************
**
** Function         bta_op_build_obj
**
** Description      Build an object from property data supplied by the user.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tBTA_OP_STATUS bta_op_build_obj(const tBTA_OP_OBJ_TBL *p_bld, UINT8 *p_data,
                                UINT16 *p_len, tBTA_OP_PROP *p_prop, UINT8 num_prop)
{
    int             i,j;
    tBTA_OP_STATUS  result = BTA_OP_OK;
    UINT8           *p = p_data;

    /* sanity check length */
    if (*p_len < p_bld->min_len)
    {
        result = BTA_OP_MEM;
    }
    else
    {
        /* adjust p_len to amount of free space minus start and end */
        *p_len -= p_bld->min_len;

        /* add begin, version */
        BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, p_bld->p_begin_str);
        p += p_bld->begin_len;

        /* for each property */
        for(i = 0; num_prop != 0; num_prop--, i++)
        {
            /* verify property is valid */
            if ((p_prop[i].name == 0) || (p_prop[i].name > p_bld->p_tbl[0].len))
            {
                result = BTA_OP_FAIL;
                break;
            }

            /* verify property will fit */
            if (bta_op_prop_len(p_bld->p_tbl, &p_prop[i]) > (p_data + p_bld->begin_len + *p_len - p))
            {
                result = BTA_OP_MEM;
                break;
            }

            /* check for filter */
            if (bta_op_prop_filter_mask !=0 && p_bld->p_prop_filter_mask_tbl != NULL && 
                p_prop[i].name <= p_bld->p_prop_filter_mask_tbl->len)
            {
                if (!(bta_op_prop_filter_mask & p_bld->p_prop_filter_mask_tbl->p_prop_filter_mask[p_prop[i].name]))
                    continue;
            }

            /* Check if the combination of parameters are allowed for
               the format we are building */
            if (bta_op_param_conflict(p_bld->fmt, &p_prop[i]))
                continue;

            /* add property string */
            BCM_STRCPY_S((char *) p, BTA_OP_REM_MEMORY, p_bld->p_tbl[p_prop[i].name].p_name);
            p += p_bld->p_tbl[p_prop[i].name].len - BTA_OP_PROP_OVHD;

            for (j= bta_op_media[0].len; j > 0; j--)
            {
                if (!bta_op_strnicmp(bta_op_media[j].media_name, p_bld->p_tbl[p_prop[i].name].p_name, bta_op_media[j].len))
                {
                    p = bta_op_add_media_param(p_bld->p_tbl, p, &p_prop[i]);
                    break;
                }
            }

            if (!j)
            {
                /* add property parameters */
                p = bta_op_add_param(p_bld->fmt, p_bld->p_tbl, p, &p_prop[i]);
            }

            /* add user data */
// btla-specific ++
            if(p_prop[i].name != BTA_OP_VCARD_CALL) 	// If the data is call date-time, use ';' instead of ':'
		    {
	            *p++ = ':';
		    }
		    else 
		    {
	            *p++ = ';';   
		    }
// btla-specific --

            memcpy(p, p_prop[i].p_data, p_prop[i].len);
            p += p_prop[i].len;
            *p++ = '\r';
            *p++ = '\n';
        }

        /* add in end */
        memcpy(p, p_bld->p_end_str, p_bld->end_len);
        p += p_bld->end_len;
    }

    *p_len = (UINT16) (p - p_data);
    return result;
}

/*******************************************************************************
**
** Function         bta_op_nextline
**
** Description      Scan to beginning of next property text line.
**                  
**
** Returns          Pointer to beginning of property or NULL if end of
**                  data reached.
**
*******************************************************************************/
static UINT8 *bta_op_nextline(UINT8 *p, UINT8 *p_end, BOOLEAN qp)
{
// btla-specific ++
    if (*p == '\r')
    {
        p--;
    }
// btla-specific --

    if ((p_end - p) > 3)
    {
        p_end -= 3;
        while (p < p_end)
        {
            if (*(++p) == '\r')
            {
                if (*(p + 1) == '\n')
                {
                    if (qp)
                    {
                        if (*(p - 1) == '=')
                        {
                            /* this is a soft break for quoted-printable*/
                            continue;
                        }
                    }

                    if ((*(p + 2) != ' ') && (*(p + 2) != '\t'))
                    {
                        return(p + 2);
                    }
                }
            }
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         bta_op_scantok
**
** Description      Scan a line for one or more tokens.
**                  
**
** Returns          Pointer to token or NULL if end of data reached.
**
*******************************************************************************/
static UINT8 *bta_op_scantok(UINT8 *p, UINT8 *p_end, const char *p_tok)
{
    int     i;
    UINT8   num_tok = strlen(p_tok);

    for (; p < p_end; p++)
    {
        for (i = 0; i < num_tok; i++)
        {
            if (*p == p_tok[i])
            {
                return p;
            }
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         bta_op_scanstr
**
** Description      Scan for a matching string.
**                  
**
** Returns          Pointer to end of match or NULL if end of data reached.
**
*******************************************************************************/
UINT8 *bta_op_scanstr(UINT8 *p, UINT8 *p_end, const char *p_str)
{
    int len = strlen(p_str);

    for (;;)
    {
        /* check for match */
        if (strncmp((char *) p, p_str, len) == 0)
        {
            p += len;
            break;
        }
        /* no match; skip to next line, checking for end */
        else if ((p = bta_op_nextline(p, p_end, FALSE)) == NULL)
        {
            break;
        }
    }
    return p;
}

/*******************************************************************************
**
** Function         bta_op_parse_obj
**
** Description      Parse an object.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid data.
**                  BTA_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tBTA_OP_STATUS bta_op_parse_obj(const tBTA_OP_OBJ_TBL *p_prs, tBTA_OP_PROP *p_prop,
                                UINT8 *p_num_prop, UINT8 *p_data, UINT16 len)
{
    UINT32          j;
    UINT8           *p_s, *p_e;
    tBTA_OP_STATUS  result = BTA_OP_OK;
    UINT8           max_prop = *p_num_prop;
    UINT8           *p_end = p_data + len;
    UINT8           prop_name = 0;
    BOOLEAN         qp=FALSE;

    *p_num_prop = 0;

    /* sanity check length */
    if (len < p_prs->min_len)
    {
        return BTA_OP_FAIL;
    }

    /* find beginning */
    if ((p_s = bta_op_scanstr(p_data, p_end, p_prs->p_begin_str)) == NULL)
    {
        return BTA_OP_FAIL;
    }

    while (*p_num_prop < max_prop)
    {
        /* scan for next delimiter */
        if ((p_e = bta_op_scantok(p_s, p_end, ".;:")) == NULL)
        {
            break;
        }

        /* deal with grouping delimiter */
        if (*p_e == '.')
        {
            p_s = p_e + 1;
            if ((p_e = bta_op_scantok(p_s, p_end, ";:")) == NULL)
            {
                break;
            }
        }

        /* we found a property; see if it matches anything in our list */
        for (j = 1; j <= p_prs->p_tbl[0].len; j++)
        {
            if (bta_op_strnicmp(p_prs->p_tbl[j].p_name, (char *) p_s,
                (p_prs->p_tbl[j].len - BTA_OP_PROP_OVHD)) == 0)
            {
                p_prop[*p_num_prop].name = prop_name = j;
                p_prop[*p_num_prop].parameters = 0;
                break;
            }
        }

        /* if not in our list we can't parse it; continue */
        if (j > p_prs->p_tbl[0].len)
        {
            if ((p_s = bta_op_nextline(p_e, p_end, FALSE)) == NULL)
            {
                break;
            }
            continue;
        }

        /* now parse out all the parameters */
        if (*p_e == ';')
        {
            while ((*p_e == ';') || (*p_e == ','))
            {
                p_s = p_e + 1;

                if ((p_e = bta_op_scantok(p_s, p_end, ",;:")) == NULL)
                {
                    break;
                }

                /* we found a parameter; see if it matches anything */

                /* check for encoding */
                qp = FALSE;
                if (bta_op_strnicmp(bta_op_encoding, (char *) p_s, BTA_OP_ENCODING_LEN) == 0)
                {
                    p_s += BTA_OP_ENCODING_LEN;
                    for (j = 1; j <= BTA_OP_NUM_ENCODINGS; j++)
                    {
                        if (bta_op_strnicmp(bta_op_encodings[j].p_name, (char *) p_s,
                            (bta_op_encodings[j].len - BTA_OP_ENCODING_LEN - 1)) == 0)
                        {
                            if (j == BTA_OP_QP_IDX)
                            {
                                /* encoding = quoted-printable*/
                                qp= TRUE;
                            }
                            p_prop[*p_num_prop].parameters |= j;
                            break;
                        }
                    }
                }
                /* check for charset */
                else if (bta_op_strnicmp(bta_op_charset, (char *) p_s, BTA_OP_CHARSET_LEN) == 0)
                {
                    p_s += BTA_OP_CHARSET_LEN;
                    for (j = 1; j <= BTA_OP_NUM_CHARSETS; j++)
                    {
                        if (bta_op_strnicmp(bta_op_charsets[j].p_name, (char *) p_s,
                            (bta_op_charsets[j].len - BTA_OP_CHARSET_LEN - 1)) == 0)
                        {
                            p_prop[*p_num_prop].parameters |= j << 3;
                            break;
                        }
                    }
                }
                /* check for property-specific parameters */
                else if (p_prs->p_tbl[prop_name].p_param_tbl != NULL)
                {
                
                    /* Check for "TYPE=" */
                    if (!bta_op_strnicmp((char *)p_s, bta_op_param_type_hdr, BTA_OP_PARAM_TYPE_HDR_LEN))
                    {
                        p_s += BTA_OP_PARAM_TYPE_HDR_LEN;
                    }

                    for (j = p_prs->p_tbl[prop_name].p_param_tbl[0].len; j > 0; j--)
                    {
                        if (bta_op_strnicmp(p_prs->p_tbl[prop_name].p_param_tbl[j].p_name,
                            (char *) p_s, (p_prs->p_tbl[prop_name].p_param_tbl[j].len - 1)) == 0)
                        {
                            p_prop[*p_num_prop].parameters |= ((UINT32) 1) << (j + 7);
                            break;
                        }
                    }
                }
                else
                {
                    /* if this the start of the param */
                    if (!p_prop[*p_num_prop].p_param)
                        p_prop[*p_num_prop].p_param = p_s;
                    if (*p_e == ':')
                        p_prop[*p_num_prop].param_len += (p_e - p_s);
                    else
                        p_prop[*p_num_prop].param_len += (p_e - p_s + 1);              
                }
            }
        }

        if (p_e == NULL)
        {
            break;
        }

        /* go to start of next property */
        p_s = p_e + 1;
        if ((p_e = bta_op_nextline(p_s, p_end, qp)) == NULL)
        {
            break;
        }

        /* save property info */
        p_prop[*p_num_prop].p_data = p_s;
        p_prop[*p_num_prop].len = (UINT16) (p_e - p_s - 2);
// btla-specific ++
        if (p_prop[*p_num_prop].len)
        {
	        (*p_num_prop)++;
        }
// btla-specific --
        p_s = p_e;
    }
    return result;
}

