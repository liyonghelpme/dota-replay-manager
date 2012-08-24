#include "core/app.h"
#include "graphics/imagelib.h"

#include "replaytree.h"
#include "frameui/fontsys.h"
#include "frameui/controlframes.h"
#include "frameui/dragdrop.h"

#include "ui/mainwnd.h"

#include <time.h>

struct TreeFoundItem
{
  bool folder;
  String name;
  TreeFoundItem() {}
  TreeFoundItem(bool f, String n)
  {
    folder = f;
    name = n;
  }
};
static int compItems(TreeFoundItem const& a, TreeFoundItem const& b)
{
  if (a.folder != b.folder)
    return a.folder ? -1 : 1;
  return String::smartCompare(a.name, b.name);
}

int ReplayTree::populate(HTREEITEM parent, String thePath)
{
  Array<TreeFoundItem> foundItems;

  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(String::buildFullName(thePath, "*"), &data);
  BOOL success = (hFind != INVALID_HANDLE_VALUE);
  while (success)
  {
    if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
    {
      if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        foundItems.push(TreeFoundItem(true, data.cFileName));
      else if (String::getExtension(data.cFileName).icompare(".w3g") == 0)
        foundItems.push(TreeFoundItem(false, data.cFileName));
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);

  foundItems.sort(compItems);

  ImageLibrary* images = getApp()->getImageLibrary();
  int count = 0;
  for (int i = 0; i < foundItems.length(); i++)
  {
    TVINSERTSTRUCT tvis;
    memset(&tvis, 0, sizeof tvis);
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = parent;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    String title = String::getFileTitle(foundItems[i].name);
    tvis.item.pszText = title.getBuffer();
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
      count++;
    }
    items[pos].treeItem = treeView->insertItem(&tvis);
    if (foundItems[i].folder)
    {
      int cnt = populate(items[pos].treeItem, items[pos].path);
      if (cnt == 0 && cfg.hideEmpty)
      {
        treeView->deleteItem(items[pos].treeItem);
        items.pop();
      }
      else
      {
        treeView->setItemText(items[pos].treeItem, String::format("%s (%d)", foundItems[i].name, cnt));
        count += cnt;
      }
    }
  }
  return count;
}
void ReplayTree::enumfiles(Array<FileInfo>& files, String thePath)
{
  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(String::buildFullName(thePath, "*"), &data);
  BOOL success = (hFind != INVALID_HANDLE_VALUE);
  while (success)
  {
    if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
    {
      String fpath = String::buildFullName(thePath, data.cFileName);
      if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        enumfiles(files, fpath);
      else if (String::getExtension(data.cFileName).icompare(".w3g") == 0)
      {
        FileInfo& fi = files.push();
        getFileInfo(fpath, fi);
      }
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);
}
int ReplayTree::compfiles(FileInfo const& a, FileInfo const& b)
{
  return a.ftime < b.ftime ? -1 : (a.ftime == b.ftime ? 0 : 1);
}
int ReplayTree::fillyear(Array<FileInfo>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis)
{
  int count = 0;
  int year = files[cur].time.wYear;
  while (cur < files.length() && files[cur].time.wYear == year)
  {
    int pos = cur;
    tvis.hParent = parent;
    tvis.item.lParam = items.length();
    TreeItem& ti = items.push();
    ti.path = format_systime(files[cur].ftime, "#%Y#%m");
    ti.type = -1;
    HTREEITEM hItem = (ti.treeItem = treeView->insertItem(&tvis));
    int cnt = fillmonth(files, cur, hItem, tvis);
    treeView->setItemText(hItem, String::format("%s (%d)",
      format_systime(files[pos].ftime, "%B"), cnt));
    count += cnt;
  }
  return count;
}
int ReplayTree::fillmonth(Array<FileInfo>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis)
{
  int count = 0;
  int year = files[cur].time.wYear;
  int month = files[cur].time.wMonth;
  uint64 day = uint64(_time64(NULL)) / (60 * 60 * 24);
  while (cur < files.length() && files[cur].time.wYear == year && files[cur].time.wMonth == month)
  {
    int pos = cur;
    tvis.hParent = parent;
    tvis.item.lParam = items.length();
    TreeItem& ti = items.push();
    ti.path = format_systime(files[cur].ftime, "#%Y#%m#%d");
    ti.type = -1;
    HTREEITEM hItem = (ti.treeItem = treeView->insertItem(&tvis));
    int cnt = fillday(files, cur, hItem);
    uint64 cday = files[cur].ftime / (60 * 60 * 24);
    if (day == cday)
      treeView->setItemText(hItem, String::format("Today, %s (%d)",
        format_systime(files[pos].ftime, "%d %b %Y"), cnt));
    else if (day - 1 == cday)
      treeView->setItemText(hItem, String::format("Yesterday, %s (%d)",
        format_systime(files[pos].ftime, "%d %b %Y"), cnt));
    else
      treeView->setItemText(hItem, String::format("%s (%d)",
        format_systime(files[pos].ftime, "%d %b %Y"), cnt));
    count += cnt;
  }
  return count;
}
int ReplayTree::fillday(Array<FileInfo>& files, int& cur, HTREEITEM parent)
{
  TVINSERTSTRUCT tvis;
  memset(&tvis, 0, sizeof tvis);
  tvis.hInsertAfter = TVI_LAST;
  tvis.hParent = parent;
  tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
  tvis.item.iImage = getApp()->getImageLibrary()->getListIndex("IconReplay");
  tvis.item.iSelectedImage = tvis.item.iImage;
  int count = 0;
  int year = files[cur].time.wYear;
  int month = files[cur].time.wMonth;
  int day = files[cur].time.wDay;
  while (cur < files.length() && files[cur].time.wYear == year &&
    files[cur].time.wMonth == month && files[cur].time.wDay == day)
  {
    if (cur == 0 || files[cur - 1].ftime != files[cur].ftime)
    {
      String fname = format_systime(files[cur].ftime, "%H:%M:%S ") +
        String::getFileTitle(files[cur].path);
      tvis.item.pszText = fname.getBuffer();
      tvis.item.lParam = items.length();
      TreeItem& ti = items.push();
      ti.path = files[cur].path;
      ti.type = MAINWND_REPLAY;
      ti.treeItem = treeView->insertItem(&tvis);
      count++;
    }
    cur++;
  }
  return count;
}

void ReplayTree::setPath(String thePath)
{
  delete tracker;
  path = thePath;
  rebuild();
  tracker = new DirChangeTracker(this, path, true);
}
void ReplayTree::onDirChange()
{
  PostMessage(treeView->getHandle(), WM_REBUILDTREE, 0, 0);
}
void ReplayTree::rebuild()
{
  treeView->setRedraw(false);

  updating = true;
  Dictionary<uint32> openFolders;
  String selected = "";
  if (items.length() == 0)
    openFolders.set(String(path).toLower(), 1);
  for (int i = 0; i < items.length(); i++)
  {
    int state = TreeView_GetItemState(treeView->getHandle(), items[i].treeItem, TVIS_EXPANDED | TVIS_SELECTED);
    String path = String(items[i].path).toLower();
    if (state & TVIS_SELECTED)
      selected = path;
    if (state & TVIS_EXPANDED)
      openFolders.set(path, 1);
  }
  int scrollPos = GetScrollPos(treeView->getHandle(), SB_VERT);

  treeView->deleteItem(TVI_ROOT);
  String rootName = String::getFileTitle(path);
  if (rootName.isEmpty())
    rootName = "Replays";
  else if (rootName[0] >= 'a' && rootName[0] <= 'z')
    rootName.replace(0, rootName[0] - 'a' + 'A');
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
  items[0].path = "#settings";

  tvis.item.pszText = rootName.getBuffer();
  tvis.item.iImage = getApp()->getImageLibrary()->getListIndex("IconClosedFolder");
  tvis.item.iSelectedImage = getApp()->getImageLibrary()->getListIndex("IconOpenFolder");
  tvis.item.lParam = 1;
  replayRoot = treeView->insertItem(&tvis);

  items.push();
  items[1].type = MAINWND_FOLDER;
  items[1].path = path;
  items[1].treeItem = replayRoot;

  int count = 0;
  if (cfg.byDate)
  {
    Array<FileInfo> files;
    enumfiles(files, path);
    files.sort(compfiles);

    int cur = 0;
    while (cur < files.length())
    {
      tvis.hParent = replayRoot;
      tvis.item.pszText = "";
      tvis.item.lParam = items.length();
      TreeItem& ti = items.push();
      ti.path = format_systime(files[cur].ftime, "#%Y");
      ti.type = -1;
      HTREEITEM hItem = (ti.treeItem = treeView->insertItem(&tvis));
      int cnt = fillyear(files, cur, hItem, tvis);
      treeView->setItemText(hItem, String::format("%s (%d)",
        format_systime(files[cur].ftime, "%Y"), cnt));
      count += cnt;
    }
  }
  else
    count = populate(replayRoot, path);

  treeView->setItemText(replayRoot, String::format("%s (%d)", rootName, count));

  for (int i = 0; i < items.length(); i++)
  {
    String path = String(items[i].path).toLower();
    if (selected == path)
      TreeView_SelectItem(treeView->getHandle(), items[i].treeItem);
    if (openFolders.has(path))
      TreeView_Expand(treeView->getHandle(), items[i].treeItem, TVE_EXPAND);
  }
  treeView->setRedraw(true);
  SetScrollPos(treeView->getHandle(), SB_VERT, scrollPos, TRUE);

  updating = false;
}
