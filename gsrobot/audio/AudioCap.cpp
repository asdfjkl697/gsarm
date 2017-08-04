//#include <atlbase.h>  //jyc20170323 remove just for win
#include "AudioCap.h"
#include "AudioTestDefine.h"

#if defined(defTest_AudioSaveCapWav)
static FILE *fstreamsrc = NULL;
#endif


struct struCapObj
{
	//jyc20170323 remove
	//CComPtr<IFilterGraph2> m_pGraph;
	//CComPtr<IMediaControl> m_pMediaControl;
	//CComPtr<ICaptureGraphBuilder2> m_capGraph;
	//CComPtr<ISampleGrabber> m_sampAudioGrabber;
	//CComPtr<IBaseFilter> capAudioFilter;

	/*
	struCapObj::struCapObj()
	{
		ReleaseRes();
	}

	struCapObj::~struCapObj(void)
	{
		ReleaseRes();
	}
	*/

	void ReleaseRes()
	{
		//m_pMediaControl = NULL;
		//capAudioFilter = NULL;
		//m_sampAudioGrabber = NULL;
		//m_capGraph = NULL;
		//m_pGraph = NULL;
	}
};

struCapObj* g_CapObj(void *pcapobj)
{
	return (struCapObj*)pcapobj;
}

AudioCap::AudioCap()
	: m_bitbuf(NULL), m_bitbufSize(0)
{
	struCapObj *pcapobj = new struCapObj();
	m_capobj = pcapobj;
	ReleaseRes();
}

AudioCap::~AudioCap(void)
{
	ReleaseRes();

	if( m_bitbuf )
	{
		delete []m_bitbuf;
		m_bitbuf = NULL;
		m_bitbufSize = 0;
	}

	if( m_capobj )
	{
		delete g_CapObj(m_capobj);
		m_capobj = NULL;
	}
}

void AudioCap::ReleaseRes()
{
	g_CapObj(m_capobj)->ReleaseRes();
}

bool AudioCap::IsOpen() const
{
	//return !(!g_CapObj(m_capobj)->m_pMediaControl);
}

