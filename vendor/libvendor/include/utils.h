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
 *  Filename:      utils.h
 *
 *  Description:   Utility functions declaration
 *
 ******************************************************************************/

#ifndef UTILS_H
#define UTILS_H

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}

/******************************************************************************
**  Type definitions
******************************************************************************/

typedef struct 
{
    void        *p_first;
    void        *p_last;
    uint16_t    count;
} BUFFER_Q;

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        utils_init
**
** Description     Utils initialization
**
** Returns         None
**
*******************************************************************************/
void utils_init ();

/*******************************************************************************
**
** Function        utils_cleanup
**
** Description     Utils cleanup
**
** Returns         None
**
*******************************************************************************/
void utils_cleanup ();

/*******************************************************************************
**
** Function        utils_queue_init
**
** Description     Initialize the given buffer queue
**
** Returns         None
**
*******************************************************************************/
void utils_queue_init (BUFFER_Q *p_q);

/*******************************************************************************
**
** Function        utils_enqueue
**
** Description     Enqueue a buffer at the tail of the given queue
**
** Returns         None
**
*******************************************************************************/
void utils_enqueue (BUFFER_Q *p_q, void *p_buf);

/*******************************************************************************
**
** Function        utils_dequeue
**
** Description     Dequeues a buffer from the head of the given queue
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue (BUFFER_Q *p_q);

/*******************************************************************************
**
** Function        utils_getnext
**
** Description     Return a pointer to the next buffer linked to the given buffer
**
** Returns         NULL if the given buffer does not point to any next buffer, 
**                 else next buffer address
**
*******************************************************************************/
void *utils_getnext (void *p_buf);

/*******************************************************************************
**
** Function        utils_remove_from_queue
**
** Description     Dequeue the given buffer from the middle of the given queue
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue (BUFFER_Q *p_q, void *p_buf);

/*******************************************************************************
**
** Function        utils_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void utils_delay (uint32_t timeout);

/*******************************************************************************
**
** Function        utils_lock
**
** Description     application calls this function before entering critical
**                 section
**
** Returns         None
**
*******************************************************************************/
void utils_lock (void);

/*******************************************************************************
**
** Function        utils_unlock
**
** Description     application calls this function when leaving critical
**                 section
**
** Returns         None
**
*******************************************************************************/
void utils_unlock (void);

#endif /* UTILS_H */

