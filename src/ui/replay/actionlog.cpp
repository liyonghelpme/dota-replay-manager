#include "core/app.h"

#include <stdio.h>
#include <stdarg.h>

#include "base/regexp.h"
#include "dota/colors.h"
#include "actionlog.h"

#include "replay/actiondump.h"
#include "frameui/dragdrop.h"

#define IDC_LOADREPLAY        120
#define IDC_RAWCODES          121
#define IDC_SEARCHMODE        122
#define IDC_SEARCHPLAYER      123
#define IDC_SEARCHTEXT        124
#define IDC_SEARCHNEXT        125
#define IDC_SEARCHPREV        126
#define IDC_ACTIONLIST        127

class ActionListFrame : public ListFrame, public ActionLogger
{
  void drawItem(DRAWITEMSTRUCT* dis);
  int drawItemText(HDC hDC, String text, RECT* rc, uint32 format, bool utf8);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  int toolHitTest(POINT pt, ToolInfo* ti)
  {
    return -1;
  }
public:
  struct Action
  {
    uint32 time;
    W3GPlayer* player;
    String text;
  };
  Array<Action> actions;

  MapData* mapData;
  bool rawCodes;
  W3GReplay* w3g;
  int lastColumnWidth;
  ActionListFrame(Frame* parent, int id)
    : ListFrame(parent, id, LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS)
    , mapData(NULL)
    , w3g(NULL)
    , rawCodes(false)
    , lastColumnWidth(0)
  {
    insertColumn(0, "");
    insertColumn(1, "");
    insertColumn(2, "");
    ListView_SetColumnWidth(hWnd, 0, 50);
    ListView_SetColumnWidth(hWnd, 1, 120);
    ListView_SetColumnWidth(hWnd, 2, LVSCW_AUTOSIZE_USEHEADER);
  }

  void reset(W3GReplay* _w3g, MapData* _mapData)
  {
    clear();
    actions.clear();
    w3g = _w3g;
    mapData = _mapData;
    lastColumnWidth = 0;
  }

  void addAction(uint32 time, W3GPlayer* player, String text)
  {
    HDC hDC = GetDC(hWnd);
    SelectObject(hDC, getFont());
    int width = drawItemText(hDC, text, NULL, DT_LEFT, false) + 12;
    ReleaseDC(hWnd, hDC);
    if (width > lastColumnWidth)
      lastColumnWidth = width;

    Action& a = actions.push();
    a.time = time;
    a.player = player;
    a.text = text;

    LVITEM lvi;
    memset(&lvi, 0, sizeof lvi);
    lvi.iItem = ListView_GetItemCount(hWnd);

    lvi.mask = LVIF_TEXT;
    lvi.pszText = "";
    ListView_InsertItem(hWnd, &lvi);
  }

  String transform(String text);
};

uint32 ActionListFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_SIZE)
  {
    int width = LOWORD(lParam) - 50 - 120;
    if (width < lastColumnWidth)
      width = lastColumnWidth;
    ListView_SetColumnWidth(hWnd, 2, width);
  }
  else if (message == WM_DRAWITEM)
  {
    drawItem((DRAWITEMSTRUCT*) lParam);
    return TRUE;
  }
  return ListFrame::onMessage(message, wParam, lParam);
}

