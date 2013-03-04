#ifndef __BASE_UTILS_H__
#define __BASE_UTILS_H__

#include <windows.h>

#include "base/string.h"
#include "base/types.h"

uint32 update_crc(uint32 crc, void const* vbuf, uint32 length);
uint32 crc32(uint8* buf, uint32 length);
uint32 crc32(String str);

#define TIME_HOURS      1
#define TIME_SECONDS    2
String format_time(long time, int flags = TIME_SECONDS);

uint32 flip_int(uint32 i);

uint64 sysTime();
String format_systime(uint64 time, char const* fmt);

#endif // __BASE_UTILS_H__
