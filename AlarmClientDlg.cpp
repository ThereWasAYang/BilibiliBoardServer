
// AlarmClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AlarmClient.h"
#include "AlarmClientDlg.h"
#include "afxdialogex.h"
#include "usermessage.h"
#include "NetworkClientSync.h"
#include "NetWorkClient.h"
#include "xLogManager.h"
#include "initimg.h"
#include "BStationBoard.h"
#include <vfw.h>
#pragma comment(lib,"vfw32.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAlarmClientDlg 对话框



CAlarmClientDlg::CAlarmClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ALARMCLIENT_DIALOG, pParent), m_nextConnect(time(NULL)), m_Addr("home.epcdiy.org:30002"), m_lastHeartbeat(0), m_heartbeatSend(false), m_pThread(NULL), m_timerLock(false), m_player(NULL), m_nextTtsTime(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pClient = new NetworkClient();
	m_pClient->addLister(this);
	m_pClient->registerMessageHandle(FN_CALLFOUND, &CAlarmClientDlg::processNotice, this);
	m_pClient->registerMessageHandle(FN_FOUNDIMAGE, &CAlarmClientDlg::processPicture, this);
	m_pClient->registerMessageHandle(FN_HEARTBEAT, &CAlarmClientDlg::processHeartBeat, this);
	m_mac = Helper::getComputerId();
}

CAlarmClientDlg::~CAlarmClientDlg()
{
	Shell_NotifyIcon(NIM_DELETE, &nd);
	SafeDelete(m_pClient);
	if (m_pThread != NULL)
	{
		m_pThread->interrupt();
		SafeDelete(m_pThread);
	}
	if (m_player != NULL)
	{
		MCIWndDestroy(m_player);
	}
	if (lastTtsFile.length() > 0)
	{
		DeleteFile(lastTtsFile.c_str());
	}
}

void CAlarmClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PEOPLEIMAGE, m_img);
}

void CAlarmClientDlg::update()
{
	m_pClient->update();
	if (time(NULL) > m_nextConnect&&m_nextConnect != 0)
	{
		m_nextConnect = 0;
		connect();
	}
	if (m_lastHeartbeat != 0)
	{
		if (time(NULL) - m_lastHeartbeat > 15 && m_heartbeatSend == false)
		{
			m_pClient->send(FN_HEARTBEAT, TOKENKEY, strlen(TOKENKEY), false);
			m_heartbeatSend = true;
		}
		if (time(NULL) - m_lastHeartbeat > 30 && m_heartbeatSend)
		{
			m_heartbeatSend = false;
			m_nextConnect = time(NULL);
		}
	}

}



void CAlarmClientDlg::onConnect(const xstring& ip)
{
	m_lastHeartbeat = time(NULL);
	m_nextConnect = 0;
	if (m_serverAddr.length() > 0)
	{
		XLOGNOTICE("Server connect "+ip+" ,sendregister="+m_mac);
		m_pClient->send(FN_ADDCLIENT, m_mac.c_str(), m_mac.length(), false);
		//requestGetConf();
	}
}

void CAlarmClientDlg::onCloseConnect(const xstring& ip)
{
	XLOGNOTICE("m_nextConnect "+Helper::IntToString(m_nextConnect)+" onDisconnect "+ ip);
	m_lastHeartbeat = 0;
	m_heartbeatSend = false;
	m_serverAddr = "";
	m_nextConnect = time(NULL) + 1;
}

BEGIN_MESSAGE_MAP(CAlarmClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_NOTIFYICON, OnNotifyIcon)
	ON_BN_CLICKED(IDOK, &CAlarmClientDlg::OnBnClickedOk)
	ON_COMMAND(ID_RBTN_B32771, &CAlarmClientDlg::OnRbtnBStation)
	ON_COMMAND(ID_RBTN_32772, &CAlarmClientDlg::OnRbtnShowAlarm)
	ON_COMMAND(ID_RBTN_32773, &CAlarmClientDlg::OnRbtnQuit)
