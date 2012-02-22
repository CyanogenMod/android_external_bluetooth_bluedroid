/*****************************************************************************
**
**  Name:       xml_parse.c
**
**  File:       XML Parser
**
**  Copyright (c) 2000-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include "xml_pars_api.h"
#include "data_types.h"
#include "bt_types.h"
/* The XML Parser is dependent on the Object Store. At present
** the object store resides in GOEP and hence the parser is 
** dependent on GOEP. The parser only uses the Object Store
** in GOEP, so if the Object Store is separated from GOEP in the
** future, the parser will not be dependent on GOEP.
*/

#include <stdlib.h>
#include <string.h>

#ifndef BIP_TRACE_XML
#define BIP_TRACE_XML FALSE
#endif

#if (defined(BIP_TRACE_XML) && BIP_TRACE_XML == TRUE)
#define XML_TRACE_DEBUG0(m)                     {BT_TRACE_0(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m);}
#define XML_TRACE_DEBUG1(m,p1)                  {BT_TRACE_1(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1);}
#define XML_TRACE_DEBUG2(m,p1,p2)               {BT_TRACE_2(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2);}
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            {BT_TRACE_3(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3);}
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         {BT_TRACE_4(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4);}
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      {BT_TRACE_5(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5);}
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   {BT_TRACE_6(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5,p6);}
#else
#define XML_TRACE_DEBUG0(m)                     
#define XML_TRACE_DEBUG1(m,p1)                  
#define XML_TRACE_DEBUG2(m,p1,p2)               
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   
#endif
  
/*****************************************************************************
**   Constants
*****************************************************************************/

#define XML_ST           '<'
#define XML_GT           '>'
#define XML_QM           '?'
#define XML_EX           '!'
#define XML_EM           '/'    /* End Mark */
#define XML_CO           ':'
#define XML_EQ           '='
#define XML_SQ           '\''
#define XML_DQ           '"'
#define XML_AM           '&'
#define XML_SC           ';'
#define XML_PD           '#'
#define XML_HX           'x'
#define XML_HY           '-'
#define XML_LB           '['

#define XML_LT_STR       "lt"
#define XML_GT_STR       "gt"
#define XML_AMP_STR      "amp"
#define XML_APOS_STR     "apos"
#define XML_QUOT_STR     "quot"

#define XML_QTAG_END_STR "?>"
#define XML_COMM_STR     "--"
#define XML_COMM_END_STR "-->"
#define XML_CDS_STR      "[CDATA["
#define XML_CDS_END_STR  "]]>"
#define XML_DOCT_STR     "<'\""

static const UINT8 xml_name_srch[] = ":=/> \t\n\r";


/*****************************************************************************
**   Type Definitions
*****************************************************************************/

enum
{
    XML_PASS_WS,
    XML_SKIP_WS,
    XML_NORM_WS
};
typedef UINT16 tXML_WS_OP;



/*****************************************************************************
**   Globals
**
**   The global below is used as the buffer set (tXML_BFR_SET) in a local
**   variable (of type tXML_MUL_STATE) in XML_Parse. The buffer set memory, is  
**   separated from the rest of tXML_MUL_STATE to make it easy to change the  
**   allocation of its memory if found necessary. See xml_alloc_bfr_set.
*****************************************************************************/

/*****************************************************************************
**   Macro Functions
*****************************************************************************/

#define XML_EOS(p_st)  ((p_st)->curr_res <= 0)              /* End Of Store */
/* white space: " ", \t, \r, \n */
#define XML_IS_WS(c)   (((c) == 0x20) || ((c) == 0x9) || \
                       ((c) == 0xD)  || ((c) == 0xA) || \
                       ((c) == 0x00) )


/*****************************************************************************
**   Function Prototypes
*****************************************************************************/

static BOOLEAN xml_get_next(tXML_MUL_STATE *, tXML_WS_OP);

static BOOLEAN xml_find_ch(tXML_MUL_STATE *, UINT8, tXML_WS_OP);

static void xml_incr_pars_res(tXML_MUL_STATE *, tXML_RESULT);

static void xml_set_bfr(tXML_MUL_STATE *, UINT8);

/* parsing static functions */

static BOOLEAN xml_elems(tXML_MUL_STATE *, BOOLEAN);

static BOOLEAN xml_qm_elem(tXML_MUL_STATE *);

static BOOLEAN xml_ex_elem(tXML_MUL_STATE *, BOOLEAN);

static BOOLEAN xml_tag_elem(tXML_MUL_STATE *);

static BOOLEAN xml_etag_elem(tXML_MUL_STATE *);

#define XML_SET_CLEAR   0
#define XML_SET_NAME    1
#define XML_SET_VALUE   2




/*****************************************************************************
**   API Functions
*****************************************************************************/

void XML_InitPars(tXML_MUL_STATE *p_st, tXML_CBACK xml_cback, void *p_usr_data)
{
    memset(p_st, 0, sizeof(tXML_MUL_STATE));
    p_st->cback         = xml_cback;
    p_st->p_usr_data    = p_usr_data;

    /* by memset()
    p_st->p_data_bfr = NULL;
    p_st->next_token    = 0;
    p_st->curr_res      = 0;
    p_st->pars_res      = XML_SUCCESS;
    p_st->skip_next_nl  = FALSE;

    p_st->prefix.p  = NULL;
    p_st->name.p    = NULL;
    p_st->value.p   = NULL;
    p_st->prefix.len= 0;
    p_st->name.len  = 0;
    p_st->value.len = 0;

    p_st->status    = XML_STS_INIT;
    */
}



