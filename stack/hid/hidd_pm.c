/*****************************************************************************/
/*                                                                           */
/*  Name:          hidd_pm.c                                                 */
/*                                                                           */
/*  Description:   this file contains the HID Device Power Management logic  */
/*                                                                           */
/*                                                                           */
/*  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.              */
/*  WIDCOMM Bluetooth Core. Proprietary and confidential.                    */
/*****************************************************************************/


#include "gki.h"
#include "bt_types.h"
#include "hiddefs.h"
#include "btm_api.h"
#include "string.h"

#include "hiddefs.h"

#include "hidd_api.h"
#include "hidd_int.h"
#include "btu.h"

#if HID_DEV_PM_INCLUDED == TRUE

/*******************************************************************************
**
** Function         hidd_pm_init
**
** Description      This function is called when connection is established. It
**                  initializes the control block for power management.
**
** Returns          void
**
*******************************************************************************/

static const tHID_DEV_PM_PWR_MD pwr_modes[] = 
{
    HID_DEV_BUSY_MODE_PARAMS,
    HID_DEV_IDLE_MODE_PARAMS,
    HID_DEV_SUSP_MODE_PARAMS
};

void hidd_pm_init( void )
{
    int i;

    hd_cb.curr_pm.mode = HCI_MODE_ACTIVE ;
    hd_cb.final_pm.mode = 0xff;

    for( i=0; i<3; i++ )
        memcpy( &hd_cb.pm_params[i], &pwr_modes[i], sizeof( tHID_DEV_PM_PWR_MD ) ) ;

    hd_cb.pm_ctrl_busy = FALSE;
}

