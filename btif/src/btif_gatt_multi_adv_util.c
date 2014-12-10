/******************************************************************************
 *
 *  Copyright (C) 2014  Broadcom Corporation
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


/*******************************************************************************
 *
 *  Filename:      btif_gatt_multi_adv_util.c
 *
 *  Description:   Multi ADV helper implementation
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "btu.h"
#include "bt_target.h"

#define LOG_TAG "BtGatt.btif"
#if (BLE_INCLUDED == TRUE)

#include "btif_gatt_multi_adv_util.h"
#include "btif_common.h"
#include <hardware/bt_gatt.h>
#include "bta_gatt_api.h"
#include "btif_gatt_util.h"

/*******************************************************************************
**  Static variables
********************************************************************************/
static int multi_adv_enable_count = 0;
static btgatt_multi_adv_common_data *p_multi_adv_com_data_cb = NULL;

btgatt_multi_adv_common_data *btif_obtain_multi_adv_data_cb()
{
    if(0 == BTM_BleMaxMultiAdvInstanceCount())
    {
        BTIF_TRACE_WARNING("%s - No instances found", __FUNCTION__);
        return NULL;
    }

    BTIF_TRACE_DEBUG("%s, Count:%d", __FUNCTION__, BTM_BleMaxMultiAdvInstanceCount());
    if (NULL == p_multi_adv_com_data_cb)
    {
        BTIF_TRACE_DEBUG("Initializing in %s", __FUNCTION__);
        p_multi_adv_com_data_cb = GKI_getbuf(sizeof(btgatt_multi_adv_common_data));
        if (NULL != p_multi_adv_com_data_cb)
        {
            memset(p_multi_adv_com_data_cb, 0, sizeof(btgatt_multi_adv_common_data));

            /* Storing both client_if and inst_id details */
            if (NULL == (p_multi_adv_com_data_cb->clntif_map =
                  GKI_getbuf(( BTM_BleMaxMultiAdvInstanceCount() * INST_ID_IDX_MAX)* sizeof(INT8)))) {
                GKI_freebuf(p_multi_adv_com_data_cb);
                BTIF_TRACE_ERROR("%s: Line No:%d, Failed to get memory.", __FUNCTION__, __LINE__);
                return NULL;
            }
            memset(p_multi_adv_com_data_cb->clntif_map, 0 ,
                  ( BTM_BleMaxMultiAdvInstanceCount() * INST_ID_IDX_MAX)* sizeof(INT8));

            if (NULL == (p_multi_adv_com_data_cb->inst_cb = GKI_getbuf(( BTM_BleMaxMultiAdvInstanceCount() + 1 )
                                              * sizeof(btgatt_multi_adv_inst_cb)))) {
                GKI_freebuf(p_multi_adv_com_data_cb);
                GKI_freebuf(p_multi_adv_com_data_cb->clntif_map);
                BTIF_TRACE_ERROR("%s: Line No:%d, Failed to get memory.", __FUNCTION__, __LINE__);
                return NULL;
            }
            memset(p_multi_adv_com_data_cb->inst_cb, 0 ,
                 ( BTM_BleMaxMultiAdvInstanceCount() + 1) * sizeof(btgatt_multi_adv_inst_cb));

            for (int i=0; i < BTM_BleMaxMultiAdvInstanceCount()*2; i += 2)
            {
                p_multi_adv_com_data_cb->clntif_map[i] = INVALID_ADV_INST;
                p_multi_adv_com_data_cb->clntif_map[i+1] = INVALID_ADV_INST;
            }
        }
    }

    return p_multi_adv_com_data_cb;
}

void btif_gattc_init_multi_adv_cb(void)
{
    // TODO: Instead of using a fragile reference counter here, one could
    //       simply track the client_if instances that are in the map.
    ++multi_adv_enable_count;
}

void btif_gattc_destroy_multi_adv_cb(int client_if)
{
    if (multi_adv_enable_count > 0)
        multi_adv_enable_count --;
    if (client_if == INVALID_CLIENT_IF) {
        multi_adv_enable_count = 0;
    }
    if(multi_adv_enable_count == 0 && p_multi_adv_com_data_cb != 0)
    {
        if (NULL != p_multi_adv_com_data_cb)
        {
            GKI_freebuf (p_multi_adv_com_data_cb->clntif_map);
            GKI_freebuf (p_multi_adv_com_data_cb->inst_cb);
            GKI_freebuf(p_multi_adv_com_data_cb);
            p_multi_adv_com_data_cb = NULL;
        }
    }
}

