// DraftDlg.cpp : implementation file
//

#include "stdafx.h"
#include "replay.h"
#include "DotAReplay.h"
#include "DotAReplayDlg.h"
#include "DraftDlg.h"
#include "ilib.h"

// CDraftDlg dialog

IMPLEMENT_DYNAMIC(CDraftDlg, CDialog)

CDraftDlg::CDraftDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDraftDlg::IDD, pParent)
{
  w3g = NULL;
  Create (IDD, pParent);
}

CDraftDlg::~CDraftDlg()
{
}

void CDraftDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDraftDlg, CDialog)
  ON_WM_DESTROY()
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CDraftDlg message handlers
BOOL CDraftDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  draftPool.Attach (GetDlgItem (IDC_DRAFTPOOL)->m_hWnd);
  draftPool.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  draftBans.Attach (GetDlgItem (IDC_DRAFT_BANS)->m_hWnd);
  draftBans.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  draftPicks.Attach (GetDlgItem (IDC_DRAFT_PICKS)->m_hWnd);
  draftPicks.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

  draftPool.InsertColumn (0, "");
  draftPool.SetImageList (getImageList (), LVSIL_SMALL);
  draftPool.SetColumnWidth (0, LVSCW_AUTOSIZE_USEHEADER);
  draftBans.InsertColumn (0, "");
  draftBans.SetImageList (getImageList (), LVSIL_SMALL);
  draftBans.SetColumnWidth (0, LVSCW_AUTOSIZE_USEHEADER);
  draftPicks.InsertColumn (0, "");
  draftPicks.SetImageList (getImageList (), LVSIL_SMALL);
  draftPicks.SetColumnWidth (0, LVSCW_AUTOSIZE_USEHEADER);

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_SENTINEL_TIP, SIDE_LEFT, PERCENT);
  loc.SetItemRelative (IDC_SCOURGE_TIP, SIDE_LEFT, IDC_SENTINEL_TIP, SIDE_LEFT);
  loc.SetItemRelative (IDC_BANS_TIP, SIDE_LEFT, IDC_SENTINEL_TIP, SIDE_LEFT);
  loc.SetItemRelative (IDC_DRAFT_BANS, SIDE_LEFT, IDC_SENTINEL_TIP, SIDE_LEFT);
  loc.SetItemRelative (IDC_PICKS_TIP, SIDE_LEFT, IDC_SENTINEL_TIP, SIDE_LEFT);
  loc.SetItemRelative (IDC_DRAFT_PICKS, SIDE_LEFT, IDC_SENTINEL_TIP, SIDE_LEFT);
  loc.SetItemRelative (IDC_DRAFTPOOL, SIDE_RIGHT, IDC_SENTINEL_TIP, SIDE_LEFT);

  loc.SetItemAbsolute (IDC_DRAFTPOOL, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_DRAFT_BANS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_DRAFT_PICKS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_DRAFT_PICKS, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}
void CDraftDlg::OnDestroy ()
{
  draftPool.Detach ();
  draftBans.Detach ();
  draftPicks.Detach ();
}
void CDraftDlg::OnSize (UINT nType, int cx, int cy)
{
  if (nType != SIZE_MINIMIZED)
    loc.Update ();
}

void CDraftDlg::setReplay (W3GReplay* tw3g)
{
  w3g = tw3g;
  draftPool.DeleteAllItems ();
  draftBans.DeleteAllItems ();
  draftPicks.DeleteAllItems ();
  draftPool.EnableWindow (FALSE);
  draftBans.EnableWindow (FALSE);
  draftPicks.EnableWindow (FALSE);
  if (w3g && w3g->dota.isDota)
  {
    DraftData* draft = &(w3g->game.draft);
    if (draft->numPool)
    {
      draftPool.EnableWindow (TRUE);
      for (int i = 0; i < draft->numPool; i++)
      {
        DotaHero* hero = getHero (draft->pool[i]);
        int pos = draftPool.InsertItem (i, hero->abbr, getImageIndex (hero->imgTag));
        draftPool.SetItemData (pos, (DWORD_PTR) (i & 1 ? 0xFFEEEE : 0xFFFFFF));
      }
    }
    if (draft->numPicks[0] || draft->numPicks[1] || draft->numBans[0] || draft->numBans[1])
    {
      int seCapt = w3g->getCaptain (1);
      int scCapt = w3g->getCaptain (2);
      wchar_t fmt[512];
      if (seCapt >= 0)
      {
        swprintf (fmt, sizeof fmt, L"Sentinel captain: %s", w3g->players[seCapt].uname);
        SetDlgItemTextW (m_hWnd, IDC_SENTINEL_TIP, fmt);
        GetDlgItem (IDC_SENTINEL_TIP)->ShowWindow (SW_SHOW);
      }
      else
        GetDlgItem (IDC_SENTINEL_TIP)->ShowWindow (SW_HIDE);
      if (scCapt >= 0)
      {
        swprintf (fmt, sizeof fmt, L"Scourge captain: %s", w3g->players[scCapt].uname);
        SetDlgItemTextW (m_hWnd, IDC_SCOURGE_TIP, fmt);
        GetDlgItem (IDC_SCOURGE_TIP)->ShowWindow (SW_SHOW);
      }
      else
        GetDlgItem (IDC_SCOURGE_TIP)->ShowWindow (SW_HIDE);
    }
    DWORD colors[2] = {getLightColor (0), getLightColor (6)};
    if (draft->numBans[0] || draft->numBans[1])
    {
      draftBans.EnableWindow (TRUE);
      int cur = draft->firstPick - 1;
      int ptr[2] = {0, 0};
      while (ptr[0] < draft->numBans[0] || ptr[1] < draft->numBans[1])
      {
        if (ptr[cur] < draft->numBans[cur])
        {
          DotaHero* hero = getHero (draft->bans[cur][ptr[cur]++]);
          int pos = draftBans.InsertItem (draftBans.GetItemCount (),
            hero->abbr, getImageIndex (hero->imgTag));
          draftBans.SetItemData (pos, colors[cur]);
        }
        cur = 1 - cur;
      }
    }
    if (draft->numPicks[0] || draft->numPicks[1])
    {
      draftPicks.EnableWindow (TRUE);
      int cur = draft->firstPick - 1;
      int mod = 1;
      int ptr[2] = {0, 0};
      while (ptr[0] < draft->numPicks[0] || ptr[1] < draft->numPicks[1])
      {
        if (ptr[cur] < draft->numPicks[cur])
        {
          DotaHero* hero = getHero (draft->picks[cur][ptr[cur]++]);
          int pos = draftPicks.InsertItem (draftPicks.GetItemCount (),
            hero->abbr, getImageIndex (hero->imgTag));
          draftPicks.SetItemData (pos, colors[cur]);
        }
        mod++;
        if (mod >= 2)
        {
          cur = 1 - cur;
          mod = 0;
        }
      }
    }
  }
}
