/*****************************************************************************/
/*                                                                           */
/*  Name:          hidd_api.c                                                */
/*                                                                           */
/*  Description:   this file contains the Device HID API entry points        */
/*                                                                           */
/*                                                                           */
/*  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.                   */
/*  WIDCOMM Bluetooth Core. Proprietary and confidential.                    */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "wcassert.h"

#include "gki.h"
#include "bt_types.h"
#include "hiddefs.h"
#include "hidd_api.h"
#include "hidd_int.h"
#include "btm_api.h"

#include "hcimsgs.h"
#include "btu.h"
#include "sdpdefs.h"
#include "sdp_api.h"

static const UINT8 HidDevLangList[]   = HID_DEV_LANGUAGELIST;

#ifndef HID_DEV_BOOT_DEVICE
#define HID_DEV_BOOT_DEVICE TRUE
#endif


/*******************************************************************************
**
** Function         HID_DevSetSDPRecord
**
** Description      This function should be called at startup to create the
**                  device SDP record
**
** Returns          0 if error else sdp handle for the record.
**
*******************************************************************************/
UINT32 HID_DevSetSDPRecord (tHID_DEV_SDP_INFO *p_sdp_info)
{
    UINT32                  sdp_handle;
    tSDP_PROTOCOL_ELEM      protocol_list[2];
    tSDP_PROTO_LIST_ELEM    additional_list;
    UINT16                  u16;
    UINT8                   u8;
    UINT8                  *pRepDescriptor, *pd;
    char buf[2];

    if( p_sdp_info == NULL )
        return (0);

    /* Create an SDP record for the ctrl/data or notification channel */
    if ((sdp_handle = SDP_CreateRecord()) == FALSE)
    {
        HIDD_TRACE_ERROR0 ("Could not create service record");
        return (0);
    }

    /* Add the UUID to the Service Class ID List */
    u16 = UUID_SERVCLASS_HUMAN_INTERFACE;
    SDP_AddServiceClassIdList(sdp_handle, 1, &u16);

    /* Build the protocol descriptor list */
    protocol_list[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
    protocol_list[0].num_params    = 1;
    protocol_list[0].params[0]     = HID_PSM_CONTROL;

    protocol_list[1].num_params    = 0;
    protocol_list[1].protocol_uuid = UUID_PROTOCOL_HIDP;

    SDP_AddProtocolList(sdp_handle, 2, protocol_list);

    /* Language base */
    SDP_AddLanguageBaseAttrIDList (sdp_handle, LANG_ID_CODE_ENGLISH, LANG_ID_CHAR_ENCODE_UTF8, 
        LANGUAGE_BASE_ID);

    /* Add the Bluetooth Profile Descriptor List (profile version number) */
    SDP_AddProfileDescriptorList(sdp_handle, UUID_SERVCLASS_HUMAN_INTERFACE, 0x0100);

    /* Add the PSM of the interrupt channel to the Additional Protocol Descriptor List */
    additional_list.num_elems = 2;
    additional_list.list_elem[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
    additional_list.list_elem[0].num_params    = 1;
    additional_list.list_elem[0].params[0]     = HID_PSM_INTERRUPT;
    additional_list.list_elem[1].protocol_uuid = UUID_PROTOCOL_HIDP;
    additional_list.list_elem[1].num_params    = 0;

    SDP_AddAdditionProtoLists (sdp_handle, 1, &additional_list);

    if( p_sdp_info->svc_name[0] != '\0' )
        SDP_AddAttribute (sdp_handle, ATTR_ID_SERVICE_NAME, TEXT_STR_DESC_TYPE, 
                      (UINT8)(strlen(p_sdp_info->svc_name)+1), (UINT8 *)p_sdp_info->svc_name);

    if( p_sdp_info->svc_descr[0] != '\0' )
        SDP_AddAttribute (sdp_handle, ATTR_ID_SERVICE_DESCRIPTION, TEXT_STR_DESC_TYPE, 
                      (UINT8)(strlen(p_sdp_info->svc_descr)+1), (UINT8 *)p_sdp_info->svc_descr);

    if( p_sdp_info->prov_name[0] != '\0' )
    SDP_AddAttribute (sdp_handle, ATTR_ID_PROVIDER_NAME, TEXT_STR_DESC_TYPE, 
                      (UINT8)(strlen(p_sdp_info->prov_name)+1), (UINT8 *)p_sdp_info->prov_name);

    /* HID parser version */
    UINT16_TO_BE_FIELD(buf,p_sdp_info->hpars_ver) ;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_PARSER_VERSION, UINT_DESC_TYPE, 2, (UINT8 *)buf);

    /* HID subclass */
    u8 = p_sdp_info->sub_class;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_DEVICE_SUBCLASS, UINT_DESC_TYPE, 1, &u8);

    /* HID country code */
    u8 = p_sdp_info->ctry_code;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_COUNTRY_CODE, UINT_DESC_TYPE, 1, &u8);

    /* HID Virtual Cable */
    u8 = HID_DEV_VIRTUAL_CABLE;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_VIRTUAL_CABLE, BOOLEAN_DESC_TYPE, 1, &u8);

    /* HID reconnect initiate */
    u8 = HID_DEV_RECONN_INITIATE;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_RECONNECT_INITIATE, BOOLEAN_DESC_TYPE, 1, &u8);

    /* HID report descriptor */
    if ( NULL != (pRepDescriptor = (UINT8 *)GKI_getbuf((UINT16)(p_sdp_info->dscp_info.dl_len + 8 ))) )
    {
        pd = pRepDescriptor;
        *pd++ = (UINT8)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
        *pd++ = (UINT8)(p_sdp_info->dscp_info.dl_len + 4);
        *pd++ = (UINT8)((UINT_DESC_TYPE << 3) | SIZE_ONE_BYTE);
        *pd++ = (UINT8)(HID_SDP_DESCRIPTOR_REPORT);
        *pd++ = (UINT8)((TEXT_STR_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
        *pd++ = (UINT8)(p_sdp_info->dscp_info.dl_len);
        memcpy (pd, p_sdp_info->dscp_info.dsc_list, p_sdp_info->dscp_info.dl_len);
        pd += p_sdp_info->dscp_info.dl_len;

        SDP_AddAttribute (sdp_handle, ATTR_ID_HID_DESCRIPTOR_LIST, DATA_ELE_SEQ_DESC_TYPE, 
                          (UINT32)(pd - pRepDescriptor), pRepDescriptor);
        GKI_freebuf( pRepDescriptor );
    }
    else
    {
        SDP_DeleteRecord( sdp_handle ); /* delete freshly allocated record */
        HIDD_TRACE_ERROR1( "HID_DevSetSDPRecord(): SDP creation failed: no memory for rep dscr len: %d",
                             p_sdp_info->dscp_info.dl_len );
        return 0;
    }
    /* HID language base list */
    SDP_AddAttribute (sdp_handle, ATTR_ID_HID_LANGUAGE_ID_BASE, DATA_ELE_SEQ_DESC_TYPE, 
                      sizeof (HidDevLangList), (UINT8 *)HidDevLangList);

    /* HID SDP disable (i.e. SDP while Control and Interrupt are up) */
#if (MAX_L2CAP_CHANNELS > 2)
    u8 = 0;
#else
    u8 = 1;
#endif
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_SDP_DISABLE, BOOLEAN_DESC_TYPE, 1, &u8);

#if defined(HID_DEV_BATTERY_POW)
    /* HID battery power */
    u8 = HID_DEV_BATTERY_POW;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_BATTERY_POWER, BOOLEAN_DESC_TYPE, 1, &u8);
