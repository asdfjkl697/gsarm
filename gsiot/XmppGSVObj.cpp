#include "XmppGSVObj.h"
#include "GSIOTClient.h"
#include "gloox/util.h"
#include "VObjClass.h"


XmppGSVObj::XmppGSVObj( const struTagParam &TagParam, const std::string &srcmethod, const defmapVObjConfig &VObjCfgList, defGSReturn result )
	:StanzaExtension(ExtIotVObj), m_result(result), m_TagParam(TagParam), m_srcmethod(srcmethod), m_VObjCfgList(VObjCfgList)
{
}

XmppGSVObj::XmppGSVObj( const Tag* tag )
	:StanzaExtension(ExtIotVObj), m_result(defGSReturn_Err), m_method(defCfgOprt_Unknown)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_VObj )
		return;

	Tag *tmgr = tag->findChild("vobjmgr");
	if( tmgr )
	{
		Tag *ttypelist = tmgr->findChild("typelist");
		if( ttypelist )
		{
			const TagList& l = ttypelist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				if((*it)->name() != "type")
					continue;

				m_getForType.insert( (IOTDeviceType)atoi( (*it)->cdata().c_str() ) );
			}
		}

		m_srcmethod = tmgr->findAttribute("method");
		std::string method = m_srcmethod;
		g_toLowerCase( method );
		if( method == "add" )
		{
			m_method = defCfgOprt_Add;
		}
		else if( method == "edit" )
		{
			m_method = defCfgOprt_Modify;
		}
		else if( method == "delete" )
		{
			m_method = defCfgOprt_Delete;
		}

		Tag *tvobjlist = tmgr->findChild("vobjlist");
		if( tvobjlist )
		{
			const TagList& l = tvobjlist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				if((*it)->name() != "vobj")
					continue;

				stru_vobj_config vobj_config;
				const defGSReturn ret = CVObj_common::Untag( *it, vobj_config );
				if( macGSSucceeded(ret) )
				{
					m_VObjCfgList[vobj_config.id] = vobj_config;

					if( defCfgOprt_AddModify == m_method )
						break;
				}
				else
				{
					m_result = ret;
				}
			}
		}
	}
}

XmppGSVObj::~XmppGSVObj(void)
{
}

void XmppGSVObj::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSVObj::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_VObj + "']";
	return filter;
}

Tag* XmppGSVObj::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_VObj );

	Tag *tmgr = new Tag( i,"vobjmgr" );
	
	if( m_TagParam.isValid && m_TagParam.isResult )
	{
		new Tag( i, "retcode", util::int2string(m_result) );
		//return i;
	}

	Tag *tvobjlist = new Tag( tmgr,"vobjlist" );
	for( defmapVObjConfig::const_iterator it=m_VObjCfgList.begin(); it!=m_VObjCfgList.end(); ++it )
	{
		tvobjlist->addChild( CVObj_common::tag( it->second, m_TagParam ) );
	}

	return i;
}
