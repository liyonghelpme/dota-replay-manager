#include "stdafx.h"
#include "rmpq.h"
#include "mpqcompress.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MPQ_BLOCK_GROW              16
#define MPQ_DATA_GROW               4096

#define MPQ_SHUNT_ID                '\x1BQPM'
#define MPQ_HEADER_ID               '\x1AQPM'

#define MPQ_HASH_OFFSET             0
#define MPQ_HASH_NAME1              1
#define MPQ_HASH_NAME2              2
#define MPQ_HASH_KEY                3
#define MPQ_HASH_ENCRYPT            4

#define MPQ_INDEX_EMPTY             0xFFFFFFFF
#define MPQ_INDEX_DELETED           0xFFFFFFFE

static char const* StripPath (char const* str)
{
  uint32 pos = 0;
  while (str[pos]) pos++;
  while (pos && str[pos - 1] != '\\') pos--;
  return str + pos;
}

struct MPQShunt
{
  //uint32 id; this field is removed on purpose
  uint32 unknown;
  uint32 headerPos;
};
struct MPQHeader
{
  uint32 id;
  uint32 headerSize;
  uint32 archiveSize;
  uint16 formatVersion;
  uint16 blockSize;
  uint32 hashTablePos;
  uint32 blockTablePos;
  uint32 hashTableSize;
  uint32 blockTableSize;
};
struct MPQHeader2
{
  int64 extBlockTablePos;
  uint16 hashTablePosHigh;
  uint16 blockTablePosHigh;
};

struct MPQHash
{
  uint32 name1;
  uint32 name2;
  uint16 locale;
  uint16 platform;
  uint32 blockIndex;
};
struct MPQBlock
{
  uint32 filePos;
  uint32 cSize;
  uint32 fSize;
  uint32 flags;
};

uint32 mpq_error = 0;
uint32 mpq_serror = 0;

uint32 MPQError ()
{
  return mpq_error;
}
uint32 MPQSavedError ()
{
  return mpq_serror;
}
void MPQResetError ()
{
  mpq_serror = 0;
}
char const* MPQErrorMessage (uint32 err)
{
  static char buf[256];
  if (err == MPQ_OK)
    return strcpy (buf, "The operation was completed successfully");
  if (err == MPQ_ERROR_INIT)
    return strcpy (buf, "The MPQ library was not initialized");
  if (err == MPQ_ERROR_NOFILE)
    return strcpy (buf, "File was not found");
  if (err == MPQ_ERROR_NONMPQ)
    return strcpy (buf, "File is not an MPQ archive");
  if (err == MPQ_ERROR_READ)
    return strcpy (buf, "Failed to read archive");
  if (err == MPQ_ERROR_PARAMS)
    return strcpy (buf, "Invalid parameters specified");
  if (err == MPQ_ERROR_DECOMPRESS)
    return strcpy (buf, "Unknown compression method or invalid data");
  if (err == MPQ_EOF)
    return strcpy (buf, "End of file reached");
  if (err == MPQ_ERROR_KEY)
    return strcpy (buf, "Could not generate file encryption seed");
  if (err == MPQ_ERROR_WRITE)
    return strcpy (buf, "Failed to write archive");
  if (err == MPQ_ERROR_FULL)
    return strcpy (buf, "Hash table full");
  if (err == MPQ_ERROR_ACCESS)
    return strcpy (buf, "Denied access to file");
  if (err == MPQ_ERROR_READONLY)
    return strcpy (buf, "Archive/file has been opened as readonly");
  if (err == MPQ_ERROR_COMPRESS)
    return strcpy (buf, "Failed to compress data");
  return strcpy (buf, "Unknown error");
}
char const* MPQLocaleName (uint16 locale)
{
  static char buf[256];
  if (locale == MPQ_LOCALE_NEUTRAL)
    return strcpy (buf, "Neutral");
  if (locale == MPQ_LOCALE_CHINESE)
    return strcpy (buf, "Chinese");
  if (locale == MPQ_LOCALE_CZECH)
    return strcpy (buf, "Czech");
  if (locale == MPQ_LOCALE_GERMAN)
    return strcpy (buf, "German");
  if (locale == MPQ_LOCALE_ENGLISH)
    return strcpy (buf, "English");
  if (locale == MPQ_LOCALE_SPANISH)
    return strcpy (buf, "Spanish");
  if (locale == MPQ_LOCALE_FRENCH)
    return strcpy (buf, "French");
  if (locale == MPQ_LOCALE_ITALIAN)
    return strcpy (buf, "Italian");
  if (locale == MPQ_LOCALE_JAPANESE)
    return strcpy (buf, "Japanese");
  if (locale == MPQ_LOCALE_KOREAN)
    return strcpy (buf, "Korean");
  if (locale == MPQ_LOCALE_POLISH)
    return strcpy (buf, "Polish");
  if (locale == MPQ_LOCALE_PORTUGUESE)
    return strcpy (buf, "Portuguese");
  if (locale == MPQ_LOCALE_RUSSIAN)
    return strcpy (buf, "Russian");
  if (locale == MPQ_LOCALE_ENGLISHUK)
    return strcpy (buf, "English (UK)");
  return strcpy (buf, "Unknown");
}

struct MPQListFile
{
  uint32 size;
  uint32 all;
  mpqname* list;
  MPQListFile ()
  {
    memset (this, 0, sizeof (MPQListFile));
  }
  ~MPQListFile ()
  {
    delete[] list;
  }
};

static const int TableSize = 1280;
static uint32 cryptTable[TableSize];
static bool init = false;
struct MPQArchive;
static MPQArchive* firstArchive = NULL;
struct MPQLoader;
static MPQLoader* firstLoader = NULL;

