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

#define LOG_TAG "btsnoop"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "bt_hci_bdroid.h"
#include "bt_utils.h"
#include "utils.h"

typedef enum {
  kCommandPacket = 1,
  kAclPacket = 2,
  kScoPacket = 3,
  kEventPacket = 4
} packet_type_t;

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

// File descriptor for btsnoop file.
static int hci_btsnoop_fd = -1;

void btsnoop_net_open();
void btsnoop_net_close();
void btsnoop_net_write(const void *data, size_t length);

static uint64_t btsnoop_timestamp(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // Timestamp is in microseconds.
  uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
  timestamp += tv.tv_usec;
  timestamp += BTSNOOP_EPOCH_DELTA;
  return timestamp;
}

static void btsnoop_write(const void *data, size_t length) {
  if (hci_btsnoop_fd != -1)
    write(hci_btsnoop_fd, data, length);

  btsnoop_net_write(data, length);
}

static void btsnoop_write_packet(packet_type_t type, const uint8_t *packet, bool is_received) {
  int length_he = 0;
  int length;
  int flags;
  int drops = 0;
  switch (type) {
    case kCommandPacket:
      length_he = packet[2] + 4;
      flags = 2;
      break;
    case kAclPacket:
      length_he = (packet[3] << 8) + packet[2] + 5;
      flags = is_received;
      break;
    case kScoPacket:
      length_he = packet[2] + 4;
      flags = is_received;
      break;
    case kEventPacket:
      length_he = packet[1] + 3;
      flags = 3;
      break;
  }

  uint64_t timestamp = btsnoop_timestamp();
  uint32_t time_hi = timestamp >> 32;
  uint32_t time_lo = timestamp & 0xFFFFFFFF;

  length = htonl(length_he);
  flags = htonl(flags);
  drops = htonl(drops);
  time_hi = htonl(time_hi);
  time_lo = htonl(time_lo);

  // This function is called from different contexts.
  utils_lock();

  btsnoop_write(&length, 4);
  btsnoop_write(&length, 4);
  btsnoop_write(&flags, 4);
  btsnoop_write(&drops, 4);
  btsnoop_write(&time_hi, 4);
  btsnoop_write(&time_lo, 4);
  btsnoop_write(&type, 1);
  btsnoop_write(packet, length_he - 1);

  utils_unlock();
}

void btsnoop_open(const char *p_path) {
  assert(p_path != NULL);
  assert(*p_path != '\0');

  btsnoop_net_open();

  if (hci_btsnoop_fd != -1) {
    ALOGE("%s btsnoop log file is already open.", __func__);
    return;
  }

  hci_btsnoop_fd = open(p_path,
                        O_WRONLY | O_CREAT | O_TRUNC,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  if (hci_btsnoop_fd == -1) {
    ALOGE("%s unable to open '%s': %s", __func__, p_path, strerror(errno));
    return;
  }

  write(hci_btsnoop_fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16);
}

void btsnoop_close(void) {
  if (hci_btsnoop_fd != -1)
    close(hci_btsnoop_fd);
  hci_btsnoop_fd = -1;

  btsnoop_net_close();
}

void btsnoop_capture(const HC_BT_HDR *p_buf, bool is_rcvd) {
  const uint8_t *p = (const uint8_t *)(p_buf + 1) + p_buf->offset;

  if (hci_btsnoop_fd == -1)
    return;

  switch (p_buf->event & MSG_EVT_MASK) {
    case MSG_HC_TO_STACK_HCI_EVT:
      btsnoop_write_packet(kEventPacket, p, false);
      break;
    case MSG_HC_TO_STACK_HCI_ACL:
    case MSG_STACK_TO_HC_HCI_ACL:
      btsnoop_write_packet(kAclPacket, p, is_rcvd);
      break;
    case MSG_HC_TO_STACK_HCI_SCO:
    case MSG_STACK_TO_HC_HCI_SCO:
      btsnoop_write_packet(kScoPacket, p, is_rcvd);
      break;
    case MSG_STACK_TO_HC_HCI_CMD:
      btsnoop_write_packet(kCommandPacket, p, true);
      break;
  }
}
