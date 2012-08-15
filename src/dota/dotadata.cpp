#include "base/version.h"
#include "base/mpqfile.h"
#include "core/app.h"

#include "dotadata.h"

uint32 idFromString(String str)
{
  if (str.length() < 4)
    return 0;
  return (uint32(str[0]) << 24) |
         (uint32(str[1]) << 16) |
         (uint32(str[2]) <<  8) |
          uint32(str[3]);
}
String idToString(uint32 id)
{
  if ((id & 0xFFFF0000) == 0x000D0000)
    return String::format("0x%08X", id);
  else
  {
    String str = "";
    str += char(id >> 24);
    str += char(id >> 16);
    str += char(id >> 8);
    str += char(id);
    return str;
  }
}

static int __cdecl recipeComp(void const* va, void const* vb)
{
  Dota::Recipe* a = (Dota::Recipe*) va;
  Dota::Recipe* b = (Dota::Recipe*) vb;
  return b->result->cost - a->result->cost;
}
Dota::Dota(uint32 ver, File* file, DotaLibrary* lib)
{
  version = ver;
  library = lib;
  ref = 0;

  for (int i = 0; i < MAX_HERO_POINT; i++)
    heroes[i].id = 0;
  numItems = 1;
  numAbilities = 1;
  heroes[0].name = "No Hero";
  heroes[0].icon = "Empty";
  items[0].name = "Empty slot";
  items[0].icon = "emptyslot";
  abilities[0].name = "None";
  abilities[0].icon = "Empty";

  numRecipes = 0;
  if (file)
  {
    String buf;
    int posHero = -1;
    int posAbility = -1;
    int posItem = -1;
    int posRecipe = -1;
    while (file->gets(buf))
    {
      buf.trim();
      if (buf[0] == '[')
      {
        if (buf == "[HERO]")
          posHero = file->tell();
        else if (buf == "[ABILITY]")
          posAbility = file->tell();
        else if (buf == "[ITEM]")
          posItem = file->tell();
        else if (buf == "[RECIPE]")
          posRecipe = file->tell();
      }
    }
    Array<String> list;
    if (posAbility >= 0)
    {
      file->seek(posAbility, SEEK_SET);
      while (file->gets(buf))
      {
        buf.trim();
        if (buf[0] == '[')
          break;
        buf.split(list, ',', true);
        if (list.length() > 6)
        {
          // name,slot,icon{,id}
          Ability* abil = &abilities[numAbilities];
          abil->name = list[0].dequote();
          abil->slot = list[1].toInt();
          abil->icon = list[2].dequote();
          abil->lvlStart = list[3].toInt();
          abil->lvlSkip = list[4].toInt();
          abil->lvlMax = list[5].toInt();
          abil->hero = NULL;
          abil->id = 0;
          for (int i = 6; i < list.length(); i++)
          {
            uint32 id = idFromString(list[i]);
            if (abil->id == 0)
              abil->id = id;
            index.set(id, iAbil | numAbilities);
          }
          numAbilities++;
        }
      }
    }
    if (posHero >= 0)
    {
      file->seek(posHero, SEEK_SET);
      while (file->gets(buf))
      {
        buf.trim();
        if (buf[0] == '[')
          break;
        buf.split(list, ',', true);
        if (list.length() > 11 && list[5].toInt())
        {
          // name,properName,tavern,icon,tavernSlot,point{,ability}{,id}
          int point = list[5].toInt();
          Hero* hero = &heroes[point];
          hero->name = list[0].dequote();
          hero->properName = list[1].dequote();
          hero->tavern = list[2].toInt();
          hero->icon = list[3].dequote();
          hero->tavernSlot = list[4].toInt();
          hero->point = point;
          for (int i = 0; i < 5; i++)
            hero->abilities[i] = getAbilityById(idFromString(list[6 + i]));
          hero->id = 0;
          for (int i = 11; i < list.length(); i++)
          {
            uint32 id = idFromString(list[i]);
            if (hero->id == 0)
              hero->id = id;
            index.set(id, iHero | point);
          }
        }
      }
    }
    if (posItem >= 0)
    {
      file->seek(posItem, SEEK_SET);
      while (file->gets(buf))
      {
        buf.trim();
        if (buf[0] == '[')
          break;
        buf.split(list, ',', true);
        if (list.length() > 3)
        {
          // name,cost,icon{,id}
          Item* item = &items[numItems];
          item->name = list[0].dequote();
          item->cost = list[1].toInt();
          item->icon = list[2].dequote();
          item->recipe = NULL;
          item->id = 0;
          for (int i = 3; i < list.length(); i++)
          {
            uint32 id = idFromString(list[i]);
            if (item->id == 0)
              item->id = id;
            index.set(id, iItem | numItems);
          }
          numItems++;
        }
      }
    }
    if (posRecipe >= 0)
    {
      file->seek(posRecipe, SEEK_SET);
      while (file->gets(buf))
      {
        buf.trim();
        if (buf[0] == '[')
          break;
        buf.split(list, ',', true);
        if (list.length() > 1 && (list.length() & 1) == 1)
        {
          // result{,source,sourceCount}
          Recipe* recipe = &recipes[numRecipes];
          recipe->result = getItemById(idFromString(list[0]));
          if (recipe->result == NULL)
            continue;
          numRecipes++;
          recipe->numSources = 0;
          for (int i = 1; i < list.length(); i += 2)
          {
            recipe->sources[recipe->numSources] = getItemById(idFromString(list[i]));
            recipe->sourceCount[recipe->numSources++] = list[i + 1].toInt();
          }
        }
      }
    }
    ////////////////////////////////////////////

    for (int i = 1; i < MAX_HERO_POINT; i++)
    {
      Hero* hero = &heroes[i];
      if (hero->id == 0)
        continue;
      for (int j = 0; j < 5; j++)
      {
        if (hero->abilities[j])
        {
          if (hero->abilities[j]->hero)
            hero->abilities[j]->hero = &heroes[0];
          else
            hero->abilities[j]->hero = hero;
        }
      }
      int sp_a = hero->name.find(' ');
      int sp_b = hero->properName.find(' ');
      if (sp_a <= sp_b + 1)
        hero->shortName = (sp_a < 0 ? hero->name : hero->name.substr(0, sp_a));
      else
        hero->shortName = (sp_b < 0 ? hero->properName : hero->properName.substr(0, sp_b));
    }
    for (int i = 0; i < numAbilities; i++)
      if (abilities[i].hero == &heroes[0])
        abilities[i].hero = NULL;
    qsort(recipes, numRecipes, sizeof(Recipe), recipeComp);
    for (int i = 0; i < numRecipes; i++)
      if (recipes[i].result)
        recipes[i].result->recipe = &recipes[i];
  }
}
Dota::Hero* Dota::getHeroById(uint32 id)
{
  uint32 i = index.get(id);
  if ((i & iMask) == iHero)
    return &heroes[i & iPos];
  else
    return NULL;
}
Dota::Ability* Dota::getAbilityById(uint32 id)
{
  uint32 i = index.get(id);
  if ((i & iMask) == iAbil)
    return &abilities[i & iPos];
  else
    return NULL;
}
Dota::Item* Dota::getItemById(uint32 id)
{
  uint32 i = index.get(id);
  if ((i & iMask) == iItem)
    return &items[i & iPos];
  else
    return NULL;
}
int Dota::getObjectById(uint32 id, Object* obj)
{
  uint32 i = index.get(id);
  if ((i & iMask) == iHero)
  {
    obj->type = OBJECT_HERO;
    obj->hero = &heroes[i & iPos];
    obj->name = obj->hero->name;
  }
  else if ((i & iMask) == iAbil)
  {
    obj->type = OBJECT_ABILITY;
    obj->ability = &abilities[i & iPos];
    obj->name = obj->ability->name;
  }
  else if ((i & iMask) == iItem)
  {
    obj->type = OBJECT_ITEM;
    obj->item = &items[i & iPos];
    obj->name = obj->item->name;
  }
  else
  {
    obj->type = OBJECT_NONE;
    obj->hero = NULL;
    obj->name = "";
  }
  return obj->type;
}
Dota::Hero* Dota::getHeroByName(char const* name)
{
  for (int i = 0; i < MAX_HERO_POINT; i++)
    if (heroes[i].name.icompare(name) == 0)
      return &heroes[i];
  return NULL;
}
Dota::Ability* Dota::getAbilityByName(char const* name)
{
  for (int i = 0; i < numAbilities; i++)
    if (abilities[i].name.icompare(name) == 0)
      return &abilities[i];
  return NULL;
}
Dota::Item* Dota::getItemByName(char const* name)
{
  for (int i = 0; i < numItems; i++)
    if (items[i].name.icompare(name) == 0)
      return &items[i];
  return NULL;
}

