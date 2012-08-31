#include "consts.h"

int convert_race (int race)
{
  if (race == 'hpea' || race == 0x01 || race == 0x41)
    return RACE_HUMAN;
  if (race == 'opeo' || race == 0x02 || race == 0x42)
    return RACE_ORC;
  if (race == 'ewsp' || race == 0x04 || race == 0x44)
    return RACE_NIGHTELF;
  if (race == 'uaco' || race == 0x08 || race == 0x48)
    return RACE_UNDEAD;
  if (race == 0x20 || race == 0x60)
    return RACE_RANDOM;
  return 0;
}
static const char raceImage[][32] = {"Empty", "human", "orc", "nightelf", "undead", "random"};
String getRaceIcon(int race)
{
  if (race < 0 || race > RACE_RANDOM)
    return raceImage[0];
  return raceImage[race];
}

const char actionNames[NUM_ACTIONS][256] = {
  "Right click",
  "Select / deselect",
  "Select group hotkey",
  "Assign group hotkey",
  "Use ability",
  "Basic commands",
  "Select subgroup",
  "Give item / drop item",
  "ESC pressed",
  "Other"
};
String action_name(int id)
{
  if (id < 0 || id >= NUM_ACTIONS)
    return String();
  return actionNames[id];
}
