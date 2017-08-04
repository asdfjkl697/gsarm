#ifndef GSIOTCONTROL_H_
#define GSIOTCONTROL_H_

#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "GSIOTDevice.h"

using namespace gloox;

class GSIOTControl:public StanzaExtension
{
private:
	GSIOTDevice *m_device;
	int m_NeedRet;
	defUserAuth m_Auth;
	bool m_isInputDevice;

public:
	GSIOTControl(GSIOTDevice *device, defUserAuth Auth=defUserAuth_RW, bool isInputDevice=true);
	GSIOTControl( const Tag* tag = 0 );
	~GSIOTControl(void);
		
	GSIOTDevice* getDevice()
	{
		return this->m_device;
	}

	int getNeedRet() const
	{
		return m_NeedRet;
	}

    // reimplemented from StanzaExtension
    virtual const std::string& filterString() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* newInstance( const Tag* tag ) const
    {
        return new GSIOTControl( tag );
    }

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* clone() const
    {
        return new GSIOTControl( *this );
    }

    // reimplemented from StanzaExt
	virtual Tag* tag() const;
};

#endif

