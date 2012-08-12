#ifndef __DOTA_H__
#define __DOTA_H__

int getVersion (int i);
int getNumVersions ();
void addVersion (int ver);
void sortVersions ();

struct DotaTavern
{
  char name[256];
  int side;
};
int getNumTaverns ();
DotaTavern* getTavern (int i);
struct DotaShop
{
  char name[256];
};
int getNumShops ();
DotaShop* getShop (int i);

struct DotaHero
{
  int tavern;               // tavern index LOAD
  unsigned long ids[16];    // list of IDs LOAD
  int numIds;
  char name[256];           // type name LOAD
  char oname[256];          // proper name LOAD
  char imgTag[256];         // image w/o path or extension LOAD
  char abbr[256];           // short name
  unsigned long abils[5];   // ability indices LOAD*
  int slot;                 // 0..11 LOAD
  int point;                // point value LOAD
  int index;
  bool matches (int id);
};
int getNumHeroes ();
DotaHero* getHero (int i);
DotaHero* getHeroById (int id);

char const* getPDTag (int pv);
char const* getPDTagMed (int pv);
char const* getPDTagWide (int pv);
char const* getAbbreviation (int pv);
char const* getPDItemTag (char const* item);

DotaHero* getHeroByPoint (int point);

#define ITEM_NORMAL     0
#define ITEM_RECIPE     1
#define ITEM_COMBO      2
struct DotaItem
{
  unsigned long ids[256];   // list of IDs LOAD
  int numIds;
  char name[256];           // name LOAD
  int cost;                 // total if recipe LOAD
  char imgTag[256];         // image w/o path or extension LOAD
  int index;
  int type;                 // normal recipe or combo
  bool matches (int id);
  int realid;
  int recipe;
};
int getNumItems ();
DotaItem* getItem (int i);
DotaItem* getItemById (int id);
DotaItem* getCombinedItem (char const* n);
DotaItem* getItemByName (char const* n);
char const* getItemIcon (int i);

struct DotaAbility
{
  unsigned long ids[16];    // list of IDs LOAD
  int numIds;
  char name[256];           // name LOAD
  unsigned long hero;       // hero or 0 if multiple
  int slot;                 // slot 0-4 LOAD
  char imgTag[256];         // image w/o path or extension LOAD
  int index;
  bool matches (int id);
};
int getNumAbilities ();
DotaAbility* getAbility (int i);
DotaAbility* getAbilityById (int id);
DotaAbility* getAbilityById (int id, int hero);
DotaAbility* getHeroAbility (int id, int slot);

struct DotaRecipe
{
  int srcid[16];            // component indices LOAD*
  int srccount[16];         // component amounts LOAD
  int numsrc;
  int recipeid;             // recipe scroll index or 0 LOAD*
  int result;               // result index LOAD*
  bool vis;
};
int getNumRecipes ();
DotaRecipe* getRecipe (int i);
DotaRecipe* findRecipe (int id);

bool loadCommonData ();
void loadDefault ();
bool loadDotaData (unsigned long version);
bool loadDotaData (char const* path);
bool parseDotaData (char const* dest, char const* path);
bool saveDotaData (char const* dest);
void freeDotaData ();

bool loadDataEx (unsigned long version, char const* mappath);
bool createPrimary ();

struct DotaBuilding
{
  float x;
  float y;
  char icon[256];
};
enum {
  BUILDING_SENTINEL_TOWER_TOP1,   // e00R
  BUILDING_SENTINEL_TOWER_TOP2,   // e011
  BUILDING_SENTINEL_TOWER_TOP3,   // e00S
  BUILDING_SENTINEL_TOWER_MID1,   // e00R
  BUILDING_SENTINEL_TOWER_MID2,   // e011
  BUILDING_SENTINEL_TOWER_MID3,   // e00S
  BUILDING_SENTINEL_TOWER_BOT1,   // e00R
  BUILDING_SENTINEL_TOWER_BOT2,   // e011
  BUILDING_SENTINEL_TOWER_BOT3,   // e00S
  BUILDING_SENTINEL_MELEE_TOP,    // eaom
  BUILDING_SENTINEL_MELEE_MID,    // eaom
  BUILDING_SENTINEL_MELEE_BOT,    // eaom
  BUILDING_SENTINEL_RANGED_TOP,   // eaoe
  BUILDING_SENTINEL_RANGED_MID,   // eaoe
  BUILDING_SENTINEL_RANGED_BOT,   // eaoe
  BUILDING_SENTINEL_TOWER1,       // e019
  BUILDING_SENTINEL_TOWER2,       // e019
  BUILDING_WORLD_TREE,            // etol -5632.0,-6144.0

  BUILDINGS_SCOURGE,
  BUILDING_SCOURGE_TOWER_TOP1 = BUILDINGS_SCOURGE,    // u00M
  BUILDING_SCOURGE_TOWER_TOP2,    // u00D
  BUILDING_SCOURGE_TOWER_TOP3,    // u00N
  BUILDING_SCOURGE_TOWER_MID1,
  BUILDING_SCOURGE_TOWER_MID2,
  BUILDING_SCOURGE_TOWER_MID3,
  BUILDING_SCOURGE_TOWER_BOT1,
  BUILDING_SCOURGE_TOWER_BOT2,
  BUILDING_SCOURGE_TOWER_BOT3,
  BUILDING_SCOURGE_MELEE_TOP,     // usep
  BUILDING_SCOURGE_MELEE_MID,
  BUILDING_SCOURGE_MELEE_BOT,
  BUILDING_SCOURGE_RANGED_TOP,    // utod
  BUILDING_SCOURGE_RANGED_MID,
  BUILDING_SCOURGE_RANGED_BOT,
  BUILDING_SCOURGE_TOWER1,        // u00T
  BUILDING_SCOURGE_TOWER2,
  BUILDING_FROZEN_THRONE,         // unpl

  NUM_BUILDINGS,

  BUILDING_TOWER = 0,
  BUILDING_MELEE = 1,
  BUILDING_RANGED = 2,
  BUILDING_THRONE = 3
};
DotaBuilding* getBuildings ();
int getBuildingId (int side, int type, int lane, int level);

unsigned long getClosestVersion (unsigned long v);

DWORD getDefaultColor (int clr);
DWORD getSlotColor (int clr);
DWORD getLightColor (int clr);
DWORD getDarkColor (int clr);
DWORD getFlipColor (int clr);
char const* getExtraColors ();
DWORD getExtraColor (int clr);

int getWc3Version (int major, int build);

static int lvlExp[] = {0, 0, 200, 500, 900, 1400, 2000, 2700, 3500, 4400, 5400, 6500,
  7700, 9000, 10400, 11900, 13500, 15200, 17000, 18900, 20900, 23000, 25200, 27500,
  29900, 32400};
static char laneName[][256] = {"None", "Top", "Mid", "Bot", "AFK?"};

#endif // __DOTA_H__
