#include "core/app.h"
#include "graphics/imagelib.h"
#include "graphics/glib.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "dota/consts.h"
#include "frameui/fontsys.h"

#include "playergold.h"

#define IDC_ALL_TEAM1         175
#define IDC_NONE_TEAM1        176
#define IDC_ALL_TEAM2         177
#define IDC_NONE_TEAM2        178
#define IDC_GRAPHBTN          180

class GoldGraphWindow : public GraphWindow
{
public:
  W3GReplay* w3g;
  GoldGraphWindow(Frame* parent)
    : GraphWindow(parent)
  {
    w3g = NULL;
  }
  String formatTip(int x, int y)
  {
    return String::format("%s - %d", w3g ? w3g->formatTime(x) : format_time(x), y);
  }
  void drawVRules(int start, int end)
  {
    EnumStruct es;
    enumTime(es);
    while (maph(es.val) < 50)
      nextTime(es);

    int offset = 0;
    if (cfg.relTime && w3g && w3g->getDotaInfo())
      offset = w3g->getDotaInfo()->start_time;
    for (int x = ((start - offset) / es.val - 1) * es.val + offset; x < end + es.val; x += es.val)
    {
      for (int sx = x - es.val + es.sub; sx < x; sx += es.sub)
        drawVRule(sx, NULL);
      drawVRule(x, w3g ? w3g->formatTime(x) : format_time(x));
    }
  }
};

void ReplayPlayerGoldTab::onSetReplay()
{
  graph->w3g = w3g;
  graph->clear();
  int teamSize[2] = {0, 0};
  for (int i = 0; i < 10; i++)
    buttons[i].frame->hide();
  ImageLibrary* lib = getApp()->getImageLibrary();
  for (int i = 0; i < w3g->getNumPlayers(); i++)
  {
    W3GPlayer* p = w3g->getPlayer(i);
    int id = p->slot.color;
    if (p->slot.slot_status != 0 && p->hero && ((id >= 1 && id <= 5) || (id >= 7 && id <= 11)))
    {
      int team = id / 6;
      int btn = id - team - 1;
      buttons[btn].frame->show();
      buttons[btn].frame->setPointEx(PT_TOPLEFT, 0.2 * teamSize[team], 0, 0, 0);
      buttons[btn].frame->setPointEx(PT_BOTTOMRIGHT, 0.2 * teamSize[team] + 0.2, 1, -5, 0);
      buttons[btn].icon->setImage(lib->getImage(p->hero->hero->icon));
      buttons[btn].check->setText(p->name);
      buttons[btn].check->setCheck(true);
      teamSize[team]++;

      graph->addGraph(id, getSlotColor(id));
      int gold = 0;
      graph->addGraphPoint(id, 0, 0);
      for (int j = 0; j < p->inv.items.length(); j++)
        graph->addGraphPoint(id, p->inv.items[j].time,
          gold += p->inv.items[j].item->cost);
      graph->addGraphPoint(id, p->time, gold += p->stats[STAT_GOLD]);
      if (cfg.smoothGold)
        graph->smoothGraph(id, 60);
    }
  }
  for (int t = 0; t < 12; t += 6)
  {
    totals[t / 6]->setCheck(false);
    graph->addGraph(t, getSlotColor(t));
    graph->enableGraph(t, false);
    for (int i = t + 1; i < t + 6; i++)
      graph->combineGraphs(t, i);
  }
}
ReplayPlayerGoldTab::ReplayPlayerGoldTab(Frame* parent)
  : ReplayTab(parent)
{
  graph = new GoldGraphWindow(this);

  ColorFrame* teams[2] = {
    new ColorFrame(this, getSlotColor(0)),
    new ColorFrame(this, getSlotColor(6))
  };
  totals[0] = new ButtonFrame("Sentinel total", this, IDC_GRAPHBTN + 0, BS_AUTOCHECKBOX);
  totals[1] = new ButtonFrame("Scourge total", this, IDC_GRAPHBTN + 6, BS_AUTOCHECKBOX);
  Frame* teamFrame[2] = {
    new Frame(this),
    new Frame(this)
  };
  ButtonFrame* allBtn[2] = {
    new ButtonFrame("All", this, IDC_ALL_TEAM1),
    new ButtonFrame("All", this, IDC_ALL_TEAM2)
  };
  ButtonFrame* noneBtn[2] = {
    new ButtonFrame("None", this, IDC_NONE_TEAM1),
    new ButtonFrame("None", this, IDC_NONE_TEAM2)
  };

  teams[0]->setPoint(PT_TOPLEFT, 10, 13);
  teams[0]->setSize(50, 10);
  totals[0]->setPoint(PT_BOTTOMLEFT, teams[0], PT_BOTTOMRIGHT, 5, 2);
  totals[0]->setSize(150, 15);
  allBtn[0]->setPoint(PT_TOPLEFT, teams[0], PT_BOTTOMLEFT, 0, 6);
  allBtn[0]->setSize(50, 15);
  noneBtn[0]->setPoint(PT_TOPLEFT, allBtn[0], PT_BOTTOMLEFT, 0, 4);
  noneBtn[0]->setSize(50, 15);
  teamFrame[0]->setPoint(PT_TOPLEFT, allBtn[0], PT_TOPRIGHT, 6, 0);
  teamFrame[0]->setPoint(PT_BOTTOMLEFT, noneBtn[0], PT_BOTTOMRIGHT, 6, 0);
  teamFrame[0]->setPoint(PT_RIGHT, -10, 0);

  noneBtn[1]->setPoint(PT_BOTTOMLEFT, 10, -10);
  noneBtn[1]->setSize(50, 15);
  allBtn[1]->setPoint(PT_BOTTOMLEFT, noneBtn[1], PT_TOPLEFT, 0, -4);
  allBtn[1]->setSize(50, 15);
  teams[1]->setPoint(PT_BOTTOMLEFT, allBtn[1], PT_TOPLEFT, 0, -6);
  teams[1]->setSize(50, 10);
  totals[1]->setPoint(PT_BOTTOMLEFT, teams[1], PT_BOTTOMRIGHT, 5, 2);
  totals[1]->setSize(150, 15);
  teamFrame[1]->setPoint(PT_TOPLEFT, allBtn[1], PT_TOPRIGHT, 6, 0);
  teamFrame[1]->setPoint(PT_BOTTOMLEFT, noneBtn[1], PT_BOTTOMRIGHT, 6, 0);
  teamFrame[1]->setPoint(PT_RIGHT, -10, 0);

  for (int i = 0; i < 10; i++)
  {
    int t = (i / 5);
    int x = (i % 5);
    buttons[i].frame = new Frame(teamFrame[t]);
    buttons[i].icon = new ImageFrame(buttons[i].frame);
    buttons[i].check = new ButtonFrame("", buttons[i].frame, IDC_GRAPHBTN + 1 + i + t, BS_AUTOCHECKBOX);
    buttons[i].bar = new ColorFrame(buttons[i].frame, getSlotColor(1 + i + t));

    buttons[i].icon->setPoint(PT_TOPLEFT, 0, 0);
    buttons[i].icon->setSize(16, 16);
    buttons[i].check->setPoint(PT_BOTTOMLEFT, buttons[i].icon, PT_BOTTOMRIGHT, 3, 0);
    buttons[i].check->setPoint(PT_TOPRIGHT, 0, 0);
    buttons[i].bar->setPoint(PT_TOPLEFT, buttons[i].icon, PT_BOTTOMLEFT, 0, 6);
    buttons[i].bar->setPoint(PT_BOTTOMRIGHT, 0, -3);
  }

  graph->setPoint(PT_TOPLEFT, noneBtn[0], PT_BOTTOMLEFT, 0, 5);
  graph->setPoint(PT_BOTTOMLEFT, teams[1], PT_TOPLEFT, 0, -8);
  graph->setPoint(PT_RIGHT, -10, 0);
}

