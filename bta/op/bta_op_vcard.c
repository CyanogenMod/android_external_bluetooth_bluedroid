/*****************************************************************************
**
**  Name:           bta_op_vcard.c
**
**  Description:    This file contains functions for parsing and building
**                  vCard objects.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_op_api.h"
#include "bta_op_fmt.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define BTA_OP_VCARD_BEGIN_LEN      26
#define BTA_OP_VCARD_END_LEN        11

#define BTA_OP_VCARD_MIN_LEN        (BTA_OP_VCARD_BEGIN_LEN + BTA_OP_VCARD_END_LEN)

const char bta_op_vcard_prs_begin[] = "BEGIN:VCARD\r\n";

const char bta_op_vcard_21_begin[] = "BEGIN:VCARD\r\nVERSION:2.1\r\n";

const char bta_op_vcard_30_begin[] = "BEGIN:VCARD\r\nVERSION:3.0\r\n";

const char bta_op_vcard_end[] = "END:VCARD\r\n";

const tBTA_OP_PARAM bta_op_vcard_adr[] =
{
    {NULL,       6},          /* Number of elements in array */
    {"DOM",      4},          /* BTA_OP_ADR_DOM */
    {"INTL",     5},          /* BTA_OP_ADR_INTL */
    {"POSTAL",   7},          /* BTA_OP_ADR_POSTAL */
    {"PARCEL",   7},          /* BTA_OP_ADR_PARCEL */
    {"HOME",     5},          /* BTA_OP_ADR_HOME */
    {"WORK",     5}           /* BTA_OP_ADR_WORK */
};

const tBTA_OP_PARAM bta_op_vcard_email[] =
{
    {NULL,       3},          /* Number of elements in array */
    {"PREF",     5},          /* BTA_OP_EMAIL_PREF */
    {"INTERNET", 9},          /* BTA_OP_EMAIL_INTERNET */
    {"X400",     5}           /* BTA_OP_EMAIL_X400 */
};

const tBTA_OP_PARAM bta_op_vcard_tel[] =
{
    {NULL,       8},          /* Number of elements in array */
    {"PREF",     5},          /* BTA_OP_TEL_PREF */
    {"WORK",     5},          /* BTA_OP_TEL_WORK */
    {"HOME",     5},          /* BTA_OP_TEL_HOME */
    {"VOICE",    6},          /* BTA_OP_TEL_VOICE */
    {"FAX",      4},          /* BTA_OP_TEL_FAX */
    {"MSG",      4},          /* BTA_OP_TEL_MSG */
    {"CELL",     5},          /* BTA_OP_TEL_CELL */
    {"PAGER",    6}           /* BTA_OP_TEL_PAGER */
};

const tBTA_OP_PARAM bta_op_vcard_photo[] =
{
    {NULL,       4},          /* Number of elements in array */
    {"VALUE=URI", 10},        /* BTA_OP_PHOTO_VALUE_URI */
    {"VALUE=URL", 10},        /* BTA_OP_PHOTO_VALUE_URL */
    {"JPEG",     5},          /* BTA_OP_PHOTO_TYPE_JPEG */
    {"GIF",      4}           /* BTA_OP_PHOTO_TYPE_GIF */
};

const tBTA_OP_PARAM bta_op_vcard_sound[] =
{
    {NULL,       4},          /* Number of elements in array */
    {"VALUE=URI", 10},        /* BTA_OP_SOUND_VALUE_URI */
    {"VALUE=URL", 10},        /* BTA_OP_SOUND_VALUE_URL */
    {"BASIC",     6},         /* BTA_OP_SOUND_TYPE_BASIC */
    {"WAVE",      5}          /* BTA_OP_SOUND_TYPE_WAVE */
};

