#ifndef __REPLAY_H__
#define __REPLAY_H__

#include "utils.h"
#include "rmpq.h"

template<class T>
class Array
{
  T* buf;
  int size;
  int mxsize;
public:
  Array ()
  {
    buf = NULL;
    size = 0;
  }
  ~Array ()
  {
    delete[] buf;
  }
  int add (T const& item)
  {
    if (buf == NULL)
    {
      mxsize = 64;
      buf = new T[mxsize]; 
    }
    else if (mxsize < size + 1)
    {
      while (mxsize < size + 1)
        mxsize *= 2;
      T* tmp = new T[mxsize];
      for (int i = 0; i < size; i++)
        tmp[i] = buf[i];
      delete[] buf;
      buf = tmp;
    }
    buf[size] = item;
    return size++;
  }
  int add ()
  {
    if (buf == NULL)
    {
      mxsize = 64;
      buf = new T[mxsize]; 
    }
    else if (mxsize < size + 1)
    {
      while (mxsize < size + 1)
        mxsize *= 2;
      T* tmp = new T[mxsize];
      for (int i = 0; i < size; i++)
        tmp[i] = buf[i];
      delete[] buf;
      buf = tmp;
    }
    return size++;
  }
  void expand (int len)
  {
    if (buf == NULL)
    {
      mxsize = 64;
      buf = new T[mxsize]; 
    }
    if (mxsize < size + len)
    {
      while (mxsize < size + len)
        mxsize *= 2;
      T* tmp = new T[mxsize];
      for (int i = 0; i < size; i++)
        tmp[i] = buf[i];
      delete[] buf;
      buf = tmp;
    }
  }
  void set (int i, T const& item)
  {
    if (i >= size)
    {
      expand (i - size + 1);
      size = i + 1;
    }
    buf[i] = item;
  }
  void append (T const* items, int cnt)
  {
    expand (cnt);
    for (int i = 0; i < cnt; i++)
      buf[size + i] = items[i];
    size += cnt;
  }
  void del (int pos)
  {
    if (size - pos > 1)
      memmove (buf + pos, buf + pos + 1, sizeof (T) * (size - pos - 1));
    size--;
  }
  int getSize () const
  {
    return size;
  }
  int& getSize ()
  {
    return size;
  }
  T& operator [] (int i)
  {
    return buf[i];
  }
  T const& operator [] (int i) const
  {
    return buf[i];
  }

  T* ptr ()
  {
    return buf;
  }
  T const* ptr () const
  {
    return buf;
  }

  void clear ()
  {
    size = 0;
  }
};

template<class T>
class SArray
{
  enum {lSize = 16};
  struct List
  {
    T ptr[lSize];
    List* next;
    int start;
    List (int p)
    {
      start = p;
      next = NULL;
    }
    ~List ()
    {
      delete next;
    }
  };
  List* head;
  List* tail;
  int size;
public:
  SArray ()
  {
    head = NULL;
    tail = NULL;
    size = 0;
  }
  ~SArray ()
  {
    delete head;
  }
  int add (T const& item)
  {
    if (head == NULL)
    {
      head = new List (0);
      tail = head;
    }
    while (size + 1 > tail->start + lSize)
    {
      tail->next = new List (tail->start + lSize);
      tail = tail->next;
    }
    set (size++, item);
    return size - 1;
  }
  int add ()
  {
    if (head == NULL)
    {
      head = new List (0);
      tail = head;
    }
    while (size + 1 > tail->start + lSize)
    {
      tail->next = new List (tail->start + lSize);
      tail = tail->next;
    }
    return size++;
  }
  void expand (int len)
  {
    if (head == NULL)
    {
      head = new List (0);
      tail = head;
    }
    while (size + len > tail->start + lSize)
    {
      tail->next = new List (tail->start + lSize);
      tail = tail->next;
    }
  }
  void set (int i, T const& item)
  {
    if (i >= size)
    {
      expand (i - size + 1);
      size = i + 1;
    }
    List* cur = head;
    while (i >= lSize)
    {
      cur = cur->next;
      i -= lSize;
    }
    cur->ptr[i] = item;
  }
  void append (T const* items, int cnt)
  {
    expand (cnt);
    int pos = size;
    List* cur = head;
    for (int i = 0; i < cnt; i++)
    {
      while (pos >= cur->start + lSize)
        cur = cur->next;
      cur->ptr[pos - cur->start] = items[i];
    }
    size += cnt;
  }
  int getSize () const
  {
    return size;
  }
  T& operator [] (int i)
  {
    List* cur = head;
    while (i >= lSize)
    {
      cur = cur->next;
      i -= lSize;
    }
    return cur->ptr[i];
  }
  T const& operator [] (int i) const
  {
    List* cur = head;
    while (i >= lSize)
    {
      cur = cur->next;
      i -= lSize;
    }
    return cur->ptr[i];
  }

