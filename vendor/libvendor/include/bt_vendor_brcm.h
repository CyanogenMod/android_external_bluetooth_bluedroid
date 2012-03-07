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
 *  Filename:      bt_vendor_brcm.h
 *
 *  Description:   A wrapper header file of bt_vendor_lib.h
 *
 *                 Contains definitions specific for running Broadcom
 *                 Bluetooth stack on Broadcom Bluetooth Controllers 
 *
 ******************************************************************************/

#ifndef BT_VENDOR_BRCM_H
#define BT_VENDOR_BRCM_H

#include "bt_vendor_lib.h"
#include "vnd_buildcfg.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

#ifndef BTVND_LINUX_BASE_PRIORITY
#define BTVND_LINUX_BASE_PRIORITY 30
#endif

#ifndef BTVND_USERIAL_READ_THREAD_PRIORITY
#define BTVND_USERIAL_READ_THREAD_PRIORITY (BTVND_LINUX_BASE_PRIORITY)
#endif

#ifndef BTVND_MAIN_THREAD_PRIORITY
#define BTVND_MAIN_THREAD_PRIORITY (BTVND_LINUX_BASE_PRIORITY-1)
#endif

#ifndef BTVND_USERIAL_READ_MEM_SIZE
#define BTVND_USERIAL_READ_MEM_SIZE (1024)
#endif

/* Vendor lib internal event ID */
#define VND_EVENT_PRELOAD               0x0001
#define VND_EVENT_POSTLOAD              0x0002
#define VND_EVENT_RX                    0x0004
#define VND_EVENT_TX                    0x0008
#define VND_EVENT_LPM_ENABLE            0x0010
#define VND_EVENT_LPM_DISABLE           0x0020
#define VND_EVENT_LPM_WAKE_DEVICE       0x0040
#define VND_EVENT_LPM_ALLOW_SLEEP       0x0080
#define VND_EVENT_LPM_IDLE_TIMEOUT      0x0100
#define VND_EVENT_EXIT                  0x0200

/* Message event mask across vendor lib and stack */
#define MSG_EVT_MASK                    0xFF00 /* eq. BT_EVT_MASK */
#define MSG_SUB_EVT_MASK                0x00FF /* eq. BT_SUB_EVT_MASK */

/* Message event ID passed from vendor lib to stack */
#define MSG_VND_TO_STACK_HCI_ERR        0x1300 /* eq. BT_EVT_TO_BTU_HCIT_ERR */
#define MSG_VND_TO_STACK_HCI_ACL        0x1100 /* eq. BT_EVT_TO_BTU_HCI_ACL */
#define MSG_VND_TO_STACK_HCI_SCO        0x1200 /* eq. BT_EVT_TO_BTU_HCI_SCO */
#define MSG_VND_TO_STACK_HCI_EVT        0x1000 /* eq. BT_EVT_TO_BTU_HCI_EVT */
#define MSG_VND_TO_STACK_L2C_SEG_XMIT   0x1900 /* eq. BT_EVT_TO_BTU_L2C_SEG_XMIT */

/* Message event ID passed from stack to vendor lib */
#define MSG_STACK_TO_VND_HCI_ACL        0x2100 /* eq. BT_EVT_TO_LM_HCI_ACL */
#define MSG_STACK_TO_VND_HCI_SCO        0x2200 /* eq. BT_EVT_TO_LM_HCI_SCO */
#define MSG_STACK_TO_VND_HCI_CMD        0x2000 /* eq. BT_EVT_TO_LM_HCI_CMD */

/* Local Bluetooth Controller ID for BR/EDR */
#define LOCAL_BR_EDR_CONTROLLER_ID      0

/* Device port name where Bluetooth controller attached */
#ifndef BLUETOOTH_UART_DEVICE_PORT
#define BLUETOOTH_UART_DEVICE_PORT      "/dev/ttyO1"    /* maguro */
#endif

/* Location of firmware patch files */
#ifndef FW_PATCHFILE_LOCATION
#define FW_PATCHFILE_LOCATION ("/vendor/firmware/")  /* maguro */
#endif

#ifndef UART_TARGET_BAUD_RATE
#define UART_TARGET_BAUD_RATE           3000000
#endif

/* sleep mode 

    0: disable
    1: UART with Host wake/BT wake out of band signals
*/
#ifndef LPM_SLEEP_MODE
#define LPM_SLEEP_MODE                  1
#endif

