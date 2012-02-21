#ifndef __UI_REPLAYTREE_H__
#define __UI_REPLAYTREE_H__

#include "frameui/framewnd.h"
#include "ui/dirchange.h"

class ReplayTree : public WindowFrame, public DirChangeHandler
{
  struct TreeItem;
  TreeItem* items;
  DirChangeTracker* tracker;
  int numItems;
  int maxItems;
  String path;

  void populate (HTREEITEM parent, String path);
public:
  ReplayTree(String path, Frame* parent);
  void setPath(String path);

  void onDirChange();
};

#endif // __UI_REPLAYTREE_H__
