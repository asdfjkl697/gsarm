#include "H264MediaFile.h"

H264MediaFile::H264MediaFile(const char* filename)
	: m_isHasNal(false), m_isHasKey(false)
{
	//fd = NULL;
	//fopen_s(&fd,filename,"wb+"); //jyc20170323 modify
	fd = fopen(filename,"wb+"); 
}

H264MediaFile::~H264MediaFile(void)
{
	fclose(fd);
}

void H264MediaFile::WriteData( char *data,int size, defH264MediaFile_Data_ datatype )
{
#if 0
	bool doSave = false;

	switch(datatype)
	{
	case defH264MediaFile_Data_Nal:
		m_isHasNal = true;
		doSave = true;
		break;

	case defH264MediaFile_Data_Key:
		m_isHasKey = true;
		if( m_isHasNal )
		{
			doSave = true;
		}
		break;

	default:
		if( m_isHasNal && m_isHasKey )
		{
			doSave = true;
		}
		break;
	}

	if( !doSave )
		return;
#endif

	//const uint8_t addData[4] = {0x00,0x00,0x00,0x01};
	//fwrite(addData, 4,1, fd);

	if( size>0 && data )
		fwrite(data,size,1,fd);
}