#endif

#if defined(HID_DEV_REMOTE_WAKE)
    /* HID remote wakeup capable */
    u8 = HID_DEV_REMOTE_WAKE;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_REMOTE_WAKE, BOOLEAN_DESC_TYPE, 1, &u8);
#endif

    /* Link supervision timeout */
    UINT16_TO_BE_FIELD(buf,HID_DEV_LINK_SUPERVISION_TO) ;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_LINK_SUPERVISION_TO, UINT_DESC_TYPE, 2, (UINT8 *)buf);

    /* HID remote wakeup capable */
    u8 = HID_DEV_NORMALLY_CONN;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_NORMALLY_CONNECTABLE, BOOLEAN_DESC_TYPE, 1, &u8);

    /* HID BOOT Device */
    u8 = HID_DEV_BOOT_DEVICE;
    SDP_AddAttribute(sdp_handle, ATTR_ID_HID_BOOT_DEVICE, BOOLEAN_DESC_TYPE, 1, &u8);

    u16 = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    SDP_AddUuidSequence (sdp_handle,  ATTR_ID_BROWSE_GROUP_LIST, 1, &u16);

    /* SSR host max latency */
    if (p_sdp_info->ssr_max_latency != HID_SSR_PARAM_INVALID)
    {
        UINT16_TO_BE_FIELD(buf,HID_DEV_LINK_SUPERVISION_TO) ;
        SDP_AddAttribute(sdp_handle, ATTR_ID_HID_SSR_HOST_MAX_LAT, UINT_DESC_TYPE, 2, (UINT8 *)buf);
    }

    /* SSR host min timeout */
    if (p_sdp_info->ssr_max_latency != HID_SSR_PARAM_INVALID)
    {
        UINT16_TO_BE_FIELD(buf,HID_DEV_LINK_SUPERVISION_TO) ;
        SDP_AddAttribute(sdp_handle, ATTR_ID_HID_SSR_HOST_MIN_TOUT, UINT_DESC_TYPE, 2, (UINT8 *)buf);
    }
    return (sdp_handle);
}

