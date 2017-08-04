#include "udpserver.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "netpacket.h"

#include "libxml/xmlmemory.h"
#include "libxml/parser.h"

#define SN_FILE "/root/sn.xml"


// 通用参数
#define defParamNm_cmd		"cmd"
#define defParamNm_id		"id"
#define defParamNm_name		"name"
#define defParamNm_ip		"ip"
#define defParamNm_ip2		"ip2"
#define defParamNm_netmask	"netmask"
#define defParamNm_gateway	"gateway"
#define defParamNm_dns		"dns"
#define defParamNm_mac		"mac"
#define defParamNm_model	"model"
#define defParamNm_ver		"ver"
#define defParamNm_SN		"sn"

#define macEncodeParamEnd( enc, key, val ) strcat( enc, key ); strcat( enc, "=" ); strcat( enc, val );
#define macEncodeParam( enc, key, val ) macEncodeParamEnd( enc, key, val ); strcat( enc, "," );

using namespace std;  //jyc20170309 add
#include <string>

#ifndef OS_UBUNTU
static void getNetworkValue(char *szBuf, uint8_t *value)
{
    int i,j;
	uint8_t len;

	for(i =0;i < strlen(szBuf);i++)
    {              
		if(*(szBuf+i)==0x27)
		{			
			memset(value,'\0',18);
			for(j=0;j<18;j++)
			{
				i++;				
				if(*(szBuf+i)==0x27)
				{
					return;
				}
				*value++=*(szBuf+i);
			}
		}
    }
}

static void getNetworkInfo(NETWORK_ADAPTER *adapter,char *interface)
{
    char szBuf[64];
	uint8_t interface_flag=0;
	FILE *fp = NULL;

    if((fp=fopen("/etc/config/network", "r"))==NULL)            
    {
        printf( "Can 't   open   file!\n"); 
        return;
    }
    while(fgets(szBuf,128,fp))                    
    {                         
		if(interface_flag==0)  //jyc20150813add
		{		
			if(strstr(szBuf,interface) != NULL)  
				interface_flag=1;
			continue;
		}  
		if(strstr(szBuf, "ipaddr") != NULL)
        {
			if((++interface_flag)>=3)break; //jyc20151028add	
			getNetworkValue(szBuf, adapter->ipaddr);			
        }
		if(strstr(szBuf, "netmask") != NULL)
        {
			getNetworkValue(szBuf, adapter->netmask);
        }
		if(strstr(szBuf, "gateway") != NULL)
        {
			getNetworkValue(szBuf, adapter->gateway);
        }
		if(strstr(szBuf, "dns") != NULL)
        {
			getNetworkValue(szBuf, adapter->dns);
        }
		if(strstr(szBuf, "macaddr") != NULL)
        {
			memset(adapter->mac, '\0', sizeof(adapter->mac));
			getNetworkValue(szBuf, adapter->mac);
        }
    }			
    fclose(fp);	
}

static void getSNInfo(NETWORK_ADAPTER *adapter){
	xmlDocPtr doc;  //jyc20160711 sn read
	xmlNodePtr cur;
	doc = xmlParseFile(SN_FILE);
	if (doc == NULL){
		printf("err1\n");
		return;
	} 
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL){
		xmlFreeDoc(doc);
		printf("err2\n");
		return;
	}
	if (!xmlStrEqual(cur->name, BAD_CAST"message")){
		xmlFreeDoc(doc);
		printf("err3\n");
		return;
	}
	if (xmlHasProp(cur, BAD_CAST"sn")){
		//this->id = atoi((char *)xmlGetProp(cur, BAD_CAST"id"));
		char *mac = (char *)xmlGetProp(cur, BAD_CAST"sn");
		memset(adapter->sn,'\0',30);
		memcpy(adapter->sn,mac,strlen(mac)); //jyc20160711
	}
	xmlFreeDoc(doc);
}
#endif

