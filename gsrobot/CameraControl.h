#ifndef CAMERACONTROL_H_
#define CAMERACONTROL_H_

#include "gloox/tag.h"
#include "ControlBase.h"
#include "CameraPTZ.h"
#include "CameraFocal.h"
#include "EditAttrMgr.h"
#include "gloox/mutexguard.h"

using namespace gloox;

class IPCameraBase;
class CameraControl:public ControlBase, public EditAttrMgr
{
private:	
	IPCameraBase* m_pCamBase;
	
	gloox::util::Mutex m_camctl_mutex;

	int m_reboot;
	bool m_HasCmdCtl;

	std::string m_name;
	std::string m_protocol;
	std::string m_ver;
	std::string m_FromUrl;
	std::vector<std::string> m_url_backup;
	std::string m_state;
	CameraPTZ m_PTZ;
	CameraFocal m_Focal;

	GSPtzFlag m_PTZFlag;
	GSFocalFlag m_FocalFlag;
	
	uint32_t m_buffertime;

	GSPTZ_CtrlCmd m_trackCtl;
	int m_trackX;
	int m_trackY;
	int m_trackEndX;
	int m_trackEndY;

public:
	CameraControl(IPCameraBase* pCamBase, const std::string& protocol,const std::string& ver,const std::string& url,const std::string& state,
		GSPtzFlag PTZFlag, GSFocalFlag FocalFlag);	
	CameraControl( const Tag* tag );
	~CameraControl(void);

	IPCameraBase* hasCamBase() const
	{
		return m_pCamBase;
	}

	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_Camera;
	}

	virtual const std::string& GetName() const
	{
		return this->m_name;
	}

	virtual void SetName( const std::string &name )
	{
		m_name = name;
	}

	const std::string& getProtocol()const
	{
		return this->m_protocol;
	}

	const std::string getStatus();
	void setStatus( const std::string& state );

	const std::string getFromUrl();
	//void setUrl( const std::string& url );

	const std::vector<std::string>& get_url_backup() const
	{
		return this->m_url_backup;
	}

	const std::string& getVer() const{
		return this->m_ver;
	}
	
	const CameraPTZ *getPtz() const{
		return &m_PTZ;
	}
	const CameraFocal *getFocal() const{
		return &m_Focal;
	}

	void setBufferTime( uint32_t buffertime )
	{
		m_buffertime = buffertime;
	}

	uint32_t getBufferTime()
	{
		return m_buffertime;
	}

	void setPTZFlag( GSPtzFlag PTZFlag )
	{
		m_PTZFlag = PTZFlag;
	}

	GSPtzFlag getPTZFlag()
	{
		return m_PTZFlag;
	}

	void setFocalFlag( GSFocalFlag FocalFlag )
	{
		m_FocalFlag = FocalFlag;
	}

	GSFocalFlag getFocalFlag()
	{
		return m_FocalFlag;
	}

	GSPTZ_CtrlCmd get_trackCtl() const
	{
		return m_trackCtl;
	}

	int get_trackX() const
	{
		return m_trackX;
	}

	int get_trackY() const
	{
		return m_trackY;
	}

	int get_trackEndX() const
	{
		return m_trackEndX;
	}

	int get_trackEndY() const
	{
		return m_trackEndY;
	}

	bool HasCmdCtl() const
	{
		return m_HasCmdCtl;
	}

	bool isReboot() const
	{
		return ( GSPTZ_CameraReboot == m_reboot );
	}

	Tag* tag(const struTagParam &TagParam) const;

	virtual ControlBase* clone( bool CreateLock=true ) const;
};

#endif

