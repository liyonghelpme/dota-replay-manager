#include "stdafx.h"
#include "sparser.h"
#include "gamecache.h"
#include "resource.h"
#include "chatfilters.h"

extern int _chatFilters;
extern bool chatHeroes;
String makeWebColor (DWORD clr);

String mkPlayerName (W3GPlayer const& p)
{
  DotaHero* hero = NULL;
  if (p.hero)
    hero = getHero (p.hero->id);
  if (chatHeroes && hero)
    return String::printf ("%s (%s)", p.name, hero->abbr);
  else
    return p.name;
}
extern bool relTime;
String fmtTime (W3GReplay* w3g, unsigned long time)
{
  int t = int (time);
  if (relTime)
    t -= int (w3g->game.startTime);
  int m = abs (t) / 60000;
  int s = (abs (t) / 1000) % 60;
  if (t < 0)
    return String::printf ("-%d:%02d", m, s);
  else
    return String::printf ("%02d:%02d", m, s);
}

String ScriptParser::fmtChat ()
{
  String res = "";
  for (int m = 0; m < w3g->chat.getSize (); m++)
  {
    W3GMessage& msg = w3g->chat[m];
    if (CChatFilters::isMessageAllowed (_chatFilters, msg.mode, msg.notifyType))
    {
      if (msg.mode == CHAT_ALL || msg.mode == CHAT_ALLIES || msg.mode == CHAT_OBSERVERS ||
        msg.mode == CHAT_PRIVATE || msg.mode == CHAT_COMMAND)
      {
        res += String::printf ("[font=\"Courier New\"]%s [/font]", fmtTime (w3g, msg.time));
        if (msg.mode == CHAT_ALL)
          res += "[All] ";
        else if (msg.mode == CHAT_ALLIES)
          res += "[Allies] ";
        else if (msg.mode == CHAT_OBSERVERS)
          res += "[Observers] ";
        else if (msg.mode == CHAT_PRIVATE)
          res += "[Private] ";
        else if (msg.mode == CHAT_COMMAND)
          res += "[Game Command] ";
        res += String::printf ("[color=%s]%s:[/color] ",
          makeWebColor (getDefaultColor (w3g->players[msg.id].slot.color)), mkPlayerName (w3g->players[msg.id]));
        res += msg.text;
        res += "\r\n";
      }
      else if (msg.mode == CHAT_NOTIFY)
      {
        res += String::printf ("[font=\"Courier New\"]%s [/font]", fmtTime (w3g, msg.time));
        for (int i = 0; msg.text[i];)
        {
          if (msg.text[i] == '@')
          {
            if (msg.text[i + 1] == 's' || msg.text[i + 1] == 'u' || msg.text[i + 1] == 'n')
            {
              i++;
              if (msg.text[i] == 's')
                res += String::printf ("[color=%s]The Sentinel[/color]", makeWebColor (getDefaultColor (0)));
              else if (msg.text[i] == 'u')
                res += String::printf ("[color=%s]The Scourge[/color]", makeWebColor (getDefaultColor (6)));
              else if (msg.text[i] == 'n')
                res += String::printf ("[color=%s]Neutral Creeps[/color]", makeWebColor (getDefaultColor (12)));
              i += 2;
            }
            else if (msg.text[i + 1] == 'r')
            {
              i += 2;
              if (msg.text[i] == '1')
                res += String::printf ("[color=%s]Haste[/color]", makeWebColor (getExtraColor (15)));
              else if (msg.text[i] == '2')
                res += String::printf ("[color=%s]Regeneration[/color]", makeWebColor (getExtraColor (16)));
              else if (msg.text[i] == '3')
                res += String::printf ("[color=%s]Double Damage[/color]", makeWebColor (getExtraColor (17)));
              else if (msg.text[i] == '4')
                res += String::printf ("[color=%s]Illusion[/color]", makeWebColor (getExtraColor (18)));
              else if (msg.text[i] == '5')
                res += String::printf ("[color=%s]Invisibility[/color]", makeWebColor (getExtraColor (19)));
              i += 2;
            }
            else if (msg.text[i + 1] == 'm')
            {
              i += 2;
              if (msg.text[i] == '3')
                res += String::printf ("is on a [color=%s]killing spree[/color]!", makeWebColor (getExtraColor (20)));
              else if (msg.text[i] == '4')
                res += String::printf ("is [color=%s]dominating[/color]!", makeWebColor (getExtraColor (21)));
              else if (msg.text[i] == '5')
                res += String::printf ("has a [color=%s]mega kill[/color]!", makeWebColor (getExtraColor (22)));
              else if (msg.text[i] == '6')
                res += String::printf ("is [color=%s]unstoppable[/color]!!", makeWebColor (getExtraColor (23)));
              else if (msg.text[i] == '7')
                res += String::printf ("is [color=%s]wicked sick[/color]!!", makeWebColor (getExtraColor (24)));
              else if (msg.text[i] == '8')
                res += String::printf ("has a [color=%s]monster kill[/color]!!", makeWebColor (getExtraColor (25)));
              else if (msg.text[i] == '9')
                res += String::printf ("is [color=%s]GODLIKE[/color]!!!", makeWebColor (getExtraColor (26)));
              else if (msg.text[i] == '0')
                res += String::printf ("is [color=%s]beyond GODLIKE[/color]. Someone KILL HIM!!!!!!", makeWebColor (getExtraColor (27)));
              i += 2;
            }
            else if (msg.text[i + 1] == 'k')
            {
              i += 2;
              if (msg.text[i] == '2')
                res += String::printf ("just got a [color=%s]Double Kill[/color]!", makeWebColor (getExtraColor (28)));
              else if (msg.text[i] == '3')
                res += String::printf ("just got a [color=%s]Triple Kill[/color]!!!", makeWebColor (getExtraColor (29)));
              else if (msg.text[i] == '4')
                res += String::printf ("just got a [color=%s]Ultra Kill[/color]!!!", makeWebColor (getExtraColor (30)));
              else if (msg.text[i] == '5')
                res += String::printf ("is on a [color=%s]Rampage[/color]!!!", makeWebColor (getExtraColor (31)));
              i += 2;
            }
            else
            {
              int id = 0;
              for (i++; msg.text[i] != '@' && msg.text[i] != '|'; i++)
                id = id * 10 + msg.text[i] - '0';
              int clr = (id >= 0 && id <= 255 && w3g->players[id].name[0])
                ? w3g->players[id].slot.color
                : 0;
              if (msg.text[i] == '|')
              {
                clr = 0;
                for (i++; msg.text[i] != '@'; i++)
                  clr = clr * 10 + msg.text[i] - '0';
              }
              if (clr)
                res += String::printf ("[color=%s]", makeWebColor (getDefaultColor (clr)));
              for (i++; msg.text[i] && msg.text[i] != ' ' && msg.text[i] != '\t' && msg.text[i] != '/'; i++)
                res += msg.text[i];
              if (id >= 0 && id <= 255 && w3g->players[id].name[0])
              {
                DotaHero* hero = NULL;
                if (w3g->players[id].hero)
                  hero = getHero (w3g->players[id].hero->id);
                if (chatHeroes && hero)
                  res += String::printf (" (%s)", hero->abbr);
              }
              if (clr)
                res += "[/color]";
            }
          }
          else
            res += msg.text[i++];
        }
        res += "\r\n";
      }
    }
  }
  return res;
}
