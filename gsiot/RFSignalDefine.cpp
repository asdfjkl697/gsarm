#include "common.h"
#include "RFSignalDefine.h"
//#include "logFileExFunc.h"
//#include "windows.h"

std::string RFSignal::Trans_Type2Str( const defRFSignalType signal_type )
{
	switch( signal_type )
	{
	case defRFSignalType_invalid:
		return "无效数据";

	case defRFSignalType_code:
		return "无线编码数据";

	case defRFSignalType_original:
		return "无线原始数据";

	case defRFSignalType_IR_code:
		return "红外编码数据";

	case defRFSignalType_IR_original:
		return "红外原始数据";
	}

	return "(无)";
}

bool RFSignal::operator == ( const RFSignal &other ) const
{
	if( !other.isValid() || !this->isValid() )
		return false;

	if( other.signal_type != this->signal_type )
		return false;

	if( other.freq != this->freq )
		return false;

	switch( signal_type )
	{
	case defRFSignalType_code:
	{
		if( other.code != this->code )
			return false;

		if( other.codeValidLen != this->codeValidLen )
			return false;

		if( other.headlen != this->headlen )
			return false;

		if( memcmp( &other.headcode, &this->headcode, sizeof( other.headcode ) ) )
			return false;

		if( other.one_high_time != this->one_high_time )
			return false;

		if( other.one_low_time != this->one_low_time )
			return false;

		if( other.zero_high_time != this->zero_high_time )
			return false;

		if( other.zero_low_time != this->zero_low_time )
			return false;

		if( other.silent_interval != this->silent_interval )
			return false;

		if( other.taillen != this->taillen )
			return false;

		if( memcmp( &other.tailcode, &this->tailcode, sizeof( other.tailcode ) ) )
			return false;

		return true;
	}
	break;

	case defRFSignalType_original:
	case defRFSignalType_IR_original:
	{
		if( other.original_headflag != this->original_headflag )
			return false;

		if( other.original_headtime != this->original_headtime )
			return false;

		if( other.original_len != this->original_len )
			return false;

		if( memcmp( &other.original, &this->original, sizeof( other.original ) ) )
			return false;

		return true;
	}
	break;

	case defRFSignalType_IR_code:
	{
		if( other.original_len != this->original_len )
			return false;

		const int originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;

		for( int i=0; i<originallen; ++i )
		{
			if( other.original[i] != this->original[i] )
				return false;
		}

		return true;
	}
	break;
	}

	return false;
}

bool RFSignal::IsNear( const RFSignal &other ) const
{
	if( !other.isValid() || !this->isValid() )
		return false;
	if( other.signal_type != this->signal_type )
		return false;
	if( other.freq != this->freq )
		return false;
	switch( signal_type )
	{
	case defRFSignalType_code:
		{
			if( other.code != this->code )
				return false;
			if( other.codeValidLen != this->codeValidLen )
				return false;
			if( other.headlen != this->headlen )
				return false;
			if( other.taillen != this->taillen )
				return false;
			return true;
		}
		break;

	case defRFSignalType_original:
	case defRFSignalType_IR_original:
		{
			if( other.original_headflag != this->original_headflag )
				return false;

			if( other.original_len != this->original_len )
				return false;

			if( abs( (int)other.original_headtime-(int)this->original_headtime ) > 200 )
				return false;
			const int originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;

			for( int i=0; i<originallen; ++i )
			{
				if( abs( (int)other.original[i]-(int)this->original[i] ) > 200 )
					return false;
			}
			return true;
		}
	break;

	case defRFSignalType_IR_code:
		{
			if( other.original_len != this->original_len )
				return false;

			const int originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;

			for( int i=0; i<originallen; ++i )
			{
				if( other.original[i] != this->original[i] )
					return false;
			}
		}
		break;
	}

	return false;
}

void RFSignal::Update( const RFSignal &src, bool onlysignal )
{
	int temp_id = this->id;
	int temp_signal_count = this->signal_count;
	*this = src;
	this->id = temp_id;
	if( !onlysignal )
	{
		this->signal_count = temp_signal_count;
	}

	const uint8_t signal_count_min  = RUNCODE_Get( defCodeIndex_RF_Send_CountMin );
	if( this->signal_count < signal_count_min )
	{
		this->signal_count = signal_count_min;
	}
}

