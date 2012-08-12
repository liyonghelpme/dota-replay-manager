#include "core/app.h"
#include "graphics/imagelib.h"
#include "shlobj.h"

#include "replaytree.h"
#include "frameui/fontsys.h"
#include "frameui/controlframes.h"
#include "frameui/dragdrop.h"

#include "ui/mainwnd.h"

#include <time.h>

#define IDC_BYDATE          136
#define IDC_REFRESH         137

#define WM_REBUILDTREE      (WM_USER+910)

class DropTreeViewFrame : public TreeViewFrame
{
protected:
  DropTarget* target;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  HTREEITEM highlight;
  DropTreeViewFrame(Frame* parent, int id = 0, int style = 0);
};
uint32 DropTreeViewFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DESTROY)
    delete target;
  else if (message == WM_REBUILDTREE)
    return getParent()->notify(message, wParam, lParam);
  return TreeViewFrame::onMessage(message, wParam, lParam);
}
DropTreeViewFrame::DropTreeViewFrame(Frame* parent, int id, int style)
  : TreeViewFrame(parent, id, style)
{
  highlight = NULL;
  target = new DropTarget(this, CF_HDROP, DROPEFFECT_MOVE | DROPEFFECT_COPY);
}

ReplayTree::ReplayTree(String thePath, MainWnd* parent)
  : Frame(parent)
{
  mainWnd = parent;
  tracker = NULL;
  updating = false;

  treeView = new DropTreeViewFrame(this, 0, TVS_HASBUTTONS | TVS_HASLINES |
    TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_EDITLABELS | WS_HSCROLL | WS_TABSTOP);
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

int ReplayTree::populate(HTREEITEM parent, String thePath)
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
      treeView->setItemText(items[pos].treeItem, String::format("%s (%d)", foundItems[i].name, cnt));
      count += cnt;
    }
  }
  return count;
}
void ReplayTree::enumfiles(Array<DateItem>& files, String thePath)
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
        HANDLE hFile = CreateFile(fpath, GENERIC_READ, FILE_SHARE_DELETE |
          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile)
        {
          FILETIME ft;
          GetFileTime(hFile, NULL, NULL, &ft);
          CloseHandle(hFile);
          DateItem& di = files.push();
          di.path = fpath;
          di.ftime = uint64(ft.dwLowDateTime) | (uint64(ft.dwHighDateTime) << 32);
          di.ftime = di.ftime / 10000000ULL - 11644473600ULL;
          FileTimeToSystemTime(&ft, &di.time);
        }
      }
    }
    success = FindNextFile(hFind, &data);
  }
  FindClose(hFind);
}
int ReplayTree::compfiles(DateItem const& a, DateItem const& b)
{
  return a.ftime < b.ftime ? -1 : (a.ftime == b.ftime ? 0 : 1);
}
int ReplayTree::fillyear(Array<DateItem>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis)
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
    ti.type = 0;
    HTREEITEM hItem = (ti.treeItem = treeView->insertItem(&tvis));
    int cnt = fillmonth(files, cur, hItem, tvis);
    treeView->setItemText(hItem, String::format("%s (%d)",
      format_systime(files[pos].ftime, "%B"), cnt));
    count += cnt;
  }
  return count;
}
int ReplayTree::fillmonth(Array<DateItem>& files, int& cur, HTREEITEM parent, TVINSERTSTRUCT& tvis)
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
    ti.type = 0;
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
int ReplayTree::fillday(Array<DateItem>& files, int& cur, HTREEITEM parent)
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
  if (cfg::byDate)
  {
    Array<DateItem> files;
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
      ti.type = 0;
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

uint32 ReplayTree::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFY:
    {
      NMTREEVIEW* hdr = (NMTREEVIEW*) lParam;
      if (hdr->hdr.code == TVN_SELCHANGED)
      {
        if (!updating)
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
        }
        return TRUE;
      }
      else if (hdr->hdr.code == TVN_BEGINDRAG)
      {
        int pos = hdr->itemNew.lParam;
        if (pos >= 0 && pos < items.length() && hdr->itemNew.hItem != replayRoot &&
          (items[pos].type == MAINWND_FOLDER || items[pos].type == MAINWND_REPLAY))
        {
          String path = items[pos].path;
          uint32 size = sizeof(DROPFILES) + path.length() + 2;
          HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, size);
          if (data)
          {
            uint8* ptr = (uint8*) GlobalLock(data);
            DROPFILES* df = (DROPFILES*) ptr;
            memset(df, 0, sizeof(DROPFILES));
            df->pFiles = sizeof(DROPFILES);
            memcpy(ptr + sizeof(DROPFILES), path.c_str(), path.length() + 1);
            ptr[size - 1] = 0;
            GlobalUnlock(data);
            DoDragDrop(CF_HDROP, data, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
          }
        }
        return TRUE;
      }
      else if (hdr->hdr.code == TVN_BEGINLABELEDIT)
      {
        NMTVDISPINFO* di = (NMTVDISPINFO*) lParam;
        if (di->item.pszText && di->item.lParam >= 0 && di->item.lParam < items.length() &&
          di->item.hItem != replayRoot &&
          (items[di->item.lParam].type == MAINWND_FOLDER || items[di->item.lParam].type == MAINWND_REPLAY))
        {
          HWND hEdit = (HWND) SendMessage(treeView->getHandle(), TVM_GETEDITCONTROL, 0, 0);
          if (hEdit)
          {
            if (items[di->item.lParam].type == MAINWND_FOLDER)
              SetWindowText(hEdit, String::getFileName(items[di->item.lParam].path));
            else
              SetWindowText(hEdit, String::getFileTitle(items[di->item.lParam].path));
          }
          return 0;
        }
        return TRUE;
      }
      else if (hdr->hdr.code == TVN_ENDLABELEDIT)
      {
        NMTVDISPINFO* di = (NMTVDISPINFO*) lParam;
        if (di->item.pszText && di->item.lParam >= 0 && di->item.lParam < items.length() &&
          di->item.hItem != replayRoot &&
          (items[di->item.lParam].type == MAINWND_FOLDER || items[di->item.lParam].type == MAINWND_REPLAY))
        {
          String src = items[di->item.lParam].path;
          String dst = String::buildFullName(String::getPath(src), di->item.pszText);
          src += '\0';
          if (items[di->item.lParam].type == MAINWND_REPLAY)
            dst += ".w3g";
          dst += '\0';
          SHFILEOPSTRUCT op;
          memset(&op, 0, sizeof op);
          op.wFunc = FO_RENAME;
          op.pFrom = src;
          op.pTo = dst;
          if (SHFileOperation(&op) == 0)
            return TRUE;
        }
        return FALSE;
      }
    }
    break;
  case WM_REBUILDTREE:
    rebuild();
    break;
  case WM_DRAGOVER:
  case WM_DRAGLEAVE:
    {
      TVHITTESTINFO ht;
      TVITEM tvi;
      memset(&tvi, 0, sizeof tvi);
      if (message == WM_DRAGOVER)
      {
        ht.pt.x = LOWORD(lParam);
        ht.pt.y = HIWORD(lParam);
        TreeView_HitTest(treeView->getHandle(), &ht);
        if (ht.hItem)
        {
          tvi.hItem = ht.hItem;
          tvi.mask = TVIF_PARAM;
          TreeView_GetItem(treeView->getHandle(), &tvi);
          if (tvi.lParam < 0 || tvi.lParam >= items.length() || items[tvi.lParam].type != MAINWND_FOLDER)
            ht.hItem = NULL;
        }
      }
      else
        ht.hItem = NULL;
      if (ht.hItem != treeView->highlight)
      {
        TVITEM tvi;
        memset(&tvi, 0, sizeof tvi);
        tvi.mask = TVIF_STATE;
        tvi.stateMask = TVIS_DROPHILITED;
        if (treeView->highlight)
        {
          tvi.hItem = treeView->highlight;
          TreeView_SetItem(treeView->getHandle(), &tvi);
        }
        if (ht.hItem)
        {
          tvi.state = TVIS_DROPHILITED;
          tvi.hItem = ht.hItem;
          TreeView_SetItem(treeView->getHandle(), &tvi);
        }
        treeView->highlight = ht.hItem;
      }
    }
    break;
  case WM_DRAGDROP:
    if (lParam == DROPEFFECT_MOVE || lParam == DROPEFFECT_COPY)
    {
      String opTo = path;
      if (treeView->highlight)
      {
        TVITEM tvi;
        memset(&tvi, 0, sizeof tvi);
        tvi.mask = TVIF_PARAM;
        tvi.hItem = treeView->highlight;
        TreeView_GetItem(treeView->getHandle(), &tvi);
        if (tvi.lParam >= 0 && tvi.lParam < items.length() && items[tvi.lParam].type == MAINWND_FOLDER)
          opTo = items[tvi.lParam].path;
      }
      DROPFILES* df = (DROPFILES*) GlobalLock((HGLOBAL) wParam);
      char* opFrom;
      if (df->fWide)
      {
        wchar_t* opFromW = (wchar_t*) ((char*) df + df->pFiles);
        int length = 0;
        while (opFromW[length])
        {
          while (opFromW[length])
            length++;
          length++;
        }
        length++;
        int bufsize = WideCharToMultiByte(CP_ACP, 0, opFromW, length, NULL, 0, NULL, NULL);
        opFrom = new char[bufsize];
        WideCharToMultiByte(CP_ACP, 0, opFromW, length, opFrom, bufsize, NULL, NULL);
      }
      else
        opFrom = (char*) df + df->pFiles;
      opTo += '\0';

      SHFILEOPSTRUCT op;
      memset(&op, 0, sizeof op);
      op.wFunc = (lParam == DROPEFFECT_MOVE ? FO_MOVE : FO_COPY);
      op.pFrom = opFrom;
      op.pTo = opTo;
      SHFileOperation(&op);
      if (df->fWide)
        delete opFrom;
      GlobalUnlock((HGLOBAL) wParam);
    }
    return 0;
  case WM_COMMAND:
    if (HIWORD(wParam) == BN_CLICKED)
    {
      if (LOWORD(wParam) == IDC_REFRESH)
      {
        rebuild();
        return 0;
      }
      else if (LOWORD(wParam) == IDC_BYDATE)
      {
        cfg::byDate = byDate->checked();
        rebuild();
      }
    }
    break;
  default:
    return M_UNHANDLED;
  }
  return 0;
}

void ReplayTree::setCurFile(String path)
{
  for (int i = 0; i < items.length(); i++)
  {
    if (items[i].path.icompare(path) == 0)
    {
      updating = true;
      TreeView_SelectItem(treeView->getHandle(), items[i].treeItem);
      updating = false;
      return;
    }
  }
}
