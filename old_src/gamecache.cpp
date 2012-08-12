#include "stdafx.h"
#include "gamecache.h"
#include "registry.h"
#include "replay.h"
#include "resource.h"
#include "updatedlg.h"
#include "dotareplay.h"

#define CACHE_SIZE      8192
int gcMaxSize = 0;
GameCache* gcache = NULL;
int gcsize = 0;
int cacheAge = 0;

void addCacheSpace (int good)
{
  if (gcache == NULL || gcsize >= gcMaxSize)
  {
    gcMaxSize += CACHE_SIZE;
    GameCache* temp = new GameCache[gcMaxSize];
    memcpy (temp, gcache, sizeof (GameCache) * good);
    delete[] gcache;
    gcache = temp;
  }
}

static char ccbuf[512];
static char cacheDir[512];
static char cacheSub[512];
static int cacheDirLength = -1;

inline wchar_t readch (MPQFILE f)
{
  wchar_t a = (wchar_t) MPQFileGetc (f);
  wchar_t b = (wchar_t) MPQFileGetc (f);
  return a | (b << 8);
}
void readstr (MPQFILE f, wchar_t* str)
{
  unsigned char t = (unsigned char) MPQFileGetc (f);
  if (t == 1)
  {
    int pos = 0;
    while (str[pos++] = readch (f))
      ;
  }
  else
  {
    ccbuf[0] = t;
    if (t)
    {
      int pos = 1;
      while (ccbuf[pos++] = (char) MPQFileGetc (f))
        ;
    }
    wcscpy (str, makeucd (ccbuf));
  }
}
void writestr (MPQFILE f, wchar_t const* str)
{
  MPQFilePutc (f, 1);
  MPQFileWrite (f, (wcslen (str) + 1) * 2, str);
}

unsigned int lastRelevant = parseVersion ("2.05");
unsigned int doShifts = parseVersion ("1.05");
extern bool extractCache;

void loadCache (CProgressDlg* progress, int min, int max)
{
  addCacheSpace (0);
  MPQFILE file;
  if (extractCache)
  {
    file = MPQOpenFSys (mprintf ("%sgamecache", reg.getPath ()), MPQFILE_READ);
    if (file == 0)
    {
      MPQFILE sfile = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "gamecache.dat", MPQFILE_READ);
      if (sfile)
      {
        file = MPQOpenFSys (mprintf ("%sgamecache", reg.getPath ()), MPQFILE_REWRITE);
        if (file)
        {
          static char buf[1024];
          int count;
          while (count = MPQFileRead (sfile, sizeof buf, buf))
            MPQFileWrite (file, count, buf);
          MPQFileSeek (file, 0, MPQSEEK_SET);
        }
        MPQCloseFile (sfile);
        MPQDeleteFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "gamecache.dat");
      }
    }
  }
  else
  {
    file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "gamecache.dat", MPQFILE_READ);
    if (file == 0)
    {
      char path[512];
      sprintf (path, "%sgamecache", reg.getPath ());
      FILE* sfile = fopen (path, "rb");
      if (sfile)
      {
        file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "gamecache.dat", MPQFILE_REWRITE);
        if (file)
        {
          static char buf[1024];
          int count;
          while (count = fread (buf, 1, sizeof buf, sfile))
            MPQFileWrite (file, count, buf);
          MPQFileSeek (file, 0, MPQSEEK_SET);
        }
        fclose (sfile);
        DeleteFile (path);
      }
    }
  }
  if (file)
  {
    unsigned int ver = MPQReadInt (file);
    gcsize = MPQReadInt (file);
    addCacheSpace (0);
    cacheAge = MPQReadInt (file);
    if (ver < parseVersion ("0.90"))
      ver = (ver / 2) * 26 + (ver & 1);
    if (!MPQError () && ver >= lastRelevant)
    {
      for (int i = 0; i < gcsize; i++)
      {
        int npos = MPQFileTell (file);
        npos += MPQReadInt (file);
        MPQFileRead (file, 8, &gcache[i].mod);
        gcache[i].flags = MPQReadInt (file);
        gcache[i].age = MPQReadInt (file);
        MPQReadString (file, gcache[i].path);
        if (gcache[i].flags & GC_NAME)
          readstr (file, gcache[i].name);
        if (gcache[i].flags & GC_RATIO)
          MPQReadString (file, gcache[i].ratio);
        if (gcache[i].flags & GC_LENGTH)
          gcache[i].length = MPQReadInt (file);
        if (gcache[i].flags & GC_MODE)
          MPQReadString (file, gcache[i].mode);
        if (gcache[i].flags & GC_COUNT)
          MPQFileRead (file, 1, &gcache[i].count);
        if (gcache[i].flags & GC_MAP)
          gcache[i].map = MPQReadInt (file);
        if (gcache[i].flags & GC_PATCH)
          gcache[i].patch = MPQReadInt (file);
        if (gcache[i].flags & GC_NAMES)
          for (int j = 0; j < gcache[i].count; j++)
            readstr (file, gcache[i].pname[j]);
        if (gcache[i].flags & GC_HEROES)
          for (int j = 0; j < gcache[i].count; j++)
            MPQFileRead (file, 1, &gcache[i].phero[j]);
        if (gcache[i].flags & GC_STATS)
          for (int j = 0; j < gcache[i].count; j++)
            for (int k = 0; k < 5; k++)
              MPQFileRead (file, 2, &gcache[i].pstats[j][k]);
        if (gcache[i].flags & GC_LEFT)
          for (int j = 0; j < gcache[i].count; j++)
            gcache[i].pleft[j] = MPQReadInt (file);
        if (gcache[i].flags & GC_GOLD)
          for (int j = 0; j < gcache[i].count; j++)
            gcache[i].pgold[j] = MPQReadInt (file);
        if (gcache[i].flags & GC_LEVEL)
          for (int j = 0; j < gcache[i].count; j++)
            MPQFileRead (file, 1, &gcache[i].plvl[j]);
        if (gcache[i].flags & GC_LANE)
          for (int j = 0; j < gcache[i].count; j++)
            MPQFileRead (file, 1, &gcache[i].plane[j]);
        if (gcache[i].flags & GC_TEAM)
          for (int j = 0; j < gcache[i].count; j++)
            MPQFileRead (file, 1, &gcache[i].pteam[j]);
        if (gcache[i].flags & GC_APM)
          for (int j = 0; j < gcache[i].count; j++)
            MPQFileRead (file, 2, &gcache[i].papm[j]);
        if (gcache[i].flags & GC_WIN)
          MPQFileRead (file, 1, &gcache[i].win);
        if (ver >= doShifts)
          MPQFileSeek (file, npos, MPQSEEK_SET);
        if ((i & 63) == 0)
          progress->SetProgress (min + (max - min) * i / gcsize);
      }
    }
    else
    {
      gcsize = 0;
      cacheAge = 0;
    }
    MPQCloseFile (file);
  }
}

