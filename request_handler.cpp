//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "request_handler.hpp"
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "server.hpp"
#include "Helper.h"

extern HWND g_hwnd;
namespace http {
namespace server {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
	
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  //std::string full_path = doc_root_ + request_path;
  //std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  //if (!is)
  //{
	 // rep = reply::stock_reply(reply::not_found);
	 // return;
  //}

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
 // char buf[512];
 // while (is.read(buf, sizeof(buf)).gcount() > 0)
	//rep.content.append(buf, is.gcount());

  //ªÿ∏¥–≠“È
  rep.content = "pay success!";

  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
}


void request_handler::handle_post(const request& req, std::string data, reply& rep,int contentlen,const xstring& ipaddr)
{
	rep = reply::stock_reply(reply::bad_request);
}

void request_handler::handle_get(const request& req, std::string data, reply& rep, int contentlen)
{
	xstring sendstr="invald json!";
	xstring decodeuri;
	if (!url_decode(req.uri, decodeuri))
	{
		decodeuri = req.uri;
	}

	if (strstr(decodeuri.c_str(), "/getBilibiliAvFans") != NULL)
	{
		xstring *retStr=new xstring();
		::SendMessage(g_hwnd, CALLBACKBJSON,(WPARAM)retStr,NULL);
		sendstr = Helper::Gb2312ToUTF_8(retStr->c_str());
		SafeDelete(retStr);
	}
	else
	{
		rep = reply::stock_reply(reply::bad_request);
		return;
	}
	rep.content.append(sendstr.c_str(), sendstr.length());
	rep.status = reply::ok;
	rep.headers.resize(3);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "text/html";
	rep.headers[2].name = "Access-Control-Allow-Origin";
	rep.headers[2].value = "*";
}
bool request_handler::url_decode(const std::string& encd,std::string& decd)   
{   
	int j,i;   
	char *cd =(char*) encd.c_str();   
	char p[2];   
	unsigned int num;   
	j=0;   
	char *buffer=new char[encd.length()+1];
	memset(buffer,0,encd.length()+1);
	for( i = 0; i < strlen(cd); i++ )   
	{   
		memset( p, '\0', 2 );   
		if( cd[i] != '%' )   
		{   
			buffer[j++] = cd[i];   
			continue;   
		}   

		p[0] = cd[++i];   
		p[1] = cd[++i];   

		p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);   
		p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);   
		buffer[j++] = (unsigned char)(p[0] * 16 + p[1]);   

	}   
	buffer[j] = '\0';   
	decd=buffer;
	delete []buffer;
	return true;   
}  

} // namespace server
} // namespace http
