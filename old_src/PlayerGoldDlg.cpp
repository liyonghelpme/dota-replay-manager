// PlayerGoldDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "PlayerGoldDlg.h"

#include "glib.h"
#include "ilib.h"
#include "replay.h"
#include ".\playergolddlg.h"
#include "colorlist.h"

extern bool smoothGold;

class CGoldGraph : public CWnd
{
	DECLARE_DYNAMIC(CGoldGraph)

//  OpenGL* gl;
  W3GReplay* w3g;
  bool pen[12];
  int pid[12];
  bool zooming;
  CRect zoomRect;
  CRect focusRect;
  void makeFocusRect (int x, int y);

public:
	CGoldGraph(CRect const& rc, CWnd* parent);

  void setReplay (W3GReplay* w3g);
  void enablePlayer (int slot, bool enable);

protected:
  afx_msg int OnCreate (LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint ();
  afx_msg void OnDestroy ();
  afx_msg void OnLButtonDown (UINT nFlags, CPoint point);
  afx_msg void OnMouseMove (UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp (UINT nFlags, CPoint point);
  afx_msg void OnSize (UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
};
IMPLEMENT_DYNAMIC(CGoldGraph, CWnd)
CGoldGraph::CGoldGraph(CRect const& rc, CWnd* parent)
{
//  gl = NULL;
  zooming = false;
  w3g = NULL;
  CreateEx (WS_EX_CLIENTEDGE, 
    ::AfxRegisterWndClass (CS_HREDRAW | CS_VREDRAW,
    LoadCursor (::AfxGetInstanceHandle (), MAKEINTRESOURCE (IDC_ZOOMIN))),
    "", WS_CHILD, rc, parent, IDC_TIMEPIC);
  ShowWindow (SW_SHOW);
}

void CGoldGraph::makeFocusRect (int x, int y)
{
  CRect rc;
  GetClientRect (rc);
  int width = (zoomRect.right - zoomRect.left - 60) / 4;
  int height = (zoomRect.bottom - zoomRect.top - 30) / 4;
  focusRect.left = x - width / 2;
  if (focusRect.left < 50)
    focusRect.left = 50;
  focusRect.right = focusRect.left + width;
  if (focusRect.right > rc.right)
  {
    focusRect.right = rc.right;
    focusRect.left = focusRect.right - width;
  }
  focusRect.top = y - height / 2;
  if (focusRect.top < 0)
    focusRect.top = 0;
  focusRect.bottom = focusRect.top + height;
  if (focusRect.bottom > rc.bottom - 20)
  {
    focusRect.bottom = rc.bottom - 20;
    focusRect.top = focusRect.bottom - height;
  }
}

void CGoldGraph::setReplay (W3GReplay* replay)
{
  w3g = replay;
  for (int i = 0; i < 12; i++)
  {
    pid[i] = -1;
    pen[i] = false;
  }
  if (w3g != NULL && w3g->dota.isDota)
  {
    for (int i = 0; i < w3g->numPlayers; i++)
    {
      int id = w3g->pindex[i];
      int clr = w3g->players[id].slot.color - 1;
      if (clr < 0 || clr > 10 || clr == 5)
        continue;
      if (clr > 5) clr--;
      pid[clr] = id;
      pen[clr] = true;
    }
  }
  InvalidateRect (NULL, FALSE);
}

void CGoldGraph::enablePlayer (int slot, bool enable)
{
  pen[slot] = enable;
  InvalidateRect (NULL, FALSE);
}

BEGIN_MESSAGE_MAP(CGoldGraph, CWnd)
  ON_WM_CREATE()
  ON_WM_PAINT()
  ON_WM_DESTROY()
  ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONUP()
  ON_WM_SIZE()
END_MESSAGE_MAP()

void CGoldGraph::OnSize (UINT nType, int cx, int cy)
{
  zoomRect.left = 52;
  zoomRect.top = 2;
  if (cx * 3 > cy * 4)
  {
    zoomRect.right = zoomRect.left + cy * 2 / 3 + 60;
    zoomRect.bottom = zoomRect.top + cy / 2 + 30;
  }
  else
  {
    zoomRect.right = zoomRect.left + cx / 2 + 60;
    zoomRect.bottom = zoomRect.top + cx * 3 / 8 + 30;
  }
}

// CGoldGraph message handlers

void CGoldGraph::OnLButtonDown (UINT nFlags, CPoint point)
{
  SetCapture ();
  zooming = true;
  makeFocusRect (point.x, point.y);
  CDC* dc = GetDC ();
  dc->DrawFocusRect (focusRect);
  ReleaseDC (dc);
  InvalidateRect (zoomRect);
}
void CGoldGraph::OnMouseMove (UINT nFlags, CPoint point)
{
  if (zooming)
  {
    CDC* dc = GetDC ();
    dc->DrawFocusRect (focusRect);
    makeFocusRect (point.x, point.y);
    dc->DrawFocusRect (focusRect);
    ReleaseDC (dc);
    InvalidateRect (zoomRect, FALSE);
  }
}
void CGoldGraph::OnLButtonUp (UINT nFlags, CPoint point)
{
  zooming = false;
  CDC* dc = GetDC ();
  dc->DrawFocusRect (focusRect);
  ReleaseDC (dc);
  InvalidateRect (zoomRect);
  ReleaseCapture ();
}
int CGoldGraph::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate (lpCreateStruct) == -1)
    return -1;

  GetClientRect (zoomRect);
//  gl = new OpenGL (m_hWnd, 0xFFFFFF);
//  if (!gl->isOk ())
//    MessageBox ("Error initializing OpenGL!", "Error", MB_OK | MB_ICONERROR);

  return 0;
}

struct PLine
{
  unsigned long t1;
  unsigned long t2;
  int g1;
  int g2;
};
int complong (void const* arg1, void const* arg2)
{
  return (*(long*)arg1) - (*(long*)arg2);
}

void CGoldGraph::OnPaint ()
{
  CPaintDC dc (this);
//  if (gl->isOk ())
//  {
//    gl->begin ();
  DCPaint dcp (&dc);
  CRect rc;
  GetClientRect (rc);
  ::FillRect (dc.m_hDC, &rc, (HBRUSH) GetStockObject (WHITE_BRUSH));
  PLine rpl[12][1024];
  int rpc[12];
  unsigned long sit[1024];
  if (w3g && w3g->dota.isDota)
  {
    int mx = 0;
    int cnt = 0;
    int ssc[2] = {0, 0};
    for (int i = 0; i < 10; i++)
    {
      if (pen[i])
      {
        if (w3g->players[pid[i]].itemCost > mx)
          mx = w3g->players[pid[i]].itemCost;
        cnt++;
      }
      if (pid[i] >= 0)
        ssc[w3g->players[pid[i]].slot.team] += w3g->players[pid[i]].itemCost;
    }
    for (int i = 0; i < 2; i++)
    {
      if (pen[i + 10])
      {
        cnt++;
        if (ssc[i] > mx)
          mx = ssc[i];
      }
    }
    if (cnt != 0)
    {
      double sy = double (rc.bottom - 30) / double (mx);
      double sx = double (rc.right - 50) / double (w3g->time);

      EnumStruct es;
      enumCount (es);
      while (double (es.val) * sy < 15)
        nextCount (es);
      for (int i = 0; i <= mx + es.val; i += es.val)
      {
        int cury = rc.bottom - 20 - (rc.bottom - 30) * i / mx;
        dcp.setColor (0, 0, 0);
        dcp.drawText (48, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);
        dcp.setColor (0.8, 0.8, 0.8);
        dcp.line (50, cury, rc.right, cury);
      }

      enumTime (es);
      while (double (es.val) * sx < 40)
        nextTime (es);
      int subx = int (double (es.sub) * sx + 0.5);
      int _start = w3g->game.startTime;
      bool first = true;
      for (unsigned long i = _start - (_start / es.val) * es.val; i < w3g->time; i += es.val)
      {
        int curx = 50 + int (__int64 (rc.right - 50) * __int64 (i) / __int64 (w3g->time));
        if (first)
        {
          dcp.setColor (0.9, 0.9, 0.9);
          for (int pos = curx; pos > 50; pos -= subx)
            dcp.line (pos, 0, pos, rc.bottom - 20);
          first = false;
        }
        dcp.setColor (0, 0, 0);
        dcp.drawText (curx, rc.bottom - 15, format_time (w3g, i, TIME_HOURS | TIME_SECONDS),
          ALIGN_X_CENTER | ALIGN_Y_TOP);
        dcp.line (curx, rc.bottom - 20, curx, rc.bottom - 15);
        dcp.setColor (0.8, 0.8, 0.8);
        dcp.line (curx, 0, curx, rc.bottom - 20);
        dcp.setColor (0.9, 0.9, 0.9);
        int pos = curx;
        for (int j = 0; j < 4; j++)
        {
          pos += subx;
          dcp.line (pos, 0, pos, rc.bottom - 20);
        }
      }

      dcp.setColor (0, 0, 0);
      dcp.line (50, 0, 50, rc.bottom - 20);
      dcp.line (50, rc.bottom - 20, rc.right, rc.bottom - 20);

      for (int i = 0; i < 10; i++)
      {
        rpc[i] = 0;
        if (pid[i] < 0 || w3g->players[pid[i]].hero == NULL || w3g->players[pid[i]].itemCost == 0) continue;
        if (pen[i])
          dcp.setColor (getSlotColor (w3g->players[pid[i]].slot.color));
        int count = w3g->players[pid[i]].inv.num_items;

        int prevc = w3g->players[pid[i]].itemCost;
        int prevy = rc.bottom - 20 - (rc.bottom - 30) * prevc / mx;
        unsigned long prevt = w3g->players[pid[i]].time;
        int prevx = 50 + int (__int64 (rc.right - 50) * __int64 (prevt) / __int64 (w3g->time));
        int curc = prevc;
        for (int j = count; j >= 0; j--)
        {
          unsigned long curt;
          if (j == 0)
            curt = 0;
          else
            curt = w3g->players[pid[i]].inv.itemt[j - 1];
          if (j == count)
            curc -= w3g->players[pid[i]].stats[STAT_GOLD];
          else
            curc -= getItem (w3g->players[pid[i]].inv.items[j])->cost;
          if (j == 0 || !smoothGold || int (prevt - curt) > 60 * (prevc - curc))
          {
            int curx = 50 + int (__int64 (rc.right - 50) * __int64 (curt) / __int64 (w3g->time));
            int cury = rc.bottom - 20 - (rc.bottom - 30) * curc / mx;
            if (pen[i])
              dcp.line (prevx, prevy, curx, cury);
            rpl[i][rpc[i]].t2 = prevt;
            rpl[i][rpc[i]].t1 = curt;
            rpl[i][rpc[i]].g2 = prevc;
            rpl[i][rpc[i]++].g1 = curc;
            prevx = curx;
            prevy = cury;
            prevt = curt;
            prevc = curc;
          }
        }
      }
      for (int i = 0; i < 2; i++)
      {
        if (!pen[i + 10]) continue;
        int sii[10];
        int nump = 0;
        int nums = 0;
        rpc[i + 10] = 0;
        for (int j = 0; j < 10; j++)
        {
          if (pid[j] < 0) continue;
          if (w3g->players[pid[j]].slot.team == i)
          {
            sii[nump++] = j;
            for (int k = 0; k < rpc[j]; k++)
            {
              sit[nums++] = rpl[j][k].t1;
              sit[nums++] = rpl[j][k].t2;
            }
          }
        }
        if (nums == 0) continue;
        qsort (sit, nums, sizeof sit[0], complong);
        int prevx, prevy;
        int prevg, prevt;
        dcp.setColor (getSlotColor (i * 6));
        for (int j = 0; j < nums; j++)
        {
          if (j && sit[j] == sit[j - 1]) continue;
          int gold = 0;
          for (int k = 0; k < nump; k++)
          {
            bool found = false;
            for (int n = rpc[sii[k]] - 1; n >= 0 && !found; n--)
            {
              if (rpl[sii[k]][n].t2 >= sit[j])
              {
                int cur = rpl[sii[k]][n].g1 + int (__int64 (rpl[sii[k]][n].g2 - rpl[sii[k]][n].g1) *
                  __int64 (sit[j] - rpl[sii[k]][n].t1) / __int64 (rpl[sii[k]][n].t2 - rpl[sii[k]][n].t1));
                gold += cur;
                found = true;
              }
            }
            if (!found)
              gold += w3g->players[pid[sii[k]]].itemCost;
          }
          int curx = 50 + int (__int64 (rc.right - 50) * __int64 (sit[j]) / __int64 (w3g->time));
          int cury = rc.bottom - 20 - (rc.bottom - 30) * gold / mx;
          if (j)
          {
            dcp.line (prevx, prevy, curx, cury);
            rpl[i + 10][rpc[i + 10]].t2 = prevt;
            rpl[i + 10][rpc[i + 10]].t1 = sit[j];
            rpl[i + 10][rpc[i + 10]].g2 = prevg;
            rpl[i + 10][rpc[i + 10]++].g1 = gold;
          }
          prevx = curx;
          prevy = cury;
          prevg = gold;
          prevt = sit[j];
        }
      }
      if (zooming)
      {
        dcp.setColor (0, 0, 0);
        ::FillRect (dc.m_hDC, &zoomRect, (HBRUSH) GetStockObject (WHITE_BRUSH));
        dc.Rectangle (&zoomRect);

        int startTime = int ((__int64 (focusRect.left - 50) * __int64 (w3g->time)) / __int64 (rc.right - 50));
        int endTime = int ((__int64 (focusRect.right - 50) * __int64 (w3g->time)) / __int64 (rc.right - 50));
        int startGold = ((rc.bottom - 20 - focusRect.bottom) * mx) / (rc.bottom - 30);
        int endGold = ((rc.bottom - 20 - focusRect.top) * mx) / (rc.bottom - 30);

        double sy = double (zoomRect.bottom - zoomRect.top - 30) / double (endGold - startGold);
        double sx = double (zoomRect.right - zoomRect.left - 60) / double (endTime - startTime);

        int end_y = zoomRect.bottom - 20;
        int size_y = zoomRect.bottom - zoomRect.top - 30;
        int mx_y = endGold - startGold;
        int start_x = zoomRect.left + 50;
        int size_x = zoomRect.right - zoomRect.left - 60;
        int mx_x = endTime - startTime;

        EnumStruct es;
        enumCount (es);
        while (double (es.val) * sy < 15)
          nextCount (es);
        for (int i = 0; i <= mx + es.val; i += es.val)
        {
          int cury = end_y - size_y * (i - startGold) / mx_y;
          if (cury > zoomRect.top + 10 && cury < zoomRect.bottom - 20)
          {
            dcp.setColor (0, 0, 0);
            dcp.drawText (zoomRect.left + 48, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);
            dcp.setColor (0.8, 0.8, 0.8);
            dcp.line (zoomRect.left + 50, cury, zoomRect.right - 10, cury);
          }
        }

        enumTime (es);
        while (double (es.val) * sx < 40)
          nextTime (es);
        int subx = int (double (es.sub) * sx + 0.5);
        int _start = w3g->game.startTime;
        bool first = true;
        for (int i = _start - (_start / es.val) * es.val; i < w3g->time; i += es.val)
        {
          int curx = start_x + int (__int64 (size_x) * __int64 (i - startTime) / __int64 (mx_x));
          if (first && curx > zoomRect.left + 50)
          {
            dcp.setColor (0.9, 0.9, 0.9);
            for (int pos = curx; pos > zoomRect.left + 50; pos -= subx)
              if (pos < zoomRect.right - 10)
                dcp.line (pos, zoomRect.top + 10, pos, zoomRect.bottom - 20);
            first = false;
          }
          if (curx > zoomRect.left + 50 && curx < zoomRect.right - 10)
          {
            dcp.setColor (0, 0, 0);
            dcp.drawText (curx, zoomRect.bottom - 15, format_time (w3g, i, TIME_HOURS | TIME_SECONDS),
              ALIGN_X_CENTER | ALIGN_Y_TOP);
            dcp.line (curx, zoomRect.bottom - 20, curx, zoomRect.bottom - 15);
            dcp.setColor (0.8, 0.8, 0.8);
            dcp.line (curx, zoomRect.top + 10, curx, zoomRect.bottom - 20);
          }
          dcp.setColor (0.9, 0.9, 0.9);
          int pos = curx;
          for (int j = 0; j < 4; j++)
          {
            pos += subx;
            if (curx > zoomRect.left + 50 && curx < zoomRect.right - 10)
              dcp.line (pos, zoomRect.top + 10, pos, zoomRect.bottom - 20);
          }
        }

        dcp.setColor (0, 0, 0);
        dcp.line (zoomRect.left + 50, zoomRect.top + 10, zoomRect.left + 50, zoomRect.bottom - 20);
        dcp.line (zoomRect.right - 10, zoomRect.top + 10, zoomRect.right - 10, zoomRect.bottom - 20);
        dcp.line (zoomRect.left + 50, zoomRect.top + 10, zoomRect.right - 10, zoomRect.top + 10);
        dcp.line (zoomRect.left + 50, zoomRect.bottom - 20, zoomRect.right - 10, zoomRect.bottom - 20);

        dc.IntersectClipRect (zoomRect.left + 51, zoomRect.top + 11, zoomRect.right - 10, zoomRect.bottom - 20);
        for (int i = 0; i < 12; i++)
        {
          if (!pen[i])
            continue;
          if (i < 10)
          {
            if (pid[i] < 0 || w3g->players[pid[i]].hero == NULL || w3g->players[pid[i]].itemCost == 0) continue;
            dcp.setColor (getSlotColor (w3g->players[pid[i]].slot.color));
          }
          else
            dcp.setColor (getSlotColor ((i - 10) * 6));

          for (int j = 0; j < rpc[i]; j++)
          {
            if (rpl[i][j].t1 < startTime && rpl[i][j].t2 < startTime) continue;
            if (rpl[i][j].t1 > endTime && rpl[i][j].t2 > endTime) continue;
            if (rpl[i][j].g1 < startGold && rpl[i][j].g2 < startGold) continue;
            if (rpl[i][j].g1 > endGold && rpl[i][j].g2 > endGold) continue;
            int x0 = start_x + int (__int64 (size_x) * __int64 (int (rpl[i][j].t2) - startTime) / __int64 (mx_x));
            int y0 = end_y - size_y * (rpl[i][j].g2 - startGold) / mx_y;
            int x1 = start_x + int (__int64 (size_x) * __int64 (int (rpl[i][j].t1) - startTime) / __int64 (mx_x));
            int y1 = end_y - size_y * (rpl[i][j].g1 - startGold) / mx_y;
            dcp.line (x0, y0, x1, y1);
          }
        }
      }
    }
  }
//    gl->end ();
//  }
}
void CGoldGraph::OnDestroy ()
{
//  delete gl;
//  gl = NULL;
}

// CPlayerGoldDlg dialog

IMPLEMENT_DYNAMIC(CPlayerGoldDlg, CDialog)
CPlayerGoldDlg::CPlayerGoldDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerGoldDlg::IDD, pParent)
{
  graph = NULL;
  for (int i = 0; i < 12; i++)
  {
    clrBars[i] = NULL;
    imgBars[i] = NULL;
  }
  Create (IDD, pParent);
}

