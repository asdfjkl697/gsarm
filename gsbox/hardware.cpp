#include "hardware.h"
#include "rs232.h"
#include <stdlib.h>

Hardware::Hardware(void)
:cache_length(0)
{
	memset(&prve_packet, 0, sizeof(serial_packet));
	memset(&soft_version, '\0', sizeof(soft_version));
	GetSoftVersion();
}

Hardware::~Hardware(void)
{
	clients.clear();
}

void Hardware::RemoveClientsHandler(ClientsHandler *handler)
{
	clients.remove(handler);
}

void Hardware::AddClientsHandler(ClientsHandler *handler)
{
	clients.push_back(handler);
}

void Hardware::onPacketRead(serial_packet *data)
{	
	std::list<ClientsHandler *>::const_iterator it = clients.begin();
	for (; it != clients.end(); it++)
	{
		(*it)->Send(data);
	}
}

void Hardware::OnEvent()
{
	uint8_t buffer[BUFFER_SIZE];
	int size = RS232_PollComport(SERIAL_PORT, buffer, BUFFER_SIZE);
	if (size >0){
		serial_packet *s_data = &this->prve_packet;
	    uint8_t *enc = buffer;
		
		if(size < 5 && (s_data->data_size==0) && ((this->cache_length+size) < 5)){
			memcpy(this->cache + this->cache_length, buffer, size);
			this->cache_length += size;

			return;
		}

		if(this->cache_length > 0){
			uint8_t temp[BUFFER_SIZE];
			memcpy(temp, this->cache, this->cache_length);
			memcpy(temp + this->cache_length, buffer, size);
			size+= this->cache_length;
			memcpy(buffer, temp, size);
			this->cache_length = 0;
		}
			
		while(size > 0){
			if(s_data->data_size == 0x0){
				if(enc[0] == 0xBC && enc[1]==0xAC){ 
					s_data->head = *enc++;
			        s_data->ack = *enc++;
				    s_data->data_size = *enc++;
					size-=3;
				}else{
					enc++;
					size--;
				}
			}else if(s_data->read_size < s_data->data_size){
				uint8_t read_size = s_data->data_size - s_data->read_size;
				if(read_size > size)
				{
					read_size = size;
				}
				if(s_data->read_size+read_size<260){ //jyc20150911debug
					memcpy(s_data->data+s_data->read_size, enc, read_size);
					s_data->read_size +=read_size;
					enc+=read_size;
				}
				size -= read_size;
			}else{
				s_data->end_ack = *enc++;
				size--;

				if(s_data->ack == s_data->end_ack){
					//printf("rec %d\n",s_data->read_size);
					onPacketRead(s_data);
				}

				memset(s_data, 0, sizeof(serial_packet));				
			}
		}
	}
}

int Hardware::GetFD()
{
	RS232_getfd(SERIAL_PORT);
}

bool Hardware::Open()
{
#ifndef OS_UBUNTU
	system("echo 18 >/sys/class/gpio/export"); //reset
	system("echo 19 >/sys/class/gpio/export"); //isp0
	system("echo 20 >/sys/class/gpio/export"); //isp1
	system("echo out >/sys/class/gpio/gpio18/direction");
	system("echo out >/sys/class/gpio/gpio19/direction");
	system("echo out >/sys/class/gpio/gpio20/direction");
	system("echo 0 >/sys/class/gpio/gpio19/value");
	system("echo 0 >/sys/class/gpio/gpio20/value");
	system("echo 1 >/sys/class/gpio/gpio18/value");
	system("sleep 1");
	system("echo 0 >/sys/class/gpio/gpio18/value");
	system("sleep 1");
#endif
	
#ifdef USESERIAL
	//board->cache_length = 0;
	return (RS232_OpenComport(SERIAL_PORT, SERIAL_BAND)==0);
#endif
}

void Hardware::Close()
{
#ifdef USESERIAL
    RS232_CloseComport(SERIAL_PORT);
#endif
}

int Hardware::Send(uint8_t *data, int size)
{	
#ifdef USESERIAL	
    return RS232_SendBuf(SERIAL_PORT, data, size);
#endif
}

void Hardware::GetSoftVersion()
{
#ifndef OS_UBUNTU
	FILE *stream;
	char buf[64];

	memset(buf,'\0',sizeof(buf));
	stream = popen("opkg list gsbox","r");
	fread(buf,sizeof(char),sizeof(buf),stream);
	memcpy(&soft_version,&buf[8],10); //gsbox 

	pclose(stream);	
#endif
}

