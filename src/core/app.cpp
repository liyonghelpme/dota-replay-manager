#include "app.h"

#include "base/mpqfile.h"
#include "base/utils.h"
#include "graphics/image.h"
#include "graphics/imagelib.h"
#include "frameui/framewnd.h"
#include "frameui/controlframes.h"
#include "ui/mainwnd.h"

Application* Application::instance = NULL;

Application::Application(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  instance = this;
  registry = NULL;
  resources = NULL;
  imageLibrary = NULL;
  hInstance = _hInstance;
}
Application::~Application()
{
  delete imageLibrary;
  delete resources;
  delete registry;
  instance = NULL;
}

int Application::run()
{
  root = String::getPath(getAppPath());

  INITCOMMONCONTROLSEX iccex;
  iccex.dwSize = sizeof iccex;
  iccex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_TREEVIEW_CLASSES;
  InitCommonControlsEx(&iccex);

  MPQInit();

  registry = new Registry();

  resources = new MPQArchive("resources.mpq");
  imageLibrary = new ImageLibrary(resources);

  MainWnd mainWnd;

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
