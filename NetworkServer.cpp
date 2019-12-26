
#include "stdafx.h"
#include "NetworkServer.h"
#include "xLogManager.h"
#include "signal.h"
#include "Connection.h"
#include "Helper.h"
#include "xLogManager.h"
template<> NetworkServer* Singleton<NetworkServer>::ms_Singleton=NULL;

//-------------------------------------------------------------------------------------
NetworkServer::NetworkServer(uint8 IOthreadNumber)
	:m_IOServerPool(IOthreadNumber),
	//m_pAcceptor(m_IOServerPool.get_io_service()),
	m_RecivePackFunc(NULL)
{

	//if(::signal(0, SIG_DFL) == SIG_ERR)



}


//-------------------------------------------------------------------------------------
NetworkServer::~NetworkServer()
{

	stop();
	Sleep(10);
}



//-------------------------------------------------------------------------------------
bool NetworkServer::start(uint32 portnumber,uint32 maxConnect)
{
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string("0.0.0.0"),portnumber);
	boost::asio::ip::tcp::acceptor* paccept = new boost::asio::ip::tcp::acceptor(getIOServer(),ep);
	m_pAcceptorCollects.push_back(paccept);
	//start("localhost",portnumber,maxConnect);
	//start(boost::asio::ip::host_name(),portnumber,maxConnect);

	for(uint32 r=0;r<m_pAcceptorCollects.size();r++)
	{
		waitConnect(r);
	}
	//*/
	m_IOServerPool.run();

	 return true;
}

void NetworkServer::start( xstring hostname,uint32 portnumber,uint32 maxConnect )
{
	boost::asio::ip::tcp::resolver resolver(getIOServer());
	//xstring hostname=boost::asio::ip::host_name();
	boost::asio::ip::tcp::resolver::query query(hostname, Helper::IntToString(portnumber));
	boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end; // End marker.
	while (iter != end)
	{
		boost::asio::ip::tcp::endpoint ep = *iter++;
		bool v4=ep.address().is_v4();
		bool v6=ep.address().is_v6();

		xstring tem=ep.address().to_string();
		///只监听v4ip段
		if(v4==true)
		{
			boost::asio::ip::tcp::acceptor *m_pAcceptor=new boost::asio::ip::tcp::acceptor(m_IOServerPool.get_io_service());
			m_pAcceptor->open(ep.protocol());
			m_pAcceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			m_pAcceptor->bind(ep);
			m_pAcceptor->listen();
			xLogMessager::getSingleton().logMessage(xstring("start listen ip:")+tem,Log_NormalLevel);
			m_pAcceptorCollects.push_back(m_pAcceptor);
		}

		std::cout << ep << std::endl;
	}
}


//-------------------------------------------------------------------------
void NetworkServer::addLister(NetWorkLister* pLister)
{
	if(pLister==NULL)
	{
		return ;
	}

	 m_Listeners.push_back(pLister);
}


//-------------------------------------------------------------------------
void NetworkServer::removeLister(NetWorkLister* pLister)
{
	if(pLister==NULL)
	{
		return ;
	}

	NetWorkListerVector::iterator it=std::find(m_Listeners.begin(),m_Listeners.end(),pLister);
	if(it!=m_Listeners.end())
	{
		m_Listeners.erase(it);
	}
}


bool NetworkServer::connect(const xstring& ip ,uint16 portnumber)
{





	ConnectionPtr  pConnect(new Connection(getIOServer(),this));

	boost::asio::ip::tcp::resolver resolver(pConnect->getSocket().get_io_service());
	boost::asio::ip::tcp::resolver::query query(ip.c_str(),Helper::IntToString(portnumber));
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end;
	boost::system::error_code error = boost::asio::error::host_not_found;

	pConnect->setIP(ip+":"+Helper::IntToString(portnumber));
	ConnectionPare connectpare;
	connectpare.ptr=pConnect;
	connectpare.index=0;
	boost::asio::async_connect(pConnect->getSocket(), endpoint_iterator,boost::bind(&NetworkServer::handleConnect, this,connectpare,
		boost::asio::placeholders::error));

	return true;

}

//-------------------------------------------------------------------------
ConnectionPtr NetworkServer::getConnectByIP(const xstring& ip)
{
	boost::mutex::scoped_lock lock(m_ConnectCollect_Mutex);
	ConnectMap::iterator it= m_ConnectCollect.find(ip);
	if(it!=m_ConnectCollect.end())
	{
		return  it->second;
	}

	return ConnectionPtr();

}

