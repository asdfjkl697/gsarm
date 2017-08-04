#include "MediaControl.h"


MediaControl::MediaControl(const std::string& protocol,
	const std::string& state,
	const std::string& url,
	const std::string& stream,
	const std::string& ver)
	: m_protocol(protocol),m_state(state),m_url(url),m_stream(stream),m_ver(ver)
{
}

MediaControl::~MediaControl(void)
{

}

MediaControl::MediaControl(const Tag* tag )
	:ControlBase(tag)
{
	if( !tag || tag->name() != defDeviceTypeTag_media )
      return;
	
	this->m_protocol = tag->findAttribute("protocol");
	this->m_ver = tag->findAttribute("ver");

	this->m_url= tag->findChild("url")->cdata();
	this->m_stream= tag->findChild("stream")->cdata();
	this->m_state= tag->findChild("state")->cdata();
}

Tag* MediaControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_media );
	i->addAttribute("protocol",this->m_protocol);
	i->addAttribute("ver",this->m_ver);

	new Tag(i,"url",this->m_url);
	new Tag(i,"stream",this->m_stream);
	new Tag(i,"state",this->m_state);

	return i;
}
