// GameInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "GameInfoDlg.h"
#include "WorkshopDlg.h"

#include "replay.h"

#include "ilib.h"
#include ".\gameinfodlg.h"
#include "dotareplaydlg.h"

extern char warReplay[256];
extern bool viewWindow;

// CGameInfoDlg dialog

IMPLEMENT_DYNAMIC(CGameInfoDlg, CDialog)
CGameInfoDlg::CGameInfoDlg(CDotAReplayDlg* dlg, CWnd* pParent /*=NULL*/)
  : CDialog(CGameInfoDlg::IDD, pParent)
{
  mdlg = dlg;
  fileName = "";
  w3g = NULL;
  bigmap.bmp = NULL;
  mappic = NULL;
  Create (IDD, pParent);
}

CGameInfoDlg::~CGameInfoDlg()
{
  delete mappic;
}

void CGameInfoDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGameInfoDlg, CDialog)
  ON_WM_DESTROY()
  ON_BN_CLICKED(IDC_WATCHREPLAY, OnBnClickedWatchreplay)
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_PLAYERINFO, OnLvnItemActivatePlayerinfo)
  ON_NOTIFY(LVN_KEYDOWN, IDC_PLAYERINFO, OnLvnKeydownPlayerinfo)
  ON_NOTIFY(NM_RCLICK, IDC_PLAYERINFO, OnNMRclickPlayerinfo)
  ON_BN_CLICKED(IDC_PLAYERSTATS, OnBnClickedPlayerstats)
  ON_WM_SIZE()
  ON_NOTIFY(HDN_TRACK, 0, OnHdnTrackPlayerinfo)
  //ON_WM_MOUSEMOVE()
  ON_STN_CLICKED(IDC_MAPIMG, OnStnClickedMapimg)
  ON_BN_CLICKED(IDC_WORKSHOP, OnBnClickedWorkshop)
  ON_WM_LBUTTONUP()
  ON_STN_DBLCLK(IDC_MAPIMG, OnStnDblclickMapimg)
  ON_BN_CLICKED(IDC_COPY_MATCHUP, &CGameInfoDlg::OnBnClickedCopyMatchup)
END_MESSAGE_MAP()

// CGameInfoDlg message handlers

#define INFO_VERSION      0
#define INFO_MAP          1
#define INFO_LOCATION     2
#define INFO_HOST         3
#define INFO_SAVER        4
#define INFO_LENGTH       5
#define INFO_RATIO        6
#define INFO_SCORE        7
#define INFO_WINNER       8
#define INFO_OBSERVERS    9
#define INFO_SAVED        10
#define INFO_NAME         11
#define INFO_MODE         12

#define PL_NAME           0
#define PL_LEVEL          1
#define PL_ITEM           2
#define PL_KILLS          3
#define PL_DEATHS         4
#define PL_ASSISTS        5
#define PL_CREEPS         6
#define PL_DENIES         7
#define PL_NEUTRALS       8
#define PL_LANE           9
#define PL_ITEMBUILD      10
#define PL_APM            11
#define PL_LEFT           12

#define PL_TEAM           1
#define PL_LAPM           2
#define PL_LLEFT          3

char infoText[][64] = {
  "Patch version",
  "Map name",
  "Map location",
  "Host name",
  "Replay saver",
  "Game length",
  "Players",
  "Game score (may be wrong)",
  "Winner",
  "Observers",
  "Replay saved",
  "Game name",
  "Game mode"
};

void CGameInfoDlg::insertMainInfo (int name, char const* val)
{
  int id = mainInfo.InsertItem (mainInfo.GetItemCount (), "");
  mainInfo.SetItemText (id, 1, infoText[name]);
  mainInfo.SetItemText (id, 2, val);
}
void CGameInfoDlg::insertMainInfo (int name, wchar_t const* val)
{
  int id = mainInfo.InsertItem (mainInfo.GetItemCount (), "");
  mainInfo.SetItemText (id, 1, infoText[name]);
  LVSetItemText (mainInfo, id, 2, val);
}

#define ID_PLM_BUILD        123
#define ID_PLM_ACTIONS      124
#define ID_PLM_COPY         125
#define ID_PLM_STATS        126
#define ID_PLM_NAME         127

