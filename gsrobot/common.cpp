/*
#include "common.h"
#include <iostream>
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <Windows.h>
#include <algorithm>
#include <cctype>
#include "RunCode.h"
#include "jid.h"
#include <regex>  // regular expression ������ʽ
#include "md5.h"
#include <tchar.h>

#include "logFileExFunc.cpp"
#include "BufferMerge.cpp"
*/

#include "common.h"
#include <stdio.h>                                                                                             
#include <stdlib.h> 
#include <time.h>

#include <algorithm>
#include <cctype>
#include "RunCode.h"
#include "gloox/jid.h"
//#include <regex>  // regular expression ������ʽ
#include "gloox/md5.h"

uint32_t Reversebytes_uint32(unsigned int value){
	uint32_t v = value;
	return (v&0x000000ff)<<24 | (v&0x0000ff00)<<8 |
		(v&0x00ff0000)>>8 | (v&0xff000000)>>24;
}

uint16_t Reversebytes_uint16(unsigned int value){
	unsigned short vv = value;
	uint16_t vvv = (uint16_t)(vv&0xff)<<8|vv>>8;
	return vvv;
}

void GetLocalTime(SYSTEMTIME *st)
{
    if(st)
    {
        struct tm *pst = NULL;
        time_t t = time(NULL);
        pst = localtime(&t);
        memcpy(st,pst,sizeof(SYSTEMTIME));
        st->wYear += 1900;
    }
}

unsigned int timeGetTime()  
{  
	unsigned int uptime = 0;  
        struct timespec on;  
        if(clock_gettime(CLOCK_MONOTONIC, &on) == 0)  
	        uptime = on.tv_sec*1000 + on.tv_nsec/1000000;  
	return uptime;  
} 

unsigned long GetTickCount()
{
    	struct timespec ts;
    	clock_gettime(CLOCK_MONOTONIC, &ts);
    	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

using namespace gloox;

IGSClientExFunc *g_pRefGSIOTClient = NULL;

void g_SYS_SetGSIOTClient( IGSClientExFunc *pGSIOTClient )
{
	if( !g_pRefGSIOTClient )
	{
		g_pRefGSIOTClient = pGSIOTClient;
	}
}

IGSClientExFunc* g_SYS_GetGSIOTClient()
{
	return g_pRefGSIOTClient;
}

//crc
//High-Order Byte Table
/* Table of CRC values for high��Corder byte */
const unsigned auchCRCHi[256] = {
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
} ;


//Low-Order Byte Table
/* Table of CRC values for low��Corder byte */
const unsigned auchCRCLo[256] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
};

std::string g_GetConnectStateStr( defConnectState state )
{
	switch( state )
	{
	case defConnectState_Null:
		return std::string("");

	case defConnectState_Connecting:
		return std::string("������");

	case defConnectState_Connected:
		return std::string("������");

	case defConnectState_Disconnected:
		return std::string("���ӶϿ�");
	}

	return std::string("����");
}

std::string g_getstr_Useable( const defUseable Useable )
{
	switch(Useable)
	{
	case defUseable_OK:
		return std::string( "��" );
		break;

	case defUseable_ErrConfig:
		return std::string("���ô���");
		break;

	case defUseable_ErrNoData:
		return std::string("���쳣�����");
		break;

	case defUseable_ErrNet:
		return std::string("�����ж�");//return std::string("���쳣�����ж�");
		break;

	default:
		if( Useable < 0 ) return std::string("���쳣");
		break;
	}

	return std::string("");
}

std::string g_CreateChangedForTS( bool base64 )
{
	char buf[32] ={0};

	uint32_t data[2] ={0};
	data[0] = (int)GetTickCount();
	data[1] = time( NULL );

	if( base64 )
	{
		g_base64_encode( buf, sizeof( buf ), (uint8_t*)&data, sizeof( data ) );
	}
	else
	{
		return g_BufferToString( ((uint8_t*)&data)+2, sizeof(data)-2, false );
		//snprintf( buf, sizeof( buf ), "%04X%08X", (int)GetTickCount(), (int)t );
	}

	return buf;
}

void g_Changed( const defCfgOprt_ oprt, const IOTDeviceType DevType, const uint32_t DevID, const uint32_t addr )
{
	switch( DevType )
	{
	case IOT_Obj_CommLink:
		return;

	case IOT_Obj_User:
		{
			//if( defCfgOprt_Add==oprt )
			//	return;
		}
		break;
	}

	//��ʱ���д��

	if( !IsRUNCODEEnable(defCodeIndex_Dis_ChangeSaveDB) )
	{
		static uint32_t s_lastChanged = GetTickCount();

		if( GetTickCount()-s_lastChanged > 60*1000 )
		{
			if( g_pRefGSIOTClient )
			{
				g_pRefGSIOTClient->get_RunCodeMgr().SetCodeAndSaveDb( defCodeIndex_SYS_Change_Global, RUNCODE_Get(defCodeIndex_SYS_Change_Global)+1, GetTickCount(), 0,0, true,true );
				s_lastChanged = GetTickCount();
			}

			return;
		}
	}

	RUNCODE_Set( defCodeIndex_SYS_Change_Global, RUNCODE_Get(defCodeIndex_SYS_Change_Global)+1, GetTickCount(), 0,0, true,true );
}

/*jyc20160826 
int sys_reset( const char* strdesc, int bFlag )
{
	//LOGMSGEX( defLOGWatch, defLOG_SYS, "ϵͳ������reason=%s\r\n", strdesc?strdesc:"" );
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "sys_reset. reason=%s\r\n", strdesc?strdesc:"" );
	usleep( 700000 );

#if defined(_DEBUG)
	LOGMSG( "debug... no sys_reset" );
	return 1;
#endif

	LPTSTR s;
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(),   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "(%s)get error  when call OpenProcessToken\r\n", strdesc?strdesc:"" );
		return 0;	
	} 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,   &tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,   (PTOKEN_PRIVILEGES)NULL, 0); 

	if (GetLastError() != ERROR_SUCCESS)
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "(%s)get error  when call AdjustTokenPrivileges\r\n", strdesc?strdesc:"" );
		return 0;	

	}
	if(!ExitWindowsEx(EWX_FORCE|(bFlag?EWX_REBOOT:EWX_POWEROFF),0)){
		DWORD err=GetLastError();
		if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, err, 0, (LPTSTR)&s, 0, NULL) == 0)
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "(%s)get error %d when call ExitWindowsEx\r\n", strdesc?strdesc:"", LOWORD(err) );
		} 
		else
		{ //success 

			LPTSTR p = _tcschr(s, _T('\r'));
			if(p != NULL)
			{
				*p = _T('\0');
			}
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "(%s)get error %d:%s when call ExitWindowsEx\r\n", strdesc?strdesc:"", LOWORD(err), (char*)p );
		} 
		return 0;	
	}
	return 1;
}
*/

// �������Ƴ�С����������ֵ
std::string g_StrToPrecision( const std::string &srcstr, const int precision )
{
	if( precision < 0 )
		return srcstr;

	const size_t pos = srcstr.find_first_of( "." );

	if( pos == std::string::npos )
		return srcstr;

	const size_t getpos = pos + precision + (precision>0?1:0); // ����"."

	if( getpos >= srcstr.size() )
		return srcstr;

	return srcstr.substr( 0, getpos );
}

int split(const std::string& str, std::vector<std::string>& ret_, std::string sep){    
	if (str.empty()){        
		return 0;    
	}

	std::string tmp;    
	std::string::size_type pos_begin = str.find_first_not_of(sep);    
	std::string::size_type comma_pos = 0;    
	while (pos_begin != std::string::npos)    
	{
		comma_pos = str.find(sep, pos_begin);        
		if (comma_pos != std::string::npos)   
		{            
			tmp = str.substr(pos_begin, comma_pos - pos_begin);            
			pos_begin = comma_pos + sep.length();        
		}        
		else        
		{            
			tmp = str.substr(pos_begin);            
			pos_begin = comma_pos;
		}        
		if (!tmp.empty())        
		{
			ret_.push_back(tmp);            
			tmp.clear();        
		}    
	}    

	return ret_.size();
}

int split_getmapval( const std::string& str, std::map<std::string,std::string>& mapval, const std::string getspec, const std::string sepmain, const std::string sepsub )
{
	if (str.empty())
	{
		return 0;
	}

	std::vector<std::string> vec;
	split( str, vec, sepmain );

	for( int i=0; i<vec.size(); ++i )
	{
		if( vec[i].empty() )
			continue;

		std::string strsub = vec[i];
		strsub.erase( 0, vec[i].find_first_not_of( " " ) );

		if( strsub.empty() )
			continue;

		std::vector<std::string> subvec;
		split( strsub, subvec, sepsub );

		std::string strkey;
		std::string strval;

		if( subvec.empty() )
		{
			continue;
		}
		else if( 1==subvec.size() )
		{
			strkey = subvec[0];
		}
		else if( subvec.size()>1 )
		{
			strkey = subvec[0];
			strval = subvec[1];
		}

		if( getspec.empty() )
		{
			mapval[strkey] = strval;
		}
		else
		{
			if( getspec == strkey )
			{
				mapval[strkey] = strval;
				return mapval.size();
			}
		}
	}

	return mapval.size();
}

