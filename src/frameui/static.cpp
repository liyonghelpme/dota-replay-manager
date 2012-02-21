#include "frameui/fontsys.h"

#include "static.h"

FontStringRegion::FontStringRegion(Region* parent)
  : StaticRegion(parent)
{
  hFont = FontSys::getSysFont();
  text = "";
  _flags = 0;
  _color = 0;
}
FontStringRegion::FontStringRegion(Region* parent, HFONT font)
  : StaticRegion(parent)
{
  hFont = font ? font : FontSys::getSysFont();
  text = "";
  _flags = 0;
  _color = 0;
}
FontStringRegion::FontStringRegion(Region* parent, String _text)
  : StaticRegion(parent)
{
  hFont = FontSys::getSysFont();
  text = _text;
  _flags = 0;
  _color = 0;
  resetSize();
}
FontStringRegion::FontStringRegion(Region* parent, String _text, HFONT font)
  : StaticRegion(parent)
{
  hFont = font ? font : FontSys::getSysFont();
  text = _text;
  _flags = 0;
  _color = 0;
  resetSize();
}
void FontStringRegion::setFont(HFONT font)
{
  hFont = font;
  invalidate();
}
void FontStringRegion::setFont(String font)
{
  hFont = FontSys::getFontByName(font);
  invalidate();
}
void FontStringRegion::setText(String _text)
{
  text = _text;
  invalidate();
}

void FontStringRegion::resetSize()
{
  Point pt = FontSys::getTextSize(hFont, text);
  setSize(pt.x, pt.y);
}

void FontStringRegion::setColor(uint32 color)
{
  _color = color;
  invalidate();
}
void FontStringRegion::setFlags(int flags)
{
  _flags = flags;
  invalidate();
}

void FontStringRegion::render(HDC hDC)
{
  SelectObject(hDC, hFont);
  SetTextColor(hDC, _color);
  RECT rc = {left(), top(), right(), bottom()};
  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, text, text.length(), &rc, _flags);
}

//////////////////

TextureRegion::TextureRegion(Region* parent)
  : StaticRegion(parent)
{
  image = NULL;
  _color = 0xFFFFFFFF;
  _background = 0;
  hBitmap = NULL;
  hDC = NULL;
  redraw = true;
}
TextureRegion::TextureRegion(Region* parent, Image* texture)
  : StaticRegion(parent)
{
  image = texture;
  _color = 0xFFFFFFFF;
  _background = 0;
  hBitmap = NULL;
  hDC = NULL;
  redraw = true;
  resetSize();
}
TextureRegion::TextureRegion(Region* parent, uint32 color)
  : StaticRegion(parent)
{
  image = NULL;
  _color = color;
  _background = 0;
  hBitmap = NULL;
  hDC = NULL;
  redraw = true;
}
TextureRegion::~TextureRegion()
{
  DeleteDC(hDC);
}

void TextureRegion::resetSize()
{
  if (image)
    setSize(image->width(), image->height());
}

void TextureRegion::setTexture(Image* texture)
{
  image = texture;
  DeleteObject(hBitmap);
  hBitmap = NULL;
  redraw = true;
  invalidate();
}
void TextureRegion::setTexture(uint32 color)
{
  image = NULL;
  _color = color;
  DeleteObject(hBitmap);
  hBitmap = NULL;
  redraw = true;
  invalidate();
}
void TextureRegion::setColor(uint32 color)
{
  _color = color;
  redraw = true;
  invalidate();
}
void TextureRegion::setBackground(uint32 background)
{
  _background = background;
  redraw = true;
  invalidate();
}

void TextureRegion::render(HDC hDst)
{
  if (width() <= 0 || height() <= 0)
    return;
  if (hDC == NULL)
    hDC = CreateCompatibleDC(NULL);
  if (hBitmap && (bmSize.x != width() || bmSize.y != height()))
  {
    DeleteObject(hBitmap);
    hBitmap = NULL;
    redraw = true;
  }
  if (hBitmap == NULL)
  {
    bmSize.set(width(), height());
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof bi;
    bi.biWidth = bmSize.x;
    bi.biHeight = -bmSize.y;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 1;
    bi.biYPelsPerMeter = 1;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    hBitmap = CreateDIBSection(hDC, (BITMAPINFO*) &bi, DIB_RGB_COLORS, (void**) &bits, NULL, 0);
    SelectObject(hDC, hBitmap);
  }
  if (redraw)
  {
    Image canvas(width(), height());
    if (image)
    {
      canvas.fill(_background);
      BLTInfo info(image);
      info.setDstSize(width(), height());
      info.modulate = _color;
      canvas.blt(info);
    }
    else
      canvas.fill(_color);
    memcpy(bits, canvas.bits(), sizeof(uint32) * canvas.width() * canvas.height());
    redraw = false;
  }
  BitBlt(hDst, left(), top(), width(), height(), hDC, 0, 0, SRCCOPY);
  //if ((_background & 0xFF000000) == 0xFF000000)
  //else
  //{
  //  BLENDFUNCTION bf;
  //  bf.BlendOp = AC_SRC_OVER;
  //  bf.BlendFlags = 0;
  //  bf.SourceConstantAlpha = 255;
  //  bf.AlphaFormat = AC_SRC_ALPHA;
  //  AlphaBlend(hDst, left(), top(), width(), height(), hDC, 0, 0, width(), height(), bf);
  //}
}
