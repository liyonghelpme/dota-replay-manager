#include "stdafx.h"
#include "ilib.h"

#include "replay.h"
#include "resource.h"
#include "settingsdlg.h"
#include "registry.h"
#include "image.h"

#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

extern MPQLOADER warloader;

// ZIP

#define LOCAL_ZIP_SIGNATURE      0x04034B50
#define CENTRAL_ZIP_SIGNATURE    0x02014B50
#define END_CENTRAL_SIGNATURE    0x06054B50
#define EXTD_LOCAL_SIGNATURE     0x08074B50

#define DEF_WBITS       15
#define ZIP_STORE       0
#define ZIP_DEFLATE     8

#pragma pack (push, 1)

struct ZipLocalFileHeader
{
  unsigned long signature;
  unsigned short versionNeededToExtract;
  unsigned short generalPurposeBitFlag;
  unsigned short compressionMethod;
  unsigned short lastModFileTime;
  unsigned short lastModFileDate;
  unsigned long crc32;
  long compressedSize;
  long uncompressedSize;
  unsigned short filenameLength;
  unsigned short extraFieldLength;
};

struct ZipDataDescriptor
{
  unsigned long crc32;
  unsigned long compressedSize;
  unsigned long uncompressedSize;
};

struct ZipCentralHeader
{
  unsigned long signature;
  unsigned short versionMadeBy;
  unsigned short versionNeededToExtract;
  unsigned short generalPurposeBitFlag;
  unsigned short compressionMethod;
  unsigned short lastModFileTime;
  unsigned short lastModFileDate;
  unsigned long crc32;
  long compressedSize;
  long uncompressedSize;
  unsigned short filenameLength;
  unsigned short extraFieldLength;
  unsigned short commentLength;
  unsigned short diskNumberStart;
  unsigned short internalFileAttibutes;
  unsigned long  externalFileAttributes;
  long relativeLocalHeaderOffset;
};

struct ZipEndOfCentralDir
{
  unsigned long signature;
  unsigned short diskNo;
  unsigned short centralDirDiskNo;
  unsigned short numEntriesOnDisk;
  unsigned short numEntries;
  unsigned long centralDirSize;
  long centralDirOffset;
  unsigned short commentLength;
};

#pragma pack (pop)

#define CHUNK_SIZE 1024

// GIF
#define MAX_CODES 4095

#pragma pack (push, 1)
struct GifHeader
{
  char sign[6];
  short screenWidth;
  short screenHeight;
  char misc;
  char back;
  char null;
};
struct GifImageHeader
{
  char sign;
  short x, y;
  short width, height;
  char misc;
};

struct Rgb
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

struct Rgba
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};
#pragma pack (pop)

static bool getBytes (void* dst, int sz, unsigned char* src, int& pos, int len)
{
  if (pos + sz > len) return false;
  memcpy (dst, src + pos, sz);
  pos += sz;
  return true;
}

namespace gif
{
  int codeSize;
  int clearCode;
  int endingCode;
  int freeCode;
  int topSlot;
  int slot;
  unsigned char b1;
  unsigned char buf[257];
  unsigned char* pbytes;
  int bytesInBlock = 0;
  int bitsLeft = 0;
  long codeMask[13] =
  {
    0,
    0x0001, 0x0003,
    0x0007, 0x000F,
    0x001F, 0x003F,
    0x007F, 0x00FF,
    0x01FF, 0x03FF,
    0x07FF, 0x0FFF
  };
  unsigned char stack[MAX_CODES + 1];
  unsigned char suffix[MAX_CODES + 1];
  int prefix [MAX_CODES + 1];

  int loadGifBlock (unsigned char* data, int& pos, int len)
  {
    pbytes = buf;

    if ((bytesInBlock = data[pos++]) == -1)
      return bytesInBlock;
    else if (bytesInBlock > 0 && !getBytes (buf, bytesInBlock, data, pos, len))
      return -1;
    return bytesInBlock;
  }
  int getGifCode (unsigned char* data, int& pos, int len)
  {
    if (bitsLeft == 0)
    {
      if (bytesInBlock <= 0 && loadGifBlock (data, pos, len) == -1)
        return -1;
      b1 = *pbytes++;
      bitsLeft = 8;
      bytesInBlock--;
    }
    int ret = b1 >> (8 - bitsLeft);
    while (codeSize > bitsLeft)
    {
      if (bytesInBlock <= 0 && loadGifBlock (data, pos, len) == -1)
        return -1;
      b1 = *pbytes++;
      ret |= b1 << bitsLeft;
      bitsLeft += 8;
      bytesInBlock--;
    }
    bitsLeft -= codeSize;
    ret &= codeMask[codeSize];

    return ret;
  }
}

