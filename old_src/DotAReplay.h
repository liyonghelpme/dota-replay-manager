// DotAReplay.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "rmpq.h"


// CDotAReplayApp:
// See DotAReplay.cpp for the implementation of this class
//

class CProgressDlg;

class CDotAReplayApp : public CWinApp
{
public:
	CDotAReplayApp();

  MPQARCHIVE res;
  CProgressDlg* progress;

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CDotAReplayApp theApp;

// CProgressDlg dialog

class CProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(CProgressDlg)
  CFont font;
  int percent;
  char subtext[512];
  int showCount;

public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDlg();

// Dialog Data
	enum { IDD = IDD_PROGRESS };

  void SetText (char const* text, int perc = -1);
  void SetSupText (char const* text, int perc = -1);
  void SetSubText (char const* text, int perc = -1);
  void SetProgress (int perc);
  void show ();
  void hide ();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  afx_msg void OnPaint ();
  virtual BOOL OnInitDialog ();
  virtual void OnOK () {}
  virtual void OnCancel () {}

	DECLARE_MESSAGE_MAP()
};
