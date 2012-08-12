#pragma once

#include "glib.h"
#include "replay.h"

class W3GReplay;
#include "colorlist.h"
#include "locmanager.h"

// CActionHistory

class CActionHistory : public CWnd
{
	DECLARE_DYNAMIC(CActionHistory)

//  OpenGL* gl;
  W3GReplay* w3g;
  int pid;
  CRect rc;

public:
	CActionHistory(CRect const& rc, CWnd* parent);
	virtual ~CActionHistory();

  void setReplay (W3GReplay* w3g);
  void setPlayer (int id);

protected:
  afx_msg int OnCreate (LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint ();
  afx_msg void OnDestroy ();
  afx_msg void OnSize (UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
};

// CActionsDlg dialog
class CDotAReplayDlg;

class CActionsDlg : public CDialog
{
	DECLARE_DYNAMIC(CActionsDlg)

  W3GReplay* w3g;
  CDotAReplayDlg* dlg;
  bool fake;
  CLocManager loc;

public:
	CActionsDlg(CDotAReplayDlg* d, CWnd* pParent = NULL);   // standard constructor
	virtual ~CActionsDlg();

// Dialog Data
	enum { IDD = IDD_PLAYERACTIONS };

  void setReplay (W3GReplay* replay);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  int curPlayer;
  CColorBox selPlayer;
  CBarList actions;
  CListCtrl hkeys;
  //CActionChart* chart;
  CActionHistory* history;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnCbnSelchangeSelplayer();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  virtual void OnOK () {}
  virtual void OnCancel () {}
  void selectPlayer (int id);
};