/*******************************************************************************
**
** Function         HID_DevInit
**
** Description      This function initializes the control block and trace variable
**
** Returns          void
**
*******************************************************************************/
void HID_DevInit (void)
{
    memset(&hd_cb, 0, sizeof(tHIDDEV_CB));

    /* Initialize control channel L2CAP configuration */
    hd_cb.l2cap_ctrl_cfg.mtu_present = TRUE;
    hd_cb.l2cap_ctrl_cfg.mtu = HID_DEV_MTU_SIZE;

    /* Initialize interrupt channel L2CAP configuration */
    hd_cb.l2cap_int_cfg.mtu_present = TRUE;
    hd_cb.l2cap_int_cfg.mtu = HID_DEV_MTU_SIZE;

    hd_cb.conn.timer_entry.param = (UINT32) hidd_proc_repage_timeout;
#if defined(HID_INITIAL_TRACE_LEVEL)
    hd_cb.trace_level = HID_INITIAL_TRACE_LEVEL;
#else
    hd_cb.trace_level = BT_TRACE_LEVEL_NONE;
#endif
}

/*******************************************************************************
**
** Function         HID_DevRegister
**
** Description      This function must be called at startup so the device receive
**                  HID related events and call other HID API Calls.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevRegister( tHID_DEV_REG_INFO *p_reg_info )
{
    tHID_STATUS st;
    BD_ADDR     bt_bd_any = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    UINT16      conn_able;

    if( p_reg_info == NULL ||
        p_reg_info->app_cback == NULL )
        return HID_ERR_INVALID_PARAM;

    if( hd_cb.reg_flag )
        return HID_ERR_ALREADY_REGISTERED;

    /* Check if the host address is provided */
    if( memcmp( p_reg_info->host_addr, bt_bd_any, BD_ADDR_LEN ) )
    {
        hd_cb.host_known = TRUE;
        memcpy( hd_cb.host_addr, p_reg_info->host_addr, BD_ADDR_LEN );
        hd_cb.dev_state = HID_DEV_ST_NO_CONN ;

        /* When host address is provided, connectibility is determined by the
           SDP attribute, otherwise device has to be connectable for initial
           pairing process with the host */
        conn_able = (UINT16) HID_DEV_NORMALLY_CONN;
    }
    else 
    {
        hd_cb.host_known = FALSE;
        conn_able = BTM_CONNECTABLE;
    }
    
    hd_cb.virtual_cable = HID_DEV_VIRTUAL_CABLE;

    /* Copy QoS parameters if provided */
    if( p_reg_info->qos_info )
    {
        memcpy( &(hd_cb.qos_info), p_reg_info->qos_info, sizeof( tHID_DEV_QOS_INFO ) );
        hd_cb.use_qos_flg = TRUE;
    }
    else 
    {
        hd_cb.use_qos_flg = FALSE;
    }

    hd_cb.callback = p_reg_info->app_cback ;

    /* Register with L2CAP */
    if( (st = hidd_conn_reg()) != HID_SUCCESS )
    {
        return st;
    }

