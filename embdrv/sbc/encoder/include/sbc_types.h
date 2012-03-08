/******************************************************************************
**
**  File Name:   $RCSfile: sbc_types.h,v $
**
**  Description: Data type declarations.
**
**  Revision :   $Id: sbc_types.h,v 1.7 2006/04/11 17:07:39 mjougit Exp $
**
**  Copyright (c) 1999-2008, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
******************************************************************************/

#ifndef SBC_TYPES_H
#define SBC_TYPES_H

#ifdef BUILDCFG
#include "bt_target.h"
#endif

#include "data_types.h"

typedef short SINT16;
typedef long SINT32;

#if (SBC_IPAQ_OPT == TRUE)

#if (SBC_FOR_EMBEDDED_LINUX == TRUE)
typedef long long SINT64;
#else
typedef __int64 SINT64;
#endif

#elif (SBC_IS_64_MULT_IN_WINDOW_ACCU == TRUE) || (SBC_IS_64_MULT_IN_IDCT == TRUE)

#if (SBC_FOR_EMBEDDED_LINUX == TRUE)
typedef long long SINT64;
#else
typedef __int64 SINT64;
#endif

#endif

#define abs32(x) ( (x >= 0) ? x : (-x) )

#endif
