
#include "faadtest.h"

//FILE * m_infile = NULL;
//FILE * m_outfile = NULL;
unsigned char m_onfreamebuf_aac[MAXONFREAMSIZE_AAC];
unsigned char m_onfreamebuf_pcm[MAXONFREAMSIZE_PCM];

int Detach_Head_Aac(ADTS_HEADER * adtsheader,FILE *m_infile)
{
	unsigned int readsize = 0;
	readsize = fread(m_onfreamebuf_aac,1,ADTS_HEADER_LENGTH,m_infile);

	if (readsize < 0)
	{
		printf("ReadFile : pAudio_Aac_File ERROR\n");
		return getchar();
	}
	if (readsize == 0)
	{
		return readsize;
	}
	if ((m_onfreamebuf_aac[0] == 0xFF)&&((m_onfreamebuf_aac[1] & 0xF0) == 0xF0))    //syncword 12个1
	{
		adtsheader->syncword = (m_onfreamebuf_aac[0] << 4 )  | (m_onfreamebuf_aac[1]  >> 4);
		adtsheader->id = ((unsigned int) m_onfreamebuf_aac[1] & 0x08) >> 3;
		adtsheader->layer = ((unsigned int) m_onfreamebuf_aac[1] & 0x06) >> 1;
		adtsheader->protection_absent = (unsigned int) m_onfreamebuf_aac[1] & 0x01;
		adtsheader->profile = ((unsigned int) m_onfreamebuf_aac[2] & 0xc0) >> 6;
		adtsheader->sf_index = ((unsigned int) m_onfreamebuf_aac[2] & 0x3c) >> 2;
		adtsheader->private_bit = ((unsigned int) m_onfreamebuf_aac[2] & 0x02) >> 1;
		adtsheader->channel_configuration = ((((unsigned int) m_onfreamebuf_aac[2] & 0x01) << 2) | (((unsigned int) m_onfreamebuf_aac[3] & 0xc0) >> 6));
		adtsheader->original = ((unsigned int) m_onfreamebuf_aac[3] & 0x20) >> 5;
		adtsheader->home = ((unsigned int) m_onfreamebuf_aac[3] & 0x10) >> 4;
		adtsheader->copyright_identification_bit = ((unsigned int) m_onfreamebuf_aac[3] & 0x08) >> 3;
		adtsheader->copyright_identification_start = (unsigned int) m_onfreamebuf_aac[3] & 0x04 >> 2;		
		adtsheader->aac_frame_length = (((((unsigned int) m_onfreamebuf_aac[3]) & 0x03) << 11) | (((unsigned int) m_onfreamebuf_aac[4] & 0xFF) << 3)| ((unsigned int) m_onfreamebuf_aac[5] & 0xE0) >> 5) ;
		adtsheader->adts_buffer_fullness = (((unsigned int) m_onfreamebuf_aac[5] & 0x1f) << 6 | ((unsigned int) m_onfreamebuf_aac[6] & 0xfc) >> 2);
		adtsheader->no_raw_data_blocks_in_frame = ((unsigned int) m_onfreamebuf_aac[6] & 0x03);
	}
	else 
	{
		printf("ADTS_HEADER : BUF ERROR\n");
//		getchar();
	}
	return readsize;
}