BOOL CGameInfoDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  mainInfo.Attach (GetDlgItem (IDC_MAININFO)->m_hWnd);
  mainInfo.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  mainInfo.InsertColumn (0, "Dummy");
  mainInfo.InsertColumn (1, "Field", LVCFMT_RIGHT);
  mainInfo.InsertColumn (2, "Value");
  mainInfo.SetColumnWidth (0, 10);
  mainInfo.SetColumnWidth (1, 150);
  mainInfo.SetColumnWidth (2, LVSCW_AUTOSIZE_USEHEADER);

  plInfo.Attach (GetDlgItem (IDC_PLAYERINFO)->m_hWnd);
  plInfo.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  plInfo.EnableToolTips (TRUE);
  plInfo.InsertColumn (PL_NAME, "Name");
  plInfo.InsertColumn (PL_LEVEL, "Level", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_ITEM, "Gold", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_KILLS, "Kills", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_DEATHS, "Deaths", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_ASSISTS, "Assists", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_CREEPS, "Creeps", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_DENIES, "Denies", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_NEUTRALS, "Neutrals", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_LANE, "Lane");
  plInfo.InsertColumn (PL_ITEMBUILD, "Items");
  plInfo.InsertColumn (PL_APM, "APM", LVCFMT_RIGHT);
  plInfo.InsertColumn (PL_LEFT, "Left", LVCFMT_RIGHT);
  plInfo.SetColumnWidth (PL_NAME, 140);
  for (int i = PL_LEVEL; i <= PL_LEFT; i++)
    plInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
  plInfo.SetColumnWidth (PL_ITEMBUILD, 115);

  plInfo.SetImageList (getImageList (), LVSIL_SMALL);

  //mappic = new CMapPicture (this);

