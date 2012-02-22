/******************************************************************************
 **                                                                           
 **  Name:          xml_flp.c                                       
 **                                                                           
 **  Description:   This module contains xml parser of obex folder listing  
 **                                                                           
 **  Copyright (c) 2004-2011, Broadcom Corporation, All Rights Reserved.      
 **  Broadcom Bluetooth Core. Proprietary and confidential.                   
 ******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "xml_flp_api.h"

#include <string.h>
#include <stdlib.h>

#ifndef FOLDER_DEBUG_XML
#define FOLDER_DEBUG_XML FALSE
#endif
#define FOLDER_DEBUG_LEN 50
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
#define XML_TRACE_DEBUG0(m)                     {BT_TRACE_0(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m);}
#define XML_TRACE_DEBUG1(m,p1)                  {BT_TRACE_1(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1);}
#define XML_TRACE_DEBUG2(m,p1,p2)               {BT_TRACE_2(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1,p2);}
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            {BT_TRACE_3(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1,p2,p3);}
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         {BT_TRACE_4(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4);}
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      {BT_TRACE_5(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5);}
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   {BT_TRACE_6(TRACE_LAYER_FTP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5,p6);}
#else
#define XML_TRACE_DEBUG0(m)                     
#define XML_TRACE_DEBUG1(m,p1)                  
#define XML_TRACE_DEBUG2(m,p1,p2)               
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   
#endif

#define XML_PERM_LEN_MAX    4

/*****************************************************************************
**   Constants
*****************************************************************************/

const UINT8 xml_folder_listing_elem[]  = "folder-listing";
const UINT8 xml_file_elem[]            = "file";
const UINT8 xml_folder_elem[]          = "folder";
const UINT8 xml_parent_folder_elem[]   = "parent-folder";
const UINT8 xml_name_attr[]            = "name";
const UINT8 xml_size_attr[]            = "size";
const UINT8 xml_type_attr[]            = "type";
const UINT8 xml_modified_attr[]        = "modified";
const UINT8 xml_created_attr[]         = "created";
const UINT8 xml_accessed_attr[]        = "accessed";
const UINT8 xml_user_perm_attr[]       = "user-perm";
const UINT8 xml_group_perm_attr[]      = "group-perm";
const UINT8 xml_other_perm_attr[]      = "other-perm";
const UINT8 xml_group_attr[]           = "group";
const UINT8 xml_owner_attr[]           = "owner";
const UINT8 xml_version_attr[]         = "version";
const UINT8 xml_lang_attr[]            = "xml:lang";
const UINT8 xml_unknown[]              = "unknown";

#define XML_FOLDER_LISTING_ELEM_ID   0x01
#define XML_FILE_ELEM_ID             0x02
#define XML_FOLDER_ELEM_ID           0x03
#define XML_PARENT_FOLDER_ELEM_ID    0x04
#define XML_MAX_OBJ_TAG_ID           XML_FOLDER_LISTING_ELEM_ID
#define XML_NAME_ATTR_ID             0x05
#define XML_SIZE_ATTR_ID             0x06
#define XML_TYPE_ATTR_ID             0x07
#define XML_MODIFIED_ATTR_ID         0x08
#define XML_CREATED_ATTR_ID          0x09
#define XML_ACCESSED_ATTR_ID         0x0a
#define XML_USER_PERM_ATTR_ID        0x0b
#define XML_GROUP_PERM_ATTR_ID       0x0c
#define XML_OTHER_PERM_ATTR_ID       0x0d
#define XML_GROUP_ATTR_ID            0x0e
#define XML_OWNER_ATTR_ID            0x0f
#define XML_VERSION_ATTR_ID          0x10
#define XML_LANG_ATTR_ID             0x11
#define XML_XP_UNKNOWN_ID            0x12
#define XML_FOLDER_MAX_ID            0x13   /* keep in sync with above */
#define XML_FOLDER_TAG_END_ID        0x13   /* closing tag found end=true */
#define XML_FOLDER_PAUSE_ID          0x14   /* closing tag found end=false */


#define XML_FOLDER_TTBL_SIZE (XML_FOLDER_MAX_ID+1)

/*****************************************************************************
**   Type Definitions
*****************************************************************************/

typedef struct
{
    const UINT8 *p_name;
    UINT8        len;
} tXML_FOLDER_TTBL_ELEM;

typedef tXML_FOLDER_TTBL_ELEM tXML_FOLDER_TTBL[];              /* Tag Table */


