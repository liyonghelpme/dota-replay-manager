#pragma once

class W3GReplay;

#include "colorlist.h"
#include "locmanager.h"
#include "mapdata.h"

// CActionLogDlg dialog

struct SmallPlayer
{
  char name[256];
  unsigned char color;
};
struct SmallReplay;
class CActionLog : public CListCtrl
{
public:
  CActionLog ()
  {
    rawcodes = false;
    w3g = NULL;
  }
  virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
  bool rawcodes;
  SmallPlayer players[256];
  MapData* map;
  W3GReplay* w3g;
};

class CActionLogDlg : public CDialog
{
	DECLARE_DYNAMIC(CActionLogDlg)
  W3GReplay* w3g;
  CLocManager loc;
  MapData* map;

public:
	CActionLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CActionLogDlg();

// Dialog Data
	enum { IDD = IDD_ACTIONLOG };

  void setReplay (W3GReplay* w3g);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CActionLog actions;
  CMenu popupMenu;
  CColorBox searchPlayer;

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  virtual void OnOK () {}
  virtual void OnCancel () {}

  char const* formatOrder (int id);

  void parseActions (int length, SmallReplay& rep);
  bool loadReplay (FILE* file);
  void addMessage (unsigned long time, int player, char const* fmt, ...);

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedLoadactions();
  afx_msg void OnBnClickedRawcodes();
  afx_msg void OnNMRclickActions(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg void OnBnClickedFindnext();
};
