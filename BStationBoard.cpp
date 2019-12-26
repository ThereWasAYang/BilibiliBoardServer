// BStationBoard.cpp : 实现文件
//

#include "stdafx.h"
#include "AlarmClient.h"
#include "BStationBoard.h"
#include "afxdialogex.h"
#include "curl/curl.h"
#include "json/json.h"
#include "Helper.h"
HWND g_hwnd;
#define CFGFILE	"bilibilicfg.dat"
size_t call_write_func(const char *ptr, size_t size, size_t nmemb, std::string *stream)
{
	//assert(stream != NULL);
	size_t len = size * nmemb;
	stream->append(ptr, len);
	return len;
}
void CBStationBoard::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	if (m_firstinit)
	{
		m_firstinit = false;
		SetTimer(1, 60000, NULL);
	}
	time_t curtime = time(NULL);
	struct tm* _tm = localtime(&curtime);
	if (_tm->tm_mday != m_currentDayFirstData.curmonthday&&m_mapAVDataCurrent.size()>0)
	{
		m_mapAVData = m_mapAVDataCurrent;
		m_currentDayFirstData.curmonthday = _tm->tm_mday;
		m_currentDayFirstData.fansnum = 0;
		updateBStation();
		saveCfg();
	}
	else
	{
		updateBStation();
	}
}

BOOL CBStationBoard::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	SetTimer(1, 100, NULL);
	loadCfg();
	m_list.InsertColumn(0, "↑", LVCFMT_LEFT, 60);
	m_list.InsertColumn(0, "收藏", LVCFMT_LEFT, 90);
	m_list.InsertColumn(0, "↑", LVCFMT_LEFT, 60);
	m_list.InsertColumn(0, "评论", LVCFMT_LEFT, 90);
	m_list.InsertColumn(0, "↑", LVCFMT_LEFT, 60);
	m_list.InsertColumn(0, "弹幕", LVCFMT_LEFT, 90);
	m_list.InsertColumn(0, "↑", LVCFMT_LEFT, 80);
	m_list.InsertColumn(0, "播放", LVCFMT_LEFT, 100);
	m_list.InsertColumn(0, "标题", LVCFMT_LEFT, 320);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_firstinit = true;
	g_hwnd = GetSafeHwnd();
	return TRUE;
}

void CBStationBoard::updateBStation()
{
	updateFansNum(); 
	updateVideoList();
}

void CBStationBoard::updateFansNum()
{
	xstring retdata;
	if (curlGet("https://api.bilibili.com/x/relation/stat?vmid=12590&jsonp=jsonp", retdata))
	{
		Json::Reader reader;
		Json::Value arrayObj;
		if (reader.parse(Helper::UTF_8ToGb2312(retdata.c_str()), arrayObj))
		{
			if (arrayObj["data"].isObject())
			{
				if (arrayObj["data"]["follower"].isInt())
				{
					int curfans = arrayObj["data"]["follower"].asInt();
					SetDlgItemInt(IDC_FANSNUM, curfans);
					if (m_currentDayFirstData.fansnum == 0)
					{
						m_currentDayFirstData.fansnum = curfans;
						SetDlgItemInt(IDC_FANSUP, 0);
					}
					else
					{
						SetDlgItemInt(IDC_FANSUP,curfans - m_currentDayFirstData.fansnum);
					}
				}
			}
		}
	}
}

void CBStationBoard::updateVideoList()
{
	xstring retdata;
	if (curlGet("https://space.bilibili.com/ajax/member/getSubmitVideos?mid=12590&pagesize=14&tid=0&page=1&keyword=&order=pubdate", retdata))
	{
		Json::Reader reader;
		Json::Value arrayObj;
		if (reader.parse(retdata.c_str(), arrayObj))
		{
			if (arrayObj["data"].isObject())
			{
				if (arrayObj["data"]["vlist"].isArray())
				{
					m_list.DeleteAllItems();
					m_mapAVDataCurrent.clear();
					Json::Value arrayList= arrayObj["data"]["vlist"];
					for (int r = 0; r < arrayList.size(); r++)
					{
						GM_AVData avdata;
						GM_AVData *avdata_compare=NULL;
						if (arrayList[r]["title"].isString())
						{
							strcpy_s(avdata.title,MAX_PATH, Helper::UTF_8ToGb2312(arrayList[r]["title"].asString().c_str()).c_str());
							mapAVData::iterator it = m_mapAVData.find(avdata.title);
							if (it != m_mapAVData.end())
							{
								avdata_compare = &it->second;
							}
							m_list.InsertItem(m_list.GetItemCount(), avdata.title);
							if (arrayList[r]["play"].isInt())
							{
								avdata.play = arrayList[r]["play"].asInt();
								xstring playstr = Helper::IntToString(avdata.play);
								m_list.SetItemText(m_list.GetItemCount() - 1, 1, playstr.c_str());
								if (avdata_compare != NULL&&avdata.play-avdata_compare->play>0)
								{
									playstr =Helper::IntToString(avdata.play - avdata_compare->play);
									m_list.SetItemText(m_list.GetItemCount() - 1, 2, playstr.c_str());
								}								
							}
							if (arrayList[r]["video_review"].isInt())
							{
								avdata.video_review = arrayList[r]["video_review"].asInt();
								xstring video_reviewstr = Helper::IntToString(avdata.video_review);
								m_list.SetItemText(m_list.GetItemCount() - 1, 3, video_reviewstr.c_str());
								if (avdata_compare != NULL&&avdata.video_review - avdata_compare->video_review > 0)
								{
									video_reviewstr =Helper::IntToString(avdata.video_review - avdata_compare->video_review);
									m_list.SetItemText(m_list.GetItemCount() - 1, 4, video_reviewstr.c_str());
								}
								
							}
							if (arrayList[r]["comment"].isInt())
							{
								avdata.comment=arrayList[r]["comment"].asInt();
								xstring commentstr = Helper::IntToString(avdata.comment);
								m_list.SetItemText(m_list.GetItemCount() - 1, 5, commentstr.c_str());
								if (avdata_compare != NULL&&avdata.comment - avdata_compare->comment > 0)
								{
									commentstr = Helper::IntToString(avdata.comment - avdata_compare->comment);
									m_list.SetItemText(m_list.GetItemCount() - 1, 6, commentstr.c_str());
								}
								
							}
							if (arrayList[r]["favorites"].isInt())
							{
								avdata.favorites = arrayList[r]["favorites"].asInt();
								xstring favoritesstr = Helper::IntToString(avdata.favorites);
								m_list.SetItemText(m_list.GetItemCount() - 1, 7, favoritesstr.c_str());
								if (avdata_compare != NULL&&avdata.favorites - avdata_compare->favorites > 0)
								{
									favoritesstr = Helper::IntToString(avdata.favorites - avdata_compare->favorites);
									m_list.SetItemText(m_list.GetItemCount() - 1, 8, favoritesstr.c_str());
								}
								
							}
							m_mapAVDataCurrent[avdata.title] = avdata;
						}
					}
				}
			}
		}
	}
}

