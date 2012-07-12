#include "window.h"

#include "core/app.h"

ATOM Window::windowClass = NULL;

WNDCLASSEX* Window::createclass(String wndClass)
{
  regClass = wndClass;

  WNDCLASSEX* wcx = new WNDCLASSEX;
  HINSTANCE hInstance = getInstance();
  if (!GetClassInfoEx(hInstance, regClass, wcx))
  {
    memset(wcx, 0, sizeof(WNDCLASSEX));
    wcx->cbSize = sizeof(WNDCLASSEX);
    wcx->lpfnWndProc = WindowProc;
    wcx->hInstance = hInstance;
    wcx->lpszClassName = regClass;
    return wcx;
  }
  delete wcx;
  return NULL;
}
void Window::create(int x, int y, int width, int height, String text, uint32 style, uint32 exStyle,
  HWND parent)
{
  if (!windowClass && regClass.isEmpty())
  {
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof wcex);
    wcex.cbSize = sizeof wcex;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = getInstance();
    wcex.lpszClassName = "WUTILSWINDOW";
    windowClass = RegisterClassEx(&wcex);
  }
  hWnd = CreateWindowEx(exStyle, regClass.isEmpty() ? "WUTILSWINDOW" : regClass, text, style,
    x, y, width, height, parent, NULL, getInstance(), this);
  SendMessage(hWnd, WM_POSTCREATE, 0, 0);
}
void Window::subclass(String wndClass, int x, int y, int width, int height, String text, uint32 style,
  uint32 exStyle, HWND parent)
{
  hWnd = CreateWindowEx(exStyle, wndClass, text, style, x, y, width, height,
    parent, NULL, getInstance(), this);
  SetWindowLong(hWnd, GWL_USERDATA, (uint32) this);
  origProc = (WNDPROC) SetWindowLong(hWnd, GWL_WNDPROC, (uint32) WindowProc);
  SendMessage(hWnd, WM_POSTCREATE, 0, 0);
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Window* wnd = (Window*) GetWindowLong(hWnd, GWL_USERDATA);
  if (wnd == NULL && uMsg == WM_CREATE)
  {
    CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
    wnd = (Window*) cs->lpCreateParams;
    SetWindowLong(hWnd, GWL_USERDATA, (uint32) wnd);
  }
  if (wnd)
  {
    if (uMsg == WM_NOTIFY)
    {
      NMHDR* nmhdr = (NMHDR*) lParam;
      if (GetParent(nmhdr->hwndFrom) == hWnd)
      {
        LRESULT result = SendMessage(nmhdr->hwndFrom, WM_NOTIFYREFLECT, wParam, lParam);
        if (result)
          return result;
      }
    }
    else if (uMsg == WM_COMMAND)
    {
      HWND hwndFrom = (HWND) lParam;
      if (GetParent(hwndFrom) == hWnd)
      {
        LRESULT result = SendMessage(hwndFrom, WM_COMMANDREFLECT, wParam, lParam);
        if (result)
          return result;
      }
    }
    else if (uMsg == WM_DRAWITEM)
    {
      DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*) lParam;
      if (dis->CtlType |= ODT_MENU && GetParent(dis->hwndItem) == hWnd)
      {
        LRESULT result = SendMessage(dis->hwndItem, WM_DRAWITEMREFLECT, wParam, lParam);
        if (result)
          return result;
      }
    }
    uint32 result = wnd->onMessage(uMsg, wParam, lParam);
    if (uMsg == WM_DESTROY)
      wnd->hWnd = NULL;
    return result;
  }
  else
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
uint32 Window::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (origProc)
    return CallWindowProc(origProc, hWnd, message, wParam, lParam);
  else
    return DefWindowProc(hWnd, message, wParam, lParam);
}

Window::Window()
{
  hWnd = NULL;
  origProc = NULL;
}
Window::~Window()
{
  if (hWnd)
  {
    if (origProc)
      SetWindowLong(hWnd, GWL_WNDPROC, (uint32) origProc);
    SetWindowLong(hWnd, GWL_USERDATA, 0);
    DestroyWindow(hWnd);
  }
}

void Window::setText(String text)
{
  SetWindowText(hWnd, text);
}
String Window::getText() const
{
  int length = GetWindowTextLength(hWnd);
  String buf;
  buf.resize(length);
  GetWindowText(hWnd, buf.getBuffer(), buf.getBufferSize());
  buf.setLength(length);
  return buf;
}

void Window::setFont(HFONT hFont)
{
  SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, TRUE);
}
HFONT Window::getFont() const
{
  return (HFONT) SendMessage(hWnd, WM_GETFONT, NULL, NULL);
}

void Window::enable(bool e)
{
  EnableWindow(hWnd, e);
}
void Window::showWindow(bool s)
{
  ShowWindow(hWnd, s ? SW_SHOW : SW_HIDE);
}

int Window::id() const
{
  return GetWindowLong(hWnd, GWL_ID);
}
void Window::setId(int id)
{
  SetWindowLong(hWnd, GWL_ID, id);
}