std::string g_lstval2str( std::vector<std::pair<std::string, std::string> >& lstval )
{
	std::string str;
	for( std::vector<std::pair<std::string,std::string> >::const_iterator it=lstval.begin(); it!=lstval.end(); ++it )
	{
		if( !str.empty() )
		{
			str += ",";
		}

		str += it->first;
		str += "=";
		str += it->second;
	}

	return str;
}

bool operator< ( const GSIOTAddrObjKey &key1, const GSIOTAddrObjKey &key2 )
{
	macOperator_Less( key1, key2, dev_type );
	macOperator_Less( key1, key2, dev_id );
	macOperator_Less( key1, key2, address_type );

	return (key1.address_id < key2.address_id);
}

bool operator< ( const struGSTime &key1, const struGSTime &key2 )
{
	macOperator_Less( key1, key2, Year );
	macOperator_Less( key1, key2, Month );
	macOperator_Less( key1, key2, Day );

	macOperator_Less( key1, key2, Hour );
	macOperator_Less( key1, key2, Minute );
	macOperator_Less( key1, key2, Second );

	return (key1.Milliseconds < key2.Milliseconds);
}

// ��ص����͵ļ���ֵ��غ�����������Ǽ�������ټ���ֵ
bool g_AddrTypeValue_hasTypeValue( const IOTDeviceType type )
{
	switch( type )
	{
	case IOT_DEVICE_Wind:
		return true;

	default:
		break;
	}

	return false;
}

int g_AddrTypeValue_getTypeValue( const IOTDeviceType type, const int v1k, const bool ValueForType )
{
	if( !ValueForType || !g_AddrTypeValue_hasTypeValue(type) )
		return v1k;

	switch( type )
	{
	case IOT_DEVICE_Wind:
		return g_WindSpeedLevel( ((float)v1k)/1000, false )*1000;

	default:
		break;
	}

	return v1k;
}

int g_WindSpeedLevel( const float WindSpeed, const bool trymerge )
{
	if( WindSpeed < 0 )
	{
		return -1;	// �޷� //(δʶ��Ҳ��ʾ�޷�)
	}
	else if( WindSpeed < 0.3 )	// 0-0.2x
	{
		return 0;	// �޷�
	}
	else if( WindSpeed < 1.6 )	// 0.3-1.5x
	{
		if( trymerge && IsRUNCODEEnable(defCodeIndex_SYS_MergeWindLevel) )
		{
			return 1;
		}

		return 1;	// ���
	}
	else if( WindSpeed < 3.4 )	// 1.6-3.3x
	{
		if( trymerge && IsRUNCODEEnable(defCodeIndex_SYS_MergeWindLevel) )
		{
			return 1;
		}

		return 2;	// ���
	}
	else if( WindSpeed < 5.5 )	// 3.4-5.4x
	{
		return 3;	// ΢��
	}
	else if( WindSpeed < 8.0 )	// 5.5-7.9x
	{
		return 4;	// �ͷ�
	}
	else if( WindSpeed < 10.8 )	// 8.0-10.7x
	{
		return 5;	// �ᾢ��
	}
	else if( WindSpeed < 13.9 )	// 10.8-13.8x
	{
		return 6;	// ǿ��
	}
	else if( WindSpeed < 17.2 )	// 13.9-17.1x
	{
		return 7;	// ����
	}
	else if( WindSpeed < 20.8 )	// 17.2-20.7x
	{
		return 8;	// ���
	}
	else if( WindSpeed < 24.5 )	// 20.8-24.4x
	{
		return 9;	// �ҷ�
	}
	else if( WindSpeed < 28.5 )	// 24.5-28.4x
	{
		return 10;	// ���
	}
	else if( WindSpeed < 32.7 )	// 28.5-32.6x
	{
		return 11;	// ����
	}
	else if( WindSpeed < 37.0 )	// 32.7-36.9x
	{
		return 12;	// ̨��
	}
	else if( WindSpeed < 41.5 )	// 37.0-41.4x
	{
		return 13;	// ̨��
	}
	else if( WindSpeed < 46.2 )	// 41.5-46.1x
	{
		return 14;	// ̨��
	}
	else if( WindSpeed < 51.0 )	// 46.2-50.9x
	{
		return 15;	// ̨��
	}
	else if( WindSpeed < 56.1 )	// 51.0-56.0x
	{
		return 16;	// ̨��
	}
	else if( WindSpeed < 61.3 )	// 56.1-61.2x
	{
		return 17;	// ̨��
	}
	else
	{
		return -2;	// �޷� //(δʶ��Ҳ��ʾ�޷�)
	}
}

uint32_t g_GetComPortWriteTime( const uint32_t len )
{
	const uint32_t COM_baudrate = RUNCODE_Get(defCodeIndex_SYS_COM_baudrate);
	
	return (uint32_t)(((float)len*12/COM_baudrate)*1000+100);
	//return (len*12/COM_baudrate+1)*1000;
}

std::string ASCIIToUTF8(const std::string& str)
{
	/*
	if( str.empty() )
		return std::string("");
	
	const char *ascii_str = str.c_str();
	int len = MultiByteToWideChar(CP_ACP, 0,ascii_str, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len+1];
	memset(wstr, 0, len+1);
	MultiByteToWideChar(CP_ACP, 0, ascii_str, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char *utf8_char = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8_char, len, NULL, NULL);
	if(wstr) delete[] wstr;

	std::string outstr = utf8_char;
	delete []utf8_char;
	return outstr;
	*/

	return str;
}

std::string UTF8ToASCII(const std::string& str)
{
	/*
	if( str.empty() )
		return std::string("");

	const char *utf8_str = str.c_str();
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len+1];
	memset(wstr, 0, len+1);
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char *ascii_char = new char[len];
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, ascii_char, len, NULL, NULL);
	if(wstr) delete[] wstr;
	std::string outstr = ascii_char;
	delete []ascii_char;
	return outstr;
	*/
	return str;
}

int getMacAddress(std::string& mac){
	/*
	PIP_ADAPTER_INFO pIpAdapterInfo =new IP_ADAPTER_INFO();
    unsigned long stSize=sizeof(IP_ADAPTER_INFO);
    int nRel= GetAdaptersInfo(pIpAdapterInfo,&stSize);
    if (ERROR_BUFFER_OVERFLOW==nRel)
    {
		delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
        nRel=GetAdaptersInfo(pIpAdapterInfo,&stSize);   
    }
    if (ERROR_SUCCESS==nRel)
    {
		while (pIpAdapterInfo)
		{
			for (UINT i=0; i< pIpAdapterInfo->AddressLength; i++){
				char temp[10];
				snprintf(temp,"%02x", pIpAdapterInfo->Address[i]);
				mac.append(temp);
			}
			break;
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
    }
    if (pIpAdapterInfo)
    {
        delete pIpAdapterInfo;
    }*/
	return 0;
}


std::string getAppPath()
{
	CHAR sPath[MAX_PATH]; 
    //GetModuleFileNameA(NULL, sPath, MAX_PATH); //jyc20160922
	char *strPath = sPath;
	std::string retPath = strPath;
    return retPath.substr(0,retPath.find_last_of("\\"));
}

bool file_exists(const char *filename)
{
	//return ( INVALID_FILE_ATTRIBUTES != GetFileAttributesA(filename) );
}

std::string getRandomCode()
{
#define n 10
	char list[n+1];
	srand((uint32_t)time(NULL));
    for(int i=0;i<n;i++)
       list[i]=rand()%26+97;
	list[n] = '\0';
	return std::string(list);
}

