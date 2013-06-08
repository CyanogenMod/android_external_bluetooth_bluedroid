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
 *  Filename:      btif_profile_queue.h
 *
 *  Description:   Bluetooth remote device connection queuing
 *
 *******************************************************************************/

#ifndef BTIF_PROFILE_QUEUE_H
#define BTIF_PROFILE_QUEUE_H

typedef bt_status_t (btif_connect_cb_t) (bt_bdaddr_t *bda);

typedef enum {
  BTIF_QUEUE_CONNECT_EVT,
  BTIF_QUEUE_ADVANCE_EVT,
  BTIF_QUEUE_PENDING_CONECT_EVT,
  BTIF_QUEUE_PENDING_CONECT_ADVANCE_EVT
} btif_queue_event_t;

bt_status_t btif_queue_connect(uint16_t uuid, const bt_bdaddr_t *bda,
                        btif_connect_cb_t *connect_cb, uint8_t connect_queue);
void btif_queue_advance();
void btif_queue_pending_retry();
void btif_queue_release();

#endif
