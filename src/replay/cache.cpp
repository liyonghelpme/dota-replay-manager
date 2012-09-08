#include "core/app.h"

#include "cache.h"

#include <stdio.h>

bool CacheManager::wantReplay(String path, uint64 ftime)
{
  path = String::fixPath(path);
  if (String::getExtension(path).icompare(".w3g"))
    return false;
  if (path.substr(0, cfg.replayPath.length()).icompare(cfg.replayPath))
    return false;
  FileInfo info;
  getFileInfo(path, info);
  return ftime == info.ftime;
}
void CacheManager::readReplay(W3GReplay* replay, GameCache& gc)
{
  FileInfo* fileInfo = replay->getFileInfo();
  if (fileInfo)
    gc.ftime = fileInfo->ftime;
  else
    gc.ftime = 0;

  W3GGame* gameInfo = replay->getGameInfo();
  DotaInfo const* dotaInfo = replay->getDotaInfo();

  gc.game_name = gameInfo->name;
  gc.game_length = gameInfo->end_time;
  gc.game_mode = gameInfo->gmode;
  gc.map_version = (dotaInfo ? dotaInfo->version : 0);
  gc.wc3_version = replay->getVersion();
  gc.winner = gameInfo->winner;

  gc.players = 0;
  gc.host = -1;
  gc.saver = -1;
  for (int i = 0; i < replay->getNumPlayers() && gc.players < 10; i++)
  {
    W3GPlayer* player = replay->getPlayer(i);
    if (player->slot.slot_status != 0 && player->slot.color < 12)
    {
      int p = gc.players++;

      gc.pname[p] = player->name;
      gc.pteam[p] = (player->slot.color < 6 ? 0 : 1);
      gc.phero[p] = (player->hero ? player->hero->hero->point : 0);
      for (int s = 0; s < 7; s++)
        gc.pstats[p][s] = player->stats[s];
      gc.ptime[p] = player->time;
      gc.plane[p] = player->lane;
      gc.plevel[p] = (player->hero ? player->hero->level : 0);
      gc.pgold[p] = player->item_cost;
      gc.papm[p] = player->apm();
    }

    if (player->name == replay->getGameInfo()->creator)
      gc.host = i;
    if (player == replay->getGameInfo()->saver)
      gc.saver = i;
  }
}
CacheManager::CacheManager()
  : cache(DictionaryMap::pathName)
{
  InitializeCriticalSection(&lock);
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "gamecache.dat"), File::READ);
  if (file)
  {
    int count = file->read_int32();
    for (int i = 0; i < count; i++)
    {
      int size = file->read_int32();
      int end = file->tell() + size;

      String path = file->readString();
      uint64 ftime;
      file->read(&ftime, sizeof ftime);
      if (wantReplay(path, ftime))
      {
        GameCache& gc = cache.create(path);
        gc.ftime = ftime;

        gc.game_name = file->readString();
        file->read(&gc.game_length, sizeof gc.game_length);
        file->read(&gc.game_mode, sizeof gc.game_mode);
        file->read(&gc.map_version, sizeof gc.map_version);
        file->read(&gc.wc3_version, sizeof gc.wc3_version);
        file->read(&gc.winner, sizeof gc.winner);

        file->read(&gc.players, sizeof gc.players);
        file->read(&gc.host, sizeof gc.host);
        file->read(&gc.saver, sizeof gc.saver);
        for (uint8 i = 0; i < gc.players; i++)
        {
          gc.pname[i] = file->readString();
          file->read(&gc.pteam[i], sizeof gc.pteam[i]);
          file->read(&gc.phero[i], sizeof gc.phero[i]);
          file->read(&gc.pstats[i], sizeof gc.pstats[i]);
          file->read(&gc.ptime[i], sizeof gc.ptime[i]);
          file->read(&gc.plane[i], sizeof gc.plane[i]);
          file->read(&gc.plevel[i], sizeof gc.plevel[i]);
          file->read(&gc.pgold[i], sizeof gc.pgold[i]);
          file->read(&gc.papm[i], sizeof gc.papm[i]);
        }
      }

      file->seek(end, SEEK_SET);
    }
    delete file;
  }
}
CacheManager::~CacheManager()
{
  EnterCriticalSection(&lock);
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "gamecache.dat"), File::REWRITE);
  if (file)
  {
    int count = 0;
    file->write_int32(count);
    for (uint32 i = cache.enumStart(); i; i = cache.enumNext(i))
    {
      int start = file->tell();
      file->write_int32(0);

      file->writeString(cache.enumGetKey(i));
      GameCache& gc = cache.enumGetValue(i);

      file->write(&gc.ftime, sizeof gc.ftime);

      file->writeString(gc.game_name);
      file->write(&gc.game_length, sizeof gc.game_length);
      file->write(&gc.game_mode, sizeof gc.game_mode);
      file->write(&gc.map_version, sizeof gc.map_version);
      file->write(&gc.wc3_version, sizeof gc.wc3_version);
      file->write(&gc.winner, sizeof gc.winner);

      file->write(&gc.players, sizeof gc.players);
      file->write(&gc.host, sizeof gc.host);
      file->write(&gc.saver, sizeof gc.saver);
      for (uint8 i = 0; i < gc.players; i++)
      {
        file->writeString(gc.pname[i]);
        file->write(&gc.pteam[i], sizeof gc.pteam[i]);
        file->write(&gc.phero[i], sizeof gc.phero[i]);
        file->write(&gc.pstats[i], sizeof gc.pstats[i]);
        file->write(&gc.ptime[i], sizeof gc.ptime[i]);
        file->write(&gc.plane[i], sizeof gc.plane[i]);
        file->write(&gc.plevel[i], sizeof gc.plevel[i]);
        file->write(&gc.pgold[i], sizeof gc.pgold[i]);
        file->write(&gc.papm[i], sizeof gc.papm[i]);
      }

      int end = file->tell();
      file->seek(start, SEEK_SET);
      file->write_int32(end - start - 4);
      file->seek(end, SEEK_SET);
      count++;
    }
    file->seek(0, SEEK_SET);
    file->write_int32(count);
    delete file;
  }
  LeaveCriticalSection(&lock);
  DeleteCriticalSection(&lock);
}

