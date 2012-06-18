/************************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

/************************************************************************************
 *
 *  Filename:      btif_hl.h
 *
 *  Description:
 *
 ***********************************************************************************/

#ifndef BTIF_HL_H
#define BTIF_HL_H

/************************************************************************************
**  Functions
************************************************************************************/


#define BTIF_HL_DATA_TYPE_NONE               0x0000
#define BTIF_HL_DATA_TYPE_PULSE_OXIMETER     0x1004   /* from BT assigned number */
#define BTIF_HL_DATA_TYPE_BLOOD_PRESSURE_MON 0x1007
#define BTIF_HL_DATA_TYPE_BODY_THERMOMETER   0x1008
#define BTIF_HL_DATA_TYPE_BODY_WEIGHT_SCALE  0x100F
#define BTIF_HL_DATA_TYPE_GLUCOSE_METER      0x1011
#define BTIF_HL_DATA_TYPE_STEP_COUNTER       0x1068

#define BTIF_HL_CCH_NUM_FILTER_ELEMS            3
#define BTIF_HL_APPLICATION_NAME_LEN          512


#define BTIF_HL_NV_MAX_APPS   16

typedef struct
{
    UINT8 mdep_cfg_idx;
    int data_type;
    tBTA_HL_MDEP_ID peer_mdep_id;
} btif_hl_extra_mdl_cfg_t;


typedef struct
{
    tBTA_HL_MDL_CFG         base;
    btif_hl_extra_mdl_cfg_t extra;
} btif_hl_mdl_cfg_t;

typedef struct
{
    btif_hl_mdl_cfg_t                 mdl_cfg[BTA_HL_NUM_MDL_CFGS];
}btif_hl_nv_mdl_data_t;


typedef struct
{
    tBTA_HL_SUP_FEATURE             sup_feature;
    tBTA_HL_DCH_CFG                 channel_type[BTA_HL_NUM_MDEPS];
    char                            srv_name[BTA_SERVICE_NAME_LEN +1];
    char                            srv_desp[BTA_SERVICE_DESP_LEN +1];
    char                            provider_name[BTA_PROVIDER_NAME_LEN +1];
    char                            application_name[BTIF_HL_APPLICATION_NAME_LEN +1];
}btif_hl_nv_app_data_t;


typedef struct
{
    BOOLEAN in_use;
    UINT16  use_freq;
}btif_hl_nv_app_t;

typedef struct
{
    btif_hl_nv_app_t   app[BTIF_HL_NV_MAX_APPS];
}btif_hl_nv_app_cb_t;



typedef struct
{
    UINT8 app_nv_idx;
    BOOLEAN active;
    UINT8 app_idx;
    btif_hl_nv_app_data_t app_data;
}btif_hl_app_data_t;

typedef struct
{
    BOOLEAN               is_app_read;
    btif_hl_nv_app_cb_t   app_cb;
    BUFFER_Q              app_queue;
}btif_hl_nv_cb_t;

typedef enum
{
    BTIF_HL_SOC_STATE_IDLE,
    BTIF_HL_SOC_STATE_W4_ADD,
    BTIF_HL_SOC_STATE_W4_CONN,
    BTIF_HL_SOC_STATE_W4_READ,
    BTIF_HL_SOC_STATE_W4_REL
} btif_hl_soc_state_t;

typedef struct
{
    int                   channel_id;
    BD_ADDR           bd_addr;
    UINT8                 mdep_cfg_idx;
    int                   max_s;
    int                   socket_id[2];
    UINT8                 app_idx;
    UINT8                 mcl_idx;
    UINT8                 mdl_idx;
    btif_hl_soc_state_t   state;
}btif_hl_soc_cb_t;

typedef struct
{
    UINT16              data_type;
    UINT16              max_tx_apdu_size;
    UINT16              max_rx_apdu_size;
} btif_hl_data_type_cfg_t;

typedef enum
{
    BTIF_HL_STATE_DISABLED,
    BTIF_HL_STATE_DISABLING,
    BTIF_HL_STATE_ENABLED,
    BTIF_HL_STATE_ENABLING,

}btif_hl_state_t;

typedef enum
{
    BTIF_HL_CCH_OP_NONE,
    BTIF_HL_CCH_OP_MDEP_FILTERING,
    BTIF_HL_CCH_OP_MATCHED_CTRL_PSM,
    BTIF_HL_CCH_OP_DCH_OPEN,
    BTIF_HL_CCH_OP_DCH_RECONNECT,
    BTIF_HL_CCH_OP_DCH_ECHO_TEST
}btif_hl_cch_op_t;

typedef enum
{
    BTIF_HL_PEND_DCH_OP_NONE,
    BTIF_HL_PEND_DCH_OP_DELETE_MDL,
    BTIF_HL_PEND_DCH_OP_OPEN,
    BTIF_HL_PEND_DCH_OP_RECONNECT
} btif_hl_pend_dch_op_t;

