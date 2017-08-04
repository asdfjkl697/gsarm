#ifndef GSIOTINFO_H_
#define GSIOTINFO_H_

#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "GSIOTDevice.h"

using namespace gloox;

class GSIOTInfo:public StanzaExtension
{
private:
	std::list<GSIOTDevice *> m_deviceList;
	bool m_isInputDevice;
	std::set<IOTDeviceType> m_getForType;

public:
	GSIOTInfo(std::list<GSIOTDevice *>& deviceList, bool isInputDevice=true);
	GSIOTInfo( const Tag* tag = 0 );
	~GSIOTInfo(void);

	virtual const std::string& filterString() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* newInstance( const Tag* tag ) const
    {
        return new GSIOTInfo( tag );
    }

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

    // reimplemented from StanzaExtension
    virtual Tag* tag() const;

    // reimplemented from StanzaExtension
    virtual StanzaExtension* clone() const
    {
        return new GSIOTInfo( *this );
    }

	bool isAllType() const
	{
		return m_getForType.empty();
	}

	bool isInGetType( IOTDeviceType type ) const
	{
		return ( m_getForType.find(type) != m_getForType.end() );
	}
};

#endif