int ByteToInt32(uint8_t *buf)
{
	return (buf[0] & 0xFF) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

uint16_t ByteToInt16(uint8_t *buf)
{
	return (buf[0] & 0xFF) | (buf[1] << 8);
}

float ByteToFloat(uint8_t *buf)
{
	float val = 0;
	uint8_t *data = (uint8_t *)&val;
	data[0] = buf[0];
	data[1] = buf[1];
	data[2] = buf[2];
	data[3] = buf[3];
	return val;
}

double ByteToDouble(uint8_t *buf)
{
	double val = 0;
	uint8_t *data = (uint8_t *)&val;
	data[0] = buf[0];
	data[1] = buf[1];
	data[2] = buf[2];
	data[3] = buf[3];
	data[4] = buf[4];
	data[5] = buf[5];
	data[6] = buf[6];
	data[7] = buf[7];
	return val;
}

std::string ByteToString(uint8_t *buf,uint32_t len)
{
	std::string str;
	char *strValue = new char[len+1];
    memcpy(strValue,buf,len);
	strValue[len] = '\0';
    str.append(strValue);
	return str;
}

uint8_t *Int16ToByte(uint8_t *buf,uint16_t val)
{
	buf[0] = val & 0xFF;
	buf[1] = val >> 8;
	return buf+2;
}
uint8_t *Int32ToByte(uint8_t *buf,uint32_t val)
{
	buf[0] = val & 0xFF;
	buf[1] = val >> 8;
	buf[2] = val >> 16;
	buf[3] = val >> 24;
	return buf+4;
}


uint8_t *LongToByte(uint8_t *buf,uint64_t val)
{
	uint8_t *data = (uint8_t *)&val;
    buf[0] = data[0];
	buf[1] = data[1];
	buf[2] = data[2];
	buf[3] = data[3];
	buf[4] = data[4];
	buf[5] = data[5];
	buf[6] = data[6];
	buf[7] = data[7];
	return buf+8;
}


uint8_t *DoubleToByte(uint8_t *buf,double val)
{
    uint8_t *data = (uint8_t *)&val;
    buf[0] = data[0];
	buf[1] = data[1];
	buf[2] = data[2];
	buf[3] = data[3];
	buf[4] = data[4];
	buf[5] = data[5];
	buf[6] = data[6];
	buf[7] = data[7];
	return buf+8;
}

uint8_t *FloatToByte(uint8_t *buf,float val)
{
	uint8_t *data = (uint8_t *)&val;
	buf[0] = data[0];
	buf[1] = data[1];
	buf[2] = data[2];
	buf[3] = data[3];
	return buf+4;
}

// �ȸߺ��
uint16_t Big_ByteToInt16(uint8_t *buf)
{
	return ((uint16_t)buf[0] << 8) | (uint16_t)(buf[1] & 0xFF);
}

uint8_t *Big_Int16ToByte(uint8_t *buf,uint16_t val)
{
	buf[0] = val >> 8;
	buf[1] = val & 0xFF;
	return buf+2;
}

//CRC-ITU
uint16_t crc16_verify( uint8_t *buf, uint16_t len )
{
	unsigned char uchCRCHi = 0xFF;

	unsigned char uchCRCLo = 0xFF;

	uint16_t uIndex;

	while (len--)
	{
		uIndex = uchCRCLo ^ *buf++ ;
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
		uchCRCHi = auchCRCLo[uIndex] ; 
	} 
#ifndef OS_OPENWRT //jyc20170505 modify  ARMLINUX UBUNTU SMALLEND   MIPS BIGEND
	return ( (uchCRCHi << 8)| uchCRCLo );
#else
	return ( (uchCRCLo << 8)| uchCRCHi );
#endif
}

uint32_t g_StringToBuffer( const std::string &str, uint8_t *buf, uint32_t len, bool hasspace )
{
	if( !hasspace )
	{
		const char *pc = str.c_str();
		const char *const pc_end = pc+str.length();

		char msg[16] ={0};
		uint8_t count = 0;

		while( pc<pc_end )
		{
			memcpy( msg, pc, 2 );
			msg[2] = 0;
			uint16_t val1 = (uint16_t)strtoul( msg, NULL, 16 );
			pc += 2;

			buf[count] = val1;

			++count;
			if( count>=len )
			{
				break;
			}
		}

		return count;
	}

	const uint32_t strmax = (str.length()+1)/3;
	uint32_t i = 0;

	for( ; i<strmax && i<len; ++i )
	{
		buf[i] = strtoul( str.substr(3*i,2).c_str(), NULL, 16 );
	}

	return i;
}

std::string g_BufferToString( unsigned char *buf, int len, bool addspace, bool bluppercase )
{
	std::string str;

	if( len > 2048 )
		return str;

	char msg[64] ={0};

	if( len > 0 )
	{
		str.reserve( len*3 );
		for( int i = 0; i<len; ++i )
		{
			snprintf( msg, sizeof(msg), bluppercase?"%s%02X":"%s%02x", 0==i?"":(addspace?" ":""), buf[i] );
			str.append( msg );
		}
	}

	return str;
}

#define PrintBufferLength 2048
#define defHexByteMaxOnLine 50
void g_PrintfByte( unsigned char *buf,int len, const char *pinfo, defLinkID LinkID, const char *pLinkName )
{
	BYTE* tt = buf;

	char msg[PrintBufferLength] = {0};
	if( LinkID<0 )
	{
		snprintf( msg, PrintBufferLength, "%s(len=%d) : \n", pinfo?pinfo:"", (int)len );
	}
	else
	{
		snprintf( msg, PrintBufferLength, "Link%d(%s) %s(len=%d) : -ThId%d\n", LinkID, pLinkName?pLinkName:"", pinfo?pinfo:"", (int)len, ::pthread_self() );
	}

	if (len > 0) 
	{
		std::string str;
		str.reserve(PrintBufferLength);
		str.append(msg);
		for(int i = 0;i<len;++i,++tt)
		{
			snprintf(msg,500, " %02X",*tt);
			str.append(msg);
			if( (i+1)<len && (i+1)%defHexByteMaxOnLine==0 )
			{
				str.append(";\r\n");
			}
		}
		str.append(";;;\r\n");
		printf( str.c_str() );
	}
}

int g_GetH264PreFix( unsigned char *buf,int len, int &prefixNum, int Max )
{
	unsigned char *tempbuf = buf;
	const int templen = len>Max ? Max:len;
	const unsigned char *tempbufEnd = buf+templen;

	prefixNum = 0;
	while( tempbufEnd-tempbuf>3 )
	{
		if( 0x00 == tempbuf[0] && 0x00 == tempbuf[1] )
		{
			if( 0x00 == tempbuf[2] )
			{
				if( 0x01 == tempbuf[3] )
				{
					prefixNum = 4;
					tempbuf += prefixNum;
					return tempbuf-buf;
				}
			}
			else if( 0x01 == tempbuf[2] )
			{
				prefixNum = 3;
				tempbuf += prefixNum;
				return tempbuf-buf;
			}
		}

		tempbuf++;
	}

	return 0;
}

unsigned char* g_Get_x264_nal_t( unsigned char *buf, int len, x264_nal_s &nal )
{
	if( !buf || len<=2 )
		return NULL;

	g_PrintfByte( buf, len>64?64:len, "g_Get_x264_nal_t" );

	unsigned char *buffer = buf;
	int size = len;

	int prefixNum = 0;
	int preindex = g_GetH264PreFix( (unsigned char*)buffer, size, prefixNum );

	buffer += preindex;
	size -= preindex;

	if( preindex<=0 )
	{
		return NULL;
	}

	if( 0x67==buffer[0]
	|| 0x68==buffer[0]
	)
	{
		int prefixNum2 = 0;
		int preindex2 = g_GetH264PreFix( (unsigned char*)buffer, size, prefixNum2 );

		int nalsize = 0;
		if( preindex2>0 )
		{
			nalsize = preindex2-prefixNum2;
		}
		else
		{
			nalsize = size;
		}

		if( nalsize>0 )
		{
			//nal.p_payload = (unsigned char *)malloc(nalsize);
			nal.p_payload = new unsigned char[nalsize];
			memcpy( nal.p_payload, buffer, nalsize );
			nal.i_payload = nalsize;

			return (unsigned char*)buffer+nalsize;
		}
	}
	
	return NULL;
}

void Copy_x264_nal_t( x264_nal_s &dest, const x264_nal_s &src )
{
	memcpy( &dest, &src, sizeof(x264_nal_s) );

	if( src.p_payload && src.i_payload>0 )
	{
		//dest.p_payload = (unsigned char *)malloc(src.i_payload);
		dest.p_payload = new unsigned char[src.i_payload];
		memcpy( dest.p_payload, src.p_payload, src.i_payload );
		dest.i_payload = src.i_payload;
	}
	else
	{
		dest.i_payload = 0;
		dest.p_payload = NULL;
	}
}

void Copy_x264_nal_t( x264_nal_s **pdest, int &destsize, const x264_nal_s *psrc, const int srcsize )
{
	Delete_x264_nal_t( pdest, destsize );

	if( !psrc || srcsize<=0 )
	{
		*pdest = NULL;
		destsize = 0;
		return;
	}

	destsize = srcsize;
	*pdest = new x264_nal_s[destsize];

	for( int i=0; i<srcsize; ++i )
	{
		Copy_x264_nal_t( (*pdest)[i], psrc[i] );
	}
}

void Delete_x264_nal_t( x264_nal_s &src )
{
	macCheckAndDel_Array(src.p_payload);
	src.i_payload = 0;
}

void Delete_x264_nal_t( x264_nal_s **psrc, int &srcsize )
{
	if( !psrc || !(*psrc) || srcsize<=0 )
		return;

	for( int i=0; i<srcsize; ++i )
	{
		Delete_x264_nal_t( (*psrc)[i] );
	}

	macCheckAndDel_Array(*psrc);
	srcsize = 0;
}

uint32_t g_h264_find_next_start_code_ex( const uint8_t *pBuf, uint32_t bufLen, uint32_t &prefix )
{
  uint32_t val, temp;
  uint32_t offset;

  prefix = 0;
  offset = 0;
  if (pBuf[0] == 0 && 
      pBuf[1] == 0 && 
      ((pBuf[2] == 1) ||
       ((pBuf[2] == 0) && pBuf[3] == 1))) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    temp = val & 0xff000000;
    val &= 0x00ffffff;
    val |= *pBuf++;
    offset++;
    if (val == 0x000001) { // H264_START_CODE
      if (temp == 0) {
		  prefix = 4;
		  return offset - 4;
	  }
	  prefix = 3;
      return offset - 3;
    }
  }
  return 0;
}