typedef enum
{
    BTIF_HL_DCH_OP_NONE,
    BTIF_HL_DCH_OP_DISC
} btif_hl_dch_op_t;


typedef struct
{
    UINT16                  data_type;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
} btif_hl_filter_elem_t;


typedef struct
{
    UINT8                       num_elems;
    btif_hl_filter_elem_t       elem[BTIF_HL_CCH_NUM_FILTER_ELEMS];
} btif_hl_cch_filter_t;

typedef struct
{
    BOOLEAN                 in_use;
    UINT16                  mdl_id;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    btif_hl_dch_op_t        dch_oper;
    tBTA_HL_MDEP_ID         local_mdep_id;
    UINT8                   local_mdep_cfg_idx;
    tBTA_HL_DCH_CFG         local_cfg;
    tBTA_HL_MDEP_ID         peer_mdep_id;
    UINT16                  peer_data_type;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
    tBTA_HL_DCH_MODE        dch_mode;
    tBTA_SEC                sec_mask;
    BOOLEAN                 is_the_first_reliable;
    BOOLEAN                 delete_mdl;
    UINT16                  mtu;
    tMCA_CHNL_CFG           chnl_cfg;
    UINT16                  tx_size;
    UINT8                   *p_tx_pkt;
    UINT8                   *p_rx_pkt;
    BOOLEAN                 cong;
    btif_hl_soc_cb_t        *p_scb;
    int                     channel_id;
}btif_hl_mdl_cb_t;

typedef enum
{
    BTIF_HL_CHAN_CB_STATE_NONE,
    BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING,
    BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING,

    BTIF_HL_CHAN_CB_STATE_DISCONNECTING_PENDING,
    BTIF_HL_CHAN_CB_STATE_DISCONNECTED_PENDING,
    BTIF_HL_CHAN_CB_STATE_DESTROYED_PENDING,
}btif_hl_chan_cb_state_t;
typedef struct
{
    int                             channel_id;
    int                             mdep_cfg_idx;
    BOOLEAN                         in_use;
    btif_hl_chan_cb_state_t         cb_state;
    btif_hl_pend_dch_op_t           op;
    BD_ADDR                         bd_addr;
    BOOLEAN                         abort_pending;
}btif_hl_pending_chan_cb_t;

typedef struct
{
    btif_hl_mdl_cb_t                mdl[BTA_HL_NUM_MDLS_PER_MCL];
    BOOLEAN                         in_use;
    BOOLEAN                         is_connected;
    UINT16                          req_ctrl_psm;
    UINT16                          ctrl_psm;
    UINT16                          data_psm;
    BD_ADDR                         bd_addr;
    UINT16                          cch_mtu;
    tBTA_SEC                        sec_mask;
    tBTA_HL_MCL_HANDLE              mcl_handle;
    btif_hl_pending_chan_cb_t       pcb;
    BOOLEAN                         valid_sdp_idx;
    UINT8                           sdp_idx;
    tBTA_HL_SDP                     sdp;
    btif_hl_cch_op_t                cch_oper;
    BOOLEAN                         cch_timer_active;
    TIMER_LIST_ENT                  cch_timer;
}btif_hl_mcl_cb_t;

typedef struct
{
    BOOLEAN         active;
    UINT16          mdl_id;
    UINT8           mdep_cfg_idx;
    BD_ADDR     bd_addr;
    int channel_id;
} btif_hl_delete_mdl_t;

typedef struct
{
    btif_hl_mcl_cb_t                mcb[BTA_HL_NUM_MCLS]; /* application Control Blocks */
    BOOLEAN                         in_use;              /* this CB is in use*/
    BOOLEAN                         reg_pending;
    BOOLEAN                         is_new_app;
    UINT8                           app_nv_idx;
    UINT8                           app_id;
    //UINT32                          sdp_handle;    /* SDP record handle */
    tBTA_HL_SUP_FEATURE             sup_feature;
    tBTA_HL_DCH_CFG                 channel_type[BTA_HL_NUM_MDEPS];
    tBTA_HL_SDP_INFO_IND            sdp_info_ind;
    btif_hl_cch_filter_t            filter;

    btif_hl_mdl_cfg_t               mdl_cfg[BTA_HL_NUM_MDL_CFGS];
    int                             mdl_cfg_channel_id[BTA_HL_NUM_MDL_CFGS];

    btif_hl_delete_mdl_t            delete_mdl;
    tBTA_HL_DEVICE_TYPE             dev_type;
    tBTA_HL_APP_HANDLE              app_handle;
    UINT16                          sec_mask;   /* Security mask for BTM_SetSecurityLevel() */
    char                            srv_name[BTA_SERVICE_NAME_LEN +1];        /* service name to be used in the SDP; null terminated*/
    char                            srv_desp[BTA_SERVICE_DESP_LEN +1];        /* service description to be used in the SDP; null terminated */
    char                            provider_name[BTA_PROVIDER_NAME_LEN +1];   /* provide name to be used in the SDP; null terminated */
    char                            application_name[BTIF_HL_APPLICATION_NAME_LEN +1];   /* applicaiton name */

}btif_hl_app_cb_t;
typedef struct
{
    BOOLEAN                         in_use;
    UINT8                           app_idx;
}btif_hl_pending_reg_cb_t;

