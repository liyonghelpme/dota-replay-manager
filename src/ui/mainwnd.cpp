#include "core/app.h"

#include "mainwnd.h"

#define SPLITTER_WIDTH      4

MainWnd::MainWnd()
{
  if (WNDCLASSEX* wcx = createclass("MainWndClass"))
  {
    wcx->hbrBackground = HBRUSH(COLOR_BTNFACE + 1);
    wcx->hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx->hIcon = LoadIcon(wcx->hInstance, MAKEINTRESOURCE(IDI_MAIN));
    RegisterClassEx(wcx);
  }
  replayTree = NULL;

  sizeCursor = LoadCursor(getInstance(), MAKEINTRESOURCE(IDC_SPLITVERT));

  Registry* reg = getApp()->getRegistry();
  splitterPos = reg->createInt("splitterPos", 200);

  create(reg->readInt("wndX", CW_USEDEFAULT),
         reg->readInt("wndY", 0),
         reg->readInt("wndWidth", CW_USEDEFAULT),
         reg->readInt("wndHeight", 0),
         "DotA Replay Manager", WS_OVERLAPPEDWINDOW, 0);

  replayTree = new ReplayTree("K:\\Games\\Warcraft III\\Replay", this);
  replayTree->setPoint(PT_TOPLEFT, 0, 0);
  replayTree->setPoint(PT_BOTTOM, 0, 0);
  replayTree->setPoint(PT_RIGHT, NULL, PT_LEFT, *splitterPos, 0);

  settings = new ExtWindowFrame(this, new SettingsWindow(this));
  settings->setPoint(PT_TOPLEFT, replayTree, PT_TOPRIGHT, SPLITTER_WIDTH, 0);
  settings->setPoint(PT_BOTTOMRIGHT, 0, 0);
}
MainWnd::~MainWnd()
{
}
void MainWnd::postLoad()
{
  ShowWindow(hWnd, getApp()->getRegistry()->readInt("wndShow", SW_SHOW));
}

uint32 MainWnd::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_SIZE:
  case WM_MOVE:
    {
      WINDOWPLACEMENT pl;
      GetWindowPlacement(hWnd, &pl);
      Registry* reg = getApp()->getRegistry();
      if (pl.showCmd == SW_SHOWNORMAL)
        pl.showCmd = SW_SHOW;
      if (pl.showCmd == SW_SHOWMAXIMIZED || pl.showCmd == SW_SHOW)
        reg->writeInt("wndShow", pl.showCmd);
      reg->writeInt("wndX", pl.rcNormalPosition.left);
      reg->writeInt("wndY", pl.rcNormalPosition.top);
      reg->writeInt("wndWidth", pl.rcNormalPosition.right - pl.rcNormalPosition.left);
      reg->writeInt("wndHeight", pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
    }
    break;
//////////////////// SPLITTER ////////////////////////
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      if (pt.x > *splitterPos && pt.x < *splitterPos + SPLITTER_WIDTH)
      {
        SetCursor(sizeCursor);
        return TRUE;
      }
    }
    break;
  case WM_LBUTTONDOWN:
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      if (pt.x > *splitterPos && pt.x < *splitterPos + SPLITTER_WIDTH)
      {
        SetCapture(hWnd);
        dragPos = pt.x;
        return TRUE;
      }
    }
    break;
  case WM_MOUSEMOVE:
    if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
    {
      int pos = short(LOWORD(lParam));
      *splitterPos += pos - dragPos;
      dragPos = pos;
      RECT rc;
      GetClientRect(hWnd, &rc);
      int oldSplitterPos = *splitterPos;
      if (*splitterPos < 50) *splitterPos = 50;
      if (*splitterPos > rc.right - 200) *splitterPos = rc.right - 200;
      dragPos += *splitterPos - oldSplitterPos;
      if (replayTree)
        replayTree->setPoint(PT_RIGHT, NULL, PT_LEFT, *splitterPos, 0);
    }
    break;
  case WM_LBUTTONUP:
    if (GetCapture() == hWnd)
      ReleaseCapture();
    break;
  }
  return FrameWindow::onMessage(message, wParam, lParam);
}
