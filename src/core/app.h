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

class Application
{
  static Application* instance;
  HINSTANCE hInstance;
  friend Application* getApp();
  String root;

  MPQArchive* resources;
  ImageLibrary* imageLibrary;
  Registry* registry;
  DotaLibrary* dotaLibrary;
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
  Registry* getRegistry() const
  {
    return registry;
  }
  DotaLibrary* getDotaLibrary() const
  {
    return dotaLibrary;
  }
  String getRootPath() const
  {
    return root;
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
