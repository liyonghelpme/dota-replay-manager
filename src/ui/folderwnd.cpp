#include "core/app.h"
#include <shlobj.h>

#include "folderwnd.h"
#include "ui/mainwnd.h"

uint32 DropListFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DESTROY)
  {
    delete target;
    target = NULL;
  }
  else if (message == WM_REBUILDLIST || message == WM_UPDATELISTITEM ||
           message == WM_POSTHEADERDRAG)
    getParent()->notify(message, wParam, lParam);
  return ListFrame::onMessage(message, wParam, lParam);
}
DropListFrame::DropListFrame(Frame* parent, int id, int style, int styleEx)
  : ListFrame(parent, id, style, styleEx)
{
  highlight = -1;
  target = new DropTarget(this, CF_HDROP, DROPEFFECT_MOVE | DROPEFFECT_COPY);
}

/////////////////////////////////////////////////////////

FolderWindow::FolderWindow(Frame* parent, MainWnd* pMainWnd)
  : Frame(parent)
{
  tracker = NULL;
  filler = NULL;
  mainWnd = pMainWnd;
  path = "";
  list = new DropListFrame(this, 0, LVS_EDITLABELS | LVS_SHOWSELALWAYS,
    LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
  list->setAllPoints();
}
FolderWindow::~FolderWindow()
{
  delete tracker;
  releaseFillers();
}

void FolderWindow::setPath(String thePath)
{
  delete tracker;
  path = thePath;
  rebuild();
  tracker = new DirChangeTracker(this, path, true);
}

void FolderWindow::onDirChange()
{
  PostMessage(list->getHandle(), WM_REBUILDLIST, 0, 0);
}

void FolderWindow::getSelList(Array<String>& sel)
{
  for (int cur = ListView_GetNextItem(list->getHandle(), -1, LVNI_SELECTED);
    cur >= 0; cur = ListView_GetNextItem(list->getHandle(), cur, LVNI_SELECTED))
  {
    int pos = list->getItemParam(cur);
    if (pos >= 0 && pos < items.length() &&
        (items[pos].type == ITEM_FOLDER || items[pos].type == ITEM_REPLAY))
      sel.push(items[pos].path);
  }
}

uint32 FolderWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFY:
    {
      NMHDR* pnm = (NMHDR*) lParam;
      if (pnm->code == LVN_ITEMACTIVATE)
      {
        NMITEMACTIVATE* pia = (NMITEMACTIVATE*) pnm;
        int pos = -1;
        if (pia->iItem >= 0)
          pos = list->getItemParam(pia->iItem);
        if (pos >= 0 && pos < items.length())
        {
          if (items[pos].type == ITEM_UPFOLDER || items[pos].type == ITEM_FOLDER)
            mainWnd->pushView(new FolderViewItem(items[pos].path));
          else if (items[pos].type == ITEM_REPLAY)
            mainWnd->pushView(new ReplayViewItem(items[pos].path));
        }
      }
      else if (pnm->code == LVN_BEGINDRAG)
      {
        Array<String> sel;
        getSelList(sel);
        if (sel.length())
        {
          HGLOBAL data = CreateFileDrop(sel);
          if (data)
            DoDragDrop(CF_HDROP, data, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
        }
        return TRUE;
      }
      else if (pnm->code == LVN_BEGINLABELEDIT)
      {
        NMLVDISPINFO* di = (NMLVDISPINFO*) lParam;
        int pos = -1;
        if (di->item.iItem >= 0)
          pos = list->getItemParam(di->item.iItem);
        if (pos >= 0 && pos < items.length() &&
            (items[pos].type == ITEM_FOLDER || items[pos].type == ITEM_REPLAY))
          return FALSE;
        return TRUE;
      }
      else if (pnm->code == LVN_ENDLABELEDIT)
      {
        NMLVDISPINFO* di = (NMLVDISPINFO*) lParam;
        int pos = -1;
        if (di->item.iItem >= 0)
          pos = list->getItemParam(di->item.iItem);
        if (di->item.pszText && pos >= 0 && pos < items.length() &&
          (items[pos].type == ITEM_FOLDER || items[pos].type == ITEM_REPLAY))
        {
          String src = items[pos].path;
          String dst = String::buildFullName(String::getPath(src), di->item.pszText);
          src += '\0';
          if (items[pos].type == ITEM_REPLAY)
            dst += ".w3g";
          dst += '\0';
          SHFILEOPSTRUCT op;
          memset(&op, 0, sizeof op);
          op.wFunc = FO_RENAME;
          op.pFrom = src;
          op.pTo = dst;
          if (SHFileOperation(&op) == 0)
          {
            items[pos].path = String::buildFullName(String::getPath(items[pos].path), di->item.pszText);
            if (items[pos].type == ITEM_REPLAY)
              items[pos].path += ".w3g";
            return TRUE;
          }
        }
        return FALSE;
      }
      else if (pnm->code == LVN_COLUMNCLICK)
      {
        NMLISTVIEW* lv = (NMLISTVIEW*) lParam;
        bool colShow[colCount] = {true,
          cfg.selColumns & COL_SAVED,
          cfg.selColumns & COL_SIZE,
          cfg.selColumns & COL_NAME,
          cfg.selColumns & COL_RATIO,
          cfg.selColumns & COL_LENGTH,
          cfg.selColumns & COL_MODE
        };
        int col = lv->iSubItem;
        for (int i = 0; i < col && col < colCount - 1; i++)
          if (!colShow[i])
            col++;
        int pos = 0;
        while (pos < colCount - 1 && cfg.colSort[pos] != col && cfg.colSort[pos] != ~col)
          pos++;
        if (pos == 0)
          cfg.colSort[0] = ~cfg.colSort[0];
        else
        {
          for (int i = pos; i > 0; i--)
            cfg.colSort[i] = cfg.colSort[i - 1];
          cfg.colSort[0] = col;
        }
        rebuild();
      }
      else if (pnm->code == HDN_ENDDRAG)
        PostMessage(list->getHandle(), WM_POSTHEADERDRAG, 0, 0);
      else if (pnm->code == HDN_ENDTRACK)
      {
        NMHEADER* nhdr = (NMHEADER*) pnm;
        bool colShow[colCount] = {true,
          cfg.selColumns & COL_SAVED,
          cfg.selColumns & COL_SIZE,
          cfg.selColumns & COL_NAME,
          cfg.selColumns & COL_RATIO,
          cfg.selColumns & COL_LENGTH,
          cfg.selColumns & COL_MODE
        };
        int count = 0;
        int colUnpos[colCount];
        for (int i = 0; i < colCount; i++)
          if (colShow[i])
            colUnpos[count++] = i;
        if (nhdr->iItem >= 0 && nhdr->iItem < count &&
          nhdr->pitem && nhdr->pitem->mask & HDI_WIDTH)
        {
          int col = colUnpos[nhdr->iItem];
          cfg.colWidth[col] = nhdr->pitem->cxy;
        }
      }
      else if (pnm->code == LVN_KEYDOWN)
      {
        NMLVKEYDOWN* kd = (NMLVKEYDOWN*) lParam;
        if (kd->wVKey == 'C' && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
          Array<String> sel;
          getSelList(sel);
          if (sel.length())
          {
            HGLOBAL hDrop = CreateFileDrop(sel);
            SetClipboard(CF_HDROP, hDrop);
          }
        }
        else if (kd->wVKey == 'V' && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
          ClipboardReader clip(CF_HDROP);
          HGLOBAL hDrop = clip.getData();
          if (hDrop)
            doPaste(hDrop, DROPEFFECT_COPY, path);
        }
        else if (kd->wVKey == VK_DELETE)
        {
          Array<String> sel;
          getSelList(sel);
          if (sel.length())
          {
            char* str = FileListToString(sel);
            SHFILEOPSTRUCT fileop;
            memset(&fileop, 0, sizeof fileop);
            fileop.wFunc = FO_DELETE;
            fileop.pFrom = str;
            SHFileOperation(&fileop);
          }
        }
        else if (kd->wVKey == VK_F2)
        {
          SetFocus(list->getHandle());
          int sel = ListView_GetNextItem(list->getHandle(), -1, LVNI_FOCUSED);
          if (sel < 0)
            sel = 0;
          ListView_EditLabel(list->getHandle(), sel);
        }
      }
    }
    break;
  case WM_DRAGOVER:
  case WM_DRAGLEAVE:
    {
      LVHITTESTINFO ht;
      LVITEM lvi;
      memset(&lvi, 0, sizeof lvi);
      if (message == WM_DRAGOVER)
      {
        ht.pt.x = LOWORD(lParam);
        ht.pt.y = HIWORD(lParam);
        ListView_HitTest(list->getHandle(), &ht);
        if (ht.iItem >= 0)
        {
          lvi.iItem = ht.iItem;
          lvi.mask = LVIF_PARAM;
          ListView_GetItem(list->getHandle(), &lvi);
          if (lvi.lParam < 0 || lvi.lParam >= items.length() ||
              (items[lvi.lParam].type != ITEM_UPFOLDER && items[lvi.lParam].type != ITEM_FOLDER))
            ht.iItem = -1;
        }
      }
      else
        ht.iItem = -1;
      if (ht.iItem != list->highlight)
      {
        lvi.mask = LVIF_STATE;
        lvi.stateMask = LVIS_DROPHILITED;
        if (list->highlight >= 0)
        {
          lvi.iItem = list->highlight;
          ListView_SetItem(list->getHandle(), &lvi);
        }
        if (ht.iItem >= 0)
        {
          lvi.state = LVIS_DROPHILITED;
          lvi.iItem = ht.iItem;
          ListView_SetItem(list->getHandle(), &lvi);
        }
        list->highlight = ht.iItem;
      }
    }
    break;
  case WM_DRAGDROP:
    if (lParam == DROPEFFECT_MOVE || lParam == DROPEFFECT_COPY)
    {
      String opTo = path;
      if (list->highlight >= 0)
      {
        int param = list->getItemParam(list->highlight);
        if (param >= 0 && param < items.length() &&
            (items[param].type == ITEM_UPFOLDER || items[param].type == ITEM_FOLDER))
          opTo = items[param].path;
      }
      doPaste((HGLOBAL) wParam, lParam, opTo);
    }
    return 0;
  case WM_POSTHEADERDRAG:
    {
      bool colShow[colCount] = {true,
        cfg.selColumns & COL_SAVED,
        cfg.selColumns & COL_SIZE,
        cfg.selColumns & COL_NAME,
        cfg.selColumns & COL_RATIO,
        cfg.selColumns & COL_LENGTH,
        cfg.selColumns & COL_MODE
      };
      int count = 0;
      int colUnpos[colCount];
      for (int i = 0; i < colCount; i++)
        if (colShow[i])
          colUnpos[count++] = i;
      int colOrder[colCount];
      ListView_GetColumnOrderArray(list->getHandle(), count, colOrder);
      int pos = 0;
      for (int i = 0; i < colCount; i++)
        if (colShow[cfg.colOrder[i]])
          cfg.colOrder[i] = colUnpos[colOrder[pos++]];
    }
    break;
  case WM_REBUILDLIST:
    rebuild();
    break;
  case WM_UPDATELISTITEM:
    updateListItem(wParam, lParam);
    break;
  default:
    return M_UNHANDLED;
  }
  return 0;
}

void FolderWindow::doPaste(HGLOBAL hDrop, uint32 effect, String opTo)
{
  DROPFILES* df = (DROPFILES*) GlobalLock(hDrop);
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
  op.wFunc = (effect == DROPEFFECT_MOVE ? FO_MOVE : FO_COPY);
  op.pFrom = opFrom;
  op.pTo = opTo;
  if (effect == DROPEFFECT_COPY)
    op.fFlags = FOF_RENAMEONCOLLISION;
  SHFileOperation(&op);
  if (df->fWide)
    delete opFrom;
  GlobalUnlock(hDrop);
}
