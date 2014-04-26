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

/******************************************************************************
 *
 *  Filename:      userial.h
 *
 *  Description:   Contains definitions used for serial port controls
 *
 ******************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  USERIAL_PORT_1,
  USERIAL_PORT_2,
  USERIAL_PORT_3,
  USERIAL_PORT_4,
  USERIAL_PORT_5,
  USERIAL_PORT_6,
  USERIAL_PORT_7,
  USERIAL_PORT_8,
  USERIAL_PORT_9,
  USERIAL_PORT_10,
  USERIAL_PORT_11,
  USERIAL_PORT_12,
  USERIAL_PORT_13,
  USERIAL_PORT_14,
  USERIAL_PORT_15,
  USERIAL_PORT_16,
  USERIAL_PORT_17,
  USERIAL_PORT_18,
} userial_port_t;

typedef enum {
    USERIAL_OP_RXFLOW_ON,
    USERIAL_OP_RXFLOW_OFF,
} userial_ioctl_op_t;

/*******************************************************************************
**
** Function        userial_init
**
** Description     Initializes the userial driver
**
*******************************************************************************/
bool userial_init(void);

/*******************************************************************************
**
** Function        userial_open
**
** Description     Open Bluetooth device with the port ID
**
*******************************************************************************/
bool userial_open(userial_port_t port);

/*******************************************************************************
**
** Function        userial_read
**
** Description     Read data from the userial port
**
** Returns         Number of bytes actually read from the userial port and
**                 copied into p_data.  This may be less than len.
**
*******************************************************************************/
uint16_t userial_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len);

/*******************************************************************************
**
** Function        userial_write
**
** Description     Write data to the userial port
**
** Returns         Number of bytes actually written to the userial port. This
**                 may be less than len.
**
*******************************************************************************/
uint16_t userial_write(uint16_t msg_id, const uint8_t *p_data, uint16_t len);

/*******************************************************************************
**
** Function        userial_close
**
** Description     Close the userial port
**
*******************************************************************************/
void userial_close(void);

/*******************************************************************************
**
** Function        userial_ioctl
**
** Description     ioctl inteface
**
*******************************************************************************/
void userial_ioctl(userial_ioctl_op_t op);
