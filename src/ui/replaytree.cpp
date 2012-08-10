#include "core/app.h"
#include "graphics/imagelib.h"

#include "replaytree.h"
#include "frameui/fontsys.h"
#include "frameui/controlframes.h"

#include "ui/mainwnd.h"

#define IDC_BYDATE          136
#define IDC_REFRESH         137

ReplayTree::ReplayTree(String thePath, MainWnd* parent)
  : Frame(parent)
{
  mainWnd = parent;
  tracker = NULL;

  treeView = new TreeViewFrame(this, 0, TVS_HASBUTTONS | TVS_HASLINES |
    TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_HSCROLL | WS_TABSTOP);
  treeView->setImageList(getApp()->getImageLibrary()->getImageList(), TVSIL_NORMAL);
  treeView->setItemHeight(18);

  byDate = new ButtonFrame("By date", this, IDC_BYDATE, BS_AUTOCHECKBOX);
  byDate->setSize(100, 23);
  byDate->setPoint(PT_BOTTOMLEFT, 0, 0);
  ButtonFrame* refresh = new ButtonFrame("Refresh", this, IDC_REFRESH);
  refresh->setSize(100, 23);
  refresh->setPoint(PT_BOTTOMRIGHT, 0, 0);

  treeView->setPoint(PT_TOPLEFT, 0, 0);
  treeView->setPoint(PT_TOPRIGHT, 0, 0);
  treeView->setPoint(PT_BOTTOM, refresh, PT_TOP, 0, -6);

  replayRoot = NULL;

  setPath(thePath);
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
  Array<FoundItem> foundItems;

  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(String::buildFullName(thePath, "*"), &data);
  BOOL success = (hFind != INVALID_HANDLE_VALUE);
  while (success)
  {
    if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
    {
      if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        foundItems.push(FoundItem(true, data.cFileName));
      else if (String::getExtension(data.cFileName).icompare(".w3g") == 0)
        foundItems.push(FoundItem(false, data.cFileName));
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);

  foundItems.sort(compItems);

  ImageLibrary* images = getApp()->getImageLibrary();
  for (int i = 0; i < foundItems.length(); i++)
  {
    TVINSERTSTRUCT tvis;
    memset(&tvis, 0, sizeof tvis);
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = parent;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvis.item.pszText = foundItems[i].name.getBuffer();
    int pos = items.length();
    tvis.item.lParam = pos;
    items.push();
    items[pos].path = String::buildFullName(thePath, foundItems[i].name);
    if (foundItems[i].folder)
    {
      items[pos].type = MAINWND_FOLDER;
      tvis.item.iImage = images->getListIndex("IconClosedFolder");
      tvis.item.iSelectedImage = images->getListIndex("IconOpenFolder");
    }
    else
    {
      items[pos].type = MAINWND_REPLAY;
      tvis.item.iImage = images->getListIndex("IconReplay");
      tvis.item.iSelectedImage = tvis.item.iImage;
    }
    items[pos].treeItem = treeView->insertItem(&tvis);
    if (foundItems[i].folder)
      populate(items[pos].treeItem, items[pos].path);
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
  treeView->deleteItem(TVI_ROOT);

  String rootName = String::getFileTitle(path);
  items.clear();

  TVINSERTSTRUCT tvis;
  memset(&tvis, 0, sizeof tvis);
  tvis.hInsertAfter = TVI_LAST;
  tvis.hParent = TVI_ROOT;
  tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
  tvis.item.pszText = "Settings";
  tvis.item.iImage = getApp()->getImageLibrary()->getListIndex("IconSettings");
  tvis.item.iSelectedImage = tvis.item.iImage;
  tvis.item.lParam = 0;

  items.push();
  items[0].type = MAINWND_SETTINGS;
  items[0].treeItem = treeView->insertItem(&tvis);

  tvis.item.pszText = rootName.getBuffer();
  tvis.item.iImage = getApp()->getImageLibrary()->getListIndex("IconClosedFolder");
  tvis.item.iSelectedImage = getApp()->getImageLibrary()->getListIndex("IconOpenFolder");
  tvis.item.lParam = 1;
  replayRoot = treeView->insertItem(&tvis);

  items.push();
  items[1].type = MAINWND_FOLDER;
  items[1].path = path;
  items[1].treeItem = replayRoot;

  populate(replayRoot, path);
}

uint32 ReplayTree::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFY:
    {
      NMTREEVIEW* hdr = (NMTREEVIEW*) lParam;
      if (hdr->hdr.code == TVN_SELCHANGED)
      {
        int pos = hdr->itemNew.lParam;
        if (pos >= 0 && pos < items.length())
        {
          if (items[pos].type == MAINWND_FOLDER)
            mainWnd->pushView(new FolderViewItem(items[pos].path));
          else if (items[pos].type == MAINWND_REPLAY)
            mainWnd->pushView(new ReplayViewItem(items[pos].path));
          else if (items[pos].type == MAINWND_SETTINGS)
            mainWnd->pushView(new SettingsViewItem());
        }
        return TRUE;
      }
    }
    break;
  }
  return M_UNHANDLED;
}
