#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"

#include "folderwnd.h"

#include "replay/cache.h"

struct FolderFillerTask
{
  String path;
  int pos;
  FolderFoundItem item;
};
struct FolderWindow::FillerThreadData
{
  HANDLE hThread;
  FolderWindow* window;
  Array<FolderFillerTask> task;
  FillerThreadData* prev;
  int colPos[colCount];
};

uint32 WINAPI FolderWindow::fillerThreadProc(void* arg)
{
  FillerThreadData* data = (FillerThreadData*) arg;
  GameCache localCache;
  for (int i = 0; i < data->task.length() && data == data->window->filler; i++)
  {
    GameCache* cache = getApp()->getCache()->getGame(data->task[i].path, &localCache);
    if (cache && data == data->window->filler)
    {
      readCacheInfo(cache, data->task[i].item, data->colPos);
      PostMessage(data->window->list->getHandle(), WM_UPDATELISTITEM,
        (uint32) data, i);
    }
  }
  return 0;
}

void FolderWindow::releaseFillers()
{
  if (filler)
  {
    int count = 0;
    for (FillerThreadData* data = filler; data; data = data->prev)
      if (data->hThread)
        count++;
    HANDLE* handles = new HANDLE[count];
    count = 0;
    for (FillerThreadData* data = filler; data; data = data->prev)
      if (data->hThread)
        handles[count++] = data->hThread;
    WaitForMultipleObjects(count, handles, TRUE, 5000);
    delete[] handles;
    while (filler)
    {
      FillerThreadData* prev = filler->prev;
      if (filler->hThread)
        CloseHandle(filler->hThread);
      delete filler;
      filler = prev;
    }
  }
}


int FolderWindow::compItems(FolderFoundItem const& a, FolderFoundItem const& b)
{
  if (a.folder != b.folder)
    return a.folder ? -1 : 1;
  for (int i = 0; i < colCount; i++)
  {
    int arg = cfg.colSort[i];
    int result = (arg & 0x80000000 ? -1 : 1);
    if (result < 0)
      arg = ~arg;
    if (arg == colFile)
      result *= String::smartCompare(a.name, b.name);
    else if (arg == colDate)
      result *= (a.ftime < b.ftime ? -1 : (a.ftime == b.ftime ? 0 : 1));
    else if (arg == colSize)
      result *= (a.size < b.size ? -1 : (a.size == b.size ? 0 : 1));
    else if (arg == colName)
      result *= String::smartCompare(a.gameName, b.gameName);
    else if (arg == colLineup)
      result *= String::smartCompare(a.lineup, b.lineup);
    else if (arg == colLength)
      result *= (a.gameLength < b.gameLength ? -1 : (a.gameLength == b.gameLength ? 0 : 1));
    else if (arg == colMode)
      result *= String::smartCompare(a.gameMode, b.gameMode);
    if (result)
      return result;
  }
  return 0;
}

