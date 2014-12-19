/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains definitions for implementing the
 *  diagnostic trace message service.
 *
 ******************************************************************************/

#ifndef BT_TRACE_H
#define BT_TRACE_H

#ifndef BTTRC_INCLUDED
#define BTTRC_INCLUDED FALSE
#endif
#ifndef BTTRC_PARSER_INCLUDED
#define BTTRC_PARSER_INCLUDED FALSE
#endif
#ifndef MAX_TRACE_RAM_SIZE
#define MAX_TRACE_RAM_SIZE 10000
#endif

/* BTE tracing IDs for debug purposes */
/* LayerIDs for stack */
#define BTTRC_ID_STK_GKI                   1
#define BTTRC_ID_STK_BTU                   2
#define BTTRC_ID_STK_HCI                   3
#define BTTRC_ID_STK_L2CAP                 4
#define BTTRC_ID_STK_RFCM_MX               5
#define BTTRC_ID_STK_RFCM_PRT              6
#define BTTRC_ID_STK_OBEX_C                7
#define BTTRC_ID_STK_OBEX_S                8
#define BTTRC_ID_STK_AVCT                  9
#define BTTRC_ID_STK_AVDT                  10
#define BTTRC_ID_STK_AVRC                  11
#define BTTRC_ID_STK_BIC                   12
#define BTTRC_ID_STK_BIS                   13
#define BTTRC_ID_STK_BNEP                  14
#define BTTRC_ID_STK_BPP                   15
#define BTTRC_ID_STK_BTM_ACL               16
#define BTTRC_ID_STK_BTM_PM                17
#define BTTRC_ID_STK_BTM_DEV_CTRL          18
#define BTTRC_ID_STK_BTM_SVC_DSC           19
#define BTTRC_ID_STK_BTM_INQ               20
#define BTTRC_ID_STK_BTM_SCO               21
#define BTTRC_ID_STK_BTM_SEC               22
#define BTTRC_ID_STK_DUN                   23
#define BTTRC_ID_STK_HID                   24
#define BTTRC_ID_STK_HSP2                  25
#define BTTRC_ID_STK_CTP                   26
#define BTTRC_ID_STK_FTC                   27
#define BTTRC_ID_STK_FTS                   28
#define BTTRC_ID_STK_GAP                   29
#define BTTRC_ID_STK_GOEP                  30
#define BTTRC_ID_STK_HCRP                  31
#define BTTRC_ID_STK_ICP                   32
#define BTTRC_ID_STK_OPC                   33
#define BTTRC_ID_STK_OPS                   34
#define BTTRC_ID_STK_PAN                   35
#define BTTRC_ID_STK_SAP                   36
#define BTTRC_ID_STK_SDP                   37
#define BTTRC_ID_STK_SLIP                  38
#define BTTRC_ID_STK_SPP                   39
#define BTTRC_ID_STK_TCS                   40
#define BTTRC_ID_STK_VDP                   41
#define BTTRC_ID_STK_MCAP                  42
#define BTTRC_ID_STK_GATT                  43
#define BTTRC_ID_STK_SMP                   44
#define BTTRC_ID_STK_NFC                   45
#define BTTRC_ID_STK_NCI                   46
#define BTTRC_ID_STK_IDEP                  47
#define BTTRC_ID_STK_NDEP                  48
#define BTTRC_ID_STK_LLCP                  49
#define BTTRC_ID_STK_RW                    50
#define BTTRC_ID_STK_CE                    51
#define BTTRC_ID_STK_SNEP                  52
#define BTTRC_ID_STK_NDEF                  53
#define BTTRC_ID_STK_HIDD                  54


/* LayerIDs for BTA */
#define BTTRC_ID_BTA_ACC                   55         /* Advanced Camera Client */
#define BTTRC_ID_BTA_AG                    56         /* audio gateway */
#define BTTRC_ID_BTA_AV                    57         /* Advanced audio */
#define BTTRC_ID_BTA_BIC                   58         /* Basic Imaging Client */
#define BTTRC_ID_BTA_BIS                   59         /* Basic Imaging Server */
#define BTTRC_ID_BTA_BP                    60         /* Basic Printing Client */
#define BTTRC_ID_BTA_CG                    61
#define BTTRC_ID_BTA_CT                    62         /* cordless telephony terminal */
#define BTTRC_ID_BTA_DG                    63         /* data gateway */
#define BTTRC_ID_BTA_DM                    64         /* device manager */
#define BTTRC_ID_BTA_DM_SRCH               65         /* device manager search */
#define BTTRC_ID_BTA_DM_SEC                66         /* device manager security */
#define BTTRC_ID_BTA_FM                    67
#define BTTRC_ID_BTA_FTC                   68         /* file transfer client */
#define BTTRC_ID_BTA_FTS                   69         /* file transfer server */
#define BTTRC_ID_BTA_HIDH                  70
#define BTTRC_ID_BTA_HIDD                  71
#define BTTRC_ID_BTA_JV                    72
#define BTTRC_ID_BTA_OPC                   73         /* object push client */
#define BTTRC_ID_BTA_OPS                   74         /* object push server */
#define BTTRC_ID_BTA_PAN                   75         /* Personal Area Networking */
#define BTTRC_ID_BTA_PR                    76         /* Printer client */
#define BTTRC_ID_BTA_SC                    77         /* SIM Card Access server */
#define BTTRC_ID_BTA_SS                    78         /* synchronization server */
#define BTTRC_ID_BTA_SYS                   79         /* system manager */
#define BTTRC_ID_AVDT_SCB                  80         /* avdt scb */
#define BTTRC_ID_AVDT_CCB                  81         /* avdt ccb */