void storeCache (CProgressDlg* progress, int min, int max)
{
  MPQFILE file;
  if (extractCache)
    file = MPQOpenFSys (mprintf ("%sgamecache", reg.getPath ()), MPQFILE_REWRITE);
  else
    file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "gamecache.dat", MPQFILE_REWRITE);
  if (file)
  {
    MPQWriteInt (file, curVersion);
    MPQWriteInt (file, gcsize);
    MPQWriteInt (file, cacheAge);
    for (int i = 0; i < gcsize; i++)
    {
      int pos = MPQFileTell (file);
      MPQWriteInt (file, 0);
      MPQFileWrite (file, 8, &gcache[i].mod);
      MPQWriteInt (file, gcache[i].flags);
      MPQWriteInt (file, gcache[i].age);
      MPQWriteString (file, gcache[i].path);
      if (gcache[i].flags & GC_NAME)
        writestr (file, gcache[i].name);
      if (gcache[i].flags & GC_RATIO)
        MPQWriteString (file, gcache[i].ratio);
      if (gcache[i].flags & GC_LENGTH)
        MPQWriteInt (file, gcache[i].length);
      if (gcache[i].flags & GC_MODE)
        MPQWriteString (file, gcache[i].mode);
      if (gcache[i].flags & GC_COUNT)
        MPQFileWrite (file, 1, &gcache[i].count);
      if (gcache[i].flags & GC_MAP)
        MPQWriteInt (file, gcache[i].map);
      if (gcache[i].flags & GC_PATCH)
        MPQWriteInt (file, gcache[i].patch);
      if (gcache[i].flags & GC_NAMES)
        for (int j = 0; j < gcache[i].count; j++)
          writestr (file, gcache[i].pname[j]);
      if (gcache[i].flags & GC_HEROES)
        for (int j = 0; j < gcache[i].count; j++)
          MPQFileWrite (file, 1, &gcache[i].phero[j]);
      if (gcache[i].flags & GC_STATS)
        for (int j = 0; j < gcache[i].count; j++)
          for (int k = 0; k < 5; k++)
            MPQFileWrite (file, 2, &gcache[i].pstats[j][k]);
      if (gcache[i].flags & GC_LEFT)
        for (int j = 0; j < gcache[i].count; j++)
          MPQWriteInt (file, gcache[i].pleft[j]);
      if (gcache[i].flags & GC_GOLD)
        for (int j = 0; j < gcache[i].count; j++)
          MPQWriteInt (file, gcache[i].pgold[j]);
      if (gcache[i].flags & GC_LEVEL)
        for (int j = 0; j < gcache[i].count; j++)
          MPQFileWrite (file, 1, &gcache[i].plvl[j]);
      if (gcache[i].flags & GC_LANE)
        for (int j = 0; j < gcache[i].count; j++)
          MPQFileWrite (file, 1, &gcache[i].plane[j]);
      if (gcache[i].flags & GC_TEAM)
        for (int j = 0; j < gcache[i].count; j++)
          MPQFileWrite (file, 1, &gcache[i].pteam[j]);
      if (gcache[i].flags & GC_APM)
        for (int j = 0; j < gcache[i].count; j++)
          MPQFileWrite (file, 2, &gcache[i].papm[j]);
      if (gcache[i].flags & GC_WIN)
        MPQFileWrite (file, 1, &gcache[i].win);
      int ssize = MPQFileTell (file) - pos;
      MPQFileSeek (file, pos, MPQSEEK_SET);
      MPQWriteInt (file, ssize);
      MPQFileSeek (file, 0, MPQSEEK_END);
      if ((i & 63) == 0)
        progress->SetProgress (min + (max - min) * i / gcsize);
    }
    MPQCloseFile (file);
  }
  delete[] gcache;
}

