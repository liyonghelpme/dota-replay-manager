#ifndef __DOTA_DOTADATA_H__
#define __DOTA_DOTADATA_H__

#include <windows.h>

#include "base/types.h"
#include "base/string.h"
#include "base/dictionary.h"
#include "base/intdict.h"

#define MAX_HERO_POINT          256

#define OBJECT_NONE             0
#define OBJECT_HERO             1
#define OBJECT_ABILITY          2
#define OBJECT_ITEM             3
#define OBJECT_RECIPE           4

uint32 idFromString(String str);
String idToString(uint32 id);

class DotaLibrary;
class File;
class Dota
{
public:
  struct Ability;
  struct Hero
  {
    uint32 id;
    String name;
    String properName;
    String icon;
    String shortName;
    Ability* abilities[5];
    int tavern;
    int tavernSlot;
    int point;

    bool hasAbility(Ability* ability)
    {
      for (int i = 0; i < 5; i++)
        if (abilities[i] == ability)
          return true;
      return false;
    }
  };
  struct Ability
  {
    uint32 id;
    String name;
    String icon;
    Hero* hero;
    int slot;
    int lvlStart;
    int lvlSkip;
    int lvlMax;
  };
  struct Recipe;
  struct Item
  {
    uint32 id;
    String name;
    String icon;
    int cost;
    Recipe* recipe;
  };
  struct Recipe
  {
    int numSources;
    Item* sources[16];
    int sourceCount[16];
    Item* result;
  };

  struct Object
  {
    int type;
    String name;
    union
    {
      Hero* hero;
      Ability* ability;
      Item* item;
    };
  };
private:
  Hero heroes[MAX_HERO_POINT];
  Ability abilities[1024];
  Item items[512];
  Recipe recipes[128];
  int numItems;
  int numAbilities;
  int numRecipes;
  enum {iHero = 0x01000000,
        iAbil = 0x02000000,
        iItem = 0x03000000,
        iMask = 0xFF000000,
        iPos  = 0x00FFFFFF};
  IntDictionary index;

  uint32 version;
  friend class DotaLibrary;
  DotaLibrary* library;
  int ref;
  Dota(uint32 ver, File* file, DotaLibrary* lib);
public:

  Hero* getHero(int point)
  {
    if (point < 0 || point >= MAX_HERO_POINT || (point > 0 && heroes[point].id == 0))
      return NULL;
    return &heroes[point];
  }

  Hero* getHeroById(uint32 id);
  Ability* getAbilityById(uint32 id);
  Item* getItemById(uint32 id);
  int getObjectById(uint32 id, Object* obj);

  Hero* getHeroByName(char const* name);
  Ability* getAbilityByName(char const* name);
  Item* getItemByName(char const* name);

  int getNumRecipes() const
  {
    return numRecipes;
  }
  Recipe* getRecipe(int i)
  {
    return &recipes[i];
  }

  void release();
};

class DotaLibrary
{
  CRITICAL_SECTION lock;

  IntDictionary versions;
  void delDota(uint32 version);
  friend class Dota;

  struct Tavern
  {
    String name;
    int side;
  };
  Array<Tavern> taverns;
  Array<String> shops;
  String heroAbbr[256];
  String heroPdTag[256];
  String heroPdTagMed[256];
  String heroPdTagWide[256];
  Dota* latest;
  Dictionary<String> itemPdTag;
  bool loadMap(String map, String dest);
  static INT_PTR CALLBACK NoDataDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
public:
  DotaLibrary();
  ~DotaLibrary();

  int getNumTaverns() const
  {
    return taverns.length();
  }
  String getTavernName(int i) const
  {
    return taverns[i].name;
  }
  int getTavernSide(int i) const
  {
    return taverns[i].side;
  }
  int getNumShops() const
  {
    return shops.length();
  }
  String getShopName(int i) const
  {
    return shops[i];
  }
  String getHeroAbbreviation(int point) const
  {
    return heroAbbr[point];
  }
  String getHeroPdTag(int point) const
  {
    return heroPdTag[point];
  }
  String getHeroPdTagMedium(int point) const
  {
    return heroPdTagMed[point];
  }
  String getHeroPdTagWide(int point) const
  {
    return heroPdTagWide[point];
  }
  String getItemPdTag(char const* name) const
  {
    return itemPdTag.get(name);
  }

  Dota* getDota()
  {
    return latest;
  }
  Dota* getDota(uint32 version, char const* mapPath = NULL);
};

#endif // __DOTA_DOTADATA_H__
