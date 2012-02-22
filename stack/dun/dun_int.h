/*****************************************************************************
**
**  Name:       dun_int.h
**
**  File:       dun/fax type definitions
**
**  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef DUN_INT_H
#define DUN_INT_H

#include "dun_api.h"
#include "sdp_api.h"

/*****************************************************************************
**   Constants
*****************************************************************************/

/*****************************************************************************
**   DUN Control Blocks
******************************************************************************/
/*****************************************************************************
**  Type definitions
*****************************************************************************/
/* Control block used by DUN_FindService(). */
typedef struct 
{
    tDUN_FIND_CBACK     *p_cback;       /* pointer to application callback */
    tSDP_DISCOVERY_DB   *p_db;          /* pointer to discovery database */
    UINT16               service_uuid;  /* service UUID of search */
} tDUN_FIND_CB;

typedef struct
{
    tDUN_FIND_CB find_cb;
    UINT8        trace_level;
} tDUN_CB;

/*****************************************************************************
**   External Definitions
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif
/*
** Define prototypes for global data
*/
#if DUN_DYNAMIC_MEMORY == FALSE
DUN_API extern tDUN_CB  dun_cb;
#else
DUN_API extern tDUN_CB *dun_cb_ptr;
#define dun_cb (*dun_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DUN_INT_H */