/* Host Stack Idle Threshold in 300ms or 25ms 

  In sleep mode 1, this is the number of firmware loops executed with no
    activity before the Host wake line is deasserted. Activity includes HCI
    traffic excluding certain sleep mode commands and the presence of SCO
    connections if the "Allow Host Sleep During SCO" flag is not set to 1.
    Each count of this parameter is roughly equivalent to 300ms or 25ms.
*/
#ifndef LPM_IDLE_THRESHOLD
#define LPM_IDLE_THRESHOLD              1
#endif

/* Host Controller Idle Threshold in 300ms or 25ms

    This is the number of firmware loops executed with no activity before the
    HC is considered idle. Depending on the mode, HC may then attempt to sleep.
    Activity includes HCI traffic excluding certain sleep mode commands and
    the presence of ACL/SCO connections.
*/
#ifndef LPM_HC_IDLE_THRESHOLD
#define LPM_HC_IDLE_THRESHOLD           1
#endif

/* BT_WAKE Polarity - 0=Active Low, 1= Active High */
#ifndef LPM_BT_WAKE_POLARITY
#define LPM_BT_WAKE_POLARITY            1    /* maguro */
#endif

/* HOST_WAKE Polarity - 0=Active Low, 1= Active High */
#ifndef LPM_HOST_WAKE_POLARITY
#define LPM_HOST_WAKE_POLARITY          1    /* maguro */
#endif

/* LPM_ALLOW_HOST_SLEEP_DURING_SCO

    When this flag is set to 0, the host is not allowed to sleep while
    an SCO is active. In sleep mode 1, the device will keep the host
    wake line asserted while an SCO is active.
    When this flag is set to 1, the host can sleep while an SCO is active.
    This flag should only be set to 1 if SCO traffic is directed to the PCM
    interface.
*/
#ifndef LPM_ALLOW_HOST_SLEEP_DURING_SCO
#define LPM_ALLOW_HOST_SLEEP_DURING_SCO 1
#endif

/* LPM_COMBINE_SLEEP_MODE_AND_LPM

    In Mode 0, always set byte 7 to 0. In sleep mode 1, device always
    requires permission to sleep between scans / periodic inquiries regardless
    of the setting of this byte. In sleep mode 1, if byte is set, device must
    have "permission" to sleep during the low power modes of sniff, hold, and
    park. If byte is not set, device can sleep without permission during these
    modes. Permission to sleep in Mode 1 is obtained if the BT_WAKE signal is
    not asserted.
*/
#ifndef LPM_COMBINE_SLEEP_MODE_AND_LPM
#define LPM_COMBINE_SLEEP_MODE_AND_LPM  1
#endif

/* LPM_ENABLE_UART_TXD_TRI_STATE

    When set to 0, the device will not tristate its UART TX line before going
    to sleep.
    When set to 1, the device will tristate its UART TX line before going to
    sleep.
*/
#ifndef LPM_ENABLE_UART_TXD_TRI_STATE
#define LPM_ENABLE_UART_TXD_TRI_STATE   0
#endif

/* LPM_PULSED_HOST_WAKE
*/
#ifndef LPM_PULSED_HOST_WAKE
#define LPM_PULSED_HOST_WAKE            0
#endif

/* LPM_IDLE_TIMEOUT_MULTIPLE

    The multiple factor of host stack idle threshold in 300ms/25ms 
*/
#ifndef LPM_IDLE_TIMEOUT_MULTIPLE
#define LPM_IDLE_TIMEOUT_MULTIPLE       10
#endif

/* SCO_CFG_INCLUDED

    Do SCO configuration by default. If the firmware patch had been embedded
    with desired SCO configuration, set this FALSE to bypass configuration
    from host software.
*/
#ifndef SCO_CFG_INCLUDED
#define SCO_CFG_INCLUDED                TRUE
#endif

#ifndef SCO_USE_I2S_INTERFACE
#define SCO_USE_I2S_INTERFACE           FALSE
#endif

#if (SCO_USE_I2S_INTERFACE == TRUE)
#define SCO_I2SPCM_PARAM_SIZE           4

/* SCO_I2SPCM_IF_MODE - 0=Disable, 1=Enable */
#ifndef SCO_I2SPCM_IF_MODE
#define SCO_I2SPCM_IF_MODE              1
#endif

/* SCO_I2SPCM_IF_ROLE - 0=Slave, 1=Master */
#ifndef SCO_I2SPCM_IF_ROLE
#define SCO_I2SPCM_IF_ROLE              1
#endif

