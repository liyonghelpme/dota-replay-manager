// PresentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "PresentDlg.h"

#include "replay.h"
#include ".\presentdlg.h"
#include "registry.h"
#include "sparser.h"

// CPresentDlg dialog

IMPLEMENT_DYNAMIC(CPresentDlg, CDialog)
CPresentDlg::CPresentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPresentDlg::IDD, pParent)
{
  Create (IDD, pParent);
}

CPresentDlg::~CPresentDlg()
{
}

void CPresentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPresentDlg, CDialog)
  ON_WM_DESTROY()
  ON_BN_CLICKED(IDC_USE_ICONS, OnChange)
  ON_CBN_SELCHANGE(IDC_FORUM_ICONS, OnChange)
  ON_BN_CLICKED(IDC_INFO_DATE, OnChange)
  ON_BN_CLICKED(IDC_INFO_PATCH, OnChange)
  ON_BN_CLICKED(IDC_INFO_MAP, OnChange)
  ON_BN_CLICKED(IDC_INFO_NAME, OnChange)
  ON_BN_CLICKED(IDC_INFO_MODE, OnChange)
  ON_BN_CLICKED(IDC_INFO_HOST, OnChange)
  ON_BN_CLICKED(IDC_INFO_SAVER, OnChange)
  ON_BN_CLICKED(IDC_INFO_LENGTH, OnChange)
  ON_BN_CLICKED(IDC_INFO_PLAYERS, OnChange)
  ON_BN_CLICKED(IDC_INFO_SCORE, OnChange)
  ON_BN_CLICKED(IDC_INFO_WINNER, OnChange)
  ON_BN_CLICKED(IDC_INFO_OBSERVERS, OnChange)
  ON_BN_CLICKED(IDC_PLAYERLIST, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_COLOR, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_LEVEL, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_HEROES, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_CREEPS, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_GOLD, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_LANE, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_APM, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_LEFT, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_ITEMS, OnChange)
  ON_BN_CLICKED(IDC_PLAYER_GROUP, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_SKILL, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_SKTIME, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_ITEM, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_ITTIME, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_ITCOST, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_ACTION, OnChange)
  ON_BN_CLICKED(IDC_DETAIL_GROUPS, OnChange)
  ON_CBN_SELCHANGE(IDC_GEN_MODE, OnCbnSelchangeGenMode)
  ON_CBN_SELCHANGE(IDC_PRESET, OnCbnSelchangePreset)
  ON_CBN_EDITCHANGE(IDC_PRESET, OnCbnEditchangePreset)
  ON_BN_CLICKED(IDC_SAVEPR, OnBnClickedSavepr)
  ON_BN_CLICKED(IDC_DELPR, OnBnClickedDelpr)
  ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
  ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
  ON_BN_CLICKED(IDC_PREVIEW, OnBnClickedPreview)
  ON_WM_SIZE()
  ON_CBN_SELCHANGE(IDC_FUNCTION, &CPresentDlg::OnCbnSelchangeFunction)
END_MESSAGE_MAP()

#define PRESET_ICONS            0x00000001
#define PRESET_DATE             0x00000002
#define PRESET_PATCH            0x00000004
#define PRESET_MAP              0x00000008
#define PRESET_NAME             0x00000010
#define PRESET_HOST             0x00000020
#define PRESET_SAVER            0x00000040
#define PRESET_LENGTH           0x00000080
#define PRESET_PLAYERS          0x00000100
#define PRESET_SCORE            0x00000200
#define PRESET_WINNER           0x00000400
#define PRESET_OBSERVERS        0x00000800
#define PRESET_PLIST            0x00001000
#define PRESET_PCOLORS          0x00002000
#define PRESET_PLEVEL           0x00004000
#define PRESET_PKD              0x00008000
#define PRESET_PCS              0x00010000
#define PRESET_PGOLD            0x00020000
#define PRESET_PLANE            0x00040000
#define PRESET_PAPM             0x00080000
#define PRESET_PLEFT            0x00100000
#define PRESET_PITEMS           0x00200000
#define PRESET_PGROUP           0x00400000
#define PRESET_SKILLS           0x00800000
#define PRESET_STIME            0x01000000
#define PRESET_ITEMS            0x02000000
#define PRESET_ITIME            0x04000000
#define PRESET_ICOST            0x08000000
#define PRESET_ACTIONS          0x10000000
#define PRESET_GROUPS           0x20000000
#define PRESET_MODE_MASK        0xC0000000
#define PRESET_MODE_TEXT        0x00000000
#define PRESET_MODE_BBCODE      0x40000000
#define PRESET_MODE_HTML        0x80000000
#define PRESET_MODE_BBHTML      0xC0000000
#define PRESET_MODE_SCRIPT      0xC0000000

#define PRESET_EX_MODE          0x00000001
#define PRESET_EX_FORUMICONS    0x00000002
#define PRESET_EX_PLAYDOTAICONS 0x00000004

#define PRESET_INFO_MASK        0x00000FFE
#define PRESET_DETAIL_MASK      0x3F800000

const unsigned long presetMinimal = PRESET_MODE_TEXT |
  PRESET_DATE | PRESET_MAP | PRESET_LENGTH | PRESET_PLIST | PRESET_PLANE;
const unsigned long presetForum = PRESET_MODE_BBCODE |
  PRESET_ICONS | PRESET_PATCH | PRESET_DATE | PRESET_MAP | PRESET_LENGTH | PRESET_WINNER |
  PRESET_PLIST | PRESET_PCOLORS | PRESET_PLEVEL | PRESET_PLANE | PRESET_PAPM | PRESET_PLEFT |
  PRESET_PITEMS;
const unsigned long presetFull = PRESET_MODE_HTML | (0xFFFFFFFF & (~PRESET_MODE_MASK) &
  (~PRESET_PLANE));
const unsigned long presetMinimalEx = PRESET_EX_MODE;
const unsigned long presetForumEx = PRESET_EX_MODE | PRESET_EX_PLAYDOTAICONS;
const unsigned long presetFullEx = PRESET_EX_MODE;

// CPresentDlg message handlers

struct PresetInfo
{
  unsigned long flags;
  unsigned long flagsEx;
  PresetInfo () {}
  PresetInfo (unsigned long f, unsigned long e) {flags = f; flagsEx = e;}
};
Array<PresetInfo> _presets;

BOOL CPresentDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  preset.Attach (GetDlgItem (IDC_PRESET)->m_hWnd);
  mode.Attach (GetDlgItem (IDC_GEN_MODE)->m_hWnd);
  func.Attach (GetDlgItem (IDC_FUNCTION)->m_hWnd);
  //if (scriptEnv)
  //{
  //  for (int i = 0; i < scriptEnv->getNumFunctions (); i++)
  //    func.InsertString (i, scriptEnv->getFunction (i));
  //}

  preset.SetItemData (preset.InsertString (0, "Minimal"),
    _presets.add (PresetInfo (presetMinimal, presetMinimalEx)));
  preset.SetItemData (preset.InsertString (0, "Forum replay"),
    _presets.add (PresetInfo (presetForum, presetForumEx)));
  preset.SetItemData (preset.InsertString (0, "Full"),
    _presets.add (PresetInfo (presetFull, presetFullEx)));

  int count = reg.readInt ("numPresets", 0);
  for (int i = 0; i < count; i++)
  {
    int cur = reg.readInt (mprintf ("preset%dVal", i), presetMinimal);
    int curEx = reg.readInt (mprintf ("preset%dValEx", i), presetMinimalEx);
    char name[256];
    reg.readString (mprintf ("preset%dName", i), name);
    preset.SetItemData (preset.InsertString (0, name),
      _presets.add (PresetInfo (cur, curEx)));
  }

  result.Attach (GetDlgItem (IDC_RESULT)->m_hWnd);
  CHARFORMAT cf;
  result.GetDefaultCharFormat (cf);
  cf.bPitchAndFamily = (cf.bPitchAndFamily & 0x0F) | FF_MODERN;
  strcpy (cf.szFaceName, "Courier New");
  result.SetDefaultCharFormat (cf);

  setReplayInfo (reg.readInt ("curPreset", presetForum), reg.readInt ("curPresetEx", presetForumEx));

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_PRESET, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SAVEPR, SIDE_LEFT, IDC_SAVEPR, SIDE_RIGHT);
  loc.SetItemRelative (IDC_DELPR, SIDE_LEFT, IDC_DELPR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP1, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP2, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP3, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP4, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP5, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_COMMENT, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEP6, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_UPDATE, SIDE_RIGHT, PERCENT);
  loc.SetItemRelative (IDC_COPY, SIDE_LEFT, IDC_UPDATE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_COPY, SIDE_RIGHT, PERCENT);
  loc.SetItemRelative (IDC_PREVIEW, SIDE_LEFT, IDC_COPY, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_PREVIEW, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_RESULT, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_RESULT, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}

void CPresentDlg::OnDestroy ()
{
  preset.Detach ();
  mode.Detach ();
  func.Detach ();
  result.Detach ();
}

const int playerBoxes[] = {IDC_DETAIL_P1, IDC_DETAIL_P2, IDC_DETAIL_P3, IDC_DETAIL_P4, IDC_DETAIL_P5,
                           IDC_DETAIL_P6, IDC_DETAIL_P7, IDC_DETAIL_P8, IDC_DETAIL_P9, IDC_DETAIL_P10};
const char playerNames[][32] = {"Blue", "Teal", "Purple", "Yellow", "Orange",
                                "Pink", "Gray", "Light Blue", "Dark Green", "Brown"};

void CPresentDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
  int countSent = 0;
  int countScourge = 0;
  if (w3g && w3g->dota.isDota)
  {
    for (int i = 0; i < w3g->dota.numSentinel; i++)
    {
      GetDlgItem (playerBoxes[i])->EnableWindow (TRUE);
      CheckDlgButton (playerBoxes[i], FALSE);
      SetDlgItemText (playerBoxes[i], w3g->players[w3g->dota.sentinel[i]].name);
      countSent++;
    }
    for (int i = 0; i < w3g->dota.numScourge; i++)
    {
      GetDlgItem (playerBoxes[i + 5])->EnableWindow (TRUE);
      CheckDlgButton (playerBoxes[i + 5], FALSE);
      SetDlgItemText (playerBoxes[i + 5], w3g->players[w3g->dota.scourge[i]].name);
      countScourge++;
    }
  }
  for (int i = countSent; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i], FALSE);
    GetDlgItem (playerBoxes[i])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i], playerNames[i]);
  }
  for (int i = countScourge; i < 5; i++)
  {
    CheckDlgButton (playerBoxes[i + 5], FALSE);
    GetDlgItem (playerBoxes[i + 5])->EnableWindow (FALSE);
    SetDlgItemText (playerBoxes[i + 5], playerNames[i + 5]);
  }
}