static const tXML_FOLDER_TTBL_ELEM xml_flp_ttbl[XML_FOLDER_TTBL_SIZE] = 
{                      /* index (FOLDER_XP_*_ID) & name                 */
    {(UINT8*) "",  XML_FOLDER_MAX_ID-1}, /* \x00  Number of elements in array */
    /* XML FOLDER element (TAG) name */
    {xml_folder_listing_elem, 14},  /* x01 folder-listing */
    {xml_file_elem, 4},  /* 0x02 file */
    {xml_folder_elem, 6},  /* x03 folder */
    {xml_parent_folder_elem, 13},  /* x04 parent-folder */
    {xml_name_attr, 4},  /* x05 name */
    {xml_size_attr, 4},  /* x06 size */
    {xml_type_attr, 4},  /* x07 type */
    {xml_modified_attr, 8},  /* x08 modified */
    {xml_created_attr, 7},  /* x09 created */
    {xml_accessed_attr, 8},  /* x0a accessed */
    {xml_user_perm_attr, 9},  /* x0b user-perm */
    {xml_group_perm_attr, 10},  /* x0c group-perm */
    {xml_other_perm_attr, 10},  /* x0d other-perm */
    {xml_group_attr, 5},  /* x0e group */
    {xml_owner_attr, 5},  /* x0f owner */
    {xml_version_attr, 7},  /* x10 version */
    {xml_lang_attr, 8},  /* x11 xml:lang */
    {xml_unknown, 7 }    /* x12 unknown */
};

#define XML_FOLDER_PTBL_SIZE 0x10
typedef UINT8 * tXML_FOLDER_PTBL_ELEM;

static const tXML_FOLDER_PTBL_ELEM xml_flp_ptbl[XML_FOLDER_PTBL_SIZE] = 
{
    (UINT8 *) "\x01\x02\x03\x04",  /* index x00, all valide attributes in above list */
    (UINT8 *) "\x10\x02\x03\x04",  /* x01 attributes and sub-tags supported */
    (UINT8 *) "\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x11", /* x02: file attributes */
    (UINT8 *) "\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x11", /* x03: folder attributes */
    (UINT8 *) "" /* x04 parent folder has no attributes */
};


#if (FOLDER_DEBUG_XML == TRUE)
void xml_flp_debug_str(tXML_STR *p_str, UINT8 *p_buf)
{
    int          dbg_len;
    if ( (p_str == NULL) || (NULL==p_str->p))
        BCM_STRCPY_S( (char *)p_buf, FOLDER_DEBUG_LEN, "(NULL)" );
    else
    {
        dbg_len = p_str->len;
        if ( dbg_len >= FOLDER_DEBUG_LEN)
            dbg_len = FOLDER_DEBUG_LEN - 1;
        BCM_STRNCPY_S( (char *)p_buf, FOLDER_DEBUG_LEN, (char *)p_str->p, dbg_len);
        p_buf[dbg_len] = 0;
    }
}

#else
#define xml_flp_debug_str(p_str, p_buf)
#endif

/*****************************************************************************
 ** Function     xml_flp_proc_tag
 ** Description
 ** Parameters   -
 ** Returns
 *****************************************************************************/
static UINT8 xml_flp_proc_tag( tXML_FOLDER_STATE *p_st, tXML_STR *p_name, tXML_STACK *p_stk)
{
    const UINT8 *p_stag;                        /* sub tag pointer */
    UINT8        curr = p_stk->stack[p_stk->top];
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
    UINT8        dbg_name[FOLDER_DEBUG_LEN];
#endif

    if (curr < XML_FOLDER_PTBL_SIZE)
    {
        /* Iterate over allowed sub-tags for the current tag. */
        for (p_stag = xml_flp_ptbl[curr]; p_stag && *p_stag ; p_stag++)
        {
            if (*p_stag >= XML_FOLDER_TTBL_SIZE)
                continue;
            if(p_name->len == xml_flp_ttbl[*p_stag].len &&
               strncmp((char *)p_name->p, (char *)xml_flp_ttbl[*p_stag].p_name, p_name->len) == 0)
            {
                p_stk->top++;
                p_stk->stack[p_stk->top] = *p_stag;

                return *p_stag;
            }
        }
    }

#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
    xml_flp_debug_str(p_name, dbg_name);
    XML_TRACE_DEBUG1("xml_flp_proc_tag: bad name:%s", dbg_name );
#endif

    p_stk->top++;
    p_stk->stack[p_stk->top] = XML_XP_UNKNOWN_ID;
    return XML_XP_UNKNOWN_ID;
}


/*****************************************************************************
 ** Function     xml_flp_proc_attr
 ** Description
 ** Parameters   -
 ** Returns
 *****************************************************************************/
