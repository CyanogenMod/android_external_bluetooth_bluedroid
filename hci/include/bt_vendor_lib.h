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

#ifndef BT_VENDOR_LIB_H
#define BT_VENDOR_LIB_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

/** Struct types */


/** Typedefs and defines */

/** Vendor operations */
typedef enum {
    BT_VND_OP_POWER_CTRL,
    BT_VND_OP_FW_CFG,
    BT_VND_OP_SCO_CFG,
    BT_VND_OP_USERIAL_OPEN,
    BT_VND_OP_USERIAL_CLOSE,
    BT_VND_OP_USERIAL_SET_BAUD,
    BT_VND_OP_GET_LPM_IDLE_TIMEOUT,
    BT_VND_OP_LPM_SET_MODE,
    BT_VND_OP_LPM_WAKE_SET_STATE,
} bt_vendor_opcode_t;

/** Power on/off control states */
typedef enum {
    BT_VND_PWR_OFF,
    BT_VND_PWR_ON,
}  bt_vendor_power_state_t;

/** LPM WAKE set state request */
typedef enum {
    BT_VND_LPM_WAKE_ASSERT,
    BT_VND_LPM_WAKE_DEASSERT,
} bt_vendor_lpm_wake_state_t;

/** Result of vendor operations */
typedef enum {
    BT_VND_OP_RESULT_SUCCESS,
    BT_VND_OP_RESULT_FAIL,
} bt_vendor_op_result_t;

/*
 * Bluetooth Host/Controller Vendor callback structure.
 */

/* vendor initialization/configuration callback */
typedef void (*cfg_result_cb)(bt_vendor_op_result_t result);

/* datapath buffer allocation callback (callout) */
typedef void* (*malloc_cb)(int size);

/* datapath buffer deallocation callback (callout) */
typedef void (*mdealloc_cb)(void *p_buf);

/* define callback of the cmd_xmit_cb */
typedef void (*tINT_CMD_CBACK)(void *p_mem);

/* hci command packet transmit callback (callout) */
typedef uint8_t (*cmd_xmit_cb)(uint16_t opcode, void *p_buf, tINT_CMD_CBACK p_cback);

typedef struct {
    /** set to sizeof(bt_vendor_callbacks_t) */
    size_t         size;

    /* notifies caller result of firmware configuration request */
    cfg_result_cb  fwcfg_cb;

    /* notifies caller result of sco configuration request */
    cfg_result_cb  scocfg_cb;

    /* notifies caller result of lpm enable/disable */
    cfg_result_cb  lpm_cb;

    /* buffer allocation request */
    malloc_cb   alloc;

    /* buffer deallocation request */
    mdealloc_cb dealloc;

    /* hci command packet transmit request */
    cmd_xmit_cb xmit_cb;
} bt_vendor_callbacks_t;

/*
 * Bluetooth Host/Controller VENDOR Interface
 */
typedef struct {
    /** Set to sizeof(bt_vndor_interface_t) */
    size_t          size;

    /**
     * Opens the interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int   (*init)(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr);

    /**  Vendor specific operations */
    int (*op)(bt_vendor_opcode_t opcode, void *param);

    /** Closes the interface */
    void  (*cleanup)(void);
} bt_vendor_interface_t;


/*
 * External shared lib functions
 */

extern const bt_vendor_interface_t* bt_vendor_get_interface(void);

#endif /* BT_VENDOR_LIB_H */

