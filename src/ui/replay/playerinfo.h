#ifndef __UI_REPLAY_PLAYERINFO__
#define __UI_REPLAY_PLAYERINFO__

#include "ui/replaywnd.h"

#include "frameui/listctrl.h"
#include "frameui/controlframes.h"

class ReplayPlayerInfoTab : public ReplayTab
{
  ComboFrameEx* players;
  ListFrame* skills;
  ListFrame* items;
  ImageFrame* kdIcons[5];
  StaticFrame* kdText[5];
  StaticFrame* wardInfo;
  StaticFrame* buildInfo;
  ImageFrame* buildImage;

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void setPlayer(W3GPlayer* player);
  void onSetReplay();
public:
  ReplayPlayerInfoTab(Frame* parent);
};

#endif // __UI_REPLAY_PLAYERINFO__