int btif_multi_adv_add_instid_map(int client_if, int inst_id, BOOLEAN gen_temp_instid)
{
    int i=1;

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
    if (NULL == p_multi_adv_data_cb)
        return INVALID_ADV_INST;

    for (i=1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++)
    {
       if (client_if == p_multi_adv_data_cb->clntif_map[i + i])
       {
          if (!gen_temp_instid)
          {
             // Write the final inst_id value obtained from stack layer
             p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
             BTIF_TRACE_DEBUG("%s -Index: %d, Found client_if: %d", __FUNCTION__,
                i, p_multi_adv_data_cb->clntif_map[i + i]);
             break;
          }
          else
          {
              //Store the passed in inst_id value
             if (inst_id != INVALID_ADV_INST)
                 p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
             else
                 p_multi_adv_data_cb->clntif_map[i + (i + 1)] = (i + 1);

             BTIF_TRACE_DEBUG("%s - Index:%d,Found client_if: %d", __FUNCTION__,
                i, p_multi_adv_data_cb->clntif_map[i + i]);
             break;
          }
       }
    }

    if (i <  BTM_BleMaxMultiAdvInstanceCount())
        return i;

    // If client ID if is not found, then write both values
    for (i=1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++)
    {
        if (INVALID_ADV_INST == p_multi_adv_data_cb->clntif_map[i + i])
        {
            p_multi_adv_data_cb->clntif_map[i + i] = client_if;
            if (inst_id != INVALID_ADV_INST)
               p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
            else
                p_multi_adv_data_cb->clntif_map[i + (i + 1)] = (i + 1);
            BTIF_TRACE_DEBUG("%s -Not found - Index:%d, client_if: %d, Inst ID: %d",
                            __FUNCTION__,i,
                            p_multi_adv_data_cb->clntif_map[i + i],
                            p_multi_adv_data_cb->clntif_map[i + (i + 1)]);
            break;
        }
    }

    if (i <  BTM_BleMaxMultiAdvInstanceCount())
        return i;
    return INVALID_ADV_INST;
}

int btif_multi_adv_instid_for_clientif(int client_if)
{
    int i=1, ret = INVALID_ADV_INST;

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if (NULL == p_multi_adv_data_cb)
        return INVALID_ADV_INST;

    // Retrieve the existing inst_id for the client_if value
    for (i=1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++)
    {
       if (client_if == p_multi_adv_data_cb->clntif_map[i + i])
       {
           BTIF_TRACE_DEBUG("%s - Client if found", __FUNCTION__, client_if);
           ret = p_multi_adv_data_cb->clntif_map[i + (i + 1)];
       }
    }

    return ret;
}

int btif_gattc_obtain_idx_for_datacb(int value, int clnt_inst_index)
{
    int i=1;

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if (NULL == p_multi_adv_data_cb)
        return INVALID_ADV_INST;

    // Retrieve the array index for the inst_id value
    for (i=1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++)
    {
       if (value == p_multi_adv_data_cb->clntif_map[i + (i + clnt_inst_index)])
           break;
    }

    if (i <  BTM_BleMaxMultiAdvInstanceCount())
    {
        BTIF_TRACE_DEBUG("%s, %d",__FUNCTION__,i);
        return i;
    }

    BTIF_TRACE_DEBUG("%s Invalid instance",__FUNCTION__);
    return INVALID_ADV_INST;
}


