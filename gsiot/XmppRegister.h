#ifndef XMPPREGISTER_H_
#define XMPPREGISTER_H_

#include "common.h"
#include "gloox/client.h"
#include "gloox/connectionlistener.h"
#include "gloox/registration.h"

using namespace gloox;

class XmppRegister:public RegistrationHandler, ConnectionListener
{
private:
    Registration *m_reg;
    Client *m_client;
	std::string m_username;
	std::string m_password;
	RegistrationResult m_result;

public:
	XmppRegister(const std::string& username,const std::string& password);
	~XmppRegister(void);

	virtual void onConnect()
	{
		m_reg->fetchRegistrationFields();
	}

	virtual bool onTLSConnect( const CertInfo& info )
    {
		return true;
	}
	virtual void onDisconnect( ConnectionError e ) { LOGMSG( "register_test: disconnected: %d\n", e ); }

	virtual void handleRegistrationFields( const JID& /*from*/, int fields, std::string instructions );
	virtual void handleRegistrationResult( const JID& /*from*/, RegistrationResult result );
	virtual void handleAlreadyRegistered( const JID& /*from*/ )
    {
       LOGMSG( "the account already exists.\n" );
    }

	virtual void handleDataForm( const JID& /*from*/, const DataForm& /*form*/ )
    {
      LOGMSG( "datForm received\n" );
    }

    virtual void handleOOB( const JID& /*from*/, const OOB& oob )
    {
      LOGMSG( "OOB registration requested. %s: %s\n", oob.desc().c_str(), oob.url().c_str() );
    }

	void start();
	bool getState()
	{
		return this->m_result == RegistrationSuccess;
	}
};

#endif

