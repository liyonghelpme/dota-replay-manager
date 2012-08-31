#include "core/app.h"
#include "graphics/imagelib.h"
#include "dota/colors.h"
#include "frameui/fontsys.h"
#include "frameui/listctrl.h"

#include "draft.h"

class HeroListFrame : public ListFrame
{
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  HeroListFrame(Frame* parent)
    : ListFrame(parent, 0, LVS_NOCOLUMNHEADER | LVS_SINGLESEL)
  {
    insertColumn(0, "");
  }

  void addHero(Dota::Hero* hero, uint32 color = 0);
};
uint32 HeroListFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_SIZE)
    setColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
  return ListFrame::onMessage(message, wParam, lParam);
}
void HeroListFrame::addHero(Dota::Hero* hero, uint32 color)
{
  addItem(hero->name, getApp()->getImageLibrary()->getListIndex(hero->icon), color);
}

void ReplayDraftTab::onSetReplay()
{
  pool->clear();
  bans->clear();
  picks->clear();

  if (w3g == NULL || w3g->getDotaInfo() == NULL)
  {
    pool->disable();
    bans->disable();
    picks->disable();
    return;
  }

  captains[0]->setText(String("Sentinel captain: ") +
    (w3g->getCaptain(0) ? w3g->getCaptain(0)->name : "(none)"));
  captains[1]->setText(String("Scourge captain: ") +
    (w3g->getCaptain(1) ? w3g->getCaptain(1)->name : "(none)"));
  captains[0]->resetSize();
  captains[1]->resetSize();

  DraftData const& draft = w3g->getDotaInfo()->draft;
  if (draft.numPool)
  {
    pool->enable();
    for (int i = 0; i < draft.numPool; i++)
      pool->addHero(draft.pool[i]);
  }
  else
    pool->disable();
  if (draft.numBans[0] || draft.numBans[1])
  {
    bans->enable();
    int pos[2] = {0, 0};
    while (pos[0] < draft.numBans[0] || pos[1] < draft.numBans[1])
    {
      for (int team = 0; team < 2; team++)
        if (pos[team] < draft.numBans[team])
          bans->addHero(draft.bans[team][pos[team]++], getLightColor(team * 6));
    }
  }
  else
    bans->disable();
  if (draft.numPicks[0] || draft.numPicks[1])
  {
    picks->enable();
    int pos[2] = {0, 0};
    int start = draft.firstPick;
    while (pos[0] < draft.numPicks[0] || pos[1] < draft.numPicks[1])
    {
      for (int t = 0, team = start; t < 2; t++, team = 1 - team)
        if (pos[team] < draft.numPicks[team])
          picks->addHero(draft.picks[team][pos[team]++], getLightColor(team * 6));
      start = 1 - start;
    }
  }
  else
    picks->disable();
}
ReplayDraftTab::ReplayDraftTab(Frame* parent)
  : ReplayTab(parent)
{
  StaticFrame* bansTip = new StaticFrame("Bans:", this);
  StaticFrame* picksTip = new StaticFrame("Picks:", this);
  pool = new HeroListFrame(this);
  bans = new HeroListFrame(this);
  picks = new HeroListFrame(this);
  captains[0] = new StaticFrame("Sentinel captain:", this);
  captains[1] = new StaticFrame("Scourge captain:", this);
  pool->setColorMode(ListFrame::colorStripe);
  bans->setColorMode(ListFrame::colorParam);
  picks->setColorMode(ListFrame::colorParam);

  pool->setPoint(PT_TOPLEFT, 10, 10);
  pool->setPointEx(PT_BOTTOMRIGHT, 0.5, 1, 0, -10);

  captains[0]->setPoint(PT_TOPLEFT, pool, PT_TOPRIGHT, 10, 0);
  captains[1]->setPoint(PT_TOPLEFT, captains[0], PT_BOTTOMLEFT, 0, 5);

  bansTip->setPoint(PT_TOPLEFT, captains[1], PT_BOTTOMLEFT, 0, 10);
  bans->setPoint(PT_TOPLEFT, bansTip, PT_BOTTOMLEFT, 0, 5);
  bans->setPointEx(PT_BOTTOMRIGHT, 1, 0.4, -10, 0);
  picksTip->setPoint(PT_TOPLEFT, bans, PT_BOTTOMLEFT, 0, 10);
  picks->setPoint(PT_TOPLEFT, picksTip, PT_BOTTOMLEFT, 0, 5);
  picks->setPoint(PT_BOTTOMRIGHT, -10, -10);
}
