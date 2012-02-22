#ifndef VND_BUILDCFG_H
#define VND_BUILDCFG_H
//#define BLUETOOTH_UART_DEVICE_PORT      "/dev/ttyHS0"       /* pyramid */
//#define FW_PATCHFILE_LOCATION           ("/etc/firmware/")  /* pyramid */
//#define LPM_BT_WAKE_POLARITY            0                   /* pyramid */
//#define LPM_HOST_WAKE_POLARITY          0                   /* pyramid */
#define BT_WAKE_VIA_USERIAL_IOCTL       TRUE
#define LPM_IDLE_TIMEOUT_MULTIPLE       5
#define BTSNOOPDISP_INCLUDED            TRUE
#define BTSNOOP_FILENAME                "/data/misc/bluedroid/btsnoop_hci.cfa" 
#define SNOOP_CONFIG_PATH               "/data/misc/bluedroid/btsnoop_enabled"
#define BTSNOOP_DBG                     FALSE
#define SCO_USE_I2S_INTERFACE           TRUE
#endif // VND_BUILDCFG_H
