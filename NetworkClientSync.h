#pragma  once

#include "Helper.h"
#include "NetworkServer.h"
class XClass NetworkClientSync
{
public:
	NetworkClientSync();
	NetworkClientSync(xstring &ipport);

	~NetworkClientSync();

	
	/**联接到远程服务器,是短连接,此函数将不执行连接,在发送的时候连接,发送并收到回包完成后断开
	*/
	void setaddress(const xstring& serverip,uint32 portnumber);

	//如果运行此函数,则启动长连接.
	bool connect();

	bool closeconnect();

	void setUserData(uint32 userdata)
	{
		m_userdata=userdata;
	}
	void setTimeOutVal(uint32 val)
	{
		m_timeoutval=val;
	}

		/**发送数据
	*/
	bool send(uint32 messageid, const char* pdata,uint32 length,uint32 retmessageId,char **retData,uint32 &retlength,bool noneedret=false);

	bool send(uint32 messageid, const char* pdata,uint32 length,uint32 retmessageId);
#ifndef LIBSY_NOUSEPROTOBUFF
	template<typename T>
	bool sendProtoBuf(uint32 mesageid ,const  ::google::protobuf::Message & pdata,uint32 retmessageId, T & retdata)
	{
		std::string buf;
		pdata.SerializePartialToString(&buf);
		char *retDataBuff=NULL;
		uint32 retlength=0;
		bool ret= send(mesageid,buf.c_str(),(uint32)buf.length(),retmessageId,&retDataBuff,retlength);
		if(ret==false)
		{
			return false;
		}
		if(retDataBuff==NULL||retlength==0)
		{
			//MessageBox(NULL,"socket返回空数据","",MB_OK);
			return false;
		}
		if(!retdata.ParseFromArray(retDataBuff,retlength))
		{
			//MessageBox(NULL,"数据解析错误",Helper::IntToString(mesageid).c_str(),MB_OK);
			SafeDeleteArray(retDataBuff);
			return false;
		}
		SafeDeleteArray(retDataBuff);
		return true;
	}
#endif
private:

	void initvalue();


	xstring m_ip;
	uint32 m_port;
	uint32 m_userdata;
	uint32 m_timeoutval;
	// 所有asio类都需要io_service对象
	boost::asio::io_service iosev;
	// socket对象
	boost::asio::ip::tcp::socket *socket;
};