MPQLISTFILE MPQCreateList (char const* filename)
{
  MPQListFile* list = new MPQListFile;
  if (filename)
  {
    FILE* f = fopen (filename, "rt");
    if (f)
    {
      char line[256];
      while (fgets (line, sizeof line, f))
      {
        uint32 len = 0;
        while (line[len]) len++;
        while (len && (line[len - 1] == '\r' || line[len - 1] == '\n')) line[--len] = 0;
        if (line[0])
          MPQListInsert ((MPQLISTFILE) list, line);
      }
      fclose (f);
    }
  }
  return (MPQLISTFILE) list;
}
uint32 MPQListInsert (MPQLISTFILE handle, char const* name)
{
  MPQListFile* list = (MPQListFile*) handle;
  if (list == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (list->size >= list->all)
  {
    uint32 ns = list->all;
    if (ns == 0)
      ns = MPQ_BLOCK_GROW;
    else while (list->size >= ns)
      ns *= 2;
    mpqname* n = new mpqname[ns];
    memcpy (n, list->list, list->size * sizeof (mpqname));
    delete[] list->list;
    list->list = n;
    list->all = ns;
  }
  strcpy (list->list[list->size++], name);
  return mpq_error = MPQ_OK;
}
uint32 MPQListSize (MPQLISTFILE handle)
{
  MPQListFile* list = (MPQListFile*) handle;
  if (list == NULL)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  return list->size;
}
char const* MPQListItem (MPQLISTFILE handle, uint32 pos)
{
  MPQListFile* list = (MPQListFile*) handle;
  if (list == NULL || pos < 0 || pos >= list->size)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return NULL;
  }
  return list->list[pos];
}
uint32 MPQDeleteList (MPQLISTFILE handle)
{
  MPQListFile* list = (MPQListFile*) handle;
  if (list == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  delete list;
  return mpq_error = MPQ_OK;
}
uint32 MPQFlushList (MPQLISTFILE handle, char const* filename)
{
  MPQListFile* list = (MPQListFile*) handle;
  if (list == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  FILE* file = fopen (filename, "wt");
  for (uint32 i = 0; i < list->size; i++)
    fprintf (file, "%s\n", list->list[i]);
  fclose (file);
  return mpq_error = MPQ_OK;
}

void MPQC_INIT ();
void MPQC_CLEANUP ();
uint32 MPQInit ()
{
  mpq_error = MPQ_OK;
  if (init)
    return mpq_serror = mpq_error = MPQ_ERROR_INIT;
  uint32 seed = 0x00100001;

  for (int i = 0; i < 256; i++)
  {
    for (int j = i; j < TableSize; j += 256)
    {
      seed = (seed * 125 + 3) % 0x2AAAAB;
      uint32 a = (seed & 0xFFFF) << 16;
      seed = (seed * 125 + 3) % 0x2AAAAB;
      uint32 b = (seed & 0xFFFF);
      cryptTable[j] = a | b;
    }
  }

  MPQC_INIT ();

  init = true;
  return mpq_error;
}

static uint32 MPQHashString (char const* str, uint32 hashType)
{
  if (!init)
  {
    mpq_serror = mpq_error = MPQ_ERROR_INIT;
    return 0;
  }
  uint32 seed1 = 0x7FED7FED;
  uint32 seed2 = 0xEEEEEEEE;
  for (int i = 0; str[i]; i++)
  {
    char ch = str[i];
    if (ch >= 'a' && ch <= 'z')
      ch = ch - 'a' + 'A';
    seed1 = cryptTable[hashType * 256 + ch] ^ (seed1 + seed2);
    seed2 = ch + seed1 + seed2 * 33 + 3;
  }
  return seed1;
}

static uint32 MPQEncryptBlock (void* ptr, uint32 size, uint32 key)
{
  if (!init)
    return mpq_serror = mpq_error = MPQ_ERROR_INIT;
  uint32 seed = 0xEEEEEEEE;
  uint32* lptr = (uint32*) ptr;
  size /= sizeof (uint32);
  for (uint32 i = 0; i < size; i++)
  {
    seed += cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
    uint32 orig = lptr[i];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += orig + seed * 32 + 3;
  }
  return 0;
}
static uint32 MPQDecryptBlock (void* ptr, uint32 size, uint32 key)
{
  if (!init)
    return mpq_serror = mpq_error = MPQ_ERROR_INIT;
  uint32 seed = 0xEEEEEEEE;
  uint32* lptr = (uint32*) ptr;
  size /= sizeof (uint32);
  for (uint32 i = 0; i < size; i++)
  {
    seed += cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += lptr[i] + seed * 32 + 3;
  }
  return 0;
}

static uint32 MPQFileTransfer (uint32 in, uint32 out, uint32 count)
{
  static uint8 buf[1024];
  uint32 done = 0;
  while (count == -1 || done < count)
  {
    uint32 to = sizeof buf;
    if (count != -1 && done + to > count)
      to = count - done;
    to = file_read (buf, to, in);
    if (to == 0) break;
    file_write (buf, to, out);
    done += to;
  }
  return done;
}

struct MPQFile;

struct MPQArchive
{
  uint32 file;
  char path[256];
  int64 offset;
  uint32 blockSize;
  // data = 2*blockSize | MPQHeader | hashTable | [fileNames] | blockTable
  uint8* data;
  MPQHeader* hdr;
  mpqname* fileNames;
  MPQHash* hashTable;
  MPQBlock* blockTable;
  MPQHeader2* hdr2;
  uint16* extBlockTable;
  int64 size;
  uint32 xsize;
  uint32 blockTableSpace;
  bool readonly;
  uint32 compressMethods;

  bool writelist;

  MPQArchive* next;
  MPQArchive* prev;
  MPQFile* firstFile;
  bool isTemp;

  MPQArchive ()
  {
    memset (this, 0, sizeof (MPQArchive));
    next = firstArchive;
    if (next)
      next->prev = this;
    firstArchive = this;
  }
  ~MPQArchive ();
};

MPQARCHIVE MPQCreateArchive (char const* filename, bool listfile, uint32 offset, uint32 hashSize, uint32 blockSize, bool v2)
{
  mpq_error = MPQ_OK;
  if (!init)
  {
    mpq_serror = mpq_error = MPQ_ERROR_INIT;
    return 0;
  }
  uint32 file;
  if (offset)
  {
    file = file_open (filename, MPQFILE_READ | MPQFILE_BINARY);
    if (file)
    {
      uint8* data = new uint8[offset];
      memset (data, 0, offset);
      file_read (data, offset, file);
      file_close (file);
      file = file_open (filename, MPQFILE_REWRITE | MPQFILE_BINARY);
      file_write (data, offset, file);
      delete data;
    }
  }
  else
    file = file_open (filename, MPQFILE_REWRITE | MPQFILE_BINARY);
  if (!file)
  {
    mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
    return 0;
  }
  MPQHeader hdr;
  hdr.id = MPQ_HEADER_ID;
  hdr.headerSize = sizeof (MPQHeader);
  if (v2)
    hdr.headerSize += sizeof (MPQHeader2);
  hdr.archiveSize = hdr.headerSize + hashSize * sizeof (MPQHash);
  hdr.formatVersion = (v2 ? 1 : 0);
  hdr.blockSize = 0;
  blockSize >>= 9;
  while (blockSize > 1)
  {
    hdr.blockSize++;
    blockSize >>= 1;
  }
  hdr.hashTablePos = hdr.headerSize;
  hdr.blockTablePos = hdr.archiveSize;
  hdr.hashTableSize = hashSize;
  hdr.blockTableSize = 0;
  if (file_write (&hdr, sizeof hdr, file) != sizeof hdr)
  {
    mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    return 0;
  }
  if (v2)
  {
    MPQHeader2 hdr2;
    hdr2.extBlockTablePos = hdr.archiveSize;
    hdr2.hashTablePosHigh = 0;
    hdr2.blockTablePosHigh = 0;
    if (file_write (&hdr2, sizeof hdr2, file) != sizeof hdr2)
    {
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
      return 0;
    }
  }
  MPQHash* hash = new MPQHash[hashSize];
  uint32 size = hashSize * sizeof (MPQHash);
  memset (hash, 0xFF, size);
  MPQEncryptBlock (hash, size, MPQHashString ("(hash table)", MPQ_HASH_KEY));
  if (file_write (hash, size, file) != size)
  {
    delete[] hash;
    mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    return 0;
  }
  delete[] hash;
  file_close (file);
  return MPQOpen (filename, MPQFILE_MODIFY | (listfile ? 0 : MPQ_NO_LISTFILE));
}

MPQARCHIVE MPQOpen (char const* filename, uint32 mode)
{
  if ((mode & 3) == MPQFILE_REWRITE)
    return MPQCreateArchive (filename, (mode & MPQ_NO_LISTFILE) == 0);
  mpq_error = MPQ_OK;
  if (!init)
  {
    mpq_serror = mpq_error = MPQ_ERROR_INIT;
    return 0;
  }
  bool readonly = ((mode & 3) == MPQFILE_READ);
  uint32 file = file_open (filename, (readonly ? MPQFILE_READ : MPQFILE_MODIFY) | MPQFILE_BINARY);
  if (!file)
  {
    if (mode == MPQFILE_MODIFY)
      return MPQCreateArchive (filename, (mode & MPQ_NO_LISTFILE) == 0);
    mpq_error = MPQ_ERROR_NOFILE;
    return 0;
  }
  file_seek (file, 0, MPQSEEK_END);
  int64 size = file_tell (file);
  // try to locate MPQ structure
  int64 offset = 0;
  for (; offset < size; offset += 512)
  {
    file_seek (file, offset, MPQSEEK_SET);
    uint32 id;
    if (file_read (&id, sizeof id, file) == (sizeof id))
    {
      if (id == MPQ_HEADER_ID)
        break;
      if (id == MPQ_SHUNT_ID)
      {
        MPQShunt shunt;
        if (file_read (&shunt, sizeof shunt, file) == (sizeof shunt))
        {
          offset += shunt.headerPos;
          break;
        }
      }
    }
  }
  if (offset >= size)
  {
    file_close (file);
    mpq_serror = mpq_error = MPQ_ERROR_NONMPQ;
    return 0;
  }
  file_seek (file, offset, MPQSEEK_SET);
  MPQHeader hdr;
  if (file_read (&hdr, sizeof hdr, file) != (sizeof hdr) || hdr.id != MPQ_HEADER_ID)
  {
    file_close (file);
    mpq_serror = mpq_error = MPQ_ERROR_NONMPQ;
    return 0;
  }
#ifdef ERROR
#undef ERROR
#endif
#define ERROR(code) {delete mpq;mpq_serror=mpq_error=code;return 0;}
  MPQArchive* mpq = new MPQArchive;
  mpq->file = file;
  mpq->readonly = readonly;
  mpq->compressMethods = MPQ_ALLOW_ZLIB;
  strcpy (mpq->path, filename);
  mpq->offset = offset;
  mpq->blockSize = (1 << (9 + hdr.blockSize));
  if (mpq->blockSize > 0x200000) // theyre joking...
  {
    MPQBlock* block = new MPQBlock[hdr.blockTableSize];
    file_seek (file, offset + hdr.blockTablePos, MPQSEEK_SET);
    mpq->blockSize = 0x100000;
    if (file_read (block, hdr.blockTableSize * sizeof (MPQBlock), file) ==
      hdr.blockTableSize * sizeof (MPQBlock))
    {
      MPQDecryptBlock (block, hdr.blockTableSize * sizeof (MPQBlock),
        MPQHashString ("(block table)", MPQ_HASH_KEY));
      for (uint32 i = 0; i < hdr.blockTableSize; i++)
        while (mpq->blockSize < block[i].fSize)
          mpq->blockSize *= 2;
    }
    delete[] block;
  }
  mpq->blockTableSpace = hdr.blockTableSize + MPQ_BLOCK_GROW;
  bool nolist = (mode & MPQ_NO_LISTFILE) != 0;
  if (hdr.formatVersion == 0)
  {
    mpq->size = hdr.archiveSize;
    mpq->xsize = sizeof hdr + hdr.hashTableSize * (sizeof (MPQHash) + (nolist ? 0 : sizeof (mpqname))) +
      mpq->blockTableSpace * sizeof (MPQBlock) + mpq->blockSize * 2;
    mpq->data = new uint8[mpq->xsize];
    mpq->hdr = (MPQHeader*) (mpq->data + mpq->blockSize * 2);
    mpq->hashTable = (MPQHash*) (mpq->hdr + 1);
    mpq->fileNames = (mpqname*) (mpq->hashTable + hdr.hashTableSize);
    mpq->blockTable = (MPQBlock*) (mpq->fileNames + (nolist ? 0 : hdr.hashTableSize));
    if (nolist)
      mpq->fileNames = NULL;
    memcpy (mpq->hdr, &hdr, sizeof hdr);
    file_seek (file, offset + hdr.hashTablePos, MPQSEEK_SET);
    if (file_read (mpq->hashTable, hdr.hashTableSize * sizeof (MPQHash), file) !=
      hdr.hashTableSize * sizeof (MPQHash))
      ERROR (MPQ_ERROR_READ);
    file_seek (file, offset + hdr.blockTablePos, MPQSEEK_SET);
    if (file_read (mpq->blockTable, hdr.blockTableSize * sizeof (MPQBlock), file) !=
      hdr.blockTableSize * sizeof (MPQBlock))
      ERROR (MPQ_ERROR_READ);
  }
  else
  {
    MPQHeader2 hdr2;
    if (file_read (&hdr2, sizeof hdr2, file) != (sizeof hdr2))
      ERROR (MPQ_ERROR_READ);
    int64 hashTablePos = hdr.hashTablePos + ((int64) hdr2.hashTablePosHigh << 32);
    int64 blockTablePos = hdr.blockTablePos + ((int64) hdr2.blockTablePosHigh << 32);
    mpq->size = hashTablePos + hdr.hashTableSize * sizeof (MPQHash);
    if (blockTablePos + hdr.blockTableSize * sizeof (MPQBlock) > mpq->size)
      mpq->size = blockTablePos + hdr.blockTableSize * sizeof (MPQBlock);
    if (hdr2.extBlockTablePos + hdr.blockTableSize * sizeof (uint16) > mpq->size)
      mpq->size = hdr2.extBlockTablePos + hdr.blockTableSize * sizeof (uint16);
    mpq->xsize = sizeof hdr + sizeof hdr2 + hdr.hashTableSize * (sizeof (MPQHash) +
      (nolist ? 0 : sizeof (mpqname))) + mpq->blockTableSpace * (sizeof (MPQBlock) + sizeof (uint16)) +
      mpq->blockSize * 2;
    mpq->data = new uint8[mpq->xsize];
    mpq->hdr = (MPQHeader*) (mpq->data + mpq->blockSize * 2);
    mpq->hdr2 = (MPQHeader2*) (mpq->hdr + 1);
    mpq->hashTable = (MPQHash*) (mpq->hdr2 + 1);
    mpq->fileNames = (mpqname*) (mpq->hashTable + hdr.hashTableSize);
    mpq->blockTable = (MPQBlock*) (mpq->fileNames + (nolist ? 0 : hdr.hashTableSize));
    if (nolist)
      mpq->fileNames = NULL;
    mpq->extBlockTable = (uint16*) (mpq->blockTable + mpq->blockTableSpace);
    memcpy (mpq->hdr, &hdr, sizeof hdr);
    memcpy (mpq->hdr2, &hdr2, sizeof hdr2);
    file_seek (file, offset + hashTablePos, MPQSEEK_SET);
    if (file_read (mpq->hashTable, hdr.hashTableSize * sizeof (MPQHash), file) !=
      hdr.hashTableSize * sizeof (MPQHash))
      ERROR (MPQ_ERROR_READ);
    file_seek (file, offset + blockTablePos, MPQSEEK_SET);
    if (file_read (mpq->blockTable, hdr.blockTableSize * sizeof (MPQBlock), file) !=
      hdr.blockTableSize * sizeof (MPQBlock))
      ERROR (MPQ_ERROR_READ);
    file_seek (file, offset + hdr2.extBlockTablePos, MPQSEEK_SET);
    if (file_read (mpq->extBlockTable, hdr.blockTableSize * sizeof (uint16), file) !=
      hdr.blockTableSize * sizeof (uint16))
      ERROR (MPQ_ERROR_READ);
  }
  if (mpq->fileNames)
    memset (mpq->fileNames, 0, sizeof (mpqname) * mpq->hdr->hashTableSize);
  MPQDecryptBlock (mpq->hashTable, hdr.hashTableSize * sizeof (MPQHash),
    MPQHashString ("(hash table)", MPQ_HASH_KEY));
  MPQDecryptBlock (mpq->blockTable, hdr.blockTableSize * sizeof (MPQBlock),
    MPQHashString ("(block table)", MPQ_HASH_KEY));

  mpq->writelist = !nolist;
  if (mpq->writelist)
  {
    MPQFILE listfile = MPQOpenFile ((MPQARCHIVE) mpq, "(listfile)", (readonly || !mpq->writelist) ? MPQFILE_READ : MPQFILE_MODIFY);
    if (listfile)
    {
      char line[256];
      while (MPQFileGets (listfile, sizeof line, line))
      {
        uint32 len = 0;
        while (line[len]) len++;
        while (len && (line[len - 1] == '\r' || line[len - 1] == '\n')) line[--len] = 0;
        uint32 pos = MPQFindFile ((MPQARCHIVE) mpq, line);
        uint32 first = pos;
        while (pos >= 0 && pos < mpq->hdr->hashTableSize)
        {
          strcpy (mpq->fileNames[pos], line);
          pos = MPQFindNextFile ((MPQARCHIVE) mpq, line, pos);
          if (pos == first)
            break;
        }
      }
      MPQCloseFile (listfile);
    }
  }

  MPQFindFile ((MPQARCHIVE) mpq, "(listfile)");
  MPQFindFile ((MPQARCHIVE) mpq, "(attributes)");
  MPQFindFile ((MPQARCHIVE) mpq, "(signature)");
  mpq_error = MPQ_OK;
  return (MPQARCHIVE) mpq;
}

uint32 MPQClose (MPQARCHIVE handle)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq)
    delete mpq;
  else
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  return mpq_error;
}

uint32 MPQListFiles (MPQARCHIVE handle, MPQLISTFILE listfile)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  MPQListFile* list = (MPQListFile*) listfile;
  if (mpq == NULL || mpq->file == 0 || list == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq->fileNames == NULL) return mpq_error = MPQ_OK;
  for (uint32 i = 0; i < list->size; i++)
  {
    uint32 pos = MPQFindFile ((MPQARCHIVE) mpq, list->list[i]);
    uint32 first = pos;
    bool ins = false;
    while (pos >= 0 && pos < mpq->hdr->hashTableSize)
    {
      if (mpq->fileNames[pos][0] == 0)
        ins = true;
      strcpy (mpq->fileNames[pos], list->list[i]);
      pos = MPQFindNextFile ((MPQARCHIVE) mpq, list->list[i], pos);
      if (pos == first)
        break;
    }
  }
  return mpq_error = MPQ_OK;
}

uint32 MPQGetCompression (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  mpq_error = MPQ_OK;
  return mpq->compressMethods;
}

uint32 MPQSetCompression (MPQARCHIVE handle, uint32 method)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  mpq->compressMethods = method;
  return mpq_error = MPQ_OK;
}

uint32 MPQSaveAs (MPQARCHIVE handle, char const* filename)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  uint32 file = file_open (filename, MPQFILE_REWRITE | MPQFILE_BINARY);
  if (file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
  file_seek (mpq->file, 0, MPQSEEK_SET);
  MPQFileTransfer (mpq->file, file, -1);
  file_close (mpq->file);
  file_close (file);
  mpq->file = file_open (filename, MPQFILE_MODIFY | MPQFILE_BINARY);
  if (mpq->isTemp)
    file_delete (mpq->path);
  strcpy (mpq->path, filename);
  mpq->isTemp = false;
  if (mpq->readonly)
    mpq->readonly = false;
  return mpq_error = MPQ_OK;
}

uint32 MPQMakeTemp (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq->isTemp)
    return mpq_error = MPQ_OK;
  char path[] = "temp000.mpq";
  uint32 file = 0;
  for (int i = 0; i < 1000; i++)
  {
    path[6] = (i % 10) + '0';
    path[5] = ((i / 10) % 10) + '0';
    path[4] = (i / 100) + '0';
    file = file_open (path, MPQFILE_READ | MPQFILE_BINARY);
    if (file == 0)
      break;
    file_close (file);
  }
  if (file != 0)
    return mpq_serror = mpq_error = MPQ_ERROR_FULL;
  if (MPQSaveAs (handle, path))
    return mpq_error;
  mpq->isTemp = true;
  return mpq_error = MPQ_OK;
}

uint32 MPQFindFile (MPQARCHIVE handle, char const* name)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  uint32 hash = MPQHashString (name, MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
  uint32 name1 = MPQHashString (name, MPQ_HASH_NAME1);
  uint32 name2 = MPQHashString (name, MPQ_HASH_NAME2);
  uint32 count = 0;
  for (uint32 cur = hash; mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY &&
    count < mpq->hdr->hashTableSize; cur = (cur + 1) % mpq->hdr->hashTableSize)
  {
    if (mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED &&
      mpq->hashTable[cur].name1 == name1 && mpq->hashTable[cur].name2 == name2)
    {
      if (mpq->fileNames)
        strcpy (mpq->fileNames[cur], name);
      return cur;
    }
    count++;
  }
  return mpq_error = MPQ_ERROR_NOFILE;
}

uint32 MPQFindFile (MPQARCHIVE handle, char const* name, uint16 locale)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  uint32 hash = MPQHashString (name, MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
  uint32 name1 = MPQHashString (name, MPQ_HASH_NAME1);
  uint32 name2 = MPQHashString (name, MPQ_HASH_NAME2);
  uint32 last = MPQ_ERROR_NOFILE;
  uint32 count = 0;
  for (uint32 cur = hash; mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY &&
    count < mpq->hdr->hashTableSize; cur = (cur + 1) % mpq->hdr->hashTableSize)
  {
    if (mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED &&
      mpq->hashTable[cur].name1 == name1 && mpq->hashTable[cur].name2 == name2)
    {
      if (mpq->hashTable[cur].locale == locale)
        return cur;
      last = cur;
    }
    count++;
  }
  if (MPQ_ERROR (last))
    mpq_error = last;
  return last;
}

uint32 MPQFindNextFile (MPQARCHIVE handle, char const* name, uint32 cur)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  uint32 name1 = MPQHashString (name, MPQ_HASH_NAME1);
  uint32 name2 = MPQHashString (name, MPQ_HASH_NAME2);
  uint32 count = 1;
  cur = (cur + 1) % mpq->hdr->hashTableSize;
  for (; mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY &&
    count < mpq->hdr->hashTableSize; cur = (cur + 1) % mpq->hdr->hashTableSize)
  {
    if (mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED &&
      mpq->hashTable[cur].name1 == name1 && mpq->hashTable[cur].name2 == name2)
      return cur;
    count++;
  }
  return mpq_error = MPQ_ERROR_NOFILE;
}

