#include "datafile.h"
#include "base/utils.h"

struct SLKEntry
{
  char type;
  String val;
};

static char const* SLKReadEntry(char const* line, SLKEntry& e)
{
  if (*line++ != ';')
    return NULL;
  e.type = *line++;
  e.val = "";
  if (*line == '"')
  {
    line++;
    while (*line != '"' && *line)
      e.val += *line++;
    while (*line != ';' && *line)
      line++;
  }
  else
  {
    while (*line != ';' && *line)
      e.val += *line++;
  }
  return line;
}
static char const* SLKReadType(char const* line, SLKEntry& e)
{
  e.type = 0;
  e.val = "";
  while (*line != ';' && *line)
    e.val += *line++;
  return line;
}

SLKFile::SLKFile(File* file)
{
  int sz = 1024;
  megabuff = new char[sz];
  megabuff[0] = 0;
  int length = 1;
  width = 0;
  height = 0;
  table = NULL;

  if (file == NULL) return;

  String line;
  SLKEntry e;

  file->seek(0, SEEK_SET);
  while (file->gets(line))
  {
    line.trim();
    char const* cur = SLKReadType(line, e);
    if (e.val == "B")
    {
      while (cur = SLKReadEntry(cur, e))
      {
        if (e.type == 'X')
          width = e.val.toInt();
        else if (e.type == 'Y')
          height = e.val.toInt();
      }
    }
  }
  if (width == 0 || height == 0)
    return;
  file->seek(0, SEEK_SET);
  table = new int[width * height];
  memset(table, 0, sizeof(int) * width * height);
  int curx = 0;
  int cury = 0;
  while (file->gets(line))
  {
    line.trim();
    char const* cur = SLKReadType(line, e);
    if (e.val == "C")
    {
      while (cur = SLKReadEntry(cur, e))
      {
        if (e.type == 'X')
          curx = e.val.toInt() - 1;
        else if (e.type == 'Y')
          cury = e.val.toInt() - 1;
        else if (e.type == 'K')
        {
          if (curx >= 0 && curx < width && cury >= 0 && cury < height)
          {
            int len = e.val.length() + 1;
            if (length + len > sz)
            {
              while (sz < length + len)
                sz *= 2;
              char* tmp = new char[sz];
              memcpy(tmp, megabuff, length);
              delete[] megabuff;
              megabuff = tmp;
            }
            if (cury == 0)
              cols.set(e.val, curx);
            memcpy(megabuff + length, e.val.c_str(), len);
            table[curx + cury * width] = length;
            length += len;
          }
        }
      }
    }
  }
}
SLKFile::~SLKFile()
{
  delete[] table;
  delete[] megabuff;
}

int __cdecl MetaData::idComp(void const* a, void const* b)
{
  return ((IdValue*) a)->id - ((IdValue*) b)->id;
}
MetaData::MetaData(File* file)
  : slk(file)
{
  numId = 0;
  id = NULL;

  if (!slk.isValid()) return;
  impRows[META_DATA_ID] = slk.getColumn("ID");
  impRows[META_DATA_FIELD] = slk.getColumn("field");
  impRows[META_DATA_INDEX] = slk.getColumn("index");
  impRows[META_DATA_REPEAT] = slk.getColumn("repeat");
  impRows[META_DATA_DATA] = slk.getColumn("data");
  impRows[META_DATA_CATEGORY] = slk.getColumn("category");
  impRows[META_DATA_DISPLAY] = slk.getColumn("displayName");
  impRows[META_DATA_SORT] = slk.getColumn("sort");
  impRows[META_DATA_TYPE] = slk.getColumn("type");
  impRows[META_DATA_USEUNIT] = slk.getColumn("useUnit");
  impRows[META_DATA_USEHERO] = slk.getColumn("useHero");
  impRows[META_DATA_USEBUILDING] = slk.getColumn("useBuilding");
  impRows[META_DATA_USEITEM] = slk.getColumn("useItem");
  impRows[META_DATA_USESPECIFIC] = slk.getColumn("useSpecific");
  if (impRows[META_DATA_ID] < 0 ||
      impRows[META_DATA_FIELD] < 0 ||
      impRows[META_DATA_INDEX] < 0)
    return;

  numId = slk.getNumRows();
  id = new IdValue[numId];
  for (int i = 0; i < numId; i++)
  {
    id[i].id = idFromString(slk.getItem(i, impRows[META_DATA_ID]));
    id[i].row = i;
//    data.set(slk.getItem(i, impRows[META_DATA_FIELD]), id[i].id);
  }

  qsort(id, numId, sizeof(IdValue), idComp);
}
MetaData::~MetaData()
{
  delete[] id;
}
int MetaData::getRow(int _id) const
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

