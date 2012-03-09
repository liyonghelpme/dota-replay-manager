#ifndef __REPLAY_HERO__
#define __REPLAY_HERO__

#include "dota/dotadata.h"

struct W3GPlayer;

struct W3GHero
{
  Dota::Hero* hero;
  uint64 oid;
  bool claimed;
  int side;

  int level;
  uint32 levelTime[32];
  struct SkillInfo
  {
    Dota::Ability* skill;
    uint32 time;
  };
  Array<SkillInfo> slearn;
  SkillInfo skills[32];

  int actions[16];

  W3GHero();
  void pushAbility(Dota::Ability* ability, uint32 time);
  void setPlayer(W3GPlayer* player);
  void process();
};

#endif // __REPLAY_HERO__
