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
#include "replay/actiondump.h"

#include "base/dictionary.h"

#include "script/data.h"

#include <shlwapi.h>

Application* Application::instance = NULL;

bool Application::logCommand(String cmd)
{
  Array<String> args;
  if (cmd.split(args, " \t", SPLIT_NOEMPTY | SPLIT_QUOTES))
  {
    if (args.length() >= 2 && args[0].substr(0, 4).icompare("-log") == 0)
    {
      int detail = 0;
      if (args[0][4] == '=')
        detail = int(args[0][5] - '0');
      if (detail < 0) detail = 0;
      if (detail > 2) detail = 2;

      File* log = File::open(args.length() > 2 ? args[2] : "log.txt", File::REWRITE);
      if (log)
      {
        uint32 error;
        W3GReplay* w3g = W3GReplay::load(args[1], 2, &error);
        if (w3g)
        {
          FileActionLogger logger(log);
          ActionDumper dumper(w3g);
          dumper.dump(&logger, detail);
          delete w3g;
        }
        else
        {
          if (error == W3GReplay::eNoFile)
            log->printf("Failed to open %s\r\n", args[1]);
          else if (error == W3GReplay::eBadFile)
            log->printf("Failed to parse replay\r\n", args[1]);
        }
        delete log;
      }

      return true;
    }
  }
  return false;
}
Application::Application(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  MPQInit();
  instance = this;
  resources = NULL;
  imageLibrary = NULL;
  dotaLibrary = NULL;
  mainWindow = NULL;
  cache = NULL;
  hInstance = _hInstance;
  _loaded = false;

  root = String::getPath(getAppPath());
  cfg.read();

  warLoader = new MPQLoader("Custom_V1");
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3x.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3xlocal.mpq"));
  warLoader->loadArchive(String::buildFullName(cfg.warPath, "war3patch.mpq"));

  if (logCommand(lpCmdLine))
    return;

  ScriptType::initTypes();
  UpdateDialog::init(hInstance);

  INITCOMMONCONTROLSEX iccex;
  iccex.dwSize = sizeof iccex;
  iccex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS |
      ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES |
      ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_DATE_CLASSES;
  InitCommonControlsEx(&iccex);
  LoadLibrary("Riched20.dll");
  OleInitialize(NULL);

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

  cache = new CacheManager();
  dotaLibrary = new DotaLibrary();

#if 0
  File* dlog = File::open("diff.txt", File::REWRITE);
  for (int pt = 0; pt < 120; pt++)
  {
    String prev = "";
    bool different = false;
    for (int ver = 1; ver <= 80 && !different; ver++)
    {
      Dota* dota = dotaLibrary->getDota(makeVersion(6, ver));
      if (dota)
      {
        Dota::Hero* hero = dota->getHero(pt);
        if (hero)
        {
          if (prev == "")
            prev = hero->name;
          else if (prev.icompare(hero->name))
            different = true;
        }
      }
    }
    if (different)
    {
      dlog->printf("  Pt=%d\r\n", pt);
      prev = "";
      for (int ver = 1; ver <= 80; ver++)
      {
        Dota* dota = dotaLibrary->getDota(makeVersion(6, ver));
        if (dota)
        {
          Dota::Hero* hero = dota->getHero(pt);
          if (hero)
          {
            if (prev.icompare(hero->name))
            {
              dlog->printf("6.%02d = %s\r\n", ver, hero->name);
              prev = hero->name;
            }
          }
        }
      }
    }
  }
  delete dlog;
#endif
#if 0
  dotaLibrary->getDota(parseVersion("6.79e"),
    "K:\\Progs\\DotAReplay\\maps\\DotA v6.79e.w3x");
  WIN32_FIND_DATA data;
  String enumPath = "K:\\Progs\\DotAReplay\\maps";
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
#endif

  mainWindow = new MainWnd();
  
  mainWindow->postLoad();
  _loaded = true;

  if (lpCmdLine[0])
  {
    COPYDATASTRUCT cd;
    cd.dwData = MAINWND_OPEN_REPLAY;
    cd.cbData = strlen(lpCmdLine) + 1;
    cd.lpData = lpCmdLine;
    PostMessage(getMainWindow(), WM_COPYDATA_FAKE, NULL, (LPARAM) &cd);
  }
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
  if (resources)
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

int Application::run()
{
  if (mainWindow)
  {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    return msg.wParam;
  }
  else
    return 0;
}

HWND Application::getMainWindow() const
{
  return (mainWindow ? mainWindow->getHandle() : NULL);
}
