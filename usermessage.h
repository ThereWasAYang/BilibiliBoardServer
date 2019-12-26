#pragma once

#define  GM_ACCEPTCOME 100
enum USERMESSAGEXY
{
	FN_ADDCLIENT=1000,
	FN_CALLFOUND,
	FN_FOUNDIMAGE,
	FN_TASKADD=10000,
	FN_GETSERVERIP=11000,
	FN_GETCAMCONF,
	FN_HEARTBEAT
};
#pragma pack(push,1)
struct GM_ImageSend
{
	char peoplename[128] = { 0 };
	char founddate[24] = {0};
	int imagelen;
};

struct GM_Task
{
	char servername[256];
	int framecnt;
	int cachesize;
};


struct GM_ServerIp
{
    char serverip[256];
};
struct GM_ConfReq
{
    char mac[32];
};
struct GM_Conf
{
    char urllist[1024];
    char fpsposlist[128];
    char namelist[128];
};
#pragma pack(pop)
#define TOKENKEY		"DFBSB"
#define TOKENKEY2		"FUCKDK"
#define NOCHANGE		"NOCHANGE"