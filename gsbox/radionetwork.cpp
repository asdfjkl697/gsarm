#include "radionetwork.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"

#define RADIO_CONFIG_FILE "radio.xml"

#define REG_PKTCTRL1 0x7


enum RADIO_ROLE
{
	ROLE_MAIN = 0x1,
	ROLE_CHILD = 0x2
};

enum RADIO_COMMAND
{
    RD_DISCOVERY = 0x1,
	RD_DIS_REPLY = 0x2,
	RD_DIS_REPLY_ACK = 0x3,
	RD_COM = 0x50,
	RD_COM_REPLY = 0x5B,
	RD_BIND = 0x80,
	RD_UNBIND = 0x88,
	RD_GETCHILD = 0x90,
	RD_GETCHILD_REPLY = 0x9B,
	RD_HEARTBEAT = 0xF0,
	RD_HEARTBEAT_ACK = 0xFC,
	RD_PACK_EXT = 0x8F,
	RD_DIS_START = 0xD0,
	RD_DIS_CHILD = 0xDC
};

enum RADIO_DISCOVERY
{
	DIS_DONE     = 0x0,
	DIS_FIND     = 0x1,
	DIS_PROGRESS = 0x2
};

RadioNetwork::RadioNetwork(BoardHandler *board, TaskManager *tm)
:state(0x1), role(0), addr(0), channel(0), _hasheatbeart(false), 
boardhandler(board), task(tm)
{
	syncHandler = new RadioSyncHandler(this, tm);
	rdHandler = NULL;
	lastpacket = NULL;
	loadConfig();
	setAddrFreq(this->role, this->addr, this->channel);
	headBeart();
}

RadioNetwork::~RadioNetwork(void)
{
	if(lastpacket != NULL){
		delete(lastpacket);
	}
	delete(syncHandler);
	stopHeadBeart();
	cleanChild();
    xmlCleanupParser();
}

