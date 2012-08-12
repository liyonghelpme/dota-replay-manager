#include "stdafx.h"
#include "dota.h"
#include "utils.h"
#include "rmpq.h"
#include "dotareplay.h"
#include "getdatadlg.h"
#include "registry.h"
#include "ilib.h"

DotaHero heroes[256];
DotaItem items[512];
DotaAbility abilities[1024];
DotaTavern taverns[32];
DotaShop shops[32];
DotaRecipe recipes[256];
int numHeroes;
int numItems;
int numAbilities;
int numTaverns;
int numShops;
int numRecipes;
static char pdTags[256][256];
static char pdTagsMed[256][256];
static char pdTagsWide[256][256];
static char abbreviations[256][256];
static char pdItemTags[256][256];
static int numPDItemTags;
static TrieNode* pdItemTagTree = NULL;
static int numVersions = 0;
static int versions[1024];

static bool usingDefault;
static int defaultVersion;

DotaHero bk_heroes[256];
DotaItem bk_items[512];
int bk_numHeroes = 0;
int bk_numItems = 0;

int getVersion (int i)
{
  if (i < 0 || i >= numVersions) return 0;
  return versions[i];
}
int getNumVersions ()
{
  return numVersions;
}
void addVersion (int ver)
{
  for (int i = 0; i < numVersions; i++)
    if (versions[i] == ver)
      return;
  versions[numVersions++] = ver;
}
static int __cdecl verSorter (void const* a, void const* b)
{
  return * (int*) a - * (int*) b;
}
void sortVersions ()
{
  qsort (versions, numVersions, sizeof versions[0], verSorter);
}

static int __cdecl recipeComp (void const* va, void const* vb)
{
  DotaRecipe* a = (DotaRecipe*) va;
  DotaRecipe* b = (DotaRecipe*) vb;
  if ((a->recipeid == 0) != (b->recipeid == 0))
    return (a->recipeid == 0 ? 1 : 0);
  DotaItem* ia = getItemById (a->result);
  DotaItem* ib = getItemById (b->result);
  if (ia == NULL || ib == NULL)
    int asdf = 0;
  return getItemById (b->result)->cost - getItemById (a->result)->cost;
}

static void _backup ()
{
  bk_numHeroes = numHeroes;
  memcpy (bk_heroes, heroes, sizeof heroes);
  bk_numItems = numItems;
  memcpy (bk_items, items, sizeof items);
}
static void _restore ()
{
  numHeroes = bk_numHeroes;
  memcpy (heroes, bk_heroes, sizeof heroes);
  numItems = bk_numItems;
  memcpy (items, bk_items, sizeof items);
}

static char* nextToken (char* str, char* dst)
{
  if (*str == '"')
  {
    str++;
    while (*str && *str != '"')
      *dst++ = *str++;
    while (*str && *str != ',')
      str++;
    if (*str)
      str++;
  }
  else
  {
    while (*str && *str != ',')
      *dst++ = *str++;
    if (*str)
      str++;
  }
  *dst = 0;
  return str;
}

