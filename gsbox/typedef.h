#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#define UDP_BUFFER_SIZE      1024 //1024


#define VERSION_HARDWARE "20131212"
#define VERSION_SOFT "20150119"


//#define OS_UBUNTU 1	   //jyc20170309 notice  linux openwrt swtich
//#define OS_OPENWRT 1
#define OS_ARMLINUX 1      //jyc20170505 add

#define USESERIAL 1  
#define SERIAL_BAND 115200 

#ifdef OS_UBUNTU
#define SERIAL_PORT 16 ///dev/ttyUSB0
#elif OS_ARMLINUX
#define SERIAL_PORT 2
#else
#define SERIAL_PORT 15 ///dev/ttyATH0
#endif

#define UDP_PORT 21680
#define SERVER_PORT 2088
#define DATA_SERVER_PORT 8089

#define DEVICE_NAME "gs_box"
#define DEVICE_ID "1001180615"

#endif
