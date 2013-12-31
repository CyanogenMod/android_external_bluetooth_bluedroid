/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hardware/bluetooth.h>
#include "sdp_api.h"
#include "sdpdefs.h"
#include "bta_sys.h"
#if TEST_APP_INTERFACE == TRUE
#include <bt_testapp.h>
static tSDP_DISC_CMPL_CB *pSDP_cmpl_cb = NULL;
static tSDP_DISCOVERY_DB tsdp_db;
static int init(tSDP_DISC_CMPL_CB *p);
static int searchServices(UINT8 *p_bd_addr);
static UINT32 createNewRecord();
static int addRecord(UINT32 handle,profileName profile);
static int addDummyService(UINT32 handle);
static BOOLEAN issdpenabled =FALSE;

static const btsdp_interface_t btsdpInterface = {
    sizeof(btsdp_interface_t),
    init,
    searchServices,
    createNewRecord,
    addRecord,
    NULL,
};

const btsdp_interface_t *btif_sdp_get_interface(void)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    printf("\n%s\n", __FUNCTION__);
    return &btsdpInterface;
}

static int init(tSDP_DISC_CMPL_CB *p)
{
    BTIF_TRACE_EVENT0("Registering for SDP Complete callbacks");
    printf("Registering SDP Calbacks\n");
    pSDP_cmpl_cb = p;
    return SUCCESS;
}

static UINT32 createNewRecord()
{
    return SDP_CreateRecord();
}

static int addRecord(UINT32 handle, profileName profile)
{

    switch(profile) {
    case SPP:
        printf("Not Supported Yet\n");
        break;
    case DUMMY:
        return addDummyService(handle);
    default:
        printf("Invalid Profile selected\n");
        //print help here
        break;
    }
    return FAIL;
}

static int searchServices(UINT8 *p_bd_addr)
{

    tSDP_UUID uuid_list[22];
    UINT16 num_uuid = 1;
    UINT8 num_attr=5;
    UINT16 attr_list[5];
    int i = 1;
    uuid_list[0].uu.uuid16=UUID_SERVCLASS_OBEX_OBJECT_PUSH;
    uuid_list[1].uu.uuid16=UUID_SERVCLASS_OBEX_FILE_TRANSFER;
    uuid_list[2].uu.uuid16=UUID_SERVCLASS_AUDIO_SOURCE;
    uuid_list[3].uu.uuid16=UUID_SERVCLASS_AUDIO_SINK;
    uuid_list[4].uu.uuid16=UUID_SERVCLASS_ADV_AUDIO_DISTRIBUTION;
    uuid_list[5].uu.uuid16=UUID_SERVCLASS_AV_REM_CTRL_TARGET;
    uuid_list[6].uu.uuid16=UUID_SERVCLASS_AV_REMOTE_CONTROL;
    uuid_list[7].uu.uuid16=UUID_SERVCLASS_AV_REM_CTRL_CONTROL;
    uuid_list[8].uu.uuid16=UUID_SERVCLASS_PANU;
    uuid_list[9].uu.uuid16=UUID_SERVCLASS_NAP;
    uuid_list[10].uu.uuid16=UUID_SERVCLASS_GN ;
    uuid_list[11].uu.uuid16=UUID_SERVCLASS_DIRECT_PRINTING;
    uuid_list[12].uu.uuid16=UUID_SERVCLASS_REFERENCE_PRINTING;
    uuid_list[13].uu.uuid16=UUID_SERVCLASS_SERIAL_PORT;
    uuid_list[14].uu.uuid16=UUID_SERVCLASS_HEADSET;
    uuid_list[15].uu.uuid16=UUID_SERVCLASS_CORDLESS_TELEPHONY ;
    uuid_list[16].uu.uuid16=UUID_SERVCLASS_PBAP_PCE;
    uuid_list[17].uu.uuid16=UUID_SERVCLASS_PBAP_PSE;
    uuid_list[18].uu.uuid16=UUID_SERVCLASS_HEADSET_HS;
    uuid_list[19].uu.uuid16=UUID_SERVCLASS_MAP_PROFILE;
    uuid_list[20].uu.uuid16=UUID_SERVCLASS_MESSAGE_ACCESS;
    uuid_list[21].uu.uuid16=UUID_SERVCLASS_MESSAGE_NOTIFICATION;
    uuid_list[0].len = LEN_UUID_16;
    uuid_list[1].len = LEN_UUID_16;
    uuid_list[2].len = LEN_UUID_16;
    uuid_list[3].len = LEN_UUID_16;
    uuid_list[4].len = LEN_UUID_16;
    uuid_list[5].len = LEN_UUID_16;
    uuid_list[6].len = LEN_UUID_16;
    uuid_list[7].len = LEN_UUID_16;
    uuid_list[8].len = LEN_UUID_16;
    uuid_list[9].len = LEN_UUID_16;
    uuid_list[10].len = LEN_UUID_16;
    uuid_list[11].len = LEN_UUID_16;
    uuid_list[12].len = LEN_UUID_16;
    uuid_list[13].len = LEN_UUID_16;
    uuid_list[14].len = LEN_UUID_16;
    uuid_list[15].len = LEN_UUID_16;
    uuid_list[16].len = LEN_UUID_16;
    uuid_list[17].len = LEN_UUID_16;
    uuid_list[18].len = LEN_UUID_16;
    uuid_list[19].len = LEN_UUID_16;
    uuid_list[20].len = LEN_UUID_16;
    uuid_list[21].len = LEN_UUID_16;
    attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
    attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;
    attr_list[2] = ATTR_ID_BT_PROFILE_DESC_LIST;
    attr_list[3] = ATTR_ID_SUPPORTED_FEATURES;
    attr_list[4] = ATTR_ID_SERVICE_AVAILABILITY ;
    BTIF_TRACE_EVENT0("Initializing SDP database");
    printf("Initializing SDP database\n");
    for(i=0;i<num_uuid;i++) {
        if(i>0) {
        sleep(1);
        }
    SDP_InitDiscoveryDb(&tsdp_db,sizeof(tsdp_db),1,&uuid_list[i],num_attr,attr_list);
    if(SDP_ServiceSearchAttributeRequest(p_bd_addr,&tsdp_db,pSDP_cmpl_cb)) {
        BTIF_TRACE_EVENT6("Search Services started for %x:%x:%x:%x:%x:%x",*(p_bd_addr),*(p_bd_addr+1),*(p_bd_addr+2),*(p_bd_addr+3),*(p_bd_addr+4),*(p_bd_addr+5));
        printf("Search Services started for %x:%x:%x:%x:%x:%x\n",*p_bd_addr,*(p_bd_addr+1),*(p_bd_addr+2),*(p_bd_addr+3),*(p_bd_addr+4),*(p_bd_addr+5));
        printf("Wait for the SDP complete CallBack\n");
    } else {
        BTIF_TRACE_EVENT0("Failed to initiate SDP search");
        printf("Failed to Initiate SDP for remote device %x:%x:%x:%x:%x:%x\n",*p_bd_addr,*(p_bd_addr+1),*(p_bd_addr+2),*(p_bd_addr+3),*(p_bd_addr+4),*(p_bd_addr+5));
        return FAIL;
    }
    }
return SUCCESS;
}

