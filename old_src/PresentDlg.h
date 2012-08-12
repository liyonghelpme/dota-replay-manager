#pragma once


// CPresentDlg dialog
class W3GReplay;
#include "locmanager.h"

class CPresentDlg : public CDialog
{
	DECLARE_DYNAMIC(CPresentDlg)

  W3GReplay* w3g;

  CLocManager loc;

public:
	CPresentDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPresentDlg();

// Dialog Data
	enum { IDD = IDD_PRESENT };

  void setReplay (W3GReplay* replay);

  void format (CString& str);
  void formatHTML (CString& str);
  void formatPlayer (CString& str, int id, int flags, int flagsEx);
  void formatDetails (CString& str, int id, int flags, int flagsEx);

protected:
  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  void setReplayInfo (unsigned long flags, unsigned long flagsEx);
  unsigned long getReplayInfo ();
  unsigned long getReplayInfoEx ();
  void OnChange ();
  void updateEnables (bool big = false);

  CComboBox preset;
  CComboBox mode;
  CComboBox func;
  CRichEditCtrl result;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual void OnOK () {}
  virtual void OnCancel () {}

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnCbnSelchangeGenMode();
  afx_msg void OnCbnSelchangePreset();
  afx_msg void OnCbnEditchangePreset();
  afx_msg void OnBnClickedSavepr();
  afx_msg void OnBnClickedDelpr();
  afx_msg void OnBnClickedUpdate();
  afx_msg void OnBnClickedCopy();
  afx_msg void OnBnClickedPreview();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnCbnSelchangeFunction();
};
