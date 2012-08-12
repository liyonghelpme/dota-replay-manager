#include "stdafx.h"
#include "replay.h"
#include "gamecache.h"
#include "rmpq.h"

#pragma comment (lib, "zlib\\zlib.lib")
#include "zlib\zlib.h"

//FILE* __f;

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
extern bool relTime;
char* format_time (W3GReplay* w3g, unsigned long time, int flags)
{
  long atime = (long) time;
  if (w3g && relTime)
    atime -= (long) w3g->game.startTime;
  return format_time (atime, flags);
}

#include <math.h>

extern int deathTreshold;
extern int repDelay;
extern int repDelayItem;
extern bool chatAssists;

static int _version;

void* gzalloc (void* param, unsigned int items, unsigned int size);
void gzfree (void* param, void* ptr);

bool gzinflate (char* old, char* buf, int csize, int usize, gzmemory* mem)
{
  if (mem)
    mem->reset ();
  z_stream zs;
  memset (&zs, 0, sizeof zs);

  zs.next_out = (unsigned char*) buf;
  zs.avail_out = usize;
  zs.zalloc = gzalloc;
  zs.zfree = gzfree;
  zs.opaque = mem;

  if (inflateInit2 (&zs, -15) != Z_OK)
    return false;
  zs.next_in = (Bytef*) old;
  zs.avail_in = csize;
  int err = inflate (&zs, Z_SYNC_FLUSH);
  inflateEnd (&zs);
//  if (err != Z_STREAM_END)
//    return true;
  return zs.avail_out == 0;
}
bool gzinflate2 (char* old, char* buf, int csize, int usize, gzmemory* mem = NULL)
{
  if (mem)
    mem->reset ();
  z_stream z;
  memset (&z, 0, sizeof z);
  z.next_in = (Bytef*) old;
  z.avail_in = (uInt) csize;
  z.next_out = (Bytef*) buf;
  z.avail_out = (uInt) usize;
  z.zalloc = gzalloc;
  z.zfree = gzfree;
  z.opaque = mem;

  if (inflateInit (&z) != Z_OK)
    return false;
  inflate (&z, Z_FINISH);
  inflateEnd (&z);
  return usize == z.total_out;
}

#define POLYNOMIAL 0xEDB88320
static unsigned long crctable[256];
void init_crc32 ()
{
  for (int i = 0; i < 256; i++)
  {
    crctable[i] = i;
    for (int j = 0; j < 8; j++)
      crctable[i] = (crctable[i] >> 1) ^ (crctable[i] & 1 ? POLYNOMIAL : 0);
  }
}
unsigned long crc32 (unsigned char const* data, unsigned long len)
{
  unsigned long crc = 0xFFFFFFFF;
  while (len--)
    crc = (crc >> 8) ^ crctable[(crc & 0xFF) ^ *data++];
  return ~crc;
}
unsigned short xor16 (unsigned long x)
{
  return (unsigned short) (x ^ (x >> 16));
}

void un_unicode (char* str, wchar_t* wstr = NULL)
{
  int count = MultiByteToWideChar (CP_UTF8, 0, (LPCSTR) str, -1, NULL, 0);
  wchar_t* buf = (wstr ? wstr : new wchar_t[count + 5]);
  MultiByteToWideChar (CP_UTF8, 0, (LPCSTR) str, -1, buf, count + 5);

  count = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, (LPWSTR) buf, -1, NULL, 0, NULL, NULL);
  if (count == 0)
    return;
  char* msg = new char[count + 5];
  WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, (LPWSTR) buf, -1, msg, count + 5, NULL, NULL);
  int sh = 0;
  for (int i = 0; msg[i]; i++)
    if (msg[i] != '\r')
      msg[sh++] = msg[i];
  msg[sh] = 0;
  strcpy (str, msg);
  if (wstr == NULL)
    delete[] buf;
  delete[] msg;
}

/////////////
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
/////////////

bool W3GHeader::read_data (FILE* f)
{
  memset (this, 0, sizeof W3GHeader);
  if (fread (this, 48, 1, f) != 1)
    return false;
  if (memcmp (intro, "Warcraft III recorded game", 26))
    return false;
  if (header_v == 0)
  {
    if (fread (&minor_v, 2, 1, f) != 1) return false;
    if (fread (&major_v, 2, 1, f) != 1) return false;
    if (fread (&build_v, 2, 1, f) != 1) return false;
    if (fread (&flags, 2, 1, f) != 1) return false;
    if (fread (&length, 4, 1, f) != 1) return false;
    if (fread (&checksum, 4, 1, f) != 1) return false;
    ident = 'WAR3';
  }
  else
  {
    if (fread (&ident, 4, 1, f) != 1) return false;
    if (fread (&major_v, 4, 1, f) != 1) return false;
    if (fread (&build_v, 2, 1, f) != 1) return false;
    if (fread (&flags, 2, 1, f) != 1) return false;
    if (fread (&length, 4, 1, f) != 1) return false;
    if (fread (&checksum, 4, 1, f) != 1) return false;
    minor_v = 0;
  }
  return true;
}
void W3GHeader::write_data (FILE* f)
{
  fwrite (this, 48, 1, f);
  if (header_v == 0)
  {
    fwrite (&minor_v, 2, 1, f);
    fwrite (&major_v, 2, 1, f);
    fwrite (&build_v, 2, 1, f);
    fwrite (&flags, 2, 1, f);
    fwrite (&length, 4, 1, f);
    fwrite (&checksum, 4, 1, f);
  }
  else
  {
    fwrite (&ident, 4, 1, f);
    fwrite (&major_v, 4, 1, f);
    fwrite (&build_v, 2, 1, f);
    fwrite (&flags, 2, 1, f);
    fwrite (&length, 4, 1, f);
    fwrite (&checksum, 4, 1, f);
  }
  int pos = ftell (f);
  while (pos < header_size)
    fputc (0, f), pos++;
}
void W3GHeader::stream_data (unsigned char* x)
{
  memcpy (x, this, 48);
  x += 48;
  int pos = 48;
  if (header_v == 0)
  {
    memcpy (x, &minor_v, 2); x += 2;
    memcpy (x, &major_v, 2); x += 2;
    memcpy (x, &build_v, 2); x += 2;
    memcpy (x, &flags, 2); x += 2;
    memcpy (x, &length, 4); x += 4;
    memcpy (x, &checksum, 4); x += 4;
    pos += 16;
  }
  else
  {
    memcpy (x, &ident, 4); x += 4;
    memcpy (x, &major_v, 4); x += 4;
    memcpy (x, &build_v, 2); x += 2;
    memcpy (x, &flags, 2); x += 2;
    memcpy (x, &length, 4); x += 4;
    memcpy (x, &checksum, 4); x += 4;
    pos += 20;
  }
  while (pos < header_size)
    *x++ = 0, pos++;
}

void W3GHero::pushAbility (int id, unsigned long time, int slot)
{
  if (nLearned > 50)
    return;
  //if (lCount <= 1 || nLearned < lCount)
  //{
  learned[nLearned].id = id;
  learned[nLearned].time = time;
  learned[nLearned].slot = slot;
  nLearned++;
  //}
}
int getMinLevel (int cnt, int slot, int id)
{
  id = getAbility (id)->ids[0];
  if (id == 'A0VB' || id == 'A0VA' || id == 'A0V9')
  {
    if (cnt >= 7)
      return 32;
    return 2 * cnt;
  }
  else if (id == 'A0VF')
  {
    if (cnt >= 4)
      return 32;
    if (_version >= 54)
      return 1 + 5 * cnt;
    else
      return 4 + 5 * cnt;
  }
  else if (slot == 3)
  {
    if (cnt >= 3)
      return 32;
    return 5 + 5 * cnt;
  }
  else if (slot == 4)
  {
    if (cnt >= 10)
      return 32;
    return 2 * cnt;
  }
  else
  {
    if (cnt >= 4)
      return 32;
    return 2 * cnt;
  }
}
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

static Array<ASItem> __del;
void W3GInventory::listItems (unsigned long time)
{
  bi.clear ();
  for (int i = 0; i < num_items && itemt[i] <= time; i++)
    bi.add (ASItem (items[i], itemt[i]));
}
void W3GInventory::sortItems ()
{
  for (int i = 0; i < bi.getSize (); i++)
  {
    for (int j = 1; j < bi.getSize (); j++)
    {
      if (bi[j - 1].time > bi[j].time)
      {
        ASItem tmp = bi[j - 1];
        bi[j - 1] = bi[j];
        bi[j] = tmp;
      }
    }
  }
}
void W3GInventory::getAllItems (unsigned long time, DotaData const& dota)
{
  getASItems (time, dota);
  for (int i = 0; i < __del.getSize (); i++)
  {
    __del[i].gone = true;
    bi.add (__del[i]);
  }
}

bool invGotRecipe (Array<ASItem>& m, DotaRecipe* recipe)
{
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = 0; j < m.getSize () && cnt < recipe->srccount[i]; j++)
      {
        if (!m[j].pregone && m[j].id == recipe->srcid[i])
        {
          m[j].pregone = true;
          cnt++;
        }
      }
      if (cnt < recipe->srccount[i])
        return false;
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
        if (!invGotRecipe (m, rcp))
          return false;
    }
  }
  return true;
}
unsigned long invGetTime (Array<ASItem>& m, DotaRecipe* recipe)
{
  unsigned long time = 0;
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = 0; j < m.getSize () && cnt < recipe->srccount[i]; j++)
      {
        if (!m[j].tpregone && !m[j].pregone && m[j].id == recipe->srcid[i])
        {
          m[j].tpregone = true;
          if (m[j].time >= time)
            time = m[j].time + 1;
          cnt++;
        }
      }
      if (cnt < recipe->srccount[i])
        return false;
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
      {
        unsigned long tm = invGetTime (m, rcp);
        if (tm > time)
          time = tm;
      }
    }
  }
  return time;
}
void invTRefresh (Array<ASItem>& m)
{
  for (int i = 0; i < m.getSize (); i++)
    m[i].tpregone = false;
}
int invRemRecipe (Array<ASItem>& m, DotaRecipe* recipe)
{
  unsigned long time = invGetTime (m, recipe);
  invTRefresh (m);
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = m.getSize () - 1; j >= 0 && cnt < recipe->srccount[i]; j--)
      {
        if (m[j].id == recipe->srcid[i] && m[j].time <= time)
        {
          m[j].gone = true;
          m[j].pregone = false;
          __del.add (m[j]);
          m.del (j);
          cnt++;
        }
      }
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
      {
        int pos = invRemRecipe (m, rcp);
        m[pos].gone = true;
        m[pos].pregone = false;
        __del.add (m[pos]);
        m.del (pos);
      }
    }
  }
  return m.add (ASItem (recipe->result, time));
}
void invRefresh (Array<ASItem>& m)
{
  for (int i = 0; i < m.getSize (); i++)
    m[i].pregone = false;
}

void W3GInventory::getASItems (unsigned long time, DotaData const& dota)
{
  listItems (time);
  __del.clear ();

  for (int i = 0; i < getNumRecipes (); i++)
  {
    DotaRecipe* recipe = getRecipe (i);
    while (invGotRecipe (bi, recipe))
    {
      invRefresh (bi);
      invRemRecipe (bi, recipe);
      sortItems ();
      invRefresh (bi);
    }
    invRefresh (bi);
  }
}

bool W3GHero::compute (unsigned long time, DotaData const& dota)
{
  if (dota.major != 6)
    return false;

  for (int i = 0; i < 5; i++)
    levels[i] = 0;
  cur_lvl = 0;
  for (int i = 0; i < level && atime[i] <= time; i++)
  {
    cur_lvl++;
    if (abilities[i] != 0)
      levels[getAbility (abilities[i])->slot]++;
  }
  return true;
}

bool W3GInventory::compute (unsigned long time, DotaData const& dota)
{
  for (int i = 0; i < 6; i++)
    inv[i] = 0;
  if (dota.major != 6)
    return false;

  getASItems (time, dota);

  for (int s = 0; s < 6 && bi.getSize () != 0; s++)
  {
    int best = 0;
    for (int i = 1; i < bi.getSize (); i++)
      if (getItem (bi[i].id)->cost > getItem (bi[best].id)->cost)
        best = i;
    inv[s] = bi[best].id;
    bi.del (best);
  }
  return true;
}

void W3GReplay::readTime (char const* filename)
{
  saved_time = "";
  saved_date = "";
  fixed_time = "";

  CFileStatus status;
  if (!CFile::GetStatus (filename, status))
    return;

  timestamp = status.m_mtime.GetTime ();
  saved_time = status.m_mtime.Format ("%d/%m/%Y %H:%M");
  saved_date = status.m_mtime.Format ("%d-%m-%Y");
  fixed_time = status.m_mtime.Format ("%H_%M");
}

