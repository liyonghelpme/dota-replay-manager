#pragma once

class W3GReplay;

#include "colorlist.h"
#include "locmanager.h"

// CPlayerInfoDlg dialog
class CDotAReplayDlg;

class CPlayerInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CPlayerInfoDlg)

  W3GReplay* w3g;
  CDotAReplayDlg* dlg;
  bool fake;
  CLocManager loc;
  CColManager scol;
  CColManager icol;

public:
	CPlayerInfoDlg(CDotAReplayDlg* d, CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerInfoDlg();

// Dialog Data
	enum { IDD = IDD_PLAYERINFO };
  
  void setReplay (W3GReplay* w3g);

  Array<unsigned long> skills;
  Array<unsigned long> items;

protected:
  CColorList itemInfo;
  CColorList skillInfo;
  CColorBox selPlayer;
  CImageBox* kicons[5];

  int curPlayer;
  bool lockSelChange;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();
  afx_msg void OnPaint ();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  virtual void OnOK () {}
  virtual void OnCancel () {}

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnCbnSelchangeSelplayer();

  void selectPlayer (int id);
  afx_msg void OnLvnItemchangedSkilllist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnItemchangedItemlist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnHdnTrackSkilllist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnTrackItemlist(NMHDR *pNMHDR, LRESULT *pResult);
};
