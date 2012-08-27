#ifndef __UI_REPLAYWND__
#define __UI_REPLAYWND__

#include "frameui/controlframes.h"
#include "replay/replay.h"

class ReplayViewItem;

#define REPLAY_GAMEINFO         0
#define REPLAY_GAMECHAT         1
#define REPLAY_TIMELINE         2
#define REPLAY_PLAYERINFO       3
#define REPLAY_ACTIONS          4
#define REPLAY_PLAYERGOLD       5
#define REPLAY_PLAYEREXP        6
#define REPLAY_PRESENT          7
#define REPLAY_NUM_TABS         8

#define ID_PLAYERBOX            378

class ReplayTab : public Frame
{
protected:
  W3GReplay* w3g;
  virtual void onSetReplay() = 0;
public:
  ReplayTab(Frame* parent)
    : Frame(parent)
    , w3g(NULL)
  {}
  void setReplay(W3GReplay* replay)
  {
    w3g = replay;
    onSetReplay();
  }
  virtual void setPlayer(W3GPlayer* player)
  {}
};

class ReplayWindow : public TabFrame
{
  W3GReplay* replay;
  ReplayViewItem* viewItem;

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void update();
public:
  ReplayWindow(Frame* parent);
  ~ReplayWindow();
  void openReplay(File* file);
  void openReplay(String path);

  void setViewItem(ReplayViewItem* item)
  {
    viewItem = item;
  }
  void setTab(int tab);
  void setPlayer(int id);
};

#endif // __UI_REPLAYWND__