void W3GReplay::readTime (SYSTEMTIME* sysTime)
{
  saved_time = "";
  saved_date = "";
  fixed_time = "";

  CTime mTime (*sysTime);

  saved_time = mTime.Format ("%d/%m/%Y %H:%M");
  saved_date = mTime.Format ("%d-%m-%Y");
  fixed_time = mTime.Format ("%H_%M");
}
void W3GReplay::alloc_temp (int size)
{
  if (size > tmpSize)
  {
    delete[] temp;
    temp = new char[size];
    tmpSize = size;
  }
}
void parseTXT (MPQARCHIVE mpq, char const* path);
void parseSLK (MPQARCHIVE mpq, char const* path);
bool W3GReplay::load (FILE* file, bool quick)
{
  if (!hdr.read_data (file))
    return false;
  fseek (file, hdr.header_size, SEEK_SET);
  size = 0;
  pos = 0;
  gotitems = false;
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader block;
    if (fread (&block, sizeof block, 1, file) != 1)
      return false;
    alloc_temp (block.c_size);
    if (fread (temp, block.c_size, 1, file) != 1)
      return false;
    addbytes (block.u_size);
    if (!gzinflate2 (temp, data + size, block.c_size, block.u_size, &mem))
      return false;
    size += block.u_size;
  }
  pos += 4;
  if (!loadPlayer ())
    return false;
  if (!loadGame ())
    return false;
  dota.isDota = preAnalyze ();
  if (dota.isDota)
  {
    int version = makeVersion (dota.major, dota.minor, dota.build);
    _version = dota.minor;
    if (!loadDataEx (version, game.map))
      return false;
  }
  //if (!quick)
  //{
  //  MPQARCHIVE mpq = MPQOpen (mprintf ("%s%s", warPath, game.map), MPQFILE_READ);
  //  if (mpq)
  //  {
  //    MPQAddArchive (loader, mpq);
  //    parseTXT (loader, "Units\\CampaignUnitStrings.txt");
  //    parseTXT (loader, "Units\\HumanUnitStrings.txt");
  //    parseTXT (loader, "Units\\OrcUnitStrings.txt");
  //    parseTXT (loader, "Units\\NightElfUnitStrings.txt");
  //    parseTXT (loader, "Units\\UndeadUnitStrings.txt");
  //    parseTXT (loader, "Units\\NeutralUnitStrings.txt");
  //    parseTXT (loader, "Units\\CampaignAbilityStrings.txt");
  //    parseTXT (loader, "Units\\HumanAbilityStrings.txt");
  //    parseTXT (loader, "Units\\OrcAbilityStrings.txt");
  //    parseTXT (loader, "Units\\NightElfAbilityStrings.txt");
  //    parseTXT (loader, "Units\\UndeadAbilityStrings.txt");
  //    parseTXT (loader, "Units\\NeutralAbilityStrings.txt");
  //    parseSLK (loader, "Units\\UnitAbilities.slk");
  //    MPQRemoveArchive (loader, mpq);
  //    MPQClose (mpq);
  //  }
  //}
  blockpos = pos;
//  __f = fopen ("dump2.txt", "wt");
  if (!parseBlocks (quick))
    return false;
//  fclose (__f);
  analyze ();
  return true;
}

void W3GReplay::add_chat (W3GMessage& msg)
{
  if (msg.text[0] && msg.utext[0] == 0)
  {
    for (int i = 0; msg.utext[i] = msg.text[i]; i++)
      ;
  }
  chat.add (msg);
}

const char game_modes[][32] = {"ap", "allpick",
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
const int numModes = sizeof game_modes / sizeof game_modes[0];
int getModeTime (__int64 mode)
{
  int time = 90;
  if (mode & (MODE_RD | MODE_LM | MODE_XL))
    time = 180;
  if (mode & (MODE_MM | MODE_VR | MODE_AP))
    time = 120;
  return (time + 30) * 1000;
}

int strxcmp (char const* a, char const* b, int l)
{
  for (int i = 0; i < l; i++)
  {
    if (a[i] != b[i])
      return 1;
  }
  return (a[l] != 0);
}

void strcut (char* s, int a, int b)
{
  if (b)
    memmove (s + a, s + a + b, (int) strlen (s + a + b) + 1);
}
void W3GReplay::parseMode (char* mstr)
{
  game.gmode &= MODE_WTF;
  game.game_mode[0] = '-';
  game.game_mode[1] = 0;
  for (int i = 0; mstr[i]; i++)
  {
    for (int j = i; mstr[j]; j++)
    {
      for (int k = 0; k < numModes; k++)
      {
        if (!strxcmp (game_modes[k], mstr + i, j - i + 1))
        {
          __int64 mask = (1LL << (__int64 (k) / 2));
          if (!(game.gmode & mask))
          {
            strcat (game.game_mode, game_modes[k & (~1)]);
            game.gmode |= mask;
          }
          strcut (mstr, i, j - i + 1);
          i = 0;
          j = i - 1;
        }
      }
    }
  }
}
void W3GReplay::getMode (W3GMessage const& msg)
{
  int leader = pindex[0];
  for (int i = 1; i < numPlayers; i++)
    if (players[pindex[i]].slot.color < players[leader].slot.color)
      leader = pindex[i];
  if (msg.id != leader)
    return;
  if (!strcmp (msg.text, "-wtf"))
  {
    game.gmode |= MODE_WTF;
    return;
  }
  if (game.gmode & (~MODE_WTF))
    return;
  int len = 1;
  for (int i = 0;; i++)
  {
    if (msg.text[i] == 0) return;
    if (msg.text[i] == '-')
      break;
  }
  char mstr[256];
  int mlen = 0;
  for (int i = 0; msg.text[i]; i++)
  {
    if (msg.text[i] >= 'a' && msg.text[i] <= 'z')
      mstr[mlen++] = msg.text[i];
    else if (msg.text[i] >= 'A' && msg.text[i] <= 'Z')
      mstr[mlen++] = msg.text[i] + 'a' - 'A';
  }
  mstr[mlen] = 0;
  parseMode (mstr);
}

bool W3GReplay::loadPlayer ()
{
  unsigned char rec = data[pos++];
  unsigned char id = data[pos++];
  players[id].player_id = id;
  players[id].initiator = (rec == 0);
  int nl = 0;
  while (data[pos] != 0)
    players[id].name[nl++] = data[pos++];
  players[id].name[nl] = 0;
  pos++;
  if (nl == 0)
    sprintf (players[id].name, "Player %d", id);
  strcpy (players[id].orgname, players[id].name);
  un_unicode (players[id].name, players[id].uname);

  if (data[pos] == 1) // custom game
    pos += 2;
  else if (data[pos] == 8) // ladder game
  {
    pos++;
    players[id].exe_runtime = * (long*) (data + pos);
//    players[id].race = convert_race (* (long*) (data + pos + 4));
    pos += 8;
  }
  players[id].actions = 0;
  players[id].time = END_TIME;
  players[id].index = numPlayers;
  pindex[numPlayers++] = id;

  return true;
}
uint32 mpq_hash_map (char const* path);
bool W3GReplay::loadGame ()
{
  int ln = 0;
  while (data[pos] != 0)
    game.name[ln++] = data[pos++];
  game.name[ln] = 0;
  un_unicode (game.name, game.uname);
  pos += 2;
  char buf[2048];
  int blen = 0;
  int mask = 0;
  int i;
  for (i = 0; data[pos + i] != 0; i++)
  {
    if (i % 8 == 0)
      mask = data[pos + i];
    else
      buf[blen++] = data[pos + i] - ((mask & (1 << (i % 8))) ? 0 : 1);
  }
  buf[blen] = 0;
  pos += i + 1;
  game.speed = buf[0];
  if (buf[1] & 1)
    game.visibility = VISIBILITY_HIDE;
  else if (buf[1] & 2)
    game.visibility = VISIBILITY_EXPLORED;
  else if (buf[1] & 4)
    game.visibility = VISIBILITY_VISIBLE;
  else if (buf[1] & 8)
    game.visibility = VISIBILITY_DEFAULT;
  game.observers = (buf[1] >> 4) & 3;
  game.teams_together = (buf[1] & 64) != NULL;
  game.lock_teams = buf[2] != NULL;
  game.shared_control = (buf[3] & 1) != NULL;
  game.random_hero = (buf[3] & 2) != NULL;
  game.random_races = (buf[3] & 4) != NULL;
  if (buf[3] & 64)
    game.observers = OBSERVERS_REFEREES;
  uint32 chksum = * (uint32*) (buf + 9);
  int bpos = 13;
  ln = 0;
  while (buf[bpos] != 0)
    game.map[ln++] = buf[bpos++];
  game.map[ln] = 0;
  un_unicode (game.map, game.umap);
  bpos++;
  ln = 0;
  while (buf[bpos] != 0)
    game.creator[ln++] = buf[bpos++];
  game.creator[ln] = 0;
  un_unicode (game.creator, game.ucreator);

  //uint32 rchksum = mpq_hash_map (game.map);

  game.slots = * (long*) (data + pos);
  pos += 4;
  game.game_type = data[pos];
  game.game_private = (data[pos + 1] != 0);
  pos += 8;
  while (data[pos] == 0x16)
  {
    loadPlayer ();
    pos += 4;
  }

  game.record_id = data[pos];
  game.record_length = * (short*) (data + pos + 1);
  game.slot_records = data[pos + 3];
  pos += 4;

  for (i = 0; i < game.slot_records; i++)
  {
    unsigned char id = data[pos];
    pos += 2;
    W3GSlot slot;
    slot.slot_status = data[pos++];
    slot.computer = data[pos++];
    slot.team = data[pos++];
    slot.color = data[pos++];
    slot.race = convert_race (data[pos++]);
    slot.org_color = slot.color;
    if (hdr.major_v >= 3)
      slot.ai_strength = data[pos++];
    if (hdr.major_v >= 7)
      slot.handicap = data[pos++];
    if (slot.computer)
      id = 128 + numPlayers;
    if (slot.slot_status == 2)
      players[id].slot = slot;
    if (slot.computer)
    {
      players[id].player_id = id;
      switch (slot.ai_strength)
      {
      case AI_EASY:
        strcpy (players[id].name, "Computer (Easy)");
        strcpy (players[id].orgname, "Computer (Easy)");
        wcscpy (players[id].uname, L"Computer (Easy)");
        break;
      case AI_NORMAL:
        strcpy (players[id].name, "Computer (Normal)");
        strcpy (players[id].orgname, "Computer (Normal)");
        wcscpy (players[id].uname, L"Computer (Normal)");
        break;
      case AI_INSANE:
        strcpy (players[id].name, "Computer (Insane)");
        strcpy (players[id].orgname, "Computer (Insane)");
        wcscpy (players[id].uname, L"Computer (Insane)");
        break;
      }
      pindex[numPlayers++] = id;
    }
//    if (players[id].race == 0)
//      players[id].race = slot.race;
  }

  game.random_seed = * (long*) (data + pos);
  game.select_mode = data[pos + 4];
  game.start_spots = data[pos + 5];
  pos += 6;

  for (i = 0; i < numPlayers; i++)
  {
    if (players[pindex[i]].slot.color > 11 ||
      players[pindex[i]].slot.slot_status == 0)
      game.hasObservers = true;
    for (int j = 0; j < 16; j++)
      players[pindex[i]].share[j] = game.shared_control;
  }

  return true;
}

void W3GReplay::addbytes (int add)
{
  if (data == NULL)
    mxsize = 1;
  add += size;
  if (mxsize < add || data == NULL)
  {
    while (mxsize < add)
      mxsize *= 2;
    char* temp = new char[mxsize];
    if (data)
      memcpy (temp, data, size);
    delete[] data;
    data = temp;
  }
}

char const* W3GReplay::fmt_player (int p)
{
  if (dota.major >= 6 && dota.minor >= 54)
    return mprintf ("%s (%d-%d)", players[p].name, players[p].stats[STAT_KILLS],
    players[p].stats[STAT_DEATHS]);
  else
    return players[p].name;
}
char const* W3GReplay::fmt_player (W3GPlayer* p)
{
  if (dota.isDota && dota.major >= 6 && dota.minor >= 54)
    return mprintf ("%s (%d-%d)", p->name, p->stats[STAT_KILLS],
    p->stats[STAT_DEATHS]);
  else
    return p->name;
}
bool W3GReplay::parseBlocks (bool quick)
{
  int prev = 0;
  GameState state;
  memset (&state, 0, sizeof state);
  state.quick = quick;
  for (int p = 0; p < numPlayers; p++)
    state.playerid[players[pindex[p]].slot.org_color] = pindex[p];
  int leave_unknown = 0;
  int chatid = 0;
  int prevPos = 0;
  while (pos < size)
  {
    if (pos / 100000 != prevPos / 100000)
      int asdf = 0;
    prevPos = pos;
    int block_id = data[pos++];
    if (pos > 12500)
      int asdf = 0;
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        short length = * (short*) (data + pos);
        short time_inc = * (short*) (data + pos + 2);
        pos += 4;
        if (!state.pause)
          state.time += time_inc;
        if (state.time > 104000)
          int asdf = 0;
        if (length > 2 && !parseActions (length - 2, state, quick))
          return false;
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (hdr.major_v > 2)
      {
        if (quick)
        {
          short length = * (short*) (data + pos + 1);
          pos += length + 3;
        }
        else
        {
          W3GMessage msg;
          msg.id = data[pos];
          short length = * (short*) (data + pos + 1);
          unsigned char flags = data[pos + 3];
          if (flags == 0x20)
          {
            msg.mode = * (unsigned long*) (data + pos + 4);
            if (msg.mode > CHAT_PRIVATE)
              msg.mode = CHAT_PRIVATE;
            strncpy (msg.text, data + pos + 8, length - 6);
            msg.text[length - 6] = 0;
            msg.index = chatid++;

            un_unicode (msg.text, msg.utext);
            msg.time = state.time;
            chat.add (msg);

            if (msg.mode == CHAT_ALL && strlen (msg.text) <= 4)
            {
              if (is_substr (msg.text, "ff")) players[msg.id].saidFF = true;
              if (is_substr (msg.text, "gg")) players[msg.id].saidGG = true;
            }
          }
          pos += length + 3;
        }
        break;
      }
    // unknown (Random number/seed for next frame)
    case 0x22:
      pos += data[pos] + 1;
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      pos += 4;
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      pos += 10;
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      pos += 8;
      break;
    // LeaveGame
    case 0x17:
      {
        leaves++;
        unsigned char id = data[pos + 4];
        players[id].time = state.time;
        int reason = players[id].leave_reason = * (long*) (data + pos);
        int result = players[id].leave_result = * (long*) (data + pos + 5);
        players[id].left = true;
        long unknown = * (long*) (data + pos + 9);
        pos += 13;

        if (leave_unknown)
          leave_unknown = unknown - leave_unknown;
        if (leaves == numPlayers)
          game.saver_id = id;
        else if (!quick)
          add_syschat (CHAT_NOTIFY_LEAVER, id, state.time, "@%d@%s has left the game.", id, fmt_player (id));
        if (reason == 0x01)
        {
          switch (result)
          {
          case 0x08: game.llost[players[id].slot.team] = true; break;
          case 0x09: game.lwinner = players[id].slot.team; break;
          }
        }
        else if (reason == 0x0C && game.saver_id)
        {
          switch (result)
          {
          case 0x07:
            if (leave_unknown > 0 && state.continue_game)
              game.lwinner = players[game.saver_id].slot.team;
            else
              game.llost[players[game.saver_id].slot.team] = true;
            break;
          case 0x08: game.llost[players[game.saver_id].slot.team] = true; break;
          case 0x09: game.lwinner = players[game.saver_id].slot.team; break;
          case 0x0B:
            if (leave_unknown > 0)
              game.lwinner = game.lwinner = players[game.saver_id].slot.team;
            break;
          }
        }
        else if (reason == 0x0C)
        {
          switch (result)
          {
          case 0x07: game.llost[15] = true; break;
          case 0x08: game.lwinner = players[id].slot.team; break;
          case 0x09: game.lwinner = 15;
          }
        }
        leave_unknown = unknown;
      }
      break;
    case 0:
      pos = size;
      break;
    }
    prev = block_id;
  }
  finishEndgame (state);
  time = state.time;
  return true;
}