static int addDummyService(UINT32 handle)
{
    UINT16 num_services = 1;
    UINT16 services_UUIDS[1];
    BOOLEAN result = TRUE;
    UINT16 browse_list[1];
    services_UUIDS[0]=UUID_SERVCLASS_TEST_SERVER;
    tSDP_PROTOCOL_ELEM  proto_list[24];
    UINT16 num_elems = 24;
    BTIF_TRACE_EVENT0("Adding a Dummy UUID: UUID_SERVCLASS_TEST_SERVER(0x9000) with all possible Protocols in the protocol List\n");
    printf("Adding a Dummy UUID: UUID_SERVCLASS_TEST_SERVER(0x9000) with all possible Protocols in the protocol List\n");

    //I can add more Service_UUIDs here
    result &=SDP_AddServiceClassIdList(handle,num_services,services_UUIDS);
    memset((void*) proto_list, 0 , num_elems*sizeof(tSDP_PROTOCOL_ELEM));
    proto_list[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
    proto_list[0].num_params = 2;
    proto_list[0].params[0] = 0x001f;
    proto_list[0].params[1] = 0x001e;
    proto_list[1].protocol_uuid = UUID_PROTOCOL_AVDTP;
    proto_list[1].num_params = 0;
    proto_list[2].protocol_uuid = UUID_PROTOCOL_FTP;
    proto_list[2].num_params = 2;
    proto_list[2].params[0] = 0x0002;
    proto_list[2].params[1] = 0x0003;
    proto_list[3].protocol_uuid = UUID_PROTOCOL_RFCOMM;
    proto_list[3].num_params = 1;
    proto_list[3].params[0] = 0x0001;
    proto_list[4].protocol_uuid = UUID_PROTOCOL_BNEP;
    proto_list[4].num_params = 0;
    proto_list[5].protocol_uuid = UUID_PROTOCOL_HIDP;
    proto_list[5].num_params = 2;
    proto_list[5].params[0] = 0x0002;
    proto_list[5].params[1] = 0x0003;
    proto_list[6].protocol_uuid = UUID_PROTOCOL_UPNP;
    proto_list[6].num_params = 2;
    proto_list[6].params[0] = 0x0002;
    proto_list[6].params[1] = 0x0003;
    proto_list[7].protocol_uuid = UUID_PROTOCOL_HCRP_CTRL;
    proto_list[7].num_params = 2;
    proto_list[7].params[0] = 0x0002;
    proto_list[7].params[1] = 0x0003;
    proto_list[8].protocol_uuid = UUID_PROTOCOL_HCRP_DATA;
    proto_list[8].num_params = 2;
    proto_list[8].params[0] = 0x0002;
    proto_list[8].params[1] = 0x0003;
    proto_list[9].protocol_uuid = UUID_PROTOCOL_HCRP_NOTIF;
    proto_list[9].num_params = 2;
    proto_list[9].params[0] = 0x0002;
    proto_list[9].params[1] = 0x0003;
    proto_list[10].protocol_uuid = UUID_PROTOCOL_CMTP;
    proto_list[10].num_params = 2;
    proto_list[10].params[0] = 0x0002;
    proto_list[10].params[1] = 0x0003;
    proto_list[11].protocol_uuid = UUID_PROTOCOL_UDI;
    proto_list[11].num_params = 2;
    proto_list[11].params[0] = 0x0002;
    proto_list[11].params[1] = 0x0003;
    proto_list[12].protocol_uuid = UUID_PROTOCOL_MCAP_CTRL;
    proto_list[12].num_params = 2;
    proto_list[12].params[0] = 0x0002;
    proto_list[12].params[1] = 0x0003;
    proto_list[13].protocol_uuid = UUID_PROTOCOL_MCAP_DATA;
    proto_list[13].num_params = 2;
    proto_list[13].params[0] = 0x0002;
    proto_list[13].params[1] = 0x0003;
    proto_list[14].protocol_uuid = UUID_PROTOCOL_TCS_BIN;
    proto_list[14].num_params = 2;
    proto_list[14].params[0] = 0x0002;
    proto_list[14].params[1] = 0x0003;
    proto_list[15].protocol_uuid = UUID_PROTOCOL_AVCTP;
    proto_list[15].num_params = 0;
    proto_list[16].protocol_uuid = UUID_PROTOCOL_WSP;
    proto_list[16].num_params = 2;
    proto_list[16].params[0] = 0x0002;
    proto_list[16].params[1] = 0x0003;
    proto_list[17].protocol_uuid = UUID_PROTOCOL_HTTP;
    proto_list[17].num_params = 2;
    proto_list[17].params[0] = 0x0002;
    proto_list[17].params[1] = 0x0003;
    proto_list[18].protocol_uuid = UUID_PROTOCOL_IP;
    proto_list[18].num_params = 2;
    proto_list[18].params[0] = 0x0002;
    proto_list[18].params[1] = 0x0003;
    proto_list[19].protocol_uuid = UUID_PROTOCOL_OBEX;
    proto_list[19].num_params = 2;
    proto_list[19].params[0] = 0x0002;
    proto_list[19].params[1] = 0x0003;
    proto_list[20].protocol_uuid = UUID_PROTOCOL_TCS_AT;
    proto_list[20].num_params = 2;
    proto_list[20].params[0] = 0x0002;
    proto_list[20].params[1] = 0x0003;
    proto_list[21].protocol_uuid = UUID_PROTOCOL_UDP;
    proto_list[21].num_params = 1;
    proto_list[21].params[0] = 0x0002;
    proto_list[22].protocol_uuid = UUID_PROTOCOL_SDP;
    proto_list[22].num_params = 2;
    proto_list[22].params[0] = 0x0002;
    proto_list[22].params[1] = 0x0003;
    proto_list[23].protocol_uuid = UUID_PROTOCOL_TCP;
    proto_list[23].num_params = 1;
    proto_list[23].params[0] = 0x0002;
    result &= SDP_AddProtocolList(handle, num_elems, proto_list);
    result &= SDP_AddProfileDescriptorList(handle, UUID_SERVCLASS_TEST_SERVER, 0x0001);
    result &= SDP_AddAttribute(handle, ATTR_ID_ADDITION_PROTO_DESC_LISTS,TEXT_STR_DESC_TYPE,(UINT32)(strlen("Manoj")),(UINT8 *)"Manoj");
    browse_list[0] = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    result &= SDP_AddUuidSequence(handle, ATTR_ID_BROWSE_GROUP_LIST, 1, browse_list);
    if(result==1) {
        printf("successfully added the details in database\n");
        bta_sys_add_uuid(services_UUIDS[0]);
        return SUCCESS;
    } else {
        printf("Failed to add details in the database\n");
        return FAIL;
}
}
#endif //#ifdef TEST_APP_INTERFACE