//  preview.Attach (GetDlgItem (IDC_MAPIMG)->m_hWnd);

  if (warReplay[0] == 0)
    SetDlgItemText (IDC_WATCHREPLAY, "Warcraft III not found");

  popupMenu.CreatePopupMenu ();
  MENUITEMINFO mii;
  memset (&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Show build";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_PLM_BUILD;
  popupMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Show actions";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_PLM_ACTIONS;
  popupMenu.InsertMenuItem (1, &mii, TRUE);

  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  popupMenu.InsertMenuItem (2, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Copy name";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_PLM_NAME;
  popupMenu.InsertMenuItem (3, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Copy stats";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_PLM_COPY;
  popupMenu.InsertMenuItem (4, &mii, TRUE);

  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  popupMenu.InsertMenuItem (5, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Find games";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_PLM_STATS;
  popupMenu.InsertMenuItem (6, &mii, TRUE);

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_MAININFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_MAININFO, SIDE_BOTTOM, PERCENT);
  loc.SetItemRelative (IDC_WATCHREPLAY, SIDE_LEFT, IDC_MAININFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_WATCHREPLAY, SIDE_RIGHT);
  loc.SetItemRelative (IDC_PLAYERSTATS, SIDE_LEFT, IDC_MAININFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_PLAYERSTATS, SIDE_RIGHT);
  loc.SetItemRelative (IDC_WORKSHOP, SIDE_LEFT, IDC_MAININFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_WORKSHOP, SIDE_RIGHT);
  loc.SetItemRelative (IDC_COPY_MATCHUP, SIDE_LEFT, IDC_MAININFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_COPY_MATCHUP, SIDE_RIGHT);
  loc.SetCenterPoint (POINT0, IDC_COPY_MATCHUP);
  loc.SetItemRelative (GetDlgItem (IDC_MAPIMG)->m_hWnd, SIDE_LEFT, POINT0, SIDE_LEFT);
  loc.SetItemRelative (GetDlgItem (IDC_MAPIMG)->m_hWnd, SIDE_RIGHT, POINT0, SIDE_RIGHT);
  loc.SetItemRelative (IDC_PLAYERINFO, SIDE_TOP, IDC_MAININFO, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_PLAYERINFO, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_PLAYERINFO, SIDE_BOTTOM);
  loc.Start ();

  icol.SetListCtrl (&plInfo);
  icol.Read ();

  return TRUE;
}
void CGameInfoDlg::OnDestroy ()
{
  mainInfo.Detach ();
  plInfo.Detach ();
//  preview.Detach ();
}

void CGameInfoDlg::setFailure (char const* msg)
{
  w3g = NULL;
  mainInfo.DeleteAllItems ();
  plInfo.DeleteAllItems ();
  plInfo.EnableWindow (FALSE);
  GetDlgItem (IDC_WATCHREPLAY)->EnableWindow (FALSE);
  GetDlgItem (IDC_PLAYERSTATS)->EnableWindow (FALSE);
  GetDlgItem (IDC_WORKSHOP)->EnableWindow (FALSE);
  GetDlgItem (IDC_COPY_MATCHUP)->EnableWindow (FALSE);
  mainInfo.SetColumnWidth (0, 5);
  mainInfo.SetColumnWidth (1, 5);
  mainInfo.SetColumnWidth (2, LVSCW_AUTOSIZE_USEHEADER);
  LVCOLUMN col;
  memset (&col, 0, sizeof col);
  col.mask = LVCF_FMT;
  col.fmt = LVCFMT_CENTER;
  mainInfo.SetColumn (2, &col);
  for (int i = 0; i < 6; i++)
    mainInfo.InsertItem (i, "");
  mainInfo.SetItemText (5, 2, msg);
}

extern char warPath[256];
void CGameInfoDlg::setReplay (W3GReplay* replay, char const* filename)
{
  w3g = replay;
  CStatic* img = (CStatic*) GetDlgItem (IDC_MAPIMG);
  RImageInfo info;
  bigmap.bmp = NULL;
  CBitmap* minimap = getImageBitmap (mprintf ("%s_mm", replay->game.map), &info);
  CBitmap* preview = getImageBitmap (mprintf ("%s_pr", replay->game.map), &info);
  if (minimap == NULL && preview == NULL)
  {
    MPQARCHIVE mpq = MPQOpen (mprintf ("%s%s", warPath, replay->game.map), MPQFILE_READ);
    if (mpq)
    {
      MPQFILE file = MPQOpenFile (mpq, "war3mapPreview.tga", MPQFILE_READ);
      if (file)
      {
        BLPImage* blp = LoadTGA (file);
        MPQCloseFile (file);
        if (blp)
        {
          if (blp->width > 128 || blp->height > 128)
            cpyImage (mprintf ("%s_pr_big", replay->game.map), blp);
          BLPPrepare (blp, 0, 128, 128);
          preview = addImage (mprintf ("%s_pr", replay->game.map), blp, &info);
        }
      }
      file = MPQOpenFile (mpq, "war3mapMap.blp", MPQFILE_READ);
      if (file)
      {
        BLPImage* blp = LoadBLP (file);
        for (int y = 0; y < blp->height; y++)
        {
          for (int x = 0; x < blp->width; x++)
          {
            uint32 p = blp->data[x + y * blp->width];
            blp->data[x + y * blp->width] = (p & 0xFF00FF00) |
              ((p & 0x000000FF) << 16) | ((p & 0x00FF0000) >> 16);
          }
        }
        MPQCloseFile (file);
        if (blp)
        {
          MPQFILE desc = MPQOpenFile (mpq, "war3map.mmp", MPQFILE_READ);
          if (desc)
          {
            BLPAddIcons (blp, desc);
            MPQCloseFile (desc);
          }
          if (blp->width > 128 || blp->height > 128)
            cpyImage (mprintf ("%s_mm_big", replay->game.map), blp);
          BLPPrepare (blp, 0, 128, 128);
          minimap = addImage (mprintf ("%s_mm", replay->game.map), blp, &info);
        }
      }
      MPQClose (mpq);
    }
  }
  if (preview)
  {
    getImageBitmap (mprintf ("%s_pr_big", replay->game.map), &bigmap);
    ((CStatic*) GetDlgItem (IDC_MAPIMG))->SetBitmap ((HBITMAP) preview->m_hObject);
    GetDlgItem (IDC_MAPIMG)->ShowWindow (SW_SHOW);
    picMode = 1;
  }
  else if (minimap)
  {
    getImageBitmap (mprintf ("%s_mm_big", replay->game.map), &bigmap);
    ((CStatic*) GetDlgItem (IDC_MAPIMG))->SetBitmap ((HBITMAP) minimap->m_hObject);
    GetDlgItem (IDC_MAPIMG)->ShowWindow (SW_SHOW);
    picMode = -1;
  }
  else
  {
    ((CStatic*) GetDlgItem (IDC_MAPIMG))->SetBitmap (NULL);
    GetDlgItem (IDC_MAPIMG)->ShowWindow (SW_HIDE);
    picMode = -1;
  }
  mainInfo.DeleteAllItems ();
  mainInfo.SetColumnWidth (0, 10);
  mainInfo.SetColumnWidth (1, 150);
  mainInfo.SetColumnWidth (2, LVSCW_AUTOSIZE_USEHEADER);
  LVCOLUMN col;
  memset (&col, 0, sizeof col);
  col.mask = LVCF_FMT;
  col.fmt = LVCFMT_LEFT;
  mainInfo.SetColumn (2, &col);
  plInfo.DeleteAllItems ();
  plInfo.EnableWindow (replay != NULL);
  fileName = "";
  GetDlgItem (IDC_WATCHREPLAY)->EnableWindow (FALSE);
  GetDlgItem (IDC_PLAYERSTATS)->EnableWindow (FALSE);
  GetDlgItem (IDC_WORKSHOP)->EnableWindow (FALSE);
  GetDlgItem (IDC_COPY_MATCHUP)->EnableWindow (FALSE);
  if (w3g == NULL) return;
  char fpath[256];
  char fname[256];
  char fext[256];
  _splitpath (w3g->game.map, NULL, fpath, fname, fext);
  strcat (fname, fext);
  if (w3g->saved_time != "")
    insertMainInfo (INFO_SAVED, w3g->saved_time);
  insertMainInfo (INFO_VERSION, formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
  insertMainInfo (INFO_MAP, fname);
  insertMainInfo (INFO_LOCATION, fpath);
  insertMainInfo (INFO_NAME, w3g->game.uname);
  if (w3g->dota.isDota)
    insertMainInfo (INFO_MODE, w3g->game.game_mode);
  insertMainInfo (INFO_HOST, w3g->game.creator);
  if (w3g->game.saver_id != 0)
    insertMainInfo (INFO_SAVER, w3g->players[w3g->game.saver_id].uname);
  else
    insertMainInfo (INFO_SAVER, "Unknown");
  if (w3g->real_time != END_TIME)
    insertMainInfo (INFO_LENGTH, mprintf ("%s (saver quit at %s)",
      format_time (w3g->time, TIME_HOURS | TIME_SECONDS),
      format_time (w3g->real_time, TIME_HOURS | TIME_SECONDS)));
  else
    insertMainInfo (INFO_LENGTH, format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
  GetDlgItem (IDC_WATCHREPLAY)->EnableWindow (warReplay[0] != 0 ? TRUE : FALSE);
  GetDlgItem (IDC_WORKSHOP)->EnableWindow (TRUE);
  GetDlgItem (IDC_COPY_MATCHUP)->EnableWindow (TRUE);
  if (filename)
    fileName = filename;
  for (int i = plInfo.GetHeaderCtrl ()->GetItemCount () - 1; i >= 0; i--)
    plInfo.DeleteColumn (i);
  if (w3g->dota.isDota)
  {
    plInfo.InsertColumn (PL_NAME, "Name");
    plInfo.InsertColumn (PL_LEVEL, "Level", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_ITEM, "Cost", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_KILLS, "Kills", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_DEATHS, "Deaths", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_ASSISTS, "Assists", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_CREEPS, "Creeps", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_DENIES, "Denies", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_NEUTRALS, "Neutrals", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_LANE, "Lane");
    plInfo.InsertColumn (PL_ITEMBUILD, "Items");
    plInfo.InsertColumn (PL_APM, "APM", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_LEFT, "Left", LVCFMT_RIGHT);

    GetDlgItem (IDC_PLAYERSTATS)->EnableWindow (TRUE);
    if (w3g->game.winner > 0)
    {
      insertMainInfo (INFO_RATIO, mprintf ("%dv%d", w3g->dota.numSentinel, w3g->dota.numScourge));
      insertMainInfo (INFO_SCORE, mprintf ("%d/%d", w3g->dota.sentinelKills, w3g->dota.scourgeKills));
    }
    if (w3g->game.winner == WINNER_UNKNOWN)
      insertMainInfo (INFO_WINNER, "Unknown");
    else if (w3g->game.winner == WINNER_SENTINEL)
      insertMainInfo (INFO_WINNER, "Sentinel");
    else if (w3g->game.winner == WINNER_SCOURGE)
      insertMainInfo (INFO_WINNER, "Scourge");
    else if (w3g->game.winner == WINNER_GSENTINEL)
      insertMainInfo (INFO_WINNER, "Sentinel (judging by chat)");
    else if (w3g->game.winner == WINNER_GSCOURGE)
      insertMainInfo (INFO_WINNER, "Scourge (judging by chat)");
    else if (w3g->game.winner == WINNER_PSENTINEL)
      insertMainInfo (INFO_WINNER, "Sentinel (judging by position)");
    else if (w3g->game.winner == WINNER_PSCOURGE)
      insertMainInfo (INFO_WINNER, "Scourge (judging by position)");
    if (w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
    {
      wchar_t* obs = wmprintf (L"");
      for (int i = 0; i < w3g->numPlayers; i++)
        if (w3g->players[w3g->pindex[i]].slot.color > 11 ||
            w3g->players[w3g->pindex[i]].slot.slot_status == 0)
        {
          if (obs[0] != 0) wcscat (obs, L", ");
          wcscat (obs, w3g->players[w3g->pindex[i]].uname);
        }
      insertMainInfo (INFO_OBSERVERS, obs);
    }

    if (w3g->dota.numSentinel)
    {
      plInfo.SetItemData (
        plInfo.InsertItem (plInfo.GetItemCount (), "Sentinel", getImageIndex ("RedBullet")),
        (DWORD_PTR) (((-1) << 24)));
      for (int i = 0; i < w3g->dota.numSentinel; i++)
        addPlayer (w3g, w3g->dota.sentinel[i]);
    }
    if (w3g->dota.numScourge)
    {
      plInfo.SetItemData (
        plInfo.InsertItem (plInfo.GetItemCount (), "Scourge", getImageIndex ("GreenBullet")),
        (DWORD_PTR) (((-1) << 24)));
      for (int i = 0; i < w3g->dota.numScourge; i++)
        addPlayer (w3g, w3g->dota.scourge[i]);
    }
    plInfo.SetColumnWidth (PL_NAME, 140);
    for (int i = PL_LEVEL; i <= PL_LEFT; i++)
    {
      if ((w3g->dota.major < 6 || w3g->dota.minor < 54) && (i == PL_ASSISTS || i == PL_NEUTRALS))
        plInfo.SetColumnWidth (i, 0);
      else
        plInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
    }
  }
  else
  {
    plInfo.InsertColumn (PL_NAME, "Name");
    plInfo.InsertColumn (PL_TEAM, "Team");
    plInfo.InsertColumn (PL_LAPM, "APM", LVCFMT_RIGHT);
    plInfo.InsertColumn (PL_LLEFT, "Left", LVCFMT_RIGHT);

    GetDlgItem (IDC_PLAYERSTATS)->EnableWindow (TRUE);
    if (w3g->game.lwinner < 0)
      insertMainInfo (INFO_WINNER, "Unknown");
    else if (w3g->game.wplayer)
      insertMainInfo (INFO_WINNER, w3g->players[w3g->game.wplayer].uname);
    else
      insertMainInfo (INFO_WINNER, mprintf ("Team %d", w3g->game.lwinner + 1));
    wchar_t* obs = wmprintf (L"");
    for (int i = 0; i < w3g->numPlayers; i++)
      if (w3g->players[w3g->pindex[i]].slot.color > 11 ||
          w3g->players[w3g->pindex[i]].slot.slot_status == 0)
      {
        if (obs[0] != 0) wcscat (obs, L", ");
        wcscat (obs, w3g->players[w3g->pindex[i]].uname);
      }
    if (obs[0])
      insertMainInfo (INFO_OBSERVERS, obs);

    for (int i = 0; i < w3g->numPlayers; i++)
      addLadderPlayer (w3g, w3g->pindex[i]);
    plInfo.SetColumnWidth (PL_NAME, 140);
    for (int i = PL_LEVEL; i <= PL_LEFT; i++)
      plInfo.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
    plInfo.SetColumnWidth (PL_ITEMBUILD, 115);
  }
  icol.Read ();
}

extern bool showEmptySlots;

void CGameInfoDlg::addPlayer (W3GReplay* w3g, int id)
{
  if (w3g->players[id].hero != NULL && w3g->time)
  {
    int i = LVInsertItem (plInfo, plInfo.GetItemCount (), w3g->players[id].uname,
      getImageIndex (getHero (w3g->players[id].hero->id)->imgTag));
    plInfo.SetItemData (i, (DWORD_PTR) ((id << 24) | getLightColor (w3g->players[id].slot.color)));
    plInfo.SetItemText (i, PL_LEVEL, mprintf ("%d", w3g->players[id].lCount));
    plInfo.SetItemText (i, PL_ITEM, mprintf ("%d", w3g->players[id].itemCost));
    if (w3g->dota.endgame)
    {
      plInfo.SetItemText (i, PL_KILLS, mprintf ("%d", w3g->players[id].stats[0]));
      plInfo.SetItemText (i, PL_DEATHS, mprintf ("%d", w3g->players[id].stats[1]));
      plInfo.SetItemText (i, PL_CREEPS, mprintf ("%d", w3g->players[id].stats[2]));
      plInfo.SetItemText (i, PL_DENIES, mprintf ("%d", w3g->players[id].stats[3]));
      if (w3g->dota.major >= 6 && w3g->dota.minor >= 54)
      {
        plInfo.SetItemText (i, PL_ASSISTS, mprintf ("%d", w3g->players[id].stats[STAT_ASSISTS]));
        plInfo.SetItemText (i, PL_NEUTRALS, mprintf ("%d", w3g->players[id].stats[STAT_NEUTRALS]));
      }
      char itembuf[256];
      int itembufSize = 0;
      for (int it = 0; it < 6; it++)
      {
        int iid = w3g->players[id].finalItems[it];
        if (iid)
        {
          DotaItem* item = getItem (iid);
          int index = (item ? getImageIndex (item->imgTag, "Unknown") : getImageIndex ("Unknown"));
          sprintf (itembuf + itembufSize, "$%d$", index);
          while (itembuf[itembufSize])
            itembufSize++;
        }
        else if (showEmptySlots)
        {
          int index = getImageIndex ("EmptySlot", "Unknown");
          sprintf (itembuf + itembufSize, "$%d$", index);
          while (itembuf[itembufSize])
            itembufSize++;
        }
      }
      itembuf[itembufSize] = 0;
      plInfo.SetItemText (i, PL_ITEMBUILD, itembuf);
    }
    plInfo.SetItemText (i, PL_LANE, laneName[w3g->players[id].lane]);
    if (w3g->players[id].time)
      plInfo.SetItemText (i, PL_APM, mprintf ("%d", w3g->players[id].actions * 60000 / w3g->players[id].time));
    if (w3g->players[id].time >= w3g->time || id >= 0x80)
      plInfo.SetItemText (i, PL_LEFT, "End");
    else
      plInfo.SetItemText (i, PL_LEFT, format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
  }
  else
    plInfo.SetItemData (
      LVInsertItem (plInfo, plInfo.GetItemCount (), w3g->players[id].uname, getImageIndex ("Empty")),
      (DWORD_PTR) ((id << 24) | RGB (255, 255, 255)));
}
void CGameInfoDlg::addLadderPlayer (W3GReplay* w3g, int id)
{
  int i = LVInsertItem (plInfo, plInfo.GetItemCount (), w3g->players[id].uname,
    getImageIndex (raceImage[w3g->players[id].race]));
  plInfo.SetItemData (i, (DWORD_PTR) ((id << 24) | getLightColor (w3g->players[id].slot.color)));
  plInfo.SetItemText (i, PL_TEAM, mprintf ("Team %d", w3g->players[id].slot.team + 1));
  if (w3g->players[id].time)
    plInfo.SetItemText (i, PL_LAPM, mprintf ("%d", w3g->players[id].actions * 60000 / w3g->players[id].time));
  if (w3g->players[id].time >= w3g->time || id >= 0x80)
    plInfo.SetItemText (i, PL_LLEFT, "End");
  else
    plInfo.SetItemText (i, PL_LLEFT, format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
}

void CGameInfoDlg::OnBnClickedWatchreplay()
{
  if (fileName != "")
  {
    char cmd[256];
    sprintf (cmd, warReplay, (char const*) fileName);
    if (viewWindow)
      strcat (cmd, " -window");
    STARTUPINFO info;
    GetStartupInfo (&info);
    PROCESS_INFORMATION pi;
    CreateProcess (NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &info, &pi);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
  }
}

void CGameInfoDlg::OnLvnItemActivatePlayerinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int sel = plInfo.GetNextItem (-1, LVNI_SELECTED);
  if (sel >= 0)
  {
    int id = ((int) plInfo.GetItemData (sel) >> 24) & 0xFF;
    if (id != 255)
      mdlg->showPlayerInfo (id);
  }
  *pResult = 0;
}

void CGameInfoDlg::copyLine (int id)
{
  wchar_t buf[1024];
  if (w3g->dota.endgame)
    swprintf (buf, L"%s (%s), level %d, build cost %d, KDA: %d-%d-%d, CS: %d/%d, lane: %s, APM: %d, left: %s",
      w3g->players[id].uname, makeucd (w3g->players[id].hero ? getHero (w3g->players[id].hero->id)->name : "No Hero"),
      w3g->players[id].hero ? w3g->players[id].hero->level : 0,
      w3g->players[id].itemCost, w3g->players[id].stats[0], w3g->players[id].stats[1], w3g->players[id].stats[4],
      w3g->players[id].stats[2], w3g->players[id].stats[3], makeucd (laneName[w3g->players[id].lane]),
      w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0, makeucd (w3g->players[id].time >= w3g->time
      ? "End" : format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS)));
  else
    swprintf (buf, L"%s (%s), level %d, build cost %d, lane: %s, APM: %d, left: %s",
      w3g->players[id].uname, makeucd (w3g->players[id].hero ? getHero (w3g->players[id].hero->id)->name : "No Hero"),
      w3g->players[id].hero ? w3g->players[id].hero->level : 0,
      w3g->players[id].itemCost, makeucd (laneName[w3g->players[id].lane]),
      w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0, makeucd (w3g->players[id].time >= w3g->time
      ? "End" : format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS)));

  if (!OpenClipboard ())
    return;
  EmptyClipboard ();

  int sz = (int) wcslen (buf);
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    return;
  }

  LPTSTR copy = (LPTSTR) GlobalLock (handle);
  memcpy (copy, buf, sz * 2 + 2);
  GlobalUnlock (handle);

  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
}

void CGameInfoDlg::copyName (int id)
{
  if (!OpenClipboard ())
    return;
  EmptyClipboard ();

  int sz = (int) wcslen (w3g->players[id].uname);
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    return;
  }

  LPTSTR copy = (LPTSTR) GlobalLock (handle);
  memcpy (copy, w3g->players[id].uname, sz * 2 + 2);
  GlobalUnlock (handle);

  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
}

void CGameInfoDlg::OnLvnKeydownPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
  int sel = plInfo.GetNextItem (-1, LVNI_SELECTED);
  if (sel >= 0 && w3g && w3g->dota.isDota && w3g->time &&
      pLVKeyDow->wVKey == 'C' && GetAsyncKeyState (VK_CONTROL))
  {
    int id = ((int) plInfo.GetItemData (sel) >> 24) & 0xFF;
    if (id < 0x80)
      copyLine (id);
  }
  *pResult = 0;
}

void CGameInfoDlg::OnNMRclickPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  CPoint pt;
  GetCursorPos (&pt);
  int sel = plInfo.GetNextItem (-1, LVNI_SELECTED);
  if (sel < 0)
    return;
  int id = ((int) plInfo.GetItemData (sel) >> 24) & 0xFF;
  if (id == 255)
    return;
  popupMenu.EnableMenuItem (ID_PLM_STATS, (id < 0x80 ? MF_ENABLED : MF_GRAYED));
  int res = popupMenu.TrackPopupMenuEx (TPM_HORIZONTAL | TPM_LEFTALIGN | TPM_RETURNCMD,
    pt.x, pt.y, this, NULL);
  switch (res)
  {
  case ID_PLM_BUILD:
    mdlg->showPlayerInfo (id);
    break;
  case ID_PLM_ACTIONS:
    mdlg->showPlayerActions (id);
    break;
  case ID_PLM_COPY:
    copyLine (id);
    break;
  case ID_PLM_STATS:
    if (id < 0x80)
      mdlg->showPlayerStats (w3g->players[id].name);
    break;
  case ID_PLM_NAME:
    copyName (id);
    break;
  }
  *pResult = 0;
}