String ActionListFrame::transform(String text)
{
  if (rawCodes || mapData == NULL)
    return text;
  String result = "";
  for (int cur = 0; cur < text.length(); cur++)
  {
    result += text[cur];
    if (text[cur] == '[')
    {
      int save = cur;
      cur++;
      uint32 id = 0;
      while (text[cur] && text[cur] != ']')
        id = id * 256 + uint8(text[cur++]);
      if (text[cur] != ']')
        id = 0;

      UnitData* unit = NULL;
      if (id)
        unit = mapData->getData()->getUnitById(id);
      if (unit == NULL)
        cur = save;
      else
      {
        result += String(unit->getStringData("Name", 0)).toAnsi();
        result += ']';
      }
    }
  }
  return result;
}
int ActionListFrame::drawItemText(HDC hDC, String text, RECT* rc, uint32 format, bool utf8)
{
  uint32 FMT = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;
  SIZE sz;
  int width = 0;

  if (utf8)
  {
    int length = convertUtf8(text);
    GetTextExtentPoint32W(hDC, wcBuf, length, &sz);
    width = sz.cx;
    if (rc)
      DrawTextW(hDC, wcBuf, length, rc, format | FMT);
  }
  else
  {
    int prev = 0;
    if (format == DT_LEFT && mapData && w3g && !rawCodes)
    {
      HIMAGELIST imgList = mapData->getImageList();
      for (int cur = 0; cur < text.length(); cur++)
      {
        uint32 id = 0;
        int save = cur;
        if (text[cur] == '[')
        {
          cur++;
          while (text[cur] && text[cur] != ']')
            id = id * 256 + uint8(text[cur++]);
          if (text[cur] != ']')
            id = 0;
        }
        UnitData* unit = NULL;
        if (id)
          unit = mapData->getData()->getUnitById(id);
        if (unit == NULL)
          cur = save;
        else
        {
          save++;
          if (save > prev)
          {
            GetTextExtentPoint32(hDC, text.c_str() + prev, save - prev, &sz);
            if (rc)
            {
              DrawText(hDC, text.c_str() + prev, save - prev, rc, format | FMT);
              rc->left += sz.cx;
            }
            width += sz.cx;
          }

          int image = mapData->getImageIndex(unit->getStringData("Art", 0));
          if (image)
          {
            if (rc && rc->left < rc->right)
            {
              ImageList_DrawEx(imgList, image, hDC, rc->left + 1, (rc->top + rc->bottom) / 2 - 8,
                rc->right - rc->left > 16 ? 16 : rc->right - rc->left, 16,
                CLR_NONE, CLR_NONE, ILD_NORMAL);
              rc->left += 18;
            }
            width += 18;
          }

          int length = convertUtf8(unit->getStringData("Name", 0));
          GetTextExtentPoint32W(hDC, wcBuf, length, &sz);
          if (rc)
          {
            DrawTextW(hDC, wcBuf, length, rc, format | FMT);
            rc->left += sz.cx;
          }
          width += sz.cx;

          prev = cur;
        }
      }
    }
    if (prev < text.length())
    {
      GetTextExtentPoint32(hDC, text.c_str() + prev, text.length() - prev, &sz);
      width += sz.cx;
      if (rc)
        DrawText(hDC, text.c_str() + prev, text.length() - prev, rc, format | FMT);
    }
  }
  return width;
}
void ActionListFrame::drawItem(DRAWITEMSTRUCT* dis)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.mask = LVIF_STATE;
  lvi.iItem = dis->itemID;
  lvi.iSubItem = 0;
  lvi.stateMask = 0xFFFF;
  ListView_GetItem(hWnd, &lvi);

  bool focus = (GetFocus() == hWnd);
  uint32 style = GetWindowLong(hWnd, GWL_STYLE);
  bool selected = (focus || (style & LVS_SHOWSELALWAYS)) && (lvi.state & LVIS_SELECTED);
  selected = selected || (lvi.state & LVIS_DROPHILITED);

  uint32 color = 0xFFFFFF;
  uint32 clrTextSave, clrBkSave;
  if (selected)
  {
    clrTextSave = SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    clrBkSave = SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
  }
  else
    clrBkSave = SetBkColor(dis->hDC, color);

  RECT allLabels, label, wnd;
  ListView_GetItemRect(hWnd, dis->itemID, &allLabels, LVIR_BOUNDS);
  ListView_GetItemRect(hWnd, dis->itemID, &label, LVIR_LABEL);
  GetClientRect(hWnd, &wnd);

  ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &allLabels, NULL, 0, NULL);

  RECT item = label;
  item.left += 2;
  item.right -= 2;

  drawItemText(dis->hDC, w3g->formatTime(actions[lvi.iItem].time), &item, DT_RIGHT, false);

  label.left = label.right;
  label.right = label.left + 120;

  item = label;
  item.left += 6;
  item.right -= 6;

  if (actions[lvi.iItem].player)
  {
    if (!selected)
    {
      SetBkColor(dis->hDC, getLightColor(actions[lvi.iItem].player->slot.color));
      ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &label, NULL, 0, NULL);
      SetBkColor(dis->hDC, color);
    }
    drawItemText(dis->hDC, actions[lvi.iItem].player->name, &item, DT_LEFT, true);
  }

  label.left = label.right;
  label.right = wnd.right;
  item = label;
  item.left += 6;

  drawItemText(dis->hDC, actions[lvi.iItem].text, &item, DT_LEFT, false);

  if (selected)
    SetTextColor(dis->hDC, clrTextSave);
  SetBkColor(dis->hDC, clrBkSave);
}

//////////////////////////////////////////////////////////

