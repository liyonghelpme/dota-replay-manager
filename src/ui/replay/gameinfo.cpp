#include "core/app.h"
#include "graphics/imagelib.h"
#include "base/version.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "base/mpqfile.h"
#include "dota/consts.h"
#include "frameui/dragdrop.h"

#include "gameinfo.h"

#define IDC_MAPIMAGE          100
#define IDC_WATCHREPLAY       101
#define IDC_COPYMATCHUP       102

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
  players->setColorMode(ListFrame::colorParam);
  players->setPoint(PT_BOTTOMLEFT, 10, -10);
  players->setPoint(PT_BOTTOMRIGHT, -10, -10);
  players->setHeight(250);
  players->show();

  info = new SimpleListFrame(this, 0, LVS_ALIGNLEFT | LVS_REPORT |
    LVS_NOCOLUMNHEADER | LVS_NOSCROLL | LVS_SINGLESEL | WS_DISABLED, WS_EX_STATICEDGE);
  info->setPoint(PT_TOPLEFT, 10, 10);
  info->setPoint(PT_TOPRIGHT, -200, 10);
  info->setPoint(PT_BOTTOM, players, PT_TOP, 0, -10);

  watchReplay = new ButtonFrame("Watch replay", this, IDC_WATCHREPLAY);
  watchReplay->setHeight(23);
  watchReplay->setPoint(PT_TOPLEFT, info, PT_TOPRIGHT, 6, 0);
  watchReplay->setPoint(PT_TOPRIGHT, -10, 10);

  copyMatchup = new ButtonFrame("Copy matchup", this, IDC_COPYMATCHUP);
  copyMatchup->setHeight(23);
  copyMatchup->setPoint(PT_TOPLEFT, watchReplay, PT_BOTTOMLEFT, 0, 5);
  copyMatchup->setPoint(PT_TOPRIGHT, watchReplay, PT_BOTTOMRIGHT, 0, 5);

  map = new StaticFrame(this, IDC_MAPIMAGE, SS_BITMAP | SS_CENTERIMAGE, WS_EX_CLIENTEDGE);
  map->setPoint(PT_TOP, copyMatchup, PT_BOTTOM, 0, 10);
  map->setSize(132, 132);

  mapCanvas = new Image(128, 128);
  mapCanvas->fill(Image::clr(0, 255, 0));
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
void ReplayGameInfoTab::addInfo(String name, String value, bool utf8)
{
  int pos = info->addItem("");
  info->setItemText(pos, 1, name);
  if (utf8)
    info->setItemTextUtf8(pos, 2, value);
  else
    info->setItemText(pos, 2, value);
}
String ReplayGameInfoTab::getWatchCmd()
{
  if (w3g == NULL)
  {
    watchReplay->setText("Watch replay");
    return "";
  }
  if (w3g->getFileInfo() == NULL)
  {
    watchReplay->setText("Unknown file location");
    return "";
  }
  if (cfg.warPath.isEmpty())
  {
    watchReplay->setText("Warcraft III not found");
    return "";
  }
  String exe = String::buildFullName(cfg.warPath, "War3.exe");
  FileInfo fi;
  if (!getFileInfo(exe, fi))
  {
    watchReplay->setText("Warcraft III not found");
    return "";
  }
  String cmd = String::format("\"%s\" -loadfile \"%s\"", exe, w3g->getFileInfo()->path);
  if (cfg.viewWindow)
    cmd += " -window";
  return cmd;
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
  mapCanvas->fill(Image::clr(0, 0, 0));
  HDC hDC = GetDC(map->getHandle());
  mapCanvas->fillBitmap(mapBitmap, hDC);
  ReleaseDC(map->getHandle(), hDC);
  map->invalidate();
  players->enable(w3g != NULL);
  watchReplay->enable(!getWatchCmd().isEmpty());
  copyMatchup->enable(w3g != NULL);
  if (w3g == NULL)
    return;

  MPQArchive* mapArchive = MPQArchive::open(String::buildFullName(
    cfg.warPath, w3g->getGameInfo()->map), MPQFILE_READ);
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

  if (w3g->getFileInfo())
    addInfo("Replay saved", format_systime(w3g->getFileInfo()->ftime, "%c"));
  addInfo("Warcraft version", formatVersion(w3g->getVersion()));
  addInfo("Map", String::getFileTitle(w3g->getGameInfo()->map));
  addInfo("Map location", String::getPath(w3g->getGameInfo()->map));
  addInfo("Game name", w3g->getGameInfo()->name, true);
  if (w3g->getDotaInfo())
    addInfo("Game mode", w3g->getGameInfo()->game_mode);
  addInfo("Host name", w3g->getGameInfo()->creator, true);
  if (W3GPlayer* saver = w3g->getGameInfo()->saver)
    addInfo("Replay saver", saver->name, true);
  addInfo("Game length", format_time(w3g->getLength()));
  info->setColumnWidth(2, LVSCW_AUTOSIZE);

  String observers = "";
  for (int i = 0; i < w3g->getNumPlayers(); i++)
  {
    W3GPlayer* player = w3g->getPlayer(i);
    if (player->slot.color > 11 || player->slot.slot_status == 0)
    {
      if (!observers.isEmpty()) observers += ", ";
      observers += player->name;
    }
  }
  if (!observers.isEmpty())
    addInfo("Observers", observers, true);

  LockWindowUpdate(players->getHandle());

  ImageLibrary* ilib = getApp()->getImageLibrary();

  if (DotaInfo const* dotaInfo = w3g->getDotaInfo())
  {
    addInfo("Players", String::format("%dv%d", dotaInfo->team_size[0], dotaInfo->team_size[1]));
    addInfo("Score", String::format("%d/%d", dotaInfo->team_kills[0], dotaInfo->team_kills[1]));
    if (w3g->getGameInfo()->winner == WINNER_UNKNOWN)
      addInfo("Winner", "Unknown");
    else if (w3g->getGameInfo()->winner == WINNER_SENTINEL)
      addInfo("Winner", "Sentinel");
    else if (w3g->getGameInfo()->winner == WINNER_GSENTINEL ||
             w3g->getGameInfo()->winner == WINNER_PSENTINEL)
      addInfo("Winner (guess)", "Sentinel");
    else if (w3g->getGameInfo()->winner == WINNER_SCOURGE)
      addInfo("Winner", "Scourge");
    else if (w3g->getGameInfo()->winner == WINNER_GSCOURGE ||
             w3g->getGameInfo()->winner == WINNER_PSCOURGE)
      addInfo("Winner (guess)", "Scourge");

    players->insertColumn(PL_NAME, "Name");
    players->setColumnUTF8(PL_NAME, true);
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

    for (int i = 0; i <= PL_LEFT; i++)
    {
      if (dotaInfo->version < makeVersion(6, 53, 0) && (i == PL_ASSISTS || i == PL_NEUTRALS))
        players->setColumnWidth(i, 0);
      else
        players->setColumnWidth(i, cfg.giColWidth[i]);
    }
  }
  else
  {
    players->insertColumn(PL_NAME, "Name");
    players->setColumnUTF8(PL_NAME, true);
    players->insertColumn(PL_TEAM, "Team");
    players->insertColumn(PL_LAPM, "APM", LVCFMT_RIGHT);
    players->insertColumn(PL_LLEFT, "Left", LVCFMT_RIGHT);

    if (w3g->getGameInfo()->ladder_winner < 0)
      addInfo("Winner", "Unknown");
    else if (w3g->getGameInfo()->ladder_wplayer)
      addInfo("Winner", w3g->getGameInfo()->ladder_wplayer->name, true);
    else
      addInfo("Winner", String::format("Team %d", w3g->getGameInfo()->ladder_winner + 1));

    for (int i = 0; i < w3g->getNumPlayers(); i++)
      addLadderPlayer(w3g->getPlayer(i));

    for (int i = 0; i <= PL_LLEFT; i++)
      players->setColumnWidth(i, cfg.giLColWidth[i]);
  }

  LockWindowUpdate(NULL);
}
void ReplayGameInfoTab::addPlayer(W3GPlayer* player)
{
  ImageLibrary* ilib = getApp()->getImageLibrary();
  int i;
  if (player->hero && w3g->getLength())
  {
    i = players->addItem(player->name, ilib->getListIndex(player->hero->hero->icon),
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
      players->setItemText(i, PL_LANE, getLaneName(player->lane));
      String items = "";
      for (int it = 0; it < 6; it++)
      {
        if (player->inv.final[it])
          items.printf("$%d$", ilib->getListIndex(player->inv.final[it]->icon, "Unknown"));
        else if (cfg.showEmptySlots)
          items.printf("$%d$", ilib->getListIndex("EmptySlot"));
      }
      players->setItemText(i, PL_ITEMBUILD, items);
    }
  }
  else
    i = players->addItem(player->name, ilib->getListIndex("Empty"),
      (player->player_id << 24) | getLightColor(player->slot.color));
  players->setItemText(i, PL_APM, String(player->apm()));
  if (player->time >= w3g->getLength() || player->player_id >= 0x80)
    players->setItemText(i, PL_LEFT, "End");
  else
    players->setItemText(i, PL_LEFT, w3g->formatTime(player->time));
}
void ReplayGameInfoTab::addLadderPlayer(W3GPlayer* player)
{
  if (player->slot.color > 11 || player->slot.slot_status == 0)
    return;
  ImageLibrary* ilib = getApp()->getImageLibrary();
  int i = players->addItem(player->name,
    ilib->getListIndex(getRaceIcon(player->race)),
    (player->player_id << 24) | getLightColor(player->slot.color));
  players->setItemText(i, PL_TEAM, String::format("Team %d", player->slot.team + 1));
  players->setItemText(i, PL_LAPM, String(player->apm()));
  if (player->time >= w3g->getLength() || player->player_id >= 0x80)
    players->setItemText(i, PL_LLEFT, "End");
  else
    players->setItemText(i, PL_LLEFT, w3g->formatTime(player->time));
}
uint32 ReplayGameInfoTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_NOTIFY)
  {
    NMHDR* hdr = (NMHDR*) lParam;
    if (hdr->code == HDN_ENDTRACK)
    {
      NMHEADER* header = (NMHEADER*) hdr;
      if (header->iItem >= 0 && header->iItem <= PL_LEFT &&
          header->pitem && header->pitem->mask & HDI_WIDTH)
      {
        if (w3g && w3g->getDotaInfo())
          cfg.giColWidth[header->iItem] = header->pitem->cxy;
        else if (header->iItem <= PL_LLEFT)
          cfg.giLColWidth[header->iItem] = header->pitem->cxy;
      }
    }
  }
  else if (message == WM_COMMAND)
  {
    switch (LOWORD(wParam))
    {
    case IDC_MAPIMAGE:
      if (HIWORD(wParam) == STN_CLICKED)
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
      }
      else if (HIWORD(wParam) == STN_DBLCLK)
      {
        if (mapImages[1 - curImage])
        {
          curImage = 1 - curImage;
          mapCanvas->fill(Image::clr(0, 0, 0));
          BLTInfo info(mapImages[curImage]);
          info.setDstSize(mapCanvas->width(), mapCanvas->height());
          mapCanvas->blt(info);
          HDC hDC = GetDC(map->getHandle());
          mapCanvas->fillBitmap(mapBitmap, hDC);
          ReleaseDC(map->getHandle(), hDC);
          map->invalidate();
        }
      }
      return 0;
    case IDC_WATCHREPLAY:
      {
        String cmd = getWatchCmd();
        if (!cmd.isEmpty())
        {
          STARTUPINFO info;
          GetStartupInfo(&info);
          PROCESS_INFORMATION pi;
          CreateProcess(NULL, cmd.getBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &info, &pi);
          CloseHandle(pi.hProcess);
          CloseHandle(pi.hThread);
        }
      }
      return 0;
    case IDC_COPYMATCHUP:
      if (w3g)
      {
        String result = "";
        int marked[12];
        memset(marked, 0, sizeof marked);
        for (int i = 0; i < 12; i++)
        {
          if (marked[i])
            continue;
          W3GPlayer* team = w3g->getPlayerInSlot(i);
          if (team)
          {
            if (!result.isEmpty())
              result += " -VS- ";
            bool first = true;
            for (int j = 0; j < 12; j++)
            {
              W3GPlayer* player = w3g->getPlayerInSlot(j);
              if (player && player->slot.team == team->slot.team)
              {
                marked[j] = 1;
                if (!first)
                  result += ", ";
                first = false;
                result += player->name;
                if (w3g->getDotaInfo() && player->hero)
                {
                  String abbr = getApp()->getDotaLibrary()->getHeroAbbreviation(player->hero->hero->point);
                  if (abbr.isEmpty())
                    result += String::format(" (%s)", player->hero->hero->shortName);
                  else
                    result += String::format(" (%s)", abbr);
                }
              }
            }
          }
        }
        if (!result.isEmpty())
          SetClipboard(CF_TEXT, CreateGlobalText(result));
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
