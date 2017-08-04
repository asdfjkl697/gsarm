#include "Video.h"
#include "common.h"  //jyc20170612 add
#include "recorder.h"
//#include "player.h"
#include <faac.h>

//转换矩阵
#define MY(a,b,c) (( a*  0.2989  + b*  0.5866  + c*  0.1145))
#define MU(a,b,c) (( a*(-0.1688) + b*(-0.3312) + c*  0.5000 + 128))
#define MV(a,b,c) (( a*  0.5000  + b*(-0.4184) + c*(-0.0816) + 128))
//#define MY(a,b,c) (( a*  0.257 + b*  0.504  + c*  0.098+16))
//#define MU(a,b,c) (( a*( -0.148) + b*(- 0.291) + c* 0.439 + 128))
//#define MV(a,b,c) (( a*  0.439  + b*(- 0.368) + c*( - 0.071) + 128))

//大小判断
#define DY(a,b,c) (MY(a,b,c) > 255 ? 255 : (MY(a,b,c) < 0 ? 0 : MY(a,b,c)))
#define DU(a,b,c) (MU(a,b,c) > 255 ? 255 : (MU(a,b,c) < 0 ? 0 : MU(a,b,c)))
#define DV(a,b,c) (MV(a,b,c) > 255 ? 255 : (MV(a,b,c) < 0 ? 0 : MV(a,b,c)))
#define CLIP(a) ((a) > 255 ? 255 : ((a) < 0 ? 0 : (a)))


//global variable
//char *write_filename = "test0513.264";
//int ii = 5; //frames_total，这里设置你需要录制多少帧
IplImage * img;
CvCapture * cap;
unsigned char * RGB1;
int videochanle = 0; //jyc20170525 视频通道


//RGB to YUV
void Convert(unsigned char *RGB, unsigned char *YUV, unsigned int width, unsigned int height)
{
    //变量声明
    unsigned int i, x, y, j;
    unsigned char *Y = NULL;
    unsigned char *U = NULL;
    unsigned char *V = NULL;

    Y = YUV;
    U = YUV + width*height;
    V = U + ((width*height) >> 2);
    for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
    {
        j = y*width + x;
        i = j * 3;
        Y[j] = (unsigned char)(DY(RGB[i], RGB[i + 1], RGB[i + 2]));
        if (x % 2 == 1 && y % 2 == 1)
        {
            j = (width >> 1) * (y >> 1) + (x >> 1);
            //上面i仍有效
            U[j] = (unsigned char)
                ((DU(RGB[i], RGB[i + 1], RGB[i + 2]) +
                DU(RGB[i - 3], RGB[i - 2], RGB[i - 1]) +
                DU(RGB[i - width * 3], RGB[i + 1 - width * 3], RGB[i + 2 - width * 3]) +
                DU(RGB[i - 3 - width * 3], RGB[i - 2 - width * 3], RGB[i - 1 - width * 3])) / 4);
            V[j] = (unsigned char)
                ((DV(RGB[i], RGB[i + 1], RGB[i + 2]) +
                DV(RGB[i - 3], RGB[i - 2], RGB[i - 1]) +
                DV(RGB[i - width * 3], RGB[i + 1 - width * 3], RGB[i + 2 - width * 3]) +
                DV(RGB[i - 3 - width * 3], RGB[i - 2 - width * 3], RGB[i - 1 - width * 3])) / 4);
        }
    }
}

void getyuv(unsigned char *yuv) {
	img = cvQueryFrame(cap);
	for (int i = 0; i < IMAGE_HEIGHT; i++) {
		for (int j = 0; j < IMAGE_WIDTH; j++) {
			RGB1[(i * IMAGE_WIDTH + j) * 3] = img->imageData[i * widthStep + j * 3 + 2];
			RGB1[(i * IMAGE_WIDTH + j) * 3 + 1] = img->imageData[i * widthStep + j * 3 + 1];
			RGB1[(i * IMAGE_WIDTH + j) * 3 + 2] = img->imageData[i * widthStep + j * 3];
		}
	}
	Convert(RGB1, yuv, IMAGE_WIDTH, IMAGE_HEIGHT);
}

