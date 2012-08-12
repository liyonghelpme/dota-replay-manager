#pragma once

extern unsigned int lastVersion;
extern const unsigned int curVersion;
void updateVersion ();

// CUpdateDlg dialog

class CUpdateDlg : public CDialog
{
	DECLARE_DYNAMIC(CUpdateDlg)

  bool autoCheck;
  bool end;
  HANDLE thread;

  friend DWORD WINAPI LoadUpdate (LPVOID param);

public:
	CUpdateDlg(bool forced = false, CWnd* pParent = NULL);   // standard constructor
	virtual ~CUpdateDlg();

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

// Dialog Data
	enum { IDD = IDD_UPDATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedUpdate();
  afx_msg void OnBnClickedForum();
  afx_msg void OnBnClickedForum2();
};
