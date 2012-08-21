/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_hci_bdroid.h
 *
 *  Description:   A wrapper header file of bt_hci_lib.h
 *
 *                 Contains definitions specific for interfacing with Bluedroid
 *                 Bluetooth stack
 *
 ******************************************************************************/

#ifndef BT_HCI_BDROID_H
#define BT_HCI_BDROID_H

#include "bt_hci_lib.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

#ifndef BTHC_LINUX_BASE_POLICY
#define BTHC_LINUX_BASE_POLICY SCHED_NORMAL
#endif

#if (BTHC_LINUX_BASE_POLICY != SCHED_NORMAL)
#ifndef BTHC_LINUX_BASE_PRIORITY
#define BTHC_LINUX_BASE_PRIORITY 30
#endif

#ifndef BTHC_USERIAL_READ_THREAD_PRIORITY
#define BTHC_USERIAL_READ_THREAD_PRIORITY (BTHC_LINUX_BASE_PRIORITY)
#endif

#ifndef BTHC_MAIN_THREAD_PRIORITY
#define BTHC_MAIN_THREAD_PRIORITY (BTHC_LINUX_BASE_PRIORITY-1)
#endif
#endif  // (BTHC_LINUX_BASE_POLICY != SCHED_NORMAL)

#ifndef BTHC_USERIAL_READ_MEM_SIZE
#define BTHC_USERIAL_READ_MEM_SIZE (1024)
#endif

#ifndef BTSNOOPDISP_INCLUDED
#define BTSNOOPDISP_INCLUDED TRUE
#endif

/* Host/Controller lib internal event ID */
#define HC_EVENT_PRELOAD               0x0001
#define HC_EVENT_POSTLOAD              0x0002
#define HC_EVENT_RX                    0x0004
#define HC_EVENT_TX                    0x0008
#define HC_EVENT_LPM_ENABLE            0x0010
#define HC_EVENT_LPM_DISABLE           0x0020
#define HC_EVENT_LPM_WAKE_DEVICE       0x0040
#define HC_EVENT_LPM_ALLOW_SLEEP       0x0080
#define HC_EVENT_LPM_IDLE_TIMEOUT      0x0100
#define HC_EVENT_EXIT                  0x0200

/* Message event mask across Host/Controller lib and stack */
#define MSG_EVT_MASK                    0xFF00 /* eq. BT_EVT_MASK */
#define MSG_SUB_EVT_MASK                0x00FF /* eq. BT_SUB_EVT_MASK */

/* Message event ID passed from Host/Controller lib to stack */
#define MSG_HC_TO_STACK_HCI_ERR        0x1300 /* eq. BT_EVT_TO_BTU_HCIT_ERR */
#define MSG_HC_TO_STACK_HCI_ACL        0x1100 /* eq. BT_EVT_TO_BTU_HCI_ACL */
#define MSG_HC_TO_STACK_HCI_SCO        0x1200 /* eq. BT_EVT_TO_BTU_HCI_SCO */
#define MSG_HC_TO_STACK_HCI_EVT        0x1000 /* eq. BT_EVT_TO_BTU_HCI_EVT */
#define MSG_HC_TO_STACK_L2C_SEG_XMIT   0x1900 /* eq. BT_EVT_TO_BTU_L2C_SEG_XMIT */

/* Message event ID passed from stack to vendor lib */
#define MSG_STACK_TO_HC_HCI_ACL        0x2100 /* eq. BT_EVT_TO_LM_HCI_ACL */
#define MSG_STACK_TO_HC_HCI_SCO        0x2200 /* eq. BT_EVT_TO_LM_HCI_SCO */
#define MSG_STACK_TO_HC_HCI_CMD        0x2000 /* eq. BT_EVT_TO_LM_HCI_CMD */

/* Local Bluetooth Controller ID for BR/EDR */
#define LOCAL_BR_EDR_CONTROLLER_ID      0

/******************************************************************************
**  Type definitions and return values
******************************************************************************/

typedef struct
{
    uint16_t          event;
    uint16_t          len;
    uint16_t          offset;
    uint16_t          layer_specific;
} HC_BT_HDR;

#define BT_HC_HDR_SIZE (sizeof(HC_BT_HDR))


typedef struct _hc_buffer_hdr
{
    struct _hc_buffer_hdr *p_next;   /* next buffer in the queue */
    uint8_t   reserved1;
    uint8_t   reserved2;
    uint8_t   reserved3;
    uint8_t   reserved4;
} HC_BUFFER_HDR_T;

#define BT_HC_BUFFER_HDR_SIZE (sizeof(HC_BUFFER_HDR_T))

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

extern bt_hc_callbacks_t *bt_hc_cbacks;

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        bthc_signal_event
**
** Description     Perform context switch to bt_hc main thread
**
** Returns         None
**
*******************************************************************************/
extern void bthc_signal_event(uint16_t event);

#endif /* BT_HCI_BDROID_H */