/*****************************************************************************
**
** Function         XML_MulParse
**
** Description
**     The current implementation of the xml_pars_api supports only those
**     XML-contructs needed in BPP SOAP-messages. The parser must have a
**     small footprint and is therefore small and simple.
**
**     According to SOAP a message must not contain the doctypedecl construct
**     (production) and it must not contain Processing Instructions (PI 
**     production), i.e. these constructs are not supported. In addition, 
**     CDATA sections, any external or internal entities and the XML
**     Declaration are not supported (not used in BPP). Should any of these 
**     be included in a message being parsed, they will be reported returning
**     a warning code. The parser will then try to find the next tag.
**     When the parser reports an XML-event using the callback it will always
**     continue, even if the callback returns false. All strings in event
**     data passed with the callback are limited to 64 bytes in size, except
**     the prefix string which has 32 as max size. Consequtive XML_CHARDATA
**     events are not supported. Leading and trailing white space is removed
**     from the value string before sending the XML_CHARDATA event.
**
**     This function and also all other helping static parsing functions use
**     more than one return statement in a function. The reason is that
**     a parse error has been found and to exit as soon as possible.
**     If one had used only one return in each function, the path
**     representing a correct xml syntax had been expressed with very deeply
**     nested if-statements.
**
** Parameters       
**     see h-file
** Returns
**     see h-file
*****************************************************************************/

tXML_RESULT XML_MulParse(tXML_MUL_STATE *p_st, tXML_OS *p_os)
{
    BOOLEAN   found;
    BOOLEAN   query, partial = FALSE;
    BOOLEAN   parse_ok = TRUE;
    int       keep_size;
    tXML_RESULT res = XML_SUCCESS;
    tXML_RESULT old_pars_res;

    p_st->curr_res = 1; /* not EOS */
    memcpy(&p_st->xml_os, p_os, sizeof(tXML_OS));
    old_pars_res        = p_st->pars_res;
    p_st->pars_res      = XML_SUCCESS;
    p_st->prefix.len    = 0;
    p_st->name.len      = 0;
    p_st->value.len     = 0;
    p_st->p_last_stm    = 0;
    p_st->p_copy        = 0;

#if ((defined (BIP_TRACE_XML) && BIP_TRACE_XML == TRUE) || (defined FOLDER_DEBUG_XML && FOLDER_DEBUG_XML== TRUE))
    XML_TRACE_DEBUG4("XML_MulParse status:%d, pars_res: %d, begin:%x, end:x%x",
        p_st->status, old_pars_res, p_os->p_begin, p_os->p_end);
#endif

    /* this do-while(0) loop is to avoid too many return statements in this routine.
     * it's easier to "cleanup" with only one return statement */
    if(p_st->status == XML_STS_INIT)
    {

        p_st->p_cur = p_os->p_begin;
#if ((defined (BIP_TRACE_XML) && BIP_TRACE_XML == TRUE) || (defined FOLDER_DEBUG_XML && FOLDER_DEBUG_XML== TRUE))
        XML_TRACE_DEBUG1("p_cur:x%x", p_st->p_cur);
#endif
        do
        {
            if (!xml_get_next(p_st, XML_PASS_WS)) /* obj store empty or err */
            {
                res = XML_OBJ_ST_EMPTY;
                break;
            }

            found = FALSE;
            while (!XML_EOS(p_st) && !found)
            { /* skip all but top element */
                if (!xml_find_ch(p_st, XML_ST, XML_PASS_WS) ||
                    !xml_get_next(p_st, XML_PASS_WS))
                {
                    res = XML_ERR;
                    break;
                }

                if (p_st->next_token == XML_QM)
                {
                    parse_ok = xml_qm_elem(p_st);
                }
                else if (p_st->next_token == XML_EX)
                {
                    parse_ok = xml_ex_elem(p_st, TRUE);
                }
                else if (p_st->next_token == XML_EM)
                {
                    parse_ok = FALSE;
                    if (!xml_get_next(p_st, XML_PASS_WS))
                    {
                        res = XML_ERR;
                        break;
                    }
                }
                else
                {
                    found = TRUE;
                    parse_ok = TRUE;
                }

                if (!parse_ok)
                    xml_incr_pars_res(p_st, XML_ERR);
            }
        } while (0);
        p_st->status = XML_STS_1STM;
    }
    else if(old_pars_res == XML_NO_PROP)
    {
    }
    else
    {
#if ((defined (BIP_TRACE_XML) && BIP_TRACE_XML == TRUE) || (defined FOLDER_DEBUG_XML && FOLDER_DEBUG_XML== TRUE))
        XML_TRACE_DEBUG2("p_st->last_bfr.p:x%x, p_st->used_last_bfr:%d",
            p_st->last_bfr.p, p_st->used_last_bfr);
#endif

/* if there was some data left, read it here. */
        if(p_st->partial_st.last_bfr.p && p_st->partial_st.used_last_bfr  )
        {
             memcpy(p_st->last_bfr.p, p_st->partial_st.last_bfr.p,  p_st->partial_st.used_last_bfr);          
             p_st->used_last_bfr = p_st->partial_st.used_last_bfr;  
             p_st->last_bfr.p[p_st->partial_st.used_last_bfr] = 0;
             p_st->event_data.part.parse = p_st->partial_st.event_data.part.parse; 

             /* set length to 0 */
             p_st->partial_st.used_last_bfr = 0;
             XML_TRACE_DEBUG1("retrieved PARTIAL data = [%s]\n", p_st->last_bfr.p); 

             p_st->p_cur = p_st->last_bfr.p;
             /* continuation packet */
             /* read a ch, setup xml_set_bfr */
             xml_get_next(p_st, XML_PASS_WS);   
             p_st->event_data.copy.p_begin   = p_st->xml_os.p_begin;
             p_st->event_data.copy.last.p    = p_st->last_bfr.p;
             p_st->event_data.copy.last.len  = p_st->used_last_bfr;
             p_st->cback(XML_COPY, &(p_st->event_data), p_st->p_usr_data);   
        }
        else
        {
            if(p_st->used_last_bfr == 0)
            {
                p_st->p_cur = p_os->p_begin;
                xml_get_next(p_st, XML_PASS_WS);
            }
            else
                return XML_NO_MEM;
        }
#if ((defined (BIP_TRACE_XML) && BIP_TRACE_XML == TRUE) || (defined FOLDER_DEBUG_XML && FOLDER_DEBUG_XML== TRUE))
        XML_TRACE_DEBUG1("p_st->p_cur:x%x", p_st->p_cur);
#endif
    }

    XML_TRACE_DEBUG0("XML_MulParse end while");

    if(res == XML_SUCCESS)
    {
        /* here we found "<(a-z)" */
        if (!XML_EOS(p_st))
        {
            if(p_st->status == XML_STS_1STM)
            {
                /* remeber the beginning position right after '<' in the first line                 */
                /* if the first line can't be parsed at first round, save it to the second parse    */
                p_st->p_copy = p_st->p_cur - 1; 
                parse_ok = xml_tag_elem(p_st);
            }
            
           /* parsed the first line */
            XML_TRACE_DEBUG0("XML_MulParse exit xml_tag_elem");
            
            if (!parse_ok)
            {
                query = p_st->cback(XML_QUERY, &(p_st->event_data), p_st->p_usr_data);

                /* if first line parsing is not completed while reach the end of stack, ERROR occurs */
                if (query == TRUE)
                    xml_incr_pars_res(p_st, XML_ERR);
                else    /* first line parsing to be continued, copy partial data at later point*/
                    partial = TRUE;
            }
            else /* first line is parsed ok, change parsing status */
                p_st->status = XML_STS_1TAG;

                

            if (!XML_EOS(p_st) && parse_ok)
            {
                parse_ok = xml_elems(p_st, parse_ok);
                query = p_st->cback(XML_QUERY, &(p_st->event_data), p_st->p_usr_data);
                if (parse_ok == FALSE || query == FALSE)
                {                    
                    partial = TRUE;

                }
                else                  
                    p_st->status = XML_STS_DONE;
            }

            /* copy partial data if any */
            if (partial)
            {
                if(p_st->pars_res == XML_NO_PROP)
                {
                    p_st->p_cur = p_st->p_copy;
                    p_st->event_data.part.parse     = p_st->pars_res;
                    p_st->event_data.part.p_keep    = p_st->p_cur;
                    XML_TRACE_DEBUG1("p_st->p_cur:x%x (last_stm)", p_st->p_cur);
                    p_st->cback(XML_PARTIAL, &(p_st->event_data), p_st->p_usr_data);
                    xml_incr_pars_res(p_st, XML_NO_END);
                }
                else
                {
                    if( p_st->last_bfr.p &&
                        (p_st->p_copy > p_st->xml_os.p_begin) &&
                        (p_st->p_copy < p_st->xml_os.p_end) )
                    {
                        keep_size = p_st->xml_os.p_end - p_st->p_copy;
                        if(keep_size < p_st->last_bfr.len)
                       {
                            /* store the partial data to a temporary buffer,
                              NOT to the queue of buffers as it would overwrite current ones!  */
                            if(p_st->partial_st.last_bfr.p )
                            {
                                XML_TRACE_DEBUG0("Store partial data\n");   
                                BCM_STRNCPY_S((char *)p_st->partial_st.last_bfr.p, 512, (char *)p_st->p_copy, keep_size);      
                                p_st->partial_st.used_last_bfr= keep_size;
                                p_st->partial_st.last_bfr.p[keep_size] = 0;
                                p_st->partial_st.event_data.part.parse = p_st->pars_res;
                                p_st->partial_st.event_data.part.p_keep= p_st->last_bfr.p;
                            }
                            else
                                XML_TRACE_DEBUG0("ERROR to store partial data");

                            p_st->cback(XML_PARTIAL, &(p_st->event_data), p_st->p_usr_data);                      
                            xml_incr_pars_res(p_st, XML_NO_END);
                        }
                    }
                }/* else NO_PROP */
            } /* end of partial */
        } /* end of !XML_EOS(p_st) */
    } /* end of res == XML_SUCCESS */

     
    return p_st->pars_res;
}


