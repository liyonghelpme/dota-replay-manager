#ifndef __REPLAY_PLAYER_H__
#define __REPLAY_PLAYER_H__

#include <string.h>
#include "base/types.h"
#include "base/array.h"

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
};
struct CompositeItem
{
  int id;
  uint32 time;
  bool gone;
  bool pregone;
  bool tpregone;
  CompositeItem() {}
  CompositeItem(int i, uint32 t)
  {
    id = i;
    time = t;
    gone = false;
    pregone = false;
    tpregone = false;
  }
};
struct W3GSkill
{
  int id;
  int slot;
  uint32 time;
  int pos;
};
struct DotaData;
struct W3GHero
{
  int id;
  uint32 oid1, oid2;
  int level;
  int abilities[32];
  uint32 atime[32];
  W3GSkill learned[64];
  int nLearned;
  int side;
  bool claimed;

  int cur_lvl;
  int levels[5];

  int actions[16];

  W3GHero()
  {
    reset();
  }
  void reset()
  {
    oid1 = 0;
    oid2 = 0;
    level = 0;
    cur_lvl = 0;
    nLearned = 0;
    claimed = false;
    memset(actions, 0, sizeof actions);
    memset(abilities, 0, sizeof abilities);
  }

  void pushAbility(int id, uint32 time, int slot);
  void fixAbilities();
  bool fixFrom(int pos);
  void undoFrom(int pos);
  bool compute(uint32 time, DotaData const& dota);
};
struct W3GInventory
{
  int items[256];
  unsigned long itemt[256];
  int num_items;
  Array<CompositeItem> bi;

  int inv[6];

  W3GInventory()
  {
    reset();
  }
  void reset()
  {
    num_items = 0;
    bi.clear();
  }

  void getCompositeItems(unsigned long time, DotaData const& dota);
  void getAllItems(unsigned long time, DotaData const& dota);
  void listItems(unsigned long time);
  void sortItems();
  bool compute(unsigned long time, DotaData const& dota);
};

struct W3GId
{
  uint32 id1;
  uint32 id2;
};
struct W3GPlayer
{
  String name;
  String orgname;

  uint8 player_id;
  bool initiator;
  sint32 exe_runtime;
  sint32 race;
  int actions;
  W3GSlot slot;
  W3GHero* curSel;
  int numWards;
  int heroId;

  uint32 deathTime[128];
  uint32 deathEnd[128];
  int nDeaths;
  int curLevel;

  int hkassign[16];
  int hkuse[16];

  int pkilled[256];
  int pdied[256];
  int sdied;
  int skilled;

  bool saidFF;
  bool saidGG;

  int stats[16];
  uint32 ltime[32];
  int lCount;

  uint32 time;
  long leave_reason;
  long leave_result;
  bool left;

  bool share[16];

  int acounter[NUM_ACTIONS];
  W3GHero* hero;

  W3GId sel[16];
  int numSel;
  int curTab;

  int lane;
  int counted;
  float avgClickX;
  float avgClickY;

  int itemCost;

  int index;

  int afterLeave;
  int finalItems[6];
  W3GInventory inv;

  W3GPlayer()
  {
    memset(&player_id, 0, ((char*) &inv) - ((char*) &player_id));
    lCount = 1;
  }
};

#endif // __REPLAY_PLAYER_H__
