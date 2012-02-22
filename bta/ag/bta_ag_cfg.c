/*****************************************************************************
**
**  Name:           bta_ag_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the audio gateway.
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "gki.h"
#include "bta_api.h"
#include "bta_ag_api.h"

#ifndef BTA_AG_CIND_INFO
#define BTA_AG_CIND_INFO     "(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"service\",(0-3)),(\"signal\",(0-6)),(\"roam\",(0,1)),(\"battchg\",(0-5)),(\"callheld\",(0-2)),(\"bearer\",(0-7))"
#endif

#ifndef BTA_AG_CHLD_VAL_ECC
#define BTA_AG_CHLD_VAL_ECC  "(0,1,1x,2,2x,3,4)"
#endif

#ifndef BTA_AG_CHLD_VAL
#define BTA_AG_CHLD_VAL  "(0,1,2,3,4)"
#endif

#ifndef BTA_AG_CONN_TIMEOUT
#define BTA_AG_CONN_TIMEOUT     5000
#endif

#ifndef BTA_AG_SCO_PKT_TYPES
/* S1 packet type setting from HFP 1.5 spec */
#define BTA_AG_SCO_PKT_TYPES    /* BTM_SCO_LINK_ALL_PKT_MASK */ (BTM_SCO_LINK_ONLY_MASK          | \
                                                                 BTM_SCO_PKT_TYPES_MASK_EV3      | \
                                                                 BTM_SCO_PKT_TYPES_MASK_NO_3_EV3 | \
                                                                 BTM_SCO_PKT_TYPES_MASK_NO_2_EV5 | \
                                                                 BTM_SCO_PKT_TYPES_MASK_NO_3_EV5)
#endif

const tBTA_AG_CFG bta_ag_cfg =
{
    BTA_AG_CIND_INFO,
    BTA_AG_CONN_TIMEOUT,
    BTA_AG_SCO_PKT_TYPES,
    BTA_AG_CHLD_VAL_ECC,
    BTA_AG_CHLD_VAL
};

tBTA_AG_CFG *p_bta_ag_cfg = (tBTA_AG_CFG *) &bta_ag_cfg;
