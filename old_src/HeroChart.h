#pragma once

#include "colorlist.h"
#include "locmanager.h"
class CTavernList;
class CDotAReplayDlg;

// CHeroChart dialog

class CHeroChart : public CDialog
{
	DECLARE_DYNAMIC(CHeroChart)
  CDotAReplayDlg* dlg;
  wchar_t player[256];
  CLocManager loc;
  CColManager hcol;
  CColManager rcol;

public:
	CHeroChart(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHeroChart();

  void refresh ();
  void onChangeTavern (int sub = 0);

  void selHero (int hero);

// Dialog Data
	enum { IDD = IDD_HEROCHART };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CListCtrl reps;
  CColorList heroes;
  CTavernList* taverns;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnLvnItemchangedHerolist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedUpdate();
  void onChangeHero ();
  afx_msg void OnLvnItemActivateFoundlist(NMHDR *pNMHDR, LRESULT *pResult);
  virtual void OnOK () {}
  virtual void OnCancel () {}
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnHdnTrackHerolist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnTrackFoundlist(NMHDR *pNMHDR, LRESULT *pResult);

  void setChartHero (int hero);
};
