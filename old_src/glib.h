#ifndef __G_LIB_H__
#define __G_LIB_H__

#include <gl/gl.h>


class Color
{
public:
   double r, g, b, a;
   Color (DWORD rgb, double A = 1)
   {
     r = double (rgb & 0xFF) / 255.0;
     g = double ((rgb >> 8) & 0xFF) / 255.0;
     b = double ((rgb >> 16) & 0xFF) / 255.0;
     a = A;
   }
   Color (double R, double G, double B, double A = 1)
   {
      r = R;
      g = G;
      b = B;
     a = A;
   }
   Color ()
   {}
   Color& set (double R, double G, double B, double A = 1)
   {
      r = R;
      g = G;
      b = B;
      a = A;
      return *this;
   }

   void clamp (double x = 0, double y = 1)
   {
      if (r < x) r = x;
      if (r > y) r = y;
      if (g < x) g = x;
      if (g > y) g = y;
      if (b < x) b = x;
      if (b > y) b = y;
      if (a < x) a = x;
      if (a > y) a = y;
   }

   COLORREF getColor () const
   {
     int R = int (r * 255.0);
     int G = int (g * 255.0);
     int B = int (b * 255.0);
     if (R > 255) R = 255;
     if (R < 0) R = 0;
     if (G > 255) G = 255;
     if (G < 0) G = 0;
     if (B > 255) B = 255;
     if (B < 0) B = 0;
     return RGB (R, G, B);
   }
};

void glColor (Color const& color);

void glLine (int x0, int y0, int x1, int y1);
void glRect (int x0, int y0, int x1, int y1);
void glFillRect (int x0, int y0, int x1, int y1);
void glPoint (int x, int y);

struct GLImage;

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
  CFont font;
  uint32* lgen;
public:
  int wd, ht;

  OpenGL (HWND window, Color clr = Color (0, 0, 0));
  ~OpenGL ();

  bool isOk () const
  {
    return ok;
  }

  void begin ();
  void end ();

  int getTextWidth (char const* str);
  int getTextHeight (char const* str);
  int getTextWidth (wchar_t const* str);
  int getTextHeight (wchar_t const* str);

  void drawText (int x, int y, char const* str, int mode = 0);
  void drawText (int x, int y, wchar_t const* str, int mode = 0);
  void drawRect (int x, int y, GLImage const& img, int mode = 0);
  void drawRect (int x, int y, GLImage const& img, int x0, int y0, int sx, int sy, int mode = 0);

  void onSize (int cx, int cy);
};

struct EnumStruct
{
  int val;
  int id;
  int base;
  int sub;
};
void enumCount (EnumStruct& e);
void enumTime (EnumStruct& e);
void nextCount (EnumStruct& e);
void nextTime (EnumStruct& e);

class DCPaint
{
  CPen pens[256];
  COLORREF colors[256];
  bool dashed[256];
  int numPens;
  CDC* dc;
  bool cdash;
  COLORREF cclr;
  CFont font;
public:
  DCPaint (CDC* hdc);
  void setDash (bool dash)
  {
    cdash = dash;
    setColor (cclr);
  }
  void setColor (double r, double g, double b)
  {
    setColor (Color (r, g, b).getColor ());
  }
  void setColor (COLORREF clr);
  void line (int x0, int y0, int x1, int y1)
  {
    dc->MoveTo (x0, y0);
    dc->LineTo (x1, y1);
  }
  void drawText (int x, int y, char const* str, int mode = 0);
  void drawText (int x, int y, wchar_t const* str, int mode = 0);
};

#endif // __G_LIB_H__
