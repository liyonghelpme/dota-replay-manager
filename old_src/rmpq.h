#ifndef __RMPQ_H__
#define __RMPQ_H__

#include <string.h>

#define MPQFILE_READ          0           // request read-only access
#define MPQFILE_REWRITE       1           // request write-only access, file is erased
                                          // for library functions this gives read-write access
#define MPQFILE_MODIFY        2           // request read-write access
#define MPQFILE_ENCRYPT       0x04
#define MPQFILE_COMPRESS      0x08
#define MPQFILE_FIXSEED       0x10
#define MPQFILE_NOENCRYPT     0x0400
#define MPQFILE_NOCOMPRESS    0x0800
#define MPQFILE_NOFIXSEED     0x1000

#define MPQSEEK_SET           0
#define MPQSEEK_CUR           1
#define MPQSEEK_END           2

#define MPQFILE_BINARY        0           // only for file system functions (doesn't work on mpq archives)
#define MPQFILE_TEXT          0x40

#define MPQ_NO_LISTFILE       0x0100      // for MPQOpen function, do not modify/create the (listfile)

/////////////////////////////// CONFIGURATION SECTION /////////////////////////

// modify these typedef's to work on your compiler
#include "types.h"

// define these functions to work with files (this is needed because 64-bit
// operations are implemented differently on different platforms)
uint32 file_open (char const* filename, int mode);
void file_close (uint32 file);
uint32 file_read (void* buf, uint32 size, uint32 file);
uint32 file_write (void const* buf, uint32 size, uint32 file);
void file_seek (uint32 file, int64 pos, int mode);
int64 file_tell (uint32 file);
void file_delete (char const* filename);

//////////////////////////// END OF CONFIGURATION SECTION /////////////////////

#define MPQ_OK                      0       // operation completed successfully
#define MPQ_ERROR_INIT              -1      // initialization function wasn't called
#define MPQ_ERROR_NOFILE            -2      // file not found
#define MPQ_ERROR_NONMPQ            -3      // specified file does not contain MPQ structure
#define MPQ_ERROR_READ              -4      // generic read error
#define MPQ_ERROR_PARAMS            -5      // invalid parameters, e.g. null file
#define MPQ_ERROR_DECOMPRESS        -6      // unknown compression method or invalid data
#define MPQ_EOF                     -7      // end of file reached
#define MPQ_ERROR_KEY               -8      // could not find file key
#define MPQ_ERROR_WRITE             -9      // generic write error
#define MPQ_ERROR_FULL              -10     // hash table full
#define MPQ_ERROR_ACCESS            -11     // access to file denied
#define MPQ_ERROR_READONLY          -12     // archive/file has been opened as readonly
#define MPQ_ERROR_COMPRESS          -13     // failed to compress data
#define MPQ_ERROR(val)              ((val)>=uint32(-13))

#define MPQ_LOCALE_NEUTRAL          0x0000
#define MPQ_LOCALE_CHINESE          0x0404
#define MPQ_LOCALE_CZECH            0x0405
#define MPQ_LOCALE_GERMAN           0x0407
#define MPQ_LOCALE_ENGLISH          0x0409
#define MPQ_LOCALE_SPANISH          0x040A
#define MPQ_LOCALE_FRENCH           0x040C
#define MPQ_LOCALE_ITALIAN          0x0410
#define MPQ_LOCALE_JAPANESE         0x0411
#define MPQ_LOCALE_KOREAN           0x0412
#define MPQ_LOCALE_POLISH           0x0415
#define MPQ_LOCALE_PORTUGUESE       0x0416
#define MPQ_LOCALE_RUSSIAN          0x0419
#define MPQ_LOCALE_ENGLISHUK        0x0809

#define MPQ_FILE_COMPRESS_PKWARE    0x00000100      // MPQ block flags, don't worry about those if you
#define MPQ_FILE_COMPRESS_MULTI     0x00000200      // don't know what they mean
#define MPQ_FILE_COMPRESSED         0x0000FF00      // Get them with MPQFileAttributes (x, MPQ_FILE_FLAGS)
#define MPQ_FILE_ENCRYPTED          0x00010000
#define MPQ_FILE_FIXSEED            0x00020000
#define MPQ_FILE_SINGLE_UNIT        0x01000000
#define MPQ_FILE_DUMMY_FILE         0x02000000
#define MPQ_FILE_HAS_EXTRA          0x04000000
#define MPQ_FILE_EXISTS             0x80000000

#define MPQ_FILE_FLAGS              0               // parameter for MPQFileAttribute and MPQItemAttribute
#define MPQ_FILE_COMPSIZE           1
#define MPQ_FILE_SIZE               2
#define MPQ_FILE_LOCALE             3
#define MPQ_FILE_PLATFORM           4
#define MPQ_FILE_OFFSET             5

