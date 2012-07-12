#include "inventory.h"
#include "core/app.h"

/*
static Array<ASItem> __del;
void W3GInventory::listItems (unsigned long time)
{
  bi.clear ();
  for (int i = 0; i < num_items && itemt[i] <= time; i++)
    bi.add (ASItem (items[i], itemt[i]));
}
void W3GInventory::sortItems ()
{
  for (int i = 0; i < bi.getSize (); i++)
  {
    for (int j = 1; j < bi.getSize (); j++)
    {
      if (bi[j - 1].time > bi[j].time)
      {
        ASItem tmp = bi[j - 1];
        bi[j - 1] = bi[j];
        bi[j] = tmp;
      }
    }
  }
}
void W3GInventory::getAllItems (unsigned long time, DotaData const& dota)
{
  getASItems (time, dota);
  for (int i = 0; i < __del.getSize (); i++)
  {
    __del[i].gone = true;
    bi.add (__del[i]);
  }
}

bool invGotRecipe (Array<ASItem>& m, DotaRecipe* recipe)
{
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = 0; j < m.getSize () && cnt < recipe->srccount[i]; j++)
      {
        if (!m[j].pregone && m[j].id == recipe->srcid[i])
        {
          m[j].pregone = true;
          cnt++;
        }
      }
      if (cnt < recipe->srccount[i])
        return false;
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
        if (!invGotRecipe (m, rcp))
          return false;
    }
  }
  return true;
}
unsigned long invGetTime (Array<ASItem>& m, DotaRecipe* recipe)
{
  unsigned long time = 0;
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = 0; j < m.getSize () && cnt < recipe->srccount[i]; j++)
      {
        if (!m[j].tpregone && !m[j].pregone && m[j].id == recipe->srcid[i])
        {
          m[j].tpregone = true;
          if (m[j].time >= time)
            time = m[j].time + 1;
          cnt++;
        }
      }
      if (cnt < recipe->srccount[i])
        return false;
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
      {
        unsigned long tm = invGetTime (m, rcp);
        if (tm > time)
          time = tm;
      }
    }
  }
  return time;
}
void invTRefresh (Array<ASItem>& m)
{
  for (int i = 0; i < m.getSize (); i++)
    m[i].tpregone = false;
}
int invRemRecipe (Array<ASItem>& m, DotaRecipe* recipe)
{
  unsigned long time = invGetTime (m, recipe);
  invTRefresh (m);
  for (int i = 0; i < recipe->numsrc; i++)
  {
    int sid = getItem (recipe->srcid[i])->recipe;
    if (sid < 0)
    {
      int cnt = 0;
      for (int j = m.getSize () - 1; j >= 0 && cnt < recipe->srccount[i]; j--)
      {
        if (m[j].id == recipe->srcid[i] && m[j].time <= time)
        {
          m[j].gone = true;
          m[j].pregone = false;
          __del.add (m[j]);
          m.del (j);
          cnt++;
        }
      }
    }
    else
    {
      DotaRecipe* rcp = getRecipe (sid);
      for (int j = 0; j < recipe->srccount[i]; j++)
      {
        int pos = invRemRecipe (m, rcp);
        m[pos].gone = true;
        m[pos].pregone = false;
        __del.add (m[pos]);
        m.del (pos);
      }
    }
  }
  return m.add (ASItem (recipe->result, time));
}
void invRefresh (Array<ASItem>& m)
{
  for (int i = 0; i < m.getSize (); i++)
    m[i].pregone = false;
}

void W3GInventory::getASItems (unsigned long time, DotaData const& dota)
{
  listItems (time);
  __del.clear ();

  for (int i = 0; i < getNumRecipes (); i++)
  {
    DotaRecipe* recipe = getRecipe (i);
    while (invGotRecipe (bi, recipe))
    {
      invRefresh (bi);
      invRemRecipe (bi, recipe);
      sortItems ();
      invRefresh (bi);
    }
    invRefresh (bi);
  }
}

bool W3GHero::compute (unsigned long time, DotaData const& dota)
{
  if (dota.major != 6)
    return false;

  for (int i = 0; i < 5; i++)
    levels[i] = 0;
  cur_lvl = 0;
  for (int i = 0; i < level && atime[i] <= time; i++)
  {
    cur_lvl++;
    if (abilities[i] != 0)
      levels[getAbility (abilities[i])->slot]++;
  }
  return true;
}

bool W3GInventory::compute (unsigned long time, DotaData const& dota)
{
  for (int i = 0; i < 6; i++)
    inv[i] = 0;
  if (dota.major != 6)
    return false;

  getASItems (time, dota);

  for (int s = 0; s < 6 && bi.getSize () != 0; s++)
  {
    int best = 0;
    for (int i = 1; i < bi.getSize (); i++)
      if (getItem (bi[i].id)->cost > getItem (bi[best].id)->cost)
        best = i;
    inv[s] = bi[best].id;
    bi.del (best);
  }
  return true;
}
*/

W3GInventory::W3GInventory()
{
  memset(final, 0, sizeof final);
  wards = 0;
}
void W3GInventory::addItem(Dota::Item* item, uint32 time)
{
  if (items.length() && items[items.length() - 1].item == item)
  {
    uint32 last = items[items.length() - 1].time;
    if ((item->cost > 100 && last - time < cfg::repDelayItems) ||
                            (last - time < cfg::repDelayItems / 3))
      return;
  }
  W3GItem& it = items.push();
  it.item = item;
  it.time = time;
  it.flags = 0;
  if (item->name == "Observer Wards" || item->name == "Sentry Wards")
    wards += 2;
}