struct MPQFile
{
  uint32 fsys;
  MPQArchive* mpq;
  uint32 mode;
  uint32 index;
  uint32 flags;
  uint64 offset;
  uint32 size;
  uint32 numBlocks;
  uint32* blockPos;
  uint32 origKey;
  uint32 key;
  uint32 xflags;

  uint32 pos;

  uint32 curBlock;
  uint8* buf;
  uint32 curBlockSize;
  bool modified;

  MPQFile* next;
  MPQFile* prev;

  MPQFile (MPQArchive* ampq)
  {
    memset (this, 0, sizeof (MPQFile));
    if (ampq)
    {
      mpq = ampq;
      next = mpq->firstFile;
      if (next)
        next->prev = this;
      mpq->firstFile = this;
    }
    else
    {
      next = NULL;
      prev = NULL;
    }
  }
  ~MPQFile ()
  {
    delete[] blockPos;
    delete[] buf;
    if (next)
      next->prev = prev;
    if (prev)
      prev->next = next;
    else if (mpq)
      mpq->firstFile = next;
  }
};

static uint32 MPQDetectSeed1 (uint32* block, uint32 result, uint32 mbx)
{
  uint32 temp = (block[0] ^ result) - 0xEEEEEEEE;
  for(int i = 0; i < 256; i++)
  {
    uint32 key = temp - cryptTable[MPQ_HASH_ENCRYPT * 256 + i];
    if ((key & 0xFF) != i)
      continue;
    uint32 seed = 0xEEEEEEEE + cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
    uint32 res = block[0] ^ (key + seed);
    if (res != result)
      continue;

    uint32 save = key + 1;

    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += res + seed * 32 + 3;

    seed += cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
    res = block[1] ^ (key + seed);
    if (res <= mbx)
      return save;
  }
  return 0;
}
static uint32 MPQDetectSeed2 (uint32* block, int num, ...)
{
  va_list argList;
  if (num < 2)
    return 0;
  uint32 result[16];
  va_start (argList, num);
  for (int i = 0; i < num; i++)
    result[i] = va_arg (argList, uint32);
  va_end (argList);

  uint32 temp = (block[0] ^ result[0]) - 0xEEEEEEEE;
  for(int i = 0; i < 256; i++)
  {
    uint32 key = temp - cryptTable[MPQ_HASH_ENCRYPT * 256 + i];
    if ((key & 0xFF) != i)
      continue;
    uint32 seed = 0xEEEEEEEE + cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
    uint32 res = block[0] ^ (key + seed);
    if (res != result[0])
      continue;

    uint32 save = key;

    for (int j = 1; j < num; j++)
    {
      key = ((~key << 21) + 0x11111111) | (key >> 11);
      seed += res + seed * 32 + 3;

      seed += cryptTable[MPQ_HASH_ENCRYPT * 256 + (key & 0xFF)];
      res = block[j] ^ (key + seed);
      if (res != result[j])
        break;
      else if (j == num - 1)
        return save;
    }
  }
  return 0;
}

static uint32 MPQLoadBlock (MPQFile* file, uint32 index)
{
  if (index < 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (index >= file->numBlocks)
    return mpq_error = MPQ_EOF;
  if (file->curBlock != index || file->buf == NULL)
  {
    if (file->buf == NULL)
      file->buf = new uint8[file->mpq->blockSize];
    uint32 offset = index * file->mpq->blockSize;
    uint32 size = (file->size - offset > file->mpq->blockSize ? file->mpq->blockSize : file->size - offset);
    uint32 len = size;
    uint8* buf = file->buf;
    if (file->flags & MPQ_FILE_COMPRESSED)
    {
      offset = file->blockPos[index];
      len = file->blockPos[index + 1] - file->blockPos[index];
      if (len < size)
        buf = file->mpq->data;
    }
    file_seek (file->mpq->file, file->offset + int32 (offset), MPQSEEK_SET);
    if (file_read (buf, len, file->mpq->file) != len)
      return mpq_serror = mpq_error = MPQ_ERROR_READ;
    if (file->flags & MPQ_FILE_ENCRYPTED)
    {
      bool wz = (file->key == 0);
      if (file->key == 0) file->key = MPQDetectSeed2 ((uint32*) buf, 3, ID_WAVE, file->size - 8, 0x45564157);
      if (file->key == 0) file->key = MPQDetectSeed2 ((uint32*) buf, 2, 0x00905A4D, 0x00000003);
      if (file->key == 0) file->key = MPQDetectSeed2 ((uint32*) buf, 2, 0x34E1F3B9, 0xD5B0DBFA);
      if (file->key == 0)
        return mpq_serror = mpq_error = MPQ_ERROR_KEY;
      else if (wz)
      {
        file->origKey = file->key;
        if (file->flags & MPQ_FILE_FIXSEED)
          file->origKey = (file->key ^ file->size) - (uint32) (file->offset - file->mpq->offset);
      }
      MPQDecryptBlock (buf, size, file->key + index);
    }
    uint32 realSize = size;
    if (len < size)
    {
      if (file->flags & MPQ_FILE_COMPRESS_PKWARE)
        pkzip_decompress (buf, len, file->buf, &realSize);
      if (file->flags & MPQ_FILE_COMPRESS_MULTI)
        mpq_decompress (buf, len, file->buf, &realSize);
      if (mpq_error)
        return mpq_error;
    }

    if (realSize != size)
      return mpq_serror = mpq_error = MPQ_ERROR_READ;

    file->curBlock = index;
    file->curBlockSize = size;
  }
  return mpq_error = MPQ_OK;
}

static uint32 MPQLoadSingleUnit (MPQFile* file)
{
  uint32 csize = file->mpq->blockTable[file->mpq->hashTable[file->index].blockIndex].cSize;
  file->buf = new uint8[file->size];
  uint8* buf = file->buf;
  if (csize < file->size)
    buf = new uint8[csize];
  file_seek (file->mpq->file, file->offset, MPQSEEK_SET);
  if (file_read (buf, csize, file->mpq->file) != csize)
  {
    if (csize < file->size) delete[] buf;
    return mpq_serror = mpq_error = MPQ_ERROR_READ;
  }
  if (file->flags & MPQ_FILE_ENCRYPTED)
  {
    bool wz = (file->key == 0);
    if (file->key == 0) file->key = MPQDetectSeed2 ((uint32*) buf, 3, ID_WAVE, file->size - 8, 0x45564157);
    if (file->key == 0) file->key = MPQDetectSeed2 ((uint32*) buf, 2, 0x00905A4D, 0x00000003);
    if (file->key == 0)
    {
      if (csize < file->size) delete[] buf;
      return mpq_serror = mpq_error = MPQ_ERROR_KEY;
    }
    else if (wz)
    {
      file->origKey = file->key;
      if (file->flags & MPQ_FILE_FIXSEED)
        file->origKey = (file->key ^ file->size) - (uint32) (file->offset - file->mpq->offset);
    }
    MPQDecryptBlock (buf, csize, file->key);
  }
  file->curBlockSize = file->size;
  if (csize < file->size)
  {
    if (file->flags & MPQ_FILE_COMPRESS_PKWARE)
      pkzip_decompress (buf, csize, file->buf, &file->curBlockSize);
    if (file->flags & MPQ_FILE_COMPRESS_MULTI)
      mpq_decompress (buf, csize, file->buf, &file->curBlockSize);
    delete[] buf;
    if (mpq_error)
      return mpq_error;
  }
  file->curBlock = 0;
  if (file->curBlockSize != file->size)
    return mpq_serror = mpq_error = MPQ_ERROR_READ;
  return mpq_error = MPQ_OK;
}

uint32 MPQGetHashSize (MPQARCHIVE handle)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  return mpq->hdr->hashTableSize;
}

bool MPQFileExists (MPQARCHIVE handle, uint32 pos)
{
  mpq_error = MPQ_OK;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return false;
  }
  return pos >= 0 && pos < mpq->hdr->hashTableSize &&
    mpq->hashTable[pos].blockIndex >= 0 && mpq->hashTable[pos].blockIndex < mpq->hdr->blockTableSize;
}

bool MPQFileExists (MPQARCHIVE handle, char const* name)
{
  MPQFindFile (handle, name);
  bool res = (mpq_error == MPQ_OK);
  if (mpq_error == MPQ_ERROR_NOFILE)
    mpq_error = MPQ_OK;
  return res;
}
bool MPQFileExists (MPQARCHIVE handle, char const* name, uint16 locale)
{
  uint32 pos = MPQFindFile (handle, name, locale);
  bool res = false;
  if (mpq_error == MPQ_OK)
    res = (((MPQArchive*) handle)->hashTable[pos].locale == locale);
  if (mpq_error == MPQ_ERROR_NOFILE)
    mpq_error = MPQ_OK;
  return res;
}

