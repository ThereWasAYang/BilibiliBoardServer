#pragma once
#include "afxcmn.h"

struct GM_AVData
{
	char title[MAX_PATH];
	int play;
	int video_review;
	int comment;
	int favorites;
};
typedef std::map<xstring,GM_AVData> mapAVData;
struct GM_CurrentDayFirstData
{
	int curmonthday;
	int fansnum;
	int avlistcount;
};
// CBStationBoard 对话框

class CBStationBoard : public CDialogEx
{
	DECLARE_DYNAMIC(CBStationBoard)
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	void updateBStation();
	void updateFansNum();
	void updateVideoList();
	void loadCfg();
	void saveCfg();
	LRESULT OnGetFansAndAvListJson(WPARAM wParam, LPARAM lParam);
	xstring getFullCfgPath();
public:
	static bool curlGet(const xstring& url, xstring& resultstr);
	CBStationBoard(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CBStationBoard();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BSTATION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()

	HICON m_hIcon;
	mapAVData m_mapAVData;
	mapAVData m_mapAVDataCurrent;
	GM_CurrentDayFirstData m_currentDayFirstData;
	CListCtrl m_list;
	bool m_firstinit;
};
