#include "base/types.h"
#include "rect.h"

static inline int min(int a, int b)
{
  return a < b ? a : b;
}
static inline int max(int a, int b)
{
  return a > b ? a : b;
}

void RectCollection::reset()
{
  rects[maxRects].set(max_int32, max_int32, min_int32, min_int32);
  count = 0;
}
void RectCollection::add(Rect const& rect)
{
  if (rect.left < rects[maxRects].left)
    rects[maxRects].left = rect.left;
  if (rect.top < rects[maxRects].top)
    rects[maxRects].top = rect.top;
  if (rect.right > rects[maxRects].right)
    rects[maxRects].right = rect.right;
  if (rect.bottom > rects[maxRects].bottom)
    rects[maxRects].bottom = rect.bottom;
  bool found = false;
  for (int i = 0; i < count; i++)
  {
    if (rect.left >= rects[i].left && rect.top >= rects[i].top &&
        rect.right <= rects[i].right && rect.bottom <= rects[i].bottom)
    {
      return;
    }
    if (rect.left <= rects[i].left && rect.top <= rects[i].top &&
        rect.right >= rects[i].right && rect.bottom >= rects[i].bottom)
    {
      if (found)
        rects[i--] = rects[--count];
      else
      {
        rects[i] = rect;
        found = true;
      }
    }
  }
  if (!found)
  {
    if (count < maxRects)
      rects[count++] = rect;
    else
    {
      int best = -1;
      int best_area = 0;
      for (int i = 0; i < count; i++)
      {
        int area = (max(rect.right, rects[i].right) - min(rect.left, rects[i].left)) *
                   (max(rect.bottom, rects[i].bottom) - min(rect.top, rects[i].top));
        area -= (rects[i].right - rects[i].left) * (rects[i].bottom - rects[i].top);
        if (rect.right > rects[i].left && rect.left < rects[i].right &&
            rect.bottom > rects[i].top && rect.top < rects[i].bottom)
          area += (min(rect.right, rects[i].right) - max(rect.left, rects[i].left)) *
                  (min(rect.bottom, rects[i].bottom) - max(rect.top, rects[i].top));
        if (best < 0 || area < best_area)
        {
          best = i;
          best_area = area;
        }
      }
      if (rect.left < rects[best].left)
        rects[best].left = rect.left;
      if (rect.top < rects[best].top)
        rects[best].top = rect.top;
      if (rect.right > rects[best].right)
        rects[best].right = rect.right;
      if (rect.bottom > rects[best].bottom)
        rects[best].bottom = rect.bottom;
      for (int i = 0; i < count; i++)
      {
        if (rects[best].left <= rects[i].left && rects[best].top <= rects[i].top &&
            rects[best].right >= rects[i].right && rects[best].bottom >= rects[i].bottom &&
            i != best)
        {
          rects[i] = rects[--count];
          if (count == best)
            best = i;
          i--;
        }
      }
    }
  }
}
