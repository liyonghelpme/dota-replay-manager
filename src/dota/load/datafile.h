#ifndef __DOTA_LOAD_DATAFILE__
#define __DOTA_LOAD_DATAFILE__

#include "base/types.h"
#include "base/string.h"
#include "base/dictionary.h"
#include "base/intdict.h"
#include "base/file.h"
#include "dota/dotadata.h"
#include <stdlib.h>

class SLKFile
{
  Dictionary<uint32> cols;
  char* megabuff;
  int* table;
  int width;
  int height;
public:
  SLKFile(File* file);
  ~SLKFile();

  bool isValid() const
  {
    return height > 0;
  }

  int getNumRows() const
  {
    return height - 1;
  }
  int getNumColumns() const
  {
    return width;
  }
  char const* getColumn(int i) const
  {
    return megabuff + table[i];
  }
  char const* getItem(int i, int j) const
  {
    return megabuff + table[(i + 1) * width + j];
  }
  bool isItemSet(int i, int j) const
  {
    return table[(i + 1) * width + j] != 0;
  }
  int getColumn(char const* name) const
  {
    return cols.has(name) ? cols.get(name) : -1;
  }
};

#define META_DATA_ID            0
#define META_DATA_FIELD         1
#define META_DATA_INDEX         2
#define META_DATA_REPEAT        3
#define META_DATA_DATA          4
#define META_DATA_CATEGORY      5
#define META_DATA_DISPLAY       6
#define META_DATA_SORT          7
#define META_DATA_TYPE          8
#define META_DATA_USEUNIT       9
#define META_DATA_USEHERO       10
#define META_DATA_USEBUILDING   11
#define META_DATA_USEITEM       12
#define META_DATA_USESPECIFIC   13
class MetaData
{
  SLKFile slk;
//  Dictionary<uint32> data;
  struct IdValue
  {
    uint32 id;
    int row;
  };
  int impRows[14];
  IdValue* id;
  int numId;
  static int __cdecl MetaData::idComp(void const* a, void const* b);
public:
  MetaData(File* file);
  ~MetaData();

  bool isValid() const
  {
    return id != NULL;
  }

  int getNumRows() const
  {
    return slk.getNumRows();
  }
  int getRow(int id) const;
  int getRowInt(int row, int type) const
  {
    return atoi(slk.getItem(row, impRows[type]));
  }
  char const* getRowString(int row, int type) const
  {
    return slk.getItem(row, impRows[type]);
  }
  char const* getValue(int id, int* index = NULL) const
  {
    int row = getRow(id);
    if (row < 0) return NULL;
    if (index) *index = atoi(slk.getItem(row, impRows[META_DATA_INDEX]));
    return slk.getItem(row, impRows[META_DATA_FIELD]);
  }
  //int getId(char const* value) const
  //{
  //  return data.has(value) ? data[value] : -1;
  //}
};

class WTSData
{
  char* megabuff;
  int length;
  int size;
  struct IdString
  {
    int id;
    int pos;
  };
  IdString* strings;
  int count;
  void append(char c);
public:
  WTSData(File* file);
  ~WTSData();

  char const* getString(int id) const;
};
class WEStrings
{
  char* megabuff;
  int length;
  int size;
  Dictionary<uint32> data;
public:
  WEStrings();
  void merge(File* file);
  ~WEStrings();

  char const* getString(char const* str) const;
};

class ObjectData;
class UnitData
{
  char* megabuff;
  int size;
  int length;
  int id;
  UnitData* base;
  int* ptr;
  int ptrsize;
  void alloc(int len);
  void mklen(int x);
  ObjectData* owner;
  int ref;
public:
  UnitData(ObjectData* owner, int id, UnitData* base = NULL);
  ~UnitData();

  uint32 getID() const
  {
    return id;
  }
  UnitData* getBase()
  {
    return (base && base->id != id) ? base : NULL;
  }
  UnitData const* getBase() const
  {
    return (base && base->id != id) ? base : NULL;
  }
  void setData(int x, char const* data, int index = -1);
  char const* getData(int x) const
  {
    return (x < ptrsize && ptr[x] != 0) ? megabuff + ptr[x] : (base && x < base->ptrsize ?
      base->megabuff + base->ptr[x] : megabuff);
  }
  bool isDataSet(int x) const
  {
    return (x < ptrsize && ptr[x] != 0) ? true : (base && x < base->ptrsize ? base->ptr[x] != 0 : false);
  }
  void compress();

