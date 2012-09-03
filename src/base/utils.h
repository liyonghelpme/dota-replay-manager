#ifndef __BASE_UTILS_H__
#define __BASE_UTILS_H__

#include <windows.h>

#include "base/string.h"
#include "base/types.h"

void OpenURL (String str);

uint32 update_crc(uint32 crc, void const* vbuf, uint32 length);
uint32 crc32(uint8* buf, uint32 length);
uint32 crc32(String str);

String getAppPath(bool quoted = false);
bool getRegString(HKEY key, char const* subkey, char const* value, String& result);

#define TIME_HOURS      1
#define TIME_SECONDS    2
String format_time(long time, int flags = TIME_SECONDS);

bool browseForFolder(String prompt, String& result);

uint32 flip_int(uint32 i);

uint64 sysTime();
String format_systime(uint64 time, char const* fmt);

struct FileInfo
{
  String path;
  uint64 ftime;
  uint32 size;
  SYSTEMTIME time;
};
bool getFileInfo(char const* path, FileInfo& fi);
bool pathFileExists(char const* path);

bool createPath(String path);

int SHFileOperationEx(SHFILEOPSTRUCT* pFileOp);

String getOpenReplayName(HWND hWnd);

class Locker
{
  CRITICAL_SECTION* sect;
public:
  Locker(CRITICAL_SECTION& lock)
  {
    sect = &lock;
    EnterCriticalSection(sect);
  }
  ~Locker()
  {
    LeaveCriticalSection(sect);
  }
};

#endif // __BASE_UTILS_H__