void resetData ()
{
  numHeroes = 1;
  numItems = 1;
  numAbilities = 1;
  numRecipes = 0;
  memset (heroes, 0, sizeof heroes);
  memset (items, 0, sizeof items);
  memset (abilities, 0, sizeof abilities);
  memset (recipes, 0, sizeof recipes);
  strcpy (heroes[0].name, "No Hero");
  strcpy (heroes[0].imgTag, "Empty");
  heroes[0].tavern = -1;
  strcpy (items[0].name, "Empty slot");
  strcpy (items[0].imgTag, "emptyslot");
  strcpy (abilities[0].name, "None");
  strcpy (abilities[0].imgTag, "Empty");
}
static void postLoad ()
{
  for (int i = 1; i < numHeroes; i++)
  {
    if (heroes[i].index == 0) continue;
    for (int j = 0; j < 5; j++)
    {
      DotaAbility* abil = getAbilityById (heroes[i].abils[j]);
      if (abil)
      {
        heroes[i].abils[j] = abil->index;
        if (abil->hero == 0)
          abil->hero = i;
        else
          abil->hero = -1;
      }
      else
        heroes[i].abils[j] = 0;
    }
    stripstr (heroes[i].name);
    stripstr (heroes[i].oname);
    int sp_a = -1, sp_b = -1;
    for (int j = 0; heroes[i].name[j]; j++)
      if (heroes[i].name[j] == ' ')
      {sp_a = j; break;}
    for (int j = 0; heroes[i].oname[j]; j++)
      if (heroes[i].oname[j] == ' ')
      {sp_b = j; break;}
    if (sp_a <= sp_b + 1)
    {
      strcpy (heroes[i].abbr, heroes[i].name);
      if (sp_a >= 0) heroes[i].abbr[sp_a] = 0;
    }
    else
    {
      strcpy (heroes[i].abbr, heroes[i].oname);
      if (sp_b >= 0) heroes[i].abbr[sp_b] = 0;
    }
    setImageTip (heroes[i].imgTag, heroes[i].name);
  }
  for (int i = 0; i < numAbilities; i++)
  {
    if (abilities[i].hero == -1)
      abilities[i].hero = 0;
    setImageTip (abilities[i].imgTag, abilities[i].name);
  }
  qsort (recipes, numRecipes, sizeof recipes[0], recipeComp);
  for (int i = 0; i < numItems; i++)
  {
    items[i].recipe = -1;
    setImageTip (items[i].imgTag, items[i].name);
  }
  for (int i = 0; i < numRecipes; i++)
  {
    if (recipes[i].recipeid)
      recipes[i].recipeid = getItemById (recipes[i].recipeid)->index;
    recipes[i].result = getItemById (recipes[i].result)->index;
    if (recipes[i].result)
    {
      items[recipes[i].result].recipe = i;
      items[recipes[i].result].type = ITEM_COMBO;
    }
    for (int j = 0; j < recipes[i].numsrc; j++)
      recipes[i].srcid[j] = getItemById (recipes[i].srcid[j])->index;
  }
}
extern char warPath[];
bool loadCommonData ()
{
  numTaverns = 0;
  numShops = 0;
  numPDItemTags = 1;
  memset (taverns, 0, sizeof taverns);
  memset (pdTags, 0, sizeof pdTags);
  memset (pdTagsMed, 0, sizeof pdTagsMed);
  memset (pdTagsWide, 0, sizeof pdTagsWide);
  memset (pdItemTags, 0, sizeof pdItemTags);
  memset (abbreviations, 0, sizeof abbreviations);
  resetData ();
  delete pdItemTagTree;
  pdItemTagTree = NULL;
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  MPQFILE data = MPQOpenFile (res, "dota\\common.txt", MPQFILE_READ);
  char buf[256];
  char token[256];
  if (data)
  {
    MPQFileGets (data, sizeof buf, buf);
    stripstr (buf);
    while (true)
    {
      if (!strcmp (buf, "[TAVERN]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, taverns[numTaverns].name);
          ptr = nextToken (ptr, token);
          taverns[numTaverns].side = atoi (token);
          numTaverns++;
        }
      }
      else if (!strcmp (buf, "[SHOP]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          nextToken (buf, shops[numShops++].name);
        }
      }
      else if (!strcmp (buf, "[PDTAG]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          ptr = nextToken (ptr, pdTags[atoi (token)]);
        }
      }
      else if (!strcmp (buf, "[PDTAGMED]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          ptr = nextToken (ptr, pdTagsMed[atoi (token)]);
        }
      }
      else if (!strcmp (buf, "[PDTAGWIDE]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          ptr = nextToken (ptr, pdTagsWide[atoi (token)]);
        }
      }
      else if (!strcmp (buf, "[ABBREVIATION]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          ptr = nextToken (ptr, abbreviations[atoi (token)]);
        }
      }
      else if (!strcmp (buf, "[PLAYDOTAITEMTAG]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          ptr = nextToken (ptr, pdItemTags[numPDItemTags]);
          pdItemTagTree = addString (pdItemTagTree, token, numPDItemTags);
          numPDItemTags++;
        }
      }
      else
        break;
    }
    MPQCloseFile (data);

    defaultVersion = 0;
    for (uint32 i = 0; i < MPQGetHashSize (res); i++)
    {
      if (MPQFileExists (res, i))
      {
        char const* name = MPQGetFileName (res, i);
        char vertxt[8];
        if (name && !strnicmp (name, "dota\\", 5))
        {
          int len = (int) strlen (name);
          if (len > 9 && len < 16 && !stricmp (name + len - 4, ".txt"))
          {
            memcpy (vertxt, name + 5, len - 9);
            vertxt[len - 9] = 0;
            int ver = parseVersion (vertxt);
            if (ver)
            {
              versions[numVersions++] = ver;
              if (ver > defaultVersion)
                defaultVersion = ver;
            }
          }
        }
      }
    }
    if (defaultVersion != 0)
    {
      loadDotaData (defaultVersion);
      _backup ();
    }
    return true;
  }
  _backup ();
  return false;
}
bool loadDataEx (unsigned long version, char const* mappath)
{
  if (loadDotaData (version))
    return true;
  int mode = reg.readInt ("dataMode", 0);
  int copy;
  bool save = reg.readInt ("dataSave", 1) != 0;
  if (mode == 0)
  {
    CGetDataDlg dlg (true, version, mappath);
    if (dlg.DoModal () != IDOK)
      return false;
    if (dlg.mode == dlg.Closest || dlg.mode == dlg.Version)
    {
      copy = dlg.copyfrom;
      mode = 1;
    }
    else //if (dlg.mode == dlg.Used || dlg.mode == dlg.Path)
    {
      if (dlg.mode == dlg.Path)
        mappath = dlg.path;
      mode = 2;
    }
    save = dlg.saved;
    if (dlg.alwaysdo)
    {
      reg.writeInt ("dataMode", mode);
      reg.writeInt ("dataSave", save ? 1 : 0);
    }
  }
  else if (mode == 1)
    copy = getClosestVersion (version);
  if (mode == 1)
  {
    if (!loadDotaData (copy))
      return false;
    if (save)
      saveDotaData (mprintf ("dota\\%s.txt", formatVersion (version)));
  }
  else //if (mode == 2)
  {
    char mpath[256];
    if (save)
      sprintf (mpath, "dota\\%s.txt", formatVersion (version));
    else
      strcpy (mpath, "dota\\temp.txt");
    if (!parseDotaData (mpath, mappath))
      return false;
    if (!loadDotaData (mpath))
      return false;
  }
  if (save)
    addVersion (version);
  return true;
}
bool loadDataEz (unsigned long version, char const* mappath)
{
  if (loadDotaData (version))
    return true;
  char mpath[256];
  sprintf (mpath, "dota\\%s.txt", formatVersion (version));
  if (!parseDotaData (mpath, mappath))
    return false;
  if (!loadDotaData (mpath))
    return false;
  addVersion (version);
  return true;
}
bool createPrimary ()
{
  CGetDataDlg dlg (false, 0, NULL);
  if (dlg.DoModal () != IDOK)
    return false;
  char mpath[256];
  sprintf (mpath, "dota\\%s.txt", formatVersion (dlg.ver));
  if (!parseDotaData (mpath, dlg.path))
    return false;
  if (!loadDotaData (mpath))
    return false;
  addVersion (dlg.ver);
  defaultVersion = dlg.ver;
  if (!loadDotaData (dlg.ver))
    return false;
  _backup ();
  return true;
}

