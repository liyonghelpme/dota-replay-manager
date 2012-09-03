#include "core/app.h"
#include "graphics/imagelib.h"
#include "graphics/glib.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "dota/consts.h"
#include "frameui/fontsys.h"

#include "playerexp.h"

#define IDC_ALL_TEAM1         175
#define IDC_NONE_TEAM1        176
#define IDC_ALL_TEAM2         177
#define IDC_NONE_TEAM2        178
#define IDC_GRAPHBTN          180

static int ExpTable[] = {0,
  0, 200, 500, 900, 1400,
  2000, 2700, 3500, 4400, 5400,
  6500, 7700, 9000, 10400, 11900,
  13500, 15200, 17000, 18900, 20900,
  23000, 25200, 27500, 29900, 32400
};

class ExpGraphWindow : public GraphWindow
{
public:
  W3GReplay* w3g;
  ExpGraphWindow(Frame* parent)
    : GraphWindow(parent)
  {
    w3g = NULL;
  }
  String formatTip(int x, int y)
  {
    int level = 1;
    while (level < 25 && ExpTable[level + 1] <= y)
      level++;
    return String::format("%s - %d", w3g ? w3g->formatTime(x) : format_time(x), level);
  }
  void drawHRules(int start, int end)
  {
    bool sublines = (mapv(100) >= 10);
    for (int level = 1; level <= 25; level++)
    {
      if (sublines)
      {
        for (int exp = ExpTable[level - 1] + 100; exp < ExpTable[level]; exp++)
          drawHRule(exp, NULL);
      }
      drawHRule(ExpTable[level], String(level));
    }
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

void ReplayPlayerExpTab::onSetReplay()
{
  graph->w3g = w3g;
  graph->clear();
  if (w3g == NULL)
    return;
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
      graph->addGraphPoint(id, 0, 0);
      for (int j = 2; j <= p->hero->level; j++)
        graph->addGraphPoint(id, p->hero->levelTime[j],
          ExpTable[j], p->hero->levelTime[j] > p->time);
      if (p->time > p->hero->levelTime[p->hero->level])
        graph->addGraphPoint(id, p->time, ExpTable[p->hero->level]);
      graph->addGraphPoint(id, w3g->getLength(true), ExpTable[p->hero->level], true);
    }
  }
}
ReplayPlayerExpTab::ReplayPlayerExpTab(Frame* parent)
  : ReplayTab(parent)
{
  graph = new ExpGraphWindow(this);

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

  allBtn[0]->setPoint(PT_TOPLEFT, 10, 10);
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
  graph->setPoint(PT_BOTTOMLEFT, allBtn[1], PT_TOPLEFT, 0, -5);
  graph->setPoint(PT_RIGHT, -10, 0);
}

uint32 ReplayPlayerExpTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
  {
    int id = LOWORD(wParam);
    if (id >= IDC_GRAPHBTN + 1 && id <= IDC_GRAPHBTN + 5)
      graph->enableGraph(id - IDC_GRAPHBTN, buttons[id - IDC_GRAPHBTN - 1].check->checked());
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
