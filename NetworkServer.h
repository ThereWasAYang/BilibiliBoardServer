

#pragma once
#include "Singleton.h"
#include "Connection.h"
#include "NetWorkPack.h"
#include "io_service_pool.h"
#include "EventManager.h"
#ifndef LIBSY_NOUSEPROTOBUFF
#include <google/protobuf/message.h>
#endif

typedef boost::function<void (NetWorkPackPtr  pPack  )> NetworkFun;
///网络监听器
class NetWorkLister
{
public:

	NetWorkLister(){}


	virtual void  onConnect(const xstring& ip)=0;

	virtual void  onCloseConnect(const xstring& ip)=0;

	virtual void  onMessage(NetWorkPackPtr pack)=0;


};

typedef std::vector<NetWorkLister*> NetWorkListerVector;






class Connection;


///接收回包对象
class ReceivePacker
{
public:

	ReceivePacker(){}


	virtual ~ReceivePacker(){}


	/**加入一个网络包
	*/
	void     addPack(const NetWorkPackPtr& pack)
	{
		if(pack.use_count()==0)
		{
			return ;
		}
		boost::mutex::scoped_lock lock(m_ReceivePackCollect_Mutex);
		m_ReceivePackCollect.push_back(pack);
	}


protected:


	typedef std::deque<NetWorkPackPtr> NetWorkPackCollect;

	NetWorkPackCollect                 m_ReceivePackCollect;

	boost::mutex                       m_ReceivePackCollect_Mutex;///互锁对象


};





///



class XClass NetworkServer  :public Singleton<NetworkServer> ,public  ReceivePacker ,public CEventManager
{


	 friend class Connection;

public:


	/**构造函数
	*@param IOthreadNumber io线程数量
	*@param workThreadNumber 工作线程数量
	*/
	NetworkServer(uint8 IOthreadNumber=4);


	virtual ~NetworkServer();


	/**开启服务器
	*@param portnumber listen network portnumber
	*@return return true if sucess else return false
	*/
	bool start(uint32 portnumber,uint32 maxConnect=65320);


	/**关闭服务器
	*/
	void stop();


	int32 getLastMsgRole()
	{
		return m_LastMsgRole;
	}


	int32 getlastMsgID()
	{
		return m_LastMsgID;
	}


	/**更新每帧收包，并广播收包消息。广播连接和断开消息
	*
	*/
	void update();

	/**刷帧函数;*/
	//void netUpdate();

	/**发送消息
	*@param messageid 消息id
	*@param pdata 消息内容
	*@param ip     目标ip
	*@param 异步发送,返回true 表示发送到发送队列当中
	*/
	bool send(uint32 messageid,const char* pdata,uint32 length,const xstring& ip,bool isencrypt,uint32 userdata = 0,int32 serverId = 0);


	template<typename T>
	bool send(uint32 mesageid ,const T& t,const xstring& ip,bool isencrypt, uint32 userdata = 0,int32 serverId = 0)
	{
		return send(mesageid,(const char *)&t,sizeof(T),ip,isencrypt,userdata,serverId);
	}
#ifndef LIBSY_NOUSEPROTOBUFF

	bool sendProtoBuf(uint32 mesageid ,const  ::google::protobuf::Message & pdata,const xstring& ip,bool isencrypt)
	{
		std::string buf;
		pdata.SerializePartialToString(&buf);
		return send(mesageid,buf.c_str(),(uint32)buf.length(),ip,isencrypt,0,0);
	}
#endif
	/*template<typename T>
	bool sendProtoBuf(uint32 mesageid ,const  T & pdata,const xstring& ip,uint32 userdata = 0,int32 serverId = 0)
	{
		std::string buf;
		pdata.SerializePartialToString(&buf);
		return send(mesageid,buf.c_str(),(uint32)buf.length(),ip,userdata,serverId);
	}
*/

	/**通过ip:port来获取指定的连接
	*这是多程安全函数
	*@return 需要判断是否正确获取了连接
	*/
	ConnectionPtr getConnectByIP(const xstring& ip);


	/**连接到远程机器
	*/
	bool connect(const xstring& ip ,uint16 portnumber);

	/**断开一个连接
	*/
	bool closeConnect(const xstring& ip);

	/**加入一个监听
	*/
	void addLister(NetWorkLister* pLister);


	/**移出一个监听
	*/
	void removeLister(NetWorkLister* pLister);

	/**设置收包操作函数;*/
	void setRecivePackFunc(NetworkFun func)	{m_RecivePackFunc = func;}

private:

	void start(xstring hostname,uint32 portnumber,uint32 maxConnect);

protected:


	///返回asio::io_server对像
	 boost::asio::io_service& getIOServer(){return m_IOServerPool.get_io_service();}


	 /**等待连接并发执行
	 */
	 void waitConnect(uint32 index);



	 ///有客户端连接的回调函数,并发执行。
	 void handleConnect(ConnectionPare pConnectPare,const boost::system::error_code& e);



public:
	 /**通知有连接关闭
	 */
	 void notifyCloseConnect(const xstring& ip);


	 /**通知有连接进入
	 */
	 void notifyNewConnect(const xstring&   ip);







protected:



	io_service_pool                m_IOServerPool;///工作线程池


	std::vector<boost::asio::ip::tcp::acceptor*> m_pAcceptorCollects;


	///所有连接集合
	//typedef std::list<ConnectionPtr> ConnectList;
	typedef xHashMap<xstring,ConnectionPtr>ConnectMap;


	///所有连接的集合
	ConnectMap         m_ConnectCollect;
	boost::mutex       m_ConnectCollect_Mutex;



	typedef std::deque<xstring> ConnectDeque;
	///所以的新连接
	ConnectDeque        m_NewConnectCollect;
	///新连接集合锁
	boost::mutex       m_NewConnectCollect_Mutex;


	///所有断开连接
	ConnectDeque        m_CloseConnectCollect;
	///断开连接锁
	boost::mutex       m_CloseConnectCollect_Mutex;




	////网络监听器
	NetWorkListerVector   m_Listeners;

	NetworkFun m_RecivePackFunc;				///收取数据所有数据包操作函数;

	int32 m_LastMsgRole;				//最后发送消息的角色
	int32 m_LastMsgID;					//最后发送的消息ID
};