static UINT8 xml_flp_proc_attr(tXML_FOLDER_STATE *p_st, tXML_STR *p_name, tXML_STACK *p_stk)
{
    const UINT8 *p_stag;                        /* sub tag pointer */
    UINT8       curr = p_stk->stack[p_stk->top];
#if (FOLDER_DEBUG_XML == TRUE)
    UINT8       dbg_name[FOLDER_DEBUG_LEN];
#endif

    if (curr < XML_FOLDER_PTBL_SIZE)
    {
        /* Iterate over allowed sub-tags for the current tag. */
        for (p_stag = xml_flp_ptbl[curr]; p_stag && *p_stag; p_stag++)
        {
            if (*p_stag >= XML_FOLDER_TTBL_SIZE)
                continue;
            if(p_name->len == xml_flp_ttbl[*p_stag].len &&
               strncmp((char *)p_name->p, (char *)xml_flp_ttbl[*p_stag].p_name, p_name->len) == 0)
            { 
                p_stk->top++;
                p_stk->stack[p_stk->top] = *p_stag;

                return *p_stag;
            }
        }
    }

#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
    xml_flp_debug_str(p_name, dbg_name);
    XML_TRACE_DEBUG1("xml_flp_proc_attr: bad name:%s", dbg_name);
#endif
    return XML_XP_UNKNOWN_ID;
}
/*****************************************************************************
 ** Function         xml_flp_get_perm
 ** Description      Translate permission character into XML_PERM_MASK
 ** Returns          XML_PERM_MASK: permission mask   
 *****************************************************************************/
static XML_PERM_MASK xml_flp_get_perm(char *right, UINT16 len )
{
    XML_PERM_MASK   mask = 0;
    UINT8   perm_str[XML_PERM_LEN_MAX] = {0};

    memcpy(perm_str, right, XML_PERM_LEN_MAX);
    perm_str[len] = '\0';

    if (strchr((char *)perm_str, 'R')) 
        mask |= XML_PERM_READ_B;
    if (strchr((char *)perm_str, 'W'))   
        mask |= XML_PERM_WRITE_B;
    if (strchr((char *)perm_str, 'D')) 
        mask |= XML_PERM_DELETE_B;
    
#if (FOLDER_DEBUG_XML == TRUE)
    XML_TRACE_DEBUG2("xml_flp_get_perm [%s] -> %d",perm_str, mask);
#endif
    return mask;
}

/*****************************************************************************
 ** Function         xml_flp_find_ch_n_copy
 ** Description      copy any chacter till one char in p_str. Any char in p_str
 **                  will stop copy pointed by p_begin
 ** Parameters
 ** Returns
 *****************************************************************************/
static void xml_flp_find_ch_n_copy( tXML_MCOPY *p_copy )
{
    const UINT8   *p_tmp;
    const UINT8   *p_str  = (UINT8 *)">";     /* was: ":=/> \t\n\r". i think we should copy till 
                                                 closing flag */
    unsigned int   last   = XML_FOLDER_CARRY_OVER_LEN;     /* maximum carry over len we can support */
    UINT8         *p_last = p_copy->last.p + p_copy->last.len - 1; /* point to the last char of carry
                                                                      over buffer */
    BOOLEAN        found  = FALSE;

    /* check if the last char in p_last is in p_str */
    for (p_tmp = p_str; *p_tmp; p_tmp++)
    {
        if (*p_last == *p_tmp)
        {
            found = TRUE;
            break;
        }
    } /* for */

    if (found == FALSE)
    {
        /* if not in p_str, move chars from p_begin to p_last
         * until reached last_len or any char in p_str */
        p_last++;
        last -= p_copy->last.len;       /* calculate the maximum number of chars we can copy */
        while (*(p_copy->p_begin) && last) /* rl: not sure that this buffer is termninated by a 0 */
        {
            /* copy from source (new buffer) to carry over. adjust only carry over ptr. */
            *p_last++ = *p_copy->p_begin;
            last--;
            for (p_tmp = p_str; *p_tmp; p_tmp++)
            {
                if (*p_copy->p_begin == *p_tmp)
                {
                    p_copy->p_begin++;  /* adjust pointer to point to next char to read */
                    /* calculate new length of carry over buffer contents */
                    p_copy->last.len = XML_FOLDER_CARRY_OVER_LEN-last;
                    *p_last = 0;        /* NULL terminate p_last. rl: not really neccessary. */
                    return;
                }
            } /* for */
            p_copy->p_begin++;  /* update now to next char. this way abort char is also copied */
        } /* while */
    } /* !found */
}

