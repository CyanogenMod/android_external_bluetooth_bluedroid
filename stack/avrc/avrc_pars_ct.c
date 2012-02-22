/*****************************************************************************
**
**  Name:       avrc_pars_ct.c
**
**  Description:Interface to AVRCP parse message functions for the Control Role
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
** Function         avrc_pars_vendor_rsp
**
** Description      This function parses the vendor specific commands defined by
**                  Bluetooth SIG
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
static tAVRC_STS avrc_pars_vendor_rsp(tAVRC_MSG_VENDOR *p_msg, tAVRC_RESPONSE *p_result, UINT8 *p_buf, UINT16 buf_len)
{
    tAVRC_STS  status = AVRC_STS_NO_ERROR;
    UINT8   *p = p_msg->p_vendor_data;
    UINT16  len;
    UINT8   xx, yy;
    tAVRC_NOTIF_RSP_PARAM   *p_param;
    tAVRC_APP_SETTING       *p_app_set;
    tAVRC_APP_SETTING_TEXT  *p_app_txt;
    tAVRC_ATTR_ENTRY        *p_entry;
    UINT32  *p_u32;
    UINT8   *p_u8;
    UINT16  size_needed;

    BE_STREAM_TO_UINT8 (p_result->pdu, p);
    p++; /* skip the reserved/packe_type byte */
    BE_STREAM_TO_UINT16 (len, p);
    AVRC_TRACE_DEBUG4("avrc_pars_vendor_rsp() ctype:0x%x pdu:0x%x, len:%d/0x%x", p_msg->hdr.ctype, p_result->pdu, len, len);
    if (p_msg->hdr.ctype == AVRC_RSP_REJ)
    {
        p_result->rsp.status = *p;
        return p_result->rsp.status;
    }

    switch (p_result->pdu)
    {
    case AVRC_PDU_GET_CAPABILITIES:         /* 0x10 */
        p_result->get_caps.capability_id = *p++;
        if (AVRC_IS_VALID_CAP_ID(p_result->get_caps.capability_id))
        {
            p_result->get_caps.count = *p++;
            if (p_result->get_caps.capability_id == AVRC_CAP_COMPANY_ID)
            {
                p_u32 = p_result->get_caps.param.company_id;
                for (xx=0; xx<p_result->get_caps.count; xx++)
                {
                    AVRC_BE_STREAM_TO_CO_ID (p_u32[xx], p);
                }
            }
            else
            {
                p_u8 = p_result->get_caps.param.event_id;
                for (xx=0; xx<p_result->get_caps.count; xx++)
                {
                    BE_STREAM_TO_UINT8 (p_u8[xx], p);
                }
            }
        }
        else
            status = AVRC_STS_BAD_PARAM;
        break;

    case AVRC_PDU_LIST_PLAYER_APP_ATTR:     /* 0x11 */
        BE_STREAM_TO_UINT8 (p_result->list_app_attr.num_attr, p);
        p_u8 = p_result->list_app_attr.attrs;
        for(xx=0, yy=0; xx< p_result->list_app_attr.num_attr; xx++)
        {
            /* only report the valid player app attributes */
            if (AVRC_IsValidPlayerAttr(*p))
                p_u8[yy++] = *p;
            p++;
        }
        p_result->list_app_attr.num_attr = yy;
        break;

    case AVRC_PDU_LIST_PLAYER_APP_VALUES:   /* 0x12 */
        BE_STREAM_TO_UINT8 (p_result->list_app_values.num_val, p);
        p_u8 = p_result->list_app_values.vals;
        for(xx=0; xx< p_result->list_app_values.num_val; xx++)
        {
            p_u8[xx] = *p++;
        }
        break;

    case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE: /* 0x13 */
        BE_STREAM_TO_UINT8 (p_result->get_cur_app_val.num_val, p);
        size_needed = sizeof(tAVRC_APP_SETTING);
        if (p_buf)
        {
            p_result->get_cur_app_val.p_vals = (tAVRC_APP_SETTING *)p_buf;
            p_app_set = p_result->get_cur_app_val.p_vals;
            for(xx=0; ((xx< p_result->get_cur_app_val.num_val) && (buf_len > size_needed)); xx++)
            {
                buf_len -= size_needed;
                BE_STREAM_TO_UINT8  (p_app_set[xx].attr_id, p);
                BE_STREAM_TO_UINT8  (p_app_set[xx].attr_val, p);
            }
            if (xx != p_result->get_cur_app_val.num_val)
            {
                AVRC_TRACE_ERROR2("GET_CUR_PLAYER_APP_VALUE not enough room:%d orig num_val:%d",
                    xx, p_result->get_cur_app_val.num_val);
                p_result->get_cur_app_val.num_val = xx;
            }
        }
        else
        {
            AVRC_TRACE_ERROR2("GET_CUR_PLAYER_APP_VALUE not enough room:len: %d needed for struct: %d", buf_len, size_needed);
            status = AVRC_STS_INTERNAL_ERR;
        }
        break;

    case AVRC_PDU_SET_PLAYER_APP_VALUE:     /* 0x14 */
    case AVRC_PDU_INFORM_DISPLAY_CHARSET:  /* 0x17 */
    case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:/* 0x18 */
        /* no additional parameters */
        if (len != 0)
            status = AVRC_STS_INTERNAL_ERR;
        break;

    case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT: /* 0x15 */
    case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:/* 0x16 */
        BE_STREAM_TO_UINT8 (p_result->get_app_attr_txt.num_attr, p);
        size_needed = sizeof(tAVRC_APP_SETTING_TEXT) * p_result->get_app_attr_txt.num_attr;
        if (p_buf && (buf_len > size_needed))
        {
            p_result->get_app_attr_txt.p_attrs = (tAVRC_APP_SETTING_TEXT *)p_buf;
            p_app_txt = p_result->get_app_attr_txt.p_attrs;
            p_u8 = p_buf + size_needed;
            buf_len -= size_needed;
            for(xx=0; xx< p_result->get_app_attr_txt.num_attr; xx++)
            {
                BE_STREAM_TO_UINT8  (p_app_txt[xx].attr_id, p);
                BE_STREAM_TO_UINT16 (p_app_txt[xx].charset_id, p);
                BE_STREAM_TO_UINT8  (p_app_txt[xx].str_len, p);
                p_app_txt[xx].p_str = p_u8;
                if (buf_len > p_app_txt[xx].str_len)
                {
                    BE_STREAM_TO_ARRAY(p, p_u8, p_app_txt[xx].str_len);
                    p_u8 += p_app_txt[xx].str_len;
                    buf_len -= p_app_txt[xx].str_len;
                }
                else
                {
                    AVRC_TRACE_ERROR4("GET_CUR_PLAYER_APP_VALUE not enough room:[%d] len orig/left: %d/%d, orig num_attr:%d",
                        xx, p_app_txt[xx].str_len, buf_len, p_result->get_app_attr_txt.num_attr);
                    p_app_txt[xx].str_len = (UINT8)buf_len;
                    BE_STREAM_TO_ARRAY(p, p_u8, p_app_txt[xx].str_len);
                    p_result->get_app_attr_txt.num_attr = xx+1;
                    status = AVRC_STS_INTERNAL_ERR;
                    break;
                }
            }
        }
        else
        {
            AVRC_TRACE_ERROR2("GET_CUR_PLAYER_APP_VALUE not enough room:len: %d needed for struct: %d", buf_len, size_needed);
            status = AVRC_STS_INTERNAL_ERR;
        }
        break;

    case AVRC_PDU_GET_ELEMENT_ATTR:         /* 0x20 */
        BE_STREAM_TO_UINT8 (p_result->get_elem_attrs.num_attr, p);
        size_needed = sizeof(tAVRC_ATTR_ENTRY) * p_result->get_elem_attrs.num_attr;
        if (p_buf && (buf_len > size_needed))
        {
            p_result->get_elem_attrs.p_attrs = (tAVRC_ATTR_ENTRY *)p_buf;
            p_entry = p_result->get_elem_attrs.p_attrs;
            p_u8 = p_buf + size_needed;
            buf_len -= size_needed;
            for(xx=0; xx< p_result->get_elem_attrs.num_attr; xx++)
            {
                BE_STREAM_TO_UINT32 (p_entry[xx].attr_id, p);
                BE_STREAM_TO_UINT16 (p_entry[xx].name.charset_id, p);
                BE_STREAM_TO_UINT16 (p_entry[xx].name.str_len, p);
                p_entry[xx].name.p_str = p_u8;
                if (buf_len > p_entry[xx].name.str_len)
                {
                    BE_STREAM_TO_ARRAY(p, p_u8, p_entry[xx].name.str_len);
                    p_u8 += p_entry[xx].name.str_len;
                    buf_len -= p_entry[xx].name.str_len;
                }
                else
                {
                    AVRC_TRACE_ERROR4("GET_ELEMENT_ATTR not enough room:[%d] len orig/left: %d/%d, orig num_attr:%d",
                        xx, p_entry[xx].name.str_len, buf_len, p_result->get_elem_attrs.num_attr);
                    p_entry[xx].name.str_len = buf_len;
                    BE_STREAM_TO_ARRAY(p, p_u8, p_entry[xx].name.str_len);
                    p_result->get_elem_attrs.num_attr = xx + 1;
                    status = AVRC_STS_INTERNAL_ERR;
                    break;
                }
            }
        }
        else
        {
            AVRC_TRACE_ERROR2("GET_ELEMENT_ATTR not enough room:len: %d needed for struct: %d", buf_len, size_needed);
            status = AVRC_STS_INTERNAL_ERR;
        }
        break;

    case AVRC_PDU_GET_PLAY_STATUS:          /* 0x30 */
        BE_STREAM_TO_UINT32 (p_result->get_play_status.song_len, p);
        BE_STREAM_TO_UINT32 (p_result->get_play_status.song_pos, p);
        BE_STREAM_TO_UINT8 (p_result->get_play_status.play_status, p);
        if (len != 9)
            status = AVRC_STS_INTERNAL_ERR;
        break;

    case AVRC_PDU_REGISTER_NOTIFICATION:    /* 0x31 */
        BE_STREAM_TO_UINT8 (p_result->reg_notif.event_id, p);
        p_param = &p_result->reg_notif.param;
        switch (p_result->reg_notif.event_id)
        {
        case AVRC_EVT_PLAY_STATUS_CHANGE:   /* 0x01 */
            BE_STREAM_TO_UINT8 (p_param->play_status, p);
            if ((p_param->play_status > AVRC_PLAYSTATE_REV_SEEK) &&
                (p_param->play_status != AVRC_PLAYSTATE_ERROR) )
            {
                status = AVRC_STS_BAD_PARAM;
            }
            break;

        case AVRC_EVT_TRACK_CHANGE:         /* 0x02 */
            BE_STREAM_TO_ARRAY (p, p_param->track, AVRC_UID_SIZE);
            break;

        case AVRC_EVT_TRACK_REACHED_END:    /* 0x03 */
        case AVRC_EVT_TRACK_REACHED_START:  /* 0x04 */
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        case AVRC_EVT_NOW_PLAYING_CHANGE:   /* 0x09 */
        case AVRC_EVT_AVAL_PLAYERS_CHANGE:  /* 0x0a */
#endif /* (AVRC_ADV_CTRL_INCLUDED == TRUE) */
            /* these events do not have additional parameters */
            break;

        case AVRC_EVT_PLAY_POS_CHANGED:     /* 0x05 */
            BE_STREAM_TO_UINT32 (p_param->play_pos, p);
            break;

        case AVRC_EVT_BATTERY_STATUS_CHANGE:/* 0x06 */
            BE_STREAM_TO_UINT8 (p_param->battery_status, p);
            if (!AVRC_IS_VALID_BATTERY_STATUS(p_param->battery_status))
            {
                status = AVRC_STS_BAD_PARAM;
            }
            break;

        case AVRC_EVT_SYSTEM_STATUS_CHANGE: /* 0x07 */
            BE_STREAM_TO_UINT8 (p_param->system_status, p);
            if (!AVRC_IS_VALID_SYSTEM_STATUS(p_param->system_status))
            {
                status = AVRC_STS_BAD_PARAM;
            }
            break;

        case AVRC_EVT_APP_SETTING_CHANGE:   /* 0x08 */
            BE_STREAM_TO_UINT8 (p_param->player_setting.num_attr, p);
            if (p_param->player_setting.num_attr > AVRC_MAX_APP_SETTINGS)
                p_param->player_setting.num_attr = AVRC_MAX_APP_SETTINGS;
            for (xx=0; xx<p_param->player_setting.num_attr; xx++)
            {
                BE_STREAM_TO_UINT8 (p_param->player_setting.attr_id[xx], p);
                BE_STREAM_TO_UINT8 (p_param->player_setting.attr_value[xx], p);
                if (!avrc_is_valid_player_attrib_value(p_param->player_setting.attr_id[xx], p_param->player_setting.attr_value[xx]))
                {
                    status = AVRC_STS_BAD_PARAM;
                    break;
                }
            }
            break;

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        case AVRC_EVT_ADDR_PLAYER_CHANGE:   /* 0x0b */
            BE_STREAM_TO_UINT16 (p_param->addr_player.player_id, p);
            BE_STREAM_TO_UINT16 (p_param->addr_player.uid_counter, p);
            break;

        case AVRC_EVT_UIDS_CHANGE:          /* 0x0c */
            BE_STREAM_TO_UINT16 (p_param->uid_counter, p);
            break;

        case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
            BE_STREAM_TO_UINT8 (p_param->volume, p);
            break;
#endif /* (AVRC_ADV_CTRL_INCLUDED == TRUE) */

        default:
            status = AVRC_STS_BAD_PARAM;
            break;
        }
        break;

    /* case AVRC_PDU_REQUEST_CONTINUATION_RSP: 0x40 */
    /* case AVRC_PDU_ABORT_CONTINUATION_RSP:   0x41 */

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    case AVRC_PDU_SET_ABSOLUTE_VOLUME:      /* 0x50 */
    case AVRC_PDU_SET_ADDRESSED_PLAYER:     /* 0x60 */
    case AVRC_PDU_PLAY_ITEM:                /* 0x74 */
    case AVRC_PDU_ADD_TO_NOW_PLAYING:       /* 0x90 */
        BE_STREAM_TO_UINT8 (p_result->volume.volume, p);
        if (len != 1)
            status = AVRC_STS_INTERNAL_ERR;
        break;
#endif /* (AVRC_ADV_CTRL_INCLUDED == TRUE) */

    default:
        status = AVRC_STS_BAD_CMD;
        break;
    }

    return status;
}