// btla-specific ++
/* LayerIDs added for BTL-A. Probably should modify bte_logmsg.c in future. */
#define BTTRC_ID_STK_RFCOMM                82
#define BTTRC_ID_STK_RFCOMM_DATA           83
#define BTTRC_ID_STK_OBEX                  84
#define BTTRC_ID_STK_A2D                   85
#define BTTRC_ID_STK_BIP                   86

/* LayerIDs for BT APP */
#define BTTRC_ID_BTAPP                     87
#define BTTRC_ID_LATENCY_AUDIO             88
#define BTTRC_ID_BT_PROTOCOL               89         /* this is a temporary solution to allow dynamic
                                                         enable/disable of BT_PROTOCOL_TRACE */
#define BTTRC_ID_MAX_ID                    BTTRC_ID_BT_PROTOCOL
// btla-specific --
#define BTTRC_ID_ALL_LAYERS                0xFF       /* all trace layers */
typedef UINT8 tBTTRC_LAYER_ID;

/* Trace type definitions. Note that these are mutually exclusive in a trace. This
means that any trace can be either error,warning,api,event or dbg */
#define BTTRC_TYPE_ERROR                   0x01       /* Traces for error situation */
#define BTTRC_TYPE_WARNING	               0x02       /* Traces for warning situation */
#define BTTRC_TYPE_API                     0x04       /* Traces for API */
#define BTTRC_TYPE_EVENT                   0x08       /* Traces for EVENT */
#define BTTRC_TYPE_ACTION                  0x10       /* Traces for Action functions */
#define BTTRC_TYPE_DBG                     0x20       /* Traces for debugging purpose */
typedef UINT8 tBTTRC_TYPE;

/* Masks to identify the stack that originated the trace */
#define BTTRC_TRACE_LITE                   0x80       /* MM Lite stack */
#define BTTRC_TRACE_EMBD                   0x40       /* Embedded host stack */

/* Parameter datatypes used in Trace APIs */
#define BTTRC_PARAM_UINT8                  1
#define BTTRC_PARAM_UINT16                 2
#define BTTRC_PARAM_UINT32                 3
typedef UINT8 tBTTRC_PARAM_TYPE;

/* Special token definitions */
#define BTTRC_TOKEN_SM_STATE               0xFFFF     /* Token indicating the State of a State m/c */

// btla-specific ++
typedef struct {
    tBTTRC_LAYER_ID layer_id;
    tBTTRC_TYPE     type;      /* TODO: use tBTTRC_TYPE instead of "classical level 0-5" */
} tBTTRC_LEVEL;

typedef UINT8 (tBTTRC_SET_TRACE_LEVEL)( UINT8 );

typedef struct {
    const tBTTRC_LAYER_ID         layer_id_start;
    const tBTTRC_LAYER_ID         layer_id_end;
    tBTTRC_SET_TRACE_LEVEL        *p_f;
    const char                    *trc_name;
    UINT8                         trace_level;
} tBTTRC_FUNC_MAP;

extern tBTTRC_FUNC_MAP bttrc_set_level_map[];
extern const UINT16 bttrc_map_size;
extern BT_API tBTTRC_LEVEL * BTA_SysSetTraceLevel( tBTTRC_LEVEL * p_levels );
// btla-specific --


#ifdef __cplusplus
extern "C" {
#endif

/* External declaration for appl_trace_level here to avoid to add the declaration in all the files using APPL_TRACExxx macros */
extern UINT8 appl_trace_level ;

// btla-specific ++
EXPORT_API extern void BTE_InitTraceLevels( void );
// btla-specific --

/* Prototype for message logging function. */
EXPORT_API extern void LogMsg (UINT32 trace_set_mask, const char *fmt_str, ...);

/* Prototype for stack tracing function. */
EXPORT_API extern void BTTRC_StackTrace0(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token);
EXPORT_API extern void BTTRC_StackTrace1(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val);
EXPORT_API extern void BTTRC_StackTrace2(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val,
                                   tBTTRC_PARAM_TYPE p2_type, UINT32 p2_val);
EXPORT_API extern void BTTRC_StackTrace3(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val,
                                   tBTTRC_PARAM_TYPE p2_type, UINT32 p2_val,
                                   tBTTRC_PARAM_TYPE p3_type, UINT32 p3_val);
EXPORT_API extern void BTTRC_StackTrace4(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val,
                                   tBTTRC_PARAM_TYPE p2_type, UINT32 p2_val,
                                   tBTTRC_PARAM_TYPE p3_type, UINT32 p3_val,
                                   tBTTRC_PARAM_TYPE p4_type, UINT32 p4_val);
EXPORT_API extern void BTTRC_StackTrace5(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val,
                                   tBTTRC_PARAM_TYPE p2_type, UINT32 p2_val,
                                   tBTTRC_PARAM_TYPE p3_type, UINT32 p3_val,
                                   tBTTRC_PARAM_TYPE p4_type, UINT32 p4_val,
                                   tBTTRC_PARAM_TYPE p5_type, UINT32 p5_val);
EXPORT_API extern void BTTRC_StackTrace6(tBTTRC_LAYER_ID layer_id,
                                   tBTTRC_TYPE type,
                                   UINT16 token,
                                   tBTTRC_PARAM_TYPE p1_type, UINT32 p1_val,
                                   tBTTRC_PARAM_TYPE p2_type, UINT32 p2_val,
                                   tBTTRC_PARAM_TYPE p3_type, UINT32 p3_val,
                                   tBTTRC_PARAM_TYPE p4_type, UINT32 p4_val,
                                   tBTTRC_PARAM_TYPE p5_type, UINT32 p5_val,
                                   tBTTRC_PARAM_TYPE p6_type, UINT32 p6_val);

// btla-specific ++
/* p_levels must be a 0 terminated list ! */
//EXPORT_API extern tBTTRC_LEVEL * BTA_SysSetTraceLevel( tBTTRC_LEVEL * p_levels );
// btla-specific --

#ifdef __cplusplus
}
#endif

