// PlayerStatsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "PlayerStatsDlg.h"
#include "DotAReplayDlg.h"
#include "ilib.h"
#include "gamecache.h"
#include ".\playerstatsdlg.h"

// CPlayerStatsDlg dialog

IMPLEMENT_DYNAMIC(CPlayerStatsDlg, CDialog)
CPlayerStatsDlg::CPlayerStatsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerStatsDlg::IDD, pParent)
{
  dlg = (CDotAReplayDlg*) pParent;
  games.m_hWnd = NULL;
  Create (IDD, pParent);
}

CPlayerStatsDlg::~CPlayerStatsDlg()
{
}

void CPlayerStatsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPlayerStatsDlg, CDialog)
  ON_WM_SIZE()
  ON_WM_DESTROY()
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_STATS, OnLvnItemActivateStats)
  ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickStats)
END_MESSAGE_MAP()

#define COL_HERO      0
#define COL_KD        1
#define COL_CS        2
#define COL_LANE      3
#define COL_GOLD      4
#define COL_AGOLD     5
#define COL_LVL       6
#define COL_ALVL      7
#define COL_SCORE     8
#define COL_RESULT    9
#define COL_LEFT      10
#define COL_LENGTH    11
#define COL_APM       12
#define COL_NAME      13
#define COL_DATE      14
#define NUM_COLS      15

// CPlayerStatsDlg message handlers

BOOL CPlayerStatsDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  games.simple = true;
  games.Attach (GetDlgItem (IDC_STATS)->m_hWnd);
  games.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  games.SetImageList (getImageList (), LVSIL_SMALL);
  games.InsertColumn (COL_HERO, "Hero");
  games.InsertColumn (COL_KD, "K-D");
  games.InsertColumn (COL_CS, "CS");
  games.InsertColumn (COL_LANE, "Lane");
  games.InsertColumn (COL_GOLD, "Gold");
  games.InsertColumn (COL_AGOLD, "Avg. Gold");
  games.InsertColumn (COL_LVL, "Level");
  games.InsertColumn (COL_ALVL, "Avg. Lvl");
  games.InsertColumn (COL_SCORE, "Score");
  games.InsertColumn (COL_RESULT, "Result");
  games.InsertColumn (COL_LEFT, "Left");
  games.InsertColumn (COL_LENGTH, "Length");
  games.InsertColumn (COL_APM, "APM");
  games.InsertColumn (COL_NAME, "Game name");
  games.InsertColumn (COL_DATE, "Game date");

  return TRUE;
}

void CPlayerStatsDlg::OnSize (UINT nType, int cx, int cy)
{
  if (games.m_hWnd != NULL)
    games.SetWindowPos (NULL, 6, 0, cx - 6, cy, SWP_NOZORDER);
}

void CPlayerStatsDlg::OnDestroy ()
{
  games.Detach ();
}

static int sortBy = COL_HERO;
static bool sortUp = false;

int isGreater (StatsItem const* a, StatsItem const* b)
{
  switch (sortBy)
  {
  case COL_HERO:
    return a->hero - b->hero;
  case COL_KD:
    if (a->kills < 0 || b->kills < 0)
      return a->kills >= 0 ? -1 : 1;
    return a->kills * b->deaths == a->deaths * b->kills
      ? (a->kills == b->kills ? a->deaths - b->deaths : b->kills - a->kills)
      : (b->kills * a->deaths - b->deaths * a->kills);
  case COL_CS:
    if (a->creeps < 0 || b->creeps < 0)
      return a->creeps >= 0 ? -1 : 1;
    return a->creeps == b->creeps ? b->denies - a->denies : b->creeps - a->creeps;
  case COL_LANE:
    return a->lane - b->lane;
  case COL_GOLD:
    return b->gold - a->gold;
  case COL_AGOLD:
    return b->agold - a->agold;
  case COL_LVL:
    return b->lvl - a->lvl;
  case COL_ALVL:
    return b->alvl - a->alvl;
  case COL_SCORE:
    if (a->score[0] < 0 || b->score[0] < 0)
      return a->score[0] >= 0 ? -1 : 1;
    return a->score[0] * b->score[1] == a->score[1] * b->score[0]
      ? (a->score[0] == b->score[0] ? a->score[1] - b->score[1] : b->score[0] - a->score[0])
      : (b->score[0] * a->score[1] - b->score[1] * a->score[0]);
  case COL_LEFT:
    return (a->time - a->left) - (b->time - b->left);
  case COL_LENGTH:
    return b->time - a->time;
  case COL_APM:
    return b->apm - a->apm;
  case COL_NAME:
    return wcscmp (a->name, b->name);
  case COL_DATE:
    return b->date == a->date ? (b->date > a->date ? -1 : 1) : 0;
  default:
    return 0;
  }
}
int itemCompare (void const* ap, void const* bp)
{
  StatsItem const* a = (StatsItem const*) ap;
  StatsItem const* b = (StatsItem const*) bp;
  int res = isGreater (a, b);
  if (res == 0)
    res = a->curpos - b->curpos;
  if (sortUp)
    res = -res;
  return res;
}

static char const laneTypes[][32] = {"Jungle", "Solo", "Dual", "3 or more", "AFK"};
static char const resTypes[][32] = {"Won", "Lost", "?"};

