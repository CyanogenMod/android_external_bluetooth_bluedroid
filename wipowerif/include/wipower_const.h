/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
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

#ifndef _WIPOWER_CONSTANTS_H_
#define _WIPOWER_CONSTANTS_H_

#define WP_HCI_VS_CMD 0xFC1F

#define WP_HCI_CMD_SET_CURRENT_LIMIT 0x16
#define WP_HCI_CMD_SET_CHARGE_OUTPUT 0x17
#define WP_HCI_CMD_ENABLE_ALERT      0x18
#define WP_HCI_CMD_ENABLE_DATA       0x19
#define WP_HCI_CMD_GET_CURRENT_LIMIT 0x1A
#define WP_HCI_CMD_GET_CHARGE_OUTPUT 0x1B
#define WP_HCI_CMD_ENABLE_POWER      0x1C

#define WP_HCI_EVENT_ALERT    0x15
#define WP_HCI_EVENT_DATA     0x16
#define WP_HCI_EVENT_POWER_ON 0x17

/*
 * enable: 0x00 disabled, 0x01 enabled: enable/disable detection command
 * on: 0x00 power down  [event on PRU placed on to PTU]
 *     0x01 power up    [event on PRU taken out of PTU]
 * time_flag: if true then host advertises on 600ms and if false its for 30ms
 * i.e. for PTU to short beacon on long beacon detection on charge complete or
 * charge required.
 */
int enable_power_apply(bool enable, bool on, bool time_flag);

#endif /*_WIPOWER_CONSTANTS_H_*/
