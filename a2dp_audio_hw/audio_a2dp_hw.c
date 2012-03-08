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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/

#define LOG_TAG "audio_a2dp_hw"
#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/errno.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <cutils/str_parms.h>
#include <cutils/log.h>
#include <cutils/sockets.h>


#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#define A2DP_AUDIO_HARDWARE_INTERFACE "audio.a2dp"

#define A2DP_SOCK_PATH "/data/misc/bluedroid/.a2dp_sock"

#define AUDIO_CHANNEL_DEFAULT_RATE 44100
#define AUDIO_CHANNEL_DEFAULT_FORMAT AUDIO_FORMAT_PCM_16_BIT;
#define AUDIO_CHANNEL_OUTPUT_BUFFER_SZ (20*512) /* fixme -- tune this */

#define SKT_DISCONNECTED (-1)

#define ADEV_TRACE() LOGV("%s", __FUNCTION__);
#define ADEV_LOG(fmt, ...)  LOGD ("%s: " fmt,__FUNCTION__, ## __VA_ARGS__)


struct a2dp_stream_out;

struct a2dp_audio_device {
    struct audio_hw_device device;
    struct a2dp_stream_out *output;
};

struct a2dp_stream_out {
    struct audio_stream_out stream;
    int sock_fd;
};

struct a2dp_stream_in {
    struct audio_stream_in stream;
};

static size_t out_get_buffer_size(const struct audio_stream *stream);

/*****************************************************************************
**
**   BT stack adaptation
**
*****************************************************************************/

void set_blocking(int s)
{
    int opts;
    opts = fcntl(s, F_GETFL);
    if (opts < 0)
        ADEV_LOG("set blocking (%s)", strerror(errno));
    opts &= ~O_NONBLOCK;
    fcntl(s, F_SETFL, opts);
}

static inline int connect_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);

    set_blocking(s);

    if(socket_local_client_connect(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) >= 0)
    {
        ADEV_LOG("connect to %s (fd:%d)", name, s);
        return s;
    }
    else
    {
        ADEV_LOG("connect to %s (fd:%d) failed, errno:%s", name, s,  strerror(errno));
    }

    close(s);

    return -1;        
}

static int skt_connect(char *path)
{
    int ret;
    int fd;
    struct sockaddr_un remote;
    int len;

    ADEV_LOG("connect to %s", path);

    if ((fd = connect_server_socket(path)) == -1)
    {
        ADEV_LOG("failed to connect (%s)", strerror(errno));
        close(fd);
        return -1;
    }

    len = AUDIO_CHANNEL_OUTPUT_BUFFER_SZ;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&len, (int)sizeof(len));

    /* add checks for return status */

    LOGD("sonnected to stack fd = %d", fd);

    return fd;
}

static int skt_write(int fd, const void *p, size_t len)
{
    int sent;
    struct pollfd pfd;

    ADEV_TRACE();

    pfd.fd = fd;
    pfd.events = POLLOUT;

    /* poll for 500 ms */

    /* send time out */
    if (poll(&pfd, 1, 500) == 0)
        return 0;

    if ((sent = send(fd, p, len, MSG_NOSIGNAL)) == -1) {
        LOGE("stream_write failed with errno=%d", errno);
        return -1;
    }

    return sent;
}

static int skt_disconnect(int fd)
{
    ADEV_LOG("fd %d", fd);

    if (fd != SKT_DISCONNECTED)
    {
        close(fd);
    }
    return 0;
}

/*****************************************************************************
**
**  audio output callbacks
**
*****************************************************************************/


static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_CHANNEL_DEFAULT_RATE;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ADEV_LOG("out_set_sample_rate : %d", rate);

    if (rate != AUDIO_CHANNEL_DEFAULT_RATE)
    {
        LOGE("only rate %d supported", AUDIO_CHANNEL_DEFAULT_RATE);
        return -1;
    }

    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_CHANNEL_OUTPUT_BUFFER_SZ;
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_CHANNEL_OUT_STEREO;
}

static int out_get_format(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_CHANNEL_DEFAULT_FORMAT;
}

static int out_set_format(struct audio_stream *stream, int format)
{
    ADEV_LOG("out_set_format %x", format);
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    ADEV_TRACE();
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ADEV_TRACE();
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    int ret;
    struct str_parms *parms;

    ADEV_TRACE();

    parms = str_parms_create_str(kvpairs);

    /* fixme -- translate parameter settings to bluedroid */

    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ADEV_TRACE();
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    ADEV_TRACE();
    return 0;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    ADEV_TRACE();
    return 0;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct a2dp_stream_out *out = (struct a2dp_stream_out *)stream;
    int sent;

    ADEV_LOG("write %d bytes", bytes);

    /* connect socket if not available */
    if (out->sock_fd == SKT_DISCONNECTED)
    {
        out->sock_fd = skt_connect(A2DP_SOCK_PATH);
        if (out->sock_fd < 0)
            return -1;
    }

    sent = skt_write(out->sock_fd, buffer, bytes);

    ADEV_LOG("wrote %d bytes out of %d bytes", sent, bytes);
    return sent;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    ADEV_TRACE();
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ADEV_TRACE();
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ADEV_TRACE();
    return 0;
}

