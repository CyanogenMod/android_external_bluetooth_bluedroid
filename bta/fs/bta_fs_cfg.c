/*****************************************************************************
**
**  Name:           bta_fs_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for the BTA File System.
**
**  Copyright (c) 2003-2008, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
#include "bta_fs_api.h"

/* Character used as path separator */
#ifndef BTA_FS_PATH_SEPARATOR
#define BTA_FS_PATH_SEPARATOR   ((char) 0x2f)   /* 0x2f ('/'), or 0x5c ('\') */
#endif

/* Maximum path length supported by MMI */
#ifndef BTA_FS_PATH_LEN
#define BTA_FS_PATH_LEN         294
#endif

#ifndef BTA_FS_FILE_LEN
#define BTA_FS_FILE_LEN         256
#endif

const tBTA_FS_CFG bta_fs_cfg =
{
    BTA_FS_FILE_LEN,
    BTA_FS_PATH_LEN,
    BTA_FS_PATH_SEPARATOR   
};

tBTA_FS_CFG *p_bta_fs_cfg = (tBTA_FS_CFG *)&bta_fs_cfg;

