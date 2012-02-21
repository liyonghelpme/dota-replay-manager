#ifndef __FRAMEUI_FRAME_H__
#define __FRAMEUI_FRAME_H__

#include <windows.h>

#include "frameui/region.h"
#include "frameui/static.h"

#define PT_TOPLEFT        0x00
#define PT_TOP            0x01
#define PT_TOPRIGHT       0x02
#define PT_LEFT           0x10
#define PT_CENTER         0x11
#define PT_RIGHT          0x12
#define PT_BOTTOMLEFT     0x20
#define PT_BOTTOM         0x21
#define PT_BOTTOMRIGHT    0x22

#include "event.h"

class Frame : public Region
{
  Frame* firstChild;
  Frame* lastChild;
  Frame* prevSibling;
  Frame* nextSibling;
  StaticRegion** staticRegions;
  int numStaticRegions;
  int maxStaticRegions;
  StaticRegion* addStaticRegion(StaticRegion* r);
  void onChangeVisibility();
public:
  Frame(Frame* parent);
  ~Frame();

  void setParent(Frame* parent);
  Frame* getParent() const
  {
    return (Frame*) parent;
  }

  Frame* getFirstChild() const
  {
    return firstChild;
  }
  Frame* getLastChild() const
  {
    return lastChild;
  }
  Frame* getPrevSibling() const
  {
    return prevSibling;
  }
  Frame* getNextSibling() const
  {
    return nextSibling;
  }

  void show(bool s = true);

  FontStringRegion* createFontString()
  {
    return (FontStringRegion*) addStaticRegion(new FontStringRegion(this));
  }
  FontStringRegion* createFontString(HFONT font)
  {
    return (FontStringRegion*) addStaticRegion(new FontStringRegion(this, font));
  }
  FontStringRegion* createFontString(String text)
  {
    return (FontStringRegion*) addStaticRegion(new FontStringRegion(this, text));
  }
  FontStringRegion* createFontString(String text, HFONT font)
  {
    return (FontStringRegion*) addStaticRegion(new FontStringRegion(this, text, font));
  }
  TextureRegion* createTexture()
  {
    return (TextureRegion*) addStaticRegion(new TextureRegion(this));
  }
  TextureRegion* createTexture(Image* texture)
  {
    return (TextureRegion*) addStaticRegion(new TextureRegion(this, texture));
  }
  TextureRegion* createTexture(uint32 color)
  {
    return (TextureRegion*) addStaticRegion(new TextureRegion(this, color));
  }

  virtual void render(HDC hDC);
};

#endif // __FRAMEUI_FRAME_H__