/*****************************************************************************
**   Static Functions
*****************************************************************************/



/*****************************************************************************
**
** Function         xml_set_bfr
**
** Description
**     Sets the buffer that is going to be used when tokens are pushed from
**     p_st->next_token into some buffer in the buffer set. 
**
** Parameters
**     p_st (in/out) :    the parser state
**     p_bfr (in) :       the buffer that will get all tokens (characters)
**                        NULL is allowed in which case no buffer is used.
**     bfr_max_ind (in) : the max index into the buffer in which a non-null
**                        char may be stored
**
** Returns
**     -
*****************************************************************************/
static void xml_set_bfr(tXML_MUL_STATE *p_st, UINT8 set)
{
    switch(set)
    {
    case XML_SET_NAME:
        p_st->name.p     = p_st->p_cur - 1;
        p_st->p_data_bfr = p_st->name.p;
        p_st->name.len   = 0;
        break;
    case XML_SET_VALUE:
        p_st->value.p    = p_st->p_cur - 1;
        p_st->p_data_bfr = p_st->value.p;
        p_st->value.len  = 0;
        break;
    default:
        p_st->p_data_bfr = NULL;
    }
}


/*****************************************************************************
**
** Function         xml_write_bfr
**
** Description
**     Pushes (copies) the character from p_st->next_token to the buffer, if 
**     any, that has been set calling xml_set_bfr.
**
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     -
*****************************************************************************/

static void xml_write_bfr(tXML_MUL_STATE *p_st)
{
    if (p_st->p_data_bfr)
    {
        if(p_st->p_data_bfr == p_st->name.p)
            p_st->name.len++;
        else
            p_st->value.len++;
    }
}


