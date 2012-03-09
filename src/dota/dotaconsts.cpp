#include "consts.h"

static const char game_modes[][32] = {
  "ap", "allpick",
  "ar", "allrandom",
  "lm", "leaguemode",
  "mm", "mirrormatch",
  "tr", "teamrandom",
  "dm", "deathmatch",
  "mr", "moderandom",
  "sp", "shuffleplayers",
  "aa", "allagility",
  "ai", "allintelligence",

  "as", "allstrength",
  "id", "itemdrop",
  "np", "nopowerups",
  "sc", "supercreeps",
  "em", "easymode",
  "du", "duplicatemode",
  "sh", "samehero",
  "vr", "voterandom",
  "rv", "reverse",
  "rd", "randomdraft",

  "om", "onlymid",
  "xl", "extendedleague",
  "nm", "nomid",
  "nt", "notop",
  "nb", "nobot",
  "ns", "noswap",
  "nr", "norepick",
  "ts", "terrainsnow",
  "sd", "singledraft",
  "cd", "captainsdraft",

  "pm", "poolingmode",
  "oi", "observerinfo",
  "mi", "miniheroes",
  "cm", "captainsmode",
  "fr", "fastrespawn",
  "mo", "meleeonly",
  "ro", "rangeonly",
  "er", "experimentalrunes",
  "rs", "randomside",
  "so", "switchon"
};
static const int num_modes = sizeof game_modes / sizeof game_modes[0];
uint64 parseMode(String mode, String* parsed)
{
  uint64 result = 0;
  if (parsed)
    *parsed = "-";
  for (int i = 0; mode[i]; i++)
  {
    for (int j = i; mode[j]; j++)
    {
      int len = j - i + 1;
      for (int k = 0; k < num_modes; k++)
      {
        if (!strncmp(game_modes[k], mode.c_str() + i, len) && game_modes[k][len] == 0)
        {
          uint64 mask = (1LL << (uint64(k) / 2));
          if (!(result & mask) && parsed)
            *parsed += game_modes[k & (~1)];
          result |= mask;
          mode.cut(i, len);
          i = 0;
          j = -1;
          break;
        }
      }
    }
  }
  return result;
}

int getModeTime (uint64 mode)
{
  int time = 90;
  if (mode & (MODE_RD | MODE_LM | MODE_XL))
    time = 180;
  if (mode & (MODE_MM | MODE_VR | MODE_AP))
    time = 120;
  return (time + 30) * 1000;
}