//---------------------------------------------------------------------------------------
void NetworkServer::waitConnect(uint32 index)
{
	if(index>=m_pAcceptorCollects.size())
	{
		return;
	}
	ConnectionPtr  pConnect(new Connection(getIOServer(),this));
	ConnectionPare pConnectPare;
	pConnectPare.ptr=pConnect;
	pConnectPare.index=index;
	boost::asio::ip::tcp::acceptor *pAccepter=m_pAcceptorCollects[index];
	pAccepter->async_accept(pConnect->getSocket(),
		boost::bind(&NetworkServer::handleConnect,this,pConnectPare,boost::asio::placeholders::error)
		);
	return ;
}

//---------------------------------------------------------------------------------------
 void NetworkServer::handleConnect(ConnectionPare pConnectPare,const boost::system::error_code& e)
 {
	 ConnectionPtr pConnect=pConnectPare.ptr;
	 ///如果有新连接进来
	 ///如果是多线程io_server这个地方需要加锁
	 if(!e)
	 {
		 pConnect->startRead();
		 boost::mutex::scoped_lock  lock(m_ConnectCollect_Mutex);
		 m_ConnectCollect.insert(std::make_pair(pConnect->getIP(),pConnect));
		 lock.unlock();

		 ///加入到新连接队列中
		 boost::mutex::scoped_lock  newlock(m_NewConnectCollect_Mutex);
		 m_NewConnectCollect.push_back(pConnect->getIP());
		 xstring log=xstring("connect:")+pConnect->getIP()+xstring("\n");
		 std::cout << log << std::endl;


	 }else
	 {
		 std::cout << ("connect failed...") << std::endl;
		 ///通知联接失败..
		 pConnect->close();
         xLogMessager::getSingleton().logMessage(e.message(),Log_ErrorLevel,true);
//        return;
	 }

	 waitConnect(pConnectPare.index);
 }

 //-------------------------------------------------------------------------------------
 void NetworkServer::notifyCloseConnect(const xstring& ip)
 {
	 boost::mutex::scoped_lock lock(m_CloseConnectCollect_Mutex);
	 m_CloseConnectCollect.push_back(ip);
	 lock.unlock();


	 return ;

 }

 //-------------------------------------------------------------------------------------
 void NetworkServer::notifyNewConnect(const xstring&   ip)
 {
	 boost::mutex::scoped_lock lock(m_NewConnectCollect_Mutex);
	 m_NewConnectCollect.push_back(ip);
	 return ;

 }


//-------------------------------------------------------------------------------------
void NetworkServer::stop()
{
	m_IOServerPool.stop();

	for(uint32 r=0;r<m_pAcceptorCollects.size();r++)
	{
		//m_pAcceptorCollects[r]->close();
		SafeDelete(m_pAcceptorCollects[r]);
	}


}



//-------------------------------------------------------------------------------------

bool NetworkServer::send( uint32 messageid,const char* pdata,uint32 length,const xstring& ip,bool isencrypt,uint32 userdata /*= 0*/,int32 serverId /*= 0*/ )
{
	if(pdata==NULL||length==NULL)
	{
		return false;
	}

	boost::mutex::scoped_lock lock(m_ConnectCollect_Mutex);
	ConnectMap::iterator it=m_ConnectCollect.find(ip);
	if(it==m_ConnectCollect.end())
	{
		return false;
	}
	it->second->send(messageid,pdata,length,isencrypt,userdata,serverId);
	return true;

}


