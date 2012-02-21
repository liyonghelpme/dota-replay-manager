#ifndef __FRAMEUI_CONTROLFRAMES_H__
#define __FRAMEUI_CONTROLFRAMES_H__

#include "frameui/frame.h"
#include "frameui/window.h"
#include "frameui/framewnd.h"

class ButtonFrame : public WindowFrame
{
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
public:
  EditFrame(Frame* parent, int id = 0, int style = ES_AUTOHSCROLL);
};

class ComboFrame : public WindowFrame
{
  void onMove();
  int boxHeight;
public:
  ComboFrame(Frame* parent, int id = 0, int style = CBS_DROPDOWNLIST);
  int addString(String text, int data = 0);
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

#endif // __FRAMEUI_CONTROLFRAMES_H__
