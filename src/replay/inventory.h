#ifndef __REPLAY_INVENTORY_H__
#define __REPLAY_INVENTORY_H__

#include "dota/dotadata.h"
#include "base/array.h"

#define ITEM_FLAG_USED      0x01
#define ITEM_FLAG_FOUND     0x02

struct W3GItem
{
  Dota::Item* item;
  uint32 time;
  uint32 flags;
};
struct W3GInventory
{
  Array<W3GItem> items;
  Array<W3GItem> comb;
  Dota::Item* final[6];
  Dota::Item* computed[6];
  int wards;

  W3GInventory();
  void addItem(Dota::Item* item, uint32 time);

  void compute(uint32 time, Dota* dota, bool combine = true);
private:
  bool hasRecipe(Dota::Recipe* recipe, uint32& time);
  W3GItem* remRecipe(Dota::Recipe* recipe);
  void clearFlags(int mask);
};

#endif // __REPLAY_INVENTORY_H__
