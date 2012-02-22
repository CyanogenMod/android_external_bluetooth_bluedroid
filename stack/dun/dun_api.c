/*****************************************************************************/
/*                                                                           */
/*  Name:          dun_api.c                                                 */
/*                                                                           */
/*  Description:   This file contains the DUN API routines                   */
/*                                                                           */
/*                                                                           */
/*  Copyright (c) 1999-2009, Broadcom Corp., All Rights Reserved.            */
/*  Broadcom Bluetooth Core. Proprietary and confidential.                   */
/*****************************************************************************/

#include <string.h>
#include "bt_target.h"
#include "btm_api.h"
#include "port_api.h"
#include "dun_api.h"
#include "dun_int.h"
#include "rfcdefs.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* Number of attributes in DUN and Fax SDP records. */
#define DUN_NUM_ATTR_DUN        4
#define DUN_NUM_ATTR_FAX        7

/* Number of protocol elements in protocol element list. */
#define DUN_NUM_PROTO_ELEMS     2


/* Global DUN/FAX control block structure
*/
#if DUN_DYNAMIC_MEMORY == FALSE
tDUN_CB  dun_cb;
#endif

/******************************************************************************
**
** Function         dun_sdp_cback
**
** Description      This is the SDP callback function used by DUN_FindService.
**                  This function will be executed by SDP when the service
**                  search is completed.  If the search is successful, it
**                  finds the first record in the database that matches the
**                  UUID of the search.  Then retrieves various parameters
**                  from the record.  When it is finished it calls the
**                  application callback function.
**
** Returns          Nothing.
**
******************************************************************************/
static void dun_sdp_cback(UINT16 status)
{
    tSDP_DISC_REC       *p_rec = NULL;
    tSDP_DISC_ATTR      *p_attr;
    tSDP_PROTOCOL_ELEM  pe;
    UINT8               scn = 0;
    UINT16              name_len = 0;
    char                *p_name = "";
    UINT16              options = 0;
    BOOLEAN             found = FALSE;

    DUN_TRACE_API1("dun_sdp_cback status: %d", status);

    if (status == SDP_SUCCESS)
    {
        /* loop through all records we found */
        do
        {
            /* get next record; if none found, we're done */
            if ((p_rec = SDP_FindServiceInDb(dun_cb.find_cb.p_db, 
                            dun_cb.find_cb.service_uuid, p_rec)) == NULL)
            {
                break;
            }

            /* get scn from proto desc list; if not found, go to next record */
            if (SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe))
            {
                scn = (UINT8) pe.params[0];
            }
            else
            {
                continue;
            }

            /* get service name */
            if ((p_attr = SDP_FindAttributeInRec(p_rec,
                            ATTR_ID_SERVICE_NAME)) != NULL)
            {
                p_name = (char *) p_attr->attr_value.v.array;
                name_len = SDP_DISC_ATTR_LEN(p_attr->attr_len_type);
            }

            /* get audio support */
            if ((p_attr = SDP_FindAttributeInRec(p_rec,
                            ATTR_ID_AUDIO_FEEDBACK_SUPPORT)) != NULL)
            {
                options |= (p_attr->attr_value.v.u8) ? DUN_OPTIONS_AUDIO : 0;
            }

            /* get fax options */
            if(dun_cb.find_cb.service_uuid == UUID_SERVCLASS_FAX)
            {
                if ((p_attr = SDP_FindAttributeInRec(p_rec,
                                ATTR_ID_FAX_CLASS_1_SUPPORT)) != NULL)
                {
                    options |= (p_attr->attr_value.v.u8) ? DUN_OPTIONS_CLASS1 : 0;
                }

                if ((p_attr = SDP_FindAttributeInRec(p_rec,
                                ATTR_ID_FAX_CLASS_2_0_SUPPORT)) != NULL)
                {
                    options |= (p_attr->attr_value.v.u8) ? DUN_OPTIONS_CLASS20 : 0;
                }

                if ((p_attr = SDP_FindAttributeInRec(p_rec,
                                ATTR_ID_FAX_CLASS_2_SUPPORT)) != NULL)
                {
                    options |= (p_attr->attr_value.v.u8) ? DUN_OPTIONS_CLASS2 : 0;
                }
            }

            /* we've got everything, we're done */
            found = TRUE;
            break;

        } while (TRUE);
    }

    /* return info from sdp record in app callback function */
    if (dun_cb.find_cb.p_cback != NULL)
    {
        (*dun_cb.find_cb.p_cback)(found, scn, p_name, name_len, options);
    }

    return;
}