char const* format_coord (char const* data)
{
  if (* (int*) data == 0xFFFFFFFF)
    return mprintf ("0xFFFFFFFF");
  else
    return mprintf ("%f", * (float*) data);
}

extern bool ignoreBasic;

bool W3GReplay::parseActions (int len, GameState& state, bool quick)
{
  int end = pos + len;
  CString notif;
  while (pos < end)
  {
    unsigned char id = data[pos++];
    short length = * (short*) (data + pos);
    pos += 2;
    int next = pos + length;
    bool was_deselect = false;
    bool was_subgroup = false;

    int prev = 0;
    while (pos < next)
    {
      unsigned char action = data[pos++];
      switch (action)
      {
      // Pause game
      case 0x01:
        {
          state.pause = true;
          if (!quick)
          {
            add_syschat (CHAT_NOTIFY_PAUSE, id, state.time, "%s has paused the game.", players[id].name);
            if (log)
              fprintf (log, "%s Player: %-15s 0x01: Pause game\n", format_time (state.time), players[id].orgname);
          }
        }
        break;
      // Resume game
      case 0x02:
        {
          state.pause = false;
          if (!quick)
          {
            add_syschat (CHAT_NOTIFY_PAUSE, id, state.time, "%s has resumed the game.", players[id].name);
            if (log)
              fprintf (log, "%s Player: %-15s 0x02: Resume game\n", format_time (state.time), players[id].orgname);
          }
        }
        break;
      // Set game speed in single player game (options menu)
      case 0x03:
        if (log && !quick)
          fprintf (log, "%s Player: %-15s 0x03: Set game speed to %d\n", format_time (state.time), players[id].orgname,
            int (data[pos]));
        pos++;
        break;
      // Increase game speed in single player game (Num+)
      case 0x04:
        if (log && !quick)
          fprintf (log, "%s Player: %-15s 0x04: Increase game speed\n", format_time (state.time), players[id].orgname);
        break;
      // Decrease game speed in single player game (Num-)
      case 0x05:
        if (log && !quick)
          fprintf (log, "%s Player: %-15s 0x05: Decrease game speed\n", format_time (state.time), players[id].orgname);
        break;
      // Save game
      case 0x06:
        {
          if (quick)
          {
            while (data[pos++])
              ;
          }
          else
          {
            notif = "";
            while (data[pos] != 0)
            {
              notif += data[pos];
              pos++;
            }
            pos++;
            add_syschat (CHAT_NOTIFY_PAUSE, id, state.time, "%s has saved the game to %s.", players[id].name, (char const*) notif);
            if (log)
              fprintf (log, "%s Player: %-15s 0x06: Save game to %s\n", format_time (state.time), players[id].name,
                (char const*) notif);
          }
        }
        break;
      // Save game finished
      case 0x07:
        if (log && !quick)
          fprintf (log, "%s Player: %-15s 0x07: Save game finished (unknown = 0x%08X)\n",
            format_time (state.time), players[id].orgname, * (int*) (data + pos));
        pos += 4;
        break;
      // Unit/building ability (no additional parameters)
      case 0x10:
        {
          players[id].actions++;
          unsigned short flags;
          if (hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (data + pos);
            pos += 2;
          }
          else
            flags = data[pos++];
          unsigned long itemid = * (unsigned long*) (data + pos);
          pos += 4;
          if (hdr.major_v >= 7)
          {
            pos += 8;
            if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
              fprintf (log, "%s Player: %-15s 0x10: Ability. "
                "flags: 0x%04X, ItemID: %s, unknownA: 0x%08X, unknownB: 0x%08X\n", format_time (state.time),
                players[id].orgname, int (flags), make_id (itemid), * (int*) (data + pos), * (int*) (data + pos + 4));
          }
          else if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
            fprintf (log, "%s Player: %-15s 0x10: Ability. "
              "flags: 0x%04X, ItemID: %s\n", format_time (state.time),
              players[id].orgname, int (flags), make_id (itemid));

          W3GItem item;
//          fix_itemid (itemid);
          convert_itemid (item, itemid);
          if (itemid == 'Aamk')
            int adsf = 0;

          if (players[id].curSel)
            players[id].curSel->actions[players[id].index]++;

          if (item.type == ITEM_ABILITY && players[id].curSel)
          {
            DotaAbility* abl = (players[id].hero ? getAbilityById (itemid, players[id].hero->id) : NULL);
            if (abl == NULL)
              abl = getAbilityById (itemid, players[id].curSel->id);
            else
              players[id].curSel = players[id].hero;
            if (abl == NULL)
            {
              abl = getAbilityById (itemid);
              for (int i = 0; i < heroes.getSize (); i++)
              {
                if (::getHero (heroes[i].id)->index == abl->hero)
                {
                  players[id].curSel = &heroes[i];
                  break;
                }
              }
            }
            if (abl != NULL)
              players[id].curSel->pushAbility (abl->index, state.time, abl->slot);
          }
          else if (item.type == ITEM_ITEM)
          {
            if (players[id].inv.num_items == 0 ||
              players[id].inv.items[players[id].inv.num_items - 1] != item.id ||
              state.time > players[id].inv.itemt[players[id].inv.num_items - 1] + repDelayItem ||
              ((strstr (item.name, "Ironwood") || strstr (item.name, "Tango")) &&
              state.time > players[id].inv.itemt[players[id].inv.num_items - 1] + repDelayItem / 3))
            {
              DotaItem* it = getItemById (itemid);
              if (it->cost != 0)
              {
                int num = players[id].inv.num_items++;
                players[id].inv.items[num] = item.id;
                players[id].inv.itemt[num] = state.time;
                if (!strcmp (it->name, "Observer Wards") ||
                    !strcmp (it->name, "Sentry Wards"))
                  players[id].numWards += 2;
              }
            }
          }
        }
        break;
      // Unit/building ability (with target position)
      case 0x11:
        {
          players[id].actions++;
          unsigned short flags;
          if (hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (data + pos);
            pos += 2;
          }
          else
            flags = data[pos++];
          unsigned long itemid = * (unsigned long*) (data + pos);
          pos += 4;
          unsigned long iid_a = 0xFFFFFFFF;
          unsigned long iid_b = 0xFFFFFFFF;
          if (hdr.major_v >= 7)
          {
            iid_a = * (unsigned long*) (data + pos);
            iid_b = * (unsigned long*) (data + pos + 4);
            if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
              fprintf (log, "%s Player: %-15s 0x11: Ability. "
                "flags: 0x%04X, ItemID: %s, unknownA: 0x%08X, unknownB: 0x%08X, X: %s, Y: %s\n",
                format_time (state.time), players[id].orgname, int (flags), make_id (itemid), * (int*) (data + pos),
                * (int*) (data + pos + 4), format_coord (data + pos + 8), format_coord (data + pos + 12));
            pos += 8;
          }
          else if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
            fprintf (log, "%s Player: %-15s 0x11: Ability. "
              "flags: 0x%04X, ItemID: %s, X: %s, Y: %s\n",
              format_time (state.time), players[id].orgname, int (flags), make_id (itemid), format_coord (data + pos),
              format_coord (data + pos + 4));
          float targx = * (float*) (data + pos);
          float targy = * (float*) (data + pos + 4);
          pos += 8;
          if (players[id].numWards && iid_a != 0xFFFFFFFF && iid_b != 0xFFFFFFFF && itemid >= ID_USEITEM1 && itemid <= ID_USEITEM6)
          {
            players[id].numWards--;
            if (!quick)
              wards.add (W3GWard (targx, targy, id, state.time));
          }

          if (players[id].curSel)
            players[id].curSel->actions[players[id].index]++;

          if (itemid <= ID_HOLD)
          {
            players[id].acounter[ACTION_BASIC]++;
            if (isValidPos (targx) && isValidPos (targy) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_BASIC, targx, targy, itemid, state.time));
          }
          else
          {
            players[id].acounter[ACTION_ABILITY]++;
            if (isValidPos (targx) && isValidPos (targy) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_ABILITY, targx, targy, itemid, state.time));
          }
        }
        break;
      // Unit/building ability (with target position and target object ID)
      case 0x12:
        {
          players[id].actions++;
          unsigned short flags;
          if (hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (data + pos);
            pos += 2;
          }
          else
            flags = data[pos++];
          unsigned long itemid = * (unsigned long*) (data + pos);
          pos += 4;
          if (hdr.major_v >= 7)
          {
            if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
              fprintf (log, "%s Player: %-15s 0x12: Ability. "
                "flags: 0x%04X, ItemID: %s, unknownA: 0x%08X, unknownB: 0x%08X, X: %s, Y: %s, "
                "ID: 0x%08X %08X\n",
                format_time (state.time), players[id].orgname, int (flags), make_id (itemid), * (int*) (data + pos),
                * (int*) (data + pos + 4), format_coord (data + pos + 8), format_coord (data + pos + 12),
                * (int*) (data + pos + 16), * (int*) (data + pos + 20));
            pos += 8;
          }
          else if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
            fprintf (log, "%s Player: %-15s 0x12: Ability. "
              "flags: 0x%04X, ItemID: %s, X: %s, Y: %s, ID: 0x%08X %08X\n",
              format_time (state.time), players[id].orgname, int (flags), make_id (itemid), format_coord (data + pos),
              format_coord (data + pos + 4), * (int*) (data + pos + 8), * (int*) (data + pos + 12));
          float targx = * (float*) (data + pos);
          float targy = * (float*) (data + pos + 4);
          unsigned long obj1 = * (unsigned long*) (data + pos + 8);
          unsigned long obj2 = * (unsigned long*) (data + pos + 12);
          pos += 16;

          if (players[id].curSel)
            players[id].curSel->actions[players[id].index]++;

          if (itemid == ID_RIGHTCLICK)
          {
            unsigned long mtime = getModeTime (game.gmode);
            if (game.startTime)
              mtime = game.startTime;
            if (state.time > mtime && state.time < mtime + 180000)
            {
              players[id].avgClickX = players[id].avgClickX * players[id].counted + targx;
              players[id].avgClickY = players[id].avgClickY * players[id].counted + targy;
              players[id].counted++;
              players[id].avgClickX /= players[id].counted;
              players[id].avgClickY /= players[id].counted;
            }
            players[id].acounter[ACTION_RIGHTCLICK]++;
            if (isValidPos (targx) && isValidPos (targy) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_RIGHTCLICK, targx, targy, itemid, state.time));
          }
          else if (itemid <= ID_HOLD)
          {
            players[id].acounter[ACTION_BASIC]++;
            if (isValidPos (targx) && isValidPos (targy) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_BASIC, targx, targy, itemid, state.time));
          }
          else
          {
            players[id].acounter[ACTION_ABILITY]++;
            if (isValidPos (targx) && isValidPos (targy) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_ABILITY, targx, targy, itemid, state.time));
          }
        }
        break;
      // Give item to Unit / Drop item on ground
      case 0x13:
        {
          players[id].actions++;
          unsigned short flags;
          if (hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (data + pos);
            pos += 2;
          }
          else
            flags = data[pos++];
          unsigned long itemid = * (unsigned long*) (data + pos);
          pos += 4;
          if (hdr.major_v >= 7)
          {
            if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
              fprintf (log, "%s Player: %-15s 0x13: Give item. "
                "flags: 0x%04X, ItemID: %s, unknownA: 0x%08X, unknownB: 0x%08X, X: %s, Y: %s, "
                "TID: 0x%08X %08X, IID: 0x%08X %08X\n",
                format_time (state.time), players[id].orgname, int (flags), make_id (itemid), * (int*) (data + pos),
                * (int*) (data + pos + 4), format_coord (data + pos + 8), format_coord (data + pos + 12),
                * (int*) (data + pos + 16), * (int*) (data + pos + 20),
                * (int*) (data + pos + 24), * (int*) (data + pos + 28));
            pos += 8;
          }
          else if (log && !quick && (!ignoreBasic || itemid >= ID_GIVE))
            fprintf (log, "%s Player: %-15s 0x13: Give item. "
              "flags: 0x%04X, ItemID: %s, X: %s, Y: %s, "
              "TID: 0x%08X %08X, IID: 0x%08X %08X\n",
              format_time (state.time), players[id].orgname, int (flags), make_id (itemid),
              format_coord (data + pos), format_coord (data + pos + 4),
              * (int*) (data + pos + 8), * (int*) (data + pos + 12),
              * (int*) (data + pos + 16), * (int*) (data + pos + 20));
          float targx = * (float*) (data + pos);
          float targy = * (float*) (data + pos + 4);
          unsigned long obj1 = * (unsigned long*) (data + pos + 8);
          unsigned long obj2 = * (unsigned long*) (data + pos + 12);
          unsigned long item1 = * (unsigned long*) (data + pos + 16);
          unsigned long item2 = * (unsigned long*) (data + pos + 20);
          pos += 24;

          if (players[id].curSel)
            players[id].curSel->actions[players[id].index]++;

          players[id].acounter[ACTION_ITEM]++;
          if (isValidPos (targx) && isValidPos (targy) && !quick)
            pactions[players[id].index].add (W3GAction (ACTION_ITEM, targx, targy, itemid, state.time));
        }
        break;
      // Unit/building ability (with two target positions and two item IDs)
      case 0x14:
        {
          players[id].actions++;
          unsigned short flags;
          if (hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (data + pos);
            pos += 2;
          }
          else
            flags = data[pos++];
          unsigned long itemida = * (unsigned long*) (data + pos);
          pos += 4;
          if (hdr.major_v >= 7)
          {
            if (log && !quick && (!ignoreBasic || itemida >= ID_GIVE))
              fprintf (log, "%s Player: %-15s 0x14: Ability. "
                "flags: 0x%04X, ItemID A: %s, unknownA: 0x%08X, unknownB: 0x%08X, AX: %s, AY: %s, "
                "ItemID B: %s, unknown: 0x%02X%08X%08X, BX: %s, BY: %s\n",
                format_time (state.time), players[id].orgname, int (flags), make_id (itemida),
                * (int*) (data + pos), * (int*) (data + pos + 4),
                format_coord (data + pos + 8), format_coord (data + pos + 12),
                make_id (* (unsigned long*) (data + pos + 16)),
                (* (int*) (data + pos + 28)) & 0xFF, * (int*) (data + pos + 24), * (int*) (data + pos + 20),
                format_coord (data + pos + 29), format_coord (data + pos + 33));
            pos += 8;
          }
          else if (log && !quick && (!ignoreBasic || itemida >= ID_GIVE))
            fprintf (log, "%s Player: %-15s 0x14: Ability. "
              "flags: 0x%04X, ItemID A: %s, AX: %s, AY: %s, "
              "ItemID B: %s, unknown: 0x%02X%08X%08X, BX: %s, BY: %s\n",
              format_time (state.time), players[id].orgname, int (flags), make_id (itemida),
              format_coord (data + pos), format_coord (data + pos + 4),
              make_id (* (unsigned long*) (data + pos + 16)),
              (* (int*) (data + pos + 20)) & 0xFF, * (int*) (data + pos + 16), * (int*) (data + pos + 12),
              format_coord (data + pos + 21), format_coord (data + pos + 25));
          float targax = * (float*) (data + pos);
          float targay = * (float*) (data + pos + 4);
          unsigned long itemidb = * (unsigned long*) (data + pos + 8);
          pos += 21;
          float targbx = * (float*) (data + pos);
          float targby = * (float*) (data + pos + 4);
          pos += 8;

          if (players[id].curSel)
            players[id].curSel->actions[players[id].index]++;

          if (itemida == ID_RIGHTCLICK)
          {
            unsigned long mtime = getModeTime (game.gmode);
            if (game.startTime)
              mtime = game.startTime;
            if (state.time > mtime && state.time < mtime + 180000)
            {
              players[id].avgClickX = players[id].avgClickX * players[id].counted + targax;
              players[id].avgClickY = players[id].avgClickY * players[id].counted + targay;
              players[id].counted++;
              players[id].avgClickX /= players[id].counted;
              players[id].avgClickY /= players[id].counted;
            }
            players[id].acounter[ACTION_RIGHTCLICK]++;
            if (isValidPos (targax) && isValidPos (targay) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_RIGHTCLICK, targax, targay, itemida, state.time));
          }
          else if (itemida <= ID_HOLD)
          {
            players[id].acounter[ACTION_BASIC]++;
            if (isValidPos (targax) && isValidPos (targay) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_BASIC, targax, targay, itemida, state.time));
          }
          else
          {
            players[id].acounter[ACTION_ABILITY]++;
            if (isValidPos (targax) && isValidPos (targay) && !quick)
              pactions[players[id].index].add (W3GAction (ACTION_ABILITY, targax, targay, itemida, state.time));
          }
        }
        break;
      // Change Selection (Unit, Building, Area)
      case 0x16:
        {
          unsigned char mode = data[pos];
          unsigned short num = * (unsigned short*) (data + pos + 1);
          if (log && !quick)
            fprintf (log, "%s Player: %-15s 0x16: Change selection. mode: 0x%02X, n: %d\n",
              format_time (state.time), players[id].orgname, int (mode), int (num));
          pos += 3;
          for (int i = 0; i < num; i++)
          {
            if (log && !quick)
              fprintf (log, "  ID: 0x%08X %08X\n",
                * (int*) (data + pos), * (int*) (data + pos + 4));
            pos += 8;
          }

          if (mode == 0x02 || !was_deselect)
          {
            players[id].actions++;
            players[id].acounter[ACTION_SELECT]++;
            if (!quick)
              pactions[players[id].index].add (W3GAction (ACTION_SELECT, state.time));
          }
          was_deselect = (mode == 0x02);
        }
        break;
      // Assign Group Hotkey
      case 0x17:
        {
          unsigned char group = data[pos];
          unsigned short num = * (unsigned short*) (data + pos + 1);
          if (pos >= 96000)
            int asdf = 0;
          if (log && !quick)
            fprintf (log, "%s Player: %-15s 0x17: Assign hotkey. group: 0x%02X, n: %d\n",
              format_time (state.time), players[id].orgname, int (group), int (num));
          pos += 3;
          for (int i = 0; i < num; i++)
          {
            if (log && !quick)
              fprintf (log, "  ID: 0x%08X %08X\n",
                * (int*) (data + pos), * (int*) (data + pos + 4));
            pos += 8;
          }

          if (group < 12)
          {
            players[id].actions++;
            players[id].acounter[ACTION_ASSIGNHOTKEY]++;
            players[id].hkassign[group]++;
            if (!quick)
              pactions[players[id].index].add (W3GAction (ACTION_ASSIGNHOTKEY, state.time));
          }
        }
        break;
      // Select Group Hotkey
      case 0x18:
        {
          unsigned char group = data[pos];
          if (log && !quick)
            fprintf (log, "%s Player: %-15s 0x18: Select hotkey. group: 0x%02X, unknown: 0x%02X\n",
              format_time (state.time), players[id].orgname, int (group), int (* (char*) (data + pos + 1)));
          pos += 2;

          players[id].actions++;
          players[id].acounter[ACTION_SELECTHOTKEY]++;
          players[id].hkuse[group]++;
          if (!quick)
            pactions[players[id].index].add (W3GAction (ACTION_SELECTHOTKEY, state.time));
        }
        break;
      // Select Subgroup
      case 0x19:
        if (hdr.build_v >= 6040 || hdr.major_v > 14)
        {
          unsigned long itemid = * (unsigned long*) (data + pos);
          unsigned long objid1 = * (unsigned long*) (data + pos + 4);
          unsigned long objid2 = * (unsigned long*) (data + pos + 8);
          if (log && !quick)
            fprintf (log, "%s Player: %-15s 0x19: Select subgroup. ItemID: %s, ID: 0x%08X %08X\n",
              format_time (state.time), players[id].orgname, make_id (itemid),
              objid1, objid2);
          pos += 12;
//          fix_itemid (itemid);
          if (players[id].race == 0)
            players[id].race = convert_race (itemid);
          DotaHero* hero = getHeroById (itemid);
          if (hero)
          {
            W3GHero* h = getHero (objid1, objid2, players[id].slot.team, hero->index);
            if (h)
            {
              players[id].curSel = h;
              players[id].curSel->id = hero->index;
              if (players[id].hero == NULL && players[id].heroId == itemid)
                players[id].hero = h;
            }
          }

          if (was_subgroup)
          {
            players[id].actions++;
            players[id].acounter[ACTION_SUBGROUP]++;
            if (!quick)
              pactions[players[id].index].add (W3GAction (ACTION_SUBGROUP, state.time));
          }
        }
        else
        {
          unsigned char subgroup = data[pos++];

          if (subgroup != 0 && subgroup != 0xFF && !was_subgroup)
          {
            players[id].actions++;
            players[id].acounter[ACTION_SUBGROUP]++;
            if (!quick)
              pactions[players[id].index].add (W3GAction (ACTION_SUBGROUP, state.time));
          }
          was_subgroup = (subgroup == 0xFF);
        }
        break;
      // Pre Subselection
      // version < 14b: Only in scenarios, maybe a trigger-related command
      case 0x1A:
        if (log && !quick && !ignoreBasic)
          fprintf (log, "%s Player: %-15s 0x1A: Pre subselection\n",
              format_time (state.time), players[id].orgname);
        if (hdr.build_v > 6040 || hdr.major_v > 14)
          was_subgroup = (prev == 0x19 || prev == 0);
        else
          pos += 9;
        break;
      // Only in scenarios, maybe a trigger-related command
      // version <= 14b: Select Ground Item
      case 0x1B:
        pos += 9;
        if (hdr.build_v <= 6040 && hdr.major_v <= 14)
          players[id].actions++;
        break;
      // Select Ground Item
      // version < 14b: Cancel hero revival (new in 1.13)
      case 0x1C:
        if (hdr.build_v > 6040 || hdr.major_v > 14)
          pos += 9;
        else
          pos += 8;
        players[id].actions++;
        break;
      // Cancel hero revival
      case 0x1D:
        if (hdr.build_v > 6040 || hdr.major_v > 14)
          pos += 8;
        else
          pos += 5;
        players[id].actions++;
        break;
      // Remove unit from building queue
      case 0x1E:
        pos += 5;
        players[id].actions++;
        break;
      // Found in replays with patch version 1.04 and 1.05.
      case 0x21:
        pos += 8;
        break;
      // Single Player Cheats
      case 0x20:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
      case 0x28:
      case 0x29:
      case 0x2A:
      case 0x2B:
      case 0x2C:
      case 0x2D:
      case 0x2E:
      case 0x2F:
      case 0x30:
      case 0x31:
      case 0x32:
        {
          if (quick)
          {
            switch (action)
            {
            case 0x27:
              pos += 5;
              break;
            case 0x28:
              pos += 5;
              break;
            case 0x2D:
              pos += 5;
              break;
            case 0x2E:
              pos += 4;
              break;
            }
          }
          else
          {
            notif = "";
            switch (action)
            {
            case 0x20:
              notif = "TheDudeAbides (Fast cooldown)";
              break;
            case 0x22:
              notif = "SomebodySetUpUsTheBomb (Instant defeat)";
              break;
            case 0x23:
              notif = "WarpTen (Speeds construction)";
              break;
            case 0x24:
              notif = "IocainePowder (Fast Death/Decay)";
              break;
            case 0x25:
              notif = "PointBreak (Removes food limit)";
              break;
            case 0x26:
              notif = "WhosYourDaddy (God mode)";
              break;
            case 0x27:
              notif = "KeyserSoze (Gives you X Gold)";
              pos += 5;
              break;
            case 0x28:
              notif = "LeafitToMe (Gives you X Lumber)";
              pos += 5;
              break;
            case 0x29:
              notif = "ThereIsNoSpoon (Unlimited Mana)";
              break;
            case 0x2A:
              notif = "StrengthAndHonor (No defeat)";
              break;
            case 0x2B:
              notif = "ItVexesMe (Disable victory conditions)";
              break;
            case 0x2C:
              notif = "WhoIsJohnGalt (Enable research)";
              break;
            case 0x2D:
              notif = "GreedIsGood (Gives you X Gold and Lumber)";
              pos += 5;
              break;
            case 0x2E:
              notif = "DayLightSavings (Set a time of day)";
              pos += 4;
              break;
            case 0x2F:
              notif = "ISeeDeadPeople (Remove fog of war)";
              break;
            case 0x30:
              notif = "Synergy (Disable tech tree requirements)";
              break;
            case 0x31:
              notif = "SharpAndShiny (Research upgrades)";
              break;
            case 0x32:
              notif = "AllYourBaseAreBelongToUs (Instant victory)";
              break;
            }
            add_syschat (0, id, state.time, "%s uses a cheat %s.", players[id].name, (char const*) notif);
          }
        }
        break;
      // Change ally options
      case 0x50:
        {
          unsigned char slot = data[pos];
          unsigned long flags = * (unsigned long*) (data + pos + 1);
          pos += 5;
          if (!quick)
          {
            W3GPlayer* other;
            if ((flags & 0x40) && !players[id].share[slot] && (other = getPlayerInSlot (slot)))
              add_syschat (CHAT_NOTIFY_CONTROL, id, state.time, "@%d@%s shares control with @%d@%s.",
              id, players[id].name, other->player_id, other->name);
            else if (!(flags & 0x40) && players[id].share[slot] && (other = getPlayerInSlot (slot)))
              add_syschat (CHAT_NOTIFY_CONTROL, id, state.time, "@%d@%s disables control sharing with @%d@%s.",
              id, players[id].name, other->player_id, other->name);
          }
          players[id].share[slot] = (flags & 0x40) != 0;
        }
        break;
      // Transfer resources
      case 0x51:
        {
          unsigned char slot = data[pos];
          unsigned long gold = * (unsigned long*) (data + pos + 1);
          unsigned long lumber = * (unsigned long*) (data + pos + 5);
          pos += 9;

          W3GPlayer* other = getPlayerInSlot (slot);
          if (other && !quick)
            add_syschat (0, id, state.time, "@%d@%s sends @%d@%s %d gold and %d lumber.",
            id, players[id].name, other->player_id, other->name, gold, lumber);
        }
        break;
      // Map trigger chat command (?)
      case 0x60:
        {
          pos += 8;
          W3GMessage msg;
          msg.id = id;
          msg.mode = CHAT_COMMAND;
          msg.time = state.time;
          int len = 0;
          while (data[pos] != 0)
            msg.text[len++] = data[pos++];
          pos++;
          msg.text[len] = 0;
          un_unicode (msg.text, msg.utext);
          if (!quick)
          {
            int cnt = 0;
            int cur;
            for (cur = chat.getSize () - 1; cur >= 0 && cnt < 10; cur--)
            {
              if (chat[cur].id == id)
                break;
              cnt++;
            }
            if (cur >= 0 && chat[cur].id == id && !strcmp (chat[cur].text, msg.text))
            {
            }
            else
              chat.add (msg);
          }
          if (state.time < 15000)
            getMode (msg);

          // handle -switch
          if ((game.gmode & MODE_SO) && !game.hasObservers)
          {
            if (game.pSwitching != 0 && state.time - game.switchStart < 60000)
            {
              if (!strcmp (msg.text, "-ok") || !strcmp (msg.text, "-switch accept"))
              {
                state.switchAccept[id] = true;
                int bad = 0;
                int good = 0;
                int total = 0;
                for (int i = 0; i < numPlayers; i++)
                {
                  if (!players[pindex[i]].left && !players[pindex[i]].slot.computer)
                  {
                    total++;
                    if (state.switchAccept[pindex[i]])
                      good++;
                    else
                      bad++;
                  }
                }
                if (bad < 2 && good >= total / 2 && (good > 1 || total != 2))
                {
                  add_syschat (0, 0, state.time, "The switch process between @%d|%d@%s and @%d|%d@%s has been completed successfully",
                    game.pSwitching, players[game.pSwitching].slot.color, fmt_player (game.pSwitching),
                    game.pSwitchingWith, players[game.pSwitchingWith].slot.color, fmt_player (game.pSwitchingWith));
                  unsigned char temp = players[game.pSwitching].slot.color;
                  players[game.pSwitching].slot.color = players[game.pSwitchingWith].slot.color;
                  players[game.pSwitchingWith].slot.color = temp;
                  temp = players[game.pSwitching].slot.team;
                  players[game.pSwitching].slot.team = players[game.pSwitchingWith].slot.team;
                  players[game.pSwitchingWith].slot.team = temp;
                  game.pSwitching = 0;
                }
              }
              else if (!strcmp (msg.text, "-no"))
                game.pSwitching = 0;
            }
            else
            {
              if (!strncmp (msg.text, "-switch ", 8) && msg.text[8] >= '1' && msg.text[8] <= '5')
              {
                memset (state.switchAccept, 0, sizeof state.switchAccept);
                state.switchAccept[id] = true;
                int tcolor = msg.text[8] - '0';
                if (players[id].slot.color < 6)
                  tcolor += 6;
                game.pSwitchingWith = -tcolor;
                for (int i = 0; i < numPlayers; i++)
                  if (players[pindex[i]].slot.color == tcolor)
                    game.pSwitchingWith = pindex[i];
                if (game.pSwitchingWith >= 0)
                {
                  game.pSwitching = id;
                  game.switchStart = state.time;
                }
              }
            }
          }
        }
        break;
      // ESC pressed
      case 0x61:
        players[id].actions++;
        players[id].acounter[ACTION_ESC]++;
        if (!quick)
          pactions[players[id].index].add (W3GAction (ACTION_ESC, state.time));
        break;
      // Scenario Trigger
      case 0x62:
        pos += 8;
        if (hdr.major_v >= 7)
          pos += 4;
        break;
      // Enter select hero skill submenu for WarCraft III patch version <= 1.06
      case 0x65:
        players[id].actions++;
        break;
      // Enter select hero skill submenu
      // Enter select building submenu for WarCraft III patch version <= 1.06
      case 0x66:
        players[id].actions++;
        break;
      // Enter select building submenu
      // Minimap signal (ping) for WarCraft III patch version <= 1.06
      case 0x67:
        if (hdr.major_v < 7)
        {
          float posx = * (float*) (data + pos);
          float posy = * (float*) (data + pos + 4);
          if (!quick)
          {
            W3GMessage msg;
            msg.id = id;
            msg.mode = CHAT_PING;
            msg.time = state.time;
            msg.x = posx;
            msg.y = posy;
            chat.add (msg);
          }
          pos += 12;
        }
        else
          players[id].actions++;
        break;
      // Minimap signal (ping)
      // Continue Game (BlockB) for WarCraft III patch version <= 1.06
      case 0x68:
        if (hdr.major_v >= 7)
        {
          float posx = * (float*) (data + pos);
          float posy = * (float*) (data + pos + 4);
          if (!quick)
          {
            W3GMessage msg;
            msg.id = id;
            msg.mode = CHAT_PING;
            msg.time = state.time;
            msg.x = posx;
            msg.y = posy;
            chat.add (msg);
          }
          pos += 12;
        }
        else
          pos += 16;
        break;
      // Continue Game (BlockB)
      // Continue Game (BlockA) for WarCraft III patch version <= 1.06
      case 0x69:
      // Continue Game (BlockA)
      case 0x6A:
        state.continue_game = true;
        pos += 16;
        break;
      // SyncStoredInteger actions (endgame action)
      case 0x6B:
        {
          CString cache = "";
          CString slot = "";
          CString stat = "";
          char* a = data + pos;
          while (data[pos] != 0)
            cache += data[pos++];
          pos++;
          char* b = data + pos;
          while (data[pos] != 0)
            slot += data[pos++];
          pos++;
          char* c = data + pos;
          while (data[pos] != 0)
            stat += data[pos++];
          pos++;
          unsigned long value = * (unsigned long*) (data + pos);
          pos += 4;
          if (log && !quick)
            fprintf (log, "%s 0x06B: SyncStoredInteger (%s, %s, %s, %d)\n",
              format_time (state.time), a, b, c, value);
          parseEndgame (slot, stat, value, state);
        }
        break;
      // Unknown (v6.39 and v6.39b endgame)
      case 0x70:
        {
          CString cache = "";
          CString slot = "";
          CString stat = "";
          while (data[pos] != 0)
            cache += data[pos++];
          pos++;
          while (data[pos] != 0)
            slot += data[pos++];
          pos++;
          while (data[pos] != 0)
            stat += data[pos++];
          pos++;
        }
        break;
      // Only in scenarios, maybe a trigger-related command
      case 0x75:
        pos++;
        break;
      default:
        pos++;
      }
      was_deselect = false;
      was_subgroup = false;
    }
    pos = next;
  }
  pos = end;
  return true;
}

