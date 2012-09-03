#include "replay.h"
#include "base/gzmemory.h"
#include "core/app.h"
#include "replay/cache.h"

#include <stdarg.h>
#include <stdio.h>

struct W3GBlockHeader
{
  uint16 c_size;
  uint16 u_size;
  uint16 check1;
  uint16 check2;
};

bool W3GHeader::read(File* f)
{
  memset(this, 0, sizeof(W3GHeader));
  if (f->read(this, 48) != 48)
    return false;
  if (memcmp(intro, "Warcraft III recorded game", 26))
    return false;
  if (header_v == 0)
  {
    if (f->read(&minor_v, 2) != 2) return false;
    if (f->read(&major_v, 2) != 2) return false;
    if (f->read(&build_v, 2) != 2) return false;
    if (f->read(&flags, 2) != 2) return false;
    if (f->read(&length, 4) != 4) return false;
    if (f->read(&checksum, 4) != 4) return false;
    ident = 'WAR3';
  }
  else
  {
    if (f->read(&ident, 4) != 4) return false;
    if (f->read(&major_v, 4) != 4) return false;
    if (f->read(&build_v, 2) != 2) return false;
    if (f->read(&flags, 2) != 2) return false;
    if (f->read(&length, 4) != 4) return false;
    if (f->read(&checksum, 4) != 4) return false;
    minor_v = 0;
  }
  return true;
}
void W3GHeader::write(File* f)
{
  f->write(this, 48);
  if (header_v == 0)
  {
    f->write(&minor_v, 2);
    f->write(&major_v, 2);
    f->write(&build_v, 2);
    f->write(&flags, 2);
    f->write(&length, 4);
    f->write(&checksum, 4);
  }
  else
  {
    f->write(&ident, 4);
    f->write(&major_v, 4);
    f->write(&build_v, 2);
    f->write(&flags, 2);
    f->write(&length, 4);
    f->write(&checksum, 4);
  }
  for (int pos = f->tell(); pos < header_size; pos++)
    f->putc(0);
}

File* W3GReplay::unpack(File* f, W3GHeader& hdr)
{
  if (!hdr.read (f))
    return NULL;

  f->seek(0, SEEK_END);
  int file_size = f->tell();

  f->seek(hdr.header_size, SEEK_SET);
  int pos = hdr.header_size;
  int max_source = 1024;
  int total_dest = 0;
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader block;
    if (f->read(&block, sizeof block) != sizeof block)
      return NULL;
    pos += sizeof block;
    if (pos + block.c_size > file_size)
      return NULL;
    f->seek(block.c_size, SEEK_CUR);

    if (block.c_size > max_source)
      max_source = block.c_size;
    total_dest += block.u_size;
  }

  if (total_dest == 0)
    return NULL;

  char* source = new char[max_source];
  char* dest = new char[total_dest];
  int dest_pos = 0;

  gzmemory gzmem;

  f->seek(hdr.header_size, SEEK_SET);
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader block;
    f->read(&block, sizeof block);
    f->read(source, block.c_size);
    if (!gzinflate2(source, dest + dest_pos, block.c_size, block.u_size, &gzmem))
    {
      delete[] source;
      delete[] dest;
      return NULL;
    }
    dest_pos += block.u_size;
  }

  delete[] source;
  return File::memfile(dest, dest_pos);
}
W3GReplay* W3GReplay::load(File* replay, bool quick, uint32* error)
{
  uint32 localError;
  if (error == NULL)
    error = &localError;
  *error = 0;

  W3GHeader hdr;
  File* unpacked = unpack(replay, hdr);
  if (unpacked)
  {
    W3GReplay* w3g = new W3GReplay(unpacked, hdr, quick, error);
    if (*error)
    {
      delete w3g;
      w3g = NULL;
    }
    return w3g;
  }
  else
  {
    *error = eBadFile;
    return NULL;
  }
}
W3GReplay* W3GReplay::load(char const* path, bool quick, uint32* error)
{
  uint32 localError;
  if (error == NULL)
    error = &localError;
  *error = 0;

  File* file;
  if (cfg.enableUrl && File::isValidURL(path))
    file = File::openURL(path);
  else
    file = File::open(path, File::READ);
  W3GReplay* replay = (file ? load(file, quick, error) : NULL);
  if (file == NULL)
    *error = eNoFile;
  delete file;

  if (replay)
    replay->setPath(path);

  return replay;
}
void W3GReplay::setPath(char const* path)
{
  if (fileInfo == NULL)
    fileInfo = new FileInfo;
  ::getFileInfo(path, *fileInfo);
  getApp()->getCache()->addGame(this);
}
W3GReplay::W3GReplay(File* unpacked, W3GHeader const& header, bool quick, uint32* error)
{
  fileInfo = NULL;
  quickLoad = quick;
  replay = unpacked;
  memcpy(&hdr, &header, sizeof hdr);

  numPlayers = 0;
  memset(players, 0, sizeof players);

  replay->seek(5, SEEK_SET);

  loadPlayer();
  plist[0]->initiator = true;
  loadGame();

  if (validPlayers())
    dotaInfo = DotaInfo::getDota(game->map);
  else
    dotaInfo = NULL;

  if (dotaInfo)
  {
    dota = getApp()->getDotaLibrary()->getDota(dotaInfo->version, game->map);
    if (dota == NULL)
    {
      *error = eNoMap;
      return;
    }
    for (int i = 0; i < numPlayers; i++)
      plist[i]->actions.start(quick, plist[i]->slot.team);
  }
  else
    dota = NULL;

  blockPos = replay->tell();
  if (!parseBlocks())
  {
    *error = eBadFile;
    return;
  }

  analyze();
}
W3GReplay::~W3GReplay()
{
  for (int i = 0; i < numPlayers; i++)
    delete plist[i];
  delete replay;
  delete game;
  if (dota)
    dota->release();
}