unsigned long getClosestVersion (unsigned long v)
{
  int best = v;
  int bid = 0;
  for (int i = 0; i < numVersions; i++)
  {
    int cur = versions[i] - v;
    if (cur < 0) cur = -cur;
    if (cur < best)
    {
      best = cur;
      bid = versions[i];
    }
  }
  return bid;
}

bool loadDotaData (unsigned long version)
{
  char path[256];
  sprintf (path, "dota\\%s.txt", formatVersion (version));
  return loadDotaData (path);
}
bool loadDotaData (char const* path)
{
  MPQFILE data = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, path, MPQFILE_READ);
  char buf[1024];
  char token[1024];
  if (data)
  {
    MPQFileGets (data, sizeof buf, buf);
    stripstr (buf);
    resetData ();
    while (true)
    {
      if (!strcmp (buf, "[HERO]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          int pos = numHeroes++;
          strcpy (heroes[pos].name, token);
          ptr = nextToken (ptr, heroes[pos].oname);
          ptr = nextToken (ptr, token);
          heroes[pos].tavern = atoi (token);
          ptr = nextToken (ptr, heroes[pos].imgTag);
          ptr = nextToken (ptr, token);
          heroes[pos].slot = atoi (token);
          ptr = nextToken (ptr, token);
          heroes[pos].point = atoi (token);
          for (int i = 0; i < 5; i++)
          {
            ptr = nextToken (ptr, token);
            heroes[pos].abils[i] = makeID (token);
          }
          while (*ptr)
          {
            ptr = nextToken (ptr, token);
            heroes[pos].ids[heroes[pos].numIds++] = makeID (token);
          }
          heroes[pos].index = pos;
        }
      }
      else if (!strcmp (buf, "[ITEM]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          int pos = numItems++;
          strcpy (items[pos].name, token);
          ptr = nextToken (ptr, token);
          items[pos].cost = atoi (token);
          ptr = nextToken (ptr, items[pos].imgTag);
          while (*ptr)
          {
            ptr = nextToken (ptr, token);
            items[pos].ids[items[pos].numIds++] = makeID (token);
          }
          items[pos].index = pos;
        }
      }
      else if (!strcmp (buf, "[ABILITY]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          int pos = numAbilities++;
          strcpy (abilities[pos].name, token);
          ptr = nextToken (ptr, token);
          abilities[pos].slot = atoi (token);
          ptr = nextToken (ptr, abilities[pos].imgTag);
          while (*ptr)
          {
            ptr = nextToken (ptr, token);
            abilities[pos].ids[abilities[pos].numIds++] = makeID (token);
          }
          abilities[pos].index = pos;
        }
      }
      else if (!strcmp (buf, "[RECIPE]"))
      {
        while (true)
        {
          MPQFileGets (data, sizeof buf, buf);
          stripstr (buf);
          if (buf[0] == '[' || MPQError ())
            break;
          char* ptr = nextToken (buf, token);
          recipes[numRecipes].result = makeID (token);
          while (*ptr)
          {
            ptr = nextToken (ptr, token);
            recipes[numRecipes].srcid[recipes[numRecipes].numsrc] = makeID (token);
            ptr = nextToken (ptr, token);
            recipes[numRecipes].srccount[recipes[numRecipes].numsrc] = atoi (token);
            recipes[numRecipes].numsrc++;
          }
          numRecipes++;
        }
      }
      else
        break;
    }
    MPQCloseFile (data);
    postLoad ();
    return true;
  }
  return false;
}
char const* getHeroId (int hero)
{
  static char empty[] = "0";
  if (hero < 0 || hero >= numHeroes || heroes[hero].numIds == 0) return empty;
  return make_id (heroes[hero].ids[0]);
}
char const* getItemId (int item)
{
  static char empty[] = "0";
  if (item < 0 || item >= numItems || items[item].numIds == 0) return empty;
  return make_id (items[item].ids[0]);
}
char const* getAbilityId (int ability)
{
  static char empty[] = "0";
  if (ability < 0 || ability >= numAbilities || abilities[ability].numIds == 0) return empty;
  return make_id (abilities[ability].ids[0]);
}
void freeDotaData ()
{
  delete pdItemTagTree;
  pdItemTagTree = NULL;
}

