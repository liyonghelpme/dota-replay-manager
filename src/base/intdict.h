#ifndef __BASE_INTDICT_H__
#define __BASE_INTDICT_H__

#include "types.h"

class IntDictionary
{
  uint8* buf;
  uint32 size;
  uint32 count;
  int maxDepth;
public:
  explicit IntDictionary (int numBytes = 4);
  ~IntDictionary ();

  uint32 set (uint32 key, uint32 value);
  uint32 del (uint32 key);
  uint32 get (uint32 key) const;
  bool has (uint32 key) const;

  uint32 enumStart () const;
  uint32 enumNext (uint32 cur) const;
  uint32 enumGetKey (uint32 cur) const;
  uint32 enumGetValue (uint32 cur) const;

  void clear ();
};

#endif // __BASE_INTDICT_H__
