#include "player.h"
#include "recorder.h"

Player::Player()
{

}

Player::~Player()
{

}

void Player::initPlayer()
{
    int rc;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir=0;
    snd_pcm_uframes_t periodsize;

    /* Open PCM device for playback. */
    rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_PLAYBACK, 0);//SND_PCM_NONBLOCK);
    if (rc < 0)
    {
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
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);  //jyc20170531 2->1

    /* 44100 bits/second sampling rate (CD quality) */
    val = SAMPLERATE;
printf("DEBUG val=%d dir=%d.......\n",val,dir);
    snd_pcm_hw_params_set_rate_near(handle, params,&val,&dir);
printf("DEBUG val=%d dir=%d.......\n",val,dir);
    //val = SAMPLERATE;

    /* Set period size to 32 frames. */
    frames = 32;
    periodsize = frames * 2;
    rc = snd_pcm_hw_params_set_buffer_size_near(handle, params, &periodsize);
    if (rc < 0)
    {
         printf("Unable to set buffer size %li : %s\n", frames * 2, snd_strerror(rc));

    }
printf("DEBUG frames=%d dir=%d.......\n",frames,dir);
    periodsize /= 2;
    rc = snd_pcm_hw_params_set_period_size_near(handle, params, &periodsize, 0);
    if (rc < 0)
    {
        printf("Unable to set period size %li : %s\n", periodsize,  snd_strerror(rc));
    }

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
printf("DEBUG periodsize=%d frames=%d.......\n",periodsize,frames);
    size = frames * 2; /* 2 bytes/sample, 2 channels */
}

void Player::play(char *buffer,int size)
{
    int rc;
    while(rc = snd_pcm_writei(handle, buffer, size)<0)
    {
        usleep(2000);
        if (rc == -EPIPE)
        {
              fprintf(stderr, "underrun occurred\n");
              snd_pcm_prepare(handle);
        }
        else if (rc < 0)
        {
              fprintf(stderr,"error from writei: %s\n",snd_strerror(rc));
        }
    }
}