SerialPortModule RFSignal::GetModuleIndex( bool isSend )
{
	switch( signal_type )
	{
	case defRFSignalType_code:
		{
			if( defFreq_433 == freq )
			{
				return (isSend ? RXB8_433_TX : RXB8_433);
			}

			return (isSend ? RXB8_315_TX : RXB8_315);
		}
		break;

	case defRFSignalType_original:
		{
			if( defFreq_433 == freq )
			{
				return RXB8_433_original;
			}

			return RXB8_315_original;
		}
		break;

	case defRFSignalType_IR_code:
		{
			return (isSend ? MOD_IR_TX_code : MOD_IR_RX_code);
		}
		break;

	case defRFSignalType_IR_original:
		{
			return (isSend ? MOD_IR_TX_original : MOD_IR_RX_original);
		}
		break;
	}

	return MOD_NULL;
}

std::string RFSignal::Print( const char *info, bool doPrint ) const
{
	char buf[1024] = {0};
	/*jyc20160824
	const int thisThreadId = ::GetCurrentThreadId();

	switch( signal_type )
	{
	case defRFSignalType_code:
		{
			snprintf( buf, sizeof( buf ), "RFSignal(%s) type=%d, freq=%d, code=%d, codeValidLen=%d, headlen=%d, taillen=%d -ThId%d\r\n", info?info:"", this->signal_type, this->freq, this->code, this->codeValidLen, this->headlen, this->taillen, thisThreadId );
		}
		break;

	case defRFSignalType_original:
	case defRFSignalType_IR_original:
		{
			snprintf( buf, sizeof( buf ), "RFSignal(%s) type=%d, freq=%d, og_headflag=%d, og_headtime=%d, og_len=%d -ThId%d\r\n", info?info:"", this->signal_type, this->freq, this->original_headflag, this->original_headtime, this->original_len, thisThreadId );
		}
		break;

	case defRFSignalType_IR_code:
	default:
		{
			snprintf( buf, sizeof( buf ), "RFSignal(%s) type=%d, freq=%d, og_len=%d -ThId%d\r\n", info?info:"", this->signal_type, this->freq, this->original_len, thisThreadId );
		}
		break;
	}*/

	if( doPrint )
	{
		LOGMSG( buf );
	}

	return std::string(buf);
}

std::string RFSignal::GetUIShow( const uint8_t spec_count ) const
{
	char buf[1024] ={0};

	switch( signal_type )
	{
	case defRFSignalType_original:
	case defRFSignalType_IR_original:
		//jyc20160824
		//snprintf( buf, sizeof(buf), "发送次数=%d, 数据长度=%d, 发送需时%s=%lums, 原始头标志=%d, 原始头=%d, 数据=%s", signal_count, original_len, spec_count?"(1)":"", GetSendNeedTime(spec_count), original_headflag, original_headtime, Get_original( 10 ).c_str() );
		break;

	case defRFSignalType_code:
		snprintf( buf, sizeof(buf), "发送次数=%d, 编码=%d, 编码长=%d, 编码头长=%d, 编码尾长=%d", signal_count, code, codeValidLen, headlen, taillen );
		break;

	case defRFSignalType_IR_code:
	default:
		snprintf( buf, sizeof(buf), "发送次数=%d, 数据长度=%d, 数据=%s", signal_count, original_len, Get_original(1,true).c_str() );
		break;
	}

	return std::string( buf );
}