int Detach_Head_Aac2(ADTS_HEADER * adtsheader,char *aacplaybuf)
{
	memcpy(m_onfreamebuf_aac,aacplaybuf,ADTS_HEADER_LENGTH);
	if ((m_onfreamebuf_aac[0] == 0xFF)&&((m_onfreamebuf_aac[1] & 0xF0) == 0xF0))    //syncword 12个1
	{
		adtsheader->syncword = (m_onfreamebuf_aac[0] << 4 )  | (m_onfreamebuf_aac[1]  >> 4);
		adtsheader->id = ((unsigned int) m_onfreamebuf_aac[1] & 0x08) >> 3;
		adtsheader->layer = ((unsigned int) m_onfreamebuf_aac[1] & 0x06) >> 1;
		adtsheader->protection_absent = (unsigned int) m_onfreamebuf_aac[1] & 0x01;
		adtsheader->profile = ((unsigned int) m_onfreamebuf_aac[2] & 0xc0) >> 6;
		adtsheader->sf_index = ((unsigned int) m_onfreamebuf_aac[2] & 0x3c) >> 2;
		adtsheader->private_bit = ((unsigned int) m_onfreamebuf_aac[2] & 0x02) >> 1;
		adtsheader->channel_configuration = ((((unsigned int) m_onfreamebuf_aac[2] & 0x01) << 2) | (((unsigned int) m_onfreamebuf_aac[3] & 0xc0) >> 6));
		adtsheader->original = ((unsigned int) m_onfreamebuf_aac[3] & 0x20) >> 5;
		adtsheader->home = ((unsigned int) m_onfreamebuf_aac[3] & 0x10) >> 4;
		adtsheader->copyright_identification_bit = ((unsigned int) m_onfreamebuf_aac[3] & 0x08) >> 3;
		adtsheader->copyright_identification_start = (unsigned int) m_onfreamebuf_aac[3] & 0x04 >> 2;
		adtsheader->aac_frame_length = (((((unsigned int) m_onfreamebuf_aac[3]) & 0x03) << 11) | (((unsigned int) m_onfreamebuf_aac[4] & 0xFF) << 3)| ((unsigned int) m_onfreamebuf_aac[5] & 0xE0) >> 5) ;
		adtsheader->adts_buffer_fullness = (((unsigned int) m_onfreamebuf_aac[5] & 0x1f) << 6 | ((unsigned int) m_onfreamebuf_aac[6] & 0xfc) >> 2);
		adtsheader->no_raw_data_blocks_in_frame = ((unsigned int) m_onfreamebuf_aac[6] & 0x03);
	}
	else
	{
		printf("ADTS_HEADER : BUF ERROR\n");
	}
	return ADTS_HEADER_LENGTH;
}

int accdecoder2(char* aacplaybuf, int aacplaylen, char *pcmplaybuf)
{
	NeAACDecFrameInfo frame_info;
	NeAACDecHandle hdecoder = 0;
	unsigned long samplerate = 0;
	unsigned char channels = 0;
	int read_data_size = 0;
	ADTS_HEADER adtsheader;
	int ret = 0;
	unsigned char * pcm_buf;
	int pcmplaylen = 0;

	printf("--------程序运行开始----------\n");
//////////////////////////////////////////////////////////////////////////
	memset(&frame_info, 0, sizeof(frame_info));

	//分帧
	//Detach_Head_Aac(&adtsheader,m_infile);
	Detach_Head_Aac2(&adtsheader, aacplaybuf);
	aacplaybuf += ADTS_HEADER_LENGTH;
	aacplaylen -= ADTS_HEADER_LENGTH;
	//read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH , 1 ,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);
	memcpy(m_onfreamebuf_aac + ADTS_HEADER_LENGTH, aacplaybuf,
			adtsheader.aac_frame_length - ADTS_HEADER_LENGTH);
	aacplaybuf += adtsheader.aac_frame_length - ADTS_HEADER_LENGTH;
	aacplaylen -= adtsheader.aac_frame_length - ADTS_HEADER_LENGTH;

	//* 打开解码器
	hdecoder = NeAACDecOpen();

	//防止采样频率加倍
#if 1
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hdecoder);
	conf->dontUpSampleImplicitSBR = 1;
	NeAACDecSetConfiguration(hdecoder, conf);
