/******************************************************************************
 *
 *  Copyright (C) 2003-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <string.h>

#include "gki.h"
#include "avrc_api.h"
#include "avrc_defs.h"
#include "avrc_int.h"

/*****************************************************************************
**  Global data
*****************************************************************************/
#if (AVRC_METADATA_INCLUDED == TRUE)

#define EVT_AVAIL_PLAYER_CHANGE_RSP_LENGTH 1
#define EVT_ADDR_PLAYER_CHANGE_RSP_LENGTH 5

/*******************************************************************************
**
** Function         avrc_bld_get_capability_rsp
**
** Description      This function builds the Get Capability response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_capability_rsp (tAVRC_GET_CAPS_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_count;
    UINT16  len = 0;
    UINT8   xx;
    UINT32  *p_company_id;
    UINT8   *p_event_id;
    tAVRC_STS status = AVRC_STS_NO_ERROR;

    if (!(AVRC_IS_VALID_CAP_ID(p_rsp->capability_id)))
    {
        AVRC_TRACE_ERROR1("avrc_bld_get_capability_rsp bad parameter. p_rsp: %x", p_rsp);
        status = AVRC_STS_BAD_PARAM;
        return status;
    }

    AVRC_TRACE_API0("avrc_bld_get_capability_rsp");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    UINT8_TO_BE_STREAM(p_data, p_rsp->capability_id);
    p_count = p_data;

    if (len == 0)
    {
        *p_count = p_rsp->count;
        p_data++;
        len = 2; /* move past the capability_id and count */
    }
    else
    {
        p_data = p_start + p_pkt->len;
        *p_count += p_rsp->count;
    }

    if (p_rsp->capability_id == AVRC_CAP_COMPANY_ID)
    {
        p_company_id = p_rsp->param.company_id;
        for (xx=0; xx< p_rsp->count; xx++)
        {
            UINT24_TO_BE_STREAM(p_data, p_company_id[xx]);
        }
        len += p_rsp->count * 3;
    }
    else
    {
        p_event_id = p_rsp->param.event_id;
        *p_count = 0;
        for (xx=0; xx< p_rsp->count; xx++)
        {
            if (AVRC_IS_VALID_EVENT_ID(p_event_id[xx]))
            {
                (*p_count)++;
                UINT8_TO_BE_STREAM(p_data, p_event_id[xx]);
            }
        }
        len += (*p_count);
    }
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    status = AVRC_STS_NO_ERROR;

    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_attr_rsp