#define defRFSignalOriginalStrMax 512
bool RFSignal::Set_original( const std::string &stroriginal, const bool isByteStr )
{
	const bool isByteStreamFmt = ( defRFSignalType_IR_code == signal_type );

	if( isByteStr )
	{
		uint8_t count = g_StringToBuffer( stroriginal, (uint8_t*)original, sizeof( original ) );

		this->original_len = isByteStreamFmt ? count : count/2;

		return true;
	}

	this->original_len = 0;

	if( stroriginal.empty() )
	{
		return false;
	}

	char msg[16] = {0};

	const char *pc = stroriginal.c_str();
	const char *const pc_end = pc+stroriginal.length();

	memcpy( msg, pc, 4 );
	msg[4] = 0;

	char *stopstring = NULL;
	uint16_t flag = (uint16_t)strtoul( msg, &stopstring, 16 );

	if( 1!=flag )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "Set_original, id=%d", id );
		//return false;
	}

	pc += 5; // 跳过头

	uint8_t count = 0;

	if( isByteStreamFmt )
	{
		uint8_t *pdata = (uint8_t*)this->original;
		while( pc<pc_end )
		{
			// 1
			memcpy( msg, pc, 2 );
			msg[2] = 0;
			stopstring = NULL;
			uint16_t val1 = (uint16_t)strtoul( msg, &stopstring, 16 );
			pc += 2;

			pdata[count] = val1;

			++count;
			if( count>=defRFSignalOriginalMax*2 )
			{
				break;
			}
		}
	}
	else
	{
		while( pc<pc_end )
		{
			// 16
			//memcpy( msg, pc, 4 );
			//msg[4] = 0;

			//stopstring = NULL;
			//this->original[count] = (uint16_t)strtoul( msg, &stopstring, 16 );

			//pc += 4;

			// 1
			memcpy( msg, pc, 2 );
			msg[2] = 0;
			stopstring = NULL;
			uint16_t val1 = (uint16_t)strtoul( msg, &stopstring, 16 );
			pc += 2;

			memcpy( msg, pc, 2 );
			msg[2] = 0;
			stopstring = NULL;
			uint16_t val2 = (uint16_t)strtoul( msg, &stopstring, 16 );
			pc += 2;

#ifdef OS_OPENWRT	//jyc20170505 modify OS_UBUNTU_FLAG
			this->original[count] = val1<<8 | val2&0xFF;
#else
			this->original[count] = val2<<8 | val1&0xFF;			
#endif

			++count;
			if( count>=defRFSignalOriginalMax )
			{
				break;
			}
		}
	}

	this->original_len = count;
	return true;
}

// format:1/10/16
std::string RFSignal::Get_original( int format, const bool isByteStr ) const
{
	if( isByteStr )
	{
		char msg[64] ={0};
		std::string str;
		str.reserve( defRFSignalOriginalStrMax );

		int originallen = this->original_len <= defRFSignalOriginalMax*2 ? this->original_len : defRFSignalOriginalMax*2;

		uint8_t *pdata = (uint8_t*)this->original;
		for( int i = 0; i<originallen; ++i )
		{
			if( i>0 )
			{
				str.append( " " );
			}

			if( 1==format )
			{
				snprintf( msg, sizeof( msg ), "%02X", pdata[i] );
			}

			str.append( msg );
		}

		return str;
	}


	const bool isByteStreamFmt = (defRFSignalType_IR_code == signal_type);

	char msg[64] = {0};

	int originallen = 0;

	if( isByteStreamFmt )
	{
		if( 1 != format )
			return std::string( "" );

		originallen = this->original_len <= defRFSignalOriginalMax*2 ? this->original_len : defRFSignalOriginalMax*2;
	}
	else
	{
		originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;
	}

	if( originallen<=0 )
	{
		return std::string("");
	}

	std::string str;
	str.reserve(defRFSignalOriginalStrMax);

	uint16_t flag = (uint16_t)format;

	snprintf( msg, sizeof(msg), "%04X_", flag );
	str.append( msg );

	if( isByteStreamFmt )
	{
		uint8_t *pdata = (uint8_t*)this->original;
		for( int i = 0; i<originallen; ++i )
		{
			if( 1==format )
			{
				snprintf( msg, sizeof( msg ), "%02X", pdata[i] );
			}

			str.append( msg );
		}

		return str;
	}

	for( int i = 0; i<originallen; ++i )
	{
		if( 16==format )
		{
			snprintf( msg, sizeof(msg), "%04X", this->original[i] );
		}
		else if( 1==format )
		{
			const uint8_t *btval = (uint8_t*)&(this->original[i]);
			snprintf( msg, sizeof(msg), "%02X%02X", btval[0], btval[1] );
		}
		else
		{
			if( 0!=i )
				str.append( "," );
			snprintf( msg, sizeof(msg), "%d", this->original[i] );
		}

		str.append( msg );
	}

	return str;
}

#define defRFSignalFile_AppName "RFSignal"
#define defRFSignalFile_Ver "1000"

