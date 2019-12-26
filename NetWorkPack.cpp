
#include "stdafx.h"
#include "NetWorkPack.h"
unsigned char net_byteChange(char input)
{
	///00001111
	unsigned char high4=0xF&input;
	unsigned char low4=0xF0&input;
	unsigned char final=((high4<<4)&0xF0)|((low4>>4)&0XF);
	return final;
}

void net_encrypt(char *buff,int len)
{
	for(int r=0;r<len;r++)
	{
		buff[r]^=64;
		buff[r]=net_byteChange(buff[r]);
	}
}
void net_decrypt(char *buff,int len)
{
	for(int r=0;r<len;r++)
	{
		buff[r]=net_byteChange(buff[r]);
		buff[r]^=64;
	}
}
//------------------------------------------------------------------------
NetWorkPack::NetWorkPack()
	:m_pbuffer(NULL),m_pbufferDec(NULL),m_Size(0),m_encryptpack(false)
{


}


//------------------------------------------------------------------------
NetWorkPack::NetWorkPack(uint32 MessageID,const  void* pdata,uint32 length,bool encrypt,uint32 userdata,int32 serverId /*= 0*/)
:m_pbuffer(NULL),m_pbufferDec(NULL),m_Size(0),m_encryptpack(encrypt)
{
	if(pdata==NULL||length==0)
	{
		return ;
	}

	///数据包定义
   ///包头(两字节)+(版本1字节)+（消息包长度2字节,整包长度，加包头版本号)+(消息id4个字节)+(消息体)
	///包头有10个字节
	m_Size=length+PACK_HEAD_SIZE;
	m_pbuffer=(char*) malloc(m_Size);
	memset(m_pbuffer,0,m_Size);
	///先把包头写入
	if(encrypt)
	{
		m_pbuffer[PACK_HEAD_OFFSET]=PACKHEADENCRYPT;
	}
	else
	{
		m_pbuffer[PACK_HEAD_OFFSET]=PACKHEAD0;
	}
	m_pbuffer[PACK_HEAD_OFFSET+1]=PACKHEAD1;
	//写入版本号
	m_pbuffer[PACK_VERSION_OFFSET]=PACKVERSION;

	///写入长度
	uint32* lh=(uint32*)(m_pbuffer+PACK_LENGTH_OFFSET);
	(*lh)=m_Size;

	///写入消息id
	uint32* messageid=(uint32*)(m_pbuffer+PACK_MESSSAGEID_OFFSET);
	(*messageid)=MessageID;

	uint32* m_userdata=(uint32*)(m_pbuffer+PACK_USERDATA_OFFSET);
	(*m_userdata)=userdata;

	///写入服务器id数据;
// 	m_pbuffer[PACK_SERVERID_OFFSET] = serverId;
	uint32* pServerId=(uint32*)(m_pbuffer+PACK_SERVERID_OFFSET);
	(*pServerId)=serverId;

	///写入数据包
	memcpy(m_pbuffer+PACK_MESSAGE_OFFSET,pdata,length);
	if(encrypt)
	{
		net_encrypt(m_pbuffer+PACK_MESSAGE_OFFSET,length);
	}


}


//------------------------------------------------------------------------
//*
bool   NetWorkPack:: resize(uint32 messageid,uint32 userdata,int32 serverId,uint32 messageLength)
{
	if(messageLength==0)
	{
		return false	;

	}
	SafeFree(m_pbuffer);
	SafeDelete(m_pbufferDec);

	m_Size=messageLength+PACK_HEAD_SIZE;
	m_pbuffer=(char*) malloc(m_Size);
	memset(m_pbuffer,0,m_Size);

	///先把包头写入
	if(m_encryptpack)
	{
		m_pbuffer[PACK_HEAD_OFFSET]=PACKHEADENCRYPT;
	}
	else
	{
		m_pbuffer[PACK_HEAD_OFFSET]=PACKHEAD0;
	}
	m_pbuffer[PACK_HEAD_OFFSET+1]=PACKHEAD1;
	//写入版本号
	m_pbuffer[PACK_VERSION_OFFSET]=PACKVERSION;

	///写入长度
	uint32* lh=(uint32*)(m_pbuffer+PACK_LENGTH_OFFSET);
	(*lh)=m_Size;

	///写消息id
	uint32* pmessageid=(uint32*)(m_pbuffer+PACK_MESSSAGEID_OFFSET);
	(*pmessageid)=messageid;


	///写用户数据
	uint32* puserdata=(uint32*)(m_pbuffer+PACK_USERDATA_OFFSET);
	(*puserdata)=userdata;

	///写入服务器id数据;
// 	m_pbuffer[PACK_SERVERID_OFFSET] = serverId;
	uint32* pServerId=(uint32*)(m_pbuffer+PACK_SERVERID_OFFSET);
	(*pServerId)=serverId;


	return true;

}
//*/

