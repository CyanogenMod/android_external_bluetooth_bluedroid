/********************************************************************************
**                                                                              *
**  Name        uipc_linux.h                                                    *
**                                                                              *
**  Function    Linux specific UIPC definitions                                 *
**                                                                              *
**                                                                              *
**  Copyright (c) 2007-2010, Broadcom Corp., All Rights Reserved.               *
**  Proprietary and confidential.                                               *
**                                                                              *
*********************************************************************************/
#ifndef _UIPC_LINUX_H_
#define _UIPC_LINUX_H_

typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(-1)
#define SOCKET_ERROR            (-1)

/* tcp/ip socket configuration */
typedef struct {
    char            *p_address;
    unsigned int    port;
} tUIPC_LINUX_CFG_TCP ;


#ifdef __cplusplus
extern "C" {
#endif

/* Socket configuration for GLGPS interface */
extern tUIPC_LINUX_CFG_TCP uipc_linux_cfg_glgps;

#ifdef __cplusplus
}
#endif
#endif /* _UIPC_LINUX_H_ */