void btif_gattc_adv_data_packager(int client_if, bool set_scan_rsp,
                bool include_name, bool include_txpower, int min_interval, int max_interval,
                int appearance, int manufacturer_len, char* manufacturer_data,
                int service_data_len, char* service_data, int service_uuid_len,
                char* service_uuid, btif_adv_data_t *p_multi_adv_inst)
{
    memset(p_multi_adv_inst, 0 , sizeof(btif_adv_data_t));

    p_multi_adv_inst->client_if = (uint8_t) client_if;
    p_multi_adv_inst->set_scan_rsp = set_scan_rsp;
    p_multi_adv_inst->include_name = include_name;
    p_multi_adv_inst->include_txpower = include_txpower;
    p_multi_adv_inst->min_interval = min_interval;
    p_multi_adv_inst->max_interval = max_interval;
    p_multi_adv_inst->appearance = appearance;
    p_multi_adv_inst->manufacturer_len = manufacturer_len;

    if (manufacturer_len > 0)
    {
        if (NULL != (p_multi_adv_inst->p_manufacturer_data = GKI_getbuf(manufacturer_len)))
        {
            memcpy(p_multi_adv_inst->p_manufacturer_data, manufacturer_data, manufacturer_len);
        } else {
            BTIF_TRACE_ERROR("%s: Line No:%d, Failed to get memory.", __FUNCTION__, __LINE__);
        }
    }

    p_multi_adv_inst->service_data_len = service_data_len;
    if (service_data_len > 0)
    {
        if (NULL != (p_multi_adv_inst->p_service_data = GKI_getbuf(service_data_len)))
        {
            memcpy(p_multi_adv_inst->p_service_data, service_data, service_data_len);
        } else {
            BTIF_TRACE_ERROR("%s: Line No:%d, Failed to get memory.", __FUNCTION__, __LINE__);
        }
    }

    p_multi_adv_inst->service_uuid_len = service_uuid_len;
    if (service_uuid_len > 0)
    {
        if (NULL != (p_multi_adv_inst->p_service_uuid = GKI_getbuf(service_uuid_len)))
        {
            memcpy(p_multi_adv_inst->p_service_uuid, service_uuid, service_uuid_len);
        } else {
            BTIF_TRACE_ERROR("%s: Line No:%d, Failed to get memory.", __FUNCTION__, __LINE__);
        }
    }
}