**
** Description      This function builds the List Application Settings Attribute
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_attr_rsp (tAVRC_LIST_APP_ATTR_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_num;
    UINT16  len = 0;
    UINT8   xx;

    AVRC_TRACE_API0("avrc_bld_list_app_settings_attr_rsp");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    p_num = p_data;
    if (len == 0)
    {
        /* first time initialize the attribute count */
        *p_num = 0;
        p_data++;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_rsp->num_attr; xx++)
    {
        if(AVRC_IsValidPlayerAttr(p_rsp->attrs[xx]))
        {
            (*p_num)++;
            UINT8_TO_BE_STREAM(p_data, p_rsp->attrs[xx]);
        }
    }

    len = *p_num + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_values_rsp
**
** Description      This function builds the List Application Setting Values
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_values_rsp (tAVRC_LIST_APP_VALUES_RSP *p_rsp,
    BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_num;
    UINT8   xx;
    UINT16  len;

    AVRC_TRACE_API0("avrc_bld_list_app_settings_values_rsp");

    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    /* get the existing length, if any, and also the num attributes */
    BE_STREAM_TO_UINT16(len, p_data);
    p_num = p_data;
    /* first time initialize the attribute count */
    if (len == 0)
    {
        *p_num = p_rsp->num_val;
        p_data++;
    }
    else
    {
        p_data = p_start + p_pkt->len;
        *p_num += p_rsp->num_val;
    }


    for (xx=0; xx<p_rsp->num_val; xx++)
    {
        UINT8_TO_BE_STREAM(p_data, p_rsp->vals[xx]);
    }

    len = *p_num + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_cur_app_setting_value_rsp
**
** Description      This function builds the Get Current Application Setting Value
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_cur_app_setting_value_rsp (tAVRC_GET_CUR_APP_VALUE_RSP *p_rsp,
    BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_count;
    UINT16  len;
    UINT8   xx;

    if (!p_rsp->p_vals)
    {
        AVRC_TRACE_ERROR0("avrc_bld_get_cur_app_setting_value_rsp NULL parameter");
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_get_cur_app_setting_value_rsp");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;
    if (len == 0)
    {
        /* first time initialize the attribute count */
        *p_count = 0;
        p_data++;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_rsp->num_val; xx++)
    {
        if (avrc_is_valid_player_attrib_value(p_rsp->p_vals[xx].attr_id, p_rsp->p_vals[xx].attr_val))
        {
            (*p_count)++;
            UINT8_TO_BE_STREAM(p_data, p_rsp->p_vals[xx].attr_id);
            UINT8_TO_BE_STREAM(p_data, p_rsp->p_vals[xx].attr_val);
        }
    }
    len = ((*p_count) << 1) + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_set_app_setting_value_rsp
**
** Description      This function builds the Set Application Setting Value
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_set_app_setting_value_rsp (tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    /* nothing to be added. */
    AVRC_TRACE_API0("avrc_bld_set_app_setting_value_rsp");
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_app_setting_text_rsp
**
** Description      This function builds the Get Application Settings Attribute Text
**                  or Get Application Settings Value Text response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_app_setting_text_rsp (tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_count;
    UINT16  len, len_left;
    UINT8   xx;
    tAVRC_STS   sts = AVRC_STS_NO_ERROR;
    UINT8       num_added = 0;

    if (!p_rsp->p_attrs)
    {
        AVRC_TRACE_ERROR0("avrc_bld_app_setting_text_rsp NULL parameter");
        return AVRC_STS_BAD_PARAM;
    }
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    len_left = GKI_get_buf_size(p_pkt) - BT_HDR_SIZE - p_pkt->offset - p_pkt->len;

    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;

    if (len == 0)
    {
        *p_count = 0;
        p_data++;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_rsp->num_attr; xx++)
    {
        if  (len_left < (p_rsp->p_attrs[xx].str_len + 4))
        {
            AVRC_TRACE_ERROR3("avrc_bld_app_setting_text_rsp out of room (str_len:%d, left:%d)",
                xx, p_rsp->p_attrs[xx].str_len, len_left);
            p_rsp->num_attr = num_added;
            sts = AVRC_STS_INTERNAL_ERR;
            break;
        }
        if ( !p_rsp->p_attrs[xx].str_len || !p_rsp->p_attrs[xx].p_str )
        {
            AVRC_TRACE_ERROR1("avrc_bld_app_setting_text_rsp NULL attr text[%d]", xx);
            continue;
        }
        UINT8_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].attr_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].charset_id);
        UINT8_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].str_len);
        ARRAY_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].p_str, p_rsp->p_attrs[xx].str_len);
        (*p_count)++;
        num_added++;
    }
    len = p_data - p_count;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return sts;
}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_attr_text_rsp
**
** Description      This function builds the Get Application Setting Attribute Text
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_attr_text_rsp (tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp,
    BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_get_app_setting_attr_text_rsp");
    return avrc_bld_app_setting_text_rsp(p_rsp, p_pkt);
}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_value_text_rsp
**
** Description      This function builds the Get Application Setting Value Text
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_value_text_rsp (tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp,
    BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_get_app_setting_value_text_rsp");
    return avrc_bld_app_setting_text_rsp(p_rsp, p_pkt);
}

/*******************************************************************************
**
** Function         avrc_bld_inform_charset_rsp
**
** Description      This function builds the Inform Displayable Character Set
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_charset_rsp (tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    /* nothing to be added. */
    AVRC_TRACE_API0("avrc_bld_inform_charset_rsp");
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_inform_battery_status_rsp
**
** Description      This function builds the Inform Battery Status
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_battery_status_rsp (tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    /* nothing to be added. */
    AVRC_TRACE_API0("avrc_bld_inform_battery_status_rsp");
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_elem_attrs_rsp
**
** Description      This function builds the Get Element Attributes
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_elem_attrs_rsp (tAVRC_GET_ELEM_ATTRS_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start, *p_len, *p_count;
    UINT16  len;
    UINT8   xx;

    AVRC_TRACE_API0("avrc_bld_get_elem_attrs_rsp");
    if (!p_rsp->p_attrs)
    {
        AVRC_TRACE_ERROR0("avrc_bld_get_elem_attrs_rsp NULL parameter");
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;

    if (len == 0)
    {
        *p_count = 0;
        p_data++;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_rsp->num_attr; xx++)
    {
        if (!AVRC_IS_VALID_MEDIA_ATTRIBUTE(p_rsp->p_attrs[xx].attr_id))
        {
            AVRC_TRACE_ERROR2("avrc_bld_get_elem_attrs_rsp invalid attr id[%d]: %d", xx, p_rsp->p_attrs[xx].attr_id);
            continue;
        }
        if ( !p_rsp->p_attrs[xx].name.p_str )
        {
            p_rsp->p_attrs[xx].name.str_len = 0;
        }
        UINT32_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].attr_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.charset_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.str_len);
        ARRAY_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.p_str, p_rsp->p_attrs[xx].name.str_len);
        (*p_count)++;
    }
    len = p_data - p_count;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_play_status_rsp
