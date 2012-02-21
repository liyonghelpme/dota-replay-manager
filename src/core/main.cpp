#include <windows.h>

#include "core/app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Application app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  return app.run();
}