int getNumTaverns ()
{
  return numTaverns;
}
DotaTavern* getTavern (int i)
{
  if (i < 0 || i >= numTaverns)
    return NULL;
  return &taverns[i];
}
int getNumShops ()
{
  return numShops;
}
DotaShop* getShop (int i)
{
  if (i < 0 || i >= numShops)
    return NULL;
  return &shops[i];
}

int getNumHeroes ()
{
  return numHeroes;
}
DotaHero* getHero (int i)
{
  if (i < 0 || i >= numHeroes)
    return NULL;
  return &heroes[i];
}
DotaHero* getHeroByName (char const* n)
{
  for (int i = numHeroes - 1; i >= 0; i--)
    if (!stricmp (n, heroes[i].name))
      return &heroes[i];
  return NULL;
}
DotaHero* getHeroById (int id)
{
  for (int i = numHeroes - 1; i >= 0; i--)
    for (int j = 0; j < heroes[i].numIds; j++)
      if (heroes[i].ids[j] == id)
        return &heroes[i];
  return NULL;
}
DotaHero* getHeroByPoint (int point)
{
  for (int i = numHeroes - 1; i >= 0; i--)
    if (heroes[i].point == point)
      return &heroes[i];
  return NULL;
}
char const* getPDTag (int pv)
{
  if (pv < 0 || pv >= 256 || pdTags[pv][0] == 0) return NULL;
  return pdTags[pv];
}
char const* getPDTagMed (int pv)
{
  if (pv < 0 || pv >= 256 || pdTagsMed[pv][0] == 0) return NULL;
  return pdTagsMed[pv];
}
char const* getPDTagWide (int pv)
{
  if (pv < 0 || pv >= 256 || pdTagsWide[pv][0] == 0) return NULL;
  return pdTagsWide[pv];
}
char const* getAbbreviation (int pv)
{
  if (pv < 0 || pv >= 256 || abbreviations[pv][0] == 0) return NULL;
  return abbreviations[pv];
}
char const* getPDItemTag (char const* name)
{
  int id = getValue (pdItemTagTree, name);
  if (id == 0) return NULL;
  return pdItemTags[id];
}
bool DotaHero::matches (int id)
{
  for (int i = 0; i < numIds; i++)
    if (ids[i] == id)
      return true;
  return false;
}

