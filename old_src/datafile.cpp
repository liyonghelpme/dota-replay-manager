#include "stdafx.h"
#include "datafile.h"
void un_unicode (char* str, wchar_t* wstr = NULL);

struct SLKEntry
{
  char type;
  char val[512];
  int len;
};

static char* SLKReadEntry (char* line, SLKEntry& e)
{
  if (*line++ != ';')
    return NULL;
  e.type = *line++;
  e.len = 0;
  if (*line == '"')
  {
    line++;
    while (*line != '"' && *line)
      e.val[e.len++] = *line++;
    while (*line != ';' && *line)
      line++;
  }
  else
  {
    while (*line != ';' && *line)
      e.val[e.len++] = *line++;
  }
  e.val[e.len] = 0;
  return line;
}
static char* SLKReadType (char* line, SLKEntry& e)
{
  e.type = 0;
  e.len = 0;
  while (*line != ';' && *line)
    e.val[e.len++] = *line++;
  e.val[e.len] = 0;
  return line;
}
static char* ININextValue (char* line, char* buf)
{
  while (*line == ' ')
    line++;
  if (*line == '"')
  {
    line++;
    while (*line && *line != '"')
      *buf++ = *line++;
    *buf = 0;
    if (*line != '"')
      return NULL;
    line++;
    if (*line && *line != ',')
      return NULL;
  }
  else
  {
    while (*line && *line != ',')
      *buf++ = *line++;
    *buf = 0;
  }
  while (*line == ' ')
    line++;
  return line;
}

SLKFile::SLKFile (MPQFILE file)
{
  int sz = 1024;
  megabuff = new char[sz];
  megabuff[0] = 0;
  int length = 1;
  width = 0;
  height = 0;
  table = NULL;
  cols = NULL;

  if (file == 0) return;

  char line[1024];
  SLKEntry e;

  MPQFileSeek (file, 0, MPQSEEK_SET);
  while (MPQFileGets (file, sizeof line, line))
  {
    stripstr (line);
    char* cur = SLKReadType (line, e);
    if (!strcmp (e.val, "B"))
    {
      while (cur = SLKReadEntry (cur, e))
      {
        if (e.type == 'X')
          width = atoi (e.val);
        else if (e.type == 'Y')
          height = atoi (e.val);
      }
    }
  }
  if (width == 0 || height == 0)
    return;
  MPQFileSeek (file, 0, MPQSEEK_SET);
  table = new int[width * height];
  memset (table, 0, sizeof (int) * width * height);
  int curx = 0;
  int cury = 0;
  while (MPQFileGets (file, sizeof line, line))
  {
    stripstr (line);
    char* cur = SLKReadType (line, e);
    if (!strcmp (e.val, "C"))
    {
      while (cur = SLKReadEntry (cur, e))
      {
        if (e.type == 'X')
          curx = atoi (e.val) - 1;
        else if (e.type == 'Y')
          cury = atoi (e.val) - 1;
        else if (e.type == 'K')
        {
          if (curx >= 0 && curx < width && cury >= 0 && cury < height)
          {
            e.len++;
            if (length + e.len > sz)
            {
              while (sz < length + e.len)
                sz *= 2;
              char* tmp = new char[sz];
              memcpy (tmp, megabuff, length);
              delete[] megabuff;
              megabuff = tmp;
            }
            if (cury == 0)
              cols = addString (cols, e.val, curx + 1);
            memcpy (megabuff + length, e.val, e.len);
            table[curx + cury * width] = length;
            length += e.len;
          }
        }
      }
    }
  }
  MPQCloseFile (file);
}

SLKFile::~SLKFile ()
{
  delete[] table;
  delete[] megabuff;
  delete cols;
}

