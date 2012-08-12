#include "stdafx.h"
#include "sparser.h"
#include "gamecache.h"

String ScriptReader::next ()
{
  while (isspace (script[cur]))
    cur++;
  if (script[cur] == ':' || script[cur] == '=')
  {
    curToken = "";
    curToken += script[cur++];
  }
  else if (script[cur] == 0 || script[cur] == '}')
    curToken = "";
  else if ((script[cur] >= 'a' && script[cur] <= 'z') || (script[cur] >= 'A' && script[cur] <= 'Z'))
  {
    curToken = "";
    while ((script[cur] >= 'a' && script[cur] <= 'z') || (script[cur] >= 'A' && script[cur] <= 'Z') ||
      (script[cur] >= '0' && script[cur] <= '9'))
      curToken += script[cur++];
  }
  else
    curToken = "";
  return curToken;
}

ScriptEnum::ScriptEnum (int num, String att, bool own)
{
  count = num;
  if (num)
  {
    scopes = new ScriptScope*[num];
    memset (scopes, 0, sizeof (ScriptScope*) * num);
  }
  else
    scopes = NULL;
  attach = att;
  ascope = new ScriptScope (attach, "");
  owned = own;
}
ScriptEnum::~ScriptEnum ()
{
  if (owned)
    for (int i = 0; i < count; i++)
      delete scopes[i];
  delete[] scopes;
  if (ascope->getReference () == NULL)
    delete ascope;
}

ScriptScope* ScriptScope::getChild (String child) const
{
  for (ScriptScope* cur = ref->firstChild; cur; cur = cur->next)
    if (cur->name == child)
      return cur;
  return NULL;
}
ScriptScope::ScriptScope (String n, String v)
{
  ref = this;
  next = NULL;
  prev = NULL;
  parent = NULL;
  firstChild = NULL;
  lastChild = NULL;
  list = NULL;
  name = n;
  value = v;
}
ScriptScope* ScriptScope::reference (String n)
{
  ScriptScope* r = new ScriptScope (n, value);
  r->ref = this;
  return r;
}
ScriptScope::~ScriptScope ()
{
  delete list;
  while (firstChild)
  {
    ScriptScope* n = firstChild->next;
    delete firstChild;
    firstChild = n;
  }
}
ScriptScope* ScriptScope::addChild (ScriptScope* child)
{
  child->prev = lastChild;
  if (lastChild)
    lastChild->next = child;
  else
    firstChild = child;
  lastChild = child;
  return child;
}
ScriptScope* ScriptScope::addChild (String name, String value)
{
  return addChild (new ScriptScope (name, value));
}
ScriptScope* ScriptScope::remChild (String child)
{
  for (ScriptScope* cur = firstChild; cur; cur = cur->next)
  {
    if (cur->name == child)
    {
      if (cur->prev)
        cur->prev->next = cur->next;
      else
        firstChild = cur->next;
      if (cur->next)
        cur->next->prev = cur->prev;
      else
        lastChild = cur->prev;
      return cur;
    }
  }
  return NULL;
}

ScriptScope* ScriptParser::eval (ScriptReader& reader)
{
  ScriptScope* scope = root->getChild (reader.token ());
  while (reader.next () == ":" && scope)
    scope = scope->getChild (reader.next ());
  while (!reader.token ().isEmpty ())
    reader.next ();
  return scope;
}
bool ScriptParser::ceval (ScriptReader& reader)
{
  bool inv = false;
  if (reader.token () == "not")
  {
    inv = true;
    reader.next ();
  }
  ScriptScope* scope = root->getChild (reader.token ());
  while (reader.next () == ":" && scope)
    scope = scope->getChild (reader.next ());
  if (scope == NULL)
  {
    while (!reader.token ().isEmpty ())
      reader.next ();
    return inv;
  }
  if (reader.token () == "=")
  {
    ScriptScope* scope2 = root->getChild (reader.next ());
    while (reader.next () == ":" && scope2)
      scope2 = scope2->getChild (reader.next ());
    while (!reader.token ().isEmpty ())
      reader.next ();
    if (scope2 == NULL)
      return inv;
    if (scope->getValue ().caseInsensitiveCompare (scope2->getValue ()) == 0)
      return !inv;
    else
      return inv;
  }
  while (!reader.token ().isEmpty ())
    reader.next ();
  if (scope->getValue () != "" && scope->getValue () != "0" && scope->getValue () != "false")
    return !inv;
  else
    return inv;
}