static uint8_t get_three(char *addr)
{
	uint8_t i,j=0;
	for(i=0;i<strlen(addr);i++)
	{
		if(addr[i]=='.')
			j++;
		if(j==3)
			return i;
	}
}

static uint8_t cmp_route_addr(char *addr1,char *addr2)
{
	uint8_t len;
	len = get_three(addr1);
	if(memcmp(addr1,addr2,len)==0)
		return 0;
	else
		return 1;
}


UDPServer::UDPServer(uint16_t port)
:_port(port)
{
#ifndef OS_UBUNTU
	getNetworkInfo(&adapter,"'lan'");
	getNetworkInfo(&adapter2,"'wwan'");
	getSNInfo(&adapter);
	
#else
	_version = (uint8_t *)malloc(sizeof(20));
	const char* mac = "00:11:cc:dd:33:ff";
	memset(adapter.mac, '\0', sizeof(adapter.mac));
	memcpy(adapter.mac, mac, strlen(mac));
	memcpy(_version, VERSION_SOFT, sizeof(VERSION_SOFT));
#endif
}

UDPServer::~UDPServer(void)
{
#ifdef OS_UBUNTU
	free(_version);
#endif
}

int findaddr(uint8_t *s_str,uint8_t *d_str,uint16_t len){
	int i,j,k,count=0;
	char temp[80];
	int length=sizeof(d_str);
	for(i=0;i<len;i++){
		k=0;
		for(j=i;j<i+length;j++)
			temp[k++]=s_str[j];
		if(memcmp(temp,d_str,length))count++;
		else{
			return count;
		}			
	}
	return -1;
}

static void modifysmartconfig(char *buf,char *flag,char *path)
{
    char szBuf[64];
	FILE *fp = NULL;

    if((fp=fopen(path, "r+"))==NULL)            
    {
        printf( "Can 't   open   file!\n"); 
        return;
    }
	
	while(fgets(szBuf,64,fp)!=NULL){
		if(strstr(szBuf,flag)!=NULL){
			long int position;
			char q[40];
			int szlen = strlen(szBuf);
			int len = strlen(buf);
			int flaglen = strlen(flag);
			int lentmp = szlen - 9 -flaglen;
			if(szlen!=40)break; //jyc20170315 set notice
			if(flaglen+len+9>38)break; 
			memset(q,' ',40);
			q[0]='\t';
			memcpy(&q[1],"option ",7);
			memcpy(&q[8],flag,flaglen);
			q[flaglen+8]=' ';
			q[flaglen+9]='\'';
			memcpy(&q[flaglen+10],buf,len);
			q[flaglen+len+10]='\'';
			q[39]='\0';
			printf(" %s len=%d szlen=%d flaglen=%d lentmp=%d\n",q,len,szlen,flaglen,lentmp);
			position = ftell(fp);
			fseek(fp, position-szlen, SEEK_SET);
			fwrite(q, szlen-1, 1, fp);
			break;
		}
	}
    fclose(fp);	
}


