/*****************************************************************************
**
**  Name:       goep_fs.c
**
**  File:       Implements the Object Interface for GOEP Profiles
**
**  Copyright (c) 2000-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "gki.h"
#include "btu.h"
#include "goep_fs.h"

/*****************************************************************************
**
**  Function:    GOEP_OpenRsp
**
**  Purpose:     Report the status of tGOEP_OPEN_CBACK callback function.
**
**  Parameters:  fd         - File handle.
**               status     - Status of the operation.
**               file_size  - total number of bytes in this file.
**               event_id   - event id as given in the tGOEP_OPEN_CBACK function.
**
**  Returns:     void
**
*****************************************************************************/
void GOEP_OpenRsp (tGOEP_FD fd, tGOEP_STATUS status, UINT32 file_size,
                   UINT16 event_id)
{
    tGOEP_OPEN_RSP  *p_evt_msg;
    UINT16          size = sizeof(tGOEP_OPEN_RSP);

    /* get an GKI buffer and send the event along with the event data to BTU task */
    p_evt_msg = (tGOEP_OPEN_RSP *)GKI_getbuf(size);
    if (p_evt_msg != NULL)
    {
        memset(&p_evt_msg->hdr, 0, sizeof(BT_HDR));
        p_evt_msg->hdr.event    = event_id;
        p_evt_msg->fd           = fd;
        p_evt_msg->status       = status;
        p_evt_msg->file_size    = file_size;

        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_evt_msg);
    }
}

/*****************************************************************************
**
**  Function:    GOEP_ReadRsp
**
**  Purpose:     Report the status of tGOEP_READ_CBACK callback function.
**
**  Parameters:  fd         - File handle.
**               status     - Status of the operation.
**               bytes_read - total number of bytes read from the file.
**               event_id   - event id as given in the tGOEP_READ_CBACK function.
**
**  Returns:     void
**
*****************************************************************************/
void GOEP_ReadRsp (tGOEP_FD fd, tGOEP_STATUS status, UINT16 bytes_read,
                   UINT16 event_id)
{
    tGOEP_READ_RSP  *p_evt_msg;
    UINT16          size = sizeof(tGOEP_READ_RSP);

    /* get an GKI buffer and send the event along with the event data to BTU task */
    p_evt_msg = (tGOEP_READ_RSP *)GKI_getbuf(size);
    if (p_evt_msg != NULL)
    {
        memset(&p_evt_msg->hdr, 0, sizeof(BT_HDR));
        p_evt_msg->hdr.event    = event_id;
        p_evt_msg->fd           = fd;
        p_evt_msg->status       = status;
        p_evt_msg->bytes_read   = bytes_read;

        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_evt_msg);
    }
}

/*****************************************************************************
**
**  Function:    GOEP_WriteRsp
**
**  Purpose:     Report the status of tGOEP_WRITE_CBACK callback function.
**
**  Parameters:  fd         - File handle.
**               status     - Status of the operation.
**               event_id   - event id as given in the tGOEP_WRITE_CBACK function.
**
**  Returns:     void
**
*****************************************************************************/
void GOEP_WriteRsp (tGOEP_FD fd, tGOEP_STATUS status, UINT16 event_id)
{
    tGOEP_WRITE_RSP *p_evt_msg;
    UINT16          size = sizeof(tGOEP_WRITE_RSP);

    /* get an GKI buffer and send the event along with the event data to BTU task */
    p_evt_msg = (tGOEP_WRITE_RSP *)GKI_getbuf(size);
    if (p_evt_msg != NULL)
    {
        memset(&p_evt_msg->hdr, 0, sizeof(BT_HDR));
        p_evt_msg->hdr.event    = event_id;
        p_evt_msg->fd           = fd;
        p_evt_msg->status       = status;

        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_evt_msg);
    }
}

/*****************************************************************************
**
**  Function:   GOEP_DirentryRsp
**
**  Purpose:    Report the status of tGOEP_DIRENTRY_CBACK callback function.
**
** Parameters:  status - GOEP_OK if p_entry points to a valid entry.
**                       GOEP_EODIR if no more entries (p_entry is ignored).
**                       GOEP_FAIL if any errors have occurred.
**              event_id   - event id as given in the tGOEP_DIRENTRY_CBACK function.
**
**  Returns:    void
**
*****************************************************************************/
void GOEP_DirentryRsp (tGOEP_STATUS status, UINT16 event_id)
{
    tGOEP_DIRENTRY_RSP *p_evt_msg;
    UINT16              size = sizeof(tGOEP_DIRENTRY_RSP);

    /* get an GKI buffer and send the event along with the event data to BTU task */
    p_evt_msg = (tGOEP_DIRENTRY_RSP *)GKI_getbuf(size);
    if (p_evt_msg != NULL)
    {
        memset(&p_evt_msg->hdr, 0, sizeof(BT_HDR));
        p_evt_msg->hdr.event    = event_id;
        p_evt_msg->status       = status;

        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_evt_msg);
    }
}

