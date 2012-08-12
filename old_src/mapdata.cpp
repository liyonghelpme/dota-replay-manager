#include "stdafx.h"
#include "mapdata.h"
#include "utils.h"
#include "rmpq.h"
#include "image.h"
#include "ilib.h"

extern MPQLOADER warloader;
extern char warPath[];

MapData::MapData (char const* path)
{
  map = MPQOpen (path, MPQFILE_READ);
  images = NULL;
  if (map == 0)
    map = MPQOpen (mprintf ("%s%s", warPath, path), MPQFILE_READ);
  if (map)
  {
    iList.Create (16, 16, ILC_COLOR24, 16, 16);
    MPQAddArchive (warloader, map);
    LoadGameData (data, warloader, WC3_LOAD_UNITS | WC3_LOAD_ITEMS |
      WC3_LOAD_ABILITIES | WC3_LOAD_UPGRADES | WC3_LOAD_MERGED | WC3_LOAD_NO_WEONLY);
    MPQRemoveArchive (warloader, map);

    maxImages = 16;
    numImages = 1;
    images = new imgname[maxImages];
    memset (images, 0, sizeof (imgname) * maxImages);

    CDC dc;
    dc.Attach (GetDC (NULL));

    unsigned char* bm = new unsigned char[16 * 16 * 4];
    memset (bm, 0, 16 * 16 * 4);

    CBitmap bmp;
    bmp.CreateCompatibleBitmap (&dc, 16, 16);
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof bi;
    bi.biWidth = 16;
    bi.biHeight = -16;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 1;
    bi.biYPelsPerMeter = 1;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    SetDIBits (dc.m_hDC, (HBITMAP) bmp.m_hObject, 0, 16, bm, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    iList.Add (&bmp, (CBitmap*) NULL);
    delete[] bm;
  }
}
MapData::~MapData ()
{
  MPQClose (map);
  delete[] images;
}

ObjectData* MapData::getData ()
{
  return data.merged;
}
int MapData::getIconIndex (char const* path)
{
  for (int i = 0; i < numImages; i++)
    if (!stricmp (images[i], path))
      return i;
  MPQAddArchive (warloader, map);
  BLPImage* src = MPQLoadImage (warloader, path);
  MPQRemoveArchive (warloader, map);
  if (src)
  {
    if (numImages >= maxImages)
    {
      maxImages += 16;
      imgname* temp = new imgname[maxImages];
      memset (temp, 0, sizeof (imgname) * maxImages);
      memcpy (temp, images, sizeof (imgname) * numImages);
      delete[] images;
      images = temp;
    }
    strcpy (images[numImages], path);

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
    delete src;

    CDC dc;
    dc.Attach (GetDC (NULL));

    unsigned char* bm = new unsigned char[16 * 16 * 4];
    for (int i = 0; i < 256; i++)
    {
      bm[i * 4 + 0] = cmp.out[2][i];
      bm[i * 4 + 1] = cmp.out[1][i];
      bm[i * 4 + 2] = cmp.out[0][i];
      bm[i * 4 + 3] = 0;
    }

    CBitmap bmp;
    bmp.CreateCompatibleBitmap (&dc, 16, 16);
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof bi;
    bi.biWidth = 16;
    bi.biHeight = -16;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 1;
    bi.biYPelsPerMeter = 1;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    SetDIBits (dc.m_hDC, (HBITMAP) bmp.m_hObject, 0, 16, bm, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    iList.Add (&bmp, (CBitmap*) NULL);
    delete[] bm;
    return numImages++;
  }
  return 0;
}
