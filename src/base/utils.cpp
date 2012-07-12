#include <windows.h>
#include <shlobj.h>

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
  if (*pt == '"')
  {
    if (!quoted)
      pt++;
    esym = '"';
  }
  int end = 0;
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
  if (result.getBuffer()[size - 1] != 0)
    size++;
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
