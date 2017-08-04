#ifndef GSIOTDEVICEINFO_H_
#define GSIOTDEVICEINFO_H_

#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "GSIOTDevice.h"

using namespace gloox;

class GSIOTDeviceInfo:public StanzaExtension
{
private:
	GSIOTDevice *m_device;
	bool m_isInputDevice;
	defUserAuth m_Auth;
	int m_share;

public:
	GSIOTDeviceInfo(GSIOTDevice *device, defUserAuth Auth, int share, bool isInputDevice=true);
	GSIOTDeviceInfo( const Tag* tag = 0 );
	~GSIOTDeviceInfo(void);

	GSIOTDevice *GetDevice()
	{
		return this->m_device;
	}

	bool isShare() const;
	
	virtual const std::string& filterString() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* newInstance( const Tag* tag ) const
    {
        return new GSIOTDeviceInfo( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

    // reimplemented from StanzaExtension
    virtual Tag* tag() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* clone() const
    {
        return new GSIOTDeviceInfo( *this );
    }
};

#endif
