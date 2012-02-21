#include "core/app.h"
#include "graphics/imagelib.h"

#include "replaytree.h"
#include "frameui/fontsys.h"

struct ReplayTree::TreeItem
{
};
ReplayTree::ReplayTree(String thePath, Frame* parent)
  : WindowFrame(parent)
{
  tracker = NULL;
  subclass(WC_TREEVIEW, "",
    TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS |
    WS_HSCROLL | WS_TABSTOP | WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);

  TreeView_SetImageList(hWnd, getApp()->getImageLibrary()->getImageList(), TVSIL_NORMAL);
  TreeView_SetItemHeight(hWnd, 18);

  setPath(thePath);
  setFont(FontSys::getSysFont());

  items = NULL;
  numItems = 0;
  maxItems = 0;
}

struct FoundItem
{
  bool folder;
  String name;
  FoundItem() {}
  FoundItem(bool f, String n)
  {
    folder = f;
    name = n;
  }
};
static int compItems(FoundItem const& a, FoundItem const& b)
{
  if (a.folder != b.folder)
    return a.folder ? -1 : 1;
  return String::smartCompare(a.name, b.name);
}

void ReplayTree::populate(HTREEITEM parent, String thePath)
{
  Array<FoundItem> items;

  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(String::buildFullName(thePath, "*"), &data);
  BOOL success = (hFind != INVALID_HANDLE_VALUE);
  while (success)
  {
    if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
    {
      if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        items.push(FoundItem(true, data.cFileName));
      else if (String::getExtension(data.cFileName).caseInsensitiveCompare(".w3g") == 0)
        items.push(FoundItem(false, data.cFileName));
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);

  items.sort(compItems);

  ImageLibrary* images = getApp()->getImageLibrary();
  for (int i = 0; i < items.length(); i++)
  {
    TVINSERTSTRUCT tvis;
    memset(&tvis, 0, sizeof tvis);
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = parent;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = const_cast<char*>(items[i].name.c_str());
    if (items[i].folder)
    {
      tvis.item.iImage = images->getListIndex("IconClosedFolder");
      tvis.item.iSelectedImage = images->getListIndex("IconOpenFolder");
      HTREEITEM folder = TreeView_InsertItem(hWnd, &tvis);
      populate(folder, String::buildFullName(thePath, items[i].name));
    }
    else
    {
      tvis.item.iImage = images->getListIndex("IconReplay");
      tvis.item.iSelectedImage = tvis.item.iImage;
      TreeView_InsertItem(hWnd, &tvis);
    }
  }
}
void ReplayTree::setPath(String thePath)
{
  delete tracker;
  path = thePath;
  onDirChange();
  tracker = new DirChangeTracker(this, path, true);
}
void ReplayTree::onDirChange()
{
  TreeView_DeleteItem(hWnd, TVI_ROOT);
  populate(TVI_ROOT, path);
}