void ReplayActionLogTab::parseReplay()
{
  if (mapData)
    return;
  loadButton->disable();
  loadButton->setText("Loading...");
  mapData = new MapData(w3g->getGameInfo()->map);
  if (!mapData->isLoaded())
  {
    delete mapData;
    mapData = NULL;
    loadButton->setText("Failed");
    return;
  }

  searchMode->enable();
  searchPlayer->reset();
  searchPlayer->enable();
  searchNext->enable();
  searchPrev->enable();
  searchText->enable();

  searchPlayer->addString("All actions", 0xFFFFFF, "Unknown", -1);
  searchPlayer->addString("System actions (SyncStoredInteger)", 0xFFFFFF, "IconReplay", 0);
  DotaInfo const* info = w3g->getDotaInfo();
  if (info)
  {
    for (int t = 0; t < 2; t++)
    {
      if (info->team_size[t])
        searchPlayer->addString(t ? "Scourge" : "Sentinel",
          0, t ? "GreenBullet" : "RedBullet", 0);
      for (int i = 0; i < info->team_size[t]; i++)
      {
        W3GPlayer* player = info->teams[t][i];
        if (player->hero)
          searchPlayer->addString(String::format("%s (%s)", player->name, player->hero->hero->name),
            getLightColor(player->slot.color), player->hero->hero->icon, (uint32) player);
        else
          searchPlayer->addString(String::format("%s (No Hero)", player->name),
            getLightColor(player->slot.color), NULL, (uint32) player);
      }
    }
  }
  else
  {
    for (int i = 0; i < w3g->getNumPlayers(); i++)
    {
      W3GPlayer* player = w3g->getPlayer(i);
      searchPlayer->addString(player->name, getLightColor(player->slot.color),
        getRaceIcon(player->race), (uint32) player);
    }
  }
  searchPlayer->setCurSel(0);

  actionList->reset(w3g, mapData);
  actionList->enable();
  actionList->setRedraw(false);

  ActionDumper dumper(w3g, mapData);
  dumper.dump(actionList, DUMP_FULL);

  actionList->setRedraw(true);

  loadButton->setText("Done");
}
void ReplayActionLogTab::onSetReplay()
{
  delete mapData;
  mapData = NULL;
  actionList->clear();
  actionList->reset(NULL, NULL);
  actionList->disable();
  searchMode->disable();
  searchPlayer->reset();
  searchPlayer->disable();
  searchNext->disable();
  searchPrev->disable();
  searchText->disable();

  loadButton->setText("Load");
  loadButton->enable();
}
#define ID_ACTION_COPY        100
ReplayActionLogTab::ReplayActionLogTab(Frame* parent)
  : ReplayTab(parent)
{
  mapData = NULL;

  ctxMenu = CreatePopupMenu();
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;
  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Copy";
  mii.cch = strlen(mii.dwTypeData);
  mii.wID = ID_ACTION_COPY;
  InsertMenuItem(ctxMenu, 0, TRUE, &mii);

  loadButton = new ButtonFrame("Load", this, IDC_LOADREPLAY);
  rawCodes = new ButtonFrame("Show raw codes", this, IDC_RAWCODES, BS_AUTOCHECKBOX);
  searchMode = new ComboFrame(this, IDC_SEARCHMODE);
  searchPlayer = new ComboFrameEx(this, IDC_SEARCHPLAYER);
  searchNext = new ButtonFrame("Find next", this, IDC_SEARCHNEXT);
  searchPrev = new ButtonFrame("Find prev", this, IDC_SEARCHPREV);
  searchText = new EditFrame(this, IDC_SEARCHTEXT);
  actionList = new ActionListFrame(this, IDC_ACTIONLIST);

  searchMode->addString("Text contains");
  searchMode->addString("Text equals");
  searchMode->addString("Text starts with");
  searchMode->addString("Regular expression");
  searchMode->setCurSel(0);

  loadButton->setPoint(PT_TOPLEFT, 10, 10);
  loadButton->setSize(75, 21);
  rawCodes->setPoint(PT_BOTTOMLEFT, loadButton, PT_BOTTOMRIGHT, 10, 0);
  rawCodes->setSize(150, 21);
  searchMode->setPoint(PT_TOPLEFT, loadButton, PT_BOTTOMLEFT, 0, 4);
  searchMode->setWidth(120);
  searchPlayer->setPoint(PT_TOPLEFT, searchMode, PT_TOPRIGHT, 5, 0);
  searchText->setPoint(PT_TOPLEFT, searchMode, PT_BOTTOMLEFT, 0, 3);
  searchText->setHeight(21);
  searchText->setPoint(PT_RIGHT, -10, 0);
  searchNext->setSize(70, 21);
  searchPrev->setSize(70, 21);
  searchPrev->setPoint(PT_BOTTOMRIGHT, searchText, PT_TOPRIGHT, 0, -3);
  searchNext->setPoint(PT_TOPRIGHT, searchPrev, PT_TOPLEFT, -5, 0);
  searchPlayer->setPoint(PT_TOPRIGHT, searchNext, PT_TOPLEFT, -5, 0);

  actionList->setPoint(PT_TOPLEFT, searchText, PT_BOTTOMLEFT, 0, 4);
  actionList->setPoint(PT_BOTTOMRIGHT, -10, -10);
}
ReplayActionLogTab::~ReplayActionLogTab()
{
  DestroyMenu(ctxMenu);
  delete mapData;
}