void W3GReplay::loadPlayer()
{
  W3GPlayer* player = new W3GPlayer(replay);
  player->index = numPlayers;

  players[player->player_id] = player;
  plist[numPlayers++] = player;
}

W3GGame::W3GGame(File* f)
{
  name = f->readString();
  f->getc();

  char buf[2048];
  int blen = 0;
  int mask = 0;
  for (int i = 0; int c = f->getc(); i++)
  {
    if (i % 8 == 0)
      mask = c;
    else
      buf[blen++] = c - 1 + ((mask >> (i % 8)) & 1);
  }
  buf[blen] = 0;

  speed = buf[0];
  if (buf[1] & 1)
    visibility = VISIBILITY_HIDE;
  else if (buf[1] & 2)
    visibility = VISIBILITY_EXPLORED;
  else if (buf[1] & 4)
    visibility = VISIBILITY_VISIBLE;
  else
    visibility = VISIBILITY_DEFAULT;
  observers = ((buf[1] >> 4) & 3);
  teams_together = ((buf[1] & 64) != 0);
  lock_teams = (buf[2] != 0);
  shared_control = ((buf[3] & 1) != 0);
  random_hero = ((buf[3] & 2) != 0);
  random_races = ((buf[3] & 4) != 0);
  if (buf[3] & 64)
    observers = OBSERVERS_REFEREES;

  uint32 chksum = *(uint32*) (buf + 9);

  map = (buf + 13);
  int bpos = 0;
  while (buf[bpos])
    bpos++;
  creator = (buf + bpos + 1);

  slots = f->read_int32();
  game_type = f->getc();
  game_private = (f->getc() != 0);
  f->seek(6, SEEK_CUR);

  saver = NULL;
  winner = 0;
  ladder_winner = -1;
  ladder_wplayer = NULL;
  gmode = 0;
  end_time = 0;
  memset(ladder_lost, 0, sizeof ladder_lost);
}