int make_int (CString const& str)
{
  int val = 0;
  for (int i = 0; str[i]; i++)
  {
    if (str[i] < '0' || str[i] > '9') return 0;
    val = val * 10 + str[i] - '0';
  }
  return val;
}

void W3GReplay::finishEndgame (GameState& state)
{
  //for (int i = 0; i < numPlayers; i++)
  //{
  //  if (state.playerid[i])
  //  {
  //    for (int j = 0; j < 7; j++)
  //      players[pindex[i]].stats[j] = state.endgame[pindex[i]][j];
  //    if (state.endgameid[i] != players[pindex[i]].slot.color)
  //    {
  //      players[pindex[i]].slot.color = state.endgameid[i];
  //      players[pindex[i]].slot.team = (i > 5 ? 1 : 0);
  //    }
  //  }
  //}
}
int W3GReplay::_getCaptain (int team)
{
  int best = -team;
  int aMin = (team == 1 ? 1 : 7);
  int aMax = (team == 1 ? 5 : 11);
  for (int i = 0; i < numPlayers; i++)
  {
    int clr = players[pindex[i]].slot.color;
    if (clr >= aMin && clr <= aMax)
    {
      best = clr;
      aMax = clr - 1;
    }
  }
  return best;
}
int W3GReplay::getCaptain (int team)
{
  int best = 0;
  int aMin = (team == 1 ? 1 : 7);
  int aMax = (team == 1 ? 5 : 11);
  for (int i = 0; i < numPlayers; i++)
  {
    int clr = players[pindex[i]].slot.color;
    if (clr >= aMin && clr <= aMax)
    {
      best = pindex[i];
      aMax = clr - 1;
    }
  }
  return best;
}
void W3GReplay::add_syschat (int type, int id, unsigned long time, char const* fmt, ...)
{
  W3GMessage msg;
  va_list ap;
  va_start (ap, fmt);
  vsprintf (msg.text, fmt, ap);
  va_end (ap);
  msg.id = id;
  msg.mode = CHAT_NOTIFY;
  msg.notifyType = type;
  msg.time = time;
  add_chat (msg);
}
void W3GReplay::parseEndgame (CString const& slot, CString const& stat, unsigned long value, GameState& state)
{
  dota.endgame = true;

//  fprintf (__f, "  {0x%08X, \"%s\"},\n", value, (char const*) stat);
  if (slot == "Global")
  {
    if (stat == "Winner")
    {
      int id = getBuildingId (2 - value, BUILDING_THRONE, 0, 0);
      if (id >= 0)
        dota.bdTime[id] = state.time;
      game.winner = value;
      real_time = state.time;
    }
  }
  else if (slot == "Data")
  {
    W3GEvent e;
    memset (&e, 0, sizeof e);
    e.time = state.time;
    static char __race[5][16] = {"Sentinel", "Scourge", "The Sentinel", "The Scourge", "Neutral Creeps"};
    static char __raceid[5][16] = {"s", "u", "s", "u", "n"};
    static char __side[3][16] = {"top", "middle", "bottom"};
    static char __rax[3][16] = {"melee", "ranged"};
    static char __tact[2][16] = {"destroyed", "denied"};
    if (stat == "GameStart")
    {
      e.type = EVENT_GAMESTART;
      game.startTime = state.time;
    }
    else if (stat.Left (4) == "Pool")
    {
      e.type = EVENT_POOL;
      e.p[0] = make_int (stat.Mid (4));
      DotaHero* hero = getHeroByPoint (value);
      if (hero == NULL)
        hero = getHeroById (value);
      e.p[1] = (hero ? hero->index : -1);
      if (hero)
        game.draft.pool[game.draft.numPool++] = hero->index;
    }
    else if (stat.Left (3) == "Ban")
    {
      e.type = EVENT_HEROBAN;
      e.p[0] = make_int (stat.Mid (3));
      DotaHero* hero = getHeroByPoint (value);
      if (hero == NULL)
        hero = getHeroById (value);
      e.p[0] = (e.p[0] > 5 ? 2 : 1);
      e.p[1] = (hero ? hero->index : -1);
      if (game.draft.firstPick == 0)
        game.draft.firstPick = e.p[0];
      if (hero)
        game.draft.bans[e.p[0] - 1][game.draft.numBans[e.p[0] - 1]++] = hero->index;
      if (hero && !state.quick)
      {
        int pl = getCaptain (e.p[0]);
        if (pl >= 0)
          add_syschat (CHAT_NOTIFY_PICKS, 0, state.time, "@%d|%d@%s has banned %s",
          pl, players[pl].slot.color, players[pl].name, hero->abbr);
        else
          add_syschat (CHAT_NOTIFY_PICKS, 0, state.time, "@%s@ has banned %s",
          __raceid[-pl + 1], hero->abbr);
      }
    }
    else if (stat.Left (4) == "Pick")
    {
      e.type = EVENT_HEROPICK;
      e.p[0] = make_int (stat.Mid (4));
      DotaHero* hero = getHeroByPoint (value);
      if (hero == NULL)
        hero = getHeroById (value);
      e.p[0] = (e.p[0] > 5 ? 2 : 1);
      e.p[1] = (hero ? hero->index : -1);
      if (game.draft.firstPick == 0)
        game.draft.firstPick = e.p[0];
      if (hero)
        game.draft.picks[e.p[0] - 1][game.draft.numPicks[e.p[0] - 1]++] = hero->index;
      if (hero && !state.quick)
      {
        int pl = getCaptain (e.p[0]);
        if (pl >= 0)
          add_syschat (CHAT_NOTIFY_PICKS, 0, state.time, "@%d|%d@%s has picked %s",
          pl, players[pl].slot.color, players[pl].name, hero->abbr);
        else
          add_syschat (CHAT_NOTIFY_PICKS, 0, state.time, "@%s@ has picked %s",
          __raceid[-pl + 1], hero->abbr);
      }
    }
    else if (stat.Left (6) == "Assist")
    {
      e.type = EVENT_HEROASSIST;
      e.p[0] = make_int (stat.Mid (6));
      e.p[0] = state.playerid[e.p[0]];
      e.p[1] = state.playerid[value];
      if (e.p[0] > 0 && e.p[1] > 0 &&
          (players[e.p[0]].slot.color > 5) != (players[e.p[1]].slot.color > 5))
      {
        if (chatAssists && state.lastKillTarget == e.p[1] &&
          state.time - state.lastKillTime < 250 && state.lastKillPlayer != e.p[0] && !state.quick)
        {
          char* text = chat[state.lastKillMessage].text;
          wchar_t* utext = chat[state.lastKillMessage].utext;
          int len = (int) strlen (text);
          int wlen = (int) wcslen (utext);
          int pos = len;
          if (state.lastKillFirst)
          {
            strcpy (text + pos, " Assist: ");
            pos += 9;
            state.lastKillFirst = false;
          }
          else
            text[pos++] = '/';
          sprintf (text + pos, "@%d|%d@%s", e.p[0], players[e.p[0]].slot.color,
            players[e.p[0]].name);
          while (text[len])
            utext[wlen++] = text[len++];
          utext[wlen] = 0;
        }
        players[e.p[0]].stats[STAT_ASSISTS]++;
      }
    }
    else if (stat.Left (4) == "Hero")
    {
      e.type = EVENT_HEROKILL;
      e.p[0] = state.playerid[value];
      e.p[1] = make_int (stat.Mid (4));
      e.p[1] = state.playerid[e.p[1]];
      bool killCounts = false;

      state.lastKillTarget = e.p[1];
      state.lastKillTime = state.time;
      state.lastKillMessage = chat.getSize ();
      state.lastKillFirst = true;
      state.lastKillPlayer = e.p[0];

      if (e.p[0] > 0 && e.p[1] > 0 &&
          (players[e.p[0]].slot.color > 5) != (players[e.p[1]].slot.color > 5))
      {
        if (state.time - state.lastKill[e.p[0]] < 18000)
          state.quickKill[e.p[0]]++;
        else
          state.quickKill[e.p[0]] = 1;
        state.spree[e.p[0]]++;
        killCounts = true;
        state.lastKill[e.p[0]] = state.time;
        players[e.p[0]].stats[STAT_KILLS]++;
      }
      if (e.p[1] > 0)
      {
        if (e.p[1] != e.p[0])
          state.spree[e.p[1]] = 0;
        if (players[e.p[1]].nDeaths < 128)
        {
          players[e.p[1]].deathTime[players[e.p[1]].nDeaths] = state.time;
          players[e.p[1]].deathEnd[players[e.p[1]].nDeaths] = state.time
            + 4000 * players[e.p[1]].lCount;
          players[e.p[1]].nDeaths++;
        }
        players[e.p[1]].stats[STAT_DEATHS]++;
      }
      if (!state.quick)
      {
        if (e.p[0] > 0 && e.p[1] > 0)
        {
          if (e.p[0] == e.p[1])
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s has killed himself.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]));
          else if ((players[e.p[0]].slot.color > 5) == (players[e.p[1]].slot.color > 5))
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s was killed by his teammate @%d|%d@%s.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]),
              e.p[0], players[e.p[0]].slot.color, fmt_player (e.p[0]));
          else
          {
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s was killed by @%d|%d@%s.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]),
              e.p[0], players[e.p[0]].slot.color, fmt_player (e.p[0]));
            players[e.p[1]].pdied[e.p[0]]++;
            players[e.p[0]].pkilled[e.p[1]]++;
          }
        }
        else if (e.p[1] > 0)
        {
          if (value == 6)
          {
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s was killed by @u@.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]));
            if (players[e.p[1]].slot.team == 0)
              players[e.p[1]].sdied++;
          }
          else if (value < 6)
          {
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s was killed by @s@.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]));
            if (players[e.p[1]].slot.team == 1)
              players[e.p[1]].sdied++;
          }
          else
            add_syschat (CHAT_NOTIFY_KILL, e.p[0], state.time, "@%d|%d@%s was killed by @n@.",
              e.p[1], players[e.p[1]].slot.color, fmt_player (e.p[1]));
        }
      }
      if (killCounts)
      {
        int spree = state.spree[e.p[0]];
        int quickKill = state.quickKill[e.p[0]];
        if (!state.quick)
        {
          if (spree >= 3)
            add_syschat (CHAT_NOTIFY_SPREE, e.p[0], state.time, "@%d|%d@%s @m%d@",
            e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, spree >= 10 ? 0 : spree);
          if (quickKill >= 2)
            add_syschat (CHAT_NOTIFY_FASTKILL, e.p[0], state.time, "@%d|%d@%s @k%d@",
            e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, quickKill > 5 ? 5 : quickKill);
        }
      }
    }
    else if (stat.Left (5) == "Tower")
    {
      e.type = EVENT_TOWERKILL;
      if (value == 0 || value == 6)
        e.p[0] = 0;
      else
        e.p[0] = state.playerid[value];
      e.p[1] = stat[5] - '0';
      e.p[2] = stat[7] - '0';
      e.p[3] = stat[6] - '0';
      int id = getBuildingId (e.p[1], BUILDING_TOWER, e.p[2], e.p[3]);
      if (id >= 0)
      {
        if (e.p[3] == 4 && dota.bdTime[id])
          id++;
        dota.bdTime[id] = state.time;
      }
      if (!state.quick)
      {
        int kp;
        int act = 0;
        if (value == 0)
          kp = -2;
        else if (value == 6)
          kp = -3;
        else
          kp = state.playerid[value];
        if (value <= 5 && e.p[1] == 0 || value > 5 && e.p[1] == 1)
          act = 1;
        if (kp > 0 && act == 0)
          players[kp].skilled++;
        if (kp > 0)
          add_syschat (CHAT_NOTIFY_TOWER, kp, state.time, "%s %s level %d tower was %s by @%d|%d@%s.",
          __race[e.p[1]], __side[e.p[2]], e.p[3], __tact[act],
          kp, players[kp].slot.color, fmt_player (kp));
        else if (kp < 0)
          add_syschat (CHAT_NOTIFY_TOWER, 0, state.time, "%s %s level %d tower was %s by @%s@.",
          __race[e.p[1]], __side[e.p[2]], e.p[3], __tact[act], __raceid[-kp]);
      }
    }
    else if (stat.Left (3) == "Rax")
    {
      e.type = EVENT_RAXKILL;
      if (value == 0 || value == 6)
        e.p[0] = 0;
      else
        e.p[0] = state.playerid[value];
      e.p[1] = stat[3] - '0';
      e.p[2] = stat[4] - '0';
      e.p[3] = stat[5] - '0';
      int id = getBuildingId (e.p[1], e.p[3] ? BUILDING_RANGED : BUILDING_MELEE, e.p[2], 0);
      if (id >= 0)
        dota.bdTime[id] = state.time;
      if (!state.quick)
      {
        int kp;
        int act = 0;
        if (value == 0)
          kp = -2;
        else if (value == 6)
          kp = -3;
        else
          kp = state.playerid[value];
        if (value <= 5 && e.p[1] == 0 || value > 5 && e.p[1] == 1)
          act = 1;
        if (kp > 0 && act == 0)
          players[kp].skilled++;
        if (kp > 0)
          add_syschat (CHAT_NOTIFY_BARRACKS, kp, state.time, "%s %s %s barracks were %s by @%d|%d@%s.",
          __race[e.p[1]], __side[e.p[2]], __rax[e.p[3]], __tact[act],
          kp, players[kp].slot.color, fmt_player (kp));
        else if (kp < 0)
          add_syschat (CHAT_NOTIFY_BARRACKS, 0, state.time, "%s %s %s barracks were %s by @%s@.",
          __race[e.p[1]], __side[e.p[2]], __rax[e.p[3]], __tact[act], __raceid[-kp]);
      }
    }
    else if (stat.Left (7) == "Courier")
    {
      e.type = EVENT_COURIERKILL;
      e.p[0] = state.playerid[value];
      e.p[1] = state.playerid[make_int (stat.Mid (7))];
      if (!state.quick)
      {
        int kp;
        if (value == 0)
          kp = -2;
        else if (value == 6)
          kp = -3;
        else
          kp = state.playerid[value];
        if (kp > 0)
          add_syschat (CHAT_NOTIFY_COURIER, e.p[0], state.time, "@%d|%d@%s's courier was killed by @%d|%d@%s.",
          e.p[1], players[e.p[1]].slot.color, players[e.p[1]].name,
          e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name);
        else if (kp < 0)
          add_syschat (CHAT_NOTIFY_COURIER, e.p[0], state.time, "@%d|%d@%s's courier was killed by @%s@.",
          e.p[1], players[e.p[1]].slot.color, players[e.p[1]].name, __raceid[-kp]);
      }
    }
    else if (stat == "Tree")
    {
      e.type = EVENT_TREEHEALTH;
      e.p[0] = value;
      e.p[1] = 0;
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_TREE, 0, state.time, "The World Tree is at %d%%!", value);
    }
    else if (stat == "Throne")
    {
      e.type = EVENT_TREEHEALTH;
      e.p[0] = value;
      e.p[1] = 1;
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_TREE, 0, state.time, "The Frozen Throne is at %d%%!", value);
    }
    else if (stat == "Roshan")
    {
      e.type = EVENT_ROSHANKILL;
      e.p[0] = value;
      e.p[1] = 0;
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_ROSHAN, 0, state.time, "Roshan has been slain by @%s@!",
        __raceid[value]);
    }
    else if (stat == "AegisOn")
    {
      e.type = EVENT_AEGISON;
      e.p[0] = state.playerid[value];
      e.p[1] = 0;
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_AEGIS, 0, state.time, "@%d|%d@%s has acquired Aegis of the Immortal",
        e.p[0], players[e.p[0]].slot.color, fmt_player (e.p[0]));
    }
    else if (stat == "AegisOff")
    {
      e.type = EVENT_AEGISOFF;
      e.p[0] = state.playerid[value];
      e.p[1] = 0;
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_AEGIS, 0, state.time, "@%d|%d@%s has lost the Aegis of the Immortal",
        e.p[0], players[e.p[0]].slot.color, fmt_player (e.p[0]));
    }
    else if (stat.Left (4) == "Mode")
    {
      e.type = EVENT_GAMEMODE;
      e.p[0] = (value ? state.playerid[value] : -1);
      e.p[1] = 0;
      static char mstr[256];
      strcpy (mstr, stat.Mid (4));
      parseMode (mstr);
      if (!state.quick)
      {
        if (value)
          add_syschat (CHAT_NOTIFY_GAMEMODE, 0, state.time, "@%d|%d@%s has selected %s mode",
          e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, game.game_mode);
        else
          add_syschat (CHAT_NOTIFY_GAMEMODE, 0, state.time, "Game mode automatically set to %s",
          game.game_mode);
      }
    }
    else if (stat.Left (9) == "RuneStore")
    {
      e.type = EVENT_RUNESTORE;
      e.p[0] = state.playerid[value];
      e.p[1] = make_int (stat.Mid (9));
      state.runestore[e.p[0]] = e.p[1];
      if (!state.quick)
        add_syschat (CHAT_NOTIFY_RUNE, 0, state.time, "@%d|%d@%s has bottled the @r%d@ rune",
        e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, e.p[1]);
    }
    else if (stat.Left (7) == "RuneUse")
    {
      e.type = EVENT_RUNEUSE;
      e.p[0] = state.playerid[value];
      e.p[1] = make_int (stat.Mid (7));
      if (!state.quick)
      {
        if (e.p[1] == state.runestore[e.p[0]])
        {
          state.runestore[e.p[0]] = 0;
          add_syschat (CHAT_NOTIFY_RUNE, 0, state.time, "@%d|%d@%s has used the stored @r%d@ rune",
            e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, e.p[1]);
        }
        else
          add_syschat (CHAT_NOTIFY_RUNE, 0, state.time, "@%d|%d@%s has acquired a @r%d@ rune",
            e.p[0], players[e.p[0]].slot.color, players[e.p[0]].name, e.p[1]);
      }
    }
    else if (stat.Left (5) == "Level")
    {
      e.type = EVENT_LEVELUP;
      e.p[0] = state.playerid[value];
      e.p[1] = make_int (stat.Mid (5));
      while (players[e.p[0]].lCount < e.p[1])
        players[e.p[0]].ltime[players[e.p[0]].lCount++] = state.time;
    }
    else if (stat.Left (2) == "CK")
    {
      e.type = EVENT_PLAYERLEAVE;
      e.p[0] = state.playerid[value];
      sscanf (stat, "CK%dD%dN%d", &e.p[1], &e.p[2], &e.p[3]);
      players[e.p[0]].stats[STAT_CREEPS] = e.p[1];
      players[e.p[0]].stats[STAT_DENIES] = e.p[2];
      players[e.p[0]].stats[STAT_NEUTRALS] = e.p[3];
    }
    events.add (e);
  }
  else
  {
    int srow = make_int (slot);
    if (srow == 0) return;
    int pid = state.playerid[srow];
    if (stat == "id")
    {
      if (value > 5) value++;
      players[pid].slot.color = value;
      players[pid].slot.team = (value > 5 ? 1 : 0);
    }
    else
    {
      int scol = make_int (stat);
      if (scol >= 1 && scol <= 7)
        players[pid].stats[scol - 1] = value;
      else if (scol == 9)
      {
        players[pid].heroId = value;
        if (players[pid].hero == NULL)
        {
          DotaHero* hero = getHeroById (value);
          if (hero)
            players[pid].hero = newHero (players[pid].slot.team, hero->index);
        }
      }
      else if (stat[0] == '8')
      {
        gotitems = true;
        int sindex = make_int (stat.Right (1));
        if (sindex >= 0 && sindex <= 5)
        {
          DotaItem* item = getItemById (value);
          W3GPlayer* p = &players[pid];
          if (item)
            p->finalItems[sindex] = item->index;
          else
            p->finalItems[sindex] = 0;
        }
      }
    }
  }
}

