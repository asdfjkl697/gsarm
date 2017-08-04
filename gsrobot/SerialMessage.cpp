#include "SerialMessage.h"
#include "DeviceConnection.h"
#include "gsiot/GSIOTClient.h"
#include "RunCode.h"



//////////////////////////////////////////////////////////////////////////


SerialMessage::SerialMessage(ISerialPortHandler *handler)
	:m_handler(handler), m_lastPrintIs_CurCmd(0)//,m_queue_state(false)
{
}


SerialMessage::~SerialMessage(void)
{
	LOGMSG("~SerialMessage\r\n");

	m_queue_mutex.lock();

	while(m_buffer_queue.size()>0){
		SERIALDATABUFFER *buffer = m_buffer_queue.front();
		delete buffer;
		m_buffer_queue.pop_front();
	}

	m_queue_mutex.unlock();
}

void SerialMessage::doMessage( defLinkID LinkID, uint8_t *data, uint32_t size, const IOTDeviceType DevType, const uint32_t DevID, ControlBase *ctl, DeviceAddress *address, uint32_t overtime, uint32_t QueueOverTime, uint32_t nextInterval )
{
	if( LinkID<0 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "doMessage:LinkID=%d error!!!\r\n", LinkID );
		return;
	}

	uint8_t ModuleIndex = data[4];
	if( IsDisableModule( ModuleIndex, false ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: no send. <IsDisableModule(%d)>!!!\r\n", ModuleIndex );
		return;
	}

		m_queue_mutex.lock();

		if( !m_buffer_queue.empty() )
		{
			std::list<SERIALDATABUFFER*>::iterator it = m_buffer_queue.begin();
			std::list<SERIALDATABUFFER*>::iterator itEnd = m_buffer_queue.end();
			while( it!=itEnd )
			{
				SERIALDATABUFFER *curbuffer = (*it);

				// same is exist
				if( curbuffer->IsSame( LinkID, data, size, DevType, DevID, ctl, address, overtime, QueueOverTime, nextInterval ) )
				{
					curbuffer->SetNowTime();
					LOGMSG( "SerialMsg IsSame IsBeing LinkID=%d, Dev(%d,%d), addr=%d", LinkID, DevType, DevID, address?address->GetAddress():0 );

					m_queue_mutex.unlock();
					return;
				}

				++it;
			}
		}

		SERIALDATABUFFER *buffer = new SERIALDATABUFFER();
		buffer->LinkID = LinkID;

		buffer->data = new uint8_t[size];
		buffer->size = size;
		memcpy(buffer->data,data,size);

		buffer->overtime = overtime;
		buffer->QueueOverTime = QueueOverTime;
		buffer->nextInterval = nextInterval;
		buffer->DevType = DevType;
		buffer->DevID = DevID;
		buffer->ctl = GSIOTClient::CloneControl( ctl, false );

		if( address )
			buffer->address = (DeviceAddress*)address->clone();
		else
			buffer->address = NULL;

		m_buffer_queue.push_back(buffer);

		m_queue_mutex.unlock();
}

void SerialMessage::Check()
{
	SERIALDATABUFFER *buffer = NULL;

	m_queue_mutex.lock();

	std::list<SERIALDATABUFFER*>::iterator it = m_buffer_queue.begin();
	std::list<SERIALDATABUFFER*>::iterator itEnd = m_buffer_queue.end();
	while( it!=itEnd )
	{
		buffer = (*it);

		// �ѳ�ʱ�Ĳ����ͣ�ֱ��ɾ��
		if( buffer->IsQueueOverTime() )
		{
			buffer->Print( "Check: SerialMsg buf IsQueueOverTime" );
			delete buffer;
			buffer = NULL;

			m_buffer_queue.erase(it);
			it = m_buffer_queue.begin();
			itEnd = m_buffer_queue.end();
			continue;
		}

		++it;
	}

	m_queue_mutex.unlock();
}

