#include "core/app.h"
#include "graphics/imagelib.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "dota/consts.h"

#include "playerinfo.h"

#define ID_SKILLLIST        109
#define ID_ITEMLIST         110

ReplayPlayerInfoTab::ReplayPlayerInfoTab(Frame* parent)
  : ReplayTab(parent)
{
  players = new ComboFrameEx(this, ID_PLAYERBOX);
  players->setPoint(PT_TOP, 0, 10);
  players->setWidth(250);

  skills = new ListFrame(this, ID_SKILLLIST);
  skills->setPoint(PT_TOPRIGHT, players, PT_BOTTOM, -3, 22);
  skills->setPoint(PT_BOTTOMLEFT, 10, -50);
  items = new ListFrame(this, ID_ITEMLIST);
  items->setPoint(PT_TOPLEFT, players, PT_BOTTOM, 3, 22);
  items->setPoint(PT_BOTTOMRIGHT, -10, -50);

  skills->insertColumn(0, "Skill");
  skills->insertColumn(1, "");
  skills->insertColumn(2, "Time");
  items->insertColumn(0, "Item");
  items->insertColumn(1, "Time");
  items->insertColumn(2, "Cost");

  StaticFrame* tip = new StaticFrame("Skill build:", this);
  tip->setPoint(PT_BOTTOM, skills, PT_TOP, 0, -4);
  tip = new StaticFrame("Item build:", this);
  tip->setPoint(PT_BOTTOM, items, PT_TOP, 0, -4);

  tip = new StaticFrame("Kill/death info:", this);
  tip->setPoint(PT_TOPLEFT, skills, PT_BOTTOMLEFT, 0, 4);
  for (int i = 0; i < 5; i++)
  {
    kdIcons[i] = new ImageFrame(this);
    if (i == 0)
      kdIcons[i]->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 4);
    else
      kdIcons[i]->setPoint(PT_BOTTOMLEFT, kdText[i - 1], PT_BOTTOMRIGHT, 4, 0);
    kdText[i] = new StaticFrame("", this);
    kdText[i]->setPoint(PT_BOTTOMLEFT, kdIcons[i], PT_BOTTOMRIGHT, 2, 0);
  }

  wardInfo = new StaticFrame("Wards placed:", this);
  wardInfo->setPoint(PT_TOPLEFT, items, PT_BOTTOMLEFT, 0, 4);

  buildInfo = new StaticFrame("Final build:", this);
  buildInfo->setPoint(PT_TOPLEFT, wardInfo, PT_BOTTOMLEFT, 0, 4);
  buildImage = new ImageFrame(this);
  buildImage->setPoint(PT_BOTTOMLEFT, buildInfo, PT_BOTTOMRIGHT, 4, 0);
}

uint32 ReplayPlayerInfoTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  return 0;
}

void ReplayPlayerInfoTab::onSetReplay()
{
  players->reset();
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

    players->setCurSel(1);
  }
  else
  {
    players->disable();
  }
}
void ReplayPlayerInfoTab::setPlayer(W3GPlayer* player)
{
  for (int i = 0; i < players->getCount(); i++)
  {
    if (players->getItemData(i) == (uint32) player)
    {
      players->setCurSel(i);
      break;
    }
  }
}
