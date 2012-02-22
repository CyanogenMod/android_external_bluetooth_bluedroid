/*****************************************************************************
**
**  Name:       goep_int.h
**
**  File:       Generic Object Exchange Profile Internal Definitions
**
**  Copyright (c) 2000-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef GOEP_INT_H
#define GOEP_INT_H

#include "bt_target.h"
#include "goep_util.h"



/*****************************************************************************
**   Constants
*****************************************************************************/
#define GOEP_PROTOCOL_COUNT         3



/*****************************************************************************
** Main Control Block Structure
*****************************************************************************/

typedef struct
{
    UINT8 trace_level;
} tGOEP_CB;

/*****************************************************************************
**   Function Prototypes
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#if GOEP_DYNAMIC_MEMORY == FALSE
GOEP_API extern tGOEP_CB  goep_cb;
#else
GOEP_API extern tGOEP_CB *goep_cb_ptr;
#define goep_cb (*goep_cb_ptr)
#endif



#ifdef __cplusplus
}
#endif

#endif /* GOEP_INT_H */