bool is_substr (char const* a, char const* b)
{
  for (int i = 0; a[i]; i++)
    if (!strnicmp (a + i, b, strlen (b)))
      return true;
  return false;
}

inline float vec_len (float x, float y)
{
  return sqrt (x * x + y * y);
}

extern wchar_t ownNames[256];

bool W3GReplay::preAnalyze ()
{
  for (int i = 0; game.map[i]; i++)
  {
    if (!strnicmp (game.map + i, "dota allstars v", strlen ("dota allstars v")) ||
        !strnicmp (game.map + i, "dota_allstars_v", strlen ("dota_allstars_v")) ||
        !strnicmp (game.map + i, "dota v", strlen ("dota v")))
    {
      int vi = i;
      if (!strnicmp (game.map + i, "dota v", strlen ("dota v")))
        vi += (int) strlen ("dota v");
      else
        vi += (int) strlen ("dota allstars v");
      if (game.map[vi] < '0' || game.map[vi] > '9')
        return false;
      dota.major = game.map[vi] - '0';
      while (game.map[vi] != '.' && game.map[vi])
        vi++;
      if (game.map[vi] != '.' || game.map[vi + 1] < '0' || game.map[vi + 1] > '9'
                              || game.map[vi + 2] < '0' || game.map[vi + 2] > '9')
        return false;
      dota.minor = (game.map[vi + 1] - '0') * 10 + (game.map[vi + 2] - '0');
      if (game.map[vi + 3] == 'b')
        dota.build = 1;
      if (game.map[vi + 3] == 'c')
        dota.build = 2;
      break;
    }
  }
  if (dota.major != 0)
  {
    for (int i = 0; i < numPlayers; i++)
    {
      int id = pindex[i];
      if (players[id].slot.color > 11) continue; // observer
      if (players[id].slot.slot_status == 0)
        continue;
      if (players[id].slot.color == 0 || players[id].slot.color == 6)
        return false;
      int team = (players[id].slot.color > 6 ? 1 : 0);
      if (team != players[id].slot.team)
        return false;
    }
  }
  return dota.major != 0;
}