CPlayerGoldDlg::~CPlayerGoldDlg()
{
  delete graph;
  for (int i = 0; i < 12; i++)
  {
    delete clrBars[i];
    delete imgBars[i];
  }
}

void CPlayerGoldDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPlayerGoldDlg, CDialog)
  ON_BN_CLICKED(IDC_DETAIL_P1, OnBnClickedDetailP1)
  ON_BN_CLICKED(IDC_DETAIL_P2, OnBnClickedDetailP2)
  ON_BN_CLICKED(IDC_DETAIL_P3, OnBnClickedDetailP3)
  ON_BN_CLICKED(IDC_DETAIL_P4, OnBnClickedDetailP4)
  ON_BN_CLICKED(IDC_DETAIL_P5, OnBnClickedDetailP5)
  ON_BN_CLICKED(IDC_DETAIL_P6, OnBnClickedDetailP6)
  ON_BN_CLICKED(IDC_DETAIL_P7, OnBnClickedDetailP7)
  ON_BN_CLICKED(IDC_DETAIL_P8, OnBnClickedDetailP8)
  ON_BN_CLICKED(IDC_DETAIL_P9, OnBnClickedDetailP9)
  ON_BN_CLICKED(IDC_DETAIL_P10, OnBnClickedDetailP10)
  ON_BN_CLICKED(IDC_ALLSENT, OnBnClickedAllsent)
  ON_BN_CLICKED(IDC_NOSENT, OnBnClickedNosent)
  ON_BN_CLICKED(IDC_ALLSCOURGE, OnBnClickedAllscourge)
  ON_BN_CLICKED(IDC_NOSCOURGE, OnBnClickedNoscourge)
  ON_BN_CLICKED(IDC_DETAIL_S1, OnBnClickedDetailS1)
  ON_BN_CLICKED(IDC_DETAIL_S2, OnBnClickedDetailS2)
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CPlayerGoldDlg message handlers
static const int playerBoxes[] = {IDC_DETAIL_P1, IDC_DETAIL_P2, IDC_DETAIL_P3, IDC_DETAIL_P4, IDC_DETAIL_P5,
                                  IDC_DETAIL_P6, IDC_DETAIL_P7, IDC_DETAIL_P8, IDC_DETAIL_P9, IDC_DETAIL_P10,
                                  IDC_DETAIL_S1, IDC_DETAIL_S2};