int __cdecl MetaData::idComp (void const* a, void const* b)
{
  return ((IdValue*) a)->id - ((IdValue*) b)->id;
}
MetaData::MetaData (MPQFILE file)
  : slk (file)
{
  numId = 0;
  id = NULL;
  trie = NULL;

  if (!slk.isValid ()) return;
  impRows[META_DATA_ID] = slk.getColumn ("ID");
  impRows[META_DATA_FIELD] = slk.getColumn ("field");
  impRows[META_DATA_INDEX] = slk.getColumn ("index");
  impRows[META_DATA_REPEAT] = slk.getColumn ("repeat");
  impRows[META_DATA_DATA] = slk.getColumn ("data");
  impRows[META_DATA_CATEGORY] = slk.getColumn ("category");
  impRows[META_DATA_DISPLAY] = slk.getColumn ("displayName");
  impRows[META_DATA_SORT] = slk.getColumn ("sort");
  impRows[META_DATA_TYPE] = slk.getColumn ("type");
  impRows[META_DATA_USEUNIT] = slk.getColumn ("useUnit");
  impRows[META_DATA_USEHERO] = slk.getColumn ("useHero");
  impRows[META_DATA_USEBUILDING] = slk.getColumn ("useBuilding");
  impRows[META_DATA_USEITEM] = slk.getColumn ("useItem");
  impRows[META_DATA_USESPECIFIC] = slk.getColumn ("useSpecific");
  if (impRows[META_DATA_ID] < 0 ||
      impRows[META_DATA_FIELD] < 0 ||
      impRows[META_DATA_INDEX] < 0)
    return;

  numId = slk.getNumRows ();
  id = new IdValue[numId];

  for (int i = 0; i < numId; i++)
  {
    id[i].id = makeID (slk.getItem (i, impRows[META_DATA_ID]));
    id[i].row = i;
    trie = addString (trie, slk.getItem (i, impRows[META_DATA_FIELD]), id[i].id);
  }

  qsort (id, numId, sizeof (IdValue), idComp);
}
MetaData::~MetaData ()
{
  delete[] id;
  delete trie;
}
int MetaData::getRow (int _id) const
{
  int left = 0;
  int right = numId;
  while (left <= right)
  {
    int mid = (left + right) / 2;
    if (id[mid].id == _id)
      return id[mid].row;
    if (id[mid].id < _id)
      left = mid + 1;
    else
      right = mid - 1;
  }
  return -1;
}

