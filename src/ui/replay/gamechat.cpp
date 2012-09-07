#include "core/app.h"

#include "gamechat.h"
#include "frameui/controlframes.h"

#include "base/utils.h"
#include "base/regexp.h"
#include "dota/colors.h"

#define IDC_SEARCH_MODE       1098
#define IDC_SEARCH_TEXT       1099
#define IDC_SEARCH_PLAYER     1100
#define IDC_SEARCH_PLAYERE    1101
#define IDC_SEARCH_PREV       1102
#define IDC_SEARCH_NEXT       1103
#define IDC_CHAT_FILTERS      1104

#define SEARCH_CONTAINS       0
#define SEARCH_EQUALS         1
#define SEARCH_STARTSWITH     2
#define SEARCH_REGEXP         3
#define SEARCH_PLAYERCHAT     4
#define SEARCH_PLAYEREVENT    5

ReplayGameChatTab::ReplayGameChatTab(Frame* parent)
  : ReplayTab(parent)
{
  searchMode = new ComboFrame(this, IDC_SEARCH_MODE);

  searchText = new EditFrame(this, IDC_SEARCH_TEXT);
  searchPlayer = new ComboFrameEx(this, IDC_SEARCH_PLAYER);
  searchPlayerEvent = new ComboFrameEx(this, IDC_SEARCH_PLAYERE);
  searchPrev = new ButtonFrame("Find prev", this, IDC_SEARCH_PREV);
  searchNext = new ButtonFrame("Find next", this, IDC_SEARCH_NEXT);
  chatFilters = new ButtonFrame("Filters", this, IDC_CHAT_FILTERS);

  searchMode->setWidth(120);
  searchText->setHeight(21);
  searchPrev->setSize(70, 21);
  searchNext->setSize(70, 21);
  chatFilters->setSize(70, 21);

  searchMode->setPoint(PT_TOPLEFT, 10, 10);
  searchText->setPoint(PT_TOPLEFT, searchMode, PT_TOPRIGHT, 5, 0);
  searchPlayer->setPoint(PT_TOPLEFT, searchMode, PT_TOPRIGHT, 5, 0);
  searchPlayerEvent->setPoint(PT_TOPLEFT, searchMode, PT_TOPRIGHT, 5, 0);
  chatFilters->setPoint(PT_TOPRIGHT, -10, 10);
  searchPrev->setPoint(PT_TOPRIGHT, chatFilters, PT_TOPLEFT, -5, 0);
  searchNext->setPoint(PT_TOPRIGHT, searchPrev, PT_TOPLEFT, -5, 0);
  searchText->setPoint(PT_TOPRIGHT, searchNext, PT_TOPLEFT, -5, 0);
  searchPlayer->setPoint(PT_TOPRIGHT, searchNext, PT_TOPLEFT, -5, 0);
  searchPlayerEvent->setPoint(PT_TOPRIGHT, searchNext, PT_TOPLEFT, -5, 0);

  searchPlayer->hide();
  searchPlayerEvent->hide();

  chatFrame = new RichEditFrame(this);
  chatFrame->setPoint(PT_TOPLEFT, searchMode, PT_BOTTOMLEFT, 0, 5);
  chatFrame->setPoint(PT_BOTTOMRIGHT, -10, -10);
}
ReplayGameChatTab::~ReplayGameChatTab()
{
}
static void add_color(String& text, uint32 clr)
{
  text.printf("\\red%d\\green%d\\blue%d;", clr & 0xFF, (clr >> 8) & 0xFF, (clr >> 16) & 0xFF);
}
String ReplayGameChatTab::sanitize(String str)
{
  String result = "";
  for (int i = 0; i < str.length(); i++)
  {
    short wcode = str[i];
    if ((wcode & 0x80) == 0)
    {
      if (wcode == '{' || wcode == '}' || wcode == '\\')
        result += '\\';
      result += char(wcode);
    }
    else
    {
      if ((wcode & 0xF0) == 0xF0)
      {
        while (wcode & 0x40)
        {
          i++;
          wcode <<= 1;
        }
        continue;
      }
      if (wcode & 0x20)
      {
        wcode = ((wcode & 0x0F) << 6) | (str[++i] & 0x3F);
        wcode = (wcode << 6) | (str[++i] & 0x3F);
      }
      else
        wcode = ((wcode & 0x1F) << 6) | (str[++i] & 0x3F);
      result.printf("\\u%d?", int(wcode));
    }
  }
  return result;
}
String ReplayGameChatTab::sanitizePlayer(W3GPlayer* player)
{
  String name = sanitize(player->name);
  if (player->hero && cfg.chatHeroes)
    return String::format("%s (%s)", name, player->hero->hero->shortName);
  else
    return name;
}
String ReplayGameChatTab::sanitizeNotify(String str)
{
  String text = sanitize(str);
  String result = "";
  for (int i = 0; i < text.length(); i++)
  {
    if (text[i] == '@')
    {
      i++;
      switch (text[i])
      {
      case 's':
        result += "{\\cf1 The Sentinel}";
        break;
      case 'u':
        result += "{\\cf7 The Scourge}";
        break;
      case 'n':
        result += "{\\cf13 Neutral Creeps}";
        break;
      case 'r':
        switch (text[i + 1])
        {
        case '1':
          result += "{\\cf15 Haste}";
          break;
        case '2':
          result += "{\\cf16 Regeneration}";
          break;
        case '3':
          result += "{\\cf17 Double Damage}";
          break;
        case '4':
          result += "{\\cf18 Illusion}";
          break;
        case '5':
          result += "{\\cf19 Invisibility}";
          break;
        }
        break;
      case 'm':
        switch (text[i + 1])
        {
        case '3':
          result += "is on a {\\cf20 killing spree}!";
          break;
        case '4':
          result += "is {\\cf21 dominating}!";
          break;
        case '5':
          result += "has a {\\cf22 mega kill}!";
          break;
        case '6':
          result += "is {\\cf23 unstoppable}!!";
          break;
        case '7':
          result += "is {\\cf24 wicked sick}!!";
          break;
        case '8':
          result += "has a {\\cf25 monster kill}!!";
          break;
        case '9':
          result += "is {\\cf26 GODLIKE}!!!";
          break;
        case '0':
          result += "is {\\cf27 beyond GODLIKE}. Someone KILL HIM!!!!!!";
          break;
        }
        break;
      case 'k':
        switch (text[i + 1])
        {
        case '2':
          result += "just got a {\\cf28 Double Kill}!";
          break;
        case '3':
          result += "just got a {\\cf29 Triple Kill}!!!";
          break;
        case '4':
          result += "just got a {\\cf30 Ultra Kill}!!!";
          break;
        case '5':
          result += "is on a {\\cf31 Rampage}!!!";
          break;
        }
        break;
      default:
        {
          int id = 0;
          int pos = i;
          while (text[pos] >= '0' && text[pos] <= '9')
            id = id * 10 + int(text[pos++] - '0');
          int color = w3g->getPlayerById(id)->slot.color;
          if (text[pos] == '|')
          {
            color = 0;
            pos++;
            while (text[pos] >= '0' && text[pos] <= '9')
              color = color * 10 + int(text[pos++] - '0');
          }
          if (color)
            result.printf("{\\cf%d ", color + 1);
          result += sanitizePlayer(w3g->getPlayerById(id));
          if (color)
            result += "}";
        }
        break;
      }
      while (text[i] != '@')
        i++;
    }
    else
      result += text[i];
  }
  return result;
}
void ReplayGameChatTab::refill()
{
  lines.clear();
  if (w3g == NULL)
  {
    chatFrame->setText("");
    return;
  }
  String text = "{\\rtf1\\ansi{\\fonttbl\\f0\\f";
  if (cfg.chatFont.lfPitchAndFamily == FF_DECORATIVE)
    text += "decor";
  else if (cfg.chatFont.lfPitchAndFamily == FF_MODERN)
    text += "modern";
  else if (cfg.chatFont.lfPitchAndFamily == FF_ROMAN)
    text += "roman";
  else if (cfg.chatFont.lfPitchAndFamily == FF_SCRIPT)
    text += "script";
  else if (cfg.chatFont.lfPitchAndFamily == FF_SWISS)
    text += "swiss";
  else
    text += "nil";
  text.printf(" %s;}{\\colortbl;", cfg.chatFont.lfFaceName);
  for (int i = 0; i < 13; i++)
  {
    if (cfg.chatColors == 0)
      add_color(text, getDefaultColor(i));
    else if (cfg.chatColors == 1)
      add_color(text, getSlotColor(i));
    else
      add_color(text, getDarkColor(i));
  }
  add_color(text, cfg.chatFg);
  for (int i = 15; i <= 31; i++)
    add_color(text, getExtraColor(i));
  HDC hDC = GetDC(NULL);
  int fontSize = -MulDiv(cfg.chatFont.lfHeight, 144, GetDeviceCaps(hDC, LOGPIXELSY));
  ReleaseDC(NULL, hDC);
  text.printf("}\\f0\\cf14\\fs%d\\pard", fontSize);
  if (cfg.chatFont.lfUnderline)
    text += "\\ul";
  if (cfg.chatFont.lfItalic)
    text += "\\i";
  if (cfg.chatFont.lfStrikeOut)
    text += "\\strike";
  if (cfg.chatFont.lfWeight > 400)
    text += "\\b";
  text += "\n";

  uint32 prevTime = 0x7FFFFFFF;

  for (int i = 0; i < w3g->getNumMessages(); i++)
  {
    W3GMessage& msg = w3g->getMessage(i);
    if (!FilterMessage(msg))
      continue;
    String line = "";
    switch (msg.mode)
    {
    case CHAT_ALL:
      line.printf("%s\t [All] {\\ul\\cf%d %s:} %s\\line\n",
        w3g->formatTime(msg.time), w3g->getPlayerById(msg.id)->slot.color + 1,
        sanitizePlayer(w3g->getPlayerById(msg.id)), sanitize(msg.text));
      break;
    case CHAT_ALLIES:
      line.printf("%s\t [Allies] {\\ul\\cf%d %s:} %s\\line\n",
        w3g->formatTime(msg.time), w3g->getPlayerById(msg.id)->slot.color + 1,
        sanitizePlayer(w3g->getPlayerById(msg.id)), sanitize(msg.text));
      break;
    case CHAT_OBSERVERS:
      line.printf("%s\t [Observers] {\\ul\\cf%d %s:} %s\\line\n",
        w3g->formatTime(msg.time), w3g->getPlayerById(msg.id)->slot.color + 1,
        sanitizePlayer(w3g->getPlayerById(msg.id)), sanitize(msg.text));
      break;
    case CHAT_PRIVATE:
      line.printf("%s\t [Private] {\\ul\\cf%d %s:} %s\\line\n",
        w3g->formatTime(msg.time), w3g->getPlayerById(msg.id)->slot.color + 1,
        sanitizePlayer(w3g->getPlayerById(msg.id)), sanitize(msg.text));
      break;
    case CHAT_COMMAND:
      line.printf("%s\t [Game Command] {\\ul\\cf%d %s:} %s\\line\n",
        w3g->formatTime(msg.time), w3g->getPlayerById(msg.id)->slot.color + 1,
        sanitizePlayer(w3g->getPlayerById(msg.id)), sanitize(msg.text));
      break;
    case CHAT_NOTIFY:
      line.printf("%s\t %s\\line\n",
        w3g->formatTime(msg.time), sanitizeNotify(msg.text));
      break;
    }
    if (!line.isEmpty())
    {
      if (msg.time > prevTime + 30000)
      {
        text += "\\line\n";
        lines.push(NULL);
      }
      prevTime = msg.time;
      text += line;
      lines.push(&msg);
    }
  }

  text += "}";

  chatFrame->setBackgroundColor(cfg.chatBg);
  chatFrame->setRichText(text);
}
void ReplayGameChatTab::onSetReplay()
{
  int sMode = searchMode->getCurSel();
  if (sMode < 0)
    sMode = 0;
  searchMode->reset();
  searchPlayer->reset();
  searchPlayerEvent->reset();
  searchMode->enable(w3g != NULL);
  searchPlayer->enable(w3g != NULL);
  searchText->enable(w3g != NULL);
  searchNext->enable(w3g != NULL);
  searchPrev->enable(w3g != NULL);
  chatFilters->enable(w3g != NULL);
  if (w3g)
  {
    searchMode->addString("Text contains");
    searchMode->addString("Text equals");
    searchMode->addString("Text starts with");
    searchMode->addString("Regular expression");
    searchMode->addString("Player chat");
    if (w3g->getDotaInfo())
      searchMode->addString("Player event");

    searchMode->setCurSel(0);
    if (DotaInfo const* dota = w3g->getDotaInfo())
    {
      for (int team = 0; team < 2; team++)
      {
        searchPlayer->addString(team == 0 ? "The Sentinel" : "The Scourge",
          0, team == 0 ? "RedBullet" : "GreenBullet", 0);
        searchPlayerEvent->addString(team == 0 ? "The Sentinel" : "The Scourge",
          0xFFFFFF, team == 0 ? "RedBullet" : "GreenBullet", -team - 1);
        for (int i = 0; i < dota->team_size[team]; i++)
        {
          W3GPlayer* player = dota->teams[team][i];
          searchPlayer->addString(player->name, getLightColor(player->slot.color),
            player->hero ? player->hero->hero->icon : "Empty", player->slot.color);
          searchPlayerEvent->addString(player->name, getLightColor(player->slot.color),
            player->hero ? player->hero->hero->icon : "Empty", player->slot.color);
        }
      }
      searchPlayerEvent->addString("Neutral Creeps",
        0xFFFFFF, "random", -3);
    }
    else
    {
      for (int i = 0; i < w3g->getNumPlayers(); i++)
      {
        W3GPlayer* player = w3g->getPlayer(i);
        searchPlayer->addString(player->name, getLightColor(player->slot.color),
          getRaceIcon(player->race), player->slot.color);
        searchPlayerEvent->addString(player->name, getLightColor(player->slot.color),
          getRaceIcon(player->race), player->slot.color);
      }
    }

    searchMode->setCurSel(sMode);
    searchPlayer->setCurSel(0);
    searchPlayerEvent->setCurSel(0);
  }
  refill();
}