String ScriptParser::parse (String script)
{
  struct
  {
    int pos;
    ScriptEnum* list;
    int cur;
  } forstack[32];
  int numfors = 0;
  String result = "";
  for (int i = 0; i < script.length (); i++)
  {
    if (script[i] == '$' && script[i + 1] == '$')
    {
      while (script[i] != '\n' && script[i] != '\r')
        i++;
      while (script[i] == '\n' || script[i] == '\r')
        i++;
      i--;
    }
    else if (script[i] == '{')
    {
      i++;
      if (script[i] == '{')
        result += script[i];
      else
      {
        ScriptReader reader (script, i);
        if (reader.token () == "chatlog")
        {
          result += fmtChat ();
          while (reader.next () != "")
            ;
          i = reader.getPos ();
        }
        else if (reader.token () == "for")
        {
          if (numfors > 30)
            return "Too much nested for-loops!";
          if (reader.next () != ":")
            return "List expected!";
          reader.next ();
          if (reader.token () == "lanes")
            int asdf = 0;
          ScriptScope* scope = eval (reader);
          if (scope == NULL || scope->getEnum () == NULL)
            return "List expected!";
          forstack[numfors].list = scope->getEnum ();
          forstack[numfors].cur = 0;
          i = reader.getPos ();
          forstack[numfors].pos = i;
          if (root->getChild (forstack[numfors].list->getAttach ()))
            return "Cannot nest loops with same variable!";
          if (forstack[numfors].list->getCount () == 0)
          {
            for (int level = 1; level > 0; i++)
            {
              if (script[i] == '{')
              {
                i++;
                if (script[i] != '{')
                {
                  ScriptReader rd (script, i);
                  if (rd.token () == "for")
                    level++;
                  else if (rd.token () == "endfor")
                    level--;
                  while (rd.next () != "")
                    ;
                  i = rd.getPos ();
                  if (level == 0)
                    break;
                }
              }
            }
          }
          else
          {
            ScriptScope* a = forstack[numfors].list->getAttachScope ();
            a->setReference (forstack[numfors].list->getScope (0));
            root->addChild (a);
            root->getChild ("first")->setValue ("true");
            root->getChild ("odd")->setValue ("true");
            root->getChild ("even")->setValue ("false");
            root->getChild ("last")->setValue (forstack[numfors].list->getCount () == 1 ? "true" : "false");
            numfors++;
          }
        }
        else if (reader.token () == "endfor")
        {
          if (numfors <= 0)
            return "Unexpected endfor without matching for loop!";
          forstack[numfors - 1].cur++;
          if (forstack[numfors - 1].cur < forstack[numfors - 1].list->getCount ())
          {
            i = forstack[numfors - 1].pos;
            forstack[numfors - 1].list->getAttachScope ()->setReference (
              forstack[numfors - 1].list->getScope (forstack[numfors - 1].cur));
            root->getChild ("first")->setValue ("false");
            root->getChild ("odd")->setValue ((forstack[numfors - 1].cur % 2) == 0 ? "true" : "false");
            root->getChild ("even")->setValue ((forstack[numfors - 1].cur % 2) == 1 ? "true" : "false");
            root->getChild ("last")->setValue (forstack[numfors - 1].list->getCount () ==
              forstack[numfors - 1].cur + 1 ? "true" : "false");
          }
          else
          {
            root->remChild (forstack[numfors - 1].list->getAttach ());
            forstack[numfors - 1].list->getAttachScope ()->setReference (NULL);
            numfors--;
            if (reader.next () != "")
              return "Unexpected arguments in endfor!";
            i = reader.getPos ();
            if (numfors == 0)
            {
              root->getChild ("first")->setValue ("false");
              root->getChild ("last")->setValue ("false");
              root->getChild ("odd")->setValue ("false");
              root->getChild ("even")->setValue ("false");
            }
            else
            {
              root->getChild ("first")->setValue (forstack[numfors - 1].cur == 0 ? "true" : "false");
              root->getChild ("last")->setValue (forstack[numfors - 1].list->getCount () ==
                forstack[numfors - 1].cur + 1 ? "true" : "false");
              root->getChild ("odd")->setValue ((forstack[numfors - 1].cur % 2) == 0 ? "true" : "false");
              root->getChild ("even")->setValue ((forstack[numfors - 1].cur % 2) == 1 ? "true" : "false");
            }
          }
        }
        else if (reader.token () == "if")
        {
          while (true)
          {
            ScriptReader rrd (script, i);
            if (rrd.token () == "endif")
            {
              while (rrd.next () != "")
                ;
              i = rrd.getPos ();
              break;
            }
            bool res = true;
            if (rrd.token () != "else")
            {
              if (rrd.next () != ":")
                return "Condition expected!";
              rrd.next ();
              res = ceval (rrd);
            }
            else if (rrd.next () != "")
              return "Unexpected arguments to else!";
            if (res)
            {
              i = rrd.getPos ();
              break;
            }
            else
            {
              for (int level = 1; level > 0; i++)
              {
                if (script[i] == '{')
                {
                  i++;
                  if (script[i] != '{')
                  {
                    ScriptReader rd (script, i);
                    if (rd.token () == "if")
                      level++;
                    else if (rd.token () == "endif")
                      level--;
                    if (level == 1 && (rd.token () == "elseif" || rd.token () == "else"))
                      break;
                    if (level == 0 && rd.token () == "endif")
                      break;
                    while (rd.next () != "")
                      ;
                    i = rd.getPos ();
                  }
                }
              }
            }
          }
        }
        else if (reader.token () == "else" || reader.token () == "elseif")
        {
          for (int level = 1; level > 0; i++)
          {
            if (script[i] == '{')
            {
              i++;
              if (script[i] != '{')
              {
                ScriptReader rd (script, i);
                if (rd.token () == "if")
                  level++;
                else if (rd.token () == "endif")
                  level--;
                while (rd.next () != "")
                  ;
                i = rd.getPos ();
              }
            }
          }
        }
        else if (reader.token () == "endif")
        {
          while (reader.next () != "")
            ;
          i = reader.getPos ();
        }
        else
        {
          int oldpos = reader.getPos ();
          ScriptScope* res = eval (reader);
          if (res)
            result += res->getValue ();
          else
            return "Unknown variable " + script.substring (oldpos, reader.getPos ());
          i = reader.getPos ();
        }
      }
    }
    else if (script[i] == '}')
    {
      i++;
      if (script[i] != '}')
        return "Unexpected } with no matching {!";
      result += "}";
    }
    else
      result += script[i];
  }
  return result;
}

