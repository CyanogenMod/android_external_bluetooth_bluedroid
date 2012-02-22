/*****************************************************************************
**
**  Name:           bta_dg_api.c
**
**  Description:    This is the implementation of the API for the data gateway
**                  (DG) subsystem of BTA, Widcomm's Bluetooth application
**                  layer for mobile phones.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
    
#if defined(BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)

#include <string.h>
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_dg_api.h"
#include "bta_dg_int.h"
#include "gki.h"
#include "bd.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_dg_reg =
{
    bta_dg_hdl_event,
    BTA_DgDisable
};

/*******************************************************************************
**
** Function         BTA_DgEnable
**
** Description      Enable the data gateway service.  This function must be
**                  called before any other functions in the DG API are called.
**                  When the enable operation is complete the callback function
**                  will be called with a BTA_DG_ENABLE_EVT.  After the DG
**                  service is enabled a server can be started by calling
**                  BTA_DgListen().
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgEnable(tBTA_DG_CBACK *p_cback)
{
    tBTA_DG_API_ENABLE  *p_buf;

    /* register with BTA system manager */
    GKI_sched_lock();
    bta_sys_register(BTA_ID_DG, &bta_dg_reg);
    GKI_sched_unlock();

    APPL_TRACE_API0( "BTA_DgEnable");
    if ((p_buf = (tBTA_DG_API_ENABLE *) GKI_getbuf(sizeof(tBTA_DG_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_DgDisable
**
** Description      Disable the data gateway service.  Before calling this
**                  function all DG servers must be shut down by calling
**                  BTA_DgShutdown().
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgDisable(void)
{
    BT_HDR  *p_buf;

    APPL_TRACE_API0( "BTA_DgDisable");
    bta_sys_deregister(BTA_ID_DG);
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_DG_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_DgListen
**
** Description      Create a DG server for DUN, FAX or SPP.  After creating a
**                  server peer devices can open an RFCOMM connection to the
**                  server.  When the listen operation has started the callback
**                  function will be called with a BTA_DG_LISTEN_EVT providing
**                  the handle associated with this server.  The handle
**                  identifies server when calling other DG functions such as
**                  BTA_DgClose() or BTA_DgShutdown().
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgListen(tBTA_SERVICE_ID service, tBTA_SEC sec_mask,
                  char *p_service_name, UINT8 app_id)
{
    tBTA_DG_API_LISTEN  *p_buf;

    APPL_TRACE_API1( "BTA_DgListen %d", service);
    if ((p_buf = (tBTA_DG_API_LISTEN *) GKI_getbuf(sizeof(tBTA_DG_API_LISTEN))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_API_LISTEN_EVT;
        BCM_STRNCPY_S(p_buf->name, sizeof(p_buf->name), p_service_name, BTA_SERVICE_NAME_LEN);
        p_buf->sec_mask = sec_mask;
        p_buf->service = service;
        p_buf->app_id = app_id;
        bta_sys_sendmsg(p_buf);
    }
}
/*******************************************************************************
**
** Function         BTA_DgOpen
**
** Description      Open a DG client connection to a peer device.  BTA first
**                  searches for the requested service on the peer device.  If
**                  the service name is specified it will also match the
**                  service name.  Then BTA initiates an RFCOMM connection to
**                  the peer device.  The handle associated with the connection
**                  is returned with the BTA_DG_OPEN_EVT.  If the connection
**                  fails or closes at any time the callback function will be
**                  called with a BTA_DG_CLOSE_EVT.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgOpen(BD_ADDR bd_addr, tBTA_SERVICE_ID service, tBTA_SEC sec_mask,
                char *p_service_name, UINT8 app_id)
{
    tBTA_DG_API_OPEN  *p_buf;

    APPL_TRACE_API0( "BTA_DgOpen");
    if ((p_buf = (tBTA_DG_API_OPEN *) GKI_getbuf(sizeof(tBTA_DG_API_OPEN))) != NULL)
    {
        p_buf->hdr.event = BTA_DG_API_OPEN_EVT;
        bdcpy(p_buf->bd_addr, bd_addr);
        if (p_service_name != NULL)
        {
            BCM_STRNCPY_S(p_buf->name, sizeof(p_buf->name), p_service_name, BTA_SERVICE_NAME_LEN);
        }
        else
        {
            p_buf->name[0] = '\0';
        }
        p_buf->sec_mask = sec_mask;
        p_buf->service = service;
        p_buf->app_id = app_id;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_DgClose
**
** Description      Close a DG server connection to a peer device.  BTA will
**                  close the RFCOMM connection to the peer device.  The server
**                  will still be listening for subsequent connections.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgClose(UINT16 handle)
{
    BT_HDR  *p_buf;

    APPL_TRACE_API0( "BTA_DgClose");
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_DG_API_CLOSE_EVT;
        p_buf->layer_specific = handle;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         BTA_DgShutdown
**
** Description      Shutdown a DG server previously started by calling
**                  BTA_DgListen().  The server will no longer be available
**                  to peer devices.  If there is currently a connection open
**                  to the server it will be closed.
**                  
**
** Returns          void
**
*******************************************************************************/
void BTA_DgShutdown(UINT16 handle)
{
    BT_HDR  *p_buf;

    APPL_TRACE_API0( "BTA_DgShutdown");
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_DG_API_SHUTDOWN_EVT;
        p_buf->layer_specific = handle;
        bta_sys_sendmsg(p_buf);
    }
}
#endif /* BTA_DG_INCLUDED */
