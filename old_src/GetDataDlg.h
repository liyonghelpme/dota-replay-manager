#pragma once


// CGetDataDlg dialog

class CGetDataDlg : public CDialog
{
	DECLARE_DYNAMIC(CGetDataDlg)

  bool fix;
  char const* pth;

public:
	CGetDataDlg (bool fixed, int version, char const* path, CWnd* pParent = NULL);   // standard constructor
	virtual ~CGetDataDlg();

// Dialog Data
	enum { IDD = IDD_GETDATA };

  enum {Cancel, Closest, Version, Used, Path};
  int ver;
  int mode;
  int copyfrom;
  char path[256];
  bool saved;
  bool alwaysdo;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedMapbrowse();
  afx_msg void OnBnClickedClosest();
  afx_msg void OnBnClickedCopyversion();
  afx_msg void OnBnClickedLoadmap();
  afx_msg void OnBnClickedLoadthismap();
  afx_msg void OnEnChangeVersion();
  afx_msg void OnBnClickedOk();
};
