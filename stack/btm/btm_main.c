/*****************************************************************************
**
**  Name:          btm_main.c
**
**  Description:   This file contains the definition of the btm control block
**                  when BTM_DYNAMIC_MEMORY is used.
**
**
**  Copyright (c) 2002-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_types.h"
#include "bt_target.h"
#include <string.h>
#include "btm_int.h"

/* Global BTM control block structure
*/
#if BTM_DYNAMIC_MEMORY == FALSE
tBTM_CB  btm_cb;
#endif

/*******************************************************************************
**
** Function         btm_init
**
** Description      This function is called at BTM startup to allocate the
**                  control block (if using dynamic memory), and initializes the
**                  tracing level.  It then initializes the various components of
**                  btm.
**
** Returns          void
**
*******************************************************************************/
void btm_init (void)
{
    /* All fields are cleared; nonzero fields are reinitialized in appropriate function */
    memset(&btm_cb, 0, sizeof(tBTM_CB));

#if defined(BTM_INITIAL_TRACE_LEVEL)
    btm_cb.trace_level = BTM_INITIAL_TRACE_LEVEL;
#else
    btm_cb.trace_level = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif
    /* Initialize BTM component structures */
    btm_inq_db_init();                  /* Inquiry Database and Structures */
    btm_acl_init();                     /* ACL Database and Structures */
    btm_sec_init(BTM_SEC_MODE_SP);      /* Security Manager Database and Structures */
#if BTM_SCO_INCLUDED == TRUE
    btm_sco_init();                     /* SCO Database and Structures (If included) */
#endif

    btm_dev_init();                     /* Device Manager Structures & HCI_Reset */
}