/******************************************************************************
**
** Trace configurable parameters
**
******************************************************************************/

/* Enables or disables verbose trace information. */
#ifndef BT_TRACE_VERBOSE
#define BT_TRACE_VERBOSE    FALSE
#endif

/* Enables or disables all trace messages. */
#ifndef BT_USE_TRACES
#define BT_USE_TRACES       TRUE
#endif


/******************************************************************************
**
** Trace Levels
**
** The following values may be used for different levels:
**      BT_TRACE_LEVEL_NONE    0        * No trace messages to be generated
**      BT_TRACE_LEVEL_ERROR   1        * Error condition trace messages
**      BT_TRACE_LEVEL_WARNING 2        * Warning condition trace messages
**      BT_TRACE_LEVEL_API     3        * API traces
**      BT_TRACE_LEVEL_EVENT   4        * Debug messages for events
**      BT_TRACE_LEVEL_DEBUG   5        * Debug messages (general)
******************************************************************************/

// btla-specific ++
/* Core Stack default trace levels */
#ifndef HCI_INITIAL_TRACE_LEVEL
#define HCI_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef BTM_INITIAL_TRACE_LEVEL
#define BTM_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef L2CAP_INITIAL_TRACE_LEVEL
#define L2CAP_INITIAL_TRACE_LEVEL           BT_TRACE_LEVEL_WARNING
#endif

#ifndef RFCOMM_INITIAL_TRACE_LEVEL
#define RFCOMM_INITIAL_TRACE_LEVEL          BT_TRACE_LEVEL_WARNING
#endif

#ifndef SDP_INITIAL_TRACE_LEVEL
#define SDP_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef GAP_INITIAL_TRACE_LEVEL
#define GAP_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef BNEP_INITIAL_TRACE_LEVEL
#define BNEP_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_WARNING
#endif

#ifndef PAN_INITIAL_TRACE_LEVEL
#define PAN_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef A2D_INITIAL_TRACE_LEVEL
#define A2D_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef AVDT_INITIAL_TRACE_LEVEL
#define AVDT_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_WARNING
#endif

#ifndef AVCT_INITIAL_TRACE_LEVEL
#define AVCT_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_WARNING
#endif

#ifndef AVRC_INITIAL_TRACE_LEVEL
#define AVRC_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_WARNING
#endif

#ifndef MCA_INITIAL_TRACE_LEVEL
#define MCA_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef HID_INITIAL_TRACE_LEVEL
#define HID_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef APPL_INITIAL_TRACE_LEVEL
#define APPL_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_WARNING
#endif

#ifndef BT_TRACE_APPL
#define BT_TRACE_APPL   BT_USE_TRACES
#endif

#ifndef GATT_INITIAL_TRACE_LEVEL
#define GATT_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif

#ifndef SMP_INITIAL_TRACE_LEVEL
#define SMP_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_WARNING
#endif
// btla-specific --


#if (BTTRC_INCLUDED == TRUE)
/***************************************************************************************/
/* BTTRC MACROS */

#define BTTRC_EVENT(lid, event, state)   \
            {BTTRC_StackTrace1(lid, BTTRC_TYPE_EVENT, event, BTTRC_PARAM_UINT8, state);}
#define BTTRC_ACTION(lid, action)  \
            {BTTRC_StackTrace0(lid, BTTRC_TYPE_ACTION, action);}
#define BTTRC_STATE(lid, state)   \
            {BTTRC_StackTrace1(lid, BTTRC_TYPE_EVENT, BTTRC_TOKEN_SM_STATE, BTTRC_PARAM_UINT8, state);}

#define BTTRC_API0(lid, api)  \
            {BTTRC_StackTrace0(lid, BTTRC_TYPE_API, api);}
#define BTTRC_API1(lid, api, p1_t,p1_v)  \
            {BTTRC_StackTrace1(lid, BTTRC_TYPE_API, api, p1_t,p1_v);}
#define BTTRC_API2(lid, api, p1_t,p1_v,p2_t,p2_v)  \
            {BTTRC_StackTrace2(lid, BTTRC_TYPE_API, api, p1_t,p1_v,p2_t,p2_v);}