uint32 ReplayActionLogTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_CONTEXTMENU:
    if (w3g && ListView_GetSelectedCount(actionList->getHandle()) > 0)
    {
      POINT pt;
      GetCursorPos(&pt);
      int result = TrackPopupMenuEx(ctxMenu, TPM_HORIZONTAL | TPM_LEFTALIGN |
        TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, actionList->getHandle(), NULL);
      if (result == ID_ACTION_COPY)
      {
        String result = "";
        for (int sel = ListView_GetNextItem(actionList->getHandle(), -1, LVNI_SELECTED);
          sel >= 0; sel = ListView_GetNextItem(actionList->getHandle(), sel, LVNI_SELECTED))
        {
          if (!result.isEmpty())
            result += "\r\n";
          result += String::format("%6s ", w3g->formatTime(actionList->actions[sel].time));
          if (actionList->actions[sel].player)
            result += String::format("<%s> ", actionList->actions[sel].player->name);
          result += actionList->transform(actionList->actions[sel].text);
        }
        SetClipboard(CF_UNICODETEXT, CreateGlobalText(result));
      }
    }
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_LOADREPLAY:
      parseReplay();
      break;
    case IDC_RAWCODES:
      actionList->rawCodes = rawCodes->checked();
      actionList->invalidate();
      break;
    case IDC_SEARCHNEXT:
    case IDC_SEARCHPREV:
      {
        int mode = searchMode->getCurSel();
        int spnum = searchPlayer->getCurSel();
        uint32 pid = (spnum < 0 ? -1 : searchPlayer->getItemData(spnum));
        W3GPlayer* player = (pid == -1 ? NULL : (W3GPlayer*) pid);
        String text = searchText->getText();
        int dir = (LOWORD(wParam) == IDC_SEARCHNEXT ? 1 : -1);
        RegExp* re = (mode == 3 ? new RegExp(text, REGEXP_CASE_INSENSITIVE) : NULL);

        bool match = false;
        int cur = ListView_GetNextItem(actionList->getHandle(), -1, LVNI_SELECTED);
        for (int i = 0; i < actionList->actions.length() && !match; i++)
        {
          cur += dir;
          if (cur < 0) cur = 0;
          if (cur > actionList->actions.length()) cur = actionList->actions.length() - 1;

          match = true;
          if (pid != -1)
            match = (actionList->actions[cur].player == player);
          if (match)
          {
            String line = actionList->transform(actionList->actions[cur].text);
            if (mode == 0)
              match = (line.ifind(text) >= 0);
            else if (mode == 1)
              match = (line.icompare(text) == 0);
            else if (mode == 2)
              match = (line.substring(0, line.fromUtfPos(
                text.getUtfLength())).icompare(text) == 0);
            else if (mode == 3)
              match = (re->find(line) >= 0);
          }
        }
        if (match)
        {
          for (int sel = ListView_GetNextItem(actionList->getHandle(), -1, LVNI_SELECTED);
              sel >= 0; sel = ListView_GetNextItem(actionList->getHandle(), sel, LVNI_SELECTED))
            ListView_SetItemState(actionList->getHandle(), sel, 0, LVIS_SELECTED);
          ListView_SetItemState(actionList->getHandle(), cur, LVIS_SELECTED, LVIS_SELECTED);
          ListView_EnsureVisible(actionList->getHandle(), cur, FALSE);
          SetFocus(actionList->getHandle());
        }
        else
          MessageBox(actionList->getHandle(), "Nothing found!", "Error", MB_OK);

        delete re;
      }
      break;
    }
    break;
  default:
    return M_UNHANDLED;
  }
  return 0;
}
