#pragma once


// CTimelineDlg dialog

class CTimePicture;
class W3GReplay;
#include "locmanager.h"

class CTimelineDlg : public CDialog
{
	DECLARE_DYNAMIC(CTimelineDlg)

  W3GReplay* w3g;
  unsigned long time;
  DWORD lastTime;
  int prevPos;
  CLocManager loc;
  CRect offs;

public:
	CTimelineDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTimelineDlg();

// Dialog Data
	enum { IDD = IDD_TIMELINE };
  void setReplay (W3GReplay* replay);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CSliderCtrl slider;
  int speed;
  CTimePicture* pic;


  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  virtual void OnOK () {}
  virtual void OnCancel () {}

  afx_msg void OnShowWindow (BOOL bShow, UINT nStatus);

  afx_msg void OnTimer (UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedPlay();
  afx_msg void OnSize (UINT nType, int cx, int cy);
};
