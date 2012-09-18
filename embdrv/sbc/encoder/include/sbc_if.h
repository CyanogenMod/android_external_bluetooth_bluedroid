#ifndef _SBC_IF_H
#define _SBC_IF_H

#define PCM_BUFFER_SIZE 512

/*
 SBC_Init - called once for each track played

 pcm_sample_freq - 4000 to 48000
 channels - 1 mono 2 stereo
 bits_per_sample - 8 or 16
 return - 0 sucess
*/

int SBC_init(int pcm_sample_freq, int channels, int bits_per_sample);

/*
 SBC_write - called repeatedly with pcm_in pointer
	increasing by length until track is finished.

 pcm_in - pointer to PCM buffer
 length - any
 sbc_out - pointer to SBC output buffer
 return - number of bytes written to sbc_out
*/

int SBC_write(unsigned char *pcm_in, int length, unsigned char *sbc_out); 
#endif
