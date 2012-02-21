#ifndef __FRAMEUI_STATIC_H__
#define __FRAMEUI_STATIC_H__

#include <windows.h>

#include "base/types.h"
#include "base/string.h"
#include "graphics/image.h"
#include "frameui/region.h"

class StaticRegion : public Region
{
public:
  StaticRegion(Region* parent)
    : Region(parent)
  {}
  virtual void render(HDC hDC) = NULL;
};

class FontStringRegion : public StaticRegion
{
  HFONT hFont;
  String text;
  int _flags;
  uint32 _color;
public:
  FontStringRegion(Region* parent);
  FontStringRegion(Region* parent, HFONT font);
  FontStringRegion(Region* parent, String text);
  FontStringRegion(Region* parent, String text, HFONT font);
  void setFont(HFONT font);
  void setFont(String font);
  void setText(String text);
  String getText() const
  {
    return text;
  }

  void resetSize();

  void setColor(uint32 color);
  void setFlags(int flags);
  uint32 color() const
  {
    return _color;
  }
  int flags() const
  {
    return _flags;
  }

  void render(HDC hDC);
};

class TextureRegion : public StaticRegion
{
  Image* image;
  HDC hDC;
  HBITMAP hBitmap;
  uint32* bits;
  uint32 _color;
  uint32 _background;
  bool redraw;
  Point bmSize;
public:
  TextureRegion(Region* parent);
  TextureRegion(Region* parent, Image* texture);
  TextureRegion(Region* parent, uint32 color);
  ~TextureRegion();

  void resetSize();

  void setTexture(Image* texture);
  void setTexture(uint32 color);
  void setColor(uint32 color);
  void setBackground(uint32 _background);
  uint32 color() const
  {
    return _color;
  }
  uint32 background() const
  {
    return _background;
  }

  void render(HDC hDC);
};

#endif // __FRAMEUI_STATIC_H__
