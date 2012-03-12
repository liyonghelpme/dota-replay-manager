#ifndef __FRAMEUI_WINDOW_H__
#define __FRAMEUI_WINDOW_H__

#include <windows.h>

#include "base/types.h"
#include "base/string.h"

#define WM_NOTIFYREFLECT    (WM_USER+0x0157)
#define WM_COMMANDREFLECT   (WM_USER+0x0158)
#define WM_POSTCREATE       (WM_USER+0x0159)

class Window
{
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static ATOM windowClass;
  String regClass;
protected:
  HWND hWnd;
  WNDPROC origProc;
  virtual uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  WNDCLASSEX* createclass(String wndClass);
  void create(int x, int y, int width, int height, String text, uint32 style, uint32 exStyle,
    HWND parent = NULL);
  void subclass(String wndClass, int x, int y, int width, int height, String text, uint32 style,
    uint32 exStyle, HWND parent = NULL);
public:
  Window();
  virtual ~Window();

  operator HWND() const
  {
    return hWnd;
  }
  HWND getHandle() const
  {
    return hWnd;
  }

  // random functions
  void setText(String text);
  String getText() const;

  void setFont(HFONT hFont);
  HFONT getFont() const;

  void enable(bool e = true);
  void disable()
  {
    enable(false);
  }
  void showWindow(bool s = true);
  void hideWindow()
  {
    showWindow(false);
  }

  int id() const;
  void setId(int id);
};

#endif // __FRAMEUI_WINDOW_H__
