#ifndef __VIDEOH264_H__
#define __VIDEOH264_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

void videoh264init();

#ifdef __cplusplus
};
#endif

#endif

