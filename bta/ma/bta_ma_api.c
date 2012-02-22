/*****************************************************************************
**
**  Name:           bta_mse_api.c
**
**  Description:    This is the implementation of the common API for the Message
**                  Acess Profile of BTA, Broadcom Corp's Bluetooth
**                  application layer for mobile phones.
**
**  Copyright (c) 2009-2011 Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if defined(BTA_MSE_INCLUDED) && (BTA_MSE_INCLUDED == TRUE)


#include <string.h>
#include "bta_ma_api.h"
#include "bta_ma_util.h"
#include "bta_ma_co.h"
/*******************************************************************************
**
** Function         BTA_MaBmsgCreate
**
** Description      Create and initialize an instance of a tBTA_MA_BMSG structure.
**                  
** Parameters       None  
**
** Returns          Pointer to a bMessage object, or NULL if this fails.
**
*******************************************************************************/
tBTA_MA_BMSG * BTA_MaBmsgCreate(void)
{
    tBTA_MA_BMSG *p_msg = (tBTA_MA_BMSG *) bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG));

    if ( p_msg )
    {
        memset(p_msg, 0, sizeof(tBTA_MA_BMSG));
    }
    else
    {
        APPL_TRACE_ERROR0("BTA_MaBmsgCreate failed");
    }

    return(p_msg);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgFree
**
** Description      Destroy (free) the contents of a tBTA_MA_BMSG structure.
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgFree(tBTA_MA_BMSG * p_msg)
{
    if ( p_msg )
    {
        /* free folder string */
        bta_ma_bmsg_free(p_msg->p_folder);

        /* free all the vCards */
        bta_ma_bmsg_free_vcards(p_msg->p_orig);

        /* free the envelopes */
        bta_ma_bmsg_free_envelope(p_msg->p_envelope);

        /* free b-message structure itself */
        bta_ma_bmsg_free(p_msg);
    }
}

