#ifndef __UI_VIEWITEM__
#define __UI_VIEWITEM__

#include "base/string.h"

class MainWnd;

class ViewItem
{
  ViewItem* next;
  ViewItem* prev;
public:
  ViewItem()
  {
    next = NULL;
    prev = NULL;
  }
  virtual ~ViewItem() {}
  virtual void apply(MainWnd* wnd) = 0;
  virtual bool isPermanent() const
  {
    return true;
  }

  ViewItem* push(ViewItem* item);
  void free();

  ViewItem* back();
  ViewItem* forward()
  {
    return next;
  }

  bool hasPrev() const
  {
    return (prev != NULL);
  }
  bool hasNext() const
  {
    return (next != NULL);
  }
};

class SettingsViewItem : public ViewItem
{
public:
  void apply(MainWnd* wnd);
  bool isPermanent() const
  {
    return false;
  }
};

class FolderViewItem : public ViewItem
{
  String path;
public:
  FolderViewItem(String folder)
  {
    path = folder;
  }
  void apply(MainWnd* wnd);
};

class ReplayViewItem : public ViewItem
{
  String path;
  int tab;
public:
  ReplayViewItem(String replay)
  {
    path = replay;
    tab = 0;
  }
  void setTab(int replayTab)
  {
    tab = replayTab;
  }
  void apply(MainWnd* wnd);
};

#endif // __UI_VIEWITEM__
