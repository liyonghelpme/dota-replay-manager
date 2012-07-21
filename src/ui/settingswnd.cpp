#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"
#include "frameui/controlframes.h"

#include "settingswnd.h"

#define ITEM_BASE           157

SettingsWindow::SettingsWindow(Frame* parent)
  : TabFrame(parent)
{
  addAllItems();
}

WindowFrame* SettingsWindow::addStringItem(int tab, cfg::ConfigItem* item)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_STRING;
  cur.item = item;
  cur.ctrl = NULL;
  EditFrame* frame = new EditFrame(getTab(tab), ITEM_BASE + pos);
  frame->setText(*(cfg::StringItem*) item);
  cur.ctrl = frame;
  return cur.ctrl;
}
WindowFrame* SettingsWindow::addIntItem(int tab, cfg::ConfigItem* item)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_INT;
  cur.item = item;
  cur.ctrl = NULL;
  EditFrame* frame = new EditFrame(getTab(tab), ITEM_BASE + pos, ES_AUTOHSCROLL | ES_NUMBER);
  frame->setText(String(*(cfg::IntItem*) item));
  cur.ctrl = frame;
  return cur.ctrl;
}
WindowFrame* SettingsWindow::addBoolItem(int tab, cfg::ConfigItem* item, int mask)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_BOOL;
  cur.item = item;
  cur.mask = mask;
  cur.ctrl = NULL;
  ButtonFrame* frame = new ButtonFrame("", getTab(tab), ITEM_BASE + pos, BS_AUTOCHECKBOX);
  frame->setCheck(((*(cfg::IntItem*) item) & mask) != 0);
  cur.ctrl = frame;
  return cur.ctrl;
}

uint32 SettingsWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  uint32 result = handleExtra(message, wParam, lParam);
  if (result)
    return result;
  switch (message)
  {
  case WM_COMMAND:
    if (lParam)
    {
      int id = LOWORD(wParam);
      int code = HIWORD(wParam);
      if (id >= ITEM_BASE && id < ITEM_BASE + items.length() && items[id - ITEM_BASE].ctrl)
      {
        SettingsItem& cur = items[id - ITEM_BASE];
        switch (cur.type)
        {
        case ITEM_STRING:
          if (code == EN_CHANGE)
            *(cfg::StringItem*) cur.item = cur.ctrl->getText();
          break;
        case ITEM_INT:
          if (code == EN_CHANGE)
            *(cfg::IntItem*) cur.item = cur.ctrl->getText().toInt();
          break;
        case ITEM_BOOL:
          if (code == BN_CLICKED)
            *(cfg::IntItem*) cur.item = ((*(cfg::IntItem*) cur.item) & (~cur.mask)) |
              (((ButtonFrame*) cur.ctrl)->checked() ? cur.mask : 0);
          break;
        }
        return TRUE;
      }
    }
    break;
  }
  return TabFrame::onMessage(message, wParam, lParam);
}
void SettingsWindow::updateKey(cfg::ConfigItem* item)
{
  Registry* reg = getApp()->getRegistry();
  for (int i = 0; i < items.length(); i++)
  {
    if (items[i].item == item)
    {
      switch (items[i].type)
      {
      case ITEM_STRING:
        items[i].ctrl->setText(*(cfg::StringItem*) item);
        break;
      case ITEM_INT:
        items[i].ctrl->setText(String(*(cfg::IntItem*) item));
        break;
      case ITEM_BOOL:
        ((ButtonFrame*) items[i].ctrl)->setCheck(((*(cfg::IntItem*) item) & items[i].mask) != 0);
        break;
      }
    }
  }
}
