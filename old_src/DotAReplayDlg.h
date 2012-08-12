// DotAReplayDlg.h : header file
//

#pragma once

#include "replay.h"
#include "locmanager.h"
#include "colorlist.h"
#include "views.h"

class CSettingsDlg;
class CPlayerStatsDlg;
class CScreenshotDlg;
class CHeroChart;

struct SearchStruct;

#define FILL_TREE           0
#define FILL_FOLDER         1
#define FILL_SEARCH         2

struct DateItem
{
  char path[256];
  CTime time;
  DateItem () {}
  DateItem (char const* p, CTime const& t)
  {
    strcpy (path, p);
    time = t;
  }
};

class CDotAReplayDlg;
class CDropTarget : public COleDropTarget
{
public:
  CDotAReplayDlg* dlg;
  virtual DROPEFFECT OnDragOver (CWnd* pWnd, COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point);
  virtual DROPEFFECT OnDropEx (CWnd* pWnd, COleDataObject* pDataObject,
    DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
};

class CProgressDlg;
// CDotAReplayDlg dialog
class CDotAReplayDlg : public CDialog
{
  CLocManager loc;
  CLocManager tabloc;
  POINT initSize;
  void* dst;
  CProgressDlg* progress;
  int chatFilters;
  CColorBox searchPlayer;
  void updateChat ();
  void updateSearchPlayer ();
  int firstView;
  int lastView;
  int curView;
// Construction
public:
  static CDotAReplayDlg* instance;

	CDotAReplayDlg(void* dest, CProgressDlg* prgb, CWnd* pParent = NULL);	// standard constructor
	~CDotAReplayDlg ();

// Dialog Data
	enum { IDD = IDD_DOTAREPLAY_DIALOG };

  void updateTreeView ();
  bool updating;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

  void OnChangePath ();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

  CDropTarget dropTarget;
  CRichEditCtrl chatbox;
  CWnd* tabwnd[16];
  CTabCtrl tabs;
  int curTab;
  CTreeCtrl files;
  CListCtrl curfold;
  CString viewPath;
  CSettingsDlg* settings;
  CPlayerStatsDlg* stats;
  CScreenshotDlg* scrn;
  CHeroChart* hchart;
  CProgressCtrl fload;
  int viewMode;
  bool backingUp;

  CMenu fileMenu;
  CMenu trayMenu;

  __int64 lastDate;

  W3GReplay* w3g;

  void applyView (int pos);

  HGLOBAL copyFiles (int* list = NULL, int* count = NULL);

  int fillFolder (char const* path, int mode, void* lparam = NULL);
  int fillByDate (char const* path, HTREEITEM parent);
  void batchCopy (char const* path, Array<CString>& files, int limit = -1);
  int fillYear (Array<DateItem>& reps, int& cur, int year, HTREEITEM parent);
  int fillMonth (Array<DateItem>& reps, int& cur, int month, HTREEITEM parent);
  int fillDate (Array<DateItem>& reps, int& cur, int date, HTREEITEM parent);
  
  virtual void OnOK ()
  {
    if (SetFocus ()->GetDlgCtrlID () == IDC_REPLAYFILE)
      OnBnClickedParsereplay ();
  }
  virtual void OnCancel ()
  {
  }
  afx_msg void OnClose ()
  {
    DestroyWindow ();
  }

  void updateView (int mode);

	// Generated message map functions
	virtual BOOL OnInitDialog();
  afx_msg void OnDestroy ();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
  virtual BOOL OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult);

  afx_msg void OnTimer (UINT_PTR nIDEvent);

  void modifyTabs (bool dota);

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedParsereplay();
  void OpenReplay ();
  afx_msg void OnTcnSelchangeTabs(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnItemActivateFilelist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedBatchhelp();
  afx_msg void OnBnClickedBatchcopy();
  afx_msg void OnBnClickedExplore();
  afx_msg void OnBnClickedBrowsereplay();

  void showPlayerInfo (int id);
  void showPlayerActions (int id);
  void selPlayerInfo (int id);
  void selPlayerActions (int id);
  void showPlayerStats (char const* name);
  void showReplay (char const* path);
  void showGamePlayers (W3GReplay* w3g);

  afx_msg void OnBnClickedRefresh();
  afx_msg void OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnEndlabeleditFilelist(NMHDR *pNMHDR, LRESULT *pResult);

  //afx_msg void OnDropFiles (HDROP hDropInfo);

  void pasteFiles (HDROP drop, bool move = false);
  void cacheAll ();

  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnMove (int x, int y);
  LRESULT OnTrayNotify (WPARAM wParam, LPARAM lParam);

  HDROP getCopiedFiles (COleDataObject* pDataObject = NULL);
  afx_msg void OnBnClickedSearch();
  afx_msg void OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedBydate();
  afx_msg void OnGetMinMaxInfo (MINMAXINFO* lpMMI);
  afx_msg BOOL OnCopyData (CWnd* pWnd, COPYDATASTRUCT* cd);
  afx_msg void OnBnClickedFindnext();
  afx_msg void OnBnClickedFilters();
  afx_msg void OnCbnSelchangeSearchtype();
  afx_msg void OnLvnBegindragFilelist(NMHDR *pNMHDR, LRESULT *pResult);

  DROPEFFECT OnDragOver (COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point);
  DROPEFFECT OnDropEx (COleDataObject* pDataObject,
    DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
  afx_msg void OnBnClickedBack();
  afx_msg void OnBnClickedForward();
};
