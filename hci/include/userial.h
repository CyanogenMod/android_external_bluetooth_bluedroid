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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      userial.h
 *
 *  Description:   Contains definitions used for serial port controls
 *
 ******************************************************************************/

#ifndef USERIAL_H
#define USERIAL_H

/******************************************************************************
**  Constants & Macros
******************************************************************************/

/**** port IDs ****/
#define USERIAL_PORT_1          0
#define USERIAL_PORT_2          1
#define USERIAL_PORT_3          2
#define USERIAL_PORT_4          3
#define USERIAL_PORT_5          4
#define USERIAL_PORT_6          5
#define USERIAL_PORT_7          6
#define USERIAL_PORT_8          7
#define USERIAL_PORT_9          8
#define USERIAL_PORT_10         9
#define USERIAL_PORT_11         10
#define USERIAL_PORT_12         11
#define USERIAL_PORT_13         12
#define USERIAL_PORT_14         13
#define USERIAL_PORT_15         14
#define USERIAL_PORT_16         15
#define USERIAL_PORT_17         16
#define USERIAL_PORT_18         17

typedef enum {
    USERIAL_OP_INIT,
    USERIAL_OP_RXFLOW_ON,
    USERIAL_OP_RXFLOW_OFF,
} userial_ioctl_op_t;

/******************************************************************************
**  Type definitions
******************************************************************************/

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        userial_init
**
** Description     Initializes the userial driver
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_init(void);

/*******************************************************************************
**
** Function        userial_open
**
** Description     Open Bluetooth device with the port ID
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_open(uint8_t port);

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
uint16_t  userial_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len);

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
uint16_t userial_write(uint16_t msg_id, uint8_t *p_data, uint16_t len);

/*******************************************************************************
**
** Function        userial_close
**
** Description     Close the userial port
**
** Returns         None
**
*******************************************************************************/
void userial_close(void);

/*******************************************************************************
**
** Function        userial_ioctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void userial_ioctl(userial_ioctl_op_t op, void *p_data);

#endif /* USERIAL_H */

