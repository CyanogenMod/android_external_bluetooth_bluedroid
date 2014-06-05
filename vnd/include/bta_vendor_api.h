
/******************************************************************************
 *
 *  Copyright (C) 2003-2014 Broadcom Corporation
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

/*****************************************************************************
**
**  Name:           bta_vendor_api.h
**
**  Description:       VENDOR custom Ble API Bluetooth application definitions
**
*****************************************************************************/
#ifndef BTA_VENDOR_API_H
#define BTA_VENDOR_API_H

#include "bta_api.h"
#include "bta_sys.h"
#include "vendor_api.h"




#if (BLE_INCLUDED == TRUE && BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE)

/*******************************************************************************
**
** Function         BTA_BrcmInit
**
** Description      This function initializes Broadcom specific VS handler in BTA
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_BrcmInit (void);

#endif

#ifdef __cplusplus
}
#endif

#endif