void CPresentDlg::setReplayInfo (unsigned long flags, unsigned long flagsEx)
{
  bool found = false;
  for (int i = 0; i < preset.GetCount (); i++)
  {
    if (_presets[(int) preset.GetItemData (i)].flags == flags &&
        _presets[(int) preset.GetItemData (i)].flagsEx == flagsEx)
    {
      preset.SetCurSel (i);
      found = true;
    }
  }
  if (!found)
    preset.SetWindowText ("Custom");
  if (((flags >> 30) & 3) == 3)
  {
    mode.SetCurSel (3);
    func.SetCurSel (flags & ((1 << 30) - 1));
    flags = 0;
    flagsEx = 0;
  }
  else
    mode.SetCurSel ((flags >> 30) & 3);
  CheckDlgButton (IDC_USE_ICONS, flags & PRESET_ICONS);
  if (flagsEx & PRESET_EX_FORUMICONS)
    ((CComboBox*) GetDlgItem (IDC_FORUM_ICONS))->SetCurSel (1);
  else if (flagsEx & PRESET_EX_PLAYDOTAICONS)
    ((CComboBox*) GetDlgItem (IDC_FORUM_ICONS))->SetCurSel (2);
  else
    ((CComboBox*) GetDlgItem (IDC_FORUM_ICONS))->SetCurSel (0);
  CheckDlgButton (IDC_INFO_DATE, flags & PRESET_DATE);
  CheckDlgButton (IDC_INFO_PATCH, flags & PRESET_PATCH);
  CheckDlgButton (IDC_INFO_MAP, flags & PRESET_MAP);
  CheckDlgButton (IDC_INFO_NAME, flags & PRESET_NAME);
  CheckDlgButton (IDC_INFO_MODE, flagsEx & PRESET_EX_MODE);
  CheckDlgButton (IDC_INFO_HOST, flags & PRESET_HOST);
  CheckDlgButton (IDC_INFO_SAVER, flags & PRESET_SAVER);
  CheckDlgButton (IDC_INFO_LENGTH, flags & PRESET_LENGTH);
  CheckDlgButton (IDC_INFO_PLAYERS, flags & PRESET_PLAYERS);
  CheckDlgButton (IDC_INFO_SCORE, flags & PRESET_SCORE);
  CheckDlgButton (IDC_INFO_WINNER, flags & PRESET_WINNER);
  CheckDlgButton (IDC_INFO_OBSERVERS, flags & PRESET_OBSERVERS);
  CheckDlgButton (IDC_PLAYERLIST, flags & PRESET_PLIST);
  CheckDlgButton (IDC_PLAYER_COLOR, flags & PRESET_PCOLORS);
  CheckDlgButton (IDC_PLAYER_LEVEL, flags & PRESET_PLEVEL);
  CheckDlgButton (IDC_PLAYER_HEROES, flags & PRESET_PKD);
  CheckDlgButton (IDC_PLAYER_CREEPS, flags & PRESET_PCS);
  CheckDlgButton (IDC_PLAYER_GOLD, flags & PRESET_PGOLD);
  CheckDlgButton (IDC_PLAYER_LANE, flags & PRESET_PLANE);
  CheckDlgButton (IDC_PLAYER_APM, flags & PRESET_PAPM);
  CheckDlgButton (IDC_PLAYER_LEFT, flags & PRESET_PLEFT);
  CheckDlgButton (IDC_PLAYER_ITEMS, flags & PRESET_PITEMS);
  CheckDlgButton (IDC_PLAYER_GROUP, flags & PRESET_PGROUP);
  CheckDlgButton (IDC_DETAIL_SKILL, flags & PRESET_SKILLS);
  CheckDlgButton (IDC_DETAIL_SKTIME, flags & PRESET_STIME);
  CheckDlgButton (IDC_DETAIL_ITEM, flags & PRESET_ITEMS);
  CheckDlgButton (IDC_DETAIL_ITTIME, flags & PRESET_ITIME);
  CheckDlgButton (IDC_DETAIL_ITCOST, flags & PRESET_ICOST);
  CheckDlgButton (IDC_DETAIL_ACTION, flags & PRESET_ACTIONS);
  CheckDlgButton (IDC_DETAIL_GROUPS, flags & PRESET_GROUPS);
  updateEnables (true);
  reg.writeInt ("curPreset", flags);
  OnCbnEditchangePreset ();
}

unsigned long CPresentDlg::getReplayInfo ()
{
  unsigned long info = 0;
  if (mode.GetCurSel () == 3)
    return func.GetCurSel () | (3 << 30);
  if (IsDlgButtonChecked (IDC_USE_ICONS)) info |= PRESET_ICONS;
  if (IsDlgButtonChecked (IDC_INFO_DATE)) info |= PRESET_DATE;
  if (IsDlgButtonChecked (IDC_INFO_PATCH)) info |= PRESET_PATCH;
  if (IsDlgButtonChecked (IDC_INFO_MAP)) info |= PRESET_MAP;
  if (IsDlgButtonChecked (IDC_INFO_NAME)) info |= PRESET_NAME;
  if (IsDlgButtonChecked (IDC_INFO_HOST)) info |= PRESET_HOST;
  if (IsDlgButtonChecked (IDC_INFO_SAVER)) info |= PRESET_SAVER;
  if (IsDlgButtonChecked (IDC_INFO_LENGTH)) info |= PRESET_LENGTH;
  if (IsDlgButtonChecked (IDC_INFO_PLAYERS)) info |= PRESET_PLAYERS;
  if (IsDlgButtonChecked (IDC_INFO_SCORE)) info |= PRESET_SCORE;
  if (IsDlgButtonChecked (IDC_INFO_WINNER)) info |= PRESET_WINNER;
  if (IsDlgButtonChecked (IDC_INFO_OBSERVERS)) info |= PRESET_OBSERVERS;
  if (IsDlgButtonChecked (IDC_PLAYERLIST)) info |= PRESET_PLIST;
  if (IsDlgButtonChecked (IDC_PLAYER_COLOR)) info |= PRESET_PCOLORS;
  if (IsDlgButtonChecked (IDC_PLAYER_LEVEL)) info |= PRESET_PLEVEL;
  if (IsDlgButtonChecked (IDC_PLAYER_HEROES)) info |= PRESET_PKD;
  if (IsDlgButtonChecked (IDC_PLAYER_CREEPS)) info |= PRESET_PCS;
  if (IsDlgButtonChecked (IDC_PLAYER_GOLD)) info |= PRESET_PGOLD;
  if (IsDlgButtonChecked (IDC_PLAYER_LANE)) info |= PRESET_PLANE;
  if (IsDlgButtonChecked (IDC_PLAYER_APM)) info |= PRESET_PAPM;
  if (IsDlgButtonChecked (IDC_PLAYER_LEFT)) info |= PRESET_PLEFT;
  if (IsDlgButtonChecked (IDC_PLAYER_ITEMS)) info |= PRESET_PITEMS;
  if (IsDlgButtonChecked (IDC_PLAYER_GROUP)) info |= PRESET_PGROUP;
  if (IsDlgButtonChecked (IDC_DETAIL_SKILL)) info |= PRESET_SKILLS;
  if (IsDlgButtonChecked (IDC_DETAIL_SKTIME)) info |= PRESET_STIME;
  if (IsDlgButtonChecked (IDC_DETAIL_ITEM)) info |= PRESET_ITEMS;
  if (IsDlgButtonChecked (IDC_DETAIL_ITTIME)) info |= PRESET_ITIME;
  if (IsDlgButtonChecked (IDC_DETAIL_ITCOST)) info |= PRESET_ICOST;
  if (IsDlgButtonChecked (IDC_DETAIL_ACTION)) info |= PRESET_ACTIONS;
  if (IsDlgButtonChecked (IDC_DETAIL_GROUPS)) info |= PRESET_GROUPS;
  return info | (mode.GetCurSel () << 30);
}

unsigned long CPresentDlg::getReplayInfoEx ()
{
  unsigned long infoEx = 0;
  if (IsDlgButtonChecked (IDC_INFO_MODE)) infoEx |= PRESET_EX_MODE;
  int forumSel = ((CComboBox*) GetDlgItem (IDC_FORUM_ICONS))->GetCurSel ();
  if (forumSel == 1)
    infoEx |= PRESET_EX_FORUMICONS;
  else
    infoEx |= PRESET_EX_PLAYDOTAICONS;
  return infoEx;
}

void CPresentDlg::OnChange ()
{
  if (preset.GetCurSel () >= 0)
  {
    preset.SetWindowText ("Custom");
    OnCbnEditchangePreset ();
  }
  reg.writeInt ("curPreset", getReplayInfo ());
  updateEnables ();
}

static int __allboxes[] = {IDC_USE_ICONS, IDC_FORUM_ICONS, IDC_INFO_DATE, IDC_INFO_PATCH,
  IDC_INFO_MAP, IDC_INFO_NAME, IDC_INFO_MODE, IDC_INFO_HOST, IDC_INFO_SAVER,
  IDC_INFO_LENGTH, IDC_INFO_PLAYERS, IDC_INFO_SCORE, IDC_INFO_WINNER, IDC_INFO_OBSERVERS,
  IDC_PLAYERLIST, IDC_PLAYER_COLOR, IDC_PLAYER_HEROES, IDC_PLAYER_LEVEL, IDC_PLAYER_CREEPS,
  IDC_PLAYER_GOLD, IDC_PLAYER_APM, IDC_PLAYER_LANE, IDC_PLAYER_LEFT, IDC_PLAYER_ITEMS,
  IDC_PLAYER_GROUP, IDC_DETAIL_SKILL, IDC_DETAIL_SKTIME, IDC_DETAIL_ITEM, IDC_DETAIL_ITTIME,
  IDC_DETAIL_ITCOST, IDC_DETAIL_ACTION, IDC_DETAIL_GROUPS, IDC_COMMENT, IDC_STRIPTAGS,
  IDC_PREVIEW};
