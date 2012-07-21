#include "core/app.h"

#include "framewnd.h"

WindowFrame::WindowFrame(Frame* parent)
  : Frame(parent)
{
  Frame* cur = getParent();
  while (cur->getParent())
    cur = cur->getParent();
  RootWindow* frm = dynamic_cast<RootWindow*>(cur);
  if (frm)
    ownerWindow = frm->getHandle();
  else
    ownerWindow = NULL;
}

void WindowFrame::onMove()
{
  if (hWnd)
  {
    if (visible())
    {
      if (IsWindowVisible(hWnd))
        SetWindowPos(hWnd, NULL, left(), top(), width(), height(), SWP_NOZORDER);
      else
      {
        ShowWindow(hWnd, SW_SHOWNA);
        SetWindowPos(hWnd, HWND_TOP, left(), top(), width(), height(), 0);
      }
    }
    else
      ShowWindow(hWnd, SW_HIDE);
  }
}
void WindowFrame::create(String text, uint32 style, uint32 exStyle)
{
  Window::create(0, 0, 10, 10, text, style, exStyle, ownerWindow);
}
void WindowFrame::create(String wndClass, String text, uint32 style, uint32 exStyle)
{
  Window::subclass(wndClass, 0, 0, 10, 10, text, style, exStyle, ownerWindow);
}

uint32 WindowFrame::onWndMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (uint32 result = onMessage(message, wParam, lParam))
    return result;
  return Window::onWndMessage(message, wParam, lParam);
}
uint32 WindowFrame::notify(uint32 message, uint32 wParam, uint32 lParam)
{
  Frame* cur = getParent();
  uint32 result = 0;
  while (cur && (result = cur->onMessage(message, wParam, lParam)) == 0)
    cur = cur->getParent();
  return result;
}

///////////////////////////////////////////////////////

RootWindow::RootWindow()
  : Frame(NULL)
{
  masterFrame = new MasterFrame(this);
}
RootWindow::~RootWindow()
{
  delete masterFrame;
}

uint32 RootWindow::onControlMessage(HWND hControl, uint32 message, uint32 wParam, uint32 lParam)
{
  Frame* cur = dynamic_cast<WindowFrame*>(Window::fromHandle(hControl));
  uint32 result = 0;
  while (cur && (result = cur->onMessage(message, wParam, lParam)) == 0)
    cur = cur->getParent();
  return result;
}

uint32 RootWindow::onWndMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFY:
    return onControlMessage(((NMHDR*) lParam)->hwndFrom, message, wParam, lParam);
  case WM_COMMAND:
    return onControlMessage((HWND) lParam, message, wParam, lParam);
  case WM_DRAWITEM:
    {
      DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*) lParam;
      if (dis->CtlType != ODT_MENU)
      {
        if (uint32 result = onControlMessage(dis->hwndItem, message, wParam, lParam))
          return result;
      }
    }
    break;
  case WM_SIZE:
    {
      RECT rc;
      GetClientRect(hWnd, &rc);
      masterFrame->setSize(rc.right, rc.bottom);
    }
    break;
  }
  if (uint32 result = onMessage(message, wParam, lParam))
    return result;
  return Window::onWndMessage(message, wParam, lParam);
}