void W3GReplay::analyze ()
{
  if (real_time < time)
  {
    unsigned long t = real_time;
    real_time = time;
    time = t;
  }
  for (int i = 0; i < numPlayers; i++)
  {
    int id = pindex[i];
    if (players[id].time >= time)
      players[id].time = time;
  }
  if (game.saver_id == 0)
  {
    int pos = 0;
    int cur = 0;
    while (game.saver_id == 0)
    {
      if (ownNames[cur] == ' ' || ownNames[cur] == ',' || ownNames[cur] == ';' || ownNames[cur] == 0)
      {
        for (int i = 0; i < numPlayers && cur > pos; i++)
          if (!wcsnicmp (players[pindex[i]].uname, ownNames + pos, cur - pos))
            game.saver_id = pindex[i];
        pos = cur + 1;
      }
      if (ownNames[cur] == 0)
        break;
      cur++;
    }
  }
  if (!dota.isDota)
  {
    game.game_mode[0] = 0;
    if (game.lwinner == 15)
      game.lwinner = players[game.saver_id].slot.team;
    else if (game.lwinner < 0)
    {
      bool found = false;
      for (int i = 0; i < numPlayers; i++)
      {
        if (game.llost[players[pindex[i]].slot.team] == false)
        {
          if (!found)
            game.lwinner = players[pindex[i]].slot.team;
          else
            game.lwinner = -1;
          found = true;
        }
      }
      int count = 0;
      int pos = -1;
      for (int i = 0; i < numPlayers; i++)
      {
        if (players[pindex[i]].slot.team == game.lwinner)
        {
          pos = pindex[i];
          count++;
        }
      }
      if (count == 1)
        game.wplayer = pos;
      else
        game.wplayer = 0;
    }
    for (int i = 0; i < numPlayers; i++)
      if (players[pindex[i]].race == 0)
        players[pindex[i]].race = players[pindex[i]].slot.race;
    return;
  }
  if ((game.gmode & (~MODE_WTF)) == 0)
  {
    if (game.gmode & MODE_WTF)
      strcpy (game.game_mode, "-wtf");
    else
      strcpy (game.game_mode, "Normal mode");
  }
  else if (game.gmode & MODE_WTF)
    strcat (game.game_mode, " -wtf");
  for (int i = 0; i < numPlayers; i++)
  {
    int id = pindex[i];
    DotaHero* hero = players[id].heroId ? getHeroById (players[id].heroId) : NULL;
    if (players[id].hero == NULL)
    {
      for (int j = 0; j < heroes.getSize (); j++)
        if ((players[id].hero == NULL || heroes[j].actions[players[id].index] >
            players[id].hero->actions[players[id].index]) && !heroes[j].claimed)
          players[id].hero = &heroes[j];
    }
    if (hero && (players[id].hero == NULL || players[id].hero->id != hero->index))
    {
      for (int j = 0; j < heroes.getSize (); j++)
        if (heroes[j].id == hero->index)
        {
          players[id].hero = &heroes[j];
          break;
        }
    }
  }
  for (int i = 0; i < heroes.getSize (); i++)
  {
    int pl = -1;
    int upl = -1;
    heroes[i].fixAbilities ();
    for (int j = 0; j < numPlayers; j++)
    {
      if (players[pindex[j]].hero == &heroes[i])
      {
        if (pl < 0 || heroes[i].actions[pl] < heroes[i].actions[j])
          pl = j;
        DotaHero* hero = players[pindex[j]].heroId ? getHeroById (players[pindex[j]].heroId) : NULL;
        if (hero && heroes[i].id == hero->index)
          upl = j;
      }
    }
    if (upl >= 0)
      pl = upl;
    for (int j = 0; j < numPlayers; j++)
      if (players[pindex[j]].hero == &heroes[i] && j != pl)
        players[pindex[j]].hero = NULL;
  }
  for (int i = 0; i < numPlayers; i++)
  {
    int id = pindex[i];
    if (players[id].slot.color > 11) continue; // observer
    if (players[id].slot.slot_status == 0)
      continue;
    int team = (players[id].slot.color > 6 ? 1 : 0);
    if (team)
    {
      int pos = dota.numScourge;
      while (pos > 0)
      {
        if (players[dota.scourge[pos - 1]].slot.color < players[id].slot.color)
          break;
        dota.scourge[pos] = dota.scourge[pos - 1];
        pos--;
      }
      dota.numScourge++;
      dota.scourge[pos] = id;
      dota.sentinelKills += players[id].stats[STAT_DEATHS];
      if (players[id].saidFF)
        dota.numFFScourge++;
    }
    else
    {
      int pos = dota.numSentinel;
      while (pos > 0)
      {
        if (players[dota.sentinel[pos - 1]].slot.color < players[id].slot.color)
          break;
        dota.sentinel[pos] = dota.sentinel[pos - 1];
        pos--;
      }
      dota.numSentinel++;
      dota.sentinel[pos] = id;
      dota.scourgeKills += players[id].stats[STAT_DEATHS];
      if (players[id].saidFF)
        dota.numFFSentinel++;
    }
    if (players[id].saidGG)
      dota.numGG++;

    players[id].acounter[ACTION_OTHER] = players[id].actions;
    for (int j = 0; j < ACTION_OTHER; j++)
      players[id].acounter[ACTION_OTHER] -= players[id].acounter[j];

    if (players[id].counted)
    {
      if ((players[id].avgClickX > 4000.0f && players[id].avgClickY < -5000.0f) ||
          (players[id].avgClickX > -1500.0f && players[id].avgClickY < -6000.0f) ||
          (players[id].avgClickX > 5500.0f && players[id].avgClickY < 1000.0f))
        players[id].lane = LANE_BOTTOM;
      else if ((players[id].avgClickX < -4500.0f && players[id].avgClickY > 3500.0f) ||
               (players[id].avgClickX < -5500.0f && players[id].avgClickY > -3000.0f) ||
               (players[id].avgClickX < 1000.0f && players[id].avgClickY > 4500.0f))
        players[id].lane = LANE_TOP;
      else
      {
        float csum = players[id].avgClickX + players[id].avgClickY;
        float cdif = players[id].avgClickX - players[id].avgClickY;
        if ((csum > -7000.0f && csum < 4000.0f && cdif > -1000.0f && cdif < 2000.0f) ||
            (csum > -2500.0f && csum < 1000.0f && cdif > -1500.0f && cdif < 2300.0f))
          players[id].lane = LANE_MIDDLE;
      }
    }
    else
      players[id].lane = LANE_AFK;

    for (int j = 0; j < players[id].inv.num_items; j++)
      players[id].itemCost += getItem (players[id].inv.items[j])->cost;
    players[id].itemCost += players[id].stats[STAT_GOLD];

    if (players[id].lCount <= 1 && players[id].hero)
    {
      int cLevel = 1;
      int pLevel = 1;
      for (int i = 0; i < players[id].nDeaths; i++)
      {
        while (cLevel < players[id].hero->level && players[id].hero->atime[cLevel] < players[id].deathTime[i])
          pLevel = cLevel++;
        players[id].deathEnd[i] = players[id].deathTime[i] + pLevel * 4000;
      }
    }

    unsigned long lastTime = 0;
    for (int j = 0; j < pactions[i].getSize (); j++)
    {
      if (pactions[i][j].type == ACTION_RIGHTCLICK)
      {
        pactions[i][j].inactive = pactions[i][j].time - lastTime;
        pmove[i].add (pactions[i][j]);
      }
      if (pactions[i][j].type == ACTION_RIGHTCLICK ||
          pactions[i][j].type == ACTION_ABILITY ||
          pactions[i][j].type == ACTION_ITEM ||
          pactions[i][j].type == ACTION_BASIC)
        lastTime = pactions[i][j].time;
    }
    int nextDeath = 0;
    for (int j = 0; j < pmove[i].getSize (); j++)
    {
      if (j == 0 || (nextDeath < players[id].nDeaths && players[id].deathTime[nextDeath] < pmove[i][j].time))
      {
        while (nextDeath < players[id].nDeaths && players[id].deathEnd[nextDeath] < pmove[i][j].time)
          nextDeath++;
        if (team == 0)
        {
          pmove[i][j].hx = -6864;
          pmove[i][j].hy = -6784;
        }
        else
        {
          pmove[i][j].hx = 6400;
          pmove[i][j].hy = 6096;
        }
      }
      //if (j == 0 || (pmove[i][j].time > 180000 && int (pmove[i][j].inactive) > deathTreshold * 1000))
      //{
      //}
      else
      {
        float trav = float (pmove[i][j].time - pmove[i][j - 1].time) * 0.5f;
        float dist = vec_len (pmove[i][j - 1].posx - pmove[i][j - 1].hx,
                              pmove[i][j - 1].posy - pmove[i][j - 1].hy);
        if (trav > dist) trav = 1;
        else if (trav > 1e-5) trav /= dist;
        pmove[i][j].hx = pmove[i][j - 1].hx + (pmove[i][j - 1].posx - pmove[i][j - 1].hx) * trav;
        pmove[i][j].hy = pmove[i][j - 1].hy + (pmove[i][j - 1].posy - pmove[i][j - 1].hy) * trav;
      }
    }
    if (players[id].lCount < 0 && players[id].hero)
      players[id].lCount = players[id].hero->level;

    if (!gotitems && players[id].hero)
    {
      players[id].inv.compute (time, dota);
      for (int i = 0; i < 6; i++)
        players[id].finalItems[i] = players[id].inv.inv[i];
    }
  }
  if (game.winner == 0)
  {
    int mxff = dota.numFFScourge;
    if (dota.numFFSentinel > mxff)
      mxff = dota.numFFSentinel;
    if (mxff > 2 && (mxff > 3 || dota.numGG > 4))
    {
      if (dota.numFFSentinel < 2) game.winner = WINNER_GSENTINEL;
      if (dota.numFFScourge < 2) game.winner = WINNER_GSCOURGE;
    }
  }
  if (game.winner == 0)
  {
    int inSentinel = 0;
    int inScourge = 0;
    for (int i = 0; i < dota.numSentinel; i++)
      if (pmove[players[dota.sentinel[i]].index].getSize () &&
          pmove[players[dota.sentinel[i]].index][pmove[players[dota.sentinel[i]].index].getSize () - 1].hx > 2200.0f &&
          pmove[players[dota.sentinel[i]].index][pmove[players[dota.sentinel[i]].index].getSize () - 1].hy > 2200.0f)
        inSentinel++;
    for (int i = 0; i < dota.numScourge; i++)
      if (pmove[players[dota.scourge[i]].index].getSize () &&
          pmove[players[dota.scourge[i]].index][pmove[players[dota.scourge[i]].index].getSize () - 1].hx < -2800.0f &&
          pmove[players[dota.scourge[i]].index][pmove[players[dota.scourge[i]].index].getSize () - 1].hy < -2800.0f)
        inScourge++;
    if (inSentinel >= 4 && inScourge <= 2)
      game.winner = WINNER_PSENTINEL;
    else if (inScourge >= 4 && inSentinel <= 2)
      game.winner = WINNER_PSCOURGE;
  }
}


