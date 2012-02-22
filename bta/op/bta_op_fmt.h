/*****************************************************************************
**
**  Name:           bta_op_fmt.h
**
**  Description:    This is the interface file for common functions and data 
**                  types used by the OPP object formatting functions.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_OP_FMT_H
#define BTA_OP_FMT_H

/*****************************************************************************
** Constants
*****************************************************************************/

#define BTA_OP_PROP_OVHD        3

/*****************************************************************************
** Data types
*****************************************************************************/

enum {
    BTA_OP_FMT_NONE=0,
    BTA_OP_FMT_VCARD21,
    BTA_OP_FMT_VCARD30,
    BTA_OP_FMT_VCAL10,
    BTA_OP_FMT_VNOTE11
};

/*Supported formats */
typedef UINT8 tBTA_OP_SUP_FMT;

typedef struct
{
    const char              *p_name;
    UINT8                   len;
} tBTA_OP_PARAM;

typedef struct
{
    const char              *p_name;
    const tBTA_OP_PARAM     *p_param_tbl;
    UINT8                   len;
} tBTA_OP_PROP_TBL;


typedef struct
{
    const UINT32 *p_prop_filter_mask;
    UINT8 len;
} tBTA_OP_PROP_FILTER_MASK_TBL;

typedef struct
{
    const tBTA_OP_PROP_TBL  *p_tbl;
    tBTA_OP_SUP_FMT         fmt;
    const char              *p_begin_str;
    const char              *p_end_str;
    UINT8                   begin_len;
    UINT8                   end_len;
    UINT8                   min_len;
    const tBTA_OP_PROP_FILTER_MASK_TBL  *p_prop_filter_mask_tbl;
} tBTA_OP_OBJ_TBL;

typedef struct
{
    const char              *media_name;
    UINT8                   len;
} tBTA_OP_PROP_MEDIA;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern tBTA_OP_STATUS bta_op_build_obj(const tBTA_OP_OBJ_TBL *p_bld,
                                       UINT8 *p_data, UINT16 *p_len,
                                       tBTA_OP_PROP *p_prop, UINT8 num_prop);

extern tBTA_OP_STATUS bta_op_parse_obj(const tBTA_OP_OBJ_TBL *p_prs,
                                       tBTA_OP_PROP *p_prop, UINT8 *p_num_prop,
                                       UINT8 *p_data, UINT16 len);
extern tBTA_OP_STATUS bta_op_get_property_by_name(const tBTA_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tBTA_OP_PROP *p_prop, UINT8 num_prop, UINT8 *p_data,
                                UINT16 *p_len);
extern UINT8 *bta_op_scanstr(UINT8 *p, UINT8 *p_end, const char *p_str);
extern void bta_op_set_prop_filter_mask(UINT32 mask);

#endif /* BTA_OP_FMT_H */

