/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/************************************************************************************
 *
 *  Filename:     sdptool.c
 *
 *  Description:   Bluedroid Test application
 *
 ***********************************************************************************/


#include <stdio.h>
#include <sys/capability.h>
#include <ctype.h>
#include "l2c_api.h"
#include "bt_testapp.h"
#include <sys/capability.h>

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define PID_FILE "/data/.bdt_pid"
#define DEBUG 1
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#define MAX_PARAMS 10
#define MAX_PARAM_LENGTH 100

#define CASE_RETURN_STR(const) case const: return #const;

/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/

static unsigned char main_done = 0;
static bt_status_t status;
typedef struct
{
    UINT32 spp;
    UINT32 dummy;
}sdp_handle;


typedef struct
{
    UINT16      status;
    UINT16      length;
    BD_NAME     remote_bd_name;
} remote_dev_name;

static sdp_handle msdpHandle;
UINT32 getSdpHandle(profileName pName);
profileName getProfileName(char *p);


/* Main API */
static bluetooth_device_t* bt_device;

const bt_interface_t* sBtInterface = NULL;

static gid_t groups[] = { AID_NET_BT, AID_INET, AID_NET_BT_ADMIN,
                          AID_SYSTEM, AID_MISC, AID_SDCARD_RW,
                          AID_NET_ADMIN, AID_VPN};

/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char bt_enabled = 0;

//const btl2cap_interface_t *sL2capInterface = NULL;
const btsdp_interface_t *sSdpInterface = NULL;

/************************************************************************************
**  Static functions
************************************************************************************/

static void process_cmd(char *p, unsigned char is_job);
static void job_handler(void *param);
char params[MAX_PARAMS][MAX_PARAM_LENGTH];
static void get_bdaddr(const char *str, bt_bdaddr_t *bd);
static void get_rm_name(char *p);
static void RemoteDeviceNameCB(void *p_data);


//parses the command passed and returns the number of parameters passed. stores the parameters in param array
// stores everything in lowercase
int parseCommand(char *p);

static void RemoteDeviceNameCB (void *p_data)
{
    remote_dev_name *rmd_name = (remote_dev_name*)p_data;
    if(NULL != rmd_name && 0 != rmd_name->length&&
       '\0' == rmd_name->remote_bd_name[rmd_name->length])
    {
        printf("Remote device name:%s\n", rmd_name->remote_bd_name);
    }
    else
    {
        printf("Failed to get Remote device name\n");
    }
    return;
}

static void pSDP_cmpl_cb(UINT16 result)
{
    printf("sdp completed .Result=%d\n\n\n\n\n",result);

    switch(result)
    {
     case SDP_SUCCESS:
     sSdpInterface->printSearchedServices();
     break;
     case SDP_NO_RECS_MATCH:
     printf("SDP_NO_RECS_MATCH\n");
     break;
     case SDP_CONN_FAILED:
     printf("SDP_CONN_FAILED\n");
     break;
     case SDP_CFG_FAILED:
     printf("SDP_CFG_FAILED\n");
     break;
     case SDP_GENERIC_ERROR:
     printf("SDP_GENERIC_ERROR\n");
     break;
     case SDP_DB_FULL:
     printf("SDP_DB_FULL\n");
     break;
     case SDP_INVALID_PDU:
     printf("SDP_INVALID_PDU\n");
     break;
     case SDP_SECURITY_ERR:
     printf("SDP_SECURITY_ERR\n");
     break;
     case SDP_CONN_REJECTED:
     printf("SDP_CONN_REJECTED\n");
     break;
     case SDP_CANCEL:
     printf("SDP_CANCEL\n");
     break;
     default:
     printf("SDP Failed for unknown reason\n");
     break;
    }
    sSdpInterface->Cleanup();
}


/************************************************************************************
**  Shutdown helper functions
************************************************************************************/

static void bdt_shutdown(void)
{
    printf("shutdown bdroid test app\n");
    main_done = 1;
}


/*****************************************************************************
** Android's init.rc does not yet support applying linux capabilities
*****************************************************************************/

