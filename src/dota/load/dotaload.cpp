#include "core/app.h"
#include "base/array.h"
#include "base/dictionary.h"
#include "base/file.h"
#include "base/mpqfile.h"
#include "dota/dotadata.h"
#include "dota/load/datafile.h"
#include "graphics/image.h"
#include "graphics/imagelib.h"

static const int numOldTaverns = 17;
static char oldTaverns[numOldTaverns][256] = {
  "Morning Tavern",
  "Sunrise Tavern",
  "Dawn Tavern",
  "Light Tavern",
  "Evening Tavern",
  "Midnight Tavern",
  "Twilight Tavern",
  "Dusk Tavern",
  "Strength (Sentinel) - Sunrise Tavern",
  "Agility (Sentinel) - Morning Tavern",
  "Intelligence (Sentinel) - Light Tavern",
  "Strength (Scourge) - Dark Tavern",
  "Agility (Scourge) - Evening Tavern",
  "Intelligence (Scourge) - Midnight Tavern",
  "Strength (Neutral) - Dusk Tavern",
  "Agility (Neutral) - Dawn Tavern",
  "Intelligence (Neutral) - Twilight Tavern"
};

class DotaLoader
{
  MPQLoader loader;
  MPQArchive* map;
  void addNewImage(String path, bool big = false)
  {
    String title = String::getFileTitle(path);
    if (getApp()->getImageLibrary()->hasImage(title))
      return;
    File* file = loader.load(path);
    if (file == NULL)
    {
      String::setExtension(path, ".blp");
      file = loader.load(path);
    }
    if (file)
    {
      Image image(file);
      if (image.bits())
        getApp()->getImageLibrary()->addImage(title, &image, big);
      delete file;
    }
  }

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
    int type;//
    int realid;//
    int recipe;//
  };
  Array<Item> items;
  Dictionary<uint32> iname;
  IntDictionary idir;

  struct Recipe
  {
    int src[16];
    int srccount[16];
    int numSrc;
    int recipe;
    int result;
    bool vis;
  };
  Array<Recipe> recipes;

  String strip_item(char const* item)
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
    int reqpos = descr.find("Requires:", 0, FIND_CASE_INSENSITIVE);
    if ((wood || gold == 0) && reqpos < 0)
      return false;
    if (!iname.has(name))
    {
      int pos = items.length();
      items.push();
      items[pos].ids[0] = item->getID();
      items[pos].numIds = 1;
      items[pos].realid = pos;
      items[pos].cost = gold;
      if (wood)
        items[pos].cost = 0;
      items[pos].name = name;
      String art = item->getStringData("Art");
      items[pos].icon = String::getFileTitle(art);
      addNewImage(art);
      if (reqpos < 0)
      {
        items[pos].type = ITEM_NORMAL;
        iname.set(name, pos);
        idir.set(item->getID(), pos);
      }
      else
      {
        items[pos].type = ITEM_RECIPE;
        items[pos].realid = items.length();
        int p2 = items.length();
        items.push();
        items[p2].numIds = 0;
        items[p2].realid = pos;
        items[p2].type = ITEM_COMBO;
        items[p2].cost = 0;
        items[p2].name = name;
        items[p2].icon = items[pos].icon;
        iname.set(name, p2);
        idir.set(item->getID(), p2);
        items[pos].icon = "BTNSnazzyScroll";
      }
      return true;
    }
    return false;
  }
  int getItemByName(char const* name)
  {
    String n = strip_item(name);
    if (iname.has(n))
      return iname.get(n);
    return -1;
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
    if (iname.has(name) && !idir.has(item->getID()))
    {
      char const* abils = item->getData("abilList");
      uint32 pos = iname.get(name);
      if ((abils[0] == 0 || abils[0] == '_') && stricmp(item->getData("class"), "Campaign"))
        pos = items[pos].realid;
      idir.set(item->getID(), pos);
      if (items[pos].numIds > 200)
        int adsf = 0;
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
    int lvlMax;
    int lvlReq;
    int lvlSkip;
  };
  Array<Ability> abilities;
  IntDictionary adir;

  uint32 addAbility(UnitData* abil)
  {
    Ability& a = abilities.push();
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
    adir.set(abil->getID(), abilities.length() - 1);
    return abilities.length() - 1;
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
        if (adir.has(src))
        {
          uint32 abil = adir.get(src);
          Ability& a = abilities[abil];
          bool has = false;
          for (int i = 0; i < a.numIds; i++)
            if (a.ids[i] == dst)
              has = true;
          if (!has)
          {
            if (a.numIds > 10)
              int adsf = 0;
            a.ids[a.numIds++] = dst;
            adir.set(dst, abil);
          }
        }
      }
    }
  }
  void addMorph(UnitData* morph)
  {
    int levels = morph->getIntData("levels");
    for (int i = 0; i < levels; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        uint32 src = idFromString(morph->getStringData(String::format("DataA%c", char('1' + i))));
        uint32 dst = idFromString(morph->getStringData(String::format("UnitID%c", char('1' + i))));
        if (hdir.has(src) && !hdir.has(dst))
        {
          int srch = hdir.get(src);
          if (heroes[srch].numIds > 10)
            int asdf = 0;
          heroes[srch].ids[heroes[srch].numIds++] = dst;
          hdir.set(dst, srch);
        }
        else if (!hdir.has(src) && hdir.has(dst))
        {
          int dsth = hdir.get(dst);
          if (heroes[dsth].numIds > 10)
            int asdf = 0;
          heroes[dsth].ids[heroes[dsth].numIds++] = src;
          hdir.set(src, dsth);
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
    int abils[5];
    int slot;
    int point;
  };
  Array<Hero> heroes;
  IntDictionary hdir;

  void addHero(ObjectData* data, UnitData* hero, int tavern)
  {
    Hero& h = heroes.push();
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
      h.abils[i] = -1;
    for (int i = 0; i < 5; i++)
    {
      uint32 abilid = idFromString(hero->getStringData("heroAbilList", i));
      int abil = -1;
      if (adir.has(abilid))
        abil = adir.get(abilid);
      else
      {
        UnitData* adata = data->getUnitById(abilid);
        if (adata)
          abil = addAbility(adata);
      }
      if (abil >= 0)
        h.abils[abilities[abil].slot] = abil;
    }
    hdir.set(hero->getID(), heroes.length() - 1);
  }

public:
  DotaLoader()
    : iname(DictionaryMap::alNumNoCase)
    , loader(*getApp()->getWarLoader())
  {
    map = NULL;
    curid = 'Xx00';
  }
  ~DotaLoader()
  {
    map->release();
  }
  bool load(String path)
  {
    map = MPQArchive::open(path, MPQFILE_READ);
    if (map == NULL)
      map = MPQArchive::open(String::buildFullName(cfg.warPath, path), MPQFILE_READ);
    if (map == NULL)
      return false;
    loader.addArchive(map);

    DotaLibrary* dotaLib = getApp()->getDotaLibrary();

    // Parsing object data...
    GameData data;
    LoadGameData(data, &loader, WC3_LOAD_UNITS | WC3_LOAD_ITEMS | WC3_LOAD_ABILITIES);

    // Parsing taverns and shops...
    Array<String> list;
    int tavernCount = 0;
    for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits(); i++)
    {
      UnitData* unit = data.data[WC3_UNITS]->getUnit(i);
      if (unit)
      {
        String name = unit->getStringData("Name");
        for (int ti = 0; ti < dotaLib->getNumTaverns(); ti++)
        {
          if (!name.icompare(dotaLib->getTavernName(ti)))
          {
            tavernCount++;
            unit->getStringData("Sellunits").split(list, ',', true);
            for (int h = 0; h < list.length(); h++)
            {
              uint32 id = idFromString(list[h]);
              if (!hdir.has(id))
              {
                UnitData* hero = data.data[WC3_UNITS]->getUnitById(id);
                if (hero)
                  addHero(data.data[WC3_ABILITIES], hero, ti);
              }
            }
            break;
          }
        }
        for (int si = 0; si < dotaLib->getNumShops(); si++)
        {
          if (!name.icompare(dotaLib->getShopName(si)))
          {
            for (int m = 0; m < 2; m++)
            {
              unit->getStringData(m ? "Sellitems" : "Sellunits").split(list, ',', true);
              for (int h = 0; h < list.length(); h++)
              {
                uint32 id = idFromString(list[h]);
                if (!idir.has(id))
                {
                  UnitData* item = data.data[m ? WC3_ITEMS : WC3_UNITS]->getUnitById(id);
                  if (item)
                    addSoldItem(item);
                }
              }
              break;
            }
          }
        }
      }
    }
    if (tavernCount == 0)
    {
      for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits(); i++)
      {
        UnitData* unit = data.data[WC3_UNITS]->getUnit(i);
        if (unit)
        {
          String name = unit->getStringData("Name");
          for (int ti = 0; ti < numOldTaverns; ti++)
          {
            if (!name.icompare(oldTaverns[ti]))
            {
              unit->getStringData("Sellunits").split(list, ',', true);
              for (int h = 0; h < list.length(); h++)
              {
                uint32 id = idFromString(list[h]);
                if (!hdir.has(id))
                {
                  UnitData* hero = data.data[WC3_UNITS]->getUnitById(id);
                  if (hero)
                    addHero(data.data[WC3_ABILITIES], hero, ti);
                }
              }
              break;
            }
          }
        }
      }
    }
    // Parsing items...
    for (int i = 0; i < items.length(); i++)
    {
      if (items[i].type == ITEM_RECIPE)
      {
        UnitData* unit = data.data[WC3_ITEMS]->getUnitById(items[i].ids[0]);
        if (unit == NULL)
          unit = data.data[WC3_UNITS]->getUnitById(items[i].ids[0]);
        if (unit)
        {
          String descr = unit->getStringData("Ubertip");
          descr.toLower();
          int reqpos = descr.find("requires:");
          if (reqpos >= 0)
          {
            if (descr.find("any of the following") >= 0 ||
                descr.find("staff of wizardry and blade of alacrity") >= 0)
            {
              // manually add scepter...
              int scepter = items[i].realid;
              int recipe = i;
              int booster = getItemByName("Point Booster");
              int axe = getItemByName("Ogre Axe");
              int blade = getItemByName("Blade of Alacrity");
              int staff = getItemByName("Staff of Wizardry");
              Recipe r;
              r.recipe = recipe;
              r.result = scepter;
              r.numSrc = 4;
              r.src[0] = recipe;
              r.srccount[0] = 1;
              r.src[1] = booster;
              r.srccount[1] = 1;
              r.src[2] = axe;
              r.srccount[2] = 1;
              r.src[3] = blade;
              r.srccount[3] = 1;
              recipes.push(r);
              r.src[3] = staff;
              recipes.push(r);
              r.src[2] = blade;
              recipes.push(r);
            }
            else
            {
              int r = recipes.length();
              recipes.push();
              recipes[r].numSrc = 0;
              recipes[r].recipe = -1;
              if (items[i].cost)
              {
                recipes[r].srccount[0] = 1;
                recipes[r].src[0] = i;
                recipes[r].numSrc = 1;
                recipes[r].recipe = i;
              }
              recipes[r].result = items[i].realid;
              if (i == 115)
                int adsf = 0;
              int pos = reqpos;
              int prev = pos;
              bool prevOr = false;
              int num = 0;
              while (true)
              {
                if (descr[pos] == 0 || (descr[pos] == '|' && descr[pos + 1] == 'n'))
                {
                  String name = strip_item(descr.substring(prev, pos));
                  int count = 1;
                  bool hasx = false;
                  int npos = 0;
                  if (name[0] >= '0' && name[0] <= '9')
                  {
                    count = atoi(name.c_str());
                    while (name[npos] && name[npos] != ' ')
                    {
                      if (name[npos] == 'x')
                        hasx = true;
                      npos++;
                    }
                    while (name[npos] == ' ')
                      npos++;
                  }
                  if (count > 1 && name[name.length() - 1] == 's')
                    name.setLength(name.length() - 1);
                  if (npos)
                    name.cut(0, npos);
                  String buy = "buy recipe to upgrade (";
                  if (name == "or")
                    prevOr = true;
                  else if (!strncmp(name, buy, buy.length()))
                  {
                    int levels = atoi(name.c_str() + buy.length());
                    int iid = items[i].realid;
                    int result = recipes[r].result;
                    for (int lvl = 2; lvl <= levels; lvl++)
                    {
                      Recipe& ur = recipes.push();
                      ur.numSrc = 2;
                      ur.recipe = recipes[r].recipe;
                      ur.src[0] = recipes[r].recipe;
                      ur.srccount[0] = 1;
                      ur.src[1] = result;
                      ur.srccount[1] = 1;
                      ur.result = items.length();
                      result = ur.result;
                      Item& it = items.push();
                      it.numIds = 0;
                      it.name.printf("%s %d", items[iid].name, lvl);
                      it.icon = items[iid].icon;
                      it.type = ITEM_COMBO;
                      it.realid = i;
                      it.cost = 0;
                      iname.set(it.name, items.length() - 1);
                    }
                    items[iid].name += " 1";
                    iname.set(items[iid].name, iid + 1);
                  }
                  else
                  {
                    if (iname.has(name))
                    {
                      int cpos = iname.get(name);
                      if (prevOr)
                      {
                        Recipe& nr = recipes.push();
                        nr = recipes[r];
                        nr.src[nr.numSrc - 1] = cpos;
                        nr.srccount[nr.numSrc - 1] = count;
                      }
                      else
                      {
                        recipes[r].src[recipes[r].numSrc] = cpos;
                        recipes[r].srccount[recipes[r].numSrc++] = count;
                      }
                      prevOr = false;
                    }
                  }
                  if (descr[pos] == 0)
                    break;
                  pos += 2;
                  prev = pos;
                }
                else
                  pos++;
              }
            }
          }
        }
      }
    }
    for (int i = 0; i < data.data[WC3_ITEMS]->getNumUnits(); i++)
    {
      UnitData* item = data.data[WC3_ITEMS]->getUnit(i);
      if (item)
        mergeItemName(item);
    }
    for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits(); i++)
    {
      UnitData* item = data.data[WC3_UNITS]->getUnit(i);
      if (item)
        mergeItemName(item);
    }
    // Parsing upgradeable abilities...
    for (int i = 0; i < data.data[WC3_ABILITIES]->getNumUnits(); i++)
    {
      UnitData* abil = data.data[WC3_ABILITIES]->getUnit(i);
      if (abil)
      {
        String code = abil->getStringData("code");
        if (code == "ANeg")
          addEngineering(abil);
        else if (code == "AEme" || code == "Abrf")
          addMorph(abil);
      }
    }
    // Computing recipe costs...
    for (int i = 0; i < recipes.length(); i++)
    {
      if (items[recipes[i].result].type > 0)
      {
        items[recipes[i].result].cost = 0;
        items[recipes[i].result].type = -items[recipes[i].result].type;
      }
    }
    for (int maxc = 0; maxc < 20; maxc++)
    {
      bool total = true;
      for (int i = 0; i < recipes.length(); i++)
      {
        int res = recipes[i].result;
        if (items[res].type < 0)
        {
          bool ok = true;
          items[res].cost = 0;
          for (int j = 0; j < recipes[i].numSrc; j++)
          {
            int comp = recipes[i].src[j];
            if (items[comp].type < 0)
            {
              ok = false;
              break;
            }
            else
              items[res].cost += items[comp].cost * recipes[i].srccount[j];
          }
          if (!ok)
            total = false;
          else
            items[res].type = ITEM_COMBO;
        }
      }
      if (total)
        break;
    }
    return true;
  }
  void write(File* file)
  {
    file->printf("[HERO]\r\n");
    for (int i = 0; i < heroes.length(); i++)
    {
      Hero& h = heroes[i];
      file->printf("\"%s\",\"%s\",%d,\"%s\",%d,%d", h.name, h.properName, h.tavern,
        h.icon, h.slot, h.point);
      for (int j = 0; j < 5; j++)
      {
        if (h.abils[j] >= 0)
          file->printf(",%s", idToString(abilities[h.abils[j]].ids[0]));
        else
          file->printf(",");
      }
      for (int j = 0; j < h.numIds; j++)
        file->printf(",%s", idToString(h.ids[j]));
      file->printf("\r\n");
    }
    file->printf("[ITEM]\r\n");
    for (int i = 0; i < items.length(); i++)
    {
      Item& it = items[i];
      if (it.numIds == 0)
        it.ids[it.numIds++] = new_id();
      file->printf("\"%s\",%d,\"%s\"", it.name, it.cost, it.icon);
      for (int j = 0; j < it.numIds; j++)
        file->printf(",%s", idToString(it.ids[j]));
      file->printf("\r\n");
    }
    file->printf("[ABILITY]\r\n");
    for (int i = 0; i < abilities.length(); i++)
    {
      Ability& a = abilities[i];
      file->printf("\"%s\",%d,\"%s\",%d,%d,%d", a.name, a.slot, a.icon, a.lvlReq, a.lvlSkip, a.lvlMax);
      for (int j = 0; j < a.numIds; j++)
        file->printf(",%s", idToString(a.ids[j]));
      file->printf("\r\n");
    }
    file->printf("[RECIPE]\r\n");
    for (int i = 0; i < recipes.length(); i++)
    {
      Recipe& r = recipes[i];
      file->printf("%s", idToString(items[r.result].ids[0]));
      for (int j = 0; j < r.numSrc; j++)
        file->printf(",%s,%d", idToString(items[r.src[j]].ids[0]), r.srccount[j]);
      file->printf("\r\n");
    }
  }
};

bool DotaLibrary::loadMap(String map, String dest)
{
  EnterCriticalSection(&lock);
  MPQArchive* res = getApp()->getResources();
  DotaLoader loader;
  if (!loader.load(map))
    return false;
  File* file = res->openFile(dest, File::REWRITE);
  loader.write(file);
  delete file;
  res->flushListFile();
  LeaveCriticalSection(&lock);
  return true;
}
