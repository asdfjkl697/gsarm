#include <stdio.h>
#include "hardware.h"
#include "tcpserver.h"
#include "obsolete.h"
#include "udpserver.h"
#include "radionetwork.h"
#include "taskmanager.h"

int main(void)
{	
	Hardware *hw = new Hardware();
	if(!hw->Open()){
		//return 0;  //jyc20170309 debug
	}
	//TimerHandler *timerhandler; //jyc20160719
		
	TcpServer *tcpServ = new TcpServer(SERVER_PORT);
	tcpServ->AddBoardHandler(hw); //add hardware handler
	
	ObsoleteServer *obsServ = new ObsoleteServer(DATA_SERVER_PORT);
	obsServ->AddBoardHandler(hw); //add hardware handler

	/*device discovery*/
	UDPServer *udpbroadcast = new UDPServer(UDP_PORT);
	udpbroadcast->SetVerion(hw->GetVersion());
	udpbroadcast->Open();

	TaskManager *tm = new TaskManager();
	tm->AddIOTask(hw);//sync io read
	tm->AddTCPServerTask(tcpServ); //add sync tcp server handler
	tm->AddTCPServerTask(obsServ);//add sync tcp server handler
	tm->AddSocketTask(udpbroadcast); //add sync udp server handler
	
	//tm->AddTimer(timerhandler, 5000); //jyc20160719
	
	obsServ->AddTask(tm); //task available in class
	
	/*radio network*/
	//RadioNetwork *radio = new RadioNetwork(hw, tm);   //remove by jyc 20160309
	//radio->SetMac(udpbroadcast->GetMacPtr()); //mac addr to radio
	//tcpServ->SetRadio(radio); //add radio to tcp server

	/*set hardware data handler to network*/
	hw->AddClientsHandler(tcpServ);
	hw->AddClientsHandler(obsServ);
	//hw->AddClientsHandler(radio);  //remove by jyc 20160309
	
	tm->Run(); //loop forever

	udpbroadcast->Close();
	//clean up
	tm->RemoveIOTask(hw);
	tm->RemoveTCPServerTask(tcpServ);
	tm->RemoveTCPServerTask(obsServ);
	tm->RemoveSocketTask(udpbroadcast);

	//tm->RemoveTimer(timerhandler); //jyc20160719
	tm->Close();
	
	delete(udpbroadcast);
	delete(obsServ);
	delete(tcpServ);
	delete(hw);
	delete(tm);
	
    return 0;
}
