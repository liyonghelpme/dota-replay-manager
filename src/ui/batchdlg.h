#ifndef __UI_BATCHDLG__
#define __UI_BATCHDLG__

#include "base/string.h"

struct GameCache;
class BatchFunc
{
public:
  virtual void handle(String file, GameCache* gc) = 0;
  virtual ~BatchFunc()
  {}
};

class BatchDialog
{
  class BatchJob;
  BatchJob* job;

  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  static DWORD WINAPI ThreadProc(LPVOID param);
public:
  enum {mCache, mCopy, mFunc};
  BatchDialog(int mode, char const* dst = NULL, char const* fmt = NULL);
  BatchDialog(int mode, BatchFunc* func, String name);
  ~BatchDialog();
  void addFolder(String path, bool recursive = true);
  void addReplay(String path);

  int run(HWND hWnd = NULL);
};

#endif // __UI_BATCHDLG__
