#ifndef __CORE_REGISTRY_H__
#define __CORE_REGISTRY_H__

#include "base/string.h"
#include "base/types.h"
#include "base/dictionary.h"
#include "base/utils.h"

#define cfgconst
#define regbasic(t,n,v)
#define regstring(n,v)
#define regarray(t,n,s)
#define regvararray(t,n)
#define regstrarray(n)
#include "cfgitems.h"
#undef regbasic
#undef regstring
#undef regarray
#undef regvararray
#undef regstrarray
#undef cfgconst

class Config
{
  enum {iBasic, iString, iArray, iStrArray};
  struct ItemData
  {
    uint8 type;
    uint32 size;
    void* ptr;
    ItemData()
    {}
    ItemData(uint8 t, uint32 s, void* p)
    {
      type = t;
      size = s;
      ptr = p;
    }
  };
  Dictionary<ItemData> items;
public:
  Config();

  void read();
  void write();
  void reset();

#define regbasic(t,n,v)     t n
#define regstring(n,v)      String n
#define regarray(t,n,s)     t n[s]
#define regvararray(t,n)    Array<t> n
#define regstrarray(n)      Array<String> n
#include "cfgitems.h"
#undef regbasic
#undef regstring
#undef regarray
#undef regvararray
#undef regstrarray
};
extern Config cfg;

#endif // __CORE_REGISTRY_H__
