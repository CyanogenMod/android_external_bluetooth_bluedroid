/******************************************************************************
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
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hw"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_vendor_brcm.h"
#include "userial.h"
#include "utils.h"
#include "upio.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef BTHW_DBG
#define BTHW_DBG FALSE
#endif

#if (BTHW_DBG == TRUE)
#define BTHWDBG LOGD
#else
#define BTHWDBG
#endif

#define FW_PATCHFILE_EXTENSION      ".hcd"
#define FW_PATCHFILE_EXTENSION_LEN  4
#define FW_PATCHFILE_PATH_MAXLEN    248 /* Local_Name length of return of
                                           HCI_Read_Local_Name */

#define HCI_CMD_MAX_LEN             258

#define HCI_RESET                               0x0C03
#define HCI_VSC_WRITE_UART_CLOCK_SETTING        0xFC45
#define HCI_VSC_UPDATE_BAUDRATE                 0xFC18
#define HCI_READ_LOCAL_NAME                     0x0C14
#define HCI_VSC_DOWNLOAD_MINIDRV                0xFC2E
#define HCI_VSC_WRITE_BD_ADDR                   0xFC01
#define HCI_VSC_WRITE_SLEEP_MODE                0xFC27
#define HCI_VSC_WRITE_SCO_PCM_INT_PARAM         0xFC1C
#define HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM     0xFC1E
#define HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM    0xFC6D
#define HCI_VSC_LAUNCH_RAM                      0xFC4E

#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE        5
#define HCI_EVT_CMD_CMPL_LOCAL_NAME_STRING      6
#define HCI_EVT_CMD_CMPL_OPCODE                 3
#define LPM_CMD_PARAM_SIZE                      12
#define UPDATE_BAUDRATE_CMD_PARAM_SIZE          6
#define HCI_CMD_PREAMBLE_SIZE                   3
#define HCD_REC_PAYLOAD_LEN_BYTE                2
#define BD_ADDR_LEN                             6
#define LOCAL_NAME_BUFFER_LEN                   32
#define LOCAL_BDADDR_PATH_BUFFER_LEN            256

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Hardware Configuration State */
enum {
    HW_CFG_START = 1,
    HW_CFG_SET_UART_CLOCK,
    HW_CFG_SET_UART_BAUD_1,
    HW_CFG_READ_LOCAL_NAME,
    HW_CFG_DL_MINIDRIVER,
    HW_CFG_DL_FW_PATCH,
    HW_CFG_SET_UART_BAUD_2,
    HW_CFG_SET_BD_ADDR
};

/* h/w config control block */
typedef struct
{
    uint8_t state;                          /* Hardware configuration state */
    int     fw_fd;                          /* FW patch file fd */
    uint8_t f_set_baud_2;                   /* Baud rate switch state */
    char    local_chip_name[LOCAL_NAME_BUFFER_LEN];
} bt_hw_cfg_cb_t;

/* low power mode parameters */
typedef struct
{
    uint8_t sleep_mode;                     /* 0(disable),1(UART),9(H5) */
    uint8_t host_stack_idle_threshold;      /* Unit scale 300ms/25ms */
    uint8_t host_controller_idle_threshold; /* Unit scale 300ms/25ms */
    uint8_t bt_wake_polarity;               /* 0=Active Low, 1= Active High */
    uint8_t host_wake_polarity;             /* 0=Active Low, 1= Active High */
    uint8_t allow_host_sleep_during_sco;
    uint8_t combine_sleep_mode_and_lpm;
    uint8_t enable_uart_txd_tri_state;      /* UART_TXD Tri-State */
    uint8_t sleep_guard_time;               /* sleep guard time in 12.5ms */
    uint8_t wakeup_guard_time;              /* wakeup guard time in 12.5ms */
    uint8_t txd_config;                     /* TXD is high in sleep state */
    uint8_t pulsed_host_wake;               /* pulsed host wake if mode = 1 */
} bt_lpm_param_t;

/* Low power mode state */
enum {
    HW_LPM_DISABLED = 0,                    /* initial state */
    HW_LPM_ENABLED,
    HW_LPM_ENABLING,
    HW_LPM_DISABLING
};

/* BT_WAKE state */
enum {
    LPM_BTWAKE_DEASSERTED = 0,              /* initial state */
    LPM_BTWAKE_W4_TX_DONE,
    LPM_BTWAKE_W4_TIMEOUT,
    LPM_BTWAKE_ASSERTED
};

/* low power mode control block */
typedef struct
{
    uint8_t state;                          /* Low power mode state */
    uint8_t btwake_state;                   /* BT_WAKE state */
    uint8_t no_tx_data;
    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;                    /* 10 times of the chip unit */
} bt_lpm_cb_t;

/******************************************************************************
**  Externs
******************************************************************************/

/* Callback function for the returned event of internal issued command */
typedef void (*tINT_CMD_CBACK)(VND_BT_HDR *p_buf);
void hci_h4_get_acl_data_length(void);
void hw_config_cback(VND_BT_HDR *p_evt_buf);
uint8_t hci_h4_send_int_cmd(uint16_t opcode, VND_BT_HDR *p_buf, \
                                  tINT_CMD_CBACK p_cback);


