#ifndef __UI_FOLDERWND__
#define __UI_FOLDERWND__

#include "frameui/framewnd.h"
#include "ui/dirchange.h"
#include "base/array.h"
#include "frameui/controlframes.h"
#include "frameui/listctrl.h"
#include "frameui/dragdrop.h"

#define WM_REBUILDLIST      (WM_USER+910)
#define WM_UPDATELISTITEM   (WM_USER+911)
#define WM_POSTHEADERDRAG   (WM_USER+912)

class DropListFrame : public ListFrame
{
protected:
  DropTarget* target;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  int highlight;
  DropListFrame(Frame* parent, int id = 0, int style = LVS_NOSORTHEADER | LVS_SINGLESEL,
    int styleEx = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
};

struct GameCache;
struct FolderFoundItem
{
  bool folder;
  String name;

  bool nodata;
  uint32 size;
  uint64 ftime;
  String gameName;
  String lineup;
  uint32 gameLength;
  String gameMode;
};
struct FolderFillerTask;
class FolderWindow : public Frame, public DirChangeHandler
{
  struct ViewItem
  {
    int type;
    int pos;
    String path;
  };
  DropListFrame* list;
  Array<ViewItem> items;
  DirChangeTracker* tracker;
  String path;
  MainWnd* mainWnd;

  HMENU ctxMenu;
  bool selfDrag;

  void getSelList(Array<String>& sel);

  enum {ITEM_UPFOLDER = -1,
        ITEM_FOLDER = 1,
        ITEM_REPLAY = 2
  };

  enum {colFile, colDate, colSize, colName, colLineup, colLength, colMode, colCount};

  struct FillerThreadData;
  FillerThreadData* filler;
  static uint32 WINAPI fillerThreadProc(void* arg);
  void releaseFillers();
  void updateListItem(uint32 wParam, uint32 lParam);

  void doPaste(HGLOBAL hDrop, uint32 effect, String opTo);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void onDirChange();
  void rebuild();
public:
  FolderWindow(Frame* parent, MainWnd* mainWnd);
  ~FolderWindow();

  void setPath(String path);
  String getPath() const
  {
    return path;
  }

  static int compItems(FolderFoundItem const& a, FolderFoundItem const& b);
  static void readCacheInfo(GameCache* cache, FolderFoundItem& item, int* colPos);
};

#endif // __UI_FOLDERWND__
