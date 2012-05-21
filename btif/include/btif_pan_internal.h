/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
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
 ************************************************************************************/


/************************************************************************************
 *
 *  Filename:      btif_pan_internal.h
 *
 *  Description:   Bluetooth pan internal
 *
 *
 ***********************************************************************************/

#ifndef btif_pan_internal_h_
#define btif_pan_internal_h_
#include "btif_pan.h"
#include "bt_types.h"

#define PAN_NAP_SERVICE_NAME "Android Network Access Point"
#define PANU_SERVICE_NAME "Android Network User"
#define TAP_IF_NAME "bt-pan"
#define ETH_ADDR_LEN        6
#ifndef PAN_SECURITY
#define PAN_SECURITY (BTM_SEC_IN_AUTHENTICATE | BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_IN_ENCRYPT | BTM_SEC_OUT_ENCRYPT)
#endif

#define PAN_STATE_UNKNOWN   0
#define PAN_STATE_OPEN      1
#define PAN_STATE_CLOSE     2
#ifndef PAN_ROLE_INACTIVE
#define PAN_ROLE_INACTIVE 0
#endif
typedef struct eth_hdr
{
    unsigned char h_dest[ETH_ADDR_LEN];
    unsigned char h_src[ETH_ADDR_LEN];
    short         h_proto;
} tETH_HDR;
typedef struct
{
    int handle;
    int state;
    UINT16 protocol;
    BD_ADDR peer;
    int local_role;
    int remote_role;
    unsigned char eth_addr[ETH_ADDR_LEN];
} btpan_conn_t;

typedef struct
{
    int btl_if_handle;
    int btl_if_handle_panu;
    int tap_fd;
    int enabled;
    int open_count;
    btpan_conn_t conns[MAX_PAN_CONNS];
} btpan_cb_t;
extern btpan_cb_t btpan_cb;
btpan_conn_t *btpan_new_conn(int handle, const BD_ADDR addr, int local_role, int peer_role);
btpan_conn_t *btpan_find_conn_addr(const BD_ADDR addr);
btpan_conn_t *btpan_find_conn_handle(UINT16 handle);
int btpan_get_connected_count(void);
int btpan_tap_open(void);
void create_tap_read_thread(int tap_fd);
void destroy_tap_read_thread(void);
int btpan_tap_close(int tap_fd);
int btpan_tap_send(int tap_fd, const BD_ADDR src, const BD_ADDR dst, UINT16 protocol,
                   const char* buff, UINT16 size, BOOLEAN ext, BOOLEAN forward);
static inline int is_empty_eth_addr(const BD_ADDR addr)
{
    int i;
    for(i = 0; i < BD_ADDR_LEN; i++)
        if(addr[i] != 0)
            return 0;
    return 1;
}
static inline int is_valid_bt_eth_addr(const BD_ADDR addr)
{
    if(is_empty_eth_addr(addr))
        return 0;
    return addr[0] & 1 ? 0 : 1; //cannot be multicasting address
}

#endif
