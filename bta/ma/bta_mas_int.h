/******************************************************************************
 *
 *  Copyright (C) 20013 The CyanogenMod Project
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
 *  This is the private file for the message access server (MAS).
 *
 ******************************************************************************/
#ifndef BTA_MAS_INT_H
#define BTA_MAS_INT_H

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_MAS_DEFAULT_VERSION             0x0100  /* for MAP version 1.0 */

#define BTA_MAS_MSG_TYPE_EMAIL              0x01
#define BTA_MAS_MSG_TYPE_SMS_GSM            0x02
#define BTA_MAS_MSG_TYPE_SMS_CDMA           0x04
#define BTA_MAS_MSG_TYPE_MMS                0x08

#endif /* BTA_MAS_INT_H */
