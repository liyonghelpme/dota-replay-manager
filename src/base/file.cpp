#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "file.h"

class NullFile : public File
{
  int pos;
public:
  NullFile()
  {
    pos = 0;
  }
  bool readonly() const
  {
    return false;
  }
  int getc()
  {
    return 0;
  }
  int putc(int c)
  {
    pos++;
    return 1;
  }
  int read(void* buf, int count)
  {
    return 0;
  }
  int write(void const* buf, int count)
  {
    pos += count;
    return count;
  }
  void seek(int pos, int rel)
  {
    if (rel == SEEK_SET) pos = rel;
    else pos += rel;
  }
  int tell() const
  {
    return pos;
  }
  bool ok() const
  {
    return true;
  }
  bool eof() const
  {
    return true;
  }
} f__null;
File* File::null = &f__null;

class LongSysFile : public File
{
  enum {bufSize = 512};
  int file;
  bool ronly;
public:
  LongSysFile(int _file, bool _ronly)
  {
    file = _file;
    ronly = _ronly;
  }
  ~LongSysFile()
  {
    if (file >= 0)
      _close(file);
  }
  void close()
  {
    if (file >= 0)
      _close(file);
    file = -1;
  }
  bool readonly() const
  {
    return ronly;
  }

  int getc()
  {
    char res;
    return _read(file, &res, 1)? res : 0;
  }
  int putc(int c)
  {
    if (!ronly)
      return _write(file, &c, 1);
    else
      return 0;
  }

  int read(void* buf, int count)
  {
    return _read(file, buf, count);
  }
  int write(void const* buf, int count)
  {
    if (!ronly)
      return _write(file, buf, count);
    else
      return 0;
  }

  int read_str(char* str)
  {
    int count = 0;
    char chr;
    while (_read(file, &chr, 1) && (*str++ = chr))
      count++;
    return count;
  }
  String readString()
  {
    String str = "";
    char chr;
    while (_read(file, &chr, 1) && chr)
      str += chr;
    return str;
  }

  void seek(int pos, int rel)
  {
    _lseek(file, pos, rel);
  }
  int tell() const
  {
    return _tell(file);
  }

  virtual void seek64(__int64 pos, int rel)
  {
    _lseeki64(file, pos, rel);
  }
  __int64 tell64() const
  {
    return _telli64(file);
  }

  bool ok() const
  {
    return file >= 0;
  }

  bool eof() const
  {
    return _eof(file) != 0;
  }
};
class SysFile : public File
{
  enum {bufSize = 512};
  FILE* file;
  bool ronly;
  int len;
public:
  SysFile(FILE* _file, bool _ronly)
  {
    file = _file;
    ronly = _ronly;
    len = 0;
    if (file)
    {
      fseek(file, 0, SEEK_END);
      len = ftell(file);
      fseek(file, 0, SEEK_SET);
    }
  }
  ~SysFile()
  {
    if (file)
      fclose(file);
  }
  void close()
  {
    if (file)
      fclose(file);
    file = NULL;
  }
  bool readonly() const
  {
    return ronly;
  }

  int getc()
  {
    int res = fgetc(file);
    if (res == EOF) return 0;
    return int(char(res));
  }
  int putc(int c)
  {
    if (!ronly)
      return fputc(c, file) == c ? 1 : 0;
    else
      return 0;
  }

  int read_str(char* str)
  {
    int count = 0;
    while (*str++ = fgetc(file))
      count++;
    return count;
  }
  String readString()
  {
    String str = "";
    char chr;
    while (chr = fgetc(file))
      str += chr;
    return str;
  }

  int read(void* buf, int count)
  {
    return fread(buf, 1, count, file);
  }
  int write(void const* buf, int count)
  {
    if (!ronly)
      return fwrite(buf, 1, count, file);
    else
      return 0;
  }

  void seek(int pos, int rel)
  {
    fseek(file, pos, rel);
  }
  int tell() const
  {
    return ftell(file);
  }

  bool ok() const
  {
    return file != NULL;
  }

  bool eof() const
  {
    return ftell(file) >= len;
  }
};
class MemFile : public File
{
  char const* ptr;
  int pos;
  int length;
  bool feof;
  bool own;
public:
  MemFile(void const* mem, int len, bool isown = false)
  {
    ptr = (char const*) mem;
    pos = 0;
    length = len;
    feof = false;
    own = isown;
  }
  ~MemFile()
  {
    if (own)
      delete[] ptr;
  }
  void close()
  {
    if (own)
      delete[] ptr;
    ptr = NULL;
  }

  int getc()
  {
    if (feof = (pos >= length))
      return 0;
    return ptr[pos++];
  }

  int read(void* buf, int count)
  {
    if (feof = (pos + count > length))
      count = length - pos;
    if (count)
      memcpy(buf, ptr + pos, count);
    pos += count;
    return count;
  }

  void seek(int _pos, int rel)
  {
    if (rel == SEEK_SET)
      pos = _pos;
    else if (rel == SEEK_CUR)
      pos += _pos;
    else if (rel == SEEK_END)
      pos = length + _pos;
    if (pos < 0) pos = 0;
    if (pos > length) pos = length;
  }
  int tell() const
  {
    return pos;
  }

  bool eof() const
  {
    return pos >= length;
  }

  bool ok() const
  {
    return ptr != NULL;
  }
};

File* File::open(char const* filename, int mode)
{
  FILE* file = NULL;
  if (mode == READ)
    file = fopen (filename, "rb");
  else if (mode == REWRITE)
    file = fopen (filename, "wb");
  else if (mode == MODIFY)
  {
    file = fopen (filename, "r+");
    if (file == NULL)
      file = fopen (filename, "w+");
  }
  if (file)
    return new SysFile (file, mode == READ);
  else
    return NULL;
}
File* File::longopen (char const* filename, int mode)
{
  int file = -1;
  if (mode == READ)
    file = _open (filename, _O_RDONLY | _O_BINARY);
  else if (mode == REWRITE)
    file = _open (filename, _O_CREAT | _O_TRUNC | _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE);
  else if (mode == MODIFY)
    file = _open (filename, _O_RDWR | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE);
  if (file >= 0)
    return new LongSysFile (file, mode == READ);
  else
    return NULL;
}
uint8* __open_url (char const* url, uint32& len);
File* File::openURL (char const* url)
{
  uint32 len;
  uint8* buf = __open_url (url, len);
  return new MemFile (buf, len, true);
}

File* File::memfile (void const* data, int length)
{
  return new MemFile (data, length);
}

static SystemLoader sysLoader ("");
FileLoader* FileLoader::system = &sysLoader;

void File::printf (char const* fmt, ...)
{
  static char buf[1024];

  va_list ap;
  va_start (ap, fmt);

  int len = _vscprintf (fmt, ap);
  char* dst;
  if (len < 1024)
    dst = buf;
  else
    dst = new char[len + 1];
  vsprintf (dst, fmt, ap);
  write (dst, len);
  if (dst != buf)
    delete[] dst;
}
String File::gets()
{
  String result = "";
  while (int c = getc())
  {
    result += c;
    if (c == '\n')
      break;
    if (c == '\r')
    {
      int pos = tell();
      if (getc() == '\n')
        result += '\n';
      else
        seek(pos, SEEK_SET);
      break;
    }
  }
  return result;
}
