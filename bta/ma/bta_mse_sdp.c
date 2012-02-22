/*****************************************************************************
**
**  Name:       bta_mse_sdp.c
**
**  File:       Implements the SDP functions used by Message Access Server
**
**  Copyright (c) 1998-2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>

#include "sdp_api.h"
#include "bta_mse_int.h"
#include "goep_util.h"

/*****************************************************************************
**
**  Function:    bta_mas_sdp_register
**
**  Purpose:     Register the Message Access service with SDP
**
**  Parameters:  p_cb           - Pointer to MA instance control block
**               p_service_name - MA server name
**               inst_id        - MAS instance ID
**               msg_type       - Supported message type(s)
**
**
**  Returns:     void
**
*****************************************************************************/
void bta_mse_ma_sdp_register (tBTA_MSE_MA_CB *p_cb, char *p_service_name, 
                              tBTA_MA_INST_ID inst_id, 
                              tBTA_MA_MSG_TYPE msg_type)
{
    UINT16              mas_service = UUID_SERVCLASS_MESSAGE_ACCESS;
    tGOEP_ERRORS        goep_status;

    goep_status = GOEP_Register(p_service_name, &p_cb->sdp_handle, p_cb->scn,
                                1, &mas_service, UUID_SERVCLASS_MAP_PROFILE, 
                                BTA_MAS_DEFAULT_VERSION);
    if (goep_status == GOEP_SUCCESS)
    {
        /* add instance ID */
        SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_MAS_INSTANCE_ID, UINT_DESC_TYPE,
                         (UINT32)1, (UINT8*)&inst_id);

        /* add supported message type */
        SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_SUPPORTED_MSG_TYPE, UINT_DESC_TYPE,
                         (UINT32)1, (UINT8*)&msg_type);

        bta_sys_add_uuid(mas_service); /* UUID_SERVCLASS_MESSAGE_ACCESS */
        APPL_TRACE_DEBUG1("MAS:  SDP Registered (handle 0x%08x)", p_cb->sdp_handle);
    }
    else
    {
        if (p_cb->sdp_handle) SDP_DeleteRecord(p_cb->sdp_handle);
        APPL_TRACE_ERROR0("bta_mse_ma_sdp_register FAILED");
    }

    return;
}
