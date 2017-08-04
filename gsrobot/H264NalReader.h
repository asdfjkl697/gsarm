#ifndef H264NALREADER_H_
#define H264NALREADER_H_

//#include <Windows.h>
#include <stdint.h>
#include "common.h"

DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit);
UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);
int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit); //jyc20170516 add
bool h264_decode_seq_parameter_set(BYTE * buf,UINT nLen,int &Width,int &Height);

#endif
