#include "core/app.h"

#include "gamechat.h"
#include "frameui/controlframes.h"

#include "base/utils.h"
#include "dota/colors.h"

ReplayGameChatTab::ReplayGameChatTab(FrameWindow* parent)
  : ReplayTab(parent)
{
  chatFrame = new RichEditFrame(this);
  chatFrame->setPoint(PT_TOPLEFT, 10, 10);
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
  if (player->hero && cfg::chatHeroes)
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
void ReplayGameChatTab::onSetReplay()
{
  Registry* reg = getApp()->getRegistry();
  LOGFONT const& chatFont = cfg::chatFont.get<LOGFONT>();
  String text = "{\\rtf1\\ansi{\\fonttbl\\f0\\f";
  if (chatFont.lfPitchAndFamily == FF_DECORATIVE)
    text += "decor";
  else if (chatFont.lfPitchAndFamily == FF_MODERN)
    text += "modern";
  else if (chatFont.lfPitchAndFamily == FF_ROMAN)
    text += "roman";
  else if (chatFont.lfPitchAndFamily == FF_SCRIPT)
    text += "script";
  else if (chatFont.lfPitchAndFamily == FF_SWISS)
    text += "swiss";
  else
    text += "nil";
  text.printf(" %s;}{\\colortbl;", chatFont.lfFaceName);
  for (int i = 0; i < 13; i++)
  {
    if (cfg::chatColors == 0)
      add_color(text, getDefaultColor(i));
    else if (cfg::chatColors == 1)
      add_color(text, getSlotColor(i));
    else
      add_color(text, getDarkColor(i));
  }
  add_color(text, cfg::chatFg);
  for (int i = 15; i <= 31; i++)
    add_color(text, getExtraColor(i));
  HDC hDC = GetDC(NULL);
  int fontSize = -MulDiv(chatFont.lfHeight, 144, GetDeviceCaps(hDC, LOGPIXELSY));
  ReleaseDC(NULL, hDC);
  text.printf("}\\f0\\cf14\\fs%d\\pard", fontSize);
  if (chatFont.lfUnderline)
    text += "\\ul";
  if (chatFont.lfItalic)
    text += "\\i";
  if (chatFont.lfStrikeOut)
    text += "\\strike";
  if (chatFont.lfWeight > 400)
    text += "\\b";
  text += "\n";

  int numLines = 0;
  uint32 prevTime = 0x7FFFFFFF;

  for (int i = 0; i < w3g->getNumMessages(); i++)
  {
    W3GMessage& msg = w3g->getMessage(i);
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
        numLines++;
      }
      prevTime = msg.time;
      text += line;
      msg.line = numLines++;
    }
  }

  text += "}";

  chatFrame->setBackgroundColor(cfg::chatBg);
  chatFrame->setRichText(text);
}
