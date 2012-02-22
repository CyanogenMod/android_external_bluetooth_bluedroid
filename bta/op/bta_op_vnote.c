/*****************************************************************************
**
**  Name:           bta_op_vnote.c
**
**  Description:    This file contains functions for parsing and building
**                  vNote objects.
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

#define BTA_OP_VNOTE_BEGIN_LEN      26
#define BTA_OP_VNOTE_END_LEN        11

#define BTA_OP_VNOTE_MIN_LEN        (BTA_OP_VNOTE_BEGIN_LEN + BTA_OP_VNOTE_END_LEN)

const char bta_op_vnote_prs_begin[] = "BEGIN:VNOTE\r\n";

const char bta_op_vnote_begin[] = "BEGIN:VNOTE\r\nVERSION:1.1\r\n";

const char bta_op_vnote_end[] = "END:VNOTE\r\n";

const tBTA_OP_PROP_TBL bta_op_vnote_tbl[] =
{
    {NULL,          NULL,   2},    /* Number of elements in array */
    {"BODY",        NULL,   7},    /* BTA_OP_VNOTE_BODY */
    {"X-IRMC-LUID", NULL,   14}    /* BTA_OP_VNOTE_LUID */
};

const tBTA_OP_OBJ_TBL bta_op_vnote_bld =
{
    bta_op_vnote_tbl,
    BTA_OP_FMT_VNOTE11,
    bta_op_vnote_begin,
    bta_op_vnote_end,
    BTA_OP_VNOTE_BEGIN_LEN,
    BTA_OP_VNOTE_END_LEN,
    BTA_OP_VNOTE_MIN_LEN
};

const tBTA_OP_OBJ_TBL bta_op_vnote_prs =
{
    bta_op_vnote_tbl,
    BTA_OP_FMT_VNOTE11,
    bta_op_vnote_prs_begin,
    bta_op_vnote_end,
    BTA_OP_VNOTE_BEGIN_LEN,
    BTA_OP_VNOTE_END_LEN,
    BTA_OP_VNOTE_MIN_LEN
};

/*******************************************************************************
**
** Function         BTA_OpBuildNote
**
** Description      Build a vNote object.  The input to this function is an
**                  array of vNote properties and a pointer to memory to store
**                  the card.  The output is a formatted vNote.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpBuildNote(UINT8 *p_note, UINT16 *p_len, tBTA_OP_PROP *p_prop,
                               UINT8 num_prop)
{
    return bta_op_build_obj(&bta_op_vnote_bld, p_note, p_len, p_prop, num_prop);
}

/*******************************************************************************
**
** Function         BTA_OpParseNote
**
** Description      Parse a vNote object.  The input to this function is a
**                  pointer to vNote data.  The output is an array of parsed
**                  vNote properties.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpParseNote(tBTA_OP_PROP *p_prop, UINT8 *p_num_prop, UINT8 *p_note,
                               UINT16 len)
{
    return bta_op_parse_obj(&bta_op_vnote_prs, p_prop, p_num_prop, p_note, len);
}

