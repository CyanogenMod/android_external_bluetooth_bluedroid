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
 *  Filename:      utils.c
 *
 *  Description:   Contains helper functions
 *
 ******************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "bt_hci_bdroid.h"
#include "utils.h"

/******************************************************************************
**  Static variables
******************************************************************************/

static pthread_mutex_t utils_mutex;

/*****************************************************************************
**   UTILS INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        utils_init
**
** Description     Utils initialization
**
** Returns         None
**
*******************************************************************************/
void utils_init (void)
{
    pthread_mutex_init(&utils_mutex, NULL);
}

/*******************************************************************************
**
** Function        utils_cleanup
**
** Description     Utils cleanup
**
** Returns         None
**
*******************************************************************************/
void utils_cleanup (void)
{
}

/*******************************************************************************
**
** Function        utils_queue_init
**
** Description     Initialize the given buffer queue
**
** Returns         None
**
*******************************************************************************/
void utils_queue_init (BUFFER_Q *p_q)
{
    p_q->p_first = p_q->p_last = NULL;
    p_q->count = 0;
}

/*******************************************************************************
**
** Function        utils_enqueue
**
** Description     Enqueue a buffer at the tail of the given queue
**
** Returns         None
**
*******************************************************************************/
void utils_enqueue (BUFFER_Q *p_q, void *p_buf)
{
    HC_BUFFER_HDR_T    *p_hdr;

    p_hdr = (HC_BUFFER_HDR_T *) ((uint8_t *) p_buf - BT_HC_BUFFER_HDR_SIZE);

    pthread_mutex_lock(&utils_mutex);

    if (p_q->p_last)
    {
        HC_BUFFER_HDR_T *p_last_hdr = \
          (HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_last - BT_HC_BUFFER_HDR_SIZE);

        p_last_hdr->p_next = p_hdr;
    }
    else
        p_q->p_first = p_buf;

    p_q->p_last = p_buf;
    p_q->count++;

    p_hdr->p_next = NULL;

    pthread_mutex_unlock(&utils_mutex);
}

/*******************************************************************************
**
** Function        utils_dequeue
**
** Description     Dequeues a buffer from the head of the given queue
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue (BUFFER_Q *p_q)
{
    HC_BUFFER_HDR_T    *p_hdr;

    pthread_mutex_lock(&utils_mutex);

    if (!p_q || !p_q->count)
    {
        pthread_mutex_unlock(&utils_mutex);
        return (NULL);
    }

    p_hdr=(HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_first-BT_HC_BUFFER_HDR_SIZE);

    if (p_hdr->p_next)
        p_q->p_first = ((uint8_t *)p_hdr->p_next + BT_HC_BUFFER_HDR_SIZE);
    else
    {
        p_q->p_first = NULL;
        p_q->p_last  = NULL;
    }

    p_q->count--;

    p_hdr->p_next = NULL;

    pthread_mutex_unlock(&utils_mutex);

    return ((uint8_t *)p_hdr + BT_HC_BUFFER_HDR_SIZE);
}

/*******************************************************************************
**
** Function        utils_getnext
**
** Description     Return a pointer to the next buffer linked to the given
**                 buffer
**
** Returns         NULL if the given buffer does not point to any next buffer,
**                 else next buffer address
**
*******************************************************************************/
void *utils_getnext (void *p_buf)
{
    HC_BUFFER_HDR_T    *p_hdr;

    p_hdr = (HC_BUFFER_HDR_T *) ((uint8_t *) p_buf - BT_HC_BUFFER_HDR_SIZE);

    if (p_hdr->p_next)
        return ((uint8_t *)p_hdr->p_next + BT_HC_BUFFER_HDR_SIZE);
    else
        return (NULL);
}

/*******************************************************************************
**
** Function        utils_remove_from_queue
**
** Description     Dequeue the given buffer from the middle of the given queue
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue (BUFFER_Q *p_q, void *p_buf)
{
    HC_BUFFER_HDR_T    *p_prev;
    HC_BUFFER_HDR_T    *p_buf_hdr;

    pthread_mutex_lock(&utils_mutex);

    if (p_buf == p_q->p_first)
    {
        pthread_mutex_unlock(&utils_mutex);
        return (utils_dequeue (p_q));
    }

    p_buf_hdr = (HC_BUFFER_HDR_T *)((uint8_t *)p_buf - BT_HC_BUFFER_HDR_SIZE);
    p_prev=(HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_first-BT_HC_BUFFER_HDR_SIZE);

    for ( ; p_prev; p_prev = p_prev->p_next)
    {
        /* If the previous points to this one, move the pointers around */
        if (p_prev->p_next == p_buf_hdr)
        {
            p_prev->p_next = p_buf_hdr->p_next;

            /* If we are removing the last guy in the queue, update p_last */
            if (p_buf == p_q->p_last)
                p_q->p_last = p_prev + 1;

            /* One less in the queue */
            p_q->count--;

            /* The buffer is now unlinked */
            p_buf_hdr->p_next = NULL;

            pthread_mutex_unlock(&utils_mutex);
            return (p_buf);
        }
    }

    pthread_mutex_unlock(&utils_mutex);
    return (NULL);
}

/*******************************************************************************
**
** Function        utils_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void utils_delay (uint32_t timeout)
{
    struct timespec delay;
    int err;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    /* [u]sleep can't be used because it uses SIGALRM */
    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno ==EINTR);
}

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
void utils_lock (void)
{
    pthread_mutex_lock(&utils_mutex);
}

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
void utils_unlock (void)
{
    pthread_mutex_unlock(&utils_mutex);
}

