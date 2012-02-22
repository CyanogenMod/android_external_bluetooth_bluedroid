/*****************************************************************************
**
**  Name:           bta_mse_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the MSE subsystem.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_mse_api.h"

#ifndef BTA_MSE_MAX_NAME_LEN
    #define BTA_MSE_MAX_NAME_LEN      256   
#endif

#ifndef BTA_MSE_OBX_RSP_TOUT
    #define BTA_MSE_OBX_RSP_TOUT      2000
#endif


const tBTA_MSE_CFG bta_mse_cfg =
{
    BTA_MSE_OBX_RSP_TOUT,
    BTA_MSE_MAX_NAME_LEN
};

tBTA_MSE_CFG *p_bta_mse_cfg = (tBTA_MSE_CFG *)&bta_mse_cfg;

