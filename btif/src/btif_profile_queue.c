/*******************************************************************************
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

/*******************************************************************************
 *
 *  Filename:      btif_profile_queue.c
 *
 *  Description:   Bluetooth remote device connection queuing implementation.
 *
 ******************************************************************************/

#include <hardware/bluetooth.h>

#define LOG_TAG "BTIF_QUEUE"
#include "btif_common.h"
#include "btif_profile_queue.h"
#include "gki.h"

/*******************************************************************************
**  Local type definitions
*******************************************************************************/

typedef enum {
  BTIF_QUEUE_CONNECT_EVT,
  BTIF_QUEUE_ADVANCE_EVT
} btif_queue_event_t;

typedef struct connect_node_tag
{
    bt_bdaddr_t bda;
    uint16_t uuid;
    uint16_t busy;
    void *p_cb;
    struct connect_node_tag *p_next;
} __attribute__((packed))connect_node_t;


/*******************************************************************************
**  Static variables
*******************************************************************************/

static connect_node_t *connect_queue;


/*******************************************************************************
**  Queue helper functions
*******************************************************************************/

static void queue_int_add(connect_node_t *p_param)
{
    connect_node_t *p_list = connect_queue;
    connect_node_t *p_node = GKI_getbuf(sizeof(connect_node_t));
    ASSERTC(p_node != NULL, "Failed to allocate new list node", 0);

    memcpy(p_node, p_param, sizeof(connect_node_t));

    if (connect_queue == NULL)
    {
        connect_queue = p_node;
        return;
    }

    while (p_list->p_next)
        p_list = p_list->p_next;
    p_list->p_next = p_node;
}

static void queue_int_advance()
{
    connect_node_t *p_head = connect_queue;
    if (connect_queue == NULL)
        return;

    connect_queue = connect_queue->p_next;
    GKI_freebuf(p_head);
}

static bt_status_t queue_int_connect_next()
{
    connect_node_t* p_head = connect_queue;

    if (p_head == NULL)
        return BT_STATUS_FAIL;

    /* If the queue is currently busy, we return  success anyway,
     * since the connection has been queued... */
    if (p_head->busy != FALSE)
        return BT_STATUS_SUCCESS;

    p_head->busy = TRUE;
    return (*(btif_connect_cb_t*)p_head->p_cb)(&p_head->bda);
}

static void queue_int_handle_evt(UINT16 event, char *p_param)
{
    switch(event)
    {
        case BTIF_QUEUE_CONNECT_EVT:
            queue_int_add((connect_node_t*)p_param);
            break;

        case BTIF_QUEUE_ADVANCE_EVT:
            queue_int_advance();
            break;
    }

    queue_int_connect_next();
}

/*******************************************************************************
**
** Function         btif_queue_connect
**
** Description      Add a new connection to the queue and trigger the next
**                  scheduled connection.
**
** Returns          BT_STATUS_SUCCESS if successful
**
*******************************************************************************/
bt_status_t btif_queue_connect(uint16_t uuid, const bt_bdaddr_t *bda,
                        btif_connect_cb_t *connect_cb)
{
    connect_node_t node;
    memset(&node, 0, sizeof(connect_node_t));
    memcpy(&(node.bda), bda, sizeof(bt_bdaddr_t));
    node.uuid = uuid;
    node.p_cb = connect_cb;

    return btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_CONNECT_EVT,
                          (char*)&node, sizeof(connect_node_t), NULL);
}

/*******************************************************************************
**
** Function         btif_queue_advance
**
** Description      Clear the queue's busy status and advance to the next
**                  scheduled connection.
**
** Returns          void
**
*******************************************************************************/
void btif_queue_advance()
{
    btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_ADVANCE_EVT,
                          NULL, 0, NULL);
}

