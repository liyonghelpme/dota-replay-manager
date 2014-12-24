#include "core/app.h"

#include <windows.h>
#include <shlobj.h>
#include <time.h>

#include "base/string.h"

#include "utils.h"

void OpenURL(String url)
{
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

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

String getAppPath(bool quoted)
{
  char const* pt = GetCommandLine();
  char esym = ' ';
  int end = 0;
  if (*pt == '"')
  {
    if (!quoted)
      pt++;
    end = 1;
    esym = '"';
  }
  while (pt[end] && pt[end] != esym)
    end++;
  String path = String(pt, end);
  for (int i = 0; i < path.length(); i++)
    if (path[i] == '/')
      path.replace(i, '\\');
  if (quoted && esym == '"')
    path += '"';
  return path;
}

bool getRegString(HKEY key, char const* subkey, char const* value, String& result)
{
  HKEY hKey;
  if (RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    return false;
  uint32 size;
  if (RegQueryValueEx(hKey, value, 0, NULL, NULL, &size) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    return false;
  }
  result.resize(size);
  if (RegQueryValueEx(hKey, value, 0, NULL, (LPBYTE) result.getBuffer(), &size) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    return false;
  }
  RegCloseKey(hKey);
  if (result.getBuffer()[size - 1] == 0)
    size--;
  result.setLength(size);
  return true;
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

bool browseForFolder(String prompt, String& result)
{
  BROWSEINFO bi;
  memset (&bi, 0, sizeof bi);
  bi.lpszTitle = prompt;
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
  ITEMIDLIST* list = SHBrowseForFolder (&bi);
  if (list == NULL)
    return false;

  char buf[MAX_PATH];
  SHGetPathFromIDList (list, buf);
  result = String::fixPath(buf);

  LPMALLOC ml;
  SHGetMalloc (&ml);
  ml->Free (list);
  ml->Release ();

  return true;
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

bool getFileInfo(char const* path, FileInfo& fi)
{
  fi.path = path;
  fi.ftime = 0;
  HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_DELETE |
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    FILETIME ft;
    GetFileTime(hFile, NULL, NULL, &ft);
    fi.size = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    fi.ftime = uint64(ft.dwLowDateTime) | (uint64(ft.dwHighDateTime) << 32);
    fi.ftime = fi.ftime / 10000000ULL - 11644473600ULL;
    FileTimeToSystemTime(&ft, &fi.time);
    return true;
  }
  return false;
}
bool pathFileExists(char const* path)
{
  HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_DELETE |
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hFile);
    return true;
  }
  return false;
}

bool createPath(String path)
{
  for (int i = 0; i <= path.length(); i++)
    if (path[i] == '\\')
      CreateDirectory(path.substring(0, i + 1), NULL);
  return true;
}

#include "ui/dirchange.h"

int SHFileOperationEx(SHFILEOPSTRUCT* pFileOp)
{
  if (pFileOp->wFunc == FO_RENAME)
    return SHFileOperation(pFileOp);
  DirChangeTracker::freezeUpdates(true);
  int result = SHFileOperation(pFileOp);
  DirChangeTracker::freezeUpdates(false);
  return result;
}

String getOpenReplayName(HWND hWnd, char const* filter)
{
  if (hWnd == NULL)
    hWnd = getApp()->getMainWindow();

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof ofn);
  ofn.lStructSize = sizeof ofn;
  ofn.hwndOwner = hWnd;
  ofn.lpstrFilter = (filter ? filter : "Warcraft III Replay Files (*.w3g)\0*.w3g\0All Files\0*\0\0");
  char buf[512] = "";
  ofn.lpstrFile = buf;
  ofn.nMaxFile = sizeof buf;
  ofn.lpstrDefExt = "w3g";
  ofn.lpstrInitialDir = cfg.replayPath;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  ofn.FlagsEx = OFN_EX_NOPLACESBAR;
  if (!GetOpenFileName(&ofn))
    return "";
  return buf;
}