__int64 getFileDate (char const* filename)
{
  CFileStatus status;
  if (!CFile::GetStatus (filename, status))
    return 0;
  return status.m_mtime.GetTime ();
}
char* fmtFileDate (char const* filename)
{
  CFileStatus status;
  if (!CFile::GetStatus (filename, status))
    return mprintf ("");
  return mprintf ("%s", (char const*) status.m_mtime.Format ("%d/%m/%Y %H:%M"));;
}
char* fmtFileSize (char const* filename)
{
  CFileStatus status;
  if (!CFile::GetStatus (filename, status))
    return mprintf ("");
  return mprintf ("%d KB", (status.m_size + 1023) / 1024);
}

CString copyReplay (char const* filename, char const* base, char const* dst, W3GReplay* rep)
{
  char path[1024];
  strcpy (path, base);
  int len = (int) strlen (path);
  CString cur = "";
  for (int i = 0; dst[i]; i++)
  {
    if (dst[i] == '\\')
    {
      if (cur == "..")
      {
        if (len > 1 && path[len - 1] == '\\' && path[len - 2] != ':')
        {
          len--;
          while (len > 0 && path[len - 1] != '\\')
            len--;
          path[len] = 0;
        }
      }
      else if (cur != "" && cur != ".")
      {
        strcat (path, cur + '\\');
        CreateDirectory (path, NULL);
      }
      cur = "";
    }
    else
      cur += dst[i];
  }
  if (cur.Find ("?") >= 0)
  {
    char buf[512];
    cur.Replace ("%", "%%");
    cur.Replace ("?", "%d");
    cur = "%s" + cur;
    for (int i = 1;; i++)
    {
      sprintf (buf, cur, path, i);
      if (!PathFileExists (buf))
        break;
    }
    strcpy (path, buf);
  }
  else
    strcat (path, cur);
  CopyFile (filename, path, FALSE);
  if (rep)
    addGame (rep, path);
  return path;
}