struct ArchiveImage
{
  char filename[256];
  ZipCentralHeader hdr;
  unsigned char* data;
  CBitmap* bmp;
  int listIndex;
  unsigned char* glbuf;
  int width;
  int height;

  ArchiveImage ()
  {
    data = NULL;
    bmp = NULL;
    glbuf = NULL;
  }
  ~ArchiveImage ()
  {
    delete[] data;
    delete bmp;
    delete[] glbuf;
  }

  bool matches (char const* title);
};
bool ArchiveImage::matches (char const* title)
{
  if (stricmp (title, filename) == 0) return true;
  static char tit[256];
  _splitpath (filename, NULL, NULL, tit, NULL);
  return stricmp (title, tit) == 0;
}

static CImageList* ilist;
static Array<ArchiveImage> images;

int sqirp (int c00, int c01, int c10, int c11, double mx, double my)
{
  int r = int ((double (c00) + double (c01 - c00) * mx) * (1 - my) +
               (double (c10) + double (c11 - c10) * mx) * my);
  if (r > 255) r = 255;
  if (r < 0) r = 0;
  return r;
}

bool loadImageGIF (ArchiveImage* img, CDC* dc)
{
  GifHeader hdr;

  unsigned char* data = img->data;
  int pos = 0;
  int len = img->hdr.uncompressedSize;

  if (!getBytes (&hdr, sizeof hdr, data, pos, len))
    return false;
  if (strncmp (hdr.sign, "GIF87a", 6) && strncmp (hdr.sign, "GIF89a", 6))
    return false;
  int bitsPerPixel = 1 + (7 & hdr.misc);
  Rgb* pal = NULL;
  if (hdr.misc & 0x80)
  {
    pal = new Rgb[1 << bitsPerPixel];
    if (!getBytes (pal, sizeof (Rgb) * (1 << bitsPerPixel), data, pos, len))
    {
      delete[] pal;
      return false;
    }
  }
  GifImageHeader imageHdr;
  if (!getBytes (&imageHdr, sizeof imageHdr, data, pos, len))
  {
    delete[] pal;
    return false;
  }
  while (imageHdr.sign == '!')
  {
    int extType = * (1 + (char*) &imageHdr);
    int size = * (2 + (char*) &imageHdr);
    pos += size + 4 - sizeof imageHdr;
    if (!getBytes (&imageHdr, sizeof imageHdr, data, pos, len))
    {
      delete[] pal;
      return false;
    }
  }
  if (imageHdr.sign != ',')
  {
    delete[] pal;
    return false;
  }
  if (imageHdr.misc & 0x80)
  {
    bitsPerPixel = 1 + (imageHdr.misc & 7);
    delete[] pal;
    pal = new Rgb[1 << bitsPerPixel];
    if (!getBytes (pal, sizeof (Rgb) * (1 << bitsPerPixel), data, pos, len))
    {
      delete[] pal;
      return false;
    }
  }
  if (pal == NULL)
    return false;
  int width = imageHdr.width;
  int height = imageHdr.height;
  int size;
  if ((size = data[pos++]) == -1)
  {
    delete[] pal;
    return false;
  }
  if (size < 2 || size > 9)
  {
    delete[] pal;
    return false;
  }
  unsigned char* ptr = new unsigned char[width * height];
  unsigned char* tmpBuf = ptr;

  gif::codeSize = size + 1;
  gif::topSlot = 1 << gif::codeSize;
  gif::clearCode = 1 << size;
  gif::endingCode = gif::clearCode + 1;
  gif::slot = gif::freeCode = gif::endingCode + 1;
  gif::bytesInBlock = gif::bitsLeft = 0;

  unsigned char* sp;
  int code;
  int c;
  int oc = 0;
  int fc = 0;
  sp = gif::stack;
  while ((c = gif::getGifCode (data, pos, len)) != gif::endingCode)
  {
    if (c < 0)
    {
      delete[] pal;
      delete[] tmpBuf;
      return false;
    }
    if (c == gif::clearCode)
    {
      gif::codeSize = size + 1;
      gif::slot = gif::freeCode;
      gif::topSlot = 1 << gif::codeSize;
      while ((c = gif::getGifCode (data, pos, len)) == gif::clearCode)
        ;
      if (c == gif::endingCode)
        break;
      if (c >= gif::slot)
        c = 0;
      oc = fc = c;
      *ptr++ = c;
    }
    else
    {
      code = c;
      if (code >= gif::slot)
      {
        if (code > gif::slot)
        {
          delete[] pal;
          delete[] tmpBuf;
          return false;
        }
        code = oc;
        *sp++ = fc;
      }
      while (code >= gif::freeCode)
      {
        *sp++ = gif::suffix[code];
        code = gif::prefix[code];
      }
      *sp++ = code;
      if (gif::slot < gif::topSlot)
      {
        gif::suffix[gif::slot] = fc = code;
        gif::prefix[gif::slot++] = oc;
        oc = c;
      }
      if (gif::slot >= gif::topSlot)
      {
        if (gif::codeSize < 12)
        {
          gif::topSlot <<= 1;
          gif::codeSize++;
        }
      }
      while (sp > gif::stack)
        *ptr++ = *--sp;
    }
  }

  img->width = width;
  img->height = height;

  int pitch = width * 4;
  unsigned char* bm = new unsigned char[height * pitch];
  img->glbuf = new unsigned char[width * height * 3];
  ptr = bm;
  unsigned char* glbits = img->glbuf + (height - 1) * width * 3;
  for (int i = 0; i < height; i++)
  {
    unsigned char* p = tmpBuf + i * width;
    unsigned char* gld = glbits;
    for (int j = 0; j < width; j++)
    {
      *ptr++ = pal[p[j]].blue;
      *ptr++ = pal[p[j]].green;
      *ptr++ = pal[p[j]].red;
      *ptr++ = 0;
      *gld++ = pal[p[j]].red;
      *gld++ = pal[p[j]].green;
      *gld++ = pal[p[j]].blue;
    }
    glbits -= width * 3;
  }
  delete[] tmpBuf;
  delete[] pal;

  img->bmp = new CBitmap ();
  if (!img->bmp->CreateCompatibleBitmap (dc, width, height))
  {
    delete img->bmp;
    delete[] bm;
    return false;
  }
  BITMAPINFOHEADER bi;
  bi.biSize = sizeof bi;
  bi.biWidth = width;
  bi.biHeight = -height;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 1;
  bi.biYPelsPerMeter = 1;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;
  if (!SetDIBits (dc->m_hDC, (HBITMAP) img->bmp->m_hObject, 0, height, bm, (BITMAPINFO*) &bi,
    DIB_RGB_COLORS))
  {
    delete img->bmp;
    delete[] bm;
    return false;
  }

  delete[] bm;
  return true;
}

