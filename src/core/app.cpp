#include "app.h"

#include "base/mpqfile.h"
#include "base/utils.h"
#include "graphics/image.h"
#include "graphics/imagelib.h"
#include "frameui/framewnd.h"
#include "frameui/controlframes.h"
#include "frameui/fontsys.h"
#include "ui/mainwnd.h"
#include "dota/dotadata.h"
#include "ui/updatedlg.h"

#include "replay/replay.h"
#include "replay/cache.h"

#include "base/dictionary.h"

#include "script/data.h"

#include <shlwapi.h>

Application* Application::instance = NULL;

Application::Application(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  MPQInit();
  ScriptType::initTypes();
  instance = this;
  resources = NULL;
  imageLibrary = NULL;
  dotaLibrary = NULL;
  mainWindow = NULL;
  cache = NULL;
  hInstance = _hInstance;
  _loaded = false;

  UpdateDialog::init(hInstance);

  root = String::getPath(getAppPath());

  INITCOMMONCONTROLSEX iccex;
  iccex.dwSize = sizeof iccex;
  iccex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS |
      ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES |
      ICC_TAB_CLASSES | ICC_UPDOWN_CLASS;
  InitCommonControlsEx(&iccex);
  LoadLibrary("Riched20.dll");
  OleInitialize(NULL);

  cfg.read();

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

  warLoader = new MPQLoader("Custom_V1");
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3x.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3xlocal.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3patch.mpq"));

  imageLibrary = new ImageLibrary(resources);

  cache = new CacheManager();
  dotaLibrary = new DotaLibrary();

  //dotaLibrary->getDota(parseVersion("6.74c"),
  //  "K:\\Progs\\DotAReplay\\docs\\maps\\DotA v6.74c.w3x");
  WIN32_FIND_DATA data;
  String enumPath = "K:\\Progs\\DotAReplay\\docs\\maps";
  HANDLE hFind = FindFirstFile(String::buildFullName(enumPath, "*"), &data);
  BOOL success = (hFind != INVALID_HANDLE_VALUE);
  while (success)
  {
    String file(data.cFileName);
    if (String::getExtension(file).icompare(".w3x") == 0)
    {
      file.toLower();
      Array<String> sub;
      if (file.rfind("dota{{_| }allstars}?{_| }v(\\d)\\.(\\d\\d)([b-z]?)[^b-z]", 0, &sub) >= 0)
      {
        int major = sub[1].toInt();
        int minor = sub[2].toInt();
        int build = 0;
        if (!sub[3].isEmpty())
          build = int(sub[3][0] - 'a');
        uint32 version = makeVersion(major, minor, build);

        dotaLibrary->getDota(version, String::buildFullName(enumPath, file));
      }
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);

  mainWindow = new MainWnd();
  
  mainWindow->postLoad();
  _loaded = true;
}
void Application::reloadWarData()
{
  warLoader->lock();
  warLoader->clear();
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3x.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3xlocal.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3patch.mpq"));
  warLoader->unlock();
}
Application::~Application()
{
  delete mainWindow;
  resources->flush();
  delete dotaLibrary;
  delete imageLibrary;
  delete resources;
  delete cache;
  delete warLoader;

  cfg.write();
  instance = NULL;
  ScriptType::freeTypes();
  MPQCleanup();
  OleFlushClipboard();
  OleUninitialize();
}
void loadDotaData(MPQLoader* ldr, String path);

int Application::run()
{
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

HWND Application::getMainWindow() const
{
  return (mainWindow ? mainWindow->getHandle() : NULL);
}
