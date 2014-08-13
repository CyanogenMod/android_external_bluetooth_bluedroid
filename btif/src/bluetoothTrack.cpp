/*
* Copyright (c) 2013, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*        * Redistributions of source code must retain the above copyright
*            notice, this list of conditions and the following disclaimer.
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


#include "bluetoothTrack.h"
#include <media/AudioTrack.h>

//#define DUMP_PCM_DATA TRUE
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/data/misc/bluedroid/output_sample.pcm";
#endif

struct BluetoothTrack {
    android::sp<android::AudioTrack> mTrack;
};

typedef struct BluetoothTrack BluetoothTrack;

BluetoothTrack *track = NULL;

int btCreateTrack(int trackFreq, int channelType)
{
    int ret = -1;
    if (track == NULL)
        track = new BluetoothTrack;
    track->mTrack = NULL;
    track->mTrack = new android::AudioTrack(AUDIO_STREAM_MUSIC, trackFreq, AUDIO_FORMAT_PCM_16_BIT,
            channelType, (int)0, (audio_output_flags_t)AUDIO_OUTPUT_FLAG_FAST, NULL, NULL, 0, 0, android::AudioTrack::TRANSFER_SYNC);
    if (track->mTrack == NULL)
    {
        delete track;
        track = NULL;
        return ret;
    }
    if (track->mTrack->initCheck() != 0)
    {
        delete track;
        track = NULL;
        return ret;
    }
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
    ret = 0;
    track->mTrack->setVolume(1, 1);
    return ret;
}

void btStartTrack()
{
    if ((track != NULL) && (track->mTrack.get() != NULL))
    {
        track->mTrack->start();
    }
}


void btDeleteTrack()
{
    if ((track != NULL) && (track->mTrack.get() != NULL))
    {
        track->mTrack.clear();
        delete track;
        track = NULL;
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
    if ((track != NULL) && (track->mTrack.get() != NULL))
    {
        track->mTrack->pause();
        track->mTrack->flush();
    }
}

void btStopTrack()
{
    if ((track != NULL) && (track->mTrack.get() != NULL))
    {
        track->mTrack->stop();
    }
}

int btWriteData(void *audioBuffer, int bufferlen)
{
    int retval = -1;
    if ((track != NULL) && (track->mTrack.get() != NULL))
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