/*****************************************************************************
** Function         xml_folder_cback
** Description      
**
** Parameters
** Returns
*****************************************************************************/
static BOOLEAN xml_folder_cback( tXML_EVENT event, void *p_event_data, void *p_usr_data)
{
    tXML_MEVT_DATA      *p_ed = (tXML_MEVT_DATA*) p_event_data;
    tXML_FOLDER_STATE  *p_st = (tXML_FOLDER_STATE *) p_usr_data;
    tXML_PROP           *p_cp = &p_st->p_prop[p_st->prop_index];
    BOOLEAN              ret = TRUE;
    UINT8                next;  /* next element */
    UINT8                curr = p_ed->stack.stack[p_ed->stack.top]; /* current element */
#if (FOLDER_DEBUG_XML == TRUE)
    UINT8           dbg_name[FOLDER_DEBUG_LEN];
    UINT8           dbg_prefix[FOLDER_DEBUG_LEN];
    UINT8           dbg_value[FOLDER_DEBUG_LEN];
#endif

#if (FOLDER_DEBUG_XML == TRUE)
    XML_TRACE_DEBUG1("xml_folder_cback:%d", event);
#endif

    switch (event)
    {
    case XML_TAG :  /* <tag-name */
        next = xml_flp_proc_tag(p_st, &p_ed->tag.name, &p_ed->stack);
#if (FOLDER_DEBUG_XML == TRUE)
        xml_flp_debug_str(&p_ed->tag.name, dbg_name);
        xml_flp_debug_str(&p_ed->tag.prefix, dbg_prefix);
        XML_TRACE_DEBUG2("XML_TAG: p:%s, name:%s", dbg_prefix, dbg_name);
        XML_TRACE_DEBUG2("top:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);

#endif
        if (next != 0)
        {
            if (next <= XML_MAX_OBJ_TAG_ID)
                p_st->obj = next;

            if(p_st->prop_index <p_st->max_num_prop)
            {
                /* we do not use prefix in FTC */
                p_cp->name   = next;
                p_cp->p_data = NULL;
                p_cp->len    = 0;
                p_cp->level  = p_ed->stack.top;
                p_st->prop_index++;
            }
            else
                ret = FALSE;
         }
        break;

    case XML_ATTRIBUTE : /* attr-name="attr-value" */
        curr = xml_flp_proc_attr(p_st, &p_ed->attr.name, &p_ed->stack);
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        xml_flp_debug_str(&p_ed->attr.name, dbg_name);
        xml_flp_debug_str(&p_ed->attr.prefix, dbg_prefix);
        xml_flp_debug_str(&p_ed->attr.value, dbg_value);
        XML_TRACE_DEBUG4("[xml folder] XML_ATTRIBUTE: p:%s, name:%s, v:%s, curr:%x",
                          dbg_prefix, dbg_name, dbg_value, curr);
#endif
        if ((curr != 0) && (curr != XML_XP_UNKNOWN_ID))
        {
            if(p_st->prop_index <p_st->max_num_prop)
            {
                p_cp->name   = curr;
                p_cp->p_data = p_ed->attr.value.p;
                p_cp->len    = p_ed->attr.value.len;
                p_cp->level  = p_ed->stack.top;
                p_st->prop_index++;
            }
            else
                ret = FALSE;
            p_ed->stack.top--;
        }
        break;

    case XML_CHARDATA :
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        xml_flp_debug_str(&p_ed->ch_data.value, dbg_value);
        XML_TRACE_DEBUG2("XML_CHARDATA: v:%s, last:%d", dbg_value, p_ed->ch_data.last);
#endif
        break;

    case XML_ETAG : /* </tag-name> */
        if(p_ed->stack.top > 0)
        {
            p_ed->stack.stack[p_ed->stack.top] = 0;
            p_ed->stack.top--;
            p_st->ended = (BOOLEAN) (p_ed->stack.top == 0);
        }
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        xml_flp_debug_str(&p_ed->etag.name, dbg_name);
        xml_flp_debug_str(&p_ed->etag.prefix, dbg_prefix);
        XML_TRACE_DEBUG2("[xml folder] XML_ETAG: p:%s, name:%s", dbg_prefix, dbg_name);
        XML_TRACE_DEBUG2("[xml folder] top:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);
#endif
        break;

    case XML_TAG_END: /* /> */
        curr = p_ed->stack.stack[p_ed->stack.top];

        if(p_st->prop_index <p_st->max_num_prop)
        {
            if(p_ed->empty_elem.end)
                p_cp->name   = XML_FOLDER_TAG_END_ID;
            else
                p_cp->name   = XML_FOLDER_PAUSE_ID;
            p_cp->p_data = NULL;
            p_cp->len    = 0;
            p_cp->level  = p_ed->stack.top;
            p_st->prop_index++;
            if(p_ed->empty_elem.end && p_ed->stack.top > 0)
            {
                p_ed->stack.top--;
            }
        }
        else
            ret = FALSE;

#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG4("[xml folder] XML_TAG_END: %d, top:x%x, stk:x%x, curr:x%x",
                          p_ed->empty_elem.end, p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top], curr);
#endif
        break;

    case XML_PARTIAL:
        if(p_st->p_prop[p_st->prop_index-1].name != XML_FOLDER_TAG_END_ID)
        {      
            p_ed->stack.top--;
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG0("[xml folder] adjust due to XML_PARTIAL");
#endif
        }
        break;

    case XML_COPY:
        xml_flp_find_ch_n_copy( &p_ed->copy );
        XML_TRACE_DEBUG1("[xml folder] XML_COPY: %s", p_ed->copy.last.p);
        break;

    default :
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG1("[xml folder] XML event: %d", event);
#endif
        break;
    }

    return ret;
}

