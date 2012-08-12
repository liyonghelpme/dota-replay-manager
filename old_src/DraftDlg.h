#pragma once

class W3GReplay;

#include "colorlist.h"
#include "locmanager.h"

// CPlayerInfoDlg dialog
class CDotAReplayDlg;

// CDraftDlg dialog

class CDraftDlg : public CDialog
{
	DECLARE_DYNAMIC(CDraftDlg)

  W3GReplay* w3g;
  CLocManager loc;

public:
	CDraftDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDraftDlg();

// Dialog Data
	enum { IDD = IDD_DRAFT };

  void setReplay (W3GReplay* w3g);
  afx_msg void OnSize (UINT nType, int cx, int cy);

protected:
  CColorList draftPool;
  CColorList draftBans;
  CColorList draftPicks;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  virtual void OnOK () {}
  virtual void OnCancel () {}

  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