static const char playerNames[][32] = {"Blue", "Teal", "Purple", "Yellow", "Orange",
                                       "Pink", "Gray", "Light Blue", "Dark Green", "Brown"};
#define CBAR_OFFS       111

BOOL CPlayerGoldDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  CRect rc;
  GetDlgItem (IDC_IMGRECT)->GetWindowRect (rc);
  ScreenToClient (rc);
  graph = new CGoldGraph (rc, this);

  for (int i = 0; i < 10; i++)
  {
    GetDlgItem (playerBoxes[i])->GetWindowRect (rc);
    ScreenToClient (rc);
    rc.top = rc.bottom + 3;
    rc.bottom = rc.top + 10;
    COLORREF clr;
    if (i < 5)
      clr = getSlotColor (i + 1);
    else
      clr = getSlotColor (i + 2);
    clrBars[i] = new CColorRect (clr, rc, this);
    GetDlgItem (playerBoxes[i])->GetWindowRect (rc);
    ScreenToClient (rc);
    rc.top = rc.bottom - 17;
    rc.bottom = rc.top + 16;
    rc.right = rc.left + 16;
    imgBars[i] = new CImageBox (rc, NULL, this);
    GetDlgItem (playerBoxes[i])->GetWindowRect (rc);
    ScreenToClient (rc);
    GetDlgItem (playerBoxes[i])->SetWindowPos (NULL, rc.left + 18, rc.top,
      rc.right - rc.left - 18, rc.bottom - rc.top, SWP_NOZORDER);
  }
  CRect ref;
  GetDlgItem (IDC_ALLSENT)->GetWindowRect (rc);
  GetDlgItem (playerBoxes[10])->GetWindowRect (ref);
  ScreenToClient (rc);
  ScreenToClient (ref);
  rc.top = ref.top + 2; rc.bottom = rc.top + 10;
  clrBars[10] = new CColorRect (getSlotColor (0), rc, this);
  GetDlgItem (IDC_ALLSCOURGE)->GetWindowRect (rc);
  GetDlgItem (playerBoxes[11])->GetWindowRect (ref);
  ScreenToClient (rc);
  ScreenToClient (ref);
  rc.top = ref.top + 2; rc.bottom = rc.top + 10;
  clrBars[11] = new CColorRect (getSlotColor (6), rc, this);

  loc.SetWindow (this);
  loc.SetItemAbsolute (graph, SIDE_RIGHT);
  loc.SetItemAbsolute (graph, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_ALLSCOURGE, SIDE_TOP, IDC_ALLSCOURGE, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_NOSCOURGE, SIDE_TOP, IDC_NOSCOURGE, SIDE_BOTTOM);
  for (int i = 0; i < 10; i++)
  {
    loc.SetItemAbsolute (clrBars[i], SIDE_LEFT, (i == 0 || i == 5) ? VALUE : PERCENT);
    if (i != 4 && i != 9)
      loc.SetItemRelative (clrBars[i], SIDE_RIGHT, clrBars[i + 1], SIDE_LEFT);
    else
      loc.SetItemAbsolute (clrBars[i], SIDE_RIGHT);
    loc.SetItemRelative (GetDlgItem (playerBoxes[i]), SIDE_LEFT, clrBars[i], SIDE_LEFT);
    loc.SetItemRelative (GetDlgItem (playerBoxes[i]), SIDE_RIGHT, clrBars[i], SIDE_RIGHT);
    loc.SetItemRelative (imgBars[i], SIDE_LEFT, clrBars[i], SIDE_LEFT);
  }
  for (int i = 5; i < 10; i++)
  {
    loc.SetItemRelative (playerBoxes[i], SIDE_TOP, playerBoxes[i], SIDE_BOTTOM);
    loc.SetItemRelative (clrBars[i], SIDE_TOP, clrBars[i], SIDE_BOTTOM);
    loc.SetItemRelative (imgBars[i], SIDE_TOP, imgBars[i], SIDE_BOTTOM);
  }
  loc.SetItemRelative (playerBoxes[11], SIDE_TOP, playerBoxes[11], SIDE_BOTTOM);
  loc.SetItemRelative (clrBars[11], SIDE_TOP, clrBars[11], SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}

void CPlayerGoldDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    GetDlgItem (playerBoxes[i])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i], playerNames[i]);
    imgBars[i]->setImage (NULL);
  }
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i + 5], FALSE);
    GetDlgItem (playerBoxes[i + 5])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i + 5], playerNames[i + 5]);
    imgBars[i + 5]->setImage (NULL);
  }
  CheckDlgButton (playerBoxes[10], FALSE);
  CheckDlgButton (playerBoxes[11], FALSE);
  GetDlgItem (playerBoxes[10])->EnableWindow (FALSE);
  GetDlgItem (playerBoxes[11])->EnableWindow (FALSE);
  if (w3g && w3g->dota.isDota)
  {
    for (int i = 0; i < w3g->dota.numSentinel; i++)
    {
      int ind = w3g->players[w3g->dota.sentinel[i]].slot.color - 1;
      GetDlgItem (playerBoxes[ind])->EnableWindow (TRUE);
      CheckDlgButton (playerBoxes[ind], TRUE);
      SetDlgItemText (playerBoxes[ind], w3g->players[w3g->dota.sentinel[i]].name);
      if (w3g->players[w3g->dota.sentinel[i]].hero)
      {
        DotaHero* hero = getHero (w3g->players[w3g->dota.sentinel[i]].hero->id);
        if (hero)
          imgBars[ind]->setImage (getImageBitmap (hero->imgTag));
      }
      GetDlgItem (playerBoxes[10])->EnableWindow (TRUE);
    }
    for (int i = 0; i < w3g->dota.numScourge; i++)
    {
      int ind = w3g->players[w3g->dota.scourge[i]].slot.color - 2;
      GetDlgItem (playerBoxes[ind])->EnableWindow (TRUE);
      CheckDlgButton (playerBoxes[ind], TRUE);
      SetDlgItemText (playerBoxes[ind], w3g->players[w3g->dota.scourge[i]].name);
      if (w3g->players[w3g->dota.scourge[i]].hero)
      {
        DotaHero* hero = getHero (w3g->players[w3g->dota.scourge[i]].hero->id);
        if (hero)
          imgBars[ind]->setImage (getImageBitmap (hero->imgTag));
      }
      GetDlgItem (playerBoxes[11])->EnableWindow (TRUE);
    }
  }
  graph->setReplay (replay);
}

