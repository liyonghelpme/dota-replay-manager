#include <time.h>

#include "base/string.h"

#include "utils.h"

uint32 update_crc(uint32 crc, void const* vbuf, uint32 length)
{
  static uint32 crc_table[256];
  static bool table_computed = false;
  uint8 const* buf = (uint8*) vbuf;
  if (!table_computed)
  {
    for (uint32 i = 0; i < 256; i++)
    {
      uint32 c = i;
      for (int k = 0; k < 8; k++)
      {
        if (c & 1)
          c = 0xEDB88320L ^ (c >> 1);
        else
          c = c >> 1;
      }
      crc_table[i] = c;
    }
    table_computed = true;
  }
  for (uint32 i = 0; i < length; i++)
    crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
  return crc;
}
uint32 crc32(uint8* buf, uint32 length)
{
  return ~update_crc(0xFFFFFFFF, buf, length);
}
uint32 crc32(String str)
{
  return ~update_crc(0xFFFFFFFF, str, str.length());
}

String format_time(long time, int flags)
{
  String result = "";
  if (time < 0)
  {
    result = "-";
    time = -time;
  }
  time /= 1000;
  if (time >= 3600 && (flags & TIME_HOURS))
    result.printf("%d:%02d", time / 3600, (time / 60) % 60);
  else
    result.printf("%d", time / 60);
  if (flags & TIME_SECONDS)
    result.printf(":%02d", time % 60);
  return result;
}

uint32 flip_int(uint32 i)
{
  return ((i >> 24) & 0x000000FF) |
         ((i >>  8) & 0x0000FF00) |
         ((i <<  8) & 0x00FF0000) |
         ((i << 24) & 0xFF000000);
}

uint64 sysTime()
{
  uint64 t;
  _time64((__time64_t*) &t);
  return t;
}
String format_systime(uint64 time, char const* fmt)
{
  tm temp;
  char buf[128];
  _localtime64_s(&temp, (__time64_t*) &time);
  strftime(buf, sizeof buf, fmt, &temp);
  return buf;
}