/******************************************************************************
**
** Function         DUN_Listen
**
** Description      This function opens a DUN or Fax connection in server mode.
**                  It configures the security settings for the connection and
**                  opens an RFCOMM connection in server mode.  It returns
**                  the handle for the RFCOMM connection.
**
**                  Input Parameters:
**                      service_uuid:  Indicates DUN or Fax.
**
**                      p_service_name:  A null-terminated character string 
**                      containing the service name.  This is used for security.
**                      
**                      scn:  The server channel number for the RFCOMM
**                      connection.
**
**                      mtu: The MTU, or maximum data frame size, for the
**                      RFCOMM connection.
**
**                      security_mask:  Security configuration.  See the BTM API.
**
**                      p_cback:  RFCOMM port callback function.  See the
**                      RFCOMM API.
**
**                  Output Parameters:
**                      p_handle:  The handle for the RFCOMM connection.
**                      This value is used in subsequent calls to the RFCOMM API.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_Listen(UINT16 service_uuid, char *p_service_name, UINT8 scn, UINT16 mtu,
                 UINT8 security_mask, UINT16 *p_handle, tPORT_CALLBACK *p_cback)
{
    BD_ADDR     bd_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    UINT8       status = DUN_SUCCESS;
    UINT8       service_id;

    DUN_TRACE_API1("DUN_Listen uuid: %x", service_uuid);

    switch (service_uuid)
    {
        case UUID_SERVCLASS_DIALUP_NETWORKING:
            service_id = BTM_SEC_SERVICE_DUN;
            break;

        case UUID_SERVCLASS_FAX:
            service_id = BTM_SEC_SERVICE_FAX;
            break;

        case UUID_SERVCLASS_SERIAL_PORT:
        default:
            service_id = BTM_SEC_SERVICE_SERIAL_PORT;
            break;
    }

    /* set up security */
    if (BTM_SetSecurityLevel(FALSE, (char *) p_service_name, service_id,
            security_mask, BT_PSM_RFCOMM, BTM_SEC_PROTO_RFCOMM, scn) == FALSE)
    {
        status = DUN_FAIL;
    }

    /* open rfcomm server connection */
    if (status == DUN_SUCCESS)
    {
        if (RFCOMM_CreateConnection(service_uuid, scn, TRUE, mtu, bd_addr,
            p_handle, p_cback) != PORT_SUCCESS)
        {
            status = DUN_FAIL;
        }
    }

    return status;
}

