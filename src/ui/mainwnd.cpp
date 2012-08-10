#include "core/app.h"

#include "ui/settingswnd.h"
#include "ui/replaywnd.h"
#include "ui/folderwnd.h"

#include "mainwnd.h"

#define SPLITTER_WIDTH      10

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

  create(CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, "DotA Replay Manager",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0);

  replayTree = new ReplayTree("K:\\Games\\Warcraft III\\Replay", this);
  replayTree->setPoint(PT_TOPLEFT, 10, 10);
  replayTree->setPoint(PT_BOTTOM, 0, -10);
  replayTree->setWidth(cfg::splitterPos - 10);

  views[MAINWND_SETTINGS] = new SettingsWindow(this);
  views[MAINWND_REPLAY] = new ReplayWindow(this);
  views[MAINWND_FOLDER] = new FolderWindow(this);
  for (int i = 0; i < MAINWND_NUM_VIEWS; i++)
  {
    views[i]->setPoint(PT_TOPLEFT, replayTree, PT_TOPRIGHT, SPLITTER_WIDTH, 0);
    views[i]->setPoint(PT_BOTTOMRIGHT, -10, -10);
    views[i]->hide();
  }
  history = NULL;
}
MainWnd::~MainWnd()
{
}
void MainWnd::postLoad()
{
  WINDOWPLACEMENT pl;
  memset(&pl, 0, sizeof pl);
  pl.length = sizeof pl;
  GetWindowPlacement(hWnd, &pl);
  pl.flags = 0;
  pl.showCmd = cfg::wndShow;
  if (cfg::wndX != CW_USEDEFAULT)
  {
    pl.rcNormalPosition.left = cfg::wndX;
    pl.rcNormalPosition.top = cfg::wndY;
  }
  if (cfg::wndWidth != CW_USEDEFAULT)
  {
    pl.rcNormalPosition.right = pl.rcNormalPosition.left + cfg::wndWidth;
    pl.rcNormalPosition.bottom = pl.rcNormalPosition.top + cfg::wndHeight;
  }
  SetWindowPlacement(hWnd, &pl);
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
    if (getApp()->loaded())
    {
      WINDOWPLACEMENT pl;
      memset(&pl, 0, sizeof pl);
      pl.length = sizeof pl;
      GetWindowPlacement(hWnd, &pl);
      Registry* reg = getApp()->getRegistry();
      cfg::wndShow = pl.showCmd;
      if (pl.showCmd == SW_SHOWNORMAL)
      {
        cfg::wndX = pl.rcNormalPosition.left;
        cfg::wndY = pl.rcNormalPosition.top;
        cfg::wndWidth = pl.rcNormalPosition.right - pl.rcNormalPosition.left;
        cfg::wndHeight = pl.rcNormalPosition.bottom - pl.rcNormalPosition.top;
      }
    }
    break;
//////////////////// SPLITTER ////////////////////////
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      if (pt.x > cfg::splitterPos && pt.x < cfg::splitterPos + SPLITTER_WIDTH)
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
      if (pt.x > cfg::splitterPos && pt.x < cfg::splitterPos + SPLITTER_WIDTH)
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
      cfg::splitterPos = cfg::splitterPos + pos - dragPos;
      dragPos = pos;
      RECT rc;
      GetClientRect(hWnd, &rc);
      int oldSplitterPos = cfg::splitterPos;
      if (cfg::splitterPos < 50) cfg::splitterPos = 50;
      if (cfg::splitterPos > rc.right - 200) cfg::splitterPos = rc.right - 200;
      dragPos += cfg::splitterPos - oldSplitterPos;
      if (replayTree)
        replayTree->setWidth(cfg::splitterPos - 10);
    }
    break;
  case WM_LBUTTONUP:
    if (GetCapture() == hWnd)
      ReleaseCapture();
    break;
  }
  return M_UNHANDLED;
}

Frame* MainWnd::setView(int view)
{
  for (int i = 0; i < MAINWND_NUM_VIEWS; i++)
    views[i]->show(i == view);
  return views[view];
}
void MainWnd::pushView(ViewItem* item)
{
  history = (history ? history->push(item) : item);
  history->apply(this);
}