void Dota::release()
{
  if (this && --ref <= 0)
    library->delDota(version);
}
DotaLibrary::DotaLibrary()
  : itemPdTag(DictionaryMap::alNumNoCase)
{
  MPQArchive* res = getApp()->getResources();
  File* common = res->openFile("dota\\common.txt", File::READ);
  if (common)
  {
    enum {cTavern = 1, cShop = 2, cAbbr = 3, cPd = 4, cPdMed = 5, cPdWide = 6, cPdItem = 7};
    int cat = 0;
    String buf;
    Array<String> list;
    while (common->gets(buf))
    {
      buf.trim();
      if (buf[0] == '[')
      {
        if (buf == "[TAVERN]")
          cat = cTavern;
        else if (buf == "[SHOP]")
          cat = cShop;
        else if (buf == "[ABBREVIATION]")
          cat = cAbbr;
        else if (buf == "[PDTAG]")
          cat = cPd;
        else if (buf == "[PDTAGMED]")
          cat = cPdMed;
        else if (buf == "[PDTAGWIDE]")
          cat = cPdWide;
        else if (buf == "[PLAYDOTAITEMTAG]")
          cat = cPdItem;
        else
          cat = 0;
      }
      else if (cat && buf.length())
      {
        buf.split(list, ',', true);
        if (cat == cTavern)
        {
          if (list.length() == 2)
          {
            Tavern& t = taverns.push();
            t.name = list[0];
            t.side = list[1].toInt();
          }
        }
        else if (cat == cShop)
        {
          if (list.length() == 1)
            shops.push(list[0]);
        }
        else if (cat == cAbbr)
        {
          if (list.length() == 2)
            heroAbbr[list[0].toInt()] = list[1];
        }
        else if (cat == cPd)
        {
          if (list.length() == 2)
            heroPdTag[list[0].toInt()] = list[1];
        }
        else if (cat == cPdMed)
        {
          if (list.length() == 2)
            heroPdTagMed[list[0].toInt()] = list[1];
        }
        else if (cat == cPdWide)
        {
          if (list.length() == 2)
            heroPdTagWide[list[0].toInt()] = list[1];
        }
        else if (cat == cPdItem)
        {
          if (list.length() == 2)
            itemPdTag.set(list[0], list[1]);
        }
      }
    }
    delete common;
  }

  latest = NULL;
  uint32 bestVersion = 0;
  int bestPos = -1;
  for (int i = 0; i < res->getHashSize(); i++)
  {
    if (res->fileExists(i) && res->getFileName(i))
    {
      String name = res->getFileName(i);
      Array<String> match;
      if (name.substr(0, 6) == "dota\\6")
        int asdf = 0;
      if (name.match("dota\\\\(\\d\\.\\d\\d[b-z]?).txt", &match))
      {
        uint32 ver = parseVersion(match[1]);
        if (bestPos < 0 || ver > bestVersion)
        {
          bestPos = i;
          bestVersion = ver;
        }
      }
    }
  }
  if (bestPos >= 0)
  {
    File* file = res->openFile(bestPos, File::READ);
    if (file)
    {
      latest = new Dota(bestVersion, file, this);
      latest->ref++;
      delete file;
      versions.set(bestVersion, (uint32) latest);
    }
  }
}
DotaLibrary::~DotaLibrary()
{
  for (uint32 i = versions.enumStart(); i; i = versions.enumNext(i))
    delete ((Dota*) versions.enumGetValue(i));
}

