/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
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
 *  Filename:      btif_sock_thread.c
 *
 *  Description:   socket select thread
 *
 *
 ***********************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>

//bta_jv_co_rfc_data
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>

#include <sys/select.h>
#include <sys/poll.h>
#include <cutils/sockets.h>
#include <alloca.h>

#define LOG_TAG "BTIF_SOCK"
#include "btif_common.h"
#include "btif_util.h"

#include "bd.h"

#include "bta_api.h"
#include "btif_sock.h"
#include "btif_sock_thread.h"
#include "btif_sock_util.h"

#include <cutils/log.h>
#define info(fmt, ...)  ALOGI ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define debug(fmt, ...) ALOGD ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define error(fmt, ...) ALOGE ("## ERROR : %s: " fmt "##",__FUNCTION__,  ## __VA_ARGS__)
#define asrt(s) if(!(s)) ALOGE ("## %s assert %s failed at line:%d ##",__FUNCTION__, #s, __LINE__)
#define print_events(events) do { \
    debug("print poll event:%x", events); \
    if (events & POLLIN) debug(  "   POLLIN "); \
    if (events & POLLPRI) debug( "   POLLPRI "); \
    if (events & POLLOUT) debug( "   POLLOUT "); \
    if (events & POLLERR) debug( "   POLLERR "); \
    if (events & POLLHUP) debug( "   POLLHUP "); \
    if (events & POLLNVAL) debug("   POLLNVAL "); \
    if (events & POLLRDHUP) debug("   POLLRDHUP"); \
    } while(0)

#define MAX_THREAD 8
#define MAX_POLL 64
#define POLL_EXCEPTION_EVENTS (POLLHUP | POLLRDHUP | POLLERR | POLLNVAL)
#define IS_EXCEPTION(e) ((e) & POLL_EXCEPTION_EVENTS)
#define IS_READ(e) ((e) & POLLIN)
#define IS_WRITE(e) ((e) & POLLOUT)
/*cmd executes in socket poll thread */
#define CMD_WAKEUP       1
#define CMD_EXIT         2
#define CMD_ADD_FD       3
#define CMD_USER_PRIVATE 4

typedef struct {
    struct pollfd pfd;
    uint32_t user_id;
    int type;
    int flags;
} poll_slot_t;
typedef struct {
    int cmd_fdr, cmd_fdw;
    int poll_count;
    poll_slot_t ps[MAX_POLL];
    int psi[MAX_POLL]; //index of poll slot
    volatile pid_t thread_id;
    btsock_signaled_cb callback;
    btsock_cmd_cb cmd_callback;
    int used;
} thread_slot_t;
static thread_slot_t ts[MAX_THREAD];



static void *sock_poll_thread(void *arg);
static inline void close_cmd_fd(int h);

static inline void add_poll(int h, int fd, int type, int flags, uint32_t user_id);

static pthread_mutex_t thread_slot_lock;


static inline void set_socket_blocking(int s, int blocking)
{
    int opts;
    opts = fcntl(s, F_GETFL);
    if (opts<0) error("set blocking (%s)", strerror(errno));
    if(blocking)
        opts &= ~O_NONBLOCK;
    else opts |= O_NONBLOCK;
    fcntl(s, F_SETFL, opts);
}

