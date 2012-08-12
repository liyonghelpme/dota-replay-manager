// PlayerInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "replay.h"
#include "DotAReplay.h"
#include "PlayerInfoDlg.h"
#include "DotAReplayDlg.h"

#include ".\playerinfodlg.h"

#include "ilib.h"

// CPlayerInfoDlg dialog

IMPLEMENT_DYNAMIC(CPlayerInfoDlg, CDialog)
CPlayerInfoDlg::CPlayerInfoDlg(CDotAReplayDlg* d, CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerInfoDlg::IDD, pParent)
{
  lockSelChange = false;
  dlg = d;
  w3g = NULL;
  fake = false;
  for (int i = 0; i < 5; i++)
    kicons[i] = NULL;
  Create (IDD, pParent);
}

CPlayerInfoDlg::~CPlayerInfoDlg()
{
  for (int i = 0; i < 5; i++)
    delete kicons[i];
}

void CPlayerInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPlayerInfoDlg, CDialog)
  ON_WM_DESTROY()
  ON_CBN_SELCHANGE(IDC_SELPLAYER, OnCbnSelchangeSelplayer)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_SKILLLIST, OnLvnItemchangedSkilllist)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_ITEMLIST, OnLvnItemchangedItemlist)
  ON_WM_SIZE()
  ON_NOTIFY(HDN_TRACK, 0, OnHdnTrackSkilllist)
  ON_NOTIFY(HDN_TRACK, 0, OnHdnTrackItemlist)
  ON_WM_PAINT()
END_MESSAGE_MAP()

static int kdtext[5] = {IDC_KDD0, IDC_KDD1, IDC_KDD2, IDC_KDD3, IDC_KDD4};

// CPlayerInfoDlg message handlers

BOOL CPlayerInfoDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  itemInfo.Attach (GetDlgItem (IDC_ITEMLIST)->m_hWnd);
  itemInfo.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
  skillInfo.Attach (GetDlgItem (IDC_SKILLLIST)->m_hWnd);
  skillInfo.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
  selPlayer.Attach (GetDlgItem (IDC_SELPLAYER)->m_hWnd);
  selPlayer.SetImageList (getImageList ());
  SetWindowLong (selPlayer.m_hWnd, GWL_STYLE, GetWindowLong (selPlayer.m_hWnd, GWL_STYLE) | CBS_OWNERDRAWVARIABLE);

  skillInfo.InsertColumn (0, "Skill");
  skillInfo.InsertColumn (1, "");
  skillInfo.InsertColumn (2, "Time");
  skillInfo.SetImageList (getImageList (), LVSIL_SMALL);
  itemInfo.InsertColumn (0, "Item");
  itemInfo.InsertColumn (1, "Time");
  itemInfo.InsertColumn (2, "Cost");
  itemInfo.SetImageList (getImageList (), LVSIL_SMALL);

  for (int i = 0; i < 3; i++)
  {
    skillInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
    itemInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
  }
  scol.Read ();
  icol.Read ();

  CBitmap* emptySlot = getImageBitmap ("emptyslot");
  for (int i = 0; i < 5; i++)
  {
    CRect rc;
    GetDlgItem (kdtext[i])->GetWindowRect (rc);
    ScreenToClient (rc);
    rc.top = rc.bottom - 17;
    rc.bottom = rc.top + 16;
    rc.left = rc.left - 18;
    rc.right = rc.left + 16;
    kicons[i] = new CImageBox (rc, emptySlot, this);
  }

  int subst[] = {1, 0, 2};
  skillInfo.SetColumnOrderArray (3, subst);

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_SELPLAYER, SIDE_LEFT, PERCENT);
  loc.SetItemAbsolute (IDC_SELPLAYER, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_SKILLLIST, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_SKILLLIST, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_SKILLTIP, SIDE_LEFT, IDC_SKILLLIST, SIDE_LEFT);
  loc.SetItemRelative (IDC_ITEMTIP, SIDE_LEFT, IDC_ITEMLIST, SIDE_LEFT);
  loc.SetItemRelative (IDC_ITEMLIST, SIDE_LEFT, IDC_SKILLLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_ITEMLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_ITEMLIST, SIDE_BOTTOM);

  loc.SetItemRelative (IDC_KILLDET, SIDE_TOP, IDC_KILLDET, SIDE_BOTTOM);
  for (int i = 0; i < 5; i++)
  {
    loc.SetItemRelative (kicons[i], SIDE_TOP, kicons[i], SIDE_BOTTOM);
    loc.SetItemRelative (kdtext[i], SIDE_TOP, kdtext[i], SIDE_BOTTOM);
  }

  loc.SetItemRelative (IDC_WARDS, SIDE_LEFT, IDC_ITEMLIST, SIDE_LEFT);
  loc.SetItemRelative (IDC_WARDS, SIDE_TOP, IDC_WARDS, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_FINALBUILD, SIDE_LEFT, IDC_WARDS, SIDE_LEFT);
  loc.SetItemRelative (IDC_FINALBUILD, SIDE_TOP, IDC_FINALBUILD, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_NOBUILD, SIDE_LEFT, IDC_FINALBUILD, SIDE_LEFT);
  loc.SetItemRelative (IDC_NOBUILD, SIDE_TOP, IDC_FINALBUILD, SIDE_TOP);
  loc.Start ();
  scol.SetListCtrl (&skillInfo);
  scol.Read ();
  icol.SetListCtrl (&itemInfo);
  icol.Read ();

  return TRUE;
}
void CPlayerInfoDlg::OnDestroy ()
{
  skillInfo.Detach ();
  itemInfo.Detach ();
  selPlayer.Detach ();
}