void FolderWindow::readCacheInfo(GameCache* cache, FolderFoundItem& item, int* colPos)
{
  ImageLibrary* lib = getApp()->getImageLibrary();

  item.gameName = "";
  item.gameMode = "";
  item.gameLength = 0;
  item.lineup = "";

  Dota* dota = getApp()->getDotaLibrary()->getDota();

  item.gameName = cache->game_name;
  if (colPos == NULL || colPos[colMode] >= 0)
    item.gameMode = formatMode(cache->game_mode);
  item.gameLength = cache->game_length;
  if ((colPos == NULL || colPos[colLineup] >= 0) && dota && cache->map_version)
  {
    int count[2] = {0, 0};
    for (int team = 0; team < 2; team++)
    {
      if (team == 1)
        item.lineup += " vs ";
      for (uint8 i = 0; i < cache->players; i++)
      {
        if (cache->pteam[i] == team && cache->phero[i])
        {
          Dota::Hero* hero = dota->getHero(cache->phero[i]);
          if (hero)
            item.lineup += String::format("$%d$", lib->getListIndex(hero->icon, "Unknown"));
          else
            item.lineup += String::format("$%d$", lib->getListIndex("Unknown"));
          count[team]++;
        }
      }
      if (team == 0 && count[team] < 5)
        item.lineup = String("$0$") * (5 - count[team]) + item.lineup;
    }
    if (!count[0] && !count[1])
      item.lineup = "";
  }
}
void FolderWindow::rebuild()
{
  static char colNames[colCount][32] = {
    "Name", "Date", "Size", "Game name", "Lineup", "Length", "Mode"
  };
  bool colShow[colCount] = {true,
    cfg.selColumns & COL_SAVED,
    cfg.selColumns & COL_SIZE,
    cfg.selColumns & COL_NAME,
    cfg.selColumns & COL_RATIO,
    cfg.selColumns & COL_LENGTH,
    cfg.selColumns & COL_MODE
  };
  int colOrder[colCount];
  int colPos[colCount];
  int numCols = 0;
  for (int i = 0; i < colCount; i++)
  {
    if (colShow[i])
      colPos[i] = numCols++;
    else
      colPos[i] = -1;
  }
  int numOrder = 0;
  for (int i = 0; i < colCount; i++)
    if (colShow[cfg.colOrder[i]])
      colOrder[numOrder++] = colPos[cfg.colOrder[i]];

  Dictionary<uint32> selected;
  for (int i = 0; i < items.length(); i++)
    if (ListView_GetItemState(list->getHandle(), items[i].pos, LVIS_SELECTED))
      selected.set(items[i].path, 1);
  int scrollPosX = GetScrollPos(list->getHandle(), SB_HORZ);
  int scrollPosY = GetScrollPos(list->getHandle(), SB_VERT);
  if (items.length() > 0)
  {
    RECT rc;
    ListView_GetItemRect(list->getHandle(), 0, &rc, LVIR_BOUNDS);
    scrollPosY *= rc.bottom - rc.top;
  }

  list->setRedraw(false);
  list->clear();
  list->clearColumns();
  for (int i = 0; i < colCount; i++)
    if (colShow[i])
      list->insertColumn(colPos[i], colNames[i]);
  if (colShow[colName])
    list->setColumnUTF8(colPos[colName], true);
  ListView_SetColumnOrderArray(list->getHandle(), numOrder, colOrder);

  ImageLibrary* lib = getApp()->getImageLibrary();

  bool hasUpFolder = false;
  Array<FolderFoundItem> found;

  if (!path.isEmpty())
  {
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile(String::buildFullName(path, "*"), &data);
    BOOL success = (hFind != INVALID_HANDLE_VALUE);
    while (success)
    {
      if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
      {
        if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
          FolderFoundItem& item = found.push();
          item.name = data.cFileName;
          item.folder = true;
        }
        else if (String::getExtension(data.cFileName).icompare(".w3g") == 0)
        {
          FolderFoundItem& item = found.push();
          item.name = data.cFileName;
          item.folder = false;
          item.nodata = false;
          String fname = String::buildFullName(path, item.name);
          if (cfg.selColumns & (COL_SAVED | COL_SIZE))
          {
            item.size = 0;
            item.ftime = 0;
            FileInfo fi;
            if (getFileInfo(fname, fi))
            {
              item.size = fi.size;
              item.ftime = fi.ftime;
            }
          }
          if (cfg.selColumns & (COL_NAME | COL_RATIO | COL_LENGTH | COL_MODE))
          {
            item.gameLength = 0;
            item.gameName = "";
            item.gameMode = "";
            item.lineup = "";
            GameCache* cache = getApp()->getCache()->getGameNow(fname);
            if (cache)
              readCacheInfo(cache, item, colPos);
            else
              item.nodata = true;
          }
        }
      }
      else if (!strcmp(data.cFileName, ".."))
        hasUpFolder = true;
      success = FindNextFile(hFind, &data);
    }
    FindClose(hFind);
  }

  found.sort(compItems);
  int colSort = (cfg.colSort[0] & 0x80000000 ? ~cfg.colSort[0] : cfg.colSort[0]);
  if (colSort >= 0 && colSort < colCount && colPos[colSort] >= 0)
  {
    HBITMAP image = NULL;
    if (cfg.colSort[0] & 0x80000000)
      image = lib->getBitmap("SortUp");
    else
      image = lib->getBitmap("SortDown");
    if (image)
    {
      HWND hHeader = ListView_GetHeader(list->getHandle());
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

  if (filler == NULL || filler->hThread)
  {
    FillerThreadData* data = new FillerThreadData;
    data->hThread = NULL;
    data->window = this;
    data->prev = filler;
    filler = data;
  }
  filler->task.clear();
  memcpy(filler->colPos, colPos, sizeof colPos);

  items.clear();
  if (hasUpFolder)
  {
    int pos = list->addItem("[Up One Level]", lib->getListIndex("levelup"), items.length());
    ViewItem& item = items.push();
    item.path = String::fixPath(String::buildFullName(path, ".."));
    item.pos = pos;
    item.type = ITEM_UPFOLDER;
  }
  for (int i = 0; i < found.length(); i++)
  {
    FolderFoundItem& fitem = found[i];
    if (fitem.folder)
    {
      int pos = list->addItem(fitem.name, lib->getListIndex("IconClosedFolder"), items.length());
      ViewItem& item = items.push();
      item.path = String::fixPath(String::buildFullName(path, fitem.name));
      item.pos = pos;
      item.type = ITEM_FOLDER;
    }
    else
    {
      int pos = list->addItem(String::getFileTitle(fitem.name),
        lib->getListIndex("IconReplay"), items.length());
      ViewItem& item = items.push();
      item.path = String::fixPath(String::buildFullName(path, fitem.name));
      item.pos = pos;
      item.type = ITEM_REPLAY;

      if (fitem.nodata)
      {
        FolderFillerTask& t = filler->task.push();
        t.path = item.path;
        t.pos = pos;
      }

      if (colShow[colDate] && fitem.ftime)
        list->setItemText(pos, colPos[colDate], format_systime(fitem.ftime, "%c"));
      if (colShow[colSize])
        list->setItemText(pos, colPos[colSize], String::format("%d KB", (fitem.size + 1023) / 1024));
      if (colShow[colName])
        list->setItemText(pos, colPos[colName], fitem.gameName);
      if (colShow[colLineup])
        list->setItemText(pos, colPos[colLineup], fitem.lineup);
      if (colShow[colLength])
        list->setItemText(pos, colPos[colLength], format_time(fitem.gameLength));
      if (colShow[colMode])
        list->setItemText(pos, colPos[colMode], fitem.gameMode);
    }
  }
  for (int i = 0; i < items.length(); i++)
    if (selected.has(items[i].path))
      ListView_SetItemState(list->getHandle(), items[i].pos, LVIS_SELECTED, LVIS_SELECTED);


  if (filler->task.length())
  {
    uint32 id;
    filler->hThread = CreateThread(NULL, 0, fillerThreadProc, filler, 0, &id);
  }

  for (int i = 0; i < colCount; i++)
    if (colShow[i])
      list->setColumnWidth(colPos[i], cfg.colWidth[i]);
  list->setRedraw(true);
  ListView_Scroll(list->getHandle(), scrollPosX, scrollPosY);

  FillerThreadData* lastFiller = NULL;
  FillerThreadData* curFiller = filler;
  while (curFiller)
  {
    if (curFiller->hThread == NULL || WaitForSingleObject(curFiller->hThread, 0) != WAIT_TIMEOUT)
    {
      if (curFiller->hThread)
        CloseHandle(curFiller->hThread);
      if (lastFiller)
        lastFiller->prev = curFiller->prev;
      else
        filler = curFiller->prev;
      delete curFiller;
    }
    else
      lastFiller = curFiller;
    if (lastFiller)
      curFiller = lastFiller->prev;
    else
      curFiller = filler;
  }
}
void FolderWindow::updateListItem(uint32 wParam, uint32 lParam)
{
  if (wParam == (uint32) filler)
  {
    int pos = filler->task[lParam].pos;
    if (filler->colPos[colName] >= 0)
      list->setItemText(pos, filler->colPos[colName], filler->task[lParam].item.gameName);
    if (filler->colPos[colLineup] >= 0)
      list->setItemText(pos, filler->colPos[colLineup], filler->task[lParam].item.lineup);
    if (filler->colPos[colLength] >= 0)
      list->setItemText(pos, filler->colPos[colLength],
        format_time(filler->task[lParam].item.gameLength));
    if (filler->colPos[colMode] >= 0)
      list->setItemText(pos, filler->colPos[colMode], filler->task[lParam].item.gameMode);
  }
}