/**********************************************************************************
** Function         xml_flp_int_fill_file_folder
** Description      fill in file/folder structure. 
**
** Parameters
** Returns          xx:   > 0 : number of properties scanned, folder entry is added
**                        = 0 : no end tag found, carry over to next parse
**                        = -1: no dst_resource avaibale, all prop left to next parse
**                        = -2: exceed max entry, no folder entry added
**********************************************************************************/
static INT16 xml_flp_int_fill_file_folder( const UINT8 type,
                                       tXML_FOLDER_STATE * p_xud,
                                       tXML_PROP *p_prop,
                                       UINT16 *num_prop,
                                       UINT8  **p_dst_data,
                                       UINT16 *dst_len)
{
    INT16       xx;
    UINT16      len;
    BOOLEAN     end = FALSE;
    tXML_PROP   *cur_prop = p_prop;
    UINT8       *p_cur_offset = *p_dst_data;

    for(xx=0; (xx < *num_prop) && !end; xx++, cur_prop++)
    {
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG5( "[xml folder] fill: num_prop:%d, name id: x%x, p_prop: x%x, p_data:%s, len: %d",
                           (*num_prop-xx) , cur_prop->name, cur_prop, cur_prop->p_data, cur_prop->len);
#endif
        switch (cur_prop->name)
        {
        case XML_NAME_ATTR_ID:
            if ( p_xud->current_entry < p_xud->max_entry )
            {
                /* as long as we do not exceed the number of entries in the ouput array copy name */
                p_xud->p_entry[p_xud->current_entry].type = type;
                /* calculate the max length to copy */
                len = (cur_prop->len<=p_xud->max_name_len) ? cur_prop->len:p_xud->max_name_len;
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {
                    p_xud->p_entry[p_xud->current_entry].data = p_cur_offset;
                    p_xud->p_entry[p_xud->current_entry].len = len;

                    memcpy( (void *)p_xud->p_entry[p_xud->current_entry].data,
                        (const void *)cur_prop->p_data,
                        len );                   
                    p_xud->p_entry[p_xud->current_entry].data[len] = 0;    /* null terminate string */
                    p_cur_offset += (len + 1);
                    *dst_len = *dst_len - (len + 1) ;

#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
                    XML_TRACE_DEBUG3("[xml folder]: catch filename [%s] len [%d] dst_len left:[%d]", 
                                     p_xud->p_entry[p_xud->current_entry].data,
                                     len, 
                                     *dst_len);
#endif
                }
                else    /* run out of dst buffer resource */
                {   
                    xx = -1;
                    return xx;
                }
            }
            else /* exceed max entry */
                return -2;

            break;

        case XML_SIZE_ATTR_ID:
            if ( p_xud->current_entry < p_xud->max_entry )
            {
                p_xud->p_entry[p_xud->current_entry].size = atol( (const char *)cur_prop->p_data );
            }
            break;

        case XML_USER_PERM_ATTR_ID:
            if ( p_xud->current_entry < p_xud->max_entry )
            {
                p_xud->p_entry[p_xud->current_entry].user_perm = xml_flp_get_perm( (char *)cur_prop->p_data, cur_prop->len);
            }
            break;
        case XML_TYPE_ATTR_ID:
        case XML_MODIFIED_ATTR_ID:
        case XML_CREATED_ATTR_ID:
        case XML_ACCESSED_ATTR_ID:
        case XML_GROUP_PERM_ATTR_ID:
        case XML_OTHER_PERM_ATTR_ID:
        case XML_GROUP_ATTR_ID:
        case XML_OWNER_ATTR_ID:
        case XML_LANG_ATTR_ID:
           
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG1( "[xml folder] ignored attr: 0x%x", cur_prop->name );
#endif
            break;

        case XML_FOLDER_TAG_END_ID:
            /* -------------------- CUSTOMER SPECIFIC ---------------------- */
            p_xud->current_entry++;     /* increment only when end tag (/>) found */

#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG1( "[xml folder] FOUND END TAG: 0x%x", cur_prop->name );
#endif

            end = TRUE;
            break;

        default:
            XML_TRACE_DEBUG1("[xml folder] unknown attrib: %d", cur_prop->name );
            break;
        }
    }
#if (FOLDER_DEBUG_XML == TRUE)
    XML_TRACE_DEBUG2("[xml folder] fill_file_folder: end:%d, xx:%d", end, xx);
#endif
#if 0
    /* if end tag not found -> split over two buffers. but parser will still show rest of 
       found properties. so return still found properties. */
    if(end == FALSE)
        xx = 0;
#endif

    /* keep track of current data buffer offset */
    *p_dst_data = p_cur_offset;
    return xx;
} /* xml_flp_int_fill_file_folder() */


