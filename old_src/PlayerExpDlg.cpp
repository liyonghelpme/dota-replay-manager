// CPlayerExpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "PlayerExpDlg.h"

#include "ilib.h"
#include "glib.h"
#include "replay.h"
#include "colorlist.h"

class CExpGraph : public CWnd
{
	DECLARE_DYNAMIC(CExpGraph)

//  OpenGL* gl;
  W3GReplay* w3g;
  bool pen[10];
  int pid[10];
  bool zooming;
  CRect zoomRect;
  CRect focusRect;
  void makeFocusRect (int x, int y);

public:
	CExpGraph(CRect const& rc, CWnd* parent);

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
IMPLEMENT_DYNAMIC(CExpGraph, CWnd)
CExpGraph::CExpGraph(CRect const& rc, CWnd* parent)
{
  zooming = false;
//  gl = NULL;
  w3g = NULL;
  CreateEx (WS_EX_CLIENTEDGE, 
    ::AfxRegisterWndClass (CS_HREDRAW | CS_VREDRAW,
    LoadCursor (::AfxGetInstanceHandle (), MAKEINTRESOURCE (IDC_ZOOMIN))),
    "", WS_CHILD, rc, parent, IDC_TIMEPIC);
  ShowWindow (SW_SHOW);
}
void CExpGraph::makeFocusRect (int x, int y)
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

void CExpGraph::setReplay (W3GReplay* replay)
{
  w3g = replay;
  for (int i = 0; i < 10; i++)
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

void CExpGraph::enablePlayer (int slot, bool enable)
{
  pen[slot] = enable;
  InvalidateRect (NULL, FALSE);
}

BEGIN_MESSAGE_MAP(CExpGraph, CWnd)
  ON_WM_CREATE()
  ON_WM_PAINT()
  ON_WM_DESTROY()
  ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONUP()
  ON_WM_SIZE()
END_MESSAGE_MAP()

// CExpGraph message handlers

void CExpGraph::OnSize (UINT nType, int cx, int cy)
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

void CExpGraph::OnLButtonDown (UINT nFlags, CPoint point)
{
  SetCapture ();
  zooming = true;
  makeFocusRect (point.x, point.y);
  CDC* dc = GetDC ();
  dc->DrawFocusRect (focusRect);
  ReleaseDC (dc);
  InvalidateRect (zoomRect);
}
void CExpGraph::OnMouseMove (UINT nFlags, CPoint point)
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
void CExpGraph::OnLButtonUp (UINT nFlags, CPoint point)
{
  zooming = false;
  CDC* dc = GetDC ();
  dc->DrawFocusRect (focusRect);
  ReleaseDC (dc);
  InvalidateRect (zoomRect);
  ReleaseCapture ();
}

int CExpGraph::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate (lpCreateStruct) == -1)
    return -1;

  GetClientRect (zoomRect);
//  gl = new OpenGL (m_hWnd, 0xFFFFFF);
//  if (!gl->isOk ())
//    MessageBox ("Error initializing OpenGL!", "Error", MB_OK | MB_ICONERROR);

  return 0;
}

