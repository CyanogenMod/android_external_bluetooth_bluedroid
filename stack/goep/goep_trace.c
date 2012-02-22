/******************************************************************************
**
**  Name:          goep_trace.c
**
**  Description:   This file contains the debug display prototypes
**
**  Copyright (c) 2000-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
******************************************************************************/
#include "bt_target.h"

#if (defined(GOEP_INCLUDED) && GOEP_INCLUDED == TRUE)

#include "goep_int.h"

#if (defined (BT_USE_TRACES) && BT_USE_TRACES == TRUE)


char *GOEP_ErrorName (tGOEP_ERRORS error)
{
    switch (error)
    {
    case GOEP_SUCCESS:              return "GOEP_SUCCESS (0x00)";
    case GOEP_ERROR:                return "GOEP_ERROR (0x01)";
    case GOEP_RESOURCES:            return "GOEP_RESOURCES (0x02)";
    case GOEP_INVALID_PARAM:        return "GOEP_INVALID_PARAM (0x03)";
    default:                        return "UNKNOWN GOEP ERROR";
    }
}

/* end if BT_TRACE_VERBOSE */
#endif

#endif /* GOEP_INCLUDED */
