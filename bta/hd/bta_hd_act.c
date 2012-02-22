/******************************************************************************
**
**  File Name:   bta_hd_act.c
**
**  Description: This module contains state machine action functions for
**               the HID Device service.
**
**  Copyright (c) 2002-2009, Broadcom Corp., All Rights Reserved.              
**  WIDCOMM Bluetooth Core. Proprietary and confidential.                    
**
******************************************************************************/

#include <string.h>
#include "data_types.h"
#include "bt_types.h"
#include "bt_target.h"
#include "hidd_api.h"
#include "bta_hd_int.h"
#include "bta_hd_api.h"
#include "l2c_api.h"
#include "btm_api.h"
#include "bd.h"



const UINT8 bta_hd_buf_len[] =
{
    BTA_HD_KBD_REPT_SIZE,   /* BTA_HD_REPT_ID_SPEC       0: other */
    BTA_HD_KBD_REPT_SIZE,   /* BTA_HD_REPT_ID_KBD        1: regular keyboard */
    BTA_HD_MOUSE_REPT_SIZE  /* BTA_HD_REPT_ID_MOUSE      2: mouse */
};

typedef struct
{
    UINT8  modifier;
    UINT8  reserved;
    UINT8  keycode_1;
    UINT8  keycode_2;
    UINT8  keycode_3;
    UINT8  keycode_4;
    UINT8  keycode_5;
    UINT8  keycode_6;
} tBTA_HD_KEY_REPORT;


#if (BT_USE_TRACES == TRUE)
const char * bta_hd_ctrl_str [] =
{
    "NOP",
    "HARD_RESET",
    "SOFT_RESET",
    "SUSPEND",
    "EXIT_SUSPEND",
    "VCAB_UNPLUG"
};

const char * bta_hd_rept_id_str [] =
{
    "MASK",
    "OTHER",
    "INPUT",
    "OUTPUT",
    "FEATURE"
};

const char * bta_hd_evt_str [] =
{
    "OPEN",
    "CLOSE",
    "RETRYING",
    "MODE_CHG",
    "PM_FAILED",
    "CONTROL",
    "GET_REPORT",
    "SET_REPORT",
    "GET_PROTO",
    "SET_PROTO",
    "GET_IDLE",
    "SET_IDLE",
    "DATA",
    "DATC",
    "L2C_CONG",
};
#endif

/*******************************************************************************
**
** Function     bta_hd_kick_timer
**
** Description  kick the timer in DM for power management
**                  
** Returns      void
**
*******************************************************************************/
static void bta_hd_kick_timer(tBTA_HD_CB *p_cb)
{
    APPL_TRACE_DEBUG0("bta_hd_kick_timer");
#if (BTM_SSR_INCLUDED == TRUE)
    if(!p_cb->use_ssr)
#endif
    {
        bta_sys_busy(BTA_ID_HD, p_cb->app_id, p_cb->peer_addr);
        bta_sys_idle(BTA_ID_HD, p_cb->app_id, p_cb->peer_addr);
    }
}
/*******************************************************************************
**
** Function         bta_hd_ssr_timer_cback
**
** Description      Sends disable event to application
**                  
**
** Returns          void                  
**
*******************************************************************************/
#if (BTM_SSR_INCLUDED == TRUE)
static void bta_hd_ssr_timer_cback (TIMER_LIST_ENT *p_tle)
{
    tBTA_HD_CB *p_cb = (tBTA_HD_CB *)p_tle->param;
    p_cb->use_ssr = BTA_DmUseSsr(p_cb->peer_addr);
    APPL_TRACE_DEBUG1("bta_hd_ssr_timer_cback use_ssr:%d", p_cb->use_ssr);
    bta_sys_idle(BTA_ID_HD, p_cb->app_id, p_cb->peer_addr);
}
#endif

/*******************************************************************************
**
** Function     bta_hd_send_data
**
** Description  Send data to the connected host
**              
**                  
** Returns      void
**
*******************************************************************************/
static void bta_hd_send_data(tBTA_HD_CB *p_cb, UINT8 rep_type, UINT16 len, UINT8 *p_data)
{
    BT_HDR  *p_buf;
    UINT8   *p;

    p_buf = (BT_HDR *) GKI_getbuf( (UINT16)(L2CAP_MIN_OFFSET+len + 1) );
    if(p_buf)
    {
        p = (UINT8 *)(p_buf + 1) + L2CAP_MIN_OFFSET+1;
        memcpy( p, p_data, len);
        p_buf->len = len; 
        p_buf->offset = L2CAP_MIN_OFFSET+1;
        HID_DevSendData ( TRUE, rep_type, p_buf );
        if(p_cb)
        {
            bta_hd_kick_timer(p_cb);
        }
    }
}

