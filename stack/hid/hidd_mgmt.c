/*****************************************************************************/
/*                                                                           */
/*  Name:          hidd_mgmt.c                                               */
/*                                                                           */
/*  Description:   this file contains the HID Device Management logic        */
/*                                                                           */
/*  NOTE:          In the interest of keeping code small, there is an        */
/*                 inherent assumption that L2CAP always runs a timer,       */
/*                 and so the HID management never needs a timer on L2CAP    */
/*                 actions like connect and disconnect.                      */
/*                                                                           */
/*                                                                           */
/*  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.              */
/*  WIDCOMM Bluetooth Core. Proprietary and confidential.                    */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gki.h"
#include "bt_types.h"
#include "hidd_api.h"
#include "hiddefs.h"
#include "hidd_int.h"
#include "btu.h"
#include "btm_api.h"


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static tHID_STATUS hidd_no_conn_proc_evt( UINT8 event, void *p_data );
static tHID_STATUS hidd_connecting_proc_evt( UINT8 event, void *p_data );
static tHID_STATUS hidd_connected_proc_evt( UINT8 event, void *p_data );
static tHID_STATUS hidd_disc_ing_proc_evt( UINT8 event, void *p_data );


/* State machine jump table.
*/
tHIDD_MGMT_EVT_HDLR  * const hidd_sm_proc_evt[] = 
{
    hidd_no_conn_proc_evt,
    hidd_connecting_proc_evt,
    hidd_connected_proc_evt,
    hidd_disc_ing_proc_evt
};

/* Main HID control block */
#if HID_DYNAMIC_MEMORY == FALSE
tHIDDEV_CB   hd_cb;
#endif

/*******************************************************************************
**
** Function         hidd_mgmt_process_evt
**
** Description      This function is called by other modules to report an event
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS hidd_mgmt_process_evt( UINT8 event, void *p_data )
{
    HIDD_TRACE_DEBUG3("hidd_mgmt_process_evt known: %d, s:%d, e:%d", 
        hd_cb.host_known, hd_cb.dev_state, event);
    if( hd_cb.host_known )
    {
        return ((hidd_sm_proc_evt[hd_cb.dev_state]) (event, p_data)); /* Event is passed to main State Machine */
    }

    if( event == HOST_CONN_OPEN )
    {
        hd_cb.host_known = TRUE;
        memcpy( hd_cb.host_addr, (BD_ADDR *) p_data, BD_ADDR_LEN ) ;
        hd_cb.dev_state = HID_DEV_ST_CONNECTED ;
        /* Call-Back the Application with this information */
        hd_cb.callback(HID_DEV_EVT_OPEN, 0, (tHID_DEV_CBACK_DATA *) hd_cb.host_addr ) ;
        return( HID_SUCCESS );
    }

    return( HID_ERR_HOST_UNKNOWN );
}

/*******************************************************************************
**
** Function         hidd_no_conn_proc_evt
**
** Description      NO-CONN state event handler.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
static tHID_STATUS hidd_no_conn_proc_evt( UINT8 event, void *p_data )
{
    tHID_STATUS st = HID_SUCCESS;

    switch( event )
    {
    case HOST_CONN_OPEN:
        hd_cb.dev_state = HID_DEV_ST_CONNECTED ;
        hd_cb.callback(HID_DEV_EVT_OPEN, 0, (tHID_DEV_CBACK_DATA *) hd_cb.host_addr ) ;
        break;
    case HID_API_CONNECT:
        hd_cb.conn_tries = 1;
        hd_cb.dev_state = HID_DEV_ST_CONNECTING ;
        if( (st = hidd_conn_initiate()) != HID_SUCCESS ) 
        {
#if HID_DEV_MAX_CONN_RETRY > 0
            btu_start_timer (&(hd_cb.conn.timer_entry), BTU_TTYPE_USER_FUNC, HID_DEV_REPAGE_WIN);
            return HID_SUCCESS;
#else
            hd_cb.dev_state = HID_DEV_ST_NO_CONN ;
#endif
        }
        break;
    default:
        st = HID_ERR_NO_CONNECTION;
    }

    return st;
}

/*******************************************************************************
**
** Function         hidd_proc_repage_timeout
**
** Description      function to handle timeout.
**
** Returns          void
**
*******************************************************************************/
void hidd_proc_repage_timeout (TIMER_LIST_ENT *p_tle)
{
    HIDD_TRACE_DEBUG0 ("hidd_proc_repage_timeout");
    hd_cb.conn_tries++;
    if( hidd_conn_initiate() != HID_SUCCESS ) 
    {
        if( hd_cb.conn_tries > HID_DEV_MAX_CONN_RETRY )
        {
            hd_cb.dev_state = HID_DEV_ST_NO_CONN ;
            hd_cb.callback(HID_DEV_EVT_CLOSE, 0, NULL );
        }
        else
            btu_start_timer (&(hd_cb.conn.timer_entry), BTU_TTYPE_USER_FUNC, HID_DEV_REPAGE_WIN);
    }
    else
        hd_cb.callback( HID_DEV_EVT_RETRYING, hd_cb.conn_tries, NULL );
}

