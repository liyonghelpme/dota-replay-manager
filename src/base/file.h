#ifndef __BASE_FILE_H__
#define __BASE_FILE_H__

#include "base/types.h"
#include "base/string.h"

#ifdef getc
#undef getc
#undef putc
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

class File;
class FileLoader
{
public:
  virtual ~FileLoader () {}

  virtual File* load (char const* name) = 0;

  static FileLoader* system;
};

class File
{
  int refCount;
public:
  File ()
  {
    refCount = 1;
  }
  virtual ~File () {}

  File* retain ()
  {
    ++refCount;
    return this;
  }
  int release ()
  {
    if (--refCount)
      return refCount;
    delete this;
    return 0;
  }

  virtual bool readonly () const
  {
    return true;
  }

  virtual int getc () = 0;
  virtual int putc (int c)
  {
    return 0;
  }

  virtual int read (void* buf, int count) = 0;
  virtual int write (void const* buf, int count)
  {
    return 0;
  }

  virtual void seek (int pos, int rel) = 0;
  virtual int tell () const = 0;

  virtual void seek64 (__int64 pos, int rel)
  {
    seek (pos > 0x7FFFFFFF ? 0x7FFFFFFF : int (pos), rel);
  }
  virtual __int64 tell64 () const
  {
    return tell ();
  }

  virtual bool eof () const = 0;

  virtual bool ok () const {return true;}
  virtual void close () {}

  int read_int16 ()
  {
    short res;
    read (&res, sizeof res);
    return res;
  }
  int read_int32 ()
  {
    int res;
    read (&res, sizeof res);
    return res;
  }
  virtual int read_str (char* str)
  {
    int count = 0;
    while (*str++ = getc ())
      count++;
    return count;
  }
  void write_int16 (int n)
  {
    write (&n, 2);
  }
  void write_int32 (int n)
  {
    write (&n, 4);
  }
  virtual void write_str (char const* str)
  {
    while (*str)
      putc (*str++);
    putc (0);
  }

  int readInt ()
  {
    int res;
    read (&res, sizeof res);
    return res;
  }
  float readFloat ()
  {
    float res;
    read (&res, sizeof res);
    return res;
  }
  virtual String readString ()
  {
    String str = "";
    char chr;
    while (chr = getc ())
      str += chr;
    return str;
  }
  void writeInt (int i)
  {
    write (&i, sizeof i);
  }
  void writeFloat (float f)
  {
    write (&f, sizeof f);
  }
  void writeString (String str)
  {
    write (str.c_str (), str.length () + 1);
  }

  void printf (char const* fmt, ...);

  enum {READ, REWRITE, MODIFY};

  static File* null;

  static File* open (char const* filename, int mode = READ);
  static File* longopen (char const* filename, int mode = READ);
  static File* openURL (char const* url);
  static File* load (char const* filename, FileLoader* loader = NULL)
  {
    if (loader)
      return loader->load (filename);
    else
      return open (filename);
  }
  static File* memfile (void const* data, int length);
};
class TempFile
{
  File* ptr;
public:
  TempFile (File* file)
    : ptr (file)
  {}
  ~TempFile ()
  {
    delete ptr;
  }
  operator File* ()
  {
    return ptr;
  }
  File& operator * ()
  {
    return *ptr;
  }
};

class SystemLoader : public FileLoader
{
  String path;
public:
  SystemLoader (String thePath)
  {
    path = thePath;
  }
  File* load (char const* name)
  {
    return File::open (String::buildFullName (path, name));
  }
};

#endif // __BASE_FILE_H__