/******************************************************************************
**  Static variables
******************************************************************************/

static uint8_t local_bd_addr[BD_ADDR_LEN]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t null_bdaddr[BD_ADDR_LEN] = {0,0,0,0,0,0};

static bt_hw_cfg_cb_t hw_cfg_cb;
static bt_lpm_cb_t hw_lpm_cb;

static bt_lpm_param_t lpm_param =
{
    LPM_SLEEP_MODE,
    LPM_IDLE_THRESHOLD,
    LPM_HC_IDLE_THRESHOLD,
    LPM_BT_WAKE_POLARITY,
    LPM_HOST_WAKE_POLARITY,
    LPM_ALLOW_HOST_SLEEP_DURING_SCO,
    LPM_COMBINE_SLEEP_MODE_AND_LPM,
    LPM_ENABLE_UART_TXD_TRI_STATE,
    0,  /* not applicable */
    0,  /* not applicable */
    0,  /* not applicable */
    LPM_PULSED_HOST_WAKE
};

#if (SCO_USE_I2S_INTERFACE == FALSE)
static uint8_t bt_sco_param[SCO_PCM_PARAM_SIZE] =
{
    SCO_PCM_ROUTING,
    SCO_PCM_IF_CLOCK_RATE,
    SCO_PCM_IF_FRAME_TYPE,
    SCO_PCM_IF_SYNC_MODE,
    SCO_PCM_IF_CLOCK_MODE
};

static uint8_t bt_pcm_data_fmt_param[PCM_DATA_FORMAT_PARAM_SIZE] =
{
    PCM_DATA_FMT_SHIFT_MODE,
    PCM_DATA_FMT_FILL_BITS,
    PCM_DATA_FMT_FILL_METHOD,
    PCM_DATA_FMT_FILL_NUM,
    PCM_DATA_FMT_JUSTIFY_MODE
};
#else
static uint8_t bt_sco_param[SCO_I2SPCM_PARAM_SIZE] =
{
    SCO_I2SPCM_IF_MODE,
    SCO_I2SPCM_IF_ROLE,
    SCO_I2SPCM_IF_SAMPLE_RATE,
    SCO_I2SPCM_IF_CLOCK_RATE
};
#endif

/******************************************************************************
**  Static functions
******************************************************************************/

/******************************************************************************
**  Controller Initialization Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_strncmp
**
** Description      Used to compare two strings in caseless
**
** Returns          0: match, otherwise: not match
**
*******************************************************************************/
static int hw_strncmp (const char *p_str1, const char *p_str2, const int len)
{
    int i;

    if (!p_str1 || !p_str2)
        return (1);

    for (i = 0; i < len; i++)
    {
        if (toupper(p_str1[i]) != toupper(p_str2[i]))
            return (i+1);
    }

    return 0;
}

/*******************************************************************************
**
** Function         hw_config_findpatch
**
** Description      Search for a proper firmware patch file
**                  The selected firmware patch file name with full path
**                  will be stored in the input string parameter, i.e.
**                  p_chip_id_str, when returns.
**
** Returns          TRUE when found the target patch file, otherwise FALSE
**
*******************************************************************************/
static uint8_t hw_config_findpatch(char *p_chip_id_str)
{
    DIR *dirp;
    struct dirent *dp;
    int filenamelen;
    uint8_t retval = FALSE;

    BTHWDBG("Target name = [%s]", p_chip_id_str);

    if ((dirp = opendir(FW_PATCHFILE_LOCATION)) != NULL)
    {
        /* Fetch next filename in patchfile directory */
        while ((dp = readdir(dirp)) != NULL)
        {
            /* Check if filename starts with chip-id name */
            if ((hw_strncmp(dp->d_name, p_chip_id_str, strlen(p_chip_id_str)) \
                ) == 0)
            {
                /* Check if it has .hcd extenstion */
                filenamelen = strlen(dp->d_name);
                if ((filenamelen >= FW_PATCHFILE_EXTENSION_LEN) &&
                    ((hw_strncmp(
                          &dp->d_name[filenamelen-FW_PATCHFILE_EXTENSION_LEN], \
                          FW_PATCHFILE_EXTENSION, \
                          FW_PATCHFILE_EXTENSION_LEN) \
                     ) == 0))
                {
                    LOGI("Found patchfile: %s/%s", \
                        FW_PATCHFILE_LOCATION, dp->d_name);

                    /* Make sure length does not exceed maximum */
                    if ((filenamelen + strlen(FW_PATCHFILE_LOCATION)) > \
                         FW_PATCHFILE_PATH_MAXLEN)
                    {
                        LOGE("Invalid patchfile name (too long)");
                    }
                    else
                    {
                        memset(p_chip_id_str, 0, FW_PATCHFILE_PATH_MAXLEN);
                        /* Found patchfile. Store location and name */
                        strcpy(p_chip_id_str, FW_PATCHFILE_LOCATION);
                        if (FW_PATCHFILE_LOCATION[ \
                            strlen(FW_PATCHFILE_LOCATION)- 1 \
                            ] != '/')
                        {
                            strcat(p_chip_id_str, "/");
                        }
                        strcat(p_chip_id_str, dp->d_name);
                        retval = TRUE;
                    }
                    break;
                }
            }
        }

        closedir(dirp);

        if (retval == FALSE)
        {
            /* Try again chip name without revision info */

            int len = strlen(p_chip_id_str);
            char *p = p_chip_id_str + len - 1;

            /* Scan backward and look for the first alphabet
               which is not M or m
            */
            while (len > 3) // BCM****
            {
                if ((isdigit(*p)==0) && (*p != 'M') && (*p != 'm'))
                    break;

                p--;
                len--;
            }

            if (len > 3)
            {
                *p = 0;
                retval = hw_config_findpatch(p_chip_id_str);
            }
        }
    }
    else
    {
        LOGE("Could not open %s", FW_PATCHFILE_LOCATION);
    }

    return (retval);
}

