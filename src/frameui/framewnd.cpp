#include "core/app.h"

#include "framewnd.h"

WindowFrame::WindowFrame(Frame* parent)
  : Frame(parent)
{
  Frame* cur = getParent();
  while (cur->getParent())
    cur = cur->getParent();
  FrameWindow* frm = dynamic_cast<FrameWindow*>(cur);
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
      SetWindowPos(hWnd, NULL, left(), top(), width(), height(), SWP_NOZORDER);
      ShowWindow(hWnd, SW_SHOWNA);
    }
    else
      ShowWindow(hWnd, SW_HIDE);
  }
}
void WindowFrame::create(String text, uint32 style, uint32 exStyle)
{
  Window::create(0, 0, 10, 10, text, style, exStyle, ownerWindow);
}
void WindowFrame::subclass(String wndClass, String text, uint32 style, uint32 exStyle)
{
  Window::subclass(wndClass, 0, 0, 10, 10, text, style, exStyle, ownerWindow);
}

///////////////////////////////////////////////////////

ExtWindowFrame::ExtWindowFrame(Frame* parent, Window* wnd)
  : Frame(parent)
{
  window = wnd;
}
ExtWindowFrame::~ExtWindowFrame()
{
  delete window;
}
void ExtWindowFrame::onMove()
{
  if (window && window->getHandle())
  {
    if (visible())
    {
      SetWindowPos(window->getHandle(), NULL, left(), top(), width(), height(), SWP_NOZORDER);
      ShowWindow(window->getHandle(), SW_SHOWNA);
    }
    else
      ShowWindow(window->getHandle(), SW_HIDE);
  }
}

///////////////////////////////////////////////////////

FrameWindow::FrameWindow()
  : Frame(NULL)
{
  masterRegion = new MasterRegion(this);
}
FrameWindow::~FrameWindow()
{
  delete masterRegion;
}

uint32 FrameWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_POSTCREATE:
    invalidator.setWindow(hWnd);
    masterRegion->setUpdater(&invalidator);
    break;
  case WM_SIZE:
    {
      RECT rc;
      GetClientRect(hWnd, &rc);
      masterRegion->setSize(rc.right, rc.bottom);
    }
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);
      Window::onMessage(message, (uint32) hDC, 0);
      render(hDC);
      EndPaint(hWnd, &ps);
    }
    return 0;
  }
  return Window::onMessage(message, wParam, lParam);
}
