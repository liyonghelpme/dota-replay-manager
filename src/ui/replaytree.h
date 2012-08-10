#ifndef __UI_REPLAYTREE_H__
#define __UI_REPLAYTREE_H__

#include "frameui/framewnd.h"
#include "ui/dirchange.h"
#include "base/array.h"

class MainWnd;

class TreeViewFrame;
class ButtonFrame;
class ReplayTree : public Frame, public DirChangeHandler
{
  struct TreeItem
  {
    int type;
    String path;
    HTREEITEM treeItem;
  };
  TreeViewFrame* treeView;
  Array<TreeItem> items;
  DirChangeTracker* tracker;
  String path;
  MainWnd* mainWnd;

  ButtonFrame* byDate;

  HTREEITEM replayRoot;

  void populate (HTREEITEM parent, String path);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayTree(String path, MainWnd* parent);
  void setPath(String path);

  void onDirChange();
};

#endif // __UI_REPLAYTREE_H__