GameCache* CacheManager::getGame(String path, GameCache* dst)
{
  FileInfo info;
  getFileInfo(path, info);
  GameCache* result = NULL;
  EnterCriticalSection(&lock);
  if (cache.has(path) && cache.get(path).ftime == info.ftime)
    result = &cache.get(path);
  LeaveCriticalSection(&lock);
  if (result == NULL)
  {
    W3GReplay* replay = W3GReplay::load(path, true);
    if (replay)
    {
      EnterCriticalSection(&lock);
      if (cache.has(path) && cache.get(path).ftime == info.ftime)
        result = &cache.get(path);
      else
      {
        if (dst == NULL)
          dst = &temp;
        readReplay(replay, *dst);
        result = dst;
      }
      LeaveCriticalSection(&lock);
      delete replay;
    }
  }
  return result;
}
GameCache* CacheManager::getGameNow(String path)
{
  FileInfo info;
  getFileInfo(path, info);
  GameCache* result = NULL;
  EnterCriticalSection(&lock);
  if (cache.has(path) && cache.get(path).ftime == info.ftime)
    result = &cache.get(path);
  LeaveCriticalSection(&lock);
  return result;
}
void CacheManager::addGame(W3GReplay* replay)
{
  FileInfo* info = replay->getFileInfo();
  if (info && wantReplay(info->path, info->ftime))
  {
    EnterCriticalSection(&lock);
    GameCache& gc = cache.create(info->path);
    readReplay(replay, gc);
    LeaveCriticalSection(&lock);
  }
}
void CacheManager::duplicate(String path, GameCache* game)
{
  EnterCriticalSection(&lock);
  GameCache& gc = cache.create(path);
  gc = *game;
  LeaveCriticalSection(&lock);
}

void CacheManager::enumCache(bool (*func)(String, GameCache*, void*), void* param)
{
  EnterCriticalSection(&lock);
  for (uint32 cur = cache.enumStart(); cur; cur = cache.enumNext(cur))
    if (!func(cache.enumGetKey(cur), &cache.enumGetValue(cur), param))
      break;
  LeaveCriticalSection(&lock);
}

////////////////////////////////////////////////////

