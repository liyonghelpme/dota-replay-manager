#ifndef __FRAMEUI_LISTCTRL__
#define __FRAMEUI_LISTCTRL__

#include "frameui/frame.h"
#include "frameui/window.h"
#include "frameui/framewnd.h"

class SimpleListFrame : public WindowFrame
{
public:
  SimpleListFrame(Frame* parent, int id = 0, int style = LVS_ALIGNLEFT | LVS_REPORT |
    LVS_NOCOLUMNHEADER | LVS_NOSCROLL | LVS_SINGLESEL, int styleEx = WS_EX_CLIENTEDGE);
  void clear();
  void setColumns(int numColumns);
  void setColumn(int column, int width, int fmt);
  void setColumnWidth(int column, int width);
  void setColumn(int column, int width, String text);
  int addItem(String name);
  int addItem(String name, uint32 param);
  void setItemText(int item, int column, String text);
  void setItemTextUtf8(int item, int column, String text);
};

class ListFrame : public WindowFrame
{
  int colorMode;
  Array<bool> colUtf8;

  int wcBufSize;

  int drawItemText(HDC hDC, String text, RECT* rc, uint32 format, bool utf8);
  void drawItem(DRAWITEMSTRUCT* dis);
protected:
  wchar_t* wcBuf;
  int convertUtf8(String text);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  enum {colorNone, colorStripe, colorParam};

  ListFrame(Frame* parent, int id = 0, int style = LVS_NOSORTHEADER | LVS_SINGLESEL,
    int styleEx = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  ~ListFrame();
  void clear();
  void clearColumns();
  void setColorMode(int mode)
  {
    colorMode = mode;
    InvalidateRect(hWnd, NULL, TRUE);
  }

  int getItemParam(int item);

  int getCount() const;
  void insertColumn(int i, String name, int fmt = LVCFMT_LEFT);
  void setColumnUTF8(int i, bool utf8)
  {
    colUtf8[i] = utf8;
  }
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
    wchar_t* wtext;
    BoxItem()
    {
      wtext = NULL;
    }
    ~BoxItem()
    {
      delete[] wtext;
    }
  };
  Array<BoxItem> items;
  int prevSel;

  void onMove(uint32 data);
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
    onMove(0);
  }
};

#endif // __FRAMEUI_LISTCTRL__
