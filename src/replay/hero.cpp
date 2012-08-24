#include "hero.h"
#include "replay/player.h"
#include "core/registry.h"

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
  for (int i = 0; i < slearn.length(); i++)
    slearn[i].pos = -1;
  memset(skills, 0, sizeof skills);
  if (player->level)
  {
    level = player->level;
    for (int i = 1; i <= level; i++)
      levelTime[i] = player->level_time[i];
    levelTime[1] = 0;
  }
  else
  {
    fix(0);
    level = 0;
    for (int i = 1; i <= 25; i++)
    {
      if (skills[i].skill)
      {
        for (int j = i; j > level || skills[i].time < levelTime[j]; j--)
          levelTime[j] = skills[i].time;
        level = i;
      }
    }
    levelTime[1] = 0;
  }
  memset(skills, 0, sizeof skills);
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
      for (int j = 1; j <= curLevel; j++)
        if (skills[j].skill == slearn[i].skill)
          skillLevel++;
      if (skillLevel < slearn[i].skill->lvlMax)
      {
        int reqLevel = slearn[i].skill->lvlStart + slearn[i].skill->lvlSkip * skillLevel;
        for (int j = reqLevel; j <= curLevel; j++)
        {
          if (skills[j].skill == NULL)
          {
            skills[j].skill = slearn[i].skill;
            skills[j].time = slearn[i].time;
            break;
          }
        }
      }
    }
  }
}
void W3GHero::pushAbility(Dota::Ability* ability, uint32 time)
{
  SkillInfo& skill = slearn.push();
  skill.skill = ability;
  skill.time = time;
}
void W3GHero::undo(int pos)
{
  for (int i = pos; i < slearn.length(); i++)
  {
    if (slearn[i].pos >= 0)
    {
      skills[slearn[i].pos].skill = NULL;
      slearn[i].pos = -1;
    }
  }
}
bool W3GHero::fix(int pos)
{
  for (int i = pos; i < slearn.length(); i++)
  {
    if (i > 0 && int(slearn[i].time - slearn[i - 1].time) < cfg.repDelay &&
        slearn[i].skill == slearn[i - 1].skill)
    {
      if (fix(i + 1))
      {
        slearn[i].pos = -1;
        return true;
      }
      else
        undo(i + 1);
    }
    int skillLevel = 0;
    for (int j = 1; j <= 25; j++)
      if (skills[j].skill == slearn[i].skill)
        skillLevel++;
    if (skillLevel < slearn[i].skill->lvlMax)
    {
      int reqLevel = slearn[i].skill->lvlStart + slearn[i].skill->lvlSkip * skillLevel;
      for (int j = reqLevel; j <= 25; j++)
      {
        if (skills[j].skill == NULL)
        {
          skills[j].skill = slearn[i].skill;
          skills[j].time = slearn[i].time;
          slearn[i].pos = j;
          break;
        }
      }
      if (slearn[i].pos >= 0)
      {
        for (int j = slearn[i].pos + 1; j <= 25; j++)
          if (skills[j].skill && skills[j].time + 30000 < slearn[i].time)
            return false;
      }
    }
  }
  return true;
}

void W3GHero::compute(uint32 time)
{
  for (int i = 0; i < 5; i++)
    skillLevels[i] = 0;
  currentLevel = 0;
  for (int i = 1; i <= level && levelTime[i] <= time; i++)
  {
    currentLevel++;
    if (skills[i].skill)
      skillLevels[skills[i].skill->slot]++;
  }
}