void CPlayerStatsDlg::refill ()
{
  games.DeleteAllItems ();
  for (int i = 0; i < items.getSize (); i++)
    items[i].curpos = sortUp ? -i : i;
  qsort (items.ptr (), items.getSize (), sizeof (StatsItem), itemCompare);
  for (int i = 0; i < items.getSize (); i++)
  {
    StatsItem* item = &items[i];
    DotaHero* hero = getHeroByPoint (item->hero);
    int pos = games.InsertItem (games.GetItemCount (), hero ? hero->name : "No hero",
      getImageIndex (hero ? hero->imgTag : "Empty"));
    games.SetItemData (pos, i);
    if (item->kills >= 0 && item->deaths >= 0)
      games.SetItemText (pos, COL_KD, mprintf ("%d-%d", item->kills, item->deaths));
    if (item->creeps >= 0 && item->denies >= 0)
      games.SetItemText (pos, COL_CS, mprintf ("%d/%d", item->creeps, item->denies));
    games.SetItemText (pos, COL_GOLD, mprintf ("%d", item->gold));
    games.SetItemText (pos, COL_AGOLD, mprintf ("%d", item->agold));
    if (item->lvl > 0)
      games.SetItemText (pos, COL_LVL, mprintf ("%d", item->lvl));
    if (item->alvl > 0)
      games.SetItemText (pos, COL_ALVL, mprintf ("%d", item->alvl));
    if (item->score[0] > 0 || item->score[1] > 0)
      games.SetItemText (pos, COL_SCORE, mprintf ("%d/%d", item->score[0], item->score[1]));
    if (item->left >= item->time)
      games.SetItemText (pos, COL_LEFT, "End");
    else
      games.SetItemText (pos, COL_LEFT, format_time (item->left, TIME_HOURS | TIME_SECONDS));
    games.SetItemText (pos, COL_LENGTH, format_time (item->time, TIME_HOURS | TIME_SECONDS));
    games.SetItemText (pos, COL_APM, mprintf ("%d", item->apm));
    LVSetItemText (games, pos, COL_NAME, item->name);
    games.SetItemText (pos, COL_DATE, CTime (item->date).Format ("%d/%m/%Y %H:%M"));
    games.SetItemText (pos, COL_LANE, laneTypes[item->lane]);
    games.SetItemText (pos, COL_RESULT, resTypes[item->res]);
  }
  for (int i = 0; i < NUM_COLS; i++)
    games.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
}

void CPlayerStatsDlg::setPlayer (wchar_t const* name)
{
  items.clear ();
  int needFlags = GC_NAME | GC_LENGTH | GC_COUNT | GC_NAMES | GC_HEROES |
    GC_STATS | GC_LEFT | GC_GOLD | GC_LEVEL | GC_LANE | GC_TEAM | GC_WIN | GC_APM;
  sortBy = COL_HERO;
  sortUp = false;
  for (int i = 0; i < gcsize; i++)
  {
    GameCache* gc = &gcache[i];
    if ((gc->flags & needFlags) != needFlags)
      continue;
    for (int j = 0; j < gc->count; j++)
    {
      if (!wcsicmp (gc->pname[j], name) && gc->phero[j])
      {
        StatsItem item;
        item.hero = gc->phero[j];
        item.kills = gc->pstats[j][0];
        item.deaths = gc->pstats[j][1];
        item.creeps = gc->pstats[j][2];
        item.denies = gc->pstats[j][3];
        if (gc->plane[j] == LANE_ROAMING)
          item.lane = L_JUNGLE;
        else if (gc->plane[j] == LANE_AFK)
          item.lane = L_AFK;
        else
        {
          int cnt = 0;
          for (int k = 0; k < gc->count; k++)
            if (gc->pteam[j] == gc->pteam[k] && gc->plane[j] == gc->plane[k])
              cnt++;
          if (cnt == 1)
            item.lane = L_SOLO;
          else if (cnt == 2)
            item.lane = L_DUAL;
          else
            item.lane = L_TRIPLE;
        }
        item.gold = gc->pgold[j];
        int agold = 0;
        for (int k = 0; k < gc->count; k++)
          agold += gc->pgold[k];
        item.agold = agold / gc->count;
        item.lvl = gc->plvl[j];
        int alvl = 0;
        int acc = 0;
        item.score[0] = 0;
        item.score[1] = 0;
        for (int k = 0; k < gc->count; k++)
        {
          if (gc->pstats[k][1] >= 0)
            item.score[gc->pteam[k] == gc->pteam[j] ? 1 : 0] += gc->pstats[k][1];
          if (gc->plvl[k] > 0)
          {
            alvl += gc->plvl[k];
            acc++;
          }
        }
        if (acc)
          item.alvl = alvl / acc;
        else
          item.alvl = 0;
        if (gc->win == WINNER_SENTINEL || gc->win == WINNER_GSENTINEL || gc->win == WINNER_PSENTINEL)
          item.res = (gc->pteam[j] == 0 ? R_WIN : R_LOSE);
        else if (gc->win == WINNER_SCOURGE || gc->win == WINNER_GSCOURGE || gc->win == WINNER_PSCOURGE)
          item.res = (gc->pteam[j] == 1 ? R_WIN : R_LOSE);
        else
          item.res = R_UNKNOWN;
        item.left = gc->pleft[j];
        item.time = gc->length;
        item.apm = gc->papm[j];
        wcscpy (item.name, gc->name);
        item.date = gc->mod;
        strcpy (item.path, gc->path);
        items.add (item);
      }
    }
  }
  refill ();
}

void CPlayerStatsDlg::OnLvnItemActivateStats(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int sel = games.GetNextItem (-1, LVNI_SELECTED);
  if (sel >= 0)
  {
    int id = (int) games.GetItemData (sel);
    dlg->showReplay (items[id].path);
  }
  *pResult = 0;
}

void CPlayerStatsDlg::OnHdnItemclickStats(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  if (phdr->iItem == sortBy)
    sortUp = !sortUp;
  else
  {
    sortBy = phdr->iItem;
    sortUp = false;
  }
  refill ();
  *pResult = 0;
}