bool isIconSized (CBitmap* bmp)
{
  BITMAP b;
  bmp->GetBitmap (&b);
  return (b.bmWidth == 16 && b.bmHeight == 16);
}

bool loadImages (char const* filename)
{
  char path[512];
  sprintf (path, "%s%s", reg.getPath (), filename);
  int file = open (path, O_BINARY | O_RDONLY);
  if (file == -1)
    return false;

  char buf[CHUNK_SIZE];
  int step = sizeof (ZipEndOfCentralDir);

  lseek (file, 0, SEEK_END);

  int offs = tell (file);
  int minOffs = offs - (65535 + step);
  int pos;
  if (minOffs < 0)
    minOffs = 0;

  ZipEndOfCentralDir* dirEndPtr = NULL;
  while (offs > minOffs && dirEndPtr == NULL)
  {
    if (offs > (int) sizeof buf - step)
      offs -= (int) sizeof buf - step;
    else
      offs = 0;
    lseek (file, (long) offs, SEEK_SET);

    pos = read (file, buf, sizeof buf);
    if (pos < step)
      continue;

    for (int i = pos - step; i > 0 && dirEndPtr == NULL; i--)
    {
      if (* (unsigned long*) (buf + i) == END_CENTRAL_SIGNATURE)
      {
        dirEndPtr = (ZipEndOfCentralDir*) (buf + i);
        if (lseek (file, dirEndPtr->centralDirOffset, SEEK_SET) != dirEndPtr->centralDirOffset)
        {
          close (file);
          return false;
        }
      }
    }
  }
  if (dirEndPtr)
  {
    while (true)
    {
      ArchiveImage img;
      if (read (file, &img.hdr, sizeof img.hdr) != sizeof img.hdr)
        break;
      if (img.hdr.signature != CENTRAL_ZIP_SIGNATURE ||
          img.hdr.filenameLength >= sizeof img.filename)
      {
        close (file);
        return false;
      }
      read (file, img.filename, img.hdr.filenameLength);

      img.filename[img.hdr.filenameLength] = '\0';

      images.add (img);
    }
  }
  else
  {
    close (file);
    return false;
  }

  for (int i = 0; i < images.getSize (); i++)
  {
    ArchiveImage& img = images[i];

    if (lseek (file, img.hdr.relativeLocalHeaderOffset, SEEK_SET) != img.hdr.relativeLocalHeaderOffset)
    {
      close (file);
      return false;
    }
    ZipLocalFileHeader localHdr;
    if (read (file, &localHdr, sizeof localHdr) != sizeof localHdr ||
        localHdr.signature != LOCAL_ZIP_SIGNATURE)
    {
      close (file);
      return false;
    }
    lseek (file, localHdr.filenameLength + localHdr.extraFieldLength, SEEK_CUR);

    img.data = new unsigned char[img.hdr.uncompressedSize + 1];

    switch (img.hdr.compressionMethod)
    {
    case ZIP_STORE:
      if (read (file, img.data, img.hdr.compressedSize) != img.hdr.compressedSize)
      {
        close (file);
        return false;
      }
      break;
    case ZIP_DEFLATE:
      {
        char* inBuffer = new char[img.hdr.compressedSize + 1];
        if (read (file, inBuffer, img.hdr.compressedSize) != img.hdr.compressedSize ||
           !gzinflate (inBuffer, (char*) img.data, img.hdr.compressedSize, img.hdr.uncompressedSize))
        {
          delete[] inBuffer;
          close (file);
          return false;
        }
        delete[] inBuffer;
      }
      break;
    default:
      close (file);
      return false;
    }
  }
  close (file);

  int numBTN = 0;
  CDC dc;
  dc.Attach (GetDC (NULL));

  for (int i = 0; i < images.getSize (); i++)
  {
    ArchiveImage& img = images[i];
    char ext[256];
    _splitpath (img.filename, NULL, NULL, NULL, ext);
    if (!stricmp (ext, ".gif"))
      loadImageGIF (&img, &dc);
    if (img.bmp && isIconSized (img.bmp))
      img.listIndex = numBTN++;
  }

  ilist = new CImageList ();
  ilist->Create (16, 16, ILC_COLOR24, numBTN, 16);
  for (int i = 0; i < images.getSize (); i++)
  {
    ArchiveImage& img = images[i];
    if (img.bmp && isIconSized (img.bmp))
      ilist->Add (img.bmp, (CBitmap*) NULL);
  }

  return true;
}

