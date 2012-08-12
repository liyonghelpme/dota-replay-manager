#include "stdafx.h"
#include "replay.h"

char iidsb[32][32];
int iidd = 0;
char* make_id (unsigned long id)
{
  char* iids = iidsb[iidd++];
  if (iidd >= 32) iidd = 0;
  if ((id & 0xFFFF0000) == 0x000D0000)
    sprintf (iids, "0x%08X", id);
  else
  {
    iids[3] = char (id);
    iids[2] = char (id >> 8);
    iids[1] = char (id >> 16);
    iids[0] = char (id >> 24);
    iids[4] = 0;
  }
  return iids;
}

void convert_itemid (W3GItem& item, int id)
{
  DotaHero* dhero;
  DotaAbility* dability;
  DotaItem* ditem;
  if (dhero = getHeroById (id))
  {
    item.type = ITEM_HERO;
    item.id = dhero->index;
    strcpy (item.name, dhero->name);
  }
  else if (dability = getAbilityById (id))
  {
    item.type = ITEM_ABILITY;
    item.id = dability->index;
    strcpy (item.name, dability->name);
  }
  else if (ditem = getItemById (id))
  {
    item.type = ITEM_ITEM;
    item.id = ditem->index;
    strcpy (item.name, ditem->name);
  }
  else
    item.type = ITEM_NONE;
}
