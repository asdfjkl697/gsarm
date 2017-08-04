#include "XmppGSPreset.h"
#include "gloox/util.h"
#include "../gsiot/GSIOTClient.h"

XmppGSPreset::XmppGSPreset( const struTagParam &TagParam, const std::string &srcmethod, IOTDeviceType device_type, int device_id, const defPresetQueue &PresetList, defGSReturn result )
	:StanzaExtension(ExtIotPreset), m_method(defPSMethod_Unknown), m_TagParam(TagParam), m_srcmethod(srcmethod), m_device_type(device_type), m_device_id(device_id), m_PresetList(PresetList), m_result(result)
{
}

XmppGSPreset::XmppGSPreset( const Tag* tag )
	:StanzaExtension(ExtIotPreset), m_method(defPSMethod_Unknown), m_device_type(IOT_DEVICE_Unknown), m_device_id(0), m_result(defGSReturn_Null)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_Preset )
		return;

	Tag *tmgr = tag->findChild("preset");

	if( tmgr )
	{
		if(tmgr->hasAttribute("method"))
		{
			m_srcmethod = tmgr->findAttribute("method");
			std::string method = m_srcmethod;
			g_toLowerCase( method );
			if( method == "goto" )
			{
				m_method = defPSMethod_goto;
			}
			else if( method == "add" )
			{
				m_method = defPSMethod_add;
			}
			else if( method == "del" )
			{
				m_method = defPSMethod_del;
			}
			else if( method == "edit" )
			{
				m_method = defPSMethod_edit;
			}
			else if( method == "setnew" )
			{
				m_method = defPSMethod_setnew;
			}
			else if( method == "sort" )
			{
				m_method = defPSMethod_sort;
			}
		}

		Tag *tDev = tmgr->findChild("device");
		if( tDev )
		{
			if( tDev->hasAttribute("type") )
			{
				m_device_type = (IOTDeviceType)atoi( tDev->findAttribute("type").c_str() );
			}
			else
			{
				m_device_type = IOT_DEVICE_Camera;
			}

			if( tDev->hasAttribute("id") )
			{
				m_device_id = atoi( tDev->findAttribute("id").c_str() );
			}
		}

		Tag *tPresetlist = tmgr->findChild("presetlist");
		if( tPresetlist )
		{
			const TagList& l = tPresetlist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				CPresetObj *pPreset = new CPresetObj(*it);
				m_PresetList.push_back( pPreset );
			}
		}
	}
}

XmppGSPreset::~XmppGSPreset(void)
{
	CPresetManager::DeletePresetQueue_Spec( m_PresetList );
}

void XmppGSPreset::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSPreset::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_Preset + "']";
	return filter;
}

Tag* XmppGSPreset::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_Preset );

	Tag *tmgr = new Tag( i,"preset" );

	if( !m_srcmethod.empty() )
	{
		tmgr->addAttribute( "method", m_srcmethod );
	}

	Tag *tDev = new Tag( tmgr, "device" );
	tDev->addAttribute( "type", (int)m_device_type );
	tDev->addAttribute( "id", (int)m_device_id );

	if( m_TagParam.isValid && m_TagParam.isResult )
	{
		if( !macGSIsReturnNull(m_result) )
		{
			new Tag( tmgr, "retcode", util::int2string(m_result) );
		}

		//return i;
	}

	if( !m_PresetList.empty() )
	{
		Tag *tPresetlist = new Tag( tmgr,"presetlist" );
		for( defPresetQueue::const_iterator it=m_PresetList.begin(); it!=m_PresetList.end(); ++it )
		{
			tPresetlist->addChild( (*it)->tag(m_TagParam) );
		}
	}

	return i;
}
