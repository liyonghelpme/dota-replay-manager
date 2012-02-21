#ifndef __UI_DIRCHANGE_H__
#define __UI_DIRCHANGE_H__

#include <windows.h>
#include "base/string.h"
#include "base/types.h"

class DirChangeHandler
{
public:
  virtual void onDirChange() = 0;
};

class DirChangeTracker
{
  HANDLE findChange;
  HANDLE thread;
  DirChangeHandler* handler;
  bool terminate;
  static uint32 WINAPI threadProc(void* arg);
public:
  DirChangeTracker(DirChangeHandler* handler, String path, bool subtree,
    uint32 filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
  ~DirChangeTracker();
};

#endif // __UI_DIRCHANGE_H__
