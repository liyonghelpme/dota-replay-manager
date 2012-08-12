#include "stdafx.h"
#include "rmpq.h"
#include "image.h"
#include "utils.h"

struct BLPColor
{
  int r, g, b, a;
  int cnt;
  BLPColor ()
  {
    r = g = b = a = cnt = 0;
  }
};
void ColorAdd (BLPColor& a, uint32 b)
{
  a.r += (b & 0xFF);
  a.g += ((b >> 8) & 0xFF);
  a.b += ((b >> 16) & 0xFF);
  a.a += ((b >> 23) & 0xFF);
  a.cnt++;
}
#define CBRIGHT       7/6
uint32 ColorGetF (BLPColor const& c)
{
  return clamp ((c.b / c.cnt) * CBRIGHT, 0, 255) |
        (clamp ((c.g / c.cnt) * CBRIGHT, 0, 255) << 8) |
        (clamp ((c.r / c.cnt) * CBRIGHT, 0, 255) << 16) |
        (clamp ((c.a / c.cnt) * CBRIGHT, 0, 255) << 24);
}
uint32 ColorGet (BLPColor const& c)
{
  return clamp ((c.r / c.cnt), 0, 255) |
        (clamp ((c.g / c.cnt), 0, 255) << 8) |
        (clamp ((c.b / c.cnt), 0, 255) << 16) |
        (clamp ((c.a / c.cnt), 0, 255) << 24);
}

static TrieNode* icons = NULL;

#define IMAGE_HEIGHT    16
#define IMAGE_WIDTH     32
#define IMAGE_WICON     16

BLPImage* MPQLoadImage (MPQLOADER mpq, char const* opath)
{
  char path[256];
  strcpy (path, opath);
  MPQFILE file = MPQLoadFile (mpq, path);
  if (file == NULL)
  {
    int len = (int) strlen (path);
    if (len >= 4 && path[len - 4] == '.')
    {
      strcpy (path + len - 4, ".blp");
      file = MPQLoadFile (mpq, path);
    }
    else
    {
      strcat (path, ".blp");
      file = MPQLoadFile (mpq, path);
    }
  }
  if (file)
  {
    BLPImage* blp = LoadBLP (file);
    if (blp == NULL)
      blp = LoadTGA (file);
    MPQCloseFile (file);
    return blp;
  }
  return NULL;
}
BLPImage* MPQLoadImageA (MPQARCHIVE mpq, char const* opath)
{
  char path[256];
  strcpy (path, opath);
  MPQFILE file = MPQOpenFile (mpq, path, MPQFILE_READ);
  if (file == NULL)
  {
    int len = (int) strlen (path);
    if (len >= 4 && path[len - 4] == '.')
    {
      strcpy (path + len - 4, ".blp");
      file = MPQOpenFile (mpq, path, MPQFILE_READ);
    }
  }
  if (file)
  {
    BLPImage* blp = LoadBLP (file);
    if (blp == NULL)
      blp = LoadTGA (file);
    MPQCloseFile (file);
    return blp;
  }
  return NULL;
}
int GetImage (CImageList* list, MPQLOADER mpq, char const* path, int def)
{
  if (path[0] == 0) return def;
  int id = getValue (icons, path);
  if (id) return id;
  BLPImage* blp = MPQLoadImage (mpq, path);
  if (blp)
  {
    uint32* out = new uint32[IMAGE_WIDTH * IMAGE_HEIGHT];
    memset (out, 0, IMAGE_WIDTH * IMAGE_HEIGHT * sizeof (uint32));
    int xcoeff = blp->width / IMAGE_WICON;
    int ycoeff = blp->height / IMAGE_HEIGHT;
    int coeff = xcoeff;
    if (ycoeff > coeff)
      coeff = ycoeff;
    if (coeff == 0) coeff = 1;
    int height = blp->height / coeff;
    if (height > IMAGE_HEIGHT) height = IMAGE_HEIGHT;
    int width = blp->width / coeff;
    if (width > IMAGE_WICON) width = IMAGE_WICON;
    int dx = (IMAGE_WICON - width) / 2 + (IMAGE_WIDTH - IMAGE_WICON);
    int dy = (IMAGE_HEIGHT - height) / 2;
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        BLPColor accum;
        for (int y0 = y * coeff, cy = 0; cy < coeff; y0++, cy++)
          for (int x0 = x * coeff, cx = 0; cx < coeff; x0++, cx++)
            ColorAdd (accum, blp->data[x0 + y0 * blp->width]);
        int res = (ColorGetF (accum) & 0x00FFFFFF);
        if (res == 0) res++;
        out[(IMAGE_HEIGHT - (y + dy) - 1) * IMAGE_WIDTH + dx + x] = res;
      }
    }

    CBitmap bmp;
    HDC dc = GetDC (NULL);
    bmp.CreateCompatibleBitmap (CDC::FromHandle (dc), IMAGE_WIDTH, IMAGE_HEIGHT);
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof bi;
    bi.biWidth = IMAGE_WIDTH;
    bi.biHeight = IMAGE_HEIGHT;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 1;
    bi.biYPelsPerMeter = 1;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    SetDIBits (dc, (HBITMAP) bmp.m_hObject, 0, IMAGE_HEIGHT, out, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    int index = list->Add (&bmp, (COLORREF) 0);
    icons = addString (icons, path, index);
    ReleaseDC (NULL, dc);

    delete[] out;
    delete blp;

    return index;
  }
  return def;
}
static BLPImage* mmicon[3] = {NULL, NULL, NULL};
static bool icld = false;
void deleteImages ()
{
  for (int i = 0; i < 3; i++) delete mmicon[i];
  delete icons;
  icons = NULL;
}