/*****************************************************************************
**
** Function         xml_incr_pars_res
**
** Description
**     Sets the final parsing result if the new_res provided has
**     higher rank than the current parsing result.
**
** Parameters       
**     p_st (in/out) : the parser state
**     new_res (in) :  the new parsing result
**
** Returns
**     -
*****************************************************************************/

static void xml_incr_pars_res(tXML_MUL_STATE *p_st, tXML_RESULT new_res)
{
    if (new_res > p_st->pars_res)
    {
        switch(p_st->pars_res)
        {
        /* preserve these error messages */
        case XML_OBJ_ST_EMPTY:
        case XML_NO_MEM:         /* no last_bfr.p, and the tXML_MUL_STATE is not in init */
        case XML_NO_PROP:        /* run out of tXML_PROP */
            break;

        default:
        /*
        case XML_SUCCESS:
        case XML_WARNING:
        case XML_ERR:
        */
        p_st->pars_res = new_res;
            break;
        }
    }
}


/*****************************************************************************
**
** Function         xml_read_char
**
** Description
*****************************************************************************/
static void xml_read_char(tXML_MUL_STATE *p_st)
{
    BOOLEAN get_new = FALSE;

    if (p_st->p_cur && p_st->p_cur >= p_st->last_bfr.p && p_st->p_cur < (p_st->last_bfr.p + p_st->used_last_bfr))
    {
        /* left over from previous parse */
        p_st->next_token = *p_st->p_cur;
        if(p_st->next_token == 0)
        {
            /* leftover is done, use the new one */
            p_st->p_cur         = p_st->xml_os.p_begin;
            p_st->last_bfr.p[0] = 0;
            p_st->used_last_bfr = 0;
            get_new             = TRUE;
        }
        else
        {
            p_st->p_cur++;
            p_st->curr_res = 1;
        }
    }
    else
    {
        if(p_st->p_cur == (p_st->last_bfr.p + p_st->used_last_bfr))
        {
            p_st->used_last_bfr = 0;
            p_st->p_cur         = p_st->xml_os.p_begin;
        }
        get_new = TRUE;
    }

    if(get_new)
    {
        if(p_st->p_cur && p_st->p_cur < p_st->xml_os.p_end)
        {
            /* use buffer given to XML_Parse */
            p_st->next_token = *p_st->p_cur;
            p_st->p_cur++;
            p_st->curr_res = 1;
        }
        else
            p_st->curr_res = 0;
    }


/*
    XML_TRACE_DEBUG4("xml_read_char p_cur: x%x, curr_res:%d, get_new:%d, token:%c",
        p_st->p_cur, p_st->curr_res, get_new, p_st->next_token);
*/
}

/*****************************************************************************
**
** Function         xml_get_next
**
** Description
**     Writes the character in p_st->next_token to the current buffer if set.
**     Then the next character is read from the Object Store into
**     p_st->next_token. The first time get_next is called, the current 
**     buffer must be NULL, i.e p_st->data_bfr must be NULL.
**
**     xml_get_next handles end-of-line as specified in the xml spec. It 
**     passes, skips or normalises (p.29 in XML spec) white spaces (ws) 
**     as specified in the ws_op param. Note, the ws_op applies when
**     getting one (or many characters) from Object Store into the
**     p_st->next_token. It does not apply when pushing the (initial)
**     p_st->next_token to the current buffer.
**
**     The characters are read one by one from the Object Store.
**     Presently this is not anticipated to cause any problems
**     regarding reading speed. Should it become a problem in the
**     future, a new buffer could be introduced into which a chunk
**     of characters could be put, using one Object Store read call.
**     The get_next function would then get the next character from 
**     the new buffer.
**
** Parameters
**     p_st (in/out) : the parser state
**     ws_op (in) :    the requested white space handling.      
**
** Returns
**     True if a character was successfully read into p_st->next_token.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_get_next(tXML_MUL_STATE *p_st, tXML_WS_OP ws_op)
{
    xml_write_bfr(p_st);
    do
    {
        xml_read_char(p_st);
    } while ((ws_op == XML_SKIP_WS) && XML_IS_WS(p_st->next_token) &&
             !XML_EOS(p_st));


    /* handle end-of-line if any after the do-while above */

    if (!XML_EOS(p_st) && (p_st->next_token == 0xA) && p_st->skip_next_nl)
    {   /* we have previously found 0xD (cr) and have set the state var
        ** p_st->skip_next_nl,see below
        */
        xml_read_char(p_st);
    }
    p_st->skip_next_nl = FALSE;

    if (XML_EOS(p_st))
    {
        p_st->next_token = 0;
        return FALSE;
    }

    if (p_st->next_token == 0xD)
    {
        p_st->next_token = 0xA;
        p_st->skip_next_nl = TRUE;
    }

    if ((ws_op == XML_NORM_WS) &&
        ((p_st->next_token == 0xA) || (p_st->next_token == 0x9)))
    {
        p_st->next_token = 0x20;
    }

    return TRUE;
}


/*****************************************************************************
**
** Function         xml_find_ch
**
** Description
**     Searches for the character given in ch. It starts searching in
**     p_st->next_token and if not found it gets characters from the Object 
**     Store until ch is in p_st->next_token. 
**
** Parameters
**     p_st (in/out) : the parser state
**     ch (in) :       the character to search for
**     ws_op (in) :    the requested white space handling when getting chars      
**
** Returns
**     True if the character was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_find_ch(tXML_MUL_STATE *p_st, UINT8 ch, tXML_WS_OP ws_op)
{
    while (!XML_EOS(p_st) && (p_st->next_token != ch))
        xml_get_next(p_st, ws_op);
    return (BOOLEAN) !XML_EOS(p_st);
}


/*****************************************************************************
**
** Function         xml_find_ch_n
**
** Description
**     Same function as xml_find_ch, except that any character in p_str
**     that is found will stop the search.
**
** Parameters
**     p_st (in/out) : the parser state
**     p_str (in) :    the string containing the characters searched for.
**                     Must not be NULL or an empty string.
**
** Returns
**     True if any of the characters in p_str was found.
**     Fase otherwise.
*****************************************************************************/