void WTSData::append (char c)
{
  if (length >= size)
  {
    size *= 2;
    char* tmp = new char[size];
    memcpy (tmp, megabuff, length);
    delete[] megabuff;
    megabuff = tmp;
  }
  megabuff[length++] = c;
}
WTSData::WTSData (MPQFILE file)
{
  size = 1024;
  length = 0;
  megabuff = new char[size];
  int mx = 256;
  count = 0;
  strings = new IdString[mx];
  MPQFileSeek (file, 0, MPQSEEK_SET);
  unsigned char chr[3];
  if (MPQFileRead (file, 3, chr) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    MPQFileSeek (file, 0, MPQSEEK_SET);
  char buf[256];
  while (MPQFileGets (file, sizeof buf, buf))
  {
    stripstr (buf);
    int pos = 0;
    if (!strncmp (buf, "STRING ", 7))
    {
      int id = atoi (buf + 7);
      while (MPQFileGets (file, sizeof buf, buf))
      {
        stripstr (buf);
        if (buf[0] == '{')
        {
          uint32 c;
          if (count >= mx)
          {
            mx *= 2;
            IdString* tmp = new IdString[mx];
            memcpy (tmp, strings, count * sizeof (IdString));
            delete[] strings;
            strings = tmp;
          }
          strings[count].id = id;
          strings[count++].pos = length;
          bool prevr = false;
          while ((c = MPQFileGetc (file)) && c != '}' && !MPQError ())
          {
            if ((char) c == '\r')
            {
              prevr = true;
              append ((char) c);
            }
            else
            {
              if ((char) c != '\n' || prevr == false)
            append ((char) c);
              prevr = false;
            }
          }
          length -= 2;
          append (0);
          break;
        }
      }
    }
  }
  MPQCloseFile (file);
}
WTSData::~WTSData ()
{
  delete[] strings;
  delete[] megabuff;
}
char const* WTSData::getString (int id) const
{
  int left = 0;
  int right = count;
  while (left <= right)
  {
    int mid = (left + right) / 2;
    if (strings[mid].id == id)
      return megabuff + strings[mid].pos;
    if (strings[mid].id > id)
      right = mid - 1;
    else
      left = mid + 1;
  }
  return NULL;
}

WEStrings::WEStrings ()
{
  size = 1024;
  length = 1;
  megabuff = new char[size];
  megabuff[0] = 0;
  trie = NULL;
}

void WEStrings::merge (MPQFILE file)
{
  if (file == 0) return;

  MPQFileSeek (file, 0, MPQSEEK_SET);
  unsigned char chr[3];
  if (MPQFileRead (file, 3, chr) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    MPQFileSeek (file, 0, MPQSEEK_SET);
  char buf[1024];
  while (MPQFileGets (file, sizeof buf, buf))
  {
    stripstr (buf);
    int eq = 0;
    while (buf[eq] && buf[eq] != '=')
      eq++;
    if (buf[eq])
    {
      buf[eq] = 0;
      int len = (int) strlen (buf + eq + 1) + 1;
      if (length + len > size)
      {
        while (length + len > size)
          size *= 2;
        char* tmp = new char[size];
        memcpy (tmp, megabuff, length);
        delete[] megabuff;
        megabuff = tmp;
      }
      trie = addString (trie, buf, length);
      memcpy (megabuff + length, buf + eq + 1, len);
      length += len;
    }
  }
  MPQCloseFile (file);
}
WEStrings::~WEStrings ()
{
  delete trie;
  delete[] megabuff;
}
char const* WEStrings::getString (char const* str) const
{
  int val = getValue (trie, str);
  if (val == 0)
    return NULL;
  return megabuff + val;
}

UnitData::UnitData (ObjectData* theOwner, int theId, UnitData* theBase)
{
  owner = theOwner;
  size = 256;
  megabuff = new char[size];
  megabuff[0] = 0;
  length = 1;
  ptrsize = 256;
  ptr = new int[ptrsize];
  memset (ptr, 0, ptrsize * sizeof (int));
  id = theId;
  base = theBase;
  ref = 1;
  while (base && base->base)
    base = base->base;
  if (base)
    base->ref++;
}
UnitData::~UnitData ()
{
  if (base && --base->ref == 0)
    delete base;
  delete[] megabuff;
  delete[] ptr;
}
void UnitData::release ()
{
  if (this == NULL) return;
  if (--ref == 0)
    delete this;
}
int nextComma (char const* str, int c)
{
  if (c >= 0 && str[c] == 0) return -1;
  bool instr = false;
  int i;
  for (i = c + 1; str[i]; i++)
  {
    if (str[i] == '"')
      instr = !instr;
    if (str[i] == ',' && !instr)
      return i;
  }
  return i;
}
void UnitData::alloc (int len)
{
  if (length + len > size)
  {
    while (length + len > size)
      size *= 2;
    char* tmp = new char[size];
    memcpy (tmp, megabuff, length);
    delete[] megabuff;
    megabuff = tmp;
  }
}
void UnitData::mklen (int x)
{
  if (x >= ptrsize)
  {
    int nptrsize = ptrsize;
    while (x >= nptrsize)
      nptrsize *= 2;
    int* tmp = new int[nptrsize];
    memset (tmp, 0, nptrsize * sizeof (int));
    memcpy (tmp, ptr, ptrsize * sizeof (int));
    delete[] ptr;
    ptr = tmp;
    ptrsize = nptrsize;
  }
}
void UnitData::setData (int x, char const* data, int index)
{
  if (index < 0)
  {
    int len = (int) strlen (data) + 1;
    alloc (len);
    mklen (x);
    memcpy (megabuff + length, data, len);
    ptr[x] = length;
    length += len;
  }
  else
  {
    int count = 0;
    int cur = -1;
    int prev = 0;
    int old = x < ptrsize ? ptr[x] : 0;
    char const* oldbase = NULL;
    if (old == 0 && base && x < base->ptrsize && base->ptr[x])
    {
      old = base->ptr[x];
      oldbase = base->megabuff;
    }
    char const* oldptr = (oldbase ? oldbase : megabuff) + old;
    int olen = (int) strlen (oldptr);
    int nlen = (int) strlen (data);
    if (olen)
    {
      while ((cur = nextComma (oldptr, cur)) >= 0)
      {
        if (count == index)
        {
          alloc (olen + nlen + 1 - (cur - prev));
          mklen (x);
          if (oldbase == NULL) oldbase = megabuff;
          ptr[x] = length;
          if (prev)
            memcpy (megabuff + length, oldbase + old, prev);
          length += prev;
          if (nlen)
            memcpy (megabuff + length, data, nlen);
          length += nlen;
          memcpy (megabuff + length, oldbase + old + cur, olen - cur + 1);
          length += olen - cur + 1;
          return;
        }
        prev = cur + 1;
        count++;
      }
      alloc (olen + nlen + index - count + 2);
      mklen (x);
      if (oldbase == NULL) oldbase = megabuff;
      ptr[x] = length;
      memcpy (megabuff + length, oldbase + old, olen);
      length += olen;
      while (count++ <= index)
        megabuff[length++] = ',';
      memcpy (megabuff + length, data, nlen + 1);
      length += nlen + 1;
    }
    else
    {
      alloc (nlen + index - count + 1);
      mklen (x);
      ptr[x] = length;
      while (count++ < index)
        megabuff[length++] = ',';
      memcpy (megabuff + length, data, nlen + 1);
      length += nlen + 1;
    }
  }
}
void UnitData::compress ()
{
  size = 1024;
  length = 1;
  char* oldbuff = megabuff;
  megabuff = new char[size];
  megabuff[0] = 0;

  for (int i = 0; i < ptrsize; i++)
  {
    if (ptr[i])
    {
      int len = (int) strlen (oldbuff + ptr[i]) + 1;
      alloc (len);
      memcpy (megabuff + length, oldbuff + ptr[i], len);
      ptr[i] = length;
      length += len;
    }
  }

  delete[] oldbuff;
}

ObjectData::ObjectData (WEStrings* we)
  : cols (NULL, 0)
{
  wes = we;
  dir = NULL;
  size = 32;
  count = 1;
  units = new UnitData* [size];
  units[0] = NULL;
  fields = NULL;
  numFields = 0;
}
ObjectData::~ObjectData ()
{
  for (int i = 0; i < count; i++)
    units[i]->release ();
  delete[] units;
  delete dir;
  delete fields;
}
UnitData* ObjectData::addUnit (int id, int base)
{
  int pos = getValue (dir, make_id (id));
  if (pos) return units[pos];
  if (count >= size)
  {
    size *= 2;
    UnitData** tmp = new UnitData* [size];
    memcpy (tmp, units, count * sizeof (UnitData*));
    delete[] units;
    units = tmp;
  }
  UnitData* bu = NULL;
  if (base)
    bu = getUnitById (base);
  units[count] = new UnitData (this, id, bu);
  dir = addString (dir, make_id (id), count);
  return units[count++];
}
UnitData* ObjectData::addUnit (char const* id, int base)
{
  int pos = getValue (dir, id);
  if (pos) return units[pos];
  if (count >= size)
  {
    size *= 2;
    UnitData** tmp = new UnitData* [size];
    memcpy (tmp, units, count * sizeof (UnitData*));
    delete[] units;
    units = tmp;
  }
  UnitData* bu = NULL;
  if (base)
    bu = getUnitById (base);
  units[count] = new UnitData (this, makeID (id), bu);
  dir = addString (dir, id, count);
  return units[count++];
}
void ObjectData::setUnitData (UnitData* unit, char const* field, char const* data, int index)
{
  if (unit == NULL) return;
  int x = getValue (fields, field);
  if (x == 0)
  {
    x = numFields++;
    fields = addString (fields, field, x);
    cols.setData (x, field);
  }
  unit->setData (x, data, index);
}

bool ObjectData::readSLK (MPQFILE file)
{
  SLKFile slk (file);
  if (!slk.isValid ())
    return false;
  int* colid = new int[slk.getNumColumns ()];
  for (int i = 1; i < slk.getNumColumns (); i++)
  {
    char const* field = slk.getColumn (i);
    colid[i] = getValue (fields, field);
    if (colid[i] == 0)
    {
      colid[i] = numFields++;
      fields = addString (fields, field, colid[i]);
      cols.setData (colid[i], field);
    }
  }
  for (int i = 0; i < slk.getNumRows (); i++)
  {
    UnitData* data = addUnit (slk.getItem (i, 0));
    for (int j = 1; j < slk.getNumColumns (); j++)
      if (slk.isItemSet (i, j))
        data->setData (colid[j], translate (slk.getItem (i, j)));
  }
  delete[] colid;
  return true;
}
bool ObjectData::readINI (MPQFILE file, bool split)
{
  if (file == 0) return false;
  char buf[4096];
  UnitData* cur = NULL;
  MPQFileSeek (file, 0, MPQSEEK_SET);
  unsigned char chr[3];
  if (MPQFileRead (file, 3, chr) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    MPQFileSeek (file, 0, MPQSEEK_SET);
  bool ok = true;
  while (ok && MPQFileGets (file, sizeof buf, buf))
  {
    stripstr (buf);
    if (buf[0] == '[')
    {
      if (buf[5] != ']')
        ok = false;
      else
      {
        buf[5] = 0;
        cur = getUnitById (buf + 1);
      }
    }
    else if (cur && buf[0] && buf[0] != '/')
    {
      int eq = 0;
      while (buf[eq] && buf[eq] != '=')
        eq++;
      if (buf[eq] != 0)
      {
        buf[eq] = 0;
        setUnitData (cur, buf, translate (buf + eq + 1));
      }
    }
  }
  MPQCloseFile (file);
  return true;
}
bool ObjectData::readOBJ (MPQFILE file, MetaData* meta, bool ext, WTSData* wts)
{
  if (file == 0 || meta == NULL || !meta->isValid ()) return false;
  MPQFileSeek (file, 4, MPQSEEK_SET);
  bool err = false;
  char buf[256];
  char str[4096];
  for (int tbl = 0; tbl < 2 && !MPQError () && !err; tbl++)
  {
    uint32 count = MPQReadInt (file);
    if (count > 5000) err = true;
    for (uint32 i = 0; i < count && !MPQError () && !err; i++)
    {
      uint32 oldid = flipInt (MPQReadInt (file));
      uint32 newid = flipInt (MPQReadInt (file));
      UnitData* unit = NULL;
      if (tbl)
        unit = addUnit (newid, oldid);
      else
      {
        int upos = getValue (dir, make_id (oldid));
        if (units[upos] == NULL)
        {
          err = true;
          break;
        }
        else
        {
          unit = new UnitData (this, oldid, units[upos]);
          units[upos]->release ();
          units[upos] = unit;
        }
      }
      uint32 count = MPQReadInt (file);
      if (count > 500) err = true;
      for (uint32 j = 0; j < count && !err; j++)
      {
        uint32 modid = flipInt (MPQReadInt (file));
        uint32 type = MPQReadInt (file);
        int index;
        char const* mod = meta->getValue (modid, &index);
        if (mod == NULL || type > 3)
          err = true;
        else
        {
          if (ext)
          {
            uint32 level = MPQReadInt (file);
            uint32 data = MPQReadInt (file);
            strcpy (buf, mod);
            mod = buf;
            int len = (int) strlen (buf);
            if (!strcmp (buf, "Data"))
            {
              buf[len++] = char ('A' + data);
              buf[len] = 0;
            }
            if (level)
            {
              if (index < 0)
                strcat (buf, mprintf ("%d", level));
              else
                index += level - 1;
            }
          }
          if (type == 0)
          {
            uint32 val = MPQReadInt (file);
            sprintf (str, "%d", val);
          }
          else if (type == 3)
          {
            MPQReadString (file, str);
            if (wts && !strncmp (str, "TRIGSTR_", 8))
            {
              char const* rep = wts->getString (atoi (str + 8));
              if (rep)
                strcpy (str, rep);
            }
          }
          else
          {
            float val = MPQReadFloat (file);
            sprintf (str, "%.2f", val);
          }
          setUnitData (unit, mod, translate (str), index);
          uint32 suf = flipInt (MPQReadInt (file));
          if (suf != 0 && suf != newid && suf != oldid)
            err = true;
        }
      }
      unit->compress ();
    }
  }
  MPQCloseFile (file);
  return !err;
}

void ObjectData::dump (FILE* f)
{
  for (int i = 0; i < count; i++)
  {
    if (units[i])
    {
      if (units[i]->getBase ())
        fprintf (f, "[%s:%s]\n", make_id (units[i]->getID ()), make_id (units[i]->getBase ()->getID ()));
      else
        fprintf (f, "[%s]\n", make_id (units[i]->getID ()));
      for (int j = 0; j < numFields; j++)
        if (units[i]->isDataSet (j))
          fprintf (f, "%s=%s\n", cols.getData (j), units[i]->getData (j));
    }
  }
}

char const* ObjectData::getUnitString (UnitData const* unit, char const* field, int index) const
{
  char const* src = unit->getData (getValue (fields, field));
  char* dst = mprintf ("");
  if (index < 0)
  {
    int cmp = nextComma (src, -1);
    if (cmp >= 2 && src[cmp] == 0 && src[0] == '"' && src[cmp - 1] == '"')
    {
      memcpy (dst, src + 1, cmp - 2);
      dst[cmp - 2] = 0;
    }
    else
      strcpy (dst, src);
  }
  else
  {
    int pos = -1;
    int cur = 0;
    int prev = 0;
    while ((pos = nextComma (src, pos)) >= 0)
    {
      if (cur == index)
      {
        if (pos - prev >= 2 && src[prev] == '"' && src[pos - 1] == '"')
        {
          memcpy (dst, src + prev + 1, pos - prev - 2);
          dst[pos - prev - 2] = 0;
        }
        else
        {
          memcpy (dst, src + prev, pos - prev);
          dst[pos - prev] = 0;
        }
        break;
      }
      prev = pos + 1;
      cur++;
    }
  }
  return dst;
}

void LoadGameData (GameData& game, MPQLOADER loader, int mode)
{
  if ((mode & WC3_LOAD_ALL) == 0) return;

  WTSData wts (MPQLoadFile (loader, "war3map.wts"));
  if (mode & (WC3_LOAD_DESTRUCTABLES | WC3_LOAD_DOODADS))
  {
    game.wes.merge (MPQLoadFile (loader, "UI\\WorldEditGameStrings.txt"));
    if (!(mode & WC3_LOAD_NO_WEONLY))
      game.wes.merge (MPQLoadFile (loader, "UI\\WorldEditStrings.txt"));
  }

  if (!(mode & WC3_LOAD_MERGED))
  {
    if (mode & WC3_LOAD_UNITS)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_UNITS] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\UnitAbilities.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitBalance.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitData.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\unitUI.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitWeapons.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUnitFunc.txt"));
      game.metaData[WC3_UNITS] = new MetaData (MPQLoadFile (loader, "Units\\UnitMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3u"), game.metaData[WC3_UNITS], false, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_UNITS];
        game.metaData[WC3_UNITS] = NULL;
      }
      else
        game.metaData[WC3_ITEMS] = game.metaData[WC3_UNITS];
    }

    if (mode & WC3_LOAD_ITEMS)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_ITEMS] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\ItemData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\ItemFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\ItemStrings.txt"));
      if (game.metaData[WC3_UNITS] == NULL)
        game.metaData[WC3_UNITS] = new MetaData (MPQLoadFile (loader, "Units\\UnitMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3t"), game.metaData[WC3_UNITS], false, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_UNITS];
        game.metaData[WC3_UNITS] = NULL;
      }
      else
        game.metaData[WC3_ITEMS] = game.metaData[WC3_UNITS];
    }

    if (mode & WC3_LOAD_DESTRUCTABLES)
    {
      ObjectData* data = new ObjectData (&game.wes);
      game.data[WC3_DESTRUCTABLES] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\DestructableData.slk"));
      game.metaData[WC3_DESTRUCTABLES] = new MetaData (MPQLoadFile (loader, "Units\\DestructableMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3b"), game.metaData[WC3_DESTRUCTABLES], false, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_DESTRUCTABLES];
        game.metaData[WC3_DESTRUCTABLES] = NULL;
      }
    }

    if (mode & WC3_LOAD_DOODADS)
    {
      ObjectData* data = new ObjectData (&game.wes);
      game.data[WC3_DOODADS] = data;
      data->readSLK (MPQLoadFile (loader, "Doodads\\Doodads.slk"));
      game.metaData[WC3_DOODADS] = new MetaData (MPQLoadFile (loader, "Doodads\\DoodadMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3d"), game.metaData[WC3_DOODADS], true, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_DOODADS];
        game.metaData[WC3_DOODADS] = NULL;
      }
    }

    if (mode & WC3_LOAD_ABILITIES)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_ABILITIES] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\AbilityData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityStrings.txt"), true);
      game.metaData[WC3_ABILITIES] = new MetaData (MPQLoadFile (loader, "Units\\AbilityMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3a"), game.metaData[WC3_ABILITIES], true, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_ABILITIES];
        game.metaData[WC3_ABILITIES] = NULL;
      }
    }

    if (mode & WC3_LOAD_BUFFS)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_BUFFS] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\AbilityBuffData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityStrings.txt"), true);
      game.metaData[WC3_BUFFS] = new MetaData (MPQLoadFile (loader, "Units\\AbilityBuffMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3h"), game.metaData[WC3_BUFFS], false, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_BUFFS];
        game.metaData[WC3_BUFFS] = NULL;
      }
    }

    if (mode & WC3_LOAD_UPGRADES)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_UPGRADES] = data;
      data->readSLK (MPQLoadFile (loader, "Units\\UpgradeData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUpgradeStrings.txt"));
      game.metaData[WC3_UPGRADES] = new MetaData (MPQLoadFile (loader, "Units\\UpgradeMetaData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3q"), game.metaData[WC3_UPGRADES], true, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_UPGRADES];
        game.metaData[WC3_UPGRADES] = NULL;
      }
    }
  }
  else
  {
    ObjectData* data = new ObjectData (&game.wes);
    game.merged = data;
    if (mode & (WC3_LOAD_UNITS | WC3_LOAD_ITEMS))
    {
      game.metaData[WC3_UNITS] = new MetaData (MPQLoadFile (loader, "Units\\UnitMetaData.slk"));
      game.metaData[WC3_ITEMS] = game.metaData[WC3_UNITS];
    }
    if (mode & WC3_LOAD_DESTRUCTABLES)
      game.metaData[WC3_DESTRUCTABLES] = new MetaData (MPQLoadFile (loader, "Units\\DestructableMetaData.slk"));
    if (mode & WC3_LOAD_DOODADS)
      game.metaData[WC3_DOODADS] = new MetaData (MPQLoadFile (loader, "Doodads\\DoodadMetaData.slk"));
    if (mode & WC3_LOAD_ABILITIES)
      game.metaData[WC3_ABILITIES] = new MetaData (MPQLoadFile (loader, "Units\\AbilityMetaData.slk"));
    if (mode & WC3_LOAD_BUFFS)
      game.metaData[WC3_BUFFS] = new MetaData (MPQLoadFile (loader, "Units\\AbilityBuffMetaData.slk"));
    if (mode & WC3_LOAD_UPGRADES)
      game.metaData[WC3_UPGRADES] = new MetaData (MPQLoadFile (loader, "Units\\UpgradeMetaData.slk"));

    if (mode & WC3_LOAD_UNITS)
    {
      data->readSLK (MPQLoadFile (loader, "Units\\UnitAbilities.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitBalance.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitData.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\unitUI.slk"));
      data->readSLK (MPQLoadFile (loader, "Units\\UnitWeapons.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUnitFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUnitStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUnitFunc.txt"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3u"), game.metaData[WC3_UNITS], false, &wts);
    }
    if (mode & WC3_LOAD_ITEMS)
    {
      data->readSLK (MPQLoadFile (loader, "Units\\ItemData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\ItemFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\ItemStrings.txt"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3t"), game.metaData[WC3_UNITS], false, &wts);
    }
    if (mode & WC3_LOAD_DESTRUCTABLES)
    {
      data->readSLK (MPQLoadFile (loader, "Units\\DestructableData.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3b"), game.metaData[WC3_DESTRUCTABLES], false, &wts);
    }
    if (mode & WC3_LOAD_DOODADS)
    {
      data->readSLK (MPQLoadFile (loader, "Doodads\\Doodads.slk"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3d"), game.metaData[WC3_DOODADS], true, &wts);
    }
    if (mode & WC3_LOAD_ABILITIES)
      data->readSLK (MPQLoadFile (loader, "Units\\AbilityData.slk"));
    if (mode & WC3_LOAD_BUFFS)
      data->readSLK (MPQLoadFile (loader, "Units\\AbilityBuffData.slk"));
    if (mode & (WC3_LOAD_ABILITIES | WC3_LOAD_BUFFS))
    {
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\UndeadAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CampaignAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\CommonAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\HumanAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\ItemAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NeutralAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\NightElfAbilityStrings.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityFunc.txt"), true);
      data->readINI (MPQLoadFile (loader, "Units\\OrcAbilityStrings.txt"), true);
    }
    if (mode & WC3_LOAD_ABILITIES)
      data->readOBJ (MPQLoadFile (loader, "war3map.w3a"), game.metaData[WC3_ABILITIES], true, &wts);
    if (mode & WC3_LOAD_BUFFS)
      data->readOBJ (MPQLoadFile (loader, "war3map.w3h"), game.metaData[WC3_BUFFS], false, &wts);
    if (mode & WC3_LOAD_UPGRADES)
    {
      data->readSLK (MPQLoadFile (loader, "Units\\UpgradeData.slk"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NightElfUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\OrcUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\UndeadUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\CampaignUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\NeutralUpgradeStrings.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUpgradeFunc.txt"));
      data->readINI (MPQLoadFile (loader, "Units\\HumanUpgradeStrings.txt"));
      data->readOBJ (MPQLoadFile (loader, "war3map.w3q"), game.metaData[WC3_UPGRADES], true, &wts);
    }
    if (!(mode & WC3_LOAD_KEEP_METADATA))
    {
      game.metaData[WC3_ITEMS] = NULL;
      for (int i = 0; i < NUM_WC3_TYPES; i++)
      {
        delete game.metaData[i];
        game.metaData[i] = NULL;
      }
    }
  }
}
