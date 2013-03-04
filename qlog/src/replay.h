#ifndef __REPLAY__
#define __REPLAY__

#include "base/types.h"
#include "base/file.h"

#define CHAT_ALL              0x00
#define CHAT_ALLIES           0x01
#define CHAT_OBSERVERS        0x02
#define CHAT_PRIVATE          0x03

struct W3GHeader
{
  char intro[28];
  uint32 header_size;
  uint32 c_size;
  uint32 header_v;
  uint32 u_size;
  uint32 blocks;
  uint32 ident;
  uint32 major_v;
  uint16 minor_v;
  uint16 build_v;
  uint16 flags;
  uint32 length;
  uint32 checksum;

  W3GHeader()
  {
    memset(this, 0, sizeof(W3GHeader));
  }
  bool read(File* f);
};

class W3GReplay
{
  W3GHeader hdr;
  File* replay;
  int blockPos;
  String* players[256];
  W3GReplay(File* unpacked, W3GHeader const& hdr, uint32* error);
  static File* unpack(File* f, W3GHeader& hdr);
  void loadPlayer();
  void loadGame();
  bool validPlayers();
public:
  enum {eOk, eNoFile, eBadFile};

  ~W3GReplay();
  static W3GReplay* load(char const* path, uint32* error);

  int getVersion() const
  {
    return hdr.major_v;
  }
  String* getPlayerById(int id)
  {
    return players[id];
  }
  File* getFile()
  {
    replay->seek(blockPos, SEEK_SET);
    return replay;
  }
};

#endif // __REPLAY__
