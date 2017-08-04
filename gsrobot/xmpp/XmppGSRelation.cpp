#include "XmppGSRelation.h"
#include "../gsiot/GSIOTClient.h"
#include "gloox/util.h"


XmppGSRelation::XmppGSRelation( const struTagParam &TagParam, IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList, defGSReturn result )
	:StanzaExtension(ExtIotRelation), m_result(result), m_TagParam(TagParam), m_device_type(device_type), m_device_id(device_id), m_ChildList(ChildList)
{
}

XmppGSRelation::XmppGSRelation( const Tag* tag )
	:StanzaExtension(ExtIotRelation), m_result(defGSReturn_Null), m_device_type(IOT_DEVICE_Unknown), m_device_id(0)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_RELATION )
		return;

	Tag *tmgr = tag->findChild("relation");

	if( tmgr )
	{
		Tag *tDev = tmgr->findChild("device");
		if( tDev )
		{
			if( tDev->hasAttribute("type") )
			{
				m_device_type = (IOTDeviceType)atoi( tDev->findAttribute("type").c_str() );
			}

			if( tDev->hasAttribute("id") )
			{
				m_device_id = atoi( tDev->findAttribute("id").c_str() );
			}
		}

		Tag *tchildlist = tmgr->findChild("child");
		if( tchildlist )
		{
			const TagList& l = tchildlist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				Tag *tChildDev = (*it);
				if( tChildDev->name() == "device" )
				{
					if( tChildDev->hasAttribute("type") && tChildDev->hasAttribute("id") )
					{
						IOTDeviceType child_dev_type = (IOTDeviceType)atoi( tChildDev->findAttribute("type").c_str() );
						int child_dev_id = atoi( tChildDev->findAttribute("id").c_str() );
						int child_dev_subid = tChildDev->hasAttribute("sid") ? atoi( tChildDev->findAttribute("sid").c_str() ):0;

						m_ChildList.push_back( struRelationChild(child_dev_type,child_dev_id,child_dev_subid) );
					}
				}
			}
		}
	}
}

XmppGSRelation::~XmppGSRelation(void)
{
}

void XmppGSRelation::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSRelation::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_RELATION + "']";
	return filter;
}

Tag* XmppGSRelation::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_RELATION );

	Tag *tmgr = new Tag( i,"relation" );

	Tag *tDev = new Tag( tmgr, "device" );
	tDev->addAttribute( "type", (int)m_device_type );
	tDev->addAttribute( "id", (int)m_device_id );

	if( m_TagParam.isValid && m_TagParam.isResult )
	{
		if( !macGSIsReturnNull(m_result) )
		{
			if( macGSSucceeded(m_result) )
			{
				new Tag( i, "result", std::string(defGSReturnStr_Succeed) );
			}
			else
			{
				new Tag( i, "result", std::string(defGSReturnStr_Fail) );
				new Tag( i, "errcode", util::int2string(m_result) );
			}
		}

		return i;
	}

	Tag *tchildlist = new Tag( tmgr,"child" );
	for( deflstRelationChild::const_iterator it=m_ChildList.begin(); it!=m_ChildList.end(); ++it )
	{
		Tag *tChildDev = new Tag( tchildlist, "device" );
		tChildDev->addAttribute( "type", (int)it->child_dev_type );
		tChildDev->addAttribute( "id", (int)it->child_dev_id );
		if( it->child_dev_subid ) tChildDev->addAttribute( "sid", (int)it->child_dev_subid );
	}

	return i;
}