// cap_name为空表示自动
bool AudioCap::OpenAudio( const std::string cap_name, unsigned int channels, unsigned int samplerate, unsigned int BitsSample, unsigned int nAvgBytesPerSec )
{
	/* jyc20170323 remove
    HRESULT hr = S_OK;
	std::string reason;
	CComPtr<IBaseFilter> m_audioGrabFlt = NULL;
	CComPtr<IEnumMoniker> pEm = NULL;

	IMoniker *pM = NULL;
	int index = 0;
	char sName[1024] = {0};

    CComPtr<ICreateDevEnum> pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (hr != NOERROR)
	{
		reason = "CoCreateInstance";
        goto label_OpenAudio_Failed;
    }

    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory,
        &pEm, 0);
    if (hr != NOERROR) 
	{
		reason = "CreateClassEnumerator";
        goto label_OpenAudio_Failed;
    }
    
    pEm->Reset();
    ULONG cFetched;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
		IPropertyBag *pBag=0;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
			if(hr == NOERROR)
			{
				//获取设备名称            
				WideCharToMultiByte(CP_ACP,0,var.bstrVal,-1,sName, 1024 ,"",NULL);
				//std::string(sName) );
				SysFreeString(var.bstrVal);

				bool isfindcap = false;
				if( cap_name.empty() ) // 自动，选择第一个找到的可用音频输入设备
				{
					isfindcap = true;
				}
				else
				{
					if( cap_name == sName )
					{
						isfindcap = true;
					}
				}

				if( isfindcap )
				{
					pBag->Release();
					break;
				}
			}
			pBag->Release();
		}
		pM->Release();
		pM = NULL;
    }

    if(!pM)
	{
		reason = "pM is NULL";
        goto label_OpenAudio_Failed;
    }

    pCreateDevEnum = NULL;

	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IFilterGraph2, (void **)&g_CapObj(m_capobj)->m_pGraph);
    if(hr<0)
	{
		reason = "m_pGraph";
        goto label_OpenAudio_Failed;
    }
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, (void **)&g_CapObj(m_capobj)->m_capGraph);
    if(hr<0)
	{
		reason = "m_capGraph";
        goto label_OpenAudio_Failed;
    }
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, 
		IID_ISampleGrabber, (LPVOID *)&g_CapObj(m_capobj)->m_sampAudioGrabber);
    if(hr<0)
	{
		reason = "m_sampAudioGrabber";
        goto label_OpenAudio_Failed;
    }
	hr = g_CapObj(m_capobj)->m_pGraph->QueryInterface(IID_IMediaControl, (void **)&g_CapObj(m_capobj)->m_pMediaControl);
    if(hr<0)
	{
		reason = "m_pMediaControl";
        goto label_OpenAudio_Failed;
    }
	hr = g_CapObj(m_capobj)->m_capGraph->SetFiltergraph(g_CapObj(m_capobj)->m_pGraph);
    if(hr<0)
	{
		reason = "SetFiltergraph";
        goto label_OpenAudio_Failed;
    }

	hr = g_CapObj(m_capobj)->m_sampAudioGrabber->QueryInterface(IID_IBaseFilter, (void **)&m_audioGrabFlt);
    if(hr<0)
	{
		reason = "m_audioGrabFlt";
        goto label_OpenAudio_Failed;
    }
    
	ConfigureSampleAudioGrabber(g_CapObj(m_capobj)->m_sampAudioGrabber);

    // Add the frame grabber to the graph
	hr = g_CapObj(m_capobj)->m_pGraph->AddFilter(m_audioGrabFlt, L"Audio Grabber");
    if(hr<0)
	{
		reason = "AddFilter";
        goto label_OpenAudio_Failed;
    }

	hr = g_CapObj(m_capobj)->m_pGraph->AddSourceFilterForMoniker(pM, NULL, L"Audio Input", &g_CapObj(m_capobj)->capAudioFilter);
    if(hr<0)
	{
		reason = "AddSourceFilterForMoniker";
        goto label_OpenAudio_Failed;
    }
    pM->Release();

        
    IAMStreamConfig*  iconfig = NULL;
    iconfig = NULL;
	hr = g_CapObj(m_capobj)->m_capGraph->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, g_CapObj(m_capobj)->capAudioFilter, IID_IAMStreamConfig, (void**)&iconfig);
    if(hr<0)
	{
		reason = "IAMStreamConfig";
        goto label_OpenAudio_Failed;
    }
    //set stream config
    AM_MEDIA_TYPE* pmt = NULL;    
    if(iconfig->GetFormat(&pmt) !=S_OK) 
	{
		printf("GetFormat Failed ! \n");
		reason = "GetFormat";
		goto label_OpenAudio_Failed;   
	}

    WAVEFORMATEX *wave = (WAVEFORMATEX *)pmt->pbFormat;
    
	wave->wFormatTag = WAVE_FORMAT_PCM;
    wave->nSamplesPerSec = samplerate;
    wave->nChannels = channels;
	wave->wBitsPerSample = BitsSample;
	wave->nBlockAlign = wave->nChannels * (wave->wBitsPerSample/8);

	if( nAvgBytesPerSec )
		wave->nAvgBytesPerSec = nAvgBytesPerSec;
	else
		wave->nAvgBytesPerSec = wave->nSamplesPerSec * wave->nBlockAlign;

	wave->cbSize=0;

    hr = iconfig->SetFormat(pmt);
    if(hr<0)
	{
		reason = "SetFormat";
        goto label_OpenAudio_Failed;
    }

	hr = g_CapObj(m_capobj)->m_capGraph->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, g_CapObj(m_capobj)->capAudioFilter, NULL, m_audioGrabFlt);
    if(hr<0)
	{
		reason = "RenderStream";
        goto label_OpenAudio_Failed;
    }

#if defined(defTest_AudioSaveCapWav)
	char out_buffer[1024];
	int out_len = 0;
	char *pout_buffer = out_buffer;

	write_wav_head( wave, NULL, 0, pout_buffer, &out_len );
	if( out_len > 0 )
	{
		fstreamsrc = fopen("d:\\file_cap_src.wav","wb+");
		if( fstreamsrc )
			fwrite( pout_buffer, 1, out_len, fstreamsrc );
	}
#endif

	*/

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap::OpenAudio(%s) success. ch=%d, bitSize=%d, SampleRate=%d, ByteRate=%d\r\n", sName, channels, BitsSample, samplerate, nAvgBytesPerSec );
    return 1;

label_OpenAudio_Failed:

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap::OpenAudio(%s) failed! ch=%d, bitSize=%d, SampleRate=%d, ByteRate=%d, reason=%s\r\n", sName, channels, BitsSample, samplerate, nAvgBytesPerSec, reason.empty()?"":reason.c_str() );
	return 0;
}