**
** Description      This function builds the Get Play Status
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_play_status_rsp (tAVRC_GET_PLAY_STATUS_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_get_play_status_rsp");
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2;

    /* add fixed lenth - song len(4) + song position(4) + status(1) */
    UINT16_TO_BE_STREAM(p_data, 9);
    UINT32_TO_BE_STREAM(p_data, p_rsp->song_len);
    UINT32_TO_BE_STREAM(p_data, p_rsp->song_pos);
    UINT8_TO_BE_STREAM(p_data, p_rsp->play_status);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_notify_rsp
**
** Description      This function builds the Notification response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_notify_rsp (tAVRC_REG_NOTIF_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len;
    UINT16  len = 0;
    UINT8   xx;
    tAVRC_STS status = AVRC_STS_NO_ERROR;

    AVRC_TRACE_API0("avrc_bld_notify_rsp");

    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    p_data += 2;

    UINT8_TO_BE_STREAM(p_data, p_rsp->event_id);
    switch (p_rsp->event_id)
    {
    case AVRC_EVT_PLAY_STATUS_CHANGE:       /* 0x01 */
        /* p_rsp->param.play_status >= AVRC_PLAYSTATE_STOPPED is always TRUE */
        if ((p_rsp->param.play_status <= AVRC_PLAYSTATE_REV_SEEK) ||
            (p_rsp->param.play_status == AVRC_PLAYSTATE_ERROR) )
        {
            UINT8_TO_BE_STREAM(p_data, p_rsp->param.play_status);
            len = 2;
        }
        else
        {
            AVRC_TRACE_ERROR0("bad play state");
            status = AVRC_STS_BAD_PARAM;
        }
        break;

    case AVRC_EVT_TRACK_CHANGE:             /* 0x02 */
        ARRAY_TO_BE_STREAM(p_data, p_rsp->param.track, AVRC_UID_SIZE);
        len = (UINT8)(AVRC_UID_SIZE + 1);
        break;

    case AVRC_EVT_TRACK_REACHED_END:        /* 0x03 */
    case AVRC_EVT_TRACK_REACHED_START:      /* 0x04 */
        len = 1;
        break;

    case AVRC_EVT_PLAY_POS_CHANGED:         /* 0x05 */
        UINT32_TO_BE_STREAM(p_data, p_rsp->param.play_pos);
        len = 5;
        break;

    case AVRC_EVT_BATTERY_STATUS_CHANGE:    /* 0x06 */
        if (AVRC_IS_VALID_BATTERY_STATUS(p_rsp->param.battery_status))
        {
            UINT8_TO_BE_STREAM(p_data, p_rsp->param.battery_status);
            len = 2;
        }
        else
        {
            AVRC_TRACE_ERROR0("bad battery status");
            status = AVRC_STS_BAD_PARAM;
        }
        break;

    case AVRC_EVT_SYSTEM_STATUS_CHANGE:     /* 0x07 */
        if (AVRC_IS_VALID_SYSTEM_STATUS(p_rsp->param.system_status))
        {
            UINT8_TO_BE_STREAM(p_data, p_rsp->param.system_status);
            len = 2;
        }
        else
        {
            AVRC_TRACE_ERROR0("bad system status");
            status = AVRC_STS_BAD_PARAM;
        }
        break;

    case AVRC_EVT_APP_SETTING_CHANGE:       /* 0x08 */
        if (p_rsp->param.player_setting.num_attr > AVRC_MAX_APP_SETTINGS)
            p_rsp->param.player_setting.num_attr = AVRC_MAX_APP_SETTINGS;

        if (p_rsp->param.player_setting.num_attr > 0)
        {
            UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.num_attr);
            len = 2;
            for (xx=0; xx<p_rsp->param.player_setting.num_attr; xx++)
            {
                if (avrc_is_valid_player_attrib_value(p_rsp->param.player_setting.attr_id[xx],
                    p_rsp->param.player_setting.attr_value[xx]))
                {
                    UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.attr_id[xx]);
                    UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.attr_value[xx]);
                }
                else
                {
                    AVRC_TRACE_ERROR0("bad player app seeting attribute or value");
                    status = AVRC_STS_BAD_PARAM;
                    break;
                }
                len += 2;
            }
        }
        else
            status = AVRC_STS_BAD_PARAM;
        break;

    case AVRC_EVT_AVAL_PLAYERS_CHANGE:
        len = EVT_AVAIL_PLAYER_CHANGE_RSP_LENGTH;
        break;

    case AVRC_EVT_ADDR_PLAYER_CHANGE:
        UINT16_TO_BE_STREAM(p_data,p_rsp->param.addr_player.player_id);
        UINT16_TO_BE_STREAM(p_data,p_rsp->param.addr_player.uid_counter);
        len = EVT_ADDR_PLAYER_CHANGE_RSP_LENGTH;
        break;

    default:
        status = AVRC_STS_BAD_PARAM;
        AVRC_TRACE_ERROR0("unknown event_id");
    }

    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_next_rsp
