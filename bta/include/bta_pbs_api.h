/*****************************************************************************
**
**  Name:           bta_pbs_api.h
**
**  Description:    This is the public interface file for the phone book access
**                  (PB) server subsystem of BTA, Widcomm's
**                  Bluetooth application layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_PB_API_H
#define BTA_PB_API_H

#include "bta_api.h"
#include "btm_api.h"
#include "bta_sys.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

/**************************
**  Common Definitions
***************************/

/* Profile supported features */
#define BTA_PBS_SUPF_DOWNLOAD     0x0001
#define BTA_PBS_SURF_BROWSE       0x0002

/* Profile supported repositories */
#define BTA_PBS_REPOSIT_LOCAL      0x01    /* Local PhoneBook */
#define BTA_PBS_REPOSIT_SIM        0x02    /* SIM card PhoneBook */

#endif
