#ifndef VIDEO_H_
#define VIDEO_H_
/*
 * camera_yuv_x264.cpp
 */
#include <stdint.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <string.h>
#include <iostream>


#define CLEAR(x) (memset((&x),0,sizeof(x)))
#define ENCODER_PRESET "veryfast"
#if 0
#define IMAGE_WIDTH   320 //320
#define IMAGE_HEIGHT  240 //240
#define widthStep 1920 //960
#else
#define IMAGE_WIDTH   480
#define IMAGE_HEIGHT  360
#define widthStep 1920
#endif
//widthStep为图像宽度的3倍
/*配置参数
 * 使用默认参数，在这里使用了zerolatency的选项，使用这个选项之后，就不会有
 * delayed_frames，如果你使用不是这个的话，还需要在编码完成之后得到缓存的
 * 编码帧
 */
#define ENCODER_TUNE   "zerolatency"
#define ENCODER_PROFILE  "baseline"
#define ENCODER_COLORSPACE X264_CSP_I420

extern "C" {
//#include "include/x264.h"
#include <x264.h>
typedef struct my_x264_encoder {
	x264_param_t * x264_parameter;
	char parameter_preset[20];
	char parameter_tune[20];
	char parameter_profile[20];
	x264_t * x264_encoder;
	x264_picture_t * yuv420p_picture;
	long colorspace;
	unsigned char *yuv;
	x264_nal_t * nal;
} my_x264_encoder;
}

void Convert(unsigned char *RGB, unsigned char *YUV, unsigned int width,unsigned int height);
void getyuv(unsigned char *yuv);
void VideoRelease();
my_x264_encoder * VideoInit();

#endif