static void config_permissions(void)
{
    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;

    printf("set_aid_and_cap : pid %d, uid %d gid %d\n", getpid(), getuid(), getgid());

    header.pid = 0;

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    setuid(AID_BLUETOOTH);
    setgid(AID_BLUETOOTH);

    header.version = _LINUX_CAPABILITY_VERSION;

    cap.effective = cap.permitted =  cap.inheritable =
                    1 << CAP_NET_RAW |
                    1 << CAP_NET_ADMIN |
                    1 << CAP_NET_BIND_SERVICE |
                    1 << CAP_SYS_RAWIO |
                    1 << CAP_SYS_NICE |
                    1 << CAP_SETGID;

    capset(&header, &cap);
    setgroups(sizeof(groups)/sizeof(groups[0]), groups);
}



/*****************************************************************************
**   Logger API
*****************************************************************************/


/*******************************************************************************
 ** Misc helper functions
 *******************************************************************************/
static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

/*******************************************************************************
 ** Console helper functions
 *******************************************************************************/

void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

static void get_bdaddr(const char *str, bt_bdaddr_t *bd) {
    char *d = ((char *)bd), *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d++ = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(bd, 0, sizeof(bt_bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}

#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)
#define if_cmd(str)  if (is_cmd(str))

typedef struct
{
    const char *name;
    const UINT32 identifier;
}profile;

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    unsigned char is_job;
} t_cmd;


const t_cmd console_cmd_list[];
static int console_cmd_maxlen = 0;
const profile supportedProfiles[] =
{
    {"dummy",DUMMY},
    {"all",ALL},
    {"spp",SPP},
    {"ftp",FTP},
    {"opp",OPP},
    {NULL, NOT_SUPPORTED},
};

static void cmdjob_handler(void *param)
{
    char *job_cmd = (char*)param;

    process_cmd(job_cmd, 1);
    free(job_cmd);
}

static int create_cmdjob(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    if (job_cmd) {
        strlcpy(job_cmd, cmd, sizeof(cmd));

        if (pthread_create(&thread_id, NULL,
                       (void*)cmdjob_handler, (void*)job_cmd)!=0)
            perror("pthread_create");
    }
    else
        perror("SDP: create_cmdjob(), failed to allocate memory");
    return 0;
}

/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/

int HAL_load(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    printf("Loading HAL lib + extensions\n");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0)
    {
        err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            bt_device = (bluetooth_device_t *)device;
            sBtInterface = bt_device->get_bluetooth_interface();
        }
    }

    printf("HAL library loaded (%s)\n", strerror(err));

    return err;
}

int HAL_unload(void)
{
    int err = 0;

    printf("Unloading HAL lib");

    sBtInterface = NULL;

    printf("HAL library unloaded (%s)\n", strerror(err));

    return err;
}

/*******************************************************************************
 ** HAL test functions & callbacks
 *******************************************************************************/

void setup_test_env(void)
{
    int i = 0;

    while (console_cmd_list[i].name != NULL)
    {
        console_cmd_maxlen = MAX(console_cmd_maxlen, (int)strlen(console_cmd_list[i].name));
        i++;
    }
}

void check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS) {
        printf("HAL REQUEST FAILED\n");
    } else {
        printf("HAL REQUEST SUCCESS\n");
    }
}

static void adapter_state_changed(bt_state_t state)
{
    int V1 = 1000, V2=2;
    bt_property_t property = {9 /*BT_PROPERTY_DISCOVERY_TIMEOUT*/, 4, &V1};
    bt_property_t property1 = {7 /*SCAN*/, 2, &V2};
    bt_property_t property2 ={1,6,"Manoj"};

    if (state == BT_STATE_ON) {
        bt_enabled = 1;
    } else {
    bt_enabled = 0;
    }
    if (bt_enabled) {
        printf("\nSetinng it to scan\n");
        status = sBtInterface->set_adapter_property(&property1);
        status = sBtInterface->set_adapter_property(&property);
        status = sBtInterface->set_adapter_property(&property2);
    }
}

