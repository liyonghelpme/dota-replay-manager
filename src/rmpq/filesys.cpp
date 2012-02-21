#include "rmpq.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windows.h>

uint32 file_open (char const* filename, int mode)
{
  int omd = 0;
  if (mode & MPQFILE_TEXT)
    omd |= O_TEXT;
  else
    omd |= O_BINARY;
  if ((mode & 3) == MPQFILE_READ)
    omd |= O_RDONLY;
  else if ((mode & 3) == MPQFILE_REWRITE)
    omd |= O_WRONLY | O_TRUNC | O_CREAT;
  else
    omd |= O_RDWR;
  if (omd & O_CREAT)
    return open (filename, omd, S_IREAD | S_IWRITE) + 1;
  else
    return open (filename, omd) + 1;
}
void file_close (uint32 file)
{
  close (file - 1);
}
uint32 file_read (void* buf, uint32 size, uint32 file)
{
  return read (file - 1, buf, size);
}
uint32 file_write (void const* buf, uint32 size, uint32 file)
{
  return write (file - 1, buf, size);
}
void file_seek (uint32 file, int64 pos, int mode)
{
  _lseeki64 (file - 1, pos, mode);
}
int64 file_tell (uint32 file)
{
  return _telli64 (file - 1);
}
void file_delete (char const* filename)
{
  int res = DeleteFile (filename);
}

