#include "stdafx.h"
#include "glib.h"
#include "ilib.h"
#include <math.h>

#pragma comment (lib, "opengl32.lib")

void glColor (Color const& color)
{
  glColor4d (color.r, color.g, color.b, color.a);
}

void glLine (int x0, int y0, int x1, int y1)
{
  glBegin (GL_LINES);
  glVertex2i (x0, y0);
  glVertex2i (x1, y1);
  glEnd ();
}

void glPoint (int x, int y)
{
  glBegin (GL_POINTS);
  glVertex2i (x, y);
  glEnd ();
}

void glRect (int x0, int y0, int x1, int y1)
{
  glBegin (GL_LINE_LOOP);
  glVertex2i (x0, y0);
  glVertex2i (x1, y0);
  glVertex2i (x1, y1);
  glVertex2i (x0, y1);
  glEnd ();
}
void glFillRect (int x0, int y0, int x1, int y1)
{
  glBegin (GL_POLYGON);
  glVertex2i (x0, y0);
  glVertex2i (x1, y0);
  glVertex2i (x1, y1);
  glVertex2i (x0, y1);
  glEnd ();
}

OpenGL::OpenGL (HWND window, Color clr)
{
  hWnd = window;
  hdc = NULL;
  hrc = NULL;
  ok = false;

  RECT rc;
  GetWindowRect (hWnd, &rc);
  wd = rc.right - rc.left - 1;
  ht = rc.bottom - rc.top - 1;

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
  hdc = GetDC (hWnd);
  pixelFormat = ChoosePixelFormat (hdc, &pfd);

  if (pixelFormat == 0)
    return;
  if (SetPixelFormat (hdc, pixelFormat, &pfd) == FALSE)
    return;

  hrc = wglCreateContext (hdc);
  if (hrc == NULL)
    return;
  if (wglMakeCurrent (hdc, hrc) == FALSE)
    return;

  glClearColor    ((float) clr.r, (float) clr.g, (float) clr.b, 0);
  glClearDepth    (1);  
  glClearStencil  (0);
  glHint          (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glEnable        (GL_TEXTURE_2D);
  glDisable       (GL_DEPTH_TEST);
  glEnable        (GL_BLEND);
  glDepthFunc     (GL_LEQUAL);
  glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelStorei   (GL_PACK_ALIGNMENT, 1);
  glPixelStorei   (GL_UNPACK_ALIGNMENT, 1);

  int ht = -MulDiv (10, GetDeviceCaps (hdc, LOGPIXELSY), 72);
  font.CreateFont (ht, 0, 0, 0, FW_LIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Georgia");
  SelectObject (hdc, font.m_hObject);
  if (wglUseFontBitmaps (hdc, 0, 256, tBase) == FALSE)
    return;
  lgen = new uint32[65536 / 32];
  memset(lgen, 0, (65536 / 32) * sizeof(uint32));
  for (int i = 0; i < 256 / 32; i++)
    lgen[i] = 0xFFFFFFFF;
  SetBkMode(hdc, TRANSPARENT);

  wglMakeCurrent(NULL, NULL);

  ok = true;
}

OpenGL::~OpenGL ()
{
  delete[] lgen;
  if (hrc != NULL)
    wglDeleteContext (hrc);
  if (hdc != NULL)
    ReleaseDC (hWnd, hdc);
}

int OpenGL::getTextWidth (char const* str)
{
  CSize sz;
  GetTextExtentPoint32 (hdc, str, (int) strlen (str), &sz);
  return sz.cx;
}
int OpenGL::getTextHeight (char const* str)
{
  CSize sz;
  GetTextExtentPoint32 (hdc, str, (int) strlen (str), &sz);
  return sz.cy;
}
int OpenGL::getTextWidth (wchar_t const* str)
{
  CSize sz;
  GetTextExtentPoint32W (hdc, str, (int) wcslen (str), &sz);
  return sz.cx;
}
int OpenGL::getTextHeight (wchar_t const* str)
{
  CSize sz;
  GetTextExtentPoint32W (hdc, str, (int) wcslen (str), &sz);
  return sz.cy;
}

void OpenGL::drawText (int x, int y, char const* str, int mode)
{
  int len = (int) strlen (str);
  CSize sz;
  GetTextExtentPoint32 (hdc, str, len, &sz);
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
  glRasterPos2i (x, y);
  glListBase (tBase);
  glCallLists (len, GL_UNSIGNED_BYTE, str);
}
void OpenGL::drawText (int x, int y, wchar_t const* str, int mode)
{
  int len = (int) wcslen (str);
  CSize sz;
  GetTextExtentPoint32W (hdc, str, len, &sz);
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
  glRasterPos2i (x, y);
  glListBase (tBase);
  for (int i = 0; str[i]; i++)
  {
    int c = (unsigned short) str[i];
    if (!lgen[c])
    {
      wglUseFontBitmapsW (hdc, c, 1, tBase + c);
      lgen[c] = true;
    }
  }
  glCallLists (len, GL_UNSIGNED_SHORT, str);
}
void OpenGL::drawRect (int x, int y, GLImage const& img, int mode)
{
  drawRect (x, y, img, 0, 0, img.width, img.height, mode);
}
static unsigned char rectBuf[4096];
void OpenGL::drawRect (int x, int y, GLImage const& img, int x0, int y0, int sx, int sy, int mode)
{
  y0 = img.height - y0 - sy;
  x -= x0;
  y += y0;
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
  if (img.bits)
  {
    y += sy;
    if (x < 0)
    {
      x0 -= x;
      sx += x;
    }
    if (x + x0 + sx > wd)
      sx = wd - x0 - x;
    if (y >= ht)
    {
      y0 += y - ht + 1;
      sx -= y - ht + 1;
    }
    if (y - y0 - sy < -1)
      sy = y - y0 + 1;
    glPixelStorei (GL_UNPACK_ROW_LENGTH, img.width);
    glRasterPos2i (x + x0, y - y0);
    glDrawPixels (sx, sy, GL_RGB, GL_UNSIGNED_BYTE, img.bits + x0 * 3 + y0 * img.width * 3);
  }
  else
  {
    glRasterPos2i (x + x0, y - y0);
    double clr[4];
    glGetDoublev (GL_CURRENT_COLOR, clr);
    int cli[3];
    for (int i = 0; i < 3; i++)
    {
      cli[i] = int (clr[i] * 255 + 0.5);
      if (cli[i] > 255) cli[i] = 255;
      if (cli[i] < 0) cli[i] = 0;
    }
    for (int i = 0; i < sx * sy; i++)
    {
      rectBuf[i * 3 + 0] = cli[0];
      rectBuf[i * 3 + 1] = cli[1];
      rectBuf[i * 3 + 2] = cli[2];
    }
    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glRasterPos2i (x + x0, y + sy - y0);
    glDrawPixels (sx, sy, GL_RGB, GL_UNSIGNED_BYTE, rectBuf);
  }
}

void OpenGL::onSize (int cx, int cy)
{
  wd = cx - 1;
  ht = cy - 1;
}
void OpenGL::begin ()
{
  wglMakeCurrent (hdc, hrc);
  glViewport (0, 0, wd + 1, ht + 1);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, wd, ht, 0, -1, 1);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glColor3d (1, 1, 1);
}

void OpenGL::end ()
{
  glFlush();
  SwapBuffers (hdc);
  wglMakeCurrent (NULL, NULL);
}

void enumCount (EnumStruct& e)
{
  e.val = 1;
  e.id = 0;
  e.base = 1;
}
void enumTime (EnumStruct& e)
{
  e.val = 1000;
  e.id = 0;
  e.base = 1000;
  e.sub = 1000;
}
void nextCount (EnumStruct& e)
{
  switch (e.id)
  {
  case 0:
    e.val = e.base * 2;
    e.id = 1;
    break;
  case 1:
    e.val = e.base * 5;
    e.id = 2;
    break;
  case 2:
    e.base *= 10;
    e.val = e.base;
    e.id = 0;
    break;
  }
  e.sub = e.val / 5;
}
void nextTime (EnumStruct& e)
{
  switch (e.id)
  {
  case 0:
    e.val = e.base * 2;
    e.id = 1;
    break;
  case 1:
    e.val = e.base * 5;
    e.id = 2;
    break;
  case 2:
    e.val = e.base * 10;
    e.id = 3;
    break;
  case 3:
    e.val = e.base * 30;
    e.id = 4;
    break;
  case 4:
    e.base *= 60;
    e.val = e.base;
    e.id = 0;
    break;
  }
  e.sub = e.val / 5;
}

void DCPaint::setColor (COLORREF clr)
{
  cclr = clr;
  for (int i = 0; i < numPens; i++)
  {
    if (colors[i] == clr && dashed[i] == cdash)
    {
      dc->SelectObject (&pens[i]);
      return;
    }
  }
  int index = 0;
  if (numPens < 255)
    index = numPens++;
  pens[index].CreatePen (cdash ? PS_DASH : PS_SOLID, 1, clr);
  dashed[index] = cdash;
  colors[index] = clr;
  dc->SelectObject (&pens[index]);
}
void DCPaint::drawText (int x, int y, char const* str, int mode)
{
  int len = (int) strlen (str);
  int flags = DT_SINGLELINE;
  CRect rc;
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
  dc->DrawText (str, len, rc, flags);
}
void DCPaint::drawText (int x, int y, wchar_t const* str, int mode)
{
  int len = (int) wcslen (str);
  int flags = DT_SINGLELINE;
  CRect rc;
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
  DrawTextW (dc->m_hDC, str, len, rc, flags);
}
DCPaint::DCPaint (CDC* hdc)
{
  dc = hdc;
  numPens = 0;
  cdash = false;

  int ht = -MulDiv (8, GetDeviceCaps (dc->m_hDC, LOGPIXELSY), 72);
  font.CreateFont (ht, 0, 0, 0, FW_LIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Georgia");
  dc->SelectObject (&font);
  dc->SetBkMode (TRANSPARENT);
}
