#include <stdlib.h>
#include <memory.h>
#include "frame.h"

MasterFrame::MasterFrame(Frame* root)
{
  maxFrames = 8;
  numFrames = 1;
  frames = new Frame*[maxFrames * 2];
  frames[0] = root;
  frames[0]->master = this;
  frames[0]->mr_pos = 0;
  moveid = 0;
  updating = false;
  frames[0]->mr_valid = true;
  frames[0]->_x = 0;
  frames[0]->_y = 0;
  frames[0]->_width = 0;
  frames[0]->_height = 0;
}
MasterFrame::~MasterFrame()
{
  frames[0]->master = NULL;
  for (int i = 1; i < numFrames; i++)
  {
    frames[i]->master = NULL;
    delete frames[i];
  }
  delete[] frames;
}
void MasterFrame::setSize(int width, int height)
{
  frames[0]->_width = width;
  frames[0]->_height = height;
  deepUpdateFrame(frames[0]);
}
void MasterFrame::addFrame(Frame* r)
{
  if (numFrames >= maxFrames)
  {
    maxFrames *= 2;
    Frame** temp = new Frame*[maxFrames * 2];
    memcpy(temp, frames, sizeof(Frame*) * numFrames);
    delete[] frames;
    frames = temp;
  }
  r->mr_pos = numFrames;
  frames[numFrames++] = r;
}
void MasterFrame::removeFrame(Frame* r)
{
  for (int i = r->mr_pos + 1; i < numFrames; i++)
  {
    for (int j = 0; j < Frame::Anchor::count; j++)
      if (frames[i]->anchors[j].rel == r)
        frames[i]->anchors[j].active = false;
    frames[i]->mr_pos = i - 1;
    frames[i - 1] = frames[i];
  }
  numFrames--;
}
void MasterFrame::setPoint(Frame* r, int point, Frame* rel, int relPoint, int x, int y)
{
  if (rel == NULL)
    rel = frames[0];
  // move the tree
  if (!updating)
    moveid++;
  int cur = r->mr_pos;
  int target = rel->mr_pos;
  int count = 1;
  if (cur < target && updating)
    return;
  r->mr_moving = moveid;
  if (cur < target)
  {
    frames[numFrames] = r;
    while (++cur <= target)
    {
      for (int i = 0; i < Frame::Anchor::count; i++)
      {
        if (frames[cur]->anchors[i].rel &&
            frames[cur]->anchors[i].rel->mr_moving == moveid)
        {
          frames[cur]->mr_moving = moveid;
          frames[numFrames + (count++)] = frames[cur];
          break;
        }
      }
      if (cur == target && frames[cur]->mr_moving == moveid)
        return;
    }
    count = 0;
    for (cur = r->mr_pos; cur <= target; cur++)
    {
      if (frames[cur]->mr_moving == moveid)
        count++;
      else
      {
        frames[cur]->mr_pos = cur - count;
        frames[cur - count] = frames[cur];
      }
    }
    for (int i = 0; i < count; i++)
    {
      frames[numFrames + i]->mr_pos = target - count + i + 1;
      frames[target - count + i + 1] = frames[numFrames + i];
    }
  }
  else
    target = cur;
  // add the anchor
  if ((point & 0x0F) == 0x00) // hLeft
  {
    r->anchors[Frame::Anchor::hCenter].active = false;
    if (r->anchors[Frame::Anchor::hRight].active)
      r->anchors[Frame::Anchor::hWidth].active = false;
    Frame::Anchor& a = r->anchors[Frame::Anchor::hLeft];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = x;
  }
  else if ((point & 0x0F) == 0x01) // hCenter
  {
    if (r->anchors[Frame::Anchor::hLeft].active == false &&
        r->anchors[Frame::Anchor::hRight].active == false)
    {
      Frame::Anchor& a = r->anchors[Frame::Anchor::hCenter];
      a.active = true;
      a.rel = rel;
      a.relPoint = relPoint;
      a.offset = x;
    }
  }
  else // hRight
  {
    r->anchors[Frame::Anchor::hCenter].active = false;
    if (r->anchors[Frame::Anchor::hLeft].active)
      r->anchors[Frame::Anchor::hWidth].active = false;
    Frame::Anchor& a = r->anchors[Frame::Anchor::hRight];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = x;
  }
  if ((point & 0xF0) == 0x00) // vTop
  {
    r->anchors[Frame::Anchor::vCenter].active = false;
    if (r->anchors[Frame::Anchor::vBottom].active)
      r->anchors[Frame::Anchor::vHeight].active = false;
    Frame::Anchor& a = r->anchors[Frame::Anchor::vTop];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = y;
  }
  else if ((point & 0xF0) == 0x10) // vCenter
  {
    if (r->anchors[Frame::Anchor::vTop].active == false &&
        r->anchors[Frame::Anchor::vBottom].active == false)
    {
      Frame::Anchor& a = r->anchors[Frame::Anchor::vCenter];
      a.active = true;
      a.rel = rel;
      a.relPoint = relPoint;
      a.offset = y;
    }
  }
  else // vBottom
  {
    r->anchors[Frame::Anchor::vCenter].active = false;
    if (r->anchors[Frame::Anchor::vTop].active)
      r->anchors[Frame::Anchor::vHeight].active = false;
    Frame::Anchor& a = r->anchors[Frame::Anchor::vBottom];
    a.active = true;
    a.rel = rel;
    a.relPoint = relPoint;
    a.offset = y;
  }
  // update positions
  if (!updating)
  {
    updating = true;
    for (int cur = target - count + 1; cur < numFrames; cur++)
    {
      for (int i = 0; i < Frame::Anchor::count && frames[cur]->mr_moving != moveid; i++)
      {
        if (frames[cur]->anchors[i].rel &&
            frames[cur]->anchors[i].rel->mr_moving == moveid)
          frames[cur]->mr_moving = moveid;
      }
      if (frames[cur]->mr_moving == moveid)
        updateFrame(frames[cur]);
    }
    updating = false;
  }
}
void MasterFrame::setWidth(Frame* r, int width)
{
  if (r->anchors[Frame::Anchor::hLeft].active &&
      r->anchors[Frame::Anchor::hRight].active)
    return;
  Frame::Anchor& c = r->anchors[Frame::Anchor::hWidth];
  c.active = true;
  c.offset = width;
  deepUpdateFrame(r);
}
void MasterFrame::setHeight(Frame* r, int height)
{
  if (r->anchors[Frame::Anchor::vTop].active &&
      r->anchors[Frame::Anchor::vBottom].active)
    return;
  Frame::Anchor& c = r->anchors[Frame::Anchor::vHeight];
  c.active = true;
  c.offset = height;
  deepUpdateFrame(r);
}
void MasterFrame::deepUpdateFrame(Frame* r)
{
  if (!updating)
  {
    updating = true;
    moveid++;
    r->mr_moving = moveid;
    for (int cur = r->mr_pos; cur < numFrames; cur++)
    {
      for (int i = 0; i < Frame::Anchor::count && frames[cur]->mr_moving != moveid; i++)
      {
        if (frames[cur]->anchors[i].rel &&
            frames[cur]->anchors[i].rel->mr_moving == moveid)
          frames[cur]->mr_moving = moveid;
      }
      if (frames[cur]->mr_moving == moveid)
        updateFrame(frames[cur]);
    }
    updating = false;
  }
}
inline int getRegionX(Frame* r, int point)
{
  if ((point & 0x0F) == 0x00)
    return r->left();
  else if ((point & 0x0F) == 0x01)
    return r->left() + r->width() / 2;
  else
    return r->left() + r->width();
}
inline int getRegionY(Frame* r, int point)
{
  if ((point & 0xF0) == 0x00)
    return r->top();
  else if ((point & 0xF0) == 0x10)
    return r->top() + r->height() / 2;
  else
    return r->top() + r->height();
}
void MasterFrame::updateFrame(Frame* r)
{
  int oldLeft = r->left();
  int oldTop = r->top();
  int oldRight = r->right();
  int oldBottom = r->bottom();
  if (r->anchors[Frame::Anchor::hLeft].active &&
      r->anchors[Frame::Anchor::hRight].active)
  {
    Frame::Anchor& la = r->anchors[Frame::Anchor::hLeft];
    Frame::Anchor& ra = r->anchors[Frame::Anchor::hRight];
    if (la.rel && la.rel->mr_valid && ra.rel && ra.rel->mr_valid)
    {
      r->_x = getRegionX(la.rel, la.relPoint) + la.offset;
      r->_width = getRegionX(ra.rel, ra.relPoint) + ra.offset - r->_x;
    }
    else
    {
      if (r->mr_valid)
      {
        r->mr_valid = false;
        r->onChangeVisibility();
      }
      return;
    }
  }
  else if (r->anchors[Frame::Anchor::hWidth].active)
  {
    r->_width = r->anchors[Frame::Anchor::hWidth].offset;
    Frame::Anchor& la = r->anchors[Frame::Anchor::hLeft];
    Frame::Anchor& ra = r->anchors[Frame::Anchor::hRight];
    Frame::Anchor& ca = r->anchors[Frame::Anchor::hCenter];
    if (la.active && la.rel && la.rel->mr_valid)
      r->_x = getRegionX(la.rel, la.relPoint) + la.offset;
    else if (ra.active && ra.rel && ra.rel->mr_valid)
      r->_x = getRegionX(ra.rel, ra.relPoint) + ra.offset - r->_width;
    else if (ca.active && ca.rel && ca.rel->mr_valid)
      r->_x = getRegionX(ca.rel, ca.relPoint) + ca.offset - r->_width / 2;
    else
    {
      if (r->mr_valid)
      {
        r->mr_valid = false;
        r->onChangeVisibility();
      }
      return;
    }
  }
  if (r->anchors[Frame::Anchor::vTop].active &&
      r->anchors[Frame::Anchor::vBottom].active)
  {
    Frame::Anchor& ta = r->anchors[Frame::Anchor::vTop];
    Frame::Anchor& ba = r->anchors[Frame::Anchor::vBottom];
    if (ta.rel && ta.rel->mr_valid && ba.rel && ba.rel->mr_valid)
    {
      r->_y = getRegionY(ta.rel, ta.relPoint) + ta.offset;
      r->_height = getRegionY(ba.rel, ba.relPoint) + ba.offset - r->_y;
    }
    else
    {
      if (r->mr_valid)
      {
        r->mr_valid = false;
        r->onChangeVisibility();
      }
      return;
    }
  }
  else if (r->anchors[Frame::Anchor::vHeight].active)
  {
    r->_height = r->anchors[Frame::Anchor::vHeight].offset;
    Frame::Anchor& ta = r->anchors[Frame::Anchor::vTop];
    Frame::Anchor& ba = r->anchors[Frame::Anchor::vBottom];
    Frame::Anchor& ca = r->anchors[Frame::Anchor::vCenter];
    if (ta.active && ta.rel && ta.rel->mr_valid)
      r->_y = getRegionY(ta.rel, ta.relPoint) + ta.offset;
    else if (ba.active && ba.rel && ba.rel->mr_valid)
      r->_y = getRegionY(ba.rel, ba.relPoint) + ba.offset - r->_height;
    else if (ca.active && ca.rel && ca.rel->mr_valid)
      r->_y = getRegionY(ca.rel, ca.relPoint) + ca.offset - r->_height / 2;
    else
    {
      if (r->mr_valid)
      {
        r->mr_valid = false;
        r->onChangeVisibility();
      }
      return;
    }
  }
  if (r->left() != oldLeft || r->top() != oldTop ||
      r->right() != oldRight || r->bottom() != oldBottom ||
      !r->mr_valid)
  {
    if (!r->mr_valid)
    {
      r->mr_valid = true;
      r->onChangeVisibility();
    }
    else
      r->onMove();
  }
}

