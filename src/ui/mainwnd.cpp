#include "core/app.h"

#include "ui/settingswnd.h"
#include "ui/replaywnd.h"
#include "ui/folderwnd.h"
#include "frameui/controlframes.h"

#include "mainwnd.h"

#define SPLITTER_WIDTH      10

#define IDC_ADDRESSBAR        741
#define IDC_BROWSE            742
#define IDC_OPEN              743
#define IDC_BACK              744
#define IDC_FORWARD           745

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

  create(CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, "DotA Replay Manager",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0);

  replayTree = new ReplayTree(cfg.replayPath, this);
  replayTree->setPoint(PT_TOPLEFT, 10, 10);
  replayTree->setPoint(PT_BOTTOM, 0, -10);
  replayTree->setWidth(cfg.splitterPos - 10);

  addressBar = new EditFrame(this, IDC_ADDRESSBAR);
  addressBar->setHeight(23);

  hForward = new ButtonFrame("Forward", this, IDC_FORWARD);
  hForward->setPoint(PT_TOPRIGHT, -10, 10);
  hForward->setSize(60, 23);

  hBack = new ButtonFrame("Back", this, IDC_BACK);
  hBack->setPoint(PT_BOTTOMRIGHT, hForward, PT_BOTTOMLEFT, -4, 0);
  hBack->setSize(60, 23);

  StaticFrame* vline = new StaticFrame(this, 0, SS_ETCHEDVERT);
  vline->setPoint(PT_BOTTOMRIGHT, hBack, PT_BOTTOMLEFT, -3, 0);
  vline->setSize(2, 23);

  ButtonFrame* bOpen = new ButtonFrame("Open", this, IDC_OPEN);
  bOpen->setPoint(PT_BOTTOMRIGHT, vline, PT_BOTTOMLEFT, -3, 0);
  bOpen->setSize(60, 23);

  ButtonFrame* bBrowse = new ButtonFrame("Browse", this, IDC_BROWSE);
  bBrowse->setPoint(PT_BOTTOMRIGHT, bOpen, PT_BOTTOMLEFT, -4, 0);
  bBrowse->setSize(60, 23);

  addressBar->setPoint(PT_TOPLEFT, replayTree, PT_TOPRIGHT, SPLITTER_WIDTH, 0);
  addressBar->setPoint(PT_BOTTOMRIGHT, bBrowse, PT_BOTTOMLEFT, -4, 0);

  views[MAINWND_SETTINGS] = new SettingsWindow(this);
  views[MAINWND_REPLAY] = new ReplayWindow(this);
  views[MAINWND_FOLDER] = new FolderWindow(this, this);
  for (int i = 0; i < MAINWND_NUM_VIEWS; i++)
  {
    views[i]->setPoint(PT_TOPLEFT, addressBar, PT_BOTTOMLEFT, 0, 10);
    views[i]->setPoint(PT_BOTTOMRIGHT, -10, -10);
    views[i]->hide();
  }
  history = NULL;

  pushView(new FolderViewItem(cfg.replayPath));

  hBack->enable(history && history->hasPrev());
  hForward->enable(history && history->hasNext());
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
  pl.showCmd = cfg.wndShow;
  if (cfg.wndX != CW_USEDEFAULT)
  {
    pl.rcNormalPosition.left = cfg.wndX;
    pl.rcNormalPosition.top = cfg.wndY;
  }
  if (cfg.wndWidth != CW_USEDEFAULT)
  {
    pl.rcNormalPosition.right = pl.rcNormalPosition.left + cfg.wndWidth;
    pl.rcNormalPosition.bottom = pl.rcNormalPosition.top + cfg.wndHeight;
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
      cfg.wndShow = pl.showCmd;
      if (pl.showCmd == SW_SHOWNORMAL)
      {
        cfg.wndX = pl.rcNormalPosition.left;
        cfg.wndY = pl.rcNormalPosition.top;
        cfg.wndWidth = pl.rcNormalPosition.right - pl.rcNormalPosition.left;
        cfg.wndHeight = pl.rcNormalPosition.bottom - pl.rcNormalPosition.top;
      }
    }
    break;
  case WM_COMMAND:
    if (HIWORD(wParam) == BN_CLICKED)
    {
      if (LOWORD(wParam) == IDC_BACK || LOWORD(wParam) == IDC_FORWARD)
      {
        if (history)
          history = (LOWORD(wParam) == IDC_BACK ? history->back() : history->forward());
        if (history)
          history->apply(this);
        hBack->enable(history && history->hasPrev());
        hForward->enable(history && history->hasNext());
        return 0;
      }
      else if (LOWORD(wParam) == IDC_OPEN || LOWORD(wParam) == IDC_BROWSE)
      {
        String path;
        if (LOWORD(wParam) == IDC_BROWSE)
        {
          OPENFILENAME ofn;
          memset(&ofn, 0, sizeof ofn);
          ofn.lStructSize = sizeof ofn;
          ofn.hwndOwner = hWnd;
          ofn.lpstrFilter = "Warcraft III Replay Files (*.w3g)\0*.w3g\0All Files\0*\0\0";
          char buf[512] = "";
          ofn.lpstrFile = buf;
          ofn.nMaxFile = sizeof buf;
          ofn.lpstrDefExt = "w3g";
          ofn.lpstrInitialDir = cfg.replayPath;
          ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
          ofn.FlagsEx = OFN_EX_NOPLACESBAR;
          if (!GetOpenFileName(&ofn))
            return 0;
          path = buf;
        }
        else
          path = String::fixPath(addressBar->getText());
        uint32 attr = GetFileAttributes(path);
        if (attr != INVALID_FILE_ATTRIBUTES)
        {
          if (attr & FILE_ATTRIBUTE_DIRECTORY)
            pushView(new FolderViewItem(path));
          else
            pushView(new ReplayViewItem(path));
        }
        return 0;
      }
    }
    break;
  case WM_UPDATEPATH:
    replayTree->setPath(cfg.replayPath);
    return 0;
//////////////////// SPLITTER ////////////////////////
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      if (pt.x > cfg.splitterPos && pt.x < cfg.splitterPos + SPLITTER_WIDTH)
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
      if (pt.x > cfg.splitterPos && pt.x < cfg.splitterPos + SPLITTER_WIDTH)
      {
        SetCapture(hWnd);
        dragPos = pt.x;
        return 0;
      }
    }
    break;
  case WM_MOUSEMOVE:
    if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
    {
      int pos = short(LOWORD(lParam));
      cfg.splitterPos = cfg.splitterPos + pos - dragPos;
      dragPos = pos;
      RECT rc;
      GetClientRect(hWnd, &rc);
      int oldSplitterPos = cfg.splitterPos;
      if (cfg.splitterPos < 50) cfg.splitterPos = 50;
      if (cfg.splitterPos > rc.right - 200) cfg.splitterPos = rc.right - 200;
      dragPos += cfg.splitterPos - oldSplitterPos;
      if (replayTree)
        replayTree->setWidth(cfg.splitterPos - 10);
      return 0;
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
  String title = history->getTitle();
  if (title.isEmpty())
    setText("DotA Replay Manager");
  else
    setText(title + " - DotA Replay Manager");
  hBack->enable(history && history->hasPrev());
  hForward->enable(history && history->hasNext());
}
void MainWnd::setAddress(String text)
{
  addressBar->setText(text);
  replayTree->setCurFile(text);
}
