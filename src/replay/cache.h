#ifndef __REPLAY_CACHE__
#define __REPLAY_CACHE__

#include "base/types.h"
#include "base/string.h"
#include "base/dictionary.h"
#include "replay/replay.h"

#include <windows.h>

struct GameCache
{
  uint64 ftime;

  String game_name;
  uint32 game_length;
  uint64 game_mode;
  uint32 map_version;
  uint32 wc3_version;
  uint8 winner;

  uint8 players;
  String pname[10];
  uint8 pteam[10];
  uint8 phero[10];
  uint16 pstats[10][7];
  uint32 ptime[10];
  uint8 plane[10];
  uint8 plevel[10];
  uint32 pgold[10];
  uint16 papm[10];
};

class CacheManager
{
  Dictionary<GameCache> cache;
  GameCache temp;
  CRITICAL_SECTION lock;
  bool wantReplay(String path, uint64 ftime);
  void readReplay(W3GReplay* replay, GameCache& gc);
public:
  CacheManager();
  ~CacheManager();

  GameCache* getGame(String path, GameCache* dst = NULL);
  GameCache* getGameNow(String path);
  void addGame(W3GReplay* replay);
};

#endif // __REPLAY_CACHE__
