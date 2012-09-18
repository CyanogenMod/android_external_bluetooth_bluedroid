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

#ifndef BTIF_SOCK_SDP_H
#define BTIF_SOCK_SDP_H

static inline BOOLEAN is_uuid_empty(const uint8_t* uuid)
{
   static  uint8_t empty_uuid[16];
   return memcmp(uuid, empty_uuid, sizeof(empty_uuid)) == 0;
}

int add_rfc_sdp_rec(const char* name, const uint8_t* uuid, int scn);
void del_rfc_sdp_rec(int handle);
BOOLEAN is_reserved_rfc_channel(int channel);
int get_reserved_rfc_channel(const uint8_t* uuid);

#endif
