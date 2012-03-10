#include "core/app.h"
#include "base/array.h"
#include "base/dictionary.h"
#include "base/file.h"
#include "base/mpqfile.h"
#include "dota/dotadata.h"
#include "dota/load/datafile.h"
#include "graphics/image.h"
#include "graphics/imagelib.h"

class DotaLoader
{
  MPQLoader* loader;
  MPQArchive* map;
  void addNewImage(String path, bool big = false)
  {
    File* file = loader->load(path);
    if (file)
    {
      Image image(file);
      if (image.bits())
        getApp()->getImageLibrary()->addImage(String::getFileTitle(path), &image, big);
      delete file;
    }
  }

//////////////////////////////////////////////

  uint32 curid;
  uint32 new_id()
  {
    if ((curid & 0xFF) == '9')
      curid = (curid & (~0xFF)) + 'A';
    else if ((curid & 0xFF) == 'Z')
      curid = (curid & (~0xFF)) + 0x100 + '0';
    else
      curid++;
    return curid;
  }
  String simplify(String str)
  {
    String res = "";
    for (int i = 0; i < str.length(); i++)
    {
      if ((str[i] >= '0' && str[i] <= '9') ||
          (str[i] >= 'a' && str[i] <= 'z'))
        res += str[i];
      else if (str[i] >= 'A' && str[i] <= 'Z')
        res += char(str[i] + 'a' - 'A');
    }
    return res;
  }

//////////////////////////////////////////////

#define ITEM_NORMAL     0
#define ITEM_RECIPE     1
#define ITEM_COMBO      2

  struct Item
  {
    uint32 ids[256];
    int numIds;
    String name;
    String icon;
    int cost;//
    int index;//
    int type;//
    int realid;//
    int recipe;//
  };
  Array<Item> items;
  Dictionary<uint32> idir;
  IntDictionary iadded;
  String strip_item(char const* item, int len = 1000000)
  {
    String res = "";
    int pos = 0;
    if (!strncmp(item, "Disabled ", 9))
      pos += 9;
    while (item[pos])
    {
      if (item[pos] == '|')
      {
        if (item[pos + 1] == 'r' || item[pos + 1] == 'n')
          pos += 2;
        else if (item[pos + 1] == 'c')
          pos += 10;
        else
          res += item[pos++];
      }
      else if (item[pos] == '-' && pos && item[pos - 1] == ' ')
        break;
      else if (item[pos] == '(' && (item[pos + 1] < '0' || item[pos + 1] > '9'))
        break;
      else
        res += item[pos++];
    }
    res.trim();
    if (!stricmp(res.c_str() + res.length() - 6, "recipe"))
    {
      res.cut(res.length() - 6);
      res.trim();
    }
    return res;
  }
  bool addSoldItem(UnitData* item)
  {
    int gold = item->getIntData("goldcost");
    int wood = item->getIntData("lumbercost");
    String name = strip_item(item->getStringData("Name"));
    String descr = item->getStringData("Ubertip");
    int reqpos = name.find("Requires:");
    if ((wood || gold == 0) && reqpos < 0)
      return false;
    int pos = idir.get(simplify(name)) - 1;
    if (pos < 0)
    {
      pos = items.length();
      Item& it = items.push();
      it.index = pos;
      it.ids[0] = item->getID();
      it.numIds = 1;
      it.realid = pos;
      it.cost = gold;
      if (wood)
        it.cost = 0;
      it.name = name;
      String art = item->getStringData("Art");
      it.icon = String::getFileTitle(art);
      addNewImage(art);
      if (reqpos < 0)
      {
        it.type = ITEM_NORMAL;
        idir.set(simplify(name), pos + 1);
      }
      else
      {
        it.type = ITEM_RECIPE;
        it.realid = items.length();
        Item& ri = items.push();
        ri.index = it.realid;
        ri.ids[0] = new_id();
        ri.numIds = 1;
        ri.realid = pos;
        ri.type = ITEM_COMBO;
        ri.cost = 0;
        ri.name = name;
        ri.icon = it.icon;
        idir.set(simplify(name), it.realid + 1);
        it.icon = "BTNSnazzyScroll";
      }
      return true;
    }
    return false;
  }
  uint32 getItemByName(char const* name)
  {
    String n = strip_item(name);
    uint32 pos = idir.get(simplify(n));
    if (pos > 0)
      return items[pos - 1].ids[0];
    else
      return 0;
  }
  void mergeItemName(UnitData* item)
  {
    String name = strip_item(item->getStringData("Name"));
    if (name.length() >= 7 && !strnicmp(name.c_str() + name.length() - 7, "level ", 6))
      name.cut(name.length() - 7, 6);
    if (name.length() >= 4 && name[name.length() - 1] == ')' &&
       (name[name.length() - 3] == '(' || name[name.length() - 4] == '('))
    {
      if (name[name.length() - 4] == '(')
        name.setLength(name.length() - 5);
      else
        name.setLength(name.length() - 4);
    }
    int pos = idir.get(simplify(name)) - 1;
    if (pos >= 0 && !iadded.has(item->getID()))
    {
      iadded.set(item->getID(), 1);
      char const* abils = item->getData("abilList");
      if ((abils[0] == 0 || abils[0] == '_') && stricmp(item->getData("class"), "Campaign"))
        pos = items[pos].realid;
      items[pos].ids[items[pos].numIds++] = item->getID();
    }
  }

//////////////////////////////////////////////

