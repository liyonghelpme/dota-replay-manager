#include "core/app.h"

#include "data.h"

#include "dota/colors.h"
#include "dota/dotadata.h"

ScriptType::ScriptType(int t)
{
  type = t;
  enumType = NULL;
  ref = 0;

  if (type == tEnum)
    addElement("count", tBasic);
}
ScriptType::ScriptType(ScriptType* t)
{
  for (uint32 cur = t->dir.enumStart(); cur; cur = t->dir.enumNext(cur))
    dir.set(t->dir.enumGetKey(cur), t->dir.enumGetValue(cur));
  for (int i = 0; i < t->types.length(); i++)
  {
    types.push(t->types[i]);
    t->types[i]->ref++;
  }
  enumType = t->enumType;
  if (enumType)
    enumType->ref++;
  type = t->type;
  ref = 0;

  if (type == tEnum)
    addElement("count", tBasic);
}
ScriptType::~ScriptType()
{
  for (int i = 0; i < types.length(); i++)
  {
    if (--types[i]->ref == 0)
      delete types[i];
  }
  if (enumType && --enumType->ref == 0)
    delete enumType;
}
void ScriptType::addElement(char const* name, ScriptType* element)
{
  if (!dir.has(name))
  {
    dir.set(name, types.length());
    types.push(element);
    element->ref++;
  }
}
ScriptType* ScriptType::addElement(char const* name, int t)
{
  if (dir.has(name))
    return types[dir.get(name)];
  else
  {
    dir.set(name, types.length());
    ScriptType* element = new ScriptType(t);
    types.push(element);
    element->ref++;
    return element;
  }
}
void ScriptType::setEnumType(ScriptType* t)
{
  if (enumType && --enumType->ref == 0)
    delete enumType;
  enumType = t;
  enumType->ref++;
  enumType->addElement("first", tBasic);
  enumType->addElement("last", tBasic);
  enumType->addElement("odd", tBasic);
  enumType->addElement("even", tBasic);
}

ScriptValue::ScriptValue(ScriptType* t)
{
  type = t;
  elements = new ScriptValue*[t->getNumElements()];
  memset(elements, 0, sizeof(ScriptValue*) * t->getNumElements());
  ref = 0;

  if (type->getType() == ScriptType::tEnum)
    addElement("count")->setValue(value = "0");
  //for (int i = 0; i < t->getNumElements(); i++)
  //  elements[i] = new ScriptValue(t->getElement(i));
}
void ScriptValue::addEnum(ScriptValue* element)
{
  if (list.length() == 0)
    element->getElement("first")->setValue("true");
  else
  {
    element->getElement("first")->setValue("false");
    list[list.length() - 1]->getElement("last")->setValue("false");
  }
  element->getElement("last")->setValue("true");
  element->getElement("odd")->setValue(list.length() & 1 ? "false" : "true");
  element->getElement("even")->setValue(list.length() & 1 ? "true" : "false");
  list.push(element);
  getElement("count")->setValue(value = String(list.length()));
}
ScriptValue* ScriptValue::addEnum()
{
  ScriptValue* value = new ScriptValue(type->getEnumType());
  addEnum(value);
  return value;
}

ScriptValue::~ScriptValue()
{
  for (int i = 0; i < type->getNumElements(); i++)
    if (elements[i] && --elements[i]->ref == 0)
      delete elements[i];
  delete[] elements;
}
void ScriptValue::addElement(char const* name, ScriptValue* value)
{
  elements[type->getElementPos(name)] = value;
  value->ref++;
}
ScriptValue* ScriptValue::addElement(char const* name)
{
  int pos = type->getElementPos(name);
  elements[pos] = new ScriptValue(type->getElement(pos));
  elements[pos]->ref++;
  return elements[pos];
}

