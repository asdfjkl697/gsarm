#pragma once

#include "../common.h"

//#include "DirectShow/Include/qedit.h"  //jyc20170322 
//#include "DirectShow/Include/dshow.h"
#pragma comment(lib,"strmiids.lib")

//#include <windows.h>
#include <queue>
#include "AudioCapBase.h"


#ifndef MYFREEMEDIATYPE
#define MYFREEMEDIATYPE(mt)	{if ((mt).cbFormat != 0)		\
					{CoTaskMemFree((PVOID)(mt).pbFormat);	\
					(mt).cbFormat = 0;						\
					(mt).pbFormat = NULL;					\
				}											\
				if ((mt).pUnk != NULL)						\
				{											\
					(mt).pUnk->Release();					\
					(mt).pUnk = NULL;						\
				}}	
#endif


class AudioCap /* : public ISampleGrabberCB  */   //jyc20170322 remove
{   
public:
	AudioCap();
	~AudioCap(void);

	static bool GetDevices( std::list<std::string> &devs );

	void setAudioCB( AudioCapCallBack *cb )
	{
		m_cb = cb;
	}

	bool IsOpen() const;

	bool OpenAudio( const std::string cap_name, unsigned int channels, unsigned int samplerate, unsigned int BitsSample, unsigned int nAvgBytesPerSec );
	void CloseAudio();
	bool StartCap();
	void StopCap();

	unsigned char *new_bitbuf( unsigned int needsize )
	{
		if( m_bitbuf && needsize > m_bitbufSize )
		{
			delete []m_bitbuf;
			m_bitbuf = NULL;
			m_bitbufSize = 0;
		}

		if( !m_bitbuf )
		{
			m_bitbufSize = needsize;
			m_bitbuf = new unsigned char[m_bitbufSize];
			memset( m_bitbuf, 0, m_bitbufSize );
		}

		return m_bitbuf;
	}

public:
	/*
    STDMETHODIMP_(ULONG) AddRef() { return 1; }     
    STDMETHODIMP_(ULONG) Release() { return 2; }      
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)     
    { 
        if (NULL == ppvObject) 
            return E_POINTER;        
        if (riid == __uuidof(IUnknown))         
        {             
            *ppvObject = static_cast<IUnknown*>(this);              
            return S_OK;         
        }        
        if (riid == __uuidof(ISampleGrabberCB))         
        {            
            *ppvObject = static_cast<ISampleGrabberCB*>(this);
            return S_OK;         
        }         
        return E_NOTIMPL;     
    }
    HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample);        
    HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime,BYTE *pBuffer, long BufferLen);
	*/
	//jyc20170322 remove

protected:
	void ReleaseRes();
	
	unsigned char *m_bitbuf;
	unsigned int m_bitbufSize;
	void *m_capobj;
	
	//WAVEFORMATEX *wave;    //jyc20170322 remove
	//void ConfigureSampleAudioGrabber(ISampleGrabber *m_sampleGrabber);

	AudioCapCallBack *m_cb;
};
