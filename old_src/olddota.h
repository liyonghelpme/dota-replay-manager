#ifndef __DOTA_H__
#define __DOTA_H__

struct DotaHero
{
  int tavern;
  union
  {
    unsigned long id;
    unsigned long ids[1];
  };
  char name[256];
  char oname[256];
  int fid;
  char imgTag[256];

  char abbr[256];

  int index;
};
int getNumHeroes ();
DotaHero* getHero (int i);
DotaHero* getHeroById (int id);

struct DotaTavern
{
  char name[256];
  int side;
};
DotaTavern* getTavern (int i);
inline int getNumTaverns ()
{
  return 8;
}

struct DotaItem
{
  union
  {
    unsigned long id;
    unsigned long ids[1];
  };
  unsigned long alt;
  char name[256];
  int cost;
  char imgTag[256];

  int index;
};
int getNumItems ();
DotaItem* getItem (int i);
DotaItem* getItemById (int id);
DotaItem* getCombinedItem (char const* n);
DotaItem* getItemByName (char const* n);
char const* getItemIcon (int i);

struct DotaAbility
{
  union
  {
    unsigned long id;
    unsigned long ids[1];
  };
  char name[256];
  unsigned long hero;
  int slot;
  char imgTag[256];
  unsigned long rhero;

  int index;
};
int getNumAbilities ();
DotaAbility* getAbility (int i);
DotaAbility* getAbilityById (int id);
DotaAbility* getAbilityById (int id, int hero);
DotaAbility* getHeroAbility (int id, int slot);

struct DotaData;

void fixVersion (DotaData* dota);
void resetVersion ();
void setEquiv (char const* from, int to);

void fix_itemid (unsigned long& id);

void zero_abils ();
void set_abils (unsigned long id, char const* list);

DWORD getDefaultColor (int clr);
DWORD getSlotColor (int clr);
DWORD getLightColor (int clr);
DWORD getDarkColor (int clr);
DWORD getFlipColor (int clr);

static int lvlExp[] = {0, 0, 200, 500, 900, 1400, 2000, 2700, 3500, 4400, 5400, 6500,
  7700, 9000, 10400, 11900, 13500, 15200, 17000, 18900, 20900, 23000, 25200, 27500,
  29900, 32400};

void delDotaData ();

#endif // __DOTA_H__