#if (AVCT_BROWSE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         avrc_pars_browsing_rsp
**
** Description      This function parses the commands that go through the
**                  browsing channel
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
static tAVRC_STS avrc_pars_browsing_rsp(tAVRC_MSG_BROWSE *p_msg, tAVRC_RESPONSE *p_result, UINT8 *p_buf, UINT16 buf_len)
{
    tAVRC_STS  status = AVRC_STS_NO_ERROR;
    UINT8   *p = p_msg->p_browse_data;
    UINT16  len;
    int     i, count;
    UINT8   *p_left = p_buf;
    tAVRC_ITEM  *p_item;
    tAVRC_ITEM_PLAYER   *p_player;
    tAVRC_ITEM_FOLDER   *p_folder;
    tAVRC_ITEM_MEDIA    *p_media;
    tAVRC_ATTR_ENTRY    *p_attrs;
    UINT16  item_len;
    UINT8   xx;
    UINT16  size_needed;

    p_result->pdu = *p++;
    BE_STREAM_TO_UINT16 (len, p);
    BE_STREAM_TO_UINT8 (status, p);
    AVRC_TRACE_DEBUG4("avrc_pars_browsing_rsp() pdu:0x%x, len:%d/0x%x, status:0x%x", p_result->pdu, len, len, status);
    if (status != AVRC_STS_NO_ERROR)
        return status;

    switch (p_result->pdu)
    {
    case AVRC_PDU_SET_BROWSED_PLAYER:   /* 0x70 */
        BE_STREAM_TO_UINT16 (p_result->br_player.uid_counter, p);
        BE_STREAM_TO_UINT32 (p_result->br_player.num_items, p);
        BE_STREAM_TO_UINT16 (p_result->br_player.charset_id, p);
        BE_STREAM_TO_UINT8 (p_result->br_player.folder_depth, p);
        p_result->br_player.p_folders = NULL;
        if (p_result->br_player.folder_depth == 0)
            return status;

        size_needed = sizeof(tAVRC_NAME) * p_result->br_player.folder_depth;
        if (p_buf && (buf_len > size_needed))
        {
            p_result->br_player.p_folders = (tAVRC_NAME *)p_buf;
            p_left = p_buf + size_needed;
            count = p_result->br_player.folder_depth;
            buf_len -= size_needed;
            for (i=0; i<count; i++)
            {
                p_result->br_player.p_folders[i].p_str = p_left;
                BE_STREAM_TO_UINT16 (p_result->br_player.p_folders[i].str_len, p);
                if (buf_len > p_result->br_player.p_folders[i].str_len)
                {
                    BE_STREAM_TO_ARRAY (p, p_result->br_player.p_folders[i].p_str, p_result->br_player.p_folders[i].str_len);
                    p_left += p_result->br_player.p_folders[i].str_len;
                    buf_len -= p_result->br_player.p_folders[i].str_len;
                }
                else
                {
                    AVRC_TRACE_ERROR4("SET_BROWSED_PLAYER not enough room:[%d] len orig/left: %d/%d, orig depth:%d",
                        i, p_result->br_player.p_folders[i].str_len, buf_len, p_result->br_player.folder_depth);
                    p_result->br_player.p_folders[i].str_len = buf_len;
                    BE_STREAM_TO_ARRAY (p, p_result->br_player.p_folders[i].p_str, p_result->br_player.p_folders[i].str_len);
                    p_result->br_player.folder_depth = i+1;
                    status = AVRC_STS_INTERNAL_ERR;
                    break;
                }
            }
        }
        else
        {
            AVRC_TRACE_ERROR2("SET_BROWSED_PLAYER not enough room:len: %d needed for struct: %d", buf_len, size_needed);
            status = AVRC_STS_INTERNAL_ERR;
        }
        break;

    case AVRC_PDU_GET_FOLDER_ITEMS:     /* 0x71 */
        BE_STREAM_TO_UINT16 (p_result->get_items.uid_counter, p);
        BE_STREAM_TO_UINT16 (p_result->get_items.item_count, p);
        p_result->get_items.p_item_list = NULL;
        if (p_result->get_items.item_count == 0)
            return status;
        size_needed = sizeof(tAVRC_ITEM) * p_result->get_items.item_count;
        if (p_buf && (buf_len > size_needed))
        {
            p_result->get_items.p_item_list = p_item = (tAVRC_ITEM *)p_buf;
            p_left = p_buf + size_needed;
            count = p_result->get_items.item_count;
            buf_len -= size_needed;
            for (i=0; i<count; i++)
            {
                BE_STREAM_TO_UINT8 (p_item[i].item_type, p);
                BE_STREAM_TO_UINT16 (item_len, p);
                AVRC_TRACE_DEBUG3("[%d] type:%d len left:%d", i, p_item[i].item_type, buf_len);
                switch(p_item[i].item_type)
                {
                case AVRC_ITEM_PLAYER:
                    p_player = &p_item[i].u.player;
                    BE_STREAM_TO_UINT16 (p_player->player_id, p);
                    BE_STREAM_TO_UINT8 (p_player->major_type, p);
                    BE_STREAM_TO_UINT32 (p_player->sub_type, p);
                    BE_STREAM_TO_UINT8 (p_player->play_status, p);
                    BE_STREAM_TO_ARRAY (p, p_player->features, AVRC_FEATURE_MASK_SIZE);
                    BE_STREAM_TO_UINT16 (p_player->name.charset_id, p);
                    BE_STREAM_TO_UINT16 (p_player->name.str_len, p);
                    p_player->name.p_str = p_left;
                    if (buf_len > p_player->name.str_len)
                    {
                        p_left += p_player->name.str_len;
                        BE_STREAM_TO_ARRAY (p, p_player->name.p_str, p_player->name.str_len);
                        buf_len -= p_player->name.str_len;
                    }
                    else
                    {
                        AVRC_TRACE_ERROR4("GET_FOLDER_ITEMS player not enough room:[%d] len orig/left: %d/%d, orig item count:%d",
                            i, p_player->name.str_len, buf_len, p_result->get_items.item_count);
                        p_player->name.str_len = buf_len;
                        BE_STREAM_TO_ARRAY (p, p_player->name.p_str, p_player->name.str_len);
                        p_result->get_items.item_count = i+1;
                        return AVRC_STS_INTERNAL_ERR;
                    }
                    break;

                case AVRC_ITEM_FOLDER:
                    p_folder = &p_item[i].u.folder;
                    BE_STREAM_TO_ARRAY (p, p_folder->uid, AVRC_UID_SIZE);
                    BE_STREAM_TO_UINT8 (p_folder->type, p);
                    BE_STREAM_TO_UINT8 (p_folder->playable, p);
                    BE_STREAM_TO_UINT16 (p_folder->name.charset_id, p);
                    BE_STREAM_TO_UINT16 (p_folder->name.str_len, p);
                    p_folder->name.p_str = p_left;
                    if (buf_len > p_folder->name.str_len)
                    {
                        p_left += p_folder->name.str_len;
                        BE_STREAM_TO_ARRAY (p, p_folder->name.p_str, p_folder->name.str_len);
                        buf_len -= p_folder->name.str_len;
                    }
                    else
                    {
                        AVRC_TRACE_ERROR4("GET_FOLDER_ITEMS folder not enough room:[%d] len orig/left: %d/%d, orig item count:%d",
                            i, p_folder->name.str_len, buf_len, p_result->get_items.item_count);
                        p_folder->name.str_len = buf_len;
                        BE_STREAM_TO_ARRAY (p, p_folder->name.p_str, p_folder->name.str_len);
                        p_result->get_items.item_count = i+1;
                        return AVRC_STS_INTERNAL_ERR;
                    }
                    break;

                case AVRC_ITEM_MEDIA:
                    p_media = &p_item[i].u.media;
                    BE_STREAM_TO_ARRAY (p, p_media->uid, AVRC_UID_SIZE);
                    BE_STREAM_TO_UINT8 (p_media->type, p);
                    BE_STREAM_TO_UINT16 (p_media->name.charset_id, p);
                    BE_STREAM_TO_UINT16 (p_media->name.str_len, p);
                    p_media->name.p_str = p_left;
                    if (buf_len < p_media->name.str_len)
                    {
                        AVRC_TRACE_ERROR4("GET_FOLDER_ITEMS media not enough room:[%d] len orig/left: %d/%d, orig item count:%d",
                            i, p_media->name.str_len, buf_len, p_result->get_items.item_count);
                        p_media->name.str_len = buf_len;
                        BE_STREAM_TO_ARRAY (p, p_media->name.p_str, p_media->name.str_len);
                        p_media->attr_count = 0;
                        p_media->p_attr_list = NULL;
                        p_result->get_items.item_count = i+1;
                        return AVRC_STS_INTERNAL_ERR;
                    }
                    p_left += p_media->name.str_len;
                    buf_len -= p_media->name.str_len;
                    BE_STREAM_TO_ARRAY (p, p_media->name.p_str, p_media->name.str_len);
                    BE_STREAM_TO_UINT8 (p_media->attr_count, p);
                    size_needed = sizeof(tAVRC_ATTR_ENTRY) * p_media->attr_count;
                    if (buf_len < size_needed)
                    {
                        AVRC_TRACE_ERROR4("GET_FOLDER_ITEMS media not enough room:[%d] attr_count orig/left: %d/%d, orig item count:%d",
                            i, p_media->attr_count, buf_len, p_result->get_items.item_count);
                        p_media->name.str_len = buf_len;
                        BE_STREAM_TO_ARRAY (p, p_media->name.p_str, p_media->name.str_len);
                        p_media->attr_count = 0;
                        p_media->p_attr_list = NULL;
                        p_result->get_items.item_count = i+1;
                        return AVRC_STS_INTERNAL_ERR;
                    }
                    p_media->p_attr_list = p_attrs = (tAVRC_ATTR_ENTRY *)p_left;
                    p_left += size_needed;
                    buf_len -= size_needed;
                    for (xx= 0; xx<p_media->attr_count; xx++)
                    {
                        BE_STREAM_TO_UINT32 (p_attrs[xx].attr_id, p);
                        BE_STREAM_TO_UINT16 (p_attrs[xx].name.charset_id, p);
                        BE_STREAM_TO_UINT16 (p_attrs[xx].name.str_len, p);
                        p_attrs[xx].name.p_str = p_left;
                        if (buf_len > p_attrs[xx].name.str_len)
                        {
                            p_left += p_attrs[xx].name.str_len;
                            BE_STREAM_TO_ARRAY (p, p_attrs[xx].name.p_str, p_attrs[xx].name.str_len);
                            buf_len -= p_attrs[xx].name.str_len;
                        }
                        else
                        {
                            AVRC_TRACE_ERROR4("GET_FOLDER_ITEMS media not enough room:[%d] attr name len orig/left: %d/%d, orig item count:%d",
                                i, p_attrs[xx].name.str_len, buf_len, p_result->get_items.item_count);
                            p_attrs[xx].name.str_len = buf_len;
                            BE_STREAM_TO_ARRAY (p, p_attrs[xx].name.p_str, p_attrs[xx].name.str_len);
                            p_media->attr_count = xx+1;
                            p_result->get_items.item_count = i+1;
                            return AVRC_STS_INTERNAL_ERR;
                        }
                    }
                    break;
                }
            }
        }
        break;

    case AVRC_PDU_CHANGE_PATH:          /* 0x72 */
        BE_STREAM_TO_UINT32 (p_result->chg_path.num_items, p);
        break;

    case AVRC_PDU_GET_ITEM_ATTRIBUTES:  /* 0x73 */
        BE_STREAM_TO_UINT8 (p_result->get_attrs.attr_count, p);
        p_result->get_attrs.p_attr_list = p_attrs = NULL;
        if (p_result->get_attrs.attr_count == 0)
            return status;

        size_needed = sizeof(tAVRC_ATTR_ENTRY) * p_result->get_attrs.attr_count;
        if (p_buf && (buf_len > size_needed))
        {
            p_result->get_attrs.p_attr_list = p_attrs = (tAVRC_ATTR_ENTRY *)p_left;
            p_left += size_needed;
            buf_len -= size_needed;
            for (xx= 0; xx<p_result->get_attrs.attr_count; xx++)
            {
                BE_STREAM_TO_UINT32 (p_attrs[xx].attr_id, p);
                BE_STREAM_TO_UINT16 (p_attrs[xx].name.charset_id, p);
                BE_STREAM_TO_UINT16 (p_attrs[xx].name.str_len, p);
                p_attrs[xx].name.p_str = p_left;
                if (buf_len > p_attrs[xx].name.str_len)
                {
                    p_left += p_attrs[xx].name.str_len;
                    BE_STREAM_TO_ARRAY (p, p_attrs[xx].name.p_str, p_attrs[xx].name.str_len);
                    buf_len -= p_attrs[xx].name.str_len;
                }
                else
                {
                    AVRC_TRACE_ERROR4("GET_ITEM_ATTRIBUTES not enough room:[%d] len orig/left: %d/%d, orig attr_count:%d",
                        xx, p_attrs[xx].name.str_len, buf_len, p_result->get_attrs.attr_count);
                    p_attrs[xx].name.str_len = buf_len;
                    BE_STREAM_TO_ARRAY (p, p_attrs[xx].name.p_str, p_attrs[xx].name.str_len);
                    p_result->get_attrs.attr_count = xx+1;
                    status = AVRC_STS_INTERNAL_ERR;
                    break;
                }
            }
        }
        break;

    case AVRC_PDU_SEARCH:               /* 0x80 */
        BE_STREAM_TO_UINT16 (p_result->search.uid_counter, p);
        BE_STREAM_TO_UINT32 (p_result->search.num_items, p);
        break;

    default:
        status = AVRC_STS_BAD_CMD;
        break;
    }

    return status;
}
#endif /* (AVCT_BROWSE_INCLUDED == TRUE)*/

