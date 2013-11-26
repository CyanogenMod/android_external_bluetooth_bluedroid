/******************************************************************************
 *
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
 *  Filename:      hci_ibs.c
 *
 *  Description:   Contains S/W In Band Sleep Protocol functions
 *
 ******************************************************************************/

#define LOG_TAG "hci_ibs"

#include <utils/Log.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "userial.h"
#include "bt_vendor_lib.h"

/******************************************************************************
**  Externs
******************************************************************************/

extern tUSERIAL_IF *p_userial_if;

/******************************************************************************
**  Constants & Macros
******************************************************************************/

/* HCI_IBS protocol messages */
#define HCI_IBS_WAKE_ACK        0xFC
#define HCI_IBS_WAKE_IND        0xFD
#define HCI_IBS_SLEEP_IND       0xFE

#define TX_IDLE_DELAY           10

/* HCI_IBS transmit side sleep protocol states */
typedef enum {
    HCI_IBS_TX_ASLEEP = 0,
    HCI_IBS_TX_WAKING,
    HCI_IBS_TX_AWAKE,
}tx_ibs_states;

/* HCI_IBS receive side sleep protocol states */
typedef enum {
    HCI_IBS_RX_ASLEEP = 0,
    HCI_IBS_RX_AWAKE,
}rx_ibs_states;

typedef enum {
    HCI_IBS_TX_VOTE_CLOCK_ON = 0,
    HCI_IBS_TX_VOTE_CLOCK_OFF,
    HCI_IBS_RX_VOTE_CLOCK_ON,
    HCI_IBS_RX_VOTE_CLOCK_OFF,
} hci_ibs_clock_state_vote;

typedef struct{

    pthread_mutex_t wack_lock;
    pthread_mutex_t hci_ibs_lock;
    uint8_t tx_ibs_state;
    uint8_t tx_vote;
    uint8_t rx_ibs_state;
    uint8_t rx_vote;

    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;

}IBS_STATE_MACHINE;

IBS_STATE_MACHINE ibs_state_machine;
static volatile unsigned char wack_recvd = 0;
pthread_cond_t wack_cond = PTHREAD_COND_INITIALIZER;


void ibs_start_wack_timer(void);
void ibs_stop_wack_timer(void);
void ibs_msm_serial_clock_vote(uint8_t vote, IBS_STATE_MACHINE * ibs_state);

static void hci_ibs_tx_idle_timeout()
{
    uint8_t ibs_data;

    ALOGI("Inactivity on the Tx path!");

    /*Obtain IBS State lock*/
    pthread_mutex_lock(&ibs_state_machine.hci_ibs_lock);

    /* Check for Tx state */
    switch (ibs_state_machine.tx_ibs_state)
    {
    case HCI_IBS_TX_ASLEEP:
        ALOGI("%s: Tx path is already asleep", __FUNCTION__);
        break;
    case HCI_IBS_TX_AWAKE:
        /*Step1: Send Sleep Ind. to BT-Device*/
        ALOGD("%s: Sending SLEEP-IND. to BT-Device", __FUNCTION__);
        ibs_data = HCI_IBS_SLEEP_IND;
        p_userial_if->write(0 /*dummy*/,(uint8_t *) &ibs_data, 1);

        /*Step2: Transition to TX_ASLEEP state*/
        ALOGI("%s: Transitioning to TX_ASLEEP", __FUNCTION__);
        ibs_state_machine.tx_ibs_state = HCI_IBS_TX_ASLEEP;

        /*Step3: Vote Off UART CLK on behalf of Tx path*/
        ALOGI("%s: Voting Off UART CLK on behalf of Tx path", __FUNCTION__);
        ibs_msm_serial_clock_vote(HCI_IBS_TX_VOTE_CLOCK_OFF, &ibs_state_machine);
        break;
    default:
        ALOGE("%s: Invalid Tx state: %d!", __FUNCTION__,
            ibs_state_machine.tx_ibs_state);
        break;
    }

    /*Release IBS State lock*/
    pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
}

/*******************************************************************************
**
** Function        hci_ibs_wake_retrans_timeout
**
** Description     Send Wake Ind. packet and restart wake ack. retrans. timer
**
** Returns         None
**
*******************************************************************************/
static void hci_ibs_wake_retrans_timeout(union sigval arg)
{
    uint8_t ibs_data;

    /*Step1: Restart the Wack Ack. retransmission timer*/
    ALOGI("%s: Wake-Ack. Timeout. Resending Wake Ind to BT-Device", __FUNCTION__);
    ibs_start_wack_timer();

    /*Step2: Send Wake Ind. to BT-Device*/
    ibs_data = HCI_IBS_WAKE_IND;
    p_userial_if->write(0 /*dummy*/,(uint8_t *) &ibs_data, 1);
    ALOGD("%s: Restarting Wack Ack. retransmission timer!", __FUNCTION__);

    /*
     * To-Do:
     * Have max. N retries and return failure if Wake Ack. still not received
     */

    return ;
}

