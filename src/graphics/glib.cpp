#include "glib.h"
#include "image.h"
#include "base/utils.h"

#pragma comment (lib, "opengl32.lib")

OpenGL::OpenGL(HWND window, uint32 color)
{
  hWnd = window;
  hdc = NULL;
  hrc = NULL;
  font = NULL;
  ok = false;

  RECT rc;
  GetWindowRect(hWnd, &rc);
  width = rc.right - rc.left - 1;
  height = rc.bottom - rc.top - 1;

  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof (PIXELFORMATDESCRIPTOR),
    1,                                     // version number
    PFD_DRAW_TO_WINDOW |                   // format must support Window
    PFD_SUPPORT_OPENGL |                   // format must support OpenGL
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
  font = CreateFont(ht, 0, 0, 0, FW_LIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Georgia");
  SelectObject(hdc, font);
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
OpenGL::~OpenGL()
{
  delete[] lgen;
  if (hrc != NULL)
    wglDeleteContext(hrc);
  if (font != NULL)
    DeleteObject(font);
  if (hdc != NULL)
    ReleaseDC(hWnd, hdc);
}

void OpenGL::begin()
{
  wglMakeCurrent(hdc, hrc);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glColor3d(1, 1, 1);
}
void OpenGL::end()
{
  glFlush();
  SwapBuffers(hdc);
  wglMakeCurrent(NULL, NULL);
}

int OpenGL::getTextWidth(char const* str)
{
  SIZE sz;
  GetTextExtentPoint32(hdc, str, strlen(str), &sz);
  return sz.cx;
}
int OpenGL::getTextHeight(char const* str)
{
  SIZE sz;
  GetTextExtentPoint32(hdc, str, strlen(str), &sz);
  return sz.cy;
}

void OpenGL::text(int x, int y, char const* str, int mode)
{
  int length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, NULL, 0);
  wchar_t* wide = new wchar_t[length + 5];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wide, length + 5);
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
  glRasterPos2i(x, y);
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
void OpenGL::image(int x, int y, Image const* img, int mode)
{
  image(x, y, img, 0, 0, img ? img->width() : 0, img ? img->height() : 0, mode);
}
void OpenGL::image(int x, int y, Image const* img, int x0, int y0, int sx, int sy, int mode)
{
  switch (mode & ALIGN_X)
  {
  case ALIGN_X_LEFT:
    break;
  case ALIGN_X_RIGHT:
    x -= sx;
    break;
  case ALIGN_X_CENTER:
    x -= sx / 2;
    break;
  }
  switch (mode & ALIGN_Y)
  {
  case ALIGN_Y_TOP:
    break;
  case ALIGN_Y_BOTTOM:
    y -= sy;
    break;
  case ALIGN_Y_CENTER:
    y -= sy / 2;
    break;
  }
  if (img)
  {
    if (x < 0)
    {
      x0 -= x;
      sx += x;
      x = 0;
    }
    if (x + sx > width)
      sx = width - x;
    if (y < 0)
    {
      y0 -= y;
      sy += y;
      y = 0;
    }
    if (y + sy > height)
      sy = height - y;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, img->width());
    glRasterPos2i(x, y);
    glPixelZoom(1, -1);
    glDrawPixels(sx, sy, GL_BGRA_EXT, GL_UNSIGNED_BYTE, img->bits() + x0 + y0 * img->width());
    glPixelZoom(1, 1);
  }
  else
  {
    fillrect(x, y, x + sx, y + sy);
    //glRasterPos2i(x + x0, y - y0);
    //double clr[4];
    //glGetDoublev(GL_CURRENT_COLOR, clr);
    //int cli[3];
    //for (int i = 0; i < 3; i++)
    //{
    //  cli[i] = int (clr[i] * 255 + 0.5);
    //  if (cli[i] > 255) cli[i] = 255;
    //  if (cli[i] < 0) cli[i] = 0;
    //}
    //for (int i = 0; i < sx * sy; i++)
    //{
    //  rectBuf[i * 3 + 0] = cli[0];
    //  rectBuf[i * 3 + 1] = cli[1];
    //  rectBuf[i * 3 + 2] = cli[2];
    //}
    //glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    //glRasterPos2i (x + x0, y + sy - y0);
    //glDrawPixels (sx, sy, GL_RGB, GL_UNSIGNED_BYTE, rectBuf);
  }
}

void OpenGL::color(uint32 color)
{
  glColor4d(double(color & 0xFF) / 255,
            double((color >> 8) & 0xFF) / 255,
            double((color >> 16) & 0xFF) / 255,
            double((color >> 24) & 0xFF) / 255);
}
void OpenGL::color(uint32 color, int alpha)
{
  glColor4d(double(color & 0xFF) / 255,
            double((color >> 8) & 0xFF) / 255,
            double((color >> 16) & 0xFF) / 255,
            double(alpha) / 255);
}
void OpenGL::line(int x0, int y0, int x1, int y1)
{
  glBegin(GL_LINES);
  glVertex2i(x0, y0);
  glVertex2i(x1, y1);
  glEnd();
}
void OpenGL::rect(int x0, int y0, int x1, int y1)
{
  glBegin(GL_LINE_LOOP);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
}
void OpenGL::fillrect(int x0, int y0, int x1, int y1)
{
  glBegin(GL_POLYGON);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
}
void OpenGL::point(int x, int y)
{
  glBegin(GL_POINTS);
  glVertex2i(x, y);
  glEnd();
}

void OpenGL::onSize(int cx, int cy)
{
  width = cx - 1;
  height = cy - 1;
}

uint32 OpenGL::genTexture(Image const* img)
{
  if (img == NULL)
    return 0;

  uint32 id;
  wglMakeCurrent(hdc, hrc);
  glGenTextures(1, (GLuint*) &id);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0, 4, img->width(), img->height(), 0, GL_BGRA_EXT,
    GL_UNSIGNED_BYTE, img->bits());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  wglMakeCurrent(NULL, NULL);
  return id;
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

//////////////////////////////////

void DCPaint::setColor(uint32 color)
{
  cclr = color;
  for (int i = 0; i < numPens; i++)
  {
    if (penColor[i] == color && penDash[i] == cdash)
    {
      SelectObject(dc, pens[i]);
      return;
    }
  }
  int index = 0;
  if (numPens < 255)
    index = numPens++;
  else
    DeleteObject(pens[index]);
  pens[index] = CreatePen(cdash ? PS_DASH : PS_SOLID, 1, color);
  penDash[index] = cdash;
  penColor[index] = color;
  SelectObject(dc, pens[index]);
}
void DCPaint::text(int x, int y, char const* str, int mode)
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
  DrawText(dc, str, len, &rc, flags);
}
DCPaint::DCPaint(HDC hdc)
{
  dc = hdc;
  numPens = 0;
  cdash = false;

  int ht = -MulDiv(8, GetDeviceCaps(dc, LOGPIXELSY), 72);
  font = CreateFont(ht, 0, 0, 0, FW_LIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Georgia");
  SelectObject(dc, font);
  SetBkMode(dc, TRANSPARENT);
}
DCPaint::~DCPaint()
{
  DeleteObject(font);
  for (int i = 0; i < numPens; i++)
    DeleteObject(pens[i]);
}
