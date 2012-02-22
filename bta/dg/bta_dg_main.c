/*****************************************************************************
**
**  Name:           bta_dg_main.c
**
**  Description:    This file contains the data gateway main functions and
**                  state machine.
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)

#include <string.h>
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_dg_api.h"
#include "bta_dg_int.h"
#include "port_api.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/

/* state machine states */
enum
{
    BTA_DG_INIT_ST,
    BTA_DG_OPENING_ST,
    BTA_DG_OPEN_ST,
    BTA_DG_CLOSING_ST
};

/* state machine action enumeration list */
enum
{
    BTA_DG_DEL_RECORD,
    BTA_DG_SHUTDOWN,
    BTA_DG_CLOSE,
    BTA_DG_SET_DEALLOC,
    BTA_DG_RX_PATH,
    BTA_DG_TX_PATH,
    BTA_DG_FC_STATE,
    BTA_DG_WRITEBUF,
    BTA_DG_CONTROL,
    BTA_DG_RFC_CONTROL,
    BTA_DG_RFC_OPEN,
    BTA_DG_RFC_CLOSE,
    BTA_DG_DEALLOC,
    BTA_DG_DISC_RESULT,
    BTA_DG_DO_DISC,
    BTA_DG_OPEN,
    BTA_DG_FREE_DB,
    BTA_DG_SETUP_SERVER,
    BTA_DG_IGNORE
};

/* type for action functions */
typedef void (*tBTA_DG_ACTION)(tBTA_DG_SCB *p_scb, tBTA_DG_DATA *p_data);

/* action function list */
const tBTA_DG_ACTION bta_dg_action[] =
{
    bta_dg_del_record,
    bta_dg_shutdown,
    bta_dg_close,
    bta_dg_set_dealloc,
    bta_dg_rx_path,
    bta_dg_tx_path,
    bta_dg_fc_state,
    bta_dg_writebuf,
    bta_dg_control,
    bta_dg_rfc_control,
    bta_dg_rfc_open,
    bta_dg_rfc_close,
    bta_dg_dealloc,
    bta_dg_disc_result,
    bta_dg_do_disc,
    bta_dg_open,
    bta_dg_free_db,
    bta_dg_setup_server
};

/* state table information */
#define BTA_DG_ACTIONS              2       /* number of actions */
#define BTA_DG_NEXT_STATE           2       /* position of next state */
#define BTA_DG_NUM_COLS             3       /* number of columns in state tables */