uint32 ReplayGameChatTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND)
  {
    switch (LOWORD(wParam))
    {
    case IDC_CHAT_FILTERS:
      DialogBoxParam(getInstance(), MAKEINTRESOURCE(IDD_CHATFILTERS), getApp()->getMainWindow(),
        FiltersDlgProc, w3g == NULL || w3g->getDotaInfo() == NULL);
      onSetReplay();
      return 0;
    case IDC_SEARCH_MODE:
      if (searchMode->getCurSel() == SEARCH_PLAYERCHAT)
      {
        searchText->hide();
        searchPlayer->show();
        searchPlayerEvent->hide();
        wParam = MAKELONG(IDC_SEARCH_PLAYER, CBN_SELCHANGE);
      }
      else if (searchMode->getCurSel() == SEARCH_PLAYEREVENT)
      {
        searchText->hide();
        searchPlayer->hide();
        searchPlayerEvent->show();
        wParam = MAKELONG(IDC_SEARCH_PLAYERE, CBN_SELCHANGE);
      }
      else
      {
        searchText->show();
        searchPlayer->hide();
        searchPlayerEvent->hide();
        return 0;
      }
    case IDC_SEARCH_PLAYER:
    case IDC_SEARCH_PLAYERE:
      {
        ComboFrameEx* src = (LOWORD(wParam) == IDC_SEARCH_PLAYER ?
          searchPlayer : searchPlayerEvent);
        ComboFrameEx* dst = (LOWORD(wParam) == IDC_SEARCH_PLAYER ?
          searchPlayerEvent : searchPlayer);
        int sel = src->getCurSel();
        int dsel = 0;
        if (sel >= 0)
        {
          int id = src->getItemData(sel);
          for (int i = 0; i < dst->getCount(); i++)
            if (dst->getItemData(i) == id)
              dsel = i;
        }
        dst->setCurSel(dsel);
      }
      return 0;
    case IDC_SEARCH_NEXT:
    case IDC_SEARCH_PREV:
      {
        int line = SendMessage(chatFrame->getHandle(), EM_LINEFROMCHAR, -1, 0);
        int dir = (LOWORD(wParam) == IDC_SEARCH_NEXT ? 1 : -1);
        int count = 0;

        int mode = searchMode->getCurSel();
        String text = "";
        RegExp* re = NULL;
        if (mode == SEARCH_CONTAINS || mode == SEARCH_EQUALS ||
            mode == SEARCH_STARTSWITH || mode == SEARCH_REGEXP)
        {
          text = searchText->getText();
          if (mode == SEARCH_REGEXP)
            re = new RegExp(text, REGEXP_CASE_INSENSITIVE);
        }
        else if (mode == SEARCH_PLAYERCHAT || mode == SEARCH_PLAYEREVENT)
        {
          ComboFrameEx* box = (mode == SEARCH_PLAYERCHAT ?
            searchPlayer : searchPlayerEvent);
          int sel = box->getCurSel();
          if (sel >= 0)
          {
            int id = box->getItemData(sel);
            if (id >= 0)
              text.printf("@%d@", id);
            else if (id == -1)
              text = "@s@";
            else if (id == -2)
              text = "@u@";
            else if (id == -3)
              text = "@n@";
          }
        }

        while (count < lines.length())
        {
          line += dir;
          if (line < 0) line = lines.length() - 1;
          if (line >= lines.length()) line = 0;

          if (lines[line])
          {
            if (mode == SEARCH_CONTAINS)
            {
              if (lines[line]->text.ifind(text) >= 0)
                break;
            }
            else if (mode == SEARCH_EQUALS)
            {
              if (lines[line]->text.icompare(text) == 0)
                break;
            }
            else if (mode == SEARCH_STARTSWITH)
            {
              if (lines[line]->text.substring(0, lines[line]->text.fromUtfPos(
                  text.getUtfLength())).icompare(text) == 0)
                break;
            }
            else if (mode == SEARCH_REGEXP)
            {
              if (re->find(lines[line]->text) >= 0)
                break;
            }
            else if (mode == SEARCH_PLAYERCHAT)
            {
              if (lines[line]->mode != CHAT_NOTIFY && lines[line]->text.find(text))
                break;
            }
            else if (mode == SEARCH_PLAYEREVENT)
            {
              if (lines[line]->mode == CHAT_NOTIFY && lines[line]->text.find(text))
                break;
            }
          }

          count++;
        }

        if (count < lines.length())
        {
          int start = SendMessage(chatFrame->getHandle(), EM_LINEINDEX, line, 0);
          int end = SendMessage(chatFrame->getHandle(), EM_LINEINDEX, line + 1, 0) - 1;
          if (start >= 0 && end >= 0)
          {
            SendMessage(chatFrame->getHandle(), EM_SETSEL, start, end);
            SetFocus(chatFrame->getHandle());
          }
        }
      }
      return 0;
    }
  }
  return M_UNHANDLED;
}
