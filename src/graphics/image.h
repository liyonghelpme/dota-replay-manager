#ifndef __GRAPHICS_IMAGE_H__
#define __GRAPHICS_IMAGE_H__

#include <windows.h>

#include "base/intdict.h"
#include "graphics/rect.h"

class File;
struct BLTInfo;

class Image
{
  static unsigned char mtable[65536];
  static bool gtable;
  static void initTable();

  int _width;
  int _height;
  uint32* _bits;
  enum {_opaque, _alpha, _premult};
  int mode;
  bool unowned;

  bool loadPNG (File* file);
  bool loadGIF (File* file);
  bool loadTGA (File* file);
  bool loadBIN (File* file);
  bool loadBLP (File* file);
  bool loadBLP2 (File* file);
  bool load (File* file);

  static unsigned long flip_color(unsigned long clr)
  {
    return (clr & 0xFF00FF00) | ((clr >> 16) & 0x0000FF) | ((clr << 16) & 0xFF0000);
  }

  Rect clipRect;

  void make_premult();

  RectUpdater* changes;
  //CDC* srcDC;
  //CBitmap* bmp;
public:
  Image(int width, int height);
  Image(char const* filename);
  Image(File* file);
  Image(int width, int height, uint32* bits);
  ~Image();

  void writePNG(File* file);
  void writeBIN(File* file);

  uint32* bits()
  {
    return _bits;
  }

  void blt(int x, int y, Image const* src);
  void blt(int x, int y, Image const* src, int srcX, int srcY, int srcW, int srcH);

  void blt(BLTInfo& info);

  void fill(unsigned int color);
  void fill(unsigned int color, int x, int y, int width, int height);

  int width() const
  {
    return _width;
  }
  int height() const
  {
    return _height;
  }

  static unsigned int clr(int r, int g, int b, int a = 255)
  {
    return (((unsigned char) a) << 24) | (((unsigned char) r) << 16) | (((unsigned char) g) << 8) | ((unsigned char) b);
  }
  static unsigned int clr(int c, int a = 255)
  {
    return (((unsigned char) a) << 24) | (flip_color (c) & 0x00FFFFFF);
  }
  static unsigned char brightness(int r, int g, int b)
  {
    int mn = r, mx = r;
    if (g < r) mn = r; else mx = r;
    if (b < mn) mn = b;
    if (b > mx) mx = b;
    return (mn + mx) / 2;
  }

  //void setClipRect (CDC* dc, int x, int y);
  void setClipRect(Rect const& rc)
  {
    clipRect = rc;
  }
  void resetClipRect()
  {
    clipRect.left = 0;
    clipRect.top = 0;
    clipRect.right = _width;
    clipRect.bottom = _height;
  }

  void desaturate();

  void sharpen(float coeff);
  void modBrightness(float coeff);

  bool getRect(int& left, int& top, int& right, int& bottom) const;

  void setPixel(int x, int y, unsigned int color)
  {
    _bits[x + y * _width] = color;
  }

  void notifyChanges(RectUpdater* rc)
  {
    changes = rc;
  }

  HBITMAP createBitmap(HDC hDC = NULL);
  void fillBitmap(HBITMAP hBitmap, HDC hDC);

  //void eraseBkgnd (CDC* dc, int x, int y, unsigned long color);
  //void eraseBkgnd (CDC* dc, int x, int y, CRect const& rc, unsigned long color);
  //void render (CDC* dc, int x, int y, bool opaque = true);
  //void fillBitmap (CBitmap* bmp, CDC* pDC);
};

struct BLTInfo
{
  Image const* src;
  int x, y;
  int srcX, srcY;
  int srcW, srcH;
  bool flipX, flipY;
  int dstW, dstH;
  bool desaturate;
  int alphaMode;
  unsigned long modulate;

  enum {Blend, Add};

  BLTInfo()
  {
    x = 0;
    y = 0;
    src = NULL;
    srcX = 0;
    srcY = 0;
    srcW = 0;
    srcH = 0;
    flipX = false;
    flipY = false;
    dstW = 0;
    dstH = 0;
    desaturate = false;
    alphaMode = Blend;
    modulate = 0xFFFFFFFF;
  }
  BLTInfo(Image const* img, int _x = 0, int _y = 0)
  {
    reset(img, _x, _y);
  }
  void reset(Image const* img, int _x = 0, int _y = 0)
  {
    x = _x;
    y = _y;
    src = img;
    srcX = 0;
    srcY = 0;
    srcW = img->width();
    srcH = img->height();
    flipX = false;
    flipY = false;
    dstW = srcW;
    dstH = srcH;
    desaturate = false;
    alphaMode = Blend;
    modulate = 0xFFFFFFFF;
  }
  void setDstSize(int w, int h)
  {
    dstW = w;
    dstH = h;
  }
  void setSrcRect(int x, int y, int w, int h)
  {
    srcX = x;
    srcY = y;
    srcW = w;
    srcH = h;
  }
  void setColor(int r, int g, int b)
  {
    modulate = ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
  }
  void setColor(float r, float g, float b)
  {
    int ri = int (r * 255.0f + 0.5f);
    int gi = int (g * 255.0f + 0.5f);
    int bi = int (b * 255.0f + 0.5f);
    if (ri < 0) ri = 0; if (ri > 255) ri = 255;
    if (gi < 0) gi = 0; if (gi > 255) gi = 255;
    if (bi < 0) bi = 0; if (bi > 255) bi = 255;
    setColor(ri, gi, bi);
  }
};

//class CImageList;
//class Icon;
//
//class ImageList
//{
//  CImageList* list;
//  IntDictionary images;
//  Image4D* img;
//  CBitmap* bmp;
//  int width;
//  int height;
//  int background;
//public:
//  ImageList (int width, int height, int background = 0xFFFFFFFF);
//  ~ImageList ();
//
//  void reset ();
//  int getImagePos (Image4D* img);
//  int getIconPos (Icon* icon, int img = 0);
//  int getBlankPos ();
//  CImageList* getList () const
//  {
//    return list;
//  }
//};

#endif // __GRAPHICS_IMAGE_H__
