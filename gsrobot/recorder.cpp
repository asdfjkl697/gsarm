#include "recorder.h"

Recorder::Recorder()
{
    size = 0;
}

Recorder::~Recorder()
{

}

void Recorder::closeRecoder()
{
    snd_pcm_close(handle);
}

void Recorder::initRecoder()
{
    int rc;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr,"unable to open pcm device: %s\n",snd_strerror(rc));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params,
                          SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                                  SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);

    /* 44100 bits/second sampling rate (CD quality) */
    val = SampleRate;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                      &val, &dir);

    /* Set period size to 32 frames. */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle,
                                  params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
        exit(1);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,&frames, &dir);
    size = frames * 2; /* 2 bytes/sample, 2 channels */

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,&val, &dir);
    //loops = 5000000 / val;  //获取5秒中所对应的loop这里用不到

}

int Recorder::recode(char* &buffer,int bufsize)
{
    int rc;
    rc = snd_pcm_readi(handle, buffer, bufsize);
    if (rc == -EPIPE)
    {
      /* EPIPE means overrun */
        fprintf(stderr, "overrun occurred\n");snd_pcm_prepare(handle);
    }
    else if (rc < 0)
    {
        buffer = NULL;
        bufsize = 0;
        fprintf(stderr,"error from read: %s\n",snd_strerror(rc));
    }
    else if (rc != bufsize)
    {

        fprintf(stderr, "short read, read %d frames\n", rc);
    }

    return rc;
}


