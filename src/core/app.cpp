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
  MPQInit();
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
  MPQCleanup();
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

  registry = new Registry();

  String path = String::getPath(getAppPath());
  String resPath = String::buildFullName(path, "resources.mpq");
  String patchPath = String::buildFullName(path, "install.mpq");
  File* tOpen = File::open(resPath, File::READ);
  if (tOpen == NULL)
  {
    tOpen = File::open(patchPath, File::READ);
    if (tOpen)
    {
      delete tOpen;
      MoveFile(patchPath, resPath);
    }
  }
  else
    delete tOpen;
  resources = MPQArchive::open(resPath);
  MPQArchive* patch = MPQArchive::open(patchPath, File::READ);
  if (patch)
  {
    for (uint32 i = 0; i < patch->getHashSize(); i++)
    {
      char const* name = patch->getFileName(i);
      if (name)
      {
        MPQFile* source = patch->openFile(i, File::READ);
        if (source)
        {
          MPQFile* dest = resources->openFile(name, File::REWRITE);
          if (dest)
          {
            static uint8 buf[1024];
            while (int length = source->read(buf, sizeof buf))
              dest->write(buf, length);
            delete dest;
          }
          delete source;
        }
      }
    }
    delete patch;
    DeleteFile(patchPath);
  }

  imageLibrary = new ImageLibrary(resources);

  MainWnd mainWnd;

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
