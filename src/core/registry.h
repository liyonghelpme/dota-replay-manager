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
  void dwriteInt(char const* name, int value);
  void dwriteInt64(char const* name, __int64 value);
  void dwriteDouble(char const* name, double value);
  void dwriteString(char const* name, char const* value);
  void dwriteBinary(char const* name, void const* data, uint32 size);
public:
  Registry();
  ~Registry();

  void delKey(char const* name);

  void writeInt(char const* name, int value);
  int readInt(char const* name, int def = 0);
  void writeInt64(char const* name, __int64 value);
  __int64 readInt64(char const* name, __int64 def = 0);
  void writeDouble(char const* name, double value);
  double readDouble(char const* name, double def = 0);
  void writeString (char const* name, char const* value);
  String readString(char const* name, char const* def = NULL);
  void writeBinary(char const* name, void const* data, uint32 size);
  uint32 readBinary(char const* name, void* data, uint32 size);

  int* createInt(char const* name, int def = 0);
  __int64* createInt64(char const* name, __int64 def = 0);
  double* createDouble(char const* name, double def = 0);

  void restoreDefaults();
};

#endif // __CORE_REGISTRY_H__
