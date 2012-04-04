/*****************************************************************************
**
**  Name:           bta_jv_co.h
**
**  Description:    This is the interface file for java interface call-out
**                  functions.
**
**  Copyright (c) 2007, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_JV_CO_H
#define BTA_JV_CO_H

#include "bta_jv_api.h"

/*****************************************************************************
**  Function Declarations
*****************************************************************************/


/*******************************************************************************
**
** Function         bta_jv_co_rfc_data
**
** Description      This function is called by JV to send data to the java glue
**                  code when the RX data path is configured to use a call-out
**
** Returns          void
**
*******************************************************************************/

BTA_API extern int bta_co_rfc_data_incoming(void *user_data, BT_HDR *p_buf);
BTA_API extern int bta_co_rfc_data_outgoing_size(void *user_data, int *size);
BTA_API extern int bta_co_rfc_data_outgoing(void *user_data, UINT8* buf, UINT16 size);

#endif /* BTA_DG_CO_H */