/*******************************************************************************
**
** Function         hw_ascii_2_bdaddr
**
** Description      This function converts ASCII string of Bluetooth device
**                  address (xx:xx:xx:xx:xx:xx) into a BD_ADDR type
**
** Returns          None
**
*******************************************************************************/
void hw_ascii_2_bdaddr (char *p_ascii, uint8_t *p_bd)
{
    int     x;
    uint8_t c;

    for (x = 0; x < BD_ADDR_LEN; x++)
    {
        if ((*p_ascii>='0') && (*p_ascii<='9'))
            c = (*p_ascii - '0') << 4;
        else
            c = (toupper(*p_ascii) - 'A' + 10) << 4;

        p_ascii++;
        if ((*p_ascii>='0') && (*p_ascii<='9'))
            c |= (*p_ascii - '0');
        else
            c |= (toupper(*p_ascii) - 'A' + 10);

        p_ascii+=2;  // skip ':' and point to next valid digit
        *p_bd++ = c;
    }
}

/*******************************************************************************
**
** Function         hw_bdaddr_2_ascii
**
** Description      This function converts a BD_ADDR type address into ASCII
**                  string of Bluetooth device address (xx:xx:xx:xx:xx:xx)
**
** Returns          None
**
*******************************************************************************/
void hw_bdaddr_2_ascii (uint8_t *p_bd, char *p_ascii)
{
    int     x;
    uint8_t c;

    for (x = 0; x < BD_ADDR_LEN; x++)
    {
        c = (*p_bd >> 4) & 0x0f;
        if (c < 10)
            *p_ascii++ = c + '0';
        else
            *p_ascii++ = c - 10 + 'A';

        c = *p_bd & 0x0f;
        if (c < 10)
            *p_ascii++ = c + '0';
        else
            *p_ascii++ = c - 10 + 'A';

        p_bd++;
        *p_ascii++ = ':';
    }
    *--p_ascii = '\0';
}


/*******************************************************************************
**
** Function         hw_config_get_bdaddr
**
** Description      Get Bluetooth Device Address
**
** Returns          None
**
*******************************************************************************/
void hw_config_get_bdaddr(uint8_t *local_addr)
{
    char val[LOCAL_BDADDR_PATH_BUFFER_LEN];
    uint8_t bHaveValidBda = FALSE;

    BTHWDBG("Look for bdaddr storage path in prop %s", PROPERTY_BT_BDADDR_PATH);

    /* Get local bdaddr storage path from property */
    if (property_get(PROPERTY_BT_BDADDR_PATH, val, NULL))
    {
        int addr_fd;

        BTHWDBG("local bdaddr is stored in %s", val);

        if ((addr_fd = open(val, O_RDONLY)) != -1)
        {
            memset(val, 0, LOCAL_BDADDR_PATH_BUFFER_LEN);
            read(addr_fd, val, FACTORY_BT_BDADDR_STORAGE_LEN);
            hw_ascii_2_bdaddr(val, local_addr);

            /* If this is not a reserved/special bda, then use it */
            if (memcmp(local_addr, null_bdaddr, BD_ADDR_LEN) != 0)
            {
                bHaveValidBda = TRUE;
                LOGI("Got Factory BDA %02X:%02X:%02X:%02X:%02X:%02X",
                    local_addr[0], local_addr[1], local_addr[2],
                    local_addr[3], local_addr[4], local_addr[5]);
            }

            close(addr_fd);
        }
    }

    /* No factory BDADDR found. Look for previously generated random BDA */
    if ((!bHaveValidBda) && \
        (property_get(PERSIST_BDADDR_PROPERTY, val, NULL)))
    {
        hw_ascii_2_bdaddr(val, local_addr);
        bHaveValidBda = TRUE;
        LOGI("Got prior random BDA %02X:%02X:%02X:%02X:%02X:%02X",
            local_addr[0], local_addr[1], local_addr[2],
            local_addr[3], local_addr[4], local_addr[5]);
    }

    /* Generate new BDA if necessary */
    if (!bHaveValidBda)
    {
        /* Seed the random number generator */
        srand((unsigned int) (time(0)));

        /* No autogen BDA. Generate one now. */
        local_addr[0] = 0x22;
        local_addr[1] = 0x22;
        local_addr[2] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr[3] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr[4] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr[5] = (uint8_t) ((rand() >> 8) & 0xFF);

        /* Convert to ascii, and store as a persistent property */
        hw_bdaddr_2_ascii(local_addr, val);

        LOGI("No preset BDA. Generating BDA: %s for prop %s",
             val, PERSIST_BDADDR_PROPERTY);

        if (property_set(PERSIST_BDADDR_PROPERTY, val) < 0)
            LOGE("Failed to set random BDA in prop %s",PERSIST_BDADDR_PROPERTY);
    }
} /* btl_cfg_get_bdaddr() */


