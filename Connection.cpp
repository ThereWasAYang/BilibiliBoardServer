

#include "stdafx.h"
#include "Connection.h"
#include "NetworkServer.h"
#include "NetWorkClient.h"
#include "Helper.h"
#include "NetWorkPack.h"
#include "xLogManager.h"
#define MAX_SEND_LEN 10240


//----------------------------------------------------------------------------------
Connection::Connection(boost::asio::io_service& ioserver,ReceivePacker* pReceivePack)
	:m_Socket(ioserver),m_Sending(false),m_ReceivePacker(pReceivePack), m_isClose(false)
{
	
	m_pSendBuff = NULL;
	m_iBuffSize = 0;
}

//----------------------------------------------------------------------------------
Connection::~Connection()
{
	if(m_Socket.is_open())
	{
		m_Socket.close();
	}

	boost::system::error_code ignored_ec;
	m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

	SafeDelete(m_pSendBuff);
	m_iBuffSize = 0;

}



//----------------------------------------------------------------------------------
bool Connection::startRead()
{
	
	if(m_Socket.is_open()==false)
	{
	  return  false;
	}

	try
	{
		xstring ipaddr=m_Socket.remote_endpoint().address().to_string();
		uint16 port=m_Socket.remote_endpoint().port();
		///获取ip
		m_IP=ipaddr+_TEXT(":")+Helper::IntToString(port);
	}
	catch( boost::system::system_error* e)
	{
		if(e!=NULL)
		{
			//remote_endpoint
			xLogMessager::getSingleton().logMessage(e->what(),Log_ErrorLevel);
		}

	}
	catch (...)
	{

		return  true;


	}

	memset(m_MessageHead,0,sizeof(m_MessageHead));

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_MessageHead,sizeof(m_MessageHead)),
		bind(
		&Connection::handReadHead,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);
	/*m_Socket.async_read(boost::asio::buffer(m_MessageHead,sizeof(m_MessageHead)),
		bind(
		&Connection::handReadHead,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);*/


	

	

	return   true	;

}

//----------------------------------------------------------------------------------
void Connection::close()
{


	 if( m_isClose==true)
	 { 
		 return ;
	 }

	  m_isClose=true;
	if(getIP().length()>0)
	{
		NetworkServer* pServer=dynamic_cast<NetworkServer*>(m_ReceivePacker);
		if(pServer!=NULL)
		{
			pServer->notifyCloseConnect(getIP());
		}
		else
		{
			NetworkClient *pClinet =dynamic_cast<NetworkClient*>(m_ReceivePacker);
			if(pClinet!=NULL)
			{
				pClinet->notifyCloseConnect(getIP());
			}
		}
	}
	if(m_Socket.is_open())
	{
		m_Socket.close();
		boost::system::error_code ignored_ec;
		m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec); 
	}

	
}

//----------------------------------------------------------------------------------
const xstring& Connection::getIP()const 
{
	return m_IP;
}


//----------------------------------------------------------------------------------
bool Connection::send(uint32 messageid,const char* pdata,uint32	lenght,bool isencrypt,uint32 roleid/*=0*/,int32 serverId /*= 0*/)
{
	if(pdata==NULL||lenght==0)
	{
		return false;
	}

	NetWorkPackPtr pPack(new NetWorkPack(messageid,pdata,lenght,isencrypt,roleid,serverId));
	///锁住压入到发送队列当中去
	boost::mutex::scoped_lock lock(m_SendPackCollect_Mutex);
	m_SendPackCollect.push_back(pPack);


	doSend();

	///执行真实的发送动作
	return true;


}



