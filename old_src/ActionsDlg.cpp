// ActionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "ActionsDlg.h"
#include "DotAReplayDlg.h"

#include "ilib.h"
#include "colorlist.h"
#include "replay.h"

// CActionsDlg dialog

IMPLEMENT_DYNAMIC(CActionsDlg, CDialog)
CActionsDlg::CActionsDlg(CDotAReplayDlg* d, CWnd* pParent /*=NULL*/)
	: CDialog(CActionsDlg::IDD, pParent)
{
  history = NULL;
  dlg = d;
  w3g = NULL;
  fake = false;
  Create (IDD, pParent);
}

CActionsDlg::~CActionsDlg()
{
  delete history;
}

void CActionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CActionsDlg, CDialog)
  ON_CBN_SELCHANGE(IDC_SELPLAYER, OnCbnSelchangeSelplayer)
  ON_WM_DESTROY()
  ON_WM_SIZE()
END_MESSAGE_MAP()

// CActionHistory

IMPLEMENT_DYNAMIC(CActionHistory, CWnd)

void CActionHistory::OnSize (UINT nType, int cx, int cy)
{
  GetClientRect (rc);
  setPlayer (pid);
}

CActionHistory::CActionHistory(CRect const& rc, CWnd* parent)
{
//  gl = NULL;
  w3g = NULL;
  CreateEx (WS_EX_CLIENTEDGE, NULL, "", WS_CHILD, rc, parent, IDC_TIMEPIC);
  ShowWindow (SW_SHOW);
}

#define BUCKET      2

CActionHistory::~CActionHistory()
{
}
void CActionHistory::setReplay (W3GReplay* replay)
{
  w3g = replay;
  pid = -1;
  InvalidateRect (NULL, FALSE);
}
static Array<int> agraph;
static int agraphmx;
void CActionHistory::setPlayer (int id)
{
  pid = id;
  agraph.clear ();
  agraphmx = 0;
  if (w3g && pid >= 0 && rc.right > 100)
  {
    int id = w3g->players[pid].index;
    if (w3g->pindex[id] == pid)
    {
      int curb = 0;
      unsigned long btime = BUCKET * w3g->players[pid].time / (rc.right - 30);
      agraph.add (0);
      for (int i = 0; i < w3g->pactions[id].getSize (); i++)
      {
        while (w3g->pactions[id][i].time > (__int64 (curb * BUCKET + BUCKET) * __int64 (w3g->players[pid].time) /
          __int64 (rc.right - 30)))
        {
          agraph[curb] = agraph[curb] * 60000 / btime;
          if (agraph[curb] > agraphmx)
            agraphmx = agraph[curb];
          agraph.add (0);
          curb++;
        }
        agraph[curb]++;
      }
      agraph[curb] = agraph[curb] * 60000 / btime;
      if (agraph[curb] > agraphmx)
        agraphmx = agraph[curb];
    }
  }
  InvalidateRect (NULL, FALSE);
}

BEGIN_MESSAGE_MAP(CActionHistory, CWnd)
  ON_WM_CREATE()
  ON_WM_PAINT()
  ON_WM_DESTROY()
  ON_WM_SIZE()
END_MESSAGE_MAP()

// CActionHistory message handlers

int CActionHistory::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate (lpCreateStruct) == -1)
    return -1;

  GetClientRect (rc);
//  gl = new OpenGL (m_hWnd, 0xFFFFFF);
//  if (!gl->isOk ())
//    MessageBox ("Error initializing OpenGL!", "Error", MB_OK | MB_ICONERROR);

  return 0;
}
void CActionHistory::OnPaint ()
{
  CPaintDC dc (this);
//  if (gl->isOk ())
//  {
//    gl->begin ();
  DCPaint dcp (&dc);
  CRect rc;
  GetClientRect (rc);
  ::FillRect (dc.m_hDC, &rc, (HBRUSH) GetStockObject (WHITE_BRUSH));
  dcp.setColor (0, 0, 0);
  if (w3g && pid >= 0 && agraphmx)
  {
    int id = w3g->players[pid].index;
    dcp.line (30, 0, 30, rc.bottom - 20);
    dcp.line (30, rc.bottom - 20, rc.right, rc.bottom - 20);
    for (int i = 0; i < agraph.getSize (); i++)
    {
      dcp.line (30 + i * BUCKET, rc.bottom - 20, 30 + i * BUCKET,
        rc.bottom - 20 - (rc.bottom - 30) * agraph[i] / agraphmx);
    }
    double sy = double (rc.bottom - 30) / double (agraphmx);
    double sx = double (rc.right - 30) / double (w3g->players[pid].time);

    EnumStruct es;
    enumCount (es);
    while (double (es.val) * sy < 15)
      nextCount (es);
    int cury = rc.bottom - 20;
    int stepy = int (double (es.val) * sy + 0.5);
    for (int i = 0; i <= agraphmx + es.val; i += es.val, cury -= stepy)
      dcp.drawText (28, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);

    enumTime (es);
    while (double (es.val) * sx < 40)
      nextTime (es);
    int curx = 30;
    int stepx = int (double (es.val) * sx + 0.5);
    int _start = w3g->game.startTime;
    for (unsigned long i = _start - (_start / es.val) * es.val; i < w3g->time; i += es.val)
    {
      int curx = 30 + int (double (i) * sx + 0.5);
      dcp.drawText (curx, rc.bottom - 15, format_time (w3g, i, TIME_HOURS | TIME_SECONDS), ALIGN_X_CENTER | ALIGN_Y_TOP);
    }
  }
//    gl->end ();
//  }
}
void CActionHistory::OnDestroy ()
{
//  delete gl;
//  gl = NULL;
}