void W3GReplay::loadGame()
{
  game = new W3GGame(replay);

  while ((game->record_id = replay->getc()) == 0x16)
  {
    loadPlayer();
    replay->seek(4, SEEK_CUR);
  }
  game->record_length = replay->read_int16();
  game->slot_records = replay->getc();

  for (int i = 0; i < int(game->slot_records); i++)
  {
    uint8 id = replay->getc();
    replay->seek(1, SEEK_CUR);
    W3GSlot slot(replay, hdr);
    if (slot.computer)
    {
      id = 128 + numPlayers;
      if (slot.ai_strength == AI_EASY)
        players[id] = new W3GPlayer(id, "Computer (Easy)");
      else if (slot.ai_strength == AI_NORMAL)
        players[id] = new W3GPlayer(id, "Computer (Normal)");
      else
        players[id] = new W3GPlayer(id, "Computer (Insane)");
      players[id]->index = numPlayers;
      plist[numPlayers++] = players[id];
    }
    if (slot.slot_status == 2 && players[id])
      memcpy(&players[id]->slot, &slot, sizeof slot);
  }

  game->random_seed = replay->read_int32();
  game->select_mode = replay->getc();
  game->start_spots = replay->getc();

  game->has_observers = false;
  for (int i = 0; i < numPlayers; i++)
  {
    if (plist[i]->slot.color > 11 ||
        plist[i]->slot.slot_status == 0)
      game->has_observers = true;
    for (int j = 0; j < 16; j++)
      plist[i]->share[j] = game->shared_control;
  }
}

DotaInfo* DotaInfo::getDota(String map)
{
  map.toLower();
  Array<String> sub;
  if (map.rfind("dota{{_| }allstars}?{_| }v(\\d)\\.(\\d\\d)([b-z]?)[^b-z]", 0, &sub) < 0)
    return NULL;
  DotaInfo* data = new DotaInfo;
  memset(data, 0, sizeof(DotaInfo));
  int major = sub[1].toInt();
  int minor = sub[2].toInt();
  int build = 0;
  if (!sub[3].isEmpty())
    build = int(sub[3][0] - 'a');
  data->version = makeVersion(major, minor, build);
  return data;
}
bool W3GReplay::validPlayers()
{
  for (int i = 0; i < numPlayers; i++)
  {
    if (plist[i]->slot.color > 11 || plist[i]->slot.slot_status == 0)
      continue;
    if (plist[i]->slot.color == 0 || plist[i]->slot.color == 6)
      return false;
    if (plist[i]->slot.team != (plist[i]->slot.color > 6 ? 1 : 0))
      return false;
  }
  return true;
}
void W3GReplay::addMessage(int type, int id, uint32 time, char const* fmt, ...)
{
  W3GMessage& msg = messages.push();
  msg.id = id;
  msg.mode = CHAT_NOTIFY;
  msg.notifyType = type;
  msg.time = time;

  va_list ap;
  va_start(ap, fmt);
  int len = _vscprintf(fmt, ap);
  msg.text.resize(len);
  vsprintf(msg.text.getBuffer(), fmt, ap);
  msg.text.setLength(len);
}

