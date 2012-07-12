#ifndef __GRAPHICS_GLIB__
#define __GRAPHICS_GLIB__

#include <windows.h>
#include <gl/gl.h>
#include "base/types.h"

class Image;

#define ALIGN_X           0x0F
#define ALIGN_X_LEFT      0x00
#define ALIGN_X_RIGHT     0x01
#define ALIGN_X_CENTER    0x02
#define ALIGN_Y           0xF0
#define ALIGN_Y_TOP       0x00
#define ALIGN_Y_BOTTOM    0x10
#define ALIGN_Y_CENTER    0x20

class OpenGL
{
  enum {tBase = 1000};
  HWND hWnd;
  HDC hdc;
  HGLRC hrc;
  bool ok;
  HFONT font;
  uint32* lgen;
public:
  int width, height;

  OpenGL(HWND window, uint32 color = 0);
  ~OpenGL();

  bool isOk() const
  {
    return ok;
  }

  void begin();
  void end();

  int getTextWidth(char const* str);
  int getTextHeight(char const* str);

  void text(int x, int y, char const* str, int mode = 0);
  void image(int x, int y, Image const* img, int mode = 0);
  void image(int x, int y, Image const* img, int x0, int y0, int sx, int sy, int mode = 0);

  void color(uint32 color);
  void color(uint32 color, int alpha);
  void line(int x0, int y0, int x1, int y1);
  void rect(int x0, int y0, int x1, int y1);
  void fillrect(int x0, int y0, int x1, int y1);
  void point(int x, int y);

  uint32 genTexture(Image const* img);

  void onSize(int cx, int cy);
};

struct EnumStruct
{
  int val;
  int id;
  int base;
  int sub;
};
void enumCount(EnumStruct& e);
void enumTime(EnumStruct& e);
void nextCount(EnumStruct& e);
void nextTime(EnumStruct& e);

class DCPaint
{
  HPEN pens[256];
  uint32 penColor[256];
  bool penDash[256];
  int numPens;
  HDC dc;
  bool cdash;
  uint32 cclr;
  HFONT font;
public:
  DCPaint(HDC hdc);
  ~DCPaint();
  void setDash(bool dash)
  {
    cdash = dash;
    setColor(cclr);
  }
  void setColor(uint32 color);
  void line(int x0, int y0, int x1, int y1)
  {
    MoveToEx(dc, x0, y0, NULL);
    LineTo(dc, x1, y1);
  }
  void text(int x, int y, char const* str, int mode = 0);
};

#endif // __GRAPHICS_GLIB__
