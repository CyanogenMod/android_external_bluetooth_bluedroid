/*****************************************************************************
**
**  Name:           bta_pbs_int.h
**
**  Description:    This is the private file for the phone book access
**                  server (PBS).
**
**  Copyright (c) 2003-2010, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_PBS_INT_H
#define BTA_PBS_INT_H

#include "bt_target.h"
#include "bta_pbs_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_PBS_TARGET_UUID "\x79\x61\x35\xf0\xf0\xc5\x11\xd8\x09\x66\x08\x00\x20\x0c\x9a\x66"
#define BTA_PBS_UUID_LENGTH                 16
#define BTA_PBS_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */

#define BTA_PBS_DEFAULT_VERSION             0x0101  /* for PBAP PSE version 1.1 */


/* Configuration structure */
typedef struct
{
    UINT8       realm_charset;          /* Server only */
    BOOLEAN     userid_req;             /* TRUE if user id is required during obex authentication (Server only) */
    UINT8       supported_features;     /* Server supported features */
    UINT8       supported_repositories; /* Server supported repositories */

} tBTA_PBS_CFG;


/*****************************************************************************
**  Global data
*****************************************************************************/


#endif /* BTA_PBS_INT_H */
