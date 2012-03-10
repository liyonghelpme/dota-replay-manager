#include "app.h"

#include "base/mpqfile.h"
#include "base/utils.h"
#include "graphics/image.h"
#include "graphics/imagelib.h"
#include "frameui/framewnd.h"
#include "frameui/controlframes.h"
#include "ui/mainwnd.h"
#include "dota/dotadata.h"

#include "replay/replay.h"

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
  delete dotaLibrary;
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

  dotaLibrary = new DotaLibrary();

  resources = new MPQArchive("resources.mpq");
  imageLibrary = new ImageLibrary(resources);

  W3GReplay* replay = W3GReplay::load("destroy.w3g");
  delete replay;

  MainWnd mainWnd;

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
