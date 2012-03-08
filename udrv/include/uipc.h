/********************************************************************************
**                                                                              *
**  Name        uipc.h                                                          *
**                                                                              *
**  Function    UIPC wrapper interface          *
**                                                                              *
**                                                                              *
**  Copyright (c) 2007, Broadcom Corp., All Rights Reserved.                    *
**  Proprietary and confidential.                                               *
**                                                                              *
*********************************************************************************/
#ifndef UIPC_H
#define UIPC_H

#ifndef UDRV_API
#define UDRV_API
#endif


#define UIPC_CH_ID_ALL  0   /* used to address all the ch id at once */
#define UIPC_CH_ID_0    1   /* shared mem interface */
#define UIPC_CH_ID_1    2   /* TCP socket (GPS) */
#define UIPC_CH_ID_2    3   /* BTIF control socket */
#define UIPC_CH_ID_3    4   /* BTIF HH */
#define UIPC_CH_ID_4    5   /* Future usage */
#define UIPC_CH_ID_5    6   /* Future usage */
#define UIPC_CH_ID_6    7   /* Future usage */
#define UIPC_CH_ID_7    8   /* Future usage */
#define UIPC_CH_ID_8    9   /* Future usage */
#define UIPC_CH_ID_9    10  /* Future usage */
#define UIPC_CH_ID_10   11  /* Future usage */
#define UIPC_CH_ID_11   12  /* Future usage */
#define UIPC_CH_ID_12   13  /* Future usage */
#define UIPC_CH_ID_13   14  /* Future usage */
#define UIPC_CH_ID_14   15  /* Future usage */
#define UIPC_CH_ID_15   16  /* Future usage */
#define UIPC_CH_ID_16   17  /* Future usage */
#define UIPC_CH_ID_17   18  /* Future usage */
#define UIPC_CH_ID_18   19  /* Future usage */
#define UIPC_CH_ID_19   20  /* Future usage */
#define UIPC_CH_ID_20   21  /* Future usage */
#define UIPC_CH_ID_21   22  /* Future usage */
#define UIPC_CH_ID_22   23  /* Future usage */
#define UIPC_CH_ID_23   24  /* Future usage */
#define UIPC_CH_ID_24   25  /* Future usage */



#define UIPC_CH_NUM 25

typedef UINT8 tUIPC_CH_ID;

/*
 * UIPC IOCTL Requests
 */
enum
{
    UIPC_REQ_TX_FLUSH = 1,      /* Request to flush the TX FIFO */
    UIPC_REQ_RX_FLUSH,          /* Request to flush the RX FIFO */

    UIPC_WRITE_BLOCK,           /* Make write blocking */
    UIPC_WRITE_NONBLOCK,        /* Make write non blocking */

    UIPC_REG_CBACK,             /* Set a new call back */
    UIPC_SET_RX_WM,             /* Set Rx water mark */

    UIPC_REQ_TX_READY,          /* Request an indication when Tx ready */
    UIPC_REQ_RX_READY           /* Request an indication when Rx data ready */
};

typedef void (tUIPC_RCV_CBACK)(BT_HDR *p_msg); /* points to BT_HDR which describes event type and length of data; len contains the number of bytes of entire message (sizeof(BT_HDR) + offset + size of data) */

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         UIPC_Init
**
** Description      Initialize UIPC module
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern void UIPC_Init(void *);

/*******************************************************************************
**
** Function         UIPC_Open
**
** Description      Open UIPC interface
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern BOOLEAN UIPC_Open(tUIPC_CH_ID ch_id, tUIPC_RCV_CBACK *p_cback);

/*******************************************************************************
**
** Function         UIPC_Close
**
** Description      Close UIPC interface
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern void UIPC_Close(tUIPC_CH_ID ch_id);

/*******************************************************************************
**
** Function         UIPC_SendBuf
**
** Description      Called to transmit a message over UIPC.
**                  Message buffer will be freed by UIPC_SendBuf.
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern BOOLEAN UIPC_SendBuf(tUIPC_CH_ID ch_id, BT_HDR *p_msg);

/*******************************************************************************
**
** Function         UIPC_Send
**
** Description      Called to transmit a message over UIPC.
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern BOOLEAN UIPC_Send(tUIPC_CH_ID ch_id, UINT16 msg_evt, UINT8 *p_buf, UINT16 msglen);

/*******************************************************************************
**
** Function         UIPC_Read
**
** Description      Called to read a message from UIPC.
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern UINT32 UIPC_Read(tUIPC_CH_ID ch_id, UINT16 *p_msg_evt, UINT8 *p_buf, UINT32 len);

/*******************************************************************************
**
** Function         UIPC_Ioctl
**
** Description      Called to control UIPC.
**
** Returns          void
**
*******************************************************************************/
UDRV_API extern BOOLEAN UIPC_Ioctl(tUIPC_CH_ID ch_id, UINT32 request, void *param);

#ifdef __cplusplus
}
#endif


#endif  /* UIPC_H */