int Udpconfignet(uint8_t *buffer,uint16_t size){
	char macch[18];
	uint8_t macflag[4]={'m','a','c',':'};
	uint8_t jidflag[4]={'j','i','d',':'};
	uint8_t crcflag[4]={'c','r','c',':'};
	uint8_t keyflag[4]={'k','e','y',':'};
#ifdef OS_UBUNTU
	char netpath[64]="/home/chen/smartconfig/sta/network";
	char wifipath[64]="/home/chen/smartconfig/sta/wireless";
	char jidpath[64]="/home/chen/smartconfig/gsjid";
#else
	char netpath[64]="/root/smartconfig/sta/network";
	char wifipath[64]="/root/smartconfig/sta/wireless";
	char jidpath[64]="/root/smartconfig/gsjid";
#endif
	uint8_t i,temp[20];
	int macaddr,jidaddr,crcaddr,keyaddr;
	int len;
	uint32_t len_r;
	len_r=(uint32_t)buffer[0]|buffer[1]<<8;
	if(size!=len_r+4)return -1;
	buffer+=4;
	macaddr=findaddr(buffer,macflag,size);
	jidaddr=findaddr(buffer,jidflag,size);
	keyaddr=findaddr(buffer,keyflag,size);
	crcaddr=findaddr(buffer,crcflag,size);
	if((macaddr<0)||(jidaddr<macaddr)||(keyaddr<jidaddr)||(crcaddr<keyaddr)) //fail config
		return -1;
	len=jidaddr-macaddr-sizeof(macflag);
	if(len!=6)return -2; //mac failed
	memcpy(temp,&buffer[macaddr+sizeof(macflag)],len);
	for(i=0;i<6;i++){
		uint8_t m=temp[i]>>4;
		uint8_t n=temp[i]&0xf;
		macch[3*i]=(m<10)?('0'+m):('a'-10+m);
		macch[3*i+1]=(n<10)?('0'+n):('a'-10+n);
		macch[3*i+2]=':';
	}
	macch[17]='\0';
	printf("mac=%s\n",macch);
	modifysmartconfig(macch,(char*)"macaddr",netpath);
	modifysmartconfig(macch,(char*)"macaddr",wifipath);
	len=keyaddr-jidaddr-sizeof(jidflag);
	if(len<1)return -1;
	memcpy(temp,&buffer[jidaddr+sizeof(jidflag)],len);
	temp[len]='\0';
	printf("jid=%s\n",(char*)temp);
	modifysmartconfig((char*)temp,(char*)"hostname",netpath);
	modifysmartconfig((char*)temp,(char*)"gsjid",jidpath);
	len=crcaddr-keyaddr-sizeof(keyflag);
	if(len<1)return -1;
	memcpy(temp,&buffer[keyaddr+sizeof(keyflag)],len);
	temp[len]='\0';
	printf("key=%s\n",(char*)temp);
	modifysmartconfig((char*)temp,(char*)"key",jidpath);
#ifndef OS_UBUNTU
	system("/root/smartconfig/netconfig.sh");
#endif
	return 1;
}


int Udpconfigwifi(uint8_t *buffer,uint16_t size){
	uint8_t sidflag[4]={'s','i','d',':'};
	uint8_t keyflag[4]={'k','e','y',':'};
	uint8_t crcflag[4]={'c','r','c',':'};
#ifdef OS_UBUNTU
	char netpath[64]="/home/chen/smartconfig/sta/network";
	char wifipath[64]="/home/chen/smartconfig/sta/wireless";
	char jidpath[64]="/home/chen/smartconfig/gsjid";
#else
	char netpath[64]="/root/smartconfig/sta/network";
	char wifipath[64]="/root/smartconfig/sta/wireless";
	char jidpath[64]="/root/smartconfig/gsjid";
#endif
	uint8_t i,temp[20];
	int sidaddr,keyaddr,crcaddr;
	int len;
	uint32_t len_r;
	len_r=(uint32_t)buffer[0]|buffer[1]<<8;
	if(size!=len_r+4)return -1;
	buffer+=4;
	sidaddr=findaddr(buffer,sidflag,size);
	keyaddr=findaddr(buffer,keyflag,size);
	crcaddr=findaddr(buffer,crcflag,size);
	if((sidaddr<0)||(keyaddr<sidaddr)||(crcaddr<keyaddr)) //sort
		return -1;
	len=keyaddr-sidaddr-sizeof(sidflag);
	if(len<1)return -1;
	memcpy(temp,&buffer[sidaddr+sizeof(sidflag)],len);
	temp[len]='\0';
	printf("sid=%s\n",(char*)temp);
	modifysmartconfig((char*)temp,(char*)"ssid",wifipath);
	len=crcaddr-keyaddr-sizeof(keyflag);
	if(len<1)return -1;
	memcpy(temp,&buffer[keyaddr+sizeof(keyflag)],len);
	temp[len]='\0';
	printf("key=%s\n",(char*)temp);
	modifysmartconfig((char*)temp,(char*)"key",wifipath);
#ifndef OS_UBUNTU
	system("/root/smartconfig/wificonfig.sh");
#endif
	return 1;
}

