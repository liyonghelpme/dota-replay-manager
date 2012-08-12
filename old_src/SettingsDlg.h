#pragma once

#define COL_SAVED       0x01
#define COL_SIZE        0x02
#define COL_NAME        0x04
#define COL_RATIO       0x10
#define COL_LENGTH      0x20
#define COL_MODE        0x40

// CSettingsDlg dialog
#include "locmanager.h"

class CDotAReplayDlg;
class W3GReplay;
void unslash (CString& str);
void unslash (char* str);
void qualify (char* str);
void qualify (CString& str);

class CClickColor : public CWnd
{
	DECLARE_DYNAMIC(CClickColor)
  COLORREF bg;
  COLORREF fg;
  LOGFONT curfont;
  CFont font;
  int id;
  int curslot;
  int curTime;

public:
  void Create (CRect const& rc, CWnd* parent, int id = IDC_STATIC);

  void setColor (COLORREF b, COLORREF f)
  {
    bg = b;
    fg = f;
    InvalidateRect (NULL);
  }
  void setFont (LOGFONT const* newfont);

protected:
  afx_msg BOOL OnEraseBkgnd (CDC* pDC);
  afx_msg void OnPaint ();
  afx_msg void OnLButtonDown (UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp (UINT nFlags, CPoint point);
  afx_msg void OnTimer (UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};

class CSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CSettingsDlg)

  bool started;

  void getExtent (int t, CRect& rc);
  void switchTab ();

  CLocManager loc;

  CClickColor chatClr;

public:
	CSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSettingsDlg();

  CString replayPath;

  CString getBatchFormat ();
  CString getCopyName (W3GReplay* w3g, char const* filename, char const* batch = NULL);
  bool autoCopy ();
  bool autoView ();
  bool useTray ();

// Dialog Data
	enum { IDD = IDD_SETTINGSPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  void readSettings ();
  void applySettings ();

  virtual void OnOK () {}
  virtual void OnCancel () {}
  afx_msg void OnTimer (UINT_PTR nIDEvent);

  CDotAReplayDlg* dlg;
  CTabCtrl tab;

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnEnChangeReplaypath();
  afx_msg void OnBnClickedHelpformat();
  afx_msg void OnEnChangeCopyformat();
  afx_msg void OnBnClickedAutoview();
  afx_msg void OnBnClickedAutocopy();
  afx_msg void OnBnClickedAbout();
  afx_msg void OnBnClickedDrawchat();
  afx_msg void OnEnChangeChatlife();
  afx_msg void OnBnClickedUsetray();
  afx_msg void OnBnClickedViewwindow();
  afx_msg void OnEnChangeTimedeath();
  afx_msg void OnEnChangeRepdelay();
  afx_msg void OnBnClickedDrawwards();
  afx_msg void OnEnChangeWardlife();
  afx_msg void OnBnClickedResetpath();
  afx_msg void OnEnChangeMaxfiles();
  afx_msg void OnEnChangeImgurl();
  afx_msg void OnBnClickedReadme();
  afx_msg void OnBnClickedDrawpings();
  afx_msg void OnBnClickedUpdates();
  afx_msg void OnBnClickedAutoupdate();
  afx_msg void OnBnClickedShowlevels();
  afx_msg void OnBnClickedSetregopen();
  afx_msg void OnTcnSelchangeOpttabs(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedShowassemble();
  afx_msg void OnBnClickedSkillcolors();
  afx_msg void OnBnClickedSyncsel();
  afx_msg void OnBnClickedUselog();
  afx_msg void OnBnClickedShowdetails();
  afx_msg void OnBnClickedDetSaved();
  afx_msg void OnBnClickedDetSize();
  afx_msg void OnBnClickedDetName();
  afx_msg void OnBnClickedDetRatio();
  afx_msg void OnBnClickedDetLength();
  afx_msg void OnBnClickedSavecache();
  afx_msg void OnBnClickedDetMode();
  afx_msg void OnBnClickedEnableurl();
  afx_msg void OnBnClickedCacheall();
  afx_msg void OnBnClickedBrowsepath();
  afx_msg void OnBnClickedReset();
  afx_msg void OnBnClickedSmoothgold();
  afx_msg void OnBnClickedChatheroes();
  afx_msg void OnBnClickedIgnorebasic();
  afx_msg void OnEnChangeYourname();
  afx_msg void OnEnChangeRepdelayitem();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnEnChangeWarpath();
  afx_msg void OnBnClickedResetwar();
  afx_msg void OnBnClickedBrowsewar();
  afx_msg void OnBnClickedChatBG();
  afx_msg void OnBnClickedChatfont();
  afx_msg void OnCbnSelchangeChatplclr();
  afx_msg void OnBnClickedReltime();
  afx_msg void OnBnClickedChatassists();
  afx_msg void OnBnClickedShowempty();
  afx_msg void OnBnClickedDrawbuildings();
  //afx_msg void OnBnClickedForumicons();
};
