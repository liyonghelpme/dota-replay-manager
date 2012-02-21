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
  IntDictionary (int numBytes = 4);
  ~IntDictionary ();

  uint32 set (uint32 key, uint32 value);
  uint32 get (uint32 key);
  bool has (uint32 key);

  uint32 enumStart ();
  uint32 enumNext (uint32 cur);
  uint32 enumGetKey (uint32 cur);
  uint32 enumGetValue (uint32 cur);

  void clear ();
};

#endif // __BASE_INTDICT_H__
