#ifndef ISERIALPORTHANDLER_H_
#define ISERIALPORTHANDLER_H_

#include "CommLink.h"

#define defSerialBufPreLen 32

#define MIAN_PORT_Pkt_LEN	(255)
#define MIAN_PORT_UNPkt_LEN (1+1+1+1) // 不包含 包内容的长度，包头+确认位+包长度存储+结束确认位
#define MIAN_PORT_BUF_MAX (MIAN_PORT_Pkt_LEN+MIAN_PORT_UNPkt_LEN) // 包长度缓存大小上限  +包头+确认位+包长度存储+结束确认位

class ISerialPortHandler
{
public:
	virtual int SendData(CCommLinkRun *CommLink, unsigned char *buf,int size){ return 0; };
	virtual void OnDataReceived( CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize ){};
	virtual int RecvData(CCommLinkRun *CommLink, unsigned char *buf,int size){ return 0; };
};

#endif