/*******************************************************************************
**
** Function         BTA_MaBmsgSetReadSts 
**
** Description      Set the bmessage-readstatus-property value for the bMessage
**                  object. If the 'read_sts' is TRUE then value will be "READ", 
**                  otherwise it is "UNREAD".
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  read_sts - Read status TRUE- read FALSE - unread 
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetReadSts(tBTA_MA_BMSG * p_msg, BOOLEAN read_sts)
{
    if ( p_msg )
        p_msg->read_sts = read_sts;
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetReadSts
**
** Description      Get the bmessage-readstatus-property value for the bMessage
**                  object 
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                     
** Returns          Read status (TRUE/FALSE) for the specified bMessage.
**
*******************************************************************************/
BOOLEAN BTA_MaBmsgGetReadSts(tBTA_MA_BMSG * p_msg)
{
    return( p_msg ? p_msg->read_sts : FALSE);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgSetMsgType
**
** Description      Set the bmessage-type-property value for the bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  msg_type - Message type
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetMsgType(tBTA_MA_BMSG * p_msg, tBTA_MA_MSG_TYPE msg_type)
{
    if ( p_msg )
        p_msg->msg_type = msg_type;
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetMsgType
**
** Description      Get the bmessage-type-property value for the specified 
**                  bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  
** Returns          Message type
**
*******************************************************************************/
tBTA_MA_MSG_TYPE BTA_MaBmsgGetMsgType(tBTA_MA_BMSG * p_msg)
{
    /* return 0 (for lack of a better value) */
    return( p_msg ? p_msg->msg_type : 0);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgSetFolder
**
** Description      Set the bmessage-folder-property value for the bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  p_folder - Pointer to a folder path
**                    
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetFolder(tBTA_MA_BMSG *p_msg, char *p_folder)
{
    if ( p_msg && p_folder )
    {
        /* free any existing string */
        if ( p_msg->p_folder )
            bta_ma_bmsg_free(p_msg->p_folder);

        /* allocate a new one */
        p_msg->p_folder = (char *) bta_ma_bmsg_alloc(strlen(p_folder)+1);
        if ( p_msg->p_folder )
            BCM_STRNCPY_S(p_msg->p_folder, strlen(p_folder)+1, p_folder, strlen(p_folder)+1);
    }
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetFolder
**
** Description      Get the bmessage-folder-property value for the specified 
**                  bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**        
** Returns          Pointer to folder path string, or NULL if it has not been set.
**
*******************************************************************************/
char * BTA_MaBmsgGetFolder(tBTA_MA_BMSG * p_msg)
{
    return(p_msg ? (char *) p_msg->p_folder : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgAddOrigToBmsg
**
** Description      Add an originator to the bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**        
** Returns          Pointer to a new vCard structure, or NULL if this function 
**                  fails.
**
*******************************************************************************/
tBTA_MA_BMSG_VCARD * BTA_MaBmsgAddOrigToBmsg(tBTA_MA_BMSG * p_msg)
{
    tBTA_MA_BMSG_VCARD *   p_vcard = NULL;
    tBTA_MA_BMSG_VCARD *   p_last_vcard = NULL;

    if ( p_msg )
    {
        p_vcard = (tBTA_MA_BMSG_VCARD *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_VCARD));
        if ( p_vcard )
        {
            memset(p_vcard, 0, sizeof(tBTA_MA_BMSG_VCARD));
            p_vcard->version = BTA_MA_VCARD_VERSION_21;    /* default to 2.1 */

            if ( p_msg->p_orig )
            {
                /* set pointer to the last entry in the list */
                p_last_vcard = p_msg->p_orig;
                while ( p_last_vcard->p_next )
                    p_last_vcard =  (tBTA_MA_BMSG_VCARD *) p_last_vcard->p_next;

                p_last_vcard->p_next = p_vcard;
            }
            else
                p_msg->p_orig = p_vcard;
        }
    }

    return(p_vcard);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetOrigFromBmsg
**
** Description      Get the first originator vCard information from the specified
**                  bMessage object
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**        
** Returns          Pointer to first 'originator vCard, or NULL not used.
**
*******************************************************************************/
tBTA_MA_BMSG_VCARD * BTA_MaBmsgGetOrigFromBmsg(tBTA_MA_BMSG * p_msg)
{
    return(p_msg ? (tBTA_MA_BMSG_VCARD *) p_msg->p_orig : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgAddEnvToBmsg
**
** Description      Add a new envelope to the bMessage object. This is the first
**                  (top) level envelope. bMessage allows up to 3 levels of envelopes.
**                  application should call BTA_MaBmsgAddEnvToEnv to add the 2nd 
**                  3rd level enevelope. 
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  
** Returns          Pointer to a new envelope structure, or NULL if this
**                  function fails.
**
*******************************************************************************/
tBTA_MA_BMSG_ENVELOPE * BTA_MaBmsgAddEnvToBmsg(tBTA_MA_BMSG * p_msg)
{
    tBTA_MA_BMSG_ENVELOPE * p_envelope = NULL;

    if ( p_msg )
    {
        p_envelope = (tBTA_MA_BMSG_ENVELOPE *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_ENVELOPE));
        if ( p_envelope )
        {
            memset(p_envelope, 0, sizeof(tBTA_MA_BMSG_ENVELOPE));
            p_msg->p_envelope = p_envelope;
        }
    }

    return( p_envelope );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgAddEnvToEnv
**
** Description      Add a child envelope to an existing envelope. 
**
** Parameters       p_envelope - Pointer to a parent envelope                   
**                  
** Returns          Pointer to an envelope structure, or NULL if this
**                  function fails.
**
*******************************************************************************/
tBTA_MA_BMSG_ENVELOPE * BTA_MaBmsgAddEnvToEnv(tBTA_MA_BMSG_ENVELOPE * p_envelope)
{
    tBTA_MA_BMSG_ENVELOPE * p_new_envelope = NULL;

    if ( p_envelope )
    {
        p_new_envelope = (tBTA_MA_BMSG_ENVELOPE *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_ENVELOPE));
        if ( p_new_envelope )
        {
            memset(p_new_envelope, 0, sizeof(tBTA_MA_BMSG_ENVELOPE));
            p_envelope->p_next = p_new_envelope;
        }
    }

    return( p_new_envelope );
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetEnv
**
** Description      Get the pointer of the first level envelope. 
**
** Parameters       p_bmsg - Pointer to a bMessage object                   
**                  
** Returns          Pointer to the first level envelope structure, or NULL if it
**                  does not exist
**
*******************************************************************************/
tBTA_MA_BMSG_ENVELOPE * BTA_MaBmsgGetEnv(tBTA_MA_BMSG * p_msg)
{
    return(p_msg ? (tBTA_MA_BMSG_ENVELOPE *) p_msg->p_envelope : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetNextEnv
**
** Description      Get the child envelope of the specified parent envelope. 
**
** Parameters       p_env - Pointer to a parent envelope                   
**                  
** Returns          Pointer to a child enevelope. NULL if the 
**                  envelope does not have a 'child' envelope.
**
*******************************************************************************/
tBTA_MA_BMSG_ENVELOPE * BTA_MaBmsgGetNextEnv(tBTA_MA_BMSG_ENVELOPE * p_env)
{
    return(p_env ?  (tBTA_MA_BMSG_ENVELOPE *)p_env->p_next :  NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgAddRecipToEnv
**
** Description      Add recipient to the specified envelope. 
**
** Parameters       p_env - Pointer to a envelope                   
**                  
** Returns          Pointer to a vCard structure. NULL if it 
**                  fails to allocate a vCard structure.
**
*******************************************************************************/
tBTA_MA_BMSG_VCARD * BTA_MaBmsgAddRecipToEnv(tBTA_MA_BMSG_ENVELOPE * p_env)
{
    tBTA_MA_BMSG_VCARD *   p_vcard = NULL;
    tBTA_MA_BMSG_VCARD *   p_last_vcard = NULL;

    if ( p_env )
    {
        p_vcard = (tBTA_MA_BMSG_VCARD *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_VCARD));
        if ( p_vcard )
        {
            memset(p_vcard, 0, sizeof(tBTA_MA_BMSG_VCARD));
            p_vcard->version = BTA_MA_VCARD_VERSION_21;    /* default to 2.1 */

            if ( p_env->p_recip )
            {
                /* set pointer to the last entry in the list */
                p_last_vcard = p_env->p_recip;
                while ( p_last_vcard->p_next )
                    p_last_vcard = (tBTA_MA_BMSG_VCARD *) p_last_vcard->p_next;

                p_last_vcard->p_next = p_vcard;
            }
            else
                p_env->p_recip = p_vcard;
        }
    }

    return(p_vcard);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetRecipFromEnv
**
** Description      Get the first recipient's vCard from the specified envelope. 
**
** Parameters       p_env - Pointer to a envelope                   
**                  
** Returns          Pointer to the first recipient's vCard structure. NULL if it 
**                  has not be set.
**
*******************************************************************************/
tBTA_MA_BMSG_VCARD * BTA_MaBmsgGetRecipFromEnv(tBTA_MA_BMSG_ENVELOPE * p_env)
{
    return(p_env ? p_env->p_recip : NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgAddBodyToEnv
**
** Description      Add a message body to the specified envelope. 
**
** Parameters       p_env - Pointer to a envelope                   
**                  
** Returns          Pointer to a message body structure. 
**                  NULL if it fails to allocate a message body structure.
**
*******************************************************************************/
tBTA_MA_BMSG_BODY * BTA_MaBmsgAddBodyToEnv(tBTA_MA_BMSG_ENVELOPE * p_env)
{
    tBTA_MA_BMSG_BODY * p_body = NULL;

    if ( p_env )
    {
        /* free any existing body */
        if ( p_env->p_body )
            bta_ma_bmsg_free_body(p_env->p_body);

        /* allocate a new one */
        p_body = (tBTA_MA_BMSG_BODY *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_BODY));
        if ( p_body )
        {
            memset(p_body, 0, sizeof(tBTA_MA_BMSG_BODY));
            p_body->encoding = BTA_MA_BMSG_ENC_8BIT;   /* default */
        }

        p_env->p_body = p_body;
    }

    return(p_body);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetBodyFromEnv
**
** Description      Get the message body pointer from the specified envelope. 
**
** Parameters       p_env - Pointer to a envelope                   
**                  
** Returns          Pointer to a message body structure. 
**                  NULL if it has not been set.
**
*******************************************************************************/
tBTA_MA_BMSG_BODY * BTA_MaBmsgGetBodyFromEnv(tBTA_MA_BMSG_ENVELOPE * p_env)
{
    return( p_env ? p_env->p_body : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgSetBodyEncoding 
**
** Description      Set the bmessage-body-encoding-property value for the bMessage
**                  body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                  encoding - encoding scheme 
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetBodyEncoding(tBTA_MA_BMSG_BODY * p_body, tBTA_MA_BMSG_ENCODING encoding)
{
    if ( p_body )
        p_body->encoding = encoding;
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetBodyEncoding 
**
** Description      Get the bmessage-body-encoding-property value for the specified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          Message encoding scheme 
**
*******************************************************************************/
tBTA_MA_BMSG_ENCODING BTA_MaBmsgGetBodyEncoding(tBTA_MA_BMSG_BODY * p_body)
{
    return( p_body ? p_body->encoding : BTA_MA_BMSG_ENC_8BIT );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgSetBodyPartid 
**
** Description      Set the bmessage-body-part-ID value for the speicified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                  part_id - Part ID (range: from 0 to 65535)
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetBodyPartid(tBTA_MA_BMSG_BODY * p_body, UINT16 part_id)
{
    if ( p_body )
    {
        p_body->part_id = part_id;
        p_body->is_multipart = TRUE;
    }
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetBodyPartid 
**
** Description      Get the bmessage-body-part-ID value for the specified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          The value of the Part ID
**
*******************************************************************************/
UINT16 BTA_MaBmsgGetBodyPartid(tBTA_MA_BMSG_BODY * p_body)
{
    return( p_body ? p_body->part_id : 0 );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgIsBodyMultiPart 
**
** Description      Is this a multi-part body
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          TURE - if this is a multi-part body
**
*******************************************************************************/
BOOLEAN BTA_MaBmsgIsBodyMultiPart(tBTA_MA_BMSG_BODY * p_body)
{
    return( p_body ? p_body->is_multipart : FALSE );
}

/*******************************************************************************
**
** Function         BTA_MaBmsgSetBodyCharset 
**
** Description      Set the bmessage-body-charset-property value for the speicified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                  charset - Charset
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetBodyCharset(tBTA_MA_BMSG_BODY * p_body, tBTA_MA_CHARSET charset)
{
    if ( p_body )
        p_body->charset = charset;
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetBodyCharset 
**
** Description      Get the bmessage-body-charset-property value for the speicified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          Charset
**
*******************************************************************************/
tBTA_MA_CHARSET BTA_MaBmsgGetBodyCharset(tBTA_MA_BMSG_BODY * p_body)
{
    return( p_body ? p_body->charset : BTA_MA_CHARSET_UNKNOWN );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgSetBodyLanguage 
**
** Description      Set the bmessage-body-language-property value for the speicified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                  Language - the language of the message
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetBodyLanguage(tBTA_MA_BMSG_BODY * p_body, tBTA_MA_BMSG_LANGUAGE language)
{
    if ( p_body )
        p_body->language = language;
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetBodyLanguage 
**
** Description      Get the bmessage-body-language-property value for the speicified
**                  bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          the language of the message
**
*******************************************************************************/
tBTA_MA_BMSG_LANGUAGE BTA_MaBmsgGetBodyLanguage(tBTA_MA_BMSG_BODY * p_body)
{
    return( p_body ? p_body->language : BTA_MA_BMSG_LANG_UNSPECIFIED );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgAddContentToBody 
**
** Description      Add a message content to the speicified bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          Pointer to a message content.
**                  NULL if it fails to allocate a message content buffer
**
*******************************************************************************/
tBTA_MA_BMSG_CONTENT * BTA_MaBmsgAddContentToBody(tBTA_MA_BMSG_BODY * p_body)
{
    tBTA_MA_BMSG_CONTENT * p_content = NULL;
    tBTA_MA_BMSG_CONTENT * p_last_content = NULL;

    if ( p_body )
    {
        p_content = (tBTA_MA_BMSG_CONTENT *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_CONTENT));
        if ( p_content )
        {
            memset(p_content, 0, sizeof(tBTA_MA_BMSG_CONTENT));

            if ( p_body->p_content )
            {
                /* set pointer to the last entry in the list */
                p_last_content = p_body->p_content;
                while ( p_last_content->p_next )
                    p_last_content = (tBTA_MA_BMSG_CONTENT *)p_last_content->p_next;

                p_last_content->p_next = p_content;
            }
            else
                p_body->p_content = p_content;
        }
    }

    return(p_content);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetContentFromBody 
**
** Description      Get a message content from the speicified bMessage body.
**
** Parameters       p_body - Pointer to a bMessage body                   
**                 
** Returns          Pointer to a message content.
**                  NULL if it has not been set.
**
*******************************************************************************/
tBTA_MA_BMSG_CONTENT * BTA_MaBmsgGetContentFromBody(tBTA_MA_BMSG_BODY * p_body)
{
    return(p_body ? p_body->p_content : NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetNextContent 
**
** Description      Get the next message content from the speicified message content.
**
** Parameters       p_content - Pointer to a message content                  
**                 
** Returns          Pointer to a message content.
**                  NULL if it has not been set.
**
*******************************************************************************/
tBTA_MA_BMSG_CONTENT * BTA_MaBmsgGetNextContent(tBTA_MA_BMSG_CONTENT * p_content)
{
    return(p_content ? (tBTA_MA_BMSG_CONTENT *)p_content->p_next : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBmsgAddMsgContent 
**
** Description      Add a text string to the speicified message content.
**
** Parameters       p_content - Pointer to a message content   
**                  p_text - Pointer to a text string                 
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgAddMsgContent(tBTA_MA_BMSG_CONTENT * p_content, char * p_text)
{
    tBTA_MA_BMSG_MESSAGE * p_message;

    if ( p_content )
    {
        p_message = (tBTA_MA_BMSG_MESSAGE *)bta_ma_bmsg_alloc(sizeof(tBTA_MA_BMSG_MESSAGE));
        if ( p_message )
        {
            memset(p_message, 0, sizeof(tBTA_MA_BMSG_MESSAGE));

            p_message->p_text = (char *)bta_ma_bmsg_alloc(strlen(p_text)+1);

            if ( p_message->p_text )
            {
                BCM_STRNCPY_S(p_message->p_text, strlen(p_text)+1, p_text, strlen(p_text)+1);

                /* if the content already points to a message,
                ** then we tack it on the end of the message list.
                */
                if ( p_content->p_message && p_content->p_last )
                    p_content->p_last->p_next = p_message;
                else
                    p_content->p_message = p_message;

                /* keep track of the last message text we added */
                p_content->p_last = p_message;
            }
            else
                bta_ma_bmsg_free(p_message);
        }
    }
}

/*******************************************************************************
**
** Function         BTA_MaBmsgGetMsgContent 
**
** Description      Get the first text string from the speicified message content.
**
** Parameters       p_content - Pointer to a message content   
**                 
** Returns          Pointer to the first text string.
**                  NULL if it has not been set.    
**
*******************************************************************************/
char * BTA_MaBmsgGetMsgContent(tBTA_MA_BMSG_CONTENT * p_content)
{
    char * p_text = NULL;
    if ( p_content && p_content->p_message )
    {
        /* reset 'last' pointer for when 'get_next' is called. */
        p_content->p_last = p_content->p_message;

        p_text = p_content->p_message->p_text;
    }

    return( p_text );
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetMsgContent 
**
** Description      Get the next text string from the speicified message content.
**
** Parameters       p_content - Pointer to a message content   
**                 
** Returns          Pointer to the next text string.
**                  NULL if it has not been set.    
**
*******************************************************************************/
char * BTA_MaBmsgGetNextMsgContent(tBTA_MA_BMSG_CONTENT * p_content)
{
    char * p_text = NULL;

    if ( p_content && p_content->p_last )
    {
        /* advance pointer */
        p_content->p_last = ( tBTA_MA_BMSG_MESSAGE *)p_content->p_last->p_next;

        if ( p_content->p_last )
            p_text = p_content->p_last->p_text;
    }

    return( p_text );
}


/*******************************************************************************
**
** Function         BTA_MaBmsgGetNextVcard 
**
** Description      Get the next vCard from the speicified vCard.
**
** Parameters       p_vcard - Pointer to a vCard   
**                 
** Returns          Pointer to the next vCard.
**                  NULL if it has not been set.    
**
*******************************************************************************/
tBTA_MA_BMSG_VCARD * BTA_MaBmsgGetNextVcard(tBTA_MA_BMSG_VCARD * p_vcard)
{
    return(p_vcard ? (tBTA_MA_BMSG_VCARD *)p_vcard->p_next : NULL);
}


/*******************************************************************************
**
** Function         BTA_MaBmsgSetVcardVersion 
**
** Description      Set the vCard version for the speicified vCard.
**
** Parameters       p_vcard - Pointer to a vCard                
**                  version - vcard version
**                 
** Returns          None
**
*******************************************************************************/
void BTA_MaBmsgSetVcardVersion(tBTA_MA_BMSG_VCARD * p_vcard, tBTA_MA_VCARD_VERSION version)
{
    if ( p_vcard )
        p_vcard->version = version;
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetVcardVersion 
**
** Description      Get the vCard version from the speicified vCard.
**
** Parameters       p_vcard - Pointer to a vCard                   
**                 
** Returns          vCard version number
**
*******************************************************************************/
tBTA_MA_VCARD_VERSION BTA_MaBmsgGetVcardVersion(tBTA_MA_BMSG_VCARD * p_vcard)
{
    return(p_vcard ? p_vcard->version : 0);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgAddVcardProp 
**
** Description      Add a property to the speicified vCard.
**
** Parameters       p_vcard - Pointer to a vCard  
**                  prop - Indicate which vCard peoperty 
**                  p_value - Pointer to the vCard property value
**                  p_param - Pointer to the vCard property parameter
**                 
** Returns          Pointer to the vCard peoperty
**
*******************************************************************************/
tBTA_MA_VCARD_PROPERTY * BTA_MaBmsgAddVcardProp(tBTA_MA_BMSG_VCARD *p_vcard, tBTA_MA_VCARD_PROP prop, char *p_value, char *p_param)
{
    tBTA_MA_VCARD_PROPERTY * p_prop = NULL;
    tBTA_MA_VCARD_PROPERTY * p_last_prop = NULL;

    if ( p_vcard && prop < BTA_MA_VCARD_PROP_MAX )
    {
        p_prop = (tBTA_MA_VCARD_PROPERTY *) bta_ma_bmsg_alloc(sizeof(tBTA_MA_VCARD_PROPERTY));
        if ( p_prop )
        {
            memset(p_prop, 0, sizeof(tBTA_MA_VCARD_PROPERTY));

            /* Set the value (if given) */
            if ( p_value )
            {
                p_prop->p_value = (char *)bta_ma_bmsg_alloc(strlen(p_value)+1);
                if ( p_prop->p_value )
                    BCM_STRNCPY_S(p_prop->p_value, strlen(p_value)+1, p_value, strlen(p_value)+1);
            }

            /* Set the parameter (if given) */
            if ( p_param )
            {
                p_prop->p_param = (char *)bta_ma_bmsg_alloc(strlen(p_param)+1);
                if ( p_prop->p_param )
                    BCM_STRNCPY_S(p_prop->p_param, strlen(p_param)+1, p_param, strlen(p_param)+1);
            }

            /* There can be more than one of a property.  So add this property to the end of the list. */
            if ( p_vcard->p_prop[prop] == NULL )
                p_vcard->p_prop[prop] = p_prop;
            else
            {
                p_last_prop = p_vcard->p_prop[prop];
                while ( p_last_prop->p_next )
                    p_last_prop = (tBTA_MA_VCARD_PROPERTY *)p_last_prop->p_next;

                p_last_prop->p_next = p_prop;
            }
        }
    }

    return(p_prop);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetVcardProp 
**
** Description      Get the vCard property from the speicified vCard peoperty enum.
**
** Parameters       p_vcard - Pointer to a vCard  
**                  prop - Indicate which vCard peoperty 
**                 
** Returns          Pointer to the vCard peoperty.
**                  NULL if the vCard peoperty does not exist
**
*******************************************************************************/
tBTA_MA_VCARD_PROPERTY * BTA_MaBmsgGetVcardProp(tBTA_MA_BMSG_VCARD * p_vcard, tBTA_MA_VCARD_PROP prop)
{
    return( p_vcard && prop < BTA_MA_VCARD_PROP_MAX ? p_vcard->p_prop[prop] : NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetNextVcardProp 
**
** Description      Get the next vCard property from the speicified vCard peoperty.
**
** Parameters       p_prop - Pointer to a vCard property  
**                 
** Returns          Pointer to the next vCard peoperty.
**                  NULL if the next vCard peoperty does not exist
**
*******************************************************************************/
tBTA_MA_VCARD_PROPERTY * BTA_MaBmsgGetNextVcardProp(tBTA_MA_VCARD_PROPERTY * p_prop)
{
    return(p_prop ? (tBTA_MA_VCARD_PROPERTY *)p_prop->p_next : NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetVcardPropValue 
**
** Description      Get the vCard property value from the speicified vCard peoperty.
**
** Parameters       p_prop - Pointer to a vCard property  
**                 
** Returns          Pointer to the vCard peoperty value.
**                  NULL if the vCard peoperty value has not been set.
**
*******************************************************************************/
char * BTA_MaBmsgGetVcardPropValue(tBTA_MA_VCARD_PROPERTY * p_prop)
{
    return(p_prop ? p_prop->p_value : NULL);
}
/*******************************************************************************
**
** Function         BTA_MaBmsgGetVcardPropParam 
**
** Description      Get the vCard property parameter from the speicified vCard peoperty.
**
** Parameters       p_prop - Pointer to a vCard property  
**                 
** Returns          Poiter to the vCard peoperty parameter.
**                  NULL if the vCard peoperty parameter has not been set.
**
*******************************************************************************/
char * BTA_MaBmsgGetVcardPropParam(tBTA_MA_VCARD_PROPERTY * p_prop)
{
    return(p_prop ? p_prop->p_param : NULL);
}

/*******************************************************************************
**
** Function         BTA_MaBuildMapBmsgObj
**
** Description      Builds a specification compliant bMessage object given a
**                  generic bMessage internal structure.
**                             
** Parameters       p_msg - pointer to bMessage object structure (input).
**                  p_stream - Output stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS BTA_MaBuildMapBmsgObj(tBTA_MA_BMSG * p_msg, 
                                     tBTA_MA_STREAM * p_stream)
{
    tBTA_MA_STATUS status = BTA_MA_STATUS_FAIL;


    if ( p_msg && p_stream )
    {
        bta_ma_stream_str(p_stream, "BEGIN:BMSG");
        bta_ma_stream_str(p_stream, "\r\nVERSION:1.0");

        /* Read Status */
        bta_ma_stream_str(p_stream, "\r\nSTATUS:");
        bta_ma_stream_str(p_stream, (BTA_MaBmsgGetReadSts(p_msg) ? "READ" : "UNREAD"));

        /* Type */
        bta_ma_stream_str(p_stream, "\r\nTYPE:");
        bta_ma_stream_str(p_stream, bta_ma_msg_typ_to_string(BTA_MaBmsgGetMsgType(p_msg)));

        /* Folder */
        bta_ma_stream_str(p_stream, "\r\nFOLDER:");
        bta_ma_stream_str(p_stream, BTA_MaBmsgGetFolder(p_msg));

        /* Originator(s) */
        bta_ma_stream_vcards(p_stream, BTA_MaBmsgGetOrigFromBmsg(p_msg));

        /* Envelopes (nested) */
        bta_ma_stream_envelopes(p_stream, BTA_MaBmsgGetEnv(p_msg));

        bta_ma_stream_str(p_stream, "\r\nEND:BMSG\r\n");

        /* if buffer overflowed then return failure */
        status = bta_ma_stream_ok(p_stream) 
                 ? BTA_MA_STATUS_OK
                 : BTA_MA_STATUS_FAIL;
    }
#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("BTA_MA_STATUS =%d (0-OK) BTA_MaBuildMapBmsgObj", status);
#endif
    return( status );
}
/*******************************************************************************
**
** Function         bta_ma_parse_map_bmsg_obj
**
** Description      Parses a bMessage object from a stream into a generic
**                  bMessage internal structure.
**                             
** Parameters       p_msg - pointer to bMessage object structure (output).
**                  p_stream - Input stream.
**
** Returns          BTA_MA_STATUS_OK if successful.  BTA_MA_STATUS_FAIL if not.
**
*******************************************************************************/
tBTA_MA_STATUS BTA_MaParseMapBmsgObj(tBTA_MA_BMSG * p_msg,
                                     tBTA_MA_STREAM * p_stream)
{
    char sz[BTA_MA_MAX_SIZE];
    UINT32 prop_mask = 0;
    tBTA_MA_STATUS status = BTA_MA_STATUS_FAIL;
    tBTA_MA_BMSG_VCARD * p_vcard = NULL;
    tBTA_MA_BMSG_ENVELOPE * p_envelope = NULL;
    tBTA_MA_MSG_TYPE msg_type = 0;

    APPL_TRACE_EVENT0("BTA_MaParseMapBmsgObj");
    if ( p_msg && p_stream )
    {
        /* Must start with BEGIN:MSG */
        if ( bta_ma_get_tag(p_stream, sz, BTA_MA_MAX_SIZE)
             && (strcmp(sz, "BEGIN") == 0)
             && bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE)
             && (strcmp(sz, "BMSG") == 0) )
        {
            while ( bta_ma_get_tag(p_stream, sz, BTA_MA_MAX_SIZE) )
            {
                /* VERSION */
                if ( strcmp(sz, "VERSION") == 0 )
                {
                    if ( bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE) 
                         && (strcmp(sz, "1.0") == 0) )
                    {
                        prop_mask |= BTA_MA_PROP_VERSION;
                    }
                    else
                        break;  /* incorrect VERSION */
                }
                else if ( strcmp(sz, "STATUS") == 0 )
                {
                    bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                    BTA_MaBmsgSetReadSts(p_msg, (BOOLEAN) (strcmp(sz, "READ") == 0));
                }
                else if ( strcmp(sz, "TYPE") == 0 )
                {
                    bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                    if ( bta_ma_str_to_msg_typ(sz, &msg_type) )
                        BTA_MaBmsgSetMsgType(p_msg, msg_type);

                }
                else if ( strcmp(sz, "FOLDER") == 0 )
                {
                    bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                    BTA_MaBmsgSetFolder(p_msg, sz);
                }
                else if ( strcmp(sz, "BEGIN") == 0 )
                {
                    bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);

                    if ( strcmp(sz, "VCARD") == 0 )
                    {
                        p_vcard = BTA_MaBmsgAddOrigToBmsg(p_msg);
                        bta_ma_parse_vcard(p_vcard, p_stream);
                    }
                    else if ( strcmp(sz, "BENV") == 0 )
                    {
                        p_envelope = BTA_MaBmsgAddEnvToBmsg(p_msg);
                        bta_ma_parse_envelope(p_envelope, p_stream);
                    }
                    else
                    {
                        APPL_TRACE_ERROR1("bta_ma_parse_map_bmsg_obj - Invalid BEGIN: '%s'", sz);
                    }
                }
                else if ( strcmp(sz, "END") == 0 )
                {
                    bta_ma_get_value(p_stream, sz, BTA_MA_MAX_SIZE);
                    status = BTA_MA_STATUS_OK;
                    break;
                }
                else
                {
                    APPL_TRACE_ERROR1("bta_ma_parse_map_bmsg_obj - Invalid tag: '%s'", sz);
                }
            }
        }
    }

#if BTA_MSE_DEBUG == TRUE
    APPL_TRACE_EVENT1("BTA_MaParseMapBmsgObj status=%d(0-OK)", status);
#endif
    return( status );
}

/*******************************************************************************
**
** Function         BTA_MaInitMemStream
**
** Description      Initializes a memory based stream
**
** Parameters       p_stream - pointer to stream information.
**                  p_buffer - pointer to buffer to be manipulated.
**                  size - size of buffer pointed to by 'p_buffer'.
**
** Returns          TRUE if stream is opened.
**
*******************************************************************************/
BOOLEAN BTA_MaInitMemStream(tBTA_MA_STREAM * p_stream, 
                            UINT8 * p_buffer, 
                            UINT16 size)
{
    if ( !p_stream )
        return(FALSE);

    memset(p_stream, 0, sizeof(tBTA_MA_STREAM));

    p_stream->type = STRM_TYPE_MEMORY;
    p_stream->u.mem.p_buffer = p_stream->u.mem.p_next = p_buffer;
    p_stream->u.mem.size = size;

    return(TRUE);
}

/*******************************************************************************
**
** Function         BTA_MaInitFileStream
**
** Description      Initializes a file stream
**
** Parameters       p_stream - pointer to stream information.
**                  p_path - Full pathname to file to use.
**                  oflags - permissions and mode
**
** Returns          TRUE if file stream is opened.
**
*******************************************************************************/
BOOLEAN BTA_MaInitFileStream(tBTA_MA_STREAM * p_stream, 
                             const char *p_path, 
                             int oflags)
{
    if ( !p_stream )
    {
        APPL_TRACE_ERROR0("Invalid stream pointer");
        return(FALSE);
    }

    memset(p_stream, 0, sizeof(tBTA_MA_STREAM));

    p_stream->type = STRM_TYPE_FILE;

    p_stream->u.file.fd = bta_ma_co_open(p_path, oflags);

    if ( p_stream->u.file.fd == -1 )
    {
        APPL_TRACE_ERROR0("Unable to open file ");
        p_stream->status = STRM_ERROR_FILE;
    }

    /* return TRUE if stream is OK */
    return(p_stream->status == STRM_SUCCESS);
}

/*******************************************************************************
**
** Function         BTA_MaCloseStream
**
** Description      Close a stream (do any necessary clean-up).
**
** Parameters       p_stream - pointer to stream information.
**
** Returns          void
**
*******************************************************************************/
void BTA_MaCloseStream(tBTA_MA_STREAM * p_stream)
{
    if ( p_stream->type == STRM_TYPE_FILE )
    {
        bta_ma_co_close(p_stream->u.file.fd);
        p_stream->u.file.fd = BTA_FS_INVALID_FD;
    }
}
#endif /* BTA_MSE_INCLUDED */