void CActionsDlg::OnCbnSelchangeSelplayer()
{
  if (w3g)
  {
    int sel = selPlayer.GetCurSel ();
    int id = int (selPlayer.GetItemDataEx (sel));
    if (id == 0)
    {
      selPlayer.SetCurSel (curPlayer);
      return;
    }
    if (!fake)
      dlg->selPlayerInfo (id);
    curPlayer = sel;
    actions.setPlayer (id);
    history->setPlayer (id);
    actions.mx = 0;
    for (int i = 0; i < NUM_ACTIONS; i++)
    {
      actions.SetItemText (i, 1, mprintf ("%d", w3g->players[id].acounter[i]));
      if (w3g->players[id].acounter[i] > actions.mx)
        actions.mx = w3g->players[id].acounter[i];
    }
    hkeys.DeleteAllItems ();
    for (int i = 0; i < 10; i++)
    {
      if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
      {
        int cl = hkeys.InsertItem (hkeys.GetItemCount (), mprintf ("%d", i + 1));
        hkeys.SetItemText (cl, 1, mprintf ("%d", w3g->players[id].hkassign[i]));
        hkeys.SetItemText (cl, 2, mprintf ("%d", w3g->players[id].hkuse[i]));
      }
    }
    hkeys.SetColumnWidth (0, LVSCW_AUTOSIZE_USEHEADER);
    hkeys.SetColumnWidth (1, LVSCW_AUTOSIZE_USEHEADER);
    hkeys.SetColumnWidth (2, LVSCW_AUTOSIZE_USEHEADER);

    unsigned long prev = 0;
    unsigned long mx = 0, start = 0;
    int pind = 0;
    for (int i = 0; i < w3g->numPlayers; i++)
      if (w3g->pindex[i] == id)
        pind = i;
    for (int i = 0; i < w3g->pactions[pind].getSize (); i++)
    {
      unsigned long cur = w3g->pactions[pind][i].time;
      if (cur - prev > mx)
      {
        mx = cur - prev;
        start = prev;
      }
      prev = cur;
    }
    if (w3g->players[id].time - prev > mx)
    {
      mx = w3g->players[id].time - prev;
      start = prev;
    }
    SetDlgItemText (IDC_AFKTIME, mprintf ("Longest AFK period: %s (from %s to %s)",
      format_time (mx, TIME_SECONDS), format_time (w3g, start, TIME_HOURS | TIME_SECONDS),
      format_time (w3g, start + mx, TIME_HOURS | TIME_SECONDS)));
  }
}