static inline int heroLevel (W3GPlayer const& player)
{
  return player.lCount > 1 ? player.lCount : player.hero->level;
}
static inline unsigned long heroTime (W3GPlayer const& player, int i)
{
  return player.lCount <= 1 ? player.hero->atime[i] : player.ltime[i];
}
void CExpGraph::OnPaint ()
{
  CPaintDC dc (this);
//  if (gl->isOk ())
//  {
//    gl->begin ();
  DCPaint dcp (&dc);
  CRect rc;
  GetClientRect (rc);
  ::FillRect (dc.m_hDC, &rc, (HBRUSH) GetStockObject (WHITE_BRUSH));
  if (w3g && w3g->dota.isDota)
  {
    int mx = 0;
    int cnt = 0;
    for (int i = 0; i < 10; i++)
    {
      if (pen[i])
      {
        if (w3g->players[pid[i]].hero && heroLevel (w3g->players[pid[i]]) > mx)
          mx = heroLevel (w3g->players[pid[i]]);
        cnt++;
      }
    }
    mx = lvlExp[mx];
    if (cnt != 0)
    {
      double sy = double (rc.bottom - 30) / double (mx);
      double sx = double (rc.right - 50) / double (w3g->time);

      int prevy = rc.bottom;
      for (int i = 1; i <= 25; i++)
      {
        int cury = rc.bottom - 20 - (rc.bottom - 30) * lvlExp[i] / mx;
        if (cury < 0)
          break;
        if (prevy - cury > 15)
        {
          dcp.setColor (0, 0, 0);
          dcp.drawText (48, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);
          prevy = cury;
        }
        dcp.setColor (0.8, 0.8, 0.8);
        dcp.line (50, cury, rc.right, cury);
      }

      EnumStruct es;
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
            dcp.line (pos, 0, pos, rc.bottom - 30);
          first = false;
        }
        dcp.setColor (0, 0, 0);
        dcp.drawText (curx, rc.bottom - 15, format_time (w3g, i, TIME_HOURS | TIME_SECONDS), ALIGN_X_CENTER | ALIGN_Y_TOP);
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

//      glEnable (GL_LINE_STIPPLE);
      for (int i = 0; i < 10; i++)
      {
        if (!pen[i] || w3g->players[pid[i]].hero == NULL) continue;
        dcp.setColor (getSlotColor (w3g->players[pid[i]].slot.color));
        int prev = 0;
        int prevx = 50;
        int prevy = rc.bottom - 20;
        unsigned long btime[32];
        btime[heroLevel (w3g->players[pid[i]])] = END_TIME;
        for (int j = heroLevel (w3g->players[pid[i]]) - 1; j >= 0; j--)
        {
          btime[j] = btime[j + 1];
          if (heroTime (w3g->players[pid[i]], j) < btime[j])
            btime[j] = heroTime (w3g->players[pid[i]], j);
        }
        for (int j = 1; j <= heroLevel (w3g->players[pid[i]]); j++)
        {
          int cur = lvlExp[j];
          int curx = 50 + int (__int64 (rc.right - 50) * __int64 (btime[j - 1]) /
            __int64 (w3g->time));
          int cury = rc.bottom - 20 - (rc.bottom - 30) * cur / mx;
          if (btime[j - 1] > w3g->players[pid[i]].time)
            dcp.setDash (true);
//            glLineStipple (1, 0xF0F0);
          dcp.line (prevx, prevy, curx, cury);
          prev = cur;
          prevx = curx;
          prevy = cury;
        }
        dcp.setDash (false);
//        glLineStipple (1, 0xFFFF);
        if (heroTime (w3g->players[pid[i]], heroLevel (w3g->players[pid[i]]) - 1) < w3g->players[pid[i]].time)
        {
          int lastx = 50 + int (__int64 (rc.right - 50) * __int64 (w3g->players[pid[i]].time) /
              __int64 (w3g->time));
          dcp.line (prevx, prevy, lastx, prevy);
        }
      }
//      glDisable (GL_LINE_STIPPLE);
      if (zooming)
      {
        dcp.setColor (0, 0, 0);
        ::FillRect (dc.m_hDC, &zoomRect, (HBRUSH) GetStockObject (WHITE_BRUSH));
        dc.Rectangle (&zoomRect);

        int startTime = int ((__int64 (focusRect.left - 50) * __int64 (w3g->time)) / __int64 (rc.right - 50));
        int endTime = int ((__int64 (focusRect.right - 50) * __int64 (w3g->time)) / __int64 (rc.right - 50));
        int startExp = ((rc.bottom - 20 - focusRect.bottom) * mx) / (rc.bottom - 30);
        int endExp = ((rc.bottom - 20 - focusRect.top) * mx) / (rc.bottom - 30);

        double sy = double (zoomRect.bottom - zoomRect.top - 30) / double (endExp - startExp);
        double sx = double (zoomRect.right - zoomRect.left - 60) / double (endTime - startTime);

        int end_y = zoomRect.bottom - 20;
        int size_y = zoomRect.bottom - zoomRect.top - 30;
        int mx_y = endExp - startExp;
        int start_x = zoomRect.left + 50;
        int size_x = zoomRect.right - zoomRect.left - 60;
        int mx_x = endTime - startTime;

        int prevy = zoomRect.bottom;
        for (int i = 1; i <= 25; i++)
        {
          int cury = end_y - size_y * (lvlExp[i] - startExp) / mx_y;
          if (cury < zoomRect.top + 10)
            break;
          if (cury < end_y)
          {
            if (prevy - cury > 15)
            {
              dcp.setColor (0, 0, 0);
              dcp.drawText (zoomRect.left + 48, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);
              prevy = cury;
            }
            dcp.setColor (0.8, 0.8, 0.8);
            dcp.line (zoomRect.left + 50, cury, zoomRect.right - 10, cury);
          }
          if (i < 25)
          {
            dcp.setColor (0.9, 0.9, 0.9);
            int exp = lvlExp[i];
            //int estep = (lvlExp[i + 1] - lvlExp[i]) / 4;
            int estep = 100;
            for (exp += estep; exp < lvlExp[i + 1] - 5; exp += estep)
            {
              cury = end_y - size_y * (exp - startExp) / mx_y;
              if (cury > zoomRect.top + 10 && cury < end_y)
                dcp.line (zoomRect.left + 50, cury, zoomRect.right - 10, cury);
            }
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
        for (int i = 0; i < 10; i++)
        {
          if (!pen[i] || w3g->players[pid[i]].hero == NULL) continue;
          dcp.setColor (getSlotColor (w3g->players[pid[i]].slot.color));
          int prev = 0;
          int prevx = start_x;
          int prevy = end_y;
          unsigned long btime[32];
          btime[heroLevel (w3g->players[pid[i]])] = END_TIME;
          for (int j = heroLevel (w3g->players[pid[i]]) - 1; j >= 0; j--)
          {
            btime[j] = btime[j + 1];
            if (heroTime (w3g->players[pid[i]], j) < btime[j])
              btime[j] = heroTime (w3g->players[pid[i]], j);
          }
          for (int j = 1; j <= heroLevel (w3g->players[pid[i]]); j++)
          {
            int cur = lvlExp[j];
            int curx = start_x + int (__int64 (size_x) * __int64 (int (btime[j - 1]) - startTime) /
              __int64 (mx_x));
            int cury = end_y - size_y * (cur - startExp) / mx_y;
            if (btime[j - 1] > w3g->players[pid[i]].time)
              dcp.setDash (true);
  //            glLineStipple (1, 0xF0F0);
            dcp.line (prevx, prevy, curx, cury);
            prev = cur;
            prevx = curx;
            prevy = cury;
          }
          dcp.setDash (false);
  //        glLineStipple (1, 0xFFFF);
          if (heroTime (w3g->players[pid[i]], heroLevel (w3g->players[pid[i]]) - 1) < w3g->players[pid[i]].time)
          {
            int lastx = start_x + int (__int64 (size_x) * __int64 (int (w3g->players[pid[i]].time) - startTime) /
                __int64 (mx_x));
            dcp.line (prevx, prevy, lastx, prevy);
          }
        }
      }
    }
  }
//    gl->end ();
//  }
}
void CExpGraph::OnDestroy ()
{
//  delete gl;
//  gl = NULL;
}

// CPlayerExpDlg dialog

IMPLEMENT_DYNAMIC(CPlayerExpDlg, CDialog)
CPlayerExpDlg::CPlayerExpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerExpDlg::IDD, pParent)
{
  graph = NULL;
  for (int i = 0; i < 10; i++)
  {
    clrBars[i] = NULL;
    imgBars[i] = NULL;
  }
  Create (IDD, pParent);
}

