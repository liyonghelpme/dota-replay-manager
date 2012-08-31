#ifndef __UI_REPLAY_DRAFT__
#define __UI_REPLAY_DRAFT__

#include "ui/replaywnd.h"
#include "frameui/controlframes.h"

class HeroListFrame;

class ReplayDraftTab : public ReplayTab
{
  StaticFrame* captains[2];
  HeroListFrame* pool;
  HeroListFrame* picks;
  HeroListFrame* bans;

  void onSetReplay();
public:
  ReplayDraftTab(Frame* parent);
};

#endif // __UI_REPLAY_DRAFT__
