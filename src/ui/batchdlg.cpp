#include "core/app.h"

#include "batchdlg.h"
#include "base/array.h"
#include "replay/cache.h"
#include "ui/dirchange.h"

class BatchDialog::BatchJob
{
  int ref;
public:
  int mode;

  BatchFunc* func;
  String format;
  String dest;
  Array<String> source;
  Array<String> output;
  int pos;
  HWND hDlg;
  HANDLE hThread;

  BatchJob(int md, char const* dst, char const* fmt)
  {
    mode = md;
    dest = (dst ? dst : cfg.replayPath);
    format = (fmt ? fmt : cfg.copyFormat);
    func = NULL;

    ref = 1;
    pos = 0;
    hDlg = NULL;
    hThread = NULL;
  }
  BatchJob(int md, BatchFunc* f, String name)
  {
    mode = md;
    func = f;
    dest = name;

    ref = 1;
    pos = 0;
    hDlg = NULL;
    hThread = NULL;
  }
  ~BatchJob()
  {
    if (hThread)
      CloseHandle(hThread);
    delete func;
  }

  int addRef()
  {
    return InterlockedIncrement((LONG*) &ref);
  }
  int release()
  {
    int count = InterlockedDecrement((LONG*) &ref);
    if (count == 0)
      delete this;
    return count;
  }
};

BatchDialog::BatchDialog(int mode, char const* dst, char const* fmt)
{
  job = new BatchJob(mode, dst, fmt);
}
BatchDialog::BatchDialog(int mode, BatchFunc* func, String name)
{
  job = new BatchJob(mode, func, name);
}
BatchDialog::~BatchDialog()
{
  job->release();
}
void BatchDialog::addFolder(String path, bool recursive)
{
  if (job)
  {
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile(String::buildFullName(path, "*"), &data);
    BOOL success = (hFind != INVALID_HANDLE_VALUE);
    while (success)
    {
      if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
      {
        if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
          if (recursive)
            addFolder(String::buildFullName(path, data.cFileName), recursive);
        }
        else if (String::getExtension(data.cFileName).icompare(".w3g") == 0)
          addReplay(String::buildFullName(path, data.cFileName));
      }
      success = FindNextFile(hFind, &data);
    }
    FindClose(hFind);
  }
}
void BatchDialog::addReplay(String path)
{
  if (job)
  {
    job->source.push(path);
    job->output.push("");
  }
}

int BatchDialog::run(HWND hWnd)
{
  if (hWnd == NULL)
    hWnd = getApp()->getMainWindow();
  if (job->mode == mCopy)
    DirChangeTracker::freezeUpdates(true);
  int result = DialogBoxParam(getInstance(), MAKEINTRESOURCE(IDD_BATCHDLG), hWnd,
    DlgProc, (LPARAM) job);
  if (job->mode == mCopy)
    DirChangeTracker::freezeUpdates(false);
  return result;
}

#define WM_UPDATEBAR        (WM_USER+101)
INT_PTR CALLBACK BatchDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  DWORD id;
  BatchJob* job = (BatchJob*) GetWindowLong(hDlg, DWL_USER);
  switch (message)
  {
  case WM_INITDIALOG:
    job = (BatchJob*) lParam;
    SetWindowLong(hDlg, DWL_USER, lParam);
    job->hDlg = hDlg;

    SendMessage(GetDlgItem(hDlg, IDC_BATCHPROGRESS), PBM_SETRANGE, 0,
      MAKELPARAM(0, job->source.length()));
    if (job->mode != mCopy)
      ShowWindow(GetDlgItem(hDlg, IDC_TOFILE), SW_HIDE);

    job->addRef();
    job->hThread = CreateThread(NULL, 0, ThreadProc, job, 0, &id);
    if (job->hThread == NULL)
    {
      job->release();
      MessageBox(hDlg, "Unable to create thread", "Error", MB_ICONERROR | MB_OK);
      job->hDlg = NULL;
      EndDialog(hDlg, IDCANCEL);
    }
    wParam = 0;
    // intentionally skip
  case WM_UPDATEBAR:
    if (wParam < job->source.length())
    {
      if (job->mode == mCache)
      {
        SetWindowText(hDlg, String::format("Caching replays (%d/%d)",
          wParam, job->source.length()));
        SetDlgItemText(hDlg, IDC_FROMFILE, String::format("File: %s", job->source[wParam]));
      }
      else if (job->mode == mCopy)
      {
        SetWindowText(hDlg, String::format("Batch copy (%d/%d)",
          wParam, job->source.length()));
        SetDlgItemText(hDlg, IDC_FROMFILE, String::format("From: %s", job->source[wParam]));
        SetDlgItemText(hDlg, IDC_TOFILE, String::format("To: %s", job->output[wParam]));
      }
      else
      {
        SetWindowText(hDlg, String::format("%s (%d/%d)",
          job->dest, wParam, job->source.length()));
        SetDlgItemText(hDlg, IDC_FROMFILE, String::format("File: %s", job->source[wParam]));
      }
      SendMessage(GetDlgItem(hDlg, IDC_BATCHPROGRESS), PBM_SETPOS, wParam, 0);
    }
    else
    {
      job->hDlg = NULL;
      EndDialog(hDlg, IDOK);
    }
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      job->hDlg = NULL;
      EndDialog(hDlg, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}
DWORD WINAPI BatchDialog::ThreadProc(LPVOID param)
{
  GameCache localCache;
  BatchJob* job = (BatchJob*) param;
  while (job->pos < job->source.length() && job->hDlg)
  {
    GameCache* cache = getApp()->getCache()->getGame(job->source[job->pos], &localCache);
    if (cache && job->mode == mCopy)
    {
      job->output[job->pos] = cache->format(job->format, job->dest, job->source[job->pos]);
      String path = String::fixPath(String::buildFullName(cfg.replayPath, job->output[job->pos]));
      if (createPath(String::getPath(path)))
      {
        if (CopyFile(job->source[job->pos], path, FALSE))
          getApp()->getCache()->duplicate(path, cache);
      }
    }
    if (cache && job->mode == mFunc && job->func)
      job->func->handle(job->source[job->pos], cache);

    job->pos++;
    if (job->hDlg)
      PostMessage(job->hDlg, WM_UPDATEBAR, job->pos, 0);
  }
  job->release();
  return 0;
}