/*******************************************************************************
**
** Function         hw_config_set_bdaddr
**
** Description      Program controller's Bluetooth Device Address
**
** Returns          TRUE, if valid address is sent
**                  FALSE, otherwise
**
*******************************************************************************/
static uint8_t hw_config_set_bdaddr(VND_BT_HDR *p_buf)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    /* See if bdaddr needs to be programmed */
    if (memcmp(local_bd_addr, null_bdaddr, BD_ADDR_LEN) != 0)
    {
        LOGI("Setting local bd addr to %02X:%02X:%02X:%02X:%02X:%02X",
            local_bd_addr[0], local_bd_addr[1], local_bd_addr[2],
            local_bd_addr[3], local_bd_addr[4], local_bd_addr[5]);

        UINT16_TO_STREAM(p, HCI_VSC_WRITE_BD_ADDR);
        *p++ = BD_ADDR_LEN; /* parameter length */
        *p++ = local_bd_addr[5];
        *p++ = local_bd_addr[4];
        *p++ = local_bd_addr[3];
        *p++ = local_bd_addr[2];
        *p++ = local_bd_addr[1];
        *p = local_bd_addr[0];

        p_buf->len = HCI_CMD_PREAMBLE_SIZE + BD_ADDR_LEN;
        hw_cfg_cb.state = HW_CFG_SET_BD_ADDR;

        retval = hci_h4_send_int_cmd(HCI_VSC_WRITE_BD_ADDR, p_buf, \
                                     hw_config_cback);
    }

    return (retval);
}

