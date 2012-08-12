#pragma once


// CPlayerExpDlg dialog

class CExpGraph;
class CColorRect;
class CImageBox;
class W3GReplay;
#include "locmanager.h"

class CPlayerExpDlg : public CDialog
{
	DECLARE_DYNAMIC(CPlayerExpDlg)
  W3GReplay* w3g;
  CLocManager loc;

public:
	CPlayerExpDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerExpDlg();

// Dialog Data
	enum { IDD = IDD_PLAYEREXP };
  void setReplay (W3GReplay* replay);

protected:
  CExpGraph* graph;
  CImageBox* imgBars[10];
  CColorRect* clrBars[10];
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual BOOL OnInitDialog ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedDetailP1();
  afx_msg void OnBnClickedDetailP2();
  afx_msg void OnBnClickedDetailP3();
  afx_msg void OnBnClickedDetailP4();
  afx_msg void OnBnClickedDetailP5();
  afx_msg void OnBnClickedDetailP6();
  afx_msg void OnBnClickedDetailP7();
  afx_msg void OnBnClickedDetailP8();
  afx_msg void OnBnClickedDetailP9();
  afx_msg void OnBnClickedDetailP10();
  afx_msg void OnBnClickedAllsent();
  afx_msg void OnBnClickedNosent();
  afx_msg void OnBnClickedAllscourge();
  afx_msg void OnBnClickedNoscourge();
  afx_msg void OnSize (UINT nType, int cx, int cy);
  virtual void OnOK () {}
  virtual void OnCancel () {}
};