  struct Ability
  {
    uint32 ids[16];
    int numIds;
    String name;
    String icon;
    uint32 hero;
    int slot;
    uint32 index;
    int lvlMax;
    int lvlReq;
    int lvlSkip;
  };
  Array<Ability> abilities;
  IntDictionary adir;

  uint32 addAbility(UnitData* abil)
  {
    Ability& a = abilities.push();
    a.index = abilities.length() - 1;
    a.ids[0] = abil->getID();
    a.numIds = 1;
    a.name = abil->getStringData("Name");
    String art = abil->getStringData("ResearchArt");
    if (art.isEmpty())
      art = abil->getStringData("Researchart");
    a.icon = String::getFileTitle(art);
    addNewImage(art);
    int rx = abil->getStringData("Researchbuttonpos", 0).toInt();
    int ry = abil->getStringData("Researchbuttonpos", 1).toInt();
    if (ry)
      a.slot = 4;
    else
      a.slot = rx;
    a.lvlMax = abil->getIntData("levels");
    a.lvlReq = abil->getIntData("reqLevel");
    a.lvlSkip = abil->getIntData("levelSkip");
    if (a.lvlSkip == 0)
      a.lvlSkip = 2;
    adir.set(abil->getID(), abilities.length());
    return abilities.length();
  }
  void addEngineering(UnitData* eng)
  {
    int levels = eng->getIntData("levels");
    for (int i = 0; i < levels; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        String field = String::format("Data%c%c", char('C' + j), char('1' + i));
        uint32 src = idFromString(eng->getStringData(field, 0));
        uint32 dst = idFromString(eng->getStringData(field, 1));
        uint32 abil = adir.get(src);
        if (abil)
        {
          Ability& a = abilities[abil - 1];
          bool has = false;
          for (int i = 0; i < a.numIds; i++)
            if (a.ids[i] == dst)
              has = true;
          if (!has)
          {
            a.ids[a.numIds++] = dst;
            adir.set(dst, abil);
          }
        }
      }
    }
  }

//////////////////////////////////////////////

  struct Hero
  {
    int tavern;
    uint32 ids[16];
    int numIds;
    String name;
    String properName;
    String icon;
    uint32 abils[5];
    int slot;
    int point;
    int index;
  };
  Array<Hero> heroes;

  void addHero(ObjectData* data, UnitData* hero, int tavern)
  {
    Hero& h = heroes.push();
    h.index = heroes.length() - 1;
    h.tavern = tavern;
    h.ids[0] = hero->getID();
    h.numIds = 1;
    h.slot = hero->getStringData("Buttonpos", 0).toInt() +
         4 * hero->getStringData("Buttonpos", 1).toInt();
    h.point = hero->getStringData("points").toInt();
    h.name = hero->getStringData("Name");
    h.properName = hero->getStringData("Propernames", 0);
    String art = hero->getStringData("Art");
    h.icon = String::getFileTitle(art);
    addNewImage(art, true);
    for (int i = 0; i < 5; i++)
    {
      uint32 abilid = idFromString(hero->getStringData("heroAbilList", i));
      uint32 abil = adir.get(abilid);
      if (abil == 0)
      {
        UnitData* adata = data->getUnitById(abilid);
        if (adata)
          abil = addAbility(adata);
      }
      if (abil)
        h.abils[abilities[abil - 1].slot] = abil;
    }
  }

public:
  DotaLoader(MPQLoader* _loader)
  {
    curid = 'Xx00';
    loader = _loader;
    map = NULL;
  }
  bool load(String path)
  {
    map = MPQArchive::open(path, MPQFILE_READ);
    if (map == NULL)
    {
      String war = getApp()->getRegistry()->readString("warPath");
      map = MPQArchive::open(String::buildFullName(war, path), MPQFILE_READ);
    }
    if (map == NULL)
      return false;
    loader->addArchive(*map);

    GameData data;
    LoadGameData(data, loader, WC3_LOAD_UNITS | WC3_LOAD_ITEMS | WC3_LOAD_ABILITIES);

    int tavernCount = 0;
  }
};