static uint32 MPQSubOpenFile (MPQArchive* mpq, MPQFile* file, uint32 pos, uint32 key, uint32 options)
{
  if (mpq->file == 0 || pos < 0 || pos >= mpq->hdr->hashTableSize)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  uint32 block = mpq->hashTable[pos].blockIndex;
  if (block < 0 || block >= mpq->hdr->blockTableSize || (mpq->blockTable[block].flags & MPQ_FILE_EXISTS) == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_READ;
  mpq_error = MPQ_OK;
  int64 offset = mpq->offset + mpq->blockTable[block].filePos;
  if (mpq->extBlockTable)
    offset += ((int64) mpq->extBlockTable[block] << 32);
  file->mode = options;
  file->index = pos;
  file->flags = mpq->blockTable[block].flags;
  file->offset = offset;
  file->size = mpq->blockTable[block].fSize;
  if (key == 0 && mpq->fileNames && mpq->fileNames[pos][0])
    key = MPQHashString (StripPath (mpq->fileNames[pos]), MPQ_HASH_KEY);
  file->key = key;
  file->origKey = key;
  if (key && file->flags & MPQ_FILE_FIXSEED)
    file->key = (key + (uint32) (offset - mpq->offset)) ^ file->size;
  if (file->size == 0)
    return mpq_error = MPQ_OK;
  if (file->flags & MPQ_FILE_SINGLE_UNIT)
  {
    file->numBlocks = 1;
    if (MPQLoadSingleUnit (file))
      return mpq_error;
  }
  else
  {
    file->numBlocks = (file->size + mpq->blockSize - 1) / mpq->blockSize;
    if (file->flags & MPQ_FILE_COMPRESSED)
    {
      uint32 tableSize = file->numBlocks + 1;
      if (file->flags & MPQ_FILE_HAS_EXTRA)
        tableSize++;
      file->blockPos = new uint32[tableSize];
      file_seek (mpq->file, offset, MPQSEEK_SET);
      uint32 byteSize = tableSize * sizeof (uint32);
      if (file_read (file->blockPos, byteSize, mpq->file) != byteSize)
        return mpq_serror = mpq_error = MPQ_ERROR_READ;
      //if (file->blockPos[0] < byteSize || file->blockPos[0] > byteSize + 32)
      //  file->flags |= MPQ_FILE_ENCRYPTED;
      if (file->flags & MPQ_FILE_ENCRYPTED)
      {
        bool wz = (file->key == 0);
        if (file->key == 0) file->key = MPQDetectSeed1 (file->blockPos, byteSize, mpq->blockSize);
        if (file->key == 0)
          return mpq_serror = mpq_error = MPQ_ERROR_KEY;
        else if (wz)
        {
          file->origKey = file->key;
          if (file->flags & MPQ_FILE_FIXSEED)
            file->origKey = (file->key ^ file->size) - (uint32) (file->offset - mpq->offset);
        }
        MPQDecryptBlock (file->blockPos, byteSize, file->key - 1);
        //if (file->blockPos[0] < byteSize || file->blockPos[0] > byteSize + 32)
        //{
        //  file_seek (mpq->file, offset, MPQSEEK_SET);
        //  if (file_read (file->blockPos, byteSize, mpq->file) != byteSize)
        //    return mpq_error = MPQ_ERROR_READ;
        //  file->key = MPQDetectSeed1 (file->blockPos, byteSize);
        //  MPQDecryptBlock (file->blockPos, byteSize, file->key - 1);
        //  if (file->blockPos[0] != byteSize)
        //    return mpq_error = MPQ_ERROR_KEY;
        //}
      }
    }
  }
  return mpq_error = MPQ_OK;
}

static void MPQResizeBlock (MPQArchive* mpq, uint32 size)
{
  if (size <= mpq->blockTableSpace)
    return;

  uint32 xsize = mpq->xsize + (size - mpq->blockTableSpace) * sizeof (MPQBlock);
  if (mpq->extBlockTable)
    xsize += (size - mpq->blockTableSpace) * sizeof (uint16);
  uint8* data = new uint8[xsize];

  MPQHeader* hdr = (MPQHeader*) (data + mpq->blockSize * 2);
  MPQHash* hashTable = (MPQHash*) (hdr + 1);
  MPQHeader2* hdr2 = NULL;
  if (mpq->hdr->formatVersion)
  {
    hdr2 = (MPQHeader2*) (hdr + 1);
    hashTable = (MPQHash*) (hdr2 + 1);
  }
  mpqname* fileNames = (mpqname*) (hashTable + mpq->hdr->hashTableSize);
  MPQBlock* blockTable = (MPQBlock*) (fileNames + (mpq->writelist ? mpq->hdr->hashTableSize : 0));
  if (!mpq->writelist) fileNames = NULL;
  uint16* extBlockTable = NULL;
  if (mpq->hdr->formatVersion)
    extBlockTable = (uint16*) (blockTable + size);

  memcpy (hdr, mpq->hdr, (uint32) mpq->blockTable - (uint32) mpq->hdr);
  memcpy (blockTable, mpq->blockTable, sizeof (MPQBlock) * mpq->hdr->blockTableSize);
  if (extBlockTable && mpq->extBlockTable)
    memcpy (extBlockTable, mpq->extBlockTable, sizeof (uint16) * mpq->hdr->blockTableSize);

  delete[] mpq->data;
  mpq->data = data;
  mpq->hdr = hdr;
  mpq->hdr2 = hdr2;
  mpq->hashTable = hashTable;
  mpq->fileNames = fileNames;
  mpq->blockTable = blockTable;
  mpq->extBlockTable = extBlockTable;
  mpq->blockTableSpace = size;
  mpq->xsize = xsize;
}

static uint32 MPQFlushHeader (MPQArchive* mpq)
{
  file_seek (mpq->file, mpq->offset, MPQSEEK_SET);
  uint32 size = sizeof (MPQHeader);
  if (mpq->hdr->formatVersion)
    size += sizeof (MPQHeader2);
  if (file_write (mpq->hdr, size, mpq->file) != size)
    mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  else
    mpq_error = MPQ_OK;
  return mpq_error;
}
static uint32 MPQFlushHashTable (MPQArchive* mpq)
{
  int64 pos = mpq->offset + mpq->hdr->hashTablePos;
  if (mpq->hdr2)
    pos += (int64) mpq->hdr2->hashTablePosHigh << 32;
  MPQHash* buf = new MPQHash[mpq->hdr->hashTableSize];
  uint32 size = mpq->hdr->hashTableSize * sizeof (MPQHash);
  memcpy (buf, mpq->hashTable, size);
  MPQEncryptBlock (buf, size, MPQHashString ("(hash table)", MPQ_HASH_KEY));
  file_seek (mpq->file, pos, MPQSEEK_SET);
  if (file_write (buf, size, mpq->file) != size)
    mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  else
    mpq_error = MPQ_OK;
  delete[] buf;
  return mpq_error;
}
static uint32 MPQFlushBlockTable (MPQArchive* mpq)
{
  int64 pos = mpq->offset + mpq->hdr->blockTablePos;
  if (mpq->hdr2)
    pos += (int64) mpq->hdr2->blockTablePosHigh << 32;
  MPQBlock* buf = new MPQBlock[mpq->hdr->blockTableSize];
  uint32 size = mpq->hdr->blockTableSize * sizeof (MPQBlock);
  memcpy (buf, mpq->blockTable, size);
  MPQEncryptBlock (buf, size, MPQHashString ("(block table)", MPQ_HASH_KEY));
  file_seek (mpq->file, pos, MPQSEEK_SET);
  if (file_write (buf, size, mpq->file) != size)
    mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  else
    mpq_error = MPQ_OK;
  delete[] buf;

  if (mpq_error == MPQ_OK && mpq->extBlockTable && mpq->hdr2)
  {
    size = mpq->hdr->blockTableSize * sizeof (uint16);
    file_seek (mpq->file, mpq->hdr2->extBlockTablePos, MPQSEEK_SET);
    if (file_write (mpq->extBlockTable, size, mpq->file) != size)
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    else
      mpq_error = MPQ_OK;
  }
  return mpq_error;
}

static MPQFILE MPQOpenWrite (MPQArchive* mpq, uint32 pos, char const* name, uint32 options)
{
  MPQFile* file = new MPQFile (mpq);
  file->mpq = mpq;
  file->mode = (options & 3);
  file->index = pos;
  file->numBlocks = 1;
  uint32 block = mpq->hashTable[pos].blockIndex;
  bool newBlock = false;
  if (block >= 0 && block < mpq->hdr->blockTableSize)
  {
    file->flags = mpq->blockTable[block].flags;
    if ((options & 3) == MPQFILE_REWRITE || (file->flags & MPQ_FILE_EXISTS) == 0)
    {
      file->size = 0;
      file->curBlockSize = MPQ_DATA_GROW;
      file->buf = new uint8[file->curBlockSize];
    }
    else
    {
      MPQFile* old = new MPQFile (mpq);
      MPQSubOpenFile (mpq, old, pos, MPQHashString (StripPath (name), MPQ_HASH_KEY), MPQFILE_READ);
      if (mpq_error)
      {
        delete old;
        old = NULL;
      }
      if (old)
      {
        file->size = mpq->blockTable[block].fSize;
        file->curBlockSize = (file->size + MPQ_DATA_GROW - 1) / MPQ_DATA_GROW * MPQ_DATA_GROW;
        file->buf = new uint8[file->curBlockSize];
        file->offset = old->offset;
        if (old->flags & MPQ_FILE_SINGLE_UNIT)
          memcpy (file->buf, old->buf, file->size);
        else
        {
          uint32 rd = 0;
          for (uint32 i = 0; i < old->numBlocks; i++)
          {
            MPQLoadBlock (old, i);
            memcpy (file->buf + rd, old->buf, old->curBlockSize);
            rd += old->curBlockSize;
          }
        }
        MPQCloseFile ((MPQFILE) old);
      }
      else
      {
        file->size = 0;
        file->curBlockSize = MPQ_DATA_GROW;
        file->buf = new uint8[file->curBlockSize];
      }
    }
    file->flags |= MPQ_FILE_EXISTS;
    file->flags &= ~(MPQ_FILE_SINGLE_UNIT | MPQ_FILE_HAS_EXTRA |
      MPQ_FILE_DUMMY_FILE | MPQ_FILE_COMPRESS_PKWARE);
    file->flags = mpq->blockTable[block].flags;
  }
  else
  {
    newBlock = true;
    if (mpq->hdr->blockTableSize >= mpq->blockTableSpace)
      MPQResizeBlock (mpq, mpq->blockTableSpace + MPQ_BLOCK_GROW);
    block = mpq->hdr->blockTableSize++;
    mpq->hashTable[pos].blockIndex = block;
    if (mpq->fileNames)
      strcpy (mpq->fileNames[pos], name);
    mpq->blockTable[block].cSize = 0;
    mpq->blockTable[block].fSize = 0;
    mpq->blockTable[block].filePos = 0;
    if (mpq->extBlockTable)
      mpq->extBlockTable[block] = 0;

    int64 hashTablePos = mpq->hdr->hashTablePos;
    if (mpq->hdr2) hashTablePos += (int64) mpq->hdr2->hashTablePosHigh << 32;
    int64 blockTablePos = mpq->hdr->blockTablePos;
    if (mpq->hdr2) blockTablePos += (int64) mpq->hdr2->blockTablePosHigh << 32;
    if (blockTablePos < hashTablePos)
      hashTablePos += sizeof (MPQBlock);
    if (mpq->hdr2 && blockTablePos < mpq->hdr2->extBlockTablePos)
      mpq->hdr2->extBlockTablePos += sizeof (MPQBlock);
    if (mpq->hdr2)
    {
      if (mpq->hdr2->extBlockTablePos < hashTablePos)
        hashTablePos += sizeof (uint16);
      if (mpq->hdr2->extBlockTablePos < blockTablePos)
        blockTablePos += sizeof (uint16);
    }
    mpq->hdr->blockTablePos = (uint32) blockTablePos;
    if (mpq->hdr2) mpq->hdr2->blockTablePosHigh = (uint16) (blockTablePos >> 32);
    mpq->hdr->hashTablePos = (uint32) hashTablePos;
    if (mpq->hdr2) mpq->hdr2->hashTablePosHigh = (uint16) (hashTablePos >> 32);

    file->size = 0;
    file->curBlockSize = MPQ_DATA_GROW;
    file->buf = new uint8[file->curBlockSize];
    file->flags = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS_MULTI | MPQ_FILE_ENCRYPTED;
  }
  if (options & MPQFILE_ENCRYPT)
    file->flags |= MPQ_FILE_ENCRYPTED;
  if (options & MPQFILE_NOENCRYPT)
    file->flags &= ~MPQ_FILE_ENCRYPTED;
  if (options & MPQFILE_COMPRESS && (file->flags & MPQ_FILE_COMPRESSED) == 0)
    file->flags |= MPQ_FILE_COMPRESS_MULTI;
  if (options & MPQFILE_NOCOMPRESS)
    file->flags &= ~MPQ_FILE_COMPRESSED;
  if (options & MPQFILE_FIXSEED)
    file->flags |= MPQ_FILE_FIXSEED;
  if (options & MPQFILE_FIXSEED)
    file->flags &= ~MPQ_FILE_FIXSEED;
  file->key = MPQHashString (StripPath (name), MPQ_HASH_KEY);
  file->origKey = file->key;
  if (file->flags & MPQ_FILE_FIXSEED)
    file->key = (file->key + uint32 (file->offset - mpq->offset)) ^ file->size;
  if (newBlock)
  {
    mpq->blockTable[block].flags = file->flags;
    MPQFlushHeader (mpq);
    if (mpq_error == MPQ_OK) MPQFlushHashTable (mpq);
    if (mpq_error == MPQ_OK) MPQFlushBlockTable (mpq);
    if (mpq_error)
    {
      delete file;
      return 0;
    }
  }

  return (MPQFILE) file;
}

MPQFILE MPQOpenFile (MPQARCHIVE handle, char const* name, uint32 options)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  if ((options & 3) == 0)
  {
    uint32 pos = MPQFindFile (handle, name);
    if (mpq_error)
      return 0;
    MPQFile* file = new MPQFile (mpq);
    MPQSubOpenFile (mpq, file, pos, MPQHashString (StripPath (name), MPQ_HASH_KEY), options);
    if (mpq_error)
    {
      delete file;
      return 0;
    }
    return (MPQFILE) file;
  }
  if (mpq->readonly)
  {
    mpq_serror = mpq_error = MPQ_ERROR_READONLY;
    return 0;
  }
  uint32 hash = MPQHashString (name, MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
  uint32 name1 = MPQHashString (name, MPQ_HASH_NAME1);
  uint32 name2 = MPQHashString (name, MPQ_HASH_NAME2);
  uint32 count = 0;
  uint32 best = -1;
  for (; count < mpq->hdr->hashTableSize; count++, hash = (hash + 1) % mpq->hdr->hashTableSize)
  {
    if (mpq->hashTable[hash].blockIndex >= 0 &&
      mpq->hashTable[hash].blockIndex < mpq->hdr->blockTableSize &&
      mpq->hashTable[hash].name1 == name1 &&
      mpq->hashTable[hash].name2 == name2)
      break;
    if (mpq->hashTable[hash].blockIndex == MPQ_INDEX_DELETED && best >= mpq->hdr->hashTableSize)
      best = hash;
    if (mpq->hashTable[hash].blockIndex == MPQ_INDEX_EMPTY)
      break;
  }
  if (count >= mpq->hdr->hashTableSize)
  {
    if (best < mpq->hdr->hashTableSize)
      hash = best;
    else
    {
      MPQResizeHash (handle, mpq->hdr->hashTableSize + 1);
      if (!mpq_error && MPQHasUnknowns (handle))
        MPQFillHashTable (handle);
      if (!mpq_error)
        return MPQOpenFile (handle, name, options);
      return NULL;
    }
  }
  if (mpq->hashTable[hash].blockIndex < 0 ||
    mpq->hashTable[hash].blockIndex >= mpq->hdr->blockTableSize)
  {
    mpq->hashTable[hash].name1 = name1;
    mpq->hashTable[hash].name2 = name2;
    mpq->hashTable[hash].locale = MPQ_LOCALE_NEUTRAL;
    mpq->hashTable[hash].platform = 0;
  }
  return MPQOpenWrite (mpq, hash, name, options);
}
MPQFILE MPQOpenFile (MPQARCHIVE handle, char const* name, uint16 locale, uint32 options)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  if ((options & 3) == 0)
  {
    uint32 pos = MPQFindFile (handle, name, locale);
    if (mpq_error)
      return 0;
    MPQFile* file = new MPQFile (mpq);
    MPQSubOpenFile (mpq, file, pos, MPQHashString (StripPath (name), MPQ_HASH_KEY), options);
    if (mpq_error)
    {
      delete file;
      return 0;
    }
    return (MPQFILE) file;
  }
  if (mpq->readonly)
  {
    mpq_serror = mpq_error = MPQ_ERROR_READONLY;
    return 0;
  }
  uint32 hash = MPQHashString (name, MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
  uint32 name1 = MPQHashString (name, MPQ_HASH_NAME1);
  uint32 name2 = MPQHashString (name, MPQ_HASH_NAME2);
  uint32 count = 0;
  uint32 best = -1;
  for (; count < mpq->hdr->hashTableSize; count++, hash = (hash + 1) % mpq->hdr->hashTableSize)
  {
    if (mpq->hashTable[hash].blockIndex >= 0 &&
      mpq->hashTable[hash].blockIndex < mpq->hdr->blockTableSize &&
      mpq->hashTable[hash].name1 == name1 &&
      mpq->hashTable[hash].name2 == name2 &&
      mpq->hashTable[hash].locale == locale)
      break;
    if (mpq->hashTable[hash].blockIndex == MPQ_INDEX_DELETED && best >= mpq->hdr->hashTableSize)
      best = hash;
    if (mpq->hashTable[hash].blockIndex == MPQ_INDEX_EMPTY)
      break;
  }
  if (count >= mpq->hdr->hashTableSize)
  {
    if (best < mpq->hdr->hashTableSize)
      hash = best;
    else
    {
      MPQResizeHash (handle, mpq->hdr->hashTableSize + 1);
      if (!mpq_error && MPQHasUnknowns (handle))
        MPQFillHashTable (handle);
      if (!mpq_error)
        return MPQOpenFile (handle, name, locale, options);
      return NULL;
    }
  }
  if (mpq->hashTable[hash].blockIndex < 0 ||
    mpq->hashTable[hash].blockIndex >= mpq->hdr->blockTableSize)
  {
    mpq->hashTable[hash].name1 = name1;
    mpq->hashTable[hash].name2 = name2;
    mpq->hashTable[hash].locale = locale;
    mpq->hashTable[hash].platform = 0;
  }
  return MPQOpenWrite (mpq, hash, name, options);
}
MPQFILE MPQOpenFile (MPQARCHIVE handle, uint32 pos, uint32 options)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0 || (options & 3))
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  MPQFile* file = new MPQFile (mpq);
  MPQSubOpenFile (mpq, file, pos, 0, options);
  if (mpq_error)
  {
    delete file;
    return 0;
  }
  return (MPQFILE) file;
}
bool MPQTestFile (MPQARCHIVE handle, uint32 pos)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0 || pos < 0 || pos >= mpq->hdr->hashTableSize)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return false;
  }
  mpq_error = MPQ_OK;
  uint32 block = mpq->hashTable[pos].blockIndex;
  if (block < 0 || block >= mpq->hdr->blockTableSize || (mpq->blockTable[block].flags & MPQ_FILE_EXISTS) == 0)
    return false;
  uint32 size = mpq->blockTable[block].fSize;
  if (size == 0)
    return true;
  int64 offset = mpq->offset + mpq->blockTable[block].filePos;
  if (mpq->extBlockTable)
    offset += ((int64) mpq->extBlockTable[block] << 32);
  uint32 buf[4];
  if ((mpq->blockTable[block].flags & MPQ_FILE_ENCRYPTED) == 0)
    return true;
  if (mpq->fileNames && mpq->fileNames[pos][0])
    return true;
  uint32 key = 0;
  file_seek (mpq->file, offset, MPQSEEK_SET);
  if ((mpq->blockTable[block].flags & MPQ_FILE_SINGLE_UNIT) ||
     !(mpq->blockTable[block].flags & MPQ_FILE_COMPRESSED))
  {
    int rsize = (mpq->blockTable[block].cSize > 12 ? 12 : mpq->blockTable[block].cSize);
    if (file_read (buf, rsize, mpq->file) != rsize)
      return false;
    if (key == 0 && rsize >= 12) key = MPQDetectSeed2 (buf, 3, ID_WAVE, size - 8, 0x45564157);
    if (key == 0 && rsize >= 8) key = MPQDetectSeed2 (buf, 2, 0x00905A4D, 0x00000003);
    if (key == 0 && rsize >= 8) key = MPQDetectSeed2 (buf, 2, 0x34E1F3B9, 0xD5B0DBFA);
    if (key == 0)
      return false;
  }
  else
  {
    uint32 byteSize = 4 * ((size + mpq->blockSize - 1) / mpq->blockSize + 1);
    if (mpq->blockTable[block].flags & MPQ_FILE_HAS_EXTRA)
      byteSize += 4;
    if (file_read (buf, 8, mpq->file) != 8)
      return false;
    if (key == 0) key = MPQDetectSeed1 (buf, byteSize, mpq->blockSize);
    if (key == 0) return false;
  }
  return true;
}
uint32 MPQPeekFile (MPQARCHIVE handle, uint32 pos, uint8* dest)
{
  static uint8 tmpbuf[256];
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0 || pos < 0 || pos >= mpq->hdr->hashTableSize)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  mpq_error = MPQ_OK;
  uint32 block = mpq->hashTable[pos].blockIndex;
  if (block < 0 || block >= mpq->hdr->blockTableSize || (mpq->blockTable[block].flags & MPQ_FILE_EXISTS) == 0)
    return 0;
  uint32 size = mpq->blockTable[block].fSize;
  if (size == 0)
    return 0;
  int64 offset = mpq->offset + mpq->blockTable[block].filePos;
  if (mpq->extBlockTable)
    offset += ((int64) mpq->extBlockTable[block] << 32);
  uint32 buf[4];
  uint32 key = 0;
  if (mpq->fileNames && mpq->fileNames[pos][0])
    key = MPQHashString (mpq->fileNames[pos], MPQ_HASH_KEY);
  if (mpq->blockTable[block].flags & MPQ_FILE_FIXSEED)
    key = (key + (uint32) (offset - mpq->offset)) ^ size;
  file_seek (mpq->file, offset, MPQSEEK_SET);
  if ((mpq->blockTable[block].flags & MPQ_FILE_SINGLE_UNIT) ||
     !(mpq->blockTable[block].flags & MPQ_FILE_COMPRESSED))
  {
    int rsize = (mpq->blockTable[block].cSize > 256 ? 256 : mpq->blockTable[block].cSize);
    if (file_read (tmpbuf, rsize, mpq->file) != rsize)
      return 0;
    if (mpq->blockTable[block].flags & MPQ_FILE_ENCRYPTED)
    {
      if (key == 0 && rsize >= 12) key = MPQDetectSeed2 ((uint32*) tmpbuf, 3, ID_WAVE, size - 8, 0x45564157);
      if (key == 0 && rsize >= 8) key = MPQDetectSeed2 ((uint32*) tmpbuf, 2, 0x00905A4D, 0x00000003);
      if (key == 0 && rsize >= 8) key = MPQDetectSeed2 ((uint32*) tmpbuf, 2, 0x34E1F3B9, 0xD5B0DBFA);
      if (key == 0)
        return 0;
      MPQDecryptBlock (tmpbuf, rsize, key);
    }
    if ((mpq->blockTable[block].flags & MPQ_FILE_COMPRESSED) &&
        mpq->blockTable[block].cSize < size)
    {
      uint32 count = 17;
      if (mpq_part_decompress (tmpbuf, rsize, dest, &count) != MPQ_OK)
        return 0;
      return count;
    }
    else
    {
      int count = 17;
      if (rsize < count) count = rsize;
      memcpy (dest, tmpbuf, count);
      return count;
    }
  }
  else
  {
    uint32 byteSize = 4 * ((size + mpq->blockSize - 1) / mpq->blockSize + 1);
    if (mpq->blockTable[block].flags & MPQ_FILE_HAS_EXTRA)
      byteSize += 4;
    if (file_read (buf, 8, mpq->file) != 8)
      return 0;
    if (mpq->blockTable[block].flags & MPQ_FILE_ENCRYPTED)
    {
      if (key == 0) key = MPQDetectSeed1 (buf, byteSize, mpq->blockSize);
      if (key == 0) return 0;
      MPQDecryptBlock (buf, 8, key - 1);
    }
    file_seek (mpq->file, offset + buf[0], MPQSEEK_SET);
    uint32 csize = buf[1] - buf[0];
    int rsize = (csize > 256 ? 256 : csize);
    if (file_read (tmpbuf, rsize, mpq->file) != rsize)
      return 0;
    if (mpq->blockTable[block].flags & MPQ_FILE_ENCRYPTED)
      MPQDecryptBlock (tmpbuf, rsize, key);
    if ((mpq->blockTable[block].flags & MPQ_FILE_COMPRESSED) &&
        csize < size)
    {
      uint32 count = 17;
      if (mpq_part_decompress (tmpbuf, rsize, dest, &count) != MPQ_OK)
        return 0;
      return count;
    }
    else
    {
      int count = 17;
      if (rsize < count) count = rsize;
      memcpy (dest, tmpbuf, count);
      return count;
    }
  }
}
MPQFILE MPQOpenFSys (char const* name, uint32 options)
{
  uint32 fsys = file_open (name, options);
  if (fsys == 0)
  {
    mpq_error = MPQ_ERROR_NOFILE;
    return 0;
  }
  MPQFile* file = new MPQFile (NULL);
  file->fsys = fsys;
  file->mode = options;
  return (MPQFILE) file;
}
uint32 MPQReopenFile (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->fsys) return mpq_error = MPQ_OK;
  if ((file->mode & 3) == 0)
  {
    delete[] file->blockPos;
    delete[] file->buf;
    return MPQSubOpenFile (file->mpq, file, file->pos, file->origKey, file->mode);
  }
  file->offset = file->mpq->offset + file->mpq->blockTable[file->mpq->hashTable[file->index].blockIndex].filePos;
  if (file->mpq->extBlockTable)
    file->offset += (int64) file->mpq->extBlockTable[file->mpq->hashTable[file->index].blockIndex] << 32;
  file->key = file->origKey;
  if (file->flags & MPQ_FILE_FIXSEED)
    file->key = (file->key + (uint32) (file->offset - file->mpq->offset)) ^ file->size;
  return mpq_error = MPQ_OK;
}
uint32 MPQCloseFile (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file)
  {
    mpq_error = MPQ_OK;
    if (file->modified)
      MPQFlushFile (handle);
    if (file->fsys)
      file_close (file->fsys);
    delete file;
  }
  else
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  return mpq_error;
}

