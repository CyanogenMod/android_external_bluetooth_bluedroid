/*****************************************************************************
**
**  Name:         obx_dauth.c
**
**  File:         OBEX Authentication related functions
**
**  Copyright (c) 2003-2008, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "obx_api.h"





#if (OBX_CLIENT_INCLUDED == TRUE)

/*******************************************************************************
**
** Function     OBX_ReadChallenge
**
** Description  This function is called by the client to read the Realm and  
**              options of the Authentication Challenge Header in the
**              given OBEX packet ( from an OBX_PASSWORD_EVT event).
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadChallenge(BT_HDR *p_pkt, tOBX_CHARSET *p_charset,
                                         UINT8 **p_realm, UINT8 *p_len,
                                         tOBX_AUTH_OPT *p_option)
{
    BOOLEAN         status;
    tOBX_TRIPLET    triplet[OBX_MAX_NUM_AUTH_TRIPLET];
    UINT8           num_trip = OBX_MAX_NUM_AUTH_TRIPLET;
    UINT8           xx;

     /* The coverity complaints on this function is not correct. 
      * The value in triplet[] is set/initialized by OBX_ReadTriplet if it returns TRUE.
      * leave this unnecessary memset here */
    memset( triplet, 0, sizeof(triplet ));

    *p_len      = 0;
    *p_option   = OBX_AO_NONE;
    *p_realm    = NULL;
    *p_charset  = OBX_RCS_ASCII; /* 0 */
    status = OBX_ReadTriplet(p_pkt, OBX_HI_CHALLENGE, triplet, &num_trip);
    if(status)
    {
        for(xx=0; xx<num_trip; xx++)
        {
            switch(triplet[xx].tag)
            {
            case OBX_OPTIONS_CHLNG_TAG:
                *p_option   = triplet[xx].p_array[0];
                break;
            case OBX_REALM_CHLNG_TAG:
                *p_charset  = triplet[xx].p_array[0];
                *p_len      = triplet[xx].len - 1;
                *p_realm     = &(triplet[xx].p_array[1]);
                break;
                /* The user does not need to know the nonce value
            case OBX_NONCE_CHLNG_TAG:
                break;
                */
            }
        }
    }

    return status;
}
#endif



#if (OBX_SERVER_INCLUDED == TRUE)
/*******************************************************************************
**
** Function     OBX_ReadAuthParams
**
** Description  This function is called  by the server to read the User ID of 
**              the Authentication Response Header in the given OBEX packet
**              ( from an OBX_PASSWORD_EVT event).
**              This function also
**              checks if the authentication challenge header exists and
**              checks the authentication challenge options.
**
** Returns      TRUE, if the header is in the OBEX packet.
**              FALSE, otherwise.
**
*******************************************************************************/
BOOLEAN OBX_ReadAuthParams(BT_HDR *p_pkt, UINT8 **p_userid, UINT8 *p_len,
                           BOOLEAN *is_challenged, tOBX_AUTH_OPT *p_option)
{
    BOOLEAN         status;
    tOBX_TRIPLET    triplet[OBX_MAX_NUM_AUTH_TRIPLET];
    UINT8           num_trip = OBX_MAX_NUM_AUTH_TRIPLET;
    UINT8           xx;

     /* The coverity complaints on this function is not correct. 
      * The value in triplet[] is set/initialized by OBX_ReadTriplet if it returns TRUE.
      * leave this unnecessary memset here */
    memset( triplet, 0, sizeof(triplet ));

    *p_userid       = NULL;
    *p_len          = 0;
    *is_challenged  = FALSE;
    *p_option       = OBX_AO_NONE;
    status = OBX_ReadTriplet(p_pkt, OBX_HI_AUTH_RSP, triplet, &num_trip);
    if(status)
    {
        /* found authentication response, but we do not know if it has user id.
         * assume no user id */
        *p_len = 0;
        for(xx=0; xx<num_trip; xx++)
        {
            if(triplet[xx].tag == OBX_USERID_RSP_TAG)
            {
                *p_len      = triplet[xx].len;
                *p_userid   = triplet[xx].p_array;
                break;
            }
        }

        /* read the authentication challenge header,
         * only if the authentication response header exists */
        if( OBX_ReadTriplet(p_pkt, OBX_HI_CHALLENGE, triplet, &num_trip) == TRUE)
        {
            *is_challenged  = TRUE;
            for(xx=0; xx<num_trip; xx++)
            {
                if(triplet[xx].tag == OBX_OPTIONS_CHLNG_TAG)
                {
                    *p_option   = triplet[xx].p_array[0];
                    break;
                }
            }
        }
    }
    return status;
}
#endif



