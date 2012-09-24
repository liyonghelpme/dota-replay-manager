#include "inventory.h"
#include "core/app.h"

W3GInventory::W3GInventory()
{
  memset(final, 0, sizeof final);
  memset(computed, 0, sizeof computed);
  wards = 0;
}
void W3GInventory::addItem(Dota::Item* item, uint32 time)
{
  if (items.length() && items[items.length() - 1].item == item)
  {
    uint32 last = items[items.length() - 1].time;
    if ((item->cost > 100 && last - time < cfg.repDelayItems) ||
                            (last - time < cfg.repDelayItems / 3))
      return;
  }
  W3GItem& it = items.push();
  it.item = item;
  it.time = time;
  it.flags = 0;
  if (item->name == "Observer Wards" || item->name == "Sentry Wards")
    wards += 2;
}

static int CompareItems(W3GItem const& a, W3GItem const& b)
{
  return int(a.time) - int(b.time);
}

void W3GInventory::clearFlags(int mask)
{
  for (int i = 0; i < comb.length(); i++)
    comb[i].flags &= ~mask;
}

bool W3GInventory::hasRecipe(Dota::Recipe* recipe, uint32& time)
{
  for (int i = 0; i < recipe->numSources; i++)
  {
    Dota::Recipe* sub = recipe->sources[i]->recipe;
    if (sub)
    {
      for (int j = 0; j < recipe->sourceCount[i]; j++)
        if (!hasRecipe(sub, time))
          return false;
    }
    else
    {
      int count = 0;
      for (int j = 0; j < comb.length() && count < recipe->sourceCount[i]; j++)
      {
        if (comb[j].flags == 0 && comb[j].item == recipe->sources[i])
        {
          comb[j].flags |= ITEM_FLAG_FOUND;
          if (comb[j].time >= time)
            time = comb[j].time + 1;
          count++;
        }
      }
      if (count < recipe->sourceCount[i])
        return false;
    }
  }
  return true;
}
W3GItem* W3GInventory::remRecipe(Dota::Recipe* recipe)
{
  uint32 time = 0;
  clearFlags(ITEM_FLAG_FOUND);
  if (!hasRecipe(recipe, time))
    return NULL;
  clearFlags(ITEM_FLAG_FOUND);
  for (int i = 0; i < recipe->numSources; i++)
  {
    Dota::Recipe* sub = recipe->sources[i]->recipe;
    if (sub)
    {
      for (int j = 0; j < recipe->sourceCount[i]; j++)
        remRecipe(sub)->flags |= ITEM_FLAG_USED;
    }
    else
    {
      int count = 0;
      for (int j = comb.length() - 1; j >= 0 && count < recipe->sourceCount[i]; j--)
      {
        if (comb[j].flags == 0 && comb[j].item == recipe->sources[i] && comb[j].time < time)
        {
          comb[j].flags |= ITEM_FLAG_USED;
          count++;
        }
      }
    }
  }
  W3GItem& item = comb.push();
  item.item = recipe->result;
  item.time = time;
  item.flags = 0;
  return &item;
}
void W3GInventory::compute(uint32 time, Dota* dota, bool combine)
{
  if (dota == NULL)
    return;

  comb.clear();
  for (int i = 0; i < items.length() && items[i].time <= time; i++)
    comb.push(items[i]);

  if (!combine)
    return;

  for (int i = 0; i < dota->getNumRecipes(); i++)
  {
    Dota::Recipe* recipe = dota->getRecipe(i);
    while (remRecipe(recipe))
      ;
  }

  comb.sort(CompareItems);
  for (int i = 0; i < 6; i++)
    computed[i] = NULL;
  for (int i = 0; i < 6; i++)
  {
    W3GItem* best = NULL;
    for (int j = 0; j < comb.length(); j++)
      if (comb[j].flags == 0 && (best == NULL || comb[j].item->cost > best->item->cost))
        best = &comb[j];
    if (best)
    {
      best->flags |= ITEM_FLAG_FOUND;
      computed[i] = best->item;
    }
    else
      break;
  }
  clearFlags(ITEM_FLAG_FOUND);
}