static BOOLEAN xml_find_ch_n(tXML_MUL_STATE *p_st, const UINT8 *p_str)
{
    const UINT8 *p_tmp;

    while (!XML_EOS(p_st))
    {
        for (p_tmp = p_str; *p_tmp; p_tmp++)
        {
            if (p_st->next_token == *p_tmp)
                return TRUE;
        }
        xml_get_next(p_st, XML_PASS_WS);
    }
    return FALSE;
}


/*****************************************************************************
**
** Function         xml_find_str
**
** Description
**     Searches for p_str (i.e the exact sequence of characters in p_str) in 
**     the input from Object Store. The function ends with the character
**     succeeding p_str in the input, (i.e that char is in p_st->next_token
**     upon return) or with XML_EOS.
**
** Parameters
**     p_st (in/out) : the parser state
**     p_str (in) :    the string to search for and pass by.
**                     Must not be NULL or an empty string.
**
** Returns
**     True if the string was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_find_str(tXML_MUL_STATE *p_st, const UINT8 *p_str)
{
    const UINT8 *p_tmp;

    p_tmp = p_str;
    while (*p_tmp && !XML_EOS(p_st))
    {
        for (p_tmp = p_str; *p_tmp && !XML_EOS(p_st); p_tmp++)
        {
            if (p_st->next_token != *p_tmp)
                break;
            xml_get_next(p_st, XML_PASS_WS);
        }

        if ((p_tmp == p_str) && !XML_EOS(p_st))
        {
            xml_get_next(p_st, XML_PASS_WS);
        }
    }

    return (BOOLEAN) (*p_tmp == 0);
}


/*****************************************************************************
**
** Function         xml_consume_str
**
** Description
**     Checks for p_str i.e that the first character from p_str is in
**     p_st->next_token and that the successors immediately follows in the 
**     Object Store. The p_str must not be last in the Object Store.
**
** Parameters
**     p_st (in/out) : the parser state
**     p_str (in) :    the string to check if present next and to pass by      
**                     Must not be NULL.      
**
** Returns
**     True if the string was found and was not last in the Object Store.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_consume_str(tXML_MUL_STATE *p_st, const UINT8 *p_str)
{
    do
    {
        if (p_st->next_token != *p_str)
            return FALSE;
        p_str++;
        if (!xml_get_next(p_st, XML_PASS_WS))
            return FALSE;
    } while (*p_str);
    return TRUE;
}


/*****************************************************************************
**
** Function         xml_resolve_refs
**
** Description
**     Resolves predefined entity references (sect. 4.6 in the XML spec)
**     and character references (sect 4.1) that may be found in 
**     AttValue and content. (According to the XML spec it may also
**     be in an EntityValue. However EntityValues are in the 
**     doctypedecl part which is not supported).
**
**     The AttValue and content not beginning with a tag, must be
**     stored in the p_st->p_bfr_set->value buffer.
**
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     -
*****************************************************************************/

static void xml_resolve_refs(tXML_MUL_STATE *p_st)
{
    UINT8   *p_srch;       /* where next search for & starts */
    UINT8   *p_am;         /* points to found & */
    UINT8   *p_sc;         /* points to found ; and succeeding chars */
    UINT8   *p_start;
    UINT8   *p_tmp;
    UINT32  ch_code;
    UINT32  tmp_code;
    INT8    i;
    BOOLEAN resolved;
    UINT16  len_left;

    p_srch   = p_st->value.p;
    len_left = p_st->value.len;
    do
    {
        p_start = p_srch;
        p_am = (UINT8*) strchr((char*) p_srch, XML_AM);
        p_sc = p_am ? (UINT8*) strchr((char*) p_am, XML_SC) : NULL;
        /* make sure the ptr does not exceed the end of the value str */
        if(p_sc > (len_left + p_start))
            p_sc = NULL;

        if (p_am && p_sc)
        {
            resolved = FALSE;
            p_tmp = p_am + 1;
            *p_sc = 0;       /* terminate the ref by replacing ; with 0 */
            if (*p_tmp == XML_PD)                      /* character ref */
            {
                if (p_tmp[1] == XML_HX)
                    *p_tmp = '0';
                else
                {
                    for(p_tmp++; *p_tmp == '0'; p_tmp++)
                    {
                        ;
                    }
                }

                ch_code = strtoul((char*) p_tmp, NULL, 0);
                /* skip leading zero bytes */
                for (i = 3; (i >= 0) && !(ch_code >> i * 8); i--)
                {
                    ;
                }
                p_tmp = p_am;
                while (i >= 0)
                {
                    /* mask out one byte and shift it rightmost         */
                    /* preceding bytes must be zero so shift left first */
                    tmp_code = ch_code << ((3-i) * 8);
                    *p_tmp = (UINT8) (tmp_code >> 24);
                    p_tmp++;
                    i--;
                }
                resolved = TRUE;
            }
            else if (p_tmp < p_sc)           /* check if predefined ref */
            {
                resolved = TRUE;
                if (strcmp((char*) p_tmp, XML_LT_STR) == 0)
                {
                    *p_am = XML_ST;
                    p_st->value.len = p_st->value.len - 3; /* remove the length for lt; */
                    p_st->p_cur = p_st->p_cur - 3;
                }
                else if (strcmp((char*) p_tmp, XML_GT_STR) == 0)
                {
                    *p_am = XML_GT;
                    p_st->value.len = p_st->value.len - 3; /* remove the length for gt; */
                    p_st->p_cur = p_st->p_cur - 3;
                }			
                else if (strcmp((char*) p_tmp, XML_AMP_STR) == 0)
                {
                    *p_am = XML_AM;
		            p_st->value.len = p_st->value.len - 4; /* remove the length for   amp; */
                    p_st->p_cur = p_st->p_cur - 4;
                }
                else if (strcmp((char*) p_tmp, XML_APOS_STR) == 0)
                {
                    *p_am = XML_SQ;
		            p_st->value.len = p_st->value.len - 5; /* remove the length for   apos; */
                    p_st->p_cur = p_st->p_cur - 5;
                }			
                else if (strcmp((char*) p_tmp, XML_QUOT_STR) == 0)
                {
                    *p_am = XML_DQ;
	                p_st->value.len = p_st->value.len - 5; /* remove the length for   quot; */
                    p_st->p_cur = p_st->p_cur - 5;
                }			
                else
                    resolved = FALSE;
            }

            if (resolved)
            {
                p_srch = p_tmp;            /* will contain char after ; */
                p_sc++;
                while(*p_sc)
                {
                    *p_tmp++ = *p_sc++;
                }
            }
            else
            {
                *p_sc = XML_SC;                   /* restore the ref end */
                p_srch = p_sc + 1;
            }

        } /* end if */
    } while (*p_srch && p_am && p_sc);
}