void CPlayerInfoDlg::setReplay (W3GReplay* tw3g)
{
  w3g = tw3g;
  selPlayer.Reset ();
  skillInfo.DeleteAllItems ();
  itemInfo.DeleteAllItems ();
  if (w3g && w3g->dota.isDota)
  {
    skillInfo.EnableWindow (TRUE);
    itemInfo.EnableWindow (TRUE);
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
    curPlayer = 1;
    OnCbnSelchangeSelplayer ();
  }
  else
  {
    skillInfo.EnableWindow (FALSE);
    itemInfo.EnableWindow (FALSE);
    selPlayer.EnableWindow (FALSE);
  }
}

extern bool showLevels;
extern bool showAssemble;
extern bool skillColors;

void CPlayerInfoDlg::OnCbnSelchangeSelplayer()
{
  if (w3g == NULL || !w3g->dota.isDota)
    return;
  int pl = selPlayer.GetCurSel ();
  int id = (int) selPlayer.GetItemDataEx (pl);
  if (id == 0)
  {
    selPlayer.SetCurSel (curPlayer);
    return;
  }
  if (!fake)
    dlg->selPlayerActions (id);
  curPlayer = pl;

  int normal[] = {0, 1, 2};
  skillInfo.SetColumnOrderArray (3, normal);

  skillInfo.DeleteAllItems ();
  itemInfo.DeleteAllItems ();
  skills.clear ();
  items.clear ();
  int lvl[5] = {0, 0, 0, 0, 0};
  if (w3g->players[id].hero)
  {
    for (int i = 0; i < w3g->players[id].hero->level; i++)
    {
      char const* text;
      int slot = getAbility (w3g->players[id].hero->abilities[i])->slot;
      if (w3g->players[id].hero->abilities[i] == 0)
        text = mprintf ("None");
      else if (showLevels)
      {
        lvl[slot]++;
        text = mprintf ("%s (level %d)", (char const*) getAbility (w3g->players[id].hero->abilities[i])->name, lvl[slot]);
      }
      else
        text = getAbility (w3g->players[id].hero->abilities[i])->name;
      int pos = skillInfo.InsertItem (skillInfo.GetItemCount (), text,
        getImageIndex (getAbility (w3g->players[id].hero->abilities[i])->imgTag));
      skillInfo.SetItemText (pos, 1, mprintf ("%d", i + 1));
      skillInfo.SetItemText (pos, 2, format_time (w3g, w3g->players[id].hero->atime[i]));
      if (skillColors)
      {
        switch (slot)
        {
        case 0:
          skillInfo.SetItemData (pos, (DWORD_PTR) 0xCCFFCC);
          break;
        case 1:
          skillInfo.SetItemData (pos, (DWORD_PTR) 0xFFCCCC);
          break;
        case 2:
          skillInfo.SetItemData (pos, (DWORD_PTR) 0xAAFFFF);
          break;
        case 3:
          skillInfo.SetItemData (pos, (DWORD_PTR) 0xCCCCFF);
          break;
        default:
          skillInfo.SetItemData (pos, (DWORD_PTR) 0xFFFFFF);
        }
      }
      else
        skillInfo.SetItemData (pos, (DWORD_PTR) (i & 1 ? 0xFFEEEE : 0xFFFFFF));
      skills.set (pos, w3g->players[id].hero->atime[i]);
    }
    if (showAssemble)
      w3g->players[id].inv.getAllItems (END_TIME, w3g->dota);
    else
      w3g->players[id].inv.listItems (END_TIME);
    w3g->players[id].inv.sortItems ();
    int numWards[2] = {0, 0};
    int goldWards = 0;
    for (int i = 0; i < w3g->players[id].inv.bi.getSize (); i++)
    {
      DotaItem* item = getItem (w3g->players[id].inv.bi[i].id);
      if (!stricmp (item->name, "Observer Wards"))
      {
        numWards[0]++;
        goldWards += item->cost;
      }
      else if (!stricmp (item->name, "Sentry Wards"))
      {
        numWards[1]++;
        goldWards += item->cost;
      }
      unsigned long itime = w3g->players[id].inv.bi[i].time;
      int image;
      if (showAssemble)
        image = getImageIndex (getItemIcon (w3g->players[id].inv.bi[i].id));
      else
        image = getImageIndex (item->imgTag);
      int pos = itemInfo.InsertItem (itemInfo.GetItemCount (), item->name, image);
      itemInfo.SetItemText (pos, 1, format_time (w3g, itime));
      itemInfo.SetItemText (pos, 2, mprintf ("%d", item->cost));
      if (item->type == ITEM_COMBO)
        itemInfo.SetItemData (pos, (DWORD_PTR) RGB (128, 255, 128));
      else if (w3g->players[id].inv.bi[i].gone)
        itemInfo.SetItemData (pos, (DWORD_PTR) 0xDDDDDD);
      else if (showAssemble)
        itemInfo.SetItemData (pos, (DWORD_PTR) 0xFFFFFF);
      else
        itemInfo.SetItemData (pos, (DWORD_PTR) (i & 1 ? 0xFFEEEE : 0xFFFFFF));
      items.set (pos, itime);
    }
    SetDlgItemText (IDC_WARDS, mprintf ("Total ward sets: %d/%d (%d gold)", numWards[0],
      numWards[1], goldWards));
  }
  for (int i = 0; i < 3; i++)
  {
    skillInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
    itemInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
  }
  scol.Read ();
  icol.Read ();

  int subst[] = {1, 0, 2};
  skillInfo.SetColumnOrderArray (3, subst);
  skillInfo.InvalidateRect (NULL);

  //if (w3g->players[id].slot.team == 0)
  //  kicons[5]->setImage (getImageBitmap ("GreenBullet"));
  //else
  //  kicons[5]->setImage (getImageBitmap ("RedBullet"));
  //SetDlgItemText (kdtext[5], mprintf ("%d/%d", w3g->players[id].skilled, w3g->players[id].sdied));
  CBitmap* emptySlot = getImageBitmap ("emptyslot");
  for (int i = 0; i < 5; i++)
  {
    int e = (w3g->players[id].slot.team == 0 ? w3g->dota.scourge[i] : w3g->dota.sentinel[i]);
    kicons[i]->setImage (emptySlot);
    if (e)
    {
      if (w3g->players[e].hero)
      {
        DotaHero* hero = getHero (w3g->players[e].hero->id);
        if (hero)
          kicons[i]->setImage (getImageBitmap (hero->imgTag));
      }
      GetDlgItem (kdtext[i])->EnableWindow (TRUE);
      SetDlgItemText (kdtext[i], mprintf ("%d/%d", w3g->players[id].pkilled[e], w3g->players[id].pdied[e]));
    }
    else
    {
      GetDlgItem (kdtext[i])->EnableWindow (FALSE);
      SetDlgItemText (kdtext[i], "0/0");
    }
  }

  int count = 0;
  for (int i = 0; i < 6; i++)
    if (w3g->players[id].finalItems[i])
      count++;
  if (count == 0)
    GetDlgItem (IDC_NOBUILD)->ShowWindow (SW_SHOW);
  else
    GetDlgItem (IDC_NOBUILD)->ShowWindow (SW_HIDE);
  InvalidateRect (NULL);
}