/* state table for init state */
const UINT8 bta_dg_st_init[][BTA_DG_NUM_COLS] =
{
/* Event                    Action 1                Action 2                Next state */
/* API_CLOSE_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* API_SHUTDOWN_EVT */      {BTA_DG_SHUTDOWN,       BTA_DG_DEALLOC,       BTA_DG_INIT_ST},
/* API_LISTEN_EVT */        {BTA_DG_SETUP_SERVER,   BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* API_OPEN_EVT */          {BTA_DG_DO_DISC,        BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_TX_READY_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* CI_RX_READY_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* CI_TX_FLOW_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* CI_RX_WRITEBUF_EVT */    {BTA_DG_WRITEBUF,       BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* CI_CONTROL_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_OPEN_EVT */          {BTA_DG_RFC_OPEN,       BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_CLOSE_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_TX_READY_EVT */      {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_RX_READY_EVT */      {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_FC_EVT */            {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_CONTROL_EVT */       {BTA_DG_RFC_CONTROL,    BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* DISC_RESULT_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* DISC_OK_EVT */           {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* DISC_FAIL_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_INIT_ST}
};

/* state table for opening state */
const UINT8 bta_dg_st_opening[][BTA_DG_NUM_COLS] =
{
/* Event                    Action 1                Action 2                Next state */
/* API_CLOSE_EVT */         {BTA_DG_CLOSE,          BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* API_SHUTDOWN_EVT */      {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* API_LISTEN_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* API_OPEN_EVT */          {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_TX_READY_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_RX_READY_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_TX_FLOW_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_RX_WRITEBUF_EVT */    {BTA_DG_WRITEBUF,       BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* CI_CONTROL_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* RFC_OPEN_EVT */          {BTA_DG_RFC_OPEN,       BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_CLOSE_EVT */         {BTA_DG_RFC_CLOSE,      BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_TX_READY_EVT */      {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* RFC_RX_READY_EVT */      {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* RFC_FC_EVT */            {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* RFC_CONTROL_EVT */       {BTA_DG_RFC_CONTROL,    BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* DISC_RESULT_EVT */       {BTA_DG_DISC_RESULT,    BTA_DG_IGNORE,          BTA_DG_OPENING_ST},
/* DISC_OK_EVT */           {BTA_DG_FREE_DB,        BTA_DG_OPEN,            BTA_DG_OPENING_ST},
/* DISC_FAIL_EVT */         {BTA_DG_FREE_DB,        BTA_DG_RFC_CLOSE,       BTA_DG_INIT_ST}
};

/* state table for open state */
const UINT8 bta_dg_st_open[][BTA_DG_NUM_COLS] =
{
/* Event                    Action 1                Action 2                Next state */
/* API_CLOSE_EVT */         {BTA_DG_CLOSE,          BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* API_SHUTDOWN_EVT */      {BTA_DG_SHUTDOWN,       BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* API_LISTEN_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* API_OPEN_EVT */          {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* CI_TX_READY_EVT */       {BTA_DG_TX_PATH,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* CI_RX_READY_EVT */       {BTA_DG_RX_PATH,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* CI_TX_FLOW_EVT */        {BTA_DG_FC_STATE,       BTA_DG_TX_PATH,         BTA_DG_OPEN_ST},
/* CI_RX_WRITEBUF_EVT */    {BTA_DG_WRITEBUF,       BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* CI_CONTROL_EVT */        {BTA_DG_CONTROL,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_OPEN_EVT */          {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_CLOSE_EVT */         {BTA_DG_RFC_CLOSE,      BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_TX_READY_EVT */      {BTA_DG_RX_PATH,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_RX_READY_EVT */      {BTA_DG_TX_PATH,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_FC_EVT */            {BTA_DG_RX_PATH,        BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* RFC_CONTROL_EVT */       {BTA_DG_RFC_CONTROL,    BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* DISC_RESULT_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* DISC_OK_EVT */           {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST},
/* DISC_FAIL_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_OPEN_ST}
};

/* state table for closing state */
const UINT8 bta_dg_st_closing[][BTA_DG_NUM_COLS] =
{
/* Event                    Action 1                Action 2                Next state */
/* API_CLOSE_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* API_SHUTDOWN_EVT */      {BTA_DG_DEL_RECORD,     BTA_DG_SET_DEALLOC,     BTA_DG_CLOSING_ST},
/* API_LISTEN_EVT */        {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* API_OPEN_EVT */          {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* CI_TX_READY_EVT */       {BTA_DG_TX_PATH,        BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* CI_RX_READY_EVT */       {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* CI_TX_FLOW_EVT */        {BTA_DG_FC_STATE,       BTA_DG_TX_PATH,         BTA_DG_CLOSING_ST},
/* CI_RX_WRITEBUF_EVT */    {BTA_DG_WRITEBUF,       BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* CI_CONTROL_EVT */        {BTA_DG_CONTROL,        BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* RFC_OPEN_EVT */          {BTA_DG_CLOSE,          BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* RFC_CLOSE_EVT */         {BTA_DG_RFC_CLOSE,      BTA_DG_IGNORE,          BTA_DG_INIT_ST},
/* RFC_TX_READY_EVT */      {BTA_DG_RX_PATH,        BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* RFC_RX_READY_EVT */      {BTA_DG_TX_PATH,        BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* RFC_FC_EVT */            {BTA_DG_RX_PATH,        BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* RFC_CONTROL_EVT */       {BTA_DG_RFC_CONTROL,    BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* DISC_RESULT_EVT */       {BTA_DG_FREE_DB,        BTA_DG_RFC_CLOSE,       BTA_DG_INIT_ST},
/* DISC_OK_EVT */           {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST},
/* DISC_FAIL_EVT */         {BTA_DG_IGNORE,         BTA_DG_IGNORE,          BTA_DG_CLOSING_ST}
};

/* type for state table */
typedef const UINT8 (*tBTA_DG_ST_TBL)[BTA_DG_NUM_COLS];

/* state table */
const tBTA_DG_ST_TBL bta_dg_st_tbl[] = {
    bta_dg_st_init,
    bta_dg_st_opening,
    bta_dg_st_open,
    bta_dg_st_closing
};

/*****************************************************************************
** Global data
*****************************************************************************/

/* DG control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_DG_CB  bta_dg_cb;
#endif

/*******************************************************************************
**
** Function         bta_dg_scb_alloc
**
** Description      Allocate a DG server control block.
**                  
**
** Returns          pointer to the scb, or NULL if none could be allocated.
**
*******************************************************************************/
static tBTA_DG_SCB *bta_dg_scb_alloc(void)
{
    tBTA_DG_SCB     *p_scb = &bta_dg_cb.scb[0];
    int             i;

    for (i = 0; i < BTA_DG_NUM_CONN; i++, p_scb++)
    {
        if (!p_scb->in_use)
        {
            p_scb->in_use = TRUE;
            APPL_TRACE_DEBUG1("bta_dg_scb_alloc %d", bta_dg_scb_to_idx(p_scb));
            break;
        }
    }
    
    if (i == BTA_DG_NUM_CONN)
    {
        /* out of lcbs */
        p_scb = NULL;
        APPL_TRACE_WARNING0("Out of scbs");
    }
    return p_scb;
}

/*******************************************************************************
**
** Function         bta_dg_sm_execute
**
** Description      State machine event handling function for DG
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_sm_execute(tBTA_DG_SCB *p_scb, UINT16 event, tBTA_DG_DATA *p_data)
{
    tBTA_DG_ST_TBL      state_table;
    UINT8               action;
    int                 i;

    switch( event )
    {
    case BTA_DG_CI_TX_READY_EVT:
    case BTA_DG_CI_RX_READY_EVT:
    case BTA_DG_RFC_TX_READY_EVT:
    case BTA_DG_RFC_RX_READY_EVT:
        break;
    default:
        APPL_TRACE_EVENT3("DG scb=%d event=0x%x state=%d", bta_dg_scb_to_idx(p_scb), event, p_scb->state);
        break;
    }

    /* look up the state table for the current state */
    state_table = bta_dg_st_tbl[p_scb->state];

    event &= 0x00FF;

    /* set next state */
    p_scb->state = state_table[event][BTA_DG_NEXT_STATE];

    /* execute action functions */
    for (i = 0; i < BTA_DG_ACTIONS; i++)
    {
        if ((action = state_table[event][i]) != BTA_DG_IGNORE)
        {
            (*bta_dg_action[action])(p_scb, p_data);
        }
        else
        {
            break;
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_api_enable
**
** Description      Handle an API enable event.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_api_enable(tBTA_DG_DATA *p_data)
{
    /* initialize control block */
    memset(&bta_dg_cb, 0, sizeof(bta_dg_cb));

    /* store callback function */
    bta_dg_cb.p_cback = p_data->api_enable.p_cback;

    /* call callback with enable event */
    (*bta_dg_cb.p_cback)(BTA_DG_ENABLE_EVT, NULL);
}

/*******************************************************************************
**
** Function         bta_dg_api_disable
**
** Description      Handle an API disable event.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_api_disable(tBTA_DG_DATA *p_data)
{
    /* close/shutdown all scbs in use */
    tBTA_DG_SCB     *p_scb = &bta_dg_cb.scb[0];
    int             i;
    UINT16          event;

    for (i = 0; i < BTA_DG_NUM_CONN; i++, p_scb++)
    {
        if (p_scb->in_use)
        {
            event = (p_scb->is_server) ? BTA_DG_API_SHUTDOWN_EVT : BTA_DG_API_CLOSE_EVT;
            bta_dg_sm_execute(p_scb, event, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_new_conn
**
** Description      Handle an API event that creates a new connection.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_new_conn(tBTA_DG_DATA *p_data)
{
    tBTA_DG_SCB     *p_scb;

    /* allocate an scb */
    if ((p_scb = bta_dg_scb_alloc()) != NULL)
    {
        bta_dg_sm_execute(p_scb, p_data->hdr.event, p_data);
    }   
}

/*******************************************************************************
**
** Function         bta_dg_rfc_port
**
** Description      Handle a port event from RFCOMM.  This function decodes
**                  the port event mask into multiple state machine events.
**                  
**
** Returns          void
**
*******************************************************************************/
static void bta_dg_rfc_port(tBTA_DG_DATA *p_data)
{
    tBTA_DG_SCB         *p_scb;

    if ((p_scb = bta_dg_scb_by_handle(p_data->hdr.layer_specific)) != NULL)
    {
        if (p_data->rfc_port.code & PORT_EV_FC)
        {
            bta_dg_sm_execute(p_scb, BTA_DG_RFC_FC_EVT, p_data);
        }       

        if (p_data->rfc_port.code & PORT_EV_TXEMPTY)
        {
            bta_dg_sm_execute(p_scb, BTA_DG_RFC_TX_READY_EVT, p_data);
        }       

        if (p_data->rfc_port.code & PORT_EV_RXCHAR)
        {
            bta_dg_sm_execute(p_scb, BTA_DG_RFC_RX_READY_EVT, p_data);
        }       

        if (p_data->rfc_port.code & (PORT_EV_CTS | PORT_EV_DSR | PORT_EV_RING))
        {
            bta_dg_sm_execute(p_scb, BTA_DG_RFC_CONTROL_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_scb_dealloc
**
** Description      Deallocate a link control block.
**                  
**
** Returns          void
**
*******************************************************************************/
void bta_dg_scb_dealloc(tBTA_DG_SCB *p_scb)
{
    APPL_TRACE_DEBUG1("bta_dg_scb_dealloc %d", bta_dg_scb_to_idx(p_scb));
    if (p_scb->port_handle != 0)
    {
        bta_dg_cb.hdl_to_scb[p_scb->port_handle - 1] = 0;
    }
    memset(p_scb, 0, sizeof(tBTA_DG_SCB));
}

/*******************************************************************************
**
** Function         bta_dg_scb_to_idx
**
** Description      Given a pointer to an scb, return its index.
**                  
**
** Returns          Index of scb.
**
*******************************************************************************/
UINT8 bta_dg_scb_to_idx(tBTA_DG_SCB *p_scb)
{
    /* use array arithmetic to determine index */
    return ((UINT8) (p_scb - bta_dg_cb.scb)) + 1;
}

/*******************************************************************************
**
** Function         bta_dg_scb_by_idx
**
** Description      Given an scb index return pointer to scb.
**                  
**
** Returns          Pointer to scb or NULL if not allocated.
**
*******************************************************************************/
tBTA_DG_SCB *bta_dg_scb_by_idx(UINT16 idx)
{
    tBTA_DG_SCB     *p_scb;

    /* verify index */
    if (idx > 0 && idx <= BTA_DG_NUM_CONN)
    {
        p_scb = &bta_dg_cb.scb[idx - 1];
        if (!p_scb->in_use)
        {
            p_scb = NULL;
            APPL_TRACE_WARNING1("scb idx %d not allocated", idx);
        }
    }
    else
    {
        p_scb = NULL;
        APPL_TRACE_WARNING1("scb idx %d out of range", idx);
    }
    return p_scb;
}

/*******************************************************************************
**
** Function         bta_dg_scb_by_handle
**
** Description      Find scb associated with handle.
**                  
**
** Returns          Pointer to scb or NULL if not found.
**
*******************************************************************************/
tBTA_DG_SCB *bta_dg_scb_by_handle(UINT16 handle)
{
    tBTA_DG_SCB     *p_scb = NULL;

    if (bta_dg_cb.hdl_to_scb[handle - 1] != 0)
    {
        p_scb = &bta_dg_cb.scb[bta_dg_cb.hdl_to_scb[handle - 1] - 1];
        if (!p_scb->in_use)
        {
            p_scb = NULL;
        }
    }

    if (p_scb == NULL)
    {
        APPL_TRACE_WARNING1("No scb for port handle %d", handle);
    }
        
    return p_scb;
}

/*******************************************************************************
**
** Function         bta_dg_server_match
**
** Description      Check if any servers exist with the same service name
**                  and service id.
**                  
**
** Returns          Pointer to scb or NULL if not found.
**
*******************************************************************************/
tBTA_DG_SCB *bta_dg_server_match(tBTA_DG_SCB *p_srv)
{
    tBTA_DG_SCB     *p_scb = &bta_dg_cb.scb[0];
    int             i;

    for (i = 0; i < BTA_DG_NUM_CONN; i++, p_scb++)
    {
        if (p_scb == p_srv)
        {
            continue;
        }

        if (p_scb->in_use && p_scb->is_server &&
            (p_scb->service_id == p_srv->service_id) &&
            (strcmp(p_srv->name, p_scb->name) == 0))
        {
            APPL_TRACE_DEBUG1("dg server match:%d", i);
            return p_scb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         bta_dg_hdl_event
**
** Description      Data gateway main event handling function.
**                  
**
** Returns          void
**
*******************************************************************************/
BOOLEAN bta_dg_hdl_event(BT_HDR *p_msg)
{
    tBTA_DG_SCB *p_scb;
    BOOLEAN     freebuf = TRUE;

    switch (p_msg->event)
    {
        /* handle enable event */
        case BTA_DG_API_ENABLE_EVT:
            bta_dg_api_enable((tBTA_DG_DATA *) p_msg);
            break;

        /* handle disable event */
        case BTA_DG_API_DISABLE_EVT:
            bta_dg_api_disable((tBTA_DG_DATA *) p_msg);
            break;

        /* handle listen and open events */
        case BTA_DG_API_LISTEN_EVT:
        case BTA_DG_API_OPEN_EVT:
            bta_dg_new_conn((tBTA_DG_DATA *) p_msg);
            break;

        /* handle RFCOMM port callback event mask */
        case BTA_DG_RFC_PORT_EVT:
            bta_dg_rfc_port((tBTA_DG_DATA *) p_msg);
            break;

        /* events that reference scb by index rather than port handle */
        case BTA_DG_API_CLOSE_EVT:
        case BTA_DG_API_SHUTDOWN_EVT:
        case BTA_DG_DISC_RESULT_EVT:
            if ((p_scb = bta_dg_scb_by_idx(p_msg->layer_specific)) != NULL)
            {
                bta_dg_sm_execute(p_scb, p_msg->event, (tBTA_DG_DATA *) p_msg);
            }
            break;
                
        /* events that require buffer not be released */
        case BTA_DG_CI_RX_WRITEBUF_EVT:
            freebuf = FALSE;
            /* fall through */

        /* all others reference scb by port handle */
        default:
            if ((p_scb = bta_dg_scb_by_handle(p_msg->layer_specific)) != NULL)
            {
                bta_dg_sm_execute(p_scb, p_msg->event, (tBTA_DG_DATA *) p_msg);
            }
            break;
    }
    return freebuf;   
}
#endif /* BTA_DG_INCLUDED */
