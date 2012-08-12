// ChatFilters.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "ChatFilters.h"
#include "replay.h"

static const int numCheckGroups = 2;
static const int groupHeaders[numCheckGroups] = {
  IDC_CHAT_MESSAGES,
  IDC_GAME_MESSAGES
};
static const int groupSizes[numCheckGroups] = {5, 15};
static const int groupLadderSizes[numCheckGroups] = {5, 3};
static const int groupItems[numCheckGroups][32] = {
  {
    IDC_ALL_CHAT,
    IDC_ALLY_CHAT,
    IDC_OBSERVER_CHAT,
    IDC_PRIVATE_CHAT,
    IDC_GAME_COMMANDS
  },
  {
    IDC_LEAVING_PLAYERS,
    IDC_GAME_PAUSES,
    IDC_CONTROL_SHARING,
    IDC_HERO_KILLS,
    IDC_TOWER_KILLS,
    IDC_RAX_KILLS,
    IDC_COURIER_KILLS,
    IDC_TREE_HEALTH,
    IDC_ROSHAN,
    IDC_AEGIS,
    IDC_GAME_MODE,
    IDC_NOTIFY_RUNE,
    IDC_PICKS_BANS,
    IDC_FASTKILLS,
    IDC_SPREES
  }
};
#define FILTER_CHAT_COMMAND     0x00100000
static const int groupFlags[numCheckGroups][32] = {
  {
    1 << CHAT_ALL,
    1 << CHAT_ALLIES,
    1 << CHAT_OBSERVERS,
    1 << CHAT_PRIVATE,
    FILTER_CHAT_COMMAND
  },
  {
    1 << (CHAT_NOTIFY_LEAVER + 4),
    1 << (CHAT_NOTIFY_PAUSE + 4),
    1 << (CHAT_NOTIFY_CONTROL + 4),
    1 << (CHAT_NOTIFY_KILL + 4),
    1 << (CHAT_NOTIFY_TOWER + 4),
    1 << (CHAT_NOTIFY_BARRACKS + 4),
    1 << (CHAT_NOTIFY_COURIER + 4),
    1 << (CHAT_NOTIFY_TREE + 4),
    1 << (CHAT_NOTIFY_ROSHAN + 4),
    1 << (CHAT_NOTIFY_AEGIS + 4),
    1 << (CHAT_NOTIFY_GAMEMODE + 4),
    1 << (CHAT_NOTIFY_RUNE + 4),
    1 << (CHAT_NOTIFY_PICKS + 4),
    1 << (CHAT_NOTIFY_FASTKILL + 4),
    1 << (CHAT_NOTIFY_SPREE + 4)
  }
};

// CChatFilters dialog

IMPLEMENT_DYNAMIC(CChatFilters, CDialog)

CChatFilters::CChatFilters(int chatFilters, bool isLadder, CWnd* pParent /*=NULL*/)
	: CDialog(CChatFilters::IDD, pParent)
{
  filters = chatFilters;
  ladder = isLadder;
}

CChatFilters::~CChatFilters()
{
}

void CChatFilters::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatFilters, CDialog)
  ON_BN_CLICKED(IDOK, &CChatFilters::OnBnClickedOk)
  ON_BN_CLICKED(IDC_ALL_CHAT, UpdateGroup1)
  ON_BN_CLICKED(IDC_ALLY_CHAT, UpdateGroup1)
  ON_BN_CLICKED(IDC_OBSERVER_CHAT, UpdateGroup1)
  ON_BN_CLICKED(IDC_PRIVATE_CHAT, UpdateGroup1)
  ON_BN_CLICKED(IDC_GAME_COMMANDS, UpdateGroup1)
  ON_BN_CLICKED(IDC_LEAVING_PLAYERS, UpdateGroup2)
  ON_BN_CLICKED(IDC_GAME_PAUSES, UpdateGroup2)
  ON_BN_CLICKED(IDC_CONTROL_SHARING, UpdateGroup2)
  ON_BN_CLICKED(IDC_HERO_KILLS, UpdateGroup2)
  ON_BN_CLICKED(IDC_TOWER_KILLS, UpdateGroup2)
  ON_BN_CLICKED(IDC_RAX_KILLS, UpdateGroup2)
  ON_BN_CLICKED(IDC_COURIER_KILLS, UpdateGroup2)
  ON_BN_CLICKED(IDC_TREE_HEALTH, UpdateGroup2)
  ON_BN_CLICKED(IDC_ROSHAN, UpdateGroup2)
  ON_BN_CLICKED(IDC_AEGIS, UpdateGroup2)
  ON_BN_CLICKED(IDC_GAME_MODE, UpdateGroup2)
  ON_BN_CLICKED(IDC_NOTIFY_RUNE, UpdateGroup2)
  ON_BN_CLICKED(IDC_PICKS_BANS, UpdateGroup2)
  ON_BN_CLICKED(IDC_FASTKILLS, UpdateGroup2)
  ON_BN_CLICKED(IDC_SPREES, UpdateGroup2)
  ON_BN_CLICKED(IDC_CHAT_MESSAGES, &CChatFilters::OnBnClickedChatMessages)
  ON_BN_CLICKED(IDC_GAME_MESSAGES, &CChatFilters::OnBnClickedGameMessages)