const tBTA_OP_PROP_TBL bta_op_vcard_tbl[] =
{
    {NULL,          NULL,               15},    /* Number of elements in array */
    {"ADR",         bta_op_vcard_adr,   6},    /* BTA_OP_VCARD_ADR */
    {"EMAIL",       bta_op_vcard_email, 8},    /* BTA_OP_VCARD_EMAIL */
    {"FN",          NULL,               5},    /* BTA_OP_VCARD_FN */
    {"NOTE",        NULL,               7},    /* BTA_OP_VCARD_NOTE */
    {"NICKNAME",    NULL,               11},   /* BTA_OP_VACRD_NICKNAME */
    {"N",           NULL,               4},    /* BTA_OP_VCARD_N */
    {"ORG",         NULL,               6},    /* BTA_OP_VCARD_ORG */
    {"TEL",         bta_op_vcard_tel,   6},    /* BTA_OP_VCARD_TEL */
    {"TITLE",       NULL,               8},    /* BTA_OP_VCARD_TITLE */
    {"URL",         NULL,               6},    /* BTA_OP_VCARD_URL */
    {"X-IRMC-LUID", NULL,               14},   /* BTA_OP_VNOTE_LUID */
    {"BDAY",        NULL,               7},    /* BTA_OP_VCARD_BDAY */
    {"PHOTO",       bta_op_vcard_photo, 8},    /* BTA_OP_VCARD_PHOTO */
    {"SOUND",       bta_op_vcard_sound, 8},    /* BTA_OP_VCARD_SOUND */
    {"X-IRMC-CALL-DATETIME", NULL,      23}    /* BTA_OP_VCARD_CALL */
};

const UINT32 bta_op_vcard_prop_filter_mask[] =
{
/* table index should be the same as the prop table above */
    BTA_OP_FILTER_ALL,
    BTA_OP_FILTER_ADR,
    BTA_OP_FILTER_EMAIL,
    BTA_OP_FILTER_FN,
    BTA_OP_FILTER_NOTE,
    BTA_OP_FILTER_NICKNAME,
    BTA_OP_FILTER_N,
    BTA_OP_FILTER_ORG,
    BTA_OP_FILTER_TEL,
    BTA_OP_FILTER_TITLE,
    BTA_OP_FILTER_URL,
    BTA_OP_FILTER_UID,
    BTA_OP_FILTER_BDAY,
    BTA_OP_FILTER_PHOTO,
    BTA_OP_FILTER_SOUND,
    BTA_OP_FILTER_TIME_STAMP
};

const tBTA_OP_PROP_FILTER_MASK_TBL bta_op_vcard_prop_filter_mask_tbl =
{
    bta_op_vcard_prop_filter_mask,
    15
};

const tBTA_OP_OBJ_TBL bta_op_vcard_21_bld =
{
    bta_op_vcard_tbl,
    BTA_OP_FMT_VCARD21,
    bta_op_vcard_21_begin,
    bta_op_vcard_end,
    BTA_OP_VCARD_BEGIN_LEN,
    BTA_OP_VCARD_END_LEN,
    BTA_OP_VCARD_MIN_LEN,
    &bta_op_vcard_prop_filter_mask_tbl
};

const tBTA_OP_OBJ_TBL bta_op_vcard_30_bld =
{
    bta_op_vcard_tbl,
    BTA_OP_FMT_VCARD30,
    bta_op_vcard_30_begin,
    bta_op_vcard_end,
    BTA_OP_VCARD_BEGIN_LEN,
    BTA_OP_VCARD_END_LEN,
    BTA_OP_VCARD_MIN_LEN,
    &bta_op_vcard_prop_filter_mask_tbl
};

const tBTA_OP_OBJ_TBL bta_op_vcard_21_prs =
{
    bta_op_vcard_tbl,
    BTA_OP_FMT_VCARD21,
    bta_op_vcard_prs_begin,
    bta_op_vcard_end,
    BTA_OP_VCARD_BEGIN_LEN,
    BTA_OP_VCARD_END_LEN,
    BTA_OP_VCARD_MIN_LEN,
    NULL
};

