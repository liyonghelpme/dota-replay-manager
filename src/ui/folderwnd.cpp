#include "core/app.h"
#include <shlobj.h>

#include "folderwnd.h"
#include "ui/mainwnd.h"
#include "graphics/imagelib.h"
#include "ui/batchdlg.h"

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

#define ID_MENU_OPEN          1560
#define ID_MENU_BACKUP        1561
#define ID_MENU_CUT           1562
#define ID_MENU_COPY          1563
#define ID_MENU_PASTE         1564
#define ID_MENU_RENAME        1565
#define ID_MENU_DELETE        1566
#define ID_MENU_FOLDER        1567

/////////////////////////////////////////////////////////

#define ADDITEM(i,text,id)                \
  mii.dwTypeData = text;                  \
  mii.cch = strlen(mii.dwTypeData);       \
  mii.wID = id;                           \
  InsertMenuItem(ctxMenu, i, TRUE, &mii);

FolderWindow::FolderWindow(Frame* parent, MainWnd* pMainWnd)
  : Frame(parent)
{
  selfDrag = false;
  tracker = NULL;
  filler = NULL;
  mainWnd = pMainWnd;
  path = "";
  list = new DropListFrame(this, 0, LVS_EDITLABELS | LVS_SHOWSELALWAYS,
    LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
  list->setAllPoints();

  Image* sortImg = getApp()->getImageLibrary()->getImage("SortUp");
  if (sortImg)
    sortImg->replaceColor(sortImg->getPixel(0, 0), 0xFF000000 |
      Image::flip_color(GetSysColor(COLOR_MENU)));
  sortImg = getApp()->getImageLibrary()->getImage("SortDown");
  if (sortImg)
    sortImg->replaceColor(sortImg->getPixel(0, 0), 0xFF000000 |
      Image::flip_color(GetSysColor(COLOR_MENU)));

  ctxMenu = CreatePopupMenu();

  MENUITEMINFO miiSep;
  memset(&miiSep, 0, sizeof miiSep);
  miiSep.cbSize = sizeof miiSep;
  miiSep.fMask = MIIM_FTYPE;
  miiSep.fType = MFT_SEPARATOR;

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;

  ADDITEM(0, "Open", ID_MENU_OPEN);
  mii.fMask &= ~MIIM_STATE;

  ADDITEM(1, "Backup", ID_MENU_BACKUP);
  InsertMenuItem(ctxMenu, 2, TRUE, &miiSep);
  ADDITEM(3, "Copy", ID_MENU_COPY);
  ADDITEM(4, "Paste", ID_MENU_PASTE);
  InsertMenuItem(ctxMenu, 5, TRUE, &miiSep);
  ADDITEM(6, "Rename", ID_MENU_RENAME);
  ADDITEM(7, "Delete", ID_MENU_DELETE);
  InsertMenuItem(ctxMenu, 8, TRUE, &miiSep);
  ADDITEM(9, "New Folder", ID_MENU_FOLDER);
}
FolderWindow::~FolderWindow()
{
  DestroyMenu(ctxMenu);
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
  case WM_CONTEXTMENU:
    {
      POINT pt;
      GetCursorPos(&pt);

      int count = ListView_GetSelectedCount(list->getHandle());
      int item = ListView_GetNextItem(list->getHandle(), -1, LVNI_SELECTED);
      if (item >= 0)
      {
        int pos = list->getItemParam(item);
        while (count > 1 && item >= 0 && items[pos].type != ITEM_FOLDER &&
          items[pos].type != ITEM_REPLAY)
        {
          item = ListView_GetNextItem(list->getHandle(), item, LVNI_SELECTED);
          pos = (item > 0 ? list->getItemParam(item) : -1);
        }

        EnableMenuItem(ctxMenu, ID_MENU_OPEN, count > 1 ? MF_GRAYED : MF_ENABLED);
        if (pos >= 0 && (items[pos].type == ITEM_FOLDER || items[pos].type == ITEM_REPLAY))
        {
          EnableMenuItem(ctxMenu, ID_MENU_BACKUP, MF_ENABLED);
          EnableMenuItem(ctxMenu, ID_MENU_COPY, MF_ENABLED);
          EnableMenuItem(ctxMenu, ID_MENU_DELETE, MF_ENABLED);
          EnableMenuItem(ctxMenu, ID_MENU_RENAME, count > 1 ? MF_GRAYED : MF_ENABLED);
        }
        else
        {
          EnableMenuItem(ctxMenu, ID_MENU_BACKUP, MF_GRAYED);
          EnableMenuItem(ctxMenu, ID_MENU_COPY, MF_GRAYED);
          EnableMenuItem(ctxMenu, ID_MENU_DELETE, MF_GRAYED);
          EnableMenuItem(ctxMenu, ID_MENU_RENAME, MF_GRAYED);
        }
      }
      else
      {
        EnableMenuItem(ctxMenu, ID_MENU_OPEN, MF_GRAYED);
        EnableMenuItem(ctxMenu, ID_MENU_BACKUP, MF_GRAYED);
        EnableMenuItem(ctxMenu, ID_MENU_COPY, MF_GRAYED);
        EnableMenuItem(ctxMenu, ID_MENU_DELETE, MF_GRAYED);
        EnableMenuItem(ctxMenu, ID_MENU_RENAME, MF_GRAYED);
      }
      ClipboardReader cbReader(CF_HDROP);
      EnableMenuItem(ctxMenu, ID_MENU_PASTE, cbReader.getData() ? MF_ENABLED : MF_GRAYED);

      int result = TrackPopupMenuEx(ctxMenu, TPM_HORIZONTAL | TPM_LEFTALIGN |
        TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, list->getHandle(), NULL);

      switch (result)
      {
      case ID_MENU_OPEN:
        {
          NMITEMACTIVATE ia;
          memset(&ia, 0, sizeof ia);
          ia.hdr.code = LVN_ITEMACTIVATE;
          ia.hdr.hwndFrom = list->getHandle();
          ia.iItem = item;
          onMessage(WM_NOTIFY, 0, (uint32) &ia);
        }
        break;
      case ID_MENU_BACKUP:
        {
          BatchDialog batch(BatchDialog::mCopy);
          for (int item = ListView_GetNextItem(list->getHandle(), -1, LVNI_SELECTED);
              item >= 0; item = ListView_GetNextItem(list->getHandle(), item, LVNI_SELECTED))
          {
            int pos = list->getItemParam(item);
            if (pos >= 0 && items[pos].type == ITEM_REPLAY)
              batch.addReplay(items[pos].path);
            else if (pos >= 0 && items[pos].type == ITEM_FOLDER)
              batch.addFolder(items[pos].path);
          }
          batch.run(list->getHandle());
        }
        break;
      case ID_MENU_COPY:
      case ID_MENU_PASTE:
      case ID_MENU_DELETE:
      case ID_MENU_RENAME:
        {
          NMLVKEYDOWN kd;
          memset(&kd, 0, sizeof kd);
          kd.hdr.code = LVN_KEYDOWN;
          kd.hdr.hwndFrom = list->getHandle();
          if (result == ID_MENU_COPY)
            kd.wVKey = 'C';
          else if (result == ID_MENU_PASTE)
            kd.wVKey = 'V';
          else if (result == ID_MENU_DELETE)
            kd.wVKey = VK_DELETE;
          else if (result == ID_MENU_RENAME)
            kd.wVKey = VK_F2;
          onMessage(WM_NOTIFY, VK_CONTROL, (uint32) &kd);
        }
        break;
      case ID_MENU_FOLDER:
        {
          String newdir = "New Folder";
          String fulldir = String::buildFullName(path, newdir);
          FileInfo fi;
          for (int num = 1; pathFileExists(fulldir); num++)
          {
            newdir = String::format("New Folder (%d)", num);
            fulldir = String::buildFullName(path, newdir);
          }
          if (CreateDirectory(fulldir, NULL))
          {
            int pos = list->addItem(newdir,
              getApp()->getImageLibrary()->getListIndex("IconClosedFolder"), items.length());
            ViewItem& item = items.push();
            item.path = fulldir;
            item.pos = pos;
            item.type = ITEM_FOLDER;
            ListView_EditLabel(list->getHandle(), pos);
          }
        }
        break;
      }
    }
    break;
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
          {
            selfDrag = true;
            DoDragDrop(CF_HDROP, data, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
            selfDrag = false;
          }
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
          if (SHFileOperationEx(&op) == 0)
          {
            items[pos].path = String::buildFullName(String::getPath(items[pos].path), di->item.pszText);
            if (items[pos].type == ITEM_REPLAY)
              items[pos].path += ".w3g";
            return TRUE;
          }
        }
        rebuild();
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
        bool controlKey = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || wParam == VK_CONTROL);
        if (kd->wVKey == 'C' && controlKey)
        {
          Array<String> sel;
          getSelList(sel);
          if (sel.length())
          {
            HGLOBAL hDrop = CreateFileDrop(sel);
            SetClipboard(CF_HDROP, hDrop);
          }
        }
        else if (kd->wVKey == 'V' && controlKey)
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
            SHFileOperationEx(&fileop);
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
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.stateMask = LVIS_SELECTED;
          ListView_GetItem(list->getHandle(), &lvi);
          if (lvi.lParam < 0 || lvi.lParam >= items.length() ||
              (items[lvi.lParam].type != ITEM_UPFOLDER && items[lvi.lParam].type != ITEM_FOLDER))
            ht.iItem = -1;
          if (selfDrag && (lvi.state & LVIS_SELECTED))
            ht.iItem = -1;
        }
        if (wParam && selfDrag && ht.iItem == -1 && *(DWORD*) wParam == DROPEFFECT_MOVE)
          *(DWORD*) wParam = DROPEFFECT_NONE;
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
    if (ListView_GetEditControl(list->getHandle()) == NULL)
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
  SHFileOperationEx(&op);
  if (df->fWide)
    delete opFrom;
  GlobalUnlock(hDrop);
}
