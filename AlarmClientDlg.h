
// AlarmClientDlg.h : 头文件
//

#pragma once
#include "NetworkServer.h"
#include "afxwin.h"
class NetworkClient;
// CAlarmClientDlg 对话框
class CAlarmClientDlg : public CDialogEx,public NetWorkLister
{
// 构造
public:
	CAlarmClientDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CAlarmClientDlg();
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ALARMCLIENT_DIALOG };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	void update();

	//void requestGetConf();

	virtual void  onConnect(const xstring& ip);

	virtual void  onCloseConnect(const xstring& ip);

	virtual void  onMessage(NetWorkPackPtr pack) {};
private:

	void showImage(const unsigned char* rawdata,int length);

	void processNotice(NetWorkPackPtr* pPack);

	void processPicture(NetWorkPackPtr* pPack);

	//  void processGetServerUrl(NetWorkPackPtr* pPack);

	void processHeartBeat(NetWorkPackPtr* pPack);

	void connect();

	xstring getUrl();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg void OnPaint();

	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();

	void ttsUpdateThread();

	LRESULT OnNotifyIcon(WPARAM wParam, LPARAM lParam);

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnClose();

	afx_msg void OnCancel();

	afx_msg void OnRbtnBStation();

	afx_msg void OnRbtnShowAlarm();

	afx_msg void OnRbtnQuit();

	void onPopBStationBoard();

	void showicon();

	void addWords(const xstring& strs);

// 实现
protected:

	NetworkClient *m_pClient;

	time_t m_nextConnect;

	xstring m_Addr;

	xstring m_mac;

	xstring m_serverAddr;

	time_t m_lastHeartbeat;

	bool  m_heartbeatSend;

	boost::thread *m_pThread;

	xstrings m_vecTtswords;

	HICON m_hIcon;

	int m_showstate;

	CImage m_OutImg;

	NOTIFYICONDATA nd;

	CStatic m_img;

	bool m_timerLock;

	xstring lastTtsFile;

	HWND m_player;

	time_t m_nextTtsTime;

};