**
** Description      This function builds the Request Continue or Abort
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_next_rsp (tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    /* nothing to be added. */
    AVRC_TRACE_API0("avrc_bld_next_rsp");
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**
** Function      avrc_bld_set_address_player_rsp
**
** Description   This function builds the set address player response
**
** Returns       AVRC_STS_NO_ERROR, if the response is build successfully
**               Otherwise, the error code.
**
******************************************************************************/
static tAVRC_STS avrc_bld_set_address_player_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    tAVRC_STS status = AVRC_STS_NO_ERROR;
    AVRC_TRACE_API0(" avrc_bld_set_address_player_rsp");
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    /* To calculate length */
    p_data = p_start + 2;

    /* add fixed lenth status(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = (p_data - p_start);
    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_group_navigation_rsp
**
** Description      This function builds the Group Navigation
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS avrc_bld_group_navigation_rsp (UINT16 navi_id, BT_HDR *p_pkt)
{
    UINT8   *p_data;

    if (!AVRC_IS_VALID_GROUP(navi_id))
    {
        AVRC_TRACE_ERROR1("avrc_bld_group_navigation_rsp bad navigation op id: %d", navi_id);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_group_navigation_rsp");
    p_data = (UINT8 *)(p_pkt+1) + p_pkt->offset;
    UINT16_TO_BE_STREAM(p_data, navi_id);
    p_pkt->len = 2;
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**
** Function           avrc_bld_folder_item_values_rsp
**
** Description        This function builds the folder item response.
**
** Returns            AVRC_STS_NO_ERROR,if the response is built successfully
**                    otherwise error code
**
******************************************************************************/
static tAVRC_STS  avrc_bld_folder_item_values_rsp(tAVRC_GET_ITEMS_RSP *p_rsp, BT_HDR *p_pkt )
{
    UINT8 *p_data, *p_start, *p_length;
    UINT8 *item_length;
    UINT16 itemlength;
    UINT16 length = 0, item_numb = 0, i;

    AVRC_TRACE_DEBUG1(" avrc_bld_folder_item_values_rsp offset :x%x", p_pkt->offset);
    p_start       = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    /* As per AVRCP spec, first byte of response is PDU ID
     * and Response does not have any opcode
    */
    p_data        = p_start;
    /*First OCT carry PDU information */
    *p_data++      =  p_rsp->pdu;

    /* Refer Media Play list AVRCP 1.5 22.19 (Get Folder Items)
     * Mark a pointer to be filled once length is calculated at last
    */
    p_length       =  p_data;

    /*increment to adjust length*/
    p_data         = p_data + 2;
    /*Status is checked in Btif layer*/
    *p_data++     = p_rsp->status;
    if(p_rsp->status != AVRC_STS_NO_ERROR)
    {
        //TODO Response
        AVRC_TRACE_ERROR0(" ### Folder_item_values response error");
        return p_rsp->status;
    }
    /*UID Counter OCT 4 and 5*/
    UINT16_TO_BE_STREAM(p_data, p_rsp->uid_counter);
    /*Number of Items OCT 6 and 7*/
    item_numb = p_rsp->item_count;
    UINT16_TO_BE_STREAM(p_data, p_rsp->item_count);

    switch(p_rsp->p_item_list->item_type)
    {
        case AVRC_ITEM_PLAYER:
            for (i =0 ; i < item_numb ; ++i)
            {
                /* Item Type OCT 8 */
                *p_data++    = p_rsp->p_item_list[i].item_type; /* AVRC_ITEM_PLAYER, AVRC_ITEM_FOLDER, or AVRC_ITEM_MEDIA */
                /*Store item_length location and update it*/
                item_length  = p_data ;
                p_data       = p_data + 2 ;
                /* Player ID OCT 11 and 12*/
                UINT16_TO_BE_STREAM(p_data ,p_rsp->p_item_list[i].u.player.player_id) ;
                /*Major Player type , OCT 13*/
                *p_data++    = p_rsp->p_item_list[i].u.player.major_type;
                /*Player SUB type OCT 14 to 17*/
                UINT32_TO_BE_STREAM(p_data ,p_rsp->p_item_list[i].u.player.sub_type );
                /*Play Status OCT 18*/
                *p_data++   = p_rsp->p_item_list[i].u.player.play_status ;
                /*Feature BIT MASK OCT 19 to OCT 34*/
                ARRAY_TO_BE_STREAM(p_data,p_rsp->p_item_list[i].u.player.features ,AVRC_FEATURE_MASK_SIZE);
                /*Charset ID OCT 35 and 36*/
                UINT16_TO_BE_STREAM(p_data ,p_rsp->p_item_list[i].u.player.name.charset_id);
                /*Displayable name lenght OCT 37 and 38*/
                UINT16_TO_BE_STREAM(p_data ,p_rsp->p_item_list[i].u.player.name.str_len);
                /*Displayable Name OCT 39 + str lenght*/
                ARRAY_TO_BE_STREAM(p_data,p_rsp->p_item_list[i].u.player.name.p_str ,\
                                   p_rsp->p_item_list[i].u.player.name.str_len );
                itemlength = (UINT16)(p_data - item_length);
                itemlength = itemlength - 2;
                AVRC_TRACE_DEBUG1("itemlength = %d",itemlength);
                length     += itemlength + 3; //Add 2 OCTETS of param_length + 1 Media type
                UINT16_TO_BE_STREAM(item_length ,itemlength );
            }
            break;
        case AVRC_ITEM_FOLDER:
            /*Not Supported */
            AVRC_TRACE_ERROR0("### AVRC_ITEM_RESPONSE Not supported");
            break;
        case AVRC_ITEM_MEDIA:
            /* Not Supported */
            AVRC_TRACE_ERROR0("### AVRC_ITEM_Media Not supported");
            break;
        default :
            AVRC_TRACE_ERROR0("### Response not valid type");
            break;
    }
    //Update the item length item
    length    = length + 5; //Add explicit 5, 2 num items+ 2UID Counter + 1 status counter
    UINT16_TO_BE_STREAM(p_length ,length );
    p_pkt->len  = length + 3; //2 bytes of length + PDU
    AVRC_TRACE_DEBUG1("length = %d",length);
    return p_rsp->status ;
}

/**************************************************************************************
**
** Function                  avrcp_bld_setfolder_rsp
**
** Description
**
** Returns
**
************************************************************************************/
static tAVRC_STS avrcp_bld_setfolder_rsp (tAVRC_SET_BR_PLAYER_RSP *p_rsp, BT_HDR *p_pkt )
{
    UINT8 *p_data, *p_start, *p_length , *p_debug;
    UINT8 *item_length;
    UINT16 itemlength;
    UINT16 length=0 , item_numb=0 ,i;

    AVRC_TRACE_DEBUG1("avrcp_bld_setfolder_rsp offset :x%x", p_pkt->offset);
    p_start       = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_debug       = p_start;
    p_data        = p_start ; //1st byte is PDU and rsp does not have any OPCODE
    *p_data++      =  p_rsp->pdu;
    /* Refer Media Play list AVRCP 1.5 22.19 (Response for Set Folder Items)
     * Mark a pointer to be filled once length is calculated at last
    */
    p_length       =  p_data;
    /* increment to adjust length*/
    p_data         = p_data + 2 ;

    /* Status , TODO check for proper status else return error */
    *p_data++     = p_rsp->status;

    /* UID Counter OCT 4 and 5 */
    UINT16_TO_BE_STREAM(p_data ,p_rsp->uid_counter);

    /* Number of items OCT 6 to 9 */
    UINT32_TO_BE_STREAM(p_data ,p_rsp->num_items);
    /* Charset ID OCT 10 and 11 */
    UINT32_TO_BE_STREAM(p_data ,p_rsp->charset_id);
    /* Folder depth OCT 12 */
    *p_data++    = p_rsp->folder_depth;
    /* Charset ID */
    UINT32_TO_BE_STREAM(p_data ,p_rsp->charset_id);

    switch (p_rsp->folder_depth)
    {
        case 2:
            UINT16_TO_BE_STREAM(p_data, p_rsp->p_folders[0].str_len);
            ARRAY_TO_BE_STREAM(p_data,p_rsp->p_folders[0].p_str ,p_rsp->p_folders[0].str_len);
            UINT16_TO_BE_STREAM(p_data, p_rsp->p_folders[1].str_len);
            ARRAY_TO_BE_STREAM(p_data,p_rsp->p_folders[1].p_str ,p_rsp->p_folders[1].str_len);
            break;

        default:
            break;
    }
    itemlength = (UINT16)(p_data - p_start ) - 2;
    UINT16_TO_BE_STREAM(p_length, itemlength);
    p_pkt->len  = itemlength;
    AVRC_TRACE_DEBUG1("length = %d",itemlength);
    return p_rsp->status ;
}

/*******************************************************************************
**
** Function         avrc_bld_rejected_rsp
**
** Description      This function builds the General Response response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_rejected_rsp( tAVRC_RSP *p_rsp, BT_HDR *p_pkt )
{
    UINT8 *p_data, *p_start;

    AVRC_TRACE_API2("avrc_bld_rejected_rsp: status=%d, pdu:x%x", p_rsp->status, p_rsp->pdu);

    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2;
    AVRC_TRACE_DEBUG1("pdu:x%x", *p_start);

    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = p_data - p_start;

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_browse_rejected_rsp
**
** Description      This function builds the Browse Reject response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_browse_rejected_rsp (tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UINT8 *p_data, *p_start;

    AVRC_TRACE_API3("avrc_bld_browse_rejected_rsp: status=%d, pdu:x%x, offset=%d", p_rsp->status,
                      p_rsp->pdu, p_pkt->offset);

    p_start   = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data    = p_start;
    *p_data++ = p_rsp->pdu;

    UINT16_TO_BE_STREAM(p_data, 1); //Parameter length
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = p_data - p_start;
    AVRC_TRACE_DEBUG1("Browse rejected rsp length=%d", p_pkt->len);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_init_rsp_buffer
**
** Description      This function initializes the response buffer based on PDU
**
** Returns          NULL, if no GKI buffer or failure to build the message.
**                  Otherwise, the GKI buffer that contains the initialized message.
**
*******************************************************************************/
static BT_HDR *avrc_bld_init_rsp_buffer(tAVRC_RESPONSE *p_rsp)
{
    UINT16 offset = AVRC_MSG_PASS_THRU_OFFSET, chnl = AVCT_DATA_CTRL, len=AVRC_META_CMD_POOL_SIZE;
    BT_HDR *p_pkt=NULL;
    UINT8  opcode = avrc_opcode_from_pdu(p_rsp->pdu);

    AVRC_TRACE_API3("avrc_bld_init_rsp_buffer: pdu=%x, opcode=%x/%x", p_rsp->pdu, opcode,
        p_rsp->rsp.opcode);
    if (opcode != p_rsp->rsp.opcode && p_rsp->rsp.status != AVRC_STS_NO_ERROR &&
        avrc_is_valid_opcode(p_rsp->rsp.opcode))
    {
        opcode = p_rsp->rsp.opcode;
        AVRC_TRACE_API1("opcode=%x", opcode);
    }

    switch (opcode)
    {
    case AVRC_OP_PASS_THRU:
        offset  = AVRC_MSG_PASS_THRU_OFFSET;
        break;

    case AVRC_OP_VENDOR:
        offset  = AVRC_MSG_VENDOR_OFFSET;
        if (p_rsp->pdu == AVRC_PDU_GET_ELEMENT_ATTR)
            len     = AVRC_BROWSE_POOL_SIZE;
        break;
    }

    /* allocate and initialize the buffer */
    p_pkt = (BT_HDR *)GKI_getbuf(len);
    if (p_pkt)
    {
        UINT8 *p_data, *p_start;

        p_pkt->layer_specific = chnl;
        p_pkt->event    = opcode;
        p_pkt->offset   = offset;
        p_data = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
        p_start = p_data;

        /* pass thru - group navigation - has a two byte op_id, so dont do it here */
        if (opcode != AVRC_OP_PASS_THRU)
            *p_data++ = p_rsp->pdu;

        switch (opcode)
        {
        case AVRC_OP_VENDOR:
            /* reserved 0, packet_type 0 */
            UINT8_TO_BE_STREAM(p_data, 0);
            /* continue to the next "case to add length */
            /* add fixed lenth - 0 */
            UINT16_TO_BE_STREAM(p_data, 0);
            break;
        }

        p_pkt->len = (p_data - p_start);
    }
    p_rsp->rsp.opcode = opcode;
    return p_pkt;
}

/*******************************************************************************
**
** Function         AVRC_BldResponse
**
** Description      This function builds the given AVRCP response to the given
**                  GKI buffer
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS AVRC_BldResponse( UINT8 handle, tAVRC_RESPONSE *p_rsp, BT_HDR **pp_pkt)
{
    tAVRC_STS status = AVRC_STS_BAD_PARAM;
    BT_HDR *p_pkt;
    BOOLEAN alloc = FALSE;

    if (!p_rsp || !pp_pkt)
    {
        AVRC_TRACE_API2("AVRC_BldResponse. Invalid parameters passed. p_rsp=%p, pp_pkt=%p",
            p_rsp, pp_pkt);
        return AVRC_STS_BAD_PARAM;
    }

    if (*pp_pkt == NULL)
    {
        if ((*pp_pkt = avrc_bld_init_rsp_buffer(p_rsp)) == NULL)
        {
            AVRC_TRACE_API0("AVRC_BldResponse: Failed to initialize response buffer");
            return AVRC_STS_INTERNAL_ERR;
        }
        alloc = TRUE;
    }
    status = AVRC_STS_NO_ERROR;
    p_pkt = *pp_pkt;

    AVRC_TRACE_API2("AVRC_BldResponse: pdu=%x status=%x", p_rsp->rsp.pdu, p_rsp->rsp.status);
    if (p_rsp->rsp.status != AVRC_STS_NO_ERROR)
    {
        return( avrc_bld_rejected_rsp(&p_rsp->rsp, p_pkt) );
    }

    switch (p_rsp->pdu)
    {
    case AVRC_PDU_NEXT_GROUP:
    case AVRC_PDU_PREV_GROUP:
        status = avrc_bld_group_navigation_rsp(p_rsp->pdu, p_pkt);
        break;

    case AVRC_PDU_GET_CAPABILITIES:
        status = avrc_bld_get_capability_rsp(&p_rsp->get_caps, p_pkt);
        break;

    case AVRC_PDU_LIST_PLAYER_APP_ATTR:
        status = avrc_bld_list_app_settings_attr_rsp(&p_rsp->list_app_attr, p_pkt);
        break;

    case AVRC_PDU_LIST_PLAYER_APP_VALUES:
        status = avrc_bld_list_app_settings_values_rsp(&p_rsp->list_app_values, p_pkt);
        break;

    case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
        status = avrc_bld_get_cur_app_setting_value_rsp(&p_rsp->get_cur_app_val, p_pkt);
        break;

    case AVRC_PDU_SET_PLAYER_APP_VALUE:
        status = avrc_bld_set_app_setting_value_rsp(&p_rsp->set_app_val, p_pkt);
        break;

    case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
        status = avrc_bld_get_app_setting_attr_text_rsp(&p_rsp->get_app_attr_txt, p_pkt);
        break;

    case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
        status = avrc_bld_get_app_setting_value_text_rsp(&p_rsp->get_app_val_txt, p_pkt);
        break;

    case AVRC_PDU_INFORM_DISPLAY_CHARSET:
        status = avrc_bld_inform_charset_rsp(&p_rsp->inform_charset, p_pkt);
        break;

    case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:
        status = avrc_bld_inform_battery_status_rsp(&p_rsp->inform_battery_status, p_pkt);
        break;

    case AVRC_PDU_GET_ELEMENT_ATTR:
        status = avrc_bld_get_elem_attrs_rsp(&p_rsp->get_elem_attrs, p_pkt);
        break;

    case AVRC_PDU_GET_PLAY_STATUS:
        status = avrc_bld_get_play_status_rsp(&p_rsp->get_play_status, p_pkt);
        break;

    case AVRC_PDU_REGISTER_NOTIFICATION:
        status = avrc_bld_notify_rsp(&p_rsp->reg_notif, p_pkt);
        break;

    case AVRC_PDU_REQUEST_CONTINUATION_RSP:     /*        0x40 */
        status = avrc_bld_next_rsp(&p_rsp->continu, p_pkt);
        break;

    case AVRC_PDU_ABORT_CONTINUATION_RSP:       /*          0x41 */
        status = avrc_bld_next_rsp(&p_rsp->abort, p_pkt);
        break;

    case AVRC_PDU_SET_ADDRESSED_PLAYER: /*PDU 0x60*/
        status = avrc_bld_set_address_player_rsp(&p_rsp->addr_player, p_pkt);
        break;
    }

    if (alloc && (status != AVRC_STS_NO_ERROR) )
    {
        GKI_freebuf(p_pkt);
        *pp_pkt = NULL;
    }
    AVRC_TRACE_API1("AVRC_BldResponse: returning %d", status);
    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_init_browse_rsp_buffer