#if (!defined(HID_DEV_SET_CONN_MODE) || HID_DEV_SET_CONN_MODE == TRUE)
    if( BTM_SetConnectability (conn_able, HID_DEV_PAGE_SCAN_WIN, HID_DEV_PAGE_SCAN_INT) != BTM_SUCCESS )
        return HID_ERR_SET_CONNABLE_FAIL ;
#endif

    hd_cb.reg_flag = TRUE;
    hd_cb.unplug_on = FALSE;

    HIDD_TRACE_DEBUG0 ("HID_DevRegister successful");
    return HID_SUCCESS;
}

/*******************************************************************************
**
** Function         HID_DevDeregister
**
** Description      This function may be used to remove HID service records and
**                  deregister from L2CAP.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevDeregister( void )
{
    if( !hd_cb.reg_flag )
        return HID_ERR_NOT_REGISTERED;

    hidd_mgmt_process_evt( HID_API_DISCONNECT, NULL ) ; /* Disconnect first */
    /* Deregister with L2CAP */
    hidd_conn_dereg() ;
    hd_cb.reg_flag = FALSE;
    return HID_SUCCESS;
}

/*******************************************************************************
**
** Function         HID_DevConnect
**
** Description      This function may be used to initiate a connection to the host..
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevConnect( void )
{
    if( hd_cb.reg_flag == FALSE )
        return HID_ERR_NOT_REGISTERED;

    return hidd_mgmt_process_evt( HID_API_CONNECT, NULL ) ; /* This will initiate connection */
}

/*******************************************************************************
**
** Function         HID_DevDisconnect
**
** Description      This function may be used to disconnect from the host
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevDisconnect( void )
{
    if( hd_cb.reg_flag == FALSE )
        return HID_ERR_NOT_REGISTERED;

    return hidd_mgmt_process_evt( HID_API_DISCONNECT, NULL ) ; /* This will initiate disconnection */
}