/*******************************************************************************
**
** Function         hw_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_config_cback(VND_BT_HDR *p_evt_buf)
{
    char        *p_name;
    uint8_t     *p, status;
    uint16_t    opcode;
    VND_BT_HDR  *p_buf=NULL;
    uint8_t     is_proceeding = FALSE;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode,p);

    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    if ((status == 0) && bt_vendor_cbacks)
        p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BT_VND_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);

    if (p_buf != NULL)
    {
        p_buf->event = MSG_STACK_TO_VND_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        p = (uint8_t *) (p_buf + 1);

        switch (hw_cfg_cb.state)
        {
            case HW_CFG_SET_UART_BAUD_1:
                /* update baud rate of host's UART port */
                userial_change_baud(USERIAL_BAUD_3M);

                /* read local name */
                UINT16_TO_STREAM(p, HCI_READ_LOCAL_NAME);
                *p = 0; /* parameter length */

                p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                hw_cfg_cb.state = HW_CFG_READ_LOCAL_NAME;

                is_proceeding = hci_h4_send_int_cmd(HCI_READ_LOCAL_NAME, p_buf,\
                                                    hw_config_cback);
                break;

            case HW_CFG_READ_LOCAL_NAME:
                p_name = (char *) (p_evt_buf + 1) + \
                         HCI_EVT_CMD_CMPL_LOCAL_NAME_STRING;
                strncpy(hw_cfg_cb.local_chip_name, p_name, \
                        LOCAL_NAME_BUFFER_LEN-1);
                hw_cfg_cb.local_chip_name[LOCAL_NAME_BUFFER_LEN-1] = 0;
                if ((status = hw_config_findpatch(p_name)) == TRUE)
                {
                    if ((hw_cfg_cb.fw_fd = open(p_name, O_RDONLY)) == -1)
                    {
                        LOGE("vendor lib preload failed to open [%s]", p_name);
                    }
                    else
                    {
                        /* vsc_download_minidriver */
                        UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_MINIDRV);
                        *p = 0; /* parameter length */

                        p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                        hw_cfg_cb.state = HW_CFG_DL_MINIDRIVER;

                        is_proceeding = hci_h4_send_int_cmd( \
                                            HCI_VSC_DOWNLOAD_MINIDRV, p_buf, \
                                            hw_config_cback);
                    }
                }
                else
                {
                    LOGE( \
                    "vendor lib preload failed to locate firmware patch file" \
                    );
                }

                if (is_proceeding == FALSE)
                {
                    is_proceeding = hw_config_set_bdaddr(p_buf);
                }
                break;

            case HW_CFG_DL_MINIDRIVER:
                /* give time for placing firmware in download mode */
                utils_delay(50);
                hw_cfg_cb.state = HW_CFG_DL_FW_PATCH;
                /* fall through intentionally */
            case HW_CFG_DL_FW_PATCH:
                p_buf->len = read(hw_cfg_cb.fw_fd, p, HCI_CMD_PREAMBLE_SIZE);
                if (p_buf->len > 0)
                {
                    if ((p_buf->len < HCI_CMD_PREAMBLE_SIZE) || \
                        (opcode == HCI_VSC_LAUNCH_RAM))
                    {
                        LOGW("firmware patch file might be altered!");
                    }
                    else
                    {
                        p_buf->len += read(hw_cfg_cb.fw_fd, \
                                           p+HCI_CMD_PREAMBLE_SIZE,\
                                           *(p+HCD_REC_PAYLOAD_LEN_BYTE));
                        STREAM_TO_UINT16(opcode,p);
                        is_proceeding = hci_h4_send_int_cmd(opcode, p_buf, \
                                                        hw_config_cback);
                        break;
                    }
                }

                close(hw_cfg_cb.fw_fd);
                hw_cfg_cb.fw_fd = -1;

                /* Normally the firmware patch configuration file
                 * sets the new starting baud rate at 115200.
                 * So, we need update host's baud rate accordingly.
                 */
                userial_change_baud(USERIAL_BAUD_115200);

                /* Next, we would like to boost baud rate up again
                 * to desired working speed.
                 */
                hw_cfg_cb.f_set_baud_2 = TRUE;

                /* fall through intentionally */
            case HW_CFG_START:
                if (UART_TARGET_BAUD_RATE > 3000000)
                {
                    /* set UART clock to 48MHz */
                    UINT16_TO_STREAM(p, HCI_VSC_WRITE_UART_CLOCK_SETTING);
                    *p++ = 1; /* parameter length */
                    *p = 1; /* (1,"UART CLOCK 48 MHz")(2,"UART CLOCK 24 MHz") */

                    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1;
                    hw_cfg_cb.state = HW_CFG_SET_UART_CLOCK;

                    is_proceeding = hci_h4_send_int_cmd( \
                                        HCI_VSC_WRITE_UART_CLOCK_SETTING, \
                                        p_buf, hw_config_cback);
                    break;
                }
                /* fall through intentionally */
            case HW_CFG_SET_UART_CLOCK:
                /* set controller's UART baud rate to 3M */
                UINT16_TO_STREAM(p, HCI_VSC_UPDATE_BAUDRATE);
                *p++ = UPDATE_BAUDRATE_CMD_PARAM_SIZE; /* parameter length */
                *p++ = 0; /* encoded baud rate */
                *p++ = 0; /* use encoded form */
                UINT32_TO_STREAM(p, UART_TARGET_BAUD_RATE);

                p_buf->len = HCI_CMD_PREAMBLE_SIZE + \
                             UPDATE_BAUDRATE_CMD_PARAM_SIZE;
                hw_cfg_cb.state = (hw_cfg_cb.f_set_baud_2) ? \
                            HW_CFG_SET_UART_BAUD_2 : HW_CFG_SET_UART_BAUD_1;

                is_proceeding = hci_h4_send_int_cmd(HCI_VSC_UPDATE_BAUDRATE, \
                                                    p_buf, hw_config_cback);
                break;

            case HW_CFG_SET_UART_BAUD_2:
                /* update baud rate of host's UART port */
                userial_change_baud(USERIAL_BAUD_3M);

                if ((is_proceeding = hw_config_set_bdaddr(p_buf)) == TRUE)
                    break;

                /* fall through intentionally */
            case HW_CFG_SET_BD_ADDR:
                LOGI("vendor lib preload completed");
                if (bt_vendor_cbacks)
                {
                    bt_vendor_cbacks->dealloc((TRANSAC) p_buf, \
                                              (char *) (p_buf + 1));

                    bt_vendor_cbacks->preload_cb(NULL, \
                                                 BT_VENDOR_PRELOAD_SUCCESS);
                }

                hw_cfg_cb.state = 0;

                if (hw_cfg_cb.fw_fd != -1)
                {
                    close(hw_cfg_cb.fw_fd);
                    hw_cfg_cb.fw_fd = -1;
                }

                is_proceeding = TRUE;
                break;
        } // switch(hw_cfg_cb.state)
    } // if (p_buf != NULL)

    /* Free the RX event buffer */
    if (bt_vendor_cbacks)
        bt_vendor_cbacks->dealloc((TRANSAC) p_evt_buf, (char *) (p_evt_buf+1));

    if (is_proceeding == FALSE)
    {
        LOGE("vendor lib preload aborted!!!");
        if (bt_vendor_cbacks)
        {
            if (p_buf != NULL)
                bt_vendor_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf+1));

            bt_vendor_cbacks->preload_cb(NULL, BT_VENDOR_PRELOAD_FAIL);
        }

        if (hw_cfg_cb.fw_fd != -1)
        {
            close(hw_cfg_cb.fw_fd);
            hw_cfg_cb.fw_fd = -1;
        }

        hw_cfg_cb.state = 0;
    }
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function        hw_lpm_idle_timeout
**
** Description     Timeout thread of transport idle timer
**
** Returns         None
**
*******************************************************************************/
static void hw_lpm_idle_timeout(union sigval arg)
{
    BTHWDBG("..hw_lpm_idle_timeout..");

    if ((hw_lpm_cb.state == HW_LPM_ENABLED) && \
        (hw_lpm_cb.btwake_state == LPM_BTWAKE_W4_TIMEOUT))
    {
        btvnd_signal_event(VND_EVENT_LPM_IDLE_TIMEOUT);
    }
}

