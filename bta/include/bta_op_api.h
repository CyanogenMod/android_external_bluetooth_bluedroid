/*****************************************************************************
**
**  Name:           bta_op_api.h
**
**  Description:    This is the public interface file for the object push
**                  (OP) client and server subsystem of BTA, Widcomm's
**                  Bluetooth application layer for mobile phones.
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_OP_API_H
#define BTA_OP_API_H

#include "bta_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* Extra Debug Code */
#ifndef BTA_OPS_DEBUG
#define BTA_OPS_DEBUG           FALSE
#endif

#ifndef BTA_OPC_DEBUG
#define BTA_OPC_DEBUG           FALSE
#endif


/* Object format */
#define BTA_OP_VCARD21_FMT          1       /* vCard 2.1 */
#define BTA_OP_VCARD30_FMT          2       /* vCard 3.0 */
#define BTA_OP_VCAL_FMT             3       /* vCal 1.0 */
#define BTA_OP_ICAL_FMT             4       /* iCal 2.0 */
#define BTA_OP_VNOTE_FMT            5       /* vNote */
#define BTA_OP_VMSG_FMT             6       /* vMessage */
#define BTA_OP_OTHER_FMT            0xFF    /* other format */

typedef UINT8 tBTA_OP_FMT;

/* Object format mask */
#define BTA_OP_VCARD21_MASK         0x01    /* vCard 2.1 */
#define BTA_OP_VCARD30_MASK         0x02    /* vCard 3.0 */
#define BTA_OP_VCAL_MASK            0x04    /* vCal 1.0 */
#define BTA_OP_ICAL_MASK            0x08    /* iCal 2.0 */
#define BTA_OP_VNOTE_MASK           0x10    /* vNote */
#define BTA_OP_VMSG_MASK            0x20    /* vMessage */
#define BTA_OP_ANY_MASK             0x40    /* Any type of object. */

typedef UINT8 tBTA_OP_FMT_MASK;

#endif /* BTA_OP_API_H */