void WTSData::append(char c)
{
  if (length >= size)
  {
    size *= 2;
    char* tmp = new char[size];
    memcpy(tmp, megabuff, length);
    delete[] megabuff;
    megabuff = tmp;
  }
  megabuff[length++] = c;
}
WTSData::WTSData(File* file)
{
  size = 1024;
  length = 0;
  megabuff = new char[size];
  int mx = 256;
  count = 0;
  strings = new IdString[mx];

  if (file == NULL) return;
  file->seek(0, SEEK_SET);
  uint8 chr[3];
  if (file->read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    file->seek(0, SEEK_SET);
  String line;
  while (file->gets(line))
  {
    line.trim();
    int pos = 0;
    if (!strncmp(line, "STRING ", 7))
    {
      int id = atoi(line.c_str() + 7);
      while (file->gets(line))
      {
        line.trim();
        if (line[0] == '{')
        {
          if (count >= mx)
          {
            mx *= 2;
            IdString* tmp = new IdString[mx];
            memcpy(tmp, strings, count * sizeof(IdString));
            delete[] strings;
            strings = tmp;
          }
          strings[count].id = id;
          strings[count++].pos = length;
          bool prevr = false;
          int c;
          while ((c = file->getc()) && c != '}')
          {
            if (c == '\r')
            {
              prevr = true;
              append(c);
            }
            else
            {
              if (c != '\n' || prevr == false)
                append(c);
              prevr = false;
            }
          }
          length -= 2;
          append(0);
          break;
        }
      }
    }
  }
}
WTSData::~WTSData()
{
  delete[] strings;
  delete[] megabuff;
}
char const* WTSData::getString(int id) const
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

WEStrings::WEStrings()
{
  size = 1024;
  length = 1;
  megabuff = new char[size];
  megabuff[0] = 0;
}
void WEStrings::merge(File* file)
{
  if (file == NULL) return;
  file->seek(0, SEEK_SET);
  uint8 chr[3];
  if (file->read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    file->seek(0, SEEK_SET);
  String line;
  while (file->gets(line))
  {
    line.trim();
    int eq = line.find('=');
    if (eq >= 0)
    {
      int len = line.length() - eq;
      if (length + len > size)
      {
        while (length + len > size)
          size *= 2;
        char* tmp = new char[size];
        memcpy(tmp, megabuff, length);
        delete[] megabuff;
        megabuff = tmp;
      }
      data.set(line.substring(0, eq), length);
      memcpy(megabuff + length, line.c_str() + eq + 1, len);
      length += len;
    }
  }
}
WEStrings::~WEStrings()
{
  delete[] megabuff;
}
char const* WEStrings::getString(char const* str) const
{
  if (data.has(str))
    return megabuff + data.get(str);
  return NULL;
}

UnitData::UnitData(ObjectData* theOwner, int theId, UnitData* theBase)
{
  owner = theOwner;
  size = 256;
  megabuff = new char[size];
  megabuff[0] = 0;
  length = 1;
  ptrsize = 256;
  ptr = new int[ptrsize];
  memset(ptr, 0, ptrsize * sizeof(int));
  id = theId;
  base = theBase;
  ref = 1;
  while (base && base->base)
    base = base->base;
  if (base)
    base->ref++;
}
UnitData::~UnitData()
{
  if (base && --base->ref == 0)
    delete base;
  delete[] megabuff;
  delete[] ptr;
}
void UnitData::release()
{
  if (this == NULL) return;
  if (--ref == 0)
    delete this;
}
static int nextComma(char const* str, int c)
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
void UnitData::alloc(int len)
{
  if (length + len > size)
  {
    while (length + len > size)
      size *= 2;
    char* tmp = new char[size];
    memcpy(tmp, megabuff, length);
    delete[] megabuff;
    megabuff = tmp;
  }
}
void UnitData::mklen(int x)
{
  if (x >= ptrsize)
  {
    int nptrsize = ptrsize;
    while (x >= nptrsize)
      nptrsize *= 2;
    int* tmp = new int[nptrsize];
    memset(tmp, 0, nptrsize * sizeof(int));
    memcpy(tmp, ptr, ptrsize * sizeof(int));
    delete[] ptr;
    ptr = tmp;
    ptrsize = nptrsize;
  }
}
void UnitData::setData(int x, char const* data, int index)
{
  if (index < 0)
  {
    int len = strlen(data) + 1;
    alloc(len);
    mklen(x);
    memcpy(megabuff + length, data, len);
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
    int olen = strlen(oldptr);
    int nlen = strlen(data);
    if (olen)
    {
      while ((cur = nextComma(oldptr, cur)) >= 0)
      {
        if (count == index)
        {
          alloc(olen + nlen + 1 - (cur - prev));
          mklen(x);
          if (oldbase == NULL) oldbase = megabuff;
          ptr[x] = length;
          if (prev)
            memcpy(megabuff + length, oldbase + old, prev);
          length += prev;
          if (nlen)
            memcpy(megabuff + length, data, nlen);
          length += nlen;
          memcpy(megabuff + length, oldbase + old + cur, olen - cur + 1);
          length += olen - cur + 1;
          return;
        }
        prev = cur + 1;
        count++;
      }
      alloc(olen + nlen + index - count + 2);
      mklen(x);
      if (oldbase == NULL) oldbase = megabuff;
      ptr[x] = length;
      memcpy(megabuff + length, oldbase + old, olen);
      length += olen;
      while (count++ <= index)
        megabuff[length++] = ',';
      memcpy(megabuff + length, data, nlen + 1);
      length += nlen + 1;
    }
    else
    {
      alloc(nlen + index - count + 1);
      mklen(x);
      ptr[x] = length;
      while (count++ < index)
        megabuff[length++] = ',';
      memcpy(megabuff + length, data, nlen + 1);
      length += nlen + 1;
    }
  }
}
void UnitData::compress()
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
      int len = strlen(oldbuff + ptr[i]) + 1;
      alloc(len);
      memcpy(megabuff + length, oldbuff + ptr[i], len);
      ptr[i] = length;
      length += len;
    }
  }

  delete[] oldbuff;
}

