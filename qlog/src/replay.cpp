#include "replay.h"
#include "base/gzmemory.h"

struct W3GBlockHeader
{
  uint16 c_size;
  uint16 u_size;
  uint16 check1;
  uint16 check2;
};

bool W3GHeader::read(File* f)
{
  memset(this, 0, sizeof(W3GHeader));
  if (f->read(this, 48) != 48)
    return false;
  if (memcmp(intro, "Warcraft III recorded game", 26))
    return false;
  if (header_v == 0)
  {
    if (f->read(&minor_v, 2) != 2) return false;
    if (f->read(&major_v, 2) != 2) return false;
    if (f->read(&build_v, 2) != 2) return false;
    if (f->read(&flags, 2) != 2) return false;
    if (f->read(&length, 4) != 4) return false;
    if (f->read(&checksum, 4) != 4) return false;
    ident = 'WAR3';
  }
  else
  {
    if (f->read(&ident, 4) != 4) return false;
    if (f->read(&major_v, 4) != 4) return false;
    if (f->read(&build_v, 2) != 2) return false;
    if (f->read(&flags, 2) != 2) return false;
    if (f->read(&length, 4) != 4) return false;
    if (f->read(&checksum, 4) != 4) return false;
    minor_v = 0;
  }
  return true;
}

File* W3GReplay::unpack(File* f, W3GHeader& hdr)
{
  if (!hdr.read (f))
    return NULL;

  f->seek(0, SEEK_END);
  int file_size = f->tell();

  f->seek(hdr.header_size, SEEK_SET);
  int pos = hdr.header_size;
  int max_source = 1024;
  int total_dest = 0;
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader block;
    if (f->read(&block, sizeof block) != sizeof block)
      return NULL;
    pos += sizeof block;
    if (pos + block.c_size > file_size)
      return NULL;
    f->seek(block.c_size, SEEK_CUR);

    if (block.c_size > max_source)
      max_source = block.c_size;
    total_dest += block.u_size;
  }

  if (total_dest == 0)
    return NULL;

  char* source = new char[max_source];
  char* dest = new char[total_dest];
  int dest_pos = 0;

  gzmemory gzmem;

  f->seek(hdr.header_size, SEEK_SET);
  for (int i = 0; i < hdr.blocks; i++)
  {
    W3GBlockHeader block;
    f->read(&block, sizeof block);
    f->read(source, block.c_size);
    if (!gzinflate2(source, dest + dest_pos, block.c_size, block.u_size, &gzmem))
    {
      delete[] source;
      delete[] dest;
      return NULL;
    }
    dest_pos += block.u_size;
  }

  delete[] source;
  return File::memfile(dest, dest_pos);
}
W3GReplay* W3GReplay::load(char const* path, uint32* error)
{
  uint32 localError;
  if (error == NULL)
    error = &localError;
  *error = 0;

  W3GReplay* w3g = NULL;

  File* file = File::open(path, File::READ);
  if (file == NULL)
    *error = eNoFile;
  else
  {
    W3GHeader hdr;
    File* unpacked = unpack(file, hdr);
    if (unpacked)
    {
      w3g = new W3GReplay(unpacked, hdr, error);
      if (*error)
      {
        delete w3g;
        w3g = NULL;
      }
    }
    else
      *error = eBadFile;
    delete file;
  }

  return w3g;
}

W3GReplay::W3GReplay(File* unpacked, W3GHeader const& header, uint32* error)
{
  replay = unpacked;
  memcpy(&hdr, &header, sizeof hdr);

  memset(players, 0, sizeof players);

  replay->seek(5, SEEK_SET);

  loadPlayer();
  loadGame();

  blockPos = replay->tell();
}
W3GReplay::~W3GReplay()
{
  for (int i = 0; i < 256; i++)
    delete players[i];
  delete replay;
}

void W3GReplay::loadPlayer()
{
  uint8 player_id = replay->getc();
  String* name = new String(replay->readString());
  if (name->isEmpty())
    name->printf("Player %d", player_id);
  players[player_id] = name;

  uint8 key = replay->getc();
  if (key == 1)
    replay->seek(1, SEEK_CUR);
  else if (key == 8)
    replay->seek(8, SEEK_CUR);
}

void W3GReplay::loadGame()
{
  replay->readString();
  replay->seek(1, SEEK_CUR);
  replay->readString();
  replay->seek(12, SEEK_CUR);
  while (replay->getc() == 0x16)
  {
    loadPlayer();
    replay->seek(4, SEEK_CUR);
  }
  replay->seek(2, SEEK_CUR);
  int slot_records = replay->getc();
  for (int i = 0; i < slot_records; i++)
  {
    uint8 id = replay->getc();
    replay->seek(2, SEEK_CUR);
    uint8 computer = replay->getc();
    replay->seek(3, SEEK_CUR);
    uint8 ai_strength = 1;
    if (hdr.major_v >= 3)
      ai_strength = replay->getc();
    if (hdr.major_v >= 7)
      replay->seek(1, SEEK_CUR);

    if (computer)
    {
      if (ai_strength == 0)
        players[id] = new String("Computer (Easy)");
      else if (ai_strength == 1)
        players[id] = new String("Computer (Normal)");
      else if (ai_strength == 2)
        players[id] = new String("Computer (Insane)");
    }
  }

  replay->seek(6, SEEK_CUR);
}