CPlayerExpDlg::~CPlayerExpDlg()
{
  delete graph;
  for (int i = 0; i < 10; i++)
  {
    delete clrBars[i];
    delete imgBars[i];
  }
}

void CPlayerExpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPlayerExpDlg, CDialog)
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
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CPlayerExpDlg message handlers
static const int playerBoxes[] = {IDC_DETAIL_P1, IDC_DETAIL_P2, IDC_DETAIL_P3, IDC_DETAIL_P4, IDC_DETAIL_P5,
                                  IDC_DETAIL_P6, IDC_DETAIL_P7, IDC_DETAIL_P8, IDC_DETAIL_P9, IDC_DETAIL_P10};
static const char playerNames[][32] = {"Blue", "Teal", "Purple", "Yellow", "Orange",
                                       "Pink", "Gray", "Light Blue", "Dark Green", "Brown"};
#define CBAR_OFFS       111

BOOL CPlayerExpDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  CRect rc;
  GetDlgItem (IDC_IMGRECT)->GetWindowRect (rc);
  ScreenToClient (rc);
  graph = new CExpGraph (rc, this);

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
  loc.Start ();

  return TRUE;
}

void CPlayerExpDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    GetDlgItem (playerBoxes[i])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i], playerNames[i]);
  }
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i + 5], FALSE);
    GetDlgItem (playerBoxes[i + 5])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i + 5], playerNames[i + 5]);
  }
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
    }
  }
  graph->setReplay (replay);
}

void CPlayerExpDlg::OnBnClickedDetailP1()
{
  graph->enablePlayer (0, IsDlgButtonChecked (playerBoxes[0]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP2()
{
  graph->enablePlayer (1, IsDlgButtonChecked (playerBoxes[1]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP3()
{
  graph->enablePlayer (2, IsDlgButtonChecked (playerBoxes[2]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP4()
{
  graph->enablePlayer (3, IsDlgButtonChecked (playerBoxes[3]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP5()
{
  graph->enablePlayer (4, IsDlgButtonChecked (playerBoxes[4]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP6()
{
  graph->enablePlayer (5, IsDlgButtonChecked (playerBoxes[5]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP7()
{
  graph->enablePlayer (6, IsDlgButtonChecked (playerBoxes[6]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP8()
{
  graph->enablePlayer (7, IsDlgButtonChecked (playerBoxes[7]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP9()
{
  graph->enablePlayer (8, IsDlgButtonChecked (playerBoxes[8]) != 0);
}

void CPlayerExpDlg::OnBnClickedDetailP10()
{
  graph->enablePlayer (9, IsDlgButtonChecked (playerBoxes[9]) != 0);
}

void CPlayerExpDlg::OnBnClickedAllsent()
{
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], TRUE);
    graph->enablePlayer (i, true);
  }
}

void CPlayerExpDlg::OnBnClickedNosent()
{
  for (int i = 0; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    graph->enablePlayer (i, false);
  }
}

void CPlayerExpDlg::OnBnClickedAllscourge()
{
  for (int i = 5; i < 10; i++)
  {
    CheckDlgButton (playerBoxes[i], TRUE);
    graph->enablePlayer (i, true);
  }
}

void CPlayerExpDlg::OnBnClickedNoscourge()
{
  for (int i = 5; i < 10; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    graph->enablePlayer (i, false);
  }
}

void CPlayerExpDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}
