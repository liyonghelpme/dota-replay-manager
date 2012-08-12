#pragma once

#include "colorlist.h"
#include "locmanager.h"

// CTagEditorDlg dialog

class CTagEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CTagEditorDlg)
  CLocManager loc;
  CColorList tags;

public:
	CTagEditorDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTagEditorDlg();

// Dialog Data
	enum { IDD = IDD_TAGEDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  afx_msg void OnSize (UINT nType, int cx, int cy);
  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

	DECLARE_MESSAGE_MAP()
};
