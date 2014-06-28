/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

#define LOG_TAG "bte_conf"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <utils/Log.h>

#include "bta_api.h"
#include "config.h"

// TODO: eliminate these global variables.
extern char hci_logfile[256];
extern BOOLEAN hci_logging_enabled;
extern BOOLEAN trace_conf_enabled;
void bte_trace_conf_config(const config_t *config);

// Reads the stack configuration file and populates global variables with
// the contents of the file.
void bte_load_conf(const char *path) {
  assert(path != NULL);

  ALOGI("%s attempt to load stack conf from %s", __func__, path);

  config_t *config = config_new(path);
  if (!config) {
    ALOGI("%s file >%s< not found", __func__, path);
    return;
  }

  strlcpy(hci_logfile, config_get_string(config, CONFIG_DEFAULT_SECTION, "BtSnoopFileName", ""), sizeof(hci_logfile));
  hci_logging_enabled = config_get_bool(config, CONFIG_DEFAULT_SECTION, "BtSnoopLogOutput", false);
  trace_conf_enabled = config_get_bool(config, CONFIG_DEFAULT_SECTION, "TraceConf", false);

  bte_trace_conf_config(config);
  config_free(config);
}

// Parses the specified Device ID configuration file and registers the
// Device ID records with SDP.
void bte_load_did_conf(const char *p_path) {
    assert(p_path != NULL);

    config_t *config = config_new(p_path);
    if (!config) {
        ALOGE("%s unable to load DID config '%s'.", __func__, p_path);
        return;
    }

    for (int i = 1; i <= BTA_DI_NUM_MAX; ++i) {
        char section_name[16] = { 0 };
        snprintf(section_name, sizeof(section_name), "DID%d", i);

        if (!config_has_section(config, section_name)) {
            ALOGD("%s no section named %s.", __func__, section_name);
            break;
        }

        tBTA_DI_RECORD record;
        record.vendor = config_get_int(config, section_name, "vendorId", LMP_COMPID_BROADCOM);
        record.vendor_id_source = config_get_int(config, section_name, "vendorIdSource", DI_VENDOR_ID_SOURCE_BTSIG);
        record.product = config_get_int(config, section_name, "productId", 0);
        record.version = config_get_int(config, section_name, "version", 0);
        record.primary_record = config_get_bool(config, section_name, "primaryRecord", false);
        strlcpy(record.client_executable_url, config_get_string(config, section_name, "clientExecutableURL", ""), sizeof(record.client_executable_url));
        strlcpy(record.service_description, config_get_string(config, section_name, "serviceDescription", ""), sizeof(record.service_description));
        strlcpy(record.documentation_url, config_get_string(config, section_name, "documentationURL", ""), sizeof(record.documentation_url));

        if (record.vendor_id_source != DI_VENDOR_ID_SOURCE_BTSIG &&
            record.vendor_id_source != DI_VENDOR_ID_SOURCE_USBIF) {
            ALOGE("%s invalid vendor id source %d; ignoring DID record %d.", __func__, record.vendor_id_source, i);
            continue;
        }

        ALOGD("Device ID record %d : %s", i, (record.primary_record ? "primary" : "not primary"));
        ALOGD("  vendorId            = %04x", record.vendor);
        ALOGD("  vendorIdSource      = %04x", record.vendor_id_source);
        ALOGD("  product             = %04x", record.product);
        ALOGD("  version             = %04x", record.version);
        ALOGD("  clientExecutableURL = %s", record.client_executable_url);
        ALOGD("  serviceDescription  = %s", record.service_description);
        ALOGD("  documentationURL    = %s", record.documentation_url);

        uint32_t record_handle;
        tBTA_STATUS status = BTA_DmSetLocalDiRecord(&record, &record_handle);
        if (status != BTA_SUCCESS) {
            ALOGE("%s unable to set device ID record %d: error %d.", __func__, i, status);
        }
    }

    config_free(config);
}
