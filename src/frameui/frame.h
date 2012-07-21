#ifndef __FRAMEUI_FRAME_H__
#define __FRAMEUI_FRAME_H__

#include "base/types.h"

#define PT_TOPLEFT        0x00
#define PT_TOP            0x01
#define PT_TOPRIGHT       0x02
#define PT_LEFT           0x10
#define PT_CENTER         0x11
#define PT_RIGHT          0x12
#define PT_BOTTOMLEFT     0x20
#define PT_BOTTOM         0x21
#define PT_BOTTOMRIGHT    0x22

class MasterFrame
{
  friend class Frame;

  Frame** frames;
  int numFrames;
  int maxFrames;
  int moveid;
  bool updating;

  void addFrame(Frame* r);
  void updateFrame(Frame* r);
  void deepUpdateFrame(Frame* r);
  void setPoint(Frame* r, int point, Frame* rel, int relPoint, int x, int y);
  void setWidth(Frame* r, int width);
  void setHeight(Frame* r, int height);
  void removeFrame(Frame* r);
public:
  MasterFrame(Frame* root);
  ~MasterFrame();

  Frame* getRoot() const
  {
    return frames[0];
  }

  void setSize(int width, int height);
};

class Frame
{
  // master frame data
  friend class MasterFrame;
  int mr_pos;
  int mr_moving;
  bool mr_valid;

  bool _visible;
  int _x;
  int _y;
  int _width;
  int _height;
  struct Anchor
  {
    enum {hLeft, hCenter, hRight, hWidth, vTop, vCenter, vBottom, vHeight, count};
    bool active;
    Frame* rel;
    int relPoint;
    int offset;
  } anchors[8];

  MasterFrame* master;
  Frame* parent;
  Frame* firstChild;
  Frame* lastChild;
  Frame* prevSibling;
  Frame* nextSibling;

  void onChangeVisibility();
protected:
  virtual void onMove()
  {
  }
public:
  Frame(Frame* parent);
  virtual ~Frame();

  // hierarchy
  void setParent(Frame* parent);
  Frame* getParent() const
  {
    return parent;
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

  // visibility
  void show(bool s = true);
  void hide()
  {
    show(false);
  }
  bool visible() const
  {
    return _visible && mr_valid && (parent ? parent->visible() : true);
  }

  // positioning
  int left() const
  {
    return _x;
  }
  int top() const
  {
    return _y;
  }
  int right() const
  {
    return _x + _width;
  }
  int bottom() const
  {
    return _y + _height;
  }
  int width() const
  {
    return _width;
  }
  int height() const
  {
    return _height;
  }
  void setWidth(int width)
  {
    master->setWidth(this, width);
  }
  void setHeight(int height)
  {
    master->setHeight(this, height);
  }
  void setSize(int width, int height)
  {
    setWidth(width);
    setHeight(height);
  }
  void setPoint(int point, Frame* rel, int relPoint, int x, int y)
  {
    master->setPoint(this, point, rel, relPoint, x, y);
  }
  void setPoint(int point, Frame* rel, int x, int y)
  {
    setPoint(point, rel, point, x, y);
  }
  void setPoint(int point, int x, int y)
  {
    setPoint(point, parent, point, x, y);
  }
  void clearAllPoints()
  {
    for (int i = 0; i < Anchor::count; i++)
      anchors[i].active = false;
    anchors[Anchor::hWidth].active = true;
    anchors[Anchor::vHeight].active = true;
    master->deepUpdateFrame(this);
  }
  void setAllPoints(Frame* rel)
  {
    setPoint(PT_TOPLEFT, rel, PT_TOPLEFT, 0, 0);
    setPoint(PT_BOTTOMRIGHT, rel, PT_BOTTOMRIGHT, 0, 0);
  }
  void setAllPoints()
  {
    setPoint(PT_TOPLEFT, parent, PT_TOPLEFT, 0, 0);
    setPoint(PT_BOTTOMRIGHT, parent, PT_BOTTOMRIGHT, 0, 0);
  }

  // 0 = unprocessed
  virtual uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam)
  {
    return 0;
  }
};

#endif // __FRAMEUI_FRAME_H__