BOOL CActionsDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  selPlayer.Attach (GetDlgItem (IDC_SELPLAYER)->m_hWnd);
  selPlayer.SetImageList (getImageList ());
  actions.Attach (GetDlgItem (IDC_ACTIONCHART)->m_hWnd);
  actions.InsertColumn (0, "");
  actions.InsertColumn (1, "");
  actions.SetColumnWidth (0, 150);
  actions.SetColumnWidth (1, LVSCW_AUTOSIZE_USEHEADER);
  for (int i = 0; i < NUM_ACTIONS; i++)
    actions.InsertItem (i, actionNames[i]);
  hkeys.Attach (GetDlgItem (IDC_GROUPS)->m_hWnd);
  hkeys.InsertColumn (0, "Group");
  hkeys.InsertColumn (1, "Assigned");
  hkeys.InsertColumn (2, "Used");
  hkeys.SetColumnWidth (0, LVSCW_AUTOSIZE_USEHEADER);
  hkeys.SetColumnWidth (1, LVSCW_AUTOSIZE_USEHEADER);
  hkeys.SetColumnWidth (2, LVSCW_AUTOSIZE_USEHEADER);
  hkeys.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

  CRect rc;
  GetDlgItem (IDC_GRAPHAREA)->GetWindowRect (rc);
  actions.ScreenToClient (rc);
  actions.start = rc.left;
  actions.end = rc.right;
  
  GetDlgItem (IDC_ACTIONGRAPH)->GetWindowRect (rc);
  ScreenToClient (rc);
  history = new CActionHistory (rc, this);
  
  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_SELPLAYER, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (IDC_SELPLAYER, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_ACTIONCHART, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (IDC_ACTIONCHART, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (&actions, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (&actions, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_GROUPS, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (IDC_GROUPS, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_GROUPS, SIDE_BOTTOM, PERCENT);
  loc.SetItemRelative (IDC_AFKTIME, SIDE_TOP, IDC_GROUPS, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_AFKTIME, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (IDC_AFKTIME, SIDE_RIGHT, PERCENT);
  loc.SetItemRelative (history, SIDE_TOP, GetDlgItem (IDC_AFKTIME), SIDE_BOTTOM);
  loc.SetItemAbsolute (history, SIDE_RIGHT);
  loc.SetItemAbsolute (history, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}

void CActionsDlg::OnDestroy ()
{
  selPlayer.Detach ();
  actions.Detach ();
  hkeys.Detach ();
}

void CActionsDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
  actions.setReplay (replay);
  history->setReplay (replay);
  selPlayer.Reset ();
  hkeys.DeleteAllItems ();
  for (int i = 0; i < NUM_ACTIONS; i++)
    actions.SetItemText (i, 1, "");
  if (w3g && w3g->dota.isDota)
  {
    selPlayer.EnableWindow (TRUE);
    if (w3g->dota.numSentinel)
      selPlayer.InsertItem ("Sentinel", getImageIndex ("RedBullet"), 0);
    for (int i = 0; i < w3g->dota.numSentinel; i++)
    {
      if (w3g->players[w3g->dota.sentinel[i]].hero)
        selPlayer.InsertItem (wmprintf (L"%s (%s)", w3g->players[w3g->dota.sentinel[i]].uname,
          makeucd (getHero (w3g->players[w3g->dota.sentinel[i]].hero->id)->name)),
          getImageIndex (getHero (w3g->players[w3g->dota.sentinel[i]].hero->id)->imgTag),
          getLightColor (w3g->players[w3g->dota.sentinel[i]].slot.color), w3g->dota.sentinel[i]);
      else
        selPlayer.InsertItem (wmprintf (L"%s (No Hero)", w3g->players[w3g->dota.sentinel[i]].uname),
          getImageIndex ("Empty"),
          getLightColor (w3g->players[w3g->dota.sentinel[i]].slot.color), w3g->dota.sentinel[i]);
    }
    if (w3g->dota.numScourge)
      selPlayer.InsertItem ("Scourge", getImageIndex ("GreenBullet"), 0);
    for (int i = 0; i < w3g->dota.numScourge; i++)
    {
      if (w3g->players[w3g->dota.scourge[i]].hero)
        selPlayer.InsertItem (wmprintf (L"%s (%s)", w3g->players[w3g->dota.scourge[i]].uname,
          makeucd (getHero (w3g->players[w3g->dota.scourge[i]].hero->id)->name)),
          getImageIndex (getHero (w3g->players[w3g->dota.scourge[i]].hero->id)->imgTag),
          getLightColor (w3g->players[w3g->dota.scourge[i]].slot.color), w3g->dota.scourge[i]);
      else
        selPlayer.InsertItem (wmprintf (L"%s (No Hero)", w3g->players[w3g->dota.scourge[i]].uname),
          getImageIndex ("Empty"),
          getLightColor (w3g->players[w3g->dota.scourge[i]].slot.color), w3g->dota.scourge[i]);
    }
    selPlayer.SetCurSel (1);
    OnCbnSelchangeSelplayer ();
  }
  else if (w3g)
  {
    selPlayer.EnableWindow (TRUE);
    for (int i = 0; i < w3g->numPlayers; i++)
      selPlayer.InsertItem (w3g->players[w3g->pindex[i]].uname,
        getImageIndex (raceImage[w3g->players[w3g->pindex[i]].race]),
        getLightColor (w3g->players[w3g->pindex[i]].slot.color), w3g->pindex[i]);
    selPlayer.SetCurSel (0);
    OnCbnSelchangeSelplayer ();
  }
  else
    selPlayer.EnableWindow (FALSE);
}

void CActionsDlg::selectPlayer (int id)
{
  if (w3g)
  {
    for (int i = 0; i < selPlayer.GetCount (); i++)
    {
      if (int (selPlayer.GetItemDataEx (i)) == id)
      {
        selPlayer.SetCurSel (i);
        fake = true;
        OnCbnSelchangeSelplayer ();
        fake = false;
        return;
      }
    }
  }
}

void CActionsDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}
