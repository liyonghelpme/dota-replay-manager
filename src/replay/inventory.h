#ifndef __REPLAY_INVENTORY_H__
#define __REPLAY_INVENTORY_H__

#include "dota/dotadata.h"
#include "base/array.h"

struct W3GItem
{
  Dota::Item* item;
  uint32 time;
  uint32 flags;
};
struct W3GInventory
{
  Array<W3GItem> items;
  Array<W3GItem> combinedItems;
  Dota::Item* final[6];
  int wards;

  W3GInventory();
  void addItem(Dota::Item* item, uint32 time);
};

#endif // __REPLAY_INVENTORY_H__
