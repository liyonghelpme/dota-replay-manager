#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rmpq.h"
#include <windows.h>
#include "utils.h"

#define ROT3(x) (((x)<<3)|(((x)>>29)&7))
#define UNROT3(x) ((((x)>>3)&0x1FFFFFFF)|((x)<<29))

uint32 mpq_hash_file (MPQFILE file)
{
  MPQFileSeek (file, 0, MPQSEEK_SET);
  uint32 val = 0;
  uint32 len = MPQFileAttribute (file, MPQ_FILE_SIZE);
  uint32 i;
  for (i = 0; i + 3 < len; i += 4)
  {
    val ^= MPQReadInt (file);
    val = ROT3 (val);
  }
  for (; i < len; i++)
  {
    val ^= MPQFileGetc (file);
    val = ROT3 (val);
  }
  return val;
}
static char files[][1024] = {"war3map.j", "scripts\\war3map.j",
  "war3map.w3e", "war3map.wpm", "war3map.doo", "war3map.w3u",
  "war3map.w3b", "war3map.w3d", "war3map.w3a", "war3map.w3q"};
uint32 mpq_hash_file (uint32 val, MPQLOADER wc3, char const* name, bool rot)
{
  MPQFILE file = MPQLoadFile (wc3, name);
  if (file)
  {
    val ^= mpq_hash_file (file);
    if (rot)
      val = ROT3 (val);
    MPQCloseFile (file);
  }
  return val;
}
uint32 mpq_hash_map (MPQLOADER wc3)
{
  uint32 val = 0;
  val = mpq_hash_file (val, wc3, "scripts\\common.j", false);
  val = mpq_hash_file (val, wc3, "scripts\\blizzard.j", false);
  val = ROT3 (val);
  val = ROT3 (val ^ 0x3F1379E);

  for (int i = 0; i < sizeof files / sizeof files[0]; i++)
  {
    MPQFILE file = MPQLoadFile (wc3, files[i]);
    if (file)
    {
      val ^= mpq_hash_file (file);
      val = ROT3 (val);
      MPQCloseFile (file);
    }
  }

  return val;
}
extern MPQLOADER warloader;
extern char warPath[];
uint32 mpq_hash_map (char const* path)
{
  MPQARCHIVE map = MPQOpen (path, MPQFILE_READ);
  if (map == NULL)
    map = MPQOpen (mprintf ("%s%s", warPath, path), MPQFILE_READ);
  if (map == NULL)
    return 0;

  MPQAddArchive (warloader, map);

  uint32 val = mpq_hash_map (warloader);

  MPQRemoveArchive (warloader, map);
  MPQClose (map);

  return val;
}
