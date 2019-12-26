#include "stdafx.h"
#include "NetworkClientSync.h"
#include "xLogManager.h"
void NetworkClientSync::setaddress( const xstring& serverip,uint32 portnumber )
{
	m_ip=serverip;
	m_port=portnumber;
}

bool NetworkClientSync::send( uint32 messageid, const char* pdata,uint32 length,uint32 retmessageId,char **retData,uint32 &retlength,bool noneedret )
{
	NetWorkPackPtr pPack(new NetWorkPack(messageid,pdata,length,false,m_userdata,0));
	boost::system::error_code ec;
	// 发送数据
	socket->write_some(boost::asio::buffer(pPack->getBuffer(),pPack->getSize()), ec);
	// 如果出错，打印出错信息
	if(ec)
	{
		XLOGERR(
			boost::system::system_error(ec).what() );
		closeconnect();
// #ifdef WIN32
// 		MessageBox(NULL,"数据发送错误","",MB_OK);
//#endif
		return false;
	}
	if(noneedret==false)
	{
#ifdef USETIMEOUT
		socket->io_control(boost::asio::ip::tcp::socket::non_blocking_io(true));
		char m_MessageHead[PACK_HEAD_SIZE];
		int cnt=0;
		time_t starttime=time(NULL);
		while(1)
		{
			cnt+=socket->read_some(boost::asio::buffer(m_MessageHead,PACK_HEAD_SIZE), ec);
			if(cnt>=PACK_HEAD_SIZE)
			{
				break;
			}
			time_t nowtime=time(NULL);
			if(starttime+m_timeoutval<nowtime)
			{
 				//char buffer[128];
 				//sprintf(buffer,"timeout! starttime=%d,timeout=%d,nowtime=%d",starttime,m_timeoutval,nowtime);
 				//MessageBox(NULL,buffer,"",MB_OK);
				closeconnect();
				return false;
			}
		}
		uint32* pMessageid=(uint32*)(m_MessageHead+PACK_MESSSAGEID_OFFSET);
		uint32* plength=(uint32*)(m_MessageHead+PACK_LENGTH_OFFSET);
		if(*pMessageid!=retmessageId)
		{
 			//char buffer[128];
 			//sprintf(buffer,"pMessageid=%d,retmessageId=%d",*pMessageid,retmessageId);
 			//MessageBox(NULL,buffer,"数据包头回包错误",MB_OK);
			closeconnect();
			return false;
		}
		retlength=*plength-PACK_HEAD_SIZE;
		*retData=new char[retlength];
		cnt=0;
		starttime=time(NULL);
		while(1)
		{
			cnt+=socket->read_some(boost::asio::buffer(*retData,retlength), ec);
			if(cnt>=retlength)
			{
				break;
			}
			time_t nowtime=time(NULL);
			if(starttime+m_timeoutval<nowtime)
			{
				//char buffer[128];
				//sprintf(buffer,"timeout!! starttime=%d,timeout=%d,nowtime=%d",starttime,m_timeoutval,nowtime);
				//MessageBox(NULL,buffer,"",MB_OK);
				closeconnect();
				return false;
			}
		}
		socket->io_control(boost::asio::ip::tcp::socket::non_blocking_io(false));
#else
		char m_MessageHead[PACK_HEAD_SIZE];
		socket->read_some(boost::asio::buffer(m_MessageHead,PACK_HEAD_SIZE), ec);
		uint32* pMessageid=(uint32*)(m_MessageHead+PACK_MESSSAGEID_OFFSET);
		uint32* plength=(uint32*)(m_MessageHead+PACK_LENGTH_OFFSET);
		if(*pMessageid!=retmessageId)
		{
			closeconnect();
// 			char buffer[128];
// 			sprintf(buffer,"pMessageid=%d,retmessageId=%d",*pMessageid,retmessageId);
// 			MessageBox(NULL,buffer,"数据包头回包错误",MB_OK);
			return false;
		}
		retlength=*plength-PACK_HEAD_SIZE;
		*retData=new char[retlength];
		size_t cnt=0;
		while(1)
		{
			cnt+=socket->read_some(boost::asio::buffer(*retData+cnt,retlength-cnt), ec);
			if(cnt>=retlength)
			{
				break;
			}
		}
#endif
	}
	return true;
}



bool NetworkClientSync::send( uint32 messageid, const char* pdata,uint32 length,uint32 retmessageId )
{
	char *data=NULL;
	uint32 retlength=0;
	bool ret=send(messageid,pdata,length,retmessageId,&data,retlength,true);
	SafeDeleteArray(data);
	return ret;
}

NetworkClientSync::~NetworkClientSync()
{
	closeconnect();
}

NetworkClientSync::NetworkClientSync()
{
	initvalue();
}

NetworkClientSync::NetworkClientSync(xstring &ipport)
{
	initvalue();
	std::vector<xstring> strs;
	Helper::splitString(ipport, ":", strs);
	if (strs.size() != 2)
	{
		return;
	}
	m_ip = strs[0];
	m_port = Helper::StringToInt(strs[1]);
}
void NetworkClientSync::initvalue()
{
	m_ip = "127.0.0.1";
	m_port = 50000;
	socket = NULL;
	m_userdata = 0;
	m_timeoutval = 20;
}
bool NetworkClientSync::connect()
{
	if(socket!=NULL)
	{
		closeconnect();
	}
	if(socket==NULL)
	{

		xstring ipaddr;
		for(int r=0;r<3;r++)
		{
			if(Helper::hostToIPAddress(m_ip,ipaddr))
			{
				break;
			}
		}
		if(ipaddr.length()==0)
		{
			ipaddr=m_ip;
		}
		//MessageBox(NULL,Helper::IntToString(m_port).c_str(),ipaddr.c_str(),MB_OK);
		try
		{
			// socket对象
			socket=new boost::asio::ip::tcp::socket(iosev);
			// 连接端点，这里使用了本机连接，可以修改IP地址测试远程连接
			//boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(m_ip.c_str()),m_port);
			boost::asio::io_service io_serv;
			boost::asio::ip::tcp::resolver::query q(ipaddr.c_str(), Helper::IntToString(m_port));
			boost::asio::ip::tcp::resolver rslvr(io_serv);
			boost::asio::ip::tcp::resolver::iterator iter = rslvr.resolve(q);
			boost::asio::ip::tcp::resolver::iterator end;
			boost::asio::ip::tcp::endpoint point;
			bool isfound=false;
			while(iter != end)
			{
				point = *iter++;
				isfound=true;
				break;
			}
			if(isfound==false)
			{
#ifdef WIN32
#ifdef _DEBUG
				MessageBox(NULL,"resolve failed","",MB_OK);
#endif
#endif
				return false;
			}
			// 连接服务器
			boost::system::error_code ec;
			socket->connect(point,ec);
			// 如果出错，打印出错信息
			if(ec)
			{
			    #ifdef WIN32
				#ifdef _DEBUG
				MessageBox(NULL,boost::system::system_error(ec).what(),"",MB_OK);
				#endif
				#endif
				XLOGERR (boost::system::system_error(ec).what() );
				SafeDelete(socket);
				return false;
			}
			return true;
		}
		catch (boost::exception &e)
		{
		    #ifdef WIN32
			#ifdef _DEBUG
			MessageBox(NULL,boost::diagnostic_information(e).c_str(),"",MB_OK);
			#endif
			#endif
			return false;
		}


	}
	return false;
}

bool NetworkClientSync::closeconnect()
{
	if(socket!=NULL)
	{
		socket->close();
		SafeDelete(socket);
		return true;
	}
	return false;
}
