/*
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetoothTrack.h"
#include <media/AudioTrack.h>

//#define DUMP_PCM_DATA TRUE
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/data/misc/bluedroid/output_sample.pcm";
#endif

struct BluetoothTrack {
    android::AudioTrack* mTrack;
};

typedef struct BluetoothTrack BluetoothTrack;

BluetoothTrack *track = NULL;

int btCreateTrack(int trackFreq, int channelType)
{
    int ret = -1;
    track = new BluetoothTrack;
    track->mTrack = NULL;
    track->mTrack = new android::AudioTrack(AUDIO_STREAM_MUSIC, trackFreq, AUDIO_FORMAT_PCM_16_BIT,
            channelType, 0, (audio_output_flags_t)0, NULL, NULL, 0, 0, android::AudioTrack::TRANSFER_SYNC);
    if (track->mTrack == NULL) {
                delete track;
        track = NULL;
        return ret;
    }
    if (track->mTrack->initCheck() != 0) {
        delete track;
        track = NULL;
        return ret;
    }
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
    ret = 0;
    track->mTrack->setVolume(1, 1);
    track->mTrack->start();
    return ret;
}

void btStartTrack()
{
    if ((track) && (track->mTrack))
    {
        track->mTrack->start();
    }
}


void btDeleteTrack()
{
    if ((track) && (track->mTrack))
    {
        delete track;
    }
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    if (outputPcmSampleFile)
    {
        fclose(outputPcmSampleFile);
    }
    outputPcmSampleFile = NULL;
#endif
}

void btPauseTrack()
{
    if ((track) && (track->mTrack))
    {
        track->mTrack->pause();
    }
}

void btStopTrack()
{
    if ((track) && (track->mTrack))
    {
        track->mTrack->stop();
    }
}

int btWriteData(void *audioBuffer, int bufferlen)
{
    int retval = -1;
    if ((track) && (track->mTrack))
    {
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
        if (outputPcmSampleFile)
        {
            fwrite ((audioBuffer), 1, (size_t)bufferlen, outputPcmSampleFile);
        }
#endif
        retval = track->mTrack->write(audioBuffer, (size_t)bufferlen);
    }
    return retval;
}

