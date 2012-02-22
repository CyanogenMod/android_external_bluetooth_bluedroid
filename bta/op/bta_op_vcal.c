/*****************************************************************************
**
**  Name:           bta_op_vcal.c
**
**  Description:    This file contains functions for parsing and building
**                  vCal objects.
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

#define BTA_OP_TODO_BEGIN_LEN       43
#define BTA_OP_TODO_END_LEN         26

#define BTA_OP_TODO_MIN_LEN         (BTA_OP_TODO_BEGIN_LEN + BTA_OP_TODO_END_LEN)

#define BTA_OP_EVENT_BEGIN_LEN      44
#define BTA_OP_EVENT_END_LEN        27

#define BTA_OP_EVENT_MIN_LEN        (BTA_OP_EVENT_BEGIN_LEN + BTA_OP_EVENT_END_LEN)

#define BTA_OP_VCAL_BEGIN_LEN       17
#define BTA_OP_VCAL_END_LEN         15
#define BTA_OP_VCAL_MIN_LEN         (BTA_OP_VCAL_BEGIN_LEN + BTA_OP_VCAL_END_LEN)
#define BTA_OP_BEGIN_OFFSET         30

const char bta_op_vcal_begin[] = "BEGIN:VCALENDAR\r\n";

const char bta_op_todo_begin[] = "BEGIN:VCALENDAR\r\nVERSION:1.0\r\nBEGIN:VTODO\r\n";

const char bta_op_todo_end[] = "END:VTODO\r\nEND:VCALENDAR\r\n";

const char bta_op_event_begin[] = "BEGIN:VCALENDAR\r\nVERSION:1.0\r\nBEGIN:VEVENT\r\n";

const char bta_op_event_end[] = "END:VEVENT\r\nEND:VCALENDAR\r\n";

const tBTA_OP_PROP_TBL bta_op_vcal_tbl[] =
{
    {NULL,          NULL, 11},      /* Number of elements in array */
    {"CATEGORIES",  NULL, 13},      /* BTA_OP_VCAL_CATEGORIES */
    {"COMPLETED",   NULL, 12},      /* BTA_OP_VCAL_COMPLETED */
    {"DESCRIPTION", NULL, 14},      /* BTA_OP_VCAL_DESCRIPTION */
    {"DTEND",       NULL,  8},      /* BTA_OP_VCAL_DTEND */
    {"DTSTART",     NULL, 10},      /* BTA_OP_VCAL_DTSTART */
    {"DUE",         NULL,  6},      /* BTA_OP_VCAL_DUE */
    {"LOCATION",    NULL, 11},      /* BTA_OP_VCAL_LOCATION */
    {"PRIORITY",    NULL, 11},      /* BTA_OP_VCAL_PRIORITY */
    {"STATUS",      NULL,  9},      /* BTA_OP_VCAL_STATUS */
    {"SUMMARY",     NULL, 10},      /* BTA_OP_VCAL_SUMMARY */
    {"X-IRMC-LUID", NULL, 14}       /* BTA_OP_VCAL_LUID */
};

const tBTA_OP_OBJ_TBL bta_op_todo_bld =
{
    bta_op_vcal_tbl,
    BTA_OP_FMT_VCAL10,
    bta_op_todo_begin,
    bta_op_todo_end,
    BTA_OP_TODO_BEGIN_LEN,
    BTA_OP_TODO_END_LEN,
    BTA_OP_TODO_MIN_LEN
};

const tBTA_OP_OBJ_TBL bta_op_event_bld =
{
    bta_op_vcal_tbl,
    BTA_OP_FMT_VCAL10,
    bta_op_event_begin,
    bta_op_event_end,
    BTA_OP_EVENT_BEGIN_LEN,
    BTA_OP_EVENT_END_LEN,
    BTA_OP_EVENT_MIN_LEN
};

const tBTA_OP_OBJ_TBL bta_op_vcal_prs =
{
    bta_op_vcal_tbl,
    BTA_OP_FMT_VCAL10,
    bta_op_vcal_begin,
    NULL,
    BTA_OP_VCAL_BEGIN_LEN,
    BTA_OP_VCAL_END_LEN,
    BTA_OP_VCAL_MIN_LEN
};

/*******************************************************************************
**
** Function         BTA_OpBuildCal
**
** Description      Build a vCal 1.0 object.  The input to this function is an
**                  array of vCaalproperties and a pointer to memory to store
**                  the card.  The output is a formatted vCal.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpBuildCal(UINT8 *p_cal, UINT16 *p_len, tBTA_OP_PROP *p_prop,
                              UINT8 num_prop, tBTA_OP_VCAL vcal_type)
{
    tBTA_OP_OBJ_TBL *p_bld;

    if (vcal_type == BTA_OP_VCAL_EVENT)
    {
        p_bld = (tBTA_OP_OBJ_TBL *) &bta_op_event_bld;
    }
    else
    {
        p_bld = ( tBTA_OP_OBJ_TBL *) &bta_op_todo_bld;
    }

    return bta_op_build_obj(p_bld, p_cal, p_len, p_prop, num_prop);
}

/*******************************************************************************
**
** Function         BTA_OpParseCal
**
** Description      Parse a vCal object.  The input to this function is a
**                  pointer to vCal data.  The output is an array of parsed
**                  vCal properties.
**                  
**
** Returns          BTA_OP_OK if operation successful.
**                  BTA_OP_FAIL if invalid property data.
**                  BTA_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tBTA_OP_STATUS BTA_OpParseCal(tBTA_OP_PROP *p_prop, UINT8 *p_num_prop, UINT8 *p_cal,
                              UINT16 len, tBTA_OP_VCAL *p_vcal_type)
{
    if (bta_op_scanstr(p_cal, (p_cal + len), &bta_op_todo_begin[BTA_OP_BEGIN_OFFSET]) != NULL)
    {
        *p_vcal_type = BTA_OP_VCAL_TODO;
    }
    else if (bta_op_scanstr(p_cal, (p_cal + len), &bta_op_event_begin[BTA_OP_BEGIN_OFFSET]) != NULL)
    {
        *p_vcal_type = BTA_OP_VCAL_EVENT;
    }
    else
    {
        *p_num_prop = 0;
        return BTA_OP_FAIL;
    }

    return bta_op_parse_obj(&bta_op_vcal_prs, p_prop, p_num_prop, p_cal, len);
}

