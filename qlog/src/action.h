#ifndef __REPLAY_ACTION__
#define __REPLAY_ACTION__

#include "base/types.h"
#include "base/string.h"

class File;

struct ActionType;
class W3GActionParser
{
  ActionType* types[256];
  ActionType* parsed;
  uint8* buffer;
  int bufSize;
  uint8* args[16];
public:
  W3GActionParser(int version);
  ~W3GActionParser();

  uint8 parse(File* file);

  String name() const;
  uint8 type() const;
  uint32 arg_int(int i, uint32 def = 0) const;
  uint64 arg_int64(int i, uint64 def = 0) const;
  float arg_float(int i) const;
  char const* arg_string(int i) const;

  String print() const;
};

#endif // __REPLAY_ACTION__
