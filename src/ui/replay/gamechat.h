#ifndef __UI_REPLAY_GAMECHAT__
#define __UI_REPLAY_GAMECHAT__

#include "ui/replaywnd.h"

#include "frameui/controlframes.h"

class ReplayGameChatTab : public ReplayTab
{
  RichEditFrame* chatFrame;
  void onSetReplay();
  String sanitizePlayer(W3GPlayer* player);
  String sanitize(String str);
  String sanitizeNotify(String str);
public:
  ReplayGameChatTab(FrameWindow* parent);
  ~ReplayGameChatTab();
};

#endif // __UI_REPLAY_GAMECHAT__
