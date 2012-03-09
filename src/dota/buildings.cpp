#include "buildings.h"

static DotaBuilding _buildings[] = {
  {-6112, 1568, "AncientProtector"}, // level 1
  {-6112, -1248, "AncientProtector"}, // level 2
  {-6368, -4256, "AncientProtector"}, // level 3
  {-1504, -1824, "AncientProtector"}, // level 1
  {-3488, -3296, "AncientProtector"}, // level 2
  {-4448, -4960, "AncientProtector"}, // level 3
  {4960, -6752, "AncientProtector"}, // level 1
  {-544, -6688, "AncientProtector"}, // level 2
  {-3744, -6816, "AncientProtector"}, // level 3
  {-6080, -4480, "AncientOfWar"},
  {-4416, -5312, "AncientOfWar"},
  {-4032, -7040, "AncientOfWar"},
  {-6656, -4480, "AncientOfLore"},
  {-4864, -4992, "AncientOfLore"},
  {-4032, -6528, "AncientOfLore"},
  {-5600, -5728, "AncientProtector"}, // level 4
  {-5280, -6112, "AncientProtector"}, // level 4
  {-5632, -6144, "WorldTree"},

  {-4704, 5920, "SpiritTower"}, // level 1
  {32, 5920, "SpiritTower"}, // level 2
  {2976, 5792, "SpiritTower"}, // level 3
  {1056, -96, "SpiritTower"}, // level 1
  {2528, 1824, "SpiritTower"}, // level 2
  {3936, 3488, "SpiritTower"}, // level 3
  {6048, -2080, "SpiritTower"}, // level 1
  {6304, -96, "SpiritTower"}, // level 2
  {6368, 2528, "SpiritTower"}, // level 3
  {3392, 5504, "Crypt"},
  {4352, 3584, "Crypt"},
  {6656, 2880, "Crypt"},
  {3392, 6080, "TempleOfTheDamned"},
  {3904, 3904, "TempleOfTheDamned"},
  {6080, 2944, "TempleOfTheDamned"},
  {4832, 4832, "SpiritTower"}, // level 4
  {5152, 4512, "SpiritTower"}, // level 4
  {5184, 4864, "FrozenThrone"}
};
DotaBuilding* getBuildings ()
{
  return _buildings;
}
int getBuildingId (int side, int type, int lane, int level)
{
  if (side == 0)
  {
    if (type == BUILDING_TOWER)
    {
      if (level == 4)
        return BUILDING_SENTINEL_TOWER1;
      else
      {
        if (lane == 0)
        {
          if (level == 0)
            return BUILDING_SENTINEL_TOWER_TOP1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_TOP2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_TOP3;
        }
        else if (lane == 1)
        {
          if (level == 1)
            return BUILDING_SENTINEL_TOWER_MID1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_MID2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_MID3;
        }
        else if (lane == 2)
        {
          if (level == 1)
            return BUILDING_SENTINEL_TOWER_BOT1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_BOT2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_BOT3;
        }
      }
    }
    else if (type == BUILDING_MELEE)
    {
      if (lane == 0)
        return BUILDING_SENTINEL_MELEE_TOP;
      else if (lane == 1)
        return BUILDING_SENTINEL_MELEE_MID;
      else if (lane == 2)
        return BUILDING_SENTINEL_MELEE_BOT;
    }
    else if (type == BUILDING_RANGED)
    {
      if (lane == 0)
        return BUILDING_SENTINEL_RANGED_TOP;
      else if (lane == 1)
        return BUILDING_SENTINEL_RANGED_MID;
      else if (lane == 2)
        return BUILDING_SENTINEL_RANGED_BOT;
    }
    else if (type == BUILDING_THRONE)
      return BUILDING_WORLD_TREE;
  }
  else
  {
    if (type == BUILDING_TOWER)
    {
      if (level == 4)
        return BUILDING_SCOURGE_TOWER1;
      else
      {
        if (lane == 0)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_TOP1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_TOP2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_TOP3;
        }
        else if (lane == 1)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_MID1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_MID2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_MID3;
        }
        else if (lane == 2)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_BOT1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_BOT2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_BOT3;
        }
      }
    }
    else if (type == BUILDING_MELEE)
    {
      if (lane == 0)
        return BUILDING_SCOURGE_MELEE_TOP;
      else if (lane == 1)
        return BUILDING_SCOURGE_MELEE_MID;
      else if (lane == 2)
        return BUILDING_SCOURGE_MELEE_BOT;
    }
    else if (type == BUILDING_RANGED)
    {
      if (lane == 0)
        return BUILDING_SCOURGE_RANGED_TOP;
      else if (lane == 1)
        return BUILDING_SCOURGE_RANGED_MID;
      else if (lane == 2)
        return BUILDING_SCOURGE_RANGED_BOT;
    }
    else if (type == BUILDING_THRONE)
      return BUILDING_FROZEN_THRONE;
  }
  return -1;
}