//-------------------------------------------------------------------------------------
void NetworkServer::update()
{

	///先把所有收到的消息广播
	boost::mutex::scoped_lock  PackLock(m_ReceivePackCollect_Mutex);
	NetWorkPackCollect temReceivePack=m_ReceivePackCollect;
	m_ReceivePackCollect.clear();
	PackLock.unlock();

	NetWorkPackCollect::iterator beginpack=temReceivePack.begin();
	NetWorkPackCollect::iterator endpack=temReceivePack.end();
	for(;beginpack!=endpack;++beginpack)
	{
		if((*beginpack).use_count()==0)
		{
			continue;
		}
		m_LastMsgRole = (*beginpack)->getUserData();
		m_LastMsgID = (*beginpack)->getMessagID();
		//遍历listener，调用onMessage
		std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onMessage,_1,*beginpack));

		if(m_RecivePackFunc!=NULL)
		{
			m_RecivePackFunc(*beginpack);
		}
		else
		{
			fireMessage((*beginpack)->getMessagID(),&(*beginpack));
		}

	}



	///广播所有新进入的连接
	boost::mutex::scoped_lock newlock(m_NewConnectCollect_Mutex);
	ConnectDeque::iterator newit= m_NewConnectCollect.begin();
	ConnectDeque::iterator newitend= m_NewConnectCollect.end();
	for(;newit!=newitend;++newit)
	{
		std::cout << "onConnect=" << *newit << std::endl;
		std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onConnect,_1,*newit));


	}
	if(m_NewConnectCollect.size()>0)
	{
		//NLOG(xstring("finish onConnect size="+Helper::IntToString(m_NewConnectCollect.size())+"\n").c_str());
		m_NewConnectCollect.clear();
	}
	newlock.unlock();






	///广播所有离开的连接
	boost::mutex::scoped_lock closelock(m_CloseConnectCollect_Mutex);
	ConnectDeque::iterator closeit=m_CloseConnectCollect.begin();
	ConnectDeque::iterator closeitend=m_CloseConnectCollect.end();
	for(;closeit!=closeitend;++closeit)
	{
		std::cout << "onCloseConnect=" << *closeit << std::endl;
		std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onCloseConnect,_1,*closeit));

		///从正常队列中拿走
		boost::mutex::scoped_lock alllock(m_ConnectCollect_Mutex);
		ConnectMap::iterator it=m_ConnectCollect.find(*closeit);
		if(it!=m_ConnectCollect.end())
		{
			m_ConnectCollect.erase(it);
		}


	}
	if(m_CloseConnectCollect.size()>0)
	{
		//NLOG(xstring("finish onCloseConnect size="+Helper::IntToString(m_CloseConnectCollect.size())+"\n").c_str());
		m_CloseConnectCollect.clear();
	}
	closelock.unlock();


	return ;
}


//-------------------------------------------------------------------------------------

/**刷帧函数;*/
/*
void NetworkServer::netUpdate()
{

	///先把所有收到的消息广播
	boost::mutex::scoped_lock  PackLock(m_ReceivePackCollect_Mutex);
	NetWorkPackCollect temReceivePack=m_ReceivePackCollect;
	m_ReceivePackCollect.clear();
	PackLock.unlock();

	NetWorkPackCollect::iterator beginpack=temReceivePack.begin();
	NetWorkPackCollect::iterator endpack=temReceivePack.end();
	for(;beginpack!=endpack;++beginpack)
	{
		if((*beginpack).use_count()==0)
		{
			continue;
		}
		if(m_RecivePackFunc!=NULL)
		{
			m_RecivePackFunc(*beginpack);
		}
		else
		{
			fireMessage((*beginpack)->getMessagID(),&(*beginpack));
		}

	}



	///广播所有新进入的连接
	boost::mutex::scoped_lock newlock(m_NewConnectCollect_Mutex);
	ConnectDeque::iterator newit= m_NewConnectCollect.begin();
	ConnectDeque::iterator newitend= m_NewConnectCollect.end();
	for(;newit!=newitend;++newit)
	{
		std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onConnect,_1,*newit));


	}
	m_NewConnectCollect.clear();
	newlock.unlock();






	///广播所有离开的连接
	boost::mutex::scoped_lock closelock(m_CloseConnectCollect_Mutex);
	ConnectDeque::iterator closeit=m_CloseConnectCollect.begin();
	ConnectDeque::iterator closeitend=m_CloseConnectCollect.end();
	for(;closeit!=closeitend;++closeit)
	{
		std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onCloseConnect,_1,*closeit));

		///从正常队列中拿走
		boost::mutex::scoped_lock alllock(m_ConnectCollect_Mutex);
		ConnectMap::iterator it=m_ConnectCollect.find(*closeit);
		if(it!=m_ConnectCollect.end())
		{
			m_ConnectCollect.erase(it);
		}


	}
	m_CloseConnectCollect.clear();
	closelock.unlock();


	return ;
}
*/
bool NetworkServer::closeConnect( const xstring& ip )
{
	xstring ipaddr;
	if(Helper::hostToIPAddress(ip,ipaddr)==false)
	{
		return false;
	}
	boost::mutex::scoped_lock lock(m_ConnectCollect_Mutex);
	ConnectMap::iterator it=m_ConnectCollect.find(ipaddr);
	if(it==m_ConnectCollect.end())
	{
		return false;
	}
	it->second->close();
	return true;
}
