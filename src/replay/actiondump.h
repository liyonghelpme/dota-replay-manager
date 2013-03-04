#ifndef __REPLAY_ACTIONDUMP__
#define __REPLAY_ACTIONDUMP__

#include "replay/replay.h"
#include "replay/action.h"
#include "dota/mapdata.h"

#define DUMP_SIMPLE       0
#define DUMP_FULL         1
#define DUMP_FULLNAMES    2

class ActionLogger
{
public:
  virtual void addAction(uint32 time, W3GPlayer* player, String text) = 0;
};

class FileActionLogger : public ActionLogger
{
  File* log;
public:
  FileActionLogger(File* file)
  {
    log = file;
  }
  void addAction(uint32 time, W3GPlayer* player, String text);
};

class ActionDumper
{
  W3GActionParser* parser;
  uint32 time;
  bool pause;
  int version;
  W3GReplay* w3g;
  W3GPlayer* player;
  ActionLogger* logger;

  int detail;
  MapData* mapData;
  bool ownData;

  void add(char const* fmt, bool noPlayer = false);
  void parseActions(File* file, int length);
  void parseBlocks(File* file);
public:
  ActionDumper(W3GReplay* replay, MapData* data = NULL);
  ~ActionDumper();
  void dump(ActionLogger* log, int detail);
};

#endif // __REPLAY_ACTIONDUMP__
