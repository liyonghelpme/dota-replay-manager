#include "base/file.h"
#include "core/app.h"

#include "registry.h"

#define VALUE_VOID      0x00
#define VALUE_INT       0x01
#define VALUE_INT64     0x02
#define VALUE_DOUBLE    0x03
#define VALUE_STRING    0x04
#define VALUE_BINARY    0x05
#define VALUE_MASK      0x7F
#define VALUE_FROZEN    0x80

Registry::RegistryItem::RegistryItem()
{
  type = VALUE_VOID;
  size = 0;
  value = NULL;
}
Registry::RegistryItem::~RegistryItem()
{
  delete[] value;
}
void Registry::RegistryItem::set(uint8 t, uint32 s, void const* v)
{
  if ((type & VALUE_FROZEN) && ((t & VALUE_MASK) != (type & VALUE_MASK) || s != size))
    return;
  type = t | (type & VALUE_FROZEN);
  if (s != size)
  {
    size = s;
    delete[] value;
    if (size)
      value = new uint8[size];
    else
      value = NULL;
  }
  if (size)
  {
    if (v)
      memcpy(value, v, size);
    else
      memset(value, 0, size);
  }
}

Registry::Registry()
{
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "config.cfg"), File::READ);
  if (file)
  {
    int num = file->read_int32();
    for (int i = 0; i < num; i++)
    {
      uint8 nlen = file->getc();
      char name[256];
      if (file->read(name, nlen) != nlen)
        break;
      name[nlen] = 0;
      RegistryItem& item = items.create(name);
      if (file->read(&item.type, 1) != 1 ||
          file->read(&item.size, 4) != 4)
        break;
      if (item.size)
      {
        item.value = new uint8[item.size];
        if (file->read(item.value, item.size) != item.size)
          break;
      }
    }
    delete file;
  }
}
Registry::~Registry()
{
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "config.cfg"), File::REWRITE);
  if (file)
  {
    int num = 0;
    file->write_int32(num);
    for (uint32 cur = items.enumStart(); cur; cur = items.enumNext(cur))
    {
      String name = items.enumGetKey(cur);
      RegistryItem const& item = items.enumGetValue(cur);
      file->putc(name.length());
      file->write(name.c_str(), name.length());
      file->putc(item.type & VALUE_MASK);
      file->write_int32(item.size);
      if (item.size)
        file->write(item.value, item.size);
      num++;
    }
    file->seek(0, SEEK_SET);
    file->write_int32(num);
    delete file;
  }
}

void Registry::dwriteInt(String name, int value)
{
  RegistryItem& item = defaults.create(name);
  item.set(VALUE_INT, sizeof value, &value);
}
void Registry::dwriteInt64(String name, __int64 value)
{
  RegistryItem& item = defaults.create(name);
  item.set(VALUE_INT64, sizeof value, &value);
}
void Registry::dwriteDouble(String name, double value)
{
  RegistryItem& item = defaults.create(name);
  item.set(VALUE_DOUBLE, sizeof value, &value);
}
void Registry::dwriteString(String name, char const* value)
{
  RegistryItem& item = defaults.create(name);
  item.set(VALUE_STRING, strlen(value) + 1, value);
}
void Registry::dwriteBinary(String name, void const* data, uint32 size)
{
  RegistryItem& item = defaults.create(name);
  item.set(VALUE_BINARY, size, data);
}

void Registry::delKey(String name)
{
  if (items.has(name) && (items.get(name).type & VALUE_FROZEN))
    return;
  items.del(name);
}

