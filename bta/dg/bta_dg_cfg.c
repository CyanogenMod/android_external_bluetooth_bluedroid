/*****************************************************************************
**
**  Name:           bta_dg_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the BTA data gateway.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)

// btla-specific ++
#include "l2c_api.h"
#include "rfcdefs.h"
// btla-specific --
#include "bta_dg_api.h"
#include "rfcdefs.h"
#include "l2c_api.h"

/* RFCOMM MTU for SPP */
#ifndef BTA_SPP_MTU
#define BTA_SPP_MTU                 330
#endif

/* RFCOMM MTU for DUN */
#ifndef BTA_DUN_MTU
// btla-specific ++
#define BTA_DUN_MTU                 (330*3)
// btla-specific --
#endif

/* RFCOMM MTU for FAX */
#ifndef BTA_FAX_MTU
#define BTA_FAX_MTU                 112
#endif

/* RFCOMM MTU for LAP */
#ifndef BTA_LAP_MTU
#define BTA_LAP_MTU                 330
#endif

const tBTA_DG_CFG bta_dg_cfg =
{
    {BTA_SPP_MTU, BTA_DUN_MTU, BTA_FAX_MTU, BTA_LAP_MTU},

};

tBTA_DG_CFG *p_bta_dg_cfg = (tBTA_DG_CFG *)&bta_dg_cfg;
#endif /* BTA_DG_INCLUDED */
