#ifndef __UI_SEARCHRES__
#define __UI_SEARCHRES__

#include <windows.h>
#include "frameui/listctrl.h"
#include "ui/batchdlg.h"
#include "ui/folderwnd.h"

class SearchResults : public ListFrame
{
  CRITICAL_SECTION lock;
  struct SearchItem
  {
    String path;
    FolderFoundItem data;
  };
  Array<SearchItem> items;
  HMENU ctxMenu;
  void getSelList(Array<String>& sel);
  void addItem(int pos, bool progress);
  void getColPos(int* colPos);
  enum {colFile, colDate, colSize, colName, colLineup, colLength, colMode, colCount};
  static int compItems(SearchItem const& a, SearchItem const& b);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  SearchResults(Frame* parent);
  ~SearchResults();

  void rebuild();
  void doBatch(BatchDialog* batch);
  void addFile(String file);
};

#endif // __UI_SEARCHRES__