int getNumItems ()
{
  return numItems;
}
DotaItem* getItem (int i)
{
  if (i < 0 || i >= numItems)
    return NULL;
  return &items[i];
}
DotaItem* getItemByName (char const* n)
{
  for (int i = numItems - 1; i >= 0; i--)
    if (!stricmp (n, items[i].name) && items[i].ids[0] != 0)
      return &items[i];
  return NULL;
}
DotaItem* getCombinedItem (char const* n)
{
  for (int i = numItems - 1; i >= 0; i--)
    if (!stricmp (n, items[i].name) && items[i].ids[0] == 0)
      return &items[i];
  return NULL;
}
DotaItem* getItemById (int id)
{
  for (int i = numItems - 1; i >= 0; i--)
    for (int j = 0; j < items[i].numIds; j++)
      if (items[i].ids[j] == id)
        return &items[i];
  return NULL;
}
bool DotaItem::matches (int id)
{
  for (int i = 0; i < numIds; i++)
    if (ids[i] == id)
      return true;
  return false;
}

int getNumAbilities ()
{
  return numAbilities;
}
DotaAbility* getAbility (int i)
{
  if (i < 0 || i >= numAbilities)
    return NULL;
  return &abilities[i];
}
DotaAbility* getAbilityById (int id)
{
  for (int i = numAbilities - 1; i >= 0; i--)
    for (int j = 0; j < abilities[i].numIds; j++)
      if (abilities[i].ids[j] == id)
        return &abilities[i];
  return NULL;
}
DotaAbility* getAbilityById (int id, int hero)
{
  for (int i = 0; i < 5; i++)
  {
    if (heroes[hero].abils[i])
    {
      DotaAbility* abil = &abilities[heroes[hero].abils[i]];
      for (int j = 0; j < abil->numIds; j++)
        if (abil->ids[j] == id)
          return abil;
    }
  }
  return NULL;
}
DotaAbility* getHeroAbility (int id, int slot)
{
  DotaHero* hero = getHero (id);
  if (hero && hero->abils[slot])
    return &abilities[hero->abils[slot]];
  return NULL;
}
bool DotaAbility::matches (int id)
{
  for (int i = 0; i < numIds; i++)
    if (ids[i] == id)
      return true;
  return false;
}

int getNumRecipes ()
{
  return numRecipes;
}
DotaRecipe* getRecipe (int i)
{
  if (i < 0 || i >= numRecipes)
    return NULL;
  return &recipes[i];
}
DotaRecipe* findRecipe (int id)
{
  for (int i = 0; i < numRecipes; i++)
    if (recipes[i].result == id)
      return &recipes[i];
  return NULL;
}

char const* getItemIcon (int i)
{
  static char const scrollImage[] = "BTNSnazzyScroll";
  if (i < 0 || i >= numItems || items[i].type == ITEM_RECIPE)
    return scrollImage;
  else
    return items[i].imgTag;
}

