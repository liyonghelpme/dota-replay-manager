#ifndef __UI_REPLAY_ACTIONLOG__
#define __UI_REPLAY_ACTIONLOG__

#include "ui/replaywnd.h"
#include "frameui/controlframes.h"
#include "frameui/listctrl.h"
#include "dota/mapdata.h"

class ActionListFrame;

class ReplayActionLogTab : public ReplayTab
{
  ButtonFrame* loadButton;
  ButtonFrame* rawCodes;

  ComboFrame* searchMode;
  ComboFrameEx* searchPlayer;
  ButtonFrame* searchNext;
  ButtonFrame* searchPrev;
  EditFrame* searchText;

  ActionListFrame* actionList;

  MapData* mapData;

  HMENU ctxMenu;

  struct ParseState;
  void parseActions(File* file, int length, ParseState& state);
  void parseBlocks(File* file);

  void parseReplay();
  void onSetReplay();
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ReplayActionLogTab(Frame* parent);
  ~ReplayActionLogTab();
};

#endif // __UI_REPLAY_ACTIONLOG__
