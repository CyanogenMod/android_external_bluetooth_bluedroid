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
 *  Filename:      bte_main.c
 *
 *  Description:   Contains BTE core stack initialization and shutdown code
 * 
 ******************************************************************************/
#include <fcntl.h>
#include <stdlib.h>

#include "gki.h"
#include "bd.h"
#include "btu.h"
#include "bte.h"
#include "bta_api.h"
#include "bt_vendor_lib.h"

/*******************************************************************************
**  Constants & Macros
*******************************************************************************/

/*******************************************************************************
**  Local type definitions
*******************************************************************************/

/*******************************************************************************
**  Static variables
*******************************************************************************/
static bt_vendor_interface_t *bt_vendor_if=NULL;
static const bt_vendor_callbacks_t vnd_callbacks;
static BOOLEAN lpm_enabled = FALSE;

/*******************************************************************************
**  Static functions
*******************************************************************************/
static void bte_main_in_hw_init(void);

/*******************************************************************************
**  Externs
*******************************************************************************/
BTU_API extern UINT32 btu_task (UINT32 param);
BTU_API extern void BTE_Init (void);
BT_API extern void BTE_LoadStack(void);
BT_API void BTE_UnloadStack(void);
extern void scru_flip_bda (BD_ADDR dst, const BD_ADDR src);
extern void btsnoop_init(void);
extern void btsnoop_open(void);
extern void btsnoop_close(void);
extern void btsnoop_cleanup (void);



/*******************************************************************************
**                        System Task Configuration
*******************************************************************************/

/* bluetooth protocol stack (BTU) task */
#ifndef BTE_BTU_STACK_SIZE
#define BTE_BTU_STACK_SIZE       0//0x2000         /* In bytes */
#endif
#define BTE_BTU_TASK_STR        ((INT8 *) "BTU")
UINT32 bte_btu_stack[(BTE_BTU_STACK_SIZE + 3) / 4];

/******************************************************************************
**
** Function         bte_main_in_hw_init
**
** Description      Internal helper function for chip hardware init
**
** Returns          None
**
******************************************************************************/
void bte_main_in_hw_init(void)
{
    GKI_delay(200);

    if ( (bt_vendor_if = (bt_vendor_interface_t *) bt_vendor_get_interface()) == NULL) 
    {
        APPL_TRACE_ERROR0("!!! Failed to get BtVendorInterface !!!");
    }
    else
    {
        int result = bt_vendor_if->init(&vnd_callbacks);
        APPL_TRACE_EVENT1("libbt-vendor init returns %d", result);
    }
}

/******************************************************************************
**
** Function         bte_main_boot_entry
**
** Description      BTE MAIN API - Entry point for BTE chip/stack initialization
**
** Returns          None
**
******************************************************************************/
void bte_main_boot_entry(void)
{
    /* initialize OS */
    GKI_init();

    bte_main_in_hw_init();

#if (BTTRC_INCLUDED == TRUE)
    /* Initialize trace feature */
    BTTRC_TraceInit(MAX_TRACE_RAM_SIZE, &BTE_TraceLogBuf[0], BTTRC_METHOD_RAM);
#endif

    /* Initialize BTE control block */
    BTE_Init();
}

/******************************************************************************
**
** Function         bte_main_shutdown
**
** Description      BTE MAIN API - Shutdown code for BTE chip/stack
**
** Returns          None
**
******************************************************************************/
void bte_main_shutdown()
{
    if (bt_vendor_if)
        bt_vendor_if->cleanup();

    bt_vendor_if = NULL;
    GKI_shutdown();
}

/******************************************************************************
**
** Function         bte_main_enable
**
** Description      BTE MAIN API - Creates all the BTE tasks. Should be called
**                  part of the Bluetooth stack enable sequence
**
** Returns          None
**
******************************************************************************/
void bte_main_enable(void)
{
    APPL_TRACE_DEBUG1("%s", __FUNCTION__);
    lpm_enabled = FALSE;

    if (bt_vendor_if)
    {
        /* toggle chip power to ensure we will reset chip in case 
           a previous stack shutdown wasn't completed gracefully */
        bt_vendor_if->set_power(BT_VENDOR_CHIP_PWR_OFF);
        bt_vendor_if->set_power(BT_VENDOR_CHIP_PWR_ON);

        bt_vendor_if->preload(NULL);
    }

    GKI_create_task((TASKPTR)btu_task, BTU_TASK, BTE_BTU_TASK_STR,
                    (UINT16 *) ((UINT8 *)bte_btu_stack + BTE_BTU_STACK_SIZE),
                    sizeof(bte_btu_stack));

    GKI_run(0);
}