/*******************************************************************************
**
** Function         hidd_connecting_proc_evt
**
** Description      CONNECTING state event handler
**
** Returns          tHID_STATUS
**
*******************************************************************************/
static tHID_STATUS hidd_connecting_proc_evt( UINT8 event, void *p_data )
{
    switch( event )
    {
    case HOST_CONN_OPEN:
        hd_cb.dev_state = HID_DEV_ST_CONNECTED ;
        hd_cb.callback(HID_DEV_EVT_OPEN, 0, (tHID_DEV_CBACK_DATA *) hd_cb.host_addr ) ;
        break;

    case HOST_CONN_FAIL:
        if( hd_cb.conn_tries > HID_DEV_MAX_CONN_RETRY ) 
        {
            UINT16 reason = *( (UINT16 *) p_data);

            hd_cb.dev_state = HID_DEV_ST_NO_CONN;
            hd_cb.callback(HID_DEV_EVT_CLOSE, reason, NULL );
        }
#if HID_DEV_MAX_CONN_RETRY > 0
        else
        {
            btu_start_timer (&(hd_cb.conn.timer_entry), BTU_TTYPE_USER_FUNC, HID_DEV_REPAGE_WIN);
        }
#endif
        break;

    case HOST_CONN_CLOSE:
    case HOST_CONN_LOST:
        hd_cb.dev_state = HID_DEV_ST_NO_CONN;
        hd_cb.callback(HID_DEV_EVT_CLOSE, *((UINT16 *) p_data), NULL );
        break;

    case HID_API_DISCONNECT:
        hd_cb.dev_state = HID_DEV_ST_NO_CONN ;
        btu_stop_timer (&(hd_cb.conn.timer_entry));
        hidd_conn_disconnect();
        break;

    default:
        return( HID_ERR_CONN_IN_PROCESS ) ;
    }

    return (HID_SUCCESS);
}

/*******************************************************************************
**
** Function         hidd_mgmt_conn_closed
**
** Description      Called when l2cap channels have been released
**
** Returns          void
**
*******************************************************************************/
void hidd_mgmt_conn_closed( UINT16 reason )
{    
    if( hd_cb.unplug_on )
    {
        hd_cb.host_known = FALSE; /* This allows the device to accept connection from other hosts */
    }

    hd_cb.dev_state = HID_DEV_ST_NO_CONN;
    hd_cb.callback(HID_DEV_EVT_CLOSE, reason, NULL );
}

/*******************************************************************************
**
** Function         hidd_connected_proc_evt
**
** Description      CONNECTED state event handler
**
** Returns          tHID_STATUS
**
*******************************************************************************/
static tHID_STATUS hidd_connected_proc_evt( UINT8 event, void *p_data )
{
    switch( event )
    {
    case HOST_CONN_LOST:
#if HID_DEV_RECONN_INITIATE == TRUE
        hd_cb.dev_state = HID_DEV_ST_CONNECTING ;
        hd_cb.conn_tries = 0;
        btu_start_timer (&(hd_cb.conn.timer_entry), BTU_TTYPE_USER_FUNC, HID_DEV_REPAGE_WIN);
#else
        hidd_mgmt_conn_closed( *((UINT16 *) p_data) ) ;
#endif
        break;
    case HOST_CONN_CLOSE:
        hidd_mgmt_conn_closed( *((UINT16 *) p_data) ) ;
        break;
    case HID_API_DISCONNECT:
        hd_cb.dev_state = HID_DEV_ST_DISC_ING ;
        hidd_conn_disconnect();
        break;
    case HID_API_SEND_DATA: /*Send Input reports, handshake or virtual-unplug */
        return hidd_conn_snd_data( (tHID_SND_DATA_PARAMS *) p_data );
    default:
        return( HID_ERR_ALREADY_CONN ) ;
    }

    return( HID_SUCCESS );
}

/*******************************************************************************
**
** Function         hidd_disc_ing_proc_evt
**
** Description      DISCONNECTING state event handler
**
** Returns          tHID_STATUS
**
*******************************************************************************/
static tHID_STATUS hidd_disc_ing_proc_evt( UINT8 event, void *p_data )
{
    switch( event )
    {
    case HOST_CONN_LOST:
    case HOST_CONN_FAIL:
    case HOST_CONN_CLOSE:
        hidd_mgmt_conn_closed( *((UINT16 *) p_data) ) ;
        break;
    default:
        return( HID_ERR_DISCONNECTING ) ;
    }

    return( HID_SUCCESS );
}