static const int __allboxn = sizeof __allboxes / sizeof __allboxes[0];

void CPresentDlg::updateEnables (bool big)
{
  if (big)
  {
    if (mode.GetCurSel () == 3)
    {
      for (int i = 0; i < __allboxn; i++)
        GetDlgItem (__allboxes[i])->EnableWindow (FALSE);
      GetDlgItem (IDC_FUNCTION_TIP)->ShowWindow (SW_SHOW);
      GetDlgItem (IDC_FUNCTION)->ShowWindow (SW_SHOW);
    }
    else
    {
      for (int i = 0; i < __allboxn; i++)
        GetDlgItem (__allboxes[i])->EnableWindow (TRUE);
      GetDlgItem (IDC_FUNCTION_TIP)->ShowWindow (SW_HIDE);
      GetDlgItem (IDC_FUNCTION)->ShowWindow (SW_HIDE);
    }
  }
  switch (mode.GetCurSel ())
  {
  case 0:
    GetDlgItem (IDC_USE_ICONS)->EnableWindow (FALSE);
    GetDlgItem (IDC_FORUM_ICONS)->EnableWindow (FALSE);
    GetDlgItem (IDC_PLAYER_COLOR)->EnableWindow (FALSE);
    break;
  case 1:
  case 2:
    GetDlgItem (IDC_USE_ICONS)->EnableWindow (TRUE);
    GetDlgItem (IDC_FORUM_ICONS)->EnableWindow (IsDlgButtonChecked (IDC_USE_ICONS));
    GetDlgItem (IDC_PLAYER_COLOR)->EnableWindow (TRUE);
    break;
  }
  if (mode.GetCurSel () != 3)
  {
    GetDlgItem (IDC_DETAIL_SKTIME)->EnableWindow (IsDlgButtonChecked (IDC_DETAIL_SKILL));
    GetDlgItem (IDC_DETAIL_ITTIME)->EnableWindow (IsDlgButtonChecked (IDC_DETAIL_ITEM));
    GetDlgItem (IDC_DETAIL_ITCOST)->EnableWindow (IsDlgButtonChecked (IDC_DETAIL_ITEM));
    GetDlgItem (IDC_PLAYER_LEVEL)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_HEROES)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_CREEPS)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_GOLD)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_LANE)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_APM)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_LEFT)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_ITEMS)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
    GetDlgItem (IDC_PLAYER_GROUP)->EnableWindow (IsDlgButtonChecked (IDC_PLAYERLIST));
  }
}

void CPresentDlg::OnCbnSelchangeGenMode()
{
  OnChange ();
  updateEnables (true);
}

void CPresentDlg::OnCbnSelchangePreset()
{
  if (preset.GetCurSel () >= 0)
    setReplayInfo (_presets[(int) preset.GetItemData (preset.GetCurSel ())].flags,
      _presets[(int) preset.GetItemData (preset.GetCurSel ())].flagsEx);
  reg.writeInt ("curPreset", _presets[(int) preset.GetItemData (preset.GetCurSel ())].flags);
  reg.writeInt ("curPresetEx", _presets[(int) preset.GetItemData (preset.GetCurSel ())].flagsEx);
}

void CPresentDlg::OnCbnEditchangePreset()
{
  char buf[256];
  preset.GetWindowText (buf, 256);
  if (!stricmp (buf, "Minimal") ||
      !stricmp (buf, "Forum replay") ||
      !stricmp (buf, "Full"))
  {
    GetDlgItem (IDC_SAVEPR)->EnableWindow (FALSE);
    GetDlgItem (IDC_DELPR)->EnableWindow (FALSE);
    return;
  }
  else
    GetDlgItem (IDC_SAVEPR)->EnableWindow (TRUE);
  for (int i = 0; i < preset.GetCount (); i++)
  {
    char cur[256];
    preset.GetLBText (i, cur);
    if (!stricmp (cur, buf))
    {
      GetDlgItem (IDC_DELPR)->EnableWindow (TRUE);
      return;
    }
  }
  GetDlgItem (IDC_DELPR)->EnableWindow (FALSE);
}

void CPresentDlg::OnBnClickedSavepr()
{
  char name[256];
  preset.GetWindowText (name, 256);
  if (!stricmp (name, "Custom"))
  {
    MessageBox ("Please select another name for the preset.", "Error", MB_ICONERROR | MB_OK);
    return;
  }
  if (!stricmp (name, "Minimal") ||
      !stricmp (name, "Forum replay") ||
      !stricmp (name, "Full"))
    return;
  for (int i = 0; i < preset.GetCount (); i++)
  {
    char cur[256];
    preset.GetLBText (i, cur);
    if (!stricmp (cur, name))
    {
      int count = reg.readInt ("numPresets", 0);
      for (int j = 0; j < count; j++)
      {
        reg.readString (mprintf ("preset%dName", i), cur, "");
        if (!stricmp (cur, name))
        {
          if (MessageBox ("Do you want to overwrite this preset?", "Warning", MB_ICONWARNING | MB_YESNO) == IDNO)
            return;
          reg.writeString (mprintf ("preset%dName", j), name);
          int cur = getReplayInfo ();
          int curEx = getReplayInfoEx ();
          reg.writeInt (mprintf ("preset%dVal", j), cur);
          reg.writeInt (mprintf ("preset%dValEx", j), curEx);
          preset.SetItemData (i, _presets.add (PresetInfo (cur, curEx)));
          return;
        }
      }
    }
  }
  int count = reg.readInt ("numPresets", 0);
  int cur = getReplayInfo ();
  int curEx = getReplayInfoEx ();
  reg.writeString (mprintf ("preset%dName", count), name);
  reg.writeInt (mprintf ("preset%dVal", count), cur);
  reg.writeInt (mprintf ("preset%dValEx", count), curEx);
  reg.writeInt ("numPresets", count + 1);
  int item = preset.InsertString (0, name);
  preset.SetItemData (item, _presets.add (PresetInfo (cur, curEx)));
  preset.SetCurSel (item);
}

void CPresentDlg::OnBnClickedDelpr()
{
  char name[256];
  preset.GetWindowText (name, 256);
  if (!stricmp (name, "Minimal") ||
      !stricmp (name, "Forum replay") ||
      !stricmp (name, "Full"))
    return;
  for (int i = 0; i < preset.GetCount (); i++)
  {
    char cur[256];
    preset.GetLBText (i, cur);
    if (!stricmp (cur, name))
    {
      int count = reg.readInt ("numPresets", 0);
      for (int j = 0; j < count; j++)
      {
        reg.readString (mprintf ("preset%dName", i), cur, "");
        if (!stricmp (cur, name))
        {
          if (MessageBox ("Are you sure you want to delete this preset?", "Warning", MB_ICONWARNING | MB_YESNO) == IDNO)
            return;
          if (j != count - 1)
          {
            reg.readString (mprintf ("preset%dName", count - 1), cur, "");
            reg.writeString (mprintf ("preset%dName", j), cur);
            reg.writeInt (mprintf ("preset%dVal", j), reg.readInt (mprintf ("preset%dVal", count - 1), 0));
            reg.writeInt (mprintf ("preset%dValEx", j), reg.readInt (mprintf ("preset%dValEx", count - 1), 0));
            j = count - 1;
          }
          reg.delKey (mprintf ("preset%dName", j));
          reg.delKey (mprintf ("preset%dVal", j));
          reg.delKey (mprintf ("preset%dValEx", j));
          reg.writeInt ("numPresets", count - 1);
          preset.DeleteString (i);
          preset.SetWindowText ("Custom");
          return;
        }
      }
    }
  }
}

extern char imageUrl[256];

