#ifndef __UI_REPLAYWND__
#define __UI_REPLAYWND__

#include "frameui/controlframes.h"
#include "replay/replay.h"

class ReplayViewItem;

enum {REPLAY_GAMEINFO,
      REPLAY_GAMECHAT,
      REPLAY_TIMELINE,
      REPLAY_PLAYERINFO,
      REPLAY_ACTIONS,
      REPLAY_PLAYERGOLD,
      REPLAY_PLAYEREXP,
      REPLAY_PRESENT,
      REPLAY_DRAFT,
      REPLAY_ACTIONLOG,

      REPLAY_NUM_TABS
};
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
  int frametab[REPLAY_NUM_TABS];
  ReplayTab* frames[REPLAY_NUM_TABS];
  void addFrame(int tab);

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