uint32 MPQFileAttribute (MPQFILE handle, uint32 attr)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  mpq_error = MPQ_OK;
  if (file->fsys)
  {
    if (attr == MPQ_FILE_SIZE)
    {
      int64 pos = file_tell (file->fsys);
      file_seek (file->fsys, 0, MPQSEEK_END);
      uint32 size = (uint32) file_tell (file->fsys);
      file_seek (file->fsys, pos, MPQSEEK_SET);
      return size;
    }
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  }
  uint32 block = file->mpq->hashTable[file->index].blockIndex;
  if (attr == MPQ_FILE_FLAGS)
    return file->mpq->blockTable[block].flags;
  if (attr == MPQ_FILE_COMPSIZE)
    return file->mpq->blockTable[block].cSize;
  if (attr == MPQ_FILE_SIZE)
    return file->mpq->blockTable[block].fSize;
  if (attr == MPQ_FILE_LOCALE)
    return file->mpq->hashTable[file->index].locale;
  if (attr == MPQ_FILE_PLATFORM)
    return file->mpq->hashTable[file->index].platform;
  if (attr == MPQ_FILE_OFFSET)
    return file->mpq->blockTable[block].filePos;
  return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
}

uint32 MPQFileSeek (MPQFILE handle, uint32 pos, int mode)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  mpq_error = MPQ_OK;
  if (file->fsys)
  {
    file_seek (file->fsys, pos, mode);
    return (uint32) file_tell (file->fsys);
  }
  if (mode == MPQSEEK_SET)
    file->pos = pos;
  else if (mode == MPQSEEK_CUR)
    file->pos += pos;
  else if (mode == MPQSEEK_END)
    file->pos = file->size + pos;
  else
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->pos < 0)
    file->pos = 0;
  if (file->pos > file->size)
    file->pos = file->size;
  return file->pos;
}

uint32 MPQFileTell (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  mpq_error = MPQ_OK;
  if (file->fsys) return (uint32) file_tell (file->fsys);
  return file->pos;
}

uint32 MPQFileRead (MPQFILE handle, uint32 count, void* vptr)
{
  MPQFile* file = (MPQFile*) handle;
  mpq_error = MPQ_OK;
  if (file == NULL || vptr == NULL)
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq_error)
    return 0;
  if (file->fsys)
  {
    uint32 res = file_read (vptr, count, file->fsys);
    if (res != count)
      mpq_error = MPQ_EOF;
    return res;
  }
  uint32 res = 0;
  uint8* ptr = (uint8*) vptr;
  if (file->flags & MPQ_FILE_SINGLE_UNIT || file->mode)
  {
    res = count;
    if (file->pos + res > file->curBlockSize)
    {
      res = file->curBlockSize - file->pos;
      mpq_error = MPQ_EOF;
    }
    if (res)
    {
      memcpy (ptr, file->buf + file->pos, res);
      file->pos += res;
    }
  }
  else
  {
    res = 0;
    while (file->pos < file->size && res < count)
    {
      uint32 get = count - res;
      uint32 block = file->pos / file->mpq->blockSize;
      uint32 boffset = block * file->mpq->blockSize;
      uint32 bsize = file->mpq->blockSize;
      if (boffset + bsize > file->size)
        bsize = file->size - boffset;
      if (file->pos + get > boffset + bsize)
        get = boffset + bsize - file->pos;
      if (get)
      {
        if (MPQLoadBlock (file, block))
          break;
        memcpy (ptr + res, file->buf + (file->pos - boffset), get);
        res += get;
        file->pos += get;
      }
    }
    if (file->pos >= file->size && res < count)
      mpq_error = MPQ_EOF;
  }
  return res;
}
uint32 MPQFileGetc (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  mpq_error = MPQ_OK;
  if (file->fsys)
  {
    uint32 x = 0;
    if (file_read (&x, 1, file->fsys) != 1)
      mpq_error = MPQ_EOF;
    return x;
  }
  if (file->flags & MPQ_FILE_SINGLE_UNIT || file->mode)
  {
    if (file->pos >= file->size)
      return mpq_error = MPQ_EOF;
    return file->buf[file->pos++];
  }
  else
  {
    if (file->pos >= file->size)
      return mpq_error = MPQ_EOF;
    uint32 block = file->pos / file->mpq->blockSize;
    uint32 boffset = block * file->mpq->blockSize;
    if (MPQLoadBlock (file, block))
      return mpq_error;
    return file->buf[file->pos++ - boffset];
  }
}
uint32 MPQFilePutc (MPQFILE handle, uint8 c)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->mode == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  if (mpq_error)
    return 0;
  if (file->fsys)
  {
    file_write (&c, 1, file->fsys);
    return mpq_error = MPQ_OK;
  }
  uint32 newSize = file->pos;
  if (file->xflags & MPQ_FLAG_PUSH)
    newSize = file->size;
  if (file->pos >= file->curBlockSize)
  {
    uint32 nsize = file->curBlockSize + MPQ_DATA_GROW;
    uint8* nbuf = new uint8[nsize];
    memcpy (nbuf, file->buf, file->size);
    delete[] file->buf;
    file->buf = nbuf;
    file->curBlockSize = nsize;
  }
  file->modified = true;
  if (file->xflags & MPQ_FLAG_PUSH)
  {
    if (file->pos < file->size)
      memmove (file->buf + file->pos + 1, file->buf + file->pos, file->size - file->pos);
    file->buf[file->pos++] = c;
    file->size++;
  }
  else
  {
    file->buf[file->pos++] = c;
    if (file->pos > file->size)
      file->size = file->pos;
  }
  return mpq_error = MPQ_OK;
}
char* MPQFileGets (MPQFILE handle, uint32 count, char* ptr)
{
  uint32 len = 0;
  mpq_error = MPQ_OK;
  ptr[0] = 0;
  while (len < count - 1)
  {
    uint32 chr = MPQFileGetc (handle);
    if (MPQ_ERROR (mpq_error))
    {
      if (mpq_error == MPQ_EOF) break;
      else return NULL;
    }
    ptr[len++] = (char) chr;
    if (chr == '\n')
      break;
    if (chr == '\r')
    {
      int pos = MPQFileTell (handle);
      uint32 cc = MPQFileGetc (handle);
      if (cc != '\n')
        MPQFileSeek (handle, pos, MPQSEEK_SET);
      break;
    }
  }
  if (count && len == 0)
    return NULL;
  ptr[len] = 0;
  return ptr;
}