bool W3GReplay::getPlayerPos (int id, unsigned long time, float& x, float& y, float& dx, float& dy)
{
  if (time > players[id].time)
    return false;
  unsigned long lastDeath = 0;
  for (int i = 0; i < players[id].nDeaths; i++)
  {
    if (players[id].deathTime[i] <= time && players[id].deathEnd[i] > time)
      return false;
    if (players[id].deathEnd[i] <= time)
      lastDeath = players[id].deathEnd[i];
  }
  int oid = id;
  id = players[id].index;
  int l = 0;
  int r = pmove[id].getSize () - 1;
  if (r < 0) return false;
  if (time < pmove[id][0].time)
  {
    x = pmove[id][0].hx;
    y = pmove[id][0].hy;
    dx = x;
    dy = y;
    return true;
  }
  while (l < r)
  {
    int m = (l + r + 1) / 2;
    if (pmove[id][m].time > time)
      r = m - 1;
    else
      l = m;
  }
  //if (l < pmove[id].getSize () - 1 && time > 180000 && int (pmove[id][l + 1].inactive) > deathTreshold * 1000)
  //  return false;
  if (pmove[id][l].time < lastDeath)
  {
    if (players[oid].slot.team == 0)
    {
      x = -6864;
      y = -6784;
    }
    else
    {
      x = 6400;
      y = 6096;
    }
    dx = x;
    dy = y;
    return true;
  }
  float trav = float (time - pmove[id][l].time) * 0.5f;
  float dist = vec_len (pmove[id][l].posx - pmove[id][l].hx,
                        pmove[id][l].posy - pmove[id][l].hy);
  if (trav > dist) trav = 1;
  else if (trav > 1e-5) trav /= dist;
  x = pmove[id][l].hx + (pmove[id][l].posx - pmove[id][l].hx) * trav;
  y = pmove[id][l].hy + (pmove[id][l].posy - pmove[id][l].hy) * trav;
  dx = pmove[id][l].posx;
  dy = pmove[id][l].posy;
  return true;
}

int W3GReplay::getChatPos (unsigned long time)
{
  int l = 0;
  int r = chat.getSize () - 1;
  if (r < 0 || time < chat[0].time) return -1;
  while (l < r)
  {
    int m = (l + r + 1) / 2;
    if (chat[m].time > time)
      r = m - 1;
    else
      l = m;
  }
  return l;
}

int W3GReplay::getWardPos (unsigned long time)
{
  int l = 0;
  int r = wards.getSize () - 1;
  if (r < 0 || time < wards[0].time) return -1;
  while (l < r)
  {
    int m = (l + r + 1) / 2;
    if (wards[m].time > time)
      r = m - 1;
    else
      l = m;
  }
  return l;
}

W3GHero* W3GReplay::newHero (int side, int id)
{
  for (int i = 0; i < heroes.getSize (); i++)
  {
    if (heroes[i].id == id && heroes[i].side == side && !heroes[i].claimed)
    {
      heroes[i].claimed = true;
      return &heroes[i];
    }
  }
  int cur = heroes.add ();
  heroes[cur].claimed = true;
  heroes[cur].reset ();
  heroes[cur].side = side;
  heroes[cur].id = id;
  return &heroes[cur];
}
W3GHero* W3GReplay::getHero (unsigned long o1, unsigned long o2, int side, int id)
{
  for (int i = 0; i < heroes.getSize (); i++)
  {
    if (heroes[i].oid1 == 0 && heroes[i].oid2 == 0 && heroes[i].id == id && heroes[i].side == side)
    {
      heroes[i].oid1 = o1;
      heroes[i].oid2 = o2;
      return &heroes[i];
    }
    if (heroes[i].oid1 == o1 && heroes[i].oid2 == o2)
    {
      if (heroes[i].side == side)
        return &heroes[i];
      else
        return NULL;
    }
  }
  int cur = heroes.add ();
  heroes[cur].reset ();
  heroes[cur].oid1 = o1;
  heroes[cur].oid2 = o2;
  heroes[cur].side = side;
  heroes[cur].id = id;
  return &heroes[cur];
}

#define BLOCK_SIZE      8192
#define SAFE_SIZE       10000
void W3GReplay::saveas (FILE* file)
{
  static unsigned char cbuf[SAFE_SIZE];
  init_crc32 ();
  hdr.write_data (file);
  hdr.u_size = size;
  if (size & (BLOCK_SIZE - 1))
  {
    int rem = BLOCK_SIZE - (size & (BLOCK_SIZE - 1));
    addbytes (rem);
    memset (data + size, 0, rem);
    size += rem;
  }
  hdr.blocks = size / BLOCK_SIZE;
  hdr.c_size = 0;
  hdr.length = time;
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader bh;
    memset (&bh, 0, sizeof bh);
    bh.u_size = BLOCK_SIZE;
    unsigned long c_size = SAFE_SIZE;
    compress ((Bytef*) cbuf, (uLongf*) &c_size, (Bytef*) (data + (i * BLOCK_SIZE)), BLOCK_SIZE);
    bh.c_size = (short) c_size;
    bh.check1 = xor16 (crc32 ((unsigned char*) &bh, sizeof bh));
    bh.check2 = xor16 (crc32 (cbuf, bh.c_size));
    fwrite (&bh, sizeof bh, 1, file);
    fwrite (cbuf, bh.c_size, 1, file);
    hdr.c_size += bh.c_size;
  }
  hdr.checksum = 0;
  hdr.stream_data (cbuf);
  hdr.checksum = crc32 (cbuf, hdr.header_size);
  fseek (file, 0, SEEK_SET);
  hdr.write_data (file);
}

int complong (void const* arg1, void const* arg2);
int W3GReplay::cutpings (int len, bool obs)
{
  int end = pos + len;
  int cut = 0;
  while (pos < end)
  {
    unsigned char id = data[pos++];
    short* length = (short*) (data + pos);
    pos += 2;
    int next = pos + *length;
    if (!obs || players[id].slot.color > 11)
    {
      while (pos < next)
      {
        unsigned char action = data[pos++];
        switch (action)
        {
        // Pause game
        case 0x01:
          break;
        // Resume game
        case 0x02:
          break;
        // Set game speed in single player game (options menu)
        case 0x03:
          pos++;
          break;
        // Increase game speed in single player game (Num+)
        case 0x04:
          break;
        // Decrease game speed in single player game (Num-)
        case 0x05:
          break;
        // Save game
        case 0x06:
          while (data[pos++])
            ;
          break;
        // Save game finished
        case 0x07:
          pos += 4;
          break;
        // Unit/building ability (no additional parameters)
        case 0x10:
          if (hdr.major_v >= 13)
            pos += 14;
          else if (hdr.major_v >= 7)
            pos += 13;
          else
            pos += 5;
          break;
        // Unit/building ability (with target position)
        case 0x11:
          if (hdr.major_v >= 13)
            pos += 22;
          else if (hdr.major_v >= 7)
            pos += 21;
          else
            pos += 13;
          break;
        // Unit/building ability (with target position and target object ID)
        case 0x12:
          if (hdr.major_v >= 13)
            pos += 30;
          else if (hdr.major_v >= 7)
            pos += 29;
          else
            pos += 21;
          break;
        // Give item to Unit / Drop item on ground
        case 0x13:
          if (hdr.major_v >= 13)
            pos += 38;
          else if (hdr.major_v >= 7)
            pos += 37;
          else
            pos += 29;
          break;
        // Unit/building ability (with two target positions and two item IDs)
        case 0x14:
          if (hdr.major_v >= 13)
            pos += 43;
          else if (hdr.major_v >= 7)
            pos += 42;
          else
            pos += 34;
        // Change Selection (Unit, Building, Area)
        case 0x16:
        // Assign Group Hotkey
        case 0x17:
          {
            unsigned char mode = data[pos];
            unsigned short num = * (unsigned short*) (data + pos + 1);
            pos += 3 + num * 8;
          }
          break;
        // Select Group Hotkey
        case 0x18:
          pos += 2;
          break;
        // Select Subgroup
        case 0x19:
          if (hdr.build_v >= 6040 || hdr.major_v > 14)
            pos += 12;
          else
            pos += 1;
          break;
        // Pre Subselection
        // version < 14b: Only in scenarios, maybe a trigger-related command
        case 0x1A:
          if (hdr.build_v <= 6040 && hdr.major_v <= 14)
            pos += 9;
          break;
        // Only in scenarios, maybe a trigger-related command
        // version <= 14b: Select Ground Item
        case 0x1B:
          pos += 9;
          break;
        // Select Ground Item
        // version < 14b: Cancel hero revival (new in 1.13)
        case 0x1C:
          if (hdr.build_v > 6040 || hdr.major_v > 14)
            pos += 9;
          else
            pos += 8;
          break;
        // Cancel hero revival
        case 0x1D:
          if (hdr.build_v > 6040 || hdr.major_v > 14)
            pos += 8;
          else
            pos += 5;
          break;
        // Remove unit from building queue
        case 0x1E:
          pos += 5;
          break;
        // Found in replays with patch version 1.04 and 1.05.
        case 0x21:
          pos += 8;
          break;
        // Single Player Cheats
        case 0x20:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
          {
            switch (action)
            {
            case 0x27:
              pos += 5;
              break;
            case 0x28:
              pos += 5;
              break;
            case 0x2D:
              pos += 5;
              break;
            case 0x2E:
              pos += 4;
              break;
            }
          }
          break;
        // Change ally options
        case 0x50:
          pos += 5;
          break;
        // Transfer resources
        case 0x51:
          pos += 9;
          break;
        // Map trigger chat command (?)
        case 0x60:
          pos += 8;
          while (data[pos++])
            ;
          break;
        // ESC pressed
        case 0x61:
          break;
        // Scenario Trigger
        case 0x62:
          pos += 8;
          if (hdr.major_v >= 7)
            pos += 4;
          break;
        // Enter select hero skill submenu for WarCraft III patch version <= 1.06
        case 0x65:
          break;
        // Enter select hero skill submenu
        // Enter select building submenu for WarCraft III patch version <= 1.06
        case 0x66:
          break;
        // Enter select building submenu
        // Minimap signal (ping) for WarCraft III patch version <= 1.06
        case 0x67:
          if (hdr.major_v < 7)
          {
            pos--;
            next -= 13;
            end -= 13;
            size -= 13;
            if (size > pos)
              memmove (data + pos, data + pos + 13, size - pos);
            cut += 13;
            *length -= 13;
          }
          break;
        // Minimap signal (ping)
        // Continue Game (BlockB) for WarCraft III patch version <= 1.06
        case 0x68:
          if (hdr.major_v >= 7)
          {
            pos--;
            next -= 13;
            end -= 13;
            size -= 13;
            if (size > pos)
              memmove (data + pos, data + pos + 13, size - pos);
            cut += 13;
            *length -= 13;
          }
          else
            pos += 16;
          break;
        // Continue Game (BlockB)
        // Continue Game (BlockA) for WarCraft III patch version <= 1.06
        case 0x69:
        // Continue Game (BlockA)
        case 0x6A:
          pos += 16;
          break;
        // SyncStoredInteger actions (endgame action)
        case 0x6B:
          while (data[pos++])
            ;
          while (data[pos++])
            ;
          while (data[pos++])
            ;
          pos++;
          break;
        // Unknown (v6.39 and v6.39b endgame)
        case 0x70:
          while (data[pos++])
            ;
          while (data[pos++])
            ;
          while (data[pos++])
            ;
          break;
        // Only in scenarios, maybe a trigger-related command
        case 0x75:
          pos++;
          break;
        default:
          pos++;
        }
      }
    }
    pos = next;
  }
  pos = end;
  return cut;
}
void W3GReplay::cutchat (int* index, int isize, bool all, bool obs, bool ping, bool pingobs)
{
  pos = blockpos;
  if (size)
    qsort (index, isize, sizeof (int), complong);
  int count = 0;
  int cpos = 0;
  while (pos < size && (all || obs || cpos < isize))
  {
    int block_id = data[pos++];
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        short* length = (short*) (data + pos);
        pos += 4;
        if (ping || pingobs)
          *length -= cutpings (*length - 2, !ping);
        else
          pos += *length - 2;
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (hdr.major_v > 2)
      {
        short length = * (short*) (data + pos + 1);
        unsigned char flags = data[pos + 3];
        if (flags & 0x20)
        {
          unsigned long mode = * (unsigned long*) (data + pos + 4);
          if (all || (obs && mode == CHAT_OBSERVERS) || (cpos < isize && index[cpos] == count))
          {
            if (cpos < isize && index[cpos] == count)
              cpos++;
            pos--;
            length += 4;
            size -= length;
            if (size > pos)
              memmove (data + pos, data + pos + length, size - pos);
          }
          else
            pos += length + 3;
          count++;
        }
        else
          pos += length + 3;
        break;
      }
    // unknown (Random number/seed for next frame)
    case 0x22:
      pos += data[pos] + 1;
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      pos += 4;
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      pos += 10;
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      pos += 8;
      break;
    // LeaveGame
    case 0x17:
      pos += 13;
      break;
    case 0:
      pos = size;
      break;
    }
  }
}

void W3GReplay::setpath (char const* path)
{
  _splitpath (path, NULL, NULL, filename, NULL);
}
void W3GReplay::setLocation (char const* path)
{
  strcpy (filepath, path);
}
