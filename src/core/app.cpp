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

#include "base/dictionary.h"

Application* Application::instance = NULL;

Application::Application(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  instance = this;
  registry = NULL;
  resources = NULL;
  imageLibrary = NULL;
  dotaLibrary = NULL;
  hInstance = _hInstance;
  _loaded = false;
}
Application::~Application()
{
  resources->flush();
  delete dotaLibrary;
  delete imageLibrary;
  delete resources;
  delete registry;
  instance = NULL;
}
void loadDotaData(MPQLoader* ldr, String path);

int Application::run()
{
  root = String::getPath(getAppPath());

  INITCOMMONCONTROLSEX iccex;
  iccex.dwSize = sizeof iccex;
  iccex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_TREEVIEW_CLASSES;
  InitCommonControlsEx(&iccex);
  LoadLibrary("Riched20.dll");

  MPQInit();

  registry = new Registry();

  resources = new MPQArchive("resources.mpq");
  imageLibrary = new ImageLibrary(resources);

  MainWnd mainWnd;

  //MPQLoader ldr("Custom_V1");
  //String warPath = registry->readString("warPath");
  //ldr.loadArchive(String::buildFullName(warPath, "war3.mpq"));
  //ldr.loadArchive(String::buildFullName(warPath, "war3x.mpq"));
  //ldr.loadArchive(String::buildFullName(warPath, "war3xlocal.mpq"));
  //ldr.loadArchive(String::buildFullName(warPath, "war3patch.mpq"));
  //ldr.loadArchive("K:\\Games\\Warcraft III\\Maps\\Download\\DotA v6.74.w3x");

  //File* f = ldr.load("ReplaceableTextures\\CommandButtons\\BTNMountainGiant.blp");
  //Image img(f);
  //Image i16(16, 16);
  //BLTInfo blt16(&img);
  //blt16.setDstSize(16, 16);
  //i16.blt(blt16);
  //i16.modBrightness(1.16f);
  //i16.sharpen(0.10f);
  //i16.writePNG(TempFile(File::open("sven2.png", File::REWRITE)));
  //delete f;

  //return 0;

  dotaLibrary = new DotaLibrary();
  
  mainWnd.postLoad();
  _loaded = true;

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