BOOLEAN btif_gattc_copy_datacb(int cbindex, btif_adv_data_t *p_adv_data, BOOLEAN bInstData)
{
    int i=0;
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
    if (NULL == p_multi_adv_data_cb || cbindex < 0)
       return false;

    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    memset(&p_multi_adv_data_cb->inst_cb[cbindex].data, 0, sizeof(tBTA_BLE_ADV_DATA));
    p_multi_adv_data_cb->inst_cb[cbindex].mask = 0;

    p_multi_adv_data_cb->inst_cb[cbindex].is_scan_rsp = p_adv_data->set_scan_rsp ? 1 : 0;
    if (!p_adv_data->set_scan_rsp)
    {
         p_multi_adv_data_cb->inst_cb[cbindex].mask = BTM_BLE_AD_BIT_FLAGS;
         p_multi_adv_data_cb->inst_cb[cbindex].data.flag = ADV_FLAGS_GENERAL;
         if (p_multi_adv_data_cb->inst_cb[cbindex].timeout_s)
             p_multi_adv_data_cb->inst_cb[cbindex].data.flag = ADV_FLAGS_LIMITED;
         if (p_multi_adv_data_cb->inst_cb[cbindex].data.flag == 0)
            p_multi_adv_data_cb->inst_cb[cbindex].mask = 0;
    }

    if (p_adv_data->include_name)
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_DEV_NAME;

    if (p_adv_data->include_txpower)
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_TX_PWR;

    if (false == bInstData && p_adv_data->min_interval > 0 && p_adv_data->max_interval > 0 &&
        p_adv_data->max_interval > p_adv_data->min_interval)
    {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_INT_RANGE;
        p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.low =
                                        p_adv_data->min_interval;
        p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.hi =
                                        p_adv_data->max_interval;
    }
    else
    if (true == bInstData)
    {
        if (p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min > 0 &&
            p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max > 0 &&
            p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max >
            p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min)
        {
              p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.low =
              p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min;
              p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.hi =
              p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max;
        }

        if (p_adv_data->include_txpower)
        {
            p_multi_adv_data_cb->inst_cb[cbindex].data.tx_power =
                p_multi_adv_data_cb->inst_cb[cbindex].param.tx_power;
        }
    }

    if (p_adv_data->appearance != 0)
    {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_APPEARANCE;
        p_multi_adv_data_cb->inst_cb[cbindex].data.appearance = p_adv_data->appearance;
    }

    if (p_adv_data->manufacturer_len > 0 && p_adv_data->p_manufacturer_data != NULL)
    {
         p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu =
                            GKI_getbuf(sizeof(tBTA_BLE_MANU));
         if (p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu != NULL)
         {
            p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu->p_val =
                            GKI_getbuf(p_adv_data->manufacturer_len);
            if (p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu->p_val != NULL)
            {
                 p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_MANU;
                 p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu->len =
                                p_adv_data->manufacturer_len;
                 memcpy(p_multi_adv_data_cb->inst_cb[cbindex].data.p_manu->p_val,
                    p_adv_data->p_manufacturer_data, p_adv_data->manufacturer_len);
            }
         }
    }

    tBTA_BLE_PROP_ELEM *p_elem_service_data = NULL;
    if (p_adv_data->service_data_len > 0 && p_adv_data->p_service_data != NULL)
    {
         BTIF_TRACE_DEBUG("%s - In service_data", __FUNCTION__);
         p_elem_service_data = GKI_getbuf(sizeof(tBTA_BLE_PROP_ELEM));
         if (p_elem_service_data != NULL)
         {
             p_elem_service_data->p_val = GKI_getbuf(p_adv_data->service_data_len);
             if (p_elem_service_data->p_val != NULL)
             {
                 p_elem_service_data->adv_type = BTM_BLE_AD_TYPE_SERVICE_DATA;
                 p_elem_service_data->len = p_adv_data->service_data_len;
                 memcpy(p_elem_service_data->p_val, p_adv_data->p_service_data,
                             p_adv_data->service_data_len);
             } else {
                     GKI_freebuf(p_elem_service_data);
                     p_elem_service_data = NULL;
               }
         }
    }

    if (NULL != p_elem_service_data)
    {
        p_multi_adv_data_cb->inst_cb[cbindex].data.p_proprietary =
                                                   GKI_getbuf(sizeof(tBTA_BLE_PROPRIETARY));
        if (NULL != p_multi_adv_data_cb->inst_cb[cbindex].data.p_proprietary)
        {
            tBTA_BLE_PROP_ELEM *p_elem = NULL;
            tBTA_BLE_PROPRIETARY *p_prop = p_multi_adv_data_cb->inst_cb[cbindex].data.p_proprietary;
            p_prop->num_elem = 0;
            p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_PROPRIETARY;
            p_prop->num_elem = 1;
            p_prop->p_elem = GKI_getbuf(sizeof(tBTA_BLE_PROP_ELEM) * p_prop->num_elem);
            p_elem = p_prop->p_elem;
            if (NULL != p_elem)
                memcpy(p_elem++, p_elem_service_data, sizeof(tBTA_BLE_PROP_ELEM));
            GKI_freebuf(p_elem_service_data);
        }
    }

    if (p_adv_data->service_uuid_len > 0 && NULL != p_adv_data->p_service_uuid)
    {
        UINT16 *p_uuid_out16 = NULL;
        UINT32 *p_uuid_out32 = NULL;
        while (p_adv_data->service_uuid_len >= LEN_UUID_128)
        {
             bt_uuid_t uuid;
             memset(&uuid, 0, sizeof(bt_uuid_t));
             memcpy(&uuid.uu, p_adv_data->p_service_uuid, LEN_UUID_128);

             tBT_UUID bt_uuid;
             memset(&bt_uuid, 0, sizeof(tBT_UUID));
             btif_to_bta_uuid(&bt_uuid, &uuid);

             switch(bt_uuid.len)
             {
                case (LEN_UUID_16):
                {
                  if (NULL == p_multi_adv_data_cb->inst_cb[cbindex].data.p_services)
                  {
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_services =
                                                          GKI_getbuf(sizeof(tBTA_BLE_SERVICE));
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->list_cmpl = FALSE;
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->num_service = 0;
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->p_uuid =
                              GKI_getbuf(p_adv_data->service_uuid_len / LEN_UUID_128 * LEN_UUID_16);
                      p_uuid_out16 = p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->p_uuid;
                  }

                  if (NULL != p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->p_uuid)
                  {
                     BTIF_TRACE_DEBUG("%s - In 16-UUID_data", __FUNCTION__);
                     p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_SERVICE;
                     ++p_multi_adv_data_cb->inst_cb[cbindex].data.p_services->num_service;
                     if (NULL != p_uuid_out16)
                     {
                         *p_uuid_out16++ = bt_uuid.uu.uuid16;
                     } else {
                         BTIF_TRACE_ERROR("%s: Line No:%d, p_uuid_out16 (NULL)", __FUNCTION__, __LINE__);
                     }
                  }
                  break;
                }

                case (LEN_UUID_32):
                {
                   if (NULL == p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b)
                   {
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b =
                                                          GKI_getbuf(sizeof(tBTA_BLE_32SERVICE));
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->list_cmpl = FALSE;
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->num_service = 0;
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->p_uuid =
                             GKI_getbuf(p_adv_data->service_uuid_len / LEN_UUID_128 * LEN_UUID_32);
                      p_uuid_out32 = p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->p_uuid;
                   }

                   if (NULL != p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->p_uuid)
                   {
                      BTIF_TRACE_DEBUG("%s - In 32-UUID_data", __FUNCTION__);
                      p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_SERVICE_32;
                      ++p_multi_adv_data_cb->inst_cb[cbindex].data.p_service_32b->num_service;
                      if (NULL != p_uuid_out32)
                      {
                          *p_uuid_out32++ = bt_uuid.uu.uuid32;
                      } else {
                          BTIF_TRACE_ERROR("%s: Line No:%d, p_uuid_out32 (NULL)", __FUNCTION__, __LINE__);
                      }
                   }
                   break;
                }

                case (LEN_UUID_128):
                {
                   /* Currently, only one 128-bit UUID is supported */
                   if (NULL == p_multi_adv_data_cb->inst_cb[cbindex].data.p_services_128b)
                   {
                      p_multi_adv_data_cb->inst_cb[cbindex].data.p_services_128b =
                                                          GKI_getbuf(sizeof(tBTA_BLE_128SERVICE));
                      if (NULL != p_multi_adv_data_cb->inst_cb[cbindex].data.p_services_128b)
                      {
                         BTIF_TRACE_DEBUG("%s - In 128-UUID_data", __FUNCTION__);
                         p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_SERVICE_128;
                         memcpy(p_multi_adv_data_cb->inst_cb[cbindex].data.p_services_128b->uuid128,
                                                         bt_uuid.uu.uuid128, LEN_UUID_128);
                         BTIF_TRACE_DEBUG("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", bt_uuid.uu.uuid128[0],
                            bt_uuid.uu.uuid128[1],bt_uuid.uu.uuid128[2], bt_uuid.uu.uuid128[3],
                            bt_uuid.uu.uuid128[4],bt_uuid.uu.uuid128[5],bt_uuid.uu.uuid128[6],
                            bt_uuid.uu.uuid128[7],bt_uuid.uu.uuid128[8],bt_uuid.uu.uuid128[9],
                            bt_uuid.uu.uuid128[10],bt_uuid.uu.uuid128[11],bt_uuid.uu.uuid128[12],
                            bt_uuid.uu.uuid128[13],bt_uuid.uu.uuid128[14],bt_uuid.uu.uuid128[15]);
                         p_multi_adv_data_cb->inst_cb[cbindex].data.p_services_128b->list_cmpl = TRUE;
                      }
                   }
                   break;
                }

                default:
                     break;
             }

             p_adv_data->p_service_uuid += LEN_UUID_128;
             p_adv_data->service_uuid_len -= LEN_UUID_128;
        }
    }

#if (defined(BLE_PERIPHERAL_ADV_NAME) && (BLE_PERIPHERAL_ADV_NAME == TRUE))
     p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_DEV_NAME;
#endif
     return true;
}

