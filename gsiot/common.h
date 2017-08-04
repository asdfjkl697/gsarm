#ifndef COMMON_H_
#define COMMON_H_
/*
#include "typedef.h"
#include <stdio.h>
#include <io.h>
#include <vector>
#include "logFileExFunc.h"
#include "RunCode.h"*/

#include "memmacro.h"
#include "typedef.h"
#include <stdio.h>
//#include <io.h> //20150526
#include <vector>
//#include "logFileExFunc.h" //20150526
#include "RunCode.h"

#include <string.h> //jyc20160526
#include <time.h>


#define LOGMSG printf

//jyc20160824 modify socket for linux 
#define SOCKET				int
#define SOCKADDR_IN			sockaddr_in
#define SOCKADDR			sockaddr
#define INVALID_SOCKET		-1
#define SOCKET_ERROR		-1

//#define BOOL int
//#define BYTE unsigned char
//#define DWORD unsigned long int
//#define CHAR char
//#define LPSTR char *  //jyc20160824
//#define LPCSTR char*
//#define LPVOID void*
//#define LPCVOID const void*

#define TRUE                true			//jyc20170319 modify type for linux
#define FALSE               false
#define __stdcall 
typedef int                 BOOL;		
typedef unsigned char       BYTE;
typedef char                CHAR;
typedef unsigned long       DWORD;
typedef char               *LPSTR;    
typedef const char         *LPCSTR;
typedef void			   *LPVOID;
typedef const void	       *LPCVOID;


class nullptr_tt
{
public:
    template<class _Tx> operator _Tx*() const { return 0; }
    template<class _Tx, class _Ty> operator _Ty _Tx::*() const{ return 0; }
private:
    void operator& () const; //not allowed address
};
const nullptr_tt nullptr;  //jyc20160824 add nullptr


typedef struct  //jyc20160919 for time
{
  int wSecond;			/* Seconds.	[0-60] (1 leap second) */
  int wMinute;			/* Minutes.	[0-59] */
  int wHour;			/* Hours.	[0-23] */
  int wDay;			   /* Day.		[1-31] */
  int wMonth;			/* Month.	[0-11] */
  int wYear;			/* Year	- 1900.  */
  int wDayOfWeek;		/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/
#ifdef	__USE_BSD
  long int tm_gmtoff;		/* Seconds east of UTC.  */
  __const char *tm_zone;	/* Timezone abbreviation.  */
#else
  long int __tm_gmtoff;		/* Seconds east of UTC.  */
  __const char *__tm_zone;	/* Timezone abbreviation.  */
#endif
}SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st);


unsigned int timeGetTime();

// ʱ��ֵת�����ַ���ʽ�ĸ�ʽ
enum defTimeToStrFmt_
{
	defTimeToStrFmt_Full6 = 1,
	defTimeToStrFmt_Full7,
	defTimeToStrFmt_YearMonthDayHourMin,
	defTimeToStrFmt_YearMonthDay,
	defTimeToStrFmt_YearMonth,
	defTimeToStrFmt_MonthDay,
	defTimeToStrFmt_HourMinSec,
	defTimeToStrFmt_HourMin,
	defTimeToStrFmt_UTC
};

#define defNormMsgOvertime (20*1000)
#define defNormSendCtlOvertime (2*1000)

#define macIsValueOverRange( val, valmin, valmax ) ( val < valmin || val > valmax )
#define macValueRangeFix_Min( val, valmin ) if( val < valmin ) { val = valmin; }
#define macValueRangeFix_Max( val, valmax ) if( val > valmax ) { val = valmax; }

#define defFilePath "E:\\iotcontrol"						// ����װ·��

#define defFileName_App		"GSIOTController.exe"			// Ӧ�ó������
#define defFileName_AppNew	"GSIOTController_newupdate.exe"	// Ӧ�ó���������
#define defFileName_Update	"gsupdate.exe"					// ��������

#define defFileRenameFix	".tempold"						// ��������ʱ�ļ���׺


#define macOperator_Less( A, B, v ) if( A.v != B.v ) { return (A.v < B.v); }

uint32_t Reversebytes_uint32(unsigned int value); //jyc20170228 add
uint16_t Reversebytes_uint16(unsigned int value); //jyc20170228 add

struct stru_vobj_config
{
	int id;
	int enable;
	IOTDeviceType vobj_type;
	int vobj_flag;
	std::string name;
	std::string ver;
	int param[6];					
	std::string param_str[2];		

	stru_vobj_config(
		int in_id=0,
		IOTDeviceType in_vobj_type=IOT_DEVICE_Unknown,
		const std::string &in_name=c_NullStr,
		int in_vobj_flag=0,
		int in_enable=1,
		const std::string &in_ver=c_NullStr
		)
		: id(in_id), vobj_flag(in_vobj_flag), name(in_name), ver(in_ver), vobj_type(in_vobj_type), enable(in_enable)
	{
		memset( &param, 0, sizeof(param) );
	}
};
typedef std::map<int,stru_vobj_config> defmapVObjConfig; // <ID,cfg>

