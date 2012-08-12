#pragma once

// CBatchDlg dialog

#include "replay.h"

class CBatchDlg;

class FileThread
{
  static DWORD WINAPI threadProc (LPVOID param);
  bool end;
  HANDLE thread;
  CBatchDlg* dlg;
public:

  void Start (CBatchDlg* theDialog);
  void Stop ();
  void Close ();
};

class CSettingsDlg;

#define BATCH_PROCESS     1
#define BATCH_COPY        2
#define BATCH_CACHE       3

class CBatchDlg : public CDialog
{
	DECLARE_DYNAMIC(CBatchDlg)
  Array<CString>* files;
  FileThread thread;
  friend class FileThread;
  char const* fmt;
  char const* base;
  CSettingsDlg* settings;
  int mode;

public:
	CBatchDlg(Array<CString>* files, char const* fmt, char const* base, CSettingsDlg* set,
    int md, CWnd* pParent = NULL);   // standard constructor
	virtual ~CBatchDlg();

// Dialog Data
	enum { IDD = IDD_BATCHCOPY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();
  CProgressCtrl pbar;

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedCancel();
};