void unloadImages ()
{
  delete ilist;
}

CImageList* getImageList ()
{
  return ilist;
}

int getImageIndex (char const* image)
{
  for (int i = 0; i < images.getSize (); i++)
    if (images[i].matches (image))
      return images[i].listIndex;
  return getImageIndex ("Empty");
}

CBitmap* getImageBitmap (char const* image, RImageInfo* info)
{
  for (int i = 0; i < images.getSize (); i++)
    if (images[i].matches (image))
    {
      if (info)
      {
        info->width = images[i].width;
        info->height = images[i].height;
        info->bmp = images[i].bmp;
      }
      return images[i].bmp;
    }
  return NULL;
}

GLImage::GLImage (char const* image)
{
  width = 0;
  height = 0;
  bits = NULL;
  for (int i = 0; i < images.getSize (); i++)
    if (images[i].matches (image))
    {
      width = images[i].width;
      height = images[i].height;
      bits = images[i].glbuf;
      break;
    }
}

ResFile::ResFile (char const* file)
{
  size = 0;
  data = NULL;
  for (int i = 0; i < images.getSize (); i++)
    if (images[i].matches (file))
    {
      size = images[i].hdr.uncompressedSize;
      data = images[i].data;
      break;
    }
}

CBitmap* addImage (char const* image, BLPImage* blp, RImageInfo* info)
{
  if (info)
  {
    info->bmp = NULL;
    info->width = 0;
    info->height = 0;
  }
  ArchiveImage fimg;
  memset (&fimg, 0, sizeof fimg);
  ArchiveImage* img = &images[images.add (fimg)];
  strcpy (img->filename, image);
  img->data = (unsigned char*) blp->data;
  img->width = blp->width;
  img->height = blp->height;
  blp->data = NULL;
  delete blp;
  img->listIndex = getImageIndex ("Empty");
  img->glbuf = NULL;
  HDC dc = GetDC (NULL);
  img->bmp = new CBitmap ();
  if (!img->bmp->CreateCompatibleBitmap (CDC::FromHandle (dc), img->width, img->height))
  {
    ReleaseDC (NULL, dc);
    return NULL;
  }
  BITMAPINFOHEADER bi;
  bi.biSize = sizeof bi;
  bi.biWidth = img->width;
  bi.biHeight = -img->height;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 1;
  bi.biYPelsPerMeter = 1;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;
  if (!SetDIBits (dc, (HBITMAP) img->bmp->m_hObject, 0, img->height, img->data, (BITMAPINFO*) &bi,
      DIB_RGB_COLORS))
  {
    ReleaseDC (NULL, dc);
    return NULL;
  }
  if (info)
  {
    info->width = img->width;
    info->height = img->height;
    info->bmp = img->bmp;
  }
  return img->bmp;
}
CBitmap* cpyImage (char const* image, BLPImage* blp, RImageInfo* info)
{
  BLPImage* tmp = new BLPImage;
  tmp->width = blp->width;
  tmp->height = blp->height;
  tmp->data = new uint32[blp->width * blp->height];
  memcpy (tmp->data, blp->data, blp->width * blp->height * sizeof (uint32));
  return addImage (image, tmp, info);
}

void load