class IGSClientExFunc
{
public:
	IGSClientExFunc(void){};
	~IGSClientExFunc(void){};

public:
	virtual CRunCodeMgr& get_RunCodeMgr()=0; 
	virtual void OnTimeOverForCmdRecv( const defLinkID LinkID, const IOTDeviceType DevType, const uint32_t DevID, const uint32_t addr )=0;
	virtual defUseable get_all_useable_state_ForLinkID( defLinkID LinkID )=0;
};

void g_SYS_SetGSIOTClient( IGSClientExFunc *pGSIOTClient );
IGSClientExFunc* g_SYS_GetGSIOTClient();

std::string g_GetConnectStateStr( defConnectState state );
std::string g_getstr_Useable( const defUseable Useable );

std::string g_CreateChangedForTS( bool base64=false );
void g_Changed( const defCfgOprt_ oprt, const IOTDeviceType DevType, const uint32_t DevID, const uint32_t addr=defIOTDeviceAddr_DevSelf );

int sys_reset( const char* strdesc, int bFlag );

int split(const std::string& str, std::vector<std::string>& ret_, std::string sep = ",");
int split_getmapval( const std::string& str, std::map<std::string,std::string>& mapval, const std::string getspec="", const std::string sepmain=",", const std::string sepsub="=" );
std::string g_lstval2str( std::vector<std::pair<std::string, std::string> >& lstval ); //20160604
std::string g_StrToPrecision( const std::string &srcstr, const int precision );

bool operator< ( const GSIOTAddrObjKey &key1, const GSIOTAddrObjKey &key2 );
bool operator< ( const struGSTime &key1, const struGSTime &key2 );

// ��ص����͵ļ���ֵ��غ�����������Ǽ�������ټ���ֵ
bool g_AddrTypeValue_hasTypeValue( const IOTDeviceType type );
int g_AddrTypeValue_getTypeValue( const IOTDeviceType type, const int v1k, const bool ValueForType=true );

int g_WindSpeedLevel( const float WindSpeed, const bool trymerge );

uint32_t g_GetComPortWriteTime( const uint32_t len );

std::string ASCIIToUTF8(const std::string& str);
std::string UTF8ToASCII(const std::string& str);

int getMacAddress(std::string& mac);
//int file_exists(const char *filename);
bool file_exists(const char *filename);
std::string getAppPath();
std::string getRandomCode();

// �ȵͺ���ֽ���
int ByteToInt32(uint8_t *buf);
uint16_t ByteToInt16(uint8_t *buf);
float ByteToFloat(uint8_t *buf);
double ByteToDouble(uint8_t *buf);
std::string ByteToString(uint8_t *buf,uint32_t len);

// �ȵͺ���ֽ���
uint8_t *Int16ToByte(uint8_t *buf,uint16_t val);
uint8_t *Int32ToByte(uint8_t *buf,uint32_t val);
uint8_t *LongToByte(uint8_t *buf,uint64_t val);
uint8_t *DoubleToByte(uint8_t *buf,double val);
uint8_t *FloatToByte(uint8_t *buf,float val);

// �ȸߺ���ֽ���
uint16_t Big_ByteToInt16(uint8_t *buf);
uint8_t *Big_Int16ToByte(uint8_t *buf,uint16_t val);

uint16_t crc16_verify( uint8_t *buf, uint16_t len );

uint32_t g_StringToBuffer( const std::string &str, uint8_t *buf, uint32_t len, bool hasspace=true );
std::string g_BufferToString( unsigned char *buf, int len, bool addspace=true, bool bluppercase=true );

#define defPrintfByteDefaultLinkID defLinkID_Local
void g_PrintfByte( unsigned char *buf,int len, const char *pinfo, defLinkID LinkID=defLinkID_Null, const char *pLinkName=NULL );

int g_GetH264PreFix( unsigned char *buf,int len, int &prefixNum, int Max=64 );
unsigned char* g_Get_x264_nal_t( unsigned char *buf, int len, x264_nal_t &nal );

void Copy_x264_nal_t( x264_nal_t &dest, const x264_nal_t &src );
void Copy_x264_nal_t( x264_nal_t **pdest, int &destsize, const x264_nal_t *psrc, const int srcsize );
void Delete_x264_nal_t( x264_nal_t &src );
void Delete_x264_nal_t( x264_nal_t **psrc, int &srcsize );
uint32_t g_h264_find_next_start_code_ex( const uint8_t *pBuf, uint32_t bufLen, uint32_t &prefix );
uint32_t g_h264_remove_all_start_code( uint8_t *pBuf_src, uint32_t bufLen_src );

void g_toLowerCase( std::string &str );
void g_toUpperCase( std::string &str );

void g_GetLocalIP( std::list<std::string> &LocalIPList, std::list<std::string> &OtherIP );

std::string g_ConvertCameraTypeToString( IPCameraType type );

bool IsDisableModule( const uint8_t ModuleIndex, bool isRecv );

