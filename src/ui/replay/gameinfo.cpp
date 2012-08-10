#include "core/app.h"
#include "graphics/imagelib.h"
#include "base/version.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "base/mpqfile.h"
#include "dota/consts.h"

#include "gameinfo.h"

#define IDC_MAPIMAGE      100

#define INFO_VERSION      0
#define INFO_MAP          1
#define INFO_LOCATION     2
#define INFO_HOST         3
#define INFO_SAVER        4
#define INFO_LENGTH       5
#define INFO_RATIO        6
#define INFO_SCORE        7
#define INFO_WINNER       8
#define INFO_OBSERVERS    9
#define INFO_SAVED        10
#define INFO_NAME         11
#define INFO_MODE         12

#define PL_NAME           0
#define PL_LEVEL          1
#define PL_ITEM           2
#define PL_KILLS          3
#define PL_DEATHS         4
#define PL_ASSISTS        5
#define PL_CREEPS         6
#define PL_DENIES         7
#define PL_NEUTRALS       8
#define PL_LANE           9
#define PL_ITEMBUILD      10
#define PL_APM            11
#define PL_LEFT           12

#define PL_TEAM           1
#define PL_LAPM           2
#define PL_LLEFT          3

static char infoText[][64] = {
  "Patch version",
  "Map name",
  "Map location",
  "Host name",
  "Replay saver",
  "Game length",
  "Players",
  "Game score (may be wrong)",
  "Winner",
  "Observers",
  "Replay saved",
  "Game name",
  "Game mode"
};