void CGameInfoDlg::OnBnClickedPlayerstats()
{
  if (w3g)
    mdlg->showGamePlayers (w3g);
}

void CGameInfoDlg::OnSize (UINT nType, int cx, int cy)
{
  if (nType != SIZE_MINIMIZED)
  {
    loc.Update ();
    icol.Write ();
  }
}

void CGameInfoDlg::OnHdnTrackPlayerinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
  icol.Read ();
  *pResult = 0;
}

BEGIN_MESSAGE_MAP(CMapPicture, CWnd)
  ON_WM_PAINT()
END_MESSAGE_MAP()

CMapPicture::CMapPicture (CRect const& rc, CBitmap* bitmap, CWnd* parent)
{
  bmp = bitmap;
  CreateEx (WS_EX_TOOLWINDOW | WS_EX_TOPMOST, AfxRegisterWndClass (CS_SAVEBITS | CS_DROPSHADOW, NULL,
    (HBRUSH) GetStockObject (BLACK_BRUSH), NULL), NULL, WS_POPUP, rc.left, rc.top, rc.Width (), rc.Height (),
    NULL, NULL);
  SetOwner (parent);
}
void CMapPicture::OnPaint ()
{
  CPaintDC dc (this);
  CDC other;
  other.CreateCompatibleDC (&dc);
  other.SelectObject (bmp);
  CRect rc;
  GetClientRect (rc);
  dc.BitBlt (0, 0, rc.right, rc.bottom, &other, 0, 0, SRCCOPY);
  other.DeleteDC ();
}