// ���ж����ʱ���Ƴ��м������ͷ
uint32_t g_h264_remove_all_start_code( uint8_t *pBuf_src, uint32_t bufLen_src )
{
	uint8_t *pBuf = pBuf_src;
	uint32_t bufLen = bufLen_src;
	uint32_t prefix = 0;
	uint32_t offset = 0;
	while( pBuf + 3 < pBuf_src + bufLen_src )
	{
		offset = g_h264_find_next_start_code_ex( pBuf, bufLen, prefix );
		if( offset > 0 )
		{
			memcpy( pBuf + offset, pBuf + offset + prefix, bufLen - offset - prefix );
		}
		else
		{
			break;
		}

		pBuf += offset;
		bufLen -= (offset+prefix);
		bufLen_src -= prefix;
	}

	return bufLen_src; // return new size
}

void g_toLowerCase( std::string &str )
{
    transform( str.begin(), str.end(), str.begin(), tolower );
}

void g_toUpperCase( std::string &str )
{
	transform( str.begin(), str.end(), str.begin(), toupper );
}

void g_GetLocalIP( std::list<std::string> &LocalIPList, std::list<std::string> &OtherIP )
{
	char szHostName[128];
	if (gethostname(szHostName, 128) == 0)
	{
		/* jyc20160922
		hostent * ent = gethostbyname(szHostName); 
		int i = 0;
		for (; ent!= NULL && ent->h_addr_list[i]!= NULL; i++ )  
		{
			char* lpAddr = inet_ntoa(*(in_addr *)ent->h_addr_list[i]);
			if( lpAddr )
			{
				if( 192 == ((in_addr *)ent->h_addr_list[i])->S_un.S_un_b.s_b1
					&& 168 == ((in_addr *)ent->h_addr_list[i])->S_un.S_un_b.s_b2 )
				{
					LocalIPList.push_back( std::string(lpAddr) );
				}
				else
				{
					OtherIP.push_back( std::string(lpAddr) );
				}
			}
		}*/
	}
}

std::string g_ConvertCameraTypeToString( IPCameraType type )
{
	switch(type)
	{
	case TI368:
		return std::string("TI");

	case SSD1935:
		return std::string("SSD");

	case CameraType_hik:
		return std::string("Hik");

	case CameraType_dh:
		return std::string("DH");
	}

	return std::string("Unknown");
}

bool IsDisableModule( const uint8_t ModuleIndex, bool isRecv )
{
	switch(ModuleIndex)
	{
	case RXB8_315:
	case RXB8_433:
		{
			if( isRecv && IsRUNCODEEnable(defCodeIndex_Disable_Recv_RF) )
			{
				return true;
			}
		}
		break;

	case RXB8_315_TX:
	case RXB8_433_TX:
		{
			if( !isRecv && IsRUNCODEEnable(defCodeIndex_Disable_Send_RF_TX) )
			{
				return true;
			}
		}
		break;

	case Module_RS485:
		{
			return isRecv ? IsRUNCODEEnable(defCodeIndex_Disable_Recv_RS485) : IsRUNCODEEnable(defCodeIndex_Disable_Send_RS485);
		}
		break;

	case RXB8_315_original:
	case RXB8_433_original:
		{
			return isRecv ? IsRUNCODEEnable(defCodeIndex_Disable_Recv_RF_original) : IsRUNCODEEnable(defCodeIndex_Disable_Send_RF_original);
		}
		break;
	}

	return false;
}

bool g_CheckEMail( const std::string &email_address )
{
	std::string user_name, domain_name;

	if( email_address.empty() )
		return false;
/*	jyc20170301 remove because no <regex>
	std::regex pattern("([0-9A-Za-z\\-_\\.]+)@([0-9a-z]+\\.[a-z]{2,3}(\\.[a-z]{2})?)");

	if ( regex_match( email_address, pattern ) )
	{

		user_name = regex_replace( email_address, pattern, std::string("$1") );

		domain_name = regex_replace( email_address, pattern, std::string("$2") );

		return true;
	}
*/
	return false;
}

std::string g_Trans_GSReturn( defGSReturn ret )
{
	switch( ret )
	{
	case defGSReturn_Success:
		return std::string(defGSReturnStr_Succeed);

	case defGSReturn_Null:
		return std::string("null");

	case defGSReturn_Err:
		return std::string(defGSReturnStr_Fail);

	case defGSReturn_NoExist:
		return std::string("NoExist");

	case defGSReturn_IsExist:
		return std::string("IsExist");

	case defGSReturn_IsLock:
		return std::string("IsLock");

	case defGSReturn_ResLimit:
		return std::string("ResLimit");

	case defGSReturn_FunDisable:
		return std::string("FunDisable");

	case defGSReturn_IsSelf:
		return std::string("IsSelf");

	case defGSReturn_ObjDisable:
		return std::string("ObjDisable");

	case defGSReturn_ConnectObjErr:
		return std::string("ConnectObjErr");

	case defGSReturn_ConnectSvrErr:
		return std::string("ConnectSvrErr");

	case defGSReturn_CreateObjErr:
		return std::string("CreateObjErr");

	case defGSReturn_NoData:
		return std::string("NoData");

	case defGSReturn_NotFoundContent:
		return std::string("NotFoundContent");

	case defGSReturn_TimeOut:
		return std::string("TimeOut");

	case defGSReturn_UnSupport:
		return std::string("UnSupport");

	case defGSReturn_ErrUser:
		return std::string("ErrUser");

	case defGSReturn_ErrPassword:
		return std::string("ErrPassword");

	case defGSReturn_ErrParam:
		return std::string("ErrParam");

	case defGSReturn_ErrConfig:
		return std::string("ErrConfig");

	case defGSReturn_NameEmpty:
		return std::string("NameEmpty");

	case defGSReturn_ObjEditDisable:
		return std::string("ObjEditDisable");

	case defGSReturn_SameName:
		return std::string("SameName");

	case defGSReturn_SaveFailed:
		return std::string("SaveFailed");

	case defGSReturn_DBNoExist:
		return std::string("DBNoExist");

	case defGSReturn_DBNoRec:
		return std::string("DBNoRec");

	case defGSReturn_OverTimeRange:
		return std::string("OverTimeRange");
	}

	std::string strret("unknown");

	if( macGSSucceeded(ret) )
	{
		strret += defGSReturnStr_Succeed;
	}
	else if( macGSFailed(ret) )
	{
		strret += defGSReturnStr_Fail;
	}
	
	return strret;
}

time_t g_GetUTCTime()
{
#if 1
	//return time(NULL); //jyc20170224 modify linux time
	// now time
	SYSTEMTIME st;
	memset( &st, 0, sizeof(st) );
	//::GetLocalTime(&st);
	GetLocalTime(&st);
	tm temptm = {st.wSecond,st.wMinute,st.wHour,st.wDay,st.wMonth,st.wYear-1900,st.wDayOfWeek,0,0};
	return mktime(&temptm);
#else
	time_t t;
	struct tm *p;
	time(&t);
	p=localtime(&t);

	return mktime(p);
#endif
}

bool g_UTCTime_To_struGSTime( const time_t utctime, struGSTime &dest )
{
	memset( &dest, 0, sizeof(struGSTime) );

	struct tm p;
	
	//if( 0 != localtime_s( &p, &utctime ) ) //jyc20160922
	/*if( 0 != localtime_r( &utctime, &p ) )
	{
		memset( &dest, 0, sizeof(struGSTime) );
		return false;
	}*/

	localtime_r( &utctime, &p ); //jyc20170227 modify
	//g_tm_To_struGSTime( p, dest );

	dest.Year = p.tm_year+1900;
	dest.Month = (unsigned char)p.tm_mon+1;
	dest.Day = (unsigned char)p.tm_mday;
	dest.Hour = (unsigned char)p.tm_hour;
	dest.Minute = (unsigned char)p.tm_min;
	dest.Second = (unsigned char)p.tm_sec;

	return true;
}

time_t g_struGSTime_To_UTCTime( const struGSTime &src )
{
	tm tmval;
	g_struGSTime_To_tm( src, tmval );

	const bool isHour24 = ( src.Hour > 23 );
	if( isHour24 )
	{
		tmval.tm_hour = 0;
	}

	time_t utctime = mktime(&tmval);

	if( isHour24 )
	{
		utctime += 24*60*60;
	}

	return utctime;
}

void g_tm_To_struGSTime( const tm &src, struGSTime &dest )
{
	dest.Year = src.tm_year+1900;
	dest.Month = (unsigned char)src.tm_mon+1;
	dest.Day = (unsigned char)src.tm_mday;
	dest.Hour = (unsigned char)src.tm_hour;
	dest.Minute = (unsigned char)src.tm_min;
	dest.Second = (unsigned char)src.tm_sec;
}

void g_struGSTime_To_tm( const struGSTime &src, tm &dest )
{
	dest.tm_year = src.Year-1900;
	dest.tm_mon = (unsigned char)src.Month-1;
	dest.tm_mday = (unsigned char)src.Day;
	dest.tm_hour = (unsigned char)src.Hour;
	dest.tm_min = (unsigned char)src.Minute;
	dest.tm_sec = (unsigned char)src.Second;
}