//#define macRFFSave_Str(key,strval) if( !::WritePrivateProfileStringA( defRFSignalFile_AppName, key, strval, filename.c_str() ) ) { return false; }
//#define macRFFSave_Int(key,intval) _ultoa_s(intval, buf, sizeof(buf), 10); macRFFSave_Str( key, buf );
bool RFSignal::SaveToFile( const std::string &filename ) const //jyc20170224 notice
{
	char buf[256] = {0};

	// 写入信息
	/*
	macRFFSave_Str( "ver", defRFSignalFile_Ver );

	macRFFSave_Int( "signal_type", this->signal_type );
	macRFFSave_Int( "freq", this->freq );
	macRFFSave_Int( "code", this->code );
	macRFFSave_Int( "codeValidLen", this->codeValidLen );

	macRFFSave_Int( "one_high_time", this->one_high_time );
	macRFFSave_Int( "one_low_time", this->one_low_time );
	macRFFSave_Int( "zero_high_time", this->zero_high_time );
	macRFFSave_Int( "zero_low_time", this->zero_low_time );

	macRFFSave_Int( "silent_interval", this->silent_interval );
	macRFFSave_Int( "signal_count", this->signal_count );
	macRFFSave_Int( "headlen", this->headlen );
	macRFFSave_Int( "taillen", this->taillen );

	macRFFSave_Int( "headcode1", this->headcode[0] );
	macRFFSave_Int( "headcode2", this->headcode[1] );
	macRFFSave_Int( "headcode3", this->headcode[2] );
	macRFFSave_Int( "headcode4", this->headcode[3] );
	macRFFSave_Int( "headcode5", this->headcode[4] );
	macRFFSave_Int( "headcode6", this->headcode[5] );
	macRFFSave_Int( "headcode7", this->headcode[6] );

	macRFFSave_Int( "tailcode1", this->tailcode[0] );
	macRFFSave_Int( "tailcode2", this->tailcode[1] );
	macRFFSave_Int( "tailcode3", this->tailcode[2] );
	macRFFSave_Int( "tailcode4", this->tailcode[3] );
	macRFFSave_Int( "tailcode5", this->tailcode[4] );
	macRFFSave_Int( "tailcode6", this->tailcode[5] );
	macRFFSave_Int( "tailcode7", this->tailcode[6] );

	macRFFSave_Int( "original_headflag", this->original_headflag );
	macRFFSave_Int( "original_headtime", this->original_headtime );
	macRFFSave_Str( "original", this->Get_original().c_str() );
	*/

	return true;
}

//#define macRFFLoad_Str(key) ::GetPrivateProfileStringA( defRFSignalFile_AppName, key, "", buf, sizeof(buf), filename.c_str() )
//#define macRFFLoad_Int(key) ::GetPrivateProfileIntA( defRFSignalFile_AppName, key, 0, filename.c_str() )
bool RFSignal::LoadFromFile( const std::string &filename ) //jyc20170224 notice
{
	char buf[1024] = {0};

	this->id = 0;
	/*
	this->signal_type = (defRFSignalType)macRFFLoad_Int( "signal_type" );
	this->freq = (defFreq)macRFFLoad_Int( "freq" );
	this->code = macRFFLoad_Int( "code" );
	this->codeValidLen = macRFFLoad_Int( "codeValidLen" );

	this->one_high_time = macRFFLoad_Int( "one_high_time" );
	this->one_low_time = macRFFLoad_Int( "one_low_time" );
	this->zero_high_time = macRFFLoad_Int( "zero_high_time" );
	this->zero_low_time = macRFFLoad_Int( "zero_low_time" );

	this->silent_interval = macRFFLoad_Int( "silent_interval" );
	this->signal_count = macRFFLoad_Int( "signal_count" );
	this->headlen = macRFFLoad_Int( "headlen" );
	this->taillen = macRFFLoad_Int( "taillen" );

	this->headcode[0] = macRFFLoad_Int( "headcode1" );
	this->headcode[1] = macRFFLoad_Int( "headcode2" );
	this->headcode[2] = macRFFLoad_Int( "headcode3" );
	this->headcode[3] = macRFFLoad_Int( "headcode4" );
	this->headcode[4] = macRFFLoad_Int( "headcode5" );
	this->headcode[5] = macRFFLoad_Int( "headcode6" );
	this->headcode[6] = macRFFLoad_Int( "headcode7" );

	this->tailcode[0] = macRFFLoad_Int( "tailcode1" );
	this->tailcode[1] = macRFFLoad_Int( "tailcode2" );
	this->tailcode[2] = macRFFLoad_Int( "tailcode3" );
	this->tailcode[3] = macRFFLoad_Int( "tailcode4" );
	this->tailcode[4] = macRFFLoad_Int( "tailcode5" );
	this->tailcode[5] = macRFFLoad_Int( "tailcode6" );
	this->tailcode[6] = macRFFLoad_Int( "tailcode7" );

	this->original_headflag = macRFFLoad_Int( "original_headflag" );
	this->original_headtime = macRFFLoad_Int( "original_headtime" );
	macRFFLoad_Str( "original" ); this->Set_original( buf );
	*/
	return true; // this->isValid();导入后根据信息自行判断
}

