/*****************************************************************************
**
**  Name:           bta_pbs_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the BTA Phone Book Access Server.
**
**  Copyright (c) 2003-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include "bta_pbs_int.h"

/* Realm Character Set */
#ifndef BTA_PBS_REALM_CHARSET
#define BTA_PBS_REALM_CHARSET   0       /* ASCII */
#endif

/* Specifies whether or not client's user id is required during obex authentication */
#ifndef BTA_PBS_USERID_REQ
#define BTA_PBS_USERID_REQ      FALSE
#endif

const tBTA_PBS_CFG bta_pbs_cfg =
{
    BTA_PBS_REALM_CHARSET,      /* Server only */
    BTA_PBS_USERID_REQ,         /* Server only */
    (BTA_PBS_SUPF_DOWNLOAD | BTA_PBS_SURF_BROWSE),
    BTA_PBS_REPOSIT_LOCAL,
};

tBTA_PBS_CFG *p_bta_pbs_cfg = (tBTA_PBS_CFG *)&bta_pbs_cfg;
#endif /* BTA_PBS_INCLUDED */