W3GHero* W3GReplay::newHero(int side, Dota::Hero* hero)
{
  for (int i = 0; i < heroes.length(); i++)
  {
    if (heroes[i].side == side && heroes[i].hero == hero && !heroes[i].claimed)
    {
      heroes[i].claimed = true;
      return &heroes[i];
    }
  }
  W3GHero& h = heroes.push();
  h.hero = hero;
  h.side = side;
  h.claimed = true;
  return &h;
}
W3GHero* W3GReplay::getHero(uint64 oid, int side, Dota::Hero* hero)
{
  for (int i = 0; i < heroes.length(); i++)
  {
    if (heroes[i].oid == 0 && heroes[i].side == side && heroes[i].hero == hero)
    {
      heroes[i].oid = oid;
      return &heroes[i];
    }
    if (heroes[i].oid == oid)
    {
      if (heroes[i].side == side)
        return &heroes[i];
      else
        return NULL;
    }
  }
  W3GHero& h = heroes.push();
  h.hero = hero;
  h.side = side;
  h.oid = oid;
  return &h;
}
void W3GReplay::analyze()
{
  if (game->saver == NULL)
  {
    Array<String> names;
    String(cfg.ownNames).split(names, " ,;");
    for (int i = 0; i < numPlayers && game->saver == NULL; i++)
    {
      for (int j = 0; j < names.length(); j++)
      {
        if (plist[i]->name.icompare(names[j]))
        {
          game->saver = plist[i];
          break;
        }
      }
    }
  }
  if (dotaInfo == NULL)
  {
    game->game_mode = "";
    if (game->ladder_winner == 15)
      game->ladder_winner = (game->saver ? game->saver->slot.team : -1);
    if (game->ladder_winner < 0)
    {
      bool found = false;
      for (int i = 0; i < numPlayers; i++)
      {
        if (!game->ladder_lost[plist[i]->slot.team])
        {
          if (!found)
            game->ladder_winner = plist[i]->slot.team;
          else
            game->ladder_winner = -1;
          found = true;
        }
      }
      found = false;
      for (int i = 0; i < numPlayers; i++)
      {
        if (plist[i]->slot.team == game->ladder_winner)
        {
          if (!found)
            game->ladder_wplayer = plist[i];
          else
            game->ladder_winner = NULL;
          found = true;
        }
      }
    }
    for (int i = 0; i < numPlayers; i++)
      if (plist[i]->race == 0)
        plist[i]->race = plist[i]->slot.race;
    return;
  }
  if (dotaInfo->end_time == 0)
    dotaInfo->end_time = game->end_time;
  for (int i = 0; i < numPlayers; i++)
    if (plist[i]->time > dotaInfo->end_time)
      plist[i]->time = dotaInfo->end_time;
  if ((game->gmode & (~MODE_WTF)) == 0)
  {
    if (game->gmode & MODE_WTF)
      game->game_mode = "-wtf";
    else
      game->game_mode = "Normal mode";
  }
  else if (game->gmode & MODE_WTF)
    game->game_mode += " -wtf";
  for (int i = 0; i < numPlayers; i++)
  {
    if (plist[i]->slot.color > 11 || plist[i]->slot.slot_status == 0)
      continue;
    Dota::Hero* hero = dota->getHeroById(plist[i]->heroId);
    if (plist[i]->hero == NULL)
    {
      for (int j = 0; j < heroes.length(); j++)
        if ((plist[i]->hero == NULL ||
             heroes[j].actions[plist[i]->index] > plist[i]->hero->actions[plist[i]->index]) &&
            !heroes[j].claimed)
          plist[i]->hero = &heroes[j];
    }
    if (hero && (plist[i]->hero == NULL || plist[i]->hero->hero != hero))
    {
      for (int j = 0; j < heroes.length(); j++)
      {
        if (heroes[j].hero == hero)
        {
          plist[i]->hero = &heroes[j];
          break;
        }
      }
    }
  }
  for (int i = 0; i < heroes.length(); i++)
  {
    W3GPlayer* player = NULL;
    W3GPlayer* uplayer = NULL;
    for (int j = 0; j < numPlayers; j++)
    {
      if (plist[j]->hero && plist[j]->hero->hero == heroes[i].hero)
      {
        if (player == NULL || heroes[i].actions[player->index] < heroes[i].actions[j])
          player = plist[j];
        Dota::Hero* hero = dota->getHeroById(plist[j]->heroId);
        if (hero && heroes[i].hero == hero)
          uplayer = plist[j];
      }
    }
    if (uplayer)
      player = uplayer;
    for (int j = 0; j < numPlayers; j++)
      if (plist[j]->hero && plist[j]->hero->hero == heroes[i].hero && plist[j] != player)
        plist[j]->hero = NULL;
  }
  for (int i = 0; i < numPlayers; i++)
  {
    if (plist[i]->slot.color > 11 || plist[i]->slot.slot_status == 0)
      continue;
    if (plist[i]->hero)
      plist[i]->hero->setPlayer(plist[i]);
    int team = plist[i]->slot.color / 6;
    int pos = dotaInfo->team_size[team];
    while (pos > 0)
    {
      if (dotaInfo->teams[team][pos - 1]->slot.color < plist[i]->slot.color)
        break;
      dotaInfo->teams[team][pos] = dotaInfo->teams[team][pos - 1];
      pos--;
    }
    dotaInfo->team_size[team]++;
    dotaInfo->teams[team][pos] = plist[i];
    dotaInfo->team_kills[team] += plist[i]->stats[STAT_KILLS];
    dotaInfo->team_kills[1 - team] += plist[i]->sdied;
    if (plist[i]->said_ff)
      dotaInfo->team_ff[team]++;

    plist[i]->lane = plist[i]->actions.getLane();
    plist[i]->item_cost = plist[i]->stats[STAT_GOLD];
    for (int j = 0; j < plist[i]->inv.items.length(); j++)
      plist[i]->item_cost += plist[i]->inv.items[j].item->cost;

    if (!dotaInfo->item_data && plist[i]->hero)
    {
      plist[i]->inv.compute(0x7FFFFFFF, dota);
      for (int j = 0; j < 6; j++)
        plist[i]->inv.final[j] = plist[i]->inv.computed[j];
    }
  }
  if (game->winner == 0)
  {
    if (dotaInfo->team_ff[0] >= 4 && dotaInfo->team_ff[1] <= 2)
      game->winner = WINNER_GSENTINEL;
    if (dotaInfo->team_ff[1] >= 4 && dotaInfo->team_ff[0] <= 2)
      game->winner = WINNER_GSCOURGE;
  }
  if (game->winner == 0)
  {
    int inSentinel = 0;
    int inScourge = 0;
    float x, y;
    for (int i = 0; i < dotaInfo->team_size[0]; i++)
      if (dotaInfo->teams[0][i]->actions.getPosition(dotaInfo->end_time, x, y) == STATE_ALIVE &&
          x > 2200 && y > 2200)
        inSentinel++;
    for (int i = 0; i < dotaInfo->team_size[1]; i++)
      if (dotaInfo->teams[1][i]->actions.getPosition(dotaInfo->end_time, x, y) == STATE_ALIVE &&
          x < -2800 && y < -2800)
        inScourge++;
    if (inSentinel >= 4 && inScourge <= 2)
      game->winner = WINNER_PSENTINEL;
    if (inScourge >= 4 && inSentinel <= 2)
      game->winner = WINNER_PSCOURGE;
  }
}