/*******************************************************************************
**
** Function     bta_hd_flush_data
**
** Description  Send the queued data to the connected host
**              
**                  
** Returns      BOOLEAN
**
*******************************************************************************/
static BOOLEAN   bta_hd_flush_data(tBTA_HD_CB *p_cb)
{
    BT_HDR  *p_buf;
    BOOLEAN set_busy = FALSE;

    while((p_buf = (BT_HDR*)GKI_dequeue (&p_cb->out_q)) != NULL)
    {
        if(HID_DevSendData ( FALSE, HID_PAR_REP_TYPE_INPUT, p_buf ) == HID_ERR_CONGESTED)
        {
            /* can not send it now. put it back to the queue */
            GKI_enqueue_head(&p_cb->out_q, p_buf);
            break;
        }

        /* sent one packet */
        set_busy = TRUE;
    }

    if(set_busy)
    {
        bta_hd_kick_timer(p_cb);
    }
    return GKI_queue_is_empty(&p_cb->out_q);
}

/*******************************************************************************
**
** Function     bta_hd_init_con_act
**
** Description  Initialize the connection to the known host.
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_init_con_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    APPL_TRACE_DEBUG0("bta_hd_init_con_act");
    if( HID_DevConnect() != HID_SUCCESS )
    {
        bta_hd_sm_execute(p_cb, BTA_HD_DISCONNECTED_EVT, NULL);
    }
}

/*******************************************************************************
**
** Function     bta_hd_close_act
**
** Description  process close API function.
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_close_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    APPL_TRACE_DEBUG0("bta_hd_close_act");
    HID_DevDisconnect();
}

/*******************************************************************************
**
** Function     bta_hd_disable_act
**
** Description  process disable API function.
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_disable_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    APPL_TRACE_DEBUG0("bta_hd_disable_act");
    BTM_SecClrService(BTM_SEC_SERVICE_HID_SEC_CTRL);
    BTM_SecClrService(BTM_SEC_SERVICE_HID_INTR);
    if(bta_hd_cb.sdp_handle)
    {
        SDP_DeleteRecord(bta_hd_cb.sdp_handle);
        bta_hd_cb.sdp_handle = 0;
#if ( BTM_EIR_SERVER_INCLUDED == TRUE )&&(BTA_EIR_CANNED_UUID_LIST != TRUE)
        bta_sys_remove_uuid(UUID_SERVCLASS_HUMAN_INTERFACE);
#endif
    }
    HID_DevDeregister();
}

/*******************************************************************************
**
** Function     bta_hd_open_act
**
** Description  received connected event from HIDD. report to user
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_open_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    BD_ADDR_PTR p_addr = p_data->cback_data.pdata->host_bdaddr;
#if (BTM_SSR_INCLUDED == TRUE)
    UINT8 *p;
#endif

    p_cb->proto = HID_PAR_PROTOCOL_REPORT;  /* the default is report mode */
    bdcpy(p_cb->peer_addr, p_addr);
    /* inform role manager */
    bta_sys_conn_open( BTA_ID_HD ,p_cb->app_id, p_addr);
#if (BTM_SSR_INCLUDED == TRUE)
    p_cb->timer.param = (UINT32)p_cb;
    p_cb->timer.p_cback = (TIMER_CBACK*)&bta_hd_ssr_timer_cback;
    bta_sys_start_timer(&p_cb->timer, 0, 6000);
    p_cb->use_ssr = FALSE;
    if( ((NULL != (p = BTM_ReadLocalFeatures ())) && HCI_SNIFF_SUB_RATE_SUPPORTED(p)) &&
        ((NULL != (p = BTM_ReadRemoteFeatures (p_addr))) && HCI_SNIFF_SUB_RATE_SUPPORTED(p)) )
    {
        /* both local and remote devices support SSR */
        p_cb->use_ssr = TRUE;
    }
#endif
    p_cb->p_cback(BTA_HD_OPEN_EVT, (tBTA_HD *)p_addr);
}

