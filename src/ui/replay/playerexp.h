#ifndef __UI_REPLAY_PLAYEREXP__
#define __UI_REPLAY_PLAYEREXP__

#include "ui/replaywnd.h"
#include "ui/graphwnd.h"
#include "frameui/controlframes.h"

class ExpGraphWindow;

class ReplayPlayerExpTab : public ReplayTab
{
  ExpGraphWindow* graph;

  struct PlayerButtons
  {
    Frame* frame;
    ImageFrame* icon;
    ButtonFrame* check;
    ColorFrame* bar;
  };
  PlayerButtons buttons[10];

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void onSetReplay();
public:
  ReplayPlayerExpTab(Frame* parent);
};

#endif // __UI_REPLAY_PLAYEREXP__