END_MESSAGE_MAP()

BOOL CChatFilters::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  for (int i = 0; i < numCheckGroups; i++)
  {
    for (int j = 0; j < groupSizes[i]; j++)
    {
      int id = groupItems[i][j];
      if (ladder && j >= groupLadderSizes[i])
      {
        GetDlgItem (id)->ShowWindow (SW_HIDE);
        CheckDlgButton (id, 0);
      }
      else
      {
        GetDlgItem (id)->ShowWindow (SW_SHOW);
        CheckDlgButton (id, filters & groupFlags[i][j]);
      }
    }
  }
  onClickItem (0);
  onClickItem (1);

  return TRUE;
}

// CChatFilters message handlers

void CChatFilters::OnBnClickedOk()
{
  filters = 0;
  for (int i = 0; i < numCheckGroups; i++)
  {
    for (int j = 0; j < groupSizes[i]; j++)
    {
      if (ladder && j >= groupLadderSizes[i]) break;
      if (IsDlgButtonChecked (groupItems[i][j]))
        filters |= groupFlags[i][j];
    }
  }
  OnOK();
}
bool CChatFilters::isMessageAllowed (int filter, unsigned long mode, unsigned long notifyType)
{
  if (mode < 4)
    return (filter & (1 << mode)) != 0;
  if (mode == CHAT_COMMAND)
    return (filter & FILTER_CHAT_COMMAND) != 0;
  if (mode == CHAT_NOTIFY && notifyType > 0)
    return (filter & (1 << (notifyType + 4))) != 0;
  return true;
}
int CChatFilters::getDefaultFilter ()
{
  return 0xFFFFFFFF;
}

int CChatFilters::getGroupState (int group)
{
  int state = (IsDlgButtonChecked (groupItems[group][0]) ? 1 : 0);
  for (int i = 1; i < groupSizes[group]; i++)
  {
    if (ladder && i >= groupLadderSizes[group]) break;
    int curState = (IsDlgButtonChecked (groupItems[group][i]) ? 1 : 0);
    if (curState != state)
      state = 2;
  }
  return state;
}
void CChatFilters::onClickItem (int group)
{
  CheckDlgButton (groupHeaders[group], getGroupState (group));
}
void CChatFilters::onClickHeader (int group)
{
  int state = getGroupState (group);
  if (state == 1)
    state = 0;
  else
    state = 1;
  for (int i = 0; i < groupSizes[group]; i++)
  {
    if (ladder && i >= groupLadderSizes[group]) break;
    CheckDlgButton (groupItems[group][i], state);
  }
  CheckDlgButton (groupHeaders[group], state);
}
void CChatFilters::UpdateGroup1 ()
{
  onClickItem (0);
}
void CChatFilters::UpdateGroup2 ()
{
  onClickItem (1);
}
void CChatFilters::OnBnClickedChatMessages()
{
  onClickHeader (0);
}
void CChatFilters::OnBnClickedGameMessages()
{
  onClickHeader (1);
}

bool CChatFilters::shouldInsertBlanks (int filter)
{
  return true;//(filter & (1 << CHAT_ALLIES)) != 0;
}