ReplayGameInfoTab::ReplayGameInfoTab(Frame* parent)
  : ReplayTab(parent)
{
  popout = NULL;
  mapImages[0] = NULL;
  mapImages[1] = NULL;
  curImage = 0;
  players = new ListFrame(this);
  players->setPoint(PT_BOTTOMLEFT, 10, -10);
  players->setPoint(PT_BOTTOMRIGHT, -10, -10);
  players->setHeight(250);
  players->show();
  map = new StaticFrame(this, IDC_MAPIMAGE, SS_BITMAP | SS_CENTERIMAGE, WS_EX_CLIENTEDGE);
  map->setPoint(PT_BOTTOMRIGHT, players, PT_TOPRIGHT, 0, -10);
  map->setSize(132, 132);
  info = new SimpleListFrame(this, 0, WS_DISABLED, WS_EX_STATICEDGE);
  info->setPoint(PT_TOPLEFT, 10, 10);
  info->setPoint(PT_BOTTOMRIGHT, map, PT_BOTTOMLEFT, -10, 0);

  mapCanvas = new Image(128, 128);
  mapCanvas->fill(0xFF00FF00);
  HDC hDC = GetDC(map->getHandle());
  mapBitmap = mapCanvas->createBitmap(hDC);
  ReleaseDC(map->getHandle(), hDC);
  map->setImage(mapBitmap, IMAGE_BITMAP);

  info->setColumns(3);
  info->setColumn(0, 10, LVCFMT_LEFT);
  info->setColumn(1, 150, LVCFMT_RIGHT);
  info->setColumn(2, 150, LVCFMT_LEFT);
  info->show();
}
ReplayGameInfoTab::~ReplayGameInfoTab()
{
  delete mapImages[0];
  delete mapImages[1];
  delete mapCanvas;
  if (mapBitmap)
    DeleteObject(mapBitmap);
  delete popout;
}
void ReplayGameInfoTab::addInfo(String name, String value)
{
  int pos = info->addItem("");
  info->setItemText(pos, 1, name);
  info->setItemText(pos, 2, value);
}
void ReplayGameInfoTab::onSetReplay()
{
  players->clearColumns();
  players->clear();
  info->clear();
  delete mapImages[0];
  delete mapImages[1];
  mapImages[0] = NULL;
  mapImages[1] = NULL;
  curImage = 0;
  mapCanvas->fill(0);
  HDC hDC = GetDC(map->getHandle());
  mapCanvas->fillBitmap(mapBitmap, hDC);
  ReleaseDC(map->getHandle(), hDC);
  map->invalidate();
  players->enable(w3g != NULL);
  if (w3g == NULL)
    return;

  MPQArchive* mapArchive = MPQArchive::open(String::buildFullName(
    cfg::warPath, w3g->getGameInfo()->map), MPQFILE_READ);
  if (mapArchive)
  {
    File* file = mapArchive->openFile("war3mapPreview.tga", MPQFILE_READ);
    if (file)
    {
      mapImages[0] = new Image(file);
      delete file;
    }
    file = mapArchive->openFile("war3mapMap.blp", MPQFILE_READ);
    if (file)
    {
      mapImages[1] = new Image(file);
      delete file;
      if (mapImages[1])
      {
        file = mapArchive->openFile("war3map.mmp", MPQFILE_READ);
        if (file)
        {
          addMapIcons(mapImages[1], file);
          delete file;
        }
      }
    }
    delete mapArchive;
  }
  if (mapImages[0] == NULL)
  {
    mapImages[0] = mapImages[1];
    mapImages[1] = NULL;
  }
  if (mapImages[0])
  {
    BLTInfo info(mapImages[0]);
    info.setDstSize(mapCanvas->width(), mapCanvas->height());
    mapCanvas->blt(info);
    HDC hDC = GetDC(map->getHandle());
    mapCanvas->fillBitmap(mapBitmap, hDC);
    ReleaseDC(map->getHandle(), hDC);
  }
  map->invalidate();

  addInfo("Warcraft version", formatVersion(w3g->getVersion()));
  addInfo("Map", String::getFileTitle(w3g->getGameInfo()->map));
  addInfo("Map location", String::getPath(w3g->getGameInfo()->map));
  addInfo("Game name", w3g->getGameInfo()->name);
  if (w3g->getDotaInfo())
    addInfo("Game mode", w3g->getGameInfo()->game_mode);
  addInfo("Host name", w3g->getGameInfo()->creator);
  if (W3GPlayer* saver = w3g->getGameInfo()->saver)
    addInfo("Replay saver", saver->name);
  addInfo("Game length", format_time(w3g->getLength()));
  info->setColumnWidth(2, LVSCW_AUTOSIZE);

  LockWindowUpdate(players->getHandle());

  ImageLibrary* ilib = getApp()->getImageLibrary();

  if (DotaInfo const* dotaInfo = w3g->getDotaInfo())
  {
    players->insertColumn(PL_NAME, "Name");
    players->insertColumn(PL_LEVEL, "Level", LVCFMT_RIGHT);
    players->insertColumn(PL_ITEM, "Cost", LVCFMT_RIGHT);
    players->insertColumn(PL_KILLS, "Kills", LVCFMT_RIGHT);
    players->insertColumn(PL_DEATHS, "Deaths", LVCFMT_RIGHT);
    players->insertColumn(PL_ASSISTS, "Assists", LVCFMT_RIGHT);
    players->insertColumn(PL_CREEPS, "Creeps", LVCFMT_RIGHT);
    players->insertColumn(PL_DENIES, "Denies", LVCFMT_RIGHT);
    players->insertColumn(PL_NEUTRALS, "Neutrals", LVCFMT_RIGHT);
    players->insertColumn(PL_LANE, "Lane");
    players->insertColumn(PL_ITEMBUILD, "Items");
    players->insertColumn(PL_APM, "APM", LVCFMT_RIGHT);
    players->insertColumn(PL_LEFT, "Left", LVCFMT_RIGHT);

    if (dotaInfo->team_size[0])
    {
      players->addItem("Sentinel", ilib->getListIndex("RedBullet"), 0xFF000000);
      for (int i = 0; i < dotaInfo->team_size[0]; i++)
        addPlayer(dotaInfo->teams[0][i]);
    }
    if (dotaInfo->team_size[1])
    {
      players->addItem("Scourge", ilib->getListIndex("GreenBullet"), 0xFF000000);
      for (int i = 0; i < dotaInfo->team_size[1]; i++)
        addPlayer(dotaInfo->teams[1][i]);
    }

    players->setColumnWidth(PL_NAME, 140);
    for (int i = PL_LEVEL; i <= PL_LEFT; i++)
    {
      if (dotaInfo->version < makeVersion(6, 53, 0) && (i == PL_ASSISTS || i == PL_NEUTRALS))
        players->setColumnWidth(i, 0);
      else
        players->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
  }

  LockWindowUpdate(NULL);
}
void ReplayGameInfoTab::addPlayer(W3GPlayer* player)
{
  ImageLibrary* ilib = getApp()->getImageLibrary();
  if (player->hero && w3g->getLength())
  {
    int i = players->addItem(player->name, ilib->getListIndex(player->hero->hero->icon),
      (player->player_id << 24) | getLightColor(player->slot.color));
    players->setItemText(i, PL_LEVEL, String(player->level));
    players->setItemText(i, PL_ITEM, String(player->item_cost));
    if (w3g->getDotaInfo()->endgame)
    {
      players->setItemText(i, PL_KILLS, String(player->stats[STAT_KILLS]));
      players->setItemText(i, PL_DEATHS, String(player->stats[STAT_DEATHS]));
      players->setItemText(i, PL_CREEPS, String(player->stats[STAT_CREEPS]));
      players->setItemText(i, PL_DENIES, String(player->stats[STAT_DENIES]));
      if (w3g->getDotaInfo()->version >= makeVersion(6, 54, 0))
      {
        players->setItemText(i, PL_ASSISTS, String(player->stats[STAT_ASSISTS]));
        players->setItemText(i, PL_NEUTRALS, String(player->stats[STAT_NEUTRALS]));
      }
      String items = "";
      for (int it = 0; it < 6; it++)
      {
        if (player->inv.final[it])
          items.printf("$%d$", ilib->getListIndex(player->inv.final[it]->icon, "Unknown"));
        else if (cfg::showEmptySlots)
          items.printf("$%d$", ilib->getListIndex("EmptySlot"));
      }
      players->setItemText(i, PL_ITEMBUILD, items);
    }
  }
}
uint32 ReplayGameInfoTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND)
  {
    if (HIWORD(wParam) == STN_CLICKED && LOWORD(wParam) == IDC_MAPIMAGE)
    {
      if (mapImages[curImage] && popout == NULL)
      {
        RootWindow::setCapture(this);
        RECT rc;
        GetClientRect(map->getHandle(), &rc);
        ClientToScreen(map->getHandle(), (POINT*) &rc.left);
        ClientToScreen(map->getHandle(), (POINT*) &rc.right);
        rc.left = (rc.left + rc.right - mapImages[curImage]->width()) / 2;
        rc.top = (rc.top + rc.bottom - mapImages[curImage]->height()) / 2;
        rc.right = rc.left + mapImages[curImage]->width();
        rc.bottom = rc.top + mapImages[curImage]->height();
        popout = new MapPopout(&rc, mapImages[curImage]);
        ShowWindow(popout->getHandle(), SW_SHOWNA);
      }
      return 0;
    }
    else if (HIWORD(wParam) == STN_DBLCLK && LOWORD(wParam) == IDC_MAPIMAGE)
    {
      if (mapImages[1 - curImage])
      {
        curImage = 1 - curImage;
        mapCanvas->fill(0);
        BLTInfo info(mapImages[curImage]);
        info.setDstSize(mapCanvas->width(), mapCanvas->height());
        mapCanvas->blt(info);
        HDC hDC = GetDC(map->getHandle());
        mapCanvas->fillBitmap(mapBitmap, hDC);
        ReleaseDC(map->getHandle(), hDC);
        map->invalidate();
      }
      return 0;
    }
  }
  else if (message == WM_LBUTTONUP)
  {
    if (popout)
    {
      delete popout;
      popout = NULL;
      ReleaseCapture();
      return 0;
    }
  }
  return M_UNHANDLED;
}

/////////////////////////////////////////////////

uint32 MapPopout::onWndMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_PAINT)
  {
    PAINTSTRUCT ps;
    RECT rc;
    GetClientRect(hWnd, &rc);

    HDC hDC = BeginPaint(hWnd, &ps);
    BitBlt(hDC, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);

    return 0;
  }
  return Window::onWndMessage(message, wParam, lParam);
}
MapPopout::MapPopout(RECT const* rc, Image* image)
{
  if (WNDCLASSEX* wcx = createclass("MapPopout"))
  {
    wcx->style = CS_DROPSHADOW;
    wcx->hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    RegisterClassEx(wcx);
  }
  create(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, "",
    WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_TOPMOST, NULL);
  HDC hDC = GetDC(hWnd);
  dc = CreateCompatibleDC(hDC);
  bitmap = image->createBitmap(hDC);
  SelectObject(dc, bitmap);
  ReleaseDC(hWnd, hDC);
}
MapPopout::~MapPopout()
{
  DeleteDC(dc);
  DeleteObject(bitmap);
}
