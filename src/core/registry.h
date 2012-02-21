#ifndef __CORE_REGISTRY_H__
#define __CORE_REGISTRY_H__

#include "base/string.h"
#include "base/types.h"
#include "base/dictionary.h"

class Registry
{
  struct RegistryItem
  {
    uint8 type;
    uint32 size;
    uint8* value;
    RegistryItem();
    ~RegistryItem();
    void set(uint8 t, uint32 s, void const* v);
  };
  Dictionary<RegistryItem> items;
  Dictionary<RegistryItem> defaults;
  void dwriteInt(String name, int value);
  void dwriteInt64(String name, __int64 value);
  void dwriteDouble(String name, double value);
  void dwriteString(String name, char const* value);
  void dwriteBinary(String name, void const* data, uint32 size);
public:
  Registry();
  ~Registry();

  void delKey(String name);

  void writeInt(String name, int value);
  int readInt(String name, int def = 0);
  void writeInt64(String name, __int64 value);
  __int64 readInt64(String name, __int64 def = 0);
  void writeDouble(String name, double value);
  double readDouble(String name, double def = 0);
  void writeString (String name, char const* value);
  String readString(String name, char const* def = NULL);
  void writeBinary(String name, void const* data, uint32 size);
  uint32 readBinary(String name, void* data, uint32 size);

  int* createInt(String name, int def = 0);
  __int64* createInt64(String name, __int64 def = 0);
  double* createDouble(String name, double def = 0);

  void restoreDefaults();
};

#endif // __CORE_REGISTRY_H__
