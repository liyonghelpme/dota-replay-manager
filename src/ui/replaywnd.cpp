#include "core/app.h"
#include "frameui/fontsys.h"

#include "ui/viewitem.h"
#include "replaywnd.h"

#include "ui/replay/gameinfo.h"
#include "ui/replay/gamechat.h"
#include "ui/replay/timelinewnd.h"

void ReplayWindow::setTab(int i, String name, ReplayTab* tab)
{
  tabs[i] = tab;

  TCITEM item;
  memset(&item, 0, sizeof item);
  item.mask = TCIF_TEXT;
  item.pszText = name.getBuffer();
  TabCtrl_InsertItem(hWnd, i, &item);
  if (i != 0)
    tab->hide();
}

ReplayWindow::ReplayWindow(Window* parent)
{
  replay = NULL;
  viewItem = NULL;
  curTab = 0;
  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    tabs[i] = NULL;
  subclass(WC_TABCONTROL, 0, 0, 10, 10, "", WS_CHILD | WS_VISIBLE, 0, parent->getHandle());
  setFont(FontSys::getSysFont());

  setTab(REPLAY_GAMEINFO, "Game Info", new ReplayGameInfoTab(this));
  setTab(REPLAY_GAMECHAT, "Chat log", new ReplayGameChatTab(this));
  setTab(REPLAY_TIMELINE, "Timeline", new ReplayTimelineTab(this));

  SendMessage(hWnd, WM_SIZE, 0, 0);
}
ReplayWindow::~ReplayWindow()
{
  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    delete tabs[i];
  delete replay;
}

void ReplayWindow::update()
{
  for (int i = 0; i < REPLAY_NUM_TABS; i++)
    tabs[i]->setReplay(replay);
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
  if (viewItem)
    viewItem->setTab(tab);
}

uint32 ReplayWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFYREFLECT:
    {
      NMHDR* hdr = (NMHDR*) lParam;
      if (hdr->code == TCN_SELCHANGE)
      {
        curTab = TabCtrl_GetCurSel(hWnd);
        for (int i = 0; i < REPLAY_NUM_TABS; i++)
          tabs[i]->show(i == curTab);
        return TRUE;
      }
    }
    break;
  case WM_SIZE:
    {
      RECT rc;
      GetClientRect(hWnd, &rc);
      TabCtrl_AdjustRect(hWnd, FALSE, &rc);
      for (int i = 0; i < REPLAY_NUM_TABS; i++)
      {
        tabs[i]->setPoint(PT_TOPLEFT, rc.left, rc.top);
        tabs[i]->setPoint(PT_BOTTOMRIGHT, NULL, PT_TOPLEFT, rc.right, rc.bottom);
      }
    }
    break;
  }
  if (curTab >= 0 && curTab < REPLAY_NUM_TABS && tabs[curTab])
    tabs[curTab]->onMessage(message, wParam, lParam);
  return FrameWindow::onMessage(message, wParam, lParam);
}
