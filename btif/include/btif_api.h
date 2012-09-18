/******************************************************************************
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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/

/*******************************************************************************
 *
 *  Filename:      btif_api.h
 *
 *  Description:   Main API header file for all BTIF functions accessed
 *                 from main bluetooth HAL. All HAL extensions will not
 *                 require headerfiles as they would be accessed through
 *                 callout/callins.
 *
 *******************************************************************************/

#ifndef BTIF_API_H
#define BTIF_API_H

#include "btif_common.h"
#include "btif_dm.h"

/*******************************************************************************
**  BTIF CORE API
********************************************************************************/

/*******************************************************************************
**
** Function         btif_init_bluetooth
**
** Description      Creates BTIF task and prepares BT scheduler for startup
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_init_bluetooth(void);

/*******************************************************************************
**
** Function         btif_enable_bluetooth
**
** Description      Performs chip power on and kickstarts OS scheduler
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_enable_bluetooth(void);

/*******************************************************************************
**
** Function         btif_disable_bluetooth
**
** Description      Inititates shutdown of Bluetooth system.
**                  Any active links will be dropped and device entering
**                  non connectable/discoverable mode
**
** Returns          void
**
*******************************************************************************/
bt_status_t btif_disable_bluetooth(void);

/*******************************************************************************
**
** Function         btif_shutdown_bluetooth
**
** Description      Finalizes BT scheduler shutdown and terminates BTIF
**                  task.
**
**
** Returns          void
**
*******************************************************************************/
bt_status_t btif_shutdown_bluetooth(void);

/*******************************************************************************
**
** Function         btif_get_adapter_properties
**
** Description      Fetches all local adapter properties
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_get_adapter_properties(void);

/*******************************************************************************
**
** Function         btif_get_adapter_property
**
** Description      Fetches property value from local cache
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_get_adapter_property( bt_property_type_t type);

/*******************************************************************************
**
** Function         btif_set_adapter_property
**
** Description      Updates core stack with property value and stores it in
**                  local cache
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_set_adapter_property( const bt_property_t *property);

/*******************************************************************************
**
** Function         btif_get_remote_device_property
**
** Description      Fetches the remote device property from the NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_get_remote_device_property( bt_bdaddr_t *remote_addr,
                                             bt_property_type_t type);

/*******************************************************************************
**
** Function         btif_get_remote_device_properties
**
** Description      Fetches all the remote device properties from NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_get_remote_device_properties( bt_bdaddr_t *remote_addr);

/*******************************************************************************
**
** Function         btif_set_remote_device_property
**
** Description      Writes the remote device property to NVRAM.
**                  Currently, BT_PROPERTY_REMOTE_FRIENDLY_NAME is the only
**                  remote device property that can be set
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_set_remote_device_property( bt_bdaddr_t *remote_addr,
                                             const bt_property_t *property);

/*******************************************************************************
**
** Function         btif_get_remote_service_record
**
** Description      Looks up the service matching uuid on the remote device
**                  and fetches the SCN and service_name if the UUID is found
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_get_remote_service_record( bt_bdaddr_t *remote_addr,
                                            bt_uuid_t *uuid);


/*******************************************************************************
**  BTIF DM API
********************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_start_discovery
**
** Description      Start device discovery/inquiry
**
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_start_discovery(void);

/*******************************************************************************
**
** Function         btif_dm_cancel_discovery
**
** Description      Cancels search
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_cancel_discovery(void);

/*******************************************************************************
**
** Function         btif_dm_create_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_create_bond(const bt_bdaddr_t *bd_addr);

/*******************************************************************************
**
** Function         btif_dm_cancel_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_cancel_bond(const bt_bdaddr_t *bd_addr);

/*******************************************************************************
**
** Function         btif_dm_remove_bond
**
** Description      Removes bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_remove_bond(const bt_bdaddr_t *bd_addr);

/*******************************************************************************
**
** Function         btif_dm_pin_reply
**
** Description      BT legacy pairing - PIN code reply
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_pin_reply( const bt_bdaddr_t *bd_addr, uint8_t accept,
                               uint8_t pin_len, bt_pin_code_t *pin_code);

/*******************************************************************************
**
** Function         btif_dm_passkey_reply
**
** Description      BT SSP passkey reply
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_passkey_reply( const bt_bdaddr_t *bd_addr,
                                   uint8_t accept, uint32_t passkey);

/*******************************************************************************
**
** Function         btif_dm_ssp_reply
**
** Description      BT SSP Reply - Just Works, Numeric Comparison & Passkey Entry
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_ssp_reply( const bt_bdaddr_t *bd_addr,
                              bt_ssp_variant_t variant, uint8_t accept,
                              uint32_t passkey);

/*******************************************************************************
**
** Function         btif_dm_get_adapter_property
**
** Description      Queries the BTA for the adapter property
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_get_adapter_property(bt_property_t *prop);

/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_get_remote_service_record(bt_bdaddr_t *remote_addr,
                                              bt_uuid_t *uuid);


/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
bt_status_t btif_dm_get_remote_services(bt_bdaddr_t *remote_addr);

/*******************************************************************************
**
** Function         btif_dut_mode_configure
**
** Description      Configure Test Mode - 'enable' to 1 puts the device in test mode and 0 exits
**                       test mode
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
bt_status_t btif_dut_mode_configure(uint8_t enable);

/*******************************************************************************
**
** Function         btif_dut_mode_send
**
** Description     Sends a HCI Vendor specific command to the controller
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
bt_status_t btif_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len);

#endif /* BTIF_API_H */