/*****************************************************************************
**
** Function         xml_remove_trail_ws
**
** Description
**     Removes trailing white space from the p_st->p_data_bfr buffer.
**
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     -
*****************************************************************************/

static void xml_remove_trail_ws(tXML_MUL_STATE *p_st)
{
    UINT16  xx;

    if(p_st->value.p)
    {
        xx = p_st->value.len;
        while(xx && XML_IS_WS(p_st->value.p[xx-1]))
            xx--;
        p_st->value.len = xx;
    }

}


/*****************************************************************************
**   Parsing Static Functions
*****************************************************************************/


/*****************************************************************************
**
** Function         xml_name
**
** Description
**     Parses a name and its prefix if any. The prefix and name buffers
**     are set.
**     The functions ends with either white space,
**     XML_EQ, XML_EM or XML_GT in p_st->next_token or with XML_EOS.
**     
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     True if no error was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_name(tXML_MUL_STATE *p_st)
{
    BOOLEAN found = FALSE;

    p_st->prefix.p   = NULL;
    p_st->prefix.len = 0;
    xml_set_bfr(p_st, XML_SET_NAME);
    xml_find_ch_n(p_st, xml_name_srch);
    if (!XML_EOS(p_st) && (p_st->next_token == XML_CO))
    {
        if (p_st->name.len)
        {
            found = TRUE;
            /* p_st->name.len is string size in name buffer, \0 excl.
            */
            p_st->prefix.p   = p_st->name.p;
            p_st->prefix.len = p_st->name.len;
        }
        xml_get_next(p_st, XML_PASS_WS);
        xml_set_bfr(p_st, XML_SET_NAME);
        if (!XML_EOS(p_st))
        {
            xml_find_ch_n(p_st, xml_name_srch + 1);
        }
    }
    
    found = (BOOLEAN) (found || p_st->name.len);
    if(found)
        xml_set_bfr(p_st, XML_SET_CLEAR);
    return found;
}


/*****************************************************************************
**
** Function         xml_attributes
**
** Description
**     Parses an attribute list. 
**     The functions ends with the XML_GT or XML_EM char or XML_EOS.
**     Error is reported if the attribute list is last in the Object
**     Store.
**     Sends a XML_ATTRIBUTE event in the user callback for each
**     attribute found.
**     
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     True if no error was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_attributes(tXML_MUL_STATE *p_st)
{
    BOOLEAN cb_ret = TRUE;
    UINT8 q_ch;

    XML_TRACE_DEBUG1("[xml_parse] xml_attributes: res= %d", p_st->pars_res);

    while ( cb_ret)
    {
        /* if this is a white space, then the next character is read from the 
            Object Store into p_st->next_token */
        if( XML_IS_WS(p_st->next_token) )
        {
            if (!xml_get_next(p_st, XML_SKIP_WS))
                return FALSE;
        }

        if (p_st->next_token == XML_EQ)
            return FALSE;

        if ((p_st->next_token == XML_GT) || (p_st->next_token == XML_EM))
            return TRUE;
        if (!xml_name(p_st) || XML_EOS(p_st))
        {
            return FALSE;
        }
        if(XML_IS_WS(p_st->next_token))
        {
            if (!xml_get_next(p_st, XML_SKIP_WS))
                return FALSE;
        }

        if (p_st->next_token != XML_EQ)
            return FALSE;

        if (!xml_get_next(p_st, XML_SKIP_WS))
            return FALSE;

        if ((p_st->next_token != XML_SQ) && (p_st->next_token != XML_DQ))
            return FALSE;

        q_ch = p_st->next_token;
        if (!xml_get_next(p_st, XML_PASS_WS))
            return FALSE;


        xml_set_bfr(p_st, XML_SET_VALUE);
        if (!xml_find_ch(p_st, q_ch, XML_NORM_WS))
        {
            return FALSE;
        }

        xml_set_bfr(p_st, XML_SET_CLEAR);
        xml_resolve_refs(p_st);

        p_st->event_data.attr.prefix.p      = p_st->prefix.p;
        p_st->event_data.attr.prefix.len    = p_st->prefix.len;
        p_st->event_data.attr.name.p        = p_st->name.p;
        p_st->event_data.attr.name.len      = p_st->name.len;
        p_st->event_data.attr.value.p       = p_st->value.p;
        p_st->event_data.attr.value.len     = p_st->value.len;
        p_st->value.len = 0;
        cb_ret = p_st->cback(XML_ATTRIBUTE, &(p_st->event_data), p_st->p_usr_data);
        /* chk cback return */
        if(cb_ret == FALSE)
        {
            xml_incr_pars_res(p_st, XML_NO_PROP);
            return FALSE;
        }

        if (!xml_get_next(p_st, XML_PASS_WS))
            return FALSE;
    }

    return (BOOLEAN)
           ((p_st->next_token == XML_GT) || (p_st->next_token == XML_EM));
}