/**********************************************************************************
** Function         xml_flp_int_fill_evt_data
**
** Description      fill in file/folder structure. 
**
** Parameters
** Returns
**********************************************************************************/
static tXML_FOLDER_RES xml_flp_int_fill_evt_data( UINT8 op,
                                              void *p_evt_data,
                                              tXML_PROP **p_prop,
                                              UINT16 *num_prop,
                                              UINT8 **p_dst_data,
                                              UINT16 *dst_len)
{
    tXML_FOLDER_STATE  *p_xud = (tXML_FOLDER_STATE *)p_evt_data;
    INT16              inc_prop;
    UINT8               prop_name;               /* Property name. */
    UINT8               entry_type;
    tXML_PROP           *cur_prop = *p_prop;
    UINT8               *p_cur_offset = *p_dst_data;

    tXML_FOLDER_RES     x_res = XML_FOLDER_OK;
    BOOLEAN             x_no_res = FALSE;       /* guard dest buffer resource */
    

    if ( op == 0 || op > XML_MAX_OBJ_TAG_ID || *num_prop == 0)
        return XML_FOLDER_ERROR;

#if FOLDER_DEBUG_XML
    XML_TRACE_DEBUG2( "[xml folder] xml_flp_int_fill_evt_data op:%d, num_prop:%d",
                       op, *num_prop);
#endif



    while ( *num_prop > 0 && !x_no_res )
    {
#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG2("end_prop:%d, name id: x%x", *num_prop, cur_prop->name);
#endif
        prop_name = cur_prop->name;
        cur_prop++;
        *num_prop -= 1;


        switch( prop_name )
        {
        case XML_FOLDER_LISTING_ELEM_ID:
            /* skip over version attribute which should always be 1.0. this is the top level */
            break;

        case XML_FILE_ELEM_ID:
        case XML_FOLDER_ELEM_ID:
            /* folder or file: only type is the difference */
            entry_type = (XML_FILE_ELEM_ID==prop_name)?XML_OBX_FILE:XML_OBX_FOLDER;
            inc_prop = xml_flp_int_fill_file_folder( entry_type,
                                                    p_xud,
                                                    cur_prop,
                                                    num_prop,
                                                    &p_cur_offset,
                                                    dst_len);
            if (inc_prop == -1) /* no dst_buf available */
            {
                /* backup one more prop to obtain the skipped folder/file entry header */
                cur_prop --; 
                *num_prop += 1;

                x_res = XML_FOLDER_DST_NO_RES;
                x_no_res = TRUE;
            }
            else  if (inc_prop == -2) /* exceed max entry */
            {
                x_no_res = TRUE;
                x_res = XML_FOLDER_OUT_FULL;
            }
            else    /* found folder entry */
            {
                cur_prop    += inc_prop;
                *num_prop -= inc_prop;
            }
            break;

        case XML_PARENT_FOLDER_ELEM_ID:
            if ( p_xud->current_entry < p_xud->max_entry )
            {
                /* set type of entry (file, folder) */
                p_xud->p_entry[p_xud->current_entry].type = XML_OBX_FOLDER;
                /* copy folder name if dst buffer is big enough, parent folder as ".." */
                if ((*dst_len - strlen(XML_PARENT_FOLDER)) > 0)
                {
                    p_xud->p_entry[p_xud->current_entry].data = p_cur_offset;
                    p_xud->p_entry[p_xud->current_entry].len = 2;
                    memcpy( (void *)p_xud->p_entry[p_xud->current_entry].data,
                             (const void *)XML_PARENT_FOLDER, 2);
                    p_xud->p_entry[p_xud->current_entry].data[2] = '\0'; /* null terminate */
                    p_xud->current_entry ++;
                    p_cur_offset += 3 ;
                    *dst_len -= 3;
                } 
                else 
                {
                    x_no_res = TRUE;
                    x_res = XML_FOLDER_DST_NO_RES;
                }
            }
            else
            {
                x_no_res = TRUE;
                x_res = XML_FOLDER_OUT_FULL;
            }
            break;

        case XML_FOLDER_PAUSE_ID:
            
#if (FOLDER_DEBUG_XML==TRUE)
            XML_TRACE_DEBUG0( "[xml folder] xml_flp_int_fill_evt_data(): XML_FOLDER_PAUSE_ID" );
#endif
            break;

        case XML_FOLDER_TAG_END_ID:
#if (FOLDER_DEBUG_XML==TRUE)
            XML_TRACE_DEBUG0( "[xml folder] xml_flp_int_fill_evt_data(): XML_FOLDER_TAG_END_ID" );
#endif
            break;

        default:
            XML_TRACE_DEBUG1( "[xml folder] xml_flp_int_fill_evt_data():unknown element: %d", prop_name );
            break;
        }
    }

    /* keep track of current filling position, and current available dst buffer */
    *p_prop = cur_prop;
    *p_dst_data = p_cur_offset;

    return x_res;
} /* xml_flp_int_fill_evt_data() */


