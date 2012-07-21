#include "core/app.h"

#include "frameui/framewnd.h"
#include "frameui/fontsys.h"

#include "controlframes.h"

ButtonFrame::ButtonFrame(String text, Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create("Button", text, WS_CHILD | WS_TABSTOP | style, 0);
  setFont(FontSys::getSysFont());
  setId(id);
}

///////////////////////////////////////////////////////

LinkFrame::LinkFrame(String text, Frame* parent, int id)
  : WindowFrame(parent)
{
  hFont = NULL;
  uFont = NULL;
  pressed = false;
  hover = false;
  create(text, WS_CHILD, 0);
  setFont(FontSys::getSysFont());
  resetSize();
  setId(id);
}

void LinkFrame::resetSize()
{
  HDC hDC = GetDC(hWnd);
  SelectObject(hDC, hFont);
  SIZE sz;
  String text = getText();
  GetTextExtentPoint32(hDC, text, text.length(), &sz);
  ReleaseDC(hWnd, hDC);
  setSize(sz.cx, sz.cy);
}
void LinkFrame::setColor(uint32 color)
{
  _color = color;
  InvalidateRect(hWnd, NULL, TRUE);
}
void LinkFrame::setFlags(int flags)
{
  _flags = flags;
  InvalidateRect(hWnd, NULL, TRUE);
}

uint32 LinkFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_SETFONT:
    hFont = (wParam ? (HFONT) wParam : FontSys::getSysFont());
    uFont = FontSys::changeFlags(FontSys::getFlags(hFont) | FONT_UNDERLINE, hFont);
    if (lParam)
      InvalidateRect(hWnd, NULL, TRUE);
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);

      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);

      SetBkMode(hDC, TRANSPARENT);
      SetTextColor(hDC, _color);
      if (pressed || hover)
        SelectObject(hDC, uFont);
      else
        SelectObject(hDC, hFont);
      RECT rc = {0, 0, width(), height()};
      String text = getText();
      DrawText(hDC, text, text.length(), &rc, _flags);

      EndPaint(hWnd, &ps);
    }
    break;
  case WM_MOUSEMOVE:
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (!hover && x > 0 && y > 0 && x < width() && y < height())
      {
        hover = true;
        TRACKMOUSEEVENT tme;
        memset(&tme, 0, sizeof tme);
        tme.cbSize = sizeof tme;
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);
        if (!pressed)
          InvalidateRect(hWnd, NULL, FALSE);
      }
    }
    break;
  case WM_MOUSELEAVE:
    {
      hover = false;
      TRACKMOUSEEVENT tme;
      memset(&tme, 0, sizeof tme);
      tme.cbSize = sizeof tme;
      tme.dwFlags = TME_CANCEL | TME_LEAVE;
      tme.hwndTrack = hWnd;
      TrackMouseEvent(&tme);
      if (!pressed)
        InvalidateRect(hWnd, NULL, TRUE);
    }
    break;
  case WM_LBUTTONDOWN:
    SetCapture(hWnd);
    pressed = true;
    break;
  case WM_LBUTTONUP:
    if (pressed)
    {
      ReleaseCapture();
      pressed = false;
      notify(WM_COMMAND, MAKELONG(id(), BN_CLICKED), (uint32) hWnd);
    }
    break;
  default:
    return 0;
  }
  return TRUE;
}

///////////////////////////////////////////////////////

EditFrame::EditFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create("Edit", "", style | WS_CHILD | WS_TABSTOP, WS_EX_CLIENTEDGE);
  setFont(FontSys::getSysFont());
  setId(id);
}

///////////////////////////////////////////////////////

