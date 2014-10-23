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
#define SDP_DB_SIZE 8000
#define MAX_NO_OF_UUID 93
#include <bt_testapp.h>
static tSDP_DISC_CMPL_CB *pSDP_cmpl_cb = NULL;
static tSDP_DISCOVERY_DB *tsdp_db;
static int init(tSDP_DISC_CMPL_CB *p);
static int GetRemoteDeviceName(UINT8 *p_bd_addr, \
        tREMOTE_DEVICE_NAME_CB* rmd_name_callback);
static int searchServices(UINT8 *p_bd_addr);
static UINT32 createNewRecord();
static int addRecord(UINT32 handle,profileName profile);
static void printSearchedServices(void);
static void Cleanup(void);
static int addDummyService(UINT32 handle);
static BOOLEAN issdpenabled =FALSE;
static UINT16 num_uuid = MAX_NO_OF_UUID;

#define DISC_RAW_DATA_BUF       (4096)
UINT8 disc_raw_data_buf[DISC_RAW_DATA_BUF];

static const btsdp_interface_t btsdpInterface = {
    sizeof(btsdp_interface_t),
    init,
    GetRemoteDeviceName,
    searchServices,
    createNewRecord,
    addRecord,
    Cleanup,
    printSearchedServices
};

const btsdp_interface_t *btif_sdp_get_interface(void)
{
    printf("\n%s\n", __FUNCTION__);
    return &btsdpInterface;
}

