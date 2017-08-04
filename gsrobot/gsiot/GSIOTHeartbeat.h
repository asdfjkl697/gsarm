#ifndef GSIOTHEARTBEAT_H_
#define GSIOTHEARTBEAT_H_

#include "../typedef.h"
#include "gloox/stanzaextension.h"

using namespace gloox;

class GSIOTHeartbeat:public StanzaExtension
{
private:
	std::string m_ver;
	int m_deviceID;

public:
	GSIOTHeartbeat(int deviceID,const std::string& ver);
	GSIOTHeartbeat( const Tag* tag = 0 );
	~GSIOTHeartbeat(void);
	
	int GetDeviceID()
	{
		return this->m_deviceID;
	}

	virtual const std::string& filterString() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* newInstance( const Tag* tag ) const
    {
        return new GSIOTHeartbeat( tag );
    }

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

    // reimplemented from StanzaExtension
    virtual Tag* tag() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* clone() const
    {
        return new GSIOTHeartbeat( *this );
    }
};

#endif

