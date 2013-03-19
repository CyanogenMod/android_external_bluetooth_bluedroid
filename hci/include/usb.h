/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *  Portions of file: Copyright (C) 2013, Intel Corporation
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
 *  Filename:      usb.h
 *
 *  Description:   Contains definitions used for usb serial port controls
 *
 ******************************************************************************/

#ifndef USB_H
#define USB_H

/******************************************************************************
**  Constants & Macros
******************************************************************************/
/*
 * Below are the interfaces to usb driver. At present, either USB is supported
 * or UART is supported.
 */
#define usb_init            userial_init
#define usb_open            userial_open
#define usb_write           userial_write
#define usb_read            userial_read
#define usb_close           userial_close
#define usb_ioctl           userial_ioctl
#define usb_ioctl_op_t      userial_ioctl_op_t

#define BT_USB_DEVICE_INFO(cl, sc, pr) \
        .bDevClass = (cl), \
        .bDevSubClass = (sc), \
        .bDevProtocol = (pr)

typedef enum {
   USERIAL_OP_INIT,
   USERIAL_OP_RXFLOW_ON,
   USERIAL_OP_RXFLOW_OFF,
} userial_ioctl_op_t;

/******************************************************************************
**  Type definitions
******************************************************************************/

struct bt_usb_device {
    uint8_t    bDevClass;
    uint8_t    bDevSubClass;
    uint8_t    bDevProtocol;
};

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        usb_init
**
** Description     Initializes the usb driver
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t usb_init(void);

/*******************************************************************************
**
** Function        usb_open
**
** Description     Open Bluetooth device with the port ID
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t usb_open(uint8_t port);

/*******************************************************************************
**
** Function        usb_read
**
** Description     Read data from the usb port
**
** Returns         Number of bytes actually read from the usb port and
**                 copied into p_data.  This may be less than len.
**
*******************************************************************************/
uint16_t  usb_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len);

/*******************************************************************************
**
** Function        usb_write
**
** Description     Write data to the usb port
**
** Returns         Number of bytes actually written to the usb port. This
**                 may be less than len.
**
*******************************************************************************/
uint16_t usb_write(uint16_t msg_id, uint8_t *p_data, uint16_t len);

/*******************************************************************************
**
** Function        usb_close
**
** Description     Close the usb port
**
** Returns         None
**
*******************************************************************************/
void usb_close(void);

/*******************************************************************************
**
** Function        usb_ioctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void usb_ioctl(usb_ioctl_op_t op, void *p_data);

#endif /* USB_H */