  void clear ()
  {
    size = 0;
  }
};

struct gzmemory
{
  int count;
  int pos;
  char* buf;
  int size;
  void reset ();
  void* alloc (int block);
  void free (void* ptr);
  gzmemory ()
  {
    buf = NULL;
    size = 0;
    count = 65536;
  }
  ~gzmemory ()
  {
    if (buf)
      ::free (buf);
  }
};
bool gzinflate (char* old, char* buf, int csize, int usize, gzmemory* mem = NULL);

#include "dota.h"

#define RACE_HUMAN            1
#define RACE_ORC              2
#define RACE_NIGHTELF         3
#define RACE_UNDEAD           4
#define RACE_RANDOM           5

static const char raceImage[][32] = {"Empty", "human", "orc", "nightelf", "undead", "random"};

#define SPEED_SLOW            0
#define SPEED_NORMAL          1
#define SPEED_FAST            2

#define VISIBILITY_HIDE       0
#define VISIBILITY_EXPLORED   1
#define VISIBILITY_VISIBLE    2
#define VISIBILITY_DEFAULT    3

#define OBSERVERS_NONE        0
#define OBSERVERS_DEFEAT      2
#define OBSERVERS_FULL        3
#define OBSERVERS_REFEREES    4

#define GAME_LADDER           0x01
#define GAME_CUSTOM           0x09
#define GAME_SINGLE           0x0D
#define GAME_LADDERTEAM       0x20

#define COLOR_RED             0
#define COLOR_BLUE            1
#define COLOR_TEAL            2
#define COLOR_PURPLE          3
#define COLOR_YELLOW          4
#define COLOR_ORANGE          5
#define COLOR_GREEN           6
#define COLOR_PINK            7
#define COLOR_GRAY            8
#define COLOR_LIGHTBLUE       9
#define COLOR_DARKGREEN       10
#define COLOR_BROWN           11
#define COLOR_OBSERVER        12

#define AI_EASY               0
#define AI_NORMAL             1
#define AI_INSANE             2

#define SELECT_TEAMRACE       0x00
#define SELECT_RACE           0x01
#define SELECT_NONE           0x03
#define SELECT_TEAM           0x04
#define SELECT_LADDER         0xCC

#define CHAT_ALL              0x00
#define CHAT_ALLIES           0x01
#define CHAT_OBSERVERS        0x02
#define CHAT_PRIVATE          0x03
#define CHAT_COMMAND          0x7C
#define CHAT_ASCOMMAND        0x80
#define CHAT_PING             0x7E
#define CHAT_NOTIFY           0x7F
#define CHAT_NONE             0x7D

#define CHAT_NOTIFY_LEAVER    1
#define CHAT_NOTIFY_PAUSE     2
#define CHAT_NOTIFY_CONTROL   3
#define CHAT_NOTIFY_KILL      4
#define CHAT_NOTIFY_TOWER     5
#define CHAT_NOTIFY_BARRACKS  6
#define CHAT_NOTIFY_COURIER   7
#define CHAT_NOTIFY_TREE      8
#define CHAT_NOTIFY_ROSHAN    9
#define CHAT_NOTIFY_AEGIS     10
#define CHAT_NOTIFY_GAMEMODE  11
#define CHAT_NOTIFY_RUNE      12
#define CHAT_NOTIFY_PICKS     13
#define CHAT_NOTIFY_FASTKILL  14
#define CHAT_NOTIFY_SPREE     15

#define ITEM_NONE             0
#define ITEM_HERO             1
#define ITEM_ABILITY          2
#define ITEM_ITEM             3

