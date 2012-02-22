/*****************************************************************************
**
**  Name:       goep_util.c
**
**  File:       Implements the Utility API of the GOEP Profile
**
**  Copyright (c) 2000-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>

#include "btm_api.h"
#include "btu.h"        /* BTU_HCI_RCV_MBOX */
#include "goep_util.h"
#include "goep_fs.h"
#include "goep_int.h"
#include "rfcdefs.h"    /* BT_PSM_RFCOMM */
#include "wcassert.h"   /* WC_ASSERT() */
#include "l2c_api.h"   /* WC_ASSERT() */

#if GOEP_DYNAMIC_MEMORY == FALSE
tGOEP_CB goep_cb;
#endif

/*******************************************************************************
**
** Function         GOEP_Init
**
** Description      GOEP initialization  
**
** Returns          nothing
**
*******************************************************************************/
void GOEP_Init(void)
{
    memset(&goep_cb, 0, sizeof(tGOEP_CB));

#if defined(GOEP_INITIAL_TRACE_LEVEL)
    goep_cb.trace_level = GOEP_INITIAL_TRACE_LEVEL;
#else
    goep_cb.trace_level = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif

}



/*****************************************************************************
**
**  Function:    GOEP_SetSecurityLevel()
**
**  Purpose:     Register security & encryption level for GOEP server.
**               This is not to be used for the GOEP command server.
**               
**
**  Parameters:
**       char    *pName   service name;  Typically GOEP_DEFAULT_SERVICE_NAME
**                                      Not used if BTM_SEC_SERVICE_NAME_LEN is 0.
**       UINT8   level    Security level;  The mandatory GOEP security level
**                                        GOEP_DEFAULT_SECURITY, is or'd in.
**       
**
**  Returns:     (BOOLEAN) TRUE if OK
**                         FALSE if a bad parameter
**
*****************************************************************************/
BOOLEAN GOEP_SetSecurityLevel (BOOLEAN bOrig, char *pName, UINT32 service,
                               UINT8 level, UINT8 scn)
{
#if BTM_SEC_SERVICE_NAME_LEN > 0
    char    sec_name[BTM_SEC_SERVICE_NAME_LEN+1];
#endif

    /* Guarantee that there is a name */
    if (!pName)
    {
        pName = "";
    }

#if BTM_SEC_SERVICE_NAME_LEN > 0
    GOEP_TRACE_EVENT4("GOEP:  Set Security Mode;  name '%s'; service %d; level 0x%02x;  scn %d",
        pName, service, level, scn);

    /* Guarantee that name is not too long */
    if (strlen (pName) >= BTM_SEC_SERVICE_NAME_LEN)
    {
        BCM_STRNCPY_S(sec_name, sizeof(sec_name), pName, BTM_SEC_SERVICE_NAME_LEN);
        sec_name[BTM_SEC_SERVICE_NAME_LEN] = '\0';
        pName = sec_name;
    }
#else
    GOEP_TRACE_EVENT3("GOEP:  Set Security Mode: service id %d; level 0x%02x;  scn %d",
                       service, level, scn);
#endif

    /* Register with Security Manager for the specific security level */
    if (!BTM_SetSecurityLevel (bOrig, pName, (UINT8)service, level, BT_PSM_RFCOMM,
                               BTM_SEC_PROTO_RFCOMM, scn))
    {
        GOEP_TRACE_WARNING1("GOEP:  Security Registration failed for %s", pName);
        return (FALSE);
    }

    return (TRUE);
}

/*****************************************************************************
**
**  Function:    GOEP_SetTraceLevel
**
**  Purpose:     This function changes the trace level
**
**  Returns:     Nothing
*****************************************************************************/
void GOEP_SetTraceLevel(UINT8 level)
{
    goep_cb.trace_level = level;
}

/*******************************************************************************
**
** Function         GOEP_FreeBuf
**
** Description      free memory for specified packet
**
** Returns          void
**
*******************************************************************************/
void GOEP_FreeBuf (void **p_buf)
{
    if (p_buf && *p_buf)
    {
        GKI_freebuf(*p_buf);
        *p_buf = NULL;
    }
}

/*******************************************************************************
**
** Function         GOEP_SendMsg
**
** Description      send a message to BTU task
**
** Returns          void
**
*******************************************************************************/
void GOEP_SendMsg (void *p_msg)

