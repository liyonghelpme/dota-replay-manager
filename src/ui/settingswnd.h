#ifndef __UI_SETTINGSWND_H__
#define __UI_SETTINGSWND_H__

#include "frameui/controlframes.h"

#define WM_UPDATEPATH       (WM_USER+257)

#define ITEM_STRING         0x0001
#define ITEM_INT            0x0002
#define ITEM_BOOL           0x0003

class ClickColor;
class ComboFrame;
class SettingsWindow : public TabFrame
{
  struct SettingsItem
  {
    int tab;
    int type;
    cfg::ConfigItem* item;
    int mask;
    WindowFrame* ctrl;
  };
  Array<SettingsItem> items;

  ClickColor* chatColors;
  ComboFrame* chatColorMode;

  WindowFrame* addStringItem(int tab, cfg::ConfigItem* item);
  WindowFrame* addIntItem(int tab, cfg::ConfigItem* item);
  WindowFrame* addBoolItem(int tab, cfg::ConfigItem* item, int mask = 1);
  void updateKey(cfg::ConfigItem* item);

  void addAllItems();

  uint32 handleExtra(uint32 message, uint32 wParam, uint32 lParam);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  SettingsWindow(Frame* parent);
};

#endif // __UI_SETTINGSWND_H__
