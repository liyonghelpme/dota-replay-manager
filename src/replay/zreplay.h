struct W3GMessage
{
  uint8 id;
  uint32 mode;
  uint32 notifyType;
  float x;
  float y;
  uint32 time;
  int index;
  int line;
  String text;

  W3GMessage()
  {
    memset(&id, 0, ((char*) &index) - ((char*) &id));
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
  String name;
  String creator;
  String map;
  uint8 speed;
  uint8 visibility;
  uint8 observers;
  bool teams_together;
  bool lock_teams;
  bool shared_control;
  bool random_hero;
  bool random_races;
  int slots;
  uint8 game_type;
  bool game_private;
  uint8 record_id;
  sint16 record_length;
  uint8 slot_records;
  uint32 startTime;

  DraftData draft;

  uint8 saver_id;

  int winner;
  int lwinner;
  int wplayer;
  bool llost[16];

  uint32 random_seed;
  uint8 select_mode;
  uint8 start_spots;

  bool hasObservers;

  int pSwitching;
  int pSwitchingWith;
  uint32 switchStart;

  __int64 gmode;
  String game_mode;

  W3GGame()
  {
    memset(&speed, 0, ((char*) &game_mode) - ((char*) &speed));
    lwinner = -1;
  }
};

struct W3GItem
{
  int type;
  String name;
  int id;

  void convert(int i);
};

struct W3GEvent
{
  int type;
  int p[4];
  uint32 time;
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

  uint32 bdTime[NUM_BUILDINGS];

  int major;
  int minor;
  int build;

  bool endgame;

  DotaData()
  {
    memset(this, 0, sizeof(DotaData));
  }
};

struct W3GAction
{
  int type;
  float posx;
  float posy;
  uint32 id;
  float hx;
  float hy;
  uint32 time;
  uint32 inactive;

  W3GAction() {}
  W3GAction(int typ, uint32 t)
  {
    type = typ;
    posx = 0;
    posy = 0;
    id = 0;
    time = t;
  }
  W3GAction(int typ, float x, float y, uint32 _id, uint32 t)
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
  uint32 time;
  W3GWard() {}
  W3GWard(float X, float Y, int player, uint32 t)
  {
    x = X;
    y = Y;
    id = player;
    time = t;
  }
};

struct HeroSelection
{
  W3GHero* hero;
  int cnt;
  HeroSelection() {}
  HeroSelection(W3GHero* h, int c)
  {
    hero = h;
    cnt = c;
  }
};

  void addbytes(int add);
  int mxsize;

  struct GameState
  {
    bool pause;
    bool continue_game;
    uint32 time;
    bool quick;

    int lastKillTarget;
    int lastKillMessage;
    int lastKillPlayer;
    bool lastKillFirst;
    uint32 lastKillTime;

    bool switchAccept[256];
    uint32 lastKill[256];
    int quickKill[256];
    int spree[256];
    int runestore[256];
    int endgame[256][16];
    int endgameid[256];
    int playerid[256];
  };
  void parseMode(String mode);
  void getMode(W3GMessage const& msg);
  bool loadPlayer();
  bool loadGame();
  bool parseBlocks(bool quick);
  bool parseActions(int len, GameState& state, bool quick);
  bool preAnalyze();
  void analyze();
  void parseEndgame(String slot, String stat, unsigned long value, GameState& state);
  void finishEndgame(GameState& state);
  W3GHero* getHero(uint32 o1, uint32 o2, int side, int id = 0);
  W3GHero* newHero(int side, int id);
  void add_chat(W3GMessage& msg);
  void add_syschat(int type, int id, uint32 time, char const* fmt, ...);
  int cutpings(int len, bool obs);
  String fmt_player(int p);
  String fmt_player(W3GPlayer* p);
  int _getCaptain(int team);
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
  void alloc_temp(int size);

  uint32 time;
  uint32 real_time;

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