void g_struGSTime_GetCurTime( struGSTime &dt )
{
	memset( &dt, 0, sizeof(struGSTime) );
	
	SYSTEMTIME st;
	memset( &st, 0, sizeof(st) );
	::GetLocalTime(&st);

	dt.Year = st.wYear;
	dt.Month = (unsigned char)st.wMonth;
	dt.Day = (unsigned char)st.wDay;
	dt.Hour = (unsigned char)st.wHour;
	dt.Minute = (unsigned char)st.wMinute;
	dt.Second = (unsigned char)st.wSecond;
}

void g_reget_struGSTime_GetCurTime( struGSTime &dt, uint32_t &last_gettime, const uint32_t cur_time )
{
	if( cur_time-last_gettime > 5000 )
	{
		g_struGSTime_GetCurTime( dt );
		last_gettime = cur_time;
	}
}

const unsigned char g_GetMaxDay( const unsigned short Year, const unsigned char Month )
{
	struGSTime dtbase;
	dtbase.Year = Year;
	dtbase.Month = Month;
	dtbase.Day = 1;
	dtbase.Hour = 12;
	
	if( dtbase.Month<12 )
	{
		dtbase.Month += 1;
	}
	else
	{
		dtbase.Year += 1;
		dtbase.Month = 1;
	}

	time_t utctime = g_struGSTime_To_UTCTime( dtbase );
	utctime -= 24*60*60;
	g_UTCTime_To_struGSTime( utctime, dtbase );

	return dtbase.Day;
}

std::string g_TimeToStr( const time_t utctime, const defTimeToStrFmt_ fmt )
{
	if( defTimeToStrFmt_UTC == fmt )
	{
		char chtime[32] ={0};

		snprintf( chtime, sizeof( chtime ), "%d", utctime );

		return std::string( chtime );
	}

	struGSTime dt;
	g_UTCTime_To_struGSTime( utctime, dt );
	return g_TimeToStr( dt, fmt );
}

std::string g_TimeToStr( const struGSTime &dt, const defTimeToStrFmt_ fmt )
{
	char chtime[32] = {0};

	switch( fmt )
	{
	case defTimeToStrFmt_YearMonthDayHourMin:
		{
			snprintf( chtime, sizeof(chtime), "%04d-%02d-%02d %02d:%02d", dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute );
		}
		break;

	case defTimeToStrFmt_YearMonthDay:
		{
			snprintf( chtime, sizeof(chtime), "%04d-%02d-%02d", dt.Year, dt.Month, dt.Day );
		}
		break;

	case defTimeToStrFmt_YearMonth:
		{
			snprintf( chtime, sizeof(chtime), "%04d-%02d", dt.Year, dt.Month );
		}
		break;
		
	case defTimeToStrFmt_MonthDay:
		{
			snprintf( chtime, sizeof(chtime), "%02d-%02d", dt.Month, dt.Day );
		}
		break;

	case defTimeToStrFmt_HourMinSec:
		{
			snprintf( chtime, sizeof(chtime), "%02d:%02d:%02d", dt.Hour, dt.Minute, dt.Second );
		}
		break;

	case defTimeToStrFmt_HourMin:
		{
			snprintf( chtime, sizeof(chtime), "%02d:%02d", dt.Hour, dt.Minute );
		}
		break;

	case defTimeToStrFmt_Full7:
		{
			snprintf( chtime, sizeof(chtime), "%04d-%02d-%02d %02d:%02d:%02d.%03d", dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, dt.Milliseconds );
		}
		break;

	default:
		{
			snprintf( chtime, sizeof(chtime), "%04d-%02d-%02d %02d:%02d:%02d", dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );
		}
		break;
	}

	return std::string(chtime);
}

std::string g_SecondToStr( const uint32_t second )
{
	char chtime[32] ={0};

	const uint32_t s = second % 60;
	const uint32_t m = second / 60;

	//const uint32_t m = m_a % 60;
	//const uint32_t h = m_a / 60;

	std::string str;
	//if( h>0 )
	//{
	//	snprintf( chtime, sizeof( chtime ), "%dʱ", h );
	//	str += chtime;
	//}

	if( m>0 )
	{
		snprintf( chtime, sizeof( chtime ), "%d��", m );
		str += chtime;
	}

	snprintf( chtime, sizeof( chtime ), "%d��", s );
	str += chtime;

	return str;
}

// A��B�Ƿ���ͬһ�£�����UTCʱ��
bool g_IsSameMonth( const time_t utctimeA, const time_t utctimeB )
{
	struGSTime dtA;
	struGSTime dtB;

	if( !g_UTCTime_To_struGSTime( utctimeA, dtA ) )
		return false;

	if( !g_UTCTime_To_struGSTime( utctimeB, dtB ) )
		return false;

	return g_IsSameMonth( dtA, dtB );
}

// A��B�Ƿ���ͬһ�죬����UTCʱ��
bool g_IsSameDay( const time_t utctimeA, const time_t utctimeB )
{
	struGSTime dtA;
	struGSTime dtB;

	if( !g_UTCTime_To_struGSTime( utctimeA, dtA ) )
		return false;

	if( !g_UTCTime_To_struGSTime( utctimeB, dtB ) )
		return false;

	return g_IsSameDay( dtA, dtB );
}

// A��B�Ƿ���ͬһ�£����뱾��ʱ��
bool g_IsSameMonth( const struGSTime &dtA, const struGSTime &dtB )
{
	if( dtA.Month != dtB.Month )
		return false;

	if( dtA.Year != dtB.Year )
		return false;

	return true;
}

// A��B�Ƿ���ͬһ�죬���뱾��ʱ��
bool g_IsSameDay( const struGSTime &dtA, const struGSTime &dtB )
{
	if( dtA.Day != dtB.Day )
		return false;

	return g_IsSameMonth( dtA, dtB );
}

// A��B�Ƿ���ͬһСʱ�����뱾��ʱ��
bool g_IsSameHour( const struGSTime &dtA, const struGSTime &dtB )
{
	if( dtA.Hour != dtB.Hour )
		return false;

	return g_IsSameDay( dtA, dtB );
}

// A<B return <0
// A=B return =0
// A>B return >0
int g_CompareDay( const struGSTime &dtA, const struGSTime &dtB )
{
	if( dtA.Year != dtB.Year )
		return ( (int)dtA.Year - (int)dtB.Year );

	if( dtA.Month != dtB.Month )
		return ( (int)dtA.Month - (int)dtB.Month );

	return ( (int)dtA.Day - (int)dtB.Day );
}

// A<B return <0
// A=B return =0
// A>B return >0
int g_CompareMonth( const struGSTime &dtA, const struGSTime &dtB )
{
	if( dtA.Year != dtB.Year )
		return ( (int)dtA.Year - (int)dtB.Year );

	return ( (int)dtA.Month - (int)dtB.Month );
}

// �Ƿ�Ϊ��ǰ��
bool g_IsNowDay( const time_t utctime )
{
	struGSTime dt;
	g_UTCTime_To_struGSTime( utctime, dt );
	return g_IsNowDay( dt );
}

bool g_IsNowDay( const struGSTime &dt )
{
	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );

	return g_IsSameDay( curdt, dt );
}

// �Ƿ�Ϊ��ǰ��
bool g_IsNowMonth( const time_t utctime )
{
	struGSTime dt;
	g_UTCTime_To_struGSTime( utctime, dt );
	return g_IsNowMonth( dt );
}

bool g_IsNowMonth( const struGSTime &dt )
{
	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );

	return g_IsSameMonth( curdt, dt );
}

bool md5_CheckFile( const std::string& filename, std::string& digest )
{
	digest = "null!";
	FILE *fd = NULL;
	//fopen( &fd, filename.c_str(), "rb+" );

	if( !fd )
	{
		digest = "open file err!";
		return false;
	}

	unsigned char buf[1024];
	size_t ret=0;

	MD5 md5;
	while ((ret = fread(buf, 1, sizeof(buf), fd)) > 0)
		md5.feed(buf, ret);

	if (ferror(fd))
	{
		digest = "file read err!";
		fclose(fd);
		return false;
	}

	md5.finalize();
	digest = md5.hex();
	//digest = md5.binary();

	fclose(fd);
	return true;
}

#define WAVE_HEADER_SIZE 44
int write_wav_head( /*WAVEFORMATEX*/void *wf, char *in_buffer, int in_len, char *out_buffer, int *out_len )
{
	char *buffer = out_buffer;
	int *int_tmp,pos=0;

	*out_len = WAVE_HEADER_SIZE + in_len;
	//buffer = (char*)malloc(*out_len);
	//if(buffer == NULL) 
	//	return -1;
	memcpy(buffer,"RIFF",4);
	pos = 4;

	int_tmp = (int*)(buffer+pos);
	*int_tmp = WAVE_HEADER_SIZE + in_len - 8;/*���ο�Ĵ�С��������С��8��*/
	pos += 4;

	memcpy(buffer+pos,"WAVEfmt ",8);
	pos += 8;

	int_tmp = (int*)(buffer+pos);
	*int_tmp = 16;
	pos += 4;

	/*��ʽ��Ϣ*/
	memcpy(buffer+pos,wf,16);
	pos += 16;

	memcpy(buffer+pos,"data",4);
	pos += 4;

	int_tmp = (int*)(buffer+pos);
	*int_tmp = in_len;
	pos += 4;

	memcpy(buffer+pos,in_buffer,in_len);

	//*out_buffer = buffer;

	return 0;
}

