#ifndef __CORE_APP_H__
#define __CORE_APP_H__

#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#include "base/string.h"
#include "core/registry.h"

class MPQArchive;
class ImageLibrary;
class DotaLibrary;
class MainWnd;
class CacheManager;

class Application
{
  static Application* instance;
  HINSTANCE hInstance;
  friend Application* getApp();
  String root;

  MPQArchive* resources;
  ImageLibrary* imageLibrary;
  DotaLibrary* dotaLibrary;
  MainWnd* mainWindow;
  CacheManager* cache;

  bool _loaded;
public:
  Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
  ~Application();

  HINSTANCE getInstanceHandle() const
  {
    return hInstance;
  }
  MPQArchive* getResources() const
  {
    return resources;
  }
  ImageLibrary* getImageLibrary() const
  {
    return imageLibrary;
  }
  DotaLibrary* getDotaLibrary() const
  {
    return dotaLibrary;
  }
  CacheManager* getCache() const
  {
    return cache;
  }
  String getRootPath() const
  {
    return root;
  }
  HWND getMainWindow() const;

  bool loaded() const
  {
    return _loaded;
  }

  int run();
};
inline Application* getApp()
{
  return Application::instance;
}
inline HINSTANCE getInstance()
{
  return getApp()->getInstanceHandle();
}

#endif // __CORE_APP_H__
