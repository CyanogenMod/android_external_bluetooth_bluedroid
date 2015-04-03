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
 *            the names of its contributols may be used to endolse or promote
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
 *   This file contains L2cap Socket interface function prototypes
 *
 ******************************************************************************/


#ifndef BTIF_SOCK_L2C_H
#define BTIF_SOCK_L2C_H

#include "bt_target.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)

bt_status_t btsock_l2c_init(int handle);
bt_status_t btsock_l2c_cleanup();
bt_status_t btsock_l2c_listen(const char* name, const uint8_t* uuid, int channel,
                              int* sock_fd, int flags);
bt_status_t btsock_l2c_connect(const bt_bdaddr_t *bd_addr, const uint8_t* uuid,
                               int channel, int* sock_fd, int flags);
void btsock_l2c_signaled(int fd, int flags, uint32_t user_id);
bt_status_t btsock_l2c_set_sockopt(int psm, btsock_option_type_t option_name,
                                            void *option_value, int option_len);
bt_status_t btsock_l2c_get_sockopt(int channel, btsock_option_type_t option_name,
                                            void *option_value, int *option_len);

#endif

#endif