/*******************************************************************************
**
** Function        hw_lpm_start_transport_idle_timer
**
** Description     Launch transport idle timer
**
** Returns         None
**
*******************************************************************************/
static void hw_lpm_start_transport_idle_timer(void)
{
    int status;
    struct itimerspec ts;
    struct sigevent se;

    if (hw_lpm_cb.state != HW_LPM_ENABLED)
        return;

    if (hw_lpm_cb.timer_created == FALSE)
    {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &hw_lpm_cb.timer_id;
        se.sigev_notify_function = hw_lpm_idle_timeout;
        se.sigev_notify_attributes = NULL;

        /* set idle time to be LPM_IDLE_TIMEOUT_MULTIPLE times of
         * host stack idle threshold (in 300ms/25ms)
         */
        hw_lpm_cb.timeout_ms = (uint32_t)lpm_param.host_stack_idle_threshold \
                                * LPM_IDLE_TIMEOUT_MULTIPLE;

        if (strstr(hw_cfg_cb.local_chip_name, "BCM4325") != NULL)
            hw_lpm_cb.timeout_ms *= 25; // 12.5 or 25 ?
        else
            hw_lpm_cb.timeout_ms *= 300;

        status = timer_create(CLOCK_MONOTONIC, &se, &hw_lpm_cb.timer_id);

        if (status == 0)
            hw_lpm_cb.timer_created = TRUE;
    }

    if (hw_lpm_cb.timer_created == TRUE)
    {
        ts.it_value.tv_sec = hw_lpm_cb.timeout_ms/1000;
        ts.it_value.tv_nsec = 1000*(hw_lpm_cb.timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(hw_lpm_cb.timer_id, 0, &ts, 0);
        if (status == -1)
            LOGE("[START] Failed to set LPM idle timeout");
    }
}

/*******************************************************************************
**
** Function        hw_lpm_stop_transport_idle_timer
**
** Description     Launch transport idle timer
**
** Returns         None
**
*******************************************************************************/
static void hw_lpm_stop_transport_idle_timer(void)
{
    int status;
    struct itimerspec ts;

    if (hw_lpm_cb.timer_created == TRUE)
    {
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(hw_lpm_cb.timer_id, 0, &ts, 0);
        if (status == -1)
            LOGE("[STOP] Failed to set LPM idle timeout");
    }
}

/*******************************************************************************
**
** Function         hw_lpm_ctrl_cback
**
** Description      Callback function for lpm enable/disable rquest
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_ctrl_cback(VND_BT_HDR *p_evt_buf)
{
    uint8_t     next_state;

    if (*((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0)
    {
        /* Status == Success */
        hw_lpm_cb.state = (hw_lpm_cb.state == HW_LPM_ENABLING) ? \
                          HW_LPM_ENABLED : HW_LPM_DISABLED;
    }
    else
    {
        hw_lpm_cb.state = (hw_lpm_cb.state == HW_LPM_ENABLING) ? \
                          HW_LPM_DISABLED : HW_LPM_ENABLED;
    }

    if (bt_vendor_cbacks)
    {
        if (hw_lpm_cb.state == HW_LPM_ENABLED)
            bt_vendor_cbacks->lpm_cb(BT_VENDOR_LPM_ENABLED);
        else
            bt_vendor_cbacks->lpm_cb(BT_VENDOR_LPM_DISABLED);
    }

    if (hw_lpm_cb.state == HW_LPM_DISABLED)
    {
        if (hw_lpm_cb.timer_created == TRUE)
        {
            timer_delete(hw_lpm_cb.timer_id);
        }

        memset(&hw_lpm_cb, 0, sizeof(bt_lpm_cb_t));
    }

    if (bt_vendor_cbacks)
    {
        bt_vendor_cbacks->dealloc((TRANSAC) p_evt_buf, (char *) (p_evt_buf+1));
    }
}


#if (SCO_CFG_INCLUDED == TRUE)
/*****************************************************************************
**   SCO Configuration Static Functions
*****************************************************************************/

/*******************************************************************************
**
** Function         hw_sco_cfg_cback
**
** Description      Callback function for SCO configuration rquest
**
** Returns          None
**
*******************************************************************************/
void hw_sco_cfg_cback(VND_BT_HDR *p_evt_buf)
{
    uint8_t     *p;
    uint16_t    opcode;
    VND_BT_HDR  *p_buf=NULL;

    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode,p);

    /* Free the RX event buffer */
    if (bt_vendor_cbacks)
        bt_vendor_cbacks->dealloc((TRANSAC) p_evt_buf, (char *) (p_evt_buf+1));

