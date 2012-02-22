/*****************************************************************************
**
**  Name:           bta_ft_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the BTA File Transfer Client and Server.
**
**  Copyright (c) 2003-2010, Broadcom Inc., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
#include "bta_ftc_int.h"

/* Realm Character Set */
#ifndef BTA_FTS_REALM_CHARSET
#define BTA_FTS_REALM_CHARSET   0       /* ASCII */
#endif

/* Specifies whether or not client's user id is required during obex authentication */
#ifndef BTA_FTS_USERID_REQ
#define BTA_FTS_USERID_REQ      FALSE
#endif

#ifndef BTA_FTC_AUTO_FILE_LIST
#define BTA_FTC_AUTO_FILE_LIST TRUE
#endif

#ifndef BTA_FTC_ABORT_TIMEOUT
#define BTA_FTC_ABORT_TIMEOUT       7000   /* 7 Secs for the low baud rates and SRM mode */
#endif

#ifndef BTA_FTC_STOP_TIMEOUT
#define BTA_FTC_STOP_TIMEOUT        2000
#endif

#ifndef BTA_FTC_SUSPEND_TIMEOUT
#define BTA_FTC_SUSPEND_TIMEOUT     10000
#endif

#ifndef BTA_FTC_PCE_SUPPORTED_FEAT       /* mask: 1/download, 2/browsing */
#define BTA_FTC_PCE_SUPPORTED_FEAT     0 /* 3/both, 0/PBAP PCE not supported */
#endif

#ifndef BTA_FTC_PCE_SERVICE_NAME
#define BTA_FTC_PCE_SERVICE_NAME        "PBAP PCE"
#endif

#ifndef BTA_FTC_2X_SUPPORTED
#define BTA_FTC_2X_SUPPORTED            TRUE
#endif

const tBTA_FT_CFG bta_ft_cfg =
{
#if (defined BIP_INCLUDED && BIP_INCLUDED == TRUE && defined BTA_BI_INCLUDED && BTA_BI_INCLUDED == TRUE)
    bta_ftc_bi_action,          /* Client only */
#else
    NULL,                       /* Client only */
#endif

    BTA_FTS_REALM_CHARSET,      /* Server only */
    BTA_FTS_USERID_REQ,         /* Server only */
    BTA_FTC_AUTO_FILE_LIST,     /* Client only */
    BTA_FTC_PCE_SUPPORTED_FEAT, /* Client only */
    BTA_FTC_PCE_SERVICE_NAME,   /* Client only */
    BTA_FTC_ABORT_TIMEOUT,      /* Client only */
    BTA_FTC_STOP_TIMEOUT ,      /* Client only */
    BTA_FTC_SUSPEND_TIMEOUT     /* Client only - used for 2X only */
#if BTA_FTC_2X_SUPPORTED == TRUE
// btla-specific ++
    , 0 // [no reliable sess]   /* non-0 to allow reliable session (Server Only) */
    , 0 // [no reliable sess]   /* the maximum number of suspended session (Server Only) */
// btla-specific --
    , TRUE                      /* TRUE to use Obex Over L2CAP (Server Only) */
    , TRUE                      /* TRUE to engage Single Response Mode (Server Only) */
#else
    , 0                         /* non-0 to allow reliable session (Server Only) */
    , 0                         /* the maximum number of suspended session (Server Only) */
    , FALSE                     /* TRUE to use Obex Over L2CAP (Server Only) */
    , FALSE                     /* TRUE to engage Single Response Mode (Server Only) */
#endif
};

tBTA_FT_CFG *p_bta_ft_cfg = (tBTA_FT_CFG *)&bta_ft_cfg;