static void adapter_properties_changed(bt_status_t status, int num_properties, bt_property_t *properties)
{

    char Bd_addr[15] = {0};
    printf("\n adapter_properties_changed  status= %d, num_properties=%d\n", status, num_properties);
    if(NULL == properties) {
        printf("\nproperties is null\n");
        return;
    }
    switch(properties->type) {
        case BT_PROPERTY_BDADDR:
        memcpy(Bd_addr, properties->val, properties->len);
        printf("\nLocal Bd Addr = %0x:%0x:%0x:%0x:%0x:%0x\n", Bd_addr[0], Bd_addr[1], Bd_addr[2], Bd_addr[3], Bd_addr[4], Bd_addr[5]);
        break;
        default:
        break;
    }
    return;
}

static void discovery_state_changed(bt_discovery_state_t state)
{
    printf("Discovery State Updated : %s\n", (state == BT_DISCOVERY_STOPPED)?"STOPPED":"STARTED");
}

static void acl_state_changed(bt_status_t status, bt_bdaddr_t *remote_bd_addr, bt_acl_state_t state)
{
    printf("acl_state_changed : remote_bd_addr=%x:%x:%x:%x:%x:%x, acl status=%s \n",
            remote_bd_addr->address[0], remote_bd_addr->address[1], remote_bd_addr->address[2],
            remote_bd_addr->address[3], remote_bd_addr->address[4], remote_bd_addr->address[5],
            (state == BT_ACL_STATE_CONNECTED)?"ACL Connected" :"ACL Disconnected");
}


static void dut_mode_recv(uint16_t opcode, uint8_t *buf, uint8_t len)
{
    printf("DUT MODE RECV : NOT IMPLEMENTED\n");
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    adapter_properties_changed, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */
    NULL, /* device_found_cb */
    discovery_state_changed, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    dut_mode_recv, /*dut_mode_recv_cb */
    NULL,
    NULL, /*energy_info_cb */
    NULL, /*le_lpp_write_rssi_thresh_cb*/
    NULL, /*le_lpp_read_rssi_thresh_cb*/
    NULL, /*le_lpp_enable_rssi_monitor_cb*/
    NULL  /*le_lpp_rssi_threshold_evt_cb*/
};

static bool set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb, void *data) {
    static timer_t timer;
    static bool timer_created;

    if (!timer_created) {
        struct sigevent sigevent;
        memset(&sigevent, 0, sizeof(sigevent));
        sigevent.sigev_notify = SIGEV_THREAD;
        sigevent.sigev_notify_function = (void (*)(union sigval))cb;
        sigevent.sigev_value.sival_ptr = data;
        timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
        timer_created = true;
    }
    struct itimerspec new_value;
    new_value.it_value.tv_sec = delay_millis / 1000;
    new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    timer_settime(timer, 0, &new_value, NULL);
    return true;
}

static int acquire_wake_lock(const char *lock_name) {
    return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *lock_name) {
    return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t),
    set_wake_alarm,
    acquire_wake_lock,
    release_wake_lock,
};

void bdt_init(void)
{
    printf("INIT BT ");

    if (BT_STATUS_SUCCESS == (status = sBtInterface->init(&bt_callbacks))) {
        status = sBtInterface->set_os_callouts(&callouts);
    }else {
        printf("INIT BT Failed!!");
    }
    check_return_status(status);
}

void bdt_enable(void)
{
    printf("\nENABLE BT\n");
    if (bt_enabled) {
        printf("\nBluetooth is already enabled\n");
        return;
    }
    status = sBtInterface->enable();
    return;
}

void  bdt_disable(void)
{
    if (!bt_enabled) {
        printf("\nBluetooth is already disabled\n");
        return;
    }
    status = sBtInterface->disable();
    check_return_status(status);
    return;
}

void bdt_cleanup(void)
{
    printf("CLEANUP");
    sBtInterface->cleanup();
}



/*******************************************************************************
 ** Console commands
 *******************************************************************************/