void CPresentDlg::formatPlayer (CString& str, int id, int flags, int flagsEx)
{
  if ((flags & PRESET_MODE_MASK) == PRESET_MODE_TEXT)
  {
    if (w3g->players[id].hero)
      str += mprintf ("%s - %s", w3g->players[id].name, getHero (w3g->players[id].hero->id)->name);
    else
      str += mprintf ("%s - No Hero", w3g->players[id].name);
    if (flags & PRESET_PLEVEL && w3g->players[id].hero && w3g->players[id].hero->level)
      str += mprintf (", level %d", w3g->players[id].hero->level);
    if (flags & PRESET_PLANE && w3g->players[id].lane != LANE_AFK)
      str += mprintf (" (%s)", laneName[w3g->players[id].lane]);
    if (flags & PRESET_PKD && w3g->game.winner)
      str += mprintf (", K/D: %d/%d", w3g->players[id].stats[0], w3g->players[id].stats[1]);
    if (flags & PRESET_PCS && w3g->game.winner)
      str += mprintf (", CS: %d/%d", w3g->players[id].stats[2], w3g->players[id].stats[3]);
    if (flags & PRESET_PGOLD)
      str += mprintf (", build: %d gold", w3g->players[id].itemCost);
    if (flags & PRESET_PITEMS && w3g->players[id].hero)
    {
      w3g->players[id].inv.compute (END_TIME, w3g->dota);
      bool first = false;
      for (int i = 0; i < 6; i++)
        if (w3g->players[id].inv.inv[i])
          first = true;
      if (first)
      {
        str += ", items [";
        for (int i = 0; i < 6; i++)
          if (w3g->players[id].inv.inv[i])
          {
            if (!first)
              str += ", ";
            first = false;
            str += getItem (w3g->players[id].inv.inv[i])->name;
          }
        str += "]";
      }
    }
    if (flags & PRESET_PAPM)
      str += mprintf (", APM: %d", w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0);
    if (flags & PRESET_PLEFT && w3g->players[id].time + 30000 < w3g->time)
      str += mprintf (", left: %s", format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
    str += "\n";
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBCODE)
  {
    if (w3g->players[id].hero && (flags & PRESET_ICONS))
    {
      DotaHero* hero = getHero (w3g->players[id].hero->id);
      char const* playdotatag = getPDTag (hero->point);
      if ((flagsEx & PRESET_EX_PLAYDOTAICONS) && playdotatag)
        str += mprintf ("%s ", playdotatag);
      else
        str += mprintf ("[img]%s%s.gif[/img] ", imageUrl, hero->imgTag);
    }
    if (flags & PRESET_PCOLORS)
      str += mprintf ("[color=#%06X]%s[/color]", getFlipColor (w3g->players[id].slot.color),
        w3g->players[id].name);
    else
      str += w3g->players[id].name;
    if (flags & PRESET_PLEVEL && w3g->players[id].hero && w3g->players[id].hero->level)
      str += mprintf (", level %d", w3g->players[id].hero->level);
    if (flags & PRESET_PLANE && w3g->players[id].lane != LANE_AFK)
      str += mprintf (" (%s)", laneName[w3g->players[id].lane]);
    if (flags & PRESET_PKD && w3g->game.winner)
      str += mprintf (", K/D: %d/%d", w3g->players[id].stats[0], w3g->players[id].stats[1]);
    if (flags & PRESET_PCS && w3g->game.winner)
      str += mprintf (", CS: %d/%d", w3g->players[id].stats[2], w3g->players[id].stats[3]);
    if (flags & PRESET_PGOLD)
      str += mprintf (", build: %d gold", w3g->players[id].itemCost);
    if (flags & PRESET_PITEMS && w3g->players[id].hero)
    {
      w3g->players[id].inv.compute (END_TIME, w3g->dota);
      bool first = false;
      for (int i = 0; i < 6; i++)
        if (w3g->players[id].inv.inv[i])
          first = true;
      if (first)
      {
        if (flags & PRESET_ICONS)
        {
          str += ", items: ";
          for (int i = 0; i < 6; i++)
          {
            if (w3g->players[id].inv.inv[i])
            {
              char const* tag = getPDItemTag (getItemIcon (w3g->players[id].inv.inv[i]));
              if ((flagsEx & PRESET_EX_PLAYDOTAICONS) && tag)
                str += mprintf ("%s ", tag);
              else
                str += mprintf ("[img]%s%s.gif[/img]", imageUrl, getItemIcon (w3g->players[id].inv.inv[i]));
            }
          }
        }
        else
        {
          str += ", items [";
          for (int i = 0; i < 6; i++)
            if (w3g->players[id].inv.inv[i])
            {
              if (!first)
                str += ", ";
              first = false;
              str += getItem (w3g->players[id].inv.inv[i])->name;
            }
          str += "]";
        }
      }
    }
    if (flags & PRESET_PAPM)
      str += mprintf (", APM: %d", w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0);
    if (flags & PRESET_PLEFT && w3g->players[id].time + 30000 < w3g->time)
      str += mprintf (", left: %s", format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
    str += "\n";
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBHTML)
  {
    if (w3g->players[id].hero && (flags & PRESET_ICONS))
      str += mprintf ("<img src='%s%s.gif'> ", imageUrl, getHero (w3g->players[id].hero->id)->imgTag);
    if (flags & PRESET_PCOLORS)
      str += mprintf ("<font color='#%06X'>%s</font>", getFlipColor (w3g->players[id].slot.color),
        w3g->players[id].name);
    else
      str += w3g->players[id].name;
    if (flags & PRESET_PLEVEL && w3g->players[id].hero && w3g->players[id].hero->level)
      str += mprintf (", level %d", w3g->players[id].hero->level);
    if (flags & PRESET_PLANE && w3g->players[id].lane != LANE_AFK)
      str += mprintf (" (%s)", laneName[w3g->players[id].lane]);
    if (flags & PRESET_PKD && w3g->game.winner)
      str += mprintf (", K/D: %d/%d", w3g->players[id].stats[0], w3g->players[id].stats[1]);
    if (flags & PRESET_PCS && w3g->game.winner)
      str += mprintf (", CS: %d/%d", w3g->players[id].stats[2], w3g->players[id].stats[3]);
    if (flags & PRESET_PGOLD)
      str += mprintf (", build: %d gold", w3g->players[id].itemCost);
    if (flags & PRESET_PITEMS && w3g->players[id].hero)
    {
      w3g->players[id].inv.compute (END_TIME, w3g->dota);
      bool first = false;
      for (int i = 0; i < 6; i++)
        if (w3g->players[id].inv.inv[i])
          first = true;
      if (first)
      {
        if (flags & PRESET_ICONS)
        {
          str += ", items: ";
          for (int i = 0; i < 6; i++)
            if (w3g->players[id].inv.inv[i])
              str += mprintf ("<img src='%s%s.gif'>", imageUrl, getItem (w3g->players[id].inv.inv[i])->imgTag);
        }
        else
        {
          str += ", items [";
          for (int i = 0; i < 6; i++)
            if (w3g->players[id].inv.inv[i])
            {
              if (!first)
                str += ", ";
              first = false;
              str += getItem (w3g->players[id].inv.inv[i])->name;
            }
          str += "]";
        }
      }
    }
    if (flags & PRESET_PAPM)
      str += mprintf (", APM: %d", w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0);
    if (flags & PRESET_PLEFT && w3g->players[id].time + 30000 < w3g->time)
      str += mprintf (", left: %s", format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
    str += "<br>\n";
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_HTML)
  {
    if (w3g->players[id].hero && (flags & PRESET_ICONS))
      str += mprintf ("<tr><td><img src='%s%s.gif'> ", imageUrl, getHero (w3g->players[id].hero->id)->imgTag);
    else
      str += "<tr><td>";
    if (flags & PRESET_PCOLORS)
      str += mprintf ("<font color='#%06X'>%s</font>", getFlipColor (w3g->players[id].slot.color),
        w3g->players[id].name);
    else
      str += w3g->players[id].name;
    str += "</td>";
    if (flags & PRESET_PLEVEL)
    {
      if (w3g->players[id].hero && w3g->players[id].hero->level)
        str += mprintf ("<td align='right'>%d</td>", w3g->players[id].hero->level);
      else
        str += "<td align='center'>-</td>";
    }
    if (flags & PRESET_PLANE)
      str += mprintf ("<td>%s</td>", laneName[w3g->players[id].lane]);
    if (flags & PRESET_PKD && w3g->game.winner)
      str += mprintf ("<td align='right'>%d/%d</td>", w3g->players[id].stats[0], w3g->players[id].stats[1]);
    if (flags & PRESET_PCS && w3g->game.winner)
      str += mprintf ("<td align='right'>%d/%d</td>", w3g->players[id].stats[2], w3g->players[id].stats[3]);
    if (flags & PRESET_PGOLD)
      str += mprintf ("<td align='right'>%d</td>", w3g->players[id].itemCost);
    if (flags & PRESET_PITEMS)
    {
      str += "<td>";
      if (w3g->players[id].hero)
      {
        w3g->players[id].inv.compute (END_TIME, w3g->dota);
        bool first = false;
        for (int i = 0; i < 6; i++)
          if (w3g->players[id].inv.inv[i])
            first = true;
        if (first)
        {
          if (flags & PRESET_ICONS)
          {
            for (int i = 0; i < 6; i++)
              if (w3g->players[id].inv.inv[i])
                str += mprintf ("<img src='%s%s.gif'>", imageUrl, getItem (w3g->players[id].inv.inv[i])->imgTag);
          }
          else
          {
            for (int i = 0; i < 6; i++)
              if (w3g->players[id].inv.inv[i])
              {
                if (!first)
                  str += ", ";
                first = false;
                str += getItem (w3g->players[id].inv.inv[i])->name;
              }
          }
        }
        else
          str += "&nbsp;";
      }
      str += "</td>";
    }
    if (flags & PRESET_PAPM)
      str += mprintf ("<td align='right'>%d</td>", w3g->players[id].time ? w3g->players[id].actions * 60000 / w3g->players[id].time : 0);
    if (flags & PRESET_PLEFT)
    {
      if (w3g->players[id].time < w3g->time)
        str += mprintf ("<td align='right'>%s</td>", format_time (w3g, w3g->players[id].time, TIME_HOURS | TIME_SECONDS));
      else
        str += "<td align='right'>End</td>";
    }
    str += "</tr>\n";
  }
}

void CPresentDlg::formatDetails (CString& str, int id, int flags, int flagsEx)
{
  if ((flags & PRESET_MODE_MASK) == PRESET_MODE_TEXT)
  {
    if (str != "")
      str += "\n";
    str += mprintf ("Detailed report for %s:\n", w3g->players[id].name);
    if (flags & PRESET_SKILLS && w3g->players[id].hero && w3g->players[id].hero->level)
    {
      str += "Skill build:\n";
      int mxlen = 0;
      for (int i = 0; i < w3g->players[id].hero->level; i++)
      {
        int len = (int) strlen (getAbility (w3g->players[id].hero->abilities[i])->name);
        if (len > mxlen) mxlen = len;
      }
      for (int i = 0; i < w3g->players[id].hero->level; i++)
      {
        int len = (int) strlen (getAbility (w3g->players[id].hero->abilities[i])->name);
        str += mprintf ("  %2d. %s", i + 1, getAbility (w3g->players[id].hero->abilities[i])->name);
        if (flags & PRESET_STIME)
        {
          for (int j = len; j < mxlen; j++)
            str += " ";
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].hero->atime[i]));
        }
        str += "\n";
      }
    }
    if (flags & PRESET_ITEMS && w3g->players[id].hero && w3g->players[id].inv.num_items)
    {
      str += "Item build:\n";
      int mxlen = 0;
      for (int i = 0; i < w3g->players[id].inv.num_items; i++)
      {
        int len = (int) strlen (getItem (w3g->players[id].inv.items[i])->name);
        if (len > mxlen) mxlen = len;
      }
      for (int i = 0; i < w3g->players[id].inv.num_items; i++)
      {
        int len = (int) strlen (getItem (w3g->players[id].inv.items[i])->name);
        str += mprintf ("  %s", getItem (w3g->players[id].inv.items[i])->name);
        if (flags & PRESET_ITIME)
        {
          for (int j = len; j < mxlen; j++)
            str += " ";
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].inv.itemt[i]));
          len = mxlen + (int) strlen (format_time (w3g, w3g->players[id].inv.itemt[i])) - 6;
        }
        if (flags & PRESET_ICOST)
        {
          for (int j = len; j < mxlen; j++)
            str += " ";
          str += mprintf (" %4d gold", getItem (w3g->players[id].inv.items[i])->cost);
        }
        str += "\n";
      }
    }
    if (flags & PRESET_ACTIONS && w3g->players[id].actions)
    {
      str += "Actions:\n";
      int mxlen = 0;
      for (int i = 0; i < NUM_ACTIONS; i++)
        if (w3g->players[id].acounter[i] > 0 && (int) strlen (actionNames[i]) > mxlen)
          mxlen = (int) strlen (actionNames[i]);
      for (int i = 0; i < NUM_ACTIONS; i++)
      {
        if (w3g->players[id].acounter[i] == 0) continue;
        str += mprintf ("  %s ", actionNames[i]);
        int len = (int) strlen (actionNames[i]);
        for (int j = len; j < mxlen; j++)
          str += " ";
        str += mprintf ("%4d", w3g->players[id].acounter[i]);
        str += "\n";
      }
    }
    if (flags & PRESET_GROUPS)
    {
      bool groups = false;
      for (int i = 0; i < 10 && !groups; i++)
        if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
          groups = true;
      if (groups)
      {
        str += "Used groups:\n";
        int mxlen = 0;
        for (int i = 0; i < 10; i++)
          if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
            str += mprintf ("  Group %d, assigned %4d, used %4d\n", i + 1,
              w3g->players[id].hkassign[i], w3g->players[id].hkuse[i]);
      }
    }
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBCODE)
  {
    if (str != "")
      str += "\n";
    if (flags & PRESET_PCOLORS)
      str += mprintf ("[size=3]Detailed report for [color=#%06X]%s[/color]:[/size]\n",
        getFlipColor (w3g->players[id].slot.color), w3g->players[id].name);
    else
      str += mprintf ("[size=3]Detailed report for %s:[/size]\n", w3g->players[id].name);
    if (flags & PRESET_SKILLS && w3g->players[id].hero && w3g->players[id].hero->level)
    {
      str += "[b]Skill build:[/b][list=1]\n";
      for (int i = 0; i < w3g->players[id].hero->level; i++)
      {
        if (flags & PRESET_ICONS)
          str += mprintf ("[*][img]%s%s.gif[/img] %s", imageUrl,
            getAbility (w3g->players[id].hero->abilities[i])->imgTag,
            getAbility (w3g->players[id].hero->abilities[i])->name);
        else
          str += mprintf ("[*]%s", getAbility (w3g->players[id].hero->abilities[i])->name);
        if (flags & PRESET_STIME)
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].hero->atime[i]));
        str += "\n";
      }
      str += "[/list]\n";
    }
    if (flags & PRESET_ITEMS && w3g->players[id].hero && w3g->players[id].inv.num_items)
    {
      str += "[b]Item build:[/b][list]\n";
      for (int i = 0; i < w3g->players[id].inv.num_items; i++)
      {
        if (flags & PRESET_ICONS)
        {
          DotaItem* item = getItem (w3g->players[id].inv.items[i]);
          char const* tag = getPDItemTag (getItemIcon (item->index));
          if ((flagsEx & PRESET_EX_PLAYDOTAICONS) && tag)
            str += mprintf ("[*]%s %s", tag, item->name);
          else
            str += mprintf ("[*][img]%s%s.gif[/img] %s", imageUrl,
            getItemIcon (w3g->players[id].inv.items[i]), item->name);
        }
        else
          str += mprintf ("[*]%s", getItem (w3g->players[id].inv.items[i])->name);
        if (flags & PRESET_ITIME)
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].inv.itemt[i]));
        if (flags & PRESET_ICOST)
          str += mprintf (" %d gold", getItem (w3g->players[id].inv.items[i])->cost);
        str += "\n";
      }
      str += "[/list]\n";
    }
    if (flags & PRESET_ACTIONS && w3g->players[id].actions)
    {
      str += "[b]Actions:[/b][list]\n";
      for (int i = 0; i < NUM_ACTIONS; i++)
        if (w3g->players[id].acounter[i])
          str += mprintf ("[*]%s %d", actionNames[i], w3g->players[id].acounter[i]);
      str += "[/list]\n";
    }
    if (flags & PRESET_GROUPS)
    {
      bool groups = false;
      for (int i = 0; i < 10 && !groups; i++)
        if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
          groups = true;
      if (groups)
      {
        str += "[b]Used groups:[/b][list]\n";
        for (int i = 0; i < 10; i++)
          if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
            str += mprintf ("[*]Group %d, assigned %4d, used %4d\n", i + 1,
              w3g->players[id].hkassign[i], w3g->players[id].hkuse[i]);
        str += "[/list]\n";
      }
    }
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBHTML)
  {
    if (str != "")
      str += "<br>\n";
    if (flags & PRESET_PCOLORS)
      str += mprintf ("<font size='+1'>Detailed report for <font color='#%06X'>%s</font>:</font><br>\n",
        getFlipColor (w3g->players[id].slot.color), w3g->players[id].name);
    else
      str += mprintf ("<font size='+1'>Detailed report for %s:</font><br>\n", w3g->players[id].name);
    if (flags & PRESET_SKILLS && w3g->players[id].hero && w3g->players[id].hero->level)
    {
      str += "<b>Skill build:</b><ol>\n";
      for (int i = 0; i < w3g->players[id].hero->level; i++)
      {
        if (flags & PRESET_ICONS)
          str += mprintf ("<li><img src='%s%s.gif'> %s", imageUrl,
            getItemIcon (w3g->players[id].hero->abilities[i]),
            getAbility (w3g->players[id].hero->abilities[i])->name);
        else
          str += mprintf ("<li>%s", getAbility (w3g->players[id].hero->abilities[i])->name);
        if (flags & PRESET_STIME)
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].hero->atime[i]));
        str += "</li>\n";
      }
      str += "</ol>\n";
    }
    if (flags & PRESET_ITEMS && w3g->players[id].hero && w3g->players[id].inv.num_items)
    {
      str += "<b>Item build:</b><ul>\n";
      for (int i = 0; i < w3g->players[id].inv.num_items; i++)
      {
        if (flags & PRESET_ICONS)
          str += mprintf ("<li><img src='%s%s.gif'> %s", imageUrl,
            getItem (w3g->players[id].inv.items[i])->imgTag, getItem (w3g->players[id].inv.items[i])->name);
        else
          str += mprintf ("<li>%s", getItem (w3g->players[id].inv.items[i])->name);
        if (flags & PRESET_ITIME)
          str += mprintf (" (%s)", format_time (w3g, w3g->players[id].inv.itemt[i]));
        if (flags & PRESET_ICOST)
          str += mprintf (" %d gold", getItem (w3g->players[id].inv.items[i])->cost);
        str += "</li>\n";
      }
      str += "</ul>\n";
    }
    if (flags & PRESET_ACTIONS && w3g->players[id].actions)
    {
      str += "<b>Actions:</b><ul>\n";
      for (int i = 0; i < NUM_ACTIONS; i++)
        if (w3g->players[id].acounter[i])
          str += mprintf ("<li>%s %d</li>\n", actionNames[i], w3g->players[id].acounter[i]);
      str += "</ul>\n";
    }
    if (flags & PRESET_GROUPS)
    {
      bool groups = false;
      for (int i = 0; i < 10 && !groups; i++)
        if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
          groups = true;
      if (groups)
      {
        str += "<b>Used groups:</b><ul>\n";
        for (int i = 0; i < 10; i++)
          if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
            str += mprintf ("<li>Group %d, assigned %d, used %d</li>\n", i + 1,
              w3g->players[id].hkassign[i], w3g->players[id].hkuse[i]);
        str += "</ul>\n";
      }
    }
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_HTML)
  {
    if (str != "")
      str += "<br>\n";
    if (flags & PRESET_PCOLORS)
      str += mprintf ("<font size='+1'>Detailed report for <font color='#%06X'>%s</font>:</font>\n",
        getFlipColor (w3g->players[id].slot.color), w3g->players[id].name);
    else
      str += mprintf ("<font size='+1'>Detailed report for %s:</font>\n", w3g->players[id].name);
    if ((flags & PRESET_SKILLS) && w3g->players[id].hero && w3g->players[id].hero->level &&
        (flags & PRESET_ITEMS) && w3g->players[id].inv.num_items)
      str += "<table><tr valign='top'><td width='300'>\n";
    if (flags & PRESET_SKILLS && w3g->players[id].hero && w3g->players[id].hero->level)
    {
      str += "<b>Skill build:</b>\n<table>\n";
      for (int i = 0; i < w3g->players[id].hero->level; i++)
      {
        if (flags & PRESET_ICONS)
          str += mprintf ("<tr><td align='right'>%d.</td><td><img src='%s%s.gif'> %s</td>", i + 1, imageUrl,
            getAbility (w3g->players[id].hero->abilities[i])->imgTag,
            getAbility (w3g->players[id].hero->abilities[i])->name);
        else
          str += mprintf ("<tr><td align='right'>%d.</td><td>%s</td>", i + 1,
            getAbility (w3g->players[id].hero->abilities[i])->name);
        if (flags & PRESET_STIME)
          str += mprintf ("<td align='right'>%s</td>", format_time (w3g, w3g->players[id].hero->atime[i]));
        str += "</tr>\n";
      }
      str += "</table>\n";
    }
    if ((flags & PRESET_SKILLS) && w3g->players[id].hero && w3g->players[id].hero->level &&
        (flags & PRESET_ITEMS) && w3g->players[id].inv.num_items)
      str += "</td><td width='300'>\n";
    if (flags & PRESET_ITEMS && w3g->players[id].hero && w3g->players[id].inv.num_items)
    {
      str += "<b>Item build:</b>\n<table>\n";
      for (int i = 0; i < w3g->players[id].inv.num_items; i++)
      {
        if (flags & PRESET_ICONS)
          str += mprintf ("<tr><td><img src='%s%s.gif'> %s</td>", imageUrl,
            getItemIcon (w3g->players[id].inv.items[i]), getItem (w3g->players[id].inv.items[i])->name);
        else
          str += mprintf ("<tr><td>%s</td>", getItem (w3g->players[id].inv.items[i])->name);
        if (flags & PRESET_ITIME)
          str += mprintf ("<td align='right'>%s</td>", format_time (w3g, w3g->players[id].inv.itemt[i]));
        if (flags & PRESET_ICOST)
          str += mprintf ("<td align='right'>%d gold</td>", getItem (w3g->players[id].inv.items[i])->cost);
        str += "\n";
      }
      str += "</table>\n";
    }
    if ((flags & PRESET_SKILLS) && w3g->players[id].hero && w3g->players[id].hero->level &&
        (flags & PRESET_ITEMS) && w3g->players[id].inv.num_items)
      str += "</td></tr></table>\n";
    bool groups = false;
    for (int i = 0; i < 10 && !groups; i++)
      if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
        groups = true;
    if (flags & PRESET_ACTIONS && w3g->players[id].actions &&
        flags & PRESET_GROUPS && groups)
      str += "<table><tr valign='top'><td width='300'>\n";
    if (flags & PRESET_ACTIONS && w3g->players[id].actions)
    {
      str += "<b>Actions:</b>\n<table>\n";
      for (int i = 0; i < NUM_ACTIONS; i++)
        if (w3g->players[id].acounter[i])
          str += mprintf ("<tr><td>%s</td><td align='right'>%d</td></tr>\n",
            actionNames[i], w3g->players[id].acounter[i]);
      str += "</table>\n";
    }
    if (flags & PRESET_ACTIONS && w3g->players[id].actions &&
        flags & PRESET_GROUPS && groups)
      str += "</td><td width='300'>\n";
    if (flags & PRESET_GROUPS && groups)
    {
      str += "<table>\n<tr><th>Used groups</th><th>Assigned</th><th>Used</th></tr>\n";
      for (int i = 0; i < 10; i++)
        if (w3g->players[id].hkassign[i] || w3g->players[id].hkuse[i])
          str += mprintf ("<tr><td>Group %d</td><td align='right'>%d</td><td align='right'>%d</td></tr>\n",
            i + 1, w3g->players[id].hkassign[i], w3g->players[id].hkuse[i]);
      str += "</table>\n";
    }
    if (flags & PRESET_ACTIONS && w3g->players[id].actions &&
        flags & PRESET_GROUPS && groups)
      str += "</td></tr></table>\n";
  }
}

