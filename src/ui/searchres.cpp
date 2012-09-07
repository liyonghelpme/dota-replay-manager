#include "core/app.h"

#include "searchres.h"
#include "replay/cache.h"
#include "graphics/imagelib.h"
#include "ui/mainwnd.h"
#include "ui/viewitem.h"
#include "ui/replaytree.h"

#define WM_ADDFILE        (WM_USER+1092)

SearchResults::SearchResults(Frame* parent)
  : ListFrame(parent, 0, LVS_SHOWSELALWAYS, LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT)
{
  InitializeCriticalSection(&lock);
}
SearchResults::~SearchResults()
{
  DeleteCriticalSection(&lock);
}

void SearchResults::doBatch(BatchDialog* batch)
{
  EnterCriticalSection(&lock);
  items.clear();
  rebuild();
  LeaveCriticalSection(&lock);
  batch->run(hWnd);
  delete batch;
}
void SearchResults::addFile(String file)
{
  EnterCriticalSection(&lock);
  GameCache* cache = getApp()->getCache()->getGameNow(file);
  if (cache)
  {
    SearchItem& item = items.push();
    item.path = file;
    item.data.folder = false;
    item.data.nodata = false;
    FileInfo fi;
    getFileInfo(file, fi);
    item.data.name = String::getFileTitle(file);
    item.data.ftime = fi.ftime;
    item.data.size = fi.size;
    FolderWindow::readCacheInfo(cache, item.data, NULL);
    PostMessage(hWnd, WM_ADDFILE, 0, 0);
  }
  LeaveCriticalSection(&lock);
}

void SearchResults::getSelList(Array<String>& sel)
{
  sel.clear();
  for (int cur = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
      cur >= 0; cur = ListView_GetNextItem(hWnd, cur, LVNI_SELECTED))
    sel.push(items[cur].path);
}
void SearchResults::getColPos(int* colPos)
{
  bool colShow[colCount] = {true,
    cfg.selColumns & COL_SAVED,
    cfg.selColumns & COL_SIZE,
    cfg.selColumns & COL_NAME,
    cfg.selColumns & COL_RATIO,
    cfg.selColumns & COL_LENGTH,
    cfg.selColumns & COL_MODE
  };
  int numCols = 0;
  for (int i = 0; i < colCount; i++)
  {
    if (colShow[i])
      colPos[i] = numCols++;
    else
      colPos[i] = -1;
  }
}

void SearchResults::addItem(int pos, bool progress)
{
  int colPos[colCount];
  getColPos(colPos);
  int id = ListFrame::addItem(items[pos].data.name,
    getApp()->getImageLibrary()->getListIndex("IconReplay"), pos);
  if (colPos[colDate] >= 0 && items[pos].data.ftime)
    setItemText(id, colPos[colDate], format_systime(items[pos].data.ftime, "%c"));
  if (colPos[colSize] >= 0)
    setItemText(id, colPos[colSize], String::format("%d KB", (items[pos].data.size + 1023) / 1024));
  if (colPos[colName] >= 0)
    setItemText(id, colPos[colName], items[pos].data.gameName);
  if (colPos[colLineup] >= 0)
    setItemText(id, colPos[colLineup], items[pos].data.lineup);
  if (colPos[colLength] >= 0)
    setItemText(id, colPos[colLength], format_time(items[pos].data.gameLength));
  if (colPos[colMode] >= 0)
    setItemText(id, colPos[colMode], items[pos].data.gameMode);

  if (progress)
  {
    for (int i = 0; i < colCount; i++)
      if (colPos[i] >= 0 && cfg.colWidth[i] < 0)
        setColumnWidth(colPos[i], cfg.colWidth[i]);
  }
}

