#include "core/app.h"
#include "frameui/fontsys.h"

#include "ui/viewitem.h"
#include "replaywnd.h"

#include "ui/replay/gameinfo.h"
#include "ui/replay/gamechat.h"
#include "ui/replay/timelinewnd.h"
#include "ui/replay/playerinfo.h"
#include "ui/replay/actions.h"
#include "ui/replay/playergold.h"
#include "ui/replay/playerexp.h"
#include "ui/replay/present.h"
#include "ui/replay/draft.h"
#include "ui/replay/actionlog.h"

static char tabNames[][64] = {
  "Game info",
  "Chat log",
  "Timeline",
  "Builds",
  "Actions",
  "Gold chart",
  "Exp chart",
  "Presentation",
  "Draft info",
  "Action log",
};

ReplayWindow::ReplayWindow(Frame* parent)
  : TabFrame(parent)
{
  replay = NULL;
  viewItem = NULL;

  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    frametab[i] = -1;
  frames[REPLAY_GAMEINFO] = new ReplayGameInfoTab(this);
  frames[REPLAY_GAMECHAT] = new ReplayGameChatTab(this);
  frames[REPLAY_TIMELINE] = new ReplayTimelineTab(this);
  frames[REPLAY_PLAYERINFO] = new ReplayPlayerInfoTab(this);
  frames[REPLAY_ACTIONS] = new ReplayActionsTab(this);
  frames[REPLAY_PLAYERGOLD] = new ReplayPlayerGoldTab(this);
  frames[REPLAY_PLAYEREXP] = new ReplayPlayerExpTab(this);
  frames[REPLAY_PRESENT] = new ReplayPresentTab(this);
  frames[REPLAY_DRAFT] = new ReplayDraftTab(this);
  frames[REPLAY_ACTIONLOG] = new ReplayActionLogTab(this);

  failure = new StaticFrame("Failed to open replay", this);
  failure->setFont(FontSys::changeSize(24));
  failure->resetSize();
  failure->setPoint(PT_BOTTOM, this, PT_CENTER, 0, -5);
  failure->hide();

  failureInfo = new StaticFrame("", this);
  failureInfo->setFont(FontSys::changeSize(18));
  failureInfo->resetSize();
  failureInfo->setPoint(PT_TOP, this, PT_CENTER, 0, 5);
  failureInfo->hide();
}
ReplayWindow::~ReplayWindow()
{
  delete replay;
}

void ReplayWindow::addFrame(int tab)
{
  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    frametab[i] = -1;
  frametab[tab] = tabs.length();
  addTab(tabNames[tab], frames[tab]);
}
void ReplayWindow::update()
{
  clear();
  if (replay)
  {
    failure->hide();
    failureInfo->hide();

    DotaInfo const* dotaInfo = replay->getDotaInfo();

    addFrame(REPLAY_GAMEINFO);
    addFrame(REPLAY_GAMECHAT);
    if (dotaInfo)
    {
      addFrame(REPLAY_TIMELINE);
      addFrame(REPLAY_PLAYERINFO);
    }
    addFrame(REPLAY_ACTIONS);
    if (dotaInfo)
    {
      addFrame(REPLAY_PLAYERGOLD);
      addFrame(REPLAY_PLAYEREXP);
    }
    addFrame(REPLAY_PRESENT);

    if (dotaInfo && (dotaInfo->draft.numPool || dotaInfo->draft.numPicks[0] ||
        dotaInfo->draft.numPicks[1] || dotaInfo->draft.numBans[0] || dotaInfo->draft.numBans[1]))
      addFrame(REPLAY_DRAFT);

    addFrame(REPLAY_ACTIONLOG);
  }
  else
  {
    failure->show();
    failureInfo->show();
  }
  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    frames[i]->setReplay(replay);
}

void ReplayWindow::openReplay(File* file)
{
  delete replay;
  viewItem = NULL;
  replay = W3GReplay::load(file);
  update();
}
void ReplayWindow::openReplay(String path)
{
  if (FileInfo* info = (replay ? replay->getFileInfo() : NULL))
  {
    if (info->path.icompare(path) == 0)
    {
      FileInfo cur;
      getFileInfo(path, cur);
      if (cur.ftime == info->ftime)
        return;
    }
  }
  delete replay;
  viewItem = NULL;
  uint32 error;
  replay = W3GReplay::load(path, false, &error);
  if (replay == NULL)
  {
    switch (error)
    {
    case W3GReplay::eNoFile:
      failureInfo->setText("File not found");
      break;
    case W3GReplay::eBadFile:
      failureInfo->setText("Failed to parse file");
      break;
    case W3GReplay::eNoMap:
      failureInfo->setText("No map data");
      break;
    }
    failureInfo->resetSize();
  }
  update();
}
void ReplayWindow::setTab(int tab)
{
  if (frametab[tab] >= 0)
  {
    setCurSel(frametab[tab]);
    if (viewItem)
      viewItem->setTab(tab);
  }
}
void ReplayWindow::setPlayer(int id)
{
  if (replay)
  {
    W3GPlayer* player = replay->getPlayerById(id);
    if (player)
    {
      for (int i = 0; i < numTabs(); i++)
        ((ReplayTab*) getTab(i))->setPlayer(player);
    }
  }
  if (viewItem)
    viewItem->setPlayer(id);
}
uint32 ReplayWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND && LOWORD(wParam) == ID_PLAYERBOX && HIWORD(wParam) == CBN_SELCHANGE)
  {
    ComboFrameEx* box = dynamic_cast<ComboFrameEx*>(Window::fromHandle((HWND) lParam));
    if (box)
    {
      W3GPlayer* player = (W3GPlayer*) box->getItemData(box->getCurSel());
      for (int i = 0; i < numTabs(); i++)
        ((ReplayTab*) getTab(i))->setPlayer(player);
      if (player && viewItem)
        viewItem->setPlayer(player->player_id);
    }
    return TRUE;
  }
  else if (message == WM_NOTIFY && viewItem)
  {
    NMHDR* hdr = (NMHDR*) lParam;
    if (hdr->hwndFrom == hWnd && hdr->code == TCN_SELCHANGE)
    {
      if (viewItem)
        viewItem->setTab(getCurSel());
    }
  }
  return TabFrame::onMessage(message, wParam, lParam);
}
