/*****************************************************************************
**
**  Name:           bta_prm_api.h
**
**  Description:    This is the public interface file for the patch ram
**                  subsystem of BTA, Widcomm's
**                  Bluetooth application layer for mobile phones.
**
**  Copyright (c) 2003-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_PRM_API_H
#define BTA_PRM_API_H

#include "bta_api.h"
#include "bta_sys.h"

/* callback function */
typedef void (tBTA_PRM_CBACK) (tBTA_STATUS status);

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         BTA_PatchRam
**
** Description      Patch Ram function
**
** Parameters       p_cback - callback to indicate status of download operation.
**                  p_name  - path and file name of the patch file. 
**                          - if p_name is NULL, try to use patch data built into code.
**                  fs_app_id - app_id used by bta file system callout functions  
**                            - to distinguish between applications which uses FS.
**                  address - address of patch ram
**                  
**
** Returns          void
**
**
*******************************************************************************/
BTA_API extern void BTA_PatchRam(tBTA_PRM_CBACK *p_cback, const char *p_name, 
                                 UINT8 fs_app_id, UINT32 address);
                                  
#ifdef __cplusplus
}
#endif

#endif