void SerialMessage::onTimer( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink )
{
	SERIALDATABUFFER *buffer = NULL;

	m_queue_mutex.lock();

	CMsgCurCmd *pMsgCurCmd = CommLink ? &CommLink->GetMsgCurCmdObj() : &m_MsgCurCmd;
	const defLinkID curLinkID = CommLink ? CommLink->get_cfg().id : defLinkID_Local;

	std::list<SERIALDATABUFFER*>::iterator it = m_buffer_queue.begin();
	std::list<SERIALDATABUFFER*>::iterator itEnd = m_buffer_queue.end();
	while( it!=itEnd )
	{
		buffer = (*it);

		// overtime delete
		if( buffer->IsQueueOverTime() )
		{
			buffer->Print( "SerialMsg buf IsQueueOverTime" );
			delete buffer;
			buffer = NULL;

			m_buffer_queue.erase(it);
			it = m_buffer_queue.begin();
			itEnd = m_buffer_queue.end();
			continue;
		}

		if( CommLink )
		{
			if( !CommLink->IsOpen() )
			{
				buffer = NULL;
				++it;
				continue;
			}
		}
		if( curLinkID != buffer->LinkID )
		{
			buffer = NULL;
			++it;
			continue;
		}

		if( pMsgCurCmd->Is_CurCmd(buffer->ctl) )
		{
			// control print space  jyc trans
			if( timeGetTime()-m_lastPrintIs_CurCmd > 1200 )
			{
				LOGMSG( "Is_CurCmd=true\r\n" );
				m_lastPrintIs_CurCmd = timeGetTime();
			}

			buffer = NULL;
			 ++it;
			continue;
		}

		m_buffer_queue.erase(it);
		break;
	}

	m_queue_mutex.unlock();

	macHeartbeatGuard_step(130100);

	if( buffer )
	{
		macHeartbeatGuard_step(130150);
		bool isSet = pMsgCurCmd->Set_CurCmd( buffer->ctl, buffer );
		bool doSend = true;

		char chRe[64] = {0};

		macHeartbeatGuard_step(130200);

		const uint32_t lastprevInterval = timeGetTime()-pMsgCurCmd->m_ts_lastSend;
		const uint32_t needprevInterval = pMsgCurCmd->m_prevInterval;
		if( lastprevInterval < needprevInterval )
		{
			uint32_t needSleep = needprevInterval-lastprevInterval;
			needSleep = needSleep>5000 ? 5000:needSleep;

			LOGMSG( "SendSleep=%d, lastprevInterval=%d, needprevInterval=%d", needSleep, lastprevInterval, needprevInterval );

			//usleep( needSleep * 1000 );  //jyc20170302 modify 
			usleep( needSleep * 100 ); 
		}

		if( doSend )
		{
			macHeartbeatGuard_step(130300);

			m_handler->SendData( CommLink, buffer->data,buffer->size); //jyc20160901 send mark

			pMsgCurCmd->m_ts_lastSend = timeGetTime();

			if( buffer->get_nextInterval()>0 && IsRUNCODEEnable( defCodeIndex_COM_SendInterval ) )
			{
				const uint32_t ComPortWriteTime = g_GetComPortWriteTime( buffer->size );
				pMsgCurCmd->m_prevInterval = ComPortWriteTime>buffer->get_nextInterval() ? ComPortWriteTime:buffer->get_nextInterval();
			}
			else
			{
				pMsgCurCmd->m_prevInterval = 0;
			}

			//LOGMSG( "__TEST__: set prevInterval=%d after send=%d", pMsgCurCmd->m_prevInterval, buffer->size );
		}
		else
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN,  "__TEST__: no send. <%s>!", chRe );
		}

		if( !isSet )
		{
			macHeartbeatGuard_step(130400);
			delete buffer;
			macHeartbeatGuard_step(130500);
		}
	}
}
