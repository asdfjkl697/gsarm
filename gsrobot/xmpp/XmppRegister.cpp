#include "XmppRegister.h"
#include "../typedef.h"

XmppRegister::XmppRegister(const std::string& username,const std::string& password)
	:m_username(username), m_password(password), m_client(NULL), m_reg(NULL)
{
}


XmppRegister::~XmppRegister(void)
{
	if(m_reg){
      delete( m_reg );
	}
	if(m_client){
      delete( m_client );
	}
}

void XmppRegister::start()
{
	  m_client = new Client(XMPP_SERVER_DOMAIN);
      m_client->disableRoster();
      m_client->registerConnectionListener( this );

      m_reg = new Registration( m_client );
      m_reg->registerRegistrationHandler( this );
	  
      m_client->connect();
}

void XmppRegister::handleRegistrationFields( const JID& /*from*/, int fields, std::string instructions )
{
	RegistrationFields vals;
	vals.username = this->m_username;
	vals.password = this->m_password;
	m_reg->createAccount(fields, vals);
}

void XmppRegister::handleRegistrationResult( const JID& /*from*/, RegistrationResult result )
{
    LOGMSG( "result: %d\n", result );
	this->m_result = result;
    m_client->disconnect();
}
