#include "glib.h"
#include "image.h"
#include "base/utils.h"

#include "imagelib.h"
#include "core/app.h"
#include "frameui/fontsys.h"

#pragma comment (lib, "opengl32.lib")

OGLPainter::OGLPainter(HWND window, uint32 color, HFONT hFont)
{
  hWnd = window;
  hdc = NULL;
  hrc = NULL;
  font = NULL;
  ok = false;
  active = false;

  RECT rc;
  GetWindowRect(hWnd, &rc);
  width = rc.right - rc.left - 1;
  height = rc.bottom - rc.top - 1;

  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof (PIXELFORMATDESCRIPTOR),
    1,                                     // version number
    PFD_DRAW_TO_WINDOW |                   // format must support Window
    PFD_SUPPORT_OPENGL |                   // format must support OGLPainter
    PFD_DOUBLEBUFFER,                      // must support double buffering
    PFD_TYPE_RGBA,                         // request an RGBA format
    32,                                    // select color depth
    0, 0, 0, 0, 0, 0,                      // color bits
    0,                                     // no alpha buffer
    0,                                     // shift bit ignored
    0,                                     // no accumulation cuffer
    0, 0, 0, 0,                            // accumulation bits ignored
    16,                                    // Z-Buffer depth
    8,                                     // 8-bit stencil buffer
    0,                                     // no auxiliary buffer
    PFD_MAIN_PLANE,                        // main drawing layer
    0,                                     // reserved
    0, 0, 0                                // layer masks ignored
  };

  GLuint pixelFormat;
  hdc = GetDC(hWnd);
  pixelFormat = ChoosePixelFormat(hdc, &pfd);

  if (pixelFormat == 0)
    return;
  if (SetPixelFormat(hdc, pixelFormat, &pfd) == FALSE)
    return;

  hrc = wglCreateContext(hdc);
  if (hrc == NULL)
    return;
  if (wglMakeCurrent(hdc, hrc) == FALSE)
    return;

  glClearColor(float(color & 0xFF) / 255,
               float((color >> 8) & 0xFF) / 255,
               float((color >> 16) & 0xFF) / 255, 0);
  glClearDepth(1);
  glClearStencil(0);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glDepthFunc(GL_LEQUAL);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  int ht = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  if (hFont)
  {
    font = NULL;
    SelectObject(hdc, hFont);
  }
  else
  {
    font = CreateFont(ht, 0, 0, 0, FW_LIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Georgia");
    SelectObject(hdc, font);
  }
  if (wglUseFontBitmaps(hdc, 0, 256, tBase) == FALSE)
    return;
  lgen = new uint32[65536 / 32];
  memset(lgen, 0, (65536 / 32) * sizeof(uint32));
  for (int i = 0; i < 256 / 32; i++)
    lgen[i] = 0xFFFFFFFF;
  SetBkMode(hdc, TRANSPARENT);

  wglMakeCurrent(NULL, NULL);

  ok = true;
}
OGLPainter::~OGLPainter()
{
  delete[] lgen;
  if (hrc != NULL)
    wglDeleteContext(hrc);
  if (font != NULL)
    DeleteObject(font);
  if (hdc != NULL)
    ReleaseDC(hWnd, hdc);
}

void OGLPainter::begin(HDC hDC)
{
  active = true;
  wglMakeCurrent(hdc, hrc);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3d(1, 1, 1);
}
void OGLPainter::end()
{
  glFlush();
  SwapBuffers(hdc);
  wglMakeCurrent(NULL, NULL);
  active = false;
}

int OGLPainter::getTextWidth(char const* str)
{
  SIZE sz;
  GetTextExtentPoint32(hdc, str, strlen(str), &sz);
  return sz.cx;
}
int OGLPainter::getTextHeight(char const* str)
{
  SIZE sz;
  GetTextExtentPoint32(hdc, str, strlen(str), &sz);
  return sz.cy;
}

void OGLPainter::text(int x, int y, char const* str, int mode)
{
  int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  wchar_t* wide = new wchar_t[length];
  MultiByteToWideChar(CP_UTF8, 0, str, -1, wide, length);
  length = wcslen(wide);
  SIZE sz;
  GetTextExtentPoint32W(hdc, wide, length, &sz);
  switch (mode & ALIGN_X)
  {
  case ALIGN_X_LEFT:
    break;
  case ALIGN_X_RIGHT:
    x -= sz.cx;
    break;
  case ALIGN_X_CENTER:
    x -= sz.cx / 2;
    break;
  }
  switch (mode & ALIGN_Y)
  {
  case ALIGN_Y_TOP:
    y += sz.cy / 2;
    break;
  case ALIGN_Y_BOTTOM:
    break;
  case ALIGN_Y_CENTER:
    y += sz.cy / 4;
    break;
  }
  glRasterPos2i(x, y + 2);
  glListBase(tBase);
  for (int i = 0; wide[i]; i++)
  {
    int c = (unsigned short) wide[i];
    if ((lgen[c / 32] & (1 << (c & 31))) == 0)
    {
      wglUseFontBitmapsW(hdc, c, 1, tBase + c);
      lgen[c / 32] |= (1 << (c & 31));
    }
  }
  glCallLists(length, GL_UNSIGNED_SHORT, wide);
}