const tBTA_OP_OBJ_TBL bta_op_vcard_30_prs =
{
    bta_op_vcard_tbl,
    BTA_OP_FMT_VCARD30,
    bta_op_vcard_prs_begin,
    bta_op_vcard_end,
    BTA_OP_VCARD_BEGIN_LEN,
    BTA_OP_VCARD_END_LEN,
    BTA_OP_VCARD_MIN_LEN,
    NULL
};


/*******************************************************************************
**
** Function         bta_op_get_card_fmt
**
** Description      Finds the vCard format contained in buffer pointed by p_card
**                  
**
** Returns          Vcard Format BTA_OP_VCARD21_FMT/BTA_OP_VCARD30_FMT else
**                  BTA_OP_OTHER_FMT
**
*******************************************************************************/
static tBTA_OP_SUP_FMT bta_op_get_card_fmt(UINT8 *p_data, UINT16 len)
{
    UINT8           *p_end = p_data + len;
    
    if (bta_op_scanstr(p_data, p_end, bta_op_vcard_21_begin) != NULL)
    {
        return BTA_OP_FMT_VCARD21;
    }
    else if (bta_op_scanstr(p_data, p_end, bta_op_vcard_30_begin) != NULL)
    {
        return BTA_OP_FMT_VCARD30;
    }
    else
    {
        return BTA_OP_FMT_NONE;
    }
}

/*******************************************************************************
**
** Function         BTA_OpBuildCard
**
** Description      Build a vCard object.  The input to this function is
**                  requested format(2.1/3.0), an array of vCard properties 
**                  and a pointer to memory to store the card.
**                  The output is a formatted vCard.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpBuildCard(UINT8 *p_card, UINT16 *p_len, tBTA_OP_FMT fmt,
                               tBTA_OP_PROP *p_prop, UINT8 num_prop)
{
    if(fmt == BTA_OP_VCARD21_FMT)
    {
        return bta_op_build_obj(&bta_op_vcard_21_bld, p_card, p_len, p_prop, num_prop);
    }
    else if(fmt == BTA_OP_VCARD30_FMT)
    {
        return bta_op_build_obj(&bta_op_vcard_30_bld, p_card, p_len, p_prop, num_prop);
    }
    else
    {
        *p_len = 0;
        return BTA_OP_FAIL;
    }
}

/*******************************************************************************
**
** Function         BTA_OpSetCardPropFilterMask
**
** Description      Set Property Filter Mask
**                  
**
** Returns          
**
*******************************************************************************/
void BTA_OpSetCardPropFilterMask(UINT32 mask)
{
    bta_op_set_prop_filter_mask(mask);
    return;
}

/*******************************************************************************
**
** Function         BTA_OpParseCard
**
** Description      Parse a vCard 2.1 object.  The input to this function is
**                  a pointer to vCard data.  The output is an array of parsed
**                  vCard properties.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpParseCard(tBTA_OP_PROP *p_prop, UINT8 *p_num_prop,
                               UINT8 *p_card, UINT16 len)
{
    tBTA_OP_SUP_FMT fmt = bta_op_get_card_fmt(p_card, len);

    if (fmt == BTA_OP_FMT_VCARD21)
    {
        return bta_op_parse_obj(&bta_op_vcard_21_prs, p_prop, p_num_prop, p_card, len);
    }
    else if(fmt == BTA_OP_FMT_VCARD30)
    {
        return bta_op_parse_obj(&bta_op_vcard_30_prs, p_prop, p_num_prop, p_card, len);
    }
    else
    {
        *p_num_prop = 0;
        return BTA_OP_FAIL;
    }
}

/*******************************************************************************
**
** Function         BTA_OpGetCardProperty
**
** Description      Get Card property value by name.  The input to this function is
**                  property name.  The output is property value and len
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpGetCardProperty(UINT8 *p_value, UINT16 *p_len, tBTA_OP_PROP *p_prop, 
                                     UINT8 num_prop, UINT8 *p_name)
{
    return bta_op_get_property_by_name(bta_op_vcard_tbl, p_name, 
                                       p_prop, num_prop, p_value, p_len);
}


