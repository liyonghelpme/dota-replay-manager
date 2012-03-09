#include "hero.h"
#include "replay/player.h"

W3GHero::W3GHero()
{
  hero = NULL;
  side = 0;
  oid = 0;
  claimed = false;
  level = 0;
  memset(levelTime, 0, sizeof levelTime);
  memset(actions, 0, sizeof actions);
  memset(skills, 0, sizeof skills);
}
void W3GHero::setPlayer(W3GPlayer* player)
{
  level = player->level;
  for (int i = 1; i < level; i++)
    levelTime[i] = player->level_time[i];
}
void W3GHero::pushAbility(Dota::Ability* ability, uint32 time)
{
  SkillInfo& skill = slearn.push();
  skill.skill = ability;
  skill.time = time;
}
/*
void W3GHero::undoFrom (int pos)
{
  for (int i = pos; i < nLearned; i++)
    if (learned[i].pos >= 0)
      abilities[learned[i].pos] = 0;
  while (level > 0 && abilities[level - 1] == 0)
    level--;
}
bool W3GHero::fixFrom (int pos)
{
  bool res = true;
  for (int i = pos; i < nLearned; i++)
  {
    if (i > 0 && int (learned[i].time - learned[i - 1].time) < repDelay && learned[i].id == learned[i - 1].id)
    {
      if (fixFrom (i + 1))
      {
        learned[i].pos = -1;
        return true;
      }
      else
        undoFrom (i + 1);
    }
    int cnt = 0;
    for (int j = 0; j < level; j++)
      if (abilities[j] != 0 && getAbility (abilities[j])->slot == learned[i].slot)
        cnt++;
    int mlvl = getMinLevel (cnt, learned[i].slot, learned[i].id);
    bool placed = false;
    if (mlvl < 32)
    {
      while (level < mlvl)
      {
        abilities[level] = 0;
        atime[level++] = learned[i].time;
      }
      while (mlvl < level)
      {
        if (abilities[mlvl] == 0)
        {
          abilities[mlvl] = learned[i].id;
          learned[i].pos = mlvl;
          atime[mlvl] = learned[i].time;
          placed = true;
          break;
        }
        mlvl++;
      }
      if (!placed && level < 25)
      {
        abilities[level] = learned[i].id;
        learned[i].pos = level;
        atime[level++] = learned[i].time;
        placed = true;
      }
    }
    if (placed && abilities[learned[i].pos + 1] != 0 && atime[learned[i].pos + 1] + 30000 < learned[i].time)
      res = false;
    else if (!placed)
      learned[i].pos = -1;
  }
  return res;
}
void W3GHero::fixAbilities ()
{
  fixFrom (0);
}
*/
void W3GHero::process()
{
  memset(skills, 0, sizeof skills);
  if (level)
  {
    int curLevel = 0;
    int points = 0;
    for (int i = 0; i < slearn.length(); i++)
    {
      while (curLevel < level && levelTime[curLevel + 1] < slearn[i].time)
      {
        curLevel++;
        points++;
      }
      if (points)
      {
        int skillLevel = 0;
        for (int j = 0; j <= curLevel; j++)
          if (skills[j].skill == slearn[i].skill)
            skillLevel++;
        if (curLevel < slearn[i].skill->lvlMax)
        {
          int reqLevel = slearn[i].skill->lvlStart + slearn[i].skill->lvlSkip * skillLevel;
          for (int j = reqLevel; j <= curLevel; j++)
          {
            if (skills[j].skill == NULL)
            {
              skills[j].skill = slearn[i].skill;
              skills[j].time = slearn[i].time;
            }
          }
        }
      }
    }
  }
  slearn.clear();
}
