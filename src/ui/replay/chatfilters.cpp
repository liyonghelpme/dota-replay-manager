#include "core/app.h"

#include "gamechat.h"
#include "replay/message.h"

#define FILTER_CHAT_COMMAND       (1 << 4)

struct FilterItem
{
  int id;
  uint32 flag;
};
static const FilterItem items[] = {
  {IDC_MSG_CHAT, 0},
  {IDC_MSG_ALLCHAT, 1 << CHAT_ALL},
  {IDC_MSG_ALLYCHAT, 1 << CHAT_ALLIES},
  {IDC_MSG_OBSERVERCHAT, 1 << CHAT_OBSERVERS},
  {IDC_MSG_PRIVATECHAT, 1 << CHAT_PRIVATE},
  {IDC_MSG_CHATCMD, FILTER_CHAT_COMMAND},

  {IDC_MSG_GAME, 0},
  {IDC_MSG_LEAVERS, 1 << (CHAT_NOTIFY_LEAVER + 4)},
  {IDC_MSG_PAUSES, 1 << (CHAT_NOTIFY_PAUSE + 4)},
  {IDC_MSG_CONTROL, 1 << (CHAT_NOTIFY_CONTROL + 4)},
  {IDC_MSG_HEROKILLS, 1 << (CHAT_NOTIFY_KILL + 4)},
  {IDC_MSG_TOWERKILLS, 1 << (CHAT_NOTIFY_TOWER + 4)},
  {IDC_MSG_RAXKILLS, 1 << (CHAT_NOTIFY_BARRACKS + 4)},
  {IDC_MSG_COURIERKILLS, 1 << (CHAT_NOTIFY_COURIER + 4)},
  {IDC_MSG_TREEHEALTH, 1 << (CHAT_NOTIFY_TREE + 4)},
  {IDC_MSG_ROSHAN, 1 << (CHAT_NOTIFY_ROSHAN + 4)},
  {IDC_MSG_AEGIS, 1 << (CHAT_NOTIFY_AEGIS + 4)},
  {IDC_MSG_GAMEMODE, 1 << (CHAT_NOTIFY_GAMEMODE + 4)},
  {IDC_MSG_RUNES, 1 << (CHAT_NOTIFY_RUNE + 4)},
  {IDC_MSG_PICKSBANS, 1 << (CHAT_NOTIFY_PICKS + 4)},
  {IDC_MSG_FASTKILLS, 1 << (CHAT_NOTIFY_FASTKILL + 4)},
  {IDC_MSG_SPREES, 1 << (CHAT_NOTIFY_SPREE + 4)},
};
static const int numItems = sizeof items / sizeof items[0];
static const int numItemsLadder = 10;

INT_PTR CALLBACK ReplayGameChatTab::FiltersDlgProc(
  HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  BOOL ladder = GetWindowLong(hDlg, DWL_USER);
  switch (message)
  {
  case WM_INITDIALOG:
    {
      ladder = (BOOL) lParam;
      SetWindowLong(hDlg, DWL_USER, ladder);
      int header = -1;
      int groupval = -1;
      for (int i = 0; i < numItems; i++)
      {
        if (ladder && i >= numItemsLadder)
          ShowWindow(GetDlgItem(hDlg, items[i].id), SW_HIDE);
        else if (items[i].flag == 0)
        {
          if (header >= 0 && groupval >= 0)
            CheckDlgButton(hDlg, items[header].id, groupval);
          header = i;
          groupval = -1;
        }
        else
        {
          int state = (cfg.chatFilters & items[i].flag ? BST_CHECKED : BST_UNCHECKED);
          CheckDlgButton(hDlg, items[i].id, state);
          if (groupval < 0)
            groupval = state;
          else if (groupval != state)
            groupval = BST_INDETERMINATE;
        }
      }
      if (header >= 0 && groupval >= 0)
        CheckDlgButton(hDlg, items[header].id, groupval);
    }
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      if (LOWORD(wParam) == IDOK)
      {
        for (int i = 0; i < numItems; i++)
        {
          if (ladder && i >= numItemsLadder)
            break;
          if (items[i].flag)
          {
            if (IsDlgButtonChecked(hDlg, items[i].id) == BST_CHECKED)
              cfg.chatFilters |= items[i].flag;
            else
              cfg.chatFilters &= ~items[i].flag;
          }
        }
      }
      EndDialog(hDlg, LOWORD(wParam));
    }
    else
    {
      int header = -1;
      int groupval = -1;
      int count = (ladder ? numItemsLadder : numItems);
      for (int i = 0; i <= count; i++)
      {
        if (i >= count || items[i].flag == 0)
        {
          if (header >= 0)
          {
            if (LOWORD(wParam) == items[header].id)
            {
              groupval = (groupval == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
              for (int j = header; j < i; j++)
                CheckDlgButton(hDlg, items[j].id, groupval);
            }
            else
              CheckDlgButton(hDlg, items[header].id, groupval);
          }
          header = i;
          groupval = -1;
        }
        else
        {
          int state = IsDlgButtonChecked(hDlg, items[i].id);
          if (groupval < 0)
            groupval = state;
          else if (groupval != state)
            groupval = BST_INDETERMINATE;
        }
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool ReplayGameChatTab::FilterMessage(W3GMessage& msg)
{
  if (msg.mode >= CHAT_ALL && msg.mode <= CHAT_PRIVATE)
    return (cfg.chatFilters & (1 << msg.mode)) != 0;
  else if (msg.mode == CHAT_COMMAND)
    return (cfg.chatFilters & FILTER_CHAT_COMMAND) != 0;
  else if (msg.mode == CHAT_NOTIFY)
    return (cfg.chatFilters & (1 << (msg.notifyType + 4))) != 0;
  return true;
}
