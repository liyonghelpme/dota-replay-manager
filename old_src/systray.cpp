#include "stdafx.h"
#include "systray.h"
#include <shellapi.h>
#include "stdio.h"
#include "resource.h"

static HWND hWnd;
static bool iconOn = false;
static HICON curIcon;

void CreateTrayIcon (HINSTANCE hInstance, HWND phWnd)
{
  curIcon = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_TRAYICON));
  hWnd = phWnd;
  if (iconOn) return;
  iconOn = true;
  NOTIFYICONDATA nid;
  memset (&nid, 0, sizeof nid);
  nid.cbSize = sizeof nid;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
  nid.uCallbackMessage = WM_TRAYNOTIFY;
  nid.hIcon = curIcon;
  strcpy (nid.szTip, "DotA Replay Manager");
  Shell_NotifyIcon (NIM_ADD, &nid);
  iconOn = true;
}

void SetTrayIcon (HICON id)
{
  if (curIcon == id)
    return;
  curIcon = id;
  SendMessage (hWnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) curIcon);
  if (!iconOn) return;
  NOTIFYICONDATA nid;
  memset (&nid, 0, sizeof nid);
  nid.cbSize = sizeof nid;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  nid.uFlags = NIF_ICON;
  nid.hIcon = curIcon;

  Shell_NotifyIcon (NIM_MODIFY, &nid);
}

void SetTrayText (char* fmt, ...)
{
  if (!iconOn) return;
  NOTIFYICONDATA nid;
  memset (&nid, 0, sizeof nid);
  nid.cbSize = sizeof nid;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  nid.uFlags = NIF_TIP;

  va_list ap;
  va_start (ap, fmt);
  vsprintf (nid.szTip, fmt, ap);
  va_end (ap);

  Shell_NotifyIcon (NIM_MODIFY, &nid);
}

void DeleteTrayIcon ()
{
  if (!iconOn) return;
  NOTIFYICONDATA nid;
  memset (&nid, 0, sizeof nid);
  nid.cbSize = sizeof nid;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  Shell_NotifyIcon (NIM_DELETE, &nid);
  iconOn = false;
}

HICON GetTrayIcon ()
{
  return curIcon;
}

void TrayNotify (char const* title, char const* text)
{
  if (!iconOn) return;
  NOTIFYICONDATA nid;
  memset (&nid, 0, sizeof nid);
  nid.cbSize = sizeof nid;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  nid.uFlags = NIF_INFO;

  strcpy (nid.szInfo, text);
  nid.uTimeout = 10000;
  strcpy (nid.szInfoTitle, title);
  nid.dwInfoFlags = NIIF_INFO;

  BOOL result = Shell_NotifyIcon (NIM_MODIFY, &nid);
  if (!result)
  {
    int asdf = GetLastError ();
    int cry = 0;
  }
}