/*******************************************************************************
**
** Function        ibs_start_wack_timer
**
** Description     Start the wake ack. retransmission  timer
**
** Returns         None
**
*******************************************************************************/
void ibs_start_wack_timer()
{

    int status;
    struct sigevent se;
    struct itimerspec ts;

    if (ibs_state_machine.timer_created == FALSE)
    {
        se.sigev_notify_function = hci_ibs_wake_retrans_timeout;
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &ibs_state_machine.timer_id;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &ibs_state_machine.timer_id);
        if (status == 0)
            ibs_state_machine.timer_created = TRUE;
    }

    if (ibs_state_machine.timer_created == TRUE)
    {
        if (!ibs_state_machine.timeout_ms)
            ibs_state_machine.timeout_ms = TX_IDLE_DELAY;

        ts.it_value.tv_sec = ibs_state_machine.timeout_ms/1000;
        ts.it_value.tv_nsec = 1000000*(ibs_state_machine.timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(ibs_state_machine.timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("[START] Failed to set the timeout handler\n");
    }
}

/*******************************************************************************
**
** Function        ibs_stop_wack_timer
**
** Description     Stop the approporiate timer based on the timer index requested
**
** Returns         None
**
*******************************************************************************/
void ibs_stop_wack_timer()
{
    int status;
    struct sigevent se;
    struct itimerspec ts;

    if (ibs_state_machine.timer_created == TRUE)
    {
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(ibs_state_machine.timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("[STOP] Failed to stop wake ack. retransmission timer");
        else
        ALOGI("Stopped the wake ack. retransmission timer");
    }
}

/*******************************************************************************
**
** Function        ibs_msm_serial_clock_vote
**
** Description     Vote UART CLK ON/OFF based on IBS Rx and Tx state machines
**
** Returns         None
**
*******************************************************************************/
void ibs_msm_serial_clock_vote(uint8_t vote, IBS_STATE_MACHINE * ibs_state)
{
    uint8_t new_vote;
    uint8_t old_vote = (ibs_state->tx_vote | ibs_state->rx_vote);

    switch (vote)
    {
    case HCI_IBS_TX_VOTE_CLOCK_ON:
        ibs_state->tx_vote = 1;
        new_vote = 1;
        break;
    case HCI_IBS_RX_VOTE_CLOCK_ON:
        ibs_state->rx_vote = 1;
        new_vote = 1;
        break;
    case HCI_IBS_TX_VOTE_CLOCK_OFF:
        ibs_state->tx_vote = 0;
        new_vote = ibs_state->rx_vote | ibs_state->tx_vote;
        break;
    case HCI_IBS_RX_VOTE_CLOCK_OFF:
        ibs_state->rx_vote = 0;
        new_vote = ibs_state->rx_vote | ibs_state->tx_vote;
        break;
    default:
        ALOGE("ibs_msm_serial_clock_vote: Wrong vote requested!\n");
        return;
    }
    ALOGI("new_vote: (%d) ## old-vote: (%d)", new_vote, old_vote);

    if (new_vote != old_vote) {
        if (new_vote) {
            /*vote UART CLK ON using UART driver's ioctl() */
            ALOGI("%s: vote UART CLK ON using UART driver's ioctl()",
                __FUNCTION__);
            /*p_userial_if->ioctl(USERIAL_OP_CLK_ON, NULL);*/
        }
        else {
            /*vote UART CLK OFF using UART driver's ioctl() */
            ALOGI("%s: vote UART CLK OFF using UART driver's ioctl()",
                __FUNCTION__);
            /*p_userial_if->ioctl(USERIAL_OP_CLK_OFF, NULL);*/
        }
    }
}

/*******************************************************************************
**
** Function        is_recvd_data_signal
**
** Description     Check recvd. data is IBS & act based on IBS Tx and Rx state
**
** Returns         None
**
*******************************************************************************/
void is_recvd_data_signal(uint8_t * data)
{
    uint8_t ibs_data;
    int lockStatus;

    switch(*data)
    {
        case HCI_IBS_WAKE_ACK: /* Wake Ack: 0xFC recvd. from SOC */
            ALOGI("0xFC: Received Wake Ack from BT-Device");
            /* Don't acquire HCI IBS Lock as no Tx state transition is made */
            switch (ibs_state_machine.tx_ibs_state)
            {
                case HCI_IBS_TX_AWAKE:
                    ALOGE("%s: Unexpected Wake Ack. recvd. in TX_AWAKE state",
                        __FUNCTION__);
                    /* Safety measure stop wake ack timer in case its running */
                    ibs_stop_wack_timer();
                    break;
                case HCI_IBS_TX_WAKING:
                    /* Step1: Stop the Wake Ack. Retransmission timer */
                    ALOGD("%s: Stopping the Retransmission timer", __FUNCTION__);
                    ibs_stop_wack_timer();

                    /* Step2: Signal Tx path, reception of Wake Ack. from SOC */
                    ALOGI("Indicating Tx path to proceed with data transfer");
                    wack_recvd = TRUE;
                    pthread_cond_signal(&wack_cond);
                    break;
                default:
                    ALOGE("%s: Rcvd. Wake Ack. in Unexpected TX state : %d",
                        __FUNCTION__, ibs_state_machine.tx_ibs_state);
            }
            break;
        case HCI_IBS_WAKE_IND: /* Wake Ind: 0xFD recvd. from SOC to wake-up host */
            ALOGI("0xFD: Received Wake Ind. from BT-Device");
            ALOGD("%s: Step1: Obtaining IBS State lock", __FUNCTION__);
            lockStatus = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
            if (lockStatus < 0)
            {
                ALOGE("%s: IBS Lock unavilable! Cant handle WAKE-IND from SOC. "
                    "Exiting with lockStatus: %d", __FUNCTION__, lockStatus);
                return ;
            }
            else
                ALOGD("%s: Obtained IBS State lock", __FUNCTION__);
            switch (ibs_state_machine.rx_ibs_state)
            {
                case HCI_IBS_RX_ASLEEP:
                    /* Step1: Vote for UART CLK ON */
                    ALOGI("Step2: Rx path is asleep. Voting for UART CLK ON");
                    ibs_msm_serial_clock_vote(HCI_IBS_RX_VOTE_CLOCK_ON, &ibs_state_machine);

                    /* Step2: Transition to RX_AWAKE state */
                    ALOGI("Step3: Transitioning to RX_AWAKE");
                    ibs_state_machine.rx_ibs_state = HCI_IBS_RX_AWAKE;

                /* Fall through delibrately */

                case HCI_IBS_RX_AWAKE: /* Send ACK though HOST is active */

                    /* Step3: Send Wake Ack. to the SOC */
                    ALOGD("Sending Wake Ack. to BT-Device");
                    ibs_data = HCI_IBS_WAKE_ACK;
                    p_userial_if->write(0 /*dummy*/,(uint8_t *) &ibs_data, 1);
                    break;
                default:
                    ALOGE("%s: Rcvd. Wake Ind. in Unexpected RX state : %d",
                        __FUNCTION__, ibs_state_machine.rx_ibs_state);
            }
            ALOGD("%s: Releasing IBS State lock", __FUNCTION__);
            pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
            break;
        case HCI_IBS_SLEEP_IND: /* Sleep Ind: 0xFE recvd. from BT-Device */
            ALOGI("0xFE: Received Sleep Ind. from BT-Device");
            switch (ibs_state_machine.rx_ibs_state)
            {
                case HCI_IBS_RX_ASLEEP:
                    ALOGI("Rx path is already asleep");
                    break;
                case HCI_IBS_RX_AWAKE:
                    ALOGD("%s: Obtaining IBS State lock", __FUNCTION__);
                    lockStatus = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
                    if (lockStatus < 0)
                    {
                        ALOGE("%s: IBS Lock unavilable! Cant handle SLEEP-IND from SOC."
                            " Exiting with lockStatus: %d", __FUNCTION__, lockStatus);
                        return ;
                    }
                    else
                        ALOGD("%s: Obtained IBS State lock", __FUNCTION__);

                    /* Step1: Transition to RX_ASLEEP state */
                    ALOGI("Transitioning to RX_ASLEEP");
                    ibs_state_machine.rx_ibs_state = HCI_IBS_RX_ASLEEP;

                    /* Step2: Voting Off serial clock */
                    ALOGI("Voting for UART CLK OFF");
                    ibs_msm_serial_clock_vote(HCI_IBS_RX_VOTE_CLOCK_OFF, &ibs_state_machine);

                    ALOGD("%s: Releasing IBS State lock", __FUNCTION__);
                    pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
                    break;
                default:
                    ALOGE("%s: Unexpected: Rcvd. Sleep Ind. in RX state : %d",
                        __FUNCTION__, ibs_state_machine.rx_ibs_state);
                    break;
            }
            break;
    }
}

/*******************************************************************************
**
** Function        bt_device_wake_up
**
** Description     Wake up BT-Device & vote UART CLK ON prior to sending Tx data
**
** Returns         None
**
*******************************************************************************/
int bt_device_wake_up(uint8_t state)
{
    int status = 0;
    uint8_t ibs_data;

    if (state == BT_VND_LPM_WAKE_ASSERT) {
        ALOGD("CURR. TX_STATE: %d", ibs_state_machine.tx_ibs_state);
        switch (ibs_state_machine.tx_ibs_state)
        {
        case HCI_IBS_TX_ASLEEP:
            /* Step1: Obtain IBS State lock */
            ALOGD("%s: Obtaining IBS State lock", __FUNCTION__);
            pthread_mutex_lock(&ibs_state_machine.hci_ibs_lock);

            /* Step2: Transition to TX_WAKING */
            ALOGI("%s: Transitioning to TX_WAKING", __FUNCTION__);
            ibs_state_machine.tx_ibs_state = HCI_IBS_TX_WAKING;

            /* Step3: Vote for serial clock */
            ALOGI("%s: Voting for serial clock", __FUNCTION__);
            ibs_msm_serial_clock_vote(HCI_IBS_TX_VOTE_CLOCK_ON, &ibs_state_machine);

            /* Step4: Acquire the Wake Ack. lock */
            ALOGD("%s: Acquiring the Wake Ack. lock", __FUNCTION__);
            pthread_mutex_lock(&ibs_state_machine.wack_lock);

            /* Step5: Start retransmission timer & wait for the WAKE-ACK*/
            ALOGD("%s: Starting retransmission timer & waiting for WAKE-ACK",
                __FUNCTION__);
            ibs_start_wack_timer();

            /* Safety measure to avoid deadlock in case Rx thread signals before
             * the Tx thread blocks on the condition variable
             */
            wack_recvd = FALSE;

            /* Step6: Send WAKE-IND to BT-Device */
            ALOGD("%s: Sending WAKE-IND to BT-Device", __FUNCTION__);
            ibs_data = HCI_IBS_WAKE_IND;
            p_userial_if->write(0 /*dummy*/,(uint8_t *) &ibs_data, 1);

            /* Step7: Wait for reception of WAKE-ACK from BT-Device */
            ALOGI("%s: Waiting for BT-Device to send Wake Ack.", __FUNCTION__);
            while (wack_recvd == FALSE) {
                pthread_cond_wait(&wack_cond, &ibs_state_machine.wack_lock);
                ALOGI(" ### %s: Get ACK signal from Rx line !!! .", __FUNCTION__);
            }

            /* Shld. have recvd. WAKE-ACK from SOC in the RX path */
            if (wack_recvd) {
                /* Step8: Transition to TX_AWAKE */
                ALOGI("%s: Received wake-ack from SOC", __FUNCTION__);
                ALOGI("%s: Transitioning to TX_AWAKE",
                    __FUNCTION__);
                ibs_state_machine.tx_ibs_state = HCI_IBS_TX_AWAKE;
                status = 0;
                wack_recvd = FALSE;
            }
            else {
                ALOGE("%s: Failed to wake-up SOC!!!", __FUNCTION__);
                ALOGD("%s: Stopping the Wake Ack. retrans. timer",
                    __FUNCTION__);
                ibs_stop_wack_timer();
                status = -1;
                wack_recvd = FALSE;
            }
            /* Step9: Release the Wake Ack. lock */
            ALOGD("%s: Releasing the Wake Ack. lock", __FUNCTION__);
            pthread_mutex_unlock(&ibs_state_machine.wack_lock);

            /* Step10: Release the IBS State lock */
            ALOGD("%s: Releasing the IBS State lock", __FUNCTION__);
            pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
            break;
        case HCI_IBS_TX_AWAKE:
            ALOGI("BT-Device is already awake");
            break;
        case HCI_IBS_TX_WAKING:
            ALOGI("%s: BT-Device is waking up", __FUNCTION__);
            break;
        }
    } else if (state == BT_VND_LPM_WAKE_DEASSERT) {
        ALOGI("%s: Allow BT-Device to sleep", __FUNCTION__);
        hci_ibs_tx_idle_timeout();
    }

    return status;
}

/*******************************************************************************
**
** Function         lpm_vnd_sleep_func
**
** Description      Callback of vendor specific function to allow device to
**                  sleep or wakeup
**
** Returns          None
**
*******************************************************************************/
void lpm_vnd_sleep_func(uint8_t vnd_result)
{
    ALOGI("lpm_vnd_sleep_func");
    bt_device_wake_up(vnd_result);
}
