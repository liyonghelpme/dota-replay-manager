#pragma once

class W3GReplay;

#include "colorlist.h"
#include "locmanager.h"
#include "ilib.h"

// CGameInfoDlg dialog

class CDotAReplayDlg;

class CMapPicture : public CWnd
{
  CBitmap* bmp;
public:
	CMapPicture (CRect const& rc, CBitmap* bitmap, CWnd* parent);

protected:
  afx_msg void OnPaint ();
  DECLARE_MESSAGE_MAP()
};

class CGameInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CGameInfoDlg)
  W3GReplay* w3g;
  CLocManager loc;
  CColManager icol;
  CMapPicture* mappic;

public:
	CGameInfoDlg(CDotAReplayDlg* dlg, CWnd* pParent = NULL);   // standard constructor
	virtual ~CGameInfoDlg();

// Dialog Data
	enum { IDD = IDD_GAMEINFO };

  void insertMainInfo (int name, char const* val);
  void insertMainInfo (int name, wchar_t const* val);

  void setReplay (W3GReplay* w3g, char const* filename = NULL);
  void setFailure (char const* msg);

protected:

  CListCtrl mainInfo;
  CColorList plInfo;
  CString fileName;
  CDotAReplayDlg* mdlg;
  RImageInfo bigmap;
  int picMode;

  CMenu popupMenu;

  void addPlayer (W3GReplay* w3g, int id);
  void addLadderPlayer (W3GReplay* w3g, int id);

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  void copyLine (int id);
  void copyName (int id);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual void OnOK () {}
  virtual void OnCancel () {}

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedWatchreplay();
  afx_msg void OnLvnItemActivatePlayerinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnKeydownPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnNMRclickPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedPlayerstats();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnHdnTrackPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLButtonUp (UINT nFlags, CPoint point);
  afx_msg void OnStnClickedMapimg();
  afx_msg void OnBnClickedWorkshop();
  afx_msg void OnStnDblclickMapimg();
  afx_msg void OnBnClickedCopyMatchup();
};
