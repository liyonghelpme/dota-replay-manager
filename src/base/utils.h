#ifndef __BASE_UTILS_H__
#define __BASE_UTILS_H__

#include "base/string.h"
#include "base/types.h"

void OpenURL (String str);

uint32 update_crc (uint32 crc, void const* vbuf, uint32 length);
uint32 crc32 (uint8* buf, uint32 length);
uint32 crc32 (String str);

String getAppPath(bool quoted = false);
bool getRegString(HKEY key, char const* subkey, char const* value, String& result);

#define TIME_HOURS      1
#define TIME_SECONDS    2
String format_time(long time, int flags = TIME_SECONDS);

bool browseForFolder(String prompt, String& result);

#endif // __BASE_UTILS_H__
