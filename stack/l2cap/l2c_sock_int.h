/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/******************************************************************************
 *
 *   This file contains internal data structures and function for the
 *   L2CAP socket implementation.
 *
 ******************************************************************************/
#ifndef L2C_SOCK_INT_H
#define L2C_SOCK_INT_H

#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

#include "l2c_sock_api.h"
#include "l2c_api.h"
#include  "l2c_int.h"
#include "bt_target.h"
#include "gki.h"
#include "rfcdefs.h"
#include "port_api.h"

extern void l2c_sock_init (void);

/*L2CAP socket control block */
typedef struct
{
    UINT8   inx;            /* Index of this sock control block in the array */
    BOOLEAN in_use;         /* True when SCB is allocated */
    UINT8   state;          /* State of the L2C socket */
    UINT16   psm;           /* L2cap psm number */
    UINT16    lcid;         /* Local cid used for this channel */
    BD_ADDR bd_addr;        /* BD ADDR of the device for the corresponding PSM */
    BOOLEAN is_server;      /* TRUE if the server application */
    tL2C_SOCK_CBACK       *p_sock_mgmt_cback;
                            /* Callback function to receive connection  connected/disconnected */
    tL2C_SOCK_DATA_CBACK *p_l2c_sock_data_co_cback;
                            /* Callback function with callouts  */
    UINT16    rem_mtu_size;
    BOOLEAN   local_cfg_sent;
    BOOLEAN   peer_cfg_rcvd;
    BUFFER_Q          tx_queue;    /* Queue of buffers waiting to be sent  */
    BOOLEAN           is_congested;
    tL2CAP_ERTM_INFO  ertm_info; /* Pools and modes for ertm */
    tL2CAP_CFG_INFO   cfg;       /* Configuration */

} tL2C_SOCK_CB;

/* Main Control Block for the L2CAP socket */
typedef struct
{
    tL2C_SOCK_CB l2c_sock[MAX_L2C_SOCK_CONNECTIONS];
    tL2C_SOCK_CB *p_l2c_sock_lcid_cb[MAX_L2CAP_CHANNELS];   /* CB based on the L2CAP's lcid */
    tL2CAP_APPL_INFO  reg_info;  /* L2CAP Registration info */
} tL2C_SOCK_MCB;

extern tL2C_SOCK_MCB l2c_sock_mcb;

/*
 ** Define states and events for the L2C socket state machine
 */
#define L2C_SOCK_STATE_IDLE           0
#define L2C_SOCK_STATE_WAIT_CONN_CNF  1
#define L2C_SOCK_STATE_CONFIGURE      2
#define L2C_SOCK_STATE_CONNECTED      3
#define L2C_SOCK_STATE_WAIT_DISC_CNF  4

/*
 ** L2C socket events
 */
#define L2C_SOCK_EVT_START_REQ        0
#define L2C_SOCK_EVT_START_RSP        1
#define L2C_SOCK_EVT_CLOSE_REQ        2
#define L2C_SOCK_EVT_CONN_CNF         3
#define L2C_SOCK_EVT_CONN_IND         4
#define L2C_SOCK_EVT_CONF_CNF         5
#define L2C_SOCK_EVT_CONF_IND         6
#define L2C_SOCK_EVT_DISC_IND         7
#define L2C_SOCK_EVT_DISC_CNF         8
#define L2C_SOCK_EVT_DATA_IN          9
#define L2C_SOCK_EVT_DATA_OUT         10
#define L2C_SOCK_EVT_TIMEOUT          11

#ifdef __cplusplus
extern "C" {
#endif

tL2C_SOCK_CB  *l2c_sock_find_scb_by_cid (UINT16 cid);
tL2C_SOCK_CB  *l2c_sock_find_scb_by_handle (UINT16 handle);
tL2C_SOCK_CB  *l2c_sock_allocate_scb (void);
void l2c_sock_release_scb (tL2C_SOCK_CB *p_scb);
UINT16 l2c_sock_if_init (UINT16 psm);
void l2c_sock_sm_execute (tL2C_SOCK_CB *p_cb, UINT16 event, void *p_data);
tL2C_SOCK_CB *l2c_sock_alloc_channel (BD_ADDR bd_addr, BOOLEAN is_initiator);

#ifdef __cplusplus
}
#endif

#endif
#endif