int SearchResults::compItems(SearchItem const& a, SearchItem const& b)
{
  return FolderWindow::compItems(a.data, b.data);
}
void SearchResults::rebuild()
{
  EnterCriticalSection(&lock);

  static char colNames[colCount][32] = {
    "Name", "Date", "Size", "Game name", "Lineup", "Length", "Mode"
  };
  int colPos[colCount];
  getColPos(colPos);
  int colOrder[colCount];
  int numOrder = 0;
  for (int i = 0; i < colCount; i++)
    if (colPos[cfg.colOrder[i]] >= 0)
      colOrder[numOrder++] = colPos[cfg.colOrder[i]];

  Dictionary<uint32> selected;
  for (int sel = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED); sel >= 0;
      sel = ListView_GetNextItem(hWnd, sel, LVNI_SELECTED))
    selected.set(items[sel].path, 1);
  int scrollPosX = GetScrollPos(hWnd, SB_HORZ);
  int scrollPosY = GetScrollPos(hWnd, SB_VERT);
  if (items.length() > 0)
  {
    RECT rc;
    ListView_GetItemRect(hWnd, 0, &rc, LVIR_BOUNDS);
    scrollPosY *= rc.bottom - rc.top;
  }

  setRedraw(false);
  clear();
  clearColumns();
  for (int i = 0; i < colCount; i++)
    if (colPos[i] >= 0)
      insertColumn(colPos[i], colNames[i]);
  if (colPos[colName] >= 0)
    setColumnUTF8(colPos[colName], true);
  ListView_SetColumnOrderArray(hWnd, numOrder, colOrder);

  int colSort = (cfg.colSort[0] & 0x80000000 ? ~cfg.colSort[0] : cfg.colSort[0]);
  if (colSort >= 0 && colSort < colCount && colPos[colSort] >= 0 && items.length() > 0)
  {
    HBITMAP image = NULL;
    if (cfg.colSort[0] & 0x80000000)
      image = getApp()->getImageLibrary()->getBitmap("SortUp");
    else
      image = getApp()->getImageLibrary()->getBitmap("SortDown");
    if (image)
    {
      HWND hHeader = ListView_GetHeader(hWnd);
      HDITEM hdi;
      memset(&hdi, 0, sizeof hdi);
      hdi.mask = HDI_FORMAT;
      Header_GetItem(hHeader, colSort, &hdi);
      hdi.mask |= HDI_BITMAP;
      hdi.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
      hdi.hbm = image;
      Header_SetItem(hHeader, colSort, &hdi);
    }
  }

  items.sort(compItems);
  for (int i = 0; i < items.length(); i++)
    addItem(i, false);

  for (int i = 0; i < items.length(); i++)
    if (selected.has(items[i].path))
      ListView_SetItemState(hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
  for (int i = 0; i < colCount; i++)
    if (colPos[i] >= 0)
      setColumnWidth(colPos[i], cfg.colWidth[i]);
  setRedraw(true);
  ListView_Scroll(hWnd, scrollPosX, scrollPosY);

  LeaveCriticalSection(&lock);
}

uint32 SearchResults::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_ADDFILE:
    EnterCriticalSection(&lock);
    while (getCount() < items.length())
      addItem(getCount(), true);
    LeaveCriticalSection(&lock);
    return 0;
  case WM_NOTIFY:
    {
      NMHDR* pnm = (NMHDR*) lParam;
      if (pnm->code == LVN_ITEMACTIVATE)
      {
        NMITEMACTIVATE* pia = (NMITEMACTIVATE*) pnm;
        if (pia->iItem >= 0 && pia->iItem < items.length())
        {
          SendMessage(getApp()->getMainWindow(), WM_PUSHVIEW,
            (uint32) new ReplayViewItem(items[pia->iItem].path), 0);
        }
        return 0;
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
        PostMessage(hWnd, WM_POSTHEADERDRAG, 0, 0);
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
        return 0;
      }
    }
    break;
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
      ListView_GetColumnOrderArray(hWnd, count, colOrder);
      int pos = 0;
      for (int i = 0; i < colCount; i++)
        if (colShow[cfg.colOrder[i]])
          cfg.colOrder[i] = colUnpos[colOrder[pos++]];
    }
    return 0;
  }
  return ListFrame::onMessage(message, wParam, lParam);
}
