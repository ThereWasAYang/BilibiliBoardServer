
/******************************************************************************
网络模块客户
******************************************************************************/



#pragma  once


#include "NetworkServer.h"



class XClass NetworkClient :public ReceivePacker ,public CEventManager
{
public:

	NetworkClient();


	virtual ~NetworkClient();


	/**加入一个监听
	*/
	void addLister(NetWorkLister* pLister);


	/**移出一个监听
	*/
	void removeLister(NetWorkLister* pLister);


	/**联接到远程服务器
	*/
	bool connect(const xstring& serverip,uint32 portnumber);

	bool connect(const xstring& ipport);


	/**关闭联接
	*/
	bool closeConnect();
	/**通知有连接关闭
	 */
	 void notifyCloseConnect(const xstring& ip);

	void setUserData(uint32 data);
	/**发送数据
	*/
	bool send(uint32 messageid, const char* pdata,uint32 length,bool isencrypt,uint32 roleid=0,uint32 serverid=0);
#ifndef LIBSY_NOUSEPROTOBUFF
	bool sendProtoBuf(uint32 mesageid ,const  ::google::protobuf::Message & pdata,bool isencrypt,uint32 roleID=0,uint32 serverid=0)
	{
		std::string buf;
		pdata.SerializePartialToString(&buf);
		return send(mesageid,buf.c_str(),(uint32)buf.length(),isencrypt,roleID,serverid);
	}
#endif

	/**每帧更新
	*/
	void update();


	bool isConnect()const {return m_isConnect;}
	
	bool isFinishConnect()const {return m_isFinishConnect;}

protected:


	///返回asio::io_server对像
	boost::asio::io_service& getIOServer(){return m_IOServerPool.get_io_service();}

	
	 void handleConnect(ConnectionPtr pConnect,const boost::system::error_code& e);


protected:

	io_service_pool                m_IOServerPool;///工作线程池

	ConnectionPtr              m_pConnect;///远程联接

	NetWorkListerVector        m_Listeners;///监听集合

	bool                       m_isConnect ;

	bool							m_isFinishConnect;

	uint32						m_userData;
	
	typedef std::deque<xstring> ConnectDeque;
	///所以的新连接
	ConnectDeque        m_NewConnectCollect;
	///新连接集合锁
	boost::mutex       m_NewConnectCollect_Mutex;
	///所有断开连接
	ConnectDeque        m_CloseConnectCollect;
	///断开连接锁
	boost::mutex       m_CloseConnectCollect_Mutex;


};