void OGLPainter::pen(uint32 color, bool dash)
{
  glColor4d(double(color & 0xFF) / 255,
            double((color >> 8) & 0xFF) / 255,
            double((color >> 16) & 0xFF) / 255,
            1.0);
  if (dash)
  {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0F0F);
  }
  else
    glDisable(GL_LINE_STIPPLE);
}
void OGLPainter::color(uint32 color)
{
  glColor4d(double(color & 0xFF) / 255,
            double((color >> 8) & 0xFF) / 255,
            double((color >> 16) & 0xFF) / 255,
            double((color >> 24) & 0xFF) / 255);
}
void OGLPainter::color(uint32 color, int alpha)
{
  glColor4d(double(color & 0xFF) / 255,
            double((color >> 8) & 0xFF) / 255,
            double((color >> 16) & 0xFF) / 255,
            double(alpha) / 255);
}
void OGLPainter::line(int x0, int y0, int x1, int y1)
{
  glBegin(GL_LINES);
  glVertex2i(x0, y0);
  glVertex2i(x1, y1);
  glEnd();
}
void OGLPainter::rect(int x0, int y0, int x1, int y1)
{
  glBegin(GL_LINE_LOOP);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
}
void OGLPainter::fillrect(int x0, int y0, int x1, int y1)
{
  glBegin(GL_POLYGON);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
}
void OGLPainter::point(int x, int y)
{
  glBegin(GL_POINTS);
  glVertex2i(x, y);
  glEnd();
}

void OGLPainter::onSize(int cx, int cy)
{
  width = cx - 1;
  height = cy - 1;
}

uint32 OGLPainter::genTexture(Image const* img)
{
  if (img == NULL)
    return 0;

  uint32 id;
  if (!active)
    wglMakeCurrent(hdc, hrc);
  glGenTextures(1, (GLuint*) &id);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  uint32* bits = img->bitsAlpha();
  glTexImage2D(GL_TEXTURE_2D, 0, 4, img->width(), img->height(), 0, GL_BGRA_EXT,
    GL_UNSIGNED_BYTE, bits);
  delete[] bits;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  if (!active)
    wglMakeCurrent(NULL, NULL);
  return id;
}

///////////////////////////////////////////

GDIPainter::GDIPainter(HFONT hFont)
{
  hDC = NULL;
  curDash = false;
  curColor = 0xFF000000;
  font = hFont;
}
GDIPainter::~GDIPainter()
{
  for (int i = 0; i < pens.length(); i++)
    DeleteObject(pens[i].hPen);
}

void GDIPainter::begin(HDC hDC)
{
  this->hDC = hDC;
  SelectObject(hDC, font ? font : FontSys::getSysFont());
}
void GDIPainter::end()
{
}

void GDIPainter::text(int x, int y, char const* str, int mode)
{
  int len = strlen(str);
  int flags = DT_SINGLELINE;
  RECT rc;
  switch (mode & ALIGN_X)
  {
  case ALIGN_X_LEFT:
    rc.left = x;
    rc.right = x + 500;
    flags |= DT_LEFT;
    break;
  case ALIGN_X_RIGHT:
    rc.left = x - 500;
    rc.right = x;
    flags |= DT_RIGHT;
    break;
  case ALIGN_X_CENTER:
    rc.left = x - 250;
    rc.right = x + 250;
    flags |= DT_CENTER;
    break;
  }
  switch (mode & ALIGN_Y)
  {
  case ALIGN_Y_TOP:
    rc.top = y;
    rc.bottom = y + 500;
    flags |= DT_TOP;
    break;
  case ALIGN_Y_BOTTOM:
    rc.top = y - 500;
    rc.bottom = y;
    flags |= DT_BOTTOM;
    break;
  case ALIGN_Y_CENTER:
    rc.top = y - 250;
    rc.bottom = y + 250;
    flags |= DT_VCENTER;
    break;
  }
  DrawText(hDC, str, len, &rc, flags);
}

void GDIPainter::pen(uint32 color, bool dash)
{
  color &= 0xFFFFFF;
  curDash = dash;
  curColor = color;
  for (int i = 0; i < pens.length(); i++)
  {
    if (pens[i].color == color && pens[i].dash == dash)
    {
      SelectObject(hDC, pens[i].hPen);
      return;
    }
  }
  PenInfo& pi = pens.push();
  pi.hPen = CreatePen(dash ? PS_DASH : PS_SOLID, 0, color);
  pi.color = color;
  pi.dash = dash;
  SelectObject(hDC, pi.hPen);
}
void GDIPainter::color(uint32 color)
{
  color &= 0xFFFFFF;
  curColor = color;
  pen(color, curDash);
}
void GDIPainter::line(int x0, int y0, int x1, int y1)
{
  MoveToEx(hDC, x0, y0, NULL);
  LineTo(hDC, x1, y1);
}
void GDIPainter::rect(int x0, int y0, int x1, int y1)
{
  Rectangle(hDC, x0, y0, x1, y1);
}
void GDIPainter::fillrect(int x0, int y0, int x1, int y1)
{
  SetBkColor(hDC, curColor);
  RECT rc = {x0, y0, x1, y1};
  ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}
void GDIPainter::point(int x, int y)
{
  SetPixel(hDC, x, y, curColor);
}
void GDIPainter::erase(HDC hDC, HWND hWnd, uint32 color)
{
  SetBkColor(hDC, color);
  RECT rc;
  GetClientRect(hWnd, &rc);
  ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}

///////////////////////////////////////////

void enumCount(EnumStruct& e)
{
  e.val = 1;
  e.id = 0;
  e.base = 1;
}
void enumTime(EnumStruct& e)
{
  e.val = 1000;
  e.id = 0;
  e.base = 1000;
}
void nextCount(EnumStruct& e)
{
  static int factor[3] = {2, 5, 10};
  e.val = e.base * factor[e.id++];
  if (e.id >= 3)
  {
    e.base = e.val;
    e.id = 0;
  }
  e.sub = e.val / 5;
}
void nextTime(EnumStruct& e)
{
  static int factor[5] = {2, 5, 10, 30, 60};
  e.val = e.base * factor[e.id++];
  if (e.id >= 5)
  {
    e.base = e.val;
    e.id = 0;
  }
  e.sub = e.val / 5;
}