ScriptType* ScriptType::getSubType(char const* name)
{
  if (*name == 0)
    return NULL;
  ScriptType* cur = this;
  String next = "";
  while (true)
  {
    if (*name == '.' || *name == 0)
    {
      if (!next.isEmpty())
      {
        if (!cur->dir.has(next))
          return NULL;
        cur = cur->types[cur->dir.get(next)];
        next = "";
      }
    }
    else
      next += *name;
    if (*name == 0)
      break;
    name++;
  }
  return cur;
}

static ScriptType oBasic(ScriptType::tValue);
static ScriptType oGlobal(ScriptType::tStruct);
ScriptType* ScriptType::tBasic = &oBasic;
ScriptType* ScriptType::tPlayer = NULL;
ScriptType* ScriptType::tGlobal = &oGlobal;
void ScriptType::initTypes()
{
  oBasic.ref++;

  ScriptType* tString = new ScriptType(tValue);
  tString->addElement("lower", tBasic);
  tString->addElement("upper", tBasic);

  ScriptType* tVersion = new ScriptType(tValue);
  tVersion->addElement("major", tBasic);
  tVersion->addElement("minor", tBasic);
  tVersion->addElement("build", tBasic);

  ScriptType* tTime = new ScriptType(tValue);
  tTime->addElement("hours", tBasic);
  tTime->addElement("minutes", tBasic);
  tTime->addElement("seconds", tBasic);
  tTime->addElement("totalminutes", tBasic);

  ScriptType* tDate = new ScriptType(tValue);
  tDate->addElement("year", tBasic);
  tDate->addElement("month", tBasic);
  tDate->addElement("monthname", tString);
  tDate->addElement("day", tBasic);
  tDate->addElement("dayofweek", tString);
  tDate->addElement("hour", tBasic);
  tDate->addElement("hour12", tBasic);
  tDate->addElement("ampm", tString);
  tDate->addElement("minute", tBasic);
  tDate->addElement("second", tBasic);

  ScriptType* tItem = new ScriptType(tStruct);
  tItem->addElement("name", tString);
  tItem->addElement("icon", tString);
  tItem->addElement("pdicon", tString);
  tItem->addElement("combined", tBasic);
  tItem->addElement("cost", tBasic);

  ScriptType* tListItem = new ScriptType(tItem);
  tListItem->addElement("time", tTime);

  ScriptType* tHero = new ScriptType(tStruct);
  tHero->addElement("name", tString);
  tHero->addElement("propername", tString);
  tHero->addElement("abbrev", tString);
  tHero->addElement("icon", tString);
  tHero->addElement("pdicon", tString);
  tHero->addElement("pdiconmed", tString);
  tHero->addElement("pdiconwide", tString);

  ScriptType* tStats = new ScriptType(tStruct);
  tStats->addElement("herokills", tBasic);
  tStats->addElement("deaths", tBasic);
  tStats->addElement("assists", tBasic);
  tStats->addElement("creepkills", tBasic);
  tStats->addElement("creepdenies", tBasic);
  tStats->addElement("neutralkills", tBasic);

  ScriptType* tSkill = new ScriptType(tStruct);
  tSkill->addElement("name", tString);
  tSkill->addElement("level", tBasic);
  tSkill->addElement("slot", tBasic);
  tSkill->addElement("icon", tString);
  tSkill->addElement("time", tTime);

  tPlayer = new ScriptType(tStruct);
  ScriptType* tTeam = new ScriptType(tStruct);
  ScriptType* tLane = new ScriptType(tStruct);

  ScriptType* tKillDetail = new ScriptType(tStruct);
  tKillDetail->addElement("player", tPlayer);
  tKillDetail->addElement("kills", tBasic);
  tKillDetail->addElement("deaths", tBasic);

  tPlayer->addElement("name", tString);
  tPlayer->addElement("color", tString);
  tPlayer->addElement("team", tTeam);
  tPlayer->addElement("left", tTime);
  tPlayer->addElement("apm", tBasic);
  tPlayer->addElement("lane", tLane);
  tPlayer->addElement("hero", tHero);
  tPlayer->addElement("level", tBasic);
  tPlayer->addElement("stats", tStats);
  tPlayer->addElement("killdetails", tEnum)->setEnumType(tKillDetail);
  tPlayer->addElement("gold", tBasic);
  tPlayer->addElement("inventory", tEnum)->setEnumType(tItem);
  tPlayer->addElement("items", tEnum)->setEnumType(tListItem);
  tPlayer->addElement("skills", tEnum)->setEnumType(tSkill);

  ScriptType* tPlayers = new ScriptType(tEnum);
  tPlayers->setEnumType(tPlayer);

  tLane->addElement("name", tString);
  tLane->addElement("players", tPlayers);

  ScriptType* tGlobalLane = new ScriptType(tStruct);
  tGlobalLane->addElement("name", tString);
  tGlobalLane->addElement("sentinel", tPlayers);
  tGlobalLane->addElement("scourge", tPlayers);

  tTeam->addElement("players", tPlayers);
  tTeam->addElement("color", tBasic);
  tTeam->addElement("captain", tPlayer);
  tTeam->addElement("kills", tBasic);
  tTeam->addElement("lanes", tEnum)->setEnumType(tLane);

  ScriptType* tMap = new ScriptType(tString);
  tMap->addElement("path", tString);
  tMap->addElement("version", tVersion);

  ScriptType* tGamePlayers = new ScriptType(tPlayers);
  tPlayers->addElement("sentinel", tPlayers);
  tPlayers->addElement("scourge", tPlayers);

  ScriptType* tObservers = new ScriptType(tString);
  tObservers->addElement("count", tBasic);

  ScriptType* tKills = new ScriptType(tValue);
  tKills->addElement("sentinel", tBasic);
  tKills->addElement("sentinel", tBasic);

  ScriptType* tGame = new ScriptType(tStruct);
  tGame->addElement("name", tString);
  tGame->addElement("map", tMap);
  tGame->addElement("mode", tString);
  tGame->addElement("players", tGamePlayers);
  tGame->addElement("observers", tObservers);
  tGame->addElement("length", tTime);
  tGame->addElement("kills", tKills);
  tGame->addElement("host", tPlayer);
  tGame->addElement("winner", tTeam);

  ScriptType* tReplay = new ScriptType(tStruct);
  tReplay->addElement("name", tString);
  tReplay->addElement("wc3version", tVersion);
  tReplay->addElement("saver", tPlayer);
  tReplay->addElement("time", tDate);

  ScriptType* tPick = new ScriptType(tStruct);
  tPick->addElement("hero", tHero);
  tPick->addElement("team", tTeam);

  ScriptType* tPicks = new ScriptType(tEnum);
  tPicks->setEnumType(tPick);
  tPicks->addElement("sentinel", tEnum)->setEnumType(tHero);
  tPicks->addElement("scourge", tEnum)->setEnumType(tHero);

  ScriptType* tDraft = new ScriptType(tStruct);
  tDraft->addElement("first", tTeam);
  tDraft->addElement("pool", tEnum)->setEnumType(tHero);
  tDraft->addElement("bans", tPicks);
  tDraft->addElement("picks", tPicks);

  tGlobal->addElement("endl", tBasic);
  tGlobal->addElement("game", tGame);
  tGlobal->addElement("replay", tReplay);
  tGlobal->addElement("teams", tEnum)->setEnumType(tTeam);
  tGlobal->addElement("sentinel", tTeam);
  tGlobal->addElement("scourge", tTeam);
  tGlobal->addElement("lanes", tEnum)->setEnumType(tGlobalLane);
  tGlobal->addElement("draft", tDraft);
}

