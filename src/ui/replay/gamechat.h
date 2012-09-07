#ifndef __UI_REPLAY_GAMECHAT__
#define __UI_REPLAY_GAMECHAT__

#include "ui/replaywnd.h"

#include "frameui/controlframes.h"
#include "frameui/listctrl.h"

class ReplayGameChatTab : public ReplayTab
{
  static INT_PTR CALLBACK FiltersDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  static bool FilterMessage(W3GMessage& msg);

  ComboFrame* searchMode;
  EditFrame* searchText;
  ComboFrameEx* searchPlayer;
  ComboFrameEx* searchPlayerEvent;
  ButtonFrame* searchPrev;
  ButtonFrame* searchNext;
  ButtonFrame* chatFilters;

  Array<W3GMessage*> lines;
  RichEditFrame* chatFrame;
  void refill();
  void onSetReplay();
  String sanitizePlayer(W3GPlayer* player);
  String sanitize(String str);
  String sanitizeNotify(String str);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayGameChatTab(Frame* parent);
  ~ReplayGameChatTab();
};

#endif // __UI_REPLAY_GAMECHAT__
