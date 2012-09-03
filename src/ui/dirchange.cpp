#include "dirchange.h"

DirChangeTracker* DirChangeTracker::trackers = NULL;

uint32 WINAPI DirChangeTracker::threadProc(void* arg)
{
  DirChangeTracker* dc = (DirChangeTracker*) arg;
  HANDLE waits[2] = {dc->findChange, dc->terminate};
  while (WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0)
  {
    if (dc->state == sRunning)
      dc->handler->onDirChange();
    else
      dc->state = sFrozenPending;
    if (!FindNextChangeNotification(dc->findChange))
      break;
    if (WaitForSingleObject(dc->terminate, 1000) != WAIT_TIMEOUT)
      break;
  }
  return 0;
}
DirChangeTracker::DirChangeTracker(DirChangeHandler* theHandler, String path, bool subtree, uint32 filter)
{
  prev = NULL;
  next = trackers;
  if (trackers)
    trackers->prev = this;
  trackers = this;
  state = sRunning;

  thread = NULL;
  handler = theHandler;
  terminate = CreateEvent(NULL, TRUE, FALSE, NULL);
  findChange = FindFirstChangeNotification(path, subtree, filter);
  if (findChange != INVALID_HANDLE_VALUE && terminate != NULL)
  {
    uint32 id;
    thread = CreateThread(NULL, 0, threadProc, this, 0, &id);
    if (!thread)
    {
      FindCloseChangeNotification(findChange);
      findChange = INVALID_HANDLE_VALUE;
      CloseHandle(terminate);
      terminate = NULL;
    }
  }
}

DirChangeTracker::~DirChangeTracker()
{
  if (prev)
    prev->next = next;
  else
    trackers = next;
  if (next)
    next->prev = prev;

  if (findChange != INVALID_HANDLE_VALUE && terminate && thread)
  {
    SignalObjectAndWait(terminate, thread, 1000, FALSE);
    FindCloseChangeNotification(findChange);
    CloseHandle(terminate);
    CloseHandle(thread);
  }
}

void DirChangeTracker::freezeUpdates(bool freeze)
{
  for (DirChangeTracker* cur = trackers; cur; cur = cur->next)
  {
    if (freeze)
    {
      if (cur->state == sRunning)
        cur->state = sFrozen;
    }
    else
    {
      if (cur->state == sFrozenPending)
        cur->handler->onDirChange();
      cur->state = sRunning;
    }
  }
}
