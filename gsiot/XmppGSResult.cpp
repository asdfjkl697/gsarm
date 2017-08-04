#include "XmppGSResult.h"
#include "common.h"
#include "gloox/util.h"
#include "GSIOTClient.h"

XmppGSResult::XmppGSResult( const std::string &from_namespace, const defGSReturn code, const defNormResultMod_ module, const std::string &message )
	: StanzaExtension(ExtIotResult), m_namespace(from_namespace), m_code(code), m_module(module)
{
}

XmppGSResult::XmppGSResult( const Tag* tag )
	: StanzaExtension(ExtIotResult), m_code(defGSReturn_Null)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_RESULT)
		return;

}

XmppGSResult::~XmppGSResult(void)
{
}

void XmppGSResult::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSResult::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_RESULT + "']";
	return filter;
}

Tag* XmppGSResult::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_RESULT );

	Tag *t = new Tag( i,"result" );

	new Tag( t, "namespace", this->m_namespace );

	if( defNormResultMod_null != this->m_module )
	{
		new Tag( t, "module", util::int2string(this->m_module) );
	}

	new Tag( t, "code", util::int2string(this->m_code) );
	if( this->m_message.empty() )
	{
		if( macGSSucceeded(m_code) )
		{
			new Tag( t, "message", std::string(defGSReturnStr_Succeed) );
		}
		else
		{
			new Tag( t, "message", std::string(defGSReturnStr_Fail) );
		}
		//new Tag( t, "message", g_Trans_GSReturn( m_code ) );
	}
	else
	{
		new Tag( t, "message", this->m_message );
	}

	return i;
}