**
** Description      This function initializes the response buffer based on PDU
**
** Returns          NULL, if no GKI buffer or failure to build the message.
**                  Otherwise, the GKI buffer that contains the initialized message.
**
*******************************************************************************/
static BT_HDR *avrc_bld_init_browse_rsp_buffer(tAVRC_RESPONSE *p_rsp)
{
    UINT16 offset = AVCT_BROWSE_OFFSET;
    UINT16 chnl = AVCT_DATA_BROWSE;
    UINT16 len  = AVRC_BROWSE_POOL_SIZE;
    BT_HDR *p_pkt = NULL;

    AVRC_TRACE_API0("avrc_bld_init_browse_rsp_buffer ");
    /* allocate and initialize the buffer */
    p_pkt = (BT_HDR *)GKI_getbuf(len);

    if (p_pkt != NULL)
    {
        p_pkt->layer_specific = chnl;
        p_pkt->event    = AVRC_OP_BROWSE; /* Browsing Opcode */
        p_pkt->offset   = offset;
    }
    else
    {
        AVRC_TRACE_ERROR0("### browse_rsp_buffer BUFF not allocated");
    }
    return p_pkt;
}

/*******************************************************************************
**
** Function         AVRC_BldBrowseResponse
**
** Description      This function builds the given AVRCP response to the given
**                  GKI buffer
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS AVRC_BldBrowseResponse( UINT8 handle, tAVRC_RESPONSE *p_rsp, BT_HDR **pp_pkt)
{
    tAVRC_STS status = AVRC_STS_BAD_PARAM;
    BT_HDR *p_pkt;
    BOOLEAN alloc = FALSE;

    if (!p_rsp || !pp_pkt)
    {
        AVRC_TRACE_ERROR2("### BldResponse error p_rsp=%p, pp_pkt=%p",
                           p_rsp, pp_pkt);
        return AVRC_STS_BAD_PARAM;
    }

    if (*pp_pkt == NULL)
    {
        if ((*pp_pkt = avrc_bld_init_browse_rsp_buffer(p_rsp)) == NULL)
        {
            AVRC_TRACE_ERROR0("### BldResponse: Failed to initialize response buffer");
            return AVRC_STS_INTERNAL_ERR;
        }
        alloc = TRUE;
    }
    status = AVRC_STS_NO_ERROR;
    p_pkt = *pp_pkt;

    AVRC_TRACE_API2("BldResponse: pdu=%x status=%x", p_rsp->rsp.pdu, p_rsp->rsp.status);
    if (p_rsp->rsp.status != AVRC_STS_NO_ERROR)
    {
        AVRC_TRACE_ERROR0("###ERROR AVRC_BldBrowseResponse");
        return (avrc_bld_browse_rejected_rsp(&p_rsp->rsp, p_pkt));
    }

    switch (p_rsp->pdu)
    {
        case AVRC_PDU_GET_FOLDER_ITEMS:
            status = avrc_bld_folder_item_values_rsp(&p_rsp->get_items, p_pkt);
            break;

        case AVRC_PDU_SET_BROWSED_PLAYER:
            status = avrcp_bld_setfolder_rsp(&p_rsp->br_player, p_pkt);
            break;

        default :
            break;
    }
    if (alloc && (status != AVRC_STS_NO_ERROR) )
    {
        GKI_freebuf(p_pkt);
        *pp_pkt = NULL;
        AVRC_TRACE_ERROR1("### error status:%d",status);
    }
    return status;
}
#endif /* (AVRC_METADATA_INCLUDED == TRUE)*/

