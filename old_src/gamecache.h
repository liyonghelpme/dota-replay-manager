#ifndef __GAME_CACHE_H__
#define __GAME_CACHE_H__

class W3GReplay;

#define GC_NAME         0x00000004
#define GC_RATIO        0x00000010
#define GC_LENGTH       0x00000020
#define GC_MODE         0x00000040
#define GC_COUNT        0x00000080
#define GC_MAP          0x00000100
#define GC_PATCH        0x00000200
#define GC_NAMES        0x00000400
#define GC_HEROES       0x00000800
#define GC_STATS        0x00001000
#define GC_LEFT         0x00002000
#define GC_GOLD         0x00004000
#define GC_LEVEL        0x00008000
#define GC_LANE         0x00010000
#define GC_TEAM         0x00020000
#define GC_WIN          0x00040000
#define GC_APM          0x00080000
#define GC_FLAGS        0x000FFFF4

#define GC_BIGINT       0x7FFFFFFF

#define MODE_CONT       0
#define MODE_IS         1
#define MODE_IN         2

struct GameCache
{
  __int64 mod;
  int flags;
  int age;
  char path[512];
  wchar_t name[64];
  char ratio[16];
  unsigned long length;
  char mode[32];
  char count;
  int map;
  int patch;
  wchar_t pname[10][64];
  char phero[10];
  short pstats[10][5];
  unsigned long pleft[10];
  int pgold[10];
  char plane[10];
  char pteam[10];
  char plvl[10];
  short papm[10];
  char win;
  bool current ();
};
extern GameCache* gcache;
extern int gcsize;
extern int cacheAge;

class CProgressDlg;
void loadCache (CProgressDlg* progress, int min, int max);
void storeCache (CProgressDlg* progress, int min, int max);

int getGameInfo (char const* path, int flags);

void addGame (W3GReplay* w3g, char const* path);

int makeVersion (int major, int minor, int build);
int makeVersion (char const* ver, bool ib = false);
bool matchMode (char const* mode, char const* need, int m);

void setCacheDirectory (char const* dir);
char const* getCacheDirectory ();

#endif // __GAME_CACHE_H__
