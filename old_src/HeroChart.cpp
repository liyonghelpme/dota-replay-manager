// HeroChart.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "HeroChart.h"
#include "replay.h"
#include "ilib.h"
#include "gamecache.h"
#include "dotareplaydlg.h"
#include ".\herochart.h"

int heroCount[256];

// CTavernList

class CTavernList : public CWnd
{
	DECLARE_DYNAMIC(CTavernList)

  W3GReplay* w3g;
  CHeroChart* chart;

  int maxHeight;

public:
	CTavernList(CRect const& rc, CWnd* parent);
	virtual ~CTavernList();

  void setSel (int sel);
  int curSel;

protected:
  afx_msg void OnPaint ();
  afx_msg void OnLButtonDown (UINT nFlags, CPoint point);
  afx_msg void OnVScroll (UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnSize (UINT nType, int cx, int cy);
  afx_msg BOOL OnMouseWheel (UINT nFlags, short zDelta, CPoint pt);
	DECLARE_MESSAGE_MAP()
};
IMPLEMENT_DYNAMIC(CTavernList, CWnd)

CTavernList::CTavernList (CRect const& rc, CWnd* parent)
{
  chart = (CHeroChart*) parent;
  curSel = 0;
  maxHeight = 1432;
  CreateEx (WS_EX_CLIENTEDGE, NULL, "", WS_CHILD, rc, parent, IDC_TIMEPIC);
  ShowWindow (SW_SHOW);
}
CTavernList::~CTavernList()
{
}
BEGIN_MESSAGE_MAP(CTavernList, CWnd)
  ON_WM_PAINT()
  ON_WM_LBUTTONDOWN()
  ON_WM_VSCROLL()
  ON_WM_SIZE()
  ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CTavernList::OnVScroll (UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  SCROLLINFO si;
  GetScrollInfo (SB_VERT, &si);
  int prev = si.nPos;
  switch (nSBCode)
  {
  case SB_TOP:
    si.nPos = si.nMin;
    break;
  case SB_BOTTOM:
    si.nPos = si.nMax;
    break;
  case SB_LINEUP:
    si.nPos -= 5;
    break;
  case SB_LINEDOWN:
    si.nPos += 5;
    break;
  case SB_PAGEUP:
    si.nPos -= si.nPage;
    break;
  case SB_PAGEDOWN:
    si.nPos += si.nPage;
    break;
  case SB_THUMBTRACK:
    si.nPos = si.nTrackPos;
    break;
  }
  si.fMask = SIF_POS;
  SetScrollInfo (SB_VERT, &si);
  GetScrollInfo (SB_VERT, &si);
  if (si.nPos != prev)
  {                    
    ScrollWindow (0, prev - si.nPos, NULL, NULL);
    UpdateWindow ();
  }
}
BOOL CTavernList::OnMouseWheel (UINT nFlags, short zDelta, CPoint pt)
{
  CRect rc;
  GetClientRect (rc);
  static short accum = 0;
  accum += zDelta;
  if (rc.bottom < maxHeight)
  {
    int prev = GetScrollPos (SB_VERT);
    int cur = prev;
    while (accum <= -WHEEL_DELTA)
    {
      cur += 30;
      accum += WHEEL_DELTA;
    }
    while (accum >= WHEEL_DELTA)
    {
      cur -= 30;
      accum -= WHEEL_DELTA;
    }
    SetScrollPos (SB_VERT, cur);
    cur = GetScrollPos (SB_VERT);
    if (cur != prev)
    {
      ScrollWindow (0, prev - cur, NULL, NULL);
      UpdateWindow ();
    }
  }
  return TRUE;
}
void CTavernList::OnSize (UINT nType, int cx, int cy)
{
  if (cy > 100)
  {
    if (cy >= maxHeight)
    {
      SetScrollPos (SB_VERT, 0);
      ShowScrollBar (SB_VERT, FALSE);
      UpdateWindow ();
    }
    else
    {
      ShowScrollBar (SB_VERT, TRUE);
      SCROLLINFO si;
      si.cbSize = sizeof si;
      si.fMask = SIF_ALL;
      si.nMin = 0;
      si.nMax = maxHeight;
      si.nPage = cy;
      si.nPos = GetScrollPos (SB_VERT);
      si.nTrackPos = 0;
      SetScrollInfo (SB_VERT, &si);
    }
  }
}

#define NUM_TAVERNS     12
int tavCount[NUM_TAVERNS];
DotaHero* tavHero[NUM_TAVERNS][16];
int tavX[NUM_TAVERNS] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
int tavY[NUM_TAVERNS] = {60, 176, 292, 398, 514, 620, 766, 872, 988, 1094, 1210, 1316};
void CTavernList::OnLButtonDown (UINT nFlags, CPoint point)
{
  int ty[NUM_TAVERNS];
  memcpy (ty, tavY, sizeof ty);
  int scroll = GetScrollPos (SB_VERT);
  for (int i = 0; i < NUM_TAVERNS; i++)
    ty[i] -= scroll;
  int sel = -1;
  int sub = 0;
  for (int i = 0; i < NUM_TAVERNS && sel < 0; i++)
    if (point.x > tavX[i] && point.y > ty[i] && point.x < tavX[i] + 128 && point.y < ty[i] + 96)
    {
      sel = i;
      int sx = (point.x - tavX[i]) / 32;
      int sy = (point.y - ty[i]) / 32;
      sub = sx + sy * 4;
    }
  if (sel >= 0 && sel != curSel)
  {
    curSel = sel;
    InvalidateRect (NULL, FALSE);
  }
  chart->onChangeTavern (sub);
}
void CTavernList::setSel (int sel)
{
  curSel = sel;
  InvalidateRect (NULL, FALSE);
  chart->onChangeTavern ();
}
void CTavernList::OnPaint ()
{
  CPaintDC dc (this);
  CBrush* brs = CBrush::FromHandle ((HBRUSH) GetStockObject (BLACK_BRUSH));
  CBrush selbr (RGB (255, 204, 0));
  CRect rc;
  GetClientRect (rc);
  dc.FillRect (rc, brs);

  int ty[NUM_TAVERNS];
  memcpy (ty, tavY, sizeof ty);
  int scroll = GetScrollPos (SB_VERT);
  for (int i = 0; i < NUM_TAVERNS; i++)
    ty[i] -= scroll;
  CFont big;
  big.CreateFont (-MulDiv (14, dc.GetDeviceCaps (LOGPIXELSY), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Georgia");
  CFont med;
  med.CreateFont (-MulDiv (12, dc.GetDeviceCaps (LOGPIXELSY), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Georgia");
  CFont sml;
  sml.CreateFont (-MulDiv (8, dc.GetDeviceCaps (LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Arial");
  CDC bmpDc;
  bmpDc.CreateCompatibleDC (&dc);
  for (int i = 0; i < NUM_TAVERNS; i++)
  {
    if (i == curSel)
    {
      dc.FillRect (CRect (tavX[i] - 3, ty[i] - 3, tavX[i] + 131, ty[i] + 99), &selbr);
      dc.FillRect (CRect (tavX[i], ty[i], tavX[i] + 128, ty[i] + 96), brs);
    }
    else
      dc.FillRect (CRect (tavX[i] - 3, ty[i] - 3, tavX[i] + 131, ty[i] + 99), brs);
  }
  dc.SetBkMode (TRANSPARENT);
  dc.SetTextColor (getSlotColor (0));
  dc.SelectObject (&big);
  dc.DrawText ("Sentinel", -1, CRect (tavX[0], ty[0] - 55, tavX[0] + 128, ty[0] - 30), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SelectObject (&med);
  dc.DrawText ("Agility", -1, CRect (tavX[0], ty[0] - 25, tavX[0] + 128, ty[0]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.DrawText ("Strength", -1, CRect (tavX[2], ty[2] - 25, tavX[2] + 128, ty[2]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.DrawText ("Intelligence", -1, CRect (tavX[4], ty[4] - 25, tavX[4] + 128, ty[4]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor (getSlotColor (6));
  dc.SelectObject (&big);
  dc.DrawText ("Scourge", -1, CRect (tavX[6], ty[6] - 55, tavX[6] + 128, ty[6] - 30), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SelectObject (&med);
  dc.DrawText ("Agility", -1, CRect (tavX[6], ty[6] - 25, tavX[6] + 128, ty[6]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.DrawText ("Strength", -1, CRect (tavX[8], ty[8] - 25, tavX[8] + 128, ty[8]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.DrawText ("Intelligence", -1, CRect (tavX[10], ty[10] - 25, tavX[10] + 128, ty[10]), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SelectObject (&sml);
  dc.SetTextColor (RGB (255, 255, 255));
  dc.SetBkMode (OPAQUE);
  dc.SetBkColor (RGB (0, 0, 0));
  for (int i = 0; i < NUM_TAVERNS; i++)
  {
    for (int j = 0; j < tavCount[i]; j++)
    {
      DotaHero* hero = tavHero[i][j];
      int cx = (j % 4) * 32 + tavX[i];
      int cy = (j / 4) * 32 + ty[i];
      CBitmap* bmp = getImageBitmap (mprintf ("big%s", hero->imgTag));
      bmpDc.SelectObject (bmp);
      dc.BitBlt (cx, cy, 32, 32, &bmpDc, 0, 0, SRCCOPY);
      dc.DrawText (mprintf (" %d ", heroCount[hero->index]), -1, CRect (cx, cy, cx + 32, cy + 32),
        DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
    }
  }
}

// CHeroChart dialog

IMPLEMENT_DYNAMIC(CHeroChart, CDialog)
CHeroChart::CHeroChart(CWnd* pParent /*=NULL*/)
	: CDialog(CHeroChart::IDD, pParent)
{
  dlg = (CDotAReplayDlg*) pParent;
  taverns = NULL;
  Create (IDD, pParent);
}

CHeroChart::~CHeroChart()
{
  delete taverns;
}

void CHeroChart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CHeroChart, CDialog)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEROLIST, OnLvnItemchangedHerolist)
  ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_FOUNDLIST, OnLvnItemActivateFoundlist)
  ON_WM_SIZE()
  ON_WM_DESTROY()
  ON_NOTIFY(HDN_TRACK, 0, OnHdnTrackHerolist)
  ON_NOTIFY(HDN_TRACK, 0, OnHdnTrackFoundlist)
END_MESSAGE_MAP()


// CHeroChart message handlers

void CHeroChart::refresh ()
{
  taverns->setSel (0);
}
void CHeroChart::onChangeTavern (int sub)
{
  heroes.DeleteAllItems ();
  if (taverns->curSel >= 0 && taverns->curSel < NUM_TAVERNS)
  {
    for (int i = 0; i < tavCount[taverns->curSel]; i++)
    {
      DotaHero* hero = tavHero[taverns->curSel][i];
      int pos = heroes.InsertItem (heroes.GetItemCount (), hero->name, getImageIndex (hero->imgTag));
      heroes.SetItemText (pos, 1, mprintf ("%d", heroCount[hero->index]));
      heroes.SetItemData (pos, hero->point);
    }
  }
  heroes.SetItemState (sub, LVIS_SELECTED, LVIS_SELECTED);
  onChangeHero ();
  taverns->SetFocus ();
}
void CHeroChart::selHero (int hero)
{
  DotaHero* h = getHeroByPoint (hero);
  if (h)
  {
    taverns->setSel (h->tavern);
    for (int i = 0; i < heroes.GetItemCount (); i++)
    {
      if (heroes.GetItemData (i) == hero)
      {
        heroes.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
        onChangeHero ();
        break;
      }
    }
  }
}

extern wchar_t ownNames[256];

BOOL CHeroChart::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  loadDefault ();
  memset (tavCount, 0, sizeof tavCount);
  for (int i = 1; i < getNumHeroes (); i++)
  {
    DotaHero* hero = getHero (i);
    if (hero->tavern >= 0 && hero->tavern < NUM_TAVERNS)
      tavHero[hero->tavern][tavCount[hero->tavern]++] = hero;
  }
  for (int i = 0; i < NUM_TAVERNS; i++)
  {
    for (int j = 0; j < tavCount[i]; j++)
    {
      for (int k = 0; k < tavCount[i] - 1; k++)
      {
        if (tavHero[i][k]->slot > tavHero[i][k + 1]->slot)
        {
          DotaHero* temp = tavHero[i][k];
          tavHero[i][k] = tavHero[i][k + 1];
          tavHero[i][k + 1] = temp;
        }
      }
    }
  }

  reps.Attach (GetDlgItem (IDC_FOUNDLIST)->m_hWnd);
  heroes.Attach (GetDlgItem (IDC_HEROLIST)->m_hWnd);
  reps.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

  heroes.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  heroes.InsertColumn (0, "Hero");
  heroes.InsertColumn (1, "Games");
  CRect rc;
  heroes.GetClientRect (rc);
  heroes.SetColumnWidth (0, rc.right - 60);
  heroes.SetColumnWidth (1, 60);
  heroes.SetImageList (getImageList (), LVSIL_SMALL);
  heroes.simple = true;

  reps.InsertColumn (0, "Game");
  reps.InsertColumn (1, "Score");
  reps.InsertColumn (2, "Level");
  reps.InsertColumn (3, "Gold");

  GetDlgItem (IDC_TAVERNAREA)->GetWindowRect (rc);
  ScreenToClient (rc);
  taverns = new CTavernList (rc, this);
  wcscpy (player, ownNames);
  int pos = 0;
  while (player[pos] && player[pos] != ',' && player[pos] != ';' && player[pos] != ' ')
    pos++;
  player[pos] = 0;
  SetDlgItemTextW (m_hWnd, IDC_PLAYERNAME, player);
  OnBnClickedUpdate ();

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_PLAYERNAME, SIDE_RIGHT);
  loc.SetItemRelative (IDC_UPDATE, SIDE_LEFT, IDC_UPDATE, SIDE_RIGHT);
  loc.SetItemAbsolute (taverns, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_HEROLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_HEROLIST, SIDE_BOTTOM, PERCENT);
  loc.SetItemRelative (IDC_FOUNDLIST, SIDE_TOP, IDC_HEROLIST, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_FOUNDLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FOUNDLIST, SIDE_BOTTOM);
  loc.Start ();
  hcol.SetListCtrl (&heroes);
  hcol.Read ();
  rcol.SetListCtrl (&reps);
  rcol.Read ();

  return TRUE;
}

void CHeroChart::OnDestroy ()
{
  reps.Detach ();
  heroes.Detach ();
}

Array<CString> rpath;

void CHeroChart::onChangeHero ()
{
  int sel = heroes.GetNextItem (-1, LVNI_SELECTED);
  reps.DeleteAllItems ();
  rpath.clear ();
  if (sel < 0)
    reps.EnableWindow (FALSE);
  else
  {
    reps.EnableWindow (TRUE);
    //CDotAReplayDlg::instance->setChartHero (heroes.GetItemData (sel));
    for (int i = 0; i < gcsize; i++)
    {
      GameCache* gc = &gcache[i];
      for (int j = 0; j < gc->count; j++)
      {
        if ((player[0] == 0 || !wcsicmp (player, gc->pname[j])) && gc->phero[j] == (int) heroes.GetItemData (sel))
        {
          int pos = LVInsertItem (reps, reps.GetItemCount (), gc->name);
          reps.SetItemData (pos, rpath.add (gc->path));
          if (gc->pstats[j][0] >= 0)
            reps.SetItemText (pos, 1, mprintf ("%d/%d", gc->pstats[j][0], gc->pstats[j][1]));
          reps.SetItemText (pos, 2, mprintf ("%d", gc->plvl[j]));
          reps.SetItemText (pos, 3, mprintf ("%d", gc->pgold[j]));
        }
      }
    }
    for (int i = 0; i < reps.GetHeaderCtrl ()->GetItemCount (); i++)
      reps.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
    rcol.Read ();
  }
}

void CHeroChart::OnLvnItemchangedHerolist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  onChangeHero ();
  *pResult = 0;
}

void CHeroChart::OnBnClickedUpdate()
{
  static char mode[512];
  GetDlgItemTextW (m_hWnd, IDC_PLAYERNAME, player, sizeof player);
  GetDlgItemText (IDC_GAMEMODE, mode, sizeof mode);
  memset (heroCount, 0, sizeof heroCount);
  for (int i = 0; i < gcsize; i++)
  {
    GameCache* gc = &gcache[i];
    for (int j = 0; j < gc->count; j++)
    {
      if (stristr (gc->mode, mode) &&
        (player[0] == 0 || !wcsicmp (player, gc->pname[j])) && gc->phero[j] > 0)
      {
        DotaHero* hero = getHeroByPoint (gc->phero[j]);
        if (hero)
          heroCount[hero->index]++;
      }
    }
  }
  taverns->setSel (0);
}

void CHeroChart::OnLvnItemActivateFoundlist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int sel = reps.GetNextItem (-1, LVNI_SELECTED);
  if (sel >= 0)
  {
    sel = (int) reps.GetItemData (sel);
    if (sel < rpath.getSize ())
      dlg->showReplay (rpath[sel]);
  }
  *pResult = 0;
}

void CHeroChart::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
  rcol.Write ();
  hcol.Write ();
}

void CHeroChart::OnHdnTrackHerolist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  hcol.Read ();
  *pResult = 0;
}

void CHeroChart::OnHdnTrackFoundlist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  rcol.Read ();
  *pResult = 0;
}