void AudioCap::CloseAudio()
{
	//StopCap();
	ReleaseRes();
}

/* jyc20170323 remove
void AudioCap::ConfigureSampleAudioGrabber(ISampleGrabber *sampGrabber)
{
    HRESULT hr = S_OK;
    AM_MEDIA_TYPE  media;
      // Set the media type to Audio/WAVE
     ZeroMemory(&media, sizeof(AM_MEDIA_TYPE));
     media.majortype = MEDIATYPE_Audio;
     media.subtype = MEDIASUBTYPE_PCM;
     media.formattype = FORMAT_WaveFormatEx;    
     hr = sampGrabber->SetMediaType(&media);
     if(hr<0)
     {
         return;
     }
     MYFREEMEDIATYPE(media);
     // Configure the samplegrabber
     hr = sampGrabber->SetCallback(this, 1);
     if(hr<0)
     {
         return;
     }
}

HRESULT STDMETHODCALLTYPE AudioCap::SampleCB(double SampleTime, IMediaSample *pSample)
{
    return 0;
}
HRESULT STDMETHODCALLTYPE AudioCap::BufferCB(double SampleTime,BYTE *pBuffer, long BufferLen)
{
#if defined(defTest_AudioSaveCapWav)
	if( fstreamsrc && BufferLen>0 )
		fwrite( pBuffer, 1, BufferLen, fstreamsrc );
#endif

	if( m_cb )
	{
		m_cb->OnAudioBufferCB( SampleTime, pBuffer, BufferLen );
	}

    return 0;
}

bool AudioCap::StartCap()
{
	if (g_CapObj(m_capobj)->m_pMediaControl)
	{
		HRESULT hr = g_CapObj(m_capobj)->m_pMediaControl->Run();
		if( S_OK != hr )
		{
			return false;
		}

		return true;
	}

	return false;
}

void AudioCap::StopCap()
{
	if (g_CapObj(m_capobj)->m_pMediaControl)
	{
		g_CapObj(m_capobj)->m_pMediaControl->Stop();
	}
}

bool AudioCap::GetDevices( std::list<std::string> &devs )
{
   // enumerate all capture devices
    CComPtr<ICreateDevEnum> pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                    IID_ICreateDevEnum, (void**)&pCreateDevEnum);

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory,
        &pEm, 0);
    if (hr != NOERROR) return 0;

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
        IPropertyBag *pBag=0;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
            if(hr == NOERROR)
            {
                char sName[1024];
                    //获取设备名称            
                 WideCharToMultiByte(CP_ACP,0,var.bstrVal,-1,sName, 1024 ,"",NULL);
                 devs.push_back( std::string(sName) );
                 SysFreeString(var.bstrVal);                
             }
             pBag->Release();
         }
         pM->Release();
    }

    pCreateDevEnum = NULL;
    pEm = NULL;

    return 1;
}
*/