static W3GReplay w3g;

void parseW3G (int cpos, W3GReplay* w3g)
{
  gcache[cpos].flags = GC_FLAGS;
  wcscpy (gcache[cpos].name, w3g->game.uname);
  if (w3g->dota.isDota)
    strcpy (gcache[cpos].ratio, mprintf ("%dv%d", w3g->dota.numScourge, w3g->dota.numScourge));
  else
    gcache[cpos].ratio[0] = 0;
  gcache[cpos].length = w3g->time;
  strcpy (gcache[cpos].mode, w3g->game.game_mode);
  if (w3g->dota.isDota)
    gcache[cpos].map = makeVersion (w3g->dota.major, w3g->dota.minor, w3g->dota.build);
  else
    gcache[cpos].map = 0;
  gcache[cpos].patch = getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v);
  gcache[cpos].count = 0;
  gcache[cpos].win = w3g->game.winner;
  bool nzStats = false;
  for (int i = 0; i < w3g->numPlayers; i++)
  {
    int clr = w3g->players[w3g->pindex[i]].slot.color;
    if (clr >= 1 && clr <= 11 && clr != 6)
    {
      for (int j = 0; j < 5; j++)
        if (w3g->players[w3g->pindex[i]].stats[j] != 0)
          nzStats = true;
      wcscpy (gcache[cpos].pname[gcache[cpos].count], w3g->players[w3g->pindex[i]].uname);
      if (w3g->players[w3g->pindex[i]].hero)
      {
        DotaHero* hero = getHero (w3g->players[w3g->pindex[i]].hero->id);
        gcache[cpos].phero[gcache[cpos].count] = (hero ? hero->point : 0);
        gcache[cpos].plvl[gcache[cpos].count] = w3g->players[w3g->pindex[i]].hero->level;
      }
      else
      {
        gcache[cpos].phero[gcache[cpos].count] = 0;
        gcache[cpos].plvl[gcache[cpos].count] = 0;
      }
      for (int j = 0; j < 5; j++)
        gcache[cpos].pstats[gcache[cpos].count][j] = w3g->players[w3g->pindex[i]].stats[j];
      gcache[cpos].pleft[gcache[cpos].count] = w3g->players[w3g->pindex[i]].time;
      gcache[cpos].pgold[gcache[cpos].count] = w3g->players[w3g->pindex[i]].itemCost;
      gcache[cpos].plane[gcache[cpos].count] = w3g->players[w3g->pindex[i]].lane;
      gcache[cpos].pteam[gcache[cpos].count] = w3g->players[w3g->pindex[i]].slot.team;
      if (w3g->players[w3g->pindex[i]].time)
        gcache[cpos].papm[gcache[cpos].count] = short (w3g->players[w3g->pindex[i]].actions * 60000 /
          w3g->players[w3g->pindex[i]].time);
      else
        gcache[cpos].papm[gcache[cpos].count] = 0;
      gcache[cpos].count++;
    }
  }
  if (!nzStats)
  {
    for (int i = 0; i < gcache[cpos].count; i++)
      for (int j = 0; j < 5; j++)
        gcache[cpos].pstats[i][j] = -1;
  }
}

