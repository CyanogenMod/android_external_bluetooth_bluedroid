/*****************************************************************************
**
**  Name:           bta_jv_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for advanced audio
**
**  Copyright (c) 2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "gki.h"
#include "bta_api.h"
#include "bd.h"
#include "bta_jv_api.h"

#ifndef BTA_JV_SDP_DB_SIZE
#define BTA_JV_SDP_DB_SIZE          4500
#endif

#ifndef BTA_JV_SDP_RAW_DATA_SIZE
#define BTA_JV_SDP_RAW_DATA_SIZE    1800
#endif

/* The platform may choose to use dynamic memory for these data buffers.
 * p_bta_jv_cfg->p_sdp_db must be allocated/stay allocated
 * between BTA_JvEnable and BTA_JvDisable
 * p_bta_jv_cfg->p_sdp_raw_data can be allocated before calling BTA_JvStartDiscovery
 * it can be de-allocated after the last call to access the database */
static UINT8 bta_jv_sdp_raw_data[BTA_JV_SDP_RAW_DATA_SIZE];
static UINT8 bta_jv_sdp_db_data[BTA_JV_SDP_DB_SIZE];

/* JV configuration structure */
const tBTA_JV_CFG bta_jv_cfg =
{
    BTA_JV_SDP_RAW_DATA_SIZE,   /* The size of p_sdp_raw_data */
    BTA_JV_SDP_DB_SIZE,         /* The size of p_sdp_db_data */
    bta_jv_sdp_raw_data,        /* The data buffer to keep raw data */
    (tSDP_DISCOVERY_DB *)bta_jv_sdp_db_data /* The data buffer to keep SDP database */
};

tBTA_JV_CFG *p_bta_jv_cfg = (tBTA_JV_CFG *) &bta_jv_cfg;


