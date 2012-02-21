#ifndef __FRAMEUI_REGION_H__
#define __FRAMEUI_REGION_H__

#include "graphics/rect.h"

#define PT_TOPLEFT        0x00
#define PT_TOP            0x01
#define PT_TOPRIGHT       0x02
#define PT_LEFT           0x10
#define PT_CENTER         0x11
#define PT_RIGHT          0x12
#define PT_BOTTOMLEFT     0x20
#define PT_BOTTOM         0x21
#define PT_BOTTOMRIGHT    0x22

class MasterRegion
{
  friend class Region;

  RectUpdater* updater;

  Region** regions;
  int numRegions;
  int maxRegions;
  int moveid;

  void addRegion(Region* r);
  void updateRegion(Region* r);
  void deepUpdateRegion(Region* r);
  void setPoint(Region* r, int point, Region* rel, int relPoint, int x, int y);
  void setWidth(Region* r, int width);
  void setHeight(Region* r, int height);
  void removeRegion(Region* r);
  void invalidateRegion(Region const* r);
public:
  MasterRegion(Region* root);
  ~MasterRegion();

  Region* getRoot() const
  {
    return regions[0];
  }

  void setSize(int width, int height);

  void setUpdater(RectUpdater* rc)
  {
    updater = rc;
  }
};

class Region
{
  // master region data
  friend class MasterRegion;
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
    Region* rel;
    int relPoint;
    int offset;
  } anchors[8];
protected:
  MasterRegion* master;
  Region* parent;
  virtual void onMove()
  {
  }
public:
  Region(Region* parent);
  virtual ~Region();

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
  virtual void show(bool s = true)
  {
    _visible = s;
    invalidate();
  }
  void hide()
  {
    show(false);
  }
  bool shown() const
  {
    return _visible;
  }
  bool visible() const
  {
    return _visible && mr_valid && (parent ? parent->visible() : true);
  }

  void invalidate() const
  {
    master->invalidateRegion(this);
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
  void setPoint(int point, Region* rel, int relPoint, int x, int y)
  {
    master->setPoint(this, point, rel, relPoint, x, y);
  }
  void setPoint(int point, Region* rel, int x, int y)
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
    master->deepUpdateRegion(this);
  }
  void setAllPoints(Region* rel)
  {
    setPoint(PT_TOPLEFT, rel, PT_TOPLEFT, 0, 0);
    setPoint(PT_BOTTOMRIGHT, rel, PT_BOTTOMRIGHT, 0, 0);
  }
  void setAllPoints()
  {
    setPoint(PT_TOPLEFT, parent, PT_TOPLEFT, 0, 0);
    setPoint(PT_BOTTOMRIGHT, parent, PT_BOTTOMRIGHT, 0, 0);
  }
};

#endif // __FRAMEUI_REGION_H__