int getGameInfo (char const* path, int flags)
{
  __int64 gtime = getFileDate (path);
  int cpos = -1;
  int mpos = 0;
  for (int i = 0; i < gcsize && cpos < 0; i++)
  {
    if (gcache[i].mod == gtime)
      cpos = i;
    //if (gcache[i].age < gcache[mpos].age)
    //  mpos = i;
  }
  if (cpos < 0)
  {
    //if (gcsize >= sizeof gcache / sizeof gcache[0])
    //  cpos = mpos;
    //else
    addCacheSpace (gcsize);
    cpos = gcsize++;
    memset (&gcache[cpos], 0, sizeof gcache[cpos]);
    strcpy (gcache[cpos].path, path);
    gcache[cpos].mod = gtime;
    FILE* file = fopen (path, "rb");
    if (file)
    {
      w3g.clear ();
      if (w3g.load (file, true))
        parseW3G (cpos, &w3g);
      fclose (file);
    }
  }
  else if ((gcache[cpos].flags & flags) != flags)
  {
    memset (&gcache[cpos], 0, sizeof gcache[cpos]);
    strcpy (gcache[cpos].path, path);
    gcache[cpos].mod = gtime;
    FILE* file = fopen (path, "rb");
    if (file)
    {
      w3g.clear ();
      if (w3g.load (file, true))
        parseW3G (cpos, &w3g);
      fclose (file);
    }
  }
  else
    strcpy (gcache[cpos].path, path);
  gcache[cpos].age = cacheAge++;
  return cpos;
}

void addGame (W3GReplay* w3g, char const* path)
{
  __int64 gtime = getFileDate (path);
  int cpos = -1;
  int mpos = 0;
  for (int i = 0; i < gcsize && cpos < 0; i++)
  {
    if (gcache[i].mod == gtime)
      cpos = i;
  }
  if (cpos < 0)
  {
    addCacheSpace (gcsize);
    cpos = gcsize++;
  }
  memset (&gcache[cpos], 0, sizeof gcache[cpos]);
  strcpy (gcache[cpos].path, path);
  gcache[cpos].mod = gtime;
  parseW3G (cpos, w3g);
  gcache[cpos].age = cacheAge++;
}

int makeVersion (int major, int minor, int build)
{
  return (major * 100 + minor) * 26 + build;
}

int makeVersion (char const* ver, bool ib)
{
  if (ver[0] < '1' || ver[0] > '9' || ver[1] != '.' || ver[2] < '0' || ver[2] > '9' ||
    ver[3] < '0' || ver[3] > '9' || (ver[4] != 0 && (ver[4] < 'a' || ver[4] > 'z' || ver[5] != 0)))
    return 0;
  int vid = ((ver[0] - '0') * 100 + (ver[2] - '0') * 10 + ver[3] - '0') * 26;
  if (ver[4] && !ib)
    vid += ver[4] - 'a';
  return vid;
}

bool isHere (char const* mode, char* need)
{
  for (int i = 0; (need[i] >= 'a' && need[i] <= 'z') || (need[i] >= 'A' && need[i] <= 'Z');
    i += 2)
  {
    if (!strnicmp (mode, need + i, 2))
    {
      need[i] = 'q';
      need[i + 1] = 'q';
      return true;
    }
  }
  return false;
}

bool matchMode (char const* mode, char const* need, int m)
{
  char nd[64];
  if (need[0] == '-')
    need++;
  strcpy (nd, need);
  if (!stricmp (mode, "Normal mode") || !stricmp (mode, "-wtf"))
  {
    if (m == MODE_CONT || m == MODE_IS)
      return need[0] == 0;
    else
      return true;
  }
  mode++;
  for (int i = 0; mode[i] >= 'a' && mode[i] <= 'z'; i += 2)
  {
    bool r = isHere (mode + i, nd);
    if ((m == MODE_IS || m == MODE_IN) && !r)
      return false;
  }
  if (m == MODE_CONT || m == MODE_IS)
    for (int i = 0; (nd[i] >= 'a' && nd[i] <= 'z') || (nd[i] >= 'A' && nd[i] <= 'Z'); i += 2)
      if (nd[i] != 'q')
        return false;
  return true;
}

extern char replayPath[];
void unslash (char*);
void setCacheDirectory (char const* dir)
{
  strcpy (cacheSub, dir);
  sprintf (cacheDir, "%s%s", replayPath, dir);
  unslash (cacheDir);
  cacheDirLength = (int) strlen (cacheDir);
}
char const* getCacheDirectory ()
{
  return cacheSub;
}

bool GameCache::current ()
{
  if (cacheDirLength < 0)
    setCacheDirectory ("");
  return !strnicmp (path, cacheDir, cacheDirLength);
}
