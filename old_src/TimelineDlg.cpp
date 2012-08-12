// TimelineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "TimelineDlg.h"
#include ".\timelinedlg.h"

#include "replay.h"
#include "timepicture.h"

// CTimelineDlg dialog

IMPLEMENT_DYNAMIC(CTimelineDlg, CDialog)
CTimelineDlg::CTimelineDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTimelineDlg::IDD, pParent)
{
  speed = 1;
  pic = NULL;
  Create (IDD, pParent);
  w3g = NULL;
}

CTimelineDlg::~CTimelineDlg()
{
  delete pic;
}

void CTimelineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTimelineDlg, CDialog)
  ON_WM_DESTROY()
  ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpin1)
  ON_BN_CLICKED(IDC_PLAY, OnBnClickedPlay)
  ON_WM_TIMER()
  ON_WM_SHOWWINDOW()
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CTimelineDlg message handlers

BOOL CTimelineDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  GetDlgItem (IDC_PLAY)->SendMessage (BM_SETIMAGE, IMAGE_ICON,
    (LPARAM) LoadIcon (::AfxGetInstanceHandle (), MAKEINTRESOURCE (IDI_PLAY)));

  slider.Attach (GetDlgItem (IDC_TIMESLIDER)->m_hWnd);

  SetDlgItemText (IDC_SPEED, "1x");

  CRect rc;
  GetDlgItem (IDC_PICFRAME)->GetWindowRect (rc);
  ScreenToClient (rc);
  pic = new CTimePicture (rc, this);
  CRect wrc;
  GetClientRect (wrc);
  offs.left = rc.left;
  offs.top = rc.top;
  offs.right = wrc.right - rc.right;
  offs.bottom = wrc.bottom - rc.bottom;

  SetTimer (43, 100, NULL);

  loc.SetWindow (this);
  //loc.SetItemAbsolute (pic, SIDE_RIGHT);
  //loc.SetItemAbsolute (pic, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_SPEED, SIDE_TOP, IDC_SPEED, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_SPIN1, SIDE_TOP, IDC_SPEED, SIDE_TOP);
  loc.SetItemRelative (IDC_PLAY, SIDE_TOP, IDC_PLAY, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_TIMESLIDER, SIDE_RIGHT);
  loc.SetItemRelative (IDC_TIMESLIDER, SIDE_TOP, IDC_TIMESLIDER, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_CURTIME, SIDE_LEFT, IDC_CURTIME, SIDE_RIGHT);
  loc.SetItemRelative (IDC_CURTIME, SIDE_TOP, IDC_CURTIME, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}

void CTimelineDlg::OnDestroy ()
{
  slider.Detach ();
}

void CTimelineDlg::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
  speed -= pNMUpDown->iDelta;
  if (speed == 0) speed -= pNMUpDown->iDelta;
  else if (speed > 99) speed = 30;
  else if (speed < -99) speed = -30;
  SetDlgItemText (IDC_SPEED, mprintf ("%dx", speed));
  SetFocus ();
  *pResult = 0;
}

#define SLIDER_SCALE(x) else if (w3g->time < x * 20000) slider.SetTicFreq (x * 1000);

void CTimelineDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
  lastTime = GetTickCount ();
  time = 0;
  prevPos = 0;
  CheckDlgButton (IDC_PLAY, FALSE);
  pic->setReplay (replay);
  if (replay)
  {
    GetDlgItem (IDC_SPIN1)->EnableWindow (TRUE);
    GetDlgItem (IDC_TIMESLIDER)->EnableWindow (TRUE);
    GetDlgItem (IDC_PLAY)->EnableWindow (TRUE);
    slider.SetLineSize (60000);
    slider.SetPageSize (600000);
    slider.SetPos (0);
    slider.SetRange (0, w3g->time, TRUE);
    if (w3g->time < 20000)
      slider.SetTicFreq (1000);
    SLIDER_SCALE (2)
    SLIDER_SCALE (5)
    SLIDER_SCALE (10)
    SLIDER_SCALE (30)
    SLIDER_SCALE (60)
    SLIDER_SCALE (120)
    SLIDER_SCALE (300)
    SLIDER_SCALE (600)
    else
      slider.SetTicFreq (900000);
  }
  else
  {
    GetDlgItem (IDC_SPIN1)->EnableWindow (FALSE);
    GetDlgItem (IDC_TIMESLIDER)->EnableWindow (FALSE);
    GetDlgItem (IDC_PLAY)->EnableWindow (FALSE);
  }
}

void CTimelineDlg::OnTimer (UINT_PTR nIDEvent)
{
  if (w3g)
  {
    time = slider.GetPos ();
    if (IsDlgButtonChecked (IDC_PLAY))
    {
      int delta = GetTickCount () - lastTime;
      if (int (time) < -delta * speed)
        time = 0;
      else
      {
        time += delta * speed;
        if (time > w3g->time)
          time = w3g->time;
      }
      slider.SetPos (time);
      lastTime = GetTickCount ();
    }
    if (time != prevPos)
    {
      pic->setTime (time);
      prevPos = time;
    }
    if (IsDlgButtonChecked (IDC_PLAY) || (GetTickCount () % 1400) > 400)
      SetDlgItemText (IDC_CURTIME, format_time (w3g, time, TIME_HOURS | TIME_SECONDS));
    else
      SetDlgItemText (IDC_CURTIME, "");
  }
  else
    SetDlgItemText (IDC_CURTIME, "");
}

void CTimelineDlg::OnBnClickedPlay()
{
  lastTime = GetTickCount ();
}

void CTimelineDlg::OnShowWindow (BOOL bShow, UINT nStatus)
{
  if (bShow == FALSE)
    CheckDlgButton (IDC_PLAY, FALSE);
}

static double rfix = 487.0 / 512.0;
void CTimelineDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
  if (pic)
  {
    CRect rc;
    rc.left = offs.left;
    rc.top = offs.top;
    rc.right = cx - offs.right;
    rc.bottom = cy - offs.bottom;
    int wd = rc.Width () - 4;
    int ht = rc.Height () - 4;
    if (wd * 487 < ht * 512)
      ht = (wd * 487) / 512;
    else
      wd = (ht * 512) / 487;
    wd += 4;
    ht += 4;
    pic->SetWindowPos (NULL, (rc.left + rc.right - wd) / 2,
                             (rc.top + rc.bottom - ht) / 2,
                             wd, ht, SWP_NOZORDER);
  }
}
