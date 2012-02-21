#include "dirchange.h"

uint32 WINAPI DirChangeTracker::threadProc(void* arg)
{
  DirChangeTracker* dc = (DirChangeTracker*) arg;
  while (WaitForSingleObject(dc->findChange, INFINITE) == WAIT_OBJECT_0)
  {
    if (dc->terminate)
      break;
    dc->handler->onDirChange();
    if (!FindNextChangeNotification(dc->findChange))
      break;
  }
  return 0;
}
DirChangeTracker::DirChangeTracker(DirChangeHandler* theHandler, String path, bool subtree, uint32 filter)
{
  thread = INVALID_HANDLE_VALUE;
  handler = theHandler;
  terminate = false;
  findChange = FindFirstChangeNotification(path, subtree, filter);
  if (findChange != INVALID_HANDLE_VALUE)
  {
    uint32 id;
    thread = CreateThread(NULL, 0, threadProc, this, 0, &id);
    if (!thread)
    {
      FindCloseChangeNotification(findChange);
      findChange = INVALID_HANDLE_VALUE;
    }
  }
}
DirChangeTracker::~DirChangeTracker()
{
  if (findChange != INVALID_HANDLE_VALUE && thread)
  {
    terminate = true;
    SignalObjectAndWait(findChange, thread, 1000, FALSE);
    FindCloseChangeNotification(findChange);
    CloseHandle(thread);
  }
}