bool g_CheckEMail( const std::string &email_address );

std::string g_Trans_GSReturn( defGSReturn ret );

time_t g_GetUTCTime();
bool g_UTCTime_To_struGSTime( const time_t utctime, struGSTime &dest );
time_t g_struGSTime_To_UTCTime( const struGSTime &src );
void g_tm_To_struGSTime( const tm &src, struGSTime &dest );
void g_struGSTime_To_tm( const struGSTime &src, tm &dest );
void g_struGSTime_GetCurTime( struGSTime &dt );
void g_reget_struGSTime_GetCurTime( struGSTime &dt, uint32_t &last_gettime, const uint32_t cur_time );
const unsigned char g_GetMaxDay( const unsigned short Year, const unsigned char Month );
std::string g_TimeToStr( const time_t utctime, const defTimeToStrFmt_ fmt=defTimeToStrFmt_Full6 );
std::string g_TimeToStr( const struGSTime &dt, const defTimeToStrFmt_ fmt=defTimeToStrFmt_Full6 );
std::string g_SecondToStr( const uint32_t second );

bool g_IsSameMonth( const time_t utctimeA, const time_t utctimeB );
bool g_IsSameDay( const time_t utctimeA, const time_t utctimeB );
bool g_IsSameMonth( const struGSTime &dtA, const struGSTime &dtB );
bool g_IsSameDay( const struGSTime &dtA, const struGSTime &dtB );
bool g_IsSameHour( const struGSTime &dtA, const struGSTime &dtB );
int g_CompareDay( const struGSTime &dtA, const struGSTime &dtB );
int g_CompareMonth( const struGSTime &dtA, const struGSTime &dtB );

bool g_IsNowDay( const time_t utctime );
bool g_IsNowDay( const struGSTime &dt );
bool g_IsNowMonth( const time_t utctime );
bool g_IsNowMonth( const struGSTime &dt );

bool md5_CheckFile( const std::string& filename, std::string& digest );

// ��wavͷ��Ϣ����out_buffer���棬����in_buffer��Ϊ��
int write_wav_head( /*WAVEFORMATEX*/void *wf, char *in_buffer, int in_len, char *out_buffer, int *out_len );
std::string& g_replace_all_distinct( std::string& str, const std::string& old_value, const std::string& new_value );

std::string get_GSPlayBackCode_Name( GSPlayBackCode_ ControlCode );

defCodeIndex_ g_AlarmGuardTimeWNum2Index( int w );
void g_GetAlarmGuardTime( const defCodeIndex_ dayOfWeek, std::vector<uint32_t> &vecFlag, std::vector<uint32_t> &vecBegin, std::vector<uint32_t> &vecEnd );

std::string g_Trans_GSAGCurState( GSAGCurState_ state );

std::string g_parseAddrForHttp( const std::string &url );

// �洢���
bool g_isNeedSaveType( const IOTDeviceType type );
time_t g_GetTimePointSecond( const IOTDeviceType type, const bool isPrevSecond );
bool g_isTimePoint( const time_t utctime, const IOTDeviceType type );
time_t g_TransToTimePoint( const time_t utctime, const IOTDeviceType type, const bool isSave );
float g_SYS_VChgRng( const IOTDeviceType type );

std::string g_GetUnitBaseForType( const IOTDeviceType type );
std::string g_GetUnitUseValueForType( const IOTDeviceType type, const bool ValueForType );
int g_GetPrecisionForType(const IOTDeviceType type); //jyc20170422 add by jianguang
std::string g_V1kToStr( const int v1k, const std::string &strunit=c_NullStr, const int precision=1 );
std::string g_V1kToStrUseValueForType( const IOTDeviceType type, const int v1k, const bool ValueForType, const bool showunit=true, const int precision=0 );

bool g_IsRTMFP_url( const std::string &strurl );

BOOL g_DeleteFileEx( const char *filename );
int g_FileSize( const std::string &filename );
bool g_SaveStringToFile( const std::string &pathname, const std::string &strsave );
bool g_SaveBytesToFile( const std::string &pathname, const BYTE *pdata, const uint32_t len, const bool toTail=false );
std::string g_iotstore_createdir( const std::string &strsub );

std::string g_createPicPre_BaseName( const IOTDeviceType devType, const uint32_t devid );
std::string g_createPicPre_Name( const IOTDeviceType devType, const uint32_t devid, const std::string &PicPreType );
std::string g_createPicPre_FullPathName( const IOTDeviceType devType, const uint32_t devid, const std::string &PicPreType );
std::string g_createPicPre_InfoFileFullPathName( const IOTDeviceType devType, const uint32_t devid );

char *g_base64_encode( char *out, int out_size, const uint8_t *in, int in_size );
bool g_base64_encode_str( std::string &out, const uint8_t *in, int in_size );
int g_base64_decode( uint8_t *out, const char *in_str, int out_size );

bool g_FileToBase64( const std::string &filename, std::string &base64 );

#endif