static uint32 MPQGetFreeHashEntry (MPQArchive* mpq, char const* name)
{
  uint32 hash = MPQHashString (name, MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
  uint32 cur = hash;
  uint32 count = 0;
  while (mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY &&
    mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED && count < mpq->hdr->hashTableSize)
  {
    cur = (cur + 1) % mpq->hdr->hashTableSize;
    count++;
  }
  if (mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY && mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED)
    return mpq_serror = mpq_error = MPQ_ERROR_FULL;
  mpq_error = MPQ_OK;
  mpq->hashTable[cur].name1 = MPQHashString (name, MPQ_HASH_NAME1);
  mpq->hashTable[cur].name2 = MPQHashString (name, MPQ_HASH_NAME2);
  return cur;
}
static uint32 MPQGetFreeHashEntry (MPQArchive* mpq, uint32 hash)
{
  uint32 cur = hash;
  uint32 count = 0;
  while (mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY &&
    mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED && count < mpq->hdr->hashTableSize)
  {
    cur = (cur + 1) % mpq->hdr->hashTableSize;
    count++;
  }
  if (mpq->hashTable[cur].blockIndex != MPQ_INDEX_EMPTY && mpq->hashTable[cur].blockIndex != MPQ_INDEX_DELETED)
    return mpq_serror = mpq_error = MPQ_ERROR_FULL;
  mpq_error = MPQ_OK;
  return cur;
}

static uint32 MPQEncryptFileEx (MPQARCHIVE handle, char const* name, uint32 options)
{
  uint32 pos = MPQFindFile (handle, name);
  if (mpq_error)
    return mpq_error;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  uint32 blockIndex = mpq->hashTable[pos].blockIndex;
  if (blockIndex < 0 || blockIndex >= mpq->hdr->blockTableSize ||
    (mpq->blockTable[blockIndex].flags & MPQ_FILE_EXISTS) == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_READ;
    return 0;
  }
  MPQBlock* block = &mpq->blockTable[blockIndex];
  uint32 oldKey = MPQHashString (StripPath (name), MPQ_HASH_KEY);
  uint32 newKey = oldKey;
  int64 offset = mpq->offset + block->filePos;
  if (mpq->extBlockTable)
    offset += (int64) mpq->extBlockTable[blockIndex] << 32;
  if (block->flags & MPQ_FILE_FIXSEED)
    oldKey = (oldKey + (uint32) (offset - mpq->offset)) ^ block->fSize;
  if (options & MPQ_FILE_FIXSEED)
    newKey = (newKey + (uint32) (offset - mpq->offset)) ^ block->fSize;
  mpq_error = MPQ_OK;
  if ((block->flags & MPQ_FILE_SINGLE_UNIT) == 0 && (block->flags & MPQ_FILE_COMPRESSED))
  {
    uint32 numBlocks = (block->fSize + mpq->blockSize - 1) / mpq->blockSize;
    uint32 tableSize = numBlocks + 1;
    if (block->flags & MPQ_FILE_HAS_EXTRA)
      tableSize++;
    uint32* blockPos = new uint32[tableSize];
    file_seek (mpq->file, offset, MPQSEEK_SET);
    if (file_read (blockPos, tableSize * sizeof (uint32), mpq->file) != tableSize * sizeof (uint32))
      mpq_serror = mpq_error = MPQ_ERROR_READ;
    if (mpq_error == MPQ_OK)
    {
      if (block->flags & MPQ_FILE_ENCRYPTED)
        MPQDecryptBlock (blockPos, tableSize * sizeof (uint32), oldKey - 1);
      for (uint32 i = 0; i < numBlocks && mpq_error == MPQ_OK; i++)
      {
        uint32 len = blockPos[i + 1] - blockPos[i];
        file_seek (mpq->file, offset + blockPos[i], MPQSEEK_SET);
        if (file_read (mpq->data, len, mpq->file) != len)
          mpq_serror = mpq_error = MPQ_ERROR_READ;
        else
        {
          if (block->flags & MPQ_FILE_ENCRYPTED)
            MPQDecryptBlock (mpq->data, len, oldKey + i);
          if (options & MPQ_FILE_ENCRYPTED)
            MPQEncryptBlock (mpq->data, len, newKey + i);
          file_seek (mpq->file, offset + blockPos[i], MPQSEEK_SET);
          if (file_write (mpq->data, len, mpq->file) != len)
            mpq_serror = mpq_error = MPQ_ERROR_WRITE;
        }
      }
    }
    if (mpq_error == MPQ_OK)
    {
      if (options & MPQ_FILE_ENCRYPTED)
        MPQEncryptBlock (blockPos, tableSize * sizeof (uint32), newKey - 1);
      file_seek (mpq->file, offset, MPQSEEK_SET);
      if (file_write (blockPos, tableSize * sizeof (uint32), mpq->file) != tableSize * sizeof (uint32))
        mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    }
    delete[] blockPos;
  }
  else
  {
    uint8* buf = new uint8[block->cSize];
    file_seek (mpq->file, offset, MPQSEEK_SET);
    if (file_read (buf, block->cSize, mpq->file) != block->cSize)
      mpq_serror = mpq_error = MPQ_ERROR_READ;
    if (mpq_error == MPQ_OK)
    {
      if (block->flags & MPQ_FILE_ENCRYPTED)
        MPQDecryptBlock (buf, block->cSize, oldKey);
      if (options & MPQ_FILE_ENCRYPTED)
        MPQDecryptBlock (buf, block->cSize, newKey);
      file_seek (mpq->file, offset, MPQSEEK_SET);
      if (file_write (buf, block->cSize, mpq->file) != block->cSize)
        mpq_serror = mpq_error = MPQ_ERROR_READ;
    }
    delete[] buf;
  }
  block->flags = (block->flags & (~(MPQ_FILE_ENCRYPTED | MPQ_FILE_FIXSEED))) |
    (options & (MPQ_FILE_ENCRYPTED | MPQ_FILE_FIXSEED));
  return mpq_error;
}

uint32 MPQRenameFile (MPQARCHIVE handle, char const* source, char const* dest)
{
  uint32 pos = MPQFindFile (handle, source);
  if (mpq_error)
    return 0;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  uint32 block = mpq->hashTable[pos].blockIndex;
  uint16 locale = mpq->hashTable[pos].locale;
  uint16 platform = mpq->hashTable[pos].platform;
  if (block < 0 || block >= mpq->hdr->blockTableSize || (mpq->blockTable[block].flags & MPQ_FILE_EXISTS) == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_READ;
    return 0;
  }
  uint32 flags = mpq->blockTable[block].flags & (MPQ_FILE_ENCRYPTED | MPQ_FILE_FIXSEED);
  if (flags && MPQEncryptFile (handle, source, 0) != MPQ_OK)
    return mpq_error;
  mpq->hashTable[pos].blockIndex = MPQ_INDEX_DELETED;
  mpq->hashTable[pos].locale = 0xFFFF;
  mpq->hashTable[pos].platform = 0xFFFF;
  mpq->hashTable[pos].name1 = 0xFFFFFFFF;
  mpq->hashTable[pos].name2 = 0xFFFFFFFF;
  if (mpq->fileNames)
    mpq->fileNames[pos][0] = 0;
  pos = MPQGetFreeHashEntry (mpq, dest);
  mpq->hashTable[pos].blockIndex = block;
  mpq->hashTable[pos].locale = locale;
  mpq->hashTable[pos].platform = platform;
  if (mpq->fileNames)
    strcpy (mpq->fileNames[pos], dest);
  if (flags && MPQEncryptFileEx (handle, dest, flags) != MPQ_OK)
    return mpq_error;
  return MPQFlushHashTable (mpq);
}

uint32 MPQEncryptFile (MPQARCHIVE handle, char const* name, uint32 options)
{
  if (MPQEncryptFileEx (handle, name, options) != MPQ_OK)
    return mpq_error;
  return MPQFlushBlockTable ((MPQArchive*) handle);
}

uint32 MPQDeleteFile (MPQARCHIVE handle, char const* name)
{
  uint32 pos = MPQFindFile (handle, name);
  if (mpq_error)
    return 0;
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  uint32 block = mpq->hashTable[pos].blockIndex;
  if (block < 0 || block >= mpq->hdr->blockTableSize || (mpq->blockTable[block].flags & MPQ_FILE_EXISTS) == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_READ;
    return 0;
  }
  memset (&mpq->blockTable[block], 0, sizeof (MPQBlock));
  mpq->hashTable[pos].blockIndex = MPQ_INDEX_DELETED;
  mpq->hashTable[pos].locale = 0xFFFF;
  mpq->hashTable[pos].platform = 0xFFFF;
  mpq->hashTable[pos].name1 = 0xFFFFFFFF;
  mpq->hashTable[pos].name2 = 0xFFFFFFFF;
  if (mpq->fileNames)
    mpq->fileNames[pos][0] = 0;
  if (MPQFlushHashTable (mpq) != MPQ_OK)
    return mpq_error;
  if (MPQFlushBlockTable (mpq) != MPQ_OK)
    return mpq_error;
  if (mpq->writelist)
    return MPQFlushListfile (handle);
  return mpq_error;
}

uint32 MPQResizeHash (MPQARCHIVE handle, uint32 bsize)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  if (bsize <= mpq->hdr->hashTableSize)
    return mpq_error = MPQ_OK;
  uint32 size = 1;
  while (size < bsize)
    size <<= 1;

  uint32 xsize = mpq->xsize + (size - mpq->hdr->hashTableSize) * (sizeof (MPQHash) + sizeof (mpqname));
  uint8* data = new uint8[xsize];
  uint32 hashOffset = mpq->blockSize * 2;
  uint32 hashSize = mpq->hdr->hashTableSize * (sizeof (MPQHash) + sizeof (mpqname));
  uint32 newHashSize = size * (sizeof (MPQHash) + sizeof (mpqname));
  memcpy (data + hashOffset + newHashSize, mpq->data + hashOffset + hashSize, mpq->xsize - hashOffset - hashSize);
  uint8* oldData = mpq->data;
  MPQHash* oldHashTable = mpq->hashTable;
  mpqname* oldFileNames = mpq->fileNames;
  uint32 oldSize = mpq->hdr->hashTableSize;

  mpq->xsize = xsize;
  mpq->data = data;
  mpq->hdr = (MPQHeader*) (mpq->data + mpq->blockSize * 2);
  uint8* end = (uint8*) (mpq->hdr + 1);
  if (mpq->hdr->formatVersion)
  {
    mpq->hdr2 = (MPQHeader2*) end;
    end = (uint8*) (mpq->hdr2 + 1);
  }
  else
    mpq->hdr2 = NULL;
  mpq->hashTable = (MPQHash*) end;
  mpq->fileNames = (mpqname*) (mpq->hashTable + size);
  mpq->blockTable = (MPQBlock*) (mpq->fileNames + (mpq->writelist ? size : 0));
  if (!mpq->writelist) mpq->fileNames = NULL;
  if (mpq->hdr->formatVersion)
    mpq->extBlockTable = (uint16*) (mpq->blockTable + mpq->blockTableSpace);
  else  
    mpq->extBlockTable = NULL;

  memset (mpq->hashTable, 0xFF, size * sizeof (MPQHash));
  if (mpq->fileNames)
    memset (mpq->fileNames, 0x00, size * sizeof (mpqname));
  mpq->hdr->hashTableSize = size;
  memcpy (mpq->hashTable, oldHashTable, oldSize * sizeof (MPQHash));
  if (mpq->fileNames)
    memcpy (mpq->fileNames, oldFileNames, oldSize * sizeof (mpqname));

  if (oldFileNames)
  {
    for (uint32 i = 0; i < oldSize; i++)
    {
      if (oldFileNames[i][0])
      {
        uint32 pos = MPQHashString (oldFileNames[i], MPQ_HASH_OFFSET) % mpq->hdr->hashTableSize;
        if (pos != i)
        {
          memset (&mpq->hashTable[i], 0xFF, sizeof (MPQHash));
          memset (&mpq->fileNames[i], 0x00, sizeof (mpqname));
          pos = MPQGetFreeHashEntry (mpq, pos);
          memcpy (&mpq->hashTable[pos], &oldHashTable[i], sizeof (MPQHash));
          memcpy (&mpq->fileNames[pos], &oldFileNames[i], sizeof (mpqname));
        }
      }
    }
  }

  delete[] oldData;

  int64 hashTablePos = mpq->hdr->hashTablePos;
  if (mpq->hdr2) hashTablePos += (int64) mpq->hdr2->hashTablePosHigh << 32;
  int64 blockTablePos = mpq->hdr->blockTablePos;
  if (mpq->hdr2) blockTablePos += (int64) mpq->hdr2->blockTablePosHigh << 32;
  uint32 delta = (size - oldSize) * sizeof (MPQHash);

  if (hashTablePos < blockTablePos)
  {
    blockTablePos += delta;
    mpq->hdr->blockTablePos = (uint32) blockTablePos;
    if (mpq->hdr2) mpq->hdr2->blockTablePosHigh = (uint16) (blockTablePos >> 32);
  }
  if (mpq->hdr2 && hashTablePos < mpq->hdr2->extBlockTablePos)
    mpq->hdr2->extBlockTablePos += delta;

  if (MPQFlushHashTable (mpq)) return mpq_error;
  if (MPQFlushBlockTable (mpq)) return mpq_error;
  return mpq_error = MPQ_OK;
}

bool MPQHasUnknowns (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return false;
  }
  for (uint32 i = 0; i < mpq->hdr->hashTableSize; i++)
  {
    if (mpq->hashTable[i].blockIndex != MPQ_INDEX_EMPTY &&
      mpq->hashTable[i].blockIndex != MPQ_INDEX_DELETED &&
      (mpq->fileNames == NULL || mpq->fileNames[i][0] == 0))
      return true;
  }
  return false;
}
uint32 MPQFillHashTable (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  for (uint32 i = 0; i < mpq->hdr->hashTableSize; i++)
    if (mpq->hashTable[i].blockIndex == MPQ_INDEX_EMPTY)
      mpq->hashTable[i].blockIndex = MPQ_INDEX_DELETED;
  return mpq_error = MPQ_OK;
}