void Registry::writeInt(String name, int value)
{
  RegistryItem& item = items.create(name);
  item.set(VALUE_INT, sizeof value, &value);
}
int Registry::readInt(String name, int def)
{
  dwriteInt(name, def);
  RegistryItem& item = items.create(name);
  int type = (item.type & VALUE_MASK);
  if (type == VALUE_INT || type == VALUE_INT64)
    return *(int*) item.value;
  else if (type == VALUE_DOUBLE)
    return int(*(double*) item.value);
  else
  {
    item.set(VALUE_INT, sizeof def, &def);
    return def;
  }
}
void Registry::writeInt64(String name, __int64 value)
{
  RegistryItem& item = items.create(name);
  item.set(VALUE_INT64, sizeof value, &value);
}
__int64 Registry::readInt64(String name, __int64 def)
{
  dwriteInt64(name, def);
  RegistryItem& item = items.create(name);
  int type = (item.type & VALUE_MASK);
  if (type == VALUE_INT64)
    return *(__int64*) item.value;
  else if (type == VALUE_INT)
    return *(int*) item.value;
  else if (type == VALUE_DOUBLE)
    return __int64(*(double*) item.value);
  else
  {
    item.set(VALUE_INT64, sizeof def, &def);
    return def;
  }
}
void Registry::writeDouble(String name, double value)
{
  RegistryItem& item = items.create(name);
  item.set(VALUE_DOUBLE, sizeof value, &value);
}
double Registry::readDouble(String name, double def)
{
  dwriteDouble(name, def);
  RegistryItem& item = items.create(name);
  int type = (item.type & VALUE_MASK);
  if (type == VALUE_DOUBLE)
    return *(double*) item.value;
  else if (type == VALUE_INT)
    return double(*(int*) item.value);
  else if (type == VALUE_INT64)
    return double(*(__int64*) item.value);
  else
  {
    item.set(VALUE_DOUBLE, sizeof def, &def);
    return def;
  }
}
void Registry::writeString (String name, char const* value)
{
  RegistryItem& item = items.create(name);
  item.set(VALUE_STRING, strlen(value) + 1, value);
}
String Registry::readString(String name, char const* def)
{
  if (def)
    dwriteString(name, def);
  RegistryItem& item = items.create(name);
  if ((item.type & VALUE_MASK) == VALUE_STRING)
    return String((char*) item.value);
  else if (def)
  {
    item.set(VALUE_STRING, strlen(def) + 1, def);
    return def;
  }
  else
  {
    item.set(VALUE_STRING, 1, "");
    return "";
  }
}
void Registry::writeBinary(String name, void const* data, uint32 size)
{
  RegistryItem& item = items.create(name);
  item.set(VALUE_BINARY, size, data);
}
uint32 Registry::readBinary(String name, void* data, uint32 size)
{
  if (data)
    dwriteBinary(name, data, size);
  RegistryItem& item = items.create(name);
  if ((item.type & VALUE_MASK) != VALUE_VOID)
  {
    if (data)
    {
      if (size >= item.size)
        memcpy(data, item.value, item.size);
      else
        return 0;
    }
    return item.size;
  }
  else
  {
    item.set(VALUE_BINARY, size, data);
    return size;
  }
}

int* Registry::createInt(String name, int def)
{
  dwriteInt(name, def);
  RegistryItem& item = items.create(name);
  if (item.type == VALUE_VOID)
    item.set(VALUE_INT | VALUE_FROZEN, sizeof def, &def);
  else
    item.type |= VALUE_FROZEN;
  return (int*) item.value;
}
__int64* Registry::createInt64(String name, __int64 def)
{
  dwriteInt64(name, def);
  RegistryItem& item = items.create(name);
  if (item.type == VALUE_VOID)
    item.set(VALUE_INT64 | VALUE_FROZEN, sizeof def, &def);
  else
    item.type |= VALUE_FROZEN;
  return (__int64*) item.value;
}
double* Registry::createDouble(String name, double def)
{
  dwriteDouble(name, def);
  RegistryItem& item = items.create(name);
  if (item.type == VALUE_VOID)
    item.set(VALUE_DOUBLE | VALUE_FROZEN, sizeof def, &def);
  else
    item.type |= VALUE_FROZEN;
  return (double*) item.value;
}

void Registry::restoreDefaults()
{
  for (uint32 cur = defaults.enumStart(); cur; cur = defaults.enumNext(cur))
  {
    String name = defaults.enumGetKey(cur);
    RegistryItem const& def = defaults.enumGetValue(cur);
    RegistryItem& item = items.create(name);
    item.set(def.type, def.size, def.value);
  }
}