///////////////////////////////////////////////

static void setString(ScriptValue* val, String str)
{
  val->setValue(str);
  val->addElement("lower")->setValue(String(str).toLower());
  val->addElement("upper")->setValue(String(str).toUpper());
}
static void setVersion(ScriptValue* val, uint32 version)
{
  val->setValue(formatVersion(version));
  val->addElement("major")->setValue(version / 2600);
  val->addElement("minor")->setValue((version / 26) % 100);
  val->addElement("build")->setValue(String(char('a' + (version % 26))));
}
static void setColor(ScriptValue* val, uint32 color)
{
  setString(val, String::format("#%02X%02X%02X", int(color & 0xFF),
    int((color >> 8) & 0xFF), int((color >> 16) & 0xFF)));
}
static void setTime(ScriptValue* val, uint32 time, W3GReplay* w3g)
{
  val->setValue(w3g->formatTime(time));
  val->addElement("hours")->setValue(time / 3600000);
  val->addElement("minutes")->setValue((time / 60000) % 60);
  val->addElement("totalminutes")->setValue(time / 60000);
  val->addElement("seconds")->setValue((time / 1000) % 60);
}
static void setItem(ScriptValue* val, Dota::Item* item)
{
  val->setValue(item->name);
  setString(val->addElement("name"), item->name);
  setString(val->addElement("icon"), item->icon);
  setString(val->addElement("pdicon"), getApp()->getDotaLibrary()->getItemPdTag(item->name));
  val->addElement("combined")->setValue(item->recipe ? "true" : "false");
  val->addElement("cost")->setValue(item->cost);
}
static void setHero(ScriptValue* val, Dota::Hero* hero)
{
  val->setValue(hero->name);
  setString(val->addElement("name"), hero->name);
  setString(val->addElement("propername"), hero->properName);
  setString(val->addElement("abbrev"), hero->shortName);
  setString(val->addElement("icon"), hero->icon);
  setString(val->addElement("pdicon"), getApp()->getDotaLibrary()->getHeroPdTag(hero->point));
  setString(val->addElement("pdiconmed"), getApp()->getDotaLibrary()->getHeroPdTagMedium(hero->point));
  setString(val->addElement("pdiconwide"), getApp()->getDotaLibrary()->getHeroPdTagWide(hero->point));
}