uint32 MPQFileWrite (MPQFILE handle, uint32 count, void const* vptr)
{
  MPQFile* file = (MPQFile*) handle;
  mpq_error = MPQ_OK;
  if (file == NULL || vptr == NULL)
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  else if (file->mode == 0)
    mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  if (mpq_error)
    return 0;
  if (file->fsys)
    return file_write (vptr, count, file->fsys);
  uint8 const* ptr = (uint8*) vptr;
  uint32 newSize = file->pos + count;
  if (file->xflags & MPQ_FLAG_PUSH)
    newSize = file->size + count;
  if (newSize > file->curBlockSize)
  {
    uint32 nsize = (newSize + MPQ_DATA_GROW - 1) / MPQ_DATA_GROW * MPQ_DATA_GROW;
    uint8* nbuf = new uint8[nsize];
    memcpy (nbuf, file->buf, file->size);
    delete[] file->buf;
    file->buf = nbuf;
    file->curBlockSize = nsize;
  }
  if (count)
  {
    file->modified = true;
    if (file->xflags & MPQ_FLAG_PUSH)
    {
      if (file->pos < file->size)
        memmove (file->buf + count, file->buf, file->size - file->pos);
      memcpy (file->buf + file->pos, ptr, count);
      file->pos += count;
      file->size += count;
    }
    else
    {
      memcpy (file->buf + file->pos, ptr, count);
      file->pos += count;
      if (file->pos > file->size)
        file->size = file->pos;
    }
  }
  return count;
}

uint32 MPQFlushFile (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->mode == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;

  int64 hashTablePos = file->mpq->hdr->hashTablePos;
  if (file->mpq->hdr2) hashTablePos += (int64) file->mpq->hdr2->hashTablePosHigh << 32;
  int64 blockTablePos = file->mpq->hdr->blockTablePos;
  if (file->mpq->hdr2) blockTablePos += (int64) file->mpq->hdr2->blockTablePosHigh << 32;

  int64 offset = hashTablePos;
  if (blockTablePos < offset)
    offset = blockTablePos;
  if (file->mpq->hdr2 && file->mpq->hdr2->extBlockTablePos < offset)
    offset = file->mpq->hdr2->extBlockTablePos;

  if (file->flags & MPQ_FILE_FIXSEED)
    file->key = (file->origKey + (uint32) offset) ^ file->size;
  MPQBlock* block = &file->mpq->blockTable[file->mpq->hashTable[file->index].blockIndex];
  uint16* extBlock = NULL;
  if (file->mpq->extBlockTable)
    extBlock = &file->mpq->extBlockTable[file->mpq->hashTable[file->index].blockIndex];

  block->cSize = 0;
  file->flags &= ~(MPQ_FILE_SINGLE_UNIT | MPQ_FILE_HAS_EXTRA | MPQ_FILE_DUMMY_FILE | MPQ_FILE_COMPRESSED);
  file->flags |= MPQ_FILE_COMPRESS_MULTI | MPQ_FILE_EXISTS;
  block->flags = file->flags;
  block->fSize = file->size;
  block->filePos = (uint32) offset;
  if (extBlock) *extBlock = (uint16) (offset >> 32);
  offset += file->mpq->offset;
  file->offset = offset;
  uint32 numBlocks = (file->size + file->mpq->blockSize - 1) / file->mpq->blockSize;

  mpq_error = MPQ_OK;

  if (file->flags & MPQ_FILE_COMPRESSED)
  {
    uint32* blockPos = new uint32[numBlocks + 1];
    uint32 tableSize = (numBlocks + 1) * sizeof (uint32);
    memset (blockPos, 0, tableSize);
    blockPos[0] = tableSize;
    uint8* temp = file->mpq->data + file->mpq->blockSize;
    file_seek (file->mpq->file, offset, MPQSEEK_SET);
    if (file_write (blockPos, tableSize, file->mpq->file) != tableSize)
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    for (uint32 i = 0; i < numBlocks && mpq_error == MPQ_OK; i++)
    {
      uint32 start = i * file->mpq->blockSize;
      uint32 size = file->mpq->blockSize;
      if (start + size > file->size)
        size = file->size - start;
      uint32 out_size = size;
      if (file->flags & MPQ_FILE_COMPRESS_PKWARE)
        pkzip_compress (file->buf + start, size, file->mpq->data, &out_size);
      if (file->flags & MPQ_FILE_COMPRESS_MULTI)
        mpq_compress (file->buf + start, size, file->mpq->data, &out_size, temp, file->mpq->compressMethods);
      if (mpq_error == MPQ_OK)
      {
        if (file->flags & MPQ_FILE_ENCRYPTED)
          MPQEncryptBlock (file->mpq->data, out_size, file->key + i);
        if (file_write (file->mpq->data, out_size, file->mpq->file) != out_size)
          mpq_serror = mpq_error = MPQ_ERROR_WRITE;
        blockPos[i + 1] = blockPos[i] + out_size;
      }
    }
    if (mpq_error == MPQ_OK)
    {
      block->cSize = blockPos[numBlocks];
      if (file->flags & MPQ_FILE_ENCRYPTED)
        MPQEncryptBlock (blockPos, tableSize, file->key - 1);
      file_seek (file->mpq->file, offset, MPQSEEK_SET);
      if (file_write (blockPos, tableSize, file->mpq->file) != tableSize)
        mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    }
    delete[] blockPos;
  }
  else
  {
    file_seek (file->mpq->file, offset, MPQSEEK_SET);
    for (uint32 i = 0; i < numBlocks && mpq_error == MPQ_OK; i++)
    {
      uint32 start = i * file->mpq->blockSize;
      uint32 size = file->mpq->blockSize;
      if (start + size > file->size)
        size = file->size - start;
      memcpy (file->mpq->data, file->buf + start, size);
      if (file->flags & MPQ_FILE_ENCRYPTED)
        MPQEncryptBlock (file->mpq->data, size, file->key + 1);
      if (file_write (file->mpq->data, size, file->mpq->file) != size)
        mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    }
    block->cSize = block->fSize;
  }

  if (mpq_error == MPQ_OK)
  {
    hashTablePos += block->cSize;
    blockTablePos += block->cSize;
    file->mpq->hdr->blockTablePos = (uint32) blockTablePos;
    file->mpq->hdr->hashTablePos = (uint32) hashTablePos;
    if (file->mpq->hdr2)
    {
      file->mpq->hdr2->blockTablePosHigh = (uint16) (blockTablePos >> 32);
      file->mpq->hdr2->hashTablePosHigh = (uint16) (hashTablePos >> 32);
      file->mpq->hdr2->extBlockTablePos += block->cSize;
    }
  }
  uint32 serr = mpq_error;
  mpq_error = MPQ_OK;
  if (mpq_error == MPQ_OK) MPQFlushHeader (file->mpq);
  if (mpq_error == MPQ_OK) MPQFlushHashTable (file->mpq);
  if (mpq_error == MPQ_OK) MPQFlushBlockTable (file->mpq);

  if (serr)
    mpq_serror = mpq_error = serr;
  if (mpq_error == MPQ_OK)
    file->modified = false;
  return mpq_error;
}