void CPlayerInfoDlg::selectPlayer (int id)
{
  if (w3g && w3g->dota.isDota)
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

extern bool syncSelect;

void CPlayerInfoDlg::OnLvnItemchangedSkilllist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  *pResult = 0;
  int sel = skillInfo.GetNextItem (-1, LVNI_SELECTED);
  if (syncSelect && sel >= 0 && !lockSelChange)
  {
    unsigned long time = skills[sel];
    lockSelChange = true;
    for (int i = items.getSize () - 1; i >= 0; i--)
    {
      if (items[i] <= time)
      {
        itemInfo.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
        itemInfo.EnsureVisible (i, FALSE);
        lockSelChange = false;
        return;
      }
    }
    sel = itemInfo.GetNextItem (-1, LVNI_SELECTED);
    if (sel >= 0) itemInfo.SetItemState (sel, 0, LVIS_SELECTED);
    lockSelChange = false;
  }
}

void CPlayerInfoDlg::OnLvnItemchangedItemlist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  *pResult = 0;
  int sel = itemInfo.GetNextItem (-1, LVNI_SELECTED);
  if (syncSelect && sel >= 0 && !lockSelChange)
  {
    unsigned long time = items[sel];
    lockSelChange = true;
    for (int i = skills.getSize () - 1; i >= 0; i--)
    {
      if (skills[i] <= time)
      {
        skillInfo.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
        skillInfo.EnsureVisible (i, FALSE);
        lockSelChange = false;
        return;
      }
    }
    sel = skillInfo.GetNextItem (-1, LVNI_SELECTED);
    if (sel >= 0) skillInfo.SetItemState (sel, 0, LVIS_SELECTED);
    lockSelChange = false;
  }
}

void CPlayerInfoDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
  scol.Write ();
  icol.Write ();
}

void CPlayerInfoDlg::OnHdnTrackSkilllist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  scol.Read ();
  *pResult = 0;
}

void CPlayerInfoDlg::OnHdnTrackItemlist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  icol.Read ();
  *pResult = 0;
}

void CPlayerInfoDlg::OnPaint ()
{
  CPaintDC dc (this);
  CRect rc;
  GetDlgItem (IDC_FINALBUILD)->GetWindowRect (rc);
  ScreenToClient (rc);
  CPoint pos (rc.right + 5, (rc.top + rc.bottom) / 2 - 8);
  int pl = selPlayer.GetCurSel ();
  int id = (int) selPlayer.GetItemDataEx (pl);
  CImageList* icons = getImageList ();
  for (int i = 0; i < 6; i++)
  {
    int iid = w3g->players[id].finalItems[i];
    if (iid)
    {
      DotaItem* item = getItem (iid);
      int index = (item ? getImageIndex (item->imgTag, "Unknown") : getImageIndex ("Unknown"));
      icons->Draw (&dc, index, pos, ILD_NORMAL);
      pos.x += 18;
    }
  }
}