#define ACTION_RIGHTCLICK     0
#define ACTION_SELECT         1
#define ACTION_SELECTHOTKEY   2
#define ACTION_ASSIGNHOTKEY   3
#define ACTION_ABILITY        4
#define ACTION_BASIC          5
#define ACTION_SUBGROUP       6
#define ACTION_ITEM           7
#define ACTION_ESC            8
#define ACTION_OTHER          9
#define NUM_ACTIONS           10

extern const char actionNames[NUM_ACTIONS][256];

#define END_TIME              0x7FFFFFFF

#define FLAGS_QUEUE           0x0001
#define FLAGS_TOALL           0x0002
#define FLAGS_AOE             0x0004
#define FLAGS_GROUP           0x0008
#define FLAGS_NOFORMATION     0x0010
#define FLAGS_SUBGROUP        0x0040
#define FLAGS_AUTOCAST        0x0100

#define ID_RIGHTCLICK         0x000D0003
#define ID_STOP               0x000D0004
#define ID_CANCEL             0x000D0008
#define ID_RALLYPOINT         0x000D000C
#define ID_ATTACK             0x000D000F
#define ID_ATTACKGROUND       0x000D0010
#define ID_MOVE               0x000D0012
#define ID_PATROL             0x000D0016
#define ID_HOLD               0x000D0019
#define ID_GIVE               0x000D0021
#define ID_SWAPITEM1          0x000D0022
#define ID_SWAPITEM2          0x000D0023
#define ID_SWAPITEM3          0x000D0024
#define ID_SWAPITEM4          0x000D0025
#define ID_SWAPITEM5          0x000D0026
#define ID_SWAPITEM6          0x000D0027
#define ID_USEITEM1           0x000D0028
#define ID_USEITEM2           0x000D0029
#define ID_USEITEM3           0x000D002A
#define ID_USEITEM4           0x000D002B
#define ID_USEITEM5           0x000D002C
#define ID_USEITEM6           0x000D002D

#define STAT_KILLS            0
#define STAT_DEATHS           1
#define STAT_CREEPS           2
#define STAT_DENIES           3
#define STAT_ASSISTS          4
#define STAT_GOLD             5
#define STAT_NEUTRALS         6

#define WINNER_UNKNOWN        0
#define WINNER_SENTINEL       1
#define WINNER_SCOURGE        2
#define WINNER_GSENTINEL      -1
#define WINNER_GSCOURGE       -2
#define WINNER_PSENTINEL      -3
#define WINNER_PSCOURGE       -4

#define LANE_ROAMING          0
#define LANE_TOP              1
#define LANE_MIDDLE           2
#define LANE_BOTTOM           3
#define LANE_AFK              4

#define MODE_AP         0x00000001
#define MODE_AR         0x00000002
#define MODE_LM         0x00000004
#define MODE_MM         0x00000008
#define MODE_TR         0x00000010
#define MODE_DM         0x00000020
#define MODE_MR         0x00000040
#define MODE_SP         0x00000080
#define MODE_AA         0x00000100
#define MODE_AI         0x00000200

#define MODE_AS         0x00000400
#define MODE_ID         0x00000800
#define MODE_NP         0x00001000
#define MODE_SC         0x00002000
#define MODE_EM         0x00004000
#define MODE_DU         0x00008000
#define MODE_SH         0x00010000
#define MODE_VR         0x00020000
#define MODE_RV         0x00040000
#define MODE_RD         0x00080000

#define MODE_OM         0x00100000
#define MODE_XL         0x00200000
#define MODE_NM         0x00400000
#define MODE_NT         0x00800000
#define MODE_NB         0x01000000
#define MODE_NS         0x02000000
#define MODE_NR         0x04000000
#define MODE_TS         0x08000000
#define MODE_SD         0x10000000
#define MODE_CD         0x20000000

#define MODE_PM         0x40000000
#define MODE_OI         0x80000000
#define MODE_MI         0x10000000
#define MODE_CM       0x0200000000LL
#define MODE_FR       0x0400000000LL
#define MODE_MO       0x0800000000LL
#define MODE_RO       0x1000000000LL
#define MODE_ER       0x2000000000LL
#define MODE_RS       0x4000000000LL
#define MODE_SO       0x8000000000LL

#define MODE_WTF    0x800000000000LL

