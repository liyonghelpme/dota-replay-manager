#ifndef __UI_SETTINGSWND_H__
#define __UI_SETTINGSWND_H__

#include "frameui/framewnd.h"

#define WM_UPDATEPATH       (WM_USER+257)

#define ITEM_STRING         0x0001
#define ITEM_INT            0x0002
#define ITEM_BOOL           0x0003

class ClickColor;
class ComboFrame;
class SettingsWindow : public FrameWindow
{
  struct SettingsItem
  {
    int tab;
    int type;
    String key;
    int mask;
    WindowFrame* ctrl;
  };
  Array<SettingsItem> items;
  Array<Frame*> tabs;

  ClickColor* chatColors;
  ComboFrame* chatColorMode;

  int addTab(String name);
  WindowFrame* addStringItem(int tab, String key, char const* def = NULL);
  WindowFrame* addIntItem(int tab, String key, int def = 0);
  WindowFrame* addBoolItem(int tab, String key, int mask, int def = 0);
  void updateKey(String key);

  void addAllItems();

  uint32 handleExtra(uint32 message, uint32 wParam, uint32 lParam);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  SettingsWindow(Window* parent);
};

#endif // __UI_SETTINGSWND_H__