/*****************************************************************************
**
** Function         xml_elems
**
** Description
**     Parses all elements with all their content.This function is not a
**     one-to-one mapped implementation of one production from the XML spec.
**     Instead it uses a simplified iterative (as opposed to recursive)
**     approach when parsing both the element and content productions.
**
**     When a parsing error is found, this function tries to recover by
**     searching for the next element (tag).
**
**     When char data is found, the function sends the XML_CHARDATA event in
**     the user callback.
**
**     Other static functions with production names, start their parsing
**     from the first character in their production. They might check
**     that the first character (token) in the production matches
**     p_st->next_token, alternatively they can just get rid of the first 
**     char in the production by calling get_next_ch. The exceptions to this 
**     are the xml_qm_elem, xml_ex_elem, xml_etag_elem and the xml_tag_elem 
**     functions which starts with the XML_QM, XML_EX, XML_EM and the first 
**     char in the tag name, respectively.
**     
** Parameters
**     p_st (in/out) : the parser state
**     prev_ok (in) :  if parsing done before calling this function was
**                     ok. If not, the functions starts with recovering.       
**
** Returns
**     True if parsing was successful possibly with successful recoveries.
**     False if an error was found from which recovery failed (XML_EOS).
*****************************************************************************/

static BOOLEAN xml_elems(tXML_MUL_STATE *p_st, BOOLEAN prev_ok)
{
    BOOLEAN tag_found;
    BOOLEAN cb_ret = TRUE;

    while (!XML_EOS(p_st) && prev_ok)
    {
        /* remove leading ws even if char data */
        if (XML_IS_WS(p_st->next_token))
        {
            if (!xml_get_next(p_st, XML_SKIP_WS))
                return TRUE;
        }

        tag_found = (BOOLEAN) (p_st->next_token == XML_ST);
        if (!tag_found)
        {
            xml_set_bfr(p_st, XML_SET_VALUE);
            tag_found = xml_find_ch(p_st, XML_ST, XML_PASS_WS);

            xml_remove_trail_ws(p_st);
            if (p_st->value.len > 0)
            {
                xml_resolve_refs(p_st);
                p_st->event_data.ch_data.value.p    = p_st->value.p;
                p_st->event_data.ch_data.value.len  = p_st->value.len;
                p_st->event_data.ch_data.last = TRUE;
                p_st->value.len = 0;
                cb_ret = p_st->cback(XML_CHARDATA, &(p_st->event_data), p_st->p_usr_data);
                /* chk cback return */
                if(cb_ret == FALSE)
                {
                    xml_incr_pars_res(p_st, XML_NO_PROP);
                    return FALSE;
                }

            }
            xml_set_bfr(p_st, XML_SET_CLEAR);

            if (!tag_found)
                return prev_ok;
        }
        else
        {
            p_st->p_last_stm = p_st->p_cur - 1;
            
            if (p_st->p_cur)
                p_st->p_copy = p_st->p_last_stm;

            p_st->cback(XML_TOP, &(p_st->event_data), p_st->p_usr_data);
        }

        /* tag was found */
        if (!xml_get_next(p_st, XML_PASS_WS))
            return FALSE;


        if (p_st->next_token == XML_QM)
            prev_ok = xml_qm_elem(p_st);
        else if (p_st->next_token == XML_EX)
        {
            prev_ok = xml_ex_elem(p_st, FALSE);
        }
        else if (p_st->next_token == XML_EM)
        {
            prev_ok = xml_etag_elem(p_st);
        }
        else
            prev_ok = xml_tag_elem(p_st);



        if (!prev_ok)
            xml_incr_pars_res(p_st, XML_ERR);
    }

    XML_TRACE_DEBUG1("xml_elems prev_ok:%d", prev_ok);
    return prev_ok;
}


/*****************************************************************************
**
** Function         xml_qm_elem
**
** Description
**     Recognises all productions starting with "<?". That is PI and XML decl.
**     These productions are skipped and XML_WARNING is set.
**     The function starts with the XML_QM as the first char (is in 
**     p_st->next_token).It ends with the XML_GT successor (is in
**     p_st->next_token) or XML_EOS.
**
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     True if no error was found trying to recognise the start and end of
**     the productions. False otherwise.
*****************************************************************************/

static BOOLEAN xml_qm_elem(tXML_MUL_STATE *p_st)
{
    if (!xml_get_next(p_st, XML_PASS_WS))
        return FALSE;
    if (!xml_find_str(p_st, (UINT8*) XML_QTAG_END_STR))
        return FALSE;
    xml_incr_pars_res(p_st, XML_WARNING);
    return TRUE;
}


/*****************************************************************************
**
** Function         xml_ex_elem
**
** Description
**     Handles all productions starting with "<!". They are Comments, CDSect
**     doctypedecl and markupdecl. All are skipped. However, the inpar
**     prolog must be set for the function to try to detect the doctypedecl 
**     and markupdecl beginning.
**
**     The function starts with the XML_EX as the first char.
**     The function ends with XML_EOS or the char succeeding XML_GT,  
**     except for doctypedecl and marcupdecl which ends with the next XM_TAG.
**
** Parameters       
**     p_st (in/out) : the parser state
**     prolog (in) :   should be set if in prolog in which case the function
**                     tries to detect (allows) the beginning of doctypedecl
**                     and markupdecl.
** Returns
**     True if no error was found trying to recognise the start and end of
**     the productions. False otherwise.
*****************************************************************************/

