#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <shadow.h>
#include <unistd.h>
#include <crypt.h>
#include "tcpserver.h"

#include <string>
#include <iostream>
#include <list>


TcpServer::TcpServer(uint16_t port)
:_port(port), boardhandler(NULL), logged(false), radio(NULL)
{
	memset(&username, '\0', sizeof(username));
	memset(&password, '\0', sizeof(password));
}

TcpServer::~TcpServer(void)
{
	clients.clear();
}

bool TcpServer::CheckLogin(const char *user, const char *pass)
{
	if(this->logged){
		return (strcmp(user, username) == 0 && strcmp(pass, password) == 0);
	}else{	
#ifdef OS_UBUNTU	
		memcpy(username, "admin", strlen("admin"));
		memcpy(password, "123456", strlen("123456"));
		logged = true;
#else
		struct spwd *spw;
		char key_buf[48];
		char *key=key_buf;
		char salt[12];

		memset(key,'\0',sizeof(key));
		memset(salt,'\0',sizeof(salt));
  
		spw=getspnam(user);
		if(spw==NULL)
		{
			printf("getspnam err\n");
			return false;
		}

		//printf("spw->sp_pwdp:%s\n",spw->sp_pwdp);
		memcpy(salt,spw->sp_pwdp,12);
		//printf("salt:%s\n",salt);
		key=crypt(pass, salt);
		//printf("key:%s\n",key);
		if(!memcmp(key,spw->sp_pwdp, strlen(key))) //login err
		{
			memcpy(username, user, strlen(user));
			memcpy(password, pass, strlen(pass));

			logged = true;
		}
#endif	
	}
	return logged;
}

void TcpServer::OnAccept(Client *client)
{
	clients.push_back(client);
}

void TcpServer::OnDisconnect(Client *client)
{
	clients.remove(client);
}

void TcpServer::onPacketRead(Client *client, NetPacket *pkt)
{	
	if(!client->IsLogin() && pkt->GetCommand() != CMD_LOGIN){
		pkt->ResetToSend();
		pkt->EncodeBoolean(false);
		pkt->EncodeString("not login");
		pkt->SetCommand(CMD_LOGIN);
		pkt->AutoAck();
		pkt->EncodeBuffer();

		client->Send(pkt->getBuffer(), pkt->getLength());
		return;
	}
	
	switch(pkt->GetCommand())
	{
		case CMD_SERIAL:
		{
	        	uint8_t temp[260] = {0},i;
			uint8_t *enc = temp;
			enc = pkt->toHardware(enc);
			if(boardhandler!=NULL){
				boardhandler->Send(temp, enc - temp);
			}
			break;
		}
		case CMD_DEVICE_INFO:
		{
			uint8_t *enc;
			uint8_t *version;
			struct timeval tv;
			gettimeofday(&tv,NULL);

            		if(boardhandler!=NULL){
			   version = boardhandler->GetVersion();
			}

			pkt->ResetToSend();
			pkt->EncodeString((const char *)version);
			pkt->EncodeString(VERSION_HARDWARE);
			pkt->EncodeInt32(tv.tv_sec);
			pkt->SetCommand(pkt->GetCommand());
			pkt->AutoAck();
		    	pkt->EncodeBuffer();
			            
			client->Send(pkt->getBuffer(), pkt->getLength());
			break;
		}
		case CMD_SCHEDULE_TASK:
		{
			break;
		}
		case CMD_TRIGGER:
		{
			break;
		}
		case CMD_LOGIN:
		{
			char temp_user[50] = {0};
			char temp_pass[50] = {0};
			str_st username = {0};
			str_st password = {0};

			pkt->DecodeString(&username);
			pkt->DecodeString(&password);

			memcpy(temp_user, username.str, username.len);
			memcpy(temp_pass, password.str, password.len);

			pkt->ResetToSend();
			
			if(CheckLogin(temp_user, temp_pass)){
				client->SetLogin(true);
				pkt->EncodeBoolean(true);
		        pkt->EncodeString("logged");
			}else{ 
				client->SetLogin(false);
				pkt->EncodeBoolean(false);
		        pkt->EncodeString("username or password error");
			}

			pkt->AutoAck();
		    	pkt->EncodeBuffer();
		    	client->Send(pkt->getBuffer(), pkt->getLength());
			break;
		}
		case CMD_RADIONETWORK:
		{
			if(radio !=NULL) radio->OnNetData(client, pkt);			
			break;
		}
	    case CMD_SYSTEM:
		{
			uint8_t type = pkt->ReadByte();
			if(type == DATA_KEYVALUE){
			    	char temp_key[50] = {0};
			    	char temp_val[50] = {0};
				str_st key = {0};
				str_st val = {0};

				pkt->DecodeString(&key);
				pkt->DecodeString(&val);
								
                memcpy(temp_key, key.str, key.len);
			    memcpy(temp_val, val.str, val.len);
				
				if(strcmp(temp_key, "system")==0){					
					if(strcmp(temp_val, "reboot")==0){
			            pkt->ResetToSend();
				        pkt->EncodeBoolean(true);
		                pkt->EncodeString("reboot");						
			            system("reboot");
				}else if(strcmp(temp_val, "update")==0){
					type = pkt->ReadByte();
					if(type == DATA_KEYVALUE){
				        pkt->DecodeString(&key);
				        pkt->DecodeString(&val);
						memcpy(temp_key, key.str, key.len);
						memcpy(temp_val, val.str, val.len);
						if(strcmp(temp_key, "url")==0){
						//do something,if needed to use remote url
						}
					}
					pkt->ResetToSend();
					pkt->EncodeBoolean(true);
		    		pkt->EncodeString("update");
								
					system("/root/upgsbox.sh");	
						
				}else if(strcmp(temp_val, "update-firmware")==0){
					type = pkt->ReadByte();
					if(type == DATA_KEYVALUE){
						pkt->DecodeString(&key);
						pkt->DecodeString(&val);
						memcpy(temp_key, key.str, key.len);
						memcpy(temp_val, val.str, val.len);
						if(strcmp(temp_key, "url")==0){
							//do something,if needed to use remote url
						}
					}					
					pkt->ResetToSend();
					pkt->EncodeBoolean(true);
		    		pkt->EncodeString("update-firmware");
					system("/root/upfirmware.sh");	
				}	
								        
				pkt->AutoAck();
		        pkt->EncodeBuffer();
		        client->Send(pkt->getBuffer(), pkt->getLength());
			}
		}
		break;
	}
	}

	if(pkt->GetAck() == ACK_ASK){
	    pkt->ResetToSend();
		pkt->AutoAck();
		pkt->EncodeBuffer();
		client->Send(pkt->getBuffer(), pkt->getLength());
	}
}

void TcpServer::OnDataRead(Client *client,uint8_t *data, int size)
{		
	NetPacket pkt(data, size);
    	while(pkt.Decode()){
		onPacketRead(client, &pkt);
	}
}

void TcpServer::Send(serial_packet *ser_pkt)
{	
	NetPacket pkt(ser_pkt->read_size);
	pkt.SetCommand(CMD_SERIAL);
	pkt.SetAck(ACK_NOP);
	pkt.WriteBuffer(ser_pkt->data, ser_pkt->read_size);
	pkt.EncodeBuffer();
	
	std::list<Client *>::const_iterator it = clients.begin();
	for (; it != clients.end(); it++)
	{
		if((*it)->IsLogin()){
		   (*it)->Send(pkt.getBuffer(), pkt.getLength());
		}
	}
}
