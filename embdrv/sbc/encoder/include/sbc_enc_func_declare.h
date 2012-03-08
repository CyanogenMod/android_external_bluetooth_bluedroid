/******************************************************************************
**
**  File Name:   $RCSfile: sbc_enc_func_declare.h,v $
**
**  Description: Function declarations.
**
**  Revision :   $Id: sbc_enc_func_declare.h,v 1.8 2006/04/13 16:59:58 mjougit Exp $
**
**  Copyright (c) 1999-2002, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
******************************************************************************/

#ifndef SBC_FUNCDECLARE_H
#define SBC_FUNCDECLARE_H

/*#include "sbc_encoder.h"*/
/* Global data */
#if (SBC_IS_64_MULT_IN_WINDOW_ACCU == FALSE)
extern const SINT16 gas32CoeffFor4SBs[];
extern const SINT16 gas32CoeffFor8SBs[];
#else
extern const SINT32 gas32CoeffFor4SBs[];
extern const SINT32 gas32CoeffFor8SBs[];
#endif

/* Global functions*/

extern void sbc_enc_bit_alloc_mono(SBC_ENC_PARAMS *CodecParams);
extern void sbc_enc_bit_alloc_ste(SBC_ENC_PARAMS *CodecParams);

extern void SbcAnalysisInit (void);

extern void SbcAnalysisFilter4(SBC_ENC_PARAMS *strEncParams);
extern void SbcAnalysisFilter8(SBC_ENC_PARAMS *strEncParams);

extern void SBC_FastIDCT8 (SINT32 *pInVect, SINT32 *pOutVect);
extern void SBC_FastIDCT4 (SINT32 *x0, SINT32 *pOutVect);

extern void EncPacking(SBC_ENC_PARAMS *strEncParams);
extern void EncQuantizer(SBC_ENC_PARAMS *);
#if (SBC_DSP_OPT==TRUE)
    SINT32 SBC_Multiply_32_16_Simplified(SINT32 s32In2Temp,SINT32 s32In1Temp);
#endif
#endif