void UDPServer::PacketRead(uint8_t *data, int size, struct sockaddr_in *addr)
{
	data[size] = 0;
	int result;
	uint16_t i,crctemp,crcvalue;
	
    NetPacket pkt(data, size);	

	if(pkt.Decode()){
		switch(pkt.GetCommand()){
			case CMD_BROADCAST_NETCONFIG: //jyc20170310 add

				crcvalue=(uint16_t)data[size-1]<<8|data[size-2];
				crctemp=0;
				for(i=0;i<size-2;i++)crctemp+=data[i];
				if(crctemp!=crcvalue) //crc failed
					break;
				result=Udpconfignet(&data[14],size-14); //jyc20170310
				if(result<1)break;
				pkt.AutoAck();
				pkt.EncodeBuffer();		
				this->Send(addr, (char *)pkt.getBuffer(), pkt.getLength()); //jyc20170309 debug
				break;
			case CMD_BROADCAST_WIFICONFIG: //jyc20170310 add
				crcvalue=(uint16_t)data[size-1]<<8|data[size-2];
				crctemp=0;
				for(i=0;i<size-2;i++)crctemp+=data[i];
				if(crctemp!=crcvalue) //crc failed
					break;
				result=Udpconfigwifi(&data[14],size-14); //jyc20170310
				if(result<1)break;
				pkt.AutoAck();
				pkt.EncodeBuffer();		
				this->Send(addr, (char *)pkt.getBuffer(), pkt.getLength()); //jyc20170309 debug
				break;
			case CMD_BROADCAST_INFO:
			{
				NETWORK_ADAPTER *adp;
				char *route_addr;
				route_addr=inet_ntoa(addr->sin_addr);
				if((cmp_route_addr(route_addr, (char *)adapter2.ipaddr)==0)&&cmp_route_addr(route_addr, (char *)adapter.ipaddr))
					adp = &adapter2;
				else
					adp = &adapter;

				pkt.ResetToSend();
				pkt.EncodeString((const char *)adp->ipaddr); //ipaddress, replace with a real one
				pkt.EncodeString((const char *)adp->netmask);//mask, replace with a real one
				pkt.EncodeString((const char *)adp->gateway);//gateway, replace with a real one
				pkt.EncodeString((const char *)adp->dns);//DNS1, replace with a real one
				pkt.EncodeString("");//DNS2, replace with a real one
				pkt.EncodeInt16(80); //WEB PORT,replace with a real one
				pkt.EncodeInt16(DATA_SERVER_PORT); //COM PORT,replace with a real one
				pkt.EncodeInt16(SERVER_PORT); //DATA PORT
				pkt.EncodeString((const char *)adp->mac); //MAC
				pkt.EncodeString(DEVICE_NAME); //DEVICE NAME
				pkt.EncodeBoolean(0); //USE DHCP,replace with a real one
				pkt.EncodeString(DEVICE_ID); //DEVICE ID
				pkt.EncodeString((const char *)_version); //VERSION VERSION_SOFT 
				//enc = packet_encodeString(enc, VERSION_SOFT);

				pkt.AutoAck();
				pkt.EncodeBuffer();				
				this->Send(addr, (char *)pkt.getBuffer(), pkt.getLength());
				break;
			}
			
			case CMD_BROADCAST_INFO_PARAM:
				{
				char *msgsrc = (char*)pkt.get_data_ptr();
				if( strcmp( msgsrc, "cmd=find_id" )==0 )
					{
						//
						break;
					}
				
					break;//
				}
				
			case CMD_BROADCAST_INFO_EX:
			{
				NETWORK_ADAPTER *adp;
				NETWORK_ADAPTER *adp2;
				char *route_addr;
				route_addr=inet_ntoa(addr->sin_addr);
				if( (cmp_route_addr(route_addr, (char *)adapter2.ipaddr)==0)
				&& cmp_route_addr(route_addr, (char *)adapter.ipaddr)
				)
				{
					adp = &adapter2;
					adp2 = &adapter;
				}
				else
				{
					adp = &adapter;
					adp2 = &adapter2;
				}

				//char *snstr;
				//snstr=getDeviceSN();
				//getSNInfo(&adapter);

				pkt.ResetToSend();
				char *enc = (char*)pkt.get_data_ptr();
				strcpy( enc, "" );
				
				macEncodeParam( enc, defParamNm_ip, (const char*)adp->ipaddr );
				if( strlen((const char*)adp2->ipaddr)>0 ) { macEncodeParam( enc, defParamNm_ip2, (const char*)adp2->ipaddr ); }
				//macEncodeParam( enc, defParamNm_id, DEVICE_ID );
				macEncodeParam( enc, defParamNm_id, (const char *)adp->sn ); //20160711
				macEncodeParam( enc, defParamNm_name, DEVICE_NAME );
				macEncodeParam( enc, defParamNm_netmask, (const char *)adp->netmask );
				macEncodeParam( enc, defParamNm_gateway, (const char *)adp->gateway );
				macEncodeParam( enc, defParamNm_dns, (const char *)adp->dns );
				macEncodeParam( enc, defParamNm_mac, (const char *)adp->mac );
				macEncodeParam( enc, defParamNm_model, "CON" );
				macEncodeParam( enc, defParamNm_SN, (const char *)adp->sn ); //read or set sn
				macEncodeParamEnd( enc, defParamNm_ver, (const char *)_version );
				
				int msg_len = strlen(enc)+1;
				
				pkt.SetDataPos(msg_len);
				pkt.AutoAck();
				pkt.EncodeBuffer();				
				this->Send(addr, (char *)pkt.getBuffer(), pkt.getLength());
				
				break;
			}
		}
	}
}