void DotaLibrary::delDota(uint32 version)
{
  delete ((Dota*) versions.del(version));
}
Dota* DotaLibrary::getDota(uint32 version)
{
  Dota* dota = (Dota*) versions.get(version);
  if (dota == NULL)
  {
    String path = String::format("dota\\%s.txt", formatVersion(version));
    File* file = getApp()->getResources()->openFile(path, File::READ);
    if (file)
    {
      dota = new Dota(version, file, this);
      delete file;
    }
    else
      return NULL;
    if (latest == NULL || version > latest->version)
    {
      if (latest)
        latest->release();
      latest = dota;
      latest->ref++;
    }
    versions.set(version, (uint32) dota);
  }
  dota->ref++;
  return dota;
}
Dota* DotaLibrary::getDota(uint32 version, String mapPath)
{
  Dota* dota = (Dota*) versions.get(version);
  if (dota == NULL)
  {
    String path = String::format("dota\\%s.txt", formatVersion(version));
    File* file = getApp()->getResources()->openFile(path, File::READ);
    if (file == NULL)
    {
      loadMap(mapPath, path);
      file = getApp()->getResources()->openFile(path, File::READ);
    }
    if (file)
    {
      dota = new Dota(version, file, this);
      delete file;
    }
    else
      return NULL;
    if (latest == NULL || version > latest->version)
    {
      if (latest)
        latest->release();
      latest = dota;
      latest->ref++;
    }
    versions.set(version, (uint32) dota);
  }
  dota->ref++;
  return dota;
}
