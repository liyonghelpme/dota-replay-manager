#ifndef __UPDATE_DLG__
#define __UPDATE_DLG__

#include "base/string.h"

#define WM_UPDATEVERSION        (WM_USER+1072)

class UpdateDialog
{
  static CRITICAL_SECTION lock;
  static HWND instance;
  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  static DWORD WINAPI ThreadProc(LPVOID param);
public:
  static uint32 thisVersion;
  static uint32 lastVersion;
  static String changelog;

  static void init(HINSTANCE hInstance);
  static int run();
  static void check(bool force = false);
};

#endif // __UPDATE_DLG__
