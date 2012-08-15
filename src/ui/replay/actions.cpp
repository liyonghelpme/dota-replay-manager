#include "core/app.h"
#include "graphics/imagelib.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "dota/consts.h"
#include "frameui/fontsys.h"

#include "actions.h"

#include "graphics/glib.h"

#define BAR_WIDTH           170
#define BAR_HEIGHT          6

class ActionChart : public WindowFrame
{
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  W3GPlayer* player;
  W3GReplay* w3g;
public:
  ActionChart(Frame* parent)
    : WindowFrame(parent)
  {
    if (WNDCLASSEX* wcx = createclass("ActionChart"))
    {
      wcx->hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
      wcx->style |= CS_HREDRAW | CS_VREDRAW;
      RegisterClassEx(wcx);
    }
    player = NULL;
    create("", WS_CHILD, WS_EX_CLIENTEDGE);
  }
  void setReplay(W3GReplay* replay)
  {
    w3g = replay;
    invalidate();
  }
  void setPlayer(W3GPlayer* p)
  {
    player = p;
    invalidate();
  }
};
#define BUCKET_WIDTH        2
uint32 ActionChart::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_PAINT)
  {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    SelectObject(hDC, FontSys::getSysFont());

    RECT rc;
    GetClientRect(hWnd, &rc);
    if (player && w3g && player->time > 1000 && rc.right > 50 && rc.bottom > 50)
    {
      Array<int> buckets;
      buckets.resize((rc.right - 30) / BUCKET_WIDTH + 1, 0);
      int mxHeight = 0;
      for (int i = 0; i < player->actions.count(); i++)
      {
        int bucket = player->actions[i] * (rc.right - 30) / (player->time * BUCKET_WIDTH);
        if (bucket >= 0 && bucket < buckets.length())
          buckets[bucket]++;
      }
      for (int i = 0; i < buckets.length(); i++)
        if (buckets[i] > mxHeight)
          mxHeight = buckets[i];
      if (mxHeight > 0)
      {      
        SetBkColor(hDC, 0x000000);
        RECT bar;
        bar.left = 30;
        bar.right = 31;
        bar.top = 0;
        bar.bottom = rc.bottom - 20;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &bar, NULL, 0, NULL);
        bar.left = 30;
        bar.right = rc.right;
        bar.top = rc.bottom - 20;
        bar.bottom = rc.bottom - 19;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &bar, NULL, 0, NULL);

        for (int i = 0; i < buckets.length(); i++)
        {
          bar.left = i * BUCKET_WIDTH + 30;
          bar.right = bar.left + 1;
          bar.bottom = rc.bottom - 20;
          bar.top = bar.bottom - (rc.bottom - 30) * buckets[i] / mxHeight;
          ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &bar, NULL, 0, NULL);
        }

        SetBkColor(hDC, 0xFFFFFF);
        SetTextColor(hDC, 0x000000);

        int mxApm = (mxHeight * 60000 * (rc.right - 30) / (BUCKET_WIDTH * player->time));

        EnumStruct es;
        enumCount(es);
        while (es.val * (rc.bottom - 30) / mxApm < 15)
          nextCount(es);
        for (int count = 0; count < mxApm; count += es.val)
        {
          int y = rc.bottom - 20 - count * (rc.bottom - 30) / mxApm;
          RECT txt;
          txt.left = 0;
          txt.right = 28;
          txt.top = y - 10;
          txt.bottom = y + 10;
          DrawText(hDC, String(count), -1, &txt, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
        }

        enumTime(es);
        while (es.val * (rc.right - 30) / player->time < 40)
          nextTime(es);
        uint32 start = 0;
        if (w3g->getDotaInfo())
          start = w3g->getDotaInfo()->start_time;
        for (uint32 time = start % es.val; time < player->time; time += es.val)
        {
          int x = 30 + time * (rc.right - 30) / player->time;
          RECT txt;
          txt.left = x - 20;
          txt.right = x + 20;
          txt.top = rc.bottom - 15;
          txt.bottom = rc.bottom;
          DrawText(hDC, w3g->formatTime(time), -1, &txt, DT_CENTER | DT_SINGLELINE | DT_TOP);
        }
      }
    }
