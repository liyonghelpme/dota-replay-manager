#ifndef __UI_MAINWND_H__
#define __UI_MAINWND_H__

#include "frameui/framewnd.h"

#include "ui/replaytree.h"
#include "ui/viewitem.h"

#define MAINWND_SETTINGS      0
#define MAINWND_REPLAY        1
#define MAINWND_FOLDER        2
#define MAINWND_SEARCH        3
#define MAINWND_SEARCHRES     4
#define MAINWND_NUM_VIEWS     5

#define MAINWND_OPEN_REPLAY   1728

#define WM_UPDATEFILE         (WM_USER+987)
#define WM_COPYDATA_FAKE      (WM_USER+989)
#define WM_SETVIEW            (WM_USER+990)
#define WM_GETVIEW            (WM_USER+991)
#define WM_PUSHVIEW           (WM_USER+992)

class EditFrame;

class MainWnd : public RootWindow
{
  ReplayTree* replayTree;

  Frame* views[MAINWND_NUM_VIEWS];
  ViewItem* history;
  ButtonFrame* hBack;
  ButtonFrame* hForward;
  IconEditFrame* addressBar;

  HMENU trayMenu;

  // splitter
  int dragPos;
  HCURSOR sizeCursor;

  uint64 lastReplayTime;
  HICON hIcon;
  bool trayShown;
  void createTrayIcon();
  void destroyTrayIcon();
  void trayNotify(String title, String text);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  MainWnd();
  ~MainWnd();

  Frame* setView(int view);
  Frame* getView(int view);
  void pushView(ViewItem* item);
  void setAddress(String text);
  void setTreeAddress(String text);

  void postLoad();
};

#endif // __UI_MAINWND_H__