//------------------------------------------------------------------------
void  NetWorkPack::reset()
{
	SafeFree(m_pbuffer);
	SafeDelete(m_pbufferDec);
	m_Size=0;
}

//------------------------------------------------------------------------
NetWorkPack::~NetWorkPack()
{
	SafeFree(m_pbuffer);
	SafeDelete(m_pbufferDec);
}


//------------------------------------------------------------------------
uint32  NetWorkPack::getBodyLength()
{
	if(m_pbuffer==NULL)
	{
		return 0;
	}
	///写入长度
	uint32* lh=(uint32*)(m_pbuffer+PACK_LENGTH_OFFSET);
	return  (*lh)-PACK_HEAD_SIZE;
	
}

//------------------------------------------------------------------------
uint8 NetWorkPack::getVersion()const
{
	if(NULL == m_pbuffer)
	{
		return 0;
	}

	uint8* version = (uint8*)(&(m_pbuffer[PACK_VERSION_OFFSET]));

	return *version;
}


//------------------------------------------------------------------------
uint32 NetWorkPack::getMessagID()const 
{
	if(m_pbuffer==NULL)
	{
		return 0;
	}
	///写入消息id
	uint32* messageid=(uint32*)(&(m_pbuffer[PACK_MESSSAGEID_OFFSET]));
	return *messageid;
}
//------------------------------------------------------------------------
uint32 NetWorkPack::getUserData() const
{
	if(m_pbuffer==NULL)
	{
		return 0;
	}
	///写入用户数据
	uint32* userdata=(uint32*)(&(m_pbuffer[PACK_USERDATA_OFFSET]));
	return* userdata;
}
//------------------------------------------------------------------------
void NetWorkPack::setUserData( uint32 userdata )
{
	if(m_pbuffer==NULL)
	{
		return;
	}
	///写入用户数据
	memcpy(&m_pbuffer[PACK_USERDATA_OFFSET],&(userdata),sizeof(uint32));
}


//------------------------------------------------------------------------
uint32  NetWorkPack::getSize()const 
{
	return m_Size;
}



//------------------------------------------------------------------------
const char* NetWorkPack::getData() 
{
	if(m_pbuffer==NULL)
	{
		return NULL;
	}
	if(m_encryptpack)
	{
		if(getBodyLength()==0)
		{
			return NULL;
		}
		if(m_pbufferDec==NULL)
		{
			int len=getBodyLength();
			m_pbufferDec=new char[len];
			memset(m_pbufferDec,0,len);
			memcpy(m_pbufferDec,m_pbuffer+PACK_MESSAGE_OFFSET,len);
			net_decrypt(m_pbufferDec,len);
		}
		return m_pbufferDec;
	}
	return 	m_pbuffer+PACK_MESSAGE_OFFSET;
}


//------------------------------------------------------------------------
bool NetWorkPack::isPackHead(const char* pHead,uint16 length,bool &isencrypt)
{
	if(length<PACK_HEAD_SIZE)
	{
		return false;
	}

	if( (pHead[0]!=PACKHEAD0&&pHead[0]!=PACKHEADENCRYPT)  ||  (pHead[1]!=PACKHEAD1))
	{
		return false;
	}
	if(pHead[0]!=PACKHEADENCRYPT)
	{
		isencrypt=false;
	}
	else
	{
		isencrypt=true;
	}

	if(pHead[PACK_VERSION_OFFSET]!=PACKVERSION)
	{
		return false;
	}

	return true; 
}

//------------------------------------------------------------------------

/**获取包中的服务器id;*/
int32 NetWorkPack::getServerId() const
{
// 	if(m_pbuffer==NULL)
// 	{
// 		return 0;
// 	}
// 	///写入用户数据
// 	char serverIdCh = m_pbuffer[PACK_SERVERID_OFFSET];
// 	int32 serverId = (int32)serverIdCh;
// 	return serverId;

	if(m_pbuffer==NULL)
	{
		return 0;
	}
	///写入用户数据
	int32* serverId =(int32*)(&(m_pbuffer[PACK_SERVERID_OFFSET]));
	return (*serverId);

}