uint32 MPQFlushListfile (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  MPQFILE listfile = MPQOpenFile (handle, "(listfile)", MPQFILE_REWRITE);
  if (listfile)
  {
    for (uint32 i = 0; i < mpq->hdr->hashTableSize; i++)
    {
      if (mpq->fileNames[i][0])
      {
        MPQFilePuts (listfile, mpq->fileNames[i]);
        MPQFilePuts (listfile, "\r\n");
      }
    }
    MPQCloseFile (listfile);
  }
  return mpq_error = MPQ_OK;
}
uint32 MPQFlush (MPQARCHIVE handle)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (mpq->readonly)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  MPQFile* cur = mpq->firstFile;
  while (cur)
  {
    if (cur->modified && MPQFlushFile ((MPQFILE) cur))
      return mpq_error;
    cur = cur->next;
  }
  if (!mpq->readonly && mpq->writelist)
    MPQFlushListfile (handle);
  char fname[64];
  sprintf (fname, "$$temp%03d$$", uint32 (handle) % 1000);
  uint32 temp = file_open (fname, MPQFILE_REWRITE | MPQFILE_BINARY);
  if (temp == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
  mpq_error = MPQ_OK;
  if (mpq_error == MPQ_OK && mpq->offset)
  {
    file_seek (mpq->file, 0, MPQSEEK_SET);
    if (MPQFileTransfer (mpq->file, temp, (uint32) mpq->offset) != (uint32) mpq->offset)
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  }
  if (mpq_error == MPQ_OK)
  {
    if (file_write (mpq->hdr, sizeof (MPQHeader), temp) != sizeof (MPQHeader))
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    else if (mpq->hdr2 && file_write (mpq->hdr2, sizeof (MPQHeader2), temp) != sizeof (MPQHeader2))
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  }
  uint32 tmpSize = 0;
  uint8* tmp = NULL;
#define STRETCH(s) if(s>tmpSize){delete[] tmp;tmp=new uint8[s];tmpSize=s;}
  int64 pos = sizeof (MPQHeader);
  if (mpq->hdr2)
    pos += sizeof (MPQHeader2);
  for (uint32 i = 0; i < mpq->hdr->blockTableSize && mpq_error == MPQ_OK; i++)
  {
    MPQBlock* block = &mpq->blockTable[i];
    uint16* extBlock = (mpq->extBlockTable ? &mpq->extBlockTable[i] : NULL);
    int64 offset = block->filePos;
    if (extBlock)
      offset += (int64) *extBlock << 32;
    if (block->flags & MPQ_FILE_EXISTS && block->cSize != 0)
    {
      if ((block->flags & MPQ_FILE_SINGLE_UNIT) == 0 && (block->flags & MPQ_FILE_COMPRESSED))
      {
        uint32 numBlocks = (block->fSize + mpq->blockSize - 1) / mpq->blockSize;
        uint32 tableSize = (numBlocks + 1) * sizeof (uint32);
        if (block->flags & MPQ_FILE_HAS_EXTRA)
          tableSize += sizeof (uint32);
        file_seek (mpq->file, mpq->offset + offset, MPQSEEK_SET);
        STRETCH (tableSize * 2);
        if (file_read (tmp, tableSize, mpq->file) != tableSize)
          mpq_serror = mpq_error = MPQ_ERROR_WRITE;
        uint32* blockPos = (uint32*) tmp;
        uint32 key = 0;
        uint32 newKey = 0;
        if (mpq_error == MPQ_OK)
        {
          if (block->flags & MPQ_FILE_ENCRYPTED)
          {
            if (mpq->fileNames)
            {
              for (uint32 j = 0; j < mpq->hdr->hashTableSize; j++)
              {
                if (mpq->fileNames[j][0] && mpq->hashTable[j].blockIndex == i)
                {
                  key = MPQHashString (StripPath (mpq->fileNames[j]), MPQ_HASH_KEY);
                  break;
                }
              }
            }
            if (key)
            {
              newKey = key;
              if (block->flags & MPQ_FILE_FIXSEED)
              {
                newKey = (key + (uint32) pos) ^ block->fSize;
                key = (key + (uint32) offset) ^ block->fSize;
              }
            }
            else
            {
              key = MPQDetectSeed1 (blockPos, tableSize, mpq->blockSize);
              if (key == 0)
                mpq_serror = mpq_error = MPQ_ERROR_KEY;
              else
              {
                newKey = key;
                if (block->flags & MPQ_FILE_FIXSEED)
                  newKey = ((key ^ block->fSize) - (uint32) offset + (uint32) pos) ^ block->fSize;
              }
            }
          }
        }
        if (mpq_error == MPQ_OK)
        {
          if (block->flags & MPQ_FILE_ENCRYPTED)
            MPQDecryptBlock (blockPos, tableSize, key - 1);

          uint32* exBlockPos = (uint32*) (tmp + tableSize);
          memcpy (exBlockPos, blockPos, tableSize);
          uint32 delta = tableSize - blockPos[0];
          for (uint32 j = 0; j <= numBlocks; j++)
            exBlockPos[j] += delta;
          if (block->flags & MPQ_FILE_ENCRYPTED)
            MPQEncryptBlock (exBlockPos, tableSize, newKey - 1);
          if (file_write (exBlockPos, tableSize, temp) != tableSize)
            mpq_serror = mpq_error = MPQ_ERROR_WRITE;
        }
        if (mpq_error == MPQ_OK)
        {
          file_seek (mpq->file, mpq->offset + offset + int32 (blockPos[0]), MPQSEEK_SET);
          if (block->flags & MPQ_FILE_FIXSEED)
          {
            for (uint32 j = 0; j < numBlocks; j++)
            {
              uint32 size = blockPos[j + 1] - blockPos[j];
              if (file_read (mpq->data, size, mpq->file) != size)
              {
                mpq_serror = mpq_error = MPQ_ERROR_READ;
                break;
              }
              MPQDecryptBlock (mpq->data, size, key + j);
              MPQEncryptBlock (mpq->data, size, newKey + j);
              if (file_write (mpq->data, size, temp) != size)
              {
                mpq_serror = mpq_error = MPQ_ERROR_WRITE;
                break;
              }
            }
          }
          else
          {
            uint32 size = blockPos[numBlocks] - blockPos[0];
            if (MPQFileTransfer (mpq->file, temp, size) != size)
              mpq_serror = mpq_error = MPQ_ERROR_READ;
          }
          block->filePos = (uint32) pos;
          if (extBlock)
            *extBlock = (uint16) (pos >> 32);
          pos += tableSize + blockPos[numBlocks] - blockPos[0];
        }
      }
      else
      {
        file_seek (mpq->file, mpq->offset + offset, MPQSEEK_SET);
        if ((block->flags & MPQ_FILE_ENCRYPTED) && (block->flags & MPQ_FILE_FIXSEED))
        {
          STRETCH (block->cSize);
          if (file_read (tmp, block->cSize, mpq->file) != block->cSize)
            mpq_serror = mpq_error = MPQ_ERROR_READ;
          uint32 key = 0;
          if (mpq_error == MPQ_OK)
          {
            if (mpq->fileNames)
            {
              for (uint32 j = 0; j < mpq->hdr->hashTableSize; j++)
              {
                if (mpq->fileNames[j][0] && mpq->hashTable[j].blockIndex == i)
                {
                  key = MPQHashString (StripPath (mpq->fileNames[j]), MPQ_HASH_KEY);
                  break;
                }
              }
            }
            if (key)
              key = (key + (uint32) offset) ^ block->fSize;
            else
            {
              if (key == 0) key = MPQDetectSeed2 ((uint32*) tmp, 3, ID_WAVE, block->fSize - 8, 0x45564157);
              if (key == 0) key = MPQDetectSeed2 ((uint32*) tmp, 2, 0x00905A4D, 0x00000003);
              if (key == 0) key = MPQDetectSeed2 ((uint32*) tmp, 2, 0x34E1F3B9, 0xD5B0DBFA);
              if (key == 0)
                mpq_serror = mpq_error = MPQ_ERROR_KEY;
            }
          }
          if (mpq_error == MPQ_OK)
          {
            MPQDecryptBlock (tmp, block->cSize, key);
            key = (key ^ block->fSize) - (uint32) offset;
            key = (key + (uint32) pos) ^ block->fSize;
            MPQEncryptBlock (tmp, block->cSize, key);
            if (file_write (tmp, block->cSize, temp) != block->cSize)
              mpq_serror = mpq_error = MPQ_ERROR_WRITE;
          }
        }
        else
        {
          if (MPQFileTransfer (mpq->file, temp, block->cSize) != block->cSize)
            mpq_serror = mpq_error = MPQ_ERROR_WRITE;
        }
        block->filePos = (uint32) pos;
        if (extBlock)
          *extBlock = (uint16) (pos >> 32);
        pos += block->cSize;
      }
    }
  }
  if (mpq_error == MPQ_OK)
  {
    for (uint32 i = 0; i < mpq->hdr->hashTableSize; i++)
    {
      if (mpq->hashTable[i].blockIndex != MPQ_INDEX_DELETED &&
        mpq->hashTable[i].blockIndex != MPQ_INDEX_EMPTY &&
        mpq->hashTable[i].blockIndex >= mpq->hdr->blockTableSize)
      {
        mpq->hashTable[i].blockIndex = MPQ_INDEX_DELETED;
        mpq->hashTable[i].name1 = 0xFFFFFFFF;
        mpq->hashTable[i].name2 = 0xFFFFFFFF;
        mpq->hashTable[i].locale = 0xFFFF;
        mpq->hashTable[i].platform = 0xFFFF;
      }
    }
    uint32 size = mpq->hdr->hashTableSize * sizeof (MPQHash);
    STRETCH (size);
    memcpy (tmp, mpq->hashTable, size);
    MPQEncryptBlock (tmp, size, MPQHashString ("(hash table)", MPQ_HASH_KEY));
    mpq->hdr->hashTablePos = (uint32) pos;
    if (mpq->hdr2)
      mpq->hdr2->hashTablePosHigh = (uint16) (pos >> 32);
    pos += size;
    if (file_write (tmp, size, temp) != size)
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  }
  if (mpq_error == MPQ_OK)
  {
    uint32 size = mpq->hdr->blockTableSize * sizeof (MPQBlock);
    STRETCH (size);
    memcpy (tmp, mpq->blockTable, size);
    MPQEncryptBlock (tmp, size, MPQHashString ("(block table)", MPQ_HASH_KEY));
    mpq->hdr->blockTablePos = (uint32) pos;
    if (mpq->hdr2)
      mpq->hdr2->blockTablePosHigh = (uint16) (pos >> 32);
    pos += size;
    if (file_write (tmp, size, temp) != size)
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  }
  if (mpq_error == MPQ_OK)
  {
    if (mpq->extBlockTable)
    {
      uint32 size = mpq->hdr->blockTableSize * sizeof (uint16);
      if (mpq->hdr2)
        mpq->hdr2->extBlockTablePos = pos;
      pos += size;
      if (file_write (mpq->extBlockTable, size, temp) != size)
        mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    }
  }
  mpq->hdr->headerSize = sizeof (MPQHeader);
  if (mpq->hdr2)
    mpq->hdr->headerSize += sizeof (MPQHeader2);
  mpq->hdr->archiveSize = (uint32) pos;
  if (pos != mpq->hdr->archiveSize)
    mpq->hdr->archiveSize = 0;
  if (mpq_error == MPQ_OK)
  {
    file_seek (temp, mpq->offset, MPQSEEK_SET);
    if (file_write (mpq->hdr, sizeof (MPQHeader), temp) != sizeof (MPQHeader))
      mpq_serror = mpq_error = MPQ_ERROR_WRITE;
  }
  if (mpq_error == MPQ_OK)
  {
    if (mpq->hdr2)
    {
      if (file_write (mpq->hdr2, sizeof (MPQHeader2), temp) != sizeof (MPQHeader2))
        mpq_serror = mpq_error = MPQ_ERROR_WRITE;
    }
  }
  delete[] tmp;
  file_close (temp);
  if (mpq_error)
  {
    file_delete (fname);
    return mpq_error;
  }
  if ((temp = file_open (fname, MPQFILE_READ | MPQFILE_BINARY)) == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
  else
  {
    file_close (mpq->file);
    mpq->file = file_open (mpq->path, MPQFILE_REWRITE | MPQFILE_BINARY);
    if (mpq->file == 0)
    {
      file_close (temp);
      file_delete (fname);
      mpq->file = file_open (mpq->path, MPQFILE_MODIFY | MPQFILE_BINARY);
      return mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
    }
    MPQFileTransfer (temp, mpq->file, -1);
    file_close (temp);
    file_delete (fname);
    file_close (mpq->file);
    mpq->file = file_open (mpq->path, MPQFILE_MODIFY | MPQFILE_BINARY);
    if (mpq->file == 0)
      return mpq_serror = mpq_error = MPQ_ERROR_ACCESS;
    MPQFile* cur = mpq->firstFile;
    while (cur)
    {
      if (MPQReopenFile ((MPQFILE) cur))
        return mpq_error;
      cur = cur->next;
    }
    return mpq_error = MPQ_OK;
  }
}

MPQArchive::~MPQArchive ()
{
  MPQFile* cf = firstFile;
  while (cf)
  {
    MPQFile* f = cf->next;
    MPQCloseFile ((MPQFILE) cf);
    cf = f;
  }
  if (file)
    file_close (file);
  delete[] data;
  if (next)
    next->prev = prev;
  if (prev)
    prev->next = next;
  else
    firstArchive = next;
  if (isTemp)
    file_delete (path);
}

struct MPQLoader
{
  MPQARCHIVE* stack;
  bool* own;
  int count;
  int mx;

  MPQLoader* next;
  MPQLoader* prev;

  char* prefix;

  MPQLoader (char const* px)
  {
    stack = NULL;
    own = NULL;
    mx = 0;
    count = 0;
    next = firstLoader;
    if (next)
      next->prev = this;
    prev = NULL;
    firstLoader = this;
    if (px)
      prefix = strdup (px);
    else
      prefix = NULL;
  }
  ~MPQLoader ()
  {
    for (int i = 0; i < count; i++)
      if (own[i])
        MPQClose (stack[i]);
    delete[] stack;
    delete[] own;
    delete[] prefix;
    if (next)
      next->prev = prev;
    if (prev)
      prev->next = next;
    else
      firstLoader = next;
  }
};

MPQLOADER MPQCreateLoader (char const* prefix)
{
  if (!init)
  {
    mpq_serror = mpq_error = MPQ_ERROR_INIT;
    return 0;
  }
  return (MPQLOADER) new MPQLoader (prefix);
}

uint32 MPQReleaseLoader (MPQLOADER handle)
{
  if (handle == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  delete (MPQLoader*) handle;
  return mpq_error = MPQ_OK;
}

uint32 MPQAddArchive (MPQLOADER loader, MPQARCHIVE archive)
{
  if (loader == 0 || archive == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  MPQLoader* ld = (MPQLoader*) loader;
  if (ld->count >= ld->mx)
  {
    ld->mx *= 2;
    MPQARCHIVE* temp = new MPQARCHIVE[ld->mx];
    memset (temp, 0, ld->mx * sizeof (MPQARCHIVE));
    if (ld->count)
      memcpy (temp, ld->stack, ld->count * sizeof (MPQARCHIVE));
    delete[] ld->stack;
    ld->stack = temp;
    bool* btemp = new bool[ld->mx];
    memset (btemp, 0, ld->mx * sizeof (bool));
    if (ld->count)
      memcpy (btemp, ld->own, ld->count * sizeof (bool));
    delete[] ld->own;
    ld->own = btemp;
  }
  ld->own[ld->count] = false;
  ld->stack[ld->count++] = archive;
  return mpq_error = MPQ_OK;
}

uint32 MPQRemoveArchive (MPQLOADER loader, MPQARCHIVE archive)
{
  if (loader == 0 || archive == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  MPQLoader* ld = (MPQLoader*) loader;
  for (int i = 0; i < ld->count; i++)
  {
    if (ld->stack[i] == archive)
    {
      if (i < ld->count - 1)
      {
        memmove (ld->stack, ld->stack + 1, (ld->count - i - 1) * sizeof (MPQARCHIVE));
        memmove (ld->own, ld->own + 1, (ld->count - i - 1) * sizeof (bool));
      }
      ld->stack[--ld->count] = 0;
      ld->own[ld->count] = false;
      break;
    }
  }
  return mpq_error = MPQ_OK;
}

MPQFILE MPQLoadFile (MPQLOADER handle, char const* name)
{
  if (handle == 0)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return NULL;
  }
  MPQLoader* ld = (MPQLoader*) handle;
  mpq_error = MPQ_ERROR_NOFILE;
  char pxname[256];
  if (ld->prefix)
    sprintf (pxname, "%s\\%s", ld->prefix, name);
  for (int i = ld->count - 1; i >= 0; i--)
  {
    MPQFILE file = NULL;
    if (ld->prefix)
      file = MPQOpenFile (ld->stack[i], pxname, MPQFILE_READ);
    if (file == NULL)
      file = MPQOpenFile (ld->stack[i], name, MPQFILE_READ);
    if (file)
      return file;
  }
  return 0;
}

uint32 MPQLoadArchive (MPQLOADER handle, char const* path)
{
  if (handle == 0 || path == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  MPQARCHIVE mpq = MPQOpen (path, MPQFILE_READ | MPQ_NO_LISTFILE);
  if (mpq == 0)
    return mpq_error = MPQ_ERROR_NOFILE;
  MPQLoader* ld = (MPQLoader*) handle;
  if (ld->count >= ld->mx)
  {
    if (ld->mx == 0) ld->mx = 16;
    else ld->mx *= 2;
    MPQARCHIVE* temp = new MPQARCHIVE[ld->mx];
    memset (temp, 0, ld->mx * sizeof (MPQARCHIVE));
    if (ld->count)
      memcpy (temp, ld->stack, ld->count * sizeof (MPQARCHIVE));
    delete[] ld->stack;
    ld->stack = temp;
    bool* btemp = new bool[ld->mx];
    memset (btemp, 0, ld->mx * sizeof (bool));
    if (ld->count)
      memcpy (btemp, ld->own, ld->count * sizeof (bool));
    delete[] ld->own;
    ld->own = btemp;
  }
  ld->own[ld->count] = true;
  ld->stack[ld->count++] = mpq;
  return mpq_error = MPQ_OK;
}

uint32 MPQCleanup ()
{
  if (!init)
    return mpq_serror = mpq_error = MPQ_ERROR_INIT;
  MPQC_CLEANUP ();
  MPQLoader* cl = firstLoader;
  while (cl)
  {
    MPQLoader* l = cl->next;
    delete cl;
    cl = l;
  }
  MPQArchive* ca = firstArchive;
  while (ca)
  {
    MPQArchive* a = ca->next;
    delete ca;
    ca = a;
  }
  firstArchive = NULL;
  firstLoader = NULL;
  init = false;
  return mpq_error = MPQ_OK;
}

uint32 MPQReadMapName (MPQARCHIVE handle, char* buf)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  file_seek (mpq->file, 8, MPQSEEK_SET);
  int len = 0;
  while (true)
  {
    file_read (buf + len, 1, mpq->file);
    if (buf[len++] == 0)
      break;
  }
  return mpq_error = MPQ_OK;
}

uint32 MPQFileSetFlags (MPQFILE handle, uint32 flag, uint32 mask)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  file->xflags = (file->xflags & (~mask)) | flag;
  return mpq_error = MPQ_OK;
}
uint32 MPQFileGetFlags (MPQFILE handle)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return 0;
  }
  mpq_error = MPQ_OK;
  return file->xflags;
}
uint32 MPQFileDel (MPQFILE handle, uint32 size)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->mode == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  mpq_error = MPQ_OK;
  if (file->pos + size > file->size)
  {
    mpq_error = MPQ_EOF;
    size = file->size - file->pos;
  }
  if (size)
  {
    if (file->pos + size < file->size)
      memmove (file->buf + file->pos, file->buf + file->pos + size, file->size - file->pos - size);
    file->size -= size;
  }
  return mpq_error;
}
uint32 MPQFilePush (MPQFILE handle, uint32 size)
{
  MPQFile* file = (MPQFile*) handle;
  if (file == NULL)
    return mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
  if (file->mode == 0)
    return mpq_serror = mpq_error = MPQ_ERROR_READONLY;
  if (size == 0)
    return mpq_error = MPQ_OK;
  if (file->size + size > file->curBlockSize)
  {
    uint32 nsize = (file->size + size + MPQ_DATA_GROW - 1) / MPQ_DATA_GROW * MPQ_DATA_GROW;
    uint8* nbuf = new uint8[nsize];
    memcpy (nbuf, file->buf, file->size);
    delete[] file->buf;
    file->buf = nbuf;
    file->curBlockSize = nsize;
  }
  if (file->pos < file->size)
    memmove (file->buf + file->pos + size, file->buf + file->pos, file->size - file->pos);
  memset (file->buf + file->pos, 0, size);
  file->size += size;
  return mpq_error = MPQ_OK;
}

char const* MPQGetFileName (MPQARCHIVE handle, uint32 pos)
{
  MPQArchive* mpq = (MPQArchive*) handle;
  if (mpq == NULL || mpq->file == 0 || pos < 0 || pos >= mpq->hdr->hashTableSize)
  {
    mpq_serror = mpq_error = MPQ_ERROR_PARAMS;
    return NULL;
  }
  if (mpq->fileNames && mpq->fileNames[pos][0])
    return mpq->fileNames[pos];
  else
    return NULL;
}
