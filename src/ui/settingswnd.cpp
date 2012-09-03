#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"
#include "frameui/controlframes.h"
#include "ui/batchdlg.h"
#include "ui/updatedlg.h"
#include "ui/aboutdlg.h"

#include "settingswnd.h"

#define ITEM_BASE             157

#define IDC_CACHEREPLAYS      130
#define IDC_RESETSETTINGS     131
#define IDC_LOOKFORUPDATES    132
#define IDC_README            133
#define IDC_ABOUT             134

SettingsWindow::SettingsWindow(Frame* parent)
  : Frame(parent)
{
  tabs = new TabFrame(this);

  ButtonFrame* btn = new ButtonFrame("Cache all replays", this, IDC_CACHEREPLAYS);
  btn->setSize(110, 23);
  btn->setPoint(PT_BOTTOMLEFT, 0, 0);

  ButtonFrame* prev = btn;
  btn = new ButtonFrame("Reset to default", this, IDC_RESETSETTINGS);
  btn->setSize(110, 23);
  btn->setPoint(PT_BOTTOMLEFT, prev, PT_BOTTOMRIGHT, 10, 0);

  btn = new ButtonFrame("About...", this, IDC_ABOUT);
  btn->setSize(70, 23);
  btn->setPoint(PT_BOTTOMRIGHT, 0, 0);

  prev = btn;
  btn = new ButtonFrame("Readme...", this, IDC_README);
  btn->setSize(70, 23);
  btn->setPoint(PT_BOTTOMRIGHT, prev, PT_BOTTOMLEFT, -10, 0);

  prev = btn;
  btn = new ButtonFrame("Look for updates", this, IDC_LOOKFORUPDATES);
  btn->setSize(100, 23);
  btn->setPoint(PT_BOTTOMRIGHT, prev, PT_BOTTOMLEFT, -10, 0);

  tabs->setPoint(PT_TOPLEFT, 0, 0);
  tabs->setPoint(PT_TOPRIGHT, 0, 0);
  tabs->setPoint(PT_BOTTOM, btn, PT_TOP, 0, -6);

  addAllItems();
}

WindowFrame* SettingsWindow::addStringItem(int tab, void* item)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_STRING;
  cur.item = item;
  cur.ctrl = NULL;
  EditFrame* frame = new EditFrame(tabs->getTab(tab), ITEM_BASE + pos);
  frame->setText(*(String*) item);
  cur.ctrl = frame;
  return cur.ctrl;
}
WindowFrame* SettingsWindow::addIntItem(int tab, void* item)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_INT;
  cur.item = item;
  cur.ctrl = NULL;
  EditFrame* frame = new EditFrame(tabs->getTab(tab), ITEM_BASE + pos, ES_AUTOHSCROLL | ES_NUMBER);
  frame->setText(String(*(int*) item));
  cur.ctrl = frame;
  return cur.ctrl;
}
WindowFrame* SettingsWindow::addBoolItem(int tab, void* item, int mask)
{
  int pos = items.length();
  SettingsItem& cur = items.push();
  cur.tab = tab;
  cur.type = ITEM_BOOL;
  cur.item = item;
  cur.mask = mask;
  cur.ctrl = NULL;
  ButtonFrame* frame = new ButtonFrame("", tabs->getTab(tab), ITEM_BASE + pos, BS_AUTOCHECKBOX);
  frame->setCheck(((*(int*) item) & mask) != 0);
  cur.ctrl = frame;
  return cur.ctrl;
}

uint32 SettingsWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  uint32 result = handleExtra(message, wParam, lParam);
  if (result != M_UNHANDLED)
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
            *(String*) cur.item = cur.ctrl->getText();
          break;
        case ITEM_INT:
          if (code == EN_CHANGE)
            *(int*) cur.item = cur.ctrl->getText().toInt();
          break;
        case ITEM_BOOL:
          if (code == BN_CLICKED)
            *(int*) cur.item = ((*(int*) cur.item) & (~cur.mask)) |
              (((ButtonFrame*) cur.ctrl)->checked() ? cur.mask : 0);
          break;
        }
        return 0;
      }
      else if (id == IDC_CACHEREPLAYS)
      {
        BatchDialog batch(BatchDialog::mCache);
        batch.addFolder(cfg.replayPath);
        batch.run();
      }
      else if (id == IDC_RESETSETTINGS)
      {
        if (MessageBox(getApp()->getMainWindow(),
          "Are you sure you want to reset all settings to default values?", "Warning",
          MB_ICONWARNING | MB_YESNO) == IDYES)
        {
          cfg.reset();
          updateKey(NULL);
          updateExtra();
        }
      }
      else if (id == IDC_LOOKFORUPDATES)
        UpdateDialog::run();
      else if (id == IDC_README)
      {
        ShellExecute(NULL, "open", String::buildFullName(getApp()->getRootPath(),
          "readme.txt"), NULL, NULL, SW_SHOWNORMAL);
      }
      else if (id == IDC_ABOUT)
        AboutDialog::run();
    }
    return 0;
  }
  return M_UNHANDLED;
}
void SettingsWindow::updateKey(void* item)
{
  for (int i = 0; i < items.length(); i++)
  {
    if (items[i].item == item || item == NULL)
    {
      switch (items[i].type)
      {
      case ITEM_STRING:
        items[i].ctrl->setText(*(String*) items[i].item);
        break;
      case ITEM_INT:
        items[i].ctrl->setText(String(*(int*) items[i].item));
        break;
      case ITEM_BOOL:
        ((ButtonFrame*) items[i].ctrl)->setCheck(((*(int*) items[i].item) & items[i].mask) != 0);
        break;
      }
    }
  }
}
