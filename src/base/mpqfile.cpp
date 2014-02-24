#include <windows.h>
#include "mpqfile.h"

int MPQArchive::addRef()
{
  return InterlockedIncrement((LONG*) &ref);
}
int MPQArchive::release()
{
  if (this == NULL)
    return 0;
  int count = InterlockedDecrement((LONG*) &ref);
  if (count == 0)
    delete this;
  return count;
}

MPQLoader::MPQLoader(char const* prefix)
{
  CRITICAL_SECTION* cs = new CRITICAL_SECTION;
  InitializeCriticalSection(cs);
  _lock = (uint32) cs;
  _prefix = (prefix ? prefix : "");
}
MPQLoader::MPQLoader(MPQLoader const& loader)
{
  CRITICAL_SECTION* cs = new CRITICAL_SECTION;
  InitializeCriticalSection(cs);
  _lock = (uint32) cs;
  EnterCriticalSection((CRITICAL_SECTION*) loader._lock);
  _prefix = loader._prefix;
  for (int i = 0; i < loader.archives.length(); i++)
  {
    archives.push(loader.archives[i]);
    archives[i]->addRef();
  }
  LeaveCriticalSection((CRITICAL_SECTION*) loader._lock);
}
MPQLoader::~MPQLoader()
{
  CRITICAL_SECTION* cs = (CRITICAL_SECTION*) _lock;
  EnterCriticalSection(cs);
  for (int i = 0; i < archives.length(); i++)
    archives[i]->release();
  archives.clear();
  LeaveCriticalSection(cs);
  DeleteCriticalSection(cs);
  delete cs;
}

void MPQLoader::lock()
{
  EnterCriticalSection((CRITICAL_SECTION*) _lock);
}
void MPQLoader::unlock()
{
  LeaveCriticalSection((CRITICAL_SECTION*) _lock);
}

void MPQLoader::clear()
{
  EnterCriticalSection((CRITICAL_SECTION*) _lock);
  for (int i = 0; i < archives.length(); i++)
    archives[i]->release();
  archives.clear();
  LeaveCriticalSection((CRITICAL_SECTION*) _lock);
}
void MPQLoader::addArchive(MPQArchive* archive)
{
  EnterCriticalSection((CRITICAL_SECTION*) _lock);
  archive->addRef();
  archives.push(archive);
  LeaveCriticalSection((CRITICAL_SECTION*) _lock);
}
void MPQLoader::removeArchive(MPQArchive* archive)
{
  EnterCriticalSection((CRITICAL_SECTION*) _lock);
  int shift = 0;
  for (int i = 0; i < archives.length(); i++)
  {
    if (archives[i] == archive)
    {
      archives[i]->release();
      shift++;
    }
    else if (shift)
      archives[i - shift] = archives[i];
  }
  LeaveCriticalSection((CRITICAL_SECTION*) _lock);
}
bool MPQLoader::loadArchive(char const* path)
{
  MPQArchive* archive = MPQArchive::open(path, File::READ);
  if (archive)
  {
    EnterCriticalSection((CRITICAL_SECTION*) _lock);
    archives.push(archive);
    LeaveCriticalSection((CRITICAL_SECTION*) _lock);
  }
  return archive != NULL;
}

File* MPQLoader::load(char const* name)
{
  EnterCriticalSection((CRITICAL_SECTION*) _lock);
  String pxname = (_prefix.isEmpty() ? name : String::format("%s\\%s", _prefix, name));
  File* result = NULL;
  for (int i = archives.length() - 1; i >= 0 && result == NULL; i--)
  {
    if (!_prefix.isEmpty())
      result = archives[i]->openFile(pxname, File::READ);
    if (result == NULL)
      result = archives[i]->openFile(name, File::READ);
  }
  LeaveCriticalSection((CRITICAL_SECTION*) _lock);
  return result;
}
