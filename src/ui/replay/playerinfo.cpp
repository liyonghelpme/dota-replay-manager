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
  skills->setColorMode(ListFrame::colorParam);
  skills->setPoint(PT_TOPRIGHT, players, PT_BOTTOM, -3, 22);
  skills->setPoint(PT_BOTTOMLEFT, 10, -55);
  items = new ListFrame(this, ID_ITEMLIST);
  items->setColorMode(ListFrame::colorParam);
  items->setPoint(PT_TOPLEFT, players, PT_BOTTOM, 3, 22);
  items->setPoint(PT_BOTTOMRIGHT, -10, -55);

  skills->insertColumn(0, "Skill");
  skills->insertColumn(1, "");
  skills->insertColumn(2, "Time");
  int colOrder[3] = {1, 0, 2};
  ListView_SetColumnOrderArray(skills->getHandle(), 3, colOrder);
  items->insertColumn(0, "Item");
  items->insertColumn(1, "Time");
  items->insertColumn(2, "Cost");

  ImageLibrary* lib = getApp()->getImageLibrary();

  StaticFrame* tip = new StaticFrame("Skill build:", this);
  tip->setPoint(PT_BOTTOM, skills, PT_TOP, 0, -4);
  tip = new StaticFrame("Item build:", this);
  tip->setPoint(PT_BOTTOM, items, PT_TOP, 0, -4);

  tip = new StaticFrame("Killed", this);
  tip->setPoint(PT_TOPLEFT, skills, PT_BOTTOMLEFT, 0, 22);
  StaticFrame* tip2 = new StaticFrame("Killed by", this);
  tip2->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 0);
  for (int i = 0; i < 5; i++)
  {
    kdIcons[i] = new ImageFrame(this);
    if (i == 0)
    {
      kdIcons[i]->setPoint(PT_TOP, skills, PT_BOTTOM, 0, 4);
      kdIcons[i]->setPoint(PT_LEFT, tip2, PT_RIGHT, 10, 0);
    }
    else
      kdIcons[i]->setPoint(PT_BOTTOMLEFT, kdIcons[i - 1], PT_BOTTOMRIGHT, 14, 0);
    kdText[0][i] = new StaticFrame("", this);
    kdText[0][i]->setPoint(PT_BOTTOM, tip, PT_BOTTOM, 0, 0);
    kdText[0][i]->setPoint(PT_CENTER, kdIcons[i], PT_CENTER, 0, 0);
    kdText[1][i] = new StaticFrame("", this);
    kdText[1][i]->setPoint(PT_BOTTOM, tip2, PT_BOTTOM, 0, 0);
    kdText[1][i]->setPoint(PT_CENTER, kdIcons[i], PT_CENTER, 0, 0);
  }

  tip = new StaticFrame("Ward sets:", this);
  tip->setPoint(PT_TOPLEFT, items, PT_BOTTOMLEFT, 0, 10);
  wardIcons[0] = new ImageFrame(this);
  wardIcons[0]->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 4, 1);
  wardInfo[0] = new StaticFrame("0", this);
  wardInfo[0]->setPoint(PT_BOTTOMLEFT, wardIcons[0], PT_BOTTOMRIGHT, 2, -1);
  wardIcons[1] = new ImageFrame(this);
  wardIcons[1]->setPoint(PT_BOTTOMLEFT, wardInfo[0], PT_BOTTOMRIGHT, 4, 1);
  wardInfo[1] = new StaticFrame("0", this);
  wardInfo[1]->setPoint(PT_BOTTOMLEFT, wardIcons[1], PT_BOTTOMRIGHT, 2, -1);

  buildInfo = new StaticFrame("Final build:", this);
  buildInfo->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 10);
  buildImage = new ImageFrame(this);
  buildImage->setPoint(PT_BOTTOMLEFT, buildInfo, PT_BOTTOMRIGHT, 4, 0);
}