#define MPQ_FILE_NAME1              6               // for MPQItemAttribute only
#define MPQ_FILE_NAME2              7
#define MPQ_FILE_BLOCK              8

// flags for MPQFileSetFlags
#define MPQ_FLAG_PUSH               0x00010000      // when you write anything into a file the contents after
                                                    // the cursos position will be moved instead of overwritten

#define MPQ_ALLOW_BZIP2         0x10                // specify which compression methods to use
#define MPQ_ALLOW_PKZIP         0x08                // pass to MPQSetCompression
#define MPQ_ALLOW_ZLIB          0x02                // note: only MPQ_ALLOW_ZLIB should be specified, others may crash
#define MPQ_ALLOW_HUFFMAN       0x01
#define MPQ_ALLOW_WAVE_STEREO   0x80
#define MPQ_ALLOW_WAVE_MONO     0x40

// get last error value
uint32 MPQError ();
uint32 MPQSavedError ();
void MPQResetError ();
char const* MPQErrorMessage (uint32 err);
char const* MPQLocaleName (uint16 locale);
inline bool MPQEOF ()
{
  return MPQError () == MPQ_EOF;
}

// initialize MPQ library, must be called first (will return MPQ_ERROR_INIT if called twice)
uint32 MPQInit ();
// cleanup MPQ library, closes all unclosed handles
uint32 MPQCleanup ();

typedef struct {} *MPQARCHIVE;
typedef struct {} *MPQFILE;
typedef struct {} *MPQLOADER;
typedef struct {} *MPQLISTFILE;
typedef char mpqname[256];

// create a list file (optionally from file)
MPQLISTFILE MPQCreateList (char const* filename = 0);
// insert a name into list file
uint32 MPQListInsert (MPQLISTFILE handle, char const* name);
// get the size of the list file
uint32 MPQListSize (MPQLISTFILE handle);
// get list file item
char const* MPQListItem (MPQLISTFILE handle, uint32 pos);
// delete list file
uint32 MPQDeleteList (MPQLISTFILE handle);
// flush list file
uint32 MPQFlushList (MPQLISTFILE handle, char const* filename);

// create a new MPQ archive
MPQARCHIVE MPQCreateArchive (char const* filename, bool listfile = true, uint32 offset = 0, uint32 hashSize = 1024,
                             uint32 blockSize = 131072, bool v2 = false);