/*******************************************************************************
**
** Function         AVRC_ParsResponse
**
** Description      This function is a superset of AVRC_ParsMetadata to parse the response.
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
tAVRC_STS AVRC_ParsResponse (tAVRC_MSG *p_msg, tAVRC_RESPONSE *p_result, UINT8 *p_buf, UINT16 buf_len)
{
    tAVRC_STS  status = AVRC_STS_INTERNAL_ERR;
    UINT16  id;

    if (p_msg && p_result)
    {
        switch (p_msg->hdr.opcode)
        {
        case AVRC_OP_VENDOR:     /*  0x00    Vendor-dependent commands */
            status = avrc_pars_vendor_rsp(&p_msg->vendor, p_result, p_buf, buf_len);
            break;

        case AVRC_OP_PASS_THRU:  /*  0x7C    panel subunit opcode */
            status = avrc_pars_pass_thru(&p_msg->pass, &id);
            if (status == AVRC_STS_NO_ERROR)
            {
                p_result->pdu = (UINT8)id;
            }
            break;

#if (AVCT_BROWSE_INCLUDED == TRUE)
        case AVRC_OP_BROWSE:
            status = avrc_pars_browsing_rsp(&p_msg->browse, p_result, p_buf, buf_len);
            break;
#endif /* (AVCT_BROWSE_INCLUDED == TRUE) */

        default:
            AVRC_TRACE_ERROR1("AVRC_ParsCommand() unknown opcode:0x%x", p_msg->hdr.opcode);
            break;
        }
        p_result->rsp.opcode = p_msg->hdr.opcode;
    p_result->rsp.status = status;
    }
    return status;
}


#endif /* (AVRC_METADATA_INCLUDED == TRUE) */