uint32 ReplayPlayerInfoTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  return M_UNHANDLED;
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

    Dota* dota = w3g->getDotaData();
    if (dota)
    {
      Dota::Item* ward = dota->getItemByName("Observer Wards");
      if (ward)
        wardIcons[0]->setImage(getApp()->getImageLibrary()->getImage(ward->icon));
      ward = dota->getItemByName("Sentry Wards");
      if (ward)
        wardIcons[1]->setImage(getApp()->getImageLibrary()->getImage(ward->icon));
    }

    setPlayer((W3GPlayer*) players->getItemData(1));
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

  skills->setRedraw(false);
  items->setRedraw(false);
  skills->clear();
  items->clear();
  if (player && player->hero)
  {
    int levels[5] = {0, 0, 0, 0, 0};
    ImageLibrary* lib = getApp()->getImageLibrary();
    for (int i = 1; i <= player->hero->level; i++)
    {
      uint32 color = (cfg.skillColors && (i & 1)) ? 0xFFFFFF : 0xFFEEEE;
      int pos;
      if (player->hero->skills[i].skill)
      {
        Dota::Ability* skill = player->hero->skills[i].skill;
        if (cfg.skillColors)
        {
          if (skill->slot == 0)
            color = 0xCCFFCC;
          else if (skill->slot == 1)
            color = 0xFFCCCC;
          else if (skill->slot == 2)
            color = 0xAAFFFF;
          else if (skill->slot == 3)
            color = 0xCCCCFF;
        }
        int level = 0;
        if (skill->slot >= 0 && skill->slot < 5)
          level = ++levels[skill->slot];
        pos = skills->addItem((cfg.showLevels && level) ?
          String::format("%s (level %d)", skill->name, level) : skill->name,
          lib->getListIndex(skill->icon), color);
      }
      else
        pos = skills->addItem("None", lib->getListIndex("Empty"), color);
      skills->setItemText(pos, 1, String(i));
      skills->setItemText(pos, 2, w3g->formatTime(player->hero->levelTime[i]));
    }

    DotaInfo const* info = w3g->getDotaInfo();
    for (int i = 0; i < info->team_size[1 - player->slot.team]; i++)
    {
      W3GPlayer* other = info->teams[1 - player->slot.team][i];
      if (other->hero)
      {
        Image img(16, 16);
        img.fill(getFlipColor(other->slot.color) | 0xFF000000);
        img.blt(1, 1, lib->getImage(other->hero->hero->icon), 1, 1, 14, 14);
        kdIcons[i]->setImage(&img);
        kdIcons[i]->show();
        kdText[0][i]->setText(String(player->pkilled[other->index]));
        kdText[0][i]->resetSize();
        kdText[0][i]->show();
        kdText[1][i]->setText(String(player->pdied[other->index]));
        kdText[1][i]->resetSize();
        kdText[1][i]->show();
      }
      else
      {
        kdIcons[i]->setSize(16, 16);
        kdIcons[i]->hide();
        kdText[0][i]->hide();
        kdText[1][i]->hide();
      }
    }
    for (int i = info->team_size[1 - player->slot.team]; i < 5; i++)
    {
      kdIcons[i]->hide();
      kdText[0][i]->hide();
      kdText[1][i]->hide();
    }

    player->inv.compute(0x7FFFFFFF, w3g->getDotaData(), cfg.showAssemble);
    int wards[2] = {0, 0};
    int wardGold = 0;
    for (int i = 0; i < player->inv.comb.length(); i++)
    {
      W3GItem& item = player->inv.comb[i];
      if (item.item)
      {
        if (item.item->name == "Observer Wards")
        {
          wards[0]++;
          wardGold += item.item->cost;
        }
        else if (item.item->name == "Sentry Wards")
        {
          wards[1]++;
          wardGold += item.item->cost;
        }
        uint32 color;
        if (item.item->recipe)
          color = 0x80FF80;
        else if (item.flags & ITEM_FLAG_USED)
          color = 0xDDDDDD;
        else if (cfg.showAssemble)
          color = 0xFFFFFF;
        else
          color = (items->getCount() & 1 ? 0xFFEEEE : 0xFFFFFF);
        int pos = items->addItem(item.item->name, lib->getListIndex(item.item->icon), color);
        items->setItemText(pos, 1, w3g->formatTime(item.time));
        items->setItemText(pos, 2, String(item.item->cost));
      }
    }
    wardInfo[0]->setText(String(wards[0]));
    wardInfo[0]->resetSize();
    wardInfo[1]->setText(wardGold ? String::format("%d (%d gold)", wards[1], wardGold)
      : String(wards[1]));
    wardInfo[1]->resetSize();

    int numItems = 0;
    for (int i = 0; i < 6; i++)
      if (player->inv.final[i])
        numItems++;
    if (numItems)
    {
      buildInfo->setText("Final build:");
      buildInfo->resetSize();
      Image image(numItems * 17 - 1, 16);
      uint32 color = GetSysColor(COLOR_BTNFACE);
      image.fill(((color & 0x0000FF) << 16) | (color & 0x00FF00) |
        ((color & 0xFF0000) >> 16) | 0xFF000000);
      numItems = 0;
      for (int i = 0; i < 6; i++)
        if (player->inv.final[i])
          image.blt((numItems++) * 17, 0, lib->getImage(player->inv.final[i]->icon));
      buildImage->setImage(&image);
      buildImage->show();
    }
    else
    {
      buildInfo->setText("Final build: (none)");
      buildInfo->resetSize();
      buildImage->hide();
    }
  }
  for (int i = 0; i < 3; i++)
  {
    skills->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    items->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
  }
  skills->setRedraw(true);
  items->setRedraw(true);
}
