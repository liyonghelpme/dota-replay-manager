#include "base/file.h"
#include "core/app.h"

#include "registry.h"

Config cfg;

Config::Config()
{
#define regbasic(t,n,v)     items.set(#n,ItemData(iBasic,sizeof(t),&n))
#define regstring(n,v)      items.set(#n,ItemData(iString,0,&n))
#define regarray(t,n,s)     items.set(#n,ItemData(iBasic,sizeof(t)*s,n))
#define regvararray(t,n)    items.set(#n,ItemData(iArray,sizeof(t),&n))
#include "cfgitems.h"
#undef regconst
#undef regbasic
#undef regstring
#undef regarray
#undef regvararray
  reset();
}

void Config::reset()
{
#define cfginit
#define regbasic(t,n,v)     n = v
#define regstring(n,v)      n = v
#define regarray(t,n,s)
#define regvararray(t,n)
#include "cfgitems.h"
#undef regconst
#undef regbasic
#undef regstring
#undef regarray
#undef regvararray
#undef cfginit
}

void Config::read()
{
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "config.cfg"), File::READ);
  if (file)
  {
    uint32 tag = file->read_int32();
    if (tag == 'CFG2')
    {
      int count = file->read_int32();
      for (int i = 0; i < count; i++)
      {
        String name = file->readString();
        uint32 type = file->getc();
        uint32 size = file->read_int32();
        ItemData* it = items.getptr(name);
        if (it && it->type == type && it->size == size)
        {
          if (it->type == iBasic)
            file->read(it->ptr, it->size);
          else if (it->type == iString)
            *(String*) it->ptr = file->readString();
          else if (it->type == iArray)
          {
            int count = file->read_int32();
            ArrayBase* a = (ArrayBase*) it->ptr;
            a->clear();
            for (int i = 0; i < count; i++)
              file->read(a->vpush(), it->size);
          }
        }
        else
        {
          if (type == iBasic)
            file->seek(size, SEEK_CUR);
          else if (type == iString)
            file->readString();
          else if (type == iArray)
          {
            int count = file->read_int32();
            file->seek(size * count, SEEK_CUR);
          }
        }
      }
    }
    delete file;
  }
}
void Config::write()
{
  File* file = File::open(String::buildFullName(getApp()->getRootPath(), "config.cfg"), File::REWRITE);
  if (file)
  {
    file->write_int32('CFG2');
    int count = 0;
    file->write_int32(count);
    for (uint32 cur = items.enumStart(); cur; cur = items.enumNext(cur))
    {
      ItemData& it = items.enumGetValue(cur);
      file->writeString(items.enumGetKey(cur));
      file->putc(it.type);
      file->write_int32(it.size);
      if (it.type == iBasic)
        file->write(it.ptr, it.size);
      else if (it.type == iString)
        file->writeString(*(String*) it.ptr);
      else if (it.type == iArray)
      {
        ArrayBase* a = (ArrayBase*) it.ptr;
        file->write_int32(a->length());
        for (int i = 0; i < a->length(); i++)
          file->write(a->vget(i), it.size);
      }
      count++;
    }
    file->seek(4, SEEK_SET);
    file->write_int32(count);
    delete file;
  }
}