#endif

	ret = NeAACDecInit(hdecoder, m_onfreamebuf_aac, adtsheader.aac_frame_length,
			&samplerate, &channels);
	if (ret < 0) {
		printf("NeAACDecInit error\n");
		return -1;
	} else {
		printf("frame size %d\n", adtsheader.aac_frame_length);
		printf("samplerate %d, channels %d\n", samplerate, channels);
	}
	pcm_buf = m_onfreamebuf_pcm;
	memset(m_onfreamebuf_aac, 0, MAXONFREAMSIZE_AAC);
	memset(m_onfreamebuf_pcm, 0, MAXONFREAMSIZE_PCM);

	while ( aacplaylen > 0) {
		Detach_Head_Aac2(&adtsheader, aacplaybuf);
		aacplaybuf += ADTS_HEADER_LENGTH;
		aacplaylen -= ADTS_HEADER_LENGTH;

		printf("frame size %d\n", adtsheader.aac_frame_length);
		//read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH ,1,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);
		memcpy(m_onfreamebuf_aac + ADTS_HEADER_LENGTH, aacplaybuf,
				adtsheader.aac_frame_length - ADTS_HEADER_LENGTH);
		aacplaybuf += adtsheader.aac_frame_length - ADTS_HEADER_LENGTH;
		aacplaylen -= adtsheader.aac_frame_length - ADTS_HEADER_LENGTH;

		//decode ADTS frame
		pcm_buf = (unsigned char *) NeAACDecDecode(hdecoder, &frame_info,
				m_onfreamebuf_aac, adtsheader.aac_frame_length);

		if (frame_info.error > 0) {
			printf("%s\n", NeAACDecGetErrorMessage(frame_info.error));
		} else if (pcm_buf && frame_info.samples > 0) {
//			printf( "frame info: bytesconsumed %d, channels %d, header_type %d\object_type %d, samples %d, samplerate %d\n",
//					frame_info.bytesconsumed, frame_info.channels,
//					frame_info.header_type, frame_info.object_type,
//					frame_info.samples, frame_info.samplerate);
			memcpy(&pcmplaybuf[pcmplaylen], pcm_buf, frame_info.samples * frame_info.channels);
			pcmplaylen += frame_info.samples * frame_info.channels;
			//fwrite(pcm_buf, 1, frame_info.samples * frame_info.channels, m_outfile);      //2个通道
		}
		memset(m_onfreamebuf_aac, 0, MAXONFREAMSIZE_AAC);
		memset(m_onfreamebuf_pcm, 0, MAXONFREAMSIZE_PCM);
	}
printf("DEBUG 0001 pcmplaylen=%d\n",pcmplaylen);
	NeAACDecClose(hdecoder);
	return pcmplaylen;
}


int accdecoder(FILE *m_infile,FILE *m_outfile)
{
	NeAACDecFrameInfo frame_info;
	NeAACDecHandle	hdecoder  = 0;
	unsigned long samplerate = 0;
	unsigned char channels = 0;
	int read_data_size = 0;
	ADTS_HEADER  adtsheader;
	int ret = 0;
	unsigned char * pcm_buf;

	printf("--------程序运行开始----------\n");
//////////////////////////////////////////////////////////////////////////
	memset(&frame_info, 0, sizeof(frame_info));

	//分帧
    Detach_Head_Aac(&adtsheader,m_infile);
	read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH , 1 ,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);

	 //* 打开解码器
	hdecoder = NeAACDecOpen();

	//防止采样频率加倍
#if 1
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hdecoder);
	conf->dontUpSampleImplicitSBR = 1;
	NeAACDecSetConfiguration(hdecoder, conf);
#endif

	ret = NeAACDecInit(hdecoder, m_onfreamebuf_aac, adtsheader.aac_frame_length, &samplerate, &channels);
	if (ret < 0)
	{
		printf("NeAACDecInit error\n");
//		return getchar();
		return -1;
	}
	else
	{
		printf("frame size %d\n", adtsheader.aac_frame_length);
		printf("samplerate %d, channels %d\n", samplerate, channels);
	}
	pcm_buf = m_onfreamebuf_pcm;
	memset(m_onfreamebuf_aac,0,MAXONFREAMSIZE_AAC);
	memset(m_onfreamebuf_pcm,0,MAXONFREAMSIZE_PCM);

	while(Detach_Head_Aac(&adtsheader,m_infile) > 0)
	{
       printf("frame size %d\n", adtsheader.aac_frame_length);
	   read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH ,1,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);
       //decode ADTS frame
       pcm_buf  = (unsigned char *)NeAACDecDecode(hdecoder, &frame_info, m_onfreamebuf_aac, adtsheader.aac_frame_length);

       if(frame_info.error > 0)
       {
           printf("%s\n",NeAACDecGetErrorMessage(frame_info.error));
       }
       else if(pcm_buf && frame_info.samples > 0)
       {
           printf("frame info: bytesconsumed %d, channels %d, header_type %d\object_type %d, samples %d, samplerate %d\n",
           frame_info.bytesconsumed,
           frame_info.channels, frame_info.header_type,
           frame_info.object_type, frame_info.samples,
           frame_info.samplerate);

           fwrite(pcm_buf, 1, frame_info.samples * frame_info.channels, m_outfile);      //2个通道
       }
	   memset(m_onfreamebuf_aac,0,MAXONFREAMSIZE_AAC);
	   memset(m_onfreamebuf_pcm,0,MAXONFREAMSIZE_PCM);
	}
	NeAACDecClose(hdecoder);
}

