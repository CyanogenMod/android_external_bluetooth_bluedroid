/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bt_vendor_lib.h"

// Opens the vendor-specific library and sets the Bluetooth
// address of the adapter to |local_bdaddr|.
bool vendor_open(const uint8_t *local_bdaddr);

// Closes the vendor-specific library and frees all associated resources.
// Only |vendor_open| may be called after |vendor_close|.
void vendor_close(void);

// SSR cleanup to close transport FDs and power off the chip
// This function is invoked to recover from error enabling/disabling BT
// and to during hardware failure
void vendor_ssrcleanup(void);

// Sends a vendor-specific command to the library.
int vendor_send_command(bt_vendor_opcode_t opcode, void *param);