/* SCO_I2SPCM_IF_SAMPLE_RATE

    0 : 8K
    1 : 16K
    2 : 4K
*/
#ifndef SCO_I2SPCM_IF_SAMPLE_RATE
#define SCO_I2SPCM_IF_SAMPLE_RATE       0
#endif

/* SCO_I2SPCM_IF_CLOCK_RATE

    0 : 128K
    1 : 256K
    2 : 512K
    3 : 1024K
    4 : 2048K
*/
#ifndef SCO_I2SPCM_IF_CLOCK_RATE
#define SCO_I2SPCM_IF_CLOCK_RATE        1
#endif
#endif // SCO_USE_I2S_INTERFACE


#define SCO_PCM_PARAM_SIZE              5

/* SCO_PCM_ROUTING

    0 : PCM
    1 : Transport
    2 : Codec
    3 : I2S
*/
#ifndef SCO_PCM_ROUTING
#define SCO_PCM_ROUTING                 0
#endif

/* SCO_PCM_IF_CLOCK_RATE

    0 : 128K
    1 : 256K
    2 : 512K
    3 : 1024K
    4 : 2048K
*/
#ifndef SCO_PCM_IF_CLOCK_RATE
#define SCO_PCM_IF_CLOCK_RATE           4
#endif

/* SCO_PCM_IF_FRAME_TYPE - 0=Short, 1=Long */
#ifndef SCO_PCM_IF_FRAME_TYPE
#define SCO_PCM_IF_FRAME_TYPE           0
#endif

/* SCO_PCM_IF_SYNC_MODE - 0=Slave, 1=Master */
#ifndef SCO_PCM_IF_SYNC_MODE
#define SCO_PCM_IF_SYNC_MODE            0
#endif

/* SCO_PCM_IF_CLOCK_MODE - 0=Slave, 1=Master */
#ifndef SCO_PCM_IF_CLOCK_MODE
#define SCO_PCM_IF_CLOCK_MODE           0
#endif

#define PCM_DATA_FORMAT_PARAM_SIZE      5

/* PCM_DATA_FMT_SHIFT_MODE

    0 : MSB first
    1 : LSB first
*/
#ifndef PCM_DATA_FMT_SHIFT_MODE
#define PCM_DATA_FMT_SHIFT_MODE         0
#endif

/* PCM_DATA_FMT_FILL_BITS
    
    Specifies the value with which to fill unused bits 
    if Fill_Method is set to programmable
*/
#ifndef PCM_DATA_FMT_FILL_BITS
#define PCM_DATA_FMT_FILL_BITS          0
#endif

/* PCM_DATA_FMT_FILL_METHOD

    0 : 0's
    1 : 1's
    2 : Signed
    3 : Programmable
*/
#ifndef PCM_DATA_FMT_FILL_METHOD
#define PCM_DATA_FMT_FILL_METHOD        3
#endif

/* PCM_DATA_FMT_FILL_NUM

    Specifies the number of bits to be filled
*/
#ifndef PCM_DATA_FMT_FILL_NUM
#define PCM_DATA_FMT_FILL_NUM           3
#endif

/* PCM_DATA_FMT_JUSTIFY_MODE

    0 : Left justify (fill data shifted out last)
    1 : Right justify (fill data shifted out first)
*/
#ifndef PCM_DATA_FMT_JUSTIFY_MODE
#define PCM_DATA_FMT_JUSTIFY_MODE       0
#endif

/******************************************************************************
**  Type definitions and return values
******************************************************************************/

typedef struct
{
    uint16_t          event;
    uint16_t          len;
    uint16_t          offset;
    uint16_t          layer_specific;
} VND_BT_HDR;

#define BT_VND_HDR_SIZE (sizeof(VND_BT_HDR))


typedef struct _vnd_buffer_hdr
{
    struct _vnd_buffer_hdr *p_next;   /* next buffer in the queue */
    uint8_t   reserved1;
    uint8_t   reserved2;
    uint8_t   reserved3;
    uint8_t   reserved4;
} VND_BUFFER_HDR_T;

#define BT_VND_BUFFER_HDR_SIZE (sizeof(VND_BUFFER_HDR_T))

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

extern bt_vendor_callbacks_t *bt_vendor_cbacks;

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        btvnd_signal_event
**
** Description     Perform context switch to bt_vendor main thread
**
** Returns         None
**
*******************************************************************************/
extern void btvnd_signal_event(uint16_t event);

#endif /* BT_VENDOR_BRCM_H */