DWORD getSlotColor (int clr)
{
  switch (clr)
  {
  case 0:  return RGB (255, 2, 2);
  case 1:  return RGB (0, 65, 255);
  case 2:  return RGB (27, 229, 184);
  case 3:  return RGB (166, 0, 255);
  case 4:  return RGB (214, 210, 0);
  case 5:  return RGB (254, 137, 13);
  case 6:  return RGB (31, 191, 0);
  case 7:  return RGB (228, 90, 175);
  case 8:  return RGB (148, 149, 150);
  case 9:  return RGB (125, 190, 241);
  case 10: return RGB (15, 97, 69);
  case 11: return RGB (141, 77, 5);
  default: return RGB (255, 255, 255);
  }
}
DWORD getDefaultColor (int clr)
{
  switch (clr)
  {
  case 0:  return RGB (255, 2, 2);
  case 1:  return RGB (0, 65, 255);
  case 2:  return RGB (27, 229, 184);
  case 3:  return RGB (83, 0, 128);
  case 4:  return RGB (255, 252, 0);
  case 5:  return RGB (254, 137, 13);
  case 6:  return RGB (31, 191, 0);
  case 7:  return RGB (228, 90, 175);
  case 8:  return RGB (148, 149, 150);
  case 9:  return RGB (125, 190, 241);
  case 10: return RGB (15, 97, 69);
  case 11: return RGB (77, 41, 3);
  default: return RGB (255, 255, 255);
  }
}
DWORD getLightColor (int clr)
{
  DWORD c = getSlotColor (clr);
  for (int i = 0; i < 24; i += 8)
    c = (c & (~(0xFF << i))) | ((((((c >> i) & 0xFF) + 510) / 3) & 0xFF) << i);
  return c;
}
DWORD getDarkColor (int clr)
{
  DWORD c = getDefaultColor (clr);
  if ((c & 0xFF) + ((c >> 8) & 0xFF) + ((c >> 16) & 0xFF) > 383)
  {
    for (int i = 0; i < 24; i += 8)
      c = (c & (~(0xFF << i))) | (((((c >> i) & 0xFF) * 2 / 3) & 0xFF) << i);
  }
  return c;
}
DWORD getFlipColor (int clr)
{
  DWORD c = getSlotColor (clr);
  return ((c & 0x0000FF) << 16) | (c & 0x00FF00) | ((c & 0xFF0000) >> 16);
}

static char _runeColors[] = "\\red255\\green0\\blue0;" // 15 Haste
                            "\\red0\\green255\\blue0;" // 16 Regeneration
                            "\\red0\\green0\\blue255;" // 17 Double Damage
                            "\\red175\\green175\\blue0;" // 18 Illusion
                            "\\red101\\green45\\blue193;" // 19 Invisibility

                            "\\red0\\green255\\blue64;" // 20 Killing Spree
                            "\\red64\\green0\\blue128;" // 21 Dominating
                            "\\red255\\green0\\blue128;" // 22 Mega Kill
                            "\\red255\\green128\\blue0;" // 23 Unstoppable
                            "\\red128\\green128\\blue0;" // 24 Wicked Sick
                            "\\red255\\green128\\blue255;" // 25 Monster Kill
                            "\\red255\\green0\\blue0;" // 26 GODLIKE
                            "\\red255\\green128\\blue0;" // 27 Beyond GODLIKE

                            "\\red0\\green0\\blue255;" // 28 Double Kill
                            "\\red0\\green255\\blue64;" // 29 Triple Kill
                            "\\red0\\green255\\blue255;" // 30 Ultra Kill
                            "\\red0\\green170\\blue255;" // 31 Rampage
                            ;
char const* getExtraColors ()
{
  return _runeColors;
}
DWORD getExtraColor (int clr)
{
  switch (clr)
  {
  case 15: return RGB (255, 0, 0); // Haste
  case 16: return RGB (0, 255, 0); // Regeneration
  case 17: return RGB (0, 0, 255); // Double Damage
  case 18: return RGB (175, 175, 0); // Illusion
  case 19: return RGB (101, 45, 193); // Invisibility

  case 20: return RGB (0, 255, 64); // Killing Spree
  case 21: return RGB (64, 0, 128); // Dominating
  case 22: return RGB (255, 0, 128); // Mega Kill
  case 23: return RGB (255, 128, 0); // Unstoppable
  case 24: return RGB (128, 128, 0); // Wicked Sick
  case 25: return RGB (255, 128, 255); // Monster Kill
  case 26: return RGB (255, 0, 0); // GODLIKE
  case 27: return RGB (255, 128, 0); // Beyond GODLIKE

  case 28: return RGB (0, 0, 255); // Double Kill
  case 29: return RGB (0, 255, 64); // Triple Kill
  case 30: return RGB (0, 255, 255); // Ultra Kill
  case 31: return RGB (0, 170, 255); // Rampage

  default: return RGB (255, 255, 255);
  }
}

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