#if (SCO_USE_I2S_INTERFACE == FALSE)
    if (opcode == HCI_VSC_WRITE_SCO_PCM_INT_PARAM)
    {
        uint8_t ret = FALSE;

        /* Ask a new buffer to hold WRITE_PCM_DATA_FORMAT_PARAM command */
        if (bt_vendor_cbacks)
            p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BT_VND_HDR_SIZE + \
                                                HCI_CMD_PREAMBLE_SIZE + \
                                                PCM_DATA_FORMAT_PARAM_SIZE);
        if (p_buf)
        {
            p_buf->event = MSG_STACK_TO_VND_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE + PCM_DATA_FORMAT_PARAM_SIZE;

            p = (uint8_t *) (p_buf + 1);
            UINT16_TO_STREAM(p, HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM);
            *p++ = PCM_DATA_FORMAT_PARAM_SIZE;
            memcpy(p, &bt_pcm_data_fmt_param, PCM_DATA_FORMAT_PARAM_SIZE);

            if ((ret = hci_h4_send_int_cmd(HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM,\
                                           p_buf, hw_sco_cfg_cback)) == FALSE)
            {
                bt_vendor_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
            }
            else
                return;
        }
    }
#endif  // !SCO_USE_I2S_INTERFACE

    hci_h4_get_acl_data_length();
}
#endif // SCO_CFG_INCLUDED

/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/


/*******************************************************************************
**
** Function        hw_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_config_start(void)
{
    VND_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    hw_cfg_cb.state = 0;
    hw_cfg_cb.fw_fd = -1;
    hw_cfg_cb.f_set_baud_2 = FALSE;
    hw_lpm_cb.state = HW_LPM_DISABLED;

    /****************************************
     * !!! TODO !!!
     *
     * === Custom Porting Required ===
     *
     * Unique Bluetooth address should be
     * assigned to local_bd_addr[6] in
     * production line for each device.
     ****************************************/
    hw_config_get_bdaddr(local_bd_addr);

    /* Start from sending HCI_RESET */

    if (bt_vendor_cbacks)
    {
        p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BT_VND_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_VND_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_RESET);
        *p = 0; /* parameter length */

        hw_cfg_cb.state = HW_CFG_START;

        hci_h4_send_int_cmd(HCI_RESET, p_buf, hw_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            LOGE("vendor lib preload aborted [no buffer]");
            bt_vendor_cbacks->preload_cb(NULL, BT_VENDOR_PRELOAD_FAIL);
        }
    }
}

/*******************************************************************************
**
** Function        hw_lpm_enable
**
** Description     Enalbe/Disable LPM
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t hw_lpm_enable(uint8_t turn_on)
{
    VND_BT_HDR  *p_buf = NULL;
    uint8_t     *p;
    uint8_t     ret = FALSE;

    if ((hw_lpm_cb.state!=HW_LPM_DISABLED) && (hw_lpm_cb.state!=HW_LPM_ENABLED))
    {
        LOGW("Still busy on processing prior LPM enable/disable request...");
        return FALSE;
    }

    if ((turn_on == TRUE) && (hw_lpm_cb.state == HW_LPM_ENABLED))
    {
        LOGI("LPM is already on!!!");
        if (bt_vendor_cbacks)
            bt_vendor_cbacks->lpm_cb(BT_VENDOR_LPM_ENABLED);
        return TRUE;
    }
    else if ((turn_on == FALSE) && (hw_lpm_cb.state == HW_LPM_DISABLED))
    {
        LOGI("LPM is already off!!!");
        if (bt_vendor_cbacks)
            bt_vendor_cbacks->lpm_cb(BT_VENDOR_LPM_DISABLED);
        return TRUE;
    }

    if (bt_vendor_cbacks)
        p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BT_VND_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE + \
                                                       LPM_CMD_PARAM_SIZE);

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_VND_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE + LPM_CMD_PARAM_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_VSC_WRITE_SLEEP_MODE);
        *p++ = LPM_CMD_PARAM_SIZE; /* parameter length */

        if (turn_on)
        {
            memcpy(p, &lpm_param, LPM_CMD_PARAM_SIZE);
            hw_lpm_cb.state = HW_LPM_ENABLING;
        }
        else
        {
            memset(p, 0, LPM_CMD_PARAM_SIZE);
            hw_lpm_cb.state = HW_LPM_DISABLING;
        }

        if ((ret = hci_h4_send_int_cmd(HCI_VSC_WRITE_SLEEP_MODE, p_buf, \
                                        hw_lpm_ctrl_cback)) == FALSE)
        {
            hw_lpm_cb.state = (hw_lpm_cb.state == HW_LPM_ENABLING) ? \
                               HW_LPM_DISABLED : HW_LPM_ENABLED;

            if (bt_vendor_cbacks)
                bt_vendor_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf+1));
        }
    }

    return ret;
}