ScriptGlobal* ScriptGlobal::create(W3GReplay* w3g)
{
  W3GGame* gameInfo = w3g->getGameInfo();
  DotaInfo const* dotaInfo = w3g->getDotaInfo();

  ScriptGlobal* global = new ScriptGlobal(ScriptType::tGlobal);
  global->addElement("endl")->setValue("\n");

  Array<ScriptValue*> players;
  for (int i = 0; i < w3g->getNumPlayers(); i++)
  {
    W3GPlayer* player = w3g->getPlayer(i);
    players.push(new ScriptValue(ScriptType::tPlayer));
    players[i]->setValue(player->name);
    setString(players[i]->addElement("name"), player->name);
    setColor(players[i]->addElement("color"), getDefaultColor(player->slot.color));
    setTime(players[i]->addElement("left"), player->time, w3g);
    players[i]->addElement("apm")->setValue(player->apm());
    if (player->hero)
    {
      setHero(players[i]->addElement("hero"), player->hero->hero);
      players[i]->addElement("level")->setValue(player->hero->level);
    }
    ScriptValue* stats = players[i]->addElement("stats");
    stats->setValue(String::format("KD: %d/%d/%d CS: %d/%d/%d",
      player->stats[STAT_KILLS], player->stats[STAT_DEATHS], player->stats[STAT_ASSISTS],
      player->stats[STAT_CREEPS], player->stats[STAT_DENIES], player->stats[STAT_NEUTRALS]));
    stats->addElement("herokills")->setValue(player->stats[STAT_KILLS]);
    stats->addElement("deaths")->setValue(player->stats[STAT_DEATHS]);
    stats->addElement("assists")->setValue(player->stats[STAT_ASSISTS]);
    stats->addElement("creepkills")->setValue(player->stats[STAT_CREEPS]);
    stats->addElement("creepdenies")->setValue(player->stats[STAT_DENIES]);
    stats->addElement("neutralkills")->setValue(player->stats[STAT_NEUTRALS]);
    stats->addElement("gold")->setValue(player->item_cost + player->stats[STAT_GOLD]);

    ScriptValue* inventory = stats->addElement("inventory");
    for (int i = 0; i < 6; i++)
      if (player->inv.final[i])
        setItem(inventory->addEnum(), player->inv.final[i]);

    ScriptValue* items = stats->addElement("items");
    player->inv.compute(0x7FFFFFFF, w3g->getDotaData());
    for (int i = 0; i < player->inv.comb.length(); i++)
    {
      W3GItem& item = player->inv.comb[i];
      if (item.item)
      {
        ScriptValue* it = items->addEnum();
        setItem(it, item.item);
        setTime(it->addElement("time"), item.time, w3g);
      }
    }

    if (player->hero)
    {
      ScriptValue* skills = stats->addElement("skills");
      int levels[5] = {0, 0, 0, 0, 0};
      for (int i = 1; i <= player->hero->level; i++)
      {
        ScriptValue* skill = skills->addEnum();
        if (player->hero->skills[i].skill)
        {
          Dota::Ability* abil = player->hero->skills[i].skill;
          int level = 0;
          if (abil->slot >= 0 && abil->slot < 5)
            level = ++levels[abil->slot];
          skill->setValue(abil->name);
          setString(skill->addElement("name"), abil->name);
          skill->addElement("level")->setValue(level);
          skill->addElement("slot")->setValue(abil->slot);
          setString(skill->addElement("icon"), abil->icon);
          setTime(skill->addElement("time"), player->hero->skills[i].time, w3g);
        }
      }
    }
  }

  ScriptValue* sentinel = global->addElement("sentinel");
  ScriptValue* scourge = global->addElement("scourge");
  ScriptValue* teams = global->addElement("teams");
  teams->addEnum(sentinel);
  teams->addEnum(scourge);
  ScriptValue* lanes = global->addElement("lanes");

  if (dotaInfo)
  {
    sentinel->setValue("Sentinel");
    setString(sentinel->addElement("name"), "Sentinel");
    setColor(sentinel->addElement("color"), getDefaultColor(0));
    sentinel->addElement("kills")->setValue(dotaInfo->team_kills[0]);
    if (w3g->getCaptain(0))
      sentinel->addElement("captain", players[w3g->getCaptain(0)->index]);

    scourge->setValue("Scourge");
    setString(scourge->addElement("name"), "Scourge");
    setColor(scourge->addElement("color"), getDefaultColor(6));
    scourge->addElement("kills")->setValue(dotaInfo->team_kills[1]);
    if (w3g->getCaptain(1))
      scourge->addElement("captain", players[w3g->getCaptain(1)->index]);

    int laneCount[4] = {0, 0, 0, 0};
    for (int t = 0; t < 2; t++)
    {
      ScriptValue* team = (t ? scourge : sentinel);
      ScriptValue* plist = team->addElement("players");
      for (int i = 0; i < dotaInfo->team_size[t]; i++)
      {
        int id = dotaInfo->teams[t][i]->index;
        plist->addEnum(players[id]);
        laneCount[dotaInfo->teams[t][i]->lane % 4]++;

        players[id]->addElement("team", team);
        ScriptValue* killdet = players[id]->addElement("killdetails");
        for (int j = 0; j < dotaInfo->team_size[1 - t]; j++)
        {
          ScriptValue* kd = killdet->addEnum();
          int oid = dotaInfo->teams[1 - t][j]->index;
          kd->addElement("player", players[oid]);
          kd->addElement("kills")->setValue(dotaInfo->teams[t][i]->pkilled[oid]);
          kd->addElement("deaths")->setValue(dotaInfo->teams[t][i]->pdied[oid]);
        }
      }
    }
    ScriptValue* tlanes[2];
    tlanes[0] = sentinel->addElement("lanes");
    tlanes[1] = scourge->addElement("lanes");
    for (int l = 0; l < 4; l++)
    {
      if (laneCount[l] == 0)
        continue;
      ScriptValue* lane = lanes->addEnum();
      lane->setValue(getLaneName(l));
      setString(lane->getElement("name"), getLaneName(l));

      for (int t = 0; t < 2; t++)
      {
        ScriptValue* plane = lane->addElement(t ? "scourge" : "sentinel");
        ScriptValue* tlane = tlanes[t]->addEnum();
        tlane->setValue(getLaneName(l));
        setString(tlane->getElement("name"), getLaneName(l));
        tlane = tlane->getElement("players");
        for (int i = 0; i < dotaInfo->team_size[t]; i++)
        {
          if ((dotaInfo->teams[t][i]->lane % 4) == l)
          {
            int id = dotaInfo->teams[t][i]->index;
            plane->addEnum(players[id]);
            tlane->addEnum(players[id]);
            players[id]->addElement("lane", lane);
          }
        }
      }
    }
  }

  ScriptValue* game = global->addElement("game");
  game->setValue(gameInfo->name);
  setString(game->addElement("name"), gameInfo->name);

  ScriptValue* map = game->addElement("map");
  setString(map, String::getFileTitle(gameInfo->map));
  setString(map->addElement("path"), String::getPath(gameInfo->map));
  if (dotaInfo)
    setVersion(map->addElement("version"), dotaInfo->version);

  setString(game->addElement("mode"), gameInfo->game_mode);

  ScriptValue* gPlayers = game->addElement("players");
  if (dotaInfo)
  {
    for (int t = 0; t < 2; t++)
      for (int i = 0; i < dotaInfo->team_size[t]; i++)
        gPlayers->addEnum(players[dotaInfo->teams[t][i]->index]);
    gPlayers->addElement("sentinel", sentinel->getElement("players"));
    gPlayers->addElement("scourge", sentinel->getElement("scourge"));
  }
  else
  {
    for (int i = 0; i < w3g->getNumPlayers(); i++)
      if (w3g->getPlayer(i)->slot.slot_status != 0)
        gPlayers->addEnum(players[i]);
  }

  String obsList = "";
  int obsCount = 0;
  for (int i = 0; i < w3g->getNumPlayers(); i++)
  {
    W3GPlayer* p = w3g->getPlayer(i);
    if (p->slot.slot_status == 0 || p->slot.color > 11)
    {
      if (!obsList.isEmpty())
        obsList += ", ";
      obsList += p->name;
      obsCount++;
    }
  }
  ScriptValue* observers = game->addElement("observers");
  setString(observers, obsList);
  observers->addElement("count")->setValue(obsCount);

  setTime(game->addElement("length"), w3g->getLength(), w3g);

  ScriptValue* kills = game->addElement("kills");
  if (dotaInfo)
  {
    kills->setValue(String::format("%d/%d", dotaInfo->team_kills[0], dotaInfo->team_kills[1]));
    kills->addElement("sentinel")->setValue(dotaInfo->team_kills[0]);
    kills->addElement("scourge")->setValue(dotaInfo->team_kills[1]);
  }

  for (int i = 0; i < w3g->getNumPlayers(); i++)
  {
    if (!w3g->getPlayer(i)->name.icompare(gameInfo->creator))
    {
      game->addElement("host", players[i]);
      break;
    }
  }

  if (gameInfo->winner == WINNER_SENTINEL || gameInfo->winner == WINNER_GSENTINEL ||
      gameInfo->winner == WINNER_PSENTINEL)
    game->addElement("winner", sentinel);
  else if (gameInfo->winner == WINNER_SCOURGE || gameInfo->winner == WINNER_GSCOURGE ||
      gameInfo->winner == WINNER_PSCOURGE)
    game->addElement("winner", scourge);
  else
  {
    ScriptValue* draw = game->addElement("winner");
    draw->setValue("Draw");
    setString(draw->addElement("name"), "Draw");
    setColor(draw->addElement("color"), 0xFFFFFF);
  }

  ScriptValue* replay = global->addElement("replay");
  FileInfo* fileInfo = w3g->getFileInfo();
  if (fileInfo)
  {
    replay->setValue(String::getFileTitle(fileInfo->path));
    setString(replay->addElement("name"), String::getFileTitle(fileInfo->path));

    ScriptValue* time = replay->addElement("time");
    time->setValue(format_systime(fileInfo->ftime, "%d/%m/%Y %H:%M"));
    time->addElement("year")->setValue(fileInfo->time.wYear);
    time->addElement("month")->setValue(fileInfo->time.wMonth);
    setString(time->addElement("monthname"), format_systime(fileInfo->ftime, "%b"));
    time->addElement("day")->setValue(fileInfo->time.wDay);
    setString(time->addElement("dayofweek"), format_systime(fileInfo->ftime, "%a"));
    time->addElement("hour")->setValue(fileInfo->time.wHour);
    time->addElement("hour12")->setValue(((fileInfo->time.wHour + 11) % 12) + 1);
    setString(time->addElement("ampm"), fileInfo->time.wHour < 12 ? "AM" : "PM");
    time->addElement("minute")->setValue(fileInfo->time.wMinute);
    time->addElement("second")->setValue(fileInfo->time.wSecond);
  }
  setVersion(replay->addElement("wc3version"), w3g->getVersion());
  if (gameInfo->saver)
    replay->addElement("saver", players[gameInfo->saver->index]);

  if (dotaInfo && (dotaInfo->draft.numPool || dotaInfo->draft.numPicks[0] ||
    dotaInfo->draft.numPicks[1] || dotaInfo->draft.numBans[0] || dotaInfo->draft.numBans[1]))
  {
    ScriptValue* draft = global->addElement("draft");
    draft->addElement("first", dotaInfo->draft.firstPick ? scourge : sentinel);
    ScriptValue* pool = draft->addElement("pool");
    for (int i = 0; i < dotaInfo->draft.numPool; i++)
      if (dotaInfo->draft.pool[i])
        setHero(pool->addEnum(), dotaInfo->draft.pool[i]);
    ScriptValue* bans = draft->addElement("bans");
    ScriptValue* bteams[2];
    bteams[0] = bans->addElement("sentinel");
    bteams[1] = bans->addElement("scourge");
    for (int i = 0; i < dotaInfo->draft.numBans[0] || i < dotaInfo->draft.numBans[1]; i++)
    {
      for (int z = 0, t = dotaInfo->draft.firstPick; z < 2; z++, t = 1 - t)
      {
        if (i < dotaInfo->draft.numBans[t] && dotaInfo->draft.bans[t][i])
        {
          ScriptValue* hero = bteams[t]->addEnum();
          setHero(hero, dotaInfo->draft.bans[t][i]);
          bans->addEnum(hero);
        }
      }
    }
    ScriptValue* picks = draft->addElement("picks");
    ScriptValue* pteams[2];
    pteams[0] = picks->addElement("sentinel");
    pteams[1] = picks->addElement("scourge");
    for (int i = 0; i < dotaInfo->draft.numPicks[0] || i < dotaInfo->draft.numPicks[1]; i++)
    {
      int t = (dotaInfo->draft.firstPick + i) % 2;
      for (int z = 0; z < 2; z++, t = 1 - t)
      {
        if (i < dotaInfo->draft.numPicks[t] && dotaInfo->draft.picks[t][i])
        {
          ScriptValue* hero = pteams[t]->addEnum();
          setHero(hero, dotaInfo->draft.picks[t][i]);
          picks->addEnum(hero);
        }
      }
    }
  }

  for (int i = 0; i < players.length(); i++)
    players[i]->poof();

  return global;
}