/******************************************************************************
**
** Function         bte_main_disable
**
** Description      BTE MAIN API - Destroys all the BTE tasks. Should be called
**                  part of the Bluetooth stack disable sequence
**
** Returns          None
**
******************************************************************************/
void bte_main_disable(void)
{
    APPL_TRACE_DEBUG1("%s", __FUNCTION__);

    GKI_destroy_task(BTU_TASK);

    GKI_freeze();

    if (bt_vendor_if)
        bt_vendor_if->set_power(BT_VENDOR_CHIP_PWR_OFF);        
}

/******************************************************************************
**
** Function         bte_main_postload_cfg
**
** Description      BTE MAIN API - Stack postload configuration
**
** Returns          None
**
******************************************************************************/
void bte_main_postload_cfg(void)
{
    if (bt_vendor_if)
        bt_vendor_if->postload(NULL);
}

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
/******************************************************************************
**
** Function         bte_main_enable_lpm
**
** Description      BTE MAIN API - Enable/Disable low power mode operation
**
** Returns          None
**
******************************************************************************/
void bte_main_enable_lpm(BOOLEAN enable)
{
    int result = -1;
    
    if (bt_vendor_if)
        result = bt_vendor_if->lpm( \
        (enable == TRUE) ? BT_VENDOR_LPM_ENABLE : BT_VENDOR_LPM_DISABLE \
        );

    APPL_TRACE_EVENT2("vendor lib lpm enable=%d return %d", enable, result);
}

/******************************************************************************
**
** Function         bte_main_lpm_allow_bt_device_sleep
**
** Description      BTE MAIN API - Allow BT controller goest to sleep
**
** Returns          None
**
******************************************************************************/
void bte_main_lpm_allow_bt_device_sleep()
{
    int result = -1;

    if ((bt_vendor_if) && (lpm_enabled == TRUE))
        result = bt_vendor_if->lpm(BT_VENDOR_LPM_WAKE_DEASSERT);

    APPL_TRACE_DEBUG1("vendor lib lpm deassertion return %d", result);
}

/******************************************************************************
**
** Function         bte_main_lpm_wake_bt_device
**
** Description      BTE MAIN API - Wake BT controller up if it is in sleep mode
**
** Returns          None
**
******************************************************************************/
void bte_main_lpm_wake_bt_device()
{
    int result = -1;
    
    if ((bt_vendor_if) && (lpm_enabled == TRUE))
        result = bt_vendor_if->lpm(BT_VENDOR_LPM_WAKE_ASSERT);

    APPL_TRACE_DEBUG1("vendor lib lpm assertion return %d", result);
}
#endif  // HCILP_INCLUDED

/******************************************************************************
**
** Function         bte_main_hci_send
**
** Description      BTE MAIN API - This function is called by the upper stack to
**                  send an HCI message. The function displays a protocol trace
**                  message (if enabled), and then calls the 'transmit' function
**                  associated with the currently selected HCI transport
**
** Returns          None
**
******************************************************************************/
void bte_main_hci_send (BT_HDR *p_msg, UINT16 event)
{
    UINT16 sub_event = event & BT_SUB_EVT_MASK;  /* local controller ID */

    p_msg->event = event;


    if((sub_event == LOCAL_BR_EDR_CONTROLLER_ID) || \
       (sub_event == LOCAL_BLE_CONTROLLER_ID))
    {
        if (bt_vendor_if)
            bt_vendor_if->transmit_buf((TRANSAC)p_msg, \
                                       (char *) (p_msg + 1), \
                                        p_msg->len);
        else
            GKI_freebuf(p_msg);        
    }
    else
    {
        APPL_TRACE_ERROR0("Invalid Controller ID. Discarding message.");
        GKI_freebuf(p_msg);
    }
}

/******************************************************************************
**
** Function         bte_main_post_reset_init
**
** Description      BTE MAIN API - This function is mapped to BTM_APP_DEV_INIT 
**                  and shall be automatically called from BTE after HCI_Reset
**
** Returns          None
**
******************************************************************************/
void bte_main_post_reset_init()
{
    BTM_ContinueReset();
}

/*****************************************************************************
**
**   libbt-vendor Callback Functions
**
*****************************************************************************/

/******************************************************************************
**
** Function         preload_cb
**
** Description      VENDOR LIB CALLBACK API - This function is called 
**                  when vendor lib completed stack preload process
**
** Returns          None
**
******************************************************************************/
static void preload_cb(TRANSAC transac, bt_vendor_preload_result_t result)
{
    APPL_TRACE_EVENT1("vnd preload_cb %d [0:SUCCESS 1:FAIL]", result);

//    if (result == BT_VENDOR_PRELOAD_SUCCESS)
    /* keep going even if firmware patch file is missing */
    {
        /* notify BTU task that libbt-vendor is ready */
        GKI_send_event(BTU_TASK, TASK_MBOX_0_EVT_MASK);
    }
}