uint32 ReplayPlayerGoldTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
  {
    int id = LOWORD(wParam);
    if (id == IDC_GRAPHBTN)
      graph->enableGraph(0, totals[0]->checked());
    else if (id >= IDC_GRAPHBTN + 1 && id <= IDC_GRAPHBTN + 5)
      graph->enableGraph(id - IDC_GRAPHBTN, buttons[id - IDC_GRAPHBTN - 1].check->checked());
    else if (id == IDC_GRAPHBTN + 6)
      graph->enableGraph(6, totals[1]->checked());
    else if (id >= IDC_GRAPHBTN + 7 && id <= IDC_GRAPHBTN + 11)
      graph->enableGraph(id - IDC_GRAPHBTN, buttons[id - IDC_GRAPHBTN - 2].check->checked());
    else if (id == IDC_ALL_TEAM1 || id == IDC_NONE_TEAM1)
    {
      for (int i = 0; i < 5; i++)
      {
        graph->enableGraph(i + 1, id == IDC_ALL_TEAM1);
        buttons[i].check->setCheck(id == IDC_ALL_TEAM1);
      }
    }
    else if (id == IDC_ALL_TEAM2 || id == IDC_NONE_TEAM2)
    {
      for (int i = 0; i < 5; i++)
      {
        graph->enableGraph(i + 7, id == IDC_ALL_TEAM2);
        buttons[i + 5].check->setCheck(id == IDC_ALL_TEAM2);
      }
    }
    else
      return M_UNHANDLED;
    return 0;
  }
  return M_UNHANDLED;
}
