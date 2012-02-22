/*****************************************************************************
**
**  Name:       xml_bld_api.c
**
**  File:       Implements the XML Builder
**
**
**  Copyright (c) 2000-2004, WIDCOMM Inc., All Rights Reserved.
**  WIDCOMM Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include <stdio.h>

#include "bt_target.h"
#include "xml_bld_api.h"

/* The XML Builder is dependent on the Object Store. At present
** the object store resides in GOEP and hence the builder is 
** dependent on GOEP. The builder only uses the Object Store
** in GOEP, so if the Object Store is separated from GOEP in the
** future, the builder will not be dependent on GOEP.
*/

/*****************************************************************************
**   Constants
*****************************************************************************/
#define XML_ST  "<"
#define XML_EM  "/"
#define XML_SP  " "
#define XML_CO  ":"
#define XML_EQ  "="
#define XML_GT  ">"
#define XML_QT  "\""

#define XML_LF  "\n"    /* Line feed */

/*****************************************************************************
**   Interface functions
*****************************************************************************/


/*****************************************************************************
**
**  Function:    XML_BufAddTag
**
**  Purpose:     Write a start or end tag and optional prefix.
**               
**  Parameters:  
**       UINT8       **pp_buf     reference to the storage to hold the XML object
**                                GOEP_WriteStore.
**       const UINT8*  prefix     tag prefix (namespace)
**       const UINT8*  tag        tag name
**       BOOLEAN       start_tag  TRUE = start tag, FALSE = end tag
**       BOOLEAN       has_attr   TRUE if the tag contains attributes
**       
**  Returns:     XML_BLD_SUCCESS if success
**
*****************************************************************************/
tXML_BLD_RESULT XML_BufAddTag (UINT8    **pp_buf, 
                            const UINT8 *prefix,
                            const UINT8 *tag, 
                            BOOLEAN     start_tag,
                            BOOLEAN     has_attr)
{
    UINT16 status = XML_BLD_ERROR;
    int n;

    if (tag != NULL)
    {
        if(start_tag)
            n = sprintf((char *)*pp_buf, XML_ST);
        else
            n = sprintf((char *)*pp_buf, XML_ST XML_EM);
        *pp_buf += n;

        if (prefix != NULL)
        {
            n = sprintf((char *)*pp_buf, "%s" XML_CO, prefix );
            *pp_buf += n;
        }
        n = sprintf((char *)*pp_buf, "%s" , tag );
        *pp_buf += n;
        if(!has_attr)
        {
            n = sprintf((char *)*pp_buf, XML_GT);
            *pp_buf += n;
            if (!start_tag)
            {
                n = sprintf((char *)*pp_buf, XML_LF);
                *pp_buf += n;
            }
        }

        status = XML_BLD_SUCCESS;
    }
    return status;
}



/*****************************************************************************
**
**  Function:    XML_BufAddAttribute
**
**  Purpose:     Write an attribute and optional prefix.
**               
**  Parameters:  
**       UINT8       **pp_buf     reference to the storage to hold the XML object
**       const UINT8*  prefix     attribute prefix (namespace)
**       const UINT8*  attr_name  attribute name
**       const UINT8*  attr_value attribute value
**       
**  Returns:     XML_BLD_SUCCESS if success
**
*****************************************************************************/
tXML_BLD_RESULT XML_BufAddAttribute (UINT8    **pp_buf, 
                                  const UINT8 *prefix,
                                  const UINT8 *attr_name, 
                                  const UINT8 *attr_value, 
                                  tXML_ATTR_END     last_attr)
{
    UINT16 status = XML_BLD_ERROR;
    int n;

    if (attr_name != NULL && attr_value != NULL)
    {
        n = sprintf((char *)*pp_buf, XML_SP);
        *pp_buf += n;
        if (prefix != NULL)
        {
            n = sprintf((char *)*pp_buf, "%s" XML_CO, prefix );
            *pp_buf += n;
        }
        n = sprintf((char *)*pp_buf, "%s" XML_EQ XML_QT "%s", attr_name, attr_value );
        *pp_buf += n;
        switch(last_attr)
        {
        case XML_ATTR_CONT:
            n = sprintf((char *)*pp_buf, XML_QT );
            break;
        case XML_ATTR_LAST:
            n = sprintf((char *)*pp_buf, XML_QT XML_GT XML_LF );
            break;
        case XML_ATTR_ETAG:
            n = sprintf((char *)*pp_buf, XML_QT XML_EM XML_GT XML_LF );
            break;
        default:
            n = 0;
            break;
        }
        *pp_buf += n;
        status = XML_BLD_SUCCESS;
    }
    else if(last_attr == XML_ATTR_ETAG)
    {
        /* allow the call to only add the closing attribute */
        n = sprintf((char *)*pp_buf, XML_EM XML_GT XML_LF );
        *pp_buf += n ;
        status = XML_BLD_SUCCESS;
    }
    return status;
}

/*****************************************************************************
**
**  Function:    XML_BufAddCharData
**
**  Purpose:     Write the element content.
**               
**  Parameters:  
**       UINT8       **pp_buf     reference to the storage to hold the XML object
**       const UINT8*  content    element content
**       
**  Returns:     XML_BLD_SUCCESS if success
**
*****************************************************************************/
tXML_BLD_RESULT XML_BufAddCharData (UINT8 **pp_buf, const UINT8 *charData)
{
    UINT16 status = XML_BLD_ERROR;
    int n;

    if (charData != NULL)
    {
        n = sprintf((char *)*pp_buf, "%s",  charData);
        *pp_buf += n;
        status = XML_BLD_SUCCESS;
    }
    return status;
}

