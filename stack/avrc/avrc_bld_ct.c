/*****************************************************************************
**
**  Name:       avrc_bld_ct.c
**
**  Description:Interface to AVRCP build message functions for the Control Role
**
**  Copyright (c) 2008-2008, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <string.h>

#include "gki.h"
#include "avrc_api.h"
#include "avrc_defs.h"
#include "avrc_int.h"

/*****************************************************************************
**  Global data
*****************************************************************************/


#if (AVRC_METADATA_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         avrc_bld_get_capability_cmd
**
** Description      This function builds the Get Capability command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_capability_cmd (tAVRC_GET_CAPS_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    if (!AVRC_IS_VALID_CAP_ID(p_cmd->capability_id))
    {
        AVRC_TRACE_ERROR1("avrc_bld_get_capability_cmd bad capability_id:0x%x", p_cmd->capability_id);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_get_capability_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    /* add fixed lenth - capability_id(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    /* add the capability_id */
    UINT8_TO_BE_STREAM(p_data, p_cmd->capability_id);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_attr_cmd
**
** Description      This function builds the List Application Settings Attribute
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_attr_cmd (tAVRC_CMD *p_cmd, BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_list_app_settings_attr_cmd");
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_values_cmd
**
** Description      This function builds the List Application Setting Values
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_values_cmd (tAVRC_LIST_APP_VALUES_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    if (!AVRC_IsValidPlayerAttr(p_cmd->attr_id))
    {
        AVRC_TRACE_ERROR1("avrc_bld_list_app_settings_values_cmd bad attr:0x%x", p_cmd->attr_id);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_list_app_settings_values_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    /* add fixed lenth - attr_id(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    /* add the attr_id */
    UINT8_TO_BE_STREAM(p_data, p_cmd->attr_id);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_cur_app_setting_value_cmd
**
** Description      This function builds the Get Current Application Setting Value
**                  or the Get Application Setting Attribute Text command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_cur_app_setting_value_cmd (tAVRC_GET_CUR_APP_VALUE_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len;
    UINT16  len = 0;
    UINT8   xx;
    UINT8   *p_count;

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_len = p_data = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;
    if (len == 0)
    {
        /* first time initialize the attribute count */
        *p_count = 0;
        p_data++;
        len = 1;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_cmd->num_attr; xx++)
    {
        if (AVRC_IsValidPlayerAttr(p_cmd->attrs[xx]))
        {
            (*p_count)++;
            UINT8_TO_BE_STREAM(p_data, p_cmd->attrs[xx]);
            len++;
        }
    }
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_set_app_setting_value_cmd
**
** Description      This function builds the Set Application Setting Value
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_set_app_setting_value_cmd (tAVRC_SET_APP_VALUE_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len;
    UINT16  len;
    UINT8   xx;
    UINT8   *p_count;

    if (!p_cmd->p_vals)
    {
        AVRC_TRACE_ERROR0("avrc_bld_set_app_setting_value_cmd NULL parameter");
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API1("avrc_bld_set_app_setting_value_cmd num_val:%d", p_cmd->num_val);
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
        len = 1;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }


    for (xx=0; xx<p_cmd->num_val; xx++)
    {
        AVRC_TRACE_DEBUG3("[%d] id/val = 0x%x/0x%x", xx, p_cmd->p_vals[xx].attr_id, p_cmd->p_vals[xx].attr_val);
        if (avrc_is_valid_player_attrib_value(p_cmd->p_vals[xx].attr_id, p_cmd->p_vals[xx].attr_val))
        {
            (*p_count)++;
            UINT8_TO_BE_STREAM(p_data, p_cmd->p_vals[xx].attr_id);
            UINT8_TO_BE_STREAM(p_data, p_cmd->p_vals[xx].attr_val);
            len += 2;
        }
    }

    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_attr_text_cmd
**
** Description      This function builds the Get Application Setting Attribute Text
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_attr_text_cmd (tAVRC_GET_APP_ATTR_TXT_CMD *p_cmd, BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_get_app_setting_attr_text_cmd");
    return avrc_bld_get_cur_app_setting_value_cmd((tAVRC_GET_CUR_APP_VALUE_CMD *)p_cmd, p_pkt);

}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_value_text_cmd
**
** Description      This function builds the Get Application Setting Value Text
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_value_text_cmd (tAVRC_GET_APP_VAL_TXT_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len;
    UINT16  len = 0;
    UINT8   xx;
    UINT8   *p_count;

    AVRC_TRACE_API0("avrc_bld_get_app_setting_value_text_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */

    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data + 1;
    if (len == 0)
    {
        /* first time initialize the attribute count */
        UINT8_TO_BE_STREAM(p_data, p_cmd->attr_id);
        *p_count = 0;
        p_data++;
        len = 2;
    }
    else
    {
        p_data = p_start + p_pkt->len;
    }

    for (xx=0; xx<p_cmd->num_val; xx++)
    {
        (*p_count)++;
        UINT8_TO_BE_STREAM(p_data, p_cmd->vals[xx]);
        len++;
    }
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_inform_charset_cmd
**
** Description      This function builds the Inform Displayable Character Set
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_charset_cmd (tAVRC_INFORM_CHARSET_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT16  len = 0;
    UINT8   xx;

    AVRC_TRACE_API0("avrc_bld_inform_charset_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    len = 1 + (p_cmd->num_id << 1);
    UINT16_TO_BE_STREAM(p_data, len);

    UINT8_TO_BE_STREAM(p_data, p_cmd->num_id);
    for (xx=0; xx<p_cmd->num_id; xx++)
    {
        UINT16_TO_BE_STREAM(p_data, p_cmd->charsets[xx]);
    }

    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_inform_battery_status_cmd
**
** Description      This function builds the Inform Battery Status 
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_battery_status_cmd (tAVRC_BATTERY_STATUS_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    if (!AVRC_IS_VALID_BATTERY_STATUS(p_cmd->battery_status))
    {
        AVRC_TRACE_ERROR1("avrc_bld_inform_battery_status_cmd bad battery_status:0x%x", p_cmd->battery_status);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_inform_battery_status_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */
    /* add fixed lenth - battery_status(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    /* add the battery_status */
    UINT8_TO_BE_STREAM(p_data, p_cmd->battery_status);

    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_elem_attrs_cmd
**
** Description      This function builds the Get Element Attributes
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_elem_attrs_cmd (tAVRC_GET_ELEM_ATTRS_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len;
    UINT16  len = 0;
    UINT8   xx;
    UINT8   *p_count;

    AVRC_TRACE_API0("avrc_bld_get_elem_attrs_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_len = p_start + 2; /* pdu + rsvd */
    p_data = p_len + 2;

    /* 8 byte identifier 0 - PLAYING - the only valid one */
    UINT32_TO_BE_STREAM(p_data, 0);
    UINT32_TO_BE_STREAM(p_data, 0);

    p_count = p_data;
    p_data++;
    *p_count = 0;
    len = 9;
    for (xx=0; xx<p_cmd->num_attr; xx++)
    {
        if (AVRC_IS_VALID_MEDIA_ATTRIBUTE(p_cmd->attrs[xx]))
        {
            (*p_count)++;
            UINT32_TO_BE_STREAM(p_data, p_cmd->attrs[xx]);
            len += 4;
        }
    }
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_play_status_cmd
**
** Description      This function builds the Get Play Status 
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_play_status_cmd (tAVRC_CMD *p_cmd, BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_get_play_status_cmd");
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_notify_cmd
**
** Description      This function builds the Register Notification command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_notify_cmd (tAVRC_REG_NOTIF_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_notify_cmd");
    if (!AVRC_IS_VALID_EVENT_ID(p_cmd->event_id))
    {
        AVRC_TRACE_ERROR1("avrc_bld_notify_cmd bad event_id:0x%x", p_cmd->event_id);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    /* add fixed lenth 5 - event_id (1) + interval(4) */
    UINT16_TO_BE_STREAM(p_data, 5);
    UINT8_TO_BE_STREAM(p_data, p_cmd->event_id);
    UINT32_TO_BE_STREAM(p_data, p_cmd->param);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_next_cmd
**
** Description      This function builds the Request Continue or Abort command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_next_cmd (tAVRC_NEXT_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_next_cmd");

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    /* add fixed lenth 1 - pdu_id (1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_cmd->target_pdu);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_group_navigation_cmd
**
** Description      This function builds the Group Navigation
**                  command.
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS avrc_bld_group_navigation_cmd (UINT16 navi_id, BT_HDR *p_pkt)
{
    UINT8   *p_data;

    if (!AVRC_IS_VALID_GROUP(navi_id))
    {
        AVRC_TRACE_ERROR1("avrc_bld_group_navigation_cmd bad navigation op id: %d", navi_id);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API0("avrc_bld_group_navigation_cmd");
    p_data = (UINT8 *)(p_pkt+1) + p_pkt->offset;
    UINT16_TO_BE_STREAM(p_data, navi_id);
    p_pkt->len = 2;
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**  the following commands are introduced in AVRCP 1.4
*****************************************************************************/

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         avrc_bld_set_addr_player_cmd
**
** Description      This function builds the Set Addresses Player command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_set_addr_player_cmd (tAVRC_SET_ADDR_PLAYER_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_set_addr_player_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */
    /* add fixed lenth - player_id(2) */
    UINT16_TO_BE_STREAM(p_data, 2);
    UINT16_TO_BE_STREAM(p_data, p_cmd->player_id);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_set_abs_volume_cmd
**
** Description      This function builds the Set Absolute Volume command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_set_abs_volume_cmd (tAVRC_SET_VOLUME_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_set_abs_volume_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */
    /* add fixed lenth 1 - volume (1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, (AVRC_MAX_VOLUME & p_cmd->volume));
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_set_browsed_player_cmd
**
** Description      This function builds the Set Browsed Player command.
**
**                  This message goes through the Browsing channel and is
**                  valid only when AVCT_BROWSE_INCLUDED compile option is TRUE
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
#if (AVCT_BROWSE_INCLUDED == TRUE)
static tAVRC_STS avrc_bld_set_browsed_player_cmd (tAVRC_SET_BR_PLAYER_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_set_browsed_player_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 1; /* pdu */
    /* add fixed lenth - player_id(2) */
    UINT16_TO_BE_STREAM(p_data, 2);
    UINT16_TO_BE_STREAM(p_data, p_cmd->player_id);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}
#endif

/*******************************************************************************
**
** Function         avrc_bld_get_folder_items_cmd
**
** Description      This function builds the Get Folder Items command.
**
**                  This message goes through the Browsing channel and is
**                  valid only when AVCT_BROWSE_INCLUDED compile option is TRUE
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
#if (AVCT_BROWSE_INCLUDED == TRUE)
static tAVRC_STS avrc_bld_get_folder_items_cmd (tAVRC_GET_ITEMS_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len, xx;
    UINT16  len = 0;

    AVRC_TRACE_API0("avrc_bld_get_folder_items_cmd");
    if (p_cmd->scope > AVRC_SCOPE_NOW_PLAYING || p_cmd->start_item > p_cmd->end_item)
    {
        AVRC_TRACE_ERROR3("bad scope:0x%x or range (%d-%d)", p_cmd->scope, p_cmd->start_item, p_cmd->end_item);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_len = p_start + 1; /* pdu */
    p_data = p_len + 2;

    UINT8_TO_BE_STREAM(p_data, p_cmd->scope);
    UINT32_TO_BE_STREAM(p_data, p_cmd->start_item);
    UINT32_TO_BE_STREAM(p_data, p_cmd->end_item);
    /* do not allow us to send the command with attribute list when scope is player list */
    if (p_cmd->scope == AVRC_SCOPE_PLAYER_LIST)
        p_cmd->attr_count = 0;

    UINT8_TO_BE_STREAM(p_data, p_cmd->attr_count);
    len = 10;
    if ((p_cmd->attr_count != AVRC_FOLDER_ITEM_COUNT_NONE) &&
        (p_cmd->attr_count > 0))
    {
        if (p_cmd->p_attr_list)
        {
            for (xx=0; xx<p_cmd->attr_count; xx++)
            {
                UINT32_TO_BE_STREAM(p_data, p_cmd->p_attr_list[xx]);
                len += 4;
            }
        }
        else
        {
            AVRC_TRACE_ERROR1("attr_count:%d but NULL p_attr_list", p_cmd->attr_count);
            return AVRC_STS_BAD_PARAM;
        }
    }

    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}
#endif

/*******************************************************************************
**
** Function         avrc_bld_change_path_cmd
**
** Description      This function builds the Change Path command.
**
**                  This message goes through the Browsing channel and is
**                  valid only when AVCT_BROWSE_INCLUDED compile option is TRUE
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
#if (AVCT_BROWSE_INCLUDED == TRUE)
static tAVRC_STS avrc_bld_change_path_cmd (tAVRC_CHG_PATH_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_change_path_cmd");
    if (p_cmd->direction != AVRC_DIR_UP && p_cmd->direction != AVRC_DIR_DOWN)
    {
        AVRC_TRACE_ERROR1("AVRC_BldChangePathCmd bad direction:%d", p_cmd->direction);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 1; /* pdu */
    /* add fixed lenth - uid_counter(2) + direction(1) + uid(8) */
    UINT16_TO_BE_STREAM(p_data, 11);
    UINT16_TO_BE_STREAM(p_data, p_cmd->uid_counter);
    *p_data++ = p_cmd->direction;
    ARRAY_TO_BE_STREAM(p_data, p_cmd->folder_uid, AVRC_UID_SIZE);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}
#endif

/*******************************************************************************
**
** Function         avrc_bld_get_item_attrs_cmd
**
** Description      This function builds the Get Item Attributes command.
**
**                  This message goes through the Browsing channel and is
**                  valid only when AVCT_BROWSE_INCLUDED compile option is TRUE
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
#if (AVCT_BROWSE_INCLUDED == TRUE)
static tAVRC_STS avrc_bld_get_item_attrs_cmd (tAVRC_GET_ATTRS_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT8   *p_len, xx;
    UINT16  len;
    UINT8   *p_count;

    AVRC_TRACE_API0("avrc_bld_get_item_attrs_cmd");
    if (p_cmd->scope > AVRC_SCOPE_NOW_PLAYING)
    {
        AVRC_TRACE_ERROR1("bad scope:%d", p_cmd->scope);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_len = p_start + 1; /* pdu */
    p_data = p_len + 2;

    UINT8_TO_BE_STREAM(p_data, p_cmd->scope);
    ARRAY_TO_BE_STREAM(p_data, p_cmd->uid, AVRC_UID_SIZE);
    UINT16_TO_BE_STREAM(p_data, p_cmd->uid_counter);
    //UINT8_TO_BE_STREAM(p_data, p_cmd->attr_count);
    p_count = p_data++;
    len = AVRC_UID_SIZE + 4;
    *p_count = 0;

    if (p_cmd->attr_count>0)
    {
        if (p_cmd->p_attr_list)
        {
            for (xx=0; xx<p_cmd->attr_count; xx++)
            {
                if (AVRC_IS_VALID_MEDIA_ATTRIBUTE(p_cmd->p_attr_list[xx]))
                {
                    (*p_count)++;
                    UINT32_TO_BE_STREAM(p_data, p_cmd->p_attr_list[xx]);
                    len += 4;
                }
#if (BT_USE_TRACES == TRUE)
                else
                {
                    AVRC_TRACE_ERROR1("invalid attr id:%d", p_cmd->p_attr_list[xx]);
                }
#endif
            }
        }
#if (BT_USE_TRACES == TRUE)
        else
        {
            AVRC_TRACE_ERROR1("attr_count:%d, NULL p_attr_list", p_cmd->attr_count);
        }
#endif
    }
    UINT16_TO_BE_STREAM(p_len, len);

    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}
#endif

/*******************************************************************************
**
** Function         avrc_bld_search_cmd
**
** Description      This function builds the Search command.
**
**                  This message goes through the Browsing channel and is
**                  valid only when AVCT_BROWSE_INCLUDED compile option is TRUE
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
#if (AVCT_BROWSE_INCLUDED == TRUE)
static tAVRC_STS avrc_bld_search_cmd (tAVRC_SEARCH_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;
    UINT16  len;

    AVRC_TRACE_API0("avrc_bld_search_cmd");
    if (!p_cmd->string.p_str)
    {
        AVRC_TRACE_ERROR0("null string");
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 1; /* pdu */
    /* variable lenth */
    len = p_cmd->string.str_len + 4;
    UINT16_TO_BE_STREAM(p_data, len);

    UINT16_TO_BE_STREAM(p_data, p_cmd->string.charset_id);
    UINT16_TO_BE_STREAM(p_data, p_cmd->string.str_len);
    ARRAY_TO_BE_STREAM(p_data, p_cmd->string.p_str, p_cmd->string.str_len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}
#endif

/*******************************************************************************
**
** Function         avrc_bld_play_item_cmd
**
** Description      This function builds the Play Item command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_play_item_cmd (tAVRC_PLAY_ITEM_CMD *p_cmd, BT_HDR *p_pkt)
{
    UINT8   *p_data, *p_start;

    AVRC_TRACE_API0("avrc_bld_play_item_cmd");
    /* get the existing length, if any, and also the num attributes */
    p_start = (UINT8 *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2; /* pdu + rsvd */

    /* fixed lenth - scope(1) + uid(8) + uid_counter(2) */
    UINT16_TO_BE_STREAM(p_data, 11);

    UINT8_TO_BE_STREAM(p_data, p_cmd->scope);
    ARRAY_TO_BE_STREAM(p_data, p_cmd->uid, AVRC_UID_SIZE);
    UINT16_TO_BE_STREAM(p_data, p_cmd->uid_counter);
    p_pkt->len = (p_data - p_start);

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_add_to_now_playing_cmd
**
** Description      This function builds the Add to Now Playing command.
**                  
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_add_to_now_playing_cmd (tAVRC_ADD_TO_PLAY_CMD *p_cmd, BT_HDR *p_pkt)
{
    AVRC_TRACE_API0("avrc_bld_add_to_now_playing_cmd");
    return avrc_bld_play_item_cmd((tAVRC_PLAY_ITEM_CMD *)p_cmd, p_pkt);
}

#endif /* AVRC_ADV_CTRL_INCLUDED=TRUE */

/*******************************************************************************
**
** Function         avrc_bld_init_cmd_buffer
**
** Description      This function initializes the command buffer based on PDU
**
** Returns          NULL, if no GKI buffer or failure to build the message.
**                  Otherwise, the GKI buffer that contains the initialized message.
**
*******************************************************************************/
static BT_HDR *avrc_bld_init_cmd_buffer(tAVRC_COMMAND *p_cmd)
{
    UINT16 offset, chnl = AVCT_DATA_CTRL, len=AVRC_META_CMD_POOL_SIZE;
    BT_HDR *p_pkt=NULL;
    UINT8  opcode;

    opcode = avrc_opcode_from_pdu(p_cmd->pdu);
    AVRC_TRACE_API2("avrc_bld_init_cmd_buffer: pdu=%x, opcode=%x", p_cmd->pdu, opcode);

    switch (opcode)
    {
#if (AVCT_BROWSE_INCLUDED == TRUE)
    case AVRC_OP_BROWSE:
        chnl    = AVCT_DATA_BROWSE;
        offset  = AVCT_BROWSE_OFFSET;
        len     = AVRC_BROWSE_POOL_SIZE;
        break;
#endif /* AVCT_BROWSE_INCLUDED */

    case AVRC_OP_PASS_THRU:
        offset  = AVRC_MSG_PASS_THRU_OFFSET;
        break;

    case AVRC_OP_VENDOR:
        offset  = AVRC_MSG_VENDOR_OFFSET;
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
            *p_data++ = p_cmd->pdu;

        switch (opcode)
        {
        case AVRC_OP_VENDOR:
            /* reserved 0, packet_type 0 */
            UINT8_TO_BE_STREAM(p_data, 0);
            /* continue to the next "case to add length */
#if (AVCT_BROWSE_INCLUDED == TRUE)
        case AVRC_OP_BROWSE:
#endif
            /* add fixed lenth - 0 */
            UINT16_TO_BE_STREAM(p_data, 0);
            break;
        }

        p_pkt->len = (p_data - p_start);
    }
    p_cmd->cmd.opcode = opcode;
    return p_pkt;
}

/*******************************************************************************
**
** Function         AVRC_BldCommand
**
** Description      This function builds the given AVRCP command to the given 
**                  GKI buffer
**
** Returns          AVRC_STS_NO_ERROR, if the command is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS AVRC_BldCommand( tAVRC_COMMAND *p_cmd, BT_HDR **pp_pkt)
{
    tAVRC_STS status = AVRC_STS_BAD_PARAM;
    BT_HDR  *p_pkt;
    BOOLEAN alloc = FALSE;

    AVRC_TRACE_API2("AVRC_BldCommand: pdu=%x status=%x", p_cmd->cmd.pdu, p_cmd->cmd.status);
    if (!p_cmd || !pp_pkt)
    {
        AVRC_TRACE_API2("AVRC_BldCommand. Invalid parameters passed. p_cmd=%p, pp_pkt=%p", p_cmd, pp_pkt);
        return AVRC_STS_BAD_PARAM;
    }

    if (*pp_pkt == NULL)
    {
        if ((*pp_pkt = avrc_bld_init_cmd_buffer(p_cmd)) == NULL)
        {
            AVRC_TRACE_API0("AVRC_BldCommand: Failed to initialize command buffer");
            return AVRC_STS_INTERNAL_ERR;
        }
        alloc = TRUE;
    }
    status = AVRC_STS_NO_ERROR;
    p_pkt = *pp_pkt;

    switch (p_cmd->pdu)
    {
    case AVRC_PDU_NEXT_GROUP:                   /* 0x00 */
    case AVRC_PDU_PREV_GROUP:                   /* 0x01 */
        status = avrc_bld_group_navigation_cmd (p_cmd->pdu, p_pkt);
        break;

    case AVRC_PDU_GET_CAPABILITIES:
        status = avrc_bld_get_capability_cmd(&p_cmd->get_caps, p_pkt);
        break;

    case AVRC_PDU_LIST_PLAYER_APP_ATTR:
        status = avrc_bld_list_app_settings_attr_cmd(&p_cmd->list_app_attr, p_pkt);
        break;

    case AVRC_PDU_LIST_PLAYER_APP_VALUES:
        status = avrc_bld_list_app_settings_values_cmd(&p_cmd->list_app_values, p_pkt);
        break;

    case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
        status = avrc_bld_get_cur_app_setting_value_cmd(&p_cmd->get_cur_app_val, p_pkt);
        break;

    case AVRC_PDU_SET_PLAYER_APP_VALUE:
        status = avrc_bld_set_app_setting_value_cmd(&p_cmd->set_app_val, p_pkt);
        break;

    case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
        status = avrc_bld_get_app_setting_attr_text_cmd(&p_cmd->get_app_attr_txt, p_pkt);
        break;

    case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
        status = avrc_bld_get_app_setting_value_text_cmd(&p_cmd->get_app_val_txt, p_pkt);
        break;

    case AVRC_PDU_INFORM_DISPLAY_CHARSET:
        status = avrc_bld_inform_charset_cmd(&p_cmd->inform_charset, p_pkt);
        break;

    case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:
        status = avrc_bld_inform_battery_status_cmd(&p_cmd->inform_battery_status, p_pkt);
        break;

    case AVRC_PDU_GET_ELEMENT_ATTR:
        status = avrc_bld_get_elem_attrs_cmd(&p_cmd->get_elem_attrs, p_pkt);
        break;

    case AVRC_PDU_GET_PLAY_STATUS:
        status = avrc_bld_get_play_status_cmd(&p_cmd->get_play_status, p_pkt);
        break;

    case AVRC_PDU_REGISTER_NOTIFICATION:
        status = avrc_bld_notify_cmd(&p_cmd->reg_notif, p_pkt);
        break;

    case AVRC_PDU_REQUEST_CONTINUATION_RSP:     /*        0x40 */
        status = avrc_bld_next_cmd(&p_cmd->continu, p_pkt);
        break;

    case AVRC_PDU_ABORT_CONTINUATION_RSP:       /*          0x41 */
        status = avrc_bld_next_cmd(&p_cmd->abort, p_pkt);
        break;

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    case AVRC_PDU_SET_ABSOLUTE_VOLUME:         /* 0x50 */
        status = avrc_bld_set_abs_volume_cmd(&p_cmd->volume, p_pkt);
        break;

    case AVRC_PDU_SET_ADDRESSED_PLAYER:        /* 0x60 */    
        status = avrc_bld_set_addr_player_cmd(&p_cmd->addr_player, p_pkt);
        break;

    case AVRC_PDU_PLAY_ITEM:                   /* 0x74 */    
        status = avrc_bld_play_item_cmd(&p_cmd->play_item, p_pkt);
        break;

    case AVRC_PDU_ADD_TO_NOW_PLAYING:          /* 0x90 */    
        status = avrc_bld_add_to_now_playing_cmd(&p_cmd->add_to_play, p_pkt);
        break;

#if (AVCT_BROWSE_INCLUDED == TRUE)
    case AVRC_PDU_SET_BROWSED_PLAYER:          /* 0x70 */    
        status = avrc_bld_set_browsed_player_cmd(&p_cmd->br_player, p_pkt);
        break;

    case AVRC_PDU_GET_FOLDER_ITEMS:            /* 0x71 */   
        status = avrc_bld_get_folder_items_cmd(&p_cmd->get_items, p_pkt);
        break;

    case AVRC_PDU_CHANGE_PATH:                 /* 0x72 */    
        status = avrc_bld_change_path_cmd(&p_cmd->chg_path, p_pkt);
        break;

    case AVRC_PDU_GET_ITEM_ATTRIBUTES:         /* 0x73 */    
        status = avrc_bld_get_item_attrs_cmd(&p_cmd->get_attrs, p_pkt);
        break;

    case AVRC_PDU_SEARCH:                      /* 0x80 */    
        status = avrc_bld_search_cmd(&p_cmd->search, p_pkt);
        break;
#endif
#endif

    }

    if (alloc && (status != AVRC_STS_NO_ERROR) )
    {
        GKI_freebuf(p_pkt);
        *pp_pkt = NULL;
    }
    AVRC_TRACE_API1("AVRC_BldCommand: returning %d", status);
    return status;
}
#endif /* (AVRC_METADATA_INCLUDED == TRUE) */

