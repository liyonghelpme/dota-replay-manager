#include <stdlib.h>
#include <memory.h>
#include "region.h"

MasterRegion::MasterRegion(Region* root)
{
  maxRegions = 8;
  numRegions = 1;
  regions = new Region*[maxRegions * 2];
  regions[0] = root;
  regions[0]->master = this;
  regions[0]->mr_pos = 0;
  moveid = 0;
  updater = NULL;
  regions[0]->mr_valid = true;
  regions[0]->_x = 0;
  regions[0]->_y = 0;
  regions[0]->_width = 0;
  regions[0]->_height = 0;
}
MasterRegion::~MasterRegion()
{
  regions[0]->master = NULL;
  for (int i = 1; i < numRegions; i++)
  {
    regions[i]->master = NULL;
    delete regions[i];
  }
  delete[] regions;
}
void MasterRegion::setSize(int width, int height)
{
  regions[0]->_width = width;
  regions[0]->_height = height;
  deepUpdateRegion(regions[0]);
}
void MasterRegion::addRegion(Region* r)
{
  if (numRegions >= maxRegions)
  {
    maxRegions *= 2;
    Region** temp = new Region*[maxRegions * 2];
    memcpy(temp, regions, sizeof(Region*) * numRegions);
    delete[] regions;
    regions = temp;
  }
  r->mr_pos = numRegions;
  regions[numRegions++] = r;
}
void MasterRegion::removeRegion(Region* r)
{
  for (int i = r->mr_pos + 1; i < numRegions; i++)
  {
    for (int j = 0; j < Region::Anchor::count; j++)
      if (regions[i]->anchors[j].rel == r)
        regions[i]->anchors[j].active = false;
    regions[i]->mr_pos = i - 1;
    regions[i - 1] = regions[i];
  }
  numRegions--;
}
void MasterRegion::setPoint(Region* r, int point, Region* rel, int relPoint, int x, int y)
{
  if (rel == NULL)
    rel = regions[0];
  // move the tree
  moveid++;
  int cur = r->mr_pos;
  r->mr_moving = moveid;
  int target = rel->mr_pos;
  int count = 1;
  if (cur < target)
  {
    regions[numRegions] = r;
    while (++cur <= target)
    {
      for (int i = 0; i < Region::Anchor::count; i++)
      {
        if (regions[cur]->anchors[i].rel &&
            regions[cur]->anchors[i].rel->mr_moving == moveid)
        {
          regions[cur]->mr_moving = moveid;
          regions[numRegions + (count++)] = regions[cur];
          break;
        }
      }
      if (cur == target && regions[cur]->mr_moving == moveid)
        return;
    }
    count = 0;
    for (cur = r->mr_pos; cur <= target; cur++)
    {
      if (regions[cur]->mr_moving == moveid)
        count++;
      else
      {
        regions[cur]->mr_pos = cur - count;
        regions[cur - count] = regions[cur];
      }
    }
    for (int i = 0; i < count; i++)
    {
      regions[numRegions + i]->mr_pos = target - count + i + 1;
      regions[target - count + i + 1] = regions[numRegions + i];
    }
  }
  else
    target = cur;
  // add the anchor
  if ((point & 0x0F) == 0x00) // hLeft
  {
    r->anchors[Region::Anchor::hCenter].active = false;
    if (r->anchors[Region::Anchor::hRight].active)
      r->anchors[Region::Anchor::hWidth].active = false;
    Region::Anchor& a = r->anchors[Region::Anchor::hLeft];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = x;
  }
  else if ((point & 0x0F) == 0x01) // hCenter
  {
    if (r->anchors[Region::Anchor::hLeft].active == false &&
        r->anchors[Region::Anchor::hRight].active == false)
    {
      Region::Anchor& a = r->anchors[Region::Anchor::hCenter];
      a.active = true;
      a.rel = rel;
      a.relPoint = relPoint;
      a.offset = x;
    }
  }
  else // hRight
  {
    r->anchors[Region::Anchor::hCenter].active = false;
    if (r->anchors[Region::Anchor::hLeft].active)
      r->anchors[Region::Anchor::hWidth].active = false;
    Region::Anchor& a = r->anchors[Region::Anchor::hRight];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = x;
  }
  if ((point & 0xF0) == 0x00) // vTop
  {
    r->anchors[Region::Anchor::vCenter].active = false;
    if (r->anchors[Region::Anchor::vBottom].active)
      r->anchors[Region::Anchor::vHeight].active = false;
    Region::Anchor& a = r->anchors[Region::Anchor::vTop];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = y;
  }
  else if ((point & 0xF0) == 0x10) // vCenter
  {
    if (r->anchors[Region::Anchor::vTop].active == false &&
        r->anchors[Region::Anchor::vBottom].active == false)
    {
      Region::Anchor& a = r->anchors[Region::Anchor::vCenter];
      a.active = true;
      a.rel = rel;
      a.relPoint = relPoint;
      a.offset = y;
    }
  }
  else // vBottom
  {
    r->anchors[Region::Anchor::vCenter].active = false;
    if (r->anchors[Region::Anchor::vTop].active)
      r->anchors[Region::Anchor::vHeight].active = false;
    Region::Anchor& a = r->anchors[Region::Anchor::vBottom];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = y;
  }
  // update positions
  for (int cur = target - count + 1; cur < numRegions; cur++)
  {
    for (int i = 0; i < Region::Anchor::count && regions[cur]->mr_moving != moveid; i++)
    {
      if (regions[cur]->anchors[i].rel &&
          regions[cur]->anchors[i].rel->mr_moving == moveid)
        regions[cur]->mr_moving = moveid;
    }
    if (regions[cur]->mr_moving == moveid)
      updateRegion(regions[cur]);
  }
}
void MasterRegion::setWidth(Region* r, int width)
{
  if (r->anchors[Region::Anchor::hLeft].active &&
      r->anchors[Region::Anchor::hRight].active)
    return;
  Region::Anchor& c = r->anchors[Region::Anchor::hWidth];
  c.active = true;
  c.offset = width;
  deepUpdateRegion(r);
}
void MasterRegion::setHeight(Region* r, int height)
{
  if (r->anchors[Region::Anchor::vTop].active &&
      r->anchors[Region::Anchor::vBottom].active)
    return;
  Region::Anchor& c = r->anchors[Region::Anchor::vHeight];
  c.active = true;
  c.offset = height;
  deepUpdateRegion(r);
}
void MasterRegion::deepUpdateRegion(Region* r)
{
  moveid++;
  r->mr_moving = moveid;
  for (int cur = r->mr_pos; cur < numRegions; cur++)
  {
    for (int i = 0; i < Region::Anchor::count && regions[cur]->mr_moving != moveid; i++)
    {
      if (regions[cur]->anchors[i].rel &&
          regions[cur]->anchors[i].rel->mr_moving == moveid)
        regions[cur]->mr_moving = moveid;
    }
    if (regions[cur]->mr_moving == moveid)
      updateRegion(regions[cur]);
  }
}
inline int getRegionX(Region* r, int point)
{
  if ((point & 0x0F) == 0x00)
    return r->left();
  else if ((point & 0x0F) == 0x01)
    return r->left() + r->width() / 2;
  else
    return r->left() + r->width();
}
inline int getRegionY(Region* r, int point)
{
  if ((point & 0xF0) == 0x00)
    return r->top();
  else if ((point & 0xF0) == 0x10)
    return r->top() + r->height() / 2;
  else
    return r->top() + r->height();
}
void MasterRegion::updateRegion(Region* r)
{
  Rect oldRect(r->left(), r->top(), r->right(), r->bottom());
  if (r->anchors[Region::Anchor::hLeft].active &&
      r->anchors[Region::Anchor::hRight].active)
  {
    Region::Anchor& la = r->anchors[Region::Anchor::hLeft];
    Region::Anchor& ra = r->anchors[Region::Anchor::hRight];
    if (la.rel && la.rel->mr_valid && ra.rel && ra.rel->mr_valid)
    {
      r->_x = getRegionX(la.rel, la.relPoint) + la.offset;
      r->_width = getRegionX(ra.rel, ra.relPoint) + ra.offset - r->_x;
    }
    else
    {
      invalidateRegion(r);
      r->mr_valid = false;
      r->onMove();
      return;
    }
  }
  else if (r->anchors[Region::Anchor::hWidth].active)
  {
    r->_width = r->anchors[Region::Anchor::hWidth].offset;
    Region::Anchor& la = r->anchors[Region::Anchor::hLeft];
    Region::Anchor& ra = r->anchors[Region::Anchor::hRight];
    Region::Anchor& ca = r->anchors[Region::Anchor::hCenter];
    if (la.active && la.rel && la.rel->mr_valid)
      r->_x = getRegionX(la.rel, la.relPoint) + la.offset;
    else if (ra.active && ra.rel && ra.rel->mr_valid)
      r->_x = getRegionX(ra.rel, ra.relPoint) + ra.offset - r->_width;
    else if (ca.active && ca.rel && ca.rel->mr_valid)
      r->_x = getRegionX(ca.rel, ca.relPoint) + ca.offset - r->_width / 2;
    else
    {
      invalidateRegion(r);
      r->mr_valid = false;
      r->onMove();
      return;
    }
  }
  if (r->anchors[Region::Anchor::vTop].active &&
      r->anchors[Region::Anchor::vBottom].active)
  {
    Region::Anchor& ta = r->anchors[Region::Anchor::vTop];
    Region::Anchor& ba = r->anchors[Region::Anchor::vBottom];
    if (ta.rel && ta.rel->mr_valid && ba.rel && ba.rel->mr_valid)
    {
      r->_y = getRegionY(ta.rel, ta.relPoint) + ta.offset;
      r->_height = getRegionY(ba.rel, ba.relPoint) + ba.offset - r->_y;
    }
    else
    {
      invalidateRegion(r);
      r->mr_valid = false;
      r->onMove();
      return;
    }
  }
  else if (r->anchors[Region::Anchor::vHeight].active)
  {
    r->_height = r->anchors[Region::Anchor::vHeight].offset;
    Region::Anchor& ta = r->anchors[Region::Anchor::vTop];
    Region::Anchor& ba = r->anchors[Region::Anchor::vBottom];
    Region::Anchor& ca = r->anchors[Region::Anchor::vCenter];
    if (ta.active && ta.rel && ta.rel->mr_valid)
      r->_y = getRegionY(ta.rel, ta.relPoint) + ta.offset;
    else if (ba.active && ba.rel && ba.rel->mr_valid)
      r->_y = getRegionY(ba.rel, ba.relPoint) + ba.offset - r->_height;
    else if (ca.active && ca.rel && ca.rel->mr_valid)
      r->_y = getRegionY(ca.rel, ca.relPoint) + ca.offset - r->_height / 2;
    else
    {
      invalidateRegion(r);
      r->mr_valid = false;
      r->onMove();
      return;
    }
  }
  if (r->left() != oldRect.left || r->top() != oldRect.top ||
      r->right() != oldRect.right || r->bottom() != oldRect.bottom ||
      !r->mr_valid)
  {
    if (updater && r->visible())
      updater->add(oldRect);
    r->mr_valid = true;
    invalidateRegion(r);
    r->onMove();
  }
}
void MasterRegion::invalidateRegion(Region const* r)
{
  if (updater)
    updater->add(r->left(), r->top(), r->right(), r->bottom());
}

////////////////////////////

Region::Region(Region* _parent)
{
  parent = _parent;
  master = NULL;
  mr_pos = 0;
  mr_moving = 0;
  mr_valid = false;
  _x = 0;
  _y = 0;
  _width = 0;
  _height = 0;
  memset(anchors, 0, sizeof anchors);
  if (parent)
    master = parent->master;
  if (master)
    master->addRegion(this);
}
Region::~Region()
{
  if (master)
    master->removeRegion(this);
}
