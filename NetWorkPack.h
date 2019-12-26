/*********************************************************************************************
数据包定义
包头(两字节)+(版本1字节)+（消息包长度4字节,整包长度，加包头版本号)+(消息id4个字节)+(用户数据4字节)+(消息体)
***********************************************************************************************/







#pragma once
#include "Helper.h"
///定议包头的前两个字
#define PACKHEAD0  0x08
#define PACKHEADENCRYPT	0x09
#define PACKHEAD1  8


///包的版本号
#define PACKVERSION 1


///定义包头偏移量
#define  PACK_HEAD_OFFSET        0
#define  PACK_VERSION_OFFSET     2
#define  PACK_LENGTH_OFFSET      3
#define  PACK_MESSSAGEID_OFFSET  7
#define  PACK_USERDATA_OFFSET	11
#define	 PACK_SERVERID_OFFSET	15		///服务器id;
#define  PACK_MESSAGE_OFFSET     19		///包体数据;

#define  PACK_HEAD_SIZE         19

///最大的包大小
//#define  MAXPACKSIZE 65536
//#define  MAXMESSAGESIZE  65536 - 9





class XClass NetWorkPack
{

public:
	NetWorkPack();


	/**构造函数
	*@param ModeID 模块id
	*@param MessageID 消息id
	*@param pdata 数据体
	*@param leght pdata的长度
	*/
	NetWorkPack(uint32 MessageID,const void* pdata,uint32 length,bool encrypt,uint32 userdata=0,int32 serverId = 0);

	/**重新发配包大小
	*/
	bool  resize(uint32 messageid,uint32 userdata,int32 serverId, uint32 messageLength);


	/**
	*/
	virtual ~NetWorkPack();

	/**重置包
	*/
	void  reset();


	/**获取数据包(消息体)的大小
	*/
	uint32  getBodyLength();


	/**获取消息版本*/
	uint8 getVersion()const;
	
	/**获取消息id
	*/
	uint32 getMessagID()const ;

	/**获取包用户数据
	*/
	uint32 getUserData()const;

	/**设置用户数据*/
	void setUserData(uint32 userdata);

	/**获取包中的服务器id;*/
	int32 getServerId() const;


	/**获取整包大小。包括包头
	*/
	uint32  getSize()const ;



	/**获取消息包数据
	*/
	const char* getData() ; 

		/**获取消息包数据
	*/
	char* getRawData()
	{
		if(m_pbuffer==NULL)
		{
			return NULL;
		}
		return 	m_pbuffer+PACK_MESSAGE_OFFSET;
	}
	

	bool getIsEncrypt()
	{
		return m_encryptpack;
	}

	void setEncrpyt()
	{
		m_encryptpack=true;
	}


	/**获取包数据，包括数据头*/
	const char* getBuffer()const {return m_pbuffer;}


	/**判断phead是否是正确的包头信息
	*/
	static bool isPackHead(const char* pHead,uint16 length,bool &isencrypt);
	


	/**获取包的发送者的ip
	*/
	const xstring& getIP()const {return m_IP;}

	/**获取发包者的端口号*/
	const uint32 getPort()const
	{
		for(uint32 r=0;r<m_IP.length();r++)
		{
			if(m_IP[r]==':')
			{
				return Helper::StringToInt(m_IP.c_str()+r+1);
			}
		}
		return 0;
	}

	void setIP(const xstring& ip){m_IP=ip;}

protected:

	char*  m_pbuffer;///内容块

	char *	m_pbufferDec;

	uint32 m_Size;///m_pbuffer长度


	xstring m_IP;
	
	bool m_encryptpack;
};


typedef boost::shared_ptr<NetWorkPack> NetWorkPackPtr;