END_MESSAGE_MAP()


// CAlarmClientDlg 消息处理程序

void CAlarmClientDlg::showImage(const unsigned char* rawdata, int length)
{
	if (!m_OutImg.IsNull())
	{
		m_OutImg.Destroy();
	}
	IStream* pMemStream = SHCreateMemStream(rawdata, length);
	CComPtr<IStream> stream;
	stream.Attach(pMemStream); // Need to Attach, otherwise ownership is not transferred and we leak memory
	m_OutImg.Load(stream);
	Invalidate(TRUE);
	
}

void CAlarmClientDlg::processNotice(NetWorkPackPtr* pPack)
{
	xstring orgstr = std::string((char*)(*pPack)->getData(), (*pPack)->getBodyLength());
	std::string ss =Helper::UTF_8ToGb2312(orgstr.c_str());
	XLOGNOTICE("processNotice "+ss+" msgid="+Helper::IntToString ((*pPack)->getUserData()));
	m_showstate = SW_SHOW;
	SetDlgItemText(IDC_TIPTXT, ss.c_str());
	//    m_pClient->getConnectInstance(TOKENKEY)->setRoleID(pPack->getUserID());
	m_pClient->send(FN_CALLFOUND, m_mac.c_str(), m_mac.length(), false, (*pPack)->getUserData());
	addWords(orgstr);
	showImage(initpicture, sizeof(initpicture));
}

void CAlarmClientDlg::processPicture(NetWorkPackPtr* pPack)
{
	GM_ImageSend *rcvdata = (GM_ImageSend*)(*pPack)->getData();
	SetDlgItemText(IDC_FOUNDDATETIME, rcvdata->founddate);
	unsigned char *ptr = (unsigned char*)((*pPack)->getData()) + sizeof(GM_ImageSend);
	showImage(ptr, rcvdata->imagelen);
}



void CAlarmClientDlg::processHeartBeat(NetWorkPackPtr* pPack)
{
	TRACE("Get HeartBeat[%s]", xstring((*pPack)->getData(), (*pPack)->getBodyLength()).c_str());
	m_lastHeartbeat = time(NULL);
	m_heartbeatSend = false;
}

void CAlarmClientDlg::connect()
{
	XLOGNOTICE("connect!");
	xstring url = getUrl();
	xstrings strs;
	Helper::splitString(url, ":", strs);
	if (strs.size() == 2)
	{
		if (m_serverAddr.size() == 0)
		{
			XLOGNOTICE("sync connect "+ url);
			NetworkClientSync client(url);
			if (client.connect())
			{
				XLOGNOTICE("sync connected!");
				char *retPack = NULL;
				uint32 retlen = 0;
				if (client.send(FN_GETSERVERIP, m_mac.c_str(), (uint32)m_mac.length(), FN_GETSERVERIP, &retPack, retlen))
				{
					XLOGNOTICE("sync send backed! ");
					GM_ServerIp* datapack = (GM_ServerIp*)retPack;
					if (strcmp(datapack->serverip, NOCHANGE) == 0)
					{
						m_serverAddr = m_Addr;
					}
					else {
						m_serverAddr = datapack->serverip;
					}
					XLOGNOTICE("Server ip:  "+ m_serverAddr);
				}
				else
				{
					XLOGERR("sync send failed! try again...");
				}
				delete[]retPack;
			}
			else
			{
				XLOGERR("sync connect failed! try again...");
			}
			//            if (m_pClient->connect(strs[0], StringToInt(strs[1])))
			//            {
			//                m_pClient->registerMessageHandle(FN_GETSERVERIP,&speakClient::processGetServerUrl, this);
			//            } else {
			//                m_nextConnect = time(NULL) + 5;
			//            }
			m_nextConnect = time(NULL);
		}
		else {
			XLOGNOTICE("connect "+ m_serverAddr);
			strs.clear();
			Helper::splitString(m_serverAddr, ":", strs);
			if (strs.size() == 2)
			{
				if (m_pClient->connect(strs[0], Helper::StringToInt(strs[1])))
				{
					m_nextConnect = time(NULL) + 10;
				}
				else {
					m_nextConnect = time(NULL) + 5;
				}
			}
		}
	}
}

xstring CAlarmClientDlg::getUrl()
{
	return  m_Addr;
}

BOOL CAlarmClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
// 	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
// 	ASSERT(IDM_ABOUTBOX < 0xF000);
// 
// 	CMenu* pSysMenu = GetSystemMenu(FALSE);
// 	if (pSysMenu != NULL)
// 	{
// 		BOOL bNameValid;
// 		CString strAboutMenu;
// 		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
// 		ASSERT(bNameValid);
// 		if (!strAboutMenu.IsEmpty())
// 		{
// 			pSysMenu->AppendMenu(MF_SEPARATOR);
// 			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
// 		}
// 	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	m_showstate = SW_HIDE;
	ShowWindow(m_showstate);
	// TODO: 在此添加额外的初始化代码
	showImage(initpicture, sizeof(initpicture));
	SetTimer(1, 50,NULL);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CAlarmClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAlarmClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		if (!m_OutImg.IsNull())
		{
			CRect clinetrect;
			m_img.GetWindowRect(&clinetrect);
			if (clinetrect.right!= 0)
			{
				ScreenToClient(clinetrect);
				CPaintDC dc(this);
				dc.SetStretchBltMode(HALFTONE);
				{
					int orgwidth = m_OutImg.GetWidth();
					int orghight = m_OutImg.GetHeight();
					float rectwidth = float(orgwidth)*(float(clinetrect.Height()) / float(orghight));
					float rectheight = clinetrect.Height();
					if (int(rectwidth) > clinetrect.Width())
					{
						rectwidth = clinetrect.Width();
						rectheight = float(rectwidth)*orghight / float(orgwidth);
						int y = (clinetrect.Height() - rectheight) / 2;
						if (y > 0)
						{
							clinetrect.top += y;
							clinetrect.bottom -= y;
						}
					}
					else
					{
						int x = (clinetrect.Width() - rectwidth) / 2;
						if (x > 0)
						{
							clinetrect.left += x;
							clinetrect.right -= x;
						}
					}
				}
				m_OutImg.Draw(dc.m_hDC, clinetrect.left, clinetrect.top, clinetrect.Width(), clinetrect.Height());
			}

		}
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAlarmClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



LRESULT CAlarmClientDlg::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)// The backdrop icon sent us a message.  Let's see what it is
	{
	case WM_RBUTTONDOWN:
		{
			CMenu menu;
			menu.LoadMenu(IDR_MENU1);
			CMenu *pPopUp = menu.GetSubMenu(0);
			CPoint pt;
			GetCursorPos(&pt);

			SetForegroundWindow();
			pPopUp->TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
			PostMessage(WM_NULL, 0, 0);
		}
		break;
	case WM_LBUTTONDOWN:
		m_showstate = SW_SHOW;
		break;
	}
	return 0;
}

void CAlarmClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	if (m_timerLock)
	{
		return;
	}
	m_timerLock = true;
	showicon();
	ShowWindow(m_showstate);
	update();
	ttsUpdateThread();
	m_timerLock = false;
}

void CAlarmClientDlg::OnClose()
{
	m_showstate = SW_HIDE;
}

void CAlarmClientDlg::OnCancel()
{
	if (MessageBox("退出HomeAlarm Client吗？", "退出", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		CDialogEx::OnCancel();
	}
}



void CAlarmClientDlg::showicon()
{
	nd.cbSize = sizeof(NOTIFYICONDATA);
	nd.hWnd = m_hWnd;
	nd.uID = IDR_MAINFRAME;
	nd.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nd.uCallbackMessage = WM_NOTIFYICON;
	nd.hIcon = m_hIcon;
	strcpy(nd.szTip, "HomeAlarm Client");
	Shell_NotifyIcon(NIM_ADD, &nd);
}



void CAlarmClientDlg::addWords(const xstring& strs)
{
	m_vecTtswords.push_back(strs);
}

void CAlarmClientDlg::OnBnClickedOk()
{
	m_showstate = SW_HIDE;
}


void CAlarmClientDlg::ttsUpdateThread()
{	
	if (time(NULL) <= m_nextTtsTime)
	{
		return;
	}
	//for (int r = 0; r < m_vecTtswords.size(); r++)
	if(m_vecTtswords.size()>0)
	{			
		xstring rawwav;
		if (CBStationBoard::curlGet("https://translate.google.cn/translate_tts?ie=UTF-8&client=tw-ob&q=" + m_vecTtswords[0] + "&tl=zh-CN", rawwav) && rawwav.length() > 0)
		{
			char temppath[MAX_PATH] = { 0 };
			GetTempPath(MAX_PATH, temppath);
			xstring fullpath = temppath + Helper::Long64ToString(time(NULL)) + ".mp3";
			FILE *fp = fopen(fullpath.c_str(), "wb");
			if (fp != NULL)
			{
				fwrite(rawwav.c_str(), 1, rawwav.length(), fp);
				fclose(fp);
				if (lastTtsFile.length() > 0)
				{
					DeleteFile(lastTtsFile.c_str());
				}
				if (m_player != NULL)
				{
					MCIWndDestroy(m_player);
				}
				lastTtsFile = fullpath;
				if (waveOutGetNumDevs() > 0)
				{
					m_player = MCIWndCreate(this->m_hWnd, NULL, MCIWNDF_NOPLAYBAR, fullpath.c_str());
					if (m_player != NULL)
					{
						::ShowWindow(m_player, SW_HIDE);
						MCIWndSetVolume(m_player, 1000);
						MCIWndPlay(m_player);
						m_nextTtsTime = time(NULL) + strlen(Helper::UTF_8ToGb2312(m_vecTtswords[0].c_str()).c_str())  / 6;
						//Sleep(strlen(Helper::UTF_8ToGb2312(m_vecTtswords[r].c_str()).c_str())*1000/6);
					}
				}
			}
		}
		m_vecTtswords.erase(m_vecTtswords.begin());
	}
}

void CAlarmClientDlg::OnRbtnBStation()
{
	if (m_pThread == NULL)
	{
		m_pThread = new boost::thread(boost::bind(&CAlarmClientDlg::onPopBStationBoard,this));
	}
}


void CAlarmClientDlg::OnRbtnShowAlarm()
{
	m_showstate = SW_SHOW;

}
void CAlarmClientDlg::OnRbtnQuit()
{
	OnCancel();
}

void CAlarmClientDlg::onPopBStationBoard()
{
	CBStationBoard dlg;
	dlg.Create(IDD_BSTATION);
	RECT rcWorkArea;
	//获得客户可用工作区
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0))
	{
		int nScrWidth = GetSystemMetrics(SM_CXSCREEN);
		int nScrHeight = GetSystemMetrics(SM_CYSCREEN);
		CRect rcWindow;
		::GetWindowRect(dlg.GetSafeHwnd(), rcWindow);
		//获得移动到右下角的区域
		CRect rcMoveRect;
		rcMoveRect.left = rcWorkArea.right - rcWindow.Width();
		rcMoveRect.right = rcWorkArea.right;
		rcMoveRect.top = rcWorkArea.bottom - rcWindow.Height();
		rcMoveRect.bottom = rcWorkArea.bottom;
		::MoveWindow(dlg.GetSafeHwnd(), rcMoveRect.TopLeft().x, rcMoveRect.TopLeft().y, rcMoveRect.Width(), rcMoveRect.Height(), FALSE);
	}
	dlg.RunModalLoop();
	SafeDelete(m_pThread);
}
