#pragma once

#include "colorlist.h"
#include "locmanager.h"

// CScreenshotDlg dialog
class CDotAReplayDlg;
class W3GReplay;

class CScreenshotDlg : public CDialog
{
	DECLARE_DYNAMIC(CScreenshotDlg)
  CDotAReplayDlg* dlg;
  W3GReplay* ignore;
  CLocManager loc;

public:
	CScreenshotDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScreenshotDlg();

// Dialog Data
	enum { IDD = IDD_SCREENSHOT };
  void refresh ();
  void fromGame (W3GReplay* w3g);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CColorList list;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  void processList ();
  void makeEmptyList ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnLvnItemActivatePlayers(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnItemchangedPlayers(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedModify();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  virtual void OnOK () {}
  virtual void OnCancel () {}
};