void RadioNetwork::loadConfig()
{
    	xmlDocPtr doc;
	xmlNodePtr cur;
	
	doc = xmlParseFile(RADIO_CONFIG_FILE);
	if (doc == NULL) return;

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL){
		xmlFreeDoc(doc);
		return;
	}

	if (!xmlStrEqual(cur->name, BAD_CAST"radio")){
		xmlFreeDoc(doc);
		return;
	}

	if (xmlHasProp(cur, BAD_CAST"id")){
		this->id = atoi((char *)xmlGetProp(cur, BAD_CAST"id"));
	}
	if (xmlHasProp(cur, BAD_CAST"state")){
		this->state = atoi((char *)xmlGetProp(cur, BAD_CAST"state"));
	}
	if (xmlHasProp(cur, BAD_CAST"dis_addr")){
		this->dis_addr = atoi((char *)xmlGetProp(cur, BAD_CAST"dis_addr"));
	}
	if (xmlHasProp(cur, BAD_CAST"addr")){
		this->addr = atoi((char *)xmlGetProp(cur, BAD_CAST"addr"));
	}
	if (xmlHasProp(cur, BAD_CAST"channel")){
		this->channel = atoi((char *)xmlGetProp(cur, BAD_CAST"channel"));
	}
	if (xmlHasProp(cur, BAD_CAST"role")){
		this->role = atoi((char *)xmlGetProp(cur, BAD_CAST"role"));
	}


	RADIO_CHILD *cl = this->child;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (xmlStrEqual(cur->name, BAD_CAST"child")){
			if (cl == NULL){
				cl = this->child = (RADIO_CHILD *)malloc(sizeof(RADIO_CHILD));
			}
			else{
				cl = cl->next = (RADIO_CHILD *)malloc(sizeof(RADIO_CHILD));
			}

			cl->next = NULL;
			
			if (xmlHasProp(cur, BAD_CAST"id")){
				this->child_id = cl->id = atoi((char *)xmlGetProp(cur, BAD_CAST"id"));
			}
			if (xmlHasProp(cur, BAD_CAST"addr")){
				cl->addr = atoi((char *)xmlGetProp(cur, BAD_CAST"addr"));
			}
			if (xmlHasProp(cur, BAD_CAST"channel")){
				cl->channel = atoi((char *)xmlGetProp(cur, BAD_CAST"channel"));
			}
			if (xmlHasProp(cur, BAD_CAST"role")){
				cl->role = atoi((char *)xmlGetProp(cur, BAD_CAST"role"));
			}
			if (xmlHasProp(cur, BAD_CAST"recv_rssi")){
				cl->recv_rssi = atoi((char *)xmlGetProp(cur, BAD_CAST"recv_rssi"));
			}
			if (xmlHasProp(cur, BAD_CAST"send_rssi")){
				cl->send_rssi = atoi((char *)xmlGetProp(cur, BAD_CAST"send_rssi"));
			}
			if (xmlHasProp(cur, BAD_CAST"ipver")){
				cl->ipver = atoi((char *)xmlGetProp(cur, BAD_CAST"ipver"));
			}
			if (xmlHasProp(cur, BAD_CAST"mac")){
				char *mac = (char *)xmlGetProp(cur, BAD_CAST"mac");
				memcpy(cl->mac, mac, strlen(mac));
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
}

void RadioNetwork::saveXml()
{
	xmlDocPtr doc;
	xmlNodePtr curNode = NULL, child = NULL;
	char temp[64];
	doc = xmlNewDoc(BAD_CAST "1.0");
	curNode = xmlNewNode(NULL, BAD_CAST "radio");
	sprintf(temp,"%d",this->id);
	xmlSetProp(curNode, BAD_CAST"id", BAD_CAST temp);
	sprintf(temp,"%d",this->state);
	xmlSetProp(curNode, BAD_CAST"state", BAD_CAST temp);
	sprintf(temp,"%d",this->dis_addr);
	xmlSetProp(curNode, BAD_CAST"dis_addr", BAD_CAST temp);
	sprintf(temp,"%d",this->addr);
	xmlSetProp(curNode, BAD_CAST"addr", BAD_CAST temp);
	sprintf(temp,"%d",this->channel);
	xmlSetProp(curNode, BAD_CAST"channel", BAD_CAST temp);
	sprintf(temp,"%d",this->role);
	xmlSetProp(curNode, BAD_CAST"role", BAD_CAST temp);
	xmlDocSetRootElement(doc, curNode);

	RADIO_CHILD *cl = this->child;

	while(cl != NULL){
		child = xmlNewNode(NULL, BAD_CAST "child");
		sprintf(temp,"%d",cl->id);
		xmlSetProp(child, BAD_CAST"id", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->state);
		xmlSetProp(child, BAD_CAST"state", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->addr);
		xmlSetProp(child, BAD_CAST"addr", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->channel);
		xmlSetProp(child, BAD_CAST"channel", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->role);
		xmlSetProp(child, BAD_CAST"role", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->recv_rssi);
		xmlSetProp(child, BAD_CAST"recv_rssi", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->send_rssi);
		xmlSetProp(child, BAD_CAST"send_rssi", BAD_CAST temp);
	    	sprintf(temp,"%d",cl->ipver);
		xmlSetProp(child, BAD_CAST"ipver", BAD_CAST temp);
		xmlSetProp(child, BAD_CAST"mac", BAD_CAST cl->mac);
		xmlAddChild(curNode, child);

		cl = cl->next;
	}

	xmlSaveFileEnc(RADIO_CONFIG_FILE, doc, "UTF-8");
	xmlFreeDoc(doc);
}

uint8_t RadioNetwork::updateChild(int rid)
{
   RADIO_CHILD *cl = this->child;
   while(cl != NULL){
	   if(cl->id == rid){
		   cl->state = 0x1;
		   cl->timeout_count = 0;
		   return cl->addr;
	   }

	   cl = cl->next;
   }

   return 0;
}

RADIO_CHILD* RadioNetwork::getChild(RADIO_CHILD *child, uint16_t rid, uint16_t level)
{	
	RADIO_CHILD *cl = child->child;
	while(cl != NULL){
	   if(level > 0){
		   RADIO_CHILD *cl_child = getChild(cl, rid,level-1);
		   if(cl_child !=NULL){
			   return cl_child;
		   }
	   }else if(cl->id == rid){
		   return cl;
	   }

	   cl = cl->next;
	}

	return NULL;
}

void RadioNetwork::writeChild(Packet *pkt, RADIO_CHILD *cl)
{
	pkt->WriteByte(DATA_OBJECT);	
	pkt->EncodeInt16(cl->id);
	pkt->EncodeByte(cl->addr);
	pkt->EncodeByte(cl->channel);
	pkt->EncodeByte(cl->recv_rssi);
	pkt->EncodeByte(cl->send_rssi);
	pkt->EncodeByte(cl->ipver);
	pkt->EncodeString((const char *)cl->mac);
}

uint8_t RadioNetwork::getChildAddr(uint8_t rm_addr)
{
	if(this->addr == rm_addr){
		rm_addr++;
	}
	
   RADIO_CHILD *cl = this->child;
   while(cl != NULL){
	   if(cl->addr == rm_addr){
		   rm_addr++;
	   }
	   cl = cl->next;
   }
   return rm_addr;
}

void RadioNetwork::removeChild(RADIO_CHILD *child)
{
	RADIO_CHILD *swp;
    RADIO_CHILD *cl = this->child;
	if(cl == child){
		swp = child->next;
		free(child);
		this->child = swp;
		return;
	}
	
    while(cl != NULL){
	   if(cl->next == child){
		   swp = child->next;
		   free(child);
		   cl->next = swp;
		   break;
	   }
	   cl = cl->next;
    }
}

void RadioNetwork::cleanChild()
{
   RADIO_CHILD *cl = this->child;
   while(cl != NULL){
	   RADIO_CHILD *swp = cl;	   
	   cl = swp->next;
	   free(swp);
   }
   this->child = NULL;
}

void RadioNetwork::readConfig(RadioDataHandler *handler, Packet *pkt, uint16_t parent_id, uint16_t level)
{
	this->rdHandler = handler;
	RADIO_CHILD *cl = this->child;
	while(cl != NULL){
		if(level > 0){
			RADIO_CHILD *cl_child = getChild(cl, parent_id, level - 1);
			if(cl_child != NULL){
			    while(cl_child != NULL){
					 writeChild(pkt, cl_child);
					 cl_child = cl_child->next;
				}
				break;
			}else if(cl->id == parent_id){
				sendGetChildToDevice(cl, parent_id, 0);
				break;
			}
		}else{
			writeChild(pkt, cl);
		}
		cl = cl->next;
	}
	
	pkt->WriteByte(DATA_NONE);
}

RADIO_CHILD* RadioNetwork::findChildDevice(uint16_t parent_id, uint16_t level)
{
	RADIO_CHILD *cl = this->child;
	while(cl != NULL){
		if(level > 0){
			RADIO_CHILD *cl_child = getChild(cl, parent_id, level - 1);
			if(cl_child != NULL){
			    return cl;
			}
		}else if(cl->id == parent_id){
			return cl;
		}
		cl = cl->next;
	}

    return NULL;
}

bool RadioNetwork::sendDataToDevice(RadioDataHandler *handler, Packet *pkt, int length, uint16_t parent_id, uint16_t level)
{
	this->rdHandler = handler;

	RADIO_CHILD *cl = findChildDevice(parent_id, level);
	if(cl != NULL){
		if(length > 25){
			uint8_t *temp = (uint8_t *)malloc(length);
			initLastPacket();
			
			lastpacket->SetAddr(cl->addr);
   	        lastpacket->SetCmd(RD_COM);
			lastpacket->WriteInt16(parent_id);
			lastpacket->WriteByte(level);
			pkt->ReadBuffer(temp,length);
			lastpacket->WriteBuffer(temp, length);
			lastpacket->SetPos(0);

			free(temp);
			
	        syncHandler->Start(PrecessPacketCall, 100);
		}else{
			uint8_t *temp = (uint8_t *)malloc(length);
			pkt->ReadBuffer(temp,length);
			
			RadioPacket rd_pkt(cl->addr, RD_COM);
			rd_pkt.WriteInt16(parent_id);
			rd_pkt.WriteByte(level);
			rd_pkt.WriteBuffer(temp, length);
			free(temp);
			sendRadio(&rd_pkt);
		}
		return true;
	}
	return false;
}

void RadioNetwork::addOrUpdateChild(RADIO_CHILD *child)
{
	RADIO_CHILD *cl = this->child;
	if(cl == NULL){
		this->child = child;
		saveXml();
		return;
	}
	while(cl != NULL){
		if(memcmp(cl->mac, child->mac, strlen((const char *)cl->mac))==0){
			cl->id        = child->id;
			cl->addr      = child->addr;
			cl->channel   = child->channel;
			cl->state     = child->state;
			cl->recv_rssi = child->recv_rssi;
			cl->send_rssi = child->send_rssi;
			break;
		}
		if(cl->next == NULL){
			cl->next = child;
			break;
		}
		cl = cl->next;
	}

	saveXml();
}

bool RadioNetwork::addRadio(NetPacket *pkt)
{
	if(pkt->getLength() > 5){
	    RADIO_CHILD *cl = (RADIO_CHILD *)malloc(sizeof(RADIO_CHILD));
		memset(cl, 0, sizeof(RADIO_CHILD));

	    cl->addr = pkt->DecodeByte();
	    cl->channel = pkt->DecodeByte();
	    cl->recv_rssi = pkt->DecodeByte();
	    cl->send_rssi = pkt->DecodeByte();
	    cl->ipver = pkt->DecodeByte();
		
	    str_st val = {0};
		
		pkt->DecodeString(&val);
		memcpy(cl->mac, val.str, val.len);
		addOrUpdateChild(cl);
		return true;
	}
	return false;
}

bool RadioNetwork::editRadio(uint16_t rid, NetPacket *pkt)
{
	RADIO_CHILD *cl = this->child;	
	while(cl != NULL){
		if(cl->id == rid){
	        cl->addr = pkt->DecodeByte();
	        cl->channel = pkt->DecodeByte();
	        cl->recv_rssi = pkt->DecodeByte();
	        cl->send_rssi = pkt->DecodeByte();
	        cl->ipver = pkt->DecodeByte();
			
	        str_st val = {0};		
	        pkt->DecodeString(&val);
		    memcpy(cl->mac, val.str, val.len);
			saveXml();
			return true;
		}
		cl = cl->next;
	}

	return false;
}

bool RadioNetwork::deleteRadio(uint16_t rid)
{
	RADIO_CHILD *swp;
    RADIO_CHILD *cl = this->child;
	if((cl != NULL) && (cl->id == rid)){
		swp = cl->next;
		free(cl);
		this->child = swp;
		saveXml();
		return true;
	}
	
   while(cl->next != NULL){
	   if(cl->next->id == rid){
		   swp = cl->next->next;
	       free(cl->next);
		   cl->next = swp;
		   saveXml();
		   return true;
	   }
	   cl = cl->next;
   }

   return false;
}

void RadioNetwork::headBeart()
{
	if(!_hasheatbeart && state > 0){
		_hasheatbeart = true;
		task->AddTimer(this, 10000); //10 sec
	}
}

void RadioNetwork::stopHeadBeart()
{
	if(_hasheatbeart){
		task->RemoveTimer(this);
		_hasheatbeart = false;
	}
}

bool RadioNetwork::saveConfig()
{
	saveXml();
	setAddrFreq(this->role, this->addr, this->channel);
	headBeart();
	return true;
}

void RadioNetwork::sendRadio(RadioPacket *rd_pkt)
{
	    uint8_t temp[260] = {0};
	    uint8_t *enc;
	    Packet pkt(64);
	    pkt.WriteByte(0x65);
	    pkt.WriteByte(CC1101);
	    pkt.WriteByte(rd_pkt->GetAddr());
	    pkt.WriteByte(rd_pkt->GetCmd());

	    if(rd_pkt->getLength() > 0){
	       pkt.WriteBuffer(rd_pkt->getBuffer(), rd_pkt->getLength());
	    }
	
	    pkt.SetPos(0);
	    enc = pkt.toHardware(temp);
	
	    if(boardhandler!=NULL){
	       boardhandler->Send(temp, enc - temp);
	    }
}

void RadioNetwork::discoverySend()
{
	RadioPacket pkt(0x0, RD_DISCOVERY);
	pkt.WriteByte(++dis_count);
	pkt.WriteByte(channel);
	pkt.WriteInt16(100);	
	sendRadio(&pkt);
}

void RadioNetwork::setAddrFreq(uint8_t s_role,uint8_t s_addr, uint8_t s_channel)
{
	uint8_t temp[32] = {0};

	Packet pkt(20);
	pkt.WriteByte(0x65);
	pkt.WriteByte(SYSTEM_SET);
	pkt.WriteByte(CC1101);
	pkt.WriteByte(SYSTEM_WIRTE);
	pkt.WriteByte(REG_PKTCTRL1); //start addr
	pkt.WriteByte(0x4);//count
	pkt.WriteByte(0x6);//Address check and 0 (0x00) broadcast/Address check, no broadcast
	pkt.WriteByte(0x5);
	pkt.WriteByte(s_addr);
	pkt.WriteByte(s_channel);
	pkt.SetPos(0);
	
	uint8_t *enc = pkt.toHardware(temp);
	if(boardhandler!=NULL){
		boardhandler->Send(temp, enc - temp);
	}

	usleep(50000);
}


uint8_t RadioNetwork::discovery(RadioDataHandler *handler)
{
	if(this->dis_state == DIS_DONE){
	   this->dis_state = DIS_PROGRESS;
	   this->hb_count  = 0;
	   this->dis_read  = 0;
	   this->rdHandler = handler;

	   setAddrFreq(ROLE_MAIN, 0, 0);
       discoverySend();
	}

	return this->dis_state;
}

void RadioNetwork::sendDiscoveryToDevice(RADIO_CHILD *child, uint16_t sid, uint16_t rid, uint8_t level)
{
	RadioPacket pkt(child->addr, RD_DIS_START);
	pkt.WriteInt16(sid);
	pkt.WriteInt16(rid);
	pkt.WriteByte(level);
	sendRadio(&pkt);
}

void RadioNetwork::sendGetChildToDevice(RADIO_CHILD *child, uint16_t rid, uint8_t level)
{
	RadioPacket pkt(child->addr, RD_GETCHILD);
	pkt.WriteInt16(rid);
	pkt.WriteByte(level);
	sendRadio(&pkt);
}

uint8_t RadioNetwork::childDiscovery(RadioDataHandler *handler, uint16_t sid, uint16_t rid, uint16_t level)
{	
	this->rdHandler = handler;
	RADIO_CHILD *cl = this->child;
	while(cl != NULL){
	    if(level > 0){
            if(getChild(cl, rid, level) != NULL){
			   //send to first device
			   sendDiscoveryToDevice(cl, sid, rid, level-1);
			   return 0x2;
		    }
	    }else if(cl->id == rid){
			//send to this device
			sendDiscoveryToDevice(cl,sid, rid, 0);
			return 0x2;
		}
		cl = cl->next;
	}

	return 0x0;
}

void RadioNetwork::OnNetData(Client *client, NetPacket *pkt)
{
	switch(pkt->ReadByte())
	{
		case 0x1:
			{
				this->state = pkt->DecodeBoolean() ? 0x1: 0x0;
				this->role  = pkt->DecodeByte();
				this->addr = pkt->DecodeByte();
				this->channel = pkt->DecodeByte();

				pkt->SetPos(1);
				pkt->EncodeBoolean(saveConfig());				
				pkt->AutoAck();
		        pkt->EncodeBuffer();
			    client->Send(pkt->getBuffer(), pkt->getLength());
				break;
			}
			case 0x2:
			{
				pkt->SetPos(1);
				pkt->EncodeBoolean(this->state > 0);
				pkt->EncodeByte(this->role);
				pkt->EncodeByte(this->addr);
				pkt->EncodeByte(this->channel);							
				pkt->AutoAck();
		        pkt->EncodeBuffer();
			    client->Send(pkt->getBuffer(), pkt->getLength());
				break;
			}
			case 0x3:
			{
				dis_addr = pkt->DecodeByte();
				dis_channel = pkt->DecodeByte();
				
				pkt->SetPos(1);
				pkt->EncodeByte(this->discovery(client));					
                pkt->AutoAck();
		        pkt->EncodeBuffer();
			    client->Send(pkt->getBuffer(), pkt->getLength());					
				break;
			}
			case 0x4:
			{
				uint8_t opt = pkt->ReadByte();
				uint16_t rid = pkt->DecodeInt16();

				switch(opt){
					case 0:
					{
						uint16_t level = pkt->DecodeInt16();
						pkt->SetPos(pkt->GetPos());
						readConfig(client, pkt, rid, level);						
						break;
					}
					case 0xAA:
					{
						bool state = addRadio(pkt);
						
						pkt->SetPos(2);
				        pkt->EncodeBoolean(state);
						break;
					}
					case 0xEE:
					{
						bool state = editRadio(rid, pkt);
						pkt->SetPos(2);
				        pkt->EncodeBoolean(state);
						break;
					}
					case 0xDD:
					{
						bool state = deleteRadio(rid);
						pkt->SetPos(2);
				        pkt->EncodeBoolean(state);
						break;
					}
				}
					
					
                pkt->AutoAck();
		        pkt->EncodeBuffer();
			    client->Send(pkt->getBuffer(), pkt->getLength());
				break;
			}
			case 0x5:
			{
				uint16_t rid  = pkt->DecodeInt16();
				uint16_t level = pkt->DecodeInt16();

				if(rid > 0){
					uint8_t state = childDiscovery(client, this->child_id, rid, level);
					pkt->SetPos(pkt->GetPos());
					pkt->EncodeByte(state);
					pkt->AutoAck();
		            pkt->EncodeBuffer();
			        client->Send(pkt->getBuffer(), pkt->getLength());
				}
				break;
			}
			case 0xdd:
			{
				uint16_t rid  = pkt->DecodeInt16();
				uint16_t level = pkt->DecodeInt16();
	            uint16_t length = pkt->DecodeInt16();
				bool _state = sendDataToDevice(client, pkt, length, rid, level);
				pkt->SetPos(1);
				pkt->EncodeBoolean(_state);					
				pkt->AutoAck();
		        pkt->EncodeBuffer();
			    client->Send(pkt->getBuffer(), pkt->getLength());
				break;
			}
	}
}

void RadioNetwork::AsyncDiscoveryCall()
{	
	if(dis_read){
		dis_read = 0;
		return;
	}
	
	syncHandler->Stop();

	uint8_t *enc;
	
	RadioPacket pkt(0x0, RD_DIS_REPLY);

	pkt.WriteByte(dis_addr);
	pkt.WriteByte(recv_rssi);
	pkt.WriteByte(0x1);//ipv4=1, ipv6=2
	pkt.EncodeString((const char *)mac);
	sendRadio(&pkt);	
}

void RadioNetwork::AsyncChildDiscoveryCall()
{
	syncHandler->Stop();
	discoveryCallback(true, this->dis_state, this->dis_id, this->dis_level);
}

void RadioNetwork::RadioDiscoveryCall()
{
	syncHandler->Stop();
	discovery(this);
}

void RadioNetwork::AsyncPrecessPacketCall()
{
	int data_length = 0;
	data_length = lastpacket->getLength();

	if(lastpacket->IsDone()){
	    syncHandler->Stop();		
		
		lastpacket->SetPos(0);

		switch(lastpacket->GetCmd())
		{
			case RD_GETCHILD_REPLY:
			{	
				uint16_t rid   = lastpacket->ReadInt16();
				uint8_t  level = lastpacket->ReadByte();
				level++;
		        if(rdHandler !=NULL){
					rdHandler->onRadioConfigRead(lastpacket, rid, level);
				}
				break;
			}
			case RD_COM:
		    {
			    uint16_t rid = lastpacket->ReadInt16();
			    uint8_t level = lastpacket->ReadByte();

			    if(level == 0){
				   if(rid == this->id){
					   uint8_t temp[260] = {0};
			           uint8_t *enc = temp;
			           enc = lastpacket->toHardware(enc);
			           if(boardhandler!=NULL){
				           boardhandler->Send(temp, enc - temp);
			           }
				   }
			    }else{
				    sendDataToDevice(this, lastpacket, lastpacket->getLength(), rid, level);
			    }
			    break;
		    }
		}

		delete(lastpacket);
		lastpacket = NULL;		
		return;
	}
	
	if(data_length > 0){
		RadioPacket rd_pkt(dis_addr, lastpacket->GetCmd());
		if(data_length > 28){
            rd_pkt.SetCmd(RD_PACK_EXT);
			rd_pkt.WriteIndex(lastpacket->GetIndex());
			rd_pkt.WriteByte(lastpacket->GetCmd());
			data_length = 28;
		}else if(lastpacket->IsExPacket()){
			rd_pkt.SetCmd(RD_PACK_EXT);
			rd_pkt.WriteIndex(0);
			rd_pkt.WriteByte(lastpacket->GetCmd());
		}
		uint8_t temp[32] = {0};
		lastpacket->ReadBuffer(temp, data_length);		
		rd_pkt.WriteBuffer(temp, data_length);
		sendRadio(&rd_pkt);
	}else{
	    syncHandler->Stop();
		delete(lastpacket);
		lastpacket = NULL;
	}
}

void RadioNetwork::AsyncComDataCall()
{
	syncHandler->Stop();

	uint16_t rid   = lastpacket->ReadInt16();
	uint8_t  level = lastpacket->ReadByte();

	if(rdHandler !=NULL){
		rdHandler->onRadioComData(lastpacket, rid, level);
	}
}

void RadioNetwork::initLastPacket()
{
	if(lastpacket != NULL){
		delete(lastpacket);
	}
	lastpacket = new RadioPacket();
}

void RadioNetwork::onRadioPacket(RadioPacket *pkt)
{
	switch(pkt->GetCmd())
	{
		case RD_DISCOVERY:
		{
			if(this->state == 0x2 || this->role == ROLE_MAIN) return; //without main
			
			uint8_t rm_addr    = pkt->ReadByte();
			uint8_t rm_channel = pkt->ReadByte();
			uint16_t delay  = pkt->ReadInt16();
			int rssi_db;
			
			this->dis_addr = rm_addr;
			this->dis_channel = rm_channel;
			this->recv_rssi  = pkt->GetRssi();

			if(recv_rssi < 128){
                rssi_db = (recv_rssi / 2) - 74;
            } else {
                rssi_db = ((recv_rssi - 256) / 2) - 74;
            }

			delay += abs(rssi_db) * 5;

			syncHandler->Start(DiscoveryCall, delay);
			break;
		}
		case RD_DIS_REPLY:
		{
			if(dis_state != DIS_PROGRESS) {
				dis_read = 0x1;
				return;
			}
			
			RADIO_CHILD *child = (RADIO_CHILD *)malloc(sizeof(RADIO_CHILD));
			child->next = NULL;
			child->addr = getChildAddr(pkt->ReadByte());
			child->channel = this->dis_channel;
			child->role = ROLE_CHILD;
			child->recv_rssi = pkt->ReadByte();
			child->send_rssi = pkt->GetRssi();
			child->ipver = pkt->ReadByte();
			child->id   = ++this->child_id;
			child->state = 0x1;
			child->timeout_count = 0;
			
			str_st val = {0};
			pkt->DecodeString(&val);
			pkt->stringCopy(child->mac, &val);
			
			//send ack
			RadioPacket packet(0x0, RD_DIS_REPLY_ACK);
			packet.WriteByte(this->addr);
			packet.WriteByte(child->addr);
			packet.WriteByte(child->channel);
			packet.EncodeInt16(child->id);
			packet.WriteByte(child->ipver);
			packet.EncodeString((const char *)child->mac);
			sendRadio(&packet);
/*
			if(radio->dis_callback != NULL){
				radio->dis_child = child;
			    radionetwork_temptask_init(radio);
			    radio->temptask->callback = radionetwork_asyncDiscoeryFindCall;
			    radio->temptask->arg = board;
			    taskmanager_add_timer(board->task, radio->temptask, 100);
			}
*/
			//save and Notification
			addOrUpdateChild(child);
			dis_read = 0x1;	
			break;
		}
		case RD_DIS_REPLY_ACK:
		{
			char temp[18];
			
			this->dis_addr = pkt->ReadByte();
			this->addr     = pkt->ReadByte();
			this->channel  = pkt->ReadByte();
			this->role     = ROLE_CHILD;
			this->id       = pkt->DecodeInt16();

			pkt->ReadByte();

			str_st val = {0};
			pkt->DecodeString(&val);
			pkt->stringCopy((uint8_t *)temp, &val);

			if(memcmp(temp, mac, strlen((const char *)mac))==0){
				this->dis_count = 0;
				this->state = 0x2;
				cleanChild();
				saveXml();
				setAddrFreq(ROLE_CHILD, this->addr, this->channel);
				headBeart();
			}
			break;
		}
		case RD_COM:
		{
			uint16_t rid = pkt->ReadInt16();
			uint8_t level = pkt->ReadByte();

			if(level == 0){
				if(rid == this->id){
					uint8_t temp[260] = {0};
			        uint8_t *enc = temp;
			        enc = pkt->toHardware(enc);
			        if(boardhandler!=NULL){
				        boardhandler->Send(temp, enc - temp);
			        }
				}
			}else{
				sendDataToDevice(this, pkt, pkt->getLength(), rid, level);
			}
			break;
		}			
		case RD_COM_REPLY:
		{
			initLastPacket();
			pkt->ReadPacket(lastpacket);
			lastpacket->SetCmd(RD_COM_REPLY);
			lastpacket->SetPos(0);
			syncHandler->Start(ComDataBackCall, 100);
			break;
		}
		case RD_BIND:
			break;
		case RD_UNBIND:
			break;
		case RD_GETCHILD:
		{
			uint16_t rid = pkt->ReadInt16();
			uint8_t level = pkt->ReadByte();

			if(level == 0){
				if(rid == this->id){
					initLastPacket();
					lastpacket->WriteInt16(rid);
					lastpacket->WriteByte(level);
					lastpacket->SetCmd(RD_GETCHILD_REPLY);		
	                readConfig(this, lastpacket, 0, 0);	
					lastpacket->SetPos(0);
					syncHandler->Start(PrecessPacketCall, 100);
				}
			}else{
				//readConfig(pkt, rid, level);
			}
			break;
		}
		case RD_HEARTBEAT_ACK:
		case RD_HEARTBEAT:
		{
		    uint16_t rmid = pkt->DecodeInt16();
			if(rmid > 0){
				uint8_t rm_addr = 0;
				if(this->role == ROLE_MAIN){
					//update child state
					rm_addr = updateChild(rmid);
				}else{
					if(this->id == rmid){
					    this->hb_count = 0; //delay headbeart
						this->dis_count = 0;
						rm_addr = this->dis_addr;
					}else{
						//update child state
						rm_addr = updateChild(rmid);
					}
				}			

				if((pkt->GetCmd()==RD_HEARTBEAT) && (rm_addr != 0)){
					pkt->SetAddr(rm_addr);
				    pkt->SetCmd(RD_HEARTBEAT_ACK);
					usleep(50000); //50 ms
				    sendRadio(pkt);
				}
			}
			break;
		}
		case RD_PACK_EXT:
		{
			uint8_t idx = pkt->ReadByte();
			uint8_t cmd = pkt->ReadByte();

			if(idx == 1){
				initLastPacket();
				lastpacket->SetCmd(cmd);				
				pkt->ReadPacket(lastpacket);
				lastpacket->GetIndex();
			}else{
                if(lastpacket == NULL){
				   break;
				}
		        if(lastpacket->CompareExt(idx, cmd)){		
				    pkt->ReadPacket(lastpacket);
				}
			    if(lastpacket->IsDone()){
				    syncHandler->Start(PrecessPacketCall, 100);
			    }
			}
			break;
	    }
		case RD_DIS_START:
		{			
			this->child_id   = pkt->ReadInt16();
			this->dis_id     = pkt->ReadInt16();
			this->dis_level  = pkt->ReadByte();

			if(this->dis_level > 0){
				childDiscovery(this, this->child_id, this->dis_id, this->dis_level);
			}else{
			    syncHandler->Start(RadioDiscovery, 100);
			}
			break;
		}
		case RD_DIS_CHILD:
		{
		    this->dis_id = pkt->ReadInt16();
			uint8_t level = pkt->ReadByte(); //don't used anyway
			this->dis_state    = pkt->ReadByte();
			syncHandler->Start(ChildDiscoveryCall, 100);
			break;
		}
	}
}

void RadioNetwork::Send(serial_packet *ser_pkt)
{
	if((ser_pkt->data[1] == SYSTEM_RETURN) && (ser_pkt->data[2] == CC1101)){
		switch(ser_pkt->data[3])
		{
			case SYSTEM_READ:
			case SYSTEM_WIRTE:
			{
				if(ser_pkt->read_size == 6){
					this->reg_addr = ser_pkt->data[4];
					this->reg_channel = ser_pkt->data[5];
				}else if(ser_pkt->read_size == 8){
					this->reg_addr = ser_pkt->data[6];
					this->reg_channel = ser_pkt->data[7];
				}
				break;
			}
		}
		return;
	}

	if((state == 2) && (ser_pkt->data[1] != CC1101)){
		if(ser_pkt->read_size > 25){
		    if(lastpacket!=NULL)return;
		    initLastPacket();
			
		    lastpacket->SetAddr(dis_addr);
   	        lastpacket->SetCmd(RD_COM_REPLY);
		    lastpacket->WriteInt16(this->id);
		    lastpacket->WriteByte(0);
		    lastpacket->WriteBuffer(ser_pkt->data+1, ser_pkt->read_size-1);
		    lastpacket->SetPos(0);			
	        syncHandler->Start(PrecessPacketCall, 100);
		}else{
			RadioPacket rd_pkt(dis_addr, RD_COM_REPLY);
		    rd_pkt.WriteInt16(this->id);
		    rd_pkt.WriteByte(0);
		    rd_pkt.WriteBuffer(ser_pkt->data+1, ser_pkt->read_size-1);
			sendRadio(&rd_pkt);
		}
		return;
	}
	
	if((state == 0) && (ser_pkt->data[1] != CC1101)) return;

	RadioPacket pkt(ser_pkt);	
	if(pkt.Decode()){
		onRadioPacket(&pkt);
	}
}

void RadioNetwork::discoveryCallback(bool child, uint8_t code, uint16_t rid, uint8_t level)
{
	if(this->rdHandler!=NULL){
		if(child){
			rdHandler->onDiscoveryChild(code, rid, level);
		}else{
	        rdHandler->onDiscovery(code);
		}
	}
}

void RadioNetwork::onRadioConfigRead(Packet *pkt, uint16_t rid, uint8_t level)
{
	onRadioComData(pkt, rid, level);
}

void RadioNetwork::onRadioComData(Packet *pkt, uint16_t rid, uint8_t level)
{
	lastpacket->Reset();
	lastpacket->SetPos(0);
	lastpacket->WriteInt16(rid);
	lastpacket->WriteByte(level);
	lastpacket->SetPos(0);
	syncHandler->Start(PrecessPacketCall, 100);
}

void RadioNetwork::onDiscoveryChild(uint8_t code, uint16_t rid, uint8_t level)
{
	RadioPacket pkt(dis_addr, RD_DIS_CHILD);
	pkt.WriteInt16(rid);
	pkt.WriteByte(level);
	pkt.WriteByte(code);
	sendRadio(&pkt);
}

//remote call from radio
void RadioNetwork::onDiscovery(uint8_t code)
{
	//send to parnet radio
	RadioPacket pkt(dis_addr, RD_DIS_CHILD);
	pkt.WriteInt16(dis_id);
	pkt.WriteByte(dis_level);
	pkt.WriteByte(code);
	sendRadio(&pkt);
}

void RadioNetwork::OnTimer()
{
	if(this->dis_state == DIS_PROGRESS){
		if(this->dis_read){
			this->dis_read = 0;
			discoverySend();
			return;
		}else{
            this->dis_state = DIS_DONE;
		    //set config back
		    setAddrFreq(this->role, this->addr, this->channel);
		    //send discovery end packet
		    discoveryCallback(false, DIS_DONE, 0, 0);
		}
	}

	
	if(this->hb_count < 60) { //10 minute
		this->hb_count++;
		return;
	}
	
	this->hb_count = 0;

	//reg check
	if((this->reg_addr != this->addr) && (this->reg_channel!=channel)){
		setAddrFreq(this->role, this->addr, this->channel);
	}
	
	if(this->role == ROLE_MAIN){
		RADIO_CHILD *cl = this->child;
		while(cl != NULL){
			if(cl->state == 0){
			   if(cl->channel != this->channel){
	              setAddrFreq(this->role, this->addr, cl->channel);
			   }
			
			   RadioPacket pkt(cl->addr, RD_HEARTBEAT);
	           pkt.EncodeInt16(cl->id);
	           sendRadio(&pkt);
			   usleep(50000);
			}
			cl->timeout_count++;
			if(cl->timeout_count > 3){
				if(cl->state==0){
					removeChild(cl);
					break;
				}
				cl->state = 0;
				cl->timeout_count = 0;
			}
			cl = cl->next;
		}
	}else if(this->role == ROLE_CHILD){		
		if(this->state == 0x1){
			//radionetwork_setaddr_freq(ROLE_MAIN, 0x0, 0x0);
			stopHeadBeart();
			return;
		}
		
		RadioPacket pkt(dis_addr, RD_HEARTBEAT);
        pkt.EncodeInt16(this->id);
		sendRadio(&pkt);
		
		this->dis_count++;	
		if(this->dis_count > 10){
			this->state = 0x1;
		}
	}
}

