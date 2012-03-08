/******************************************************************************
**
**  File Name:   $RCSfile: sbc_dct.h,v $
**
**  Description: Definitions for the fast DCT.
**
**  Copyright (c) 1999-2008, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
******************************************************************************/

#ifndef SBC_DCT_H
#define SBC_DCT_H

#if (SBC_ARM_ASM_OPT==TRUE)
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1, s32OutLow)					\
{																			    \
    __asm																		\
{																				\
    MUL s32OutLow,(SINT32)s16In2, (s32In1>>15)        \
}																				\
}
#else
#if (SBC_DSP_OPT==TRUE)
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1 , s32OutLow) s32OutLow = SBC_Multiply_32_16_Simplified((SINT32)s16In2,s32In1);
#else
#if (SBC_IPAQ_OPT==TRUE)
/*#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1 , s32OutLow) s32OutLow=(SINT32)((SINT32)(s16In2)*(SINT32)(s32In1>>15)); */
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1 , s32OutLow) s32OutLow=(SINT32)(((SINT64)s16In2*(SINT64)s32In1)>>15);
#if (SBC_IS_64_MULT_IN_IDCT == TRUE)
#define SBC_MULT_32_32(s32In2, s32In1, s32OutLow)                           \
{                                                                           \
    s64Temp = ((SINT64) s32In2) * ((SINT64) s32In1)>>31;            \
    s32OutLow = (SINT32) s64Temp;                                                    \
}
#endif
#else
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1 , s32OutLow)                   \
{                                                                               \
    s32In1Temp = s32In1;                                                        \
    s32In2Temp = (SINT32)s16In2;                                                \
                                                                                \
    /* Multiply one +ve and the other -ve number */                             \
    if (s32In1Temp < 0)                                                         \
    {                                                                           \
        s32In1Temp ^= 0xFFFFFFFF;                                               \
        s32In1Temp++;                                                           \
        s32OutLow  = (s32In2Temp * (s32In1Temp >> 16));                         \
        s32OutLow += (( s32In2Temp * (s32In1Temp & 0xFFFF)) >> 16);             \
        s32OutLow ^= 0xFFFFFFFF;                                                \
        s32OutLow++;                                                            \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        s32OutLow  = (s32In2Temp * (s32In1Temp >> 16));                         \
        s32OutLow += (( s32In2Temp * (s32In1Temp & 0xFFFF)) >> 16);             \
    }                                                                           \
    s32OutLow <<= 1;                                                            \
}
#if (SBC_IS_64_MULT_IN_IDCT == TRUE)
#define SBC_MULT_64(s32In1, s32In2, s32OutLow, s32OutHi)  \
{\
        s32OutLow=(SINT32)(((SINT64)s32In1*(SINT64)s32In2)& 0x00000000FFFFFFFF);\
        s32OutHi=(SINT32)(((SINT64)s32In1*(SINT64)s32In2)>>32);\
}
#define SBC_MULT_32_32(s32In2, s32In1, s32OutLow)                           \
{                                                                           \
    s32HiTemp = 0;                                                          \
    SBC_MULT_64(s32In2,s32In1 , s32OutLow, s32HiTemp);                      \
    s32OutLow   = (((s32OutLow>>15)&0x1FFFF) | (s32HiTemp << 17));          \
}
#endif

#endif
#endif
#endif

#endif
