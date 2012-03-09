#ifndef __REPLAY_PLAYER_H__
#define __REPLAY_PLAYER_H__

#include <string.h>
#include "base/types.h"
#include "base/array.h"
#include "base/file.h"
#include "dota/dotadata.h"
#include "replay/hero.h"
#include "replay/inventory.h"
#include "replay/consts.h"

#include "replay/timeline.h"

struct W3GHeader;
struct W3GSlot
{
  uint8 slot_status;
  uint8 computer;
  uint8 team;
  uint8 color;
  uint8 race;
  uint8 ai_strength;
  uint8 handicap;

  uint8 org_color;

  W3GSlot() {}
  W3GSlot(File* f, W3GHeader const& hdr);
};
struct W3GPlayer
{
  uint8 player_id;
  bool initiator;
  sint32 race;
  W3GSlot slot;

  String name;
  String org_name;

  int index;

  uint32 time;
  bool left;
  int leave_reason;
  int leave_result;

  bool share[16];
  bool said_ff;

  int num_actions;
  W3GActionList actions;
  int stats[16];
  int sdied;
  int skilled;
  int pkilled[16];
  int pdied[16];
  int lane;
  int item_cost;

  int level;
  uint32 level_time[32];

  W3GHero* hero;
  W3GHero* sel;
  uint32 heroId;

  W3GInventory inv;

  String format() const;
  String format_full() const;

  W3GPlayer(uint8 id, String name);
  W3GPlayer(File* f);
private:
  void init();
};

#endif // __REPLAY_PLAYER_H__
