#include "stdafx.h"
#include "dotaload.h"
#include "ilib.h"

extern MPQLOADER warloader;
extern char warPath[];
static TrieNode256* dir;
static TrieNode* idir;
bool useropen = false;

static int __curid;
static void reset_id ()
{
  __curid = 'Xx00';
}
static int new_id ()
{
  if ((__curid & 0xFF) == '9')
    __curid = (__curid & (~0xFF)) + 'A';
  else if ((__curid & 0xFF) == 'Z')
    __curid = (__curid & (~0xFF)) + 0x100 + '0';
  else
    __curid++;
  return __curid;
}

static char* strip_item (char const* item, int len = 1000000)
{
  char* res;
  if (!strncmp (item, "Disabled ", strlen ("Disabled ")))
    res = mprintf ("%s", item + strlen ("Disabled "));
  else
    res = mprintf ("%s", item);
  int shift = 0;
  int i;
  for (i = 0; res[i] && i < len; i++)
  {
    if (res[i] == ' ' && shift == i)
      shift++;
    else if (res[i] == '|')
    {
      if (res[i + 1] == 'r' || res[i + 1] == 'n')
      {
        shift += 2;
        i++;
      }
      else if (res[i + 1] == 'c')
      {
        shift += 10;
        i += 9;
      }
      else
        res[i - shift] = res[i];
    }
    else if ((res[i] == '-' && i && res[i - 1] == ' ') || (res[i] == '(' && (res[i + 1] < '0' || res[i + 1] > '9')))
    {
      res[i] = 0;
      break;
    }
    else
      res[i - shift] = res[i];
  }
  i -= shift;
  res[i] = 0;
  while (i && res[i - 1] == ' ')
    res[--i] = 0;
  if (i >= 6 && !stricmp (res + i - 6, "Recipe"))
  {
    i -= 6;
    res[i] = 0;
    while (i && res[i - 1] == ' ')
      res[--i] = 0;
  }
  return res;
}

static bool addSoldItem (UnitData* item)
{
  int gold = item->getIntData ("goldcost");
  int wood = item->getIntData ("lumbercost");
  char const* name = strip_item (item->getStringData ("Name"));
  char const* descr = item->getStringData ("Ubertip");
  char const* reqstr = strstr (descr, "Requires:");
  if ((wood || gold == 0) && reqstr == NULL)
    return false;
  int pos = sgetValue (idir, name) - 1;
  if (pos < 0)
  {
    pos = numItems;
    items[pos].index = numItems++;
    items[pos].ids[0] = item->getID ();
    items[pos].numIds = 1;
    items[pos].realid = pos;
    items[pos].cost = gold;
    if (wood)
      items[pos].cost = 0;
    strcpy (items[pos].name, name);
    _splitpath (item->getStringData ("Art"), NULL, NULL, items[pos].imgTag, NULL);
    addNewImage (item->getStringData ("Art"), false);
    if (reqstr == NULL)
    {
      items[pos].type = ITEM_NORMAL;
      idir = saddString (idir, name, pos + 1);
    }
    else
    {
      items[pos].type = ITEM_RECIPE;
      items[pos].realid = numItems;
      items[numItems].index = numItems;
      items[numItems].ids[0] = new_id ();
      items[numItems].numIds = 1;
      items[numItems].realid = pos;
      items[numItems].type = ITEM_COMBO;
      items[numItems].cost = 0;
      strcpy (items[numItems].name, items[pos].name);
      strcpy (items[numItems].imgTag, items[pos].imgTag);
      idir = saddString (idir, name, numItems + 1);
      numItems++;
      strcpy (items[pos].imgTag, "BTNSnazzyScroll");
    }
    return true;
  }
  return false;
}
static int addAbility (UnitData* abil)
{
  DotaAbility* ptr = &abilities[numAbilities];
  ptr->index = numAbilities++;
  ptr->ids[0] = abil->getID ();
  ptr->numIds = 1;
  strcpy (ptr->name, abil->getStringData ("Name"));
  char const* art = abil->getStringData ("ResearchArt");
  if (art == NULL || *art == 0)
    art = abil->getStringData ("Researchart");
  _splitpath (art, NULL, NULL, ptr->imgTag, NULL);
  if (!strcmp (ptr->imgTag, "BTNFeedback"))
    int asdf = 0;
  addNewImage (art, false);
  int rx = atoi (abil->getStringData ("Researchbuttonpos", 0));
  int ry = atoi (abil->getStringData ("Researchbuttonpos", 1));
  if (ry)
    ptr->slot = 4;
  else
    ptr->slot = rx;
  return ptr->index * 5 + ptr->slot;
}
static void addEngineering (UnitData* eng)
{
  char fields[4][16] = {"DataCx", "DataDx", "DataEx", "DataFx"};
  int levels = eng->getIntData ("levels");
  for (int i = 0; i < levels; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      fields[j][5] = '1' + i;
      char const* src = eng->getStringData (fields[j], 0);
      char const* dst = eng->getStringData (fields[j], 1);
      int srci = getValue (dir, src);
      int abil = (srci - 1) / 5;
      if (srci > 0 && abil < numAbilities)
      {
        int abil = (srci - 1) / 5;
        int dstv = makeID (dst);
        bool hasID = false;
        for (int k = 0; k < abilities[abil].numIds; k++)
          if (abilities[abil].ids[k] == dstv)
            hasID = true;
        if (!hasID)
        {
          abilities[abil].ids[abilities[abil].numIds++] = dstv;
          dir = addString (dir, dst, srci);
        }
      }
    }
  }
}