static inline int create_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    debug("covert name to android abstract name:%s", name);
    if(socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0)
    {
        if(listen(s, 5) == 0)
        {
            debug("listen to local socket:%s, fd:%d", name, s);
            return s;
        }
        else error("listen to local socket:%s, fd:%d failed, errno:%d", name, s, errno);
    }
    else error("create local socket:%s fd:%d, failed, errno:%d", name, s, errno);
    close(s);
    return -1;
}
static inline int connect_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    set_socket_blocking(s, TRUE);
    if(socket_local_client_connect(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) >= 0)
    {
        debug("connected to local socket:%s, fd:%d", name, s);
        return s;
    }
    else error("connect to local socket:%s, fd:%d failed, errno:%d", name, s, errno);
    close(s);
    return -1;
}
static inline int accept_server_socket(int s)
{
    struct sockaddr_un client_address;
    socklen_t clen;
    int fd = accept(s, (struct sockaddr*)&client_address, &clen);
    debug("accepted fd:%d for server fd:%d", fd, s);
    return fd;
}
static inline pthread_t create_thread(void *(*start_routine)(void *), void * arg)
{
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t thread_id = -1;
    if( pthread_create(&thread_id, &thread_attr, start_routine, arg)!=0 )
    {
        error("pthread_create : %s", strerror(errno));
        return -1;
    }
    return thread_id;
}
static void init_poll(int cmd_fd);
static int alloc_thread_slot()
{
    int i;
    //revserd order to save guard uninitialized access to 0 index
    for(i = MAX_THREAD - 1; i >=0; i--)
    {
        debug("ts[%d].used:%d", i, ts[i].used);
        if(!ts[i].used)
        {
            ts[i].used = 1;
            return i;
        }
    }
    error("execeeded max thread count");
    return -1;
}
static void free_thread_slot(int h)
{
    if(0 <= h && h < MAX_THREAD)
    {
        close_cmd_fd(h);
        ts[h].used = 0;
    }
    else error("invalid thread handle:%d", h);
}
int btsock_thread_init()
{
    static int initialized;
    debug("in initialized:%d", initialized);
    if(!initialized)
    {
        initialized = 1;
        init_slot_lock(&thread_slot_lock);
        int h;
        for(h = 0; h < MAX_THREAD; h++)
        {
            ts[h].cmd_fdr = ts[h].cmd_fdw = -1;
            ts[h].used = 0;
            ts[h].thread_id = -1;
            ts[h].poll_count = 0;
            ts[h].callback = NULL;
            ts[h].cmd_callback = NULL;
        }
    }
    return TRUE;
}
int btsock_thread_create(btsock_signaled_cb callback, btsock_cmd_cb cmd_callback)
{
    int ret = FALSE;
    asrt(callback || cmd_callback);
    lock_slot(&thread_slot_lock);
    int h = alloc_thread_slot();
    unlock_slot(&thread_slot_lock);
    debug("alloc_thread_slot ret:%d", h);
    if(h >= 0)
    {
        init_poll(h);
        if((ts[h].thread_id = create_thread(sock_poll_thread, (void*)h)) != -1)
        {
            debug("h:%d, thread id:%d", h, ts[h].thread_id);
            ts[h].callback = callback;
            ts[h].cmd_callback = cmd_callback;
        }
        else
        {
            free_thread_slot(h);
            h = -1;
        }
    }
    return h;
}