void do_help(char *p)
{
    int i = 0;
    int max = 0;
    char line[128];
    int pos = 0;
    while (console_cmd_list[i].name != NULL)
    {
        pos = snprintf(line,sizeof(line), "%s", (char*)console_cmd_list[i].name);
        //pos = sprintf(line, "%s", (char*)console_cmd_list[i].name);
        printf("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
        i++;
    }
}

void do_quit(char *p)
{
    bdt_shutdown();
}

/*******************************************************************
 *
 *  BT TEST  CONSOLE COMMANDS
 *
 *  Parses argument lists and passes to API test function
 *
*/

void do_init(char *p)
{
    bdt_init();
}

void do_enable(char *p)
{
    bdt_enable();
}

void do_disable(char *p)
{
    bdt_disable();
}

void do_dut_mode_configure(char *p)
{
//    bdt_dut_mode_configure(p);
}

void do_cleanup(char *p)
{
   // bdt_cleanup();
}


const btsdp_interface_t* get_sdp_interface(void)
{
    printf("Get SDP interface");
    if (sBtInterface) {
        printf("\nGet SDP interface\n");
        return (btsdp_interface_t*)sBtInterface->get_testapp_interface(TEST_APP_SDP);
    }
    return NULL;
}

void do_sdp_init(char *p)
{
    printf("Initializing SDP interface\n");
    if(NULL == sSdpInterface) {
        sSdpInterface = get_sdp_interface();
    }
    if (sSdpInterface)
       sSdpInterface->Init(pSDP_cmpl_cb);
    msdpHandle.spp = 0;
    msdpHandle.dummy = 0;
}

void do_sdp_Search_services(char *p)
{
    bt_bdaddr_t p_bd_addr;
    int length = parseCommand(p);


    if(length==-1) {
        return;
    }
    if(length>0) {
        get_bdaddr(params[0], &p_bd_addr);
    } else {
        printf("Invalid Parameter:\n");
        do_help(NULL);
        return;
    }

    if(sSdpInterface->SearchServices((UINT8 *)&p_bd_addr)==SUCCESS) {
        printf("sdp started succeessfully\n");
    } else {
        printf("sdp failed\n");
    }
}

static void get_rm_name(char *p)
{
    bt_bdaddr_t p_bd_addr;
    int length = parseCommand(p);
    profileName pName;
    UINT32 handle;

    if(length==-1) {
        return;
    }
    if(length>0) {
        get_bdaddr(params[0], &p_bd_addr);
    } else {
        printf("Invalid Parameter:\n");
        do_help(NULL);
        return;
    }
    if(NULL != sSdpInterface) {
        if (sSdpInterface->GetRemoteDeviceName) {
            if(sSdpInterface->GetRemoteDeviceName((UINT8 *)&p_bd_addr, RemoteDeviceNameCB)) {
                printf("Remote Device Name Request Started\n");
            } else {
                printf("Remote Device Name Request failed\n");
            }
        }else{
            printf("something weird\n");
        }
    }
    else {
        printf("soemthing wrong\n");
    }
}

/**
Returns the Number of Parameters passed. Splits on space and stores them in the param Array
*/
int parseCommand(char *p)
{

    int i=0;
    int j=-1;
    int k=-1;
    int length=0;
    char prev = ' ';
    printf("The parameter passed is :%s\n",p);
    memset(params,0,sizeof(params));

    if(p==NULL) {
    return 0;
    }
    //printf("length of parameter is :%d\n",strlen(p));
    length=strlen(p);
    for(i=0;i<length;i++) {
        if(k==MAX_PARAM_LENGTH -1) {
            printf("One of the parameters specified exceed the allowed MAX length:%d of a single parameter\n",MAX_PARAM_LENGTH-1);
            return -1;
        }
        if((prev==' ')&&(*p!=' ')) {
            j++;
            if(j >= MAX_PARAMS) {
                printf("No of params specified exceed the max limit: %d\n",MAX_PARAMS);
                return -1;
            }
            k=0;
            params[j][k] = *p;
            prev = *p;
        } else if((prev!=' ') && (*p != ' ')) {
            k++;
            params[j][k] = *p;
            prev = *p;
        } else {
            prev = *p;
        }
        p++;
    }
    return (j >= 0 ? j+1:0);
}

profileName getProfileName(char *p)
{
    int i=0;
    while(supportedProfiles[i].name!=NULL) {
        if((strstr(p,supportedProfiles[i].name)==p) &&strlen(p)==strlen(supportedProfiles[i].name)) {
            return supportedProfiles[i].identifier;
        }
        i++;
    }
    return NOT_SUPPORTED;
}


UINT32 getSdpHandle(profileName pName)
{

    if(sSdpInterface==NULL) {
        printf("Initialize the stack before executing any other command\n");
        return 0;
    }
    switch(pName) {
        case SPP:
            printf("msdpHandle.spp= %d\n",(int)msdpHandle.spp);
            if(msdpHandle.spp==0) {
                msdpHandle.spp=sSdpInterface->CreateNewRecord();
            }
            printf("msdpHandle.spp= %d\n",(int)msdpHandle.spp);
            return msdpHandle.spp;
        case DUMMY:
            printf("msdpHandle.dummy= %d\n",(int)msdpHandle.dummy);
            if(msdpHandle.dummy==0) {
                msdpHandle.dummy=sSdpInterface->CreateNewRecord();
            }
            printf("msdpHandle.dummy= %d\n",(int)msdpHandle.dummy);
            return msdpHandle.dummy;
        default:
        break;

    }
    return 0;
}

void do_sdp_add_Profile(char *p)
{

    int length = parseCommand(p);
    profileName pName;
    UINT32 handle;
    if(DEBUG)printf("Number of parameters = %d\n",length);
    if(length==-1) {
        return;
    }
    if(length>0) {
        pName = getProfileName(params[0]);
    } else {
        printf("Invalid Parameter: Please specify the Profile to be added\n");
        do_help(NULL);
        return;
    }
    printf("pName= %d\n",pName);
    if(pName==NOT_SUPPORTED) {
        printf("Invalid profile name. please check the command options\n");
        do_help(NULL);
        return;
    }
    handle = getSdpHandle(pName);
    if(!sSdpInterface->AddRecord(handle,pName)==SUCCESS) {
        printf("Failed to add the specified profile\n");
    } else {
        printf("profile Successfully added\n");
    }
    return;
}
/*******************************************************************
 *
 *  CONSOLE COMMAND TABLE
 *
*/

const t_cmd console_cmd_list[] =
{

    { "help", do_help, "lists all available console commands", 0 },
    { "quit", do_quit, "", 0},
    /*
     * API CONSOLE COMMANDS
     */
    { "enablebt", do_enable, ":: enables bluetooth", 0 },
    { "disablebt", do_disable, ":: disables bluetooth", 0 },
    {"init",do_sdp_init,"::Initializes SDP database",0},
    {"searchservices",do_sdp_Search_services,"::Search services on remote phone::searchservices <aa:aa:aa:aa:aa:aa>",0},
    {"add",do_sdp_add_Profile,"::Adds a new service on the DUT-Syntax add profilename<spp/dummy/all/opp/ftp>",0},
    {"get_rm_name",get_rm_name,"::Deletes a service on the DUT-Syntax get_rm_name <aa:aa:aa:aa:aa:aa>",0},
    {NULL, NULL, "", 0},
};

/*
 * Main console command handler
*/

static void process_cmd(char *p, unsigned char is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {
            if (!is_job && console_cmd_list[i].is_job)
                create_cmdjob(p_saved);
            else
            {
                console_cmd_list[i].handler(p);
            }
            return;
        }
        i++;
    }
    printf("%s : unknown command\n", p_saved);
    do_help(NULL);
}

int main (int argc, char * argv[])
{

    int V1 = 1000, V2=2;
    int opt;
    int status = 0;
    char cmd[128];
    int args_processed = 0;
    int pid = -1;

   config_permissions();
   if ( HAL_load() < 0 ) {
       perror("HAL failed to initialize, exit\n");
       unlink(PID_FILE);
       exit(0);
   }
    setup_test_env();
    bdt_init();
    while(!main_done) {
        char line[128];
        fflush(stdout);
        fgets (line, 128, stdin);
        if (line[0]!= '\0') {
            line[strlen(line)-1] = 0;
            process_cmd(line, 0);
            memset(line, '\0', 128);
       }
    }

ERR:
    HAL_unload();
    printf(":: Bluedroid test app terminating");
    return 0;
}
