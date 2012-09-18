/*****************************************************************************
**
**  Name:           bta_hl_ci.c
**
**  Description:    This is the implementation file for the HeaLth device profile
**                  (HL) subsystem call-in functions.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bta_api.h"
#include "btm_api.h"
#include "bta_sys.h"
#include "bta_hl_api.h"
#include "bta_hl_co.h"
#include "bta_hl_int.h"

/*******************************************************************************
**
** Function         bta_hl_ci_get_tx_data
**
** Description      This function is called in response to the
**                  bta_hl_co_get_tx_data call-out function.
**
** Parameters       mdl_handle -MDL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_hl_ci_get_tx_data(  tBTA_HL_MDL_HANDLE mdl_handle,
                                            tBTA_HL_STATUS status,
                                            UINT16 evt )
{
    tBTA_HL_CI_GET_PUT_DATA  *p_evt;

#if  (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG3("bta_hl_ci_get_tx_data mdl_handle=%d status=%d evt=%d\n",
                      mdl_handle, status, evt);
#endif

    if ((p_evt = (tBTA_HL_CI_GET_PUT_DATA *)GKI_getbuf(sizeof(tBTA_HL_CI_GET_PUT_DATA))) != NULL)
    {
        p_evt->hdr.event = evt;
        p_evt->mdl_handle =  mdl_handle;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_hl_ci_put_rx_data
**
** Description      This function is called in response to the
**                  bta_hl_co_put_rx_data call-out function.
**
** Parameters       mdl_handle -MDL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_hl_ci_put_rx_data(  tBTA_HL_MDL_HANDLE mdl_handle,
                                            tBTA_HL_STATUS status,
                                            UINT16 evt )
{
    tBTA_HL_CI_GET_PUT_DATA  *p_evt;
#if  (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG3("bta_hl_ci_put_rx_data mdl_handle=%d status=%d evt=%d\n",
                      mdl_handle, status, evt);
#endif

    if ((p_evt = (tBTA_HL_CI_GET_PUT_DATA *)GKI_getbuf(sizeof(tBTA_HL_CI_GET_PUT_DATA))) != NULL)
    {
        p_evt->hdr.event = evt;
        p_evt->mdl_handle =  mdl_handle;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}


/*******************************************************************************
**
** Function         bta_hl_ci_get_echo_data
**
** Description      This function is called in response to the
**                  bta_hl_co_get_echo_data call-out function.
**
** Parameters       mcl_handle -MCL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_hl_ci_get_echo_data(  tBTA_HL_MCL_HANDLE mcl_handle,
                                              tBTA_HL_STATUS status,
                                              UINT16 evt )
{
    tBTA_HL_CI_ECHO_DATA  *p_evt;

#if  (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG3("bta_hl_ci_get_echo_data mcl_handle=%d status=%d evt=%d\n",
                      mcl_handle, status, evt);
#endif

    if ((p_evt = (tBTA_HL_CI_ECHO_DATA *)GKI_getbuf(sizeof(tBTA_HL_CI_ECHO_DATA))) != NULL)
    {
        p_evt->hdr.event = evt;
        p_evt->mcl_handle =  mcl_handle;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_hl_ci_put_echo_data
**
** Description      This function is called in response to the
**                  bta_hl_co_put_echo_data call-out function.
**
** Parameters       mcl_handle -MCL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_hl_ci_put_echo_data(  tBTA_HL_MCL_HANDLE mcl_handle,
                                              tBTA_HL_STATUS status,
                                              UINT16 evt )
{
    tBTA_HL_CI_ECHO_DATA  *p_evt;

#if  (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG3("bta_hl_ci_put_echo_data mcl_handle=%d status=%d evt=%d\n",
                      mcl_handle, status, evt);
#endif

    if ((p_evt = (tBTA_HL_CI_ECHO_DATA *)GKI_getbuf(sizeof(tBTA_HL_CI_ECHO_DATA))) != NULL)
    {
        p_evt->hdr.event = evt;
        p_evt->mcl_handle =  mcl_handle;
        p_evt->status = status;
        bta_sys_sendmsg(p_evt);
    }
}