/**************************************************************************************
** Function         XML_FolderInit
**
** Description      Initialize xml parser state machine.
**
** Parameters       p_xml_state: address of an xml parser state machine to be initialized, 
**                               allocate an additional space of size XML_FOLDER_CARRY_OVER_LEN 
**                               right after *p_xml_state to hold carry over data.
**                  p_entry    : points start of output directory entry. caller needs do 
**                               free this memory 
**                  max_entry  : max is 16 bit integer value which is the maximum number 
**                               of folder entries.
                    
**
** Returns          void
**************************************************************************************/

void XML_FolderInit( tXML_FOLDER_PARSER  *p_xml_state,
                     tXML_FOLDER_ENTRY    *p_entry,
                     const UINT16           max_entry  )
{
        /* Initialize the generic xml parser state machine.*/
        XML_InitPars( &p_xml_state->xml, xml_folder_cback, &p_xml_state->xml_user_data );

        /* User need to allocate an additional space of size XML_FOLDER_CARRY_OVER_LEN      */
        /* right after *p_xml_state to hold carry over data.                                */
        /* point to the end of the allocated buffer for p_xml_state, which is the           */
        /* beginning of buffer(XML_FOLDER_CARRY_OVER_LEN)                                   */
        p_xml_state->xml.last_bfr.p    = (UINT8 *)(p_xml_state + 1);  
        p_xml_state->xml.last_bfr.len  = XML_FOLDER_CARRY_OVER_LEN;

        /* Initialize user data     */
        p_xml_state->xml_user_data.p_entry = p_entry;   
        p_xml_state->xml_user_data.current_entry = 0;
        p_xml_state->xml_user_data.max_name_len  = XML_UI_ENTRY_MAX_NAME_LEN + 1;
        p_xml_state->xml_user_data.max_entry = (UINT16)max_entry;
        p_xml_state->xml_user_data.prop_num = 0;
}