static int init(tSDP_DISC_CMPL_CB *p)
{
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
    tSDP_UUID uuid;

    tsdp_db = (tSDP_DISCOVERY_DB *) GKI_getbuf(SDP_DB_SIZE);
    if (NULL == tsdp_db)
    {
        printf("Failed to get memory\n");
        return FAIL;
    }

    uuid.uu.uuid16 = UUID_PROTOCOL_L2CAP;
    uuid.len = LEN_UUID_16;

    SDP_InitDiscoveryDb (tsdp_db, SDP_DB_SIZE, 1, &uuid, 0, NULL);

    memset(disc_raw_data_buf, 0, sizeof(disc_raw_data_buf));
    tsdp_db->raw_data = disc_raw_data_buf;
    tsdp_db->raw_size = DISC_RAW_DATA_BUF;

    if(SDP_ServiceSearchAttributeRequest(p_bd_addr,tsdp_db,pSDP_cmpl_cb)) {
        printf("Search Services started for %x:%x:%x:%x:%x:%x\n", \
        *p_bd_addr,*(p_bd_addr+1),*(p_bd_addr+2),*(p_bd_addr+3), \
        *(p_bd_addr+4),*(p_bd_addr+5));
        printf("Wait for the SDP to complete\n");
    } else {
        printf("Failed to Initiate SDP for remote device %x:%x:%x:%x:%x:%x\n", \
        *p_bd_addr,*(p_bd_addr+1),*(p_bd_addr+2),*(p_bd_addr+3),
        *(p_bd_addr+4),*(p_bd_addr+5));
        return FAIL;
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
    printf("Adding a Dummy UUID: \
    UUID_SERVCLASS_TEST_SERVER(0x9000) \
    with all possible Protocols in the protocol List\n");

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
    result &= SDP_AddProfileDescriptorList(handle,
                UUID_SERVCLASS_TEST_SERVER, 0x0001);
    result &= SDP_AddAttribute(handle, ATTR_ID_ADDITION_PROTO_DESC_LISTS,
                TEXT_STR_DESC_TYPE,(UINT32)(strlen("Manoj")),(UINT8 *)"Manoj");
    browse_list[0] = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    result &= SDP_AddUuidSequence(handle, ATTR_ID_BROWSE_GROUP_LIST, 1,
                  browse_list);
    if(result==1) {
        printf("successfully added the details in database\n");
        bta_sys_add_uuid(services_UUIDS[0]);
        return SUCCESS;
    } else {
        printf("Failed to add details in the database\n");
        return FAIL;
}
}

static void printSearchedServices(void)
{
    tSDP_DISC_REC       *p_rec = NULL;
    tSDP_UUID uuid_list[MAX_NO_OF_UUID];
    UINT16 uuid = -1;
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
    uuid_list[22].uu.uuid16=UUID_SERVCLASS_LAN_ACCESS_USING_PPP;
    uuid_list[23].uu.uuid16=UUID_SERVCLASS_DIALUP_NETWORKING;
    uuid_list[24].uu.uuid16=UUID_SERVCLASS_IRMC_SYNC;
    uuid_list[25].uu.uuid16=UUID_SERVCLASS_IRMC_SYNC_COMMAND;
    uuid_list[26].uu.uuid16=UUID_SERVCLASS_INTERCOM;
    uuid_list[27].uu.uuid16=UUID_SERVCLASS_FAX;
    uuid_list[28].uu.uuid16=UUID_SERVCLASS_HEADSET_AUDIO_GATEWAY;
    uuid_list[29].uu.uuid16=UUID_SERVCLASS_WAP;
    uuid_list[30].uu.uuid16=UUID_SERVCLASS_WAP_CLIENT;
    uuid_list[31].uu.uuid16=UUID_SERVCLASS_IMAGING;
    uuid_list[32].uu.uuid16=UUID_SERVCLASS_IMAGING_RESPONDER;
    uuid_list[33].uu.uuid16=UUID_SERVCLASS_IMAGING_AUTO_ARCHIVE;
    uuid_list[34].uu.uuid16=UUID_SERVCLASS_IMAGING_REF_OBJECTS;
    uuid_list[35].uu.uuid16=UUID_SERVCLASS_HF_HANDSFREE;
    uuid_list[36].uu.uuid16=UUID_SERVCLASS_AG_HANDSFREE;
    uuid_list[37].uu.uuid16=UUID_SERVCLASS_DIR_PRT_REF_OBJ_SERVICE;
    uuid_list[38].uu.uuid16=UUID_SERVCLASS_REFLECTED_UI;
    uuid_list[39].uu.uuid16=UUID_SERVCLASS_BASIC_PRINTING;
    uuid_list[40].uu.uuid16=UUID_SERVCLASS_PRINTING_STATUS;
    uuid_list[41].uu.uuid16=UUID_SERVCLASS_HUMAN_INTERFACE;
    uuid_list[42].uu.uuid16=UUID_SERVCLASS_CABLE_REPLACEMENT;
    uuid_list[43].uu.uuid16=UUID_SERVCLASS_HCRP_PRINT;
    uuid_list[44].uu.uuid16=UUID_SERVCLASS_HCRP_SCAN;
    uuid_list[45].uu.uuid16=UUID_SERVCLASS_COMMON_ISDN_ACCESS;
    uuid_list[46].uu.uuid16=UUID_SERVCLASS_VIDEO_CONFERENCING_GW;
    uuid_list[47].uu.uuid16=UUID_SERVCLASS_UDI_MT;
    uuid_list[48].uu.uuid16=UUID_SERVCLASS_UDI_TA;
    uuid_list[49].uu.uuid16=UUID_SERVCLASS_VCP;
    uuid_list[50].uu.uuid16=UUID_SERVCLASS_SAP;
    uuid_list[51].uu.uuid16=UUID_SERVCLASS_PHONE_ACCESS;
    uuid_list[52].uu.uuid16=UUID_SERVCLASS_PNP_INFORMATION;
    uuid_list[53].uu.uuid16=UUID_SERVCLASS_GENERIC_NETWORKING;
    uuid_list[54].uu.uuid16=UUID_SERVCLASS_GENERIC_FILETRANSFER;
    uuid_list[55].uu.uuid16=UUID_SERVCLASS_GENERIC_AUDIO;
    uuid_list[56].uu.uuid16=UUID_SERVCLASS_GENERIC_TELEPHONY;
    uuid_list[57].uu.uuid16=UUID_SERVCLASS_UPNP_SERVICE;
    uuid_list[58].uu.uuid16=UUID_SERVCLASS_UPNP_IP_SERVICE;
    uuid_list[59].uu.uuid16=UUID_SERVCLASS_ESDP_UPNP_IP_PAN;
    uuid_list[60].uu.uuid16=UUID_SERVCLASS_ESDP_UPNP_IP_LAP;
    uuid_list[61].uu.uuid16=UUID_SERVCLASS_ESDP_UPNP_IP_L2CAP;
    uuid_list[62].uu.uuid16=UUID_SERVCLASS_VIDEO_SOURCE;
    uuid_list[63].uu.uuid16=UUID_SERVCLASS_VIDEO_SINK;
    uuid_list[64].uu.uuid16=UUID_SERVCLASS_VIDEO_DISTRIBUTION;
    uuid_list[65].uu.uuid16=UUID_SERVCLASS_HDP_PROFILE;
    uuid_list[66].uu.uuid16=UUID_SERVCLASS_HDP_SOURCE;
    uuid_list[67].uu.uuid16=UUID_SERVCLASS_HDP_SINK;
    uuid_list[68].uu.uuid16=UUID_SERVCLASS_MAP_PROFILE;
    uuid_list[69].uu.uuid16=UUID_SERVCLASS_MESSAGE_ACCESS;
    uuid_list[70].uu.uuid16=UUID_SERVCLASS_MESSAGE_NOTIFICATION;
    uuid_list[71].uu.uuid16=UUID_SERVCLASS_GAP_SERVER;
    uuid_list[72].uu.uuid16=UUID_SERVCLASS_GATT_SERVER;
    uuid_list[73].uu.uuid16=UUID_SERVCLASS_IMMEDIATE_ALERT;
    uuid_list[74].uu.uuid16=UUID_SERVCLASS_LINKLOSS;
    uuid_list[75].uu.uuid16=UUID_SERVCLASS_TX_POWER;
    uuid_list[76].uu.uuid16=UUID_SERVCLASS_CURRENT_TIME;
    uuid_list[77].uu.uuid16=UUID_SERVCLASS_DST_CHG;
    uuid_list[78].uu.uuid16=UUID_SERVCLASS_REF_TIME_UPD;
    uuid_list[79].uu.uuid16=UUID_SERVCLASS_THERMOMETER;
    uuid_list[80].uu.uuid16=UUID_SERVCLASS_DEVICE_INFO;
    uuid_list[81].uu.uuid16=UUID_SERVCLASS_NWA;
    uuid_list[82].uu.uuid16=UUID_SERVCLASS_HEART_RATE;
    uuid_list[83].uu.uuid16=UUID_SERVCLASS_PHALERT;
    uuid_list[84].uu.uuid16=UUID_SERVCLASS_BATTERY;
    uuid_list[85].uu.uuid16=UUID_SERVCLASS_BPM;
    uuid_list[86].uu.uuid16=UUID_SERVCLASS_ALERT_NOTIFICATION;
    uuid_list[87].uu.uuid16=UUID_SERVCLASS_LE_HID;
    uuid_list[88].uu.uuid16=UUID_SERVCLASS_SCAN_PARAM;
    uuid_list[89].uu.uuid16=UUID_SERVCLASS_GLUCOSE;
    uuid_list[90].uu.uuid16=UUID_SERVCLASS_RSC;
    uuid_list[91].uu.uuid16=UUID_SERVCLASS_CSC;
    uuid_list[92].uu.uuid16=UUID_SERVCLASS_TEST_SERVER;

    p_rec = NULL;

    for ( i = 0; i < MAX_NO_OF_UUID; i++)
    {
        uuid = uuid_list[i].uu.uuid16;
        /* get next record; if none found, we're done */
        if ((p_rec = SDP_FindServiceInDb(tsdp_db, uuid, p_rec)) != NULL)
        {
            printf("Remote Device Service(s) UUID(s) = 0x%x\n", uuid);
        }
    }
    return;
}

static void Cleanup(void)
{
    GKI_freebuf(tsdp_db);
    return;
}

static int GetRemoteDeviceName(UINT8 *p_bd_addr,
        tREMOTE_DEVICE_NAME_CB* rmd_name_callback)
{
   return BTM_ReadRemoteDeviceName (p_bd_addr,
        (tBTM_CMPL_CB *)rmd_name_callback, BT_TRANSPORT_BR_EDR);
}

#endif //#ifdef TEST_APP_INTERFACE