void ComboFrame::onMove()
{
  if (hWnd)
  {
    if (visible())
    {
      if (IsWindowVisible(hWnd))
        SetWindowPos(hWnd, NULL, left(), top(), width(), boxHeight, SWP_NOZORDER);
      else
      {
        ShowWindow(hWnd, SW_SHOWNA);
        SetWindowPos(hWnd, HWND_TOP, left(), top(), width(), boxHeight, 0);
      }
    }
    else
      ShowWindow(hWnd, SW_HIDE);
  }
}
ComboFrame::ComboFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  boxHeight = 500;
  create("ComboBox", "", style | WS_CHILD | WS_TABSTOP, 0);
  setFont(FontSys::getSysFont());
  setId(id);
  setHeight(21);
}
int ComboFrame::addString(String text, int data)
{
  int id = SendMessage(hWnd, CB_ADDSTRING, 0, (uint32) text.c_str());
  if (id != CB_ERR)
    SendMessage(hWnd, CB_SETITEMDATA, id, data);
  return id;
}
int ComboFrame::getItemData(int item) const
{
  return SendMessage(hWnd, CB_GETITEMDATA, item, 0);
}
void ComboFrame::setItemData(int item, int data)
{
  SendMessage(hWnd, CB_SETITEMDATA, item, data);
}
int ComboFrame::getCurSel() const
{
  return SendMessage(hWnd, CB_GETCURSEL, 0, 0);
}
void ComboFrame::setCurSel(int sel)
{
  SendMessage(hWnd, CB_SETCURSEL, sel, 0);
}

///////////////////////////////////////////////////////

StaticFrame::StaticFrame(Frame* parent, int id, int style, int exStyle)
  : WindowFrame(parent)
{
  create("Static", "", style | SS_NOTIFY | WS_CHILD | WS_TABSTOP, exStyle);
  setFont(FontSys::getSysFont());
  setId(id);
}
StaticFrame::StaticFrame(String text, Frame* parent, int id, int style, int exStyle)
  : WindowFrame(parent)
{
  create("Static", text, style | SS_NOTIFY | WS_CHILD | WS_TABSTOP, exStyle);
  setFont(FontSys::getSysFont());
  resetSize();
  setId(id);
}
void StaticFrame::setImage(HANDLE image, int type)
{
  SendMessage(hWnd, STM_SETIMAGE, (WPARAM) type, (LPARAM) image);
}
void StaticFrame::resetSize()
{
  HDC hDC = GetDC(hWnd);
  SelectObject(hDC, getFont());
  SIZE sz;
  String text = getText();
  GetTextExtentPoint32(hDC, text, text.length(), &sz);
  ReleaseDC(hWnd, hDC);
  setSize(sz.cx, sz.cy);
}

///////////////////////////////////////////////////////

#include "richedit.h"

struct EditStreamCookie
{
  String str;
  int pos;
};
DWORD CALLBACK RichEditFrame::StreamCallback(DWORD_PTR cookie, LPBYTE buff, LONG cb, LONG* pcb)
{
  EditStreamCookie* ck = (EditStreamCookie*) cookie;
  *pcb = ck->str.length() - ck->pos;
  if (*pcb > cb)
    *pcb = cb;
  if (*pcb)
    memcpy(buff, ck->str.c_str() + ck->pos, *pcb);
  ck->pos += *pcb;
  return 0;
}
RichEditFrame::RichEditFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create(RICHEDIT_CLASS, "", style | WS_CHILD | WS_TABSTOP, WS_EX_CLIENTEDGE);
  setFont(FontSys::getSysFont());
  setId(id);
}
void RichEditFrame::setBackgroundColor(uint32 color)
{
  SendMessage(hWnd, EM_SETBKGNDCOLOR, 0, color);
}
void RichEditFrame::setRichText(String text)
{
  EditStreamCookie cookie;
  cookie.str = text;
  cookie.pos = 0;
  EDITSTREAM es;
  es.dwCookie = (DWORD_PTR) &cookie;
  es.dwError = 0;
  es.pfnCallback = StreamCallback;
  SendMessage(hWnd, EM_STREAMIN, SF_RTF, (uint32) &es);
}

/////////////////////////////////

SliderFrame::SliderFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create(TRACKBAR_CLASS, "", style | WS_CHILD | WS_TABSTOP, 0);
  setFont(FontSys::getSysFont());
  setId(id);
}

