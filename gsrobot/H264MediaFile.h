#ifndef H264MEDIAFILE_H
#define H264MEDIAFILE_H

#include <stdio.h>
#include <stdint.h>

enum defH264MediaFile_Data_
{
	defH264MediaFile_Data_Normal,
	defH264MediaFile_Data_Key,
	defH264MediaFile_Data_Nal
};

class H264MediaFile
{
private:
	FILE *fd;
	bool m_isHasNal;
	bool m_isHasKey;

public:
	H264MediaFile(const char* filename);
	~H264MediaFile(void);

	void WriteData( char *data,int size, defH264MediaFile_Data_ datatype );
};

#endif