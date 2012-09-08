#ifndef __UI_REPLAY_GAMEINFO__
#define __UI_REPLAY_GAMEINFO__

#include "ui/replaywnd.h"

#include "frameui/listctrl.h"
#include "frameui/controlframes.h"
#include "graphics/image.h"

class MapPopout : public Window
{
  HDC dc;
  HBITMAP bitmap;
  uint32 onWndMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  MapPopout(RECT const* rc, Image* image);
  ~MapPopout();
};

class ReplayGameInfoTab : public ReplayTab
{
  void addInfo(String name, String value, bool utf8 = false);
  SimpleListFrame* info;
  ListFrame* players;
  ButtonFrame* watchReplay;
  ButtonFrame* copyMatchup;
  StaticFrame* map;
  MapPopout* popout;
  Image* mapImages[2];
  Image* mapCanvas;
  HBITMAP mapBitmap;
  int curImage;
  HMENU ctxMenu;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void onSetReplay();
  void addLadderPlayer(W3GPlayer* player);
  void addPlayer(W3GPlayer* player);
  String getWatchCmd();
public:
  ReplayGameInfoTab(Frame* parent);
  ~ReplayGameInfoTab();
};

#endif // __UI_REPLAY_GAMEINFO__
