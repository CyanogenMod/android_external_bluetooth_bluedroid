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
        retval = track->mTrack->write(audioBuffer, (size_t)bufferlen);
    }
    return retval;
}

