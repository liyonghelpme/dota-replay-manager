#pragma once

#include "locmanager.h"

class W3GReplay;

// CWorkshopDlg dialog

class CWorkshopDlg : public CDialog
{
	DECLARE_DYNAMIC(CWorkshopDlg)
  CLocManager loc;
  CListBox list;
  W3GReplay* replay;

public:
	CWorkshopDlg(W3GReplay* w3g, CWnd* pParent = NULL);   // standard constructor
	virtual ~CWorkshopDlg();

// Dialog Data
	enum { IDD = IDD_WORKSHOP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnGetMinMaxInfo (MINMAXINFO* lpMMI);
  virtual void OnOK ();

	DECLARE_MESSAGE_MAP()
};