/* create dummy socket pair used to wake up select loop */
static inline void init_cmd_fd(int h)
{
    asrt(ts[h].cmd_fdr == -1 && ts[h].cmd_fdw == -1);
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, &ts[h].cmd_fdr) < 0)
    {
        error("socketpair failed: %s", strerror(errno));
        return;
    }
    debug("h:%d, cmd_fdr:%d, cmd_fdw:%d", h, ts[h].cmd_fdr, ts[h].cmd_fdw);
    //add the cmd fd for read & write
    add_poll(h, ts[h].cmd_fdr, 0, SOCK_THREAD_FD_RD, 0);
}
static inline void close_cmd_fd(int h)
{
    if(ts[h].cmd_fdr != -1)
    {
        close(ts[h].cmd_fdr);
        ts[h].cmd_fdr = -1;
    }
    if(ts[h].cmd_fdw != -1)
    {
        close(ts[h].cmd_fdw);
        ts[h].cmd_fdw = -1;
    }
}
typedef struct
{
    int id;
    int fd;
    int type;
    int flags;
    uint32_t user_id;
} sock_cmd_t;
int btsock_thread_add_fd(int h, int fd, int type, int flags, uint32_t user_id)
{
    if(h < 0 || h >= MAX_THREAD)
    {
        error("invalid bt thread handle:%d", h);
        return FALSE;
    }
    if(ts[h].cmd_fdw == -1)
    {
        error("cmd socket is not created. socket thread may not initialized");
        return FALSE;
    }
    if(flags & SOCK_THREAD_ADD_FD_SYNC)
    {
        //must executed in socket poll thread
        if(ts[h].thread_id == pthread_self())
        {
            //cleanup one-time flags
            flags &= ~SOCK_THREAD_ADD_FD_SYNC;
            add_poll(h, fd, type, flags, user_id);
            return TRUE;
        }
        debug("SOCK_THREAD_ADD_FD_SYNC is not called in poll thread context, fallback to async");
    }
    sock_cmd_t cmd = {CMD_ADD_FD, fd, type, flags, user_id};
    debug("adding fd:%d, flags:0x%x", fd, flags);
    return send(ts[h].cmd_fdw, &cmd, sizeof(cmd), 0) == sizeof(cmd);
}
int btsock_thread_post_cmd(int h, int type, const unsigned char* data, int size, uint32_t user_id)
{
    if(h < 0 || h >= MAX_THREAD)
    {
        error("invalid bt thread handle:%d", h);
        return FALSE;
    }
    if(ts[h].cmd_fdw == -1)
    {
        error("cmd socket is not created. socket thread may not initialized");
        return FALSE;
    }
    sock_cmd_t cmd = {CMD_USER_PRIVATE, 0, type, size, user_id};
    debug("post cmd type:%d, size:%d, h:%d, ", type, size, h);
    sock_cmd_t* cmd_send = &cmd;
    int size_send = sizeof(cmd);
    if(data && size)
    {
        size_send = sizeof(cmd) + size;
        cmd_send = (sock_cmd_t*)alloca(size_send);
        if(cmd_send)
        {
            *cmd_send = cmd;
            memcpy(cmd_send + 1, data, size);
        }
        else
        {
            error("alloca failed at h:%d, cmd type:%d, size:%d", h, type, size_send);
            return FALSE;
        }
    }
    return send(ts[h].cmd_fdw, cmd_send, size_send, 0) == size_send;
}
int btsock_thread_wakeup(int h)
{
    if(h < 0 || h >= MAX_THREAD)
    {
        error("invalid bt thread handle:%d", h);
        return FALSE;
    }
    if(ts[h].cmd_fdw == -1)
    {
        error("thread handle:%d, cmd socket is not created", h);
        return FALSE;
    }
    sock_cmd_t cmd = {CMD_WAKEUP, 0, 0, 0, 0};
    return send(ts[h].cmd_fdw, &cmd, sizeof(cmd), 0) == sizeof(cmd);
}
int btsock_thread_exit(int h)
{
    if(h < 0 || h >= MAX_THREAD)
    {
        error("invalid bt thread handle:%d", h);
        return FALSE;
    }
    if(ts[h].cmd_fdw == -1)
    {
        error("cmd socket is not created");
        return FALSE;
    }
    sock_cmd_t cmd = {CMD_EXIT, 0, 0, 0, 0};
    if(send(ts[h].cmd_fdw, &cmd, sizeof(cmd), 0) == sizeof(cmd))
    {
        pthread_join(ts[h].thread_id, 0);
        lock_slot(&thread_slot_lock);
        free_thread_slot(h);
        unlock_slot(&thread_slot_lock);
        return TRUE;
    }
    return FALSE;
}
static void init_poll(int h)
{
    int i;
    ts[h].poll_count = 0;
    ts[h].thread_id = -1;
    ts[h].callback = NULL;
    ts[h].cmd_callback = NULL;
    for(i = 0; i < MAX_POLL; i++)
    {
        ts[h].ps[i].pfd.fd = -1;
        ts[h].psi[i] = -1;
    }
    init_cmd_fd(h);
}
static inline unsigned int flags2pevents(int flags)
{
    unsigned int pevents = 0;
    if(flags & SOCK_THREAD_FD_WR)
        pevents |= POLLOUT;
    if(flags & SOCK_THREAD_FD_RD)
        pevents |= POLLIN;
    pevents |= POLL_EXCEPTION_EVENTS;
    return pevents;
}

