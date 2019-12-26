

#include "stdafx.h"
#include "Helper.h"
#include "NetWorkClient.h"




//---------------------------------------------------------------------------------------------------
NetworkClient::NetworkClient()
	:m_isConnect(false),m_isFinishConnect(false),m_IOServerPool(1)
{
	m_IOServerPool.run();
	m_userData=0;
}



//---------------------------------------------------------------------------------------------------
NetworkClient::~NetworkClient()
{
	closeConnect();
	m_IOServerPool.stop();
	//Sleep(223);
}


//---------------------------------------------------------------------------------------------------
bool NetworkClient::connect(const xstring& serverip,uint32 portnumber)
{
	m_isConnect=false;
	m_isFinishConnect=false;
	xstring ipaddr;
	for(int r=0;r<3;r++)
	{
		if(Helper::hostToIPAddress(serverip,ipaddr))
		{
			break;
		}
	}
	if(ipaddr.length()==0)
	{
		ipaddr=serverip;
	}
	try
	{
		ConnectionPtr pConnect(new Connection(getIOServer(),this));
	
		boost::asio::ip::tcp::resolver resolver(pConnect->getSocket().get_io_service());
		boost::asio::ip::tcp::resolver::query query(ipaddr.c_str(),Helper::IntToString(portnumber));
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::async_connect(pConnect->getSocket(), endpoint_iterator,boost::bind(&NetworkClient::handleConnect, this,pConnect,
			boost::asio::placeholders::error));


	//m_thread=new boost::thread(boost::bind(&boost::asio::io_service::run,&m_IOServer));
	}
	catch (boost::exception &e)
	{
		m_isConnect=false;
		m_isFinishConnect=true;
		return false;
	}
	return true;
}

bool NetworkClient::connect(const xstring&  ipport )
{
	std::vector<xstring> ipportlist;
	Helper::splitString(ipport,":",ipportlist);
	if(ipportlist.size()!=2)
	{
		return false;
	}
	return connect(ipportlist[0],Helper::StringToInt(ipportlist[1]));
}



bool NetworkClient::closeConnect()
{
	if(m_pConnect!=NULL)
	{
		m_pConnect->close();
	}
	m_isConnect=false;
	return true;
}

//-------------------------------------------------------------------------
void NetworkClient::addLister(NetWorkLister* pLister)
{
	if(pLister==NULL)
	{
		return ;
	}

	m_Listeners.push_back(pLister);
}


//-------------------------------------------------------------------------
void NetworkClient::removeLister(NetWorkLister* pLister)
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



//---------------------------------------------------------------------------------------------------
bool NetworkClient::send(uint32 messageid,const char* pdata,uint32 length,bool isencrypt,uint32 roleid,uint32 serverid)
{
	if(pdata==NULL||length==0)
	{
		return false	;
	}

	if(m_pConnect!=NULL)
	{
		if(m_userData!=0)
		{
			roleid=m_userData;
		}
		return  m_pConnect->send(messageid,pdata,length,isencrypt,roleid,serverid);
	}
	return false;

}


//--------------------------------------------------------------------------------------------------
 void NetworkClient::handleConnect(ConnectionPtr pConnect,const boost::system::error_code& e)
 {
	 m_isFinishConnect=true;
	 if(!e)
	 {
		 m_isConnect=true;

		 m_pConnect=pConnect;
		 m_pConnect->startRead();
		 ///加入到新连接队列中
		 boost::mutex::scoped_lock  newlock(m_NewConnectCollect_Mutex);
		 m_NewConnectCollect.push_back(pConnect->getIP());

	 }else
	 {
		 m_isConnect=false;
	 }

 }
  //--------------------------------------------------------------------------------------------------

 void NetworkClient::notifyCloseConnect( const xstring& ip )
 {
	 boost::mutex::scoped_lock lock(m_CloseConnectCollect_Mutex);
	 m_CloseConnectCollect.push_back(ip);
	 lock.unlock();


	 return ;
 }

 //--------------------------------------------------------------------------------------------------
 void NetworkClient::update()
 {
	 boost::mutex::scoped_lock lock(m_ReceivePackCollect_Mutex);
	 NetWorkPackCollect::iterator it=m_ReceivePackCollect.begin();
	 NetWorkPackCollect::iterator itend=m_ReceivePackCollect.end();
	 for(;it!=itend;++it)
	 {
		 fireMessage((*it)->getMessagID(),&(*it));
		 std::for_each(m_Listeners.begin(),m_Listeners.end(),boost::bind(&NetWorkLister::onMessage,_1,*it));
	 }
	 
	 m_ReceivePackCollect.clear();

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
		 m_isConnect=false;
	 }
	 m_CloseConnectCollect.clear();
	 closelock.unlock();

 }

 void NetworkClient::setUserData( uint32 data )
 {
	 m_userData=data;
 }
