/*****************************************************************************
**
**  Name:           bta_pbs_ci.c
**
**  Description:    This is the implementation file for the phone book access server
**                  call-in functions.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_pbs_ci.h"
#include "bta_pbs_int.h"
#include "gki.h"

/*******************************************************************************
**
** Function         bta_pbs_ci_read
**
** Description      This function sends an event to BTA indicating the phone has
**                  read in the requested amount of data specified in the
**                  bta_pbs_co_read() call-out function.  
**                  
** Parameters       fd - file descriptor passed to the stack in the
**                       bta_pbs_ci_open call-in function.
**                  num_bytes_read - number of bytes read into the buffer
**                      specified in the read callout-function.
**                  status - BTA_PBS_CO_OK if get buffer of data,
**                           BTA_PBS_CO_FAIL if an error has occurred.
**                  final - indicate whether it is the final data
**                  
**
** Returns          void 
**
*******************************************************************************/
void bta_pbs_ci_read(int fd, UINT16 num_bytes_read,
                     tBTA_PBS_CO_STATUS status, BOOLEAN final)
{
    tBTA_PBS_CI_READ_EVT  *p_evt;

    if ((p_evt = (tBTA_PBS_CI_READ_EVT *) GKI_getbuf(sizeof(tBTA_PBS_CI_READ_EVT))) != NULL)
    {
        p_evt->hdr.event = BTA_PBS_CI_READ_EVT;
        p_evt->fd        = fd;
        p_evt->status    = status;
        p_evt->num_read  = num_bytes_read;
        p_evt->final = final;

        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_pbs_ci_open
**
** Description      This function sends an event to BTA indicating the phone has
**                  finished opening a pb for reading.
**                  
** Parameters       fd - file descriptor passed to the stack in the
**                       bta_pbs_ci_open call-in function.
**                  status - BTA_PBS_CO_OK if file was opened in mode specified
**                                          in the call-out function.
**                           BTA_PBS_CO_EACCES if the file exists, but contains
**                                          the wrong access permissions.
**                           BTA_PBS_CO_FAIL if any other error has occurred.
**                  file_size - The total size of the file
**
**
** Returns          void 
**
*******************************************************************************/
void bta_pbs_ci_open(int fd, tBTA_PBS_CO_STATUS status, UINT32 file_size)
{
    tBTA_PBS_CI_OPEN_EVT  *p_evt;

    if ((p_evt = (tBTA_PBS_CI_OPEN_EVT *) GKI_getbuf(sizeof(tBTA_PBS_CI_OPEN_EVT))) != NULL)
    {
        p_evt->hdr.event = BTA_PBS_CI_OPEN_EVT;
        p_evt->fd        = fd;
        p_evt->status    = status;
        p_evt->file_size = file_size;

        bta_sys_sendmsg(p_evt);
    }
}

/*******************************************************************************
**
** Function         bta_pbs_ci_getvlist
**
** Description      This function sends an event to BTA indicating the phone has
**                  finished reading a VCard list entry.
**                  
** Parameters       
**                  status - BTA_PBS_CO_OK if reading Vcard list entry
**                           BTA_PBS_CO_FAIL if any other error has occurred.
**                  final - whether it is the last entry
**
**
** Returns          void 
**
*******************************************************************************/
BTA_API extern void bta_pbs_ci_getvlist(tBTA_PBS_CO_STATUS status, BOOLEAN final)
{
    tBTA_PBS_CI_VLIST_EVT  *p_evt;

    if ((p_evt = (tBTA_PBS_CI_VLIST_EVT *) GKI_getbuf(sizeof(tBTA_PBS_CI_VLIST_EVT))) != NULL)
    {
        p_evt->hdr.event = BTA_PBS_CI_VLIST_EVT;
        p_evt->status    = status;
        p_evt->final = final;

        bta_sys_sendmsg(p_evt);
    }
}