static BOOLEAN xml_ex_elem(tXML_MUL_STATE *p_st, BOOLEAN prolog)
{
    UINT8 q_ch;

    if (!xml_get_next(p_st, XML_PASS_WS))
        return FALSE;

    if (p_st->next_token == XML_HY)                         /* comment */
    {
        if (!xml_consume_str(p_st, (UINT8*) XML_COMM_STR))
            return FALSE;

        if (!xml_find_str(p_st, (UINT8*) XML_COMM_END_STR))
            return FALSE;
    }
    else if (p_st->next_token == XML_LB)                     /* CDSect */
    {
        if (!xml_consume_str(p_st, (UINT8*) XML_CDS_STR))
            return FALSE;

        if (!xml_find_str(p_st, (UINT8*) XML_CDS_END_STR))
            return FALSE;

        xml_incr_pars_res(p_st, XML_WARNING);
    }
    else if (prolog)                    /* doctypedecl or markupdecl */
    {
        do
        {
            if (!xml_find_ch_n(p_st, (UINT8*) XML_DOCT_STR))
                return FALSE;

            if ((p_st->next_token == XML_SQ) || (p_st->next_token == XML_DQ))
            {
                q_ch = p_st->next_token;
                if (!xml_get_next(p_st, XML_PASS_WS))
                    return FALSE;

                if (!xml_find_ch(p_st, q_ch, XML_PASS_WS))
                    return FALSE;

                xml_get_next(p_st, XML_PASS_WS); 
            }
        } while (!XML_EOS(p_st) && (p_st->next_token != XML_ST));

        xml_incr_pars_res(p_st, XML_WARNING);
    }
    else                                                    /* error */
    {
        return FALSE;
    }

    return TRUE;
}


/*****************************************************************************
**
** Function         xml_tag_elem
**
** Description
**     Parses a tag element. The function starts with the char succeeding the
**     XML_ST char. 
**     The functions ends with the char succeeding the XML_GT char or 
**     with XML_EOS.
**     Sends the XML_TAG and the XML_TAG_END events in a callback each.
**     
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     True if no error was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_tag_elem(tXML_MUL_STATE *p_st)
{
    BOOLEAN     cb_ret = TRUE;

    if (!xml_name(p_st))
        return FALSE;

    p_st->event_data.tag.prefix.p   = p_st->prefix.p;
    p_st->event_data.tag.name.p     = p_st->name.p;
    p_st->event_data.tag.prefix.len = p_st->prefix.len;
    p_st->event_data.tag.name.len   = p_st->name.len;
    p_st->event_data.tag.p_last_stm = p_st->p_last_stm;
    cb_ret = p_st->cback(XML_TAG, &(p_st->event_data), p_st->p_usr_data);
    if(cb_ret == FALSE)
    {
        xml_incr_pars_res(p_st, XML_NO_PROP);
        return FALSE;
    }
    
    /* chk cback return */
    
    if (XML_EOS(p_st))
        return FALSE;

    if (XML_IS_WS(p_st->next_token))
    {
        if (!xml_attributes(p_st))
            return FALSE;
    }

    p_st->event_data.empty_elem.end = (BOOLEAN) (p_st->next_token == XML_EM);
    if (p_st->event_data.empty_elem.end)
    {
        if (!xml_get_next(p_st, XML_PASS_WS))
            return FALSE;
    }

    if (p_st->next_token != XML_GT)
        return FALSE;

    xml_get_next(p_st, XML_PASS_WS);

    cb_ret = p_st->cback(XML_TAG_END, &(p_st->event_data), p_st->p_usr_data);


    if(cb_ret == FALSE)
    {
        xml_incr_pars_res(p_st, XML_NO_PROP);
        return FALSE;
    }
    
    p_st->p_copy = p_st->p_cur - 1;
    p_st->cback(XML_TOP, &(p_st->event_data), p_st->p_usr_data);
    /* chk cback return */

    return TRUE;
}


/*****************************************************************************
**
** Function         xml_etag_elem
**
** Description
**     Parses an end tag element. The function starts with the XML_EM char.
**     The functions ends with the char succeeding the XML_GT char or 
**     with XML_EOS. Sends the XML_ETAG event in the user callback.
**     
** Parameters       
**     p_st (in/out) : the parser state
**
** Returns
**     True if no error was found.
**     False otherwise.
*****************************************************************************/

static BOOLEAN xml_etag_elem(tXML_MUL_STATE *p_st)
{
    BOOLEAN     cb_ret = TRUE;

    if (!xml_get_next(p_st, XML_PASS_WS))
        return FALSE;

    if (!xml_name(p_st))
        return FALSE;

    p_st->event_data.etag.prefix.p   = p_st->prefix.p;
    p_st->event_data.etag.name.p     = p_st->name.p;
    p_st->event_data.etag.name.len   = p_st->name.len;
    p_st->event_data.etag.prefix.len = p_st->prefix.len;
    cb_ret = p_st->cback(XML_ETAG, &(p_st->event_data), p_st->p_usr_data);
    if(cb_ret == FALSE)
    {
        xml_incr_pars_res(p_st, XML_NO_PROP);
        return FALSE;
    }
    
    p_st->p_copy = (p_st->prefix.p) ? p_st->prefix.p - 2: p_st->name.p - 2;
    p_st->cback(XML_TOP, &(p_st->event_data), p_st->p_usr_data);

    /* chk cback return */

    if (XML_EOS(p_st))
        return FALSE;

    if (XML_IS_WS(p_st->next_token))
        if (!xml_get_next(p_st, XML_SKIP_WS))
            return FALSE;

    if (p_st->next_token != XML_GT)
        return FALSE;

    xml_get_next(p_st, XML_PASS_WS);

    return TRUE;
}