/**************************************************************************************
** Function         XML_FolderParse
**
** Description      This function is called to parse the xml data received from OBEX 
**                  into folder entries associated with properties value. It can also be 
**                  used as clean up function to delete left over data from previous parse.
**                  This clean up function is typically used when application runs out of 
**                  resource and decides to discard extra xml data received.
**
** Parameters       p_xml_state: pointer to a xml parser initialized by XML_FolderInit().                      
**                  xml_data: valid pointer to OBEX list data in xml format.
**                  xml_len: length of the  package, must be non-zero value.
**                  dst_data: valid pointer to the buffer for holding converted folder entry name.
                              When dst_data is NULL, clean up all remaining data in the parser.
**                  dst_len: pointer to the length of dst_data buffer, its carry out value
**                          is the number of bytes remaining buffer.When dst_len is NULL, 
**                          it will cause to flush the internal data in the parser.
**                  num_entries: current number of entries, in the end it is the total number of entries
**
** Returns          tXML_FOLDER_RES (see xml_flp.h)
**                  XML_PENDING: parsing not completed
**                  XML_END_LIST: found /folder-listing but no final flag detected 
**                  XML_FOLDER_OUT_FULL: reached max_entry -> do not call parser anymore!!! dump data
**                  XML_FOLDER_DST_NO_RES : run out of dst buffer resource while  
**                                          some xml data still remains.

**************************************************************************************/
tXML_FOLDER_RES XML_FolderParse( tXML_FOLDER_PARSER   *p_xml_state,
                                 UINT8 *xml_data,      UINT16 xml_len, 
                                 UINT8 *dst_data,      UINT16 *dst_len,
                                 UINT16               *num_entries )
{
    tXML_OS                 xos;
    BOOLEAN              is_remain = TRUE;
    tXML_MUL_STATE      *p_xml   = &p_xml_state->xml;
    tXML_FOLDER_STATE   *p_st    = &p_xml_state->xml_user_data;
    tXML_FOLDER_RES      x_res   = XML_FOLDER_OK;
    tXML_RESULT          res     = XML_NO_PROP;
    UINT16               max_num_prop = OBX_LRG_DATA_POOL_SIZE/sizeof(tXML_PROP); /* Changed to use the reserved buffer pool */


#if (defined(FOLDER_DEBUG_XML) && FOLDER_DEBUG_XML == TRUE)
    int         xx;
    UINT8       dbg_buf[FOLDER_DEBUG_LEN];
    tXML_STR    str;
#endif

    /* if dst_data is NULL, clean up remaining data */
    if (!dst_data || !dst_len)
    {
        /* clean out remained xml data left over from last parse */
        if (p_xml_state->xml_user_data.p_prop )
        {
            GKI_freebuf( p_st->p_prop );
            p_st->p_prop = p_st->offset_prop = NULL;
            p_st->prop_num = 0;
        }
#if (FOLDER_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG0( "[xml folder] XML_FolderParse() Clean up left over!");
#endif
        return x_res;
    }

    /* if illegal OBEX data or dst buffer pointer received, return ERROR */
    if (xml_len == 0 || !xml_data)
    {
        return XML_FOLDER_ERROR;
    }
    
    /* XML_FolderParse receive new xml data, allocate buffer to hold parsed prop */
    if (p_st->offset_prop == NULL)
    {

#if (FOLDER_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG0( "[xml folder] XML_FolderParse() Receive New Data!");
            XML_TRACE_DEBUG2( "[xml folder] XML_data start[%x] end [%x]",xml_data, xml_data+xml_len);
#endif        
         is_remain = FALSE;
         if ( NULL!= (p_st->p_prop = (tXML_PROP *)GKI_getpoolbuf( OBX_LRG_DATA_POOL_ID ) ) )
         {
                /* pointing next prop to be converted into file entry */
                p_st->prop_num      = 0;                 
         } 
         else 
         {
            GKI_freebuf( p_xml_state );
            x_res = XML_FOLDER_NO_RES;
            return x_res;
         }       
    }
#if (FOLDER_DEBUG_XML == TRUE)
    else 
    {
            XML_TRACE_DEBUG0( "[xml folder] XML_FolderParse(): Keep cleaning up old xml data !");
    }
#endif 
    /* update the data address */
    xos.p_begin        = xml_data;
    xos.p_end          = xml_data + xml_len;

    while( res == XML_NO_PROP )
    {
         /* if no remaining data in p_st->p_prop, parse new xml data */
         if (!is_remain)
         {         
            p_st->max_num_prop = max_num_prop;
            p_st->prop_index     = 0;
            res = XML_MulParse( p_xml, &xos );


#if (FOLDER_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG4( "xml_folder_parse obj: %x, max: %d, num: %d, res: %d",
                               p_st->obj, max_num_prop, p_st->prop_index, res);

            if (res != 0)
            {
                    XML_TRACE_DEBUG1( "XML_MulParse Parsing: %d !!!", res);
            }

            for(xx=0; xx<p_st->prop_index; xx++)
            {
                if ( p_st->p_prop[xx].name < XML_FOLDER_MAX_ID )
                {
                    str.p   = p_st->p_prop[xx].p_data;
                    str.len = p_st->p_prop[xx].len;
                    xml_flp_debug_str(&str, dbg_buf);
                    XML_TRACE_DEBUG5( "[xml folder] parsed: index[%d]:%d:%s: %s(%d)", p_st->prop_index-xx, p_st->p_prop[xx].level,
                                       xml_flp_ttbl[p_st->p_prop[xx].name].p_name, dbg_buf, p_st->p_prop[xx].len);
                }
                else
                {
                    XML_TRACE_DEBUG3( "[xml folder] internal prop: %d:%d:%d", xx, p_st->p_prop[xx].level,
                                       p_st->p_prop[xx].name );
                }
            }
#endif
            p_st->prop_num  = p_st->prop_index ;
            p_st->offset_prop = p_st->p_prop;
         }
         else
         {
             /* This is left over data, pick up the result from the previous parse */
            res = p_xml->pars_res;
         }

          if ( res != XML_OBJ_ST_EMPTY )
          {
                x_res = xml_flp_int_fill_evt_data( p_st->obj, p_st, &p_st->offset_prop, &p_st->prop_num, &dst_data, dst_len );

                if ( (XML_FOLDER_OK == x_res) && (XML_NO_END == res) )
                {
                    /* XML_NO_END means that the xml is not completly finished and fill returns
                       ok when when partial filling has been ok */
                    x_res = XML_FOLDER_PENDING;
                }

                /* all parsed xml data has been converted into file entry */
                /* or exceed max entry number , break the parsing loop   */
                if (XML_FOLDER_OUT_FULL != x_res && XML_FOLDER_DST_NO_RES != x_res)
                {   
                    is_remain = FALSE;
                } 
                else
                    break;
          }
    } /* while */

    /* free property table. at next call a new one is allocated */
    if ((x_res != XML_FOLDER_DST_NO_RES && p_st->p_prop) ||
        XML_FOLDER_OUT_FULL == x_res) 
    { 
           GKI_freebuf( p_st->p_prop );  
           p_st->p_prop = NULL;
           p_st->offset_prop = NULL;
    }

    if ( x_res != XML_FOLDER_DST_NO_RES && p_st->ended)
    {
            /* this should happen on the same time as final flag in fact */
            x_res = XML_FOLDER_END_LIST;   /* found closing /folder-listing */
    }


    *num_entries = p_st->current_entry; /* report number of entries found. only when ended is true
                                               application really should be interested! */
    return x_res;
}

