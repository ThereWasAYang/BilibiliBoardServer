

#pragma  once



#include "NetWorkPack.h"
#include <queue>

class ReceivePacker;
class NetworkServer;


class XClass Connection :public  boost::enable_shared_from_this<Connection>
{

	friend  class  NetworkServer;
	friend  class  NetworkClient;

protected:

	

	Connection(boost::asio::io_service& ioserver,ReceivePacker* pReceivePack);

public:

	virtual ~Connection();



	/**开始读取数据
	*/
	bool startRead();



	/**获取连接对方的ip地址和端口号
	*@param portnumber true: 127.0.0.1:344  false:127.0.0.1
	*/
	const xstring& getIP()const ;



	/**see getIP
	*/
	void setIP(const xstring& p){m_IP=p;};



public:




	/**发送消息，异步发送会先送到发送队列。然后再发送
	*@param message 消息id
	*@param pdata 消息内容
	*@param lenght pdata长度
	*/
	bool send(uint32 messageid,const char* pdata,uint32	lenght,bool isencrypt,uint32 roleid=0,int32 serverId = 0);


	template<typename T>
	bool send(uint32 messageid,const T& t)
	{
		return send(messageid,&t,sizeof(T));
	}
	///关闭连接
	void close();


	boost::asio::ip::tcp::socket& getSocket(){return m_Socket;}

protected:



	

	/**当sock有数据到来时的回调函数,读到消息包头
	*@
	*/
	void handReadHead(const boost::system::error_code& error, size_t len);



	/**读到包身回调
	*/
	void handReadBody(const boost::system::error_code& error, size_t len);



	/**从发送队列中取数据发送
	*/
	bool doSend();


	/**获取写入回调
	*/
	void handWrite(const boost::system::error_code& e);



protected:

	boost::asio::ip::tcp::socket  m_Socket;

	/////接收缓冲
	//std::vector<char>  m_ReceiveBuffer;
	
	xstring           m_IP;
	char              m_MessageHead[PACK_HEAD_SIZE];///缓存每次收到的包头信息

	NetWorkPackPtr    m_ReceivePack;///正在读取的包信息


	typedef std::vector<NetWorkPackPtr> NetWorkPackQueue;
	NetWorkPackQueue   m_SendPackCollect;  ///所有的发送缓冲集合
	boost::mutex       m_SendPackCollect_Mutex;///发送包的锁


	///正在发送的网络包
	bool               m_Sending;///是否正在发送中


	ReceivePacker*        m_ReceivePacker;


	bool             m_isClose;


	char* m_pSendBuff;

	uint32 m_iBuffSize;

	//int32 m_lastSendSize;
	int32 m_iSendPackCount;

};


typedef boost::shared_ptr<Connection> ConnectionPtr;

struct ConnectionPare
{
	ConnectionPtr ptr;
	uint32 index;
};