void SliderFrame::setPos(int pos)
{
  SendMessage(hWnd, TBM_SETPOS, TRUE, pos);
}
void SliderFrame::setRange(int minValue, int maxValue)
{
  SendMessage(hWnd, TBM_SETRANGEMIN, TRUE, minValue);
  SendMessage(hWnd, TBM_SETRANGEMAX, TRUE, maxValue);
}
void SliderFrame::setLineSize(int size)
{
  SendMessage(hWnd, TBM_SETLINESIZE, 0, size);
}
void SliderFrame::setPageSize(int size)
{
  SendMessage(hWnd, TBM_SETPAGESIZE, 0, size);
}
void SliderFrame::setTicFreq(int freq)
{
  SendMessage(hWnd, TBM_SETTICFREQ, freq, 0);
}
int SliderFrame::getPos()
{
  return SendMessage(hWnd, TBM_GETPOS, 0, 0);
}

/////////////////////////////////////////

UpDownFrame::UpDownFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create(UPDOWN_CLASS, "", WS_CHILD | style, 0);
  setId(id);
}

/////////////////////////////////////////

TabFrame::TabFrame(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  create(WC_TABCONTROL, "", WS_CHILD | style, 0);
  setFont(FontSys::getSysFont());
  setId(id);
}
Frame* TabFrame::addTab(int pos, String text, Frame* frame)
{
  if (pos >= tabs.length())
    tabs.resize(pos + 1, 0);
  if (frame == NULL)
    frame = new Frame(this);
  tabs[pos] = frame;

  TCITEM item;
  memset(&item, 0, sizeof item);
  item.mask = TCIF_TEXT;
  item.pszText = text.getBuffer();
  TabCtrl_InsertItem(hWnd, pos, &item);
  frame->show(pos == TabCtrl_GetCurSel(hWnd));

  onMove();

  return frame;
}

void TabFrame::onMove()
{
  WindowFrame::onMove();

  RECT rc;
  GetClientRect(hWnd, &rc);
  TabCtrl_AdjustRect(hWnd, FALSE, &rc);
  for (int i = 0; i < tabs.length(); i++)
  {
    if (tabs[i])
    {
      tabs[i]->setPoint(PT_TOPLEFT, rc.left, rc.top);
      tabs[i]->setPoint(PT_BOTTOMRIGHT, rc.right - width(), rc.bottom - height());
    }
  }
}
uint32 TabFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_NOTIFY)
  {
    NMHDR* hdr = (NMHDR*) lParam;
    if (hdr->hwndFrom == hWnd && hdr->code == TCN_SELCHANGE)
    {
      int sel = TabCtrl_GetCurSel(hWnd);
      for (int i = 0; i < tabs.length(); i++)
        if (i != sel)
          tabs[i]->hide();
      tabs[sel]->show();
      return TRUE;
    }
  }
  return 0;
}

/////////////////////////////////////

uint32 ImageFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_PAINT)
  {
    PAINTSTRUCT ps;
    HDC hPaintDC = BeginPaint(hWnd, &ps);
    if (hBitmap)
      BitBlt(hPaintDC, 0, 0, width(), height(), hDC, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);
    return TRUE;
  }
  return 0;
}
ImageFrame::ImageFrame(Frame* parent, Image* img, bool ownd)
  : WindowFrame(parent)
{
  create("", WS_CHILD, 0);
  image = NULL;

  hDC = CreateCompatibleDC(NULL);
  hBitmap = NULL;
  setImage(img, ownd);
}
ImageFrame::~ImageFrame()
{
  if (hBitmap)
    DeleteObject(hBitmap);
  DeleteDC(hDC);
  if (owned)
    delete image;
}

void ImageFrame::setImage(Image* img, bool ownd)
{
  if (owned)
    delete image;
  if (hBitmap)
    DeleteObject(hBitmap);
  hBitmap = NULL;
  image = img;
  owned = ownd;
  if (image)
  {
    setSize(image->width(), image->height());
    hBitmap = image->createBitmap(hDC);
    SelectObject(hDC, hBitmap);
  }
  invalidate();
}