// open MPQ archive from disc
// returns 0 on failure
MPQARCHIVE MPQOpen (char const* filename, uint32 mode = MPQFILE_MODIFY);
// close an archive
uint32 MPQClose (MPQARCHIVE handle);
// try to find names of files in the archive in the given listfile
uint32 MPQListFiles (MPQARCHIVE handle, MPQLISTFILE listfile);
// get current compression method(s) for flushed files
uint32 MPQGetCompression (MPQARCHIVE handle);
// get current compression method(s) for flushed files
uint32 MPQSetCompression (MPQARCHIVE handle, uint32 method);
// duplicate the archive
uint32 MPQSaveAs (MPQARCHIVE handle, char const* filename);
// make an archive use a temporary file to prevent changes to the original one
uint32 MPQMakeTemp (MPQARCHIVE handle);
// find a file in the archive
// returns 0 if file was not found
uint32 MPQFindFile (MPQARCHIVE handle, char const* name);
// same function, but specifies a locale to use in case there are multiple files with the same name
uint32 MPQFindFile (MPQARCHIVE handle, char const* name, uint16 locale);
// find next file with the given filename (note that it might eventually loop)
uint32 MPQFindNextFile (MPQARCHIVE handle, char const* name, uint32 cur);
// test if a file can be opened
bool MPQTestFile (MPQARCHIVE handle, uint32 pos);
// open a file in archive
// returns 0 if file was not found
MPQFILE MPQOpenFile (MPQARCHIVE handle, char const* name, uint32 options);
// same function, but specifies a locale to use in case there are multiple files with the same name
MPQFILE MPQOpenFile (MPQARCHIVE handle, char const* name, uint16 locale, uint32 options);
// open a file system file. note that these will not be closed by MPQCleanup
MPQFILE MPQOpenFSys (char const* name, uint32 options);
// close an open file
uint32 MPQCloseFile (MPQFILE file);
// find the MAXIMUM number of files in the archive (hashtable size)
uint32 MPQGetHashSize (MPQARCHIVE handle);
// check if a file with given name exists
bool MPQFileExists (MPQARCHIVE handle, char const* name);
// check if a file with given name and locale exists
bool MPQFileExists (MPQARCHIVE handle, char const* name, uint16 locale);
// check if a file exists at a given hashtable position
bool MPQFileExists (MPQARCHIVE handle, uint32 pos);
// open a file at a given hashtable position
// returns 0 if file does not exist or key could not be computed
MPQFILE MPQOpenFile (MPQARCHIVE handle, uint32 pos, uint32 options = MPQFILE_READ);
// reopen a file (if it was somehow modified)
uint32 MPQReopenFile (MPQFILE handle);
// get file attribute, use MPQ_FILE_... constants
uint32 MPQFileAttribute (MPQFILE handle, uint32 attr);
// read from a file
// returns the number of bytes successfully read
uint32 MPQFileRead (MPQFILE handle, uint32 count, void* ptr);
// write to a file
// returns the number of bytes successfully written
uint32 MPQFileWrite (MPQFILE handle, uint32 count, void const* ptr);
// read a character from the file
uint32 MPQFileGetc (MPQFILE handle);
// read a line from the file
char* MPQFileGets (MPQFILE handle, uint32 count, char* ptr);
// write a character to the file
uint32 MPQFilePutc (MPQFILE handle, uint8 c);
// go to file position (use MPQSEEK_... constants)
// returns the new file position
uint32 MPQFileSeek (MPQFILE handle, uint32 pos, int mode);
// get file position
uint32 MPQFileTell (MPQFILE handle);
// rename file
uint32 MPQRenameFile (MPQARCHIVE handle, char const* source, char const* dest);
// encrypt/decrypt file
uint32 MPQEncryptFile (MPQARCHIVE handle, char const* name, uint32 options);
// delete file
uint32 MPQDeleteFile (MPQARCHIVE handle, char const* name);
// increase hash table size (size will be rounded up to power of 2)
// NOTE that this procedure will make files with unknown names potentially unaccessible
// NOTE that all open files will become invalid
uint32 MPQResizeHash (MPQARCHIVE handle, uint32 size);
// check if the archive contains files with unknown filenames
bool MPQHasUnknowns (MPQARCHIVE handle);
// change all 'empty' hash entries to 'deleted' to make files with unknown names accessible
// after hash table resize operation
uint32 MPQFillHashTable (MPQARCHIVE handle);
// set file flags
uint32 MPQFileSetFlags (MPQFILE handle, uint32 flag, uint32 mask);
// get file flags
uint32 MPQFileGetFlags (MPQFILE handle);
// delete bytes starting from file position
uint32 MPQFileDel (MPQFILE handle, uint32 size);
// push null bytes at file position
uint32 MPQFilePush (MPQFILE handle, uint32 size);
// get file name, if possible
char const* MPQGetFileName (MPQARCHIVE handle, uint32 pos);
// get attribute for a hash item
uint32 MPQItemAttribute (MPQARCHIVE handle, uint32 pos, uint32 attr);
// read first <= 17 bytes of the file (for determining file type, 17 for TGA)
uint32 MPQPeekFile (MPQARCHIVE handle, uint32 pos, uint8* dest);

// flush file
uint32 MPQFlushFile (MPQFILE handle);
// flush archive, removing unused space in place of deleted/modified files
uint32 MPQFlush (MPQARCHIVE handle);
// write a new file list
uint32 MPQFlushListfile (MPQARCHIVE handle);

// read map name from W3M/W3X file (from offset 8)
uint32 MPQReadMapName (MPQARCHIVE handle, char* buf);

// create loader object
MPQLOADER MPQCreateLoader (char const* prefix = NULL);
// release loader object
uint32 MPQReleaseLoader (MPQLOADER handle);
// add archive to loader
uint32 MPQAddArchive (MPQLOADER loader, MPQARCHIVE archive);
// remove archive from loader
uint32 MPQRemoveArchive (MPQLOADER loader, MPQARCHIVE archive);
// find a file (read only)
MPQFILE MPQLoadFile (MPQLOADER handle, char const* name);
// load archive (loader takes care of it)
uint32 MPQLoadArchive (MPQLOADER handle, char const* path);

inline uint32 MPQWriteInt (MPQFILE file, int i)
{
  return MPQFileWrite (file, sizeof i, &i);
}
inline uint32 MPQWriteFloat (MPQFILE file, float f)
{
  return MPQFileWrite (file, sizeof f, &f);
}
inline uint32 MPQWriteString (MPQFILE file, char const* s)
{
  return MPQFileWrite (file, strlen (s) + 1, s);
}
inline uint32 MPQFilePuts (MPQFILE file, char const* s)
{
  return MPQFileWrite (file, strlen (s), s);
}
inline int MPQReadInt (MPQFILE file)
{
  int x;
  MPQFileRead (file, sizeof x, &x);
  return x;
}
inline float MPQReadFloat (MPQFILE file)
{
  float x;
  MPQFileRead (file, sizeof x, &x);
  return x;
}
inline uint32 MPQReadString (MPQFILE file, char* str)
{
  while (*str++ = (char) MPQFileGetc (file))
    ;
  return MPQError ();
}
inline uint32 MPQSkipString (MPQFILE file)
{
  while (MPQFileGetc (file) && !MPQError ())
    ;
  return MPQError ();
}
#endif
