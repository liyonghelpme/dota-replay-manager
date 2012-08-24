#ifndef __GRAPHICS_GLIB__
#define __GRAPHICS_GLIB__

#include <windows.h>
#include <gl/gl.h>
#include "base/types.h"
#include "base/dictionary.h"

class Image;

#define ALIGN_X           0x0F
#define ALIGN_X_LEFT      0x00
#define ALIGN_X_RIGHT     0x01
#define ALIGN_X_CENTER    0x02
#define ALIGN_Y           0xF0
#define ALIGN_Y_TOP       0x00
#define ALIGN_Y_BOTTOM    0x10
#define ALIGN_Y_CENTER    0x20

class Painter
{
public:
  virtual bool isOk()
  {
    return true;
  };

  virtual void begin(HDC hDC) = 0;
  virtual void end() = 0;

  virtual void text(int x, int y, char const* str, int mode = 0) = 0;

  virtual void pen(uint32 color, bool dash = false) = 0;
  virtual void color(uint32 color) = 0;
  virtual void line(int x0, int y0, int x1, int y1) = 0;
  virtual void rect(int x0, int y0, int x1, int y1) = 0;
  virtual void fillrect(int x0, int y0, int x1, int y1) = 0;
  virtual void point(int x, int y) = 0;

  virtual void erase(HDC hDC, HWND hWnd, uint32 color)
  {}

  virtual void onSize(int cx, int cy)
  {}
};

class OGLPainter : public Painter
{
  enum {tBase = 1000};
  HWND hWnd;
  HDC hdc;
  HGLRC hrc;
  bool ok;
  HFONT font;
  uint32* lgen;
  bool active;
public:
  int width, height;

  OGLPainter(HWND window, uint32 color = 0, HFONT hFont = NULL);
  ~OGLPainter();

  bool isOk() const
  {
    return ok;
  }

  void begin(HDC hDC);
  void end();

  int getTextWidth(char const* str);
  int getTextHeight(char const* str);

  void text(int x, int y, char const* str, int mode = 0);

  void pen(uint32 color, bool dash = false);
  void color(uint32 color);
  void color(uint32 color, int alpha);
  void line(int x0, int y0, int x1, int y1);
  void rect(int x0, int y0, int x1, int y1);
  void fillrect(int x0, int y0, int x1, int y1);
  void point(int x, int y);

  uint32 genTexture(Image const* img);

  void onSize(int cx, int cy);
};
class GDIPainter : public Painter
{
  HDC hDC;
  HFONT font;

  struct PenInfo
  {
    HPEN hPen;
    uint32 color;
    bool dash;
  };
  Array<PenInfo> pens;

  bool curDash;
  uint32 curColor;
public:
  GDIPainter(HFONT hFont = NULL);
  ~GDIPainter();

  void begin(HDC hDC);
  void end();

  void text(int x, int y, char const* str, int mode = 0);

  void pen(uint32 color, bool dash = false);
  void color(uint32 color);
  void line(int x0, int y0, int x1, int y1);
  void rect(int x0, int y0, int x1, int y1);
  void fillrect(int x0, int y0, int x1, int y1);
  void point(int x, int y);

  void erase(HDC hDC, HWND hWnd, uint32 color);
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

#endif // __GRAPHICS_GLIB__
