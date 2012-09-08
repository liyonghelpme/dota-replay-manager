#include "core/app.h"

#include "base/version.h"
#include "base/file.h"

#include "updatedlg.h"

#pragma comment (lib, "version.lib")

static const char versionURL[] = "http://www.rivsoft.narod.ru/dotareplay_ver.txt";
static const char logURL[] = "http://www.rivsoft.narod.ru/dotareplay.log";
static const char projectURL[] = "http://code.google.com/p/dota-replay-manager/";
static const char forumURL[] = "http://www.playdota.com/forums/showthread.php?p=110886";

HWND UpdateDialog::instance = NULL;
uint32 UpdateDialog::thisVersion = 0;
uint32 UpdateDialog::lastVersion = 0;
String UpdateDialog::changelog("Loading...");
CRITICAL_SECTION UpdateDialog::lock;

INT_PTR CALLBACK UpdateDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    instance = hDlg;
    check(true);
    // intentionally skip
  case WM_UPDATEVERSION:
    EnterCriticalSection(&lock);
    SetDlgItemText(hDlg, IDC_UPDATEINFO, changelog);
    LeaveCriticalSection(&lock);
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_OPENWEB:
      OpenURL(projectURL);
      return TRUE;
    case IDC_OPENFORUM:
      OpenURL(forumURL);
      return TRUE;
    case IDOK:
    case IDCANCEL:
      instance = NULL;
      EndDialog(hDlg, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}
DWORD WINAPI UpdateDialog::ThreadProc(LPVOID param)
{
  File* verFile = File::openURL(versionURL);
  String text = "";
  if (verFile)
  {
    text = verFile->gets();
    delete verFile;
  }
  if (text.isEmpty())
  {
    EnterCriticalSection(&lock);
    changelog = String::format("Current version: %s\r\n\r\nUnable to contact server.",
      formatVersion(thisVersion));
    LeaveCriticalSection(&lock);
  }
  else
  {
    lastVersion = parseVersion(text);
    if (lastVersion > thisVersion)
    {
      String log = "";
      File* logFile = File::openURL(logURL);
      if (logFile)
      {
        String line;
        while (logFile->gets(line))
        {
          line.removeTrailingSpaces();
          if (line.substring(0, 2) == "**")
          {
            int end = 2;
            while (end < line.length() && line[end] != '*' && !s_isspace(line[end]))
              end++;
            uint32 ver = parseVersion(line.substring(2, end));
            if (ver != 0 && ver <= thisVersion)
              break;
          }
          log += line;
          log += "\r\n";
        }
        delete logFile;
      }

      EnterCriticalSection(&lock);
      changelog = String::format("Current version: %s\r\nLatest version: %s\r\n\r\nChangelog:\r\n%s",
        formatVersion(thisVersion), formatVersion(lastVersion), log);
      LeaveCriticalSection(&lock);
    }
    else
    {
      EnterCriticalSection(&lock);
      changelog = String::format("Current version: %s\r\n\r\nUp to date.",
        formatVersion(thisVersion));
      LeaveCriticalSection(&lock);
    }
  }
  EnterCriticalSection(&lock);
  if (instance)
  {
    cfg.lastVersionNotify = lastVersion;
    PostMessage(instance, WM_UPDATEVERSION, 0, 0);
  }
  if (lastVersion > cfg.lastVersionNotify)
  {
    cfg.lastVersionNotify = lastVersion;
    PostMessage(getApp()->getMainWindow(), WM_UPDATEVERSION, 0, 0);
  }
  LeaveCriticalSection(&lock);
  return 0;
}

void UpdateDialog::init(HINSTANCE hInstance)
{
  InitializeCriticalSection(&lock);

  HRSRC hVersion = FindResource(hInstance, MAKEINTRESOURCE(VS_VERSION_INFO),
    RT_VERSION);
  if (hVersion == NULL)
    return;
  HGLOBAL hGlobal = LoadResource(hInstance, hVersion);
  if (hGlobal == NULL)
    return;
  void* versionInfo = LockResource(hGlobal);
  if (versionInfo)
  {
    UINT vLen;
    void* buf;
    String entry = "\\StringFileInfo\\";
    if (VerQueryValue(versionInfo, "\\VarFileInfo\\Translation", &buf, &vLen) && vLen == 4)
      entry.printf("%04X%04X\\", *(int*) buf & 0xFFFF, (*(int*) buf >> 16) & 0xFFFF);
    else
      entry.printf("%04X04B0\\", GetUserDefaultLangID());

    if (VerQueryValue(versionInfo, entry + "ProductVersion", &buf, &vLen))
    {
      thisVersion = parseVersion((char*) buf);
      lastVersion = thisVersion;
    }
  }
  FreeResource(hGlobal);
}
void UpdateDialog::check(bool force)
{
  if (force || (cfg.autoUpdate && sysTime() - cfg.lastVersionCheck > 2 * 3600))
  {
    EnterCriticalSection(&lock);
    changelog = "Loading...";
    LeaveCriticalSection(&lock);
    cfg.lastVersionCheck = sysTime();
    DWORD id;
    HANDLE thread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &id);
    if (thread)
      CloseHandle(thread);
  }
}
int UpdateDialog::run()
{
  if (instance)
    return IDOK;
  return DialogBox(getInstance(), MAKEINTRESOURCE(IDD_UPDATEDLG),
    getApp()->getMainWindow(), DlgProc);
}