void CGameInfoDlg::OnStnClickedMapimg()
{
  if (bigmap.bmp && mappic == NULL)
  {
    SetCapture ();
    CRect rc;
    GetDlgItem (IDC_MAPIMG)->GetClientRect (rc);
    GetDlgItem (IDC_MAPIMG)->ClientToScreen (rc);
    rc.left = (rc.left + rc.right - bigmap.width) / 2;
    rc.top = (rc.top + rc.bottom - bigmap.height) / 2;
    rc.right = rc.left + bigmap.width;
    rc.bottom = rc.top + bigmap.height;
    mappic = new CMapPicture (rc, bigmap.bmp, this);
    mappic->ShowWindow (SW_SHOWNA);
  }
}
void CGameInfoDlg::OnLButtonUp (UINT nFlags, CPoint point)
{
  if (mappic)
  {
    mappic->DestroyWindow ();
    delete mappic;
    mappic = NULL;
    ReleaseCapture ();
  }
}

void CGameInfoDlg::OnBnClickedWorkshop()
{
  if (w3g)
  {
    CWorkshopDlg dlg (w3g);
    if (dlg.DoModal () == IDOK)
      GetDlgItem (IDC_WORKSHOP)->EnableWindow (FALSE);
  }
}

void CGameInfoDlg::OnStnDblclickMapimg()
{
  if (picMode == 1)
  {
    CBitmap* bmp = getImageBitmap (mprintf ("%s_mm", w3g->game.map));
    if (bmp)
    {
      getImageBitmap (mprintf ("%s_mm_big", w3g->game.map), &bigmap);
      ((CStatic*) GetDlgItem (IDC_MAPIMG))->SetBitmap ((HBITMAP) bmp->m_hObject);
      picMode = 0;
    }
  }
  else if (picMode == 0)
  {
    CBitmap* bmp = getImageBitmap (mprintf ("%s_pr", w3g->game.map));
    if (bmp)
    {
      getImageBitmap (mprintf ("%s_pr_big", w3g->game.map), &bigmap);
      ((CStatic*) GetDlgItem (IDC_MAPIMG))->SetBitmap ((HBITMAP) bmp->m_hObject);
      picMode = 1;
    }
  }
}