void VideoRelease()
{
	free(RGB1);
	cvReleaseCapture(&cap); //useful
}

my_x264_encoder * VideoInit()
{
	int ret;
	my_x264_encoder * encoder = (my_x264_encoder *) malloc(sizeof(my_x264_encoder));
	if (!encoder) {
		printf("cannot malloc my_x264_encoder !\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*encoder);

	//摄像头初始化
	cap = cvCreateCameraCapture(0);
	//cap = cvCreateCameraCapture(1); //H264 编码
	cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, IMAGE_WIDTH);
	cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, IMAGE_HEIGHT);
	//分配RGB空间，用于提取IplImage中的image_data
	RGB1 = (unsigned char *) malloc(IMAGE_HEIGHT * IMAGE_WIDTH * 3);
	/****************************************************************************
	 * encoder结构体初始化
	 ****************************************************************************/
	strcpy(encoder->parameter_preset, ENCODER_PRESET);
	strcpy(encoder->parameter_tune, ENCODER_TUNE);
	encoder->x264_parameter = (x264_param_t *) malloc(sizeof(x264_param_t));
	if (!encoder->x264_parameter) {
		printf("malloc x264_parameter error!\n");
		exit(EXIT_FAILURE);
	}
	/*初始化编码器*/
	CLEAR(*(encoder->x264_parameter));
	x264_param_default(encoder->x264_parameter);

	if ((ret = x264_param_default_preset(encoder->x264_parameter,
			encoder->parameter_preset, encoder->parameter_tune)) < 0) {
		printf("x264_param_default_preset error!\n");
		exit(EXIT_FAILURE);
	}
	/*cpuFlags 去空缓冲区继续使用不死锁保证*/
	encoder->x264_parameter->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	/*视频选项*/
	encoder->x264_parameter->i_width = IMAGE_WIDTH; //要编码的图像的宽度
	encoder->x264_parameter->i_height = IMAGE_HEIGHT; //要编码的图像的高度
	encoder->x264_parameter->i_frame_total = 0; //要编码的总帧数，不知道用0
	encoder->x264_parameter->i_keyint_max = 10; //一般为i_fps_num的一倍或两倍，视频文件总时间=总帧数/该值，time = ii / i_keyint_max;
	/*流参数*/
	encoder->x264_parameter->i_bframe = 5;
	encoder->x264_parameter->b_open_gop = 0;
	encoder->x264_parameter->i_bframe_pyramid = 0;
	encoder->x264_parameter->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

	/*log参数，不需要打印编码信息时直接注释掉*/ //jyc20170518 remove
	//	encoder->x264_parameter->i_log_level = X264_LOG_DEBUG;
	//
	//	encoder->x264_parameter->i_fps_num = 10; //码率分子
	//	encoder->x264_parameter->i_fps_den = 1; //码率分母
	//
	//	encoder->x264_parameter->b_intra_refresh = 1;
	//	encoder->x264_parameter->b_annexb = 1;
	strcpy(encoder->parameter_profile, ENCODER_PROFILE);
	if ((ret = x264_param_apply_profile(encoder->x264_parameter,
			encoder->parameter_profile)) < 0) {
		printf("x264_param_apply_profile error!\n");
		exit(EXIT_FAILURE);
	}
	/*打开编码器*/
	encoder->x264_encoder = x264_encoder_open(encoder->x264_parameter);
	encoder->colorspace = ENCODER_COLORSPACE;
	/*初始化pic*/
	encoder->yuv420p_picture = (x264_picture_t *) malloc(
			sizeof(x264_picture_t));
	if (!encoder->yuv420p_picture) {
		printf("malloc encoder->yuv420p_picture error!\n");
		exit(EXIT_FAILURE);
	}
	if ((ret = x264_picture_alloc(encoder->yuv420p_picture, encoder->colorspace,
			IMAGE_WIDTH, IMAGE_HEIGHT)) < 0) {
		printf("ret=%d\n", ret);
		printf("x264_picture_alloc error!\n");
		exit(EXIT_FAILURE);
	}
	encoder->yuv420p_picture->img.i_csp = encoder->colorspace;
	encoder->yuv420p_picture->img.i_plane = 3;
	encoder->yuv420p_picture->i_type = X264_TYPE_AUTO;
	/*申请YUV buffer*/
	encoder->yuv = (uint8_t *) malloc(IMAGE_WIDTH * IMAGE_HEIGHT * 3 / 2);
	if (!encoder->yuv) {
		printf("malloc yuv error!\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*(encoder->yuv));
	encoder->yuv420p_picture->img.plane[0] = encoder->yuv;
	encoder->yuv420p_picture->img.plane[1] = encoder->yuv
			+ IMAGE_WIDTH * IMAGE_HEIGHT;
	encoder->yuv420p_picture->img.plane[2] = encoder->yuv
			+ IMAGE_WIDTH * IMAGE_HEIGHT + IMAGE_WIDTH * IMAGE_HEIGHT / 4;

	encoder->nal = (x264_nal_t *) calloc(2, sizeof(x264_nal_t)); //一般一帧分为两个nal，不行的话就改3-8
	if (!encoder->nal) {
		printf("malloc x264_nal_t error!\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*(encoder->nal));
	return encoder;
}

void FaacCapInit(){

}

int faacap()
{
//    ULONG nSampleRate = 44100;  // 采样率
//    UINT nChannels = 1;         // 声道数
//    UINT nPCMBitSize = 16;      // 单样本位数
//    ULONG nInputSamples = 0;
//    ULONG nMaxOutputBytes = 0;

	DWORD nSampleRate = 44100;  // 采样率
	UINT nChannels = 1;         // 声道数
	UINT nPCMBitSize = 16;      // 单样本位数
	DWORD nInputSamples = 0;
	DWORD nMaxOutputBytes = 0;

    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration;

    int nBytesRead;
    int nPCMBufferSize;
    BYTE* pbPCMBuffer;
    BYTE* pbAACBuffer;

    FILE* fpOut; // AAC file for output
    fpOut = fopen("out.aac", "wb");
    FILE* fpPcm = fopen("out.pcm", "wb");

    // (1) Open FAAC engine
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if(hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }

    nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    pbPCMBuffer = new BYTE [nPCMBufferSize];
    pbAACBuffer = new BYTE [nMaxOutputBytes];

    // (2.1) Get current encoding configuration
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;

    // (2.2) Set encoding configuration
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);

    Recorder recorder;
    recorder.initRecoder();

    char *buffer = (char*)malloc(2048*2*2*2);

    int audio_samples_per_frame = 32;
    int audio_sample_rate = nSampleRate;
    int audio_pts_increment = (90000 * audio_samples_per_frame) / audio_sample_rate; //一秒钟采集多少次
    audio_pts_increment /= 2; //单声道除以2

    int i =0;
    for(;i<15*audio_pts_increment;i++) //采集15秒
    {
        fprintf(stderr,"%d\n",audio_pts_increment);
        int size;
        for (size = 0; ;)
        {
            char *buf = (char*)(buffer+size);
            int readSize = recorder.recode(buf,32);
            size += readSize *2;
            if (size >= 2048) break;
        }
        nBytesRead = size;
        // 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
        nInputSamples = nBytesRead / (nPCMBitSize / 8);
        fprintf(stderr,"nInputSamples %d\n",nInputSamples);
        // (3) Encode
        nRet = faacEncEncode(hEncoder,(int32_t*) buffer,  //jyc20170612 int -> int32_t
				nInputSamples,pbAACBuffer,nMaxOutputBytes);

        fwrite(pbAACBuffer, 1, nRet, fpOut);
        fwrite(buffer, 1, size, fpPcm);
        fprintf(stderr,"%d: faacEncEncode returns %d\n", i, nRet);
    }
    // (4) Close FAAC engine
    nRet = faacEncClose(hEncoder);
    delete[] pbAACBuffer;
    fclose(fpOut);
    free(buffer);
    return 0;
}