{
    if(p_msg)
    {
        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_msg);
    }
}

/*****************************************************************************
**
**  Function:    GOEP_Register()
**
**  Purpose:     Register an OBEX profile with SDP
**
**  Parameters:  
**
**       char *p_name   service name;  optional (may be NULL)
**       UINT32 *phSDP  handle to the created record;  cannot be NULL
**       UINT8 scn      scn desired for the service;  must be > 0
**       UINT16 version version of the service;  optional (may be 0)
**
**  Returns:     (tGOEP_ERRORS) GOEP_SUCCESS if ok;  GOEP_ERROR on error
**
**  Notes: 
**                    
**
**  Preconditions:
**    - phSDP must be set (to return the SDP record handle)
**    - scn must be set
** 
**  Postconditions:
**    - 
**
*****************************************************************************/
tGOEP_ERRORS GOEP_Register (char   *p_name,
                              UINT32 *phSDP, 
                              UINT8  scn, 
                              UINT8  num_srv_class, 
                              UINT16 *p_service_class,
                              UINT16 profile_id,
                              UINT16 version)
{
    tGOEP_ERRORS        status = GOEP_ERROR;
    tSDP_PROTOCOL_ELEM  protoList [GOEP_PROTOCOL_COUNT];
    UINT16              browse;

    /* GOEP_TRACE_EVENT4("GOEP:  Register with SDP:  name %s, scn %d, class %#x, version %d)",
                      p_name ? p_name : "NULL",
                      (UINT16)scn,
                      (UINT16)p_service_class[0],
                      version); */

    /* parameter checking */
    WC_ASSERT(phSDP);
    WC_ASSERT(scn);

    /* parameter checking */
    if (!(phSDP && scn))
    {
        return (GOEP_INVALID_PARAM);
    }

    *phSDP = SDP_CreateRecord();
    WC_ASSERT(*phSDP);
    GOEP_TRACE_API2("GOEP:  Register with SDP:  scn %d, record:0x%08x", scn, *phSDP);

    /* could a SDP handle be allocated? */
    if (*phSDP == 0)
    {
        return (GOEP_RESOURCES);
    }

    /* add service class */
    if (SDP_AddServiceClassIdList(*phSDP, num_srv_class, p_service_class))
    {
        /* add protocol list, including RFCOMM scn */
        protoList[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
        protoList[0].num_params = 0;
        protoList[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
        protoList[1].num_params = 1;
        protoList[1].params[0] = scn;
        protoList[2].protocol_uuid = UUID_PROTOCOL_OBEX;
        protoList[2].num_params = 0;

/* coverity[uninit_use_in_call]
FALSE-POSITIVE: coverity says 
Event uninit_use_in_call: Using uninitialized element of array "(protoList)->params" in call to function "SDP_AddProtocolList" 
Event uninit_use_in_call: Using uninitialized value "(protoList)->num_params" in call to function "SDP_AddProtocolList" 
Event uninit_use_in_call: Using uninitialized value "(protoList)->protocol_uuid" in call to function "SDP_AddProtocolList" 
SDP_AddProtocolList() uses (protoList)->params only when (protoList)->num_params is non-0 
*/
        if (SDP_AddProtocolList(*phSDP, GOEP_PROTOCOL_COUNT, protoList))
        {
            /* optional:  if name is not NULL, add a name entry */
            if (p_name && p_name[0] )
            {
                SDP_AddAttribute(*phSDP,
                                (UINT16)ATTR_ID_SERVICE_NAME,
                                (UINT8)TEXT_STR_DESC_TYPE,
                                (UINT32)(strlen(p_name) + 1),
                                (UINT8 *)p_name);
            } /* end of setting optional name */
            /* Add in the Bluetooth Profile Descriptor List for IrMCSync [Sync s7.1.2] */
            if (version)
            {
                SDP_AddProfileDescriptorList(*phSDP, profile_id, version);
            } /* end of setting optional profile version */
            status = GOEP_SUCCESS;
        } /* end of setting mandatory protocol list */
    } /* end of setting mandatory service class */

    /* Make the service browseable */
    browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    if (status == GOEP_SUCCESS && SDP_AddUuidSequence (*phSDP,
                                                        ATTR_ID_BROWSE_GROUP_LIST,
                                                        1,
                                                        &browse) == FALSE)
    {
        status = GOEP_ERROR;
    }

    if (status != GOEP_SUCCESS)
    {
        SDP_DeleteRecord(*phSDP);
        GOEP_TRACE_DEBUG1("GOEP_Register status: %d", status);
    }
    else
    {
        GOEP_TRACE_EVENT1("GOEP:  Register with SDP returns handle 0x%08x", *phSDP);
    }

    return (status);
}

/*****************************************************************************
**
**  Function:    GOEP_Register2()
**
**  Purpose:     Register an OBEX 1.4 profile with SDP
**
**  Parameters:  
**
**       char *p_name   service name;  optional (may be NULL)
**       UINT32 *phSDP  handle to the created record;  cannot be NULL
**       UINT8  scn      scn desired for the service;  must be > 0
**       UINT16 psm     psm desired for the service;  must be an legal L2CAP dynamic PSM
**       UINT16 version version of the service;  optional (may be 0)
**
**  Returns:     (tGOEP_ERRORS) GOEP_SUCCESS if ok;  GOEP_ERROR on error
**
**  Notes: 
**                    
**
**  Preconditions:
**    - phSDP must be set (to return the SDP record handle)
**    - psm must be set
** 
**  Postconditions:
**    - 
**
*****************************************************************************/
tGOEP_ERRORS GOEP_Register2 (char   *p_name,
                              UINT32 *phSDP, 
                              UINT16 psm, 
                              UINT8  num_srv_class, 
                              UINT16 *p_service_class,
                              UINT16 profile_id,
                              UINT16 version)
{
    tGOEP_ERRORS        status = GOEP_ERROR;
    tSDP_PROTOCOL_ELEM  protoList [GOEP_PROTOCOL_COUNT];
    UINT16              browse;
    UINT16              num_elem;
    UINT8               *p, array[3];

    /* GOEP_TRACE_EVENT4("GOEP:  Register2 with SDP:  name %s, psm 0x%x, class %#x, version %d)",
                      p_name ? p_name : "NULL",
                      (UINT16)psm,
                      (UINT16)p_service_class[0],
                      version); */

    /* parameter checking */
    WC_ASSERT(phSDP);

    /* parameter checking */
    if (!(phSDP && L2C_IS_VALID_PSM(psm)))
    {
        return (GOEP_INVALID_PARAM);
    }

    *phSDP = SDP_CreateRecord();
    WC_ASSERT(*phSDP);
    GOEP_TRACE_API2("GOEP:  Register with SDP(2):  psm 0x%x, record:0x%08x", psm, *phSDP);

    /* could a SDP handle be allocated? */
    if (*phSDP == 0)
    {
        return (GOEP_RESOURCES);
    }

    /* add service class */
    if (SDP_AddServiceClassIdList(*phSDP, num_srv_class, p_service_class))
    {
        /* add protocol list, including RFCOMM scn */
        num_elem=0;
        protoList[num_elem].protocol_uuid = UUID_PROTOCOL_L2CAP;
        protoList[num_elem].num_params = 0;
        num_elem++;
        protoList[num_elem].protocol_uuid = UUID_PROTOCOL_OBEX;
        protoList[num_elem].num_params = 0;
        num_elem++;
/* coverity[uninit_use_in_call]
FALSE-POSITIVE: coverity says 
Event uninit_use_in_call: Using uninitialized element of array "(protoList)->params" in call to function "SDP_AddProtocolList" 
Event uninit_use_in_call: Using uninitialized value "(protoList)->num_params" in call to function "SDP_AddProtocolList" 
Event uninit_use_in_call: Using uninitialized value "(protoList)->protocol_uuid" in call to function "SDP_AddProtocolList" 
SDP_AddProtocolList() uses (protoList)->params only when (protoList)->num_params is non-0 
*/
        if (SDP_AddProtocolList(*phSDP, num_elem, protoList))
        {
            p = array;
            UINT16_TO_BE_STREAM (p, psm);
            SDP_AddAttribute (*phSDP, ATTR_ID_OBX_OVR_L2CAP_PSM, UINT_DESC_TYPE, 2, array);
            
            /* optional:  if name is not NULL, add a name entry */
            if (p_name && p_name[0] )
            {
                SDP_AddAttribute(*phSDP,
                                (UINT16)ATTR_ID_SERVICE_NAME,
                                (UINT8)TEXT_STR_DESC_TYPE,
                                (UINT32)(strlen(p_name) + 1),
                                (UINT8 *)p_name);
            } /* end of setting optional name */
            /* Add in the Bluetooth Profile Descriptor List for IrMCSync [Sync s7.1.2] */
            if (version)
            {
                SDP_AddProfileDescriptorList(*phSDP, profile_id, version);
            } /* end of setting optional profile version */
            status = GOEP_SUCCESS;
        } /* end of setting mandatory protocol list */
    } /* end of setting mandatory service class */

    /* Make the service browseable */
    browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    if (status == GOEP_SUCCESS && SDP_AddUuidSequence (*phSDP,
                                                        ATTR_ID_BROWSE_GROUP_LIST,
                                                        1,
                                                        &browse) == FALSE)
    {
        status = GOEP_ERROR;
    }

    if (status != GOEP_SUCCESS)
    {
        SDP_DeleteRecord(*phSDP);
        GOEP_TRACE_DEBUG1("GOEP_Register status: %d", status);
    }
    else
    {
        GOEP_TRACE_EVENT1("GOEP:  Register with SDP returns handle 0x%08x", *phSDP);
    }


    return (status);
}

/*****************************************************************************
**
**  Function:    GOEP_AddProtoLists()
**
**  Purpose:     Add the AdditionalProtocolDescriptorLists attribute
**               to a SDP record
**
**  Parameters:  
**
**       UINT32 sdp_hdl         the SDP record handle
**       UINT8  scn             scn desired for the service;  must be > 0
**
**  Returns:     (tGOEP_ERRORS) GOEP_SUCCESS if ok;  GOEP_ERROR on error
**
**  Notes: 
**                    
**
**  Preconditions:
**    - sdp_hdl must be set (to the SDP record handle from GOEP_Register())
**    - scn must be set
** 
**  Postconditions:
**    - 
**
*****************************************************************************/
#if (BPP_INCLUDED == TRUE)
tGOEP_ERRORS GOEP_AddProtoLists ( UINT32 sdp_hdl, UINT8  scn)
{
    tGOEP_ERRORS        status = GOEP_ERROR;
    tSDP_PROTO_LIST_ELEM proto_list_elem;
    tSDP_PROTOCOL_ELEM  *p_proto_list;

    /*
    GOEP_TRACE_EVENT4("GOEP:  Register with SDP:  name %s, scn %d, class %#x, version %d)",
                      p_name ? p_name : "NULL",
                      (UINT16)scn,
                      (UINT16)p_service_class[0],
                      version);
                      */

    /* parameter checking */
    WC_ASSERT(sdp_hdl);
    WC_ASSERT(scn);

    /* parameter checking */
    if (!(sdp_hdl && scn))
    {
        return (GOEP_INVALID_PARAM);
    }

    proto_list_elem.num_elems = 0;
    p_proto_list = &proto_list_elem.list_elem[proto_list_elem.num_elems++];
    p_proto_list->num_params = 0;
    p_proto_list->protocol_uuid = UUID_PROTOCOL_L2CAP;
    p_proto_list = &proto_list_elem.list_elem[proto_list_elem.num_elems++];
    p_proto_list->num_params = 1;
    p_proto_list->params[0] = scn;
    p_proto_list->protocol_uuid = UUID_PROTOCOL_RFCOMM;
    p_proto_list = &proto_list_elem.list_elem[proto_list_elem.num_elems++];
    p_proto_list->num_params = 0;
    p_proto_list->protocol_uuid = UUID_PROTOCOL_OBEX;

    /* add protocol list, including RFCOMM scn */
    if(SDP_AddAdditionProtoLists( sdp_hdl, 1, &proto_list_elem))
        status = GOEP_SUCCESS;

    GOEP_TRACE_DEBUG1("GOEP_AddProtoLists status: %s", GOEP_ErrorName(status));

    return (status);
}
#endif

/*****************************************************************************
**
**  Function:    goep_dummy
**
**  Purpose:     This is to work around a GCC build linking problem
**
**  Returns:     none
**
*****************************************************************************/
#if (RPC_INCLUDED == TRUE && RPC_TRACE_ONLY == TRUE && GOEP_FS_INCLUDED == TRUE)
static void goep_dummy(void)
{
    GOEP_OpenRsp(0, 0, 0, 0);
}
#endif /* RPC stuff */