ObjectData::ObjectData(WEStrings* we)
  : cols(NULL, 0)
{
  wes = we;
  size = 32;
  count = 1;
  units = new UnitData*[size];
  units[0] = NULL;
  numFields = 0;
}
ObjectData::~ObjectData()
{
  for (int i = 0; i < count; i++)
    units[i]->release();
  delete[] units;
}
UnitData* ObjectData::addUnit(uint32 id, uint32 base)
{
  if (dir.has(id))
    return units[dir.get(id)];
  if (count >= size)
  {
    size *= 2;
    UnitData** tmp = new UnitData*[size];
    memcpy(tmp, units, count * sizeof(UnitData*));
    delete[] units;
    units = tmp;
  }
  UnitData* bu = NULL;
  if (base)
    bu = getUnitById(base);
  units[count] = new UnitData(this, id, bu);
  dir.set(id, count);
  return units[count++];
}
void ObjectData::setUnitData(UnitData* unit, char const* field, char const* data, int index)
{
  if (unit == NULL) return;
  int col;
  if (fields.has(field))
    col = fields.get(field);
  else
  {
    col = numFields++;
    fields.set(field, col);
    cols.setData(col, field);
  }
  unit->setData(col, data, index);
}

bool ObjectData::readSLK(File* file)
{
  SLKFile slk(file);
  if (!slk.isValid())
    return false;
  int* colid = new int[slk.getNumColumns()];
  for (int i = 1; i < slk.getNumColumns(); i++)
  {
    char const* field = slk.getColumn(i);
    if (fields.has(field))
      colid[i] = fields.get(field);
    else
    {
      colid[i] = numFields++;
      fields.set(field, colid[i]);
      cols.setData(colid[i], field);
    }
  }
  for (int i = 0; i < slk.getNumRows(); i++)
  {
    UnitData* data = addUnit(idFromString(slk.getItem(i, 0)));
    for (int j = 1; j < slk.getNumColumns(); j++)
      if (slk.isItemSet(i, j))
        data->setData(colid[j], translate(slk.getItem(i, j)));
  }
  delete[] colid;
  return true;
}
bool ObjectData::readINI(File* file, bool split)
{
  if (file == NULL) return false;
  file->seek(0, SEEK_SET);
  uint8 chr[3];
  if (file->read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF)
    file->seek(0, SEEK_SET);

  UnitData* cur = NULL;
  String line;
  bool ok = true;
  while (ok && file->gets(line))
  {
    line.trim();
    if (line[0] == '[')
    {
      if (line.length() >= 6 && line[5] != ']')
        ok = false;
      else
        cur = getUnitById(idFromString(line.substring(1, 5)));
    }
    else if (cur && line[0] && line[0] != '/')
    {
      int eq = line.find('=');
      if (eq >= 0)
        setUnitData(cur, line.substring(0, eq), translate(line.c_str() + eq + 1));
    }
  }
  return true;
}
bool ObjectData::readOBJ(File* file, MetaData* meta, bool ext, WTSData* wts)
{
  if (file == NULL || meta == NULL || !meta->isValid ()) return false;
  file->seek(4, SEEK_SET);
  bool err = false;
  String mod;
  for (int tbl = 0; tbl < 2 && !file->eof() && !err; tbl++)
  {
    uint32 count = file->read_int32();
    if (count > 5000) err = true;
    for (uint32 i = 0; i < count && !file->eof() && !err; i++)
    {
      uint32 oldid = flip_int(file->read_int32());
      uint32 newid = flip_int(file->read_int32());
      UnitData* unit = NULL;
      if (tbl)
        unit = addUnit(newid, oldid);
      else
      {
        if (!dir.has(oldid))
        {
          err = true;
          break;
        }
        int upos = dir.get(oldid);
        if (units[upos] == NULL)
        {
          err = true;
          break;
        }
        else
        {
          unit = new UnitData(this, oldid, units[upos]);
          units[upos]->release();
          units[upos] = unit;
        }
      }
      uint32 count = file->read_int32();
      if (count > 500) err = true;
      for (uint32 j = 0; j < count && !err; j++)
      {
        uint32 modid = flip_int(file->read_int32());
        uint32 type = file->read_int32();
        int index;
        char const* cmod = meta->getValue(modid, &index);
        if (cmod == NULL || type > 3)
          err = true;
        else
        {
          mod = cmod;
          if (ext)
          {
            uint32 level = file->read_int32();
            uint32 data = file->read_int32();
            if (mod == "Data")
              mod += char('A' + data);
            if (level)
            {
              if (index < 0)
                mod += String((int) level);
              else
                index += level - 1;
            }
          }
          String str;
          if (type == 0)
            str.printf("%d", file->readInt());
          else if (type == 3)
          {
            str = file->readString();
            if (wts && !strncmp(str, "TRIGSTR_", 8))
            {
              char const* rep = wts->getString(atoi(str.c_str() + 8));
              if (rep)
                str = rep;
            }
            str = translate(str);
          }
          else
            str.printf("%.2f", file->readFloat());
          setUnitData(unit, mod, str, index);
          uint32 suf = flip_int(file->read_int32());
          if (suf != 0 && suf != newid && suf != oldid)
            err = true;
        }
      }
      unit->compress();
    }
  }
  return !err;
}

