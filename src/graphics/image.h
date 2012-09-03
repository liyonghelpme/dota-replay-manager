#ifndef __GRAPHICS_IMAGE_H__
#define __GRAPHICS_IMAGE_H__

#include <windows.h>

#include "base/intdict.h"
#include "graphics/rect.h"

class File;
struct BLTInfo;

class Image
{
  static uint8 mtable[65536];
  static bool gtable;
  static void initTable();

  int _width;
  int _height;
  uint32* _bits;
  uint32 _flags;
  enum {_premult = 0x01000000,
        _unowned = 0x02000000,
        _bgcolor = 0x00FFFFFF
  };
  void updateAlpha();

  bool loadPNG(File* file);
  bool loadGIF(File* file);
  bool loadTGA(File* file);
  bool loadBIN(File* file);
  bool loadBLP(File* file);
  bool loadBLP2(File* file);
  bool load(File* file);

  Rect clipRect;

public:
  Image(int width, int height);
  Image(char const* filename);
  Image(File* file);
  Image(int width, int height, uint32* bits);
  ~Image();

  void setAlpha(Image* image);

  void setBackground(uint32 color)
  {
    _flags = (_flags & (~_bgcolor)) | (color & _bgcolor);
  }

  void writePNG(File* file);
  void writeBIN(File* file);

  uint32* bits()
  {
    return _bits;
  }
  uint32 const* bits() const
  {
    return _bits;
  }
  uint32* bitsAlpha() const;

  void blt(int x, int y, Image const* src);
  void blt(int x, int y, Image const* src, int srcX, int srcY, int srcW, int srcH);

  void blt(BLTInfo& info);

  void fill(uint32 color);
  void fill(uint32 color, int x, int y, int width, int height);

  int width() const
  {
    return _width;
  }
  int height() const
  {
    return _height;
  }

  static uint32 clr(uint8 r, uint8 g, uint8 b)
  {
    return 0xFF000000 | (r << 16) | (g << 8) | b;
  }
  static uint32 clr(uint8 r, uint8 g, uint8 b, uint8 a)
  {
    uint8* alpha = mtable + uint32(255 - a) * 256;
    return (a << 24) | (alpha[r] << 16) | (alpha[g] << 8) | alpha[b];
  }
  static uint32 clr(uint32 rgb)
  {
    return 0xFF000000 | ((rgb & 0x00FF0000) >> 16) | (rgb & 0x0000FF00) | ((rgb & 0x000000FF) << 16);
  }
  static uint32 clr(uint32 rgb, uint8 a)
  {
    uint8* alpha = mtable + uint32(255 - a) * 256;
    return alpha[(rgb >> 16) & 0xFF] |
          (alpha[(rgb >> 8) & 0xFF] << 8) |
          (alpha[rgb & 0xFF] << 16) |
          (a << 24);
  }
  static uint32 clr_noflip(uint32 rgb, uint8 a)
  {
    uint8* alpha = mtable + uint32(255 - a) * 256;
    return alpha[rgb & 0xFF] |
          (alpha[(rgb >> 8) & 0xFF] << 8) |
          (alpha[(rgb >> 16) & 0xFF] << 16) |
          (a << 24);
  }
  static uint32 clr_rgba(uint32 rgba)
  {
    uint8* alpha = mtable + (255 - ((rgba >> 24) & 0xFF)) * 256;
    return alpha[(rgba >> 16) & 0xFF] |
          (alpha[(rgba >> 8) & 0xFF] << 8) |
          (alpha[rgba & 0xFF] << 16) |
          (rgba & 0xFF000000);
  }
  static uint32 clr_rgba_noflip(uint32 rgba)
  {
    uint8* alpha = mtable + (255 - ((rgba >> 24) & 0xFF)) * 256;
    return alpha[rgba & 0xFF] |
          (alpha[(rgba >> 8) & 0xFF] << 8) |
          (alpha[(rgba >> 16) & 0xFF] << 16) |
          (rgba & 0xFF000000);
  }

  static uint8 brightness(int r, int g, int b)
  {
    int mn = r, mx = r;
    if (g < r) mn = r; else mx = r;
    if (b < mn) mn = b;
    if (b > mx) mx = b;
    return (mn + mx) / 2;
  }

  void desaturate();

  void sharpen(float coeff);
  void modBrightness(float coeff);

  uint32 getPixel(int x, int y)
  {
    return _bits[x + y * _width];
  }
  void setPixel(int x, int y, uint32 color)
  {
    _bits[x + y * _width] = color;
  }
  void replaceColor(uint32 color, uint32 with);

  bool getRect(int& left, int& top, int& right, int& bottom) const;

  HBITMAP createBitmap(HDC hDC = NULL);
  HBITMAP createAlphaBitmap(HDC hDC = NULL);
  void fillBitmap(HBITMAP hBitmap, HDC hDC);
  static Image* fromBitmap(HBITMAP hBitmap);
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
  uint32 modulate;

  enum {Blend, Add};

  BLTInfo()
  {
    reset(NULL, 0, 0);
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
    srcW = (img ? img->width() : 0);
    srcH = (img ? img->height() : 0);
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

#endif // __GRAPHICS_IMAGE_H__
