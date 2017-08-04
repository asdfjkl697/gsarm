#ifndef MEDIACONTROL_H
#define MEDIACONTROL_H

#include "gloox/tag.h"
#include "ControlBase.h"

using namespace gloox;

class MediaControl:public ControlBase
{
public:
	MediaControl(const std::string& protocol,
	const std::string& state,
	const std::string& url,
	const std::string& stream,
	const std::string& ver);
	
	MediaControl( const Tag* tag = 0 );
	~MediaControl(void);

	
	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_Video;
	}

	virtual const std::string& GetName() const
	{
		return this->m_name;
	}

	virtual void SetName( const std::string &name )
	{
		m_name = name;
	}

	const std::string& getProtocol() const
	{
		return this->m_protocol;
	}
	const std::string& getStatus() const{
		return this->m_state;
	}

	const std::string& getUrl() const{
		return this->m_url;
	}
	const std::string& getStream() const{
		return this->m_stream;
	}
	const std::string& getVer() const{
		return this->m_ver;
	}

	Tag* tag(const struTagParam &TagParam) const;
		
	virtual ControlBase* clone( bool CreateLock=true ) const{
	    return new MediaControl(*this);
	}

private:
	std::string m_name;
	std::string m_protocol;
	std::string m_ver;
	std::string m_url;
	std::string m_stream;
	std::string m_state;
};

#endif