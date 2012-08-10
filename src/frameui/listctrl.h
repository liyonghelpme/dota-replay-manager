#ifndef __FRAMEUI_LISTCTRL__
#define __FRAMEUI_LISTCTRL__

#include "frameui/frame.h"
#include "frameui/window.h"
#include "frameui/framewnd.h"

class SimpleListFrame : public WindowFrame
{
public:
  SimpleListFrame(Frame* parent, int id = 0, int style = 0, int styleEx = WS_EX_CLIENTEDGE);
  void clear();
  void setColumns(int numColumns);
  void setColumn(int column, int width, int fmt);
  void setColumnWidth(int column, int width);
  int addItem(String name);
  void setItemText(int item, int column, String text);
};

class ListFrame : public WindowFrame
{
  bool simpleColors;
  void drawItemText(HDC hDC, String text, RECT rc, uint32 format);
  int getItemTextWidth(HDC hDC, String text, uint32 format);
  void drawItem(DRAWITEMSTRUCT* dis);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ListFrame(Frame* parent, int id = 0, int style = 0, int styleEx = WS_EX_CLIENTEDGE);
  void clear();
  void clearColumns();
  void setSimpleColors(bool simple)
  {
    simpleColors = simple;
    InvalidateRect(hWnd, NULL, TRUE);
  }

  int getCount() const;
  void insertColumn(int i, String name, int fmt = LVCFMT_LEFT);
  void setColumnWidth(int i, int width);
  int addItem(String name, int image, int data);
  void setItemText(int item, int column, String text);
};

class ComboFrameEx : public WindowFrame
{
  struct BoxItem
  {
    uint32 data;
    uint32 color;
    uint32 icon;
    String text;
  };
  Array<BoxItem> items;
  int prevSel;

  void onMove();
  int boxHeight;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ComboFrameEx(Frame* parent, int id = 0, int style = CBS_DROPDOWNLIST);

  void reset();
  int addString(String text, uint32 data = 0)
  {
    return addString(text, 0xFFFFFF, NULL, data);
  }
  int addString(String text, uint32 color, char const* icon, uint32 data = 0);
  int getCount() const;
  int getItemData(int item) const;
  void setItemData(int item, int data);
  int getCurSel() const;
  void setCurSel(int sel);
  void setBoxHeight(int ht)
  {
    boxHeight = ht;
    onMove();
  }
};

#endif // __FRAMEUI_LISTCTRL__