void BLPPrepare (BLPImage* img, uint32 bk, int wd, int ht)
{
  uint32* out = new uint32[wd * ht];
  memset (out, 0, wd * ht * sizeof (uint32));
  int xcoeff = img->width / wd;
  int ycoeff = img->height / ht;
  int coeff = xcoeff;
  if (ycoeff > coeff)
    coeff = ycoeff;
  if (coeff == 0) coeff = 1;
  int height = img->height / coeff;
  if (height > ht) height = ht;
  int width = img->width / coeff;
  if (width > wd) width = wd;
  int dx = (wd - width) / 2;
  int dy = (ht - height) / 2;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      BLPColor clr;
      for (int y0 = y * coeff, cy = 0; cy < coeff; y0++, cy++)
        for (int x0 = x * coeff, cx = 0; cx < coeff; x0++, cx++)
          ColorAdd (clr, img->data[x0 + y0 * img->width]);
      clr.r /= clr.cnt;
      clr.g /= clr.cnt;
      clr.b /= clr.cnt;
      clr.a /= clr.cnt;
      clr.r = clamp (clr.r * clr.a / 255, 0, 255);
      clr.g = clamp (clr.g * clr.a / 255, 0, 255);
      clr.b = clamp (clr.b * clr.a / 255, 0, 255);
      out[(y + dy) * wd + dx + x] = RGB (clr.r, clr.g, clr.b);
    }
  }
  delete[] img->data;
  img->data = out;
  img->width = wd;
  img->height = ht;
}

extern char warPath[256];
void BLPAddIcons (BLPImage* img, MPQFILE desc)
{
  if (!icld)
  {
    MPQARCHIVE mpq = MPQOpen (mprintf ("%swar3.mpq", warPath), MPQFILE_READ);
    if (mpq)
    {
      mmicon[0] = MPQLoadImageA (mpq, "UI\\MiniMap\\MiniMapIcon\\MinimapIconGold.blp");
      mmicon[1] = MPQLoadImageA (mpq, "UI\\MiniMap\\MiniMapIcon\\MinimapIconNeutralBuilding.blp");
      mmicon[2] = MPQLoadImageA (mpq, "UI\\MiniMap\\MiniMapIcon\\MinimapIconStartLoc.blp");
      MPQClose (mpq);
    }
    icld = true;
  }
  MPQFileSeek (desc, 4, MPQSEEK_SET);
  uint32 count = MPQReadInt (desc);
  while (count--)
  {
    uint32 type = MPQReadInt (desc);
    uint32 cx = MPQReadInt (desc);
    uint32 cy = MPQReadInt (desc);
    uint32 clr = MPQReadInt (desc);
    if (type >= 0 && type <= 2 && mmicon[type])
    {
      cx = img->width * cx / 256 - mmicon[type]->width / 2;
      cy = img->height * cy / 256 - mmicon[type]->height / 2;
      for (int x0 = 0; x0 < mmicon[type]->width; x0++)
      {
        for (int y0 = 0; y0 < mmicon[type]->height; y0++)
        {
          int x = cx + x0;
          int y = cy + y0;
          if (x < 0 || x >= img->width || y < 0 || y >= img->height)
            continue;
          BLPColor dst, src;
          ColorAdd (dst, img->data[x + y * img->width]);
          ColorAdd (src, mmicon[type]->data[x0 + y0 * mmicon[type]->width]);
          if (type == 2)
          {
            src.r = src.r * ((clr >> 16) & 0xFF) / 255;
            src.g = src.g * ((clr >> 8) & 0xFF) / 255;
            src.b = src.b * (clr & 0xFF) / 255;
            src.a = src.a * ((clr >> 24) & 0xFF) / 255;
          }
          dst.r = (dst.r * (255 - src.a) + src.r * src.a) / 255;
          dst.g = (dst.g * (255 - src.a) + src.g * src.a) / 255;
          dst.b = (dst.b * (255 - src.a) + src.b * src.a) / 255;
          img->data[x + y * img->width] = ColorGet (dst);
        }
      }
    }
  }
}
