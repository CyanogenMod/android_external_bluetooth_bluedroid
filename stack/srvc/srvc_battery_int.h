/*****************************************************************************
**
**  Name:          srvc_battery_int.h
**
**  Description:   this file contains the Battery service internal interface
**                 definitions.
**
**
**  Copyright (c) 1999-2008, Broadcom Corp., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef SRVC_BATTERY_INT_H
#define SRVC_BATTERY_INT_H

#include "bt_target.h"
#include "srvc_api.h"
#include "gatt_api.h"

#ifndef BA_MAX_INT_NUM
#define BA_MAX_INT_NUM     4
#endif

#define BATTERY_LEVEL_SIZE      1


typedef struct
{
    UINT8           app_id;
    UINT16          ba_level_hdl;
    UINT16          clt_cfg_hdl;
    UINT16          rpt_ref_hdl;
    UINT16          pres_fmt_hdl;

    tBA_CBACK       *p_cback;

    UINT16          pending_handle;
    UINT8           pending_clcb_idx;
    UINT8           pending_evt;

}tBA_INST;

typedef struct
{
    tBA_INST                battery_inst[BA_MAX_INT_NUM];
    UINT8                   inst_id;
    BOOLEAN                 enabled;

}tBATTERY_CB;

#ifdef __cplusplus
extern "C" {
#endif

/* Global GATT data */
#if GATT_DYNAMIC_MEMORY == FALSE
GATT_API extern tBATTERY_CB battery_cb;
#else
GATT_API extern tBATTERY_CB *battery_cb_ptr;
#define battery_cb (*battery_cb_ptr)
#endif


extern BOOLEAN battery_valid_handle_range(UINT16 handle);

extern UINT8 battery_s_write_attr_value(UINT8 clcb_idx, tGATT_WRITE_REQ * p_value,
                                 tGATT_STATUS *p_status);
extern UINT8 battery_s_read_attr_value (UINT8 clcb_idx, UINT16 handle, tGATT_VALUE *p_value, BOOLEAN is_long, tGATT_STATUS* p_status);



#ifdef __cplusplus
}
#endif
#endif