/******************************************************************************
**
** Function         DUN_Connect
**
** Description      This function opens a DUN or Fax client connection.
**                  It configures the security settings for the connection
**                  and opens an RFCOMM connection in server mode.
**                  It returns the handle for the RFCOMM connection.
**
**                  Input Parameters:
**                      service_uuid:  Indicates DUN or Fax.
**
**                      bd_addr:  BD address of the peer device.
**                      
**                      scn:  The server channel number for the RFCOMM
**                      connection.
**
**                      mtu: The MTU, or maximum data frame size, for the
**                      RFCOMM connection.
**
**                      security_mask:  Security configuration.  See the BTM API.
**
**                      p_cback:  RFCOMM port callback function.  See the
**                      RFCOMM API.
**
**                  Output Parameters:
**                      p_handle:  The handle for the RFCOMM connection.
**                      This value is used in subsequent calls to the RFCOMM API.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_Connect(UINT16 service_uuid, BD_ADDR bd_addr, UINT8 scn, UINT16 mtu,
                  UINT8 security_mask, UINT16 *p_handle, tPORT_CALLBACK *p_cback)
{
    char    name[] = "";   /* don't bother using a name for security */
    UINT8   status = DUN_SUCCESS;
    UINT8   service_id;

    DUN_TRACE_API1("DUN_Connect uuid: %x", service_uuid);

    switch (service_uuid)
    {
        case UUID_SERVCLASS_DIALUP_NETWORKING:
            service_id = BTM_SEC_SERVICE_DUN;
            break;

        case UUID_SERVCLASS_FAX:
            service_id = BTM_SEC_SERVICE_FAX;
            break;

        case UUID_SERVCLASS_SERIAL_PORT:
        default:
            service_id = BTM_SEC_SERVICE_SERIAL_PORT;
            break;
    }

    /* set up security */
    if(BTM_SetSecurityLevel(TRUE, name, service_id, security_mask,
            BT_PSM_RFCOMM, BTM_SEC_PROTO_RFCOMM, scn) == FALSE)
    {
        status = DUN_FAIL;
    }

    /* open rfcomm client connection */
    if (status == DUN_SUCCESS)
    {
        if (RFCOMM_CreateConnection(service_uuid, scn, FALSE, mtu, bd_addr,
            p_handle, p_cback) != PORT_SUCCESS)
        {
            status = DUN_FAIL;
        }
    }

    return status;
}


/******************************************************************************
**
** Function         DUN_Close
**
** Description      This function closes a DUN or Fax client connection
**                  previously opened with DUN_Connect().
**
**                  Input Parameters:
**                      handle:  The handle for the RFCOMM connection
**                      previously returned by DUN_Connect().
**
**                  Output Parameters:
**                      None.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_Close(UINT16 handle)
{
    UINT8   status = DUN_SUCCESS;

    DUN_TRACE_API0("DUN_close");

    if (RFCOMM_RemoveConnection(handle) != PORT_SUCCESS)
    {
        status = DUN_FAIL;
    }

    return status;
}


/******************************************************************************
**
** Function         DUN_Shutdown
**
** Description      This function closes a DUN or Fax server connection
**                  previously opened with DUN_Listen().  It is called if
**                  the application wishes to close the DUN or Fax server
**                  connection at any time.
**
**                  Input Parameters:
**                      service_uuid:  Indicates DUN or Fax.
**
**                      handle:  The handle for the RFCOMM connection
**                      previously returned by DUN_Connect().
**
**                  Output Parameters:
**                      None.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_Shutdown(UINT16 handle)
{
    UINT8       status = DUN_SUCCESS;

    DUN_TRACE_API0("DUN_Shutdown");

    /* close rfcomm connection */
    if (RFCOMM_RemoveServer(handle) != PORT_SUCCESS)
    {
        status = DUN_FAIL;
    }

    return status;
}

