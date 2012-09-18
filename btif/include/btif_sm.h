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

/*****************************************************************************
 *
 *  Filename:      btif_sm.h
 *
 *  Description:   Generic BTIF state machine API
 *
 *****************************************************************************/

#ifndef BTIF_SM_H
#define BTIF_SM_H

/*****************************************************************************
**  Constants & Macros
******************************************************************************/

/* Generic Enter/Exit state machine events */
#define BTIF_SM_ENTER_EVT 0xFFFF
#define BTIF_SM_EXIT_EVT  0xFFFE


/*****************************************************************************
**  Type definitions and return values
******************************************************************************/
typedef UINT32 btif_sm_state_t;
typedef UINT32 btif_sm_event_t;
typedef void* btif_sm_handle_t;
typedef BOOLEAN(*btif_sm_handler_t)(btif_sm_event_t event, void *data);


/*****************************************************************************
**  Functions
**
**  NOTE: THESE APIs SHOULD BE INVOKED ONLY IN THE BTIF CONTEXT
**
******************************************************************************/

/*****************************************************************************
**
** Function     btif_sm_init
**
** Description  Initializes the state machine with the state handlers
**              The caller should ensure that the table and the corresponding
**              states match. The location that 'p_handlers' points to shall
**              be available until the btif_sm_shutdown API is invoked.
**
** Returns      Returns a pointer to the initialized state machine handle.
**
******************************************************************************/
btif_sm_handle_t btif_sm_init(const btif_sm_handler_t *p_handlers,
                               btif_sm_state_t initial_state);

/*****************************************************************************
**
** Function     btif_sm_shutdown
**
** Description  Tears down the state machine
**
** Returns      None
**
******************************************************************************/
void btif_sm_shutdown(btif_sm_handle_t handle);

/*****************************************************************************
**
** Function     btif_sm_get_state
**
** Description  Fetches the current state of the state machine
**
** Returns      Current state
**
******************************************************************************/
btif_sm_state_t btif_sm_get_state(btif_sm_handle_t handle);

/*****************************************************************************
**
** Function     btif_sm_dispatch
**
** Description  Dispatches the 'event' along with 'data' to the current state handler
**
** Returns      Returns BT_STATUS_OK on success, BT_STATUS_FAIL otherwise
**
******************************************************************************/
bt_status_t btif_sm_dispatch(btif_sm_handle_t handle, btif_sm_event_t event,
                                void *data);

/*****************************************************************************
**
** Function     btif_sm_change_state
**
** Description  Make a transition to the new 'state'. The 'BTIF_SM_EXIT_EVT'
**              shall be invoked before exiting the current state. The
**              'BTIF_SM_ENTER_EVT' shall be invoked before entering the new state
**
**
** Returns      Returns BT_STATUS_OK on success, BT_STATUS_FAIL otherwise
**
******************************************************************************/
bt_status_t btif_sm_change_state(btif_sm_handle_t handle, btif_sm_state_t state);

#endif /* BTIF_SM_H */