void CGameInfoDlg::OnBnClickedCopyMatchup()
{
  static wchar_t buf[4096];
  int pid[16];
  int pteam[16];
  int pclr[16];
  int pcount = 0;
  for (int i = 0; i < w3g->numPlayers; i++)
  {
    if (w3g->players[w3g->pindex[i]].slot.color < 12 && w3g->players[w3g->pindex[i]].slot.slot_status)
    {
      pid[pcount] = w3g->pindex[i];
      pclr[pcount] = w3g->players[w3g->pindex[i]].slot.color;
      pteam[pcount++] = w3g->players[w3g->pindex[i]].slot.team;
    }
  }
  for (int i = 0; i < pcount; i++)
  {
    for (int j = 0; j < pcount - 1; j++)
    {
      if (pclr[j] > pclr[j + 1])
      {
        int tmp = pid[j];
        pid[j] = pid[j + 1];
        pid[j + 1] = tmp;
        tmp = pteam[j];
        pteam[j] = pteam[j + 1];
        pteam[j + 1] = tmp;
        tmp = pclr[j];
        pclr[j] = pclr[j + 1];
        pclr[j + 1] = tmp;
      }
    }
  }
  bool first = true;
  for (int i = 0; i < pcount; i++)
  {
    if (i > 0 && pteam[i - 1] != pteam[i])
    {
      wcscat (buf, L" VS ");
      first = true;
    }
    if (first)
      first = false;
    else
      wcscat (buf, L", ");
    wcscat (buf, w3g->players[pid[i]].uname);
    if (w3g->dota.isDota)
    {
      DotaHero* hero = getHero (w3g->players[pid[i]].hero->id);
      if (hero)
      {
        char const* abbr = getAbbreviation (hero->point);
        if (abbr)
          wcscat (buf, makeucd (mprintf (" (%s)", abbr)));
        else
          wcscat (buf, makeucd (mprintf (" (%s)", hero->abbr)));
      }
    }
  }

  if (!OpenClipboard ())
    return;
  EmptyClipboard ();

  int sz = (int) wcslen (buf);
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    return;
  }

  LPTSTR copy = (LPTSTR) GlobalLock (handle);
  memcpy (copy, buf, sz * 2 + 2);
  GlobalUnlock (handle);

  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
}