#include "replay.h"
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

void loadDefault ()
{
  loadDotaData (defaultVersion);
}

//struct
//{
//  int build;
//  char ver[32];
//} buildVersions[] = {
//    1.00      |     1.00.4448     |      1.0. 0.4448    |   2002-07-03
//    1.01      |     1.01.4482     |      1.0. 1.4482    |   2002-07-05
//    1.01b     |     1.01.4482     |      1.0. 1.4483    |   2002-07-10
//    1.01c     |     1.01.4482     |                ?    |   2002-07-28
//    1.02      |     1.02.4531     |      1.0. 1.4531    |   2002-08-15
//    1.02a     |     1.02.4531     |      1.0. 1.4563    |   2002-09-06
//    1.03      |     1.03.4572     |      1.0. 3.4653    |   2002-10-09
//    1.04      |     1.04.4654     |      1.0. 3.4709    |   2002-11-04
//    1.04b     |     1.04.4654     |      1.0. 3.4709    |   2002-11-07
//    1.04c     |     1.04.4654     |      1.0. 4.4905    |   2003-01-30
//    1.05      |     1.05.4654     |      1.0. 5.4944    |   2003-01-30
//    1.06      |     1.06.4656     |      1.0. 6.5551    |   2003-06-03
//    1.07      |     1.07.6031     |      1.0. 7.5535    |   2003-07-01
//    1.10      |     1.10.6034     |      1.0.10.5610    |   2003-06-30
//    1.11      |     1.11.6035     |      1.0.11.5616    |   2003-07-15
//    1.12      |     1.12.6036     |      1.0.12.5636    |   2003-07-31
//    1.13      |     1.13.6037     |      1.0.13.5816    |   2003-12-16
//    1.13b     |     1.13.6037     |      1.0.13.5818    |   2003-12-19
//    1.14      |     1.14.6039     |      1.0.14.5840    |   2004-01-07
//    1.14b     |     1.14.6040     |      1.0.14.5846    |   2004-01-10
//    1.15      |     1.15.6043     |      1.0.15.5917    |   2004-04-14
//    1.16      |     1.16.6046     |      1.0.16.5926    |   2004-05-10
//    1.17      |     1.17.6050     |      1.0.17.5988    |   2004-09-20
//    1.18      |     1.18.6051     |      1.0.18.6030    |   2005-03-01
//  {6040, "1.14b"},
//  {4482, "1.01"},
//  {4448, "1.00"},
//  {4448, "1.00"},
//  {4448, "1.00"},
//  {4448, "1.00"},
//  {4448, "1.00"},
//  {4448, "1.00"},
//};
//
#include "gamecache.h"
int getWc3Version (int major, int build)
{
  if (build == 6040)
    return parseVersion ("1.14b");
  else
    return makeVersion (1, major, 0);
}