//----------------------------------------------------------------------------------
bool  Connection::doSend()
{
	///如果正在发送中就直接返回
	if(m_Sending==true)
	{
		return false;
	}


	///从队列中取出，如果队列中有就发送第一个
	if(m_SendPackCollect.empty())
	{
		return false;
	}

	m_Sending=true;
/*
	NetWorkPackPtr pPack=m_SendPackCollect[0];

	const char* pdata = pPack->getBuffer();
	uint32 size = pPack->getSize();
	boost::asio::async_write(m_Socket, boost::asio::buffer(pdata,size),boost::bind(&Connection::handWrite, shared_from_this(),boost::asio::placeholders::error));

	

#ifdef WIN32
	{
		uint32 id = pPack->getMessagID();
		int32 roleid = pPack->getUserData();

		const char* pdata = pPack->getBuffer();
		uint32 size = pPack->getSize();


		xstring strProcessName;
		xAppliction* pApplication = xAppliction::getSingletonPtr();
		if(pApplication!=NULL)
			strProcessName = pApplication->getApplicationName();

		Helper::dbgTrace("taitan %s send: role=%d, sender=%s, sendlen=%d, msgid=%d\n",strProcessName.c_str(),roleid,m_IP.c_str(),size,id);
	}
#endif
*/

	uint32 nFullSize = 0;
	//记录所有的
	m_iSendPackCount = 0;
	for(m_iSendPackCount=0; m_iSendPackCount<(int32)m_SendPackCollect.size(); m_iSendPackCount++)
	{
		NetWorkPackPtr pPack=m_SendPackCollect[m_iSendPackCount];
		uint32  size=pPack->getSize();

		nFullSize += size;

		if(nFullSize>MAX_SEND_LEN)
		{
			if(m_iSendPackCount==0)
				++m_iSendPackCount;	//只发这一个包
			else	//当前被判断的包不发送
				nFullSize-=size;
			break;
		}
	}

	int32 roleid=0;
	int32 serverid=0;
	xstring strMsgID;

	if(m_iSendPackCount>1)
	{
		//仅发送包大于1时，分配缓存空间
		if(m_pSendBuff==NULL)
		{
			m_iBuffSize = nFullSize;
			if(nFullSize<MAX_SEND_LEN)
				m_iBuffSize = MAX_SEND_LEN;

			m_pSendBuff = new char[m_iBuffSize];
			CHECKERRORANDRETURNRESULT(m_pSendBuff!=NULL);
		}
		else
		{
			if(m_iBuffSize<nFullSize)
			{
				SafeDelete(m_pSendBuff);
				m_pSendBuff = new char[nFullSize];
				CHECKERRORANDRETURNRESULT(m_pSendBuff!=NULL);
				m_iBuffSize = nFullSize;
			}
		}

		uint32 iCurrPos = 0;
		for(int32 i=0; i<m_iSendPackCount; i++)
		{
			NetWorkPackPtr pPack=m_SendPackCollect[i];
			const char* pdata = pPack->getBuffer();
			uint32 size = pPack->getSize();
			memcpy(m_pSendBuff+iCurrPos,pdata,size);
			iCurrPos += size;

			roleid = pPack->getUserData();
			serverid = pPack->getServerId();
			strMsgID += Helper::IntToString(pPack->getMessagID());
			strMsgID += ",";
		}

		boost::asio::async_write(m_Socket, boost::asio::buffer(m_pSendBuff,nFullSize),boost::bind(&Connection::handWrite, shared_from_this(),boost::asio::placeholders::error));
		m_SendPackCollect.erase(m_SendPackCollect.begin(),m_SendPackCollect.begin()+m_iSendPackCount);
	}
	else
	{//只有一个包
		NetWorkPackPtr pPack=m_SendPackCollect[0];

		const char* pdata = pPack->getBuffer();
		uint32 size = pPack->getSize();
		boost::asio::async_write(m_Socket, boost::asio::buffer(pdata,size),boost::bind(&Connection::handWrite, shared_from_this(),boost::asio::placeholders::error));

		roleid = pPack->getUserData();
		serverid = pPack->getServerId();
		strMsgID += Helper::IntToString(pPack->getMessagID());
	}

	//std::cout << " send to" << m_IP << ": role=" << roleid << ",serverID=" << serverid << ",count=" << m_iSendPackCount << ",sendlen=" <<nFullSize << ", msgid=" << strMsgID << std::endl;


	return true;
}

//----------------------------------------------------------------------------------
void  Connection::handWrite(const  boost::system::error_code& e)
{
	if(!e)
	{
		boost::mutex::scoped_lock lock(m_SendPackCollect_Mutex);

		m_Sending=false;
		///弹出正在发送的对像
		if(m_SendPackCollect.empty()==false)
		{
			//m_SendPackCollect.pop();
			if(m_iSendPackCount==1)	//只发一个包的情况
				m_SendPackCollect.erase(m_SendPackCollect.begin());
		}

		///如果队列中还有。发送队列里的
		doSend();



	}
	//else
	//{
		//close();
	//}

}


//----------------------------------------------------------------------------------
void  Connection::handReadHead(const boost::system::error_code& error, size_t len)
{
	///连接后的第一包需要发送密码验证连接的安全性，
	///安全性连接通过后，把收到的流数据转成包数据，再放到networkserver队列中

	if(!error)
	{
		bool isencrypt=false;
		///如果收到正确的包头。就继续读包身
		if(NetWorkPack::isPackHead(m_MessageHead,sizeof(m_MessageHead),isencrypt))
		{
			
			NetWorkPackPtr pPack(new NetWorkPack());
			m_ReceivePack=pPack;
			uint32* pMessageid=(uint32*)(m_MessageHead+PACK_MESSSAGEID_OFFSET);
			uint32* plength=(uint32*)(m_MessageHead+PACK_LENGTH_OFFSET);
			uint32 temleng=(*plength);
			temleng=temleng-PACK_HEAD_SIZE;
			uint32* userdata=(uint32*)(m_MessageHead+PACK_USERDATA_OFFSET);
			int32* serverId = (int32*)(m_MessageHead+PACK_SERVERID_OFFSET);
			if(isencrypt)
			{
				m_ReceivePack->setEncrpyt();
			}
			m_ReceivePack->resize(*pMessageid,*userdata,*serverId,temleng);
			///续继读包身
			boost::asio::async_read
				(m_Socket,
				boost::asio::buffer(m_ReceivePack->getRawData(),m_ReceivePack->getBodyLength()),
				bind(
				&Connection::handReadBody,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
				)
				);

			/*m_Socket.async_read(boost::asio::buffer(m_ReceivePack->getData(),m_ReceivePack->getBodyLength()),
				bind(
				&Connection::handReadBody,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
				)
				);
*/
		}
		
	}else 
	{
		close();

	}

	

}


//----------------------------------------------------------------------------------
void Connection::handReadBody(const boost::system::error_code& error, size_t len)
{
	if(!error)
	{
		
		if(m_ReceivePacker!=NULL&&m_ReceivePack.use_count()!=0)
		{

			m_ReceivePack->setIP(getIP());
			m_ReceivePacker->addPack(m_ReceivePack);
			///把获取的包发出去
			//std::cout << "receive from " << m_IP << ": userData=" << m_ReceivePack->getUserData() << ", serverid=" << m_ReceivePack->getServerId() << ", sendlen=" << m_ReceivePack->getBodyLength() << ", msgid=" << m_ReceivePack->getMessagID() << std::endl;

		}
		m_ReceivePack.reset();

		

		///续继读包头
		startRead();

	}else///如果读取错误 。关闭读取
	{
		close();
	}


}