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

#ifndef BT_HCI_LIB_H
#define BT_HCI_LIB_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

/** Struct types */


/** Typedefs and defines */

/* Generic purpose transac returned upon request complete */
typedef void* TRANSAC;

/** Bluetooth Power Control States */
typedef enum {
    BT_HC_CHIP_PWR_OFF,
    BT_HC_CHIP_PWR_ON,
}  bt_hc_chip_power_state_t;

/** Bluetooth Low Power Mode */
typedef enum {
    BT_HC_LPM_DISABLE,
    BT_HC_LPM_ENABLE,
    BT_HC_LPM_WAKE_ASSERT,
    BT_HC_LPM_WAKE_DEASSERT,
} bt_hc_low_power_event_t;

/** Receive flow control */
typedef enum {
    BT_RXFLOW_OFF, /* add transport device fd to select set */
    BT_RXFLOW_ON,  /* remove transport device to from select set */
} bt_rx_flow_state_t;

/** HCI logging control */
typedef enum {
    BT_HC_LOGGING_OFF,
    BT_HC_LOGGING_ON,
} bt_hc_logging_state_t;

/** Result of write request */
typedef enum {
    BT_HC_TX_SUCCESS,  /* a buffer is fully processed and can be released */
    BT_HC_TX_FAIL,     /* transmit fail */
    BT_HC_TX_FRAGMENT, /* send split ACL pkt back to stack to reprocess */
} bt_hc_transmit_result_t;

/** Result of preload initialization */
typedef enum {
    BT_HC_PRELOAD_SUCCESS,
    BT_HC_PRELOAD_FAIL,
} bt_hc_preload_result_t;

/** Result of postload initialization */
typedef enum {
    BT_HC_POSTLOAD_SUCCESS,
    BT_HC_POSTLOAD_FAIL,
} bt_hc_postload_result_t;

/** Result of low power enable/disable request */
typedef enum {
    BT_HC_LPM_DISABLED,
    BT_HC_LPM_ENABLED,
} bt_hc_lpm_request_result_t;

/** Host/Controller Library Return Status */
typedef enum {
    BT_HC_STATUS_SUCCESS,
    BT_HC_STATUS_FAIL,
    BT_HC_STATUS_NOT_READY,
    BT_HC_STATUS_NOMEM,
    BT_HC_STATUS_BUSY,
    BT_HC_STATUS_CORRUPTED_BUFFER
} bt_hc_status_t;


/* Section comment */

/*
 * Bluetooth Host/Controller callback structure.
 */

/* called upon bt host wake signal */
typedef void (*hostwake_ind_cb)(bt_hc_low_power_event_t event);

/* preload initialization callback */
typedef void (*preload_result_cb)(TRANSAC transac, bt_hc_preload_result_t result);

/* postload initialization callback */
typedef void (*postload_result_cb)(TRANSAC transac, bt_hc_postload_result_t result);

/* lpm enable/disable callback */
typedef void (*lpm_result_cb)(bt_hc_lpm_request_result_t result);

/* datapath buffer allocation callback (callout) */
typedef char* (*alloc_mem_cb)(int size);

/* datapath buffer deallocation callback (callout) */
typedef int (*dealloc_mem_cb)(TRANSAC transac, char *p_buf);

/* transmit result callback */
typedef int (*tx_result_cb)(TRANSAC transac, char *p_buf, bt_hc_transmit_result_t result);

/* a previously setup buffer is read and available for processing
   buffer is deallocated in stack when processed */
typedef int (*data_ind_cb)(TRANSAC transac, char *p_buf, int len);

typedef struct {
    /** set to sizeof(bt_hc_callbacks_t) */
    size_t         size;

    /* notifies caller result of preload request */
    preload_result_cb  preload_cb;
    
    /* notifies caller result of postload request */
    postload_result_cb  postload_cb;

    /* notifies caller result of lpm enable/disable */
    lpm_result_cb  lpm_cb;

    /* notifies hardware on host wake state */
    hostwake_ind_cb       hostwake_ind;

    /* buffer allocation request */
    alloc_mem_cb   alloc;

    /* buffer deallocation request */
    dealloc_mem_cb dealloc;

    /* notifies stack data is available */
    data_ind_cb data_ind;

    /* notifies caller when a buffer is transmitted (or failed) */
    tx_result_cb  tx_result;
} bt_hc_callbacks_t;

/*
 * Bluetooth Host/Controller Interface
 */
typedef struct {
    /** Set to sizeof(bt_hc_interface_t) */
    size_t          size;

    /**
     * Opens the interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int   (*init)(const bt_hc_callbacks_t* p_cb, unsigned char *local_bdaddr);

    /** Chip power control */
    void (*set_power)(bt_hc_chip_power_state_t state);

    /** Set low power mode wake */
    int   (*lpm)(bt_hc_low_power_event_t event);

    /** Called prior to stack initialization */
    void (*preload)(TRANSAC transac);

    /** Called post stack initialization */
    void (*postload)(TRANSAC transac);

    /** Transmit buffer */
    int (*transmit_buf)(TRANSAC transac, char *p_buf, int len);

    /** Controls receive flow */
    int (*set_rxflow)(bt_rx_flow_state_t state);

    /** Controls HCI logging on/off */
    int (*logging)(bt_hc_logging_state_t state, char *p_path);

    /** Closes the interface */
    void  (*cleanup)( void );
} bt_hc_interface_t;


/* 
 * External shared lib functions
 */

extern const bt_hc_interface_t* bt_hc_get_interface(void);

#endif /* BT_HCI_LIB_H */