inline bool isValidPos (float x)
{
  return (* (unsigned long*) &x) != 0xFFFFFFFF;
}

struct W3GHeader
{
  char intro[28];
  long header_size;
  long c_size;
  long header_v;
  long u_size;
  long blocks;
  long ident;
  long major_v;
  short minor_v;
  short build_v;
  short flags;
  long length;
  long checksum;

  W3GHeader ()
  {
    memset (this, 0, sizeof (W3GHeader));
  }
  bool read_data (FILE* f);
  void write_data (FILE* f);
  void stream_data (unsigned char* x);
};
struct W3GBlockHeader
{
  short c_size;
  short u_size;
  unsigned short check1;
  unsigned short check2;
  W3GBlockHeader ()
  {
    memset (this, 0, sizeof (W3GBlockHeader));
  }
};
struct W3GSlot
{
  unsigned char slot_status;
  unsigned char computer;
  unsigned char team;
  unsigned char color;
  unsigned char race;
  unsigned char ai_strength;
  unsigned char handicap;

  unsigned char org_color;

  W3GSlot ()
  {
    memset (this, 0, sizeof (W3GSlot));
  }
};
struct ASItem
{
  int id;
  unsigned long time;
  bool gone;
  bool pregone;
  bool tpregone;
  ASItem () {}
  ASItem (int i, unsigned long t)
  {
    id = i;
    time = t;
    gone = false;
    pregone = false;
    tpregone = false;
  }
};
struct W3GSkill
{
  int id;
  int slot;
  unsigned long time;
  int pos;
};
struct DotaData;
struct W3GHero
{
  int id;
  unsigned long oid1, oid2;
  int level;
  int abilities[32];
  unsigned long atime[32];
  W3GSkill learned[64];
  int nLearned;
  int side;
  bool claimed;

  int cur_lvl;
  int levels[5];

  int actions[16];

  W3GHero ()
  {
    reset ();
  }
  void reset ()
  {
    oid1 = 0;
    oid2 = 0;
    level = 0;
    cur_lvl = 0;
    nLearned = 0;
    claimed = false;
    memset (actions, 0, sizeof actions);
    memset (abilities, 0, sizeof abilities);
  }

  void pushAbility (int id, unsigned long time, int slot);
  void fixAbilities ();
  bool fixFrom (int pos);
  void undoFrom (int pos);
  bool compute (unsigned long time, DotaData const& dota);
};
struct W3GInventory
{
  int items[256];
  unsigned long itemt[256];
  int num_items;
  Array<ASItem> bi;

  int inv[6];

  W3GInventory ()
  {
    reset ();
  }
  ~W3GInventory ()
  {
  }
  void reset ()
  {
    num_items = 0;
    bi.clear ();
  }

  void getASItems (unsigned long time, DotaData const& dota);
  void getAllItems (unsigned long time, DotaData const& dota);
  void listItems (unsigned long time);
  void sortItems ();
  bool compute (unsigned long time, DotaData const& dota);
};
struct W3GId
{
  unsigned long id1;
  unsigned long id2;
};
struct W3GPlayer
{
  unsigned char player_id;
  bool initiator;
  char name[256];
  char orgname[256];
  wchar_t uname[256];
  long exe_runtime;
  long race;
  int actions;
  W3GSlot slot;
  W3GHero* curSel;
  int numWards;
  int heroId;

  unsigned long deathTime[128];
  unsigned long deathEnd[128];
  int nDeaths;
  int curLevel;

  int hkassign[16];
  int hkuse[16];

  int pkilled[256];
  int pdied[256];
  int sdied;
  int skilled;

  bool saidFF;
  bool saidGG;

  int stats[16];
  unsigned long ltime[32];
  int lCount;

  unsigned long time;
  long leave_reason;
  long leave_result;
  bool left;

  bool share[16];

  int acounter[NUM_ACTIONS];
  W3GHero* hero;

  W3GId sel[16];
  int numSel;
  int curTab;

  int lane;
  int counted;
  float avgClickX;
  float avgClickY;

  int itemCost;

  int index;

  int afterLeave;
  int finalItems[6];
  W3GInventory inv;