/*******************************************************************************
**
** Function         hidd_pm_set_now
**
** Description      This drives the BTM for power management settings.
**
** Returns          TRUE if Success, FALSE otherwise
**
*******************************************************************************/
BOOLEAN hidd_pm_set_now( tHID_DEV_PM_PWR_MD *p_req_mode)
{
    tHID_DEV_PM_PWR_MD act_pm = { 0, 0, 0, 0, HCI_MODE_ACTIVE } ;
    UINT8 st = BTM_SUCCESS;

    /* Do nothing if already in required state or already performing a pm function */
    if( (hd_cb.pm_ctrl_busy) ||
        ((p_req_mode->mode == hd_cb.curr_pm.mode) && ( (p_req_mode->mode == HCI_MODE_ACTIVE) ||
            ((hd_cb.curr_pm.interval >= p_req_mode->min) && (hd_cb.curr_pm.interval <= p_req_mode->max)) )) )
    {
        hd_cb.final_pm.mode = 0xff;
        return TRUE;
    }

    switch( p_req_mode->mode )
    {
    case HCI_MODE_ACTIVE:
        if( hd_cb.curr_pm.mode == HCI_MODE_SNIFF )
        {
#if BTM_PWR_MGR_INCLUDED == TRUE
            st = BTM_SetPowerMode (BTM_PM_SET_ONLY_ID, hd_cb.host_addr, (tBTM_PM_PWR_MD *) &act_pm);
#else
            st = BTM_CancelSniffMode (hd_cb.host_addr);
#endif
            hd_cb.pm_ctrl_busy = TRUE;
        }
        else if( hd_cb.curr_pm.mode == HCI_MODE_PARK )
        {
#if BTM_PWR_MGR_INCLUDED == TRUE
            st = BTM_SetPowerMode (BTM_PM_SET_ONLY_ID, hd_cb.host_addr, (tBTM_PM_PWR_MD *) &act_pm);
#else
            st = BTM_CancelParkMode (hd_cb.host_addr);
#endif
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;

    case HCI_MODE_SNIFF:
        if( hd_cb.curr_pm.mode != HCI_MODE_ACTIVE ) /* Transition through active state required */
            hidd_pm_set_now (&act_pm);
        else
        {
#if BTM_PWR_MGR_INCLUDED == TRUE
            st = BTM_SetPowerMode (BTM_PM_SET_ONLY_ID, hd_cb.host_addr, (tBTM_PM_PWR_MD *) p_req_mode);
#else
            st = BTM_SetSniffMode (hd_cb.host_addr, p_req_mode->min, p_req_mode->max, p_req_mode->attempt, p_req_mode->timeout);
#endif
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;

    case HCI_MODE_PARK:
        if( hd_cb.curr_pm.mode != HCI_MODE_ACTIVE ) /* Transition through active state required */
            hidd_pm_set_now (&act_pm);
        else
        {
#if BTM_PWR_MGR_INCLUDED == TRUE
            st = BTM_SetPowerMode (BTM_PM_SET_ONLY_ID, hd_cb.host_addr, (tBTM_PM_PWR_MD *) p_req_mode);
#else
            st = BTM_SetParkMode (hd_cb.host_addr, p_req_mode->min, p_req_mode->max);
#endif
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;

    default:
        break;
    }

    if( st == BTM_SUCCESS || st == BTM_CMD_STARTED )
        return TRUE;
    else 
    {
        st += HCI_ERR_MAX_ERR ;
        if( hd_cb.callback )
            hd_cb.callback( HID_DEV_EVT_PM_FAILED, hd_cb.conn_substate, (tHID_DEV_CBACK_DATA *) &st ) ;
        return FALSE;
    }
}

/*******************************************************************************
**
** Function         hidd_pm_set_power_mode
**
** Description      This stores the power management setting and calls fn to set
**                  the power.
**
** Returns          TRUE if Success, FALSE otherwise
**
*******************************************************************************/
BOOLEAN hidd_pm_set_power_mode( tHID_DEV_PM_PWR_MD *p_req_mode)
{
    memcpy( &hd_cb.final_pm, p_req_mode, sizeof( tHID_DEV_PM_PWR_MD ) ) ;
    return hidd_pm_set_now( p_req_mode ) ;
}


/*******************************************************************************
**
** Function         hidd_pm_proc_mode_change
**
** Description      This is the callback function, when power mode changes.
**
** Returns          void
**
*******************************************************************************/
void hidd_pm_proc_mode_change( UINT8 hci_status, UINT8 mode, UINT16 interval )
{
    if (!hd_cb.reg_flag )
        return;

    hd_cb.pm_ctrl_busy = FALSE;

    if( hci_status != HCI_SUCCESS )
    {
        if( hd_cb.callback )
            hd_cb.callback( HID_DEV_EVT_PM_FAILED, hd_cb.conn_substate, (tHID_DEV_CBACK_DATA *) &hci_status ) ;
    }
    else
    {
        hd_cb.curr_pm.mode = mode;
        hd_cb.curr_pm.interval = interval;

        if( hd_cb.final_pm.mode != 0xff )
        {
            /* If we haven't reached the final power mode, set it now */
            if( (hd_cb.final_pm.mode != hd_cb.curr_pm.mode) || 
                ( (hd_cb.final_pm.mode != HCI_MODE_ACTIVE) && 
                  ((hd_cb.curr_pm.interval < hd_cb.final_pm.min) || (hd_cb.curr_pm.interval > hd_cb.final_pm.max))
                ) )
            {
                hidd_pm_set_now( &(hd_cb.final_pm) ) ;
            }
            else
                hd_cb.final_pm.mode = 0xff;
        }
        else
        {
            if( hd_cb.curr_pm.mode == HCI_MODE_ACTIVE )
                hidd_pm_start();
        }

        if( hd_cb.callback )
            hd_cb.callback( HID_DEV_EVT_MODE_CHG, mode, (tHID_DEV_CBACK_DATA *) &interval) ;
    }
}


/*******************************************************************************
**
** Function         hidd_pm_inact_timeout
**
** Description      Called when idle timer expires.
**
** Returns          void
**
*******************************************************************************/
void hidd_pm_inact_timeout (TIMER_LIST_ENT *p_tle)
{
    hidd_pm_set_power_mode ( &(hd_cb.pm_params[HID_DEV_IDLE_CONN_ST]));
    hd_cb.conn_substate = HID_DEV_IDLE_CONN_ST;
}

/*******************************************************************************
**
** Function         hidd_pm_start
**
** Description      Starts the power management function in a given state.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS hidd_pm_start( void )
{
    hidd_pm_set_power_mode ( &(hd_cb.pm_params[HID_DEV_BUSY_CONN_ST]) );

    hd_cb.conn_substate = HID_DEV_BUSY_CONN_ST;

    hd_cb.idle_tle.param = (UINT32) hidd_pm_inact_timeout;
    btu_start_timer (&hd_cb.idle_tle, BTU_TTYPE_USER_FUNC, HID_DEV_INACT_TIMEOUT);

    return( HID_SUCCESS );
}

/*******************************************************************************
**
** Function         hidd_pm_stop
**
** Description      Stops the idle timer.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS hidd_pm_stop( void )
{
    tHID_DEV_PM_PWR_MD p_md = { 0, 0, 0, 0, HCI_MODE_ACTIVE };

    hidd_pm_set_power_mode( &p_md );
    btu_stop_timer( &hd_cb.idle_tle ) ;
    return( HID_SUCCESS );
}


/*******************************************************************************
**
** Function         hidd_pm_suspend_evt
**
** Description      Called when host suspends the device.
**
** Returns          tHID_STATUS
**
*******************************************************************************/
tHID_STATUS hidd_pm_suspend_evt( void )
{
    if( hd_cb.conn_substate == HID_DEV_BUSY_CONN_ST )
        btu_stop_timer( &hd_cb.idle_tle ) ;

    hidd_pm_set_power_mode ( &(hd_cb.pm_params[HID_DEV_SUSP_CONN_ST]) );
    hd_cb.conn_substate = HID_DEV_SUSP_CONN_ST;
    return( HID_SUCCESS );
}
#endif /* HID_DEV_PM_INCLUDED == TRUE */
