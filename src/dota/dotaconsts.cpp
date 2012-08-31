#include "consts.h"

static char lane_names[][256] = {"No lane", "Top", "Mid", "Bot", "AFK?"};
String getLaneName(int lane)
{
  return (lane >= 0 && lane < 4 ? lane_names[lane] : lane_names[4]);
}

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
          uint64 mask = (1ULL << (uint64(k) / 2));
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
String formatMode(uint64 mode)
{
  if (mode == 0)
    return "Normal mode";
  else if (mode == MODE_WTF)
    return "-wtf";
  String result = "-";
  for (int i = 0; i < num_modes / 2; i++)
    if ((mode >> i) & 1)
      result += game_modes[i * 2];
  if (mode & MODE_WTF)
    result += " -wtf";
  return result;
}

int getModeTime(uint64 mode)
{
  int time = 90;
  if (mode & (MODE_RD | MODE_LM | MODE_XL))
    time = 180;
  if (mode & (MODE_MM | MODE_VR | MODE_AP))
    time = 120;
  return (time + 30) * 1000;
}

#include "graphics/image.h"
#include "base/file.h"
#include "base/mpqfile.h"
#include "core/app.h"

class IconKeeper
{
public:
  Image* img[3];
  bool loaded;
  IconKeeper()
  {
    loaded = false;
    img[0] = NULL;
    img[1] = NULL;
    img[2] = NULL;
  }
  ~IconKeeper()
  {
    delete img[0];
    delete img[1];
    delete img[2];
  }
};
static IconKeeper _icons;

void addMapIcons(Image* image, File* desc)
{
  if (!_icons.loaded)
  {
    MPQArchive* mpq = MPQArchive::open(
      String::buildFullName(cfg.warPath, "war3.mpq"), MPQFILE_READ);
    if (mpq)
    {
      _icons.img[0] = new Image(TempFile(mpq->openFile(
        "UI\\MiniMap\\MiniMapIcon\\MinimapIconGold.blp", File::READ)));
      _icons.img[1] = new Image(TempFile(mpq->openFile(
        "UI\\MiniMap\\MiniMapIcon\\MinimapIconNeutralBuilding.blp", File::READ)));
      _icons.img[2] = new Image(TempFile(mpq->openFile(
        "UI\\MiniMap\\MiniMapIcon\\MinimapIconStartLoc.blp", File::READ)));
      _icons.loaded = true;
    }
  }
  desc->seek(4, SEEK_SET);
  uint32 count = desc->read_int32();
  while (count--)
  {
    uint32 type = desc->read_int32();
    uint32 cx = desc->read_int32();
    uint32 cy = desc->read_int32();
    uint32 clr = desc->read_int32();
    if (type >= 0 && type <= 2 && _icons.img[type])
    {
      BLTInfo info(_icons.img[type],
        image->width() * cx / 256 - _icons.img[type]->width() / 2,
        image->height() * cy / 256 - _icons.img[type]->height() / 2);
      if (type == 2)
        info.modulate = clr;
      image->blt(info);
    }
  }
}