  char const* getData(char const* field) const;
  bool isDataSet(char const* field) const;
  int getIntData(char const* field) const;
  float getRealData(char const* field) const;
  String getStringData(char const* field, int index = -1) const;

  void release();
};

class ObjectData
{
  IntDictionary dir;
  Dictionary<uint32> fields;
  int numFields;
  UnitData** units;
  int size;
  int count;
  UnitData* addUnit(uint32 id, uint32 base = 0);

  UnitData cols;

  WEStrings* wes;
  char const* translate(char const* str) const
  {
    char const* r = NULL;
    if (wes)
      r = wes->getString(str);
    if (r) return r;
    return str;
  }
public:
  ObjectData(WEStrings* we = NULL);
  ~ObjectData();

  bool readSLK(File* file);
  bool readINI(File* file, bool split = false);
  bool readOBJ(File* file, MetaData* meta, bool ext, WTSData* wts = NULL);

  int getNumUnits() const
  {
    return count - 1;
  }
  UnitData* getUnit(int i)
  {
    return units[i + 1];
  }
  UnitData const* getUnit(int i) const
  {
    return units[i + 1];
  }
  UnitData* getUnitById(uint32 id)
  {
    return units[dir.get(id)];
  }
  UnitData const* getUnitById(uint32 id) const
  {
    return units[dir.get(id)];
  }

  void setUnitData(UnitData* unit, char const* field, char const* data, int index = -1);
  char const* getUnitData(UnitData const* unit, char const* field) const
  {
    return unit->getData(fields.get(field));
  }
  bool isUnitDataSet(UnitData const* unit, char const* field) const
  {
    return unit->isDataSet(fields.get(field));
  }
  int getUnitInt(UnitData const* unit, char const* field) const
  {
    return atoi(unit->getData(fields.get(field)));
  }
  float getUnitReal(UnitData const* unit, char const* field) const
  {
    return (float) atof(unit->getData(fields.get(field)));
  }
  String getUnitString(UnitData const* unit, char const* field, int index = -1) const;

  void dump(File* f);
  void dump(char const* path)
  {
    File* f = File::open(path, File::REWRITE);
    dump(f);
    delete f;
  }
};
inline char const* UnitData::getData(char const* field) const
{
  return owner->getUnitData(this, field);
}
inline bool UnitData::isDataSet (char const* field) const
{
  return owner->isUnitDataSet(this, field);
}
inline int UnitData::getIntData(char const* field) const
{
  return owner->getUnitInt(this, field);
}
inline float UnitData::getRealData(char const* field) const
{
  return owner->getUnitReal(this, field);
}
inline String UnitData::getStringData(char const* field, int index) const
{
  return owner->getUnitString(this, field, index);
}

#define WC3_UNITS             0
#define WC3_ITEMS             1
#define WC3_DESTRUCTABLES     2
#define WC3_DOODADS           3
#define WC3_ABILITIES         4
#define WC3_BUFFS             5
#define WC3_UPGRADES          6
#define NUM_WC3_TYPES         7
struct GameData
{
  ObjectData* data[NUM_WC3_TYPES];
  ObjectData* merged;
  MetaData* metaData[NUM_WC3_TYPES];
  WEStrings wes;

  GameData()
  {
    for (int i = 0; i < NUM_WC3_TYPES; i++)
    {
      data[i] = NULL;
      metaData[i] = NULL;
    }
    merged = NULL;
  }
  ~GameData()
  {
    metaData[WC3_ITEMS] = NULL;
    for (int i = 0; i < NUM_WC3_TYPES; i++)
    {
      delete data[i];
      delete metaData[i];
    }
    delete merged;
  }
};

#define WC3_LOAD_UNITS            0x0001
#define WC3_LOAD_ITEMS            0x0002
#define WC3_LOAD_DESTRUCTABLES    0x0004
#define WC3_LOAD_DOODADS          0x0008
#define WC3_LOAD_ABILITIES        0x0010
#define WC3_LOAD_BUFFS            0x0020
#define WC3_LOAD_UPGRADES         0x0040
#define WC3_LOAD_MERGED           0x0080
#define WC3_LOAD_ALL              0x007F

#define WC3_LOAD_NO_WEONLY        0x0100
#define WC3_LOAD_KEEP_METADATA    0x0200

void LoadGameData(GameData& game, FileLoader* loader, int mode);

#endif // __DOTA_LOAD_DATAFILE__