//int faadmain()
//{
//	NeAACDecFrameInfo frame_info;
//	NeAACDecHandle    decoder  = 0;
//	unsigned long samplerate = 0;
//	unsigned char channels = 0;
//	int read_data_size = 0;
//	ADTS_HEADER  adtsheader;
//	int ret = 0;
//	unsigned char * pcm_buf;
//
//	printf("--------程序运行开始----------\n");
////	m_infile = fopen(INPUTFILENAME,"rb");
////	m_outfile = fopen(OUTPUTFILENAME,"wb");
//	m_infile = fopen("out.aac","rb");
//	m_outfile = fopen("faad.pcm","wb");
////////////////////////////////////////////////////////////////////////////
//	memset(&frame_info, 0, sizeof(frame_info));
//	 //* 打开解码器
//    decoder = NeAACDecOpen();
//
//	//分帧
//    Detach_Head_Aac(&adtsheader);
//	read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH , 1 ,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);
//	ret = NeAACDecInit(decoder, m_onfreamebuf_aac, adtsheader.aac_frame_length, &samplerate, &channels);
//	if (ret < 0)
//	{
//		printf("NeAACDecInit error\n");
//		return getchar();
//	}
//	else
//	{
//		printf("frame size %d\n", adtsheader.aac_frame_length);
//	}
//
//	pcm_buf = m_onfreamebuf_pcm;
//	memset(m_onfreamebuf_aac,0,MAXONFREAMSIZE_AAC);
//	memset(m_onfreamebuf_pcm,0,MAXONFREAMSIZE_PCM);
//
//
//	while(Detach_Head_Aac(&adtsheader) > 0)
//	{
//       printf("frame size %d\n", adtsheader.aac_frame_length);
//	   read_data_size = fread(m_onfreamebuf_aac + ADTS_HEADER_LENGTH ,1,adtsheader.aac_frame_length - ADTS_HEADER_LENGTH,m_infile);
//       //decode ADTS frame
//       pcm_buf  = (unsigned char *)NeAACDecDecode(decoder, &frame_info, m_onfreamebuf_aac, adtsheader.aac_frame_length);
//
//       if(frame_info.error > 0)
//       {
//           printf("%s\n",NeAACDecGetErrorMessage(frame_info.error));
//       }
//       else if(pcm_buf && frame_info.samples > 0)
//       {
//           printf("frame info: bytesconsumed %d, channels %d, header_type %d\object_type %d, samples %d, samplerate %d\n",
//           frame_info.bytesconsumed,
//           frame_info.channels, frame_info.header_type,
//           frame_info.object_type, frame_info.samples,
//           frame_info.samplerate);
//
//           fwrite(pcm_buf, 1, frame_info.samples * frame_info.channels, m_outfile);      //2个通道
//       }
//	   memset(m_onfreamebuf_aac,0,MAXONFREAMSIZE_AAC);
//	   memset(m_onfreamebuf_pcm,0,MAXONFREAMSIZE_PCM);
//	}
//
//	NeAACDecClose(decoder);
////////////////////////////////////////////////////////////////////////////
//	if (m_infile)
//	{
//		fclose(m_infile);
//		m_infile = NULL;
//	}
//	if (m_outfile)
//	{
//		fclose(m_outfile);
//		m_outfile = NULL;
//	}
//	printf("--------程序运行结束----------\n");
//	printf("-------请按任意键退出---------\n");
// return getchar();
//}
