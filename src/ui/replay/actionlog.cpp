#include "core/app.h"

#include <stdio.h>
#include <stdarg.h>

#include "base/regexp.h"
#include "dota/colors.h"
#include "actionlog.h"

#include "replay/orders.h"
#include "replay/action.h"
#include "frameui/dragdrop.h"

#define IDC_LOADREPLAY        120
#define IDC_RAWCODES          121
#define IDC_SEARCHMODE        122
#define IDC_SEARCHPLAYER      123
#define IDC_SEARCHTEXT        124
#define IDC_SEARCHNEXT        125
#define IDC_SEARCHPREV        126
#define IDC_ACTIONLIST        127

class ActionListFrame : public ListFrame
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

struct ReplayActionLogTab::ParseState
{
  W3GActionParser parser;
  uint32 time;
  bool pause;
  int version;

  W3GReplay* w3g;
  W3GPlayer* player;

  ActionListFrame* actionList;

  ParseState(int _version)
    : parser(_version)
  {
    version = _version;
    pause = false;
    time = 0;
  }

  void add(char const* fmt, bool noPlayer = false);
};
void ReplayActionLogTab::ParseState::add(char const* fmt, bool noPlayer)
{
  int last_arg = -1;
  String text = String::format("0x%02X: ", parser.type());
  while (*fmt)
  {
    if (*fmt == '%')
    {
      fmt++;
      char c = *fmt++;
      int arg = 0;
      if (c != 'n')
      {
        if (*fmt >= '0' && *fmt <= '9')
        {
          while (*fmt >= '0' && *fmt <= '9')
            arg = arg * 10 + int(*fmt++ - '0');
        }
        else
          arg = last_arg + 1;
        last_arg = arg;
      }

      if (c == 'n')
        text += parser.name();
      else if (c == 'a')
      {
        uint32 id = parser.arg_int(arg);
        if (actionList->mapData->getData()->getUnitById(id) || isGoodId(id))
          text += String::format("[%s]", id2String(id));
        else
        {
          char const* oid = orderId2String(id);
          if (oid)
            text += oid;
          else
            text += String::format("0x%08X", id);
        }
      }
      else if (c == 'u')
      {
        uint32 id = parser.arg_int(arg);
        if (actionList->mapData->getData()->getUnitById(id) || isGoodId(id))
          text += String::format("[%s]", id2String(id));
        else
          text += String((int) id);
      }
      else if (c == 'i')
        text += String((int) parser.arg_int(arg));
      else if (c == 'f')
        text += String::format("%.2f", parser.arg_float(arg));
      else if (c == 's')
        text += parser.arg_string(arg);
      else if (c == 'p')
      {
        W3GPlayer* p = w3g->getPlayerById(parser.arg_int(arg));
        if (p)
          text += p->name;
        else
          text += "(unknown)";
      }
      else if (c == 'c')
      {
        int key = parser.arg_int(arg);
        if (c == 9)
          text += '0';
        else
          text += char(key + '1');
      }
      else if (c == 'x')
        text += String::format("0x%04X", parser.arg_int(arg));
      else if (c == 'X')
        text += String::format("0x%08X", parser.arg_int(arg));
      else if (c == 'Y')
      {
        uint64 i64 = parser.arg_int64(arg);
        text += String::format("0x%08X%08X", int(i64 >> 32), int(i64));
      }
    }
    else
      text += *fmt++;
  }

  actionList->addAction(time, noPlayer ? NULL : player, text);
}
void ReplayActionLogTab::parseActions(File* file, int length, ParseState& state)
{
  W3GActionParser& parser = state.parser;
  int end = file->tell() + length;
  while (file->tell() < end)
  {
    uint8 id = file->getc();
    state.player = w3g->getPlayerById(id);
    int length = file->read_int16();
    int next = file->tell() + length;

    while (file->tell() < next)
    {
      uint8 type = parser.parse(file);

      switch (type)
      {
      // Pause game
      case 0x01:
        state.pause = true;
        state.add("%n");
        break;
      // Resume game
      case 0x02:
        state.pause = false;
        state.add("%n");
        break;
      // Set game speed in single player game (options menu)
      case 0x03:
        state.add("Set game speed to %i");
        break;
      // Increase game speed in single player game (Num+)
      case 0x04:
      // Decrease game speed in single player game (Num-)
      case 0x05:
        state.add("%n");
        break;
      // Save game
      case 0x06:
        state.add("Saving game to %s");
        break;
      // Save game finished
      case 0x07:
        state.add("%n");
        break;
      // Unit/building ability (no additional parameters)
      case 0x10:
        state.add("Immediate order: %a1 (flags: %x0)");
        break;
      // Unit/building ability (with target position)
      case 0x11:
        if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
          state.add("Point order: %a1 (flags: %x0)");
        else
          state.add("Point order: %a1 (X: %f3, Y: %f4, flags: %x0)");
        break;
      // Unit/building ability (with target position and target object ID)
      case 0x12:
        if (parser.arg_int64(5) == -1)
        {
          if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
            state.add("Target order: %a1 (flags: %x0)");
          else
            state.add("Target order: %a1 (X: %f3, Y: %f4, flags: %x0)");
        }
        else if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
          state.add("Target order: %a1 (Target: %Y5, flags: %x0)");
        else
          state.add("Target order: %a1 (X: %f3, Y: %f4, Target: %Y5, flags: %x0)");
        break;
      // Give item to Unit / Drop item on ground
      case 0x13:
        if (parser.arg_int64(5) == -1)
        {
          if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
            state.add("Target+object order: %a1 (Object: %Y6, flags: %x0)");
          else
            state.add("Target+object order: %a1 (X: %f3, Y: %f4, Object: %Y6, flags: %x0)");
        }
        else if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
          state.add("Target+object order: %a1 (Target: %Y5, Object: %Y6, flags: %x0)");
        else
          state.add("Target+object order: %a1 (X: %f3, Y: %f4, Target: %Y5, Object: %Y6, flags: %x0)");
        break;
      // Unit/building ability (with two target positions and two item IDs)
      case 0x14:
        if (parser.arg_int(3) == -1 && parser.arg_int(4) == -1)
        {
          if (parser.arg_int(7) == -1 && parser.arg_int(8) == -1)
            state.add("Two points order: %a1, %a5, flags: %x0");
          else
            state.add("Two points order: %a1, %a5 (X: %f7, Y: %f8), flags: %x0");
        }
        else if (parser.arg_int(7) == -1 && parser.arg_int(8) == -1)
          state.add("Two points order: %a1 (X: %f3, Y: %f4), %a5, flags: %x0");
        else
          state.add("Two points order: %a1 (X: %f3, Y: %f4), %a5 (X: %f7, Y: %f8), flags: %x0");
        break;
      // Change Selection (Unit, Building, Area)
      case 0x16:
        if (parser.arg_int(1) == 0)
        {
          if (parser.arg_int(0) == 0x01)
            state.add("Add to selection");
          else
            state.add("Remove from selection");
        }
        else if (parser.arg_int(0) == 0x01)
          state.add("Add to selection: %Y2" + String(", %Y") * (parser.arg_int(1) - 1));
        else
          state.add("Remove to selection: %Y2" + String(", %Y") * (parser.arg_int(1) - 1));
        break;
      // Assign Group Hotkey
      case 0x17:
        if (parser.arg_int(1) == 0)
          state.add("Assign group %c");
        else
          state.add("Assign group %c: %Y2" + String(", %Y") * (parser.arg_int(1) - 1));
        break;
      // Select Group Hotkey
      case 0x18:
        state.add("Select group %c");
        break;
      // Select Subgroup
      case 0x19:
        if (state.version >= 14)
          state.add("Select subgroup: %a, %Y");
        else
          state.add("Select subgroup: %i");
        break;
      // Pre Subselection
      case 0x1A:
        state.add("%n");
        break;
      // Select Ground Item
      case 0x1C:
        state.add("Select ground item %Y1");
        break;
      // Cancel hero revival
      case 0x1D:
        state.add("Cancel hero revival %Y");
        break;
      // Remove unit from building queue
      case 0x1E:
        if (parser.arg_int(0) == 0)
          state.add("Cancel training/upgrading %a1");
        else
          state.add("Remove unit/upgrade #%i from queue (%a)");
        break;
      // Single Player Cheats
      case 0x20:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
        state.add("%n");
        break;
      case 0x27:
        state.add("Cheat: KeyserSoze (%i1 Gold)");
        break;
      case 0x28:
        state.add("Cheat: LeafitToMe (%i1 Lumber)");
        break;
      case 0x29:
      case 0x2A:
      case 0x2B:
      case 0x2C:
        state.add("%n");
        break;
      case 0x2D:
        state.add("Cheat: GreedIsGood (%i1 Gold and Lumber)");
        break;
      case 0x2E:
        state.add("Cheat: DayLightSavings (Set a time of day to %f)");
        break;
      case 0x2F:
      case 0x30:
      case 0x31:
      case 0x32:
        state.add("%n");
        break;
      // Change ally options
      case 0x50:
        {
          uint32 flags = parser.arg_int(1);
          String opts = "";
          if (flags & 0x1F)
          {
            if (!opts.isEmpty()) opts += ", ";
            opts += "allied";
          }
          if (flags & 0x20)
          {
            if (!opts.isEmpty()) opts += ", ";
            opts += "vision";
          }
          if (flags & 0x40)
          {
            if (!opts.isEmpty()) opts += ", ";
            opts += "control";
          }
          if (flags & 0x400)
          {
            if (!opts.isEmpty()) opts += ", ";
            opts += "victory";
          }
          if (opts.isEmpty())
            opts = "enemy";
          state.add("Change ally options towards %p: " + opts);
        }
        break;
      // Transfer resources
      case 0x51:
        state.add("Send %i1 gold and %i2 lumber to %p0");
        break;
      // Map trigger chat command (?)
      case 0x60:
        state.add("[Chat command] %s1");
        break;
      // ESC pressed
      case 0x61:
        state.add("%n");
        break;
      // Minimap signal (ping)
      case 0x68:
        state.add("Map ping (X: %f, Y: %f)");
        break;
      // SyncStoredInteger actions (endgame action)
      case 0x6B:
        state.add("SyncStoredInteger (%s, %s, %s, %u)", true);
        break;
      // SyncStored? action
      case 0x70:
        state.add("SyncStored? (%s, %s, %s)", true);
        break;
      }
    }

    file->seek(next, SEEK_SET);
  }
  file->seek(end, SEEK_SET);
}
void ReplayActionLogTab::parseBlocks(File* file)
{
  int version = vGetMinor(w3g->getVersion());
  ParseState state(version);
  state.actionList = actionList;
  state.player = NULL;
  state.w3g = w3g;
  while (!file->eof())
  {
    int block_id = file->getc();
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        int length = file->read_int16();
        int time_inc = file->read_int16();
        if (!state.pause)
          state.time += time_inc;
        if (length > 2)
          parseActions(file, length - 2, state);
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (version > 2)
      {
        uint8 id = file->getc();
        int length = file->read_int16();
        int end = file->tell() + length;
        uint8 flags = file->getc();
        if (flags == 0x20)
        {
          uint32 mode = file->read_int32();
          String text;
          text.resize(length - 6);
          file->read(text.getBuffer(), length - 6);
          text.setLength(length - 6);

          state.player = w3g->getPlayerById(id);
          if (mode == CHAT_ALL)
            actionList->addAction(state.time, state.player, "[All] " + text);
          else if (mode == CHAT_ALLIES)
            actionList->addAction(state.time, state.player, "[Allies] " + text);
          else if (mode == CHAT_OBSERVERS)
            actionList->addAction(state.time, state.player, "[Observers] " + text);
          else if (mode >= CHAT_PRIVATE)
            actionList->addAction(state.time, state.player, "[Private] " + text);
        }
        file->seek(end, SEEK_SET);
      }
      break;
    // unknown (Random number/seed for next frame)
    case 0x22:
      file->seek(file->getc(), SEEK_CUR);
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      file->seek(4, SEEK_CUR);
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      file->seek(10, SEEK_CUR);
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      file->seek(8, SEEK_CUR);
      break;
    // LeaveGame
    case 0x17:
      {
        int reason = file->read_int32();
        uint8 id = file->getc();
        int result = file->read_int32();
        int unknown = file->read_int32();

        actionList->addAction(state.time, w3g->getPlayerById(id), "Left the game");
      }
      break;
    case 0:
      file->seek(0, SEEK_END);
      break;
    }
  }
}

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

  parseBlocks(w3g->getFile());

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
        SetClipboard(CF_TEXT, CreateGlobalText(result));
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
