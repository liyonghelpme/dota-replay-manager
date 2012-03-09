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
          uint32(str[0]);
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

Dota::Dota(uint32 ver, DotaLibrary* lib)
{
  version = ver;
  library = lib;
  ref = 0;

  load(String::format("dota\\%s.txt", formatVersion(ver)));
}
Dota::Dota(uint32 ver, String path, DotaLibrary* lib)
{
  version = ver;
  library = lib;
  ref = 0;

  load(path);
}
static int __cdecl recipeComp(void const* va, void const* vb)
{
  Dota::Recipe* a = (Dota::Recipe*) va;
  Dota::Recipe* b = (Dota::Recipe*) vb;
  return b->result->cost - a->result->cost;
}
void Dota::load(String path)
{
  for (int i = 0; i < MAX_HERO_POINT; i++)
    heroes[i].id = 0;
  int numItems = 1;
  int numAbilities = 1;
  heroes[0].name = "No Hero";
  heroes[0].icon = "Empty";
  items[0].name = "Empty slot";
  items[0].icon = "emptyslot";
  abilities[0].name = "None";
  abilities[0].icon = "Empty";

  numRecipes = 0;
  File* file = getApp()->getResources()->openFile(path, File::READ);
  if (file)
  {
    String buf;
    int posHero = -1;
    int posAbility = -1;
    int posItem = -1;
    int posRecipe = -1;
    while (!(buf = file->gets()).isEmpty())
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
      while (!(buf = file->gets()).isEmpty())
      {
        buf.trim();
        if (buf[0] == '[')
          continue;
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
      while (!(buf = file->gets()).isEmpty())
      {
        buf.trim();
        if (buf[0] == '[')
          continue;
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
      while (!(buf = file->gets()).isEmpty())
      {
        buf.trim();
        if (buf[0] == '[')
          continue;
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
      while (!(buf = file->gets()).isEmpty())
      {
        buf.trim();
        if (buf[0] == '[')
          continue;
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
    delete file;

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
void Dota::release()
{
  if (this && --ref <= 0)
    library->delDota(version);
}

DotaLibrary::DotaLibrary()
{
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
    dota = new Dota(version, this);
    versions.set(version, (uint32) dota);
  }
  dota->ref++;
  return dota;
}
