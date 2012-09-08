#include "core/app.h"
#include "graphics/imagelib.h"
#include <shlobj.h>

#include "replaytree.h"
#include "frameui/fontsys.h"
#include "frameui/controlframes.h"
#include "frameui/dragdrop.h"

#include "ui/mainwnd.h"

#define IDC_BYDATE          136
#define IDC_REFRESH         137

uint32 DropTreeViewFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DESTROY)
  {
    delete target;
    target = NULL;
  }
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
  selfDrag = NULL;
  hasSearchRes = false;

  treeView = new DropTreeViewFrame(this, 0, TVS_HASBUTTONS | TVS_HASLINES |
    TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_EDITLABELS | WS_HSCROLL | WS_TABSTOP);
  treeView->setImageList(getApp()->getImageLibrary()->getImageList(), TVSIL_NORMAL);
  treeView->setItemHeight(18);

  byDate = new ButtonFrame("By date", this, IDC_BYDATE, BS_AUTOCHECKBOX);
  byDate->setSize(100, 23);
  byDate->setPoint(PT_BOTTOMLEFT, 0, 0);
  byDate->setCheck(cfg.byDate);
  ButtonFrame* refresh = new ButtonFrame("Refresh", this, IDC_REFRESH);
  refresh->setSize(100, 23);
  refresh->setPoint(PT_BOTTOMRIGHT, 0, 0);

  treeView->setPoint(PT_TOPLEFT, 0, 0);
  treeView->setPoint(PT_TOPRIGHT, 0, 0);
  treeView->setPoint(PT_BOTTOM, refresh, PT_TOP, 0, -6);

  replayRoot = NULL;

  setPath(thePath);
}
ReplayTree::~ReplayTree()
{
  delete tracker;
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
            else if (items[pos].type == MAINWND_SEARCH)
              mainWnd->pushView(new SearchViewItem());
            else if (items[pos].type == MAINWND_SEARCHRES)
              mainWnd->pushView(new SearchResViewItem());
            else if (items[pos].type == MAINWND_HEROCHART)
              mainWnd->pushView(new HeroChartViewItem());
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
          HGLOBAL data = CreateFileDrop(items[pos].path);
          if (data)
          {
            selfDrag = hdr->itemNew.hItem;
            DoDragDrop(CF_HDROP, data, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
            selfDrag = NULL;
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
          if (SHFileOperationEx(&op) == 0)
          {
            items[di->item.lParam].path = String::buildFullName(
              String::getPath(items[di->item.lParam].path), di->item.pszText);
            if (items[di->item.lParam].type == MAINWND_REPLAY)
              items[di->item.lParam].path += ".w3g";
            return TRUE;
          }
        }
        return FALSE;
      }
    }
    break;
  case WM_REBUILDTREE:
    rebuild(wParam != 0);
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
          if (selfDrag == ht.hItem || (selfDrag &&
              TreeView_GetParent(treeView->getHandle(), selfDrag) == ht.hItem))
            ht.hItem = NULL;
        }
        if (wParam && selfDrag && ht.hItem == NULL && *(DWORD*) wParam == DROPEFFECT_MOVE)
          *(DWORD*) wParam = DROPEFFECT_NONE;
      }
      else
        ht.hItem = NULL;
      if (ht.hItem != treeView->highlight)
      {
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
      SHFileOperationEx(&op);
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
        cfg.byDate = byDate->checked();
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
  TreeView_SelectItem(treeView->getHandle(), NULL);
}
