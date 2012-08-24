#ifndef __UI_REPLAY_PLAYERGOLD__
#define __UI_REPLAY_PLAYERGOLD__

#include "ui/replaywnd.h"
#include "ui/graphwnd.h"
#include "frameui/controlframes.h"

class GoldGraphWindow;

class ReplayPlayerGoldTab : public ReplayTab
{
  GoldGraphWindow* graph;

  struct PlayerButtons
  {
    Frame* frame;
    ImageFrame* icon;
    ButtonFrame* check;
    ColorFrame* bar;
  };
  ButtonFrame* totals[2];
  PlayerButtons buttons[10];

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void onSetReplay();
public:
  ReplayPlayerGoldTab(Frame* parent);
};

#endif // __UI_REPLAY_PLAYERGOLD__