void CPlayerGoldDlg::OnBnClickedDetailP1()
{
  graph->enablePlayer (0, IsDlgButtonChecked (playerBoxes[0]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP2()
{
  graph->enablePlayer (1, IsDlgButtonChecked (playerBoxes[1]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP3()
{
  graph->enablePlayer (2, IsDlgButtonChecked (playerBoxes[2]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP4()
{
  graph->enablePlayer (3, IsDlgButtonChecked (playerBoxes[3]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP5()
{
  graph->enablePlayer (4, IsDlgButtonChecked (playerBoxes[4]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP6()
{
  graph->enablePlayer (5, IsDlgButtonChecked (playerBoxes[5]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP7()
{
  graph->enablePlayer (6, IsDlgButtonChecked (playerBoxes[6]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP8()
{
  graph->enablePlayer (7, IsDlgButtonChecked (playerBoxes[7]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP9()
{
  graph->enablePlayer (8, IsDlgButtonChecked (playerBoxes[8]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailP10()
{
  graph->enablePlayer (9, IsDlgButtonChecked (playerBoxes[9]) != 0);
}

void CPlayerGoldDlg::OnBnClickedAllsent()
{
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], TRUE);
    graph->enablePlayer (i, true);
  }
}

void CPlayerGoldDlg::OnBnClickedNosent()
{
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    graph->enablePlayer (i, false);
  }
}

void CPlayerGoldDlg::OnBnClickedAllscourge()
{
  for (int i = 5; i < 10; i++)
  {
    CheckDlgButton (playerBoxes[i], TRUE);
    graph->enablePlayer (i, true);
  }
}

void CPlayerGoldDlg::OnBnClickedNoscourge()
{
  for (int i = 5; i < 10; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    graph->enablePlayer (i, false);
  }
}

void CPlayerGoldDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}

void CPlayerGoldDlg::OnBnClickedDetailS1()
{
  graph->enablePlayer (10, IsDlgButtonChecked (playerBoxes[10]) != 0);
}

void CPlayerGoldDlg::OnBnClickedDetailS2()
{
  graph->enablePlayer (11, IsDlgButtonChecked (playerBoxes[11]) != 0);
}