#define BTTRC_API3(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)  \
            {BTTRC_StackTrace3(lid, BTTRC_TYPE_API, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v);}
#define BTTRC_API4(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)  \
            {BTTRC_StackTrace4(lid, BTTRC_TYPE_API, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v);}
#define BTTRC_API5(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)  \
            {BTTRC_StackTrace5(lid, BTTRC_TYPE_API, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v);}
#define BTTRC_API6(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)  \
            {BTTRC_StackTrace6(lid, BTTRC_TYPE_API, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v);}


#define BTTRC_DBG0(lid, dbg)  \
            {BTTRC_StackTrace0(lid, BTTRC_TYPE_DBG, dbg);}
#define BTTRC_DBG1(lid, dbg, p1_t,p1_v)  \
            {BTTRC_StackTrace1(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v);}
#define BTTRC_DBG2(lid, dbg, p1_t,p1_v,p2_t,p2_v)  \
            {BTTRC_StackTrace2(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v,p2_t,p2_v);}
#define BTTRC_DBG3(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)  \
            {BTTRC_StackTrace3(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v);}
#define BTTRC_DBG4(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)  \
            {BTTRC_StackTrace4(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v);}
#define BTTRC_DBG5(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)  \
            {BTTRC_StackTrace5(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v);}
#define BTTRC_DBG6(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)  \
            {BTTRC_StackTrace6(lid, BTTRC_TYPE_DBG, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v);}

/***************************************************************************************/
/*AVDT MACROS */

#define BTTRC_AVDT_API0(api)   \
        BTTRC_API0(BTTRC_ID_STK_AVDT, api)
#define BTTRC_AVDT_API1(api, p1_t, p1_v) \
        BTTRC_API1(BTTRC_ID_STK_AVDT, api, p1_t, p1_v)
#define BTTRC_AVDT_API2(api, p1_t, p1_v, p2_t, p2_v) \
        BTTRC_API2(BTTRC_ID_STK_AVDT, api, p1_t, p1_v, p2_t, p2_v)
/***************************************************************************************/
/*AVDT_SCB MACROS */

#define BTTRC_AVDT_SCB_EVENT(event, state)   \
            BTTRC_EVENT(BTTRC_ID_AVDT_SCB, event, state)
#define BTTRC_AVDT_SCB_ACTION(action)  \
            BTTRC_ACTION(BTTRC_ID_AVDT_SCB, action)
#define BTTRC_AVDT_SCB_STATE(next_state)   \
            BTTRC_STATE(BTTRC_ID_AVDT_SCB, next_state)

#define BTTRC_AVDT_SCB_DBG0(dbg)  \
            BTTRC_DBG0(BTTRC_ID_AVDT_SCB, dbg)
#define BTTRC_AVDT_SCB_DBG1(dbg, p1_t,p1_v)  \
            BTTRC_DBG1(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v)
#define BTTRC_AVDT_SCB_DBG2(dbg, p1_t,p1_v,p2_t,p2_v)  \
            BTTRC_DBG2(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v,p2_t,p2_v)