/*******************************************************************************
**
** Function          hw_lpm_tx_done
**
** Description       This function is to inform the lpm module
**                   if data is waiting in the Tx Q or not.
**
**                   IsTxDone: TRUE if All data in the Tx Q are gone
**                             FALSE if any data is still in the Tx Q.
**                   Typicaly this function must be called
**                   before USERIAL Write and in the Tx Done routine
**
** Returns           None
**
*******************************************************************************/
void hw_lpm_tx_done(uint8_t is_tx_done)
{
    hw_lpm_cb.no_tx_data = is_tx_done;

    if ((hw_lpm_cb.btwake_state==LPM_BTWAKE_W4_TX_DONE) && (is_tx_done==TRUE))
    {
        hw_lpm_cb.btwake_state = LPM_BTWAKE_W4_TIMEOUT;
        hw_lpm_start_transport_idle_timer();
    }
}

/*******************************************************************************
**
** Function        hw_lpm_assert_bt_wake
**
** Description     Called to wake up Bluetooth chip.
**                 Normally this is called when there is data to be sent
**                 over UART.
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
void hw_lpm_assert_bt_wake(void)
{
    if (hw_lpm_cb.state != HW_LPM_DISABLED)
    {
        BTHWDBG("LPM assert BT_WAKE");
        upio_set(UPIO_BT_WAKE, UPIO_ASSERT, lpm_param.bt_wake_polarity);

        hw_lpm_stop_transport_idle_timer();

        hw_lpm_cb.btwake_state = LPM_BTWAKE_ASSERTED;
    }

    hw_lpm_tx_done(FALSE);
}

/*******************************************************************************
**
** Function        hw_lpm_allow_bt_device_sleep
**
** Description     Start LPM idle timer if allowed
**
** Returns         None
**
*******************************************************************************/
void hw_lpm_allow_bt_device_sleep(void)
{
    if ((hw_lpm_cb.state == HW_LPM_ENABLED) && \
        (hw_lpm_cb.btwake_state == LPM_BTWAKE_ASSERTED))
    {
        if(hw_lpm_cb.no_tx_data == TRUE)
        {
            hw_lpm_cb.btwake_state = LPM_BTWAKE_W4_TIMEOUT;
            hw_lpm_start_transport_idle_timer();
        }
        else
        {
            hw_lpm_cb.btwake_state = LPM_BTWAKE_W4_TX_DONE;
        }
    }
}

/*******************************************************************************
**
** Function         hw_lpm_deassert_bt_wake
**
** Description      Deassert bt_wake if allowed
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_deassert_bt_wake(void)
{
    if ((hw_lpm_cb.state == HW_LPM_ENABLED) && (hw_lpm_cb.no_tx_data == TRUE))
    {
        BTHWDBG("LPM deassert BT_WAKE");
        upio_set(UPIO_BT_WAKE, UPIO_DEASSERT, lpm_param.bt_wake_polarity);
        hw_lpm_cb.btwake_state = LPM_BTWAKE_DEASSERTED;
    }
}

#if (SCO_CFG_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         hw_sco_config
**
** Description      Configure SCO related hardware settings
**
** Returns          None
**
*******************************************************************************/
void hw_sco_config(void)
{
    VND_BT_HDR  *p_buf = NULL;
    uint8_t     *p, ret;

#if (SCO_USE_I2S_INTERFACE == FALSE)
    uint16_t cmd_u16 = HCI_CMD_PREAMBLE_SIZE + SCO_PCM_PARAM_SIZE;
#else
    uint16_t cmd_u16 = HCI_CMD_PREAMBLE_SIZE + SCO_I2SPCM_PARAM_SIZE;
#endif

    if (bt_vendor_cbacks)
        p_buf = (VND_BT_HDR *) bt_vendor_cbacks->alloc(BT_VND_HDR_SIZE+cmd_u16);

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_VND_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = cmd_u16;

        p = (uint8_t *) (p_buf + 1);
#if (SCO_USE_I2S_INTERFACE == FALSE)
        UINT16_TO_STREAM(p, HCI_VSC_WRITE_SCO_PCM_INT_PARAM);
        *p++ = SCO_PCM_PARAM_SIZE;
        memcpy(p, &bt_sco_param, SCO_PCM_PARAM_SIZE);
        cmd_u16 = HCI_VSC_WRITE_SCO_PCM_INT_PARAM;
        LOGI("SCO PCM configure {%d, %d, %d, %d, %d}",
           bt_sco_param[0], bt_sco_param[1], bt_sco_param[2], bt_sco_param[3], \
           bt_sco_param[4]);

#else
        UINT16_TO_STREAM(p, HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM);
        *p++ = SCO_I2SPCM_PARAM_SIZE;
        memcpy(p, &bt_sco_param, SCO_I2SPCM_PARAM_SIZE);
        cmd_u16 = HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM;
        LOGI("SCO over I2SPCM interface {%d, %d, %d, %d}",
           bt_sco_param[0], bt_sco_param[1], bt_sco_param[2], bt_sco_param[3]);
#endif

        if ((ret=hci_h4_send_int_cmd(cmd_u16, p_buf, hw_sco_cfg_cback))==FALSE)
        {
            bt_vendor_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
        }
        else
            return;
    }

    if (bt_vendor_cbacks)
    {
        LOGE("vendor lib postload aborted");
        bt_vendor_cbacks->postload_cb(NULL, BT_VENDOR_POSTLOAD_FAIL);
    }
}
#endif  // SCO_CFG_INCLUDED

