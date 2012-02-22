/*****************************************************************************
**
**  Name:           bta_sys_ci.c
**
**  Description:    This is the implementation file for BTA system call-in
**                  functions.
**
**  Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_sys.h"
#include "bta_sys_ci.h"


/*******************************************************************************
**
** Function         bta_sys_hw_ci_enabled
**
** Description      This function must be called in response to function
**                  bta_sys_hw_enable_co(), when HW is indeed enabled
**                  
**
** Returns          void                  
**
*******************************************************************************/
  void bta_sys_hw_ci_enabled(tBTA_SYS_HW_MODULE module )

{
    tBTA_SYS_HW_MSG *p_msg;
    
    if ((p_msg = (tBTA_SYS_HW_MSG *) GKI_getbuf(sizeof(tBTA_SYS_HW_MSG))) != NULL)
    {
        p_msg->hdr.event = BTA_SYS_EVT_ENABLED_EVT;
        p_msg->hw_module = module;
          
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         bta_sys_hw_ci_disabled
**
** Description      This function must be called in response to function
**                  bta_sys_hw_disable_co() when HW is really OFF
**                  
**
** Returns          void                  
**
*******************************************************************************/
void bta_sys_hw_ci_disabled( tBTA_SYS_HW_MODULE module  )
{
    tBTA_SYS_HW_MSG *p_msg;
    
    if ((p_msg = (tBTA_SYS_HW_MSG *) GKI_getbuf(sizeof(tBTA_SYS_HW_MSG))) != NULL)
    {
        p_msg->hdr.event = BTA_SYS_EVT_DISABLED_EVT;
        p_msg->hw_module = module;
        
        bta_sys_sendmsg(p_msg);
    }
}

