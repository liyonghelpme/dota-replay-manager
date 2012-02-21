#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"
#include "frameui/controlframes.h"

#include "settingswnd.h"

#define ITEM_BASE           157

SettingsWindow::SettingsWindow(Window* parent)
{
  subclass(WC_TABCONTROL, 0, 0, 10, 10, "", WS_CHILD | WS_VISIBLE, 0, parent->getHandle());
  setFont(FontSys::getSysFont());

  addAllItems();
}

int SettingsWindow::addTab(String name)
{
  int pos = tabs.push(new Frame(this));
  TCITEM item;
  memset(&item, 0, sizeof item);
  item.mask = TCIF_TEXT;
  item.pszText = const_cast<char*>(name.c_str());
  TabCtrl_InsertItem(hWnd, pos, &item);

  if (pos != 0)
    tabs[pos]->hide();

  SendMessage(hWnd, WM_SIZE, 0, 0);

  return pos;
}
WindowFrame* SettingsWindow::addStringItem(int tab, String key, char const* def)
{
  int pos = items.length();
  SettingsItem& item = items.push();
  item.tab = tab;
  item.type = ITEM_STRING;
  item.key = key;
  item.ctrl = NULL;
  EditFrame* frame = new EditFrame(tabs[tab], ITEM_BASE + pos);
  frame->setText(getApp()->getRegistry()->readString(key, def));
  item.ctrl = frame;
  return item.ctrl;
}
WindowFrame* SettingsWindow::addIntItem(int tab, String key, int def)
{
  int pos = items.length();
  SettingsItem& item = items.push();
  item.tab = tab;
  item.type = ITEM_INT;
  item.key = key;
  item.ctrl = NULL;
  EditFrame* frame = new EditFrame(tabs[tab], ITEM_BASE + pos, ES_AUTOHSCROLL | ES_NUMBER);
  frame->setText(String(getApp()->getRegistry()->readInt(key, def)));
  item.ctrl = frame;
  return item.ctrl;
}
WindowFrame* SettingsWindow::addBoolItem(int tab, String key, int mask, int def)
{
  int pos = items.length();
  SettingsItem& item = items.push();
  item.tab = tab;
  item.type = ITEM_BOOL;
  item.key = key;
  item.mask = mask;
  item.ctrl = NULL;
  ButtonFrame* frame = new ButtonFrame("", tabs[tab], ITEM_BASE + pos, BS_AUTOCHECKBOX);
  frame->setCheck((getApp()->getRegistry()->readInt(key, def) & mask) != 0);
  item.ctrl = frame;
  return item.ctrl;
}

uint32 SettingsWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  uint32 result = handleExtra(message, wParam, lParam);
  if (result)
    return result;
  switch (message)
  {
  case WM_NOTIFYREFLECT:
    {
      NMHDR* hdr = (NMHDR*) lParam;
      if (hdr->code == TCN_SELCHANGE)
      {
        int sel = TabCtrl_GetCurSel(hWnd);
        for (int i = 0; i < tabs.length(); i++)
          tabs[i]->show(i == sel);
        return TRUE;
      }
    }
    break;
  case WM_SIZE:
    {
      RECT rc;
      GetClientRect(hWnd, &rc);
      TabCtrl_AdjustRect(hWnd, FALSE, &rc);
      for (int i = 0; i < tabs.length(); i++)
      {
        tabs[i]->setPoint(PT_TOPLEFT, rc.left, rc.top);
        tabs[i]->setPoint(PT_BOTTOMRIGHT, NULL, PT_TOPLEFT, rc.right, rc.bottom);
      }
    }
    break;

  case WM_COMMAND:
    if (lParam)
    {
      int id = LOWORD(wParam);
      int code = HIWORD(wParam);
      if (id >= ITEM_BASE && id < ITEM_BASE + items.length() && items[id - ITEM_BASE].ctrl)
      {
        SettingsItem& item = items[id - ITEM_BASE];
        Registry* reg = getApp()->getRegistry();
        switch (item.type)
        {
        case ITEM_STRING:
          if (code == EN_CHANGE)
            reg->writeString(item.key, item.ctrl->getText());
          break;
        case ITEM_INT:
          if (code == EN_CHANGE)
            reg->writeInt(item.key, item.ctrl->getText().toInt());
          break;
        case ITEM_BOOL:
          if (code == BN_CLICKED)
            reg->writeInt(item.key, (reg->readInt(item.key) & (~item.mask)) |
              (((ButtonFrame*) item.ctrl)->checked() ? item.mask : 0));
          break;
        }
      }
    }
    break;
  }
  return FrameWindow::onMessage(message, wParam, lParam);
}
void SettingsWindow::updateKey(String key)
{
  Registry* reg = getApp()->getRegistry();
  for (int i = 0; i < items.length(); i++)
  {
    if (items[i].key == key)
    {
      switch (items[i].type)
      {
      case ITEM_STRING:
        items[i].ctrl->setText(reg->readString(key));
        break;
      case ITEM_INT:
        items[i].ctrl->setText(String(reg->readInt(key)));
        break;
      case ITEM_BOOL:
        ((ButtonFrame*) items[i].ctrl)->setCheck((reg->readInt(key) & items[i].mask) != 0);
        break;
      }
    }
  }
}
