#ifndef __UI_MAINWND_H__
#define __UI_MAINWND_H__

#include "frameui/framewnd.h"

#include "ui/replaytree.h"
#include "ui/settingswnd.h"

class MainWnd : public FrameWindow
{
  ReplayTree* replayTree;

  ExtWindowFrame* settings;

  // splitter
  int* splitterPos;
  int dragPos;
  HCURSOR sizeCursor;

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  MainWnd();
  ~MainWnd();

  void postLoad();
};

#endif // __UI_MAINWND_H__
