/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
#include "list.h"

/*******************************************************************************
**  Local type definitions
*******************************************************************************/

typedef enum {
  BTIF_QUEUE_CONNECT_EVT,
  BTIF_QUEUE_ADVANCE_EVT,
} btif_queue_event_t;

typedef struct {
    bt_bdaddr_t bda;
    uint16_t uuid;
    bool busy;
    btif_connect_cb_t connect_cb;
} connect_node_t;

/*******************************************************************************
**  Static variables
*******************************************************************************/

static list_t *connect_queue;

/*******************************************************************************
**  Queue helper functions
*******************************************************************************/

static void queue_int_add(connect_node_t *p_param) {
    connect_node_t *p_node = GKI_getbuf(sizeof(connect_node_t));
    ASSERTC(p_node != NULL, "Failed to allocate new list node", 0);

    memcpy(p_node, p_param, sizeof(connect_node_t));

    if (!connect_queue) {
        connect_queue = list_new(GKI_freebuf);
        ASSERTC(connect_queue != NULL, "Failed to allocate list", 0);
    }

    list_append(connect_queue, p_node);
}

static void queue_int_advance() {
    if (connect_queue && !list_is_empty(connect_queue))
        list_remove(connect_queue, list_front(connect_queue));
}

static bt_status_t queue_int_connect_next() {
    if (!connect_queue || list_is_empty(connect_queue))
        return BT_STATUS_FAIL;

    connect_node_t *p_head = list_front(connect_queue);

    // If the queue is currently busy, we return success anyway,
    // since the connection has been queued...
    if (p_head->busy)
        return BT_STATUS_SUCCESS;

    p_head->busy = true;
    return p_head->connect_cb(&p_head->bda, p_head->uuid);
}

static void queue_int_handle_evt(UINT16 event, char *p_param) {
    switch(event) {
        case BTIF_QUEUE_CONNECT_EVT:
            queue_int_add((connect_node_t *)p_param);
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
bt_status_t btif_queue_connect(uint16_t uuid, const bt_bdaddr_t *bda, btif_connect_cb_t connect_cb) {
    connect_node_t node;
    memset(&node, 0, sizeof(connect_node_t));
    memcpy(&node.bda, bda, sizeof(bt_bdaddr_t));
    node.uuid = uuid;
    node.connect_cb = connect_cb;

    return btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_CONNECT_EVT,
                          (char *)&node, sizeof(connect_node_t), NULL);
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
void btif_queue_advance() {
    btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_ADVANCE_EVT,
                          NULL, 0, NULL);
}

/*******************************************************************************
**
** Function         btif_queue_release
**
** Description      Free up all the queue nodes and set the queue head to NULL
**
** Returns          void
**
*******************************************************************************/
void btif_queue_release() {
    list_free(connect_queue);
    connect_queue = NULL;
}
