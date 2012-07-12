#ifndef __CORE_REGISTRY_H__
#define __CORE_REGISTRY_H__

#include "base/string.h"
#include "base/types.h"
#include "base/dictionary.h"
#include "base/utils.h"

struct RegistryItem
{
  uint8 type;
  uint32 size;
  uint8* value;
  RegistryItem();
  ~RegistryItem();
  void set(uint8 t, uint32 s, void const* v);
};
class Registry
{
  Dictionary<RegistryItem> items;

  friend class cfg;
  RegistryItem* createItem(char const* name);
public:
  Registry();
  ~Registry();
};

class cfg
{
public:
  class ConfigItem
  {
  protected:
    friend class cfg;
    RegistryItem* item;
  public:
  };
  class IntItem : public ConfigItem
  {
  public:
    operator int();
    IntItem& operator = (int value);
  };
  class Int64Item : public ConfigItem
  {
  public:
    operator uint64();
    Int64Item& operator = (uint64 value);
  };
  class DoubleItem : public ConfigItem
  {
  public:
    operator double();
    DoubleItem& operator = (double value);
  };
  class StringItem : public ConfigItem
  {
  public:
    operator String();
    StringItem& operator = (char const* value);
  };
  class BinaryItem : public ConfigItem
  {
  public:
    uint32 size();
    uint8 const* data();
    void set(uint8* data, uint32 size);

    template<class T>
    T const& get()
    {
      return *(T*) data();
    }
  };

#define cfg_int(n,d)        static IntItem n;
#define cfg_int64(n,d)      static Int64Item n;
#define cfg_double(n,d)     static DoubleItem n;
#define cfg_string(n,d)     static StringItem n;
#define cfg_binary(n,d,s)   static BinaryItem n;
#include "cfgitems.h"
#undef cfg_int
#undef cfg_int64
#undef cfg_double
#undef cfg_string
#undef cfg_binary

  static void init(Registry* reg);
};

#endif // __CORE_REGISTRY_H__
