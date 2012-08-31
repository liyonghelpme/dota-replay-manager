#ifndef __FRAMEUI_CONTROLFRAMES_H__
#define __FRAMEUI_CONTROLFRAMES_H__

#include "frameui/frame.h"
#include "frameui/window.h"
#include "frameui/framewnd.h"

class ButtonFrame : public WindowFrame
{
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ButtonFrame(String text, Frame* parent, int id = 0, int style = BS_PUSHBUTTON);
  void setCheck(bool check)
  {
    SendMessage(hWnd, BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
  }
  bool checked() const
  {
    return SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
  }
};
class LinkFrame : public WindowFrame
{
  HFONT hFont;
  HFONT uFont;
  uint32 _color;
  uint32 _flags;
  bool pressed;
  bool hover;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  LinkFrame(String text, Frame* parent, int id = 0);

  void setColor(uint32 color);
  void setFlags(int flags);
  uint32 color() const
  {
    return _color;
  }
  int flags() const
  {
    return _flags;
  }

  void resetSize();
};

class EditFrame : public WindowFrame
{
  HBRUSH background;
  uint32 bgcolor;
  uint32 fgcolor;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  EditFrame(Frame* parent, int id = 0, int style = ES_AUTOHSCROLL);
  ~EditFrame();
  void setFgColor(uint32 color);
  void setBgColor(uint32 color);
};

class ComboFrame : public WindowFrame
{
  void onMove(uint32 data);
  int boxHeight;
public:
  ComboFrame(Frame* parent, int id = 0, int style = CBS_DROPDOWNLIST);
  int addString(String text, int data = 0);
  void delString(int pos);
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

class StaticFrame : public WindowFrame
{
public:
  StaticFrame(Frame* parent, int id = 0, int style = 0, int exStyle = 0);
  StaticFrame(String text, Frame* parent, int id = 0, int style = 0, int exStyle = 0);
  void setImage(HANDLE image, int type = IMAGE_BITMAP);
  void resetSize();
};

class RichEditFrame : public WindowFrame
{
  static DWORD CALLBACK StreamCallback(DWORD_PTR cookie, LPBYTE buff, LONG cb, LONG* pcb);
public:
  RichEditFrame(Frame* parent, int id = 0, int style = WS_VSCROLL | WS_HSCROLL |
    ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY);
  void setBackgroundColor(uint32 color);
  void setRichText(String text);
};

class SliderFrame : public WindowFrame
{
public:
  SliderFrame(Frame* parent, int id = 0, int style = TBS_AUTOTICKS | TBS_BOTH);

  void setPos(int pos);
  void setRange(int minValue, int maxValue);
  void setLineSize(int size);
  void setPageSize(int size);
  void setTicFreq(int freq);
  int getPos();
};

class UpDownFrame : public WindowFrame
{
public:
  UpDownFrame(Frame* parent, int id = 0, int style = 0);
};

class TabFrame : public WindowFrame
{
protected:
  Array<Frame*> tabs;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  TabFrame(Frame* parent, int id = 0, int style = 0);

  int numTabs() const
  {
    return tabs.length();
  }
  Frame* addTab(String text, Frame* frame = NULL);
  Frame* getTab(int pos) const
  {
    return (pos < 0 || pos >= tabs.length() ? NULL : tabs[pos]);
  }

  void clear();

  int getCurSel() const
  {
    return TabCtrl_GetCurSel(hWnd);
  }
  void setCurSel(int sel);
};

#include "graphics/image.h"

class ImageFrame : public WindowFrame
{
  HDC hDC;
  HBITMAP hBitmap;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ImageFrame(Frame* parent, Image* img = NULL);
  ~ImageFrame();

  void setImage(Image* img);
};
class ColorFrame : public WindowFrame
{
  uint32 color;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ColorFrame(Frame* parent, uint32 clr);
  ~ColorFrame();

  void setColor(uint32 clr)
  {
    color = clr;
    invalidate();
  }
};

class TreeViewFrame : public WindowFrame
{
public:
  TreeViewFrame(Frame* parent, int id = 0, int style = 0);

  void setImageList(HIMAGELIST list, int type)
  {
    TreeView_SetImageList(hWnd, list, type);
  }
  void setItemHeight(int height)
  {
    TreeView_SetItemHeight(hWnd, height);
  }

  HTREEITEM insertItem(TVINSERTSTRUCT* tvis)
  {
    return TreeView_InsertItem(hWnd, tvis);
  }
  void deleteItem(HTREEITEM item)
  {
    TreeView_DeleteItem(hWnd, item);
  }

  void setItemText(HTREEITEM item, String text);
};

#endif // __FRAMEUI_CONTROLFRAMES_H__