// return src str
std::string& g_replace_all_distinct( std::string& str, const std::string& old_value, const std::string& new_value )
{
    for( std::string::size_type pos(0); pos!=std::string::npos; pos+=new_value.length() )
	{
        if( (pos=str.find(old_value,pos))!=std::string::npos )
			str.replace( pos, old_value.length(), new_value );
        else
			break;
    }

    return   str;
}

std::string get_GSPlayBackCode_Name( GSPlayBackCode_ ControlCode )
{
	switch( ControlCode )
	{
	case GSPlayBackCode_NULL:
		return std::string("NULL");

	case GSPlayBackCode_PLAYPAUSE:
		return std::string("PlayPause");

	case GSPlayBackCode_PLAYRESTART:
		return std::string("PlayRestart");

	case GSPlayBackCode_PLAYNORMAL:
		return std::string("PlayNormal");

	case GSPlayBackCode_PLAYFAST:
		return std::string("PlayFast");

	case GSPlayBackCode_PLAYSLOW:
		return std::string("PlaySlow");

	case GSPlayBackCode_PLAYGETPOS:
		return std::string("PlayGetPos");

	case GSPlayBackCode_PLAYSETPOS:
		return std::string("PlaySetPos");

	case GSPlayBackCode_SETSPEED:
		return std::string("SetSpeed");

	case GSPlayBackCode_PlaySetTime:
		return std::string("PlaySetTime");

	case GSPlayBackCode_SkipTime:
		return std::string("SkipTime");
	}

	return std::string("(unknown)");
}

defCodeIndex_ g_AlarmGuardTimeWNum2Index( int w )
{
	switch(w)
	{
	case 1:
		return defCodeIndex_SYS_AlarmGuardTimeW1;

	case 2:
		return defCodeIndex_SYS_AlarmGuardTimeW2;

	case 3:
		return defCodeIndex_SYS_AlarmGuardTimeW3;

	case 4:
		return defCodeIndex_SYS_AlarmGuardTimeW4;

	case 5:
		return defCodeIndex_SYS_AlarmGuardTimeW5;

	case 6:
		return defCodeIndex_SYS_AlarmGuardTimeW6;

	case 7:
		return defCodeIndex_SYS_AlarmGuardTimeW7;
	}

	return defCodeIndex_Unknown;
}

void g_GetAlarmGuardTime( defCodeIndex_ dayOfWeek, std::vector<uint32_t> &vecFlag, std::vector<uint32_t> &vecBegin, std::vector<uint32_t> &vecEnd )
{
	if( !vecFlag.empty() )
	{
		vecFlag.clear();
	}

	if( !vecBegin.empty() )
	{
		vecBegin.clear();
	}

	if( !vecEnd.empty() )
	{
		vecEnd.clear();
	}

	switch(dayOfWeek)
	{
	case defCodeIndex_SYS_AlarmGuardTimeW1:
	case defCodeIndex_SYS_AlarmGuardTimeW2:
	case defCodeIndex_SYS_AlarmGuardTimeW3:
	case defCodeIndex_SYS_AlarmGuardTimeW4:
	case defCodeIndex_SYS_AlarmGuardTimeW5:
	case defCodeIndex_SYS_AlarmGuardTimeW6:
	case defCodeIndex_SYS_AlarmGuardTimeW7:
		break;

	default:
		dayOfWeek = defCodeIndex_Unknown;
	}
	
	// ���ò���ʱ�书�ܣ�����������Ч
	if( defCodeIndex_Unknown==dayOfWeek || IsRUNCODEEnable(defCodeIndex_TEST_DisableAlarmGuard) )
	{
		for( int i=0; i<defAlarmGuard_AGTimeCount; ++i )
		{
			vecFlag.push_back( 0 );
			vecBegin.push_back( 0 );
			vecEnd.push_back( 0 );
		}

		return ;
	}

	const struRunCode &rcode = RUNCODE_Ref( dayOfWeek );

	const int agTimeArray[defAlarmGuard_AGTimeCount] = 
	{
		rcode.value2,
		rcode.value3,
		rcode.value4
	};
	
	// ������Чʱ������
	for( int i=0; i<defAlarmGuard_AGTimeCount; ++i )
	{
		const uint32_t agTime_Flag = agTimeArray[i]/100000000;
		const uint32_t agTime = agTimeArray[i]%100000000;
		const uint32_t agTime_Begin = agTime/10000;
		const uint32_t agTime_End = agTime%10000;

		if( agTime_Begin==agTime_End )
		{
			vecFlag.push_back( 0 );
			vecBegin.push_back( 0 );
			vecEnd.push_back( 0 );
		}
		else
		{
			vecFlag.push_back( agTime_Flag );
			vecBegin.push_back( agTime_Begin );
			vecEnd.push_back( agTime_End );
		}
	}
}

std::string g_Trans_GSAGCurState( GSAGCurState_ state )
{
	switch(state)
	{
	case GSAGCurState_UnArmed:
		return std::string("UnArmed");

	case GSAGCurState_AllArmed:
		return std::string("AllArmed");

	case GSAGCurState_PartOfArmed:
		return std::string("PartOfArmed");

	case GSAGCurState_WaitTimeArmed:
		return std::string("WaitTimeArmed");
	}

	return std::string("Unknown");
}

std::string g_parseAddrForHttp( const std::string &url )
{
	if( url.empty() )
	{
		return std::string("");
	}

	char buf[256] = {0};
	strcpy( buf, url.c_str() );

	char *p = strstr(buf, "://");
	if( !p )
	{
		return std::string("");
	}

	p+=3;

	char *slash = strchr(p, '/');
	if( slash )
	{
		*slash = 0;
	}

	return std::string(p);
}

bool g_isNeedSaveType( const IOTDeviceType type )
{
	switch( type )
	{
	case IOT_DEVICE_CO2:
	case IOT_DEVICE_HCHO:
	case IOT_DEVICE_PM25:
	case IOT_DEVICE_Wind:
	case IOT_DEVICE_Temperature:
	case IOT_DEVICE_Humidity:
		return true;

	default:
		break;
	}

	return false;
}

// isPrevSecond and else afterSecond
time_t g_GetTimePointSecond( const IOTDeviceType type, const bool isPrevSecond )
{
	if( isPrevSecond )
	{
		return (60);
	}
	else
	{
		switch( type )
		{
		case IOT_DEVICE_Wind:
			return (5*60);

		case IOT_DEVICE_CO2:
		case IOT_DEVICE_HCHO:
		//case IOT_DEVICE_PM25:
		case IOT_DEVICE_Temperature:
		case IOT_DEVICE_Humidity:
		default:
			break;
		}
	}

	return (3*60);
}

// ��Ϊ�ڴ洢ʱ��㣨��Χ��
bool g_isTimePoint( const time_t utctime, const IOTDeviceType type )
{
	const time_t TimePoint = ( utctime / 1800 ) * 1800;
	const time_t PrevSecond = g_GetTimePointSecond( type, true );
	const time_t AfterSecond = g_GetTimePointSecond( type, false );  //jyc20170228 modify

	return ( utctime > (TimePoint-PrevSecond) ||  utctime < (TimePoint+AfterSecond) );
}

// ת�������ڴ洢ʱ��� g_isTimePoint=true
time_t g_TransToTimePoint( const time_t utctime, const IOTDeviceType type, const bool isSave )
{
	const time_t TimePoint = ( utctime / 1800 ) * 1800;
	
	// ��ǰʱ��ӽ�����һ���洢ʱ���
	if( isSave )
	{
		const time_t NextTimePoint = TimePoint + 1800;

		//if( utctime > (NextTimePoint-g_GetTimePointSecond(type, true)) ) //jyc20170228 have problem
		if( utctime > (NextTimePoint-60) )
		{
			struct tm loT;
			//localtime_s( &loT, &TimePoint );
			localtime_r( &TimePoint, &loT ); //jyc20170227modify

			struct tm loTNext;
			//localtime_s( &loTNext, &NextTimePoint );
			localtime_r( &NextTimePoint, &loTNext ); //jyc20170227modify

			if( loT.tm_mday == loTNext.tm_mday )
			{
				// �Ƿ�����ͬ��(����ʱ��)
				return NextTimePoint;
			}
		}
	}

#ifdef _DEBUG
	assert(g_isTimePoint(TimePoint,IOT_DEVICE_Unknown));
#endif

	return TimePoint;
}

float g_SYS_VChgRng( const IOTDeviceType type )
{
	float rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_Default);
	switch( type )
	{
	case IOT_DEVICE_Wind:
		break;

	case IOT_DEVICE_CO2:
		{
			rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_CO2);
		}
		break;
	case IOT_DEVICE_HCHO:
		{
			rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_HCHO);
		}
		break;
	case IOT_DEVICE_PM25:
		{
			//rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_PM25);
		}
		break;

	case IOT_DEVICE_Temperature:
		{
			rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_Temperature);
		}
		break;

	case IOT_DEVICE_Humidity:
		{
			rng = (float)RUNCODE_Get(defCodeIndex_SYS_VChgRng_Humidity);
		}
		break;

	default:
		break;
	}

	return ( rng / 1000.0f ); // 1000������
}