static void addHero (ObjectData* data, UnitData* hero, int tavern)
{
  DotaHero* ptr = &heroes[numHeroes];
  ptr->index = numHeroes++;
  ptr->tavern = tavern;
  ptr->ids[0] = hero->getID ();
  ptr->numIds = 1;
  ptr->slot = atoi (hero->getStringData ("Buttonpos", 0)) +
    4 * atoi (hero->getStringData ("Buttonpos", 1));
  ptr->point = atoi (hero->getStringData ("points"));
  strcpy (ptr->name, hero->getStringData ("Name"));
  strcpy (ptr->oname, hero->getStringData ("Propernames", 0));
  _splitpath (hero->getStringData ("Art"), NULL, NULL, ptr->imgTag, NULL);
  addNewImage (hero->getStringData ("Art"), true);
  if (!strcmp (ptr->name, "Silencer"))
    int asd = 0;
  for (int i = 0; i < 5; i++)
  {
    char const* abils = hero->getStringData ("heroAbilList", i);
    if (*abils)
    {
      int slot = getValue (dir, abils);
      if (slot == 0)
      {
        UnitData* abil = data->getUnitById (abils);
        if (abil)
          slot = addAbility (abil);
        dir = addString (dir, abils, slot + 1);
      }
      else
        slot--;
      if (slot >= 0)
        ptr->abils[slot % 5] = makeID (abils);
    }
  }
}

