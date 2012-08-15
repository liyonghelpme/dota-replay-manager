#ifndef __UI_REPLAY_ACTIONS__
#define __UI_REPLAY_ACTIONS__

#include "ui/replaywnd.h"

#include "frameui/listctrl.h"
#include "frameui/controlframes.h"

class ActionChart;
class ReplayActionsTab : public ReplayTab
{
  ComboFrameEx* players;
  SimpleListFrame* actions;
  SimpleListFrame* hotkeys;
  StaticFrame* afkinfo;
  ActionChart* chart;

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void setPlayer(W3GPlayer* player);
  void onSetReplay();
public:
  ReplayActionsTab(Frame* parent);
};

#endif // __UI_REPLAY_ACTIONS__