//
//struct EnumStruct
//{
//  int val;
//  int id;
//  int base;
//  int sub;
//};
//void enumCount(EnumStruct& e);
//void enumTime(EnumStruct& e);
//void nextCount(EnumStruct& e);
//void nextTime(EnumStruct& e);
    //EnumStruct es;
    //enumCount (es);
    //while (double (es.val) * sy < 15)
    //  nextCount (es);
    //int cury = rc.bottom - 20;
    //int stepy = int (double (es.val) * sy + 0.5);
    //for (int i = 0; i <= agraphmx + es.val; i += es.val, cury -= stepy)
    //  dcp.drawText (28, cury, mprintf ("%d", i), ALIGN_Y_CENTER | ALIGN_X_RIGHT);

    //enumTime (es);
    //while (double (es.val) * sx < 40)
    //  nextTime (es);
    //int curx = 30;
    //int stepx = int (double (es.val) * sx + 0.5);
    //int _start = w3g->game.startTime;
    //for (unsigned long i = _start - (_start / es.val) * es.val; i < w3g->time; i += es.val)
    //{
    //  int curx = 30 + int (double (i) * sx + 0.5);
    //  dcp.drawText (curx, rc.bottom - 15, format_time (w3g, i, TIME_HOURS | TIME_SECONDS), ALIGN_X_CENTER | ALIGN_Y_TOP);
    //}

    EndPaint(hWnd, &ps);
    return 0;
  }
  return M_UNHANDLED;
}