/******************************************************************************
**
** Function         DUN_AddRecord
**
** Description      This function is called by a server application to add
**                  DUN or Fax information to an SDP record.  Prior to
**                  calling this function the application must call
**                  SDP_CreateRecord() to create an SDP record.
**
**                  Input Parameters:
**                      service_uuid:  Indicates DUN or Fax.
**
**                      p_service_name:  Pointer to a null-terminated character
**                      string containing the service name.
**
**                      scn:  The server channel number of the RFCOMM
**                      connection.
**
**                      options:  Profile support options.
**
**                      sdp_handle:  SDP handle returned by SDP_CreateRecord().
**
**                  Output Parameters:
**                      None.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_AddRecord(UINT16 service_uuid, char *p_service_name, UINT8 scn,
        UINT16 options, UINT32 sdp_handle)
{
    tSDP_PROTOCOL_ELEM  proto_elem_list[DUN_NUM_PROTO_ELEMS];
    UINT8               audio;
    UINT8               class1;
    UINT8               class20;
    UINT8               class2;
    UINT16              browse_list[1];
    BOOLEAN             result = TRUE;
     
    DUN_TRACE_API1("DUN_AddRecord uuid: %x", service_uuid);
    
    memset((void*) proto_elem_list, 0 , DUN_NUM_PROTO_ELEMS*sizeof(tSDP_PROTOCOL_ELEM));

    /* add the protocol element sequence */
    proto_elem_list[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
    proto_elem_list[0].num_params = 0;
    proto_elem_list[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
    proto_elem_list[1].num_params = 1;        
    proto_elem_list[1].params[0] = scn;
    result &= SDP_AddProtocolList(sdp_handle, DUN_NUM_PROTO_ELEMS, proto_elem_list);

    /* add service class id list */
    result &= SDP_AddServiceClassIdList(sdp_handle, 1, &service_uuid);

    /* add service name */
    if (p_service_name != NULL)
    {
        result &= SDP_AddAttribute(sdp_handle, ATTR_ID_SERVICE_NAME, TEXT_STR_DESC_TYPE,
                    (UINT32)(strlen(p_service_name)+1), (UINT8 *) p_service_name);
    }

    /* add audio feedback support */
    if((service_uuid == UUID_SERVCLASS_FAX) ||
       (service_uuid == UUID_SERVCLASS_DIALUP_NETWORKING))
    {
        audio = (options & DUN_OPTIONS_AUDIO) ? TRUE : FALSE;
        result &= SDP_AddAttribute(sdp_handle, ATTR_ID_AUDIO_FEEDBACK_SUPPORT,
                    BOOLEAN_DESC_TYPE, 1, &audio);
    }
 
    /* add fax service class support */
    if(service_uuid == UUID_SERVCLASS_FAX)
    {
        class1 = (options & DUN_OPTIONS_CLASS1) ? TRUE : FALSE;
        result &= SDP_AddAttribute(sdp_handle, ATTR_ID_FAX_CLASS_1_SUPPORT,
                    BOOLEAN_DESC_TYPE, 1, &class1);
 
        class20 = (options & DUN_OPTIONS_CLASS20) ? TRUE : FALSE;
        result &= SDP_AddAttribute(sdp_handle, ATTR_ID_FAX_CLASS_2_0_SUPPORT,
                    BOOLEAN_DESC_TYPE, 1, &class20);
 
        class2 = (options & DUN_OPTIONS_CLASS2) ? TRUE : FALSE;
        result &= SDP_AddAttribute(sdp_handle, ATTR_ID_FAX_CLASS_2_SUPPORT,
                    BOOLEAN_DESC_TYPE, 1, &class2);
    }

    /* add browse group list */
    browse_list[0] = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    result &= SDP_AddUuidSequence(sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, browse_list);

    /* add profile descriptor list */    
    if (service_uuid != UUID_SERVCLASS_SERIAL_PORT)      /* BQB fix: BTA DG uses this for SPP */
        result &= SDP_AddProfileDescriptorList(sdp_handle, service_uuid, 0x0100);

    return (result ? DUN_SUCCESS : DUN_FAIL);
}

/******************************************************************************
**
** Function         DUN_FindService
**
** Description      This function is called by a client application to
**                  perform service discovery and retrieve DUN or Fax SDP
**                  record information from a server.  Information is
**                  returned for the first service record found on the
**                  server that matches the service UUID.  The callback
**                  function will be executed when service discovery is
**                  complete.  There can only be one outstanding call to
**                  DUN_FindService() at a time; the application must wait
**                  for the callback before it makes another call to
**                  the function.
**
**                  Input Parameters:
**                      service_uuid:  Indicates DUN or Fax.
**
**                      bd_addr:  BD address of the peer device.
**
**                      p_db:  Pointer to the discovery database.
**                      
**                      db_len:  Length, in bytes, of the discovery database.
**
**                      p_cback:  Pointer to the DUN_FindService()
**                      callback function.
**
**                  Output Parameters:
**                      None.
**
** Returns          DUN_SUCCESS if function execution succeeded,
**                  DUN_FAIL if function execution failed.
**
******************************************************************************/
UINT8 DUN_FindService(UINT16 service_uuid, BD_ADDR bd_addr,
                        tSDP_DISCOVERY_DB *p_db, UINT32 db_len, tDUN_FIND_CBACK *p_cback)
{
    tSDP_UUID   uuid_list;
    UINT16      num_attr;
    BOOLEAN     result = TRUE;
    UINT16      dun_attr_list[] = {ATTR_ID_SERVICE_CLASS_ID_LIST,
                                   ATTR_ID_PROTOCOL_DESC_LIST,
                                   ATTR_ID_SERVICE_NAME,
                                   ATTR_ID_AUDIO_FEEDBACK_SUPPORT,
                                   ATTR_ID_FAX_CLASS_1_SUPPORT,
                                   ATTR_ID_FAX_CLASS_2_0_SUPPORT,
                                   ATTR_ID_FAX_CLASS_2_SUPPORT};

    DUN_TRACE_API1("DUN_FindService uuid: %x", service_uuid);

    /* set up discovery database */
    uuid_list.len = LEN_UUID_16;
    uuid_list.uu.uuid16 = service_uuid;

    num_attr = (service_uuid == UUID_SERVCLASS_FAX) ?
        DUN_NUM_ATTR_FAX : DUN_NUM_ATTR_DUN;

    result = SDP_InitDiscoveryDb(p_db, db_len, 1, &uuid_list, num_attr,
                                 (UINT16 *) dun_attr_list);

    if (result == TRUE)
    {
        /* store service_uuid and discovery db pointer */
        dun_cb.find_cb.p_db = p_db;
        dun_cb.find_cb.service_uuid = service_uuid;
        dun_cb.find_cb.p_cback = p_cback;

        /* perform service search */
        result = SDP_ServiceSearchAttributeRequest(bd_addr, p_db, dun_sdp_cback);
    }

    return (result ? DUN_SUCCESS : DUN_FAIL);
}

/*******************************************************************************
**
** Function         DUN_Init
**
** Description      This function is called to initialize the control block
**                  for this layer.  It must be called before accessing any 
**                  other API functions for this layer.  It is typically called
**                  once during the start up of the stack.  
**
** Returns          void
**
*******************************************************************************/
void DUN_Init (void)
{
    /* All fields are cleared; nonzero fields are reinitialized in appropriate function */
    memset(&dun_cb, 0, sizeof(tDUN_CB));

#if defined(DUN_INITIAL_TRACE_LEVEL)
    dun_cb.trace_level = DUN_INITIAL_TRACE_LEVEL;
#else
    dun_cb.trace_level = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif
}

/******************************************************************************
**
** Function         DUN_SetTraceLevel
**
** Description      Sets the trace level for DUN. If 0xff is passed, the
**                  current trace level is returned.
**
**                  Input Parameters:
**                      new_level:  The level to set the DUN tracing to:
**                      0xff-returns the current setting.
**                      0-turns off tracing.
**                      >= 1-Errors.
**                      >= 2-Warnings.
**                      >= 3-APIs.
**                      >= 4-Events.
**                      >= 5-Debug.
**
** Returns          The new trace level or current trace level if
**                  the input parameter is 0xff.
**
******************************************************************************/
UINT8 DUN_SetTraceLevel (UINT8 new_level)
{
    if (new_level != 0xFF)
        dun_cb.trace_level = new_level;

    return (dun_cb.trace_level);
}
