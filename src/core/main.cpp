#include <windows.h>

#include "core/app.h"
#include "ui/mainwnd.h"

static const char mutexId[] = "DotaReplayMutex-7e30e6f8-87ce-4f61-a5c4-1de3e8983fba";
static const char mapId[] = "DotaReplayMap-7e30e6f8-87ce-4f61-a5c4-1de3e8983fba";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  HANDLE hMutex = CreateMutex(NULL, TRUE, mutexId);

  String cmdLine(lpCmdLine);
  cmdLine.trim();
  if (cmdLine.substring(0, 4).icompare("-log") == 0)
    cmdLine = "";
  bool alreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS && !cmdLine.isEmpty());
  if (alreadyExists && hMutex)
    WaitForSingleObject(hMutex, 2000);

  HANDLE map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, mapId);
  void* mapView = NULL;
  if (map != INVALID_HANDLE_VALUE)
  {
    mapView = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 4);
    if (mapView && alreadyExists)
    {
      HWND hWnd = *(HWND*) mapView;
      if (hWnd)
      {
        COPYDATASTRUCT cd;
        cd.dwData = MAINWND_OPEN_REPLAY;
        cd.cbData = cmdLine.length() + 1;
        cd.lpData = cmdLine.getBuffer();
        SendMessage(hWnd, WM_COPYDATA, NULL, (LPARAM) &cd);
        SetForegroundWindow(hWnd);

        UnmapViewOfFile(mapView);
        CloseHandle(map);
        if (hMutex)
          ReleaseMutex(hMutex);
        return 0;
      }
    }
  }

  Application app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  if (!alreadyExists && mapView)
    *(HWND*) mapView = app.getMainWindow();
  if (hMutex)
    ReleaseMutex(hMutex);

  int result = app.run();

  if (mapView)
    UnmapViewOfFile(mapView);
  if (map)
    CloseHandle(map);

  return result;
}
