#ifndef __FRAMEUI_FRAMEWND_H__
#define __FRAMEUI_FRAMEWND_H__

#include <windows.h>

#include "graphics/rect.h"
#include "frameui/frame.h"
#include "frameui/window.h"

class WindowFrame : public Frame, public Window
{
  HWND ownerWindow;
  void onMove();
protected:
  void create(String text, uint32 style, uint32 exStyle);
  void subclass(String wndClass, String text, uint32 style, uint32 exStyle);
public:
  WindowFrame(Frame* parent);
};
class ExtWindowFrame : public Frame
{
  Window* window;
  void onMove();
public:
  ExtWindowFrame(Frame* parent, Window* wnd);
  ~ExtWindowFrame();

  Window* getWindow() const
  {
    return window;
  }
};

class FrameWindow : public Frame, public Window
{
  class Invalidator : public RectUpdater
  {
    HWND wnd;
  public:
    Invalidator()
    {
      wnd = NULL;
    }
    void setWindow(HWND window)
    {
      wnd = window;
    }
    void add(Rect const& rc)
    {
      if (wnd)
        InvalidateRect(wnd, (RECT*) &rc, TRUE);
    }
  };

  MasterRegion* masterRegion;
  Invalidator invalidator;
protected:
  virtual uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  FrameWindow();
  ~FrameWindow();
};

#endif // __FRAMEUI_FRAMEWND_H__
