#pragma once

#include "colorlist.h"
#include "replay.h"

// CPlayerStatsDlg dialog

class CDotAReplayDlg;

#define L_JUNGLE    0
#define L_AFK       4
#define L_SOLO      1
#define L_DUAL      2
#define L_TRIPLE    3
#define R_WIN       0
#define R_LOSE      1
#define R_UNKNOWN   2

struct StatsItem
{
  int curpos;
  int hero;
  char path[512];
  int kills, deaths;
  int creeps, denies;
  int lane;
  int gold, agold;
  int lvl, alvl;
  int score[2];
  int res;
  unsigned long left, time;
  int apm;
  wchar_t name[256];
  __int64 date;
};

class CPlayerStatsDlg : public CDialog
{
	DECLARE_DYNAMIC(CPlayerStatsDlg)

  CDotAReplayDlg* dlg;

public:
	CPlayerStatsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerStatsDlg();

  void setPlayer (wchar_t const* name);

// Dialog Data
	enum { IDD = IDD_PLAYERSTATS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CColorList games;
  Array<StatsItem> items;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  afx_msg void OnSize (UINT nType, int cx, int cy);
  void refill ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnLvnItemActivateStats(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnItemclickStats(NMHDR *pNMHDR, LRESULT *pResult);
  virtual void OnOK () {}
  virtual void OnCancel () {}
};