  W3GPlayer ()
  {
    memset (this, 0, ((char*) &inv) - ((char*) this));
    lCount = 1;
  }
};
struct W3GMessage
{
  unsigned char id;
  unsigned long mode;
  unsigned long notifyType;
  union
  {
    char text[1024];
    struct
    {
      float x;
      float y;
    };
  };
  wchar_t utext[1024];
  unsigned long time;
  int index;
  int line;

  W3GMessage ()
  {
    memset (this, 0, sizeof (W3GMessage));
    index = -1;
    line = -1;
  }
};
struct DraftData
{
  int pool[64];
  int numPool;
  int bans[2][64];
  int numBans[2];
  int picks[2][64];
  int numPicks[2];
  int firstPick;
};

struct W3GGame
{
  char name[256];
  wchar_t uname[256];
  unsigned char speed;
  unsigned char visibility;
  unsigned char observers;
  bool teams_together;
  bool lock_teams;
  bool shared_control;
  bool random_hero;
  bool random_races;
  char creator[256];
  wchar_t ucreator[256];
  char map[256];
  wchar_t umap[256];
  long slots;
  unsigned char game_type;
  bool game_private;
  unsigned char record_id;
  short record_length;
  unsigned char slot_records;
  unsigned long startTime;

  DraftData draft;

  unsigned char saver_id;

  int winner;
  int lwinner;
  int wplayer;
  bool llost[16];

  long random_seed;
  unsigned char select_mode;
  unsigned char start_spots;

  bool hasObservers;

  int pSwitching;
  int pSwitchingWith;
  unsigned long switchStart;

  char game_mode[256];
  __int64 gmode;

  W3GGame ()
  {
    memset (this, 0, sizeof (W3GGame));
    lwinner = -1;
  }
};

struct W3GItem
{
  int type;
  char name[256];
  int id;
};
void convert_itemid (W3GItem& item, int id);

#define EVENT_HEROKILL      1
#define EVENT_TOWERKILL     2
#define EVENT_RAXKILL       3
#define EVENT_TREEHEALTH    4
#define EVENT_COURIERKILL   5
#define EVENT_PLAYERLEAVE   6
#define EVENT_ROSHANKILL    7
#define EVENT_AEGISON       8
#define EVENT_AEGISOFF      9
#define EVENT_GAMEMODE      10
#define EVENT_RUNESTORE     11
#define EVENT_RUNEUSE       12
#define EVENT_POOL          13
#define EVENT_HEROBAN       14
#define EVENT_HEROPICK      15
#define EVENT_GAMESTART     16
#define EVENT_LEVELUP       17
#define EVENT_HEROASSIST    18
struct W3GEvent
{
  int type;
  int p[4];
  unsigned long time;
};

struct DotaData
{
  bool isDota;
  int sentinel[8];
  int scourge[8];
  int numSentinel;
  int numScourge;
  int sentinelKills;
  int scourgeKills;
  int numGG;
  int numFFSentinel;
  int numFFScourge;

  unsigned long bdTime[NUM_BUILDINGS];

  int major;
  int minor;
  int build;

  bool endgame;

  DotaData ()
  {
    memset (this, 0, sizeof (DotaData));
  }
};

struct W3GAction
{
  int type;
  float posx;
  float posy;
  unsigned long id;
  float hx;
  float hy;
  unsigned long time;
  unsigned long inactive;

  W3GAction () {}
  W3GAction (int typ, unsigned long t)
  {
    type = typ;
    posx = 0;
    posy = 0;
    id = 0;
    time = t;
  }
  W3GAction (int typ, float x, float y, unsigned long _id, unsigned long t)
  {
    type = typ;
    posx = x;
    posy = y;
    id = _id;
    time = t;
  }
};
struct W3GWard
{
  float x;
  float y;
  int id;
  unsigned long time;
  W3GWard () {}
  W3GWard (float X, float Y, int player, unsigned long t)
  {
    x = X;
    y = Y;
    id = player;
    time = t;
  }
};

extern bool useLog;

struct HeroSelection
{
  W3GHero* hero;
  int cnt;
  HeroSelection () {}
  HeroSelection (W3GHero* h, int c)
  {
    hero = h;
    cnt = c;
  }
};

class W3GReplay
{
  void addbytes (int add);
  int mxsize;

  struct GameState
  {
    bool pause;
    bool continue_game;
    unsigned long time;
    bool quick;

