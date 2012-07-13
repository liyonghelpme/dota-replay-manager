#ifndef __REPLAY_REPLAY_H__
#define __REPLAY_REPLAY_H__

#include "base/file.h"
#include "base/version.h"
#include "base/utils.h"
#include "replay/consts.h"
#include "replay/player.h"
#include "replay/message.h"
#include "replay/hero.h"
#include "replay/timeline.h"
#include "dota/buildings.h"
#include "dota/dotadata.h"

struct W3GHeader
{
  char intro[28];
  uint32 header_size;
  uint32 c_size;
  uint32 header_v;
  uint32 u_size;
  uint32 blocks;
  uint32 ident;
  uint32 major_v;
  uint16 minor_v;
  uint16 build_v;
  uint16 flags;
  uint32 length;
  uint32 checksum;

  W3GHeader()
  {
    memset(this, 0, sizeof(W3GHeader));
  }
  bool read(File* f);
  void write(File* f);
};

struct W3GGame
{
  String name;

  uint8 speed;
  uint8 visibility;
  uint8 observers;
  bool teams_together;
  bool lock_teams;
  bool shared_control;
  bool random_hero;
  bool random_races;
  String map;
  String creator;

  int slots;
  uint8 game_type;
  bool game_private;
  uint32 end_time;

  uint8 record_id;
  sint16 record_length;
  uint8 slot_records;

  uint32 random_seed;
  uint8 select_mode;
  uint8 start_spots;
  bool has_observers;

  W3GPlayer* saver;

  int ladder_winner;
  W3GPlayer* ladder_wplayer;
  bool ladder_lost[16];
  int winner;

  uint64 gmode;
  String game_mode;

  W3GGame(File* f);
};

struct DraftData
{
  Dota::Hero* pool[64];
  int numPool;
  Dota::Hero* bans[2][64];
  int numBans[2];
  Dota::Hero* picks[2][64];
  int numPicks[2];
  int firstPick;
};
struct DotaInfo
{
  W3GPlayer* teams[2][8];
  int team_size[2];
  int team_kills[2];
  int team_ff[2];

  uint32 bdTime[NUM_BUILDINGS];

  uint32 version;

  bool endgame;
  bool item_data;

  uint32 start_time;
  uint32 end_time;

  W3GPlayer* switch_who;
  W3GPlayer* switch_with;
  uint32 switch_start;

  DraftData draft;

  static DotaInfo* getDota(String map);
};

class W3GReplay
{
  static File* unpack(File* replay, W3GHeader& hdr);

  bool quickLoad;
  W3GHeader hdr;
  File* replay;

  W3GGame* game;
  DotaInfo* dotaInfo;
  Dota* dota;

  W3GPlayer* players[256];
  W3GPlayer* plist[16];
  int numPlayers;

  Array<W3GMessage> messages;
  void addMessage(int type, int id, uint32 time, char const* fmt, ...);

  PtrArray<W3GHero> heroes;
  W3GHero* newHero(int side, Dota::Hero* hero);
  W3GHero* getHero(uint64 oid, int side, Dota::Hero* hero);

  Array<W3GWard> wards;

  void loadPlayer();
  void loadGame();
  bool validPlayers();

  int blockPos;
  bool parseBlocks();
  bool parseActions(int length, void* state);
  void parseEndgame(String slot, String stat, uint32 value, void* state);
  void analyze();

  void parseMode(W3GPlayer* player, char const* text);

  W3GReplay(File* unpacked, W3GHeader const& header, bool quick);
public:
  static W3GReplay* load(File* replay, bool quick = false);
  static W3GReplay* load(char const* path, bool quick = false);
  ~W3GReplay();

  W3GPlayer* getCaptain(int team);
  W3GPlayer* getPlayerInSlot(int slot);
  int getNumPlayers() const
  {
    return numPlayers;
  }
  W3GPlayer* getPlayer(int i)
  {
    return plist[i];
  }
  W3GPlayer* getPlayerById(int id)
  {
    return players[id];
  }

  W3GGame* getGameInfo() const
  {
    return game;
  }
  DotaInfo const* getDotaInfo() const
  {
    return dotaInfo;
  }
  Dota* getDotaData()
  {
    return dota;
  }

  uint32 getVersion() const
  {
    return makeVersion(1, hdr.major_v, 0);
  }

  int getNumMessages() const
  {
    return messages.length();
  }
  W3GMessage& getMessage(int i)
  {
    return messages[i];
  }
  int getFirstMessage(uint32 time) const;

  int getNumWards() const
  {
    return wards.length();
  }
  W3GWard& getWard(int i)
  {
    return wards[i];
  }
  int getFirstWard(uint32 time) const;

  String formatTime(uint32 time, int flags = TIME_SECONDS);

  uint32 getLength(bool throne = true);
};

#endif // __REPLAY_REPLAY_H__
