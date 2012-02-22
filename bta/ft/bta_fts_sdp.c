/*****************************************************************************
**
**  Name:       bta_fts_sdp.c
**
**  File:       Implements the SDP functions used by File Transfer Server
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>

#include "sdp_api.h"
#include "bta_fts_int.h"
#include "goep_util.h"

/*****************************************************************************
**   Constants
*****************************************************************************/


/*****************************************************************************
**
**  Function:    bta_fts_sdp_register()
**
**  Purpose:     Registers the File Transfer service with SDP
**
**  Parameters:  
**
**
**  Returns:     void
**
*****************************************************************************/
void bta_fts_sdp_register (tBTA_FTS_CB *p_cb, char *p_service_name)
{
    UINT16              ftp_service = UUID_SERVCLASS_OBEX_FILE_TRANSFER;
    tGOEP_ERRORS        status = GOEP_ERROR;
    UINT16              version = BTA_FTS_DEFAULT_VERSION;
    UINT8               temp[4], *p;

    if (p_bta_ft_cfg->over_l2cap)
    {
        version = BTA_FT_ENHANCED_VERSION;
    }
    status = GOEP_Register (p_service_name, &p_cb->sdp_handle, p_cb->scn, 1, &ftp_service,
        ftp_service, version);

    if (status == GOEP_SUCCESS)
    {
        if (p_bta_ft_cfg->over_l2cap)
        {
            /* add the psm */
            p = temp;
            UINT16_TO_BE_STREAM(p, p_cb->psm);
            SDP_AddAttribute(p_cb->sdp_handle, ATTR_ID_OBX_OVR_L2CAP_PSM, UINT_DESC_TYPE,
                      (UINT32)2, (UINT8*)temp);
        }


        bta_sys_add_uuid(ftp_service); /* UUID_SERVCLASS_OBEX_FILE_TRANSFER */
        APPL_TRACE_DEBUG1("FTS:  SDP Registered (handle 0x%08x)", p_cb->sdp_handle);
    }

    return;
}