bool UDPServer::Open()
{
	int err;
	struct sockaddr_in sin;
	int udpBufferSize = UDP_BUFFER_SIZE;

	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == socket_fd)
	{
#ifdef WIN32
		printf("socket error! error code is %d/n", WSAGetLastError());
#else
		printf("socket error! error code is %d/n", errno);
#endif
		return false;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(_port); 
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	int bOpt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, (char*)&bOpt, sizeof(bOpt));
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&udpBufferSize, sizeof(int));
	setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&udpBufferSize, sizeof(int));

	err = bind(socket_fd, (struct sockaddr*)&sin, sizeof(struct sockaddr));
	if (SOCKET_ERROR == err)
	{
#ifdef WIN32
		printf("socket error! error code is %d/n", WSAGetLastError());
#else
		printf("socket error! error code is %d/n", errno);
#endif
		return false;
	}

	return true;
}

int UDPServer::Send(struct sockaddr_in *addr, char *buffer,int size)
{
	int nSendSize = sendto(socket_fd, buffer, size, 0, (struct sockaddr*)addr, sizeof(struct sockaddr)); 
    if(SOCKET_ERROR == nSendSize)  
    {
         printf("sendto error!, error code is %d\r\n", errno);  
         return -1;
	}
	return nSendSize;
}

int UDPServer::Read(struct sockaddr_in *addr, char *buffer,int size)
{
	int addrin = sizeof(struct sockaddr);
	int ret = recvfrom(socket_fd, buffer, size, 0, (struct sockaddr*)addr, (socklen_t *)&addrin);
	if(SOCKET_ERROR == ret){
         printf("sendto error!, error code is %d\r\n", errno);  
         return -1;
	}
    return ret;  
}

void UDPServer::Close()
{
	close(socket_fd);
}