bool CBStationBoard::curlGet(const xstring& url, xstring& resultstr)
{
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	// #ifdef _DEBUG
	// 	curl_easy_setopt(curl, CURLOPT_PROXY,"http://192.168.13.19:7777");
	// #endif
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, call_write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultstr);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);//get redirect content
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 5.1; rv:17.0) Gecko/20100101 Firefox/17.0");
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK)
	{
		return false;
	}
	
	return true;
}

void CBStationBoard::loadCfg()
{
	m_mapAVData.clear();
	m_currentDayFirstData.curmonthday = -1;
	m_currentDayFirstData.avlistcount = 0;
	m_currentDayFirstData.fansnum = 0;
	FILE *fp = fopen(getFullCfgPath().c_str(),"rb");
	if (fp != NULL)
	{
		fread(&m_currentDayFirstData, sizeof(GM_CurrentDayFirstData),1,fp);
		for (int r = 0; r < m_currentDayFirstData.avlistcount; r++)
		{
			GM_AVData avdata;
			fread(&avdata,sizeof(GM_AVData),1,fp);
			m_mapAVData[avdata.title]=avdata;
		}
		fclose(fp);
	}
}

void CBStationBoard::saveCfg()
{
	FILE *fp = fopen(getFullCfgPath().c_str(), "wb");
	if (fp != NULL)
	{
		m_currentDayFirstData.avlistcount = m_mapAVData.size();
		fwrite(&m_currentDayFirstData, sizeof(GM_CurrentDayFirstData), 1, fp);
		mapAVData::iterator it = m_mapAVData.begin();
		while (it != m_mapAVData.end())
		{
			fwrite(&it->second, sizeof(GM_AVData), 1, fp);
			it++;
		}
		fclose(fp);
	}
}

LRESULT CBStationBoard::OnGetFansAndAvListJson(WPARAM wParam, LPARAM lParam)
{
	Json::Value bilibiliData;
	Json::Value fansData;
	fansData["fansnum"] = GetDlgItemInt(IDC_FANSNUM);
	fansData["fansnumup"] = GetDlgItemInt(IDC_FANSUP);
	bilibiliData["fans"] = fansData;
	Json::Value avArray;
	for (int r = 0; r < m_list.GetItemCount(); r++)
	{
		Json::Value avData;
		avData["title"] = m_list.GetItemText(r,0).GetBuffer();
		avData["play"] = Helper::StringToInt(m_list.GetItemText(r,1).GetBuffer());
		avData["playup"] = Helper::StringToInt(m_list.GetItemText(r, 2).GetBuffer());
		avData["video_review"] = Helper::StringToInt(m_list.GetItemText(r, 3).GetBuffer());
		avData["video_reviewup"] = Helper::StringToInt(m_list.GetItemText(r, 4).GetBuffer());
		avData["comment"] = Helper::StringToInt(m_list.GetItemText(r, 5).GetBuffer());
		avData["commentup"] = Helper::StringToInt(m_list.GetItemText(r, 6).GetBuffer());
		avData["favorites"] = Helper::StringToInt(m_list.GetItemText(r, 7).GetBuffer());
		avData["favoritesup"] = Helper::StringToInt(m_list.GetItemText(r,8).GetBuffer());
		avArray.append(avData);
	}
	bilibiliData["avData"] = avArray;
	Json::FastWriter fast_writer;
	xstring *pStr = (xstring*)wParam;
	pStr->append(fast_writer.write(bilibiliData));
	return 0;
}

xstring CBStationBoard::getFullCfgPath()
{
	char tmpdir[MAX_PATH] = {0};
	::GetTempPath(MAX_PATH,tmpdir);
	strcat(tmpdir,"\\");
	strcat(tmpdir, CFGFILE);
	return tmpdir;
}

// CBStationBoard 对话框

IMPLEMENT_DYNAMIC(CBStationBoard, CDialogEx)

CBStationBoard::CBStationBoard(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_BSTATION, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CBStationBoard::~CBStationBoard()
{
}

void CBStationBoard::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_list);
}


BEGIN_MESSAGE_MAP(CBStationBoard, CDialogEx)
	ON_WM_TIMER()
	ON_MESSAGE(CALLBACKBJSON, OnGetFansAndAvListJson)
END_MESSAGE_MAP()


// CBStationBoard 消息处理程序