/******************************************************************************
**
** Function         postload_cb
**
** Description      VENDOR LIB CALLBACK API - This function is called 
**                  when vendor lib completed stack postload process
**
** Returns          None
**
******************************************************************************/
static void postload_cb(TRANSAC transac, bt_vendor_postload_result_t result)
{
    APPL_TRACE_EVENT1("vnd postload_cb %d", result);
}

/******************************************************************************
**
** Function         lpm_cb
**
** Description      VENDOR LIB CALLBACK API - This function is called 
**                  back from vendor lib to indicate the current LPM state
**
** Returns          None
**
******************************************************************************/
static void lpm_cb(bt_vendor_lpm_request_result_t result)
{
    APPL_TRACE_EVENT1("vnd lpm_result_cb %d", result);
    lpm_enabled = (result == BT_VENDOR_LPM_ENABLED) ? TRUE : FALSE;    
}

/******************************************************************************
**
** Function         hostwake_ind
**
** Description      VENDOR LIB CALLOUT API - This function is called 
**                  from vendor lib to indicate the HostWake event
**
** Returns          None
**
******************************************************************************/
static void hostwake_ind(bt_vendor_low_power_event_t event)
{
    APPL_TRACE_EVENT1("vnd hostwake_ind %d", event);
}

/******************************************************************************
**
** Function         alloc
**
** Description      VENDOR LIB CALLOUT API - This function is called 
**                  from vendor lib to request for data buffer allocation
**
** Returns          NULL / pointer to allocated buffer
**
******************************************************************************/
static char *alloc(int size)
{
    BT_HDR *p_hdr = NULL;
    
//    APPL_TRACE_DEBUG1("vnd alloc size=%d", size);
    
    p_hdr = (BT_HDR *) GKI_getbuf ((UINT16) size);
    
    if (p_hdr == NULL)
    {
        APPL_TRACE_WARNING0("alloc returns NO BUFFER!");
    }
        
    return ((char *) p_hdr);
}

/******************************************************************************
**
** Function         dealloc
**
** Description      VENDOR LIB CALLOUT API - This function is called 
**                  from vendor lib to release the data buffer allocated
**                  through the alloc call earlier.
**
** Returns          bt_vnd_status_t
**
******************************************************************************/
static int dealloc(TRANSAC transac, char *p_buf)
{
    GKI_freebuf(transac);       
    return BT_VENDOR_STATUS_SUCCESS;
}

/******************************************************************************
**
** Function         data_ind
**
** Description      VENDOR LIB CALLOUT API - This function is called 
**                  from vendor lib to pass in the received HCI packets.
**
**                  The core stack is responsible for releasing the data buffer
**                  passed in from vendor lib once the core stack has done with 
**                  it.
**
** Returns          bt_vnd_status_t
**
******************************************************************************/
static int data_ind(TRANSAC transac, char *p_buf, int len)
{
    BT_HDR *p_msg = (BT_HDR *) transac;
    
//    APPL_TRACE_DEBUG2("vnd data_ind event=0x%04X (len=%d)", p_msg->event, len);


    GKI_send_msg (BTU_TASK, BTU_HCI_RCV_MBOX, transac);
    return BT_VENDOR_STATUS_SUCCESS;    
}

/******************************************************************************
**
** Function         tx_result
**
** Description      VENDOR LIB CALLBACK API - This function is called 
**                  from vendor lib once it has processed/sent the prior data 
**                  buffer which core stack passed to it through transmit_buf
**                  call earlier.
**
**                  The core stack is responsible for releasing the data buffer
**                  if it has been completedly processed.
**
** Returns          bt_vnd_status_t
**
******************************************************************************/
static int tx_result(TRANSAC transac, char *p_buf, \
                      bt_vendor_transmit_result_t result)
{
    /*
    APPL_TRACE_DEBUG2("vnd tx_result %d (event=%04X)", result, \
                      ((BT_HDR *)transac)->event);
    */
    
    if (result == BT_VENDOR_TX_FRAGMENT)
    {
        GKI_send_msg (BTU_TASK, BTU_HCI_RCV_MBOX, transac);        
    }
    else
    {
        GKI_freebuf(transac);
    }
    
    return BT_VENDOR_STATUS_SUCCESS;    
}

/*****************************************************************************
**   The libbt-vendor Callback Functions Table
*****************************************************************************/
static const bt_vendor_callbacks_t vnd_callbacks = {
    sizeof(bt_vendor_callbacks_t),
    preload_cb,
    postload_cb,
    lpm_cb,
    hostwake_ind,
    alloc,
    dealloc,
    data_ind,
    tx_result
};

