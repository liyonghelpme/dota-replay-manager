#pragma once

#include "replay.h"
#include "locmanager.h"

// CNewPresentDlg dialog

class CNewPresentDlg : public CDialog
{
	DECLARE_DYNAMIC(CNewPresentDlg)
  W3GReplay* w3g;
  CLocManager loc;
  CComboBox presets;
  void loadFormat (MPQFILE file);

public:
	CNewPresentDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNewPresentDlg();

// Dialog Data
	enum { IDD = IDD_PRESENT2 };

  void setReplay (W3GReplay* replay);
  afx_msg void OnSize (UINT nType, int cx, int cy);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();
  virtual void OnOK () {}
  virtual void OnCancel () {}

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedUpdate();
  afx_msg void OnBnClickedCopy();
  afx_msg void OnCbnSelchangePreset();
  afx_msg void OnBnClickedNewpr();
  afx_msg void OnBnClickedSavepr();
  afx_msg void OnBnClickedDelpr();
};
