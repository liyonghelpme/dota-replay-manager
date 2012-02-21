#ifndef __RECT_H__
#define __RECT_H__

#include <stdlib.h>

class Point
{
public:
  int x, y;

  Point () {}
  Point (int px, int py)
  {
    x = px;
    y = py;
  }
  Point (Point const& p)
  {
    x = p.x;
    y = p.y;
  }
  Point& operator = (Point const& p)
  {
    x = p.x;
    y = p.y;
    return *this;
  }
  Point operator + () const
  {
    return *this;
  }
  Point operator - () const
  {
    return Point (-x, -y);
  }
  Point& operator += (Point const& p)
  {
    x += p.x;
    y += p.y;
    return *this;
  }
  Point& operator -= (Point const& p)
  {
    x -= p.x;
    y -= p.y;
    return *this;
  }
  Point& operator *= (float f)
  {
    x = int (float (x) * f + 0.5f);
    y = int (float (y) * f + 0.5f);
    return *this;
  }
  Point& operator /= (float f)
  {
    x = int (float (x) / f + 0.5f);
    y = int (float (y) / f + 0.5f);
    return *this;
  }
  int& operator [] (int index)
  {
    return * (&x + index);
  }
  int operator [] (int index) const
  {
    return * (&x + index);
  }
  bool operator == (Point const& p) const
  {
    return x == p.x && y == p.y;
  }
  bool operator != (Point const& p) const
  {
    return x != p.y || y != p.y;
  }
  operator int* ()
  {
    return &x;
  }
  operator int const* () const
  {
    return &x;
  }
  Point ort () const
  {
    return Point (-y, x);
  }
  friend Point operator + (Point const& a, Point const& b)
  {
    return Point (a.x + b.x, a.y + b.y);
  }
  friend Point operator - (Point const& a, Point const& b)
  {
    return Point (a.x - b.x, a.y - b.y);
  }
  friend Point operator * (Point const& p, float f)
  {
    return Point (int (float (p.x) * f + 0.5f), int (float (p.y) * f + 0.5f));
  }
  friend Point operator * (float f, Point const& p)
  {
    return Point (int (float (p.x) * f + 0.5f), int (float (p.y) * f + 0.5f));
  }
  friend Point operator / (Point const& p, float f)
  {
    return Point (int (float (p.x) / f + 0.5f), int (float (p.y) / f + 0.5f));
  }

  Point& set (int px, int py)
  {
    x = px;
    y = py;
    return *this;
  }
};

struct Rect
{
  int left;
  int top;
  int right;
  int bottom;
  Rect () {}
  Rect (int l, int t, int r, int b)
  {
    left = l;
    top = t;
    right = r;
    bottom = b;
  }

  void set(int l, int t, int r, int b)
  {
    left = l;
    top = t;
    right = r;
    bottom = b;
  }

  bool contains (int x, int y) const
  {
    return x >= left && y >= top && x < right && y < bottom;
  }
};

class RectUpdater
{
public:
  virtual void add(Rect const& rect) = NULL;
  void add(int left, int top, int right, int bottom)
  {
    add(Rect(left, top, right, bottom));
  }
};

class RectCollection : public RectUpdater
{
  enum {maxRects = 32};
  Rect rects[maxRects + 1];
  int count;
public:
  RectCollection()
  {
    reset();
  }

  void reset();
  void add(Rect const& rect);

  Rect const& getBoundingRect() const
  {
    return rects[maxRects];
  }
  int getNumRects() const
  {
    return count;
  }
  Rect const& getRect(int i) const
  {
    return rects[i];
  }
};

#endif // __RECT_H__