/*******************************************************************************
**
** Function     bta_hd_opn_cb_act
**
** Description  process events from HIDD in open state.
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_opn_cb_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    UINT8   res_code;
    BOOLEAN *p_cong;
    tHID_DEV_DSCP_INFO  *p_info;
    tBTA_HD_CBACK_DATA  *p_evt = (tBTA_HD_CBACK_DATA *)p_data;
    BOOLEAN set_busy = TRUE;
    tBTA_HD_REPORT   rpt;
    UINT8            evt;
    BT_HDR          *p_buf;

    switch(p_evt->event)
    {
    case HID_DEV_EVT_CONTROL: /*    Host sent HID_CONTROL       Data=Control Operation */
        if(p_evt->data == HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG)
        {
            p_cb->p_cback(BTA_HD_UNPLUG_EVT, NULL);
            set_busy = FALSE;
        }
        else if(p_evt->data == HID_PAR_CONTROL_SUSPEND)
        {
            bta_sys_idle(BTA_ID_HD, p_cb->app_id, p_cb->peer_addr);
            set_busy = FALSE;
        }
        break;

    case HID_DEV_EVT_GET_REPORT:/*Host sent GET_REPORT          Data=Length pdata=structure 
                             having details of get-report.*/
        /* bta_hd_hidd_cback sends SM event only when 
        ((tHID_DEV_GET_REP_DATA *)p_data))->rep_type == HID_PAR_REP_TYPE_FEATURE */
        p_info = &p_bta_hd_cfg->sdp_info.dscp_info;
        bta_hd_send_data(p_cb, HID_PAR_REP_TYPE_FEATURE, p_info->dl_len, p_info->dsc_list);
        break;

    case HID_DEV_EVT_GET_PROTO:/*Host sent GET_PROTOCOL         Data=NA*/
        res_code = p_cb->proto;     /* the protocol (Report or Boot) mode from control block */
        bta_hd_send_data(p_cb, HID_PAR_REP_TYPE_OTHER, 1, &res_code);
        break;

    case HID_DEV_EVT_SET_PROTO:/*Host sent SET_PROTOCOL         Data=1 for Report, 0 for Boot*/
        if((p_evt->data == HID_PAR_PROTOCOL_REPORT) || (p_evt->data == HID_PAR_PROTOCOL_BOOT_MODE))
        {
            p_cb->proto = p_evt->data;
            res_code = HID_PAR_HANDSHAKE_RSP_SUCCESS;
        }
        else
            res_code = HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ;
        HID_DevHandShake(res_code);
        break;

    case HID_DEV_EVT_L2CAP_CONGEST:
        p_cong = (BOOLEAN *)p_evt->pdata;
        if(*p_cong == FALSE)
        {
            /* L2CAP is uncongested, send out the queued buffers */
            bta_hd_flush_data(p_cb);
            set_busy = FALSE;
        }
        break;

    case HID_DEV_EVT_DATA:
    case HID_DEV_EVT_DATC:
        evt = (p_evt->event == HID_DEV_EVT_DATA) ? BTA_HD_DATA_EVT: BTA_HD_DATC_EVT;
        if(p_evt->pdata)
        {
            if(p_evt->data == HID_PAR_REP_TYPE_OUTPUT)
            {
                p_buf = p_evt->pdata->buffer;

                rpt.p_data = (UINT8 *)(p_buf + 1) + p_buf->offset + 1;
                rpt.len = p_buf->len - 1;

                p_cb->p_cback(evt, (tBTA_HD *)&rpt);
            }

            GKI_freebuf (p_evt->pdata->buffer);
        }
        break;
    }

    if(set_busy)
    {
        bta_hd_kick_timer(p_cb);
    }
}

