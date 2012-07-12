#include "base/file.h"
#include "core/app.h"

#include "registry.h"

#define VALUE_VOID      0x00
#define VALUE_INT       0x01
#define VALUE_INT64     0x02
#define VALUE_DOUBLE    0x03
#define VALUE_STRING    0x04
#define VALUE_BINARY    0x05

RegistryItem::RegistryItem()
{
  type = VALUE_VOID;
  size = 0;
  value = NULL;
}
RegistryItem::~RegistryItem()
{
  delete[] value;
}
void RegistryItem::set(uint8 t, uint32 s, void const* v)
{
  type = t;
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
  cfg::init(this);
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "config.cfg"), File::READ);
  if (file)
  {
    int num = file->read_int32();
    String name;
    for (int i = 0; i < num; i++)
    {
      uint32 nlen = file->read_int32();
      name.resize(nlen);
      if (file->read(name.getBuffer(), nlen) != nlen)
        break;
      name.setLength(nlen);
      RegistryItem* item = createItem(name);
      uint8 type;
      uint32 size;
      if (file->read(&type, 1) != 1 ||
          file->read(&size, 4) != 4)
        break;
      if (item->type == VALUE_VOID || item->type == type)
      {
        item->set(type, size, NULL);
        if (file->read(item->value, size) != size)
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
      file->write_int32(name.length());
      file->write(name.c_str(), name.length());
      file->putc(item.type);
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
RegistryItem* Registry::createItem(char const* name)
{
  return &items.create(name);
}

//////////////////////////////////////////

cfg::IntItem::operator int()
{
  return *(int*) item->value;
}
cfg::IntItem& cfg::IntItem::operator = (int value)
{
  *(int*) item->value = value;
  return *this;
}

cfg::Int64Item::operator uint64()
{
  return *(uint64*) item->value;
}
cfg::Int64Item& cfg::Int64Item::operator = (uint64 value)
{
  *(uint64*) item->value = value;
  return *this;
}

cfg::DoubleItem::operator double()
{
  return *(double*) item->value;
}
cfg::DoubleItem& cfg::DoubleItem::operator = (double value)
{
  *(double*) item->value = value;
  return *this;
}

cfg::StringItem::operator String()
{
  return String((char*) item->value);
}
cfg::StringItem& cfg::StringItem::operator = (char const* value)
{
  item->set(VALUE_STRING, strlen(value) + 1, value);
  return *this;
}

uint32 cfg::BinaryItem::size()
{
  return item->size;
}
uint8 const* cfg::BinaryItem::data()
{
  return item->value;
}
void cfg::BinaryItem::set(uint8* d, uint32 s)
{
  item->set(VALUE_BINARY, s, d);
}


#define cfg_int(n,d)        cfg::IntItem cfg::n;
#define cfg_int64(n,d)      cfg::Int64Item cfg::n;
#define cfg_double(n,d)     cfg::DoubleItem cfg::n;
#define cfg_string(n,d)     cfg::StringItem cfg::n;
#define cfg_binary(n,d,s)   cfg::BinaryItem cfg::n;
#include "cfgitems.h"
#undef cfg_int
#undef cfg_int64
#undef cfg_double
#undef cfg_string
#undef cfg_binary

void cfg::init(Registry* reg)
{
#define cfg_int(n,d)        {int v=d;n.item=reg->createItem(#n);n.item->set(VALUE_INT,sizeof(int),&v);}
#define cfg_int64(n,d)      {uint64 v=d;n.item=reg->createItem(#n);n.item->set(VALUE_INT64,sizeof(uint64),&v);}
#define cfg_double(n,d)     {double v=d;n.item=reg->createItem(#n);n.item->set(VALUE_DOUBLE,sizeof(double),&v);}
#define cfg_string(n,d)     n.item=reg->createItem(#n);n=d;
#define cfg_binary(n,d,s)   n.item=reg->createItem(#n);n.item->set(VALUE_BINARY,s,(uint8*)d);
#define cfg_init
#include "cfgitems.h"
#undef cfg_int
#undef cfg_int64
#undef cfg_double
#undef cfg_string
#undef cfg_binary
#undef cfg_init
}