    int lastKillTarget;
    int lastKillMessage;
    int lastKillPlayer;
    bool lastKillFirst;
    unsigned long lastKillTime;

    bool switchAccept[256];
    unsigned long lastKill[256];
    int quickKill[256];
    int spree[256];
    int runestore[256];
    int endgame[256][16];
    int endgameid[256];
    int playerid[256];
  };
  void parseMode (char* mode);
  void getMode (W3GMessage const& msg);
  bool loadPlayer ();
  bool loadGame ();
  bool parseBlocks (bool quick);
  bool parseActions (int len, GameState& state, bool quick);
  bool preAnalyze ();
  void analyze ();
  void parseEndgame (CString const& slot, CString const& stat, unsigned long value, GameState& state);
  void finishEndgame (GameState& state);
  W3GHero* getHero (unsigned long o1, unsigned long o2, int side, int id = 0);
  W3GHero* newHero (int side, int id);
  void add_chat (W3GMessage& msg);
  void add_syschat (int type, int id, unsigned long time, char const* fmt, ...);
  int cutpings (int len, bool obs);
  char const* fmt_player (int p);
  char const* fmt_player (W3GPlayer* p);
  int _getCaptain (int team);
  gzmemory mem;
  bool gotitems;
public:
  W3GHeader hdr;
  char* data;
  int size;
  int pos;
  int blockpos;
  char* temp;
  int tmpSize;
  void alloc_temp (int size);

  unsigned long time;
  unsigned long real_time;

  char filename[256];
  char filepath[256];
  void setpath (char const* path);
  void setLocation (char const* path);

  W3GPlayer players[256];
  int pindex[16];
  int numPlayers;
  W3GGame game;
  DotaData dota;
  Array<W3GAction> pactions[16];
  Array<W3GAction> pmove[16];

  Array<W3GMessage> chat;
  Array<W3GWard> wards;
  Array<W3GEvent> events;
  SArray<W3GHero> heroes;

  FILE* log;

  int leaves;

  CString saved_time;
  CString saved_date;
  CString fixed_time;
  __int64 timestamp;

  void readTime (char const* filename);
  void readTime (SYSTEMTIME* sysTime);

  W3GReplay ()
  {
    data = NULL;
    numPlayers = 0;
    temp = new char[8192];
    tmpSize = 8192;
    leaves = 0;
    if (useLog)
      log = fopen ("log.txt", "wt");
    else
      log = NULL;
  }
  ~W3GReplay ()
  {
    delete[] temp;
    delete[] data;
    if (log) fclose (log);
  }

  W3GPlayer* getPlayerInSlot (int s)
  {
    for (int i = 0; i < numPlayers; i++)
      if (players[pindex[i]].slot.color == s)
        return &players[pindex[i]];
    return NULL;
  }

  void clear ()
  {
    numPlayers = 0;
    leaves = 0;
    if (log) fclose (log);
    if (useLog)
      log = fopen ("log.txt", "wt");
    else
      log = NULL;
    memset (&hdr, 0, sizeof hdr);
    for (int i = 0; i < sizeof players / sizeof players[0]; i++)
    {
      players[i].inv.reset ();
      memset (&players[i], 0, ((char*) &players[i].inv) - ((char*) &players[i]));
    }
    memset (&game, 0, sizeof game);
    memset (&dota, 0, sizeof dota);
    game.lwinner = -1;
    real_time = END_TIME;
    chat.clear ();
    wards.clear ();
    heroes.clear ();
    filename[0] = 0;
    for (int i = 0; i < 16; i++)
    {
      pactions[i].clear ();
      pmove[i].clear ();
    }
  }

  bool getPlayerPos (int id, unsigned long time, float& x, float& y, float& dx, float& dy);

  int getChatPos (unsigned long time);
  int getWardPos (unsigned long time);

  int getCaptain (int team);

  void saveas (FILE* file);
  void cutchat (int* index, int size, bool all, bool obs, bool ping, bool pingobs);

  bool load (FILE* file, bool quick = false);
};

__int64 getFileDate (char const* filename);
char* fmtFileDate (char const* filename);
char* fmtFileSize (char const* filename);
CString copyReplay (char const* filename, char const* base, char const* dst, W3GReplay* rep = NULL);

#endif // __REPLAY_H__