void CPresentDlg::format (CString& str)
{
  unsigned long flags = getReplayInfo ();
  unsigned long flagsEx = getReplayInfoEx ();
  str = "";
  if (w3g == NULL)
    str = "No replay loaded";
  else if (!w3g->dota.isDota)
    str = "Replay is not DotA game";
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_TEXT)
  {
    if (flags & PRESET_DATE) str += mprintf ("Date: %s\n", (char const*) w3g->saved_time);
    if (flags & PRESET_PATCH) str += mprintf ("Patch version: %s\n", formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
    if (flags & PRESET_MAP) str += mprintf ("Map: DotA %d.%02d%s\n", w3g->dota.major,
      w3g->dota.minor, w3g->dota.build ? "b" : "");
    if (flags & PRESET_NAME) str += mprintf ("Game name: %s\n", w3g->game.name);
    if (flagsEx & PRESET_EX_MODE) str += mprintf ("Game mode: %s\n", w3g->game.game_mode);
    if (flags & PRESET_HOST) str += mprintf ("Host name: %s\n", w3g->game.creator);
    if (flags & PRESET_SAVER && w3g->game.saver_id != 0)
      str += mprintf ("Replay saver: %s\n", w3g->players[w3g->game.saver_id].name);
    if (flags & PRESET_LENGTH)
      str += mprintf ("Game length: %s\n", format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
    if (flags & PRESET_PLAYERS)
      str += mprintf ("Players: %dv%d\n", w3g->dota.numSentinel, w3g->dota.numScourge);
    if (flags & PRESET_SCORE && w3g->game.winner)
      str += mprintf ("Game score: %d/%d\n", w3g->dota.sentinelKills, w3g->dota.scourgeKills);
    if (flags & PRESET_WINNER)
    {
      if (w3g->game.winner == WINNER_SENTINEL)
        str += "Winner: Sentinel\n";
      else if (w3g->game.winner == WINNER_SCOURGE)
        str += "Winner: Scourge\n";
      else if (w3g->game.winner == WINNER_GSENTINEL)
        str += "Winner: Sentinel\n";
      else if (w3g->game.winner == WINNER_GSCOURGE)
        str += "Winner: Scourge\n";
      else if (w3g->game.winner == WINNER_PSENTINEL)
        str += "Winner: Sentinel\n";
      else if (w3g->game.winner == WINNER_PSCOURGE)
        str += "Winner: Scourge\n";
    }
    if (flags & PRESET_OBSERVERS && w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
    {
      str += "Observers: ";
      bool first = true;
      for (int i = 0; i < w3g->numPlayers; i++)
        if (w3g->players[w3g->pindex[i]].slot.color > 11)
        {
          if (!first) str += ", ";
          str += w3g->players[w3g->pindex[i]].name;
          first = false;
        }
      str += "\n";
    }
    if (flags & PRESET_PLIST)
    {
      if ((flags & PRESET_PGROUP) == 0)
      {
        if (str != "")
          str += "\n";
        str += "Sentinel:\n";
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        if (str != "")
          str += "\n";
        str += "Scourge:\n";
        for (int i = 0; i < w3g->dota.numScourge; i++)
          formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
      }
      else
      {
        bool lanes[5];
        if (str != "")
          str += "\n";
        str += "Sentinel:\n";
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          lanes[w3g->players[w3g->dota.sentinel[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          if (w3g->players[w3g->dota.sentinel[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.sentinel[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numSentinel; i++)
            if (w3g->players[w3g->dota.sentinel[i]].lane == l)
              formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        }
        if (str != "")
          str += "\n";
        str += "Scourge:\n";
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numScourge; i++)
          lanes[w3g->players[w3g->dota.scourge[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numScourge; i++)
          if (w3g->players[w3g->dota.scourge[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.scourge[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numScourge; i++)
            if (w3g->players[w3g->dota.scourge[i]].lane == l)
              formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        }
      }
    }
    if (flags & PRESET_DETAIL_MASK)
    {
      for (int i = 0; i < w3g->dota.numSentinel; i++)
        if (IsDlgButtonChecked (playerBoxes[i]))
          formatDetails (str, w3g->dota.sentinel[i], flags, flagsEx);
      for (int i = 0; i < w3g->dota.numScourge; i++)
        if (IsDlgButtonChecked (playerBoxes[i + 5]))
          formatDetails (str, w3g->dota.scourge[i], flags, flagsEx);
    }
    CString comment;
    GetDlgItemText (IDC_COMMENT, comment);
    if (comment != "")
    {
      if (str != "")
        str += "\n";
      str += "Comment:\n" + comment + "\n";
    }
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBCODE)
  {
    if (flags & PRESET_DATE) str += mprintf ("[b]Date:[/b] %s\n",
      (char const*) w3g->saved_time);
    if (flags & PRESET_PATCH) str += mprintf ("[b]Patch version:[/b] %s\n", formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
    if (flags & PRESET_MAP) str += mprintf ("[b]Map:[/b] DotA %d.%02d%s\n",
      w3g->dota.major, w3g->dota.minor, w3g->dota.build ? "b" : "");
    if (flags & PRESET_NAME) str += mprintf ("[b]Game name:[/b] %s\n", w3g->game.name);
    if (flagsEx & PRESET_EX_MODE) str += mprintf ("[b]Game mode:[/b] %s\n", w3g->game.game_mode);
    if (flags & PRESET_HOST)
    {
      int hostc = 12;
      for (int i = 0; i < w3g->numPlayers; i++)
        if (!stricmp (w3g->players[w3g->pindex[i]].name, w3g->game.creator))
          hostc = w3g->players[w3g->pindex[i]].slot.color;
      if (flags & PRESET_PCOLORS)
        str += mprintf ("[b]Host name:[/b] [color=#%06X]%s[/color]\n", getFlipColor (hostc), w3g->game.creator);
      else
        str += mprintf ("[b]Host name:[/b] %s\n", w3g->game.creator);
    }
    if (flags & PRESET_SAVER && w3g->game.saver_id != 0)
    {
      if (flags & PRESET_PCOLORS)
        str += mprintf ("[b]Replay saver:[/b] [color=#%06X]%s[/color]\n",
          getFlipColor (w3g->players[w3g->game.saver_id].slot.color), w3g->players[w3g->game.saver_id].name);
      else
        str += mprintf ("[b]Replay saver:[/b] %s\n", w3g->players[w3g->game.saver_id].name);
    }
    if (flags & PRESET_LENGTH)
      str += mprintf ("[b]Game length:[/b] %s\n", format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
    if (flags & PRESET_PLAYERS)
      str += mprintf ("[b]Players:[/b] %dv%d\n", w3g->dota.numSentinel, w3g->dota.numScourge);
    if (flags & PRESET_SCORE && w3g->game.winner)
      str += mprintf ("[b]Game score:[/b] %d/%d\n", w3g->dota.sentinelKills, w3g->dota.scourgeKills);
    if (flags & PRESET_WINNER)
    {
      if (w3g->game.winner == WINNER_SENTINEL)
        str += mprintf ("[spoiler]Winner: Sentinel[/spoiler]\n");
      else if (w3g->game.winner == WINNER_SCOURGE)
        str += mprintf ("[spoiler]Winner: Scourge[/spoiler]\n");
      else if (w3g->game.winner == WINNER_GSENTINEL)
        str += mprintf ("[spoiler]Winner: Sentinel[/spoiler]\n");
      else if (w3g->game.winner == WINNER_GSCOURGE)
        str += mprintf ("[spoiler]Winner: Scourge[/spoiler]\n");
      else if (w3g->game.winner == WINNER_PSENTINEL)
        str += mprintf ("[spoiler]Winner: Sentinel[/spoiler]\n");
      else if (w3g->game.winner == WINNER_PSCOURGE)
        str += mprintf ("[spoiler]Winner: Scourge[/spoiler]\n");
    }
    if (flags & PRESET_OBSERVERS && w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
    {
      str += "[b]Observers:[/b] ";
      bool first = true;
      for (int i = 0; i < w3g->numPlayers; i++)
        if (w3g->players[w3g->pindex[i]].slot.color > 11)
        {
          if (!first) str += ", ";
          str += w3g->players[w3g->pindex[i]].name;
          first = false;
        }
      str += "\n";
    }
    if (flags & PRESET_PLIST)
    {
      if ((flags & PRESET_PGROUP) == 0)
      {
        if (str != "") str += "\n";
        str += mprintf ("[color=#%06X][size=3]Sentinel:[/size][/color]\n", getFlipColor (0));
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        str += mprintf ("\n[color=#%06X][size=3]Scourge:[/size][/color]\n", getFlipColor (6));
        for (int i = 0; i < w3g->dota.numScourge; i++)
          formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
      }
      else
      {
        bool lanes[5];
        str += mprintf ("[color=#%06X][size=3]Sentinel:[/size][/color]\n", getFlipColor (0));
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          lanes[w3g->players[w3g->dota.sentinel[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          if (w3g->players[w3g->dota.sentinel[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.sentinel[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numSentinel; i++)
            if (w3g->players[w3g->dota.sentinel[i]].lane == l)
              formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        }
        str += mprintf ("\n[color=#%06X][size=3]Scourge:[/size][/color]\n", getFlipColor (6));
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numScourge; i++)
          lanes[w3g->players[w3g->dota.scourge[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numScourge; i++)
          if (w3g->players[w3g->dota.scourge[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.scourge[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numScourge; i++)
            if (w3g->players[w3g->dota.scourge[i]].lane == l)
              formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        }
      }
    }
    if (flags & PRESET_DETAIL_MASK)
    {
      for (int i = 0; i < w3g->dota.numSentinel; i++)
        if (IsDlgButtonChecked (playerBoxes[i]))
          formatDetails (str, w3g->dota.sentinel[i], flags, flagsEx);
      for (int i = 0; i < w3g->dota.numScourge; i++)
        if (IsDlgButtonChecked (playerBoxes[i + 5]))
          formatDetails (str, w3g->dota.scourge[i], flags, flagsEx);
    }
    CString comment;
    GetDlgItemText (IDC_COMMENT, comment);
    if (comment != "")
    {
      if (str != "")
        str += "\n";
      str += "[b]Comment:[/b]\n" + comment + "\n";
    }
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_HTML)
  {
    if (flags & PRESET_INFO_MASK)
    {
      str += "<table>\n";
      if (flags & PRESET_DATE) str += mprintf ("<tr><td align='right'><b>Date:</b></td><td>%s</td></tr>\n",
       (char const*) w3g->saved_time);
      if (flags & PRESET_PATCH) str += mprintf ("<tr><td align='right'><b>Patch version:</b></td><td>%s</td></tr>\n", formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
      if (flags & PRESET_MAP) str += mprintf ("<tr><td align='right'><b>Map:</b></td><td>DotA "
        "%d.%02d%s</td></tr>\n", w3g->dota.major, w3g->dota.minor, w3g->dota.build ? "b" : "");
      if (flags & PRESET_NAME) str += mprintf ("<tr><td align='right'><b>Game name:</b></td><td>%s</td></tr>\n",
        w3g->game.name);
      if (flagsEx & PRESET_EX_MODE) str += mprintf ("<tr><td align='right'><b>Game mode:</b></td><td>%s</td></tr>\n",
        w3g->game.game_mode);
      if (flags & PRESET_HOST)
      {
        int hostc = 12;
        for (int i = 0; i < w3g->numPlayers; i++)
          if (!stricmp (w3g->players[w3g->pindex[i]].name, w3g->game.creator))
            hostc = w3g->players[w3g->pindex[i]].slot.color;
        if (flags & PRESET_PCOLORS)
          str += mprintf ("<tr><td align='right'><b>Host name:</b></td><td><font color='#%06X'>%s</font></td></tr>\n",
            getFlipColor (hostc), w3g->game.creator);
        else
          str += mprintf ("<tr><td align='right'><b>Host name:</b></td><td>%s</td></tr>\n", w3g->game.creator);
      }
      if (flags & PRESET_SAVER && w3g->game.saver_id != 0)
      {
        if (flags & PRESET_PCOLORS)
          str += mprintf ("<tr><td align='right'><b>Replay saver:</b></td><td><font color='#%06X'>%s</font></td></tr>\n",
            getFlipColor (w3g->players[w3g->game.saver_id].slot.color), w3g->players[w3g->game.saver_id].name);
        else
          str += mprintf ("<tr><td align='right'><b>Replay saver:</b></td><td>%s</td></tr>\n",
            w3g->players[w3g->game.saver_id].name);
      }
      if (flags & PRESET_LENGTH)
        str += mprintf ("<tr><td align='right'><b>Game length:</b></td><td>%s</td></tr>\n",
          format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
      if (flags & PRESET_PLAYERS)
        str += mprintf ("<tr><td align='right'><b>Players:</b></td><td>%dv%d</td></tr>\n",
          w3g->dota.numSentinel, w3g->dota.numScourge);
      if (flags & PRESET_SCORE && w3g->game.winner)
        str += mprintf ("<tr><td align='right'><b>Game score:</b></td><td>%d/%d</td></tr>\n",
          w3g->dota.sentinelKills, w3g->dota.scourgeKills);
      if (flags & PRESET_WINNER)
      {
        if (w3g->game.winner == WINNER_SENTINEL)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Sentinel</font></td></tr>\n",
            getFlipColor (0));
        else if (w3g->game.winner == WINNER_SCOURGE)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Scourge</font></td></tr>\n",
            getFlipColor (6));
        else if (w3g->game.winner == WINNER_GSENTINEL)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Sentinel</font></td></tr>\n",
            getFlipColor (0));
        else if (w3g->game.winner == WINNER_GSCOURGE)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Scourge</font></td></tr>\n",
            getFlipColor (6));
        else if (w3g->game.winner == WINNER_PSENTINEL)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Sentinel</font></td></tr>\n",
            getFlipColor (0));
        else if (w3g->game.winner == WINNER_PSCOURGE)
          str += mprintf ("<tr><td align='right'><b>Winner:</b></td><td><font color='#%06X'>Scourge</font></td></tr>\n",
            getFlipColor (6));
      }
      if (flags & PRESET_OBSERVERS && w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
      {
        str += "<tr><td align='right'><b>Observers:</b></td><td>";
        bool first = true;
        for (int i = 0; i < w3g->numPlayers; i++)
          if (w3g->players[w3g->pindex[i]].slot.color > 11)
          {
            if (!first) str += ", ";
            str += w3g->players[w3g->pindex[i]].name;
            first = false;
          }
        str += "</td></tr>\n";
      }
      str += "</table>\n";
    }
    if (flags & PRESET_PLIST)
    {
      str += "<table>\n";
      int nCols = 1;
      str += "<tr><th>Name</th>";
      if (flags & PRESET_PLEVEL)
      {
        str += "<th>Level</th>";
        nCols++;
      }
      if (flags & PRESET_PLANE)
      {
        str += "<th width='40'>Lane</th>";
        nCols++;
      }
      if (flags & PRESET_PKD && w3g->game.winner)
      {
        str += "<th width='40'>KD</th>";
        nCols++;
      }
      if (flags & PRESET_PCS && w3g->game.winner)
      {
        str += "<th width='40'>CS</th>";
        nCols++;
      }
      if (flags & PRESET_PGOLD)
      {
        str += "<th width='50'>Cost</th>";
        nCols++;
      }
      if (flags & PRESET_PITEMS)
      {
        str += "<th>Items</th>";
        nCols++;
      }
      if (flags & PRESET_PAPM)
      {
        str += "<th width='40'>APM</th>";
        nCols++;
      }
      if (flags & PRESET_PLEFT)
      {
        str += "<th width='40'>Left</th>";
        nCols++;
      }
      str += "</tr>\n";
      if ((flags & PRESET_PGROUP) == 0)
      {
        str += mprintf ("<tr><td colspan='%d'><font color='#%06X' size='+1'>Sentinel:</font></td></tr>\n",
          nCols, getFlipColor (0));
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        str += mprintf ("<tr><td colspan='%d'><font color='#%06X' size='+1'>Scourge:</font></td></tr>\n",
          nCols, getFlipColor (6));
        for (int i = 0; i < w3g->dota.numScourge; i++)
          formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
      }
      else
      {
        bool lanes[5];
        str += mprintf ("<tr><td colspan='%d'><font color='#%06X' size='+1'>Sentinel:</font></td></tr>\n",
          nCols, getFlipColor (0));
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          lanes[w3g->players[w3g->dota.sentinel[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          if (w3g->players[w3g->dota.sentinel[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.sentinel[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("<tr><td colspan='%d'>%s:</td></tr>\n", nCols, laneName[l]);
          for (int i = 0; i < w3g->dota.numSentinel; i++)
            if (w3g->players[w3g->dota.sentinel[i]].lane == l)
              formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        }
        str += mprintf ("<tr><td colspan='%d'><font color='#%06X' size='+1'>Scourge:</font></td></tr>\n",
          nCols, getFlipColor (6));
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numScourge; i++)
          lanes[w3g->players[w3g->dota.scourge[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numScourge; i++)
          if (w3g->players[w3g->dota.scourge[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.scourge[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("<tr><td colspan='%d'>%s:</td></tr>\n", nCols, laneName[l]);
          for (int i = 0; i < w3g->dota.numScourge; i++)
            if (w3g->players[w3g->dota.scourge[i]].lane == l)
              formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        }
      }
      str += "</table>\n";
    }
    if (flags & PRESET_DETAIL_MASK)
    {
      for (int i = 0; i < w3g->dota.numSentinel; i++)
        if (IsDlgButtonChecked (playerBoxes[i]))
          formatDetails (str, w3g->dota.sentinel[i], flags, flagsEx);
      for (int i = 0; i < w3g->dota.numScourge; i++)
        if (IsDlgButtonChecked (playerBoxes[i + 5]))
          formatDetails (str, w3g->dota.scourge[i], flags, flagsEx);
    }
    CString comment;
    GetDlgItemText (IDC_COMMENT, comment);
    if (comment != "")
    {
      if (str != "")
        str += "<br>\n";
      if (IsDlgButtonChecked (IDC_STRIPTAGS))
      {
        comment.Replace ("&", "&amp;");
        comment.Replace ("<", "&lt;");
        comment.Replace (">", "&rt;");
      }
      str += "<b>Comment:</b><br><pre>" + comment + "</pre>\n";
    }
  }
  //else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_SCRIPT && scriptEnv)
  //{
  //  CString text;
  //  func.GetWindowText (text);
  //  str = (char const*) scriptEnv->exec ((char const*) text, w3g);
  //}
}

void CPresentDlg::formatHTML (CString& str)
{
  unsigned long flags = getReplayInfo ();
  unsigned long flagsEx = getReplayInfoEx ();
  if ((flags & PRESET_MODE_MASK) == PRESET_MODE_TEXT)
  {
    format (str);
    str = "<html><head><title>Replay export</title></head><body><pre>" + str +
      "</pre></body></html>";
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_BBCODE)
  {
    flags = (flags & (~PRESET_MODE_MASK)) | PRESET_MODE_BBHTML;
    str = "<html><head><title>Replay export</title></head><body bgcolor='#151515' text='white' "
      "link='#ffcc00' alink='#ffcc00' vlink='#ffcc00'>";
    if (flags & PRESET_DATE) str += mprintf ("<b>Date:</b> %s<br>\n",
      (char const*) w3g->saved_time);
    if (flags & PRESET_PATCH) str += mprintf ("<b>Patch version:</b> %s<br>\n", formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
    if (flags & PRESET_MAP) str += mprintf ("<b>Map:</b> DotA %d.%02d%s<br>\n",
      w3g->dota.major, w3g->dota.minor, w3g->dota.build ? "b" : "");
    if (flags & PRESET_NAME) str += mprintf ("<b>Game name:</b> %s<br>\n", w3g->game.name);
    if (flagsEx & PRESET_EX_MODE) str += mprintf ("<b>Game mode:</b> %s<br>\n", w3g->game.game_mode);
    if (flags & PRESET_HOST)
    {
      int hostc = 12;
      for (int i = 0; i < w3g->numPlayers; i++)
        if (!stricmp (w3g->players[w3g->pindex[i]].name, w3g->game.creator))
          hostc = w3g->players[w3g->pindex[i]].slot.color;
      if (flags & PRESET_PCOLORS)
        str += mprintf ("<b>Host name:</b> <font color='#%06X'>%s</font><br>\n",
          getFlipColor (hostc), w3g->game.creator);
      else
        str += mprintf ("<b>Host name:</b> %s<br>\n", w3g->game.creator);
    }
    if (flags & PRESET_SAVER && w3g->game.saver_id != 0)
    {
      if (flags & PRESET_PCOLORS)
        str += mprintf ("<b>Replay saver:</b> <font color='#%06X'>%s</font><br>\n",
          getFlipColor (w3g->players[w3g->game.saver_id].slot.color), w3g->players[w3g->game.saver_id].name);
      else
        str += mprintf ("<b>Replay saver:</b> %s<br>\n", w3g->players[w3g->game.saver_id].name);
    }
    if (flags & PRESET_LENGTH)
      str += mprintf ("<b>Game length:</b> %s<br>\n", format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
    if (flags & PRESET_PLAYERS)
      str += mprintf ("<b>Players:</b> %dv%d<br>\n", w3g->dota.numSentinel, w3g->dota.numScourge);
    if (flags & PRESET_SCORE && w3g->game.winner)
      str += mprintf ("<b>Game score:</b> %d/%d<br>\n", w3g->dota.sentinelKills, w3g->dota.scourgeKills);
    if (flags & PRESET_WINNER)
    {
      if (w3g->game.winner == WINNER_SENTINEL || w3g->game.winner == WINNER_GSENTINEL ||
          w3g->game.winner == WINNER_PSENTINEL)
        str += mprintf ("<table border='1' cellspacing='0' width='400'><tr><td><font size='+1'>Spoiler"
          "</font></td></tr><tr><td><font color='#151515'>Winner: Sentinel</font></td></tr></table>\n");
      else if (w3g->game.winner == WINNER_SCOURGE || w3g->game.winner == WINNER_GSCOURGE ||
          w3g->game.winner == WINNER_PSCOURGE)
        str += mprintf ("<table border='1' cellspacing='0' width='400'><tr><td><font size='+1'>Spoiler"
          "</font></td></tr><tr><td><font color='#151515'>Winner: Scourge</font></td></tr></table>\n");
    }
    if (flags & PRESET_OBSERVERS && w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
    {
      str += "<b>Observers:</b> ";
      bool first = true;
      for (int i = 0; i < w3g->numPlayers; i++)
        if (w3g->players[w3g->pindex[i]].slot.color > 11)
        {
          if (!first) str += ", ";
          str += w3g->players[w3g->pindex[i]].name;
          first = false;
        }
      str += "<br>\n";
    }
    if (flags & PRESET_PLIST)
    {
      if ((flags & PRESET_PGROUP) == 0)
      {
        if (str != "") str += "\n";
        str += mprintf ("<font color='#%06X' size='+1'>Sentinel:</font><br>\n", getFlipColor (0));
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        str += mprintf ("<font color='#%06X' size='+1'>Scourge:</font><br>\n", getFlipColor (6));
        for (int i = 0; i < w3g->dota.numScourge; i++)
          formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
      }
      else
      {
        bool lanes[5];
        str += mprintf ("<font color='#%06X' size='+1'>Sentinel:</font><br>\n", getFlipColor (0));
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          lanes[w3g->players[w3g->dota.sentinel[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numSentinel; i++)
          if (w3g->players[w3g->dota.sentinel[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.sentinel[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:<br>\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numSentinel; i++)
            if (w3g->players[w3g->dota.sentinel[i]].lane == l)
              formatPlayer (str, w3g->dota.sentinel[i], flags, flagsEx);
        }
        str += mprintf ("<font color='#%06X' size='+1'>Scourge:</font><br>\n", getFlipColor (6));
        memset (lanes, 0, sizeof lanes);
        for (int i = 0; i < w3g->dota.numScourge; i++)
          lanes[w3g->players[w3g->dota.scourge[i]].lane] = true;
        for (int i = 0; i < w3g->dota.numScourge; i++)
          if (w3g->players[w3g->dota.scourge[i]].lane == LANE_ROAMING ||
              w3g->players[w3g->dota.scourge[i]].lane == LANE_AFK)
            formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        for (int l = 1; l <= 3; l++)
        {
          str += mprintf ("%s:<br>\n", laneName[l]);
          for (int i = 0; i < w3g->dota.numScourge; i++)
            if (w3g->players[w3g->dota.scourge[i]].lane == l)
              formatPlayer (str, w3g->dota.scourge[i], flags, flagsEx);
        }
      }
    }
    if (flags & PRESET_DETAIL_MASK)
    {
      for (int i = 0; i < w3g->dota.numSentinel; i++)
        if (IsDlgButtonChecked (playerBoxes[i]))
          formatDetails (str, w3g->dota.sentinel[i], flags, flagsEx);
      for (int i = 0; i < w3g->dota.numScourge; i++)
        if (IsDlgButtonChecked (playerBoxes[i + 5]))
          formatDetails (str, w3g->dota.scourge[i], flags, flagsEx);
    }
    CString comment;
    GetDlgItemText (IDC_COMMENT, comment);
    if (comment != "")
    {
      if (str != "")
        str += "<br>\n";
      comment.Replace ("&", "&amp;");
      comment.Replace ("<", "&lt;");
      comment.Replace (">", "&rt;");
      str += "<b>Comment:</b><br>\n<pre>" + comment + "</pre>\n";
    }
    str += "</body></html>";
  }
  else if ((flags & PRESET_MODE_MASK) == PRESET_MODE_HTML)
  {
    format (str);
    str = "<html><head><title>Replay export</title></head><body bgcolor='#151515' text='white' "
      "link='#ffcc00' alink='#ffcc00' vlink='#ffcc00'>" + str + "</body></html>";
  }
}

void CPresentDlg::OnBnClickedUpdate()
{
  CString str;
  format (str);
  result.SetWindowText (str);
}

void CPresentDlg::OnBnClickedCopy()
{
  CString buf;
  result.GetWindowText (buf);

  if (!OpenClipboard ())
    return;
  EmptyClipboard ();

  int count = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (LPCSTR) buf, -1, NULL, 0);
  wchar_t* msg = new wchar_t[count + 5];
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (LPCSTR) buf, -1, msg, count + 5);
  int sz = 0;
  for (; msg[sz]; sz++)
    ;
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    return;
  }

  LPTSTR copy = (LPTSTR) GlobalLock (handle);
  memcpy (copy, msg, sz * 2 + 2);
  GlobalUnlock (handle);
  delete[] msg;

  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
}

void CPresentDlg::OnBnClickedPreview()
{
  char path[256];
  CString buf;
  formatHTML (buf);
  GetTempPath (255, path);
  strcat (path, "log.html");
  FILE* f = fopen (path, "wt");
  fprintf (f, "%s", (char const*) buf);
  fclose (f);
  HKEY hKey;

  TCHAR name[128];
  DWORD size = 128;
  RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("http\\shell\\open\\ddeexec\\Application"), 0, KEY_QUERY_VALUE, &hKey);
  RegQueryValueEx (hKey, _T(""), 0, 0, (LPBYTE)name, &size);
  RegCloseKey (hKey);
  if (!_tcsnicmp (name, _T("IExplore"), 8))
    ShellExecute (NULL, _T("open"), _T("about:blank"), NULL, NULL, SW_SHOWNORMAL);
  ShellExecute (NULL, _T("open"), path, NULL, NULL, SW_SHOWNORMAL);
}

void CPresentDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}

void CPresentDlg::OnCbnSelchangeFunction()
{
  OnChange ();
}