static inline void set_poll(poll_slot_t* ps, int fd, int type, int flags, uint32_t user_id)
{
    ps->pfd.fd = fd;
    ps->user_id = user_id;
    if(ps->type != 0 && ps->type != type)
        error("poll socket type should not changed! type was:%d, type now:%d", ps->type, type);
    ps->type = type;
    ps->flags = flags;
    ps->pfd.events = flags2pevents(flags);
    ps->pfd.revents = 0;
}
static inline void add_poll(int h, int fd, int type, int flags, uint32_t user_id)
{
    asrt(fd != -1);
    int i;
    int empty = -1;
    poll_slot_t* ps = ts[h].ps;

    for(i = 0; i < MAX_POLL; i++)
    {
        if(ps[i].pfd.fd == fd)
        {
            debug("update fd poll, count:%d, slot:%d, fd:%d, \
                    type was:%d, now:%d; flags was:%x, now:%x; user_id was:%d, now:%d",
                     ts[h].poll_count, i, fd, ps[i].type, type, ps[i].flags, flags, ps[i].user_id, user_id);
            asrt(ts[h].poll_count < MAX_POLL);

            set_poll(&ps[i], fd, type, flags | ps[i].flags, user_id);
            return;
        }
        else if(empty < 0 && ps[i].pfd.fd == -1)
            empty = i;
    }
    if(empty >= 0)
    {
        asrt(ts[h].poll_count < MAX_POLL);
        set_poll(&ps[empty], fd, type, flags, user_id);
        ++ts[h].poll_count;
        debug("add new fd poll, count:%d, slot:%d, fd:%d, type:%d; flags:%x; user_id:%d",
                ts[h].poll_count, empty, fd, type, flags, user_id);
        return;
    }
    error("exceeded max poll slot:%d!", MAX_POLL);
}
static inline void remove_poll(int h, poll_slot_t* ps, int flags)
{
    if(flags == ps->flags)
    {
        //all monitored events signaled. To remove it, just clear the slot
        --ts[h].poll_count;
        debug("remove poll, count:%d, fd:%d, flags:%x", ts[h].poll_count, ps->pfd.fd, ps->flags);
        memset(ps, 0, sizeof(*ps));
        ps->pfd.fd = -1;
    }
    else
    {
        //one read or one write monitor event signaled, removed the accordding bit
        debug("remove poll flag, count:%d, fd:%d, flags signaled:%x, poll flags was:%x, new:%x",
                ts[h].poll_count, ps->pfd.fd, flags, ps->flags, ps->flags & (~flags));
        ps->flags &= ~flags;
        //update the poll events mask
        ps->pfd.events = flags2pevents(ps->flags);
    }
}
static int process_cmd_sock(int h)
{
    sock_cmd_t cmd = {-1, 0, 0, 0, 0};
    int fd = ts[h].cmd_fdr;
    if(recv(fd, &cmd, sizeof(cmd), MSG_WAITALL) != sizeof(cmd))
    {
        error("recv cmd errno:%d", errno);
        return FALSE;
    }
    debug("cmd.id:%d", cmd.id);
    switch(cmd.id)
    {
        case CMD_ADD_FD:
            debug("CMD_ADD_FD, fd:%d, flags:%d", cmd.fd, cmd.flags);
            add_poll(h, cmd.fd, cmd.type, cmd.flags, cmd.user_id);
            break;
        case CMD_WAKEUP:
            debug("CMD_WAKEUP");
            break;
        case CMD_USER_PRIVATE:
            debug("CMD_USER_PRIVATE");
            asrt(ts[h].cmd_callback);
            if(ts[h].cmd_callback)
                ts[h].cmd_callback(fd, cmd.type, cmd.flags, cmd.user_id);
            break;
        case CMD_EXIT:
            debug("CMD_EXIT");
            return FALSE;
        default:
            debug("unknown cmd: %d", cmd.id);
             break;
    }
    return TRUE;
}
static void process_data_sock(int h, struct pollfd *pfds, int count)
{
    debug("in, poll count:%d ts[h].poll_count:%d", count, ts[h].poll_count);
    asrt(count <= ts[h].poll_count);
    int i;
    for( i= 1; i < ts[h].poll_count; i++)
    {
        if(pfds[i].revents)
        {
            int ps_i = ts[h].psi[i];
            asrt(pfds[i].fd == ts[h].ps[ps_i].pfd.fd);
            uint32_t user_id = ts[h].ps[ps_i].user_id;
            int type = ts[h].ps[ps_i].type;
            int flags = 0;
            debug("signaled data fd:%d, start print poll revents[", pfds[i].fd);
            print_events(pfds[i].revents);
            debug("signaled data fd:%d, end print poll revents]", pfds[i].fd);
            if(IS_READ(pfds[i].revents))
            {
                debug("read event signaled, fd:%d, user_id:%d", pfds[i].fd, user_id);
                flags |= SOCK_THREAD_FD_RD;
            }
            if(IS_WRITE(pfds[i].revents))
            {
                debug("write event signaled, fd:%d, user_id:%d", pfds[i].fd, user_id);
                flags |= SOCK_THREAD_FD_WR;
            }
            if(IS_EXCEPTION(pfds[i].revents))
            {
                debug("exception event signaled, fd:%d, user_id:%d", pfds[i].fd, user_id);
                flags |= SOCK_THREAD_FD_EXCEPTION;
                //remove the whole slot not flags
                remove_poll(h, &ts[h].ps[ps_i], ts[h].ps[ps_i].flags);
            }
            else if(flags)
                 remove_poll(h, &ts[h].ps[ps_i], flags); //remove the monitor flags that already processed
            if(flags)
                ts[h].callback(pfds[i].fd, type, flags, user_id);
        }
    }
    debug("out");
}
static void prepare_poll_fds(int h, struct pollfd* pfds)
{
    int count = 0;
    int ps_i = 0;
    int pfd_i = 0;
    asrt(ts[h].poll_count <= MAX_POLL);
    memset(pfds, 0, sizeof(pfds[0])*ts[h].poll_count);
    while(count < ts[h].poll_count)
    {
        if(ps_i >= MAX_POLL)
        {
            error("exceed max poll range, ps_i:%d, MAX_POLL:%d, count:%d, ts[h].poll_count:%d",
                    ps_i, MAX_POLL, count, ts[h].poll_count);
            return;
        }
        if(ts[h].ps[ps_i].pfd.fd >= 0)
        {
            pfds[pfd_i] =  ts[h].ps[ps_i].pfd;
            ts[h].psi[pfd_i] = ps_i;
            count++;
            pfd_i++;
        }
        ps_i++;
    }
}
static void *sock_poll_thread(void *arg)
{
    struct pollfd pfds[MAX_POLL];
    memset(pfds, 0, sizeof(pfds));
    int h = (int)arg;
    for(;;)
    {
        prepare_poll_fds(h, pfds);
        debug("call poll, thread handle h:%d, cmd fd read:%d, ts[h].poll_count:%d",
                h, ts[h].cmd_fdr, ts[h].poll_count);
        int ret = poll(pfds, ts[h].poll_count, -1);
        if(ret == -1)
        {
            error("poll ret -1, exit the thread, errno:%d, err:%s", errno, strerror(errno));
            debug("ts[%d].poll_count:%d", h, ts[h].poll_count);
            break;
        }
        if(ret != 0)
        {
            debug("select wake up, h:%d, ret:%d, ts[h].poll_count:%d", h, ret, ts[h].poll_count);
            int need_process_data_fd = TRUE;
            if(pfds[0].revents) //cmd fd always is the first one
            {
                debug("signaled cmd_fd:%d, start print poll revents[", pfds[0].fd);
                print_events(pfds[0].revents);
                debug("signaled cmd_fd:%d, end print poll revents]", pfds[0].fd);
                asrt(pfds[0].fd == ts[h].cmd_fdr);
                if(!process_cmd_sock(h))
                {
                    debug("h:%d, process_cmd_sock return false, exit...", h);
                    break;
                }
                if(ret == 1)
                    need_process_data_fd = FALSE;
                else ret--; //exclude the cmd fd
            }
            if(need_process_data_fd)
                process_data_sock(h, pfds, ret);
        }
        else debug("no data, select ret: %d", ret);
    }
    ts[h].thread_id = -1;
    debug("socket poll thread exiting, h:%d", h);
    return 0;
}