String makeWebColor (DWORD clr)
{
  return String::printf ("#%02X%02X%02X", clr & 0xFF, (clr >> 8) & 0xFF, (clr >> 16) & 0xFF);
}

ScriptParser::ScriptParser (W3GReplay* _w3g)
{
  w3g = _w3g;

  root = new ScriptScope ("root", "root");
  memset (players, 0, sizeof players);

  root->addChild ("endl", "\r\n");
  root->addChild ("first", "false");
  root->addChild ("last", "false");
  root->addChild ("odd", "false");
  root->addChild ("even", "false");

  ScriptScope* game = root->addChild ("game", w3g->game.name);
  game->addChild ("name", w3g->game.name);
  char fpath[256];
  char fname[256];
  char fext[256];
  _splitpath (w3g->game.map, NULL, fpath, fname, fext);
  ScriptScope* map = game->addChild ("map", fname);
  map->addChild ("path", fpath);
  ScriptScope* version = map->addChild ("version",
    formatVersion (makeVersion (w3g->dota.major, w3g->dota.minor, w3g->dota.build)));
  version->addChild ("major", String (w3g->dota.major));
  version->addChild ("minor", String (w3g->dota.minor));
  version->addChild ("build", w3g->dota.build ? String ('a' + w3g->dota.build) : "");
  game->addChild ("wc3version", formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v)));
  game->addChild ("mode", w3g->game.game_mode);
  ScriptScope* nplayers = game->addChild ("players", String (w3g->numPlayers));
  ScriptScope* nsentinel = nplayers->addChild ("sentinel", String (w3g->dota.numSentinel));
  ScriptScope* nscourge = nplayers->addChild ("scourge", String (w3g->dota.numScourge));
  if (w3g->numPlayers > w3g->dota.numSentinel + w3g->dota.numScourge)
  {
    String obs = "";
    int numObs = 0;
    for (int i = 0; i < w3g->numPlayers; i++)
    {
      if (w3g->players[w3g->pindex[i]].slot.color > 11 ||
          w3g->players[w3g->pindex[i]].slot.slot_status == 0)
      {
        if (!obs.isEmpty ()) obs += ", ";
        obs += w3g->players[w3g->pindex[i]].name;
        numObs++;
      }
    }
    game->addChild ("observers", obs)->addChild ("count", String (numObs));
  }
  else
    game->addChild ("observers", "")->addChild ("count", "0");
  ScriptScope* length = game->addChild ("length", format_time (w3g->time, TIME_HOURS | TIME_SECONDS));
  length->addChild ("hours", String (int (w3g->time / 3600000)));
  length->addChild ("minutes", String::printf ("%02d", (w3g->time / 60000) % 60));
  length->addChild ("seconds", String::printf ("%02d", (w3g->time / 1000) % 60));
  length->addChild ("totalminutes", String (int (w3g->time / 60000)));
  ScriptScope* kills = game->addChild ("kills", String::printf ("%d/%d", w3g->dota.sentinelKills, w3g->dota.scourgeKills));
  kills->addChild ("sentinel", String (w3g->dota.sentinelKills));
  kills->addChild ("scourge", String (w3g->dota.scourgeKills));
  game->addChild ("host", w3g->game.creator);

  ScriptScope* replay = root->addChild ("replay", w3g->filename);
  replay->addChild ("name", w3g->filename);
  if (w3g->game.saver_id != 0)
    replay->addChild ("saver", w3g->players[w3g->game.saver_id].name);
  else
    replay->addChild ("saver", "Unknown");

  CTime time (w3g->timestamp);
  ScriptScope* reptime = replay->addChild ("time", time.Format ("%d/%m/%Y %H:%M").GetBuffer ());
  reptime->addChild ("year", String (time.GetYear ()));
  reptime->addChild ("month", String::printf ("%02d", time.GetMonth ()));
  static char monthnames[12][32] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  reptime->addChild ("monthname", monthnames[time.GetMonth () - 1]);
  reptime->addChild ("day", String::printf ("%02d", time.GetDay ()));
  static char daynames[7][32] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  reptime->addChild ("dayofweek", daynames[time.GetDayOfWeek () - 1]);
  reptime->addChild ("hour", String (time.GetHour ()));
  reptime->addChild ("hour12", String (((time.GetHour () + 11) % 12) + 1));
  reptime->addChild ("ampm", time.GetHour () < 12 ? "AM" : "PM");
  reptime->addChild ("minute", String::printf ("%02d", time.GetMinute ()));
  reptime->addChild ("second", String::printf ("%02d", time.GetSecond ()));

  tsentinel = new ScriptScope ("team", "Sentinel");
  tscourge = new ScriptScope ("team", "Scourge");
  for (int i = 0; i < w3g->numPlayers; i++)
    players[w3g->pindex[i]] = new ScriptScope ("player", w3g->players[w3g->pindex[i]].name);
  for (int i = 0; i < w3g->numPlayers; i++)
    makePlayer (players[w3g->pindex[i]], &w3g->players[w3g->pindex[i]]);

  ScriptEnum* penum = new ScriptEnum (w3g->dota.numSentinel + w3g->dota.numScourge, "player", false);
  for (int i = 0; i < w3g->dota.numSentinel; i++)
    penum->setScope (i, players[w3g->dota.sentinel[i]]);
  for (int i = 0; i < w3g->dota.numScourge; i++)
    penum->setScope (i + w3g->dota.numSentinel, players[w3g->dota.scourge[i]]);
  nplayers->setEnum (penum);
  penum = new ScriptEnum (w3g->dota.numSentinel, "player", false);
  for (int i = 0; i < w3g->dota.numSentinel; i++)
    penum->setScope (i, players[w3g->dota.sentinel[i]]);
  nsentinel->addChild ("players", "players")->setEnum (penum);
  penum = new ScriptEnum (w3g->dota.numScourge, "player", false);
  for (int i = 0; i < w3g->dota.numScourge; i++)
    penum->setScope (i, players[w3g->dota.scourge[i]]);
  nscourge->addChild ("players", "players")->setEnum (penum);

  ScriptEnum* teams = new ScriptEnum (2, "team", true);
  root->addChild ("teams", "teams")->setEnum (teams);
  penum = new ScriptEnum (w3g->dota.numSentinel, "player", false);
  for (int i = 0; i < w3g->dota.numSentinel; i++)
    penum->setScope (i, players[w3g->dota.sentinel[i]]);
  tsentinel->addChild ("players", "players")->setEnum (penum);
  tsentinel->addChild ("size", String (w3g->dota.numSentinel));
  tsentinel->addChild ("name", "Sentinel");
  tsentinel->addChild ("color", makeWebColor (getDefaultColor (0)));
  tsentinel->addChild (players[w3g->getCaptain (1)]->reference ("captain"));
  teams->setScope (0, tsentinel);
  penum = new ScriptEnum (w3g->dota.numScourge, "player", false);
  for (int i = 0; i < w3g->dota.numScourge; i++)
    penum->setScope (i, players[w3g->dota.scourge[i]]);
  tscourge->addChild ("players", "players")->setEnum (penum);
  tscourge->addChild ("size", String (w3g->dota.numScourge));
  tscourge->addChild ("name", "Scourge");
  tscourge->addChild ("color", makeWebColor (getDefaultColor (6)));
  tscourge->addChild (players[w3g->getCaptain (2)]->reference ("captain"));
  teams->setScope (1, tscourge);

  if (w3g->game.winner == WINNER_SENTINEL || w3g->game.winner == WINNER_GSENTINEL || w3g->game.winner == WINNER_PSENTINEL)
    game->addChild (tsentinel->reference ("winner"));
  else if (w3g->game.winner == WINNER_SCOURGE || w3g->game.winner == WINNER_GSCOURGE || w3g->game.winner == WINNER_PSCOURGE)
    game->addChild (tscourge->reference ("winner"));
  else
  {
    ScriptScope* draw = game->addChild ("winner", "Draw");
    draw->addChild ("name", "Draw");
    draw->addChild ("color", "#FFFFFF");
  }

  root->addChild (tsentinel->reference ("sentinel"));
  root->addChild (tscourge->reference ("scourge"));

  int planea[4] = {0, 0, 0, 0};
  int planeb[4] = {0, 0, 0, 0};
  for (int i = 0; i < w3g->dota.numSentinel; i++)
    planea[w3g->players[w3g->dota.sentinel[i]].lane % 4]++;
  for (int i = 0; i < w3g->dota.numSentinel; i++)
    planeb[w3g->players[w3g->dota.scourge[i]].lane % 4]++;
  int numLanes = 0;
  for (int i = 0; i < 4; i++)
    if (planea[i] || planeb[i])
      numLanes++;
  ScriptEnum* lanes = new ScriptEnum (numLanes, "lane", true);
  ScriptEnum* lanesa = new ScriptEnum (numLanes, "lane", true);
  ScriptEnum* lanesb = new ScriptEnum (numLanes, "lane", true);
  int curLane = 0;
  for (int i = 0; i < 4; i++)
  {
    if (planea[i] == 0 && planeb[i] == 0)
      continue;
    ScriptScope* lane = new ScriptScope ("lane", laneName[i]);
    ScriptScope* lanea = new ScriptScope ("lane", laneName[i]);
    ScriptScope* laneb = new ScriptScope ("lane", laneName[i]);
    lane->addChild ("name", laneName[i]);
    lanea->addChild ("name", laneName[i]);
    laneb->addChild ("name", laneName[i]);
    ScriptEnum* lsentinel = new ScriptEnum (planea[i], "player", false);
    ScriptEnum* lsentinela = new ScriptEnum (planea[i], "player", false);
    for (int j = 0, c = 0; j < w3g->dota.numSentinel; j++)
    {
      if ((w3g->players[w3g->dota.sentinel[j]].lane % 4) == i)
      {
        lsentinel->setScope (c, players[w3g->dota.sentinel[j]]);
        lsentinela->setScope (c++, players[w3g->dota.sentinel[j]]);
      }
    }
    lane->addChild ("sentinel", "sentinel")->setEnum (lsentinel);
    lanea->addChild ("players", "players")->setEnum (lsentinela);
    ScriptEnum* lscourge = new ScriptEnum (planeb[i], "player", false);
    ScriptEnum* lscourgeb = new ScriptEnum (planeb[i], "player", false);
    for (int j = 0, c = 0; j < w3g->dota.numScourge; j++)
    {
      if ((w3g->players[w3g->dota.scourge[j]].lane % 4) == i)
      {
        lscourge->setScope (c, players[w3g->dota.scourge[j]]);
        lscourgeb->setScope (c++, players[w3g->dota.scourge[j]]);
      }
    }
    lane->addChild ("scourge", "scourge")->setEnum (lscourge);
    laneb->addChild ("players", "players")->setEnum (lscourgeb);
    lanes->setScope (curLane, lane);
    lanesa->setScope (curLane, lanea);
    lanesb->setScope (curLane, laneb);
    curLane++;
  }
  root->addChild ("lanes", "lanes")->setEnum (lanes);
  tsentinel->addChild ("lanes", "lanes")->setEnum (lanesa);
  tscourge->addChild ("lanes", "lanes")->setEnum (lanesb);

  DraftData* draft = &(w3g->game.draft);
  if (draft->numPool || draft->numPicks[0] || draft->numPicks[1] || draft->numBans[0] || draft->numBans[1])
  {
    ScriptScope* dd = root->addChild ("draft", "draft");
    dd->addChild (draft->firstPick == 1 ? tsentinel->reference ("first") : tscourge->reference ("first"));
    if (draft->numPool)
    {
      ScriptEnum* de = new ScriptEnum (draft->numPool, "hero", true);
      for (int i = 0; i < draft->numPool; i++)
        de->setScope (i, makeHero (draft->pool[i]));
      dd->addChild ("pool", "pool")->setEnum (de);
    }
    if (draft->numBans[0] || draft->numBans[1])
    {
      ScriptEnum* bans = new ScriptEnum (draft->numBans[0] + draft->numBans[1], "ban", true);
      ScriptEnum* bansa = new ScriptEnum (draft->numBans[0], "ban", false);
      ScriptEnum* bansb = new ScriptEnum (draft->numBans[1], "ban", false);
      for (int i = 0; i < draft->numBans[0]; i++)
      {
        ScriptScope* h = makeHero (draft->bans[0][i]);
        ScriptScope* ban = new ScriptScope ("ban", h->getValue ());
        ban->addChild (h);
        ban->addChild (tsentinel->reference ("team"));
        bans->setScope (i * 2 + (draft->firstPick == 1 ? 0 : 1), ban);
        bansa->setScope (i, ban);
      }
      for (int i = 0; i < draft->numBans[1]; i++)
      {
        ScriptScope* h = makeHero (draft->bans[1][i]);
        ScriptScope* ban = new ScriptScope ("ban", h->getValue ());
        ban->addChild (h);
        ban->addChild (tscourge->reference ("team"));
        bans->setScope (i * 2 + (draft->firstPick == 1 ? 1 : 0), ban);
        bansb->setScope (i, ban);
      }
      ScriptScope* bs = dd->addChild ("bans", "bans");
      bs->setEnum (bans);
      bs->addChild ("sentinel", "sentinel")->setEnum (bansa);
      bs->addChild ("scourge", "scourge")->setEnum (bansb);
    }
    if (draft->numPicks[0] || draft->numPicks[1])
    {
      ScriptEnum* picks = new ScriptEnum (draft->numPicks[0] + draft->numPicks[1], "pick", true);
      ScriptEnum* picksa = new ScriptEnum (draft->numPicks[0], "pick", false);
      ScriptEnum* picksb = new ScriptEnum (draft->numPicks[1], "pick", false);
      for (int i = 0; i < draft->numPicks[0]; i++)
      {
        ScriptScope* h = makeHero (draft->picks[0][i]);
        ScriptScope* pick = new ScriptScope ("pick", h->getValue ());
        pick->addChild (h);
        pick->addChild (tsentinel->reference ("team"));
        picksa->setScope (i, pick);
      }
      for (int i = 0; i < draft->numPicks[1]; i++)
      {
        ScriptScope* h = makeHero (draft->picks[1][i]);
        ScriptScope* pick = new ScriptScope ("pick", h->getValue ());
        pick->addChild (h);
        pick->addChild (tscourge->reference ("team"));
        picksb->setScope (i, pick);
      }
      int ia = 0, ib = 0, it = draft->firstPick - 1, mod = 1;
      for (int i = 0; ia < draft->numPicks[0] || ib < draft->numPicks[1]; i++)
      {
        if (it == 0 && ia < draft->numPicks[0])
          picks->setScope (i, picksa->getScope (ia++));
        else if (it == 1 && ib < draft->numPicks[1])
          picks->setScope (i, picksb->getScope (ib++));
        mod++;
        if (mod >= 2)
        {
          it = 1 - it;
          mod = 0;
        }
      }
      ScriptScope* ps = dd->addChild ("picks", "picks");
      ps->setEnum (picks);
      ps->addChild ("sentinel", "sentinel")->setEnum (picksa);
      ps->addChild ("scourge", "scourge")->setEnum (picksb);
    }
  }
}
ScriptScope* ScriptParser::makeItem (int item)
{
  DotaItem* di = getItem (item);
  ScriptScope* it = new ScriptScope ("item", di ? di->name : "None");
  if (di)
  {
    it->addChild ("name", di->name);
    it->addChild ("icon", di->imgTag);
    it->addChild ("pdicon", getPDItemTag (di->imgTag));
    it->addChild ("cost", String (di->cost));
    if (di->type == ITEM_COMBO)
      it->addChild ("combined", "true");
  }
  else
  {
    it->addChild ("name", "None");
    it->addChild ("icon", "");
    it->addChild ("pdicon", "");
    it->addChild ("cost", "");
  }
  return it;
}
ScriptScope* ScriptParser::makeHero (int hero)
{
  DotaHero* dh = getHero (hero);
  ScriptScope* he = new ScriptScope ("hero", dh ? dh->name : "None");
  if (dh)
  {
    he->addChild ("name", dh->name);
    he->addChild ("propername", dh->oname);
    he->addChild ("abbrev", dh->abbr);
    he->addChild ("icon", dh->imgTag);
    he->addChild ("pdicon", getPDTag (dh->point));
    he->addChild ("pdiconmed", getPDTagMed (dh->point));
    he->addChild ("pdiconwide", getPDTagWide (dh->point));
  }
  else
  {
    he->addChild ("name", "None");
    he->addChild ("propername", "");
    he->addChild ("abbrev", "None");
    he->addChild ("icon", "");
    he->addChild ("pdicon", "");
    he->addChild ("pdiconmed", "");
    he->addChild ("pdiconwide", "");
  }
  return he;
}
ScriptScope* ScriptParser::makePlayer (ScriptScope* player, W3GPlayer* p)
{
  player->addChild ("name", p->name);
  player->addChild ("color", makeWebColor (getDefaultColor (p->slot.color)));
  player->addChild (p->slot.team == 0 ? tsentinel->reference ("team") : tscourge->reference ("team"));
  ScriptScope* left = player->addChild ("left",
    p->time >= w3g->time ? "End" : format_time (w3g, p->time, TIME_HOURS | TIME_SECONDS));
  left->addChild ("hours", String (int (p->time / 3600000)));
  left->addChild ("minutes", String::printf ("%02d", (p->time / 60000) % 60));
  left->addChild ("seconds", String::printf ("%02d", (p->time / 1000) % 60));
  left->addChild ("totalminutes", String (int (p->time / 60000)));
  player->addChild ("apm", String (int (p->time ? p->actions * 60000 / p->time : 0)));
  player->addChild ("lane", laneName[p->lane]);
  ScriptScope* hero = player->addChild (makeHero (p->hero ? p->hero->id : -1));
  hero->addChild ("level", String (p->hero ? p->hero->level : 0));
  ScriptScope* stats = player->addChild ("stats", String::printf ("KD: %d/%d/%d CS: %d/%d/%d",
    p->stats[STAT_KILLS], p->stats[STAT_DEATHS], p->stats[STAT_ASSISTS],
    p->stats[STAT_CREEPS], p->stats[STAT_DENIES], p->stats[STAT_NEUTRALS]));
  stats->addChild ("herokills", String (p->stats[STAT_KILLS]));
  stats->addChild ("deaths", String (p->stats[STAT_DEATHS]));
  stats->addChild ("assists", String (p->stats[STAT_ASSISTS]));
  stats->addChild ("creepkills", String (p->stats[STAT_CREEPS]));
  stats->addChild ("creepdenies", String (p->stats[STAT_DENIES]));
  stats->addChild ("neutralkills", String (p->stats[STAT_NEUTRALS]));
  ScriptScope* killdet = player->addChild ("killdetails", "killdetails");
  ScriptEnum* kdenum = new ScriptEnum (p->slot.team == 0 ? w3g->dota.numScourge : w3g->dota.numSentinel,
    "killdetail", true);
  for (int i = 0; i < kdenum->getCount (); i++)
  {
    int t = (p->slot.team == 0 ? w3g->dota.scourge[i] : w3g->dota.sentinel[i]);
    ScriptScope* kd = new ScriptScope ("killdetail", w3g->players[t].name);
    kd->addChild (players[t]->reference ("player"));
    kd->addChild ("kills", String (p->pkilled[t]));
    kd->addChild ("deaths", String (p->pdied[t]));
    kdenum->setScope (i, kd);
  }
  killdet->setEnum (kdenum);
  player->addChild ("gold", String (p->itemCost));
  ScriptEnum* invenum = new ScriptEnum (6, "item", true);
  for (int i = 0; i < 6; i++)
    invenum->setScope (i, makeItem (p->finalItems[i]));
  player->addChild ("inventory", "inventory")->setEnum (invenum);

  p->inv.getAllItems (END_TIME, w3g->dota);
  p->inv.sortItems ();
  ScriptEnum* ibenum = new ScriptEnum (p->inv.bi.getSize (), "item", true);
  for (int i = 0; i < p->inv.bi.getSize (); i++)
  {
    ScriptScope* item = makeItem (p->inv.bi[i].id);
    ScriptScope* time = item->addChild ("time", format_time (w3g, p->inv.bi[i].time, TIME_HOURS | TIME_SECONDS));
    time->addChild ("hours", String (int (p->inv.bi[i].time / 3600000)));
    time->addChild ("minutes", String::printf ("%02d", (p->inv.bi[i].time / 60000) % 60));
    time->addChild ("seconds", String::printf ("%02d", (p->inv.bi[i].time / 1000) % 60));
    time->addChild ("totalminutes", String (int (p->inv.bi[i].time / 60000)));
    ibenum->setScope (i, item);
  }
  player->addChild ("items", "items")->setEnum (ibenum);

  ScriptEnum* aenum = new ScriptEnum (p->hero ? p->hero->level : 0, "skill", true);
  if (p->hero)
  {
    int alvl[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < p->hero->level; i++)
    {
      DotaAbility* a = getAbility (p->hero->abilities[i]);
      ScriptScope* abil = new ScriptScope ("skill", a ? a->name : "None");
      if (a)
        alvl[a->slot]++;
      abil->addChild ("name", a ? a->name : "None");
      abil->addChild ("level", a ? String (alvl[a->slot]) : "0");
      abil->addChild ("slot", a ? String (a->slot) : 0);
      abil->addChild ("icon", a ? a->imgTag : "");
      ScriptScope* time = abil->addChild ("time", format_time (w3g, p->hero->atime[i], TIME_HOURS | TIME_SECONDS));
      time->addChild ("hours", String (int (p->hero->atime[i] / 3600000)));
      time->addChild ("minutes", String::printf ("%02d", (p->hero->atime[i] / 60000) % 60));
      time->addChild ("seconds", String::printf ("%02d", (p->hero->atime[i] / 1000) % 60));
      time->addChild ("totalminutes", String (int (p->hero->atime[i] / 60000)));
      aenum->setScope (i, abil);
    }
  }
  player->addChild ("skills", "skills")->setEnum (aenum);

  return player;
}
ScriptParser::~ScriptParser ()
{
  for (int i = 0; i < 256; i++)
    delete players[i];
  delete root;
}
