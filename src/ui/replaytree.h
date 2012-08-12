#ifndef __UI_REPLAYTREE_H__
#define __UI_REPLAYTREE_H__

#include "frameui/framewnd.h"
#include "ui/dirchange.h"
#include "base/array.h"

class MainWnd;

class DropTreeViewFrame;
class ButtonFrame;
class ReplayTree : public Frame, public DirChangeHandler
{
  struct TreeItem
  {
    int type;
    String path;
    HTREEITEM treeItem;
  };
  DropTreeViewFrame* treeView;
  Array<TreeItem> items;
  DirChangeTracker* tracker;
  String path;
  MainWnd* mainWnd;

  struct DateItem
  {
    String path;
    uint64 ftime;
    SYSTEMTIME time;
  };
  void enumfiles(Array<DateItem>& files, String path);
  static int compfiles(DateItem const& a, DateItem const& b);
  int fillyear(Array<DateItem>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis);
  int fillmonth(Array<DateItem>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis);
  int fillday(Array<DateItem>& files, int& cur, HTREEITEM parent);

  ButtonFrame* byDate;

  HTREEITEM replayRoot;

  bool updating;
  int populate (HTREEITEM parent, String path);

  void rebuild();

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayTree(String path, MainWnd* parent);
  void setPath(String path);

  void setCurFile(String path);

  void onDirChange();
};

#endif // __UI_REPLAYTREE_H__
