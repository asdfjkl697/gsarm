#ifndef RECORDER_H
#define RECORDER_H

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define SAMPLERATE    16000 //采样频率
#define SIZE    16   //量化位数
#define CHANNELS 1   //声道数目

class Recorder
{

public:
    explicit Recorder();
    ~Recorder();
    
    void initRecoder();
    int getSize(){return size;}
    int recode(char* &buffer,int bufsize); //执行录音操作：参数1:采集到的音频的存放数据，参数2：大小
private:
    int size;
    snd_pcm_t *handle;
    snd_pcm_uframes_t frames;
};

#endif // RECORDER_H