void btif_gattc_clear_clientif(int client_if)
{
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
    if (NULL == p_multi_adv_data_cb)
        return;

    // Clear both the inst_id and client_if values
    for (int i=0; i < BTM_BleMaxMultiAdvInstanceCount()*2; i+=2)
    {
        if (client_if == p_multi_adv_data_cb->clntif_map[i])
        {
            btif_gattc_cleanup_inst_cb(p_multi_adv_data_cb->clntif_map[i+1]);
            p_multi_adv_data_cb->clntif_map[i] = INVALID_ADV_INST;
            p_multi_adv_data_cb->clntif_map[i+1] = INVALID_ADV_INST;
            BTIF_TRACE_DEBUG("Cleaning up index %d for clnt_if :%d,", i/2, client_if);
            break;
        }
    }
}

void btif_gattc_cleanup_inst_cb(int inst_id)
{
    int cbindex = 0;
    // Check for invalid instance id
    if (inst_id < 0 || inst_id >= BTM_BleMaxMultiAdvInstanceCount())
        return;

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
    if (NULL == p_multi_adv_data_cb)
        return;

    if (inst_id > 0)
    {
        cbindex = btif_gattc_obtain_idx_for_datacb(inst_id, INST_ID_IDX);
        if (cbindex < 0)
            return;
    } else {
        if (STD_ADV_INSTID == inst_id)
          cbindex = STD_ADV_INSTID;
    }

    if (inst_id != INVALID_ADV_INST)
    {
        BTIF_TRACE_DEBUG("Cleaning up multi_inst_cb for inst_id %d, cbindex %d", inst_id, cbindex);
        btif_gattc_cleanup_multi_inst_cb(&p_multi_adv_data_cb->inst_cb[cbindex]);
        p_multi_adv_data_cb->inst_cb[cbindex].inst_id = INVALID_ADV_INST;
    }
}