/*******************************************************************************
**
** Function     bta_hd_input_act
**
** Description  received input event from user. send to peer
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_input_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    tBTA_HD_API_INPUT_SPEC  *p_spec = (tBTA_HD_API_INPUT_SPEC *)p_data;
    tBTA_HD_API_INPUT   *p_input = (tBTA_HD_API_INPUT *)p_data;
    tBTA_HD_KEY_REPORT  key_buf = {0};
    UINT16              len = bta_hd_buf_len[p_input->rid];
    UINT16              size;
    BT_HDR *p_buf;
    UINT8  *p;
    int     temp;
    BOOLEAN release = FALSE;

    if(p_input->rid == BTA_HD_REPT_ID_SPEC)
        len = BTA_HD_KBD_REPT_SIZE;
    size = L2CAP_MIN_OFFSET + sizeof(BT_HDR) + len;

    if ( (p_buf = (BT_HDR *)GKI_getbuf(size)) == NULL )
        return;

    p_buf->offset = L2CAP_MIN_OFFSET+1;
    p = (UINT8 *) ((UINT8 *) (p_buf+1))+L2CAP_MIN_OFFSET+1;
    *p++ = p_input->rid;
    switch(p_input->rid)
    {
    case BTA_HD_REPT_ID_SPEC:
        p--;
        memcpy(p, p_spec->seq, p_spec->len);
        /* use the proper report ID type for the key release */
        p_input->rid = *p;
        temp = BTA_HD_KBD_REPT_SIZE - p_spec->len;
        if(temp > 0)
            memset(p + p_spec->len, 0, temp);
        release = p_spec->release;
        break;

    case BTA_HD_REPT_ID_KBD:    /* 1: regular keyboard */
        key_buf.keycode_1   = p_input->keyCode;
        key_buf.modifier    = p_input->buttons;
        memcpy(p, (void *)&key_buf, len-1);
        release = p_input->release;
        break;

    case BTA_HD_REPT_ID_MOUSE:      /* 2: mouse */
        *p++ = p_input->buttons;
        *p++ = p_input->keyCode;    /* x */
        *p++ = p_input->keyCode2;   /* y */
        *p++ = p_input->wheel;
        break;

    default:
        GKI_freebuf(p_buf);
        return;
    }
    p_buf->len = len;

    GKI_enqueue(&p_cb->out_q, p_buf);

    if(release)
    {
        /* key release event */
        if ( (p_buf = (BT_HDR *)GKI_getbuf(size)) == NULL )
            return;

        p_buf->offset = L2CAP_MIN_OFFSET+1;
        p_buf->len = len;
        p = (UINT8 *) ((UINT8 *) (p_buf+1))+L2CAP_MIN_OFFSET+1;
        *p++ = p_input->rid;
        memset(p, 0, p_buf->len-1);
        GKI_enqueue(&p_cb->out_q, p_buf);
    }
    bta_hd_flush_data(p_cb);
}

/*******************************************************************************
**
** Function     bta_hd_discntd_act
**
** Description  received disconnected event from HIDD. report to user
**              The connection was not open yet at this action
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_discntd_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
#if (BTM_SSR_INCLUDED == TRUE)
    bta_sys_stop_timer(&p_cb->timer);
#endif

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_HD ,p_cb->app_id, p_cb->peer_addr);
    p_cb->p_cback(BTA_HD_CLOSE_EVT, (tBTA_HD *)p_cb->peer_addr);
}

/*******************************************************************************
**
** Function     bta_hd_discnt_act
**
** Description  received disconnected event from HIDD. report to user
**              
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_discnt_act(tBTA_HD_CB *p_cb, tBTA_HD_DATA *p_data)
{
    BT_HDR  *p_buf;
 
#if (BTM_SSR_INCLUDED == TRUE)
    bta_sys_stop_timer(&p_cb->timer);
#endif

    /* clean out queue */
    while((p_buf = (BT_HDR*)GKI_dequeue (&p_cb->out_q)) != NULL)
         GKI_freebuf(p_buf);

    /* inform role manager */
    bta_sys_conn_close( BTA_ID_HD ,p_cb->app_id, p_cb->peer_addr);
    p_cb->p_cback(BTA_HD_CLOSE_EVT, (tBTA_HD *)p_cb->peer_addr);
}