/*
 * AUDIO INPUT STREAM
 */

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return 8000;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ADEV_TRACE();
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return 320;
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_CHANNEL_IN_MONO;
}

static int in_get_format(const struct audio_stream *stream)
{
    ADEV_TRACE();
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, int format)
{
    ADEV_TRACE();
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    ADEV_TRACE();
    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ADEV_TRACE();
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    ADEV_TRACE();
    return 0;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    ADEV_TRACE();
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    ADEV_TRACE();
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    ADEV_TRACE();
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    ADEV_TRACE();
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ADEV_TRACE();
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ADEV_TRACE();

    return 0;
}


static int adev_open_output_stream(struct audio_hw_device *dev,
                                   uint32_t devices, int *format,
                                   uint32_t *channels, uint32_t *sample_rate,
                                   struct audio_stream_out **stream_out)
{
    struct a2dp_audio_device *a2dp_dev = (struct a2dp_audio_device *)dev;

    struct a2dp_stream_out *out;
    int ret;

    ADEV_TRACE();

    out = (struct a2dp_stream_out *)calloc(1, sizeof(struct a2dp_stream_out));
    if (!out)
        return -ENOMEM;

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;

    /* initialize a2dp specifics */
    a2dp_dev->output = (struct a2dp_stream_out *)&out->stream;
    a2dp_dev->output->sock_fd = SKT_DISCONNECTED;

    /* set output variables */
    if (format)
        *format = out_get_format((const struct audio_stream *)&out->stream);
    if (channels)
        *channels = out_get_channels((const struct audio_stream *)&out->stream);
    if (sample_rate)
        *sample_rate = out_get_sample_rate((const struct audio_stream *)&out->stream);

    *stream_out = &out->stream;

    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct a2dp_audio_device *a2dp_dev = (struct a2dp_audio_device *)dev;
    ADEV_TRACE();
    skt_disconnect(a2dp_dev->output->sock_fd);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct str_parms *parms;

    ADEV_TRACE();

    parms = str_parms_create_str(kvpairs);

    str_parms_dump(parms);

    ADEV_LOG("### not handled ###");
    
    return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    struct str_parms *parms;

    ADEV_TRACE();

    parms = str_parms_create_str(keys);

    str_parms_dump(parms);

    return NULL;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    struct a2dp_audio_device *a2dp_dev = (struct a2dp_audio_device*)dev;

    ADEV_TRACE();

    /* fixme -- setup control path for a2dp datapath */ 

    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ADEV_TRACE();

    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ADEV_TRACE();

    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, int mode)
{
    ADEV_TRACE();

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ADEV_TRACE();

    return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ADEV_TRACE();

    return -ENOSYS;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         uint32_t sample_rate, int format,
                                         int channel_count)
{
    ADEV_TRACE();

    return 320;
}

static int adev_open_input_stream(struct audio_hw_device *dev, uint32_t devices,
                                  int *format, uint32_t *channels,
                                  uint32_t *sample_rate,
                                  audio_in_acoustics_t acoustics,
                                  struct audio_stream_in **stream_in)
{
    struct a2dp_audio_device *ladev = (struct a2dp_audio_device *)dev;
    struct a2dp_stream_in *in;
    int ret;

    ADEV_TRACE();

    in = (struct a2dp_stream_in *)calloc(1, sizeof(struct a2dp_stream_in));

    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    *stream_in = &in->stream;
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *in)
{
    ADEV_TRACE();

    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ADEV_TRACE();

    return 0;
}

static int adev_close(hw_device_t *device)
{
    ADEV_TRACE();

    free(device);
    return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    ADEV_TRACE();

    return (AUDIO_DEVICE_OUT_ALL_A2DP);
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct a2dp_audio_device *adev;
    int ret;

    ADEV_TRACE();

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
    {
        ADEV_LOG("interface %s not matching [%s]", name, AUDIO_HARDWARE_INTERFACE);
        return -EINVAL;
    }

    adev = calloc(1, sizeof(struct a2dp_audio_device));
    if (!adev)
        return -ENOMEM;

    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = 0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.get_supported_devices = adev_get_supported_devices;
    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    *device = &adev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "A2DP Audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