W3GPlayer* W3GReplay::getCaptain(int team)
{
  W3GPlayer* best = NULL;
  int aMin = (team == 0 ? 1 : 7);
  int aMax = (team == 0 ? 5 : 11);
  for (int i = 0; i < numPlayers; i++)
  {
    if (plist[i]->slot.color >= aMin && plist[i]->slot.color <= aMax)
    {
      best = plist[i];
      aMax = plist[i]->slot.color - 1;
    }
  }
  return best;
}
W3GPlayer* W3GReplay::getPlayerInSlot(int slot)
{
  for (int i = 0; i < numPlayers; i++)
    if (plist[i]->slot.color == slot)
      return plist[i];
  return NULL;
}
uint32 W3GReplay::getLength(bool throne)
{
  if (throne && dotaInfo)
    return dotaInfo->end_time;
  return game->end_time;
}
String W3GReplay::formatTime(uint32 time, int flags)
{
  long atime = time;
  if (cfg.relTime && dotaInfo)
    atime -= (long) dotaInfo->start_time;
  return format_time(atime, flags);
}

int W3GReplay::getFirstMessage(uint32 time) const
{
  int left = 0;
  int right = messages.length() - 1;
  if (right < 0 || messages[0].time > time)
    return -1;
  while (left < right)
  {
    int m = (left + right + 1) / 2;
    if (messages[m].time > time)
      right = m - 1;
    else
      left = m;
  }
  return left;
}

int W3GReplay::getFirstWard(uint32 time) const
{
  int left = 0;
  int right = wards.length() - 1;
  if (right < 0 || wards[0].time > time)
    return -1;
  while (left < right)
  {
    int m = (left + right + 1) / 2;
    if (wards[m].time > time)
      right = m - 1;
    else
      left = m;
  }
  return left;
}
