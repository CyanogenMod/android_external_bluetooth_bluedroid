/*****************************************************************************
**
**  Name:         obx_wchar.c
**
**  File:         OBEX Headers Encode conversion Utility functions
**
**  Copyright (c) 2003-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "gki.h"
#include "obx_api.h"

/*******************************************************************************
**
** Function     OBX_CharToWchar
**
** Description  This function is called to convert ASCII to Unicode (UINT16).
**
** Returns      the length.
**
*******************************************************************************/
UINT16 OBX_CharToWchar (UINT16 *w_str, char* a_str, UINT16 w_size) 
{
    UINT16  result = 0;
    int     size = w_size;

    if (a_str == NULL || w_str == NULL)
        return 0;

    while (size > 0 && *a_str != '\0')
    {
        w_str[result++] = (UINT16) *a_str;
        a_str++;
        size--;
    }

    if (size > 0)
    {
        w_str[result] = 0;
    }

    return (result+1);
}

/*******************************************************************************
**
** Function     OBX_WcharToChar
**
** Description  This function is called to convert Unicode (UINT16) to ASCII.
**
** Returns      void.
**
*******************************************************************************/
void OBX_WcharToChar (char *a_str, UINT16* w_str, UINT16 a_size) 
{
    UINT16  result = 0;
    int     size = a_size;

    if (w_str == NULL || a_str == NULL)
        return;

    while (size > 0 && *w_str != 0)
    {
        if ((*w_str & ~0xff) != 0)
        {
            result = 0;
            break;
        }

        a_str[result++] = (char) *w_str;
        ++(w_str);
        --size;
    }

    if(size)
        a_str[result] = 0;

    return;


}

