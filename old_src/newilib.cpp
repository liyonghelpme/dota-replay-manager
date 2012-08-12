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
  int size;
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
static Array<ArchiveImage*> images;

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
  int len = img->size;

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
bool loadImageBIN (ArchiveImage* img, CDC* dc)
{
  if (img->size < 12) return false;
  int sig = ((int*) img->data)[0];
  int width = img->width = ((int*) img->data)[1];
  int height = img->height = ((int*) img->data)[2];
  if (sig != 'BIMG' || img->size != 12 + img->width * img->height * 3)
    return false;

  unsigned char* src = img->data + 12;
  int pitch = width * 4;
  unsigned char* bm = new unsigned char[height * pitch];
  img->glbuf = new unsigned char[width * height * 3];
  unsigned char* ptr = bm;
  unsigned char* glbits = img->glbuf + (height - 1) * width * 3;
  for (int i = 0; i < height; i++)
  {
    unsigned char* gld = glbits;
    for (int j = 0; j < width; j++)
    {
      *ptr++ = src[2];
      *ptr++ = src[1];
      *ptr++ = src[0];
      *ptr++ = 0;
      *gld++ = src[0];
      *gld++ = src[1];
      *gld++ = src[2];
      src += 3;
    }
    glbits -= width * 3;
  }

  img->bmp = new CBitmap ();
  if (!img->bmp->CreateCompatibleBitmap (dc, img->width, img->height))
  {
    delete img->bmp;
    delete[] bm;
    return false;
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
#include "dotareplay.h"
bool loadImage (char const* name, CDC* dc)
{
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  MPQFILE file = MPQOpenFile (res, name, MPQFILE_READ);
  if (!file)
    return false;
  ArchiveImage* img = new ArchiveImage ();
  memset (img, 0, sizeof ArchiveImage);
  images.add (img);
  strcpy (img->filename, name);
  img->size = MPQFileAttribute (file, MPQ_FILE_SIZE);
  img->data = new unsigned char[img->size];
  MPQFileRead (file, img->size, img->data);

  if (loadImageGIF (img, dc) || loadImageBIN (img, dc))
  {
    if (img->bmp && isIconSized (img->bmp))
      img->listIndex = ilist->Add (img->bmp, (CBitmap*) NULL);
  }

  MPQCloseFile (file);
  return true;
}
bool loadBinImage (MPQFILE file, CDC* dc)
{
  ArchiveImage* img = new ArchiveImage ();
  memset (img, 0, sizeof ArchiveImage);
  images.add (img);
  img->size = MPQReadInt (file);
  MPQReadString (file, img->filename);
  img->data = new unsigned char[img->size];
  MPQFileRead (file, img->size, img->data);
  if (loadImageBIN (img, dc))
    if (img->bmp && isIconSized (img->bmp))
      img->listIndex = ilist->Add (img->bmp, (CBitmap*) NULL);
  return true;
}
bool loadBinImages (CDC* dc)
{
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  MPQFILE file = MPQOpenFile (res, "images\\icons.dat", MPQFILE_READ);
  if (!file)
    return false;
  int sig = MPQReadInt (file);
  if (sig == 'IDAT')
  {
    int count = MPQReadInt (file);
    for (int i = 0; i < count; i++)
      loadBinImage (file, dc);
  }
  MPQCloseFile (file);
  return true;
}
bool convertToDat ()
{
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  MPQFILE file = MPQOpenFile (res, "images\\icons.dat", MPQFILE_MODIFY);
  if (file == NULL)
    return false;
  int count = 0;
  if (MPQFileAttribute (file, MPQ_FILE_SIZE) < 4 || MPQReadInt (file) != 'IDAT')
  {
    MPQFileSeek (file, 0, MPQSEEK_SET);
    MPQFileDel (file, MPQFileAttribute (file, MPQ_FILE_SIZE));
    MPQWriteInt (file, 'IDAT');
    MPQWriteInt (file, 0);
  }
  else
    count = MPQReadInt (file);
  for (uint32 i = 0; i < MPQGetHashSize (res); i++)
  {
    char const* name = MPQGetFileName (res, i);
    if (name && !strnicmp (name, "images\\", 7) && stricmp (name, "images\\icons.dat"))
    {
      if (stricmp (name + strlen (name) - 4, ".bin"))
        continue;
      MPQFILE tod = MPQOpenFile (res, name, MPQFILE_READ);
      if (tod)
      {
        MPQWriteInt (file, MPQFileAttribute (tod, MPQ_FILE_SIZE));
        MPQWriteString (file, name);
        static char buf[1024];
        while (int count = MPQFileRead (tod, sizeof buf, buf))
          MPQFileWrite (file, count, buf);
        MPQCloseFile (tod);
        count++;
      }
      MPQDeleteFile (res, name);
    }
  }
  MPQFileSeek (file, 4, MPQSEEK_SET);
  MPQWriteInt (file, count);
  MPQCloseFile (file);
  return true;
}
bool loadImages ()
{
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  ilist = new CImageList ();
  ilist->Create (16, 16, ILC_COLOR24, 16, 16);
  CDC dc;
  dc.Attach (GetDC (NULL));
  convertToDat ();
  for (uint32 i = 0; i < MPQGetHashSize (res); i++)
  {
    char const* name = MPQGetFileName (res, i);
    if (name && !strnicmp (name, "images\\", 7) && stricmp (name, "images\\icons.dat"))
      loadImage (name, &dc);
  }
  loadBinImages (&dc);

  return true;
}

void unloadImages ()
{
  for (int i = 0; i < images.getSize (); i++)
    delete images[i];
  delete ilist;
}

CImageList* getImageList ()
{
  return ilist;
}

int getImageIndex (char const* image)
{
  for (int i = 0; i < images.getSize (); i++)
    if (images[i]->matches (image))
      return images[i]->listIndex;
  return getImageIndex ("Empty");
}

CBitmap* getImageBitmap (char const* image, RImageInfo* info)
{
  for (int i = 0; i < images.getSize (); i++)
    if (images[i]->matches (image))
    {
      if (info)
      {
        info->width = images[i]->width;
        info->height = images[i]->height;
        info->bmp = images[i]->bmp;
      }
      return images[i]->bmp;
    }
  return NULL;
}

GLImage::GLImage (char const* image)
{
  width = 0;
  height = 0;
  bits = NULL;
  for (int i = 0; i < images.getSize (); i++)
    if (images[i]->matches (image))
    {
      width = images[i]->width;
      height = images[i]->height;
      bits = images[i]->glbuf;
      break;
    }
}

ResFile::ResFile (char const* file)
{
  size = 0;
  data = NULL;
  for (int i = 0; i < images.getSize (); i++)
    if (images[i]->matches (file))
    {
      size = images[i]->size;
      data = images[i]->data;
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
  ArchiveImage* img = new ArchiveImage;
  memset (img, 0, sizeof ArchiveImage);
  images.add (img);
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

extern MPQLOADER warloader;
struct ImgCompressor
{
  int* col[3];
  int* out[3];
  int* cnt;
  ImgCompressor (int wd, int ht)
  {
    col[0] = new int[wd * ht];
    col[1] = new int[wd * ht];
    col[2] = new int[wd * ht];
    cnt = new int[wd * ht];
    memset (col[0], 0, wd * ht * 4);
    memset (col[1], 0, wd * ht * 4);
    memset (col[2], 0, wd * ht * 4);
    memset (cnt, 0, wd * ht * 4);
    out[0] = new int[wd * ht];
    out[1] = new int[wd * ht];
    out[2] = new int[wd * ht];
    memset (out[0], 0, wd * ht * 4);
    memset (out[1], 0, wd * ht * 4);
    memset (out[2], 0, wd * ht * 4);
  }
  ~ImgCompressor ()
  {
    delete[] col[0];
    delete[] col[1];
    delete[] col[2];
    delete[] cnt;
    delete[] out[0];
    delete[] out[1];
    delete[] out[2];
  }
};
void sharpenImage (ImgCompressor& cmp, int size, float coeff)
{
  int ox[] = {-1, 0, 0, 1};
  int oy[] = {0, -1, 1, 0};
  for (int y = 0; y < size; y++)
  {
    for (int x = 0; x < size; x++)
    {
      for (int c = 0; c < 3; c++)
      {
        int count = 0;
        int accum = 0;
        for (int k = 0; k < 4; k++)
        {
          int x0 = x + ox[k];
          int y0 = y + oy[k];
          if (x0 >= 0 && x0 < size && y0 >= 0 && y0 < size)
          {
            count++;
            accum += cmp.col[c][x0 + y0 * size];
          }
          int res = int (float (cmp.col[c][x + y * size]) * (1 + coeff * count) - float (accum) * coeff + 0.5f);
          if (res < 0) res = 0;
          if (res > 255) res = 255;
          cmp.out[c][x + y * size] = res;
        }
      }
    }
  }
}
void addBigImage (BLPImage* src, char const* title)
{
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  char fname[256];
  sprintf (fname, "images\\big%s.bin", title);
  MPQFILE dst = MPQOpenFile (res, "images\\icons.bin", MPQFILE_REWRITE);
  if (dst)
  {
    MPQFileSeek (dst, 4, MPQSEEK_SET);
    int count = MPQReadInt (dst);
    MPQFileSeek (dst, 4, MPQSEEK_SET);
    MPQWriteInt (dst, count + 1);
    MPQFileSeek (dst, 0, MPQSEEK_END);
    int filePos = MPQFileTell (dst);
    MPQWriteInt (dst, 0);
    MPQWriteString (dst, fname);
    int fileStart = MPQFileTell (dst);
    MPQWriteInt (dst, 'BIMG');
    MPQWriteInt (dst, 32);
    MPQWriteInt (dst, 32);
    ImgCompressor cmp (32, 32);
    for (int x = 0; x < src->width; x++)
      for (int y = 0; y < src->height; y++)
      {
        int i = (x * 32 / src->width) + (y * 32 / src->height) * 32;
        cmp.col[0][i] += src->data[x + y * src->width] & 0xFF;
        cmp.col[1][i] += (src->data[x + y * src->width] >> 8) & 0xFF;
        cmp.col[2][i] += (src->data[x + y * src->width] >> 16) & 0xFF;
        cmp.cnt[i]++;
      }
    for (int i = 0; i < 1024; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        cmp.col[j][i] = (cmp.col[j][i] * 7 / 6) / cmp.cnt[i];
        if (cmp.col[j][i] > 255) cmp.col[j][i] = 255;
      }
    }
    sharpenImage (cmp, 32, 0.08f);
    for (int i = 0; i < 1024; i++)
      for (int j = 0; j < 3; j++)
        MPQFilePutc (dst, cmp.out[j][i]);
    CDC dc;
    dc.Attach (GetDC (NULL));
    int fileEnd = MPQFileTell (dst);
    MPQFileSeek (dst, filePos, MPQSEEK_SET);
    MPQWriteInt (dst, fileEnd - fileStart);
    MPQFileSeek (dst, filePos, MPQSEEK_SET);
    loadBinImage (dst, &dc);
    MPQCloseFile (dst);
  }
}
void addNewImage (char const* path, bool big)
{
  char title[256];
  _splitpath (path, NULL, NULL, title, NULL);
  for (int i = 0; i < images.getSize (); i++)
    if (images[i]->matches (title))
      return;
  MPQARCHIVE res = ((CDotAReplayApp*) ::AfxGetApp ())->res;
  BLPImage* src = MPQLoadImage (warloader, path);
  if (src)
  {
    char fname[256];
    sprintf (fname, "images\\%s.bin", title);
    MPQFILE dst = MPQOpenFile (res, "images\\icons.dat", MPQFILE_MODIFY);
    if (dst)
    {
      MPQFileSeek (dst, 4, MPQSEEK_SET);
      int count = MPQReadInt (dst);
      MPQFileSeek (dst, 4, MPQSEEK_SET);
      MPQWriteInt (dst, count + 1);
      MPQFileSeek (dst, 0, MPQSEEK_END);
      int filePos = MPQFileTell (dst);
      MPQWriteInt (dst, 0);
      MPQWriteString (dst, fname);
      int fileStart = MPQFileTell (dst);
      MPQWriteInt (dst, 'BIMG');
      MPQWriteInt (dst, 16);
      MPQWriteInt (dst, 16);
      ImgCompressor cmp (16, 16);
      for (int x = 0; x < src->width; x++)
        for (int y = 0; y < src->height; y++)
        {
          int i = (x * 16 / src->width) + (y * 16 / src->height) * 16;
          cmp.col[0][i] += src->data[x + y * src->width] & 0xFF;
          cmp.col[1][i] += (src->data[x + y * src->width] >> 8) & 0xFF;
          cmp.col[2][i] += (src->data[x + y * src->width] >> 16) & 0xFF;
          cmp.cnt[i]++;
        }
      for (int i = 0; i < 256; i++)
      {
        for (int j = 0; j < 3; j++)
        {
          cmp.col[j][i] = (cmp.col[j][i] * 7 / 6) / cmp.cnt[i];
          if (cmp.col[j][i] > 255) cmp.col[j][i] = 255;
        }
      }
      sharpenImage (cmp, 16, 0.16f);
      for (int i = 0; i < 256; i++)
        for (int j = 0; j < 3; j++)
          MPQFilePutc (dst, cmp.out[j][i]);
      CDC dc;
      dc.Attach (GetDC (NULL));
      int fileEnd = MPQFileTell (dst);
      MPQFileSeek (dst, filePos, MPQSEEK_SET);
      MPQWriteInt (dst, fileEnd - fileStart);
      MPQFileSeek (dst, filePos, MPQSEEK_SET);
      loadBinImage (dst, &dc);
      MPQCloseFile (dst);
    }
    if (big)
      addBigImage (src, title);
    delete src;
  }
}
