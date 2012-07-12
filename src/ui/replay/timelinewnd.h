#ifndef __UI_REPLAY_TIMELINEWND__
#define __UI_REPLAY_TIMELINEWND__

#include "ui/replaywnd.h"

#include "frameui/controlframes.h"

class OpenGL;

class TimePicture : public WindowFrame
{
  OpenGL* gl;
  uint32 tex;
  W3GReplay* w3g;
  uint32 time;

  String formatPlayer(W3GPlayer* player);
  void drawNotify(int alpha, int x, int y, String text);
  void paint();

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  TimePicture(Frame* parent);
  ~TimePicture();

  int mapx(float x);
  int mapy(float y);
  float unmapx(int x);
  float unmapy(int y);

  void setReplay(W3GReplay* replay);
  void setTime(uint32 time);

  HCURSOR cursor;
};

class ReplayTimelineTab : public ReplayTab
{
  TimePicture* picture;
  void onSetReplay();
public:
  ReplayTimelineTab(FrameWindow* parent);
  ~ReplayTimelineTab();
};

#endif // __UI_REPLAY_TIMELINEWND__