const int numOldTaverns = 17;
char oldTaverns[numOldTaverns][256] = {
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

static int _getItemByName (char const* name)
{
  char* iname = strip_item (name);
  int id = sgetValue (idir, iname) - 1;
  if (id >= 0)
    return items[id].ids[0];
  else
    return 0;
}

void mergeItemName (UnitData* item)
{
  char const* id = make_id (item->getID ());
  char* name = strip_item (item->getStringData ("Name"));
  if (!strcmp (name, "Ultimate Orb"))
    int asdf = 0;
  int len = (int) strlen (name);
  if (len >= 7 && !strnicmp (name + len - 7, "Level ", 6))
  {
    name[len - 7] = name[len - 1];
    name[len - 6] = 0;
  }
  if (len >= 4 && name[len - 1] == ')' && (name[len - 3] == '(' || name[len - 4] == '('))
  {
    if (name[len - 4] == '(')
      len -= 5;
    else
      len -= 4;
    name[len] = 0;
  }
  int pos = sgetValue (idir, name) - 1;
  if (pos >= 0 && getValue (dir, id) == 0)
  {
    dir = addString (dir, id, 1);
    char const* abils = item->getData ("abilList");
    if ((abils[0] == 0 || abils[0] == '_') && stricmp (item->getData ("class"), "Campaign"))
      pos = items[pos].realid;
    items[pos].ids[items[pos].numIds] = item->getID ();
    items[pos].numIds++;
  }
}

bool parseDotaData (char const* dest, char const* path)
{
  MPQARCHIVE map = MPQOpen (path, MPQFILE_READ);
  if (map == 0)
    map = MPQOpen (mprintf ("%s%s", warPath, path), MPQFILE_READ);
  if (map == 0)
  {
    if (useropen)
      MessageBox (NULL, mprintf ("Could not locate %s", path), "Error", MB_OK | MB_ICONHAND);
    return false;
  }
  MPQAddArchive (warloader, map);

  CProgressDlg* prg = ((CDotAReplayApp*) ::AfxGetApp ())->progress;
  char title[256];
  _splitpath (path, NULL, NULL, title, NULL);
  prg->SetSupText (mprintf ("Parsing %s", title), 0);
  prg->SetSubText ("Parsing object data...");
  prg->show ();

  GameData data;
  LoadGameData (data, warloader, WC3_LOAD_UNITS | WC3_LOAD_ITEMS | WC3_LOAD_ABILITIES);

  resetData ();

  bool ok = true;

  dir = NULL;
  idir = NULL;

  reset_id ();

  int tavernCount = 0;
  prg->SetSubText ("Parsing taverns and shops...", 40);
  for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits (); i++)
  {
    UnitData* tav = data.data[WC3_UNITS]->getUnit (i);
    if (tav != NULL)
    {
      char const* name = tav->getStringData ("Name");
      for (int ti = 0; ti < numTaverns; ti++)
      {
        if (!stricmp (taverns[ti].name, name))
        {
          tavernCount++;
          char const* herolist = tav->getData ("Sellunits");
          for (int p = 0; herolist[p];)
          {
            static char _id[5];
            int idl = 0;
            for (; idl < 4 && herolist[p]; idl++, p++)
              _id[idl] = herolist[p];
            if (idl == 4)
            {
              _id[4] = 0;
              if (getValue (dir, _id) == 0)
              {
                UnitData* hero = data.data[WC3_UNITS]->getUnitById (_id);
                if (hero)
                  addHero (data.data[WC3_ABILITIES], hero, ti);
                dir = addString (dir, _id, 1);
              }
            }
            if (herolist[p] == ',')
              p++;
            else
              break;
          }
        }
      }
      for (int si = 0; si < numShops; si++)
      {
        if (!stricmp (shops[si].name, name))
        {
          for (int m = 0; m < 2; m++)
          {
            char const* itemlist = tav->getData (m ? "Sellitems" : "Sellunits");
            for (int p = 0; itemlist[p];)
            {
              static char _id[5];
              int idl = 0;
              for (; idl < 4 && itemlist[p]; idl++, p++)
                _id[idl] = itemlist[p];
              if (idl == 4)
              {
                _id[4] = 0;
                if (getValue (dir, _id) == 0)
                {
                  UnitData* item = data.data[m ? WC3_ITEMS : WC3_UNITS]->getUnitById (_id);
                  if (item)
                    if (addSoldItem (item))
                      dir = addString (dir, _id, 1);
                }
              }
              if (itemlist[p] == ',')
                p++;
              else
                break;
            }
          }
          break;
        }
      }
    }
  }
  if (tavernCount == 0)
  {
    for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits (); i++)
    {
      UnitData* tav = data.data[WC3_UNITS]->getUnit (i);
      if (tav != NULL)
      {
        char const* name = tav->getStringData ("Name");
        for (int ti = 0; ti < numOldTaverns; ti++)
        {
          if (!stricmp (oldTaverns[ti], name))
          {
            char const* herolist = tav->getData ("Sellunits");
            for (int p = 0; herolist[p];)
            {
              static char _id[5];
              int idl = 0;
              for (; idl < 4 && herolist[p]; idl++, p++)
                _id[idl] = herolist[p];
              if (idl == 4)
              {
                _id[4] = 0;
                if (getValue (dir, _id) == 0)
                {
                  UnitData* hero = data.data[WC3_UNITS]->getUnitById (_id);
                  if (hero)
                    addHero (data.data[WC3_ABILITIES], hero, ti);
                  dir = addString (dir, _id, 1);
                }
              }
              if (herolist[p] == ',')
                p++;
              else
                break;
            }
          }
        }
      }
    }
  }
  prg->SetSubText ("Parsing items...", 60);
  for (int i = 0; i < numItems; i++)
  {
    if (items[i].index == 0) continue;
    if (items[i].type == ITEM_RECIPE)
    {
      // parse recipe
      UnitData* unit = data.data[WC3_ITEMS]->getUnitById (items[i].ids[0]);
      if (unit == NULL)
        unit = data.data[WC3_UNITS]->getUnitById (items[i].ids[0]);
      if (unit)
      {
        char const* descr = strstr (unit->getStringData ("Ubertip"), "Requires:");
        if (descr && (strstr (descr, "Any of the following") ||
                      strstr (descr, "Staff of Wizardry and Blade of Alacrity")))
        {
          // scepter
          int scepter = items[items[i].realid].ids[0];
          int recipe = items[i].ids[0];
          int booster = _getItemByName ("Point Booster");
          int axe = _getItemByName ("Ogre Axe");
          int blade = _getItemByName ("Blade of Alacrity");
          int staff = _getItemByName ("Staff of Wizardry");
          recipes[numRecipes].recipeid = recipe;
          recipes[numRecipes].result = scepter;
          recipes[numRecipes].numsrc = 4;
          recipes[numRecipes].srcid[0] = recipe;
          recipes[numRecipes].srccount[0] = 1;
          recipes[numRecipes].srcid[1] = booster;
          recipes[numRecipes].srccount[1] = 1;
          recipes[numRecipes].srcid[2] = axe;
          recipes[numRecipes].srccount[2] = 1;
          recipes[numRecipes].srcid[3] = blade;
          recipes[numRecipes].srccount[3] = 1;
          numRecipes++;
          recipes[numRecipes].recipeid = recipe;
          recipes[numRecipes].result = scepter;
          recipes[numRecipes].numsrc = 4;
          recipes[numRecipes].srcid[0] = recipe;
          recipes[numRecipes].srccount[0] = 1;
          recipes[numRecipes].srcid[1] = booster;
          recipes[numRecipes].srccount[1] = 1;
          recipes[numRecipes].srcid[2] = axe;
          recipes[numRecipes].srccount[2] = 1;
          recipes[numRecipes].srcid[3] = staff;
          recipes[numRecipes].srccount[3] = 1;
          numRecipes++;
          recipes[numRecipes].recipeid = recipe;
          recipes[numRecipes].result = scepter;
          recipes[numRecipes].numsrc = 4;
          recipes[numRecipes].srcid[0] = recipe;
          recipes[numRecipes].srccount[0] = 1;
          recipes[numRecipes].srcid[1] = booster;
          recipes[numRecipes].srccount[1] = 1;
          recipes[numRecipes].srcid[2] = staff;
          recipes[numRecipes].srccount[2] = 1;
          recipes[numRecipes].srcid[3] = blade;
          recipes[numRecipes].srccount[3] = 1;
          numRecipes++;
        }
        else if (descr)
        {
          recipes[numRecipes].numsrc = 0;
          recipes[numRecipes].recipeid = 0;
          if (items[i].cost)
          {
            recipes[numRecipes].srccount[0] = 1;
            recipes[numRecipes].srcid[0] = items[i].ids[0];
            recipes[numRecipes].numsrc = 1;
            recipes[numRecipes].recipeid = items[i].ids[0];
          }
          recipes[numRecipes].result = items[items[i].realid].ids[0];
          int num = 0;
          int pos = 0;
          int prev = 0;
          bool prevOr = false;
          while (true)
          {
            if (descr[pos] == 0 || (descr[pos] == '|' && descr[pos + 1] == 'n'))
            {
              if (num)
              {
                char* iname = strip_item (descr + prev, pos - prev);
                int count = 1;
                bool hasx = false;
                if (*iname >= '0' && *iname <= '9')
                {
                  count = atoi (iname);
                  while (*iname && *iname != ' ')
                  {
                    if (*iname == 'x')
                      hasx = true;
                    iname++;
                  }
                  while (*iname == ' ')
                    iname++;
                }
                if (count > 1)
                {
                  int len = (int) strlen (iname);
                  if (len && iname[len - 1] == 's')
                    iname[--len] = 0;
                }
                int upgrLen = (int) strlen ("Buy recipe to upgrade (");
                if (!stricmp (iname, "or"))
                  prevOr = true;
                else if (!strnicmp (iname, "Buy recipe to upgrade (", upgrLen))
                {
                  int levels = atoi (iname + upgrLen);
                  int iid = items[i].realid;
                  for (int lvl = 2; lvl <= levels; lvl++)
                  {
                    numRecipes++;
                    recipes[numRecipes].numsrc = 2;
                    recipes[numRecipes].srcid[0] = recipes[numRecipes - 1].srcid[0];
                    recipes[numRecipes].srccount[0] = 1;
                    recipes[numRecipes].srcid[1] = recipes[numRecipes - 1].result;
                    recipes[numRecipes].srccount[1] = 1;
                    items[numItems].ids[0] = new_id ();
                    recipes[numRecipes].result = items[numItems].ids[0];
                    items[numItems].numIds = 1;
                    sprintf (items[numItems].name, "%s %d", items[iid].name, lvl);
                    strcpy (items[numItems].imgTag, items[iid].imgTag);
                    items[numItems].index = numItems;
                    items[numItems].type = ITEM_COMBO;
                    items[numItems].realid = i;
                    items[numItems].cost = 0;
                    idir = saddString (idir, items[numItems].name, numItems + 1);
                    numItems++;
                  }
                  strcat (items[iid].name, " 1");
                  idir = saddString (idir, items[iid].name, iid + 1);
                }
                else
                {
                  int cpos = sgetValue (idir, iname) - 1;
                  if (cpos >= 0)
                  {
                    if (prevOr)
                    {
                      memcpy (&recipes[numRecipes + 1], &recipes[numRecipes], sizeof recipes[0]);
                      numRecipes++;
                      recipes[numRecipes].srcid[recipes[numRecipes].numsrc - 1] = items[cpos].ids[0];
                      recipes[numRecipes].srccount[recipes[numRecipes].numsrc - 1] = count;
                    }
                    else
                    {
                      recipes[numRecipes].srcid[recipes[numRecipes].numsrc] = items[cpos].ids[0];
                      recipes[numRecipes].srccount[recipes[numRecipes].numsrc] = count;
                      recipes[numRecipes].numsrc++;
                    }
                    prevOr = false;
                  }
                }
              }
              num++;
              if (descr[pos] == 0)
                break;
              pos += 2;
              prev = pos;
            }
            else
              pos++;
          }
          if (recipes[numRecipes].numsrc)
            numRecipes++;
        }
      }
    }
  }
  for (int i = 0; i < data.data[WC3_ITEMS]->getNumUnits (); i++)
  {
    UnitData* item = data.data[WC3_ITEMS]->getUnit (i);
    if (item != NULL)
      mergeItemName (item);
  }
  for (int i = 0; i < data.data[WC3_UNITS]->getNumUnits (); i++)
  {
    UnitData* item = data.data[WC3_UNITS]->getUnit (i);
    if (item != NULL)
      mergeItemName (item);
  }

  prg->SetSubText ("Parsing upgradeable abilities...", 80);
  for (int i = 0; i < data.data[WC3_ABILITIES]->getNumUnits (); i++)
  {
    UnitData* abil = data.data[WC3_ABILITIES]->getUnit (i);
    if (abil != NULL && !strcmp (abil->getStringData ("code"), "ANeg"))
      addEngineering (abil);
  }

  prg->SetSubText ("Computing recipe costs...", 90);
  for (int i = 0; i < numRecipes; i++)
  {
    int res = getItemById (recipes[i].result)->index;
    if (res > 0 && items[res].type >= 0)
    {
      items[res].cost = 0;
      items[res].type = -items[res].type;
    }
  }
  for (int maxc = 0; maxc < 20; maxc++)
  {
    bool total = true;
    for (int i = 0; i < numRecipes; i++)
    {
      int res = getItemById (recipes[i].result)->index;
      if (res > 0 && items[res].type < 0)
      {
        bool ok = true;
        items[res].cost = 0;
        for (int j = 0; j < recipes[i].numsrc; j++)
        {
          int comp = getItemById (recipes[i].srcid[j])->index;
          if (comp > 0)
          {
            if (items[comp].type < 0)
            {
              ok = false;
              break;
            }
            else
              items[res].cost += items[comp].cost * recipes[i].srccount[j];
          }
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
  //FILE* log = fopen ("recipes.txt", "wt");
  //for (int i = 0; i < numRecipes; i++)
  //{
  //  int res = getItemById (recipes[i].result)->index;
  //  if (res <= 0) continue;
  //  fprintf (log, "%s =", items[res].name);
  //  for (int j = 0; j < recipes[i].numsrc; j++)
  //  {
  //    int comp = getItemById (recipes[i].srcid[j])->index;
  //    if (comp > 0)
  //      fprintf (log, " %s x%d", items[comp].name, recipes[i].srccount[j]);
  //  }
  //  fprintf (log, "\n");
  //}
  //fclose (log);

  delete dir;
  delete idir;
  MPQRemoveArchive (warloader, map);
  MPQClose (map);
  prg->hide ();

  if (!ok) return false;
  return saveDotaData (dest);
}

bool samename (char const* a, char const* b)
{
  while (true)
  {
    while (*a && (*a < 'a' || *a > 'z') && (*a < 'A' || *a > 'Z') && (*a < '0' || *a > '9'))
      a++;
    while (*b && (*b < 'a' || *b > 'z') && (*b < 'A' || *b > 'Z') && (*b < '0' || *b > '9'))
      b++;
    char ca = *a;
    if (ca >= 'a' && ca <= 'z') ca += 'A' - 'a';
    char cb = *b;
    if (cb >= 'a' && cb <= 'z') cb += 'A' - 'a';
    if (ca != cb) return false;
    if (ca == 0) return true;
    a++;
    b++;
  }
}

bool saveDotaData (char const* dest)
{
  MPQFILE file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, dest, MPQFILE_REWRITE);
  if (file)
  {
    static int hsub[256];
    static int isub[512];
    memset (hsub, 0, sizeof hsub);
    memset (isub, 0, sizeof isub);
    int mxh = bk_numHeroes;
    for (int i = 1; i < numHeroes; i++)
    {
      bool found = false;
      for (int j = 1; j < bk_numHeroes && !found; j++)
      {
        if (samename (heroes[i].name, bk_heroes[j].name))
        {
          hsub[j] = i;
          found = true;
        }
      }
      if (!found)
        hsub[mxh++] = i;
    }
    int mxi = bk_numItems;
    for (int i = 1; i < numItems; i++)
    {
      bool found = false;
      for (int j = 1; j < bk_numItems && !found; j++)
      {
        if (items[i].type/2 == bk_items[j].type/2 && samename (items[i].name, bk_items[j].name))
        {
          isub[j] = i;
          found = true;
        }
      }
      if (!found)
        isub[mxi++] = i;
    }
    MPQFilePuts (file, "[HERO]\r\n");
    for (int m = 0; m < mxh; m++)
    {
      int i = hsub[m];
      DotaHero* hero = &heroes[i];
      MPQFilePuts (file, mprintf ("\"%s\",\"%s\",%d,\"%s\",%d,%d", hero->name, hero->oname, hero->tavern,
        hero->imgTag, hero->slot, hero->point));
      for (int j = 0; j < 5; j++)
        MPQFilePuts (file, mprintf (",%s", make_id (hero->abils[j])));
      for (int j = 0; j < hero->numIds; j++)
        MPQFilePuts (file, mprintf (",%s", make_id (hero->ids[j])));
      MPQFilePuts (file, "\r\n");
    }
    MPQFilePuts (file, "[ITEM]\r\n");
    for (int m = 0; m < mxi; m++)
    {
      int i = isub[m];
      DotaItem* item = getItem (i);
      MPQFilePuts (file, mprintf ("\"%s\",%d,\"%s\"", item->name, item->cost, item->imgTag));
      for (int j = 0; j < item->numIds; j++)
        MPQFilePuts (file, mprintf (",%s", make_id (item->ids[j])));
      MPQFilePuts (file, "\r\n");
    }
    MPQFilePuts (file, "[ABILITY]\r\n");
    for (int i = 1; i < numAbilities; i++)
    {
      DotaAbility* abil = getAbility (i);
      MPQFilePuts (file, mprintf ("\"%s\",%d,\"%s\"", abil->name, abil->slot, abil->imgTag));
      for (int j = 0; j < abil->numIds; j++)
        MPQFilePuts (file, mprintf (",%s", make_id (abil->ids[j])));
      MPQFilePuts (file, "\r\n");
    }
    MPQFilePuts (file, "[RECIPE]\r\n");
    for (int i = 0; i < numRecipes; i++)
    {
      DotaRecipe* recipe = getRecipe (i);
      MPQFilePuts (file, make_id (recipe->result));
      for (int j = 0; j < recipe->numsrc; j++)
        MPQFilePuts (file, mprintf (",%s,%d", make_id (recipe->srcid[j]), recipe->srccount[j]));
      MPQFilePuts (file, "\r\n");
    }
    MPQCloseFile (file);
  }
  else
    return false;
  MPQFlushListfile (((CDotAReplayApp*) ::AfxGetApp ())->res);
  return true;
}
