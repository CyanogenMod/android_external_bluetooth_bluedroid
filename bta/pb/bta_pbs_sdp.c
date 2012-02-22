/*****************************************************************************
**
**  Name:       bta_pbs_sdp.c
**
**  File:       Implements the SDP functions used by Phone Book Access Server
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)

#include <string.h>

#include "sdp_api.h"
#include "bta_pbs_int.h"

/*****************************************************************************
**   Constants
*****************************************************************************/


/*****************************************************************************
**
**  Function:    bta_pbs_sdp_register()
**
**  Purpose:     Registers the PBS service with SDP
**
**  Parameters:  
**
**
**  Returns:     void
**
*****************************************************************************/
void bta_pbs_sdp_register (tBTA_PBS_CB *p_cb, char *p_service_name)
{
    tSDP_PROTOCOL_ELEM  protoList [3];
    UINT16              pbs_service = UUID_SERVCLASS_PBAP_PSE;
// btla-specific ++
    UINT16              browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
// btla-specific --
    BOOLEAN             status = FALSE;

    if ((p_cb->sdp_handle = SDP_CreateRecord()) == 0)
    {
        APPL_TRACE_WARNING0("PBS SDP: Unable to register PBS Service");
        return;
    }

    /* add service class */
    if (SDP_AddServiceClassIdList(p_cb->sdp_handle, 1, &pbs_service))
    {
        memset( protoList, 0 , 3*sizeof(tSDP_PROTOCOL_ELEM) );    
        /* add protocol list, including RFCOMM scn */
        protoList[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
        protoList[0].num_params = 0;
        protoList[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
        protoList[1].num_params = 1;
        protoList[1].params[0] = p_cb->scn;
        protoList[2].protocol_uuid = UUID_PROTOCOL_OBEX;
        protoList[2].num_params = 0;

        if (SDP_AddProtocolList(p_cb->sdp_handle, 3, protoList))
        {
            status = TRUE;  /* All mandatory fields were successful */

            /* optional:  if name is not "", add a name entry */
            if (*p_service_name != '\0')
                SDP_AddAttribute(p_cb->sdp_handle,
                                 (UINT16)ATTR_ID_SERVICE_NAME,
                                 (UINT8)TEXT_STR_DESC_TYPE,
                                 (UINT32)(strlen(p_service_name) + 1),
                                 (UINT8 *)p_service_name);

            /* Add in the Bluetooth Profile Descriptor List */
            SDP_AddProfileDescriptorList(p_cb->sdp_handle,
                                             UUID_SERVCLASS_PHONE_ACCESS,
                                             BTA_PBS_DEFAULT_VERSION);

        } /* end of setting mandatory protocol list */
    } /* end of setting mandatory service class */

    /* add supported feature and repositories */
    if (status)
    {
// btla-specific ++
      //        SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_SUPPORTED_FEATURES, UINT_DESC_TYPE,
      //          (UINT32)1, (UINT8*)&p_bta_pbs_cfg->supported_features);
        SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_SUPPORTED_REPOSITORIES, UINT_DESC_TYPE,
                  (UINT32)1, (UINT8*)&p_bta_pbs_cfg->supported_repositories);

        /* Make the service browseable */
        SDP_AddUuidSequence (p_cb->sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, &browse);
// btla-specific --
    }

    if (!status)
    {
        SDP_DeleteRecord(p_cb->sdp_handle);
        APPL_TRACE_ERROR0("bta_pbs_sdp_register FAILED");
    }
    else
    {
        bta_sys_add_uuid(pbs_service);  /* UUID_SERVCLASS_PBAP_PSE */
        APPL_TRACE_DEBUG1("PBS:  SDP Registered (handle 0x%08x)", p_cb->sdp_handle);
    }

    return;
}
#endif /* BTA_PBS_INCLUDED */