void ObjectData::dump(File* f)
{
  for (int i = 0; i < count; i++)
  {
    if (units[i])
    {
      if (units[i]->getBase())
        f->printf("[%s:%s]\r\n", idToString(units[i]->getID()), idToString(units[i]->getBase()->getID()));
      else
        f->printf("[%s]\r\n", idToString(units[i]->getID()));
      for (int j = 0; j < numFields; j++)
        if (units[i]->isDataSet(j))
          f->printf("%s=%s\r\n", cols.getData(j), units[i]->getData(j));
    }
  }
}

String ObjectData::getUnitString(UnitData const* unit, char const* field, int index) const
{
  if (!fields.has(field))
    return String();
  char const* src = unit->getData(fields.get(field));
  if (index < 0)
  {
    int cmp = nextComma(src, -1);
    if (cmp >= 2 && src[cmp] == 0 && src[0] == '"' && src[cmp - 1] == '"')
      return String(src + 1, cmp - 2);
    else
      return src;
  }
  else
  {
    int pos = -1;
    int cur = 0;
    int prev = 0;
    while ((pos = nextComma(src, pos)) >= 0)
    {
      if (cur == index)
      {
        if (pos - prev >= 2 && src[prev] == '"' && src[pos - 1] == '"')
          return String(src + prev + 1, pos - prev - 2);
        else
          return String(src + prev, pos - prev);
      }
      prev = pos + 1;
      cur++;
    }
  }
  return String();
}