/* BTIF-HL control block  */
typedef struct
{
    btif_hl_app_cb_t              acb[BTA_HL_NUM_APPS];      /* HL Control Blocks */
    tBTA_HL_CTRL_CBACK            *p_ctrl_cback;             /* pointer to control callback function */
    UINT8                         next_app_id;
    UINT16                        next_channel_id;
    btif_hl_state_t               state;
    btif_hl_nv_cb_t               ncb;
} btif_hl_cb_t;


enum
{
    BTIF_HL_SEND_CONNECTED_CB,
    BTIF_HL_SEND_DISCONNECTED_CB,
    BTIF_HL_REG_APP,
    BTIF_HL_UNREG_APP,
    BTIF_HL_UPDATE_MDL,
};
typedef UINT8 btif_hl_evt_t;

typedef struct
{
    int                     app_id;
    BD_ADDR                 bd_addr;
    int                     mdep_cfg_index;
    int                     channel_id;
    btif_hl_chan_cb_state_t cb_state;
    int                     fd;
}btif_hl_send_chan_state_cb_t;


typedef struct
{
    UINT8 app_idx;
}btif_hl_reg_t;
typedef btif_hl_reg_t btif_hl_unreg_t;
typedef btif_hl_reg_t btif_hl_update_mdl_t;

typedef union
{
    btif_hl_send_chan_state_cb_t    chan_cb;
    btif_hl_reg_t                   reg;
    btif_hl_unreg_t                 unreg;
    btif_hl_update_mdl_t            update_mdl;
}btif_hl_evt_cb_t;



extern btif_hl_cb_t  btif_hl_cb;
extern btif_hl_cb_t *p_btif_hl_cb;
extern btif_hl_nv_cb_t *p_ncb;

#define BTIF_HL_GET_CB_PTR() &(btif_hl_cb)
#define BTIF_HL_GET_APP_CB_PTR(app_idx) &(btif_hl_cb.acb[(app_idx)])
#define BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx) &(btif_hl_cb.acb[(app_idx)].mcb[(mcl_idx)])
#define BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx) &(btif_hl_cb.acb[(app_idx)].mcb[(mcl_idx)].mdl[mdl_idx])
#define BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx) &(btif_hl_cb.acb[app_idx].mcb[mcl_idx].pcb)
#define BTIF_HL_GET_MDL_CFG_PTR(app_idx, item_idx) &(btif_hl_cb.acb[(app_idx)].mdl_cfg[(item_idx)])
#define BTIF_HL_GET_MDL_CFG_CHANNEL_ID_PTR(app_idx, item_idx) &(btif_hl_cb.acb[(app_idx)].mdl_cfg_channel_id[(item_idx)])
extern BOOLEAN btif_hl_find_mcl_idx(UINT8 app_idx, BD_ADDR p_bd_addr, UINT8 *p_mcl_idx);
extern BOOLEAN btif_hl_find_app_idx(UINT8 app_id, UINT8 *p_app_idx);
extern BOOLEAN btif_hl_find_avail_mcl_idx(UINT8 app_idx, UINT8 *p_mcl_idx);
extern BOOLEAN btif_hl_find_avail_mdl_idx(UINT8 app_idx, UINT8 mcl_idx,
                                          UINT8 *p_mdl_idx);
extern BOOLEAN btif_hl_find_mcl_idx_using_handle( tBTA_HL_MCL_HANDLE mcl_handle,
                                                  UINT8 *p_app_idx, UINT8 *p_mcl_idx);
extern BOOLEAN  btif_hl_save_mdl_cfg(UINT8 app_id, UINT8 item_idx, tBTA_HL_MDL_CFG *p_mdl_cfg);
extern BOOLEAN  btif_hl_delete_mdl_cfg(UINT8 app_id, UINT8 item_idx);
extern void * btif_hl_get_buf(UINT16 size);
extern void btif_hl_free_buf(void **p);
extern BOOLEAN btif_hl_find_mdl_idx_using_handle(tBTA_HL_MDL_HANDLE mdl_handle,
                                                 UINT8 *p_app_idx,UINT8 *p_mcl_idx,
                                                 UINT8 *p_mdl_idx);
extern void btif_hl_abort_pending_chan_setup(UINT8 app_idx, UINT8 mcl_idx);
extern BOOLEAN btif_hl_proc_pending_op(UINT8 app_idx, UINT8 mcl_idx);
extern BOOLEAN btif_hl_load_mdl_config (UINT8 app_id, UINT8 buffer_size,
                                        tBTA_HL_MDL_CFG *p_mdl_buf );
#endif