// 发送信号需要的时间，单位ms
uint32_t RFSignal::GetSendNeedTime( const uint8_t spec_count ) const
{
	if( defRFSignalType_original != signal_type && defRFSignalType_IR_original != signal_type )
		return 0;

	const uint8_t sendcount = spec_count ? spec_count : (signal_count>0 ? signal_count : 1);

	uint32_t timecount = original_headtime;

	const int originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;
	for( int i = 0; i<originallen; ++i )
	{
		timecount += this->original[i];
	}

	timecount *= sendcount;
	timecount /= 1000;

	int fixaddtime = 1000;
	timecount += fixaddtime;

	return ( (timecount>5000) ? 5000:timecount );
}

// 获取修正的基础单位量默认值
int RFSignal::GetFixUnit() const
{
	const int originallen = this->original_len <= defRFSignalOriginalMax ? this->original_len : defRFSignalOriginalMax;
	if( originallen<=0 )
	{
		return 0;
	}

	int min = 0;
	for( int i = 0; i<originallen; ++i )
	{
		if( !min || this->original[i]<min )
		{
			min = this->original[i];
		}
	}

	const int minRange = (min*25)/100;

	int count = 0;
	int minCountValue = 0;
	for( int i = 0; i<originallen; ++i )
	{
		if( abs(this->original[i]-min) < minRange )
		{
			count++;
			minCountValue += this->original[i];
		}
	}

	return ( 0==count ? 0 : (minCountValue/count) );
}

// 获取解码描述信息
std::string RFSignal::Get_DecodeDesc( bool showint ) const
{
	if( defRFSignalType_IR_code != signal_type )
	{
		return std::string("");
	}

	std::string str;
	std::string strdesc;
	char msg[64] ={0};

	uint8_t *ptemp = (uint8_t*)this->original;
	const uint8_t *pEnd = ptemp + this->original_len;

	// 数据头时间
	strdesc += "数据头=";
	if( showint )
	{
		for( int i=0; i<8/2; ++i )
		{
			uint16_t intdata = *((uint16_t*)(ptemp+i*2));
			snprintf( msg, sizeof( msg ), "%d,", intdata );
			strdesc += msg;
		}
	}
	strdesc += g_BufferToString( ptemp, 8 );
	ptemp += 8;
	strdesc += "; ";
	//strdesc += "\r\n";
	
	while( ptemp < pEnd )
	{
		const uint16_t dataflag = (*ptemp & 0x01);
		uint16_t datalen = (*ptemp & 0x1E) >> 1;
		const uint16_t bitlen = (*ptemp & 0xE0);
		
		if( bitlen ) datalen += 1;

		snprintf( msg, sizeof( msg ), "数据=%d%s: ", datalen, dataflag?"H":"t" );
		strdesc += msg;

		if( showint && 0==dataflag )
		{
			for( int i=0; i<datalen/2; ++i )
			{
				uint16_t intdata = *((uint16_t*)(ptemp+1+i*2));
				snprintf( msg, sizeof( msg ), "%d,", intdata );
				strdesc += msg;
			}
		}

		strdesc += g_BufferToString( ptemp, datalen+1 );
		ptemp += datalen+1;
		strdesc += "; ";
		//strdesc += "\r\n";
	}

	return strdesc;
}

