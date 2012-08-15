#ifndef __UI_REPLAYTREE_H__
#define __UI_REPLAYTREE_H__

#include "frameui/framewnd.h"
#include "ui/dirchange.h"
#include "base/array.h"
#include "frameui/controlframes.h"
#include "frameui/dragdrop.h"
#include "base/utils.h"

class MainWnd;

class DropTreeViewFrame : public TreeViewFrame
{
protected:
  DropTarget* target;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  HTREEITEM highlight;
  DropTreeViewFrame(Frame* parent, int id = 0, int style = 0);
};

#define WM_REBUILDTREE      (WM_USER+910)

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

  void enumfiles(Array<FileInfo>& files, String path);
  static int compfiles(FileInfo const& a, FileInfo const& b);
  int fillyear(Array<FileInfo>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis);
  int fillmonth(Array<FileInfo>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis);
  int fillday(Array<FileInfo>& files, int& cur, HTREEITEM parent);

  ButtonFrame* byDate;

  HTREEITEM replayRoot;

  bool updating;
  int populate (HTREEITEM parent, String path);

  void rebuild();

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayTree(String path, MainWnd* parent);
  ~ReplayTree();
  void setPath(String path);

  void setCurFile(String path);

  void onDirChange();
};

#endif // __UI_REPLAYTREE_H__
