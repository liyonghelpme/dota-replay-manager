#include "core/app.h"

#include "ui/settingswnd.h"
#include "ui/replaywnd.h"
#include "ui/folderwnd.h"
#include "ui/searchwnd.h"
#include "ui/searchres.h"
#include "ui/herochart.h"
#include "frameui/controlframes.h"
#include "graphics/imagelib.h"

#include "replay/cache.h"

#include "ui/updatedlg.h"

#include "mainwnd.h"

#define SPLITTER_WIDTH      10

#define IDC_ADDRESSBAR        741
#define IDC_BROWSE            742
#define IDC_OPEN              743
#define IDC_BACK              744
#define IDC_FORWARD           745

#define ID_TRAY_OPEN          901
#define ID_TRAY_EXIT          902
#define IDI_TRAY              1758
#define WM_TRAYNOTIFY         (WM_USER+674)

#define ID_UPDATE_TIMER       1002

MainWnd::MainWnd()
{
  hIcon = (HICON) LoadImage(getInstance(), MAKEINTRESOURCE(IDI_MAIN),
    IMAGE_ICON, 16, 16, 0);
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

  addressBar = new IconEditFrame(this, IDC_ADDRESSBAR, ES_AUTOHSCROLL);
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
  views[MAINWND_SEARCH] = new SearchWindow(this);
  views[MAINWND_SEARCHRES] = new SearchResults(this);
  views[MAINWND_HEROCHART] = new HeroChartFrame(this);
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

  trayShown = false;
  trayMenu = CreatePopupMenu();

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Open";
  mii.cch = strlen(mii.dwTypeData);
  mii.wID = ID_TRAY_OPEN;
  InsertMenuItem(trayMenu, 0, TRUE, &mii);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Exit";
  mii.cch = strlen(mii.dwTypeData);
  mii.wID = ID_TRAY_EXIT;
  InsertMenuItem(trayMenu, 1, TRUE, &mii);

  FileInfo fi;
  getFileInfo(String::buildFullName(cfg.replayPath, "LastReplay.w3g"), fi);
  lastReplayTime = fi.ftime;
}
MainWnd::~MainWnd()
{
  DestroyMenu(trayMenu);
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

  SetTimer(hWnd, ID_UPDATE_TIMER, 10000, NULL);
}

void MainWnd::createTrayIcon()
{
  if (trayShown)
    return;
  NOTIFYICONDATA nid;
  memset(&nid, 0, sizeof nid);
  nid.cbSize = NOTIFYICONDATA_V3_SIZE;
  nid.hWnd = hWnd;
  nid.uID = IDI_TRAY;
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
  nid.uCallbackMessage = WM_TRAYNOTIFY;
  nid.hIcon = hIcon;
  strcpy_s(nid.szTip, sizeof nid.szTip, getText());
  Shell_NotifyIcon(trayShown ? NIM_MODIFY : NIM_ADD, &nid);
  trayShown = true;
}
void MainWnd::destroyTrayIcon()
{
  if (trayShown)
  {
    NOTIFYICONDATA nid;
    memset(&nid, 0, sizeof nid);
    nid.cbSize = NOTIFYICONDATA_V3_SIZE;
    nid.hWnd = hWnd;
    nid.uID = IDI_TRAY;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    trayShown = false;
  }
}
void MainWnd::trayNotify(String title, String text)
{
  if (trayShown)
  {
    NOTIFYICONDATA nid;
    memset(&nid, 0, sizeof nid);
    nid.cbSize = NOTIFYICONDATA_V3_SIZE;
    nid.hWnd = hWnd;
    nid.uID = IDI_TRAY;

    nid.uFlags = NIF_INFO;
    strcpy_s(nid.szInfo, sizeof nid.szInfo, text);
    strcpy_s(nid.szInfoTitle, sizeof nid.szInfoTitle, title);
    nid.uTimeout = 10000;
    nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIcon(NIM_MODIFY, &nid);
  }
}

extern const char appId[256];