#define BTTRC_AVDT_SCB_DBG3(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)  \
            BTTRC_DBG3(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_AVDT_SCB_DBG4(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)  \
            BTTRC_DBG4(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_AVDT_SCB_DBG5(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)  \
            BTTRC_DBG5(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_AVDT_SCB_DBG6(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)  \
            BTTRC_DBG6(BTTRC_ID_AVDT_SCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)
/***************************************************************************************/
/*AVDT_CCB MACROS */

#define BTTRC_AVDT_CCB_EVENT(event, state)   \
            BTTRC_EVENT(BTTRC_ID_AVDT_CCB, event, state)
#define BTTRC_AVDT_CCB_ACTION(action)  \
            BTTRC_ACTION(BTTRC_ID_AVDT_CCB, action)
#define BTTRC_AVDT_CCB_STATE(next_state)   \
            BTTRC_STATE(BTTRC_ID_AVDT_CCB, next_state)

#define BTTRC_AVDT_CCB_DBG0(dbg)  \
            BTTRC_DBG0(BTTRC_ID_AVDT_CCB, dbg)
#define BTTRC_AVDT_CCB_DBG1(dbg, p1_t,p1_v)  \
            BTTRC_DBG1(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v)
#define BTTRC_AVDT_CCB_DBG2(dbg, p1_t,p1_v,p2_t,p2_v)  \
            BTTRC_DBG2(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v,p2_t,p2_v)
#define BTTRC_AVDT_CCB_DBG3(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)  \
            BTTRC_DBG3(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_AVDT_CCB_DBG4(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)  \
            BTTRC_DBG4(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_AVDT_CCB_DBG5(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)  \
            BTTRC_DBG5(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_AVDT_CCB_DBG6(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)  \
            BTTRC_DBG6(BTTRC_ID_AVDT_CCB, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)
/***************************************************************************************/

#else /*BTTRC_INCLUDED*/

/***************************************************************************************/
/* BTTRC MACROS */

#define BTTRC_EVENT(lid, event, state)
#define BTTRC_ACTION(lid, action)
#define BTTRC_STATE(lid, state)

#define BTTRC_API0(lid, api)
#define BTTRC_API1(lid, api, p1_t, p1_v)
#define BTTRC_API2(lid, api, p1_t, p1_v, p2_t, p2_v)
#define BTTRC_API3(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_API4(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_API5(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_API6(lid, api, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)


#define BTTRC_DBG0(lid, dbg)
#define BTTRC_DBG1(lid, dbg, p1_t,p1_v)
#define BTTRC_DBG2(lid, dbg, p1_t,p1_v,p2_t,p2_v)
#define BTTRC_DBG3(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_DBG4(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_DBG5(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_DBG6(lid, dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)

/***************************************************************************************/
/*AVDT MACROS */
#define BTTRC_AVDT_API0(api)
#define BTTRC_AVDT_API1(api, p1_t,p1_v)
#define BTTRC_AVDT_API2(api, p1_t,p1_v,p2_t,p2_v)
/***************************************************************************************/
/*AVDT_SCB MACROS */

#define BTTRC_AVDT_SCB_EVENT(event, state)
#define BTTRC_AVDT_SCB_ACTION(action)
#define BTTRC_AVDT_SCB_STATE(next_state)

#define BTTRC_AVDT_SCB_DBG0(dbg)
#define BTTRC_AVDT_SCB_DBG1(dbg, p1_t,p1_v)
#define BTTRC_AVDT_SCB_DBG2(dbg, p1_t,p1_v,p2_t,p2_v)
#define BTTRC_AVDT_SCB_DBG3(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_AVDT_SCB_DBG4(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_AVDT_SCB_DBG5(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_AVDT_SCB_DBG6(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)

/***************************************************************************************/
/*AVDT_CCB MACROS */

#define BTTRC_AVDT_CCB_EVENT(event, state)
#define BTTRC_AVDT_CCB_ACTION(action)
#define BTTRC_AVDT_CCB_STATE(next_state)

#define BTTRC_AVDT_CCB_DBG0(dbg)
#define BTTRC_AVDT_CCB_DBG1(dbg, p1_t,p1_v)
#define BTTRC_AVDT_CCB_DBG2(dbg, p1_t,p1_v,p2_t,p2_v)
#define BTTRC_AVDT_CCB_DBG3(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v)
#define BTTRC_AVDT_CCB_DBG4(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v)
#define BTTRC_AVDT_CCB_DBG5(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v)
#define BTTRC_AVDT_CCB_DBG6(dbg, p1_t,p1_v,p2_t,p2_v,p3_t,p3_v,p4_t,p4_v,p5_t,p5_v,p6_t,p6_v)

/***************************************************************************************/

#endif /*BTTRC_INCLUDED*/


#if (BT_USE_TRACES == TRUE)

#define BT_TRACE(l,t,...)                        LogMsg((TRACE_CTRL_GENERAL | (l) | TRACE_ORG_STACK | (t)), ##__VA_ARGS__)
#define BT_ERROR_TRACE(l,...)                    LogMsg(TRACE_CTRL_GENERAL | (l) | TRACE_ORG_STACK | TRACE_TYPE_ERROR, ##__VA_ARGS__)

/* Define tracing for the HCI unit
*/

#define HCI_TRACE_ERROR(...)                     {if (btu_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_HCI, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define HCI_TRACE_WARNING(...)                   {if (btu_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_HCI, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define HCI_TRACE_EVENT(...)                     {if (btu_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_HCI, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define HCI_TRACE_DEBUG(...)                     {if (btu_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}


/* Define tracing for BTM
*/
#define BTM_TRACE_ERROR(...)                     {if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define BTM_TRACE_WARNING(...)                   {if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define BTM_TRACE_API(...)                       {if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_API, ##__VA_ARGS__);}
#define BTM_TRACE_EVENT(...)                     {if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define BTM_TRACE_DEBUG(...)                     {if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}


/* Define tracing for the L2CAP unit
*/
#define L2CAP_TRACE_ERROR(...)                   {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define L2CAP_TRACE_WARNING(...)                 {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define L2CAP_TRACE_API(...)                     {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define L2CAP_TRACE_EVENT(...)                   {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define L2CAP_TRACE_DEBUG(...)                   {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define L2CAP_TRACE_VERBOSE(...)                 {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_VERBOSE) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* Define tracing for the SDP unit
*/
#define SDP_TRACE_ERROR(...)                     {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_SDP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define SDP_TRACE_WARNING(...)                   {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_SDP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define SDP_TRACE_API(...)                       {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_SDP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define SDP_TRACE_EVENT(...)                     {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_SDP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define SDP_TRACE_DEBUG(...)                     {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_SDP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* Define tracing for the RFCOMM unit
*/
#define RFCOMM_TRACE_ERROR(...)                  {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_RFCOMM, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define RFCOMM_TRACE_WARNING(...)                {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_RFCOMM, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define RFCOMM_TRACE_API(...)                    {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_RFCOMM, TRACE_TYPE_API, ##__VA_ARGS__);}
#define RFCOMM_TRACE_EVENT(...)                  {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_RFCOMM, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define RFCOMM_TRACE_DEBUG(...)                  {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_RFCOMM, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* define traces for L2c sock */
#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
#define L2C_SOCK_TRACE_ERROR(...)                  {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define L2C_SOCK_TRACE_WARNING(...)                {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define L2C_SOCK_TRACE_API(...)                    {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define L2C_SOCK_TRACE_EVENT(...)                  {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define L2C_SOCK_TRACE_DEBUG(...)                  {if (l2cb.l2cap_trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_L2CAP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#endif

/* Generic Access Profile traces */
#define GAP_TRACE_ERROR(...)                     {if (gap_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_GAP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define GAP_TRACE_EVENT(...)                     {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_GAP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define GAP_TRACE_API(...)                       {if (gap_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_GAP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define GAP_TRACE_WARNING(...)                   {if (gap_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_GAP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}

/* define traces for HID Host */
#define HIDH_TRACE_ERROR(...)                     {if (hh_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define HIDH_TRACE_WARNING(...)                   {if (hh_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define HIDH_TRACE_API(...)                       {if (hh_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_API, ##__VA_ARGS__);}
#define HIDH_TRACE_EVENT(...)                     {if (hh_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define HIDH_TRACE_DEBUG(...)                     {if (hh_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define HIDH_TRACE_VERBOSE(...)                   {if (hh_cb.trace_level >= BT_TRACE_LEVEL_VERBOSE) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* define traces for HID Device */
#define HIDD_TRACE_ERROR(...)                     {if (hd_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define HIDD_TRACE_WARNING(...)                   {if (hd_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define HIDD_TRACE_API(...)                       {if (hd_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_API, ##__VA_ARGS__);}
#define HIDD_TRACE_EVENT(...)                     {if (hd_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define HIDD_TRACE_DEBUG(...)                     {if (hd_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define HIDD_TRACE_VERBOSE(...)                   {if (hd_cb.trace_level >= BT_TRACE_LEVEL_VERBOSE) BT_TRACE(TRACE_LAYER_HID, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* define traces for BNEP */

#define BNEP_TRACE_ERROR(...)                     {if (bnep_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_BNEP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define BNEP_TRACE_WARNING(...)                   {if (bnep_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_BNEP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define BNEP_TRACE_API(...)                       {if (bnep_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_BNEP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define BNEP_TRACE_EVENT(...)                     {if (bnep_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_BNEP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define BNEP_TRACE_DEBUG(...)                     {if (bnep_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_BNEP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* define traces for PAN */

#define PAN_TRACE_ERROR(...)                     {if (pan_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_PAN, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define PAN_TRACE_WARNING(...)                   {if (pan_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_PAN, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define PAN_TRACE_API(...)                       {if (pan_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_PAN, TRACE_TYPE_API, ##__VA_ARGS__);}
#define PAN_TRACE_EVENT(...)                     {if (pan_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_PAN, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define PAN_TRACE_DEBUG(...)                     {if (pan_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_PAN, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* Define tracing for the A2DP profile
*/
#define A2D_TRACE_ERROR(...)                      {if (a2d_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_A2D, TRACE_TYPE_ERROR,##__VA_ARGS__);}
#define A2D_TRACE_WARNING(...)                    {if (a2d_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_A2D, TRACE_TYPE_WARNING,##__VA_ARGS__);}
#define A2D_TRACE_EVENT(...)                      {if (a2d_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_A2D, TRACE_TYPE_EVENT,##__VA_ARGS__);}
#define A2D_TRACE_DEBUG(...)                      {if (a2d_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_A2D, TRACE_TYPE_DEBUG,##__VA_ARGS__);}
#define A2D_TRACE_API(...)                        {if (a2d_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_A2D, TRACE_TYPE_API,##__VA_ARGS__);}

/* AVDTP
*/
#define AVDT_TRACE_ERROR(...)                     {if (avdt_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define AVDT_TRACE_WARNING(...)                   {if (avdt_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define AVDT_TRACE_EVENT(...)                     {if (avdt_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define AVDT_TRACE_DEBUG(...)                     {if (avdt_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define AVDT_TRACE_API(...)                       {if (avdt_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_API, ##__VA_ARGS__);}

/* Define tracing for the AVCTP protocol
*/
#define AVCT_TRACE_ERROR(...)                     {if (avct_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define AVCT_TRACE_WARNING(...)                   {if (avct_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define AVCT_TRACE_EVENT(...)                     {if (avct_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define AVCT_TRACE_DEBUG(...)                     {if (avct_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define AVCT_TRACE_API(...)                       {if (avct_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_API, ##__VA_ARGS__);}

/* Define tracing for the AVRCP profile
*/
#define AVRC_TRACE_ERROR(...)                      {if (avrc_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define AVRC_TRACE_WARNING(...)                    {if (avrc_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define AVRC_TRACE_EVENT(...)                      {if (avrc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define AVRC_TRACE_DEBUG(...)                      {if (avrc_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define AVRC_TRACE_API(...)                        {if (avrc_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_AVP, TRACE_TYPE_API, ##__VA_ARGS__);}

/* MCAP
*/
#define MCA_TRACE_ERROR(...)                     {if (mca_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_MCA, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define MCA_TRACE_WARNING(...)                   {if (mca_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_MCA, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define MCA_TRACE_EVENT(...)                     {if (mca_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_MCA, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define MCA_TRACE_DEBUG(...)                     {if (mca_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_MCA, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define MCA_TRACE_API(...)                       {if (mca_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_MCA, TRACE_TYPE_API, ##__VA_ARGS__);}

/* Define tracing for the ATT/GATT unit
*/
#define GATT_TRACE_ERROR(...)                     {if (gatt_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_ATT, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define GATT_TRACE_WARNING(...)                   {if (gatt_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_ATT, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define GATT_TRACE_API(...)                       {if (gatt_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_ATT, TRACE_TYPE_API, ##__VA_ARGS__);}
#define GATT_TRACE_EVENT(...)                     {if (gatt_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_ATT, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define GATT_TRACE_DEBUG(...)                     {if (gatt_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_ATT, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* Define tracing for the SMP unit
*/
#define SMP_TRACE_ERROR(...)                     {if (smp_cb.trace_level >= BT_TRACE_LEVEL_ERROR) BT_TRACE(TRACE_LAYER_SMP, TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define SMP_TRACE_WARNING(...)                   {if (smp_cb.trace_level >= BT_TRACE_LEVEL_WARNING) BT_TRACE(TRACE_LAYER_SMP, TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define SMP_TRACE_API(...)                       {if (smp_cb.trace_level >= BT_TRACE_LEVEL_API) BT_TRACE(TRACE_LAYER_SMP, TRACE_TYPE_API, ##__VA_ARGS__);}
#define SMP_TRACE_EVENT(...)                     {if (smp_cb.trace_level >= BT_TRACE_LEVEL_EVENT) BT_TRACE(TRACE_LAYER_SMP, TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define SMP_TRACE_DEBUG(...)                     {if (smp_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) BT_TRACE(TRACE_LAYER_SMP, TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

/* END OF USE TRACES */
#else

#define BT_TRACE(l,t,...)
#define BT_BT_ERROR_TRACE(l,...)

/* Define tracing for the HCI unit
*/
#define HCI_TRACE_ERROR(...)
#define HCI_TRACE_WARNING(...)
#define HCI_TRACE_EVENT(...)
#define HCI_TRACE_DEBUG(...)


/* Define tracing for BTM
*/
#define BTM_TRACE_ERROR(...)
#define BTM_TRACE_WARNING(...)
#define BTM_TRACE_API(...)
#define BTM_TRACE_EVENT(...)
#define BTM_TRACE_DEBUG(...)


/* Define tracing for the L2CAP unit
*/
#define L2CAP_TRACE_ERROR(...)
#define L2CAP_TRACE_WARNING(...)
#define L2CAP_TRACE_API(...)
#define L2CAP_TRACE_EVENT(...)
#define L2CAP_TRACE_DEBUG(...)
#define L2CAP_TRACE_VERBOSE(...)

/* Define tracing for the SDP unit
*/
#define SDP_TRACE_ERROR(...)
#define SDP_TRACE_WARNING(...)
#define SDP_TRACE_API(...)
#define SDP_TRACE_EVENT(...)
#define SDP_TRACE_DEBUG(...)

/* Define tracing for the RFCOMM unit
*/
#define RFCOMM_TRACE_ERROR(...)
#define RFCOMM_TRACE_WARNING(...)
#define RFCOMM_TRACE_API(...)
#define RFCOMM_TRACE_EVENT(...)
#define RFCOMM_TRACE_DEBUG(...)

/* Generic Access Profile traces */
#define GAP_TRACE_ERROR(...)
#define GAP_TRACE_EVENT(...)
#define GAP_TRACE_API(...)
#define GAP_TRACE_WARNING(...)

/* define traces for HID Host */
#define HIDH_TRACE_ERROR(...)
#define HIDH_TRACE_WARNING(...)
#define HIDH_TRACE_API(...)
#define HIDH_TRACE_EVENT(...)
#define HIDH_TRACE_DEBUG(...)
#define HIDH_TRACE_VERBOSE(...)

/* define traces for HID Device */
#define HIDD_TRACE_ERROR(...)
#define HIDD_TRACE_WARNING(...)
#define HIDD_TRACE_API(...)
#define HIDD_TRACE_EVENT(...)
#define HIDD_TRACE_DEBUG(...)
#define HIDD_TRACE_VERBOSE(...)

/* define traces for BNEP */

#define BNEP_TRACE_ERROR(...)
#define BNEP_TRACE_WARNING(...)
#define BNEP_TRACE_API(...)
#define BNEP_TRACE_EVENT(...)
#define BNEP_TRACE_DEBUG(...)

/* define traces for PAN */

#define PAN_TRACE_ERROR(...)
#define PAN_TRACE_WARNING(...)
#define PAN_TRACE_API(...)
#define PAN_TRACE_EVENT(...)
#define PAN_TRACE_DEBUG(...)

/* Define tracing for the A2DP profile
*/
#define A2D_TRACE_ERROR(...)
#define A2D_TRACE_WARNING(...)
#define A2D_TRACE_EVENT(...)
#define A2D_TRACE_DEBUG(...)
#define A2D_TRACE_API(...)

/* AVDTP
*/
#define AVDT_TRACE_ERROR(...)
#define AVDT_TRACE_WARNING(...)
#define AVDT_TRACE_EVENT(...)
#define AVDT_TRACE_DEBUG(...)
#define AVDT_TRACE_API(...)

/* Define tracing for the AVCTP protocol
*/
#define AVCT_TRACE_ERROR(...)
#define AVCT_TRACE_WARNING(...)
#define AVCT_TRACE_EVENT(...)
#define AVCT_TRACE_DEBUG(...)
#define AVCT_TRACE_API(...)

/* Define tracing for the AVRCP profile
*/
#define AVRC_TRACE_ERROR(...)
#define AVRC_TRACE_WARNING(...)
#define AVRC_TRACE_EVENT(...)
#define AVRC_TRACE_DEBUG(...)
#define AVRC_TRACE_API(...)

/* MCAP
*/
#define MCA_TRACE_ERROR(...)
#define MCA_TRACE_WARNING(...)
#define MCA_TRACE_EVENT(...)
#define MCA_TRACE_DEBUG(...)
#define MCA_TRACE_API(...)

/* Define tracing for the GATT
*/
#define GATT_TRACE_ERROR(...)
#define GATT_TRACE_WARNING(...)
#define GATT_TRACE_API(...)
#define GATT_TRACE_EVENT(...)
#define GATT_TRACE_DEBUG(...)

/* Define tracing for the SMP unit
*/
#define SMP_TRACE_ERROR(...)
#define SMP_TRACE_WARNING(...)
#define SMP_TRACE_API(...)
#define SMP_TRACE_EVENT(...)
#define SMP_TRACE_DEBUG(...)

#endif

#if (BT_TRACE_BTIF == TRUE)

extern UINT8 btif_trace_level;
extern UINT8 audio_latency_trace_level;

/* define traces for application */
#define BTIF_TRACE_IMP(...)                      {LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define BTIF_TRACE_ERROR(...)                    {if (btif_trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define BTIF_TRACE_WARNING(...)                  {if (btif_trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define BTIF_TRACE_API(...)                      {if (btif_trace_level >= BT_TRACE_LEVEL_API) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_API, ##__VA_ARGS__);}
#define BTIF_TRACE_EVENT(...)                    {if (btif_trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define BTIF_TRACE_DEBUG(...)                    {if (btif_trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define BTIF_TRACE_VERBOSE(...)                  {if (btif_trace_level >= BT_TRACE_LEVEL_VERBOSE) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

#else
/* define traces for Application */

#define BTIF_TRACE_IMP(...)
#define BTIF_TRACE_ERROR(...)
#define BTIF_TRACE_WARNING(...)
#define BTIF_TRACE_API(...)
#define BTIF_TRACE_EVENT(...)
#define BTIF_TRACE_DEBUG(...)
#define BTIF_TRACE_VERBOSE(...)

#endif


#if (BT_USE_TRACES == TRUE || BT_TRACE_APPL == TRUE)

/* define traces for application */
#define APPL_TRACE_IMP(...)                      {LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define APPL_TRACE_ERROR(...)                    {if (appl_trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define APPL_TRACE_WARNING(...)                  {if (appl_trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define APPL_TRACE_API(...)                      {if (appl_trace_level >= BT_TRACE_LEVEL_API) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_API, ##__VA_ARGS__);}
#define APPL_TRACE_EVENT(...)                    {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define APPL_TRACE_DEBUG(...)                    {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define APPL_TRACE_VERBOSE(...)                  {if (appl_trace_level >= BT_TRACE_LEVEL_VERBOSE) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define APPL_TRACE_LATENCY_AUDIO(...)            {if (audio_latency_trace_level >= BT_TRACE_LEVEL_VERBOSE) LogMsg(TRACE_CTRL_GENERAL | TRACE_LAYER_NONE | TRACE_ORG_APPL | TRACE_TYPE_DEBUG, ##__VA_ARGS__);}

#else
/* define traces for Application */

#define APPL_TRACE_IMP(...)
#define APPL_TRACE_ERROR(...)
#define APPL_TRACE_WARNING(...)
#define APPL_TRACE_API(...)
#define APPL_TRACE_EVENT(...)
#define APPL_TRACE_DEBUG(...)
#define APPL_TRACE_VERBOSE(...)
#define APPL_TRACE_LATENCY_AUDIO(...)

#endif

/* Simplified Trace Helper Macro
*/
#if (BT_USE_TRACES == TRUE)
#define bdld(fmt, ...) \
    do{\
        if((MY_LOG_LEVEL) >= BT_TRACE_LEVEL_DEBUG) \
            LogMsg((MY_LOG_LAYER) | TRACE_TYPE_DEBUG, "%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)

#define bdlw(fmt, ...) \
    do{\
        if((MY_LOG_LEVEL) >= BT_TRACE_LEVEL_DEBUG) \
            LogMsg((MY_LOG_LAYER) | TRACE_TYPE_WARNING, "%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)

#define bdle(fmt, ...) \
    do{\
        if((MY_LOG_LEVEL) >= BT_TRACE_LEVEL_DEBUG) \
            LogMsg((MY_LOG_LAYER) | TRACE_TYPE_ERROR, "%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)

#define bdla(assert_if) \
    do{\
        if(((MY_LOG_LEVEL) >= BT_TRACE_LEVEL_ERROR) && !(assert_if)) \
            LogMsg((MY_LOG_LAYER) | TRACE_TYPE_ERROR, "%s(L%d): assert failed: " #assert_if, __FUNCTION__, __LINE__); \
    }while(0)
#else
#define bdld(fmt, ...)  ((void)0) /*Empty statement as placeholder*/
#define bdlw(fmt, ...)  ((void)0)
#define bdle(fmt, ...)  ((void)0)
#define bdla(assert_if) ((void)0)
#endif
#endif /* BT_TRACE_H */
