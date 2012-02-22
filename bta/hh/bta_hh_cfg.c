/*****************************************************************************
**
**  Name:           bta_hh_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the BTA Hid Host.
**
**  Copyright (c) 2005, Broadcom Corp, All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
#include "bta_hh_api.h"

/* max number of device types supported by BTA */
#define BTA_HH_MAX_DEVT_SPT         7

/* size of database for service discovery */
#ifndef BTA_HH_DISC_BUF_SIZE
#define BTA_HH_DISC_BUF_SIZE        GKI_MAX_BUF_SIZE
#endif

/* application ID(none-zero) for each type of device */
#define BTA_HH_APP_ID_MI            1
#define BTA_HH_APP_ID_KB            2
#define BTA_HH_APP_ID_RMC           3
#define BTA_HH_APP_ID_3DSG          4


/* The type of devices supported by BTA HH and corresponding application ID */
tBTA_HH_SPT_TOD p_devt_list[BTA_HH_MAX_DEVT_SPT] =
{
    {BTA_HH_DEVT_MIC, BTA_HH_APP_ID_MI},
    {BTA_HH_DEVT_KBD, BTA_HH_APP_ID_KB},
    {BTA_HH_DEVT_KBD|BTA_HH_DEVT_MIC, BTA_HH_APP_ID_KB},
    {BTA_HH_DEVT_RMC, BTA_HH_APP_ID_RMC},
    {BTA_HH_DEVT_RMC | BTA_HH_DEVT_KBD, BTA_HH_APP_ID_RMC},
    {BTA_HH_DEVT_MIC | BTA_HH_DEVT_DGT, BTA_HH_APP_ID_MI},
    {BTA_HH_DEVT_UNKNOWN, BTA_HH_APP_ID_3DSG}
};


const tBTA_HH_CFG bta_hh_cfg =
{
    BTA_HH_MAX_DEVT_SPT,            /* number of supported type of devices */
    p_devt_list,                    /* ToD & AppID list */
    BTA_HH_DISC_BUF_SIZE            /* HH SDP discovery database size */
};


tBTA_HH_CFG *p_bta_hh_cfg = (tBTA_HH_CFG *)&bta_hh_cfg;