String GameCache::format(char const* fmt, char const* dst, char const* src)
{
  String result = "";
  int npos = -1;
  for (int i = 0; fmt[i]; i++)
  {
    if (fmt[i] == '<')
    {
      int prev = i++;
      String tag = "";
      while (fmt[i] && fmt[i] != '>')
        tag += fmt[i++];
      if (fmt[i] != '>')
        tag = "";

      if (tag.icompare("n") == 0)
        npos = result.length();
      else if (tag.icompare("version") == 0)
        result += formatVersion(wc3_version);
      else if (tag.icompare("map") == 0)
        result += formatVersion(map_version);
      else if (tag.icompare("name") == 0)
        result += game_name;
      else if (tag.icompare("sentinel") == 0)
      {
        int count = 0;
        for (int i = 0; i < players; i++)
          if (pteam[i] == 0)
            count++;
        result += String(count);
      }
      else if (tag.icompare("scourge") == 0)
      {
        int count = 0;
        for (int i = 0; i < players; i++)
          if (pteam[i] == 1)
            count++;
        result += String(count);
      }
      else if (tag.icompare("sentinel kills") == 0)
      {
        int count = 0;
        for (int i = 0; i < players; i++)
          if (pteam[i] == 1)
            count += pstats[i][STAT_DEATHS];
        result += String(count);
      }
      else if (tag.icompare("scourge kills") == 0)
      {
        int count = 0;
        for (int i = 0; i < players; i++)
          if (pteam[i] == 0)
            count += pstats[i][STAT_DEATHS];
        result += String(count);
      }
      else if (tag.substring(0, 5).icompare("time ") == 0)
        result += format_systime(ftime, tag.substring(5));
      else if (tag.icompare("winner") == 0)
      {
        if (winner == WINNER_SENTINEL || WINNER_GSENTINEL || WINNER_PSENTINEL)
          result += "Sentinel";
        else if (winner == WINNER_SCOURGE || WINNER_GSCOURGE || WINNER_PSCOURGE)
          result += "Scourge";
        else
          result += "Unknown";
      }
      else if (tag.substring(0, 7).icompare("player ") == 0 ||
               tag.substring(0, 5).icompare("hero ") == 0 ||
               tag.substring(0, 4).icompare("win ") == 0 ||
               tag.substring(0, 6).icompare("kills ") == 0 ||
               tag.substring(0, 7).icompare("deaths ") == 0 ||
               tag.substring(0, 7).icompare("creeps ") == 0 ||
               tag.substring(0, 7).icompare("denies ") == 0)
      {
        int id = -1;
        String cmd = tag.substring(0, tag.indexOf(' ')).toLower();
        String pid = tag.substring(tag.indexOf(' ') + 1).toLower();
        if (pid == "host")
          id = host;
        else if (pid == "saver")
          id = saver;
        else
          id = pid.toInt() - 1;
        if (id >= 0 && id < players)
        {
          if (cmd == "player")
            result += pname[id];
          else if (cmd == "hero")
          {
            Dota* dota = getApp()->getDotaLibrary()->getDota();
            Dota::Hero* hero = dota->getHero(phero[id]);
            if (hero)
              result += hero->name;
            dota->release();
          }
          else if (cmd == "win")
          {
            if (winner == WINNER_SENTINEL || WINNER_GSENTINEL || WINNER_PSENTINEL)
              result += (pteam[id] == 0 ? "Won" : "Lost");
            else if (winner == WINNER_SCOURGE || WINNER_GSCOURGE || WINNER_PSCOURGE)
              result += (pteam[id] == 1 ? "Won" : "Lost");
            else
              result += "?";
          }
          else if (cmd == "kills")
            result += String(pstats[id][STAT_KILLS]);
          else if (cmd == "deaths")
            result += String(pstats[id][STAT_DEATHS]);
          else if (cmd == "creeps")
            result += String(pstats[id][STAT_CREEPS]);
          else if (cmd == "denies")
            result += String(pstats[id][STAT_DENIES]);
        }
      }
      else if (tag.icompare("file") == 0)
        result += (src ? String::getFileTitle(src) : "");
      else if (tag.icompare("path") == 0)
        result += (src ? String::getPath(src) : "");
      else
        i = prev;
    }
    else if (fmt[i] == '/')
      result += '\\';
    else if (fmt[i] != '>' && fmt[i] != ':' && fmt[i] != '*' && fmt[i] != '?' &&
             fmt[i] != '"' && fmt[i] != '|' && fmt[i] >= ' ')
      result += fmt[i];
  }

  result += ".w3g";
  if (npos >= 0 && dst)
  {
    String fmt = result;
    for (int number = 1;; number++)
    {
      result = fmt;
      result.insert(npos, String(number));
      FILE* test = fopen(String::buildFullName(dst, result), "rb");
      if (test)
        fclose(test);
      else
        break;
    }
  }
  return result;
}
