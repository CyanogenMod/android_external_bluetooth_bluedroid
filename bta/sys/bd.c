/*****************************************************************************
**
**  Name:           bd.c
**
**  Description:    BD address services.
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "data_types.h"
#include "bd.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* global constant for "any" bd addr */
const BD_ADDR bd_addr_any = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const BD_ADDR bd_addr_null= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*****************************************************************************
**  Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bdcpy
**
** Description      Copy bd addr b to a.
**
**
** Returns          void
**
*******************************************************************************/
void bdcpy(BD_ADDR a, const BD_ADDR b)
{
    int i;

    for (i = BD_ADDR_LEN; i != 0; i--)
    {
        *a++ = *b++;
    }
}

/*******************************************************************************
**
** Function         bdcmp
**
** Description      Compare bd addr b to a.
**
**
** Returns          Zero if b==a, nonzero otherwise (like memcmp).
**
*******************************************************************************/
int bdcmp(const BD_ADDR a, const BD_ADDR b)
{
    int i;

    for (i = BD_ADDR_LEN; i != 0; i--)
    {
        if (*a++ != *b++)
        {
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
**
** Function         bdcmpany
**
** Description      Compare bd addr to "any" bd addr.
**
**
** Returns          Zero if a equals bd_addr_any.
**
*******************************************************************************/
int bdcmpany(const BD_ADDR a)
{
    return bdcmp(a, bd_addr_any);
}

/*******************************************************************************
**
** Function         bdsetany
**
** Description      Set bd addr to "any" bd addr.
**
**
** Returns          void
**
*******************************************************************************/
void bdsetany(BD_ADDR a)
{
    bdcpy(a, bd_addr_any);
}