/*******************************************************************************
**
** Function     bta_hd_hidd_cback
**
** Description  This is the HIDD callback function to process events from HIDD.
**              just post a message to BTA_HD
**                  
** Returns      void
**
*******************************************************************************/
void bta_hd_hidd_cback(UINT8 event, UINT32 data, tHID_DEV_CBACK_DATA *pdata)
{
    tBTA_HD_CBACK_DATA  *p_buf;
    tHID_DEV_GET_REP_DATA *pget_rep;
    UINT16  sm_evt = BTA_HD_INVALID_EVT;
    UINT8 res_code;

#if (BT_USE_TRACES == TRUE)
    APPL_TRACE_EVENT3("HID event=0x%x(%s), data: %d", event, bta_hd_evt_str[event], data);
#endif

    switch( event )
    {
    case HID_DEV_EVT_OPEN: /*Connected to host                  Data = 1 if Virtual Cable
                            with Interrupt and Control
                             Channels in OPEN state.            pdata = Host BD-Addr.*/
        sm_evt      = BTA_HD_CONNECTED_EVT;
        break;

    case HID_DEV_EVT_CLOSE: /*Connection with host is closed.   Data=Reason Code. */
        sm_evt      = BTA_HD_DISCONNECTED_EVT;
        break;

    /*case HID_DEV_EVT_RETRYING:                                Data=Retrial number
        Lost connection is being re-connected.
        MSKB_TRACE_0("HID_DEV_EVT_RETRYING");
        break; */

    /*case HID_DEV_EVT_MODE_CHG:    Device changed power mode.  Data=new power mode
        APPL_TRACE_DEBUG1("Mode change - %d", data);
        break; */

    /*case HID_DEV_EVT_PM_FAILED:     Host sent SET_IDLE            Data=Idle Rate
        MSKB_TRACE_0("PM - Failed");
        break; */

    case HID_DEV_EVT_CONTROL: /*    Host sent HID_CONTROL       Data=Control Operation */
#if (BT_USE_TRACES == TRUE)
        APPL_TRACE_DEBUG2("EVT_CONTROL:%d(%s)",data, bta_hd_ctrl_str[data]);
#endif
        switch(data)
        {
        case HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG:
        case HID_PAR_CONTROL_SUSPEND:
        case HID_PAR_CONTROL_EXIT_SUSPEND:
            sm_evt = BTA_HD_CBACK_EVT ;
            HID_DevHandShake(HID_PAR_HANDSHAKE_RSP_SUCCESS);
            break;
        default:
            HID_DevHandShake(HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
        }
        break;

    case HID_DEV_EVT_GET_REPORT:/*Host sent GET_REPORT            Data=Length pdata=structure 
                             having details of get-report.*/
        pget_rep = (tHID_DEV_GET_REP_DATA *) pdata;
        APPL_TRACE_DEBUG3("RepType:%d(%s), id:%d",pget_rep->rep_type,
            bta_hd_rept_id_str[pget_rep->rep_type], pget_rep->rep_id);

        if(pget_rep->rep_type == HID_PAR_REP_TYPE_FEATURE)
        {
            sm_evt = BTA_HD_CBACK_EVT ;
        }
        else
        {
            HID_DevHandShake(HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
        }
        break;
    case HID_DEV_EVT_SET_REPORT:/*Host sent SET_REPORT            Data=Length pdata=details.*/
        HID_DevHandShake(HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
        APPL_TRACE_EVENT0("HID_DEV_EVT_SET_REPORT");
        GKI_freebuf (pdata->buffer);
        break;
    case HID_DEV_EVT_GET_PROTO:/*Host sent GET_PROTOCOL            Data=NA*/
        sm_evt = BTA_HD_CBACK_EVT ;
        break;
    case HID_DEV_EVT_SET_PROTO:/*Host sent SET_PROTOCOL            Data=1 for Report, 0 for Boot*/
        sm_evt = BTA_HD_CBACK_EVT ;
        break;
    case HID_DEV_EVT_GET_IDLE:/*    Host sent GET_IDLE            Data=NA */
        /* only support infinite idle rate (= 0) */
        res_code = 0;
        bta_hd_send_data(NULL, HID_PAR_REP_TYPE_OTHER, 1, &res_code);
        break;
    case HID_DEV_EVT_SET_IDLE: /*    Host sent SET_IDLE            Data=Idle Rate */
        res_code = HID_PAR_HANDSHAKE_RSP_ERR_INVALID_PARAM;
        if( data == 0 ) /* only support infinite idle rate (= 0) */
            res_code = HID_PAR_HANDSHAKE_RSP_SUCCESS;
        HID_DevHandShake(res_code);
        break;
    case HID_DEV_EVT_L2CAP_CONGEST:
        sm_evt = BTA_HD_CBACK_EVT ;
        break;
    case HID_DEV_EVT_DATA:
    case HID_DEV_EVT_DATC:
        APPL_TRACE_EVENT1("HID_DEV_EVT_DATC/DATC event = %s", bta_hd_evt_str[event]);
        sm_evt = BTA_HD_CBACK_EVT ;
        break;
    }

    if ((sm_evt != BTA_HD_INVALID_EVT) &&
        (p_buf = (tBTA_HD_CBACK_DATA *) GKI_getbuf(
                (UINT16)(sizeof(tBTA_HD_CBACK_DATA)+sizeof(tHID_DEV_CBACK_DATA)))) != NULL)
    {
        p_buf->hdr.event    = sm_evt;
        p_buf->event        = event;
        p_buf->data         = data;
        if(pdata)
        {
            p_buf->pdata    = (tHID_DEV_CBACK_DATA *)(p_buf + 1);
            memcpy(p_buf->pdata, pdata, sizeof(tHID_DEV_CBACK_DATA));
        }
        else
        {
            p_buf->pdata    = NULL;
        }
        bta_sys_sendmsg(p_buf);
    }


}