// ֵ�Ļ�λ
std::string g_GetUnitBaseForType( const IOTDeviceType type )
{
	switch( type )
	{
	case IOT_DEVICE_CO2:
		return "ppm";
	case IOT_DEVICE_HCHO:
		return "mg/m3";
	case IOT_DEVICE_PM25:
		return "ug/m3";
			
	case IOT_DEVICE_Temperature:
		return "℃"; //return "";

	case IOT_DEVICE_Humidity:
		return "%RH";

	case IOT_DEVICE_Wind:
		return "m/s";

	default:
		break;
	}

	return c_NullStr;
}

// �����ͼ���ֵ��ʾ�ǵĵ�λ
std::string g_GetUnitUseValueForType( const IOTDeviceType type, const bool ValueForType )
{
	if( !ValueForType )
		return c_NullStr;

	switch( type )
	{
	case IOT_DEVICE_Wind:
		return "��";

	default:
		break;
	}

	return c_NullStr;
}

int g_GetPrecisionForType(const IOTDeviceType type) {
	switch (type) {
	case IOT_DEVICE_HCHO:
		return 3;
	default:
		break;
	}
	return 1;
}

std::string g_V1kToStr( const int v1k, const std::string &strunit, const int precision )
{
	char buf[256] = {0};
	const float f = ((float)v1k)/1000;

	snprintf( buf, sizeof( buf ), "%g", f );

	return ( g_StrToPrecision(std::string(buf), precision) + strunit );
}

std::string g_V1kToStrUseValueForType( const IOTDeviceType type, const int v1k, const bool ValueForType, const bool showunit, const int precision )
{
	int use_precision = precision;
	if (0 == precision) // auto
	{
		use_precision = g_GetPrecisionForType(type); //jyc20170422 add by jianguang
	}

	return g_V1kToStr(g_AddrTypeValue_getTypeValue(type, v1k, ValueForType),
			showunit ? g_GetUnitUseValueForType(type, ValueForType) : c_NullStr,
			use_precision);
}

// �ж�һ��url�Ƿ�ΪRTMFP���͵�url
bool g_IsRTMFP_url( const std::string &strurl )
{
	if( strurl.size() < 5 )
		return false;

	std::string strhead = strurl.substr( 0, 5 );
	g_toLowerCase( strhead );

	return ( strhead == "rtmfp" );
}

BOOL g_DeleteFileEx( const char *filename )
{
	/*jyc20160604
	DWORD attributes = GetFileAttributesA( filename );
	if( INVALID_FILE_ATTRIBUTES != attributes )
	{
		if( attributes & FILE_ATTRIBUTE_READONLY )
		{
			attributes &=~ FILE_ATTRIBUTE_READONLY;
			SetFileAttributesA( filename, attributes );
		}
	}

	return DeleteFileA( filename );*/
}

// �ļ���С
int g_FileSize( const std::string &filename ) //jyc20160922
{
	FILE *fd = NULL;
	//fopen( &fd, filename.c_str(), "rb+" );

	if( !fd )
	{
		return 0;
	}

	fseek( fd, 0, SEEK_END );
	const int nFileLen = ftell( fd );
	//fseek( fd, 0, SEEK_SET );

	if( nFileLen < 1 )
	{
		fclose( fd );
		return 0;
	}
	
	fclose( fd );
	return nFileLen;
}

// д�ַ���Ϣ���ļ�
bool g_SaveStringToFile( const std::string &pathname, const std::string &strsave )
{
	if( strsave.length() > 2*1024*1024 )
	{
		return false;
	}

	FILE *fd = NULL;
	//fopen( &fd, pathname.c_str(), "wb+" );

	if( !fd )
	{
		return false;
	}

	fwrite( strsave.c_str(), 1, strsave.length(), fd );
	fclose( fd );

	return true;
}

// д���ݵ��ļ�
// toTail: ĩβ
bool g_SaveBytesToFile( const std::string &pathname, const BYTE *pdata, const uint32_t len, const bool toTail )
{
	if( len > 5*1024*1024 )
	{
		return false;
	}

	if( !file_exists( pathname.c_str() ) )
	{
		return false;
	}

	FILE *fd = NULL;
	//fopen( &fd, pathname.c_str(), "wb+" );

	if( !fd )
	{
		return false;
	}

	if( toTail ) 
	{
		fseek( fd, 0, SEEK_END );
	}

	fwrite( pdata, 1, len, fd );
	fclose( fd );

	return true;
}

// �����洢Ŀ¼
std::string g_iotstore_createdir( const std::string &strsub )
{
	/*
	WIN32_FIND_DATAA ffd;
	if( INVALID_HANDLE_VALUE == FindFirstFileA( (std::string( defIotStoreDir_Main )+"\\*").c_str(), &ffd ) )
	{
		CreateDirectoryA( defIotStoreDir_Main, NULL );
	}

	const std::string strsub_full = std::string( defIotStoreDir_Main ) + "\\" + strsub;

	if( INVALID_HANDLE_VALUE == FindFirstFileA( (strsub_full+"\\*").c_str(), &ffd ) )
	{
		CreateDirectoryA( strsub_full.c_str(), NULL );
	}

	return strsub_full;*/
}

int g_base64_decode( uint8_t *out, const char *in_str, int out_size )
{
static const uint8_t map2[256] =
	{
		0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,

		0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
		0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
		0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
		0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
		0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
		0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,

		0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};

#define g_BASE64_DEC_STEP(i) do { \
    bits = map2[in[i]]; \
    if (bits & 0x80) \
        goto out ## i; \
    v = i ? (v << 6) + bits : bits; \
	} while(0)
/*
#   define g_WB32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
		    } while(0)
*/
#   define g_WL32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
		    } while(0)

//#   define AV_WN(s, p, v) ((((union unaligned_##s *) (p))->l) = (v))
//#   define g_WN32(p, v) AV_WN(32, p, v)

#define g_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define g_BSWAP32C(x) (g_BSWAP16C(x) << 16 | g_BSWAP16C((x) >> 16))

	uint8_t *dst = out;
	uint8_t *end = out + out_size;
	// no sign extension
	const uint8_t *in = (uint8_t*)in_str;
	unsigned bits = 0xff;
	unsigned v;

	while( end - dst > 3 ) {
		g_BASE64_DEC_STEP( 0 );
		g_BASE64_DEC_STEP( 1 );
		g_BASE64_DEC_STEP( 2 );
		g_BASE64_DEC_STEP( 3 );
		// Using AV_WB32 directly confuses compiler
		//v = _byteswap_ulong( v << 8 );//v = av_be2ne32( v << 8 );
		v = g_BSWAP32C( v << 8 );
		g_WL32( dst, v );
		dst += 3;
		in += 4;
	}
	if( end - dst ) {
		g_BASE64_DEC_STEP( 0 );
		g_BASE64_DEC_STEP( 1 );
		g_BASE64_DEC_STEP( 2 );
		g_BASE64_DEC_STEP( 3 );
		*dst++ = v >> 16;
		if( end - dst )
			*dst++ = v >> 8;
		if( end - dst )
			*dst++ = v;
		in += 4;
	}
	while( 1 ) {
		g_BASE64_DEC_STEP( 0 );
		in++;
		g_BASE64_DEC_STEP( 0 );
		in++;
		g_BASE64_DEC_STEP( 0 );
		in++;
		g_BASE64_DEC_STEP( 0 );
		in++;
	}

out3:
	*dst++ = v >> 10;
	v <<= 2;
out2:
	*dst++ = v >> 4;
out1:
out0:
	return bits & 1 ? -1 : dst - out;
}

char *g_base64_encode( char *out, int out_size, const uint8_t *in, int in_size )
{
#define g_BASE64_SIZE(x) (((x)+2) / 3 * 4 + 1)

#define g_RB32(x)                                    \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
	
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char *ret, *dst;
	unsigned i_bits = 0;
	int i_shift = 0;
	int bytes_remaining = in_size;

	/*if( in_size >= UINT_MAX / 4 ||
		out_size < g_BASE64_SIZE( in_size ) )
		return NULL;*/
	ret = dst = out;
	while( bytes_remaining > 3 ) {
		i_bits = g_RB32( in );
		in += 3; bytes_remaining -= 3;
		*dst++ = b64[i_bits>>26];
		*dst++ = b64[(i_bits>>20) & 0x3F];
		*dst++ = b64[(i_bits>>14) & 0x3F];
		*dst++ = b64[(i_bits>>8) & 0x3F];
	}
	i_bits = 0;
	while( bytes_remaining ) {
		i_bits = (i_bits << 8) + *in++;
		bytes_remaining--;
		i_shift += 8;
	}
	while( i_shift > 0 ) {
		*dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
		i_shift -= 6;
	}
	while( (dst - ret) & 3 )
		*dst++ = '=';
	*dst = '\0';

	return ret;
}