void LoadGameData(GameData& game, FileLoader* loader, int mode)
{
  if ((mode & WC3_LOAD_ALL) == 0) return;

  WTSData wts(TempFile(loader->load("war3map.wts")));
  if (mode & (WC3_LOAD_DESTRUCTABLES | WC3_LOAD_DOODADS))
  {
    game.wes.merge(TempFile(loader->load("UI\\WorldEditGameStrings.txt")));
    if (!(mode & WC3_LOAD_NO_WEONLY))
      game.wes.merge(TempFile(loader->load("UI\\WorldEditStrings.txt")));
  }

  if (!(mode & WC3_LOAD_MERGED))
  {
    if (mode & WC3_LOAD_UNITS)
    {
      ObjectData* data = new ObjectData;
      game.data[WC3_UNITS] = data;
      data->readSLK(TempFile(loader->load("Units\\UnitAbilities.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitBalance.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitData.slk")));
      data->readSLK(TempFile(loader->load("Units\\unitUI.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitWeapons.slk")));
      data->readINI(TempFile(loader->load("Units\\UndeadUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUnitFunc.txt")));
      game.metaData[WC3_UNITS] = new MetaData(TempFile(loader->load("Units\\UnitMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3u")), game.metaData[WC3_UNITS], false, &wts);
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
      data->readSLK(TempFile(loader->load("Units\\ItemData.slk")));
      data->readINI(TempFile(loader->load("Units\\ItemFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\ItemStrings.txt")));
      if (game.metaData[WC3_UNITS] == NULL)
        game.metaData[WC3_UNITS] = new MetaData(TempFile(loader->load("Units\\UnitMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3t")), game.metaData[WC3_UNITS], false, &wts);
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
      ObjectData* data = new ObjectData(&game.wes);
      game.data[WC3_DESTRUCTABLES] = data;
      data->readSLK(TempFile(loader->load("Units\\DestructableData.slk")));
      game.metaData[WC3_DESTRUCTABLES] = new MetaData(TempFile(loader->load("Units\\DestructableMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3b")), game.metaData[WC3_DESTRUCTABLES], false, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_DESTRUCTABLES];
        game.metaData[WC3_DESTRUCTABLES] = NULL;
      }
    }

    if (mode & WC3_LOAD_DOODADS)
    {
      ObjectData* data = new ObjectData(&game.wes);
      game.data[WC3_DOODADS] = data;
      data->readSLK(TempFile(loader->load("Doodads\\Doodads.slk")));
      game.metaData[WC3_DOODADS] = new MetaData(TempFile(loader->load("Doodads\\DoodadMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3d")), game.metaData[WC3_DOODADS], true, &wts);
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
      data->readSLK(TempFile(loader->load("Units\\AbilityData.slk")));
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityStrings.txt")), true);
      game.metaData[WC3_ABILITIES] = new MetaData(TempFile(loader->load("Units\\AbilityMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3a")), game.metaData[WC3_ABILITIES], true, &wts);
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
      data->readSLK(TempFile(loader->load("Units\\AbilityBuffData.slk")));
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityStrings.txt")), true);
      game.metaData[WC3_BUFFS] = new MetaData(TempFile(loader->load("Units\\AbilityBuffMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3h")), game.metaData[WC3_BUFFS], false, &wts);
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
      data->readSLK(TempFile(loader->load("Units\\UpgradeData.slk")));
      data->readINI(TempFile(loader->load("Units\\NightElfUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUpgradeStrings.txt")));
      game.metaData[WC3_UPGRADES] = new MetaData(TempFile(loader->load("Units\\UpgradeMetaData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3q")), game.metaData[WC3_UPGRADES], true, &wts);
      if (!(mode & WC3_LOAD_KEEP_METADATA))
      {
        delete game.metaData[WC3_UPGRADES];
        game.metaData[WC3_UPGRADES] = NULL;
      }
    }
  }
  else
  {
    ObjectData* data = new ObjectData(&game.wes);
    game.merged = data;
    if (mode & (WC3_LOAD_UNITS | WC3_LOAD_ITEMS))
    {
      game.metaData[WC3_UNITS] = new MetaData(TempFile(loader->load("Units\\UnitMetaData.slk")));
      game.metaData[WC3_ITEMS] = game.metaData[WC3_UNITS];
    }
    if (mode & WC3_LOAD_DESTRUCTABLES)
      game.metaData[WC3_DESTRUCTABLES] = new MetaData(TempFile(loader->load("Units\\DestructableMetaData.slk")));
    if (mode & WC3_LOAD_DOODADS)
      game.metaData[WC3_DOODADS] = new MetaData(TempFile(loader->load("Doodads\\DoodadMetaData.slk")));
    if (mode & WC3_LOAD_ABILITIES)
      game.metaData[WC3_ABILITIES] = new MetaData(TempFile(loader->load("Units\\AbilityMetaData.slk")));
    if (mode & WC3_LOAD_BUFFS)
      game.metaData[WC3_BUFFS] = new MetaData(TempFile(loader->load("Units\\AbilityBuffMetaData.slk")));
    if (mode & WC3_LOAD_UPGRADES)
      game.metaData[WC3_UPGRADES] = new MetaData(TempFile(loader->load("Units\\UpgradeMetaData.slk")));

    if (mode & WC3_LOAD_UNITS)
    {
      data->readSLK(TempFile(loader->load("Units\\UnitAbilities.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitBalance.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitData.slk")));
      data->readSLK(TempFile(loader->load("Units\\unitUI.slk")));
      data->readSLK(TempFile(loader->load("Units\\UnitWeapons.slk")));
      data->readINI(TempFile(loader->load("Units\\UndeadUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUnitFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUnitStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUnitFunc.txt")));
      data->readOBJ(TempFile(loader->load("war3map.w3u")), game.metaData[WC3_UNITS], false, &wts);
    }
    if (mode & WC3_LOAD_ITEMS)
    {
      data->readSLK(TempFile(loader->load("Units\\ItemData.slk")));
      data->readINI(TempFile(loader->load("Units\\ItemFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\ItemStrings.txt")));
      data->readOBJ(TempFile(loader->load("war3map.w3t")), game.metaData[WC3_UNITS], false, &wts);
    }
    if (mode & WC3_LOAD_DESTRUCTABLES)
    {
      data->readSLK(TempFile(loader->load("Units\\DestructableData.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3b")), game.metaData[WC3_DESTRUCTABLES], false, &wts);
    }
    if (mode & WC3_LOAD_DOODADS)
    {
      data->readSLK(TempFile(loader->load("Doodads\\Doodads.slk")));
      data->readOBJ(TempFile(loader->load("war3map.w3d")), game.metaData[WC3_DOODADS], true, &wts);
    }
    if (mode & WC3_LOAD_ABILITIES)
      data->readSLK(TempFile(loader->load("Units\\AbilityData.slk")));
    if (mode & WC3_LOAD_BUFFS)
      data->readSLK(TempFile(loader->load("Units\\AbilityBuffData.slk")));
    if (mode & (WC3_LOAD_ABILITIES | WC3_LOAD_BUFFS))
    {
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\UndeadAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CampaignAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\CommonAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\HumanAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\ItemAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NeutralAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\NightElfAbilityStrings.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityFunc.txt")), true);
      data->readINI(TempFile(loader->load("Units\\OrcAbilityStrings.txt")), true);
    }
    if (mode & WC3_LOAD_ABILITIES)
      data->readOBJ(TempFile(loader->load("war3map.w3a")), game.metaData[WC3_ABILITIES], true, &wts);
    if (mode & WC3_LOAD_BUFFS)
      data->readOBJ(TempFile(loader->load("war3map.w3h")), game.metaData[WC3_BUFFS], false, &wts);
    if (mode & WC3_LOAD_UPGRADES)
    {
      data->readSLK(TempFile(loader->load("Units\\UpgradeData.slk")));
      data->readINI(TempFile(loader->load("Units\\NightElfUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NightElfUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\OrcUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\UndeadUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\CampaignUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\NeutralUpgradeStrings.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUpgradeFunc.txt")));
      data->readINI(TempFile(loader->load("Units\\HumanUpgradeStrings.txt")));
      data->readOBJ(TempFile(loader->load("war3map.w3q")), game.metaData[WC3_UPGRADES], true, &wts);
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