////////////////////////////

Frame::Frame(Frame* _parent)
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
    master->addFrame(this);

  firstChild = NULL;
  lastChild = NULL;
  if (parent)
  {
    prevSibling = parent->lastChild;
    if (parent->lastChild)
      parent->lastChild->nextSibling = this;
    else
      parent->firstChild = this;
    parent->lastChild = this;
  }
  else
    prevSibling = NULL;
  nextSibling = NULL;
}
Frame::~Frame()
{
  setParent(NULL);
  Frame* cur = firstChild;
  while (cur)
  {
    Frame* next = cur->nextSibling;
    cur->parent = NULL;
    cur->prevSibling = NULL;
    cur->nextSibling = NULL;
    cur = next;
  }
  if (master)
    master->removeFrame(this);
}

void Frame::setParent(Frame* _parent)
{
  if (parent)
  {
    if (prevSibling)
      prevSibling->nextSibling = nextSibling;
    else
      parent->firstChild = nextSibling;
    if (nextSibling)
      nextSibling->prevSibling = prevSibling;
    else
      parent->lastChild = prevSibling;
    prevSibling = NULL;
    nextSibling = NULL;
  }
  parent = _parent;
  if (parent)
  {
    prevSibling = parent->lastChild;
    nextSibling = NULL;
    if (parent->lastChild)
      parent->lastChild->nextSibling = this;
    else
      parent->firstChild = this;
    parent->lastChild = this;
  }
}

void Frame::onChangeVisibility()
{
  onMove();
  for (Frame* cur = firstChild; cur; cur = cur->nextSibling)
    if (cur->_visible)
      cur->onChangeVisibility();
}
void Frame::show(bool s)
{
  if (s != _visible)
  {
    _visible = s;
    onChangeVisibility();
  }
}
