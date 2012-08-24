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

ReplayWindow::ReplayWindow(Frame* parent)
  : TabFrame(parent)
{
  replay = NULL;
  viewItem = NULL;

  addTab(REPLAY_GAMEINFO, "Game Info", new ReplayGameInfoTab(this));
  addTab(REPLAY_GAMECHAT, "Chat log", new ReplayGameChatTab(this));
  addTab(REPLAY_TIMELINE, "Timeline", new ReplayTimelineTab(this));
  addTab(REPLAY_PLAYERINFO, "Player Info", new ReplayPlayerInfoTab(this));
  addTab(REPLAY_ACTIONS, "Actions", new ReplayActionsTab(this));
  addTab(REPLAY_PLAYERGOLD, "Gold chart", new ReplayPlayerGoldTab(this));
  addTab(REPLAY_PLAYEREXP, "Exp chart", new ReplayPlayerExpTab(this));
}
ReplayWindow::~ReplayWindow()
{
  delete replay;
}

void ReplayWindow::update()
{
  for (int i = 0; i < numTabs(); i++)
    ((ReplayTab*) getTab(i))->setReplay(replay);
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
  delete replay;
  viewItem = NULL;
  replay = W3GReplay::load(path);
  update();
}
void ReplayWindow::setTab(int tab)
{
  setCurSel(tab);
  if (viewItem)
    viewItem->setTab(tab);
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
