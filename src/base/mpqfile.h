#ifndef __BASE_MPQFILE_H__
#define __BASE_MPQFILE_H__

#include "base/file.h"
#include "rmpq/rmpq.h"

class MPQListFile
{
  MPQLISTFILE handle;
public:
  MPQListFile(char const* filename = NULL)
  {
    handle = MPQCreateList(filename);
  }
  ~MPQListFile()
  {
    MPQDeleteList(handle);
  }

  uint32 insert(char const* name, bool check = false)
  {
    return MPQListInsert(handle, name, check);
  }
  uint32 size() const
  {
    return MPQListSize(handle);
  }
  char const* item(uint32 pos) const
  {
    return MPQListItem(handle, pos);
  }

  uint32 flush(char const* filename)
  {
    return MPQFlushList(handle, filename);
  }

  MPQLISTFILE getHandle() const
  {
    return handle;
  }
};

class MPQFile : public File
{
  MPQFILE handle;
  bool ronly;
  uint32 size;
  friend class MPQArchive;
  friend class MPQLoader;
  MPQFile(MPQFILE file, bool readonly)
  {
    handle = file;
    ronly = readonly;
    MPQFileSeek(handle, 0, MPQSEEK_END);
    size = MPQFileTell(handle);
    MPQFileSeek(handle, 0, MPQSEEK_SET);
  }
public:
  ~MPQFile()
  {
    MPQCloseFile(handle);
  }
  uint32 reopen()
  {
    return MPQReopenFile(handle);
  }
  uint32 attribute(uint32 attr) const
  {
    return MPQFileAttribute(handle, attr);
  }

  bool readonly() const
  {
    return ronly;
  }

  int read(void* buf, int count)
  {
    return MPQFileRead(handle, count, buf);
  }
  int write(void const* buf, int count)
  {
    return MPQFileWrite(handle, count, buf);
  }
  int getc()
  {
    uint32 c = MPQFileGetc(handle);
    if (MPQError())
      return 0;
    return c;
  }
  int putc(int c)
  {
    return MPQ_ERROR(MPQFilePutc(handle, (uint8) c)) ? 0 : 1;
  }

  void seek(int pos, int rel)
  {
    MPQFileSeek(handle, pos, rel);
  }
  int tell() const
  {
    return MPQFileTell(handle);
  }

  uint32 setFlags(uint32 flags, uint32 mask)
  {
    return MPQFileSetFlags(handle, flags, mask);
  }
  uint32 getFlags() const
  {
    return MPQFileGetFlags(handle);
  }

  uint32 del(uint32 count)
  {
    return MPQFileDel(handle, count);
  }
  uint32 push(uint32 count)
  {
    return MPQFilePush(handle, count);
  }

  uint32 flush()
  {
    return MPQFlushFile(handle);
  }

  bool eof() const
  {
    return MPQFileTell(handle) == size;
  }

  MPQFILE getHandle() const
  {
    return handle;
  }
};

class MPQArchive
{
  MPQARCHIVE handle;
  MPQArchive(MPQARCHIVE mpq)
  {
    handle = mpq;
  }
public:
  MPQArchive(char const* filename, int mode = MPQFILE_MODIFY)
  {
    handle = MPQOpen(filename, mode);
  }
  ~MPQArchive()
  {
    MPQClose(handle);
  }

  static MPQArchive* open(char const* filename, int mode = MPQFILE_MODIFY)
  {
    MPQARCHIVE mpq = MPQOpen(filename, mode);
    if (mpq)
      return new MPQArchive(mpq);
    else
      return NULL;
  }
  static MPQArchive* create(char const* filename, bool listfile = true, uint32 offset = 0,
    uint32 hashSize = 1024, uint32 blockSize = 131072, bool v2 = false)
  {
    MPQARCHIVE mpq = MPQCreateArchive(filename, listfile, offset, hashSize, blockSize, v2);
    if (mpq)
      return new MPQArchive(mpq);
    else
      return NULL;
  }

  bool isOk() const
  {
    return handle != NULL;
  }

  uint32 listFiles(MPQListFile const& list)
  {
    return MPQListFiles(handle, list.getHandle());
  }