void btif_gattc_cleanup_multi_inst_cb(btgatt_multi_adv_inst_cb *p_multi_inst_cb)
{
    if (p_multi_inst_cb == NULL)
        return;

    // Discoverability timer cleanup
    if (p_multi_inst_cb->tle_limited_timer.in_use)
        btu_stop_timer_oneshot(&p_multi_inst_cb->tle_limited_timer);

    // Manufacturer data cleanup
    if (p_multi_inst_cb->data.p_manu != NULL)
    {
        if (p_multi_inst_cb->data.p_manu->p_val != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_manu->p_val);
        GKI_freebuf(p_multi_inst_cb->data.p_manu);
    }

    // Proprietary data cleanup
    if (p_multi_inst_cb->data.p_proprietary != NULL)
    {
        int i = 0;
        tBTA_BLE_PROP_ELEM *p_elem = p_multi_inst_cb->data.p_proprietary->p_elem;
        while (i++ != p_multi_inst_cb->data.p_proprietary->num_elem
            && p_elem)
        {
            if (p_elem->p_val != NULL)
                GKI_freebuf(p_elem->p_val);
            ++p_elem;
        }

        if (p_multi_inst_cb->data.p_proprietary->p_elem != NULL)
            GKI_freebuf(p_multi_inst_cb->data.p_proprietary->p_elem);
        GKI_freebuf(p_multi_inst_cb->data.p_proprietary);
    }

    // Service list cleanup
    if (p_multi_inst_cb->data.p_services != NULL)
    {
        if (p_multi_inst_cb->data.p_services->p_uuid != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_services->p_uuid);
        GKI_freebuf(p_multi_inst_cb->data.p_services);
    }

    // Service data cleanup
    if (p_multi_inst_cb->data.p_service_data != NULL)
    {
        if (p_multi_inst_cb->data.p_service_data->p_val != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_service_data->p_val);
        GKI_freebuf(p_multi_inst_cb->data.p_service_data);
    }

    if (p_multi_inst_cb->data.p_services_128b != NULL)
        GKI_freebuf(p_multi_inst_cb->data.p_services_128b);

    if (p_multi_inst_cb->data.p_service_32b != NULL)
    {
        if (p_multi_inst_cb->data.p_service_32b->p_uuid != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_service_32b->p_uuid);
        GKI_freebuf(p_multi_inst_cb->data.p_service_32b);
    }

    if (p_multi_inst_cb->data.p_sol_services != NULL)
    {
        if (p_multi_inst_cb->data.p_sol_services->p_uuid != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_sol_services->p_uuid);
        GKI_freebuf(p_multi_inst_cb->data.p_sol_services);
    }

    if (p_multi_inst_cb->data.p_sol_service_32b != NULL)
    {
        if (p_multi_inst_cb->data.p_sol_service_32b->p_uuid != NULL)
           GKI_freebuf(p_multi_inst_cb->data.p_sol_service_32b->p_uuid);
        GKI_freebuf(p_multi_inst_cb->data.p_sol_service_32b);
    }

    if (p_multi_inst_cb->data.p_sol_service_128b != NULL)
        GKI_freebuf(p_multi_inst_cb->data.p_sol_service_128b);
}

void btif_multi_adv_timer_ctrl(int client_if, TIMER_CBACK cb)
{
    int inst_id = btif_multi_adv_instid_for_clientif(client_if);
    if (inst_id == INVALID_ADV_INST)
        return;

    int cbindex = btif_gattc_obtain_idx_for_datacb(inst_id, INST_ID_IDX);
    if (cbindex == INVALID_ADV_INST)
        return;

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
    if (p_multi_adv_data_cb == NULL)
        return;

    if (cb == NULL)
    {
        if (p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer.in_use)
            btu_stop_timer_oneshot(&p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer);
    } else {
        if (p_multi_adv_data_cb->inst_cb[cbindex].timeout_s != 0)
        {
            if (p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer.in_use)
                btu_stop_timer_oneshot(&p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer);

            memset(&p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer, 0, sizeof(TIMER_LIST_ENT));
            p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer.param = (UINT32)cb;
            p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer.data = (UINT32)client_if;
            btu_start_timer_oneshot(&p_multi_adv_data_cb->inst_cb[cbindex].tle_limited_timer,
                    BTU_TTYPE_USER_FUNC, p_multi_adv_data_cb->inst_cb[cbindex].timeout_s);
        }
    }
}

#endif
