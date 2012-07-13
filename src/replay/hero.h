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
    int pos;
  };
  Array<SkillInfo> slearn;
  SkillInfo skills[32];

  int currentLevel;
  int skillLevels[5];
  void compute(uint32 time);

  int actions[16];

  W3GHero();
  void pushAbility(Dota::Ability* ability, uint32 time);
  void setPlayer(W3GPlayer* player);
private:
  void undo(int pos);
  bool fix(int pos);
};

#endif // __REPLAY_HERO__
