#pragma once


// CPlayerGoldDlg dialog

class CGoldGraph;
class CColorRect;
class CImageBox;
class W3GReplay;
#include "locmanager.h"

class CPlayerGoldDlg : public CDialog
{
	DECLARE_DYNAMIC(CPlayerGoldDlg)
  W3GReplay* w3g;
  CLocManager loc;

public:
	CPlayerGoldDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerGoldDlg();

// Dialog Data
	enum { IDD = IDD_PLAYERGOLD };
  void setReplay (W3GReplay* replay);

protected:
  CGoldGraph* graph;
  CImageBox* imgBars[12];
  CColorRect* clrBars[12];
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
  afx_msg void OnBnClickedDetailS1();
  afx_msg void OnBnClickedDetailS2();
  virtual void OnOK () {}
  virtual void OnCancel () {}
};