/*******************************************************************************
**
** Function         HID_DevHandShake
**
** Description      This function may be used to send HAND-SHAKE to host
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevHandShake( UINT8 res_code )
{   
    tHID_SND_DATA_PARAMS hsk_data;

    if( hd_cb.reg_flag == FALSE )
        return HID_ERR_NOT_REGISTERED;

    hsk_data.trans_type = HID_TRANS_HANDSHAKE ;
    hsk_data.ctrl_ch = TRUE ;
    hsk_data.param = res_code;
    hsk_data.buf = NULL;

    return hidd_mgmt_process_evt( HID_API_SEND_DATA, &hsk_data ) ;
}

/*******************************************************************************
**
** Function         HID_DevVirtualUnplug
**
** Description      This function may be used to send VIRTUAL-UNPLUG to host
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevVirtualUnplug ( void )
{
    tHID_STATUS st;

    tHID_SND_DATA_PARAMS vup_data;

    if( hd_cb.reg_flag == FALSE )
        return HID_ERR_NOT_REGISTERED;

    vup_data.trans_type = HID_TRANS_CONTROL ;
    vup_data.ctrl_ch = TRUE ;
    vup_data.param = HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG;
    vup_data.buf = NULL;

    if( (st = hidd_mgmt_process_evt(HID_API_SEND_DATA, &vup_data)) == HID_SUCCESS )
    {
        hd_cb.unplug_on = TRUE;
    }

    return st;
}
    
/*******************************************************************************
**
** Function         HID_DevSendData
**
** Description      This function may be used to send input reports to host
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevSendData ( BOOLEAN control_ch, UINT8 rep_type, BT_HDR *data_buf )
{
    tHID_SND_DATA_PARAMS snd_data;

    WC_ASSERT(control_ch != TRUE && control_ch != FALSE);

    if( hd_cb.reg_flag == FALSE )
        return HID_ERR_NOT_REGISTERED;

    snd_data.trans_type = HID_TRANS_DATA ;
    snd_data.ctrl_ch = control_ch ;
    snd_data.param = rep_type;
    snd_data.buf = data_buf;

    return hidd_mgmt_process_evt( HID_API_SEND_DATA, &snd_data ) ;
}

/*******************************************************************************
**
** Function         HID_DevSetSecurityLevel
**
** Description      This function set security level for the Hid Device service.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevSetSecurityLevel( char serv_name[], UINT8 sec_lvl )
{
    hd_cb.sec_mask = sec_lvl;
    
    if (sec_lvl == 0)
    {
        if (!BTM_SetSecurityLevel (FALSE, serv_name, BTM_SEC_SERVICE_HID_NOSEC_CTRL, 
                                   BTM_SEC_NONE, HID_PSM_CONTROL, BTM_SEC_PROTO_HID , HIDD_NOSEC_CHN))
        {
            HIDD_TRACE_ERROR0 ("Security Registration 1 failed");
            return (HID_ERR_NO_RESOURCES);
        }
        if (!BTM_SetSecurityLevel (TRUE, serv_name, BTM_SEC_SERVICE_HID_NOSEC_CTRL, 
                                   BTM_SEC_NONE, HID_PSM_CONTROL, BTM_SEC_PROTO_HID, HIDD_NOSEC_CHN))
        {
            HIDD_TRACE_ERROR0 ("Security Registration 2 failed");
            return (HID_ERR_NO_RESOURCES);
        }
    }
    else
    {
        /* Register with Security Manager for the specific security level */
        if (!BTM_SetSecurityLevel (FALSE, serv_name, BTM_SEC_SERVICE_HID_SEC_CTRL, 
                                   sec_lvl, HID_PSM_CONTROL, BTM_SEC_PROTO_HID, HIDD_SEC_CHN))
        {
            HIDD_TRACE_ERROR0 ("Security Registration 3 failed");
            return (HID_ERR_NO_RESOURCES);
        }
        if (!BTM_SetSecurityLevel (TRUE, serv_name, BTM_SEC_SERVICE_HID_SEC_CTRL, 
                                   sec_lvl, HID_PSM_CONTROL, BTM_SEC_PROTO_HID, HIDD_SEC_CHN))
        {
            HIDD_TRACE_ERROR0 ("Security Registration 4 failed");
            return (HID_ERR_NO_RESOURCES);
        }
    }
    
    /* Register with Security Manager for the specific security level for interupt channel*/
    if (!BTM_SetSecurityLevel (TRUE, serv_name, BTM_SEC_SERVICE_HID_INTR, 
                               BTM_SEC_NONE, HID_PSM_INTERRUPT, BTM_SEC_PROTO_HID, 0))
    {
        HIDD_TRACE_ERROR0 ("Security Registration 5 failed");
        return (HID_ERR_NO_RESOURCES);
    }

    if (!BTM_SetSecurityLevel (FALSE, serv_name, BTM_SEC_SERVICE_HID_INTR, 
                               BTM_SEC_NONE, HID_PSM_INTERRUPT, BTM_SEC_PROTO_HID, 0))
    {
        HIDD_TRACE_ERROR0 ("Security Registration 6 failed");
        return (HID_ERR_NO_RESOURCES);
    }
    
    return HID_SUCCESS;
}

#if HID_DEV_PM_INCLUDED == TRUE
/*******************************************************************************
**
** Function         HID_DevSetPowerMgmtParams
**
** Description      This function may be used to change power mgmt parameters.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS HID_DevSetPowerMgmtParams( UINT8 conn_substate, tHID_DEV_PM_PWR_MD pm_params )
{
    if( conn_substate > HID_DEV_SUSP_CONN_ST )
        return (HID_ERR_INVALID_PARAM);

    memcpy( &hd_cb.pm_params[conn_substate], &pm_params, sizeof( tHID_DEV_PM_PWR_MD ) ) ;

    /* Set the power mode to new parameters if currently in that state */
    if( conn_substate == hd_cb.conn_substate )
        hidd_pm_set_power_mode ( &(hd_cb.pm_params[conn_substate]) );

    return (HID_SUCCESS);
}

#endif

