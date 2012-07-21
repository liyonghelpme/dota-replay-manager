#ifndef __UI_REPLAY_TIMELINEWND__
#define __UI_REPLAY_TIMELINEWND__

#include "ui/replaywnd.h"

#include "frameui/controlframes.h"
#include "base/dictionary.h"

class OpenGL;

class TimePicture : public WindowFrame
{
  OpenGL* gl;
  W3GReplay* w3g;
  uint32 time;

  Dictionary<uint32> images;

  String formatPlayer(W3GPlayer* player);
  void drawNotify(int alpha, int x, int y, String text);
  void rect(int x, int y, int width, int height, char const* icon, int inset = 0);
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
  uint32 lastTime;
  uint32 lastUpdate;
  int speed;
  TimePicture* picture;
  EditFrame* speedBox;
  UpDownFrame* updownBox;
  ButtonFrame* playBox;
  SliderFrame* slider;
  EditFrame* timeBox;
  void onSetReplay();
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void onMove();
public:
  ReplayTimelineTab(Frame* parent);
  ~ReplayTimelineTab();
};

#endif // __UI_REPLAY_TIMELINEWND__
