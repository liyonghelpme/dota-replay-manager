#ifndef __UI_REPLAYWND__
#define __UI_REPLAYWND__

#include "frameui/framewnd.h"
#include "replay/replay.h"

class ReplayViewItem;

#define REPLAY_GAMEINFO         0
#define REPLAY_GAMECHAT         1
#define REPLAY_TIMELINE         2
#define REPLAY_NUM_TABS         3

class ReplayTab : public Frame
{
protected:
  HWND hWnd;
  W3GReplay* w3g;
  virtual void onSetReplay() = 0;
public:
  ReplayTab(FrameWindow* parent)
    : Frame(parent)
    , hWnd(parent->getHandle())
    , w3g(NULL)
  {}
  void setReplay(W3GReplay* replay)
  {
    w3g = replay;
    onSetReplay();
  }
  virtual void onMessage(uint32 message, uint32 wParam, uint32 lParam) {}
};

class ReplayWindow : public FrameWindow
{
  W3GReplay* replay;
  ReplayViewItem* viewItem;

  ReplayTab* tabs[REPLAY_NUM_TABS];
  int curTab;

  void setTab(int i, String name, ReplayTab* tab);

  void update();
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayWindow(Window* parent);
  ~ReplayWindow();
  void openReplay(File* file);
  void openReplay(String path);

  void setViewItem(ReplayViewItem* item)
  {
    viewItem = item;
  }
  void setTab(int tab);
};

#endif // __UI_REPLAYWND__
