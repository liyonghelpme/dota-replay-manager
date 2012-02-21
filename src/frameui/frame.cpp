#include "frame.h"

Frame::Frame(Frame* _parent)
  : Region(_parent)
{
  firstChild = NULL;
  lastChild = NULL;
  if (_parent)
  {
    prevSibling = _parent->lastChild;
    if (_parent->lastChild)
      _parent->lastChild->nextSibling = this;
    else
      _parent->firstChild = this;
    _parent->lastChild = this;
  }
  else
    prevSibling = NULL;
  nextSibling = NULL;
  staticRegions = NULL;
  numStaticRegions = 0;
  maxStaticRegions = 0;
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
  //for (int i = 0; i < numStaticRegions; i++)
  //  delete staticRegions[i];
  delete[] staticRegions;
}
void Frame::setParent(Frame* _parent)
{
  if (parent)
  {
    Frame* p = (Frame*) parent;
    if (prevSibling)
      prevSibling->nextSibling = nextSibling;
    else
      p->firstChild = nextSibling;
    if (nextSibling)
      nextSibling->prevSibling = prevSibling;
    else
      p->lastChild = prevSibling;
    prevSibling = NULL;
    nextSibling = NULL;
  }
  parent = _parent;
  if (_parent)
  {
    prevSibling = _parent->lastChild;
    nextSibling = NULL;
    if (_parent->lastChild)
      _parent->lastChild->nextSibling = this;
    else
      _parent->firstChild = this;
    _parent->lastChild = this;
  }
}

void Frame::render(HDC hDC)
{
  if (shown())
  {
    for (int i = 0; i < numStaticRegions; i++)
      if (staticRegions[i]->shown())
        staticRegions[i]->render(hDC);
    for (Frame* cur = firstChild; cur; cur = cur->nextSibling)
      if (cur->shown())
        cur->render(hDC);
  }
}

StaticRegion* Frame::addStaticRegion(StaticRegion* r)
{
  if (staticRegions == NULL)
  {
    numStaticRegions = 0;
    maxStaticRegions = 8;
    staticRegions = new StaticRegion*[maxStaticRegions];
  }
  else if (numStaticRegions >= maxStaticRegions)
  {
    maxStaticRegions *= 2;
    StaticRegion** temp = new StaticRegion*[maxStaticRegions];
    memcpy(temp, staticRegions, sizeof(StaticRegion*) * numStaticRegions);
    delete[] staticRegions;
    staticRegions = temp;
  }
  return staticRegions[numStaticRegions++] = r;
}

void Frame::onChangeVisibility()
{
  onMove();
  invalidate();
  for (Frame* cur = firstChild; cur; cur = cur->nextSibling)
    if (cur->shown())
      cur->onChangeVisibility();
}
void Frame::show(bool s)
{
  if (s != shown())
  {
    Region::show(s);
    for (int i = 0; i < numStaticRegions; i++)
      if (staticRegions[i]->shown())
        staticRegions[i]->invalidate();
    onChangeVisibility();
  }
}