  uint32 getCompression() const
  {
    return MPQGetCompression(handle);
  }
  uint32 setCompression(uint32 method)
  {
    return MPQSetCompression(handle, method);
  }
  uint32 saveAs(char const* filename)
  {
    return MPQSaveAs(handle, filename);
  }
  uint32 makeTemp()
  {
    return MPQMakeTemp(handle);
  }

  uint32 findFile(char const* name) const
  {
    return MPQFindFile(handle, name);
  }
  uint32 findFile(char const* name, uint16 locale) const
  {
    return MPQFindFile(handle, name, locale);
  }
  uint32 findNextFile(char const* name, uint32 cur) const
  {
    return MPQFindNextFile(handle, name, cur);
  }
  bool testFile(uint32 pos) const
  {
    return MPQTestFile(handle, pos);
  }

  MPQFile* openFile(char const* name, uint32 options)
  {
    MPQFILE file = MPQOpenFile(handle, name, options);
    if (file)
      return new MPQFile(file, options == MPQFILE_READ);
    else
      return NULL;
  }
  MPQFile* openFile(char const* name, uint16 locale, uint32 options)
  {
    MPQFILE file = MPQOpenFile(handle, name, locale, options);
    if (file)
      return new MPQFile(file, options == MPQFILE_READ);
    else
      return NULL;
  }
  MPQFile* openFile(uint32 pos, uint32 options)
  {
    MPQFILE file = MPQOpenFile(handle, pos, options);
    if (file)
      return new MPQFile(file, options == MPQFILE_READ);
    else
      return NULL;
  }

  uint32 getHashSize() const
  {
    return MPQGetHashSize(handle);
  }

  bool fileExists(char const* name) const
  {
    return MPQFileExists(handle, name);
  }
  bool fileExists(char const* name, uint16 locale) const
  {
    return MPQFileExists(handle, name, locale);
  }
  bool fileExists(uint32 pos) const
  {
    return MPQFileExists(handle, pos);
  }

  uint32 renameFile(char const* source, char const* dest)
  {
    return MPQRenameFile(handle, source, dest);
  }
  uint32 encryptFile(char const* name, uint32 options)
  {
    return MPQEncryptFile(handle, name, options);
  }
  uint32 deleteFile(char const* name)
  {
    return MPQDeleteFile(handle, name);
  }

  uint32 resizeHash(uint32 size)
  {
    return MPQResizeHash(handle, size);
  }
  bool hasUnknowns() const
  {
    return MPQHasUnknowns(handle);
  }
  uint32 fillHashTable()
  {
    return MPQFillHashTable(handle);
  }

  char const* getFileName(uint32 pos) const
  {
    return MPQGetFileName(handle, pos);
  }
  uint32 getFileAttribute(uint32 pos, uint32 attr) const
  {
    return MPQItemAttribute(handle, pos, attr);
  }
  uint32 peekFile(uint32 pos, uint8* dest)
  {
    return MPQPeekFile(handle, pos, dest);
  }

  uint32 flush()
  {
    return MPQFlush(handle);
  }
  uint32 flushListFile()
  {
    return MPQFlushListfile(handle);
  }

  uint32 readMapName(char* buf) const
  {
    return MPQReadMapName(handle, buf);
  }

  MPQARCHIVE getHandle() const
  {
    return handle;
  }
};

class MPQLoader : public FileLoader
{
  MPQLOADER handle;
public:
  MPQLoader(char const* prefix = NULL)
  {
    handle = MPQCreateLoader(prefix);
  }
  ~MPQLoader()
  {
    MPQReleaseLoader(handle);
  }

  uint32 addArchive(MPQArchive const& archive)
  {
    return MPQAddArchive(handle, archive.getHandle());
  }
  uint32 removeArchive(MPQArchive const& archive)
  {
    return MPQRemoveArchive(handle, archive.getHandle());
  }
  uint32 loadArchive(char const* path)
  {
    return MPQLoadArchive(handle, path);
  }

  File* load(char const* name)
  {
    MPQFILE file = MPQLoadFile(handle, name);
    if (file)
      return new MPQFile(file, true);
    else
      return NULL;
  }

  MPQLOADER getHandle() const
  {
    return handle;
  }
};

extern MPQLoader* mpqLoader;

#endif // __BASE_MPQFILE_H__