ReplayActionsTab::ReplayActionsTab(Frame* parent)
  : ReplayTab(parent)
{
  players = new ComboFrameEx(this, ID_PLAYERBOX);
  players->setPoint(PT_TOP, 0, 10);
  players->setWidth(250);

  actions = new SimpleListFrame(this, 0, WS_DISABLED | LVS_REPORT | LVS_OWNERDRAWFIXED |
    LVS_NOCOLUMNHEADER | LVS_NOSCROLL);
  actions->setPoint(PT_TOP, players, PT_BOTTOM, 0, 5);
  actions->setSize(400, 150);

  hotkeys = new SimpleListFrame(this, 0, LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL);
  ListView_SetExtendedListViewStyle(hotkeys->getHandle(), LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  hotkeys->setPoint(PT_TOPLEFT, actions, PT_BOTTOMLEFT, 0, 5);
  hotkeys->setPoint(PT_TOPRIGHT, actions, PT_BOTTOMRIGHT, 0, 5);
  hotkeys->setPointEx(PT_BOTTOM, 0, 0.6, 0, 0);

  afkinfo = new StaticFrame("Longest AFK period:", this);
  afkinfo->setPoint(PT_TOPLEFT, hotkeys, PT_BOTTOMLEFT, 0, 5);

  chart = new ActionChart(this);
  chart->setPoint(PT_TOP, afkinfo, PT_BOTTOM, 0, 5);
  chart->setPoint(PT_BOTTOMLEFT, 10, -10);
  chart->setPoint(PT_BOTTOMRIGHT, -10, -10);

  actions->setColumns(3);
  actions->setColumnWidth(0, 150);
  actions->setColumnWidth(1, 50);
  actions->setColumnWidth(2, 180);
  int pos = actions->addItem("Test", BAR_WIDTH);
  actions->setItemText(pos, 1, "20");

  hotkeys->setColumns(3);
  hotkeys->setColumn(0, 100, "Group");
  hotkeys->setColumn(1, 100, "Assigned");
  hotkeys->setColumn(2, 100, "Used");
}

void ReplayActionsTab::setPlayer(W3GPlayer* player)
{
  for (int i = 0; i < players->getCount(); i++)
  {
    if (players->getItemData(i) == (uint32) player)
    {
      players->setCurSel(i);
      break;
    }
  }

  actions->clear();
  hotkeys->clear();
  if (player)
  {
    int maxActions = 0;
    for (int i = 0; i < NUM_ACTIONS; i++)
      if (player->actions.getActionCounter(i) > maxActions)
        maxActions = player->actions.getActionCounter(i);
    for (int i = 0; i < NUM_ACTIONS; i++)
    {
      int width = 0;
      if (maxActions)
        width = BAR_WIDTH * player->actions.getActionCounter(i) / maxActions;
      int pos = actions->addItem(action_name(i), width);
      actions->setItemText(pos, 1, String(player->actions.getActionCounter(i)));
    }
    for (int i = 0; i < 10; i++)
    {
      if (player->actions.getHotkeyAssign(i) || player->actions.getHotkeyUse(i))
      {
        int pos = hotkeys->addItem(String(i + 1));
        hotkeys->setItemText(pos, 1, String(player->actions.getHotkeyAssign(i)));
        hotkeys->setItemText(pos, 2, String(player->actions.getHotkeyUse(i)));
      }
    }
    uint32 afkStart = 0;
    uint32 afkEnd = 0;
    for (int i = 0; i <= player->actions.count(); i++)
    {
      uint32 start = (i == 0 ? 0 : player->actions[i - 1]);
      uint32 end = (i == player->actions.count() ? player->time : player->actions[i]);
      if (end - start > afkEnd - afkStart)
      {
        afkStart = start;
        afkEnd = end;
      }
    }
    if (afkEnd - afkStart > 10000)
      afkinfo->setText(String::format("Longest AFK time: %s (from %s to %s)",
        format_time(afkEnd - afkStart), w3g->formatTime(afkStart), w3g->formatTime(afkEnd)));
    else
      afkinfo->setText("Longest AFK time: none");
    afkinfo->resetSize();
  }
  chart->setPlayer(player);
}
void ReplayActionsTab::onSetReplay()
{
  players->reset();
  chart->setReplay(w3g);
  if (w3g && w3g->getDotaInfo())
  {
    players->enable();
    DotaInfo const* info = w3g->getDotaInfo();

    for (int t = 0; t < 2; t++)
    {
      if (info->team_size[t])
        players->addString(t ? "Scourge" : "Sentinel", 0, t ? "GreenBullet" : "RedBullet");
      for (int i = 0; i < info->team_size[t]; i++)
      {
        W3GPlayer* player = info->teams[t][i];
        if (player->hero)
          players->addString(String::format("%s (%s)", player->name, player->hero->hero->name),
            getLightColor(player->slot.color), player->hero->hero->icon, (uint32) player);
        else
          players->addString(String::format("%s (No Hero)", player->name),
            getLightColor(player->slot.color), "Empty", (uint32) player);
      }
    }

    setPlayer((W3GPlayer*) players->getItemData(1));
  }
  else
  {
    players->disable();
  }
}

uint32 ReplayActionsTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message != WM_DRAWITEM)
    return M_UNHANDLED;
  DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*) lParam;
  if (dis->hwndItem != actions->getHandle())
    return M_UNHANDLED;

  char buf[MAX_PATH];
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.mask = LVIF_TEXT | LVIF_PARAM;
  lvi.iItem = dis->itemID;
  lvi.pszText = buf;
  lvi.cchTextMax = sizeof buf;
  ListView_GetItem(dis->hwndItem, &lvi);

  RECT label;
  ListView_GetItemRect(dis->hwndItem, dis->itemID, &label, LVIR_LABEL);

  uint32 btnFace = GetSysColor(COLOR_BTNFACE);
  uint32 clrBgSave = SetBkColor(dis->hDC, btnFace);
  uint32 clrFgSave = SetTextColor(dis->hDC, 0x000000);

  RECT item = label;
  item.left += 2;
  item.right -= 2;

  DrawText(dis->hDC, lvi.pszText, -1, &item, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_WIDTH;
  int count = Header_GetItemCount(ListView_GetHeader(dis->hwndItem));
  for (int i = 1; i < count; i++)
  {
    ListView_GetColumn(dis->hwndItem, i, &lvc);
    label.left = label.right;
    label.right += lvc.cx;

    if (i == count - 1)
    {
      SetBkColor(dis->hDC, 0x000000);
      item = label;
      item.top = (label.top + label.bottom - BAR_HEIGHT) / 2;
      item.bottom = item.top + BAR_HEIGHT;
      item.right = item.left + lvi.lParam;
      ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &item, NULL, 0, NULL);
      SetBkColor(dis->hDC, btnFace);
    }
    else
    {
      lvi.iSubItem = i;
      ListView_GetItem(dis->hwndItem, &lvi);
      if (lvi.pszText[0] == 0)
        continue;

      item = label;
      item.left += 6;
      item.right -= 6;
      DrawText(dis->hDC, lvi.pszText, -1, &item, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
    }
  }
  SetTextColor(dis->hDC, clrFgSave);
  SetBkColor(dis->hDC, clrBgSave);

  return TRUE;
}