static DotaBuilding _buildings[] = {
  {-6112, 1568, "AncientProtector"}, // level 1
  {-6112, -1248, "AncientProtector"}, // level 2
  {-6368, -4256, "AncientProtector"}, // level 3
  {-1504, -1824, "AncientProtector"}, // level 1
  {-3488, -3296, "AncientProtector"}, // level 2
  {-4448, -4960, "AncientProtector"}, // level 3
  {4960, -6752, "AncientProtector"}, // level 1
  {-544, -6688, "AncientProtector"}, // level 2
  {-3744, -6816, "AncientProtector"}, // level 3
  {-6080, -4480, "AncientOfWar"},
  {-4416, -5312, "AncientOfWar"},
  {-4032, -7040, "AncientOfWar"},
  {-6656, -4480, "AncientOfLore"},
  {-4864, -4992, "AncientOfLore"},
  {-4032, -6528, "AncientOfLore"},
  {-5600, -5728, "AncientProtector"}, // level 4
  {-5280, -6112, "AncientProtector"}, // level 4
  {-5632, -6144, "WorldTree"},

  {-4704, 5920, "SpiritTower"}, // level 1
  {32, 5920, "SpiritTower"}, // level 2
  {2976, 5792, "SpiritTower"}, // level 3
  {1056, -96, "SpiritTower"}, // level 1
  {2528, 1824, "SpiritTower"}, // level 2
  {3936, 3488, "SpiritTower"}, // level 3
  {6048, -2080, "SpiritTower"}, // level 1
  {6304, -96, "SpiritTower"}, // level 2
  {6368, 2528, "SpiritTower"}, // level 3
  {3392, 5504, "Crypt"},
  {4352, 3584, "Crypt"},
  {6656, 2880, "Crypt"},
  {3392, 6080, "TempleOfTheDamned"},
  {3904, 3904, "TempleOfTheDamned"},
  {6080, 2944, "TempleOfTheDamned"},
  {4832, 4832, "SpiritTower"}, // level 4
  {5152, 4512, "SpiritTower"}, // level 4
  {5184, 4864, "FrozenThrone"}
};
DotaBuilding* getBuildings ()
{
  return _buildings;
}
int getBuildingId (int side, int type, int lane, int level)
{
  if (side == 0)
  {
    if (type == BUILDING_TOWER)
    {
      if (level == 4)
        return BUILDING_SENTINEL_TOWER1;
      else
      {
        if (lane == 0)
        {
          if (level == 0)
            return BUILDING_SENTINEL_TOWER_TOP1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_TOP2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_TOP3;
        }
        else if (lane == 1)
        {
          if (level == 1)
            return BUILDING_SENTINEL_TOWER_MID1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_MID2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_MID3;
        }
        else if (lane == 2)
        {
          if (level == 1)
            return BUILDING_SENTINEL_TOWER_BOT1;
          else if (level == 2)
            return BUILDING_SENTINEL_TOWER_BOT2;
          else if (level == 3)
            return BUILDING_SENTINEL_TOWER_BOT3;
        }
      }
    }
    else if (type == BUILDING_MELEE)
    {
      if (lane == 0)
        return BUILDING_SENTINEL_MELEE_TOP;
      else if (lane == 1)
        return BUILDING_SENTINEL_MELEE_MID;
      else if (lane == 2)
        return BUILDING_SENTINEL_MELEE_BOT;
    }
    else if (type == BUILDING_RANGED)
    {
      if (lane == 0)
        return BUILDING_SENTINEL_RANGED_TOP;
      else if (lane == 1)
        return BUILDING_SENTINEL_RANGED_MID;
      else if (lane == 2)
        return BUILDING_SENTINEL_RANGED_BOT;
    }
    else if (type == BUILDING_THRONE)
      return BUILDING_WORLD_TREE;
  }
  else
  {
    if (type == BUILDING_TOWER)
    {
      if (level == 4)
        return BUILDING_SCOURGE_TOWER1;
      else
      {
        if (lane == 0)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_TOP1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_TOP2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_TOP3;
        }
        else if (lane == 1)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_MID1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_MID2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_MID3;
        }
        else if (lane == 2)
        {
          if (level == 1)
            return BUILDING_SCOURGE_TOWER_BOT1;
          else if (level == 2)
            return BUILDING_SCOURGE_TOWER_BOT2;
          else if (level == 3)
            return BUILDING_SCOURGE_TOWER_BOT3;
        }
      }
    }
    else if (type == BUILDING_MELEE)
    {
      if (lane == 0)
        return BUILDING_SCOURGE_MELEE_TOP;
      else if (lane == 1)
        return BUILDING_SCOURGE_MELEE_MID;
      else if (lane == 2)
        return BUILDING_SCOURGE_MELEE_BOT;
    }
    else if (type == BUILDING_RANGED)
    {
      if (lane == 0)
        return BUILDING_SCOURGE_RANGED_TOP;
      else if (lane == 1)
        return BUILDING_SCOURGE_RANGED_MID;
      else if (lane == 2)
        return BUILDING_SCOURGE_RANGED_BOT;
    }
    else if (type == BUILDING_THRONE)
      return BUILDING_FROZEN_THRONE;
  }
  return -1;
}
