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
 *  This file contains L2CAP socket interface functions for the BTA layer.
 *
 ******************************************************************************/

#ifndef L2C_SOCK_API_H
#define L2C_SOCK_API_H

#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

#define L2C_SOCK_DATA_CBACK_TYPE_INCOMING          1
#define L2C_SOCK_DATA_CBACK_TYPE_OUTGOING_SIZE     2
#define L2C_SOCK_DATA_CBACK_TYPE_OUTGOING          3

typedef int  (tL2C_SOCK_DATA_CBACK) (UINT16 sock_handle, UINT8* p_buf, UINT16 len, int type);

typedef void (tL2C_SOCK_CBACK) (UINT16 sock_handle, UINT16 event);

/*
** Define l2c socket status codes
*/
#define L2C_SOCK_SUCCESS                0

#define L2C_SOCK_ERR_BASE               0

#define L2C_SOCK_UNKNOWN_ERROR          (L2C_SOCK_ERR_BASE + 1)
#define L2C_SOCK_ALREADY_OPENED         (L2C_SOCK_ERR_BASE + 2)
#define L2C_SOCK_INVALID_HANDLE         (L2C_SOCK_ERR_BASE + 3)
#define L2C_SOCK_NOT_OPENED             (L2C_SOCK_ERR_BASE + 4)
#define L2C_SOCK_NOT_CONNECTED          (L2C_SOCK_ERR_BASE + 5)
#define L2C_SOCK_PEER_CONNECTION_FAILED (L2C_SOCK_ERR_BASE + 6)
#define L2C_SOCK_CLOSED                 (L2C_SOCK_ERR_BASE + 7)
#define L2C_SOCK_LOCAL_TIMEOUT          (L2C_SOCK_ERR_BASE + 8)
#define L2C_SOCK_INVALID_PSM            (L2C_SOCK_ERR_BASE + 9)
#define L2C_SOCK_CONGESTED              (L2C_SOCK_ERR_BASE + 10)
#define L2C_SOCK_UNCONGESTED            (L2C_SOCK_ERR_BASE + 11)
#define L2C_SOCK_TX_EMPTY               (L2C_SOCK_ERR_BASE + 12)


int SOCK_L2C_CreateConnection (UINT16 psm, BOOLEAN is_server, BD_ADDR bd_addr,
                               UINT16 *p_handle, tL2C_SOCK_CBACK *p_sock_mgmt_cback);

int SOCK_L2C_RemoveConnection (UINT16 handle);

int SOCK_L2C_SetDataCallback (UINT16 sock_handle, tL2C_SOCK_DATA_CBACK *p_l2c_sock_cb);

int SOCK_L2C_WriteData (UINT16 sock_handle, int* p_len);

UINT8 *SOCK_L2C_ConnGetRemoteAddr (UINT16 sock_handle);

void L2C_SOCK_Init (void);

#ifdef __cplusplus
}
#endif

#endif
#endif
