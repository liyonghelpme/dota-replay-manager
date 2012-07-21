#ifndef __FRAMEUI_FRAMEWND_H__
#define __FRAMEUI_FRAMEWND_H__

#include <windows.h>

#include "graphics/rect.h"
#include "frameui/frame.h"
#include "frameui/window.h"

class WindowFrame : public Frame, public Window
{
  HWND ownerWindow;
protected:
  void onMove();
  uint32 onWndMessage(uint32 message, uint32 wParam, uint32 lParam);
protected:
  void create(String text, uint32 style, uint32 exStyle);
  void create(String wndClass, String text, uint32 style, uint32 exStyle);
  HWND getOwner() const
  {
    return ownerWindow;
  }
  uint32 notify(uint32 message, uint32 wParam, uint32 lParam);
public:
  WindowFrame(Frame* parent);
};

class RootWindow : public Frame, public Window
{
  MasterFrame* masterFrame;
  uint32 onControlMessage(HWND hControl, uint32 message, uint32 wParam, uint32 lParam);
  uint32 onWndMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  RootWindow();
  ~RootWindow();
};

#endif // __FRAMEUI_FRAMEWND_H__