#if 0
INT g_base64_encode2( const BYTE* inputBuffer, INT inputCount, char* outputBuffer )
{
	static const CHAR* DATA_BIN2ASCII = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	INT i;
	BYTE b0, b1, b2;

	if( (inputBuffer == NULL) || (inputCount < 0) )
	{
		return -1;    // �������
	}

	if( outputBuffer != NULL )
	{
		for( i = inputCount; i > 0; i -= 3 )
		{
			if( i >= 3 )
			{    // ��3�ֽ����ת����4��ASCII�ַ�
				b0 = *inputBuffer++;
				b1 = *inputBuffer++;
				b2 = *inputBuffer++;

				*outputBuffer++ = DATA_BIN2ASCII[b0 >> 2];
				*outputBuffer++ = DATA_BIN2ASCII[((b0 << 4) | (b1 >> 4)) & 0x3F];
				*outputBuffer++ = DATA_BIN2ASCII[((b1 << 2) | (b2 >> 6)) & 0x3F];
				*outputBuffer++ = DATA_BIN2ASCII[b2 & 0x3F];
			}
			else
			{
				b0 = *inputBuffer++;
				if( i == 2 )b1 = *inputBuffer++; else b1 = 0;

				*outputBuffer++ = DATA_BIN2ASCII[b0 >> 2];
				*outputBuffer++ = DATA_BIN2ASCII[((b0 << 4) | (b1 >> 4)) & 0x3F];
				*outputBuffer++ = (i == 1) ? TEXT( '=' ) : DATA_BIN2ASCII[(b1 << 2) & 0x3F];
				*outputBuffer++ = TEXT( '=' );
			}
		} // End for i

		*outputBuffer++ = TEXT( '\0' );    // ����ַ������
		//*outputBuffer++ = 0;
	}

	return ((inputCount + 2) / 3) * 4;    // ������Ч�ַ����
}

// ��
bool g_base64_encode3( const BYTE* input, const INT length, std::string& encoded )
{
	static const std::string alphabet64( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" );
	static const char pad = '=';
	static const char np  = (char)std::string::npos;

	char c;

	if( (input == NULL) || (length < 0) )
	{
		return false;    // �������
	}

	encoded.reserve( length * 2 );

	for( std::string::size_type i = 0; i < length; ++i )
	{
		c = static_cast<char>((input[i] >> 2) & 0x3f);
		encoded += alphabet64[c];

		c = static_cast<char>((input[i] << 4) & 0x3f);
		if( ++i < length )
			c = static_cast<char>(c | static_cast<char>((input[i] >> 4) & 0x0f));
		encoded += alphabet64[c];

		if( i < length )
		{
			c = static_cast<char>((input[i] << 2) & 0x3c);
			if( ++i < length )
				c = static_cast<char>(c | static_cast<char>((input[i] >> 6) & 0x03));
			encoded += alphabet64[c];
		}
		else
		{
			++i;
			encoded += pad;
		}

		if( i < length )
		{
			c = static_cast<char>(input[i] & 0x3f);
			encoded += alphabet64[c];
		}
		else
		{
			encoded += pad;
		}
	}

	return true;
}

static bool g_FileToBase64test( const std::string &filename, std::string &base64 )
{
	FILE *fd = NULL;
	fopen( &fd, filename.c_str(), "rb+" );

	if( !fd )
	{
		base64 = "open file err!";
		return false;
	}

	fseek( fd, 0, SEEK_END );
	const int nFileLen = ftell( fd );
	fseek( fd, 0, SEEK_SET );

	if( nFileLen < 1 )
	{
		base64 = "";
		return true;
	}

	if( nFileLen > 1*1024*1024 )
	{
		base64 = "file too large!";
		return false;
	}

	unsigned char *buf = new unsigned char[nFileLen];
	////unsigned char buf[300*1024];
	size_t ret=0;

	ret = fread( buf, 1, nFileLen, fd );

	if( ferror( fd ) )
	{
		base64 = "file read err!";
		fclose( fd );
		return false;
	}

	const int base64bufsize = nFileLen*2;

	// 1
	//std::string strimg;
	//g_base64_encode3( buf, nFileLen, strimg );

	//FILE *fdtxt = NULL;
	//fopen( &fdtxt, (filename+".txt").c_str(), "wb+" );
	//fwrite( strimg.c_str(), 1, strimg.length(), fdtxt );
	//fclose( fdtxt );

#if 1
	// 2
	char *buf64 = new char[base64bufsize];
	//char *buf64 = g_GetMemoryContainer()->GetMemory( base64bufsize );
	//char buf64[300*1024];
	memset( buf64, 0, base64bufsize );
	g_base64_encode2( buf, nFileLen, buf64 );
	std::string strimg2 = buf64;
	//g_GetMemoryContainer()->ReleaseMemory( buf64, base64bufsize );

	FILE *fdtxt2 = NULL;
	fopen( &fdtxt2, (filename+"2.txt").c_str(), "wb+" );
	fwrite( strimg2.c_str(), 1, strimg2.length(), fdtxt2 );
	fclose( fdtxt2 );

	// 3
	char *buf64ex1 = new char[base64bufsize];
	//char *buf64ex1 = g_GetMemoryContainer()->GetMemory( base64bufsize );
	//char buf64ex1[300*1024];
	memset( buf64ex1, 0, base64bufsize );
	g_base64_encode( buf64ex1, base64bufsize, buf, nFileLen );
	std::string strimgex1 = buf64ex1;
	//g_GetMemoryContainer()->ReleaseMemory( buf64ex1, base64bufsize );

	FILE *fdtxtex1 = NULL;
	fopen( &fdtxtex1, (filename+"ex1.txt").c_str(), "wb+" );
	fwrite( strimgex1.c_str(), 1, strimgex1.length(), fdtxtex1 );
	fclose( fdtxtex1 );

	if( strimgex1 != strimg2 )
	{
		base64 = "file read err!";
	}
#endif

	fclose( fd );

	//base64 = strimg;

	return true;
}
#endif

// �ļ������ֽ�ת����base64�ַ�
bool g_FileToBase64( const std::string &filename, std::string &base64 )
{
	FILE *fd = NULL;
	//fopen( &fd, filename.c_str(), "rb+" );

	if( !fd )
	{
		base64 = "";//open file err!
		return false;
	}

	fseek( fd, 0, SEEK_END );
	const int nFileLen = ftell( fd );
	fseek( fd, 0, SEEK_SET );

	if( nFileLen < 1 )
	{
		base64 = "";
		fclose( fd );
		return true;
	}

	//����
	if( nFileLen > 1024*1024 )
	{
		base64 = "";//file too large!
		fclose( fd );
		return false;
	}

	unsigned char *bufsrc = new unsigned char[nFileLen];
	size_t ret=0;

	ret = fread( bufsrc, 1, nFileLen, fd );

	if( ferror( fd ) )
	{
		base64 = "";//file read err!
		fclose( fd );
		delete []bufsrc;
		return false;
	}

	fclose( fd );

	const int base64bufsize = nFileLen*2;
	char *buf64 = new char[base64bufsize];
	memset( buf64, 0, base64bufsize );

	g_base64_encode( buf64, base64bufsize, bufsrc, nFileLen );
	base64 = buf64;

#if 0//decode
	uint8_t *dec64 = new uint8_t[base64bufsize];
	g_base64_decode( dec64, buf64, base64bufsize );

	if( memcmp( dec64, bufsrc, nFileLen ) != 0 )
	{
		base64 = "";
	}
	else
	{
		FILE *fddec = NULL;
		fopen( &fddec, (filename+"dec.jpg").c_str(), "wb+" );
		fwrite( dec64, 1, nFileLen, fddec );
		fclose( fddec );
	}
	delete []dec64;
#endif

#if 0
	FILE *fdtxt = NULL;
	fopen( &fdtxt, (filename+".txt").c_str(), "wb+" );
	fwrite( base64.c_str(), 1, base64.length(), fdtxt );
	fclose( fdtxt );
#endif

	delete[]bufsrc;
	delete[]buf64;
	return true;
}

bool g_base64_encode_str( std::string &out, const uint8_t *in, int in_size )
{
	out = "";
	if( in_size > 1024*1024 )
		return false;

	const int base64bufsize = in_size*2;
	char *buf64 = new char[base64bufsize];
	memset( buf64, 0, base64bufsize );

	g_base64_encode( buf64, base64bufsize, in, in_size );
	out = buf64;

	delete []buf64;
	return true;
}

std::string g_createPicPre_BaseName( const IOTDeviceType devType, const uint32_t devid )
{
	char name[64] ={0};

	snprintf( name, sizeof( name ), "dev_%d_%d_", devType, devid );

	return std::string( name );
}

std::string g_createPicPre_Name( const IOTDeviceType devType, const uint32_t devid, const std::string &PicPreType )
{
	return ( g_createPicPre_BaseName( devType, devid ) + PicPreType + ".jpg" );
}

std::string g_createPicPre_FullPathName( const IOTDeviceType devType, const uint32_t devid, const std::string &PicPreType )
{
	const std::string strsub_full = std::string( defIotStoreDir_Main ) + "\\" + defIotStoreDir_Pic + "\\";
	return ( strsub_full + g_createPicPre_Name(devType, devid, PicPreType) );
}

std::string g_createPicPre_InfoFileFullPathName( const IOTDeviceType devType, const uint32_t devid )
{
	return ( std::string( defIotStoreDir_Main ) + "\\" + defIotStoreDir_Pic + "\\" + g_createPicPre_BaseName(devType, devid) + "info.sta" );
}