uint32 MainWnd::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_UPDATEVERSION:
    if (cfg.autoUpdate && UpdateDialog::lastVersion > UpdateDialog::thisVersion)
      UpdateDialog::run();
    break;
  case WM_TIMER:
    if (wParam == ID_UPDATE_TIMER)
      UpdateDialog::check();
    break;
  case WM_COPYDATA:
  case WM_COPYDATA_FAKE:
    {
      COPYDATASTRUCT* cd = (COPYDATASTRUCT*) lParam;
      if (cd->dwData == MAINWND_OPEN_REPLAY)
      {
        String path((char*) cd->lpData);
        path.dequote();
        if (cfg.enableUrl && File::isValidURL(path))
          pushView(new ReplayViewItem(path));
        else
        {
          uint32 attr = GetFileAttributes(path);
          if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            pushView(new FolderViewItem(path));
          else
            pushView(new ReplayViewItem(path));
        }
      }
    }
    break;
  case WM_REBUILDTREE:
    replayTree->rebuild(wParam != 0);
    return 0;

  case WM_SETVIEW:
    return (uint32) setView(wParam);
  case WM_GETVIEW:
    return (uint32) getView(wParam);
  case WM_PUSHVIEW:
    pushView((ViewItem*) wParam);
    return 0;

  case WM_DESTROY:
    destroyTrayIcon();
    PostQuitMessage(0);
    break;
  case WM_GETMINMAXINFO:
    {
      MINMAXINFO* mmi = (MINMAXINFO*) lParam;
      mmi->ptMinTrackSize.x = cfg.splitterPos + 609;
      mmi->ptMinTrackSize.y = 580;
    }
    return 0;
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

      if (cfg.useTray && message == WM_SIZE)
      {
        if (wParam == SIZE_MINIMIZED)
        {
          ShowWindow(hWnd, SW_HIDE);
          createTrayIcon();
        }
        else
          destroyTrayIcon();
      }
    }
    break;
  case WM_TRAYNOTIFY:
    switch (LOWORD(lParam))
    {
    case WM_LBUTTONUP:
      ShowWindow(hWnd, SW_SHOW);
      ShowWindow(hWnd, SW_RESTORE);
      break;
    case WM_RBUTTONUP:
      {
        POINT pt;
        GetCursorPos(&pt);
        int result = TrackPopupMenuEx(trayMenu, TPM_HORIZONTAL | TPM_LEFTALIGN |
          TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hWnd, NULL);
        if (result == ID_TRAY_OPEN)
        {
          ShowWindow(hWnd, SW_SHOW);
          ShowWindow(hWnd, SW_RESTORE);
        }
        else if (result == ID_TRAY_EXIT)
          DestroyWindow(hWnd);
      }
      break;
    }
    return 0;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_BACK:
    case IDC_FORWARD:
      if (history)
        history = (LOWORD(wParam) == IDC_BACK ? history->back() : history->forward());
      if (history)
        history->apply(this);
      hBack->enable(history && history->hasPrev());
      hForward->enable(history && history->hasNext());
      return 0;
    case IDOK:
      if ((HWND) lParam != addressBar->getHandle())
        break;
    case IDC_OPEN:
    case IDC_BROWSE:
      {
        SetFocus(hWnd);
        String path;
        if (LOWORD(wParam) == IDC_BROWSE)
        {
          path = getOpenReplayName(hWnd);
          if (path.isEmpty())
            return 0;
        }
        else
        {
          String path = addressBar->getText();
          bool isCmd = true;
          if (path.icompare("Search results") == 0)
            pushView(new SearchResViewItem());
          else if (path.icompare("Search") == 0)
            pushView(new SearchViewItem());
          else if (path.icompare("Settings") == 0)
            pushView(new SettingsViewItem());
          else if (path.icompare("Hero chart") == 0)
            pushView(new HeroChartViewItem());
          else
            isCmd = false;
          if (isCmd)
            return 0;
          path = String::fixPath(path);
        }
        if (cfg.enableUrl && File::isValidURL(path))
          pushView(new ReplayViewItem(path));
        else
        {
          uint32 attr = GetFileAttributes(path);
          if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            pushView(new FolderViewItem(path));
          else
            pushView(new ReplayViewItem(path));
        }
      }
      return 0;
    }
    break;  
  case WM_UPDATEPATH:
    {
      FileInfo fi;
      if (getFileInfo(String::buildFullName(cfg.replayPath, "LastReplay.w3g"), fi))
        lastReplayTime = fi.ftime;
      else
        lastReplayTime = 0;
      replayTree->setPath(cfg.replayPath);
    }
    return 0;
  case WM_UPDATEFILE:
    if (cfg.autoView || cfg.autoCopy)
    {
      FileInfo fi;
      if (getFileInfo(String::buildFullName(cfg.replayPath, "LastReplay.w3g"), fi) &&
        fi.ftime != lastReplayTime)
      {
        if (cfg.autoView)
        {
          pushView(new ReplayViewItem(fi.path));
          GameCache* cache = getApp()->getCache()->getGameNow(fi.path);
          if (cache)
            trayNotify("New replay", cache->game_name);
        }
        if (cfg.autoCopy)
        {
          GameCache* cache = getApp()->getCache()->getGame(fi.path);
          if (cache)
          {
            String path = cache->format(cfg.copyFormat, cfg.replayPath);
            path = String::fixPath(String::buildFullName(cfg.replayPath, path));
            if (createPath(String::getPath(path)))
            {
              if (CopyFile(fi.path, path, FALSE))
                getApp()->getCache()->duplicate(path, cache);
            }
            if (!cfg.autoView)
              trayNotify("New replay", cache->game_name);
          }
        }
      }
      else
      lastReplayTime = fi.ftime;
    }
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
      if (cfg.splitterPos > rc.right - 600) cfg.splitterPos = rc.right - 600;
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
Frame* MainWnd::getView(int view)
{
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
  replayTree->setCurFile(text);
  String icon = "";
  if (text == "#settings")
  {
    addressBar->setText("Settings");
    icon = "IconSettings";
  }
  else if (text == "#search")
  {
    addressBar->setText("Search");
    icon = "IconSearch";
  }
  else if (text == "#searchres")
  {
    addressBar->setText("Search results");
    icon = "IconSearchResults";
  }
  else if (text == "#herochart")
  {
    addressBar->setText("Hero chart");
    icon = "IconHeroChart";
  }
  else
  {
    addressBar->setText(text);
    if (cfg.enableUrl && File::isValidURL(text))
      icon = "";
    else
    {
      uint32 attr = GetFileAttributes(text);
      if (attr != INVALID_FILE_ATTRIBUTES)
      {
        if (attr & FILE_ATTRIBUTE_DIRECTORY)
          icon = "IconClosedFolder";
        else
          icon = "IconReplay";
      }
    }
  }
  if (!icon.isEmpty())
    addressBar->setIcon(getApp()->getImageLibrary()->getListIndex(icon));
}
void MainWnd::setTreeAddress(String text)
{
  replayTree->setCurFile(text);
}
