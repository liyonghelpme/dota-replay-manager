#include "core/app.h"

#include "frameui/framewnd.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"

#include "listctrl.h"

SimpleListFrame::SimpleListFrame(Frame* parent, int id, int style, int styleEx)
  : WindowFrame(parent)
{
  create(WC_LISTVIEW, "", WS_CHILD | WS_TABSTOP | style, styleEx);
  setFont(FontSys::getSysFont());
  setId(id);
}
void SimpleListFrame::clear()
{
  ListView_DeleteAllItems(hWnd);
}
void SimpleListFrame::setColumns(int numColumns)
{
  for (int i = Header_GetItemCount(ListView_GetHeader(hWnd)) - 1; i >= numColumns; i--)
    ListView_DeleteColumn(hWnd, i);
  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_FMT | LVCF_TEXT;
  lvc.fmt = LVCFMT_LEFT;
  lvc.pszText = "";
  for (int i = Header_GetItemCount(ListView_GetHeader(hWnd)); i < numColumns; i++)
    ListView_InsertColumn(hWnd, i, &lvc);
}
void SimpleListFrame::setColumn(int column, int width, int fmt)
{
  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
  lvc.fmt = fmt;
  lvc.cx = width;
  lvc.pszText = "";
  ListView_SetColumn(hWnd, column, &lvc);
}
void SimpleListFrame::setColumnWidth(int column, int width)
{
  ListView_SetColumnWidth(hWnd, column, width);
}
void SimpleListFrame::setColumn(int column, int width, String text)
{
  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_TEXT | LVCF_WIDTH;
  lvc.cx = width;
  lvc.pszText = text.getBuffer();
  ListView_SetColumn(hWnd, column, &lvc);
}
int SimpleListFrame::addItem(String name)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = ListView_GetItemCount(hWnd);
  lvi.mask = LVIF_TEXT;
  lvi.pszText = name.getBuffer();
  ListView_InsertItem(hWnd, &lvi);
  return lvi.iItem;
}
int SimpleListFrame::addItem(String name, uint32 param)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = ListView_GetItemCount(hWnd);
  lvi.mask = LVIF_TEXT | LVIF_PARAM;
  lvi.pszText = name.getBuffer();
  lvi.lParam = param;
  ListView_InsertItem(hWnd, &lvi);
  return lvi.iItem;
}
void SimpleListFrame::setItemText(int item, int column, String text)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = item;
  lvi.iSubItem = column;
  lvi.mask = LVIF_TEXT;
  lvi.pszText = text.getBuffer();
  ListView_SetItem(hWnd, &lvi);
}
void SimpleListFrame::setItemTextUtf8(int item, int column, String text)
{
  int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.length() + 1, NULL, 0);
  wchar_t* buf = new wchar_t[size];
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.length() + 1, buf, size);

  LVITEMW lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = item;
  lvi.iSubItem = column;
  lvi.mask = LVIF_TEXT;
  lvi.pszText = buf;
  SendMessage(hWnd, LVM_SETITEMW, 0, (uint32) &lvi);

  delete[] buf;
}

////////////////////////////////////////////////////

ListFrame::ListFrame(Frame* parent, int id, int style, int styleEx)
  : WindowFrame(parent)
{
  wcBuf = NULL;
  wcBufSize = 0;
  style |= LVS_ALIGNLEFT | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHAREIMAGELISTS;
  colorMode = colorNone;
  create(WC_LISTVIEW, "", WS_CHILD | WS_TABSTOP | WS_HSCROLL | style, WS_EX_CLIENTEDGE);
  setFont(FontSys::getSysFont());
  setId(id);
  ListView_SetExtendedListViewStyle(hWnd, styleEx);
  ListView_SetImageList(hWnd, getApp()->getImageLibrary()->getImageList(), LVSIL_SMALL);

  enableTooltips(true);
}
ListFrame::~ListFrame()
{
  delete[] wcBuf;
}
//LVS_ALIGNLEFT

uint32 ListFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DRAWITEM)
  {
    drawItem((DRAWITEMSTRUCT*) lParam);
    return TRUE;
  }
  return M_UNHANDLED;
}
int ListFrame::convertUtf8(String text)
{
  int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.length(), NULL, 0);
  if (wcBufSize < size)
  {
    delete[] wcBuf;
    wcBuf = new wchar_t[size];
    wcBufSize = size;
  }
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.length(), wcBuf, size);
  return size;
}
int ListFrame::toolHitTest(POINT pt, ToolInfo* ti)
{
  static char text[65536];
  static int order[256];
  LVHITTESTINFO ht;
  memset(&ht, 0, sizeof ht);
  ht.pt = pt;
  ListView_HitTest(hWnd, &ht);
  if (ht.flags & LVHT_ONITEM)
  {
    ListView_GetItemRect(hWnd, ht.iItem, &ti->rc, LVIR_ICON);
    if (pt.x >= ti->rc.left && pt.x < ti->rc.right &&
        pt.y >= ti->rc.top && pt.y < ti->rc.bottom)
    {
      LVITEM lvi;
      memset(&lvi, 0, sizeof lvi);
      lvi.mask = LVIF_IMAGE;
      lvi.iItem = ht.iItem;
      ListView_GetItem(hWnd, &lvi);
      ti->text = getApp()->getImageLibrary()->getTooltip(lvi.iImage);
      return ti->text.isEmpty() ? -1 : 0;
    }
    else
    {
      RECT label, wnd;
      ListView_GetItemRect(hWnd, ht.iItem, &label, LVIR_LABEL);
      GetClientRect(hWnd, &wnd);
      label.right -= 1;

      LVCOLUMN lvc;
      memset(&lvc, 0, sizeof lvc);
      lvc.mask = LVCF_FMT | LVCF_WIDTH;

      HWND header = ListView_GetHeader(hWnd);
      int count = Header_GetItemCount(header);
      ListView_GetColumnOrderArray(hWnd, count, order);
      for (int i = 0; i < count; i++)
      {
        ListView_GetColumn(hWnd, order[i], &lvc);
        label.right -= lvc.cx;
        if (order[i] == 0)
          break;
      }
      for (int i = 0; i < count; i++)
      {
        ListView_GetColumn(hWnd, order[i], &lvc);
        label.left = label.right;
        label.right += lvc.cx;
        ListView_GetItemText(hWnd, ht.iItem, order[i], text, sizeof text);
        if (text[0] == 0)
          continue;

        uint32 flags = DT_LEFT;
        if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
          flags = DT_RIGHT;
        else if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_CENTER)
          flags = DT_CENTER;

        RECT item = label;
        if (order[i] == 0)
        {
          item.left += 2;
          item.right -= 2;
        }
        else
        {
          item.left += 6;
          item.right -= 6;
        }
        if (!colUtf8[order[i]] && flags == DT_LEFT)
        {
          HIMAGELIST imgList = ListView_GetImageList(hWnd, LVSIL_SMALL);
          SIZE sz;
          HDC hDC = GetDC(hWnd);
          int prev = 0;
          for (int cur = 0; text[cur] && item.left < wnd.right; cur++)
          {
            bool valid = false;
            int index = 0;
            int save = cur;
            if (text[cur] == '$')
            {
              cur++;
              while (text[cur] >= '0' && text[cur] <= '9')
                index = index * 10 + int(text[cur++] - '0');
              if (text[cur] == '$' && index >= 0 && index < ImageList_GetImageCount(imgList))
                valid = true;
              else
                cur = save;
            }
            if (valid)
            {
              if (save > prev)
              {
                GetTextExtentPoint32(hDC, text + prev, save - prev, &sz);
                item.left += sz.cx;
              }
              ti->rc.left = item.left;
              ti->rc.right = item.left + 16;
              ti->rc.top = (item.top + item.bottom) / 2 - 8;
              ti->rc.bottom = ti->rc.top + 16;
              if (pt.x >= ti->rc.left && pt.x < ti->rc.right &&
                  pt.y >= ti->rc.top && pt.y < ti->rc.bottom)
              {
                ti->text = getApp()->getImageLibrary()->getTooltip(index);
                ReleaseDC(hWnd, hDC);
                return (ti->text.isEmpty() ? -1 : 0);
              }
              item.left += 18;
              prev = cur + 1;
            }
          }
          ReleaseDC(hWnd, hDC);
        }
      }
    }
  }
  return -1;
}
int ListFrame::drawItemText(HDC hDC, String text, RECT* rc, uint32 format, bool utf8)
{
  uint32 FMT = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;
  SIZE sz;
  int width = 0;

  if (utf8)
  {
    int length = convertUtf8(text);
    GetTextExtentPoint32W(hDC, wcBuf, length, &sz);
    width = sz.cx;
    if (rc)
      DrawTextW(hDC, wcBuf, length, rc, format | FMT);
  }
  else
  {
    HIMAGELIST imgList = ListView_GetImageList(hWnd, LVSIL_SMALL);
    int prev = 0;
    if (format == DT_LEFT)
    {
      for (int cur = 0; cur < text.length(); cur++)
      {
        bool valid = false;
        int index = 0;
        int save = cur;
        if (text[cur] == '$')
        {
          cur++;
          while (text[cur] >= '0' && text[cur] <= '9')
            index = index * 10 + int(text[cur++] - '0');
          if (text[cur] == '$' && index >= 0 && index < ImageList_GetImageCount(imgList))
            valid = true;
          else
            cur = save;
        }
        if (valid)
        {
          if (save > prev)
          {
            GetTextExtentPoint32(hDC, text.c_str() + prev, save - prev, &sz);
            if (rc)
            {
              DrawText(hDC, text.c_str() + prev, save - prev, rc, format | FMT);
              rc->left += sz.cx;
            }
            width += sz.cx;
          }
          if (rc && rc->left < rc->right)
          {
            if (index != 0)
              getApp()->getImageLibrary()->drawAlpha(hDC, index, 
                rc->left + 1, (rc->top + rc->bottom) / 2 - 8,
                rc->right - rc->left > 16 ? 16 : rc->right - rc->left, 16);
            rc->left += 18;
          }
          width += 18;
          prev = cur + 1;
        }
      }
    }
    if (prev < text.length())
    {
      GetTextExtentPoint32(hDC, text.c_str() + prev, text.length() - prev, &sz);
      width += sz.cx;
      if (rc)
        DrawText(hDC, text.c_str() + prev, text.length() - prev, rc, format | FMT);
    }
  }
  return width;
}
void ListFrame::drawItem(DRAWITEMSTRUCT* dis)
{
  static char buf[65536];
  static int order[256];
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
  lvi.iItem = dis->itemID;
  lvi.iSubItem = 0;
  lvi.pszText = buf;
  lvi.cchTextMax = sizeof buf;
  lvi.stateMask = 0xFFFF;
  ListView_GetItem(hWnd, &lvi);

  uint32 color;
  if (colorMode == colorNone)
    color = 0xFFFFFF;
  else if (colorMode == colorStripe)
    color = (lvi.iItem & 1 ? 0xFFFFFF : 0xFFEEEE);
  else
    color = uint32(lvi.lParam) & 0xFFFFFF;

  if (color == 0)
  {
    lvi.state &= ~(LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED);
    if (IsWindowEnabled(hWnd))
      color = 0xFFFFFF;
    else
      color = GetSysColor(COLOR_BTNFACE);
  }

  bool focus = (GetFocus() == hWnd);
  uint32 style = GetWindowLong(hWnd, GWL_STYLE);
  bool selected = (focus || (style & LVS_SHOWSELALWAYS)) && (lvi.state & LVIS_SELECTED);
  selected = selected || (lvi.state & LVIS_DROPHILITED);

  RECT allLabels, label, icon, wnd;
  ListView_GetItemRect(hWnd, dis->itemID, &allLabels, LVIR_BOUNDS);
  ListView_GetItemRect(hWnd, dis->itemID, &label, LVIR_LABEL);
  ListView_GetItemRect(hWnd, dis->itemID, &icon, LVIR_ICON);
  GetClientRect(hWnd, &wnd);
  //allLabels.right = wnd.right;

  uint32 clrTextSave, clrBkSave;
  if (selected)
  {
    clrTextSave = SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    clrBkSave = SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
  }
  else
    clrBkSave = SetBkColor(dis->hDC, color);
//  if (ListView_GetExtendedListViewStyle(hWnd) & LVS_EX_FULLROWSELECT)
  ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &allLabels, NULL, 0, NULL);
//  else
//    ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &label, NULL, 0, NULL);

  label.right -= 1;
  RECT item = label;
  item.left += 2;
  item.right -= 2;

  if ((lvi.state & LVIS_FOCUSED) == 0 || ListView_GetEditControl(hWnd) == NULL)
    drawItemText(dis->hDC, lvi.pszText, &item, DT_LEFT, colUtf8[0]);

  HWND header = ListView_GetHeader(hWnd);

  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_FMT | LVCF_WIDTH;

  int count = Header_GetItemCount(header);
  ListView_GetColumnOrderArray(hWnd, count, order);
  for (int i = 0; i < count; i++)
  {
    ListView_GetColumn(hWnd, order[i], &lvc);
    label.right -= lvc.cx;
    if (order[i] == 0)
      break;
  }
  for (int i = 0; i < count; i++)
  {
    ListView_GetColumn(hWnd, order[i], &lvc);
    label.left = label.right;
    label.right += lvc.cx;
    if (order[i] == 0)
      continue;
    ListView_GetItemText(hWnd, lvi.iItem, order[i], buf, sizeof buf);
    if (buf[0] == 0)
      continue;
    uint32 flags = DT_LEFT;
    if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
      flags = DT_RIGHT;
    else if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_CENTER)
      flags = DT_CENTER;

    item = label;
    item.left += 6;
    item.right -= 6;
    drawItemText(dis->hDC, buf, &item, flags, colUtf8[order[i]]);
  }

  if (lvi.iImage != 0)
    getApp()->getImageLibrary()->drawAlpha(dis->hDC, lvi.iImage, 
      icon.left, icon.top, 16, 16);

  if (selected)
    SetTextColor(dis->hDC, clrTextSave);
  SetBkColor(dis->hDC, clrBkSave);
}
void ListFrame::clear()
{
  ListView_DeleteAllItems(hWnd);
}
void ListFrame::clearColumns()
{
  colUtf8.clear();
  for (int i = Header_GetItemCount(ListView_GetHeader(hWnd)) - 1; i >= 0; i--)
    ListView_DeleteColumn(hWnd, i);
}

void ListFrame::insertColumn(int i, String name, int fmt)
{
  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_FMT | LVCF_TEXT;
  lvc.fmt = fmt;
  lvc.pszText = name.getBuffer();
  ListView_InsertColumn(hWnd, i, &lvc);
  if (colUtf8.length() <= i)
    colUtf8.resize(i + 1, false);
}
void ListFrame::setColumnWidth(int i, int width)
{
  if (width == LVSCW_AUTOSIZE_USEHEADER || width == LVSCW_AUTOSIZE)
  {
    char buf[MAX_PATH];
    LVCOLUMN lvc;
    memset(&lvc, 0, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_TEXT;
    lvc.pszText = buf;
    lvc.cchTextMax = sizeof buf - 1;
    ListView_GetColumn(hWnd, i, &lvc);
    uint32 flags = DT_LEFT;
    if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
      flags = DT_RIGHT;
    else if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_CENTER)
      flags = DT_CENTER;

    int awidth = 0;
    HDC hDC = GetDC(hWnd);
    SelectObject(hDC, getFont());

    if (width == LVSCW_AUTOSIZE_USEHEADER)
    {
      SIZE sz;
      GetTextExtentPoint32(hDC, buf, strlen(buf), &sz);
      awidth = sz.cx;
    }

    for (int j = ListView_GetItemCount(hWnd) - 1; j >= 0; j--)
    {
      ListView_GetItemText(hWnd, j, i, buf, sizeof buf);
      int w = drawItemText(hDC, buf, NULL, flags, colUtf8[i]);
      if (i == 0)
        w += 18;
      if (w > awidth)
        awidth = w;
    }

    ReleaseDC(hWnd, hDC);

    width = awidth + 12;
  }
  ListView_SetColumnWidth(hWnd, i, width);
}
int ListFrame::addItem(String name, int image, int data)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = ListView_GetItemCount(hWnd);
  lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
  lvi.pszText = name.getBuffer();
  lvi.iImage = image;
  lvi.lParam = data;
  ListView_InsertItem(hWnd, &lvi);
  return lvi.iItem;
}
void ListFrame::setItemText(int item, int column, String text)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = item;
  lvi.iSubItem = column;
  lvi.mask = LVIF_TEXT;
  lvi.pszText = text.getBuffer();
  ListView_SetItem(hWnd, &lvi);
}
int ListFrame::getCount() const
{
  return ListView_GetItemCount(hWnd);
}
int ListFrame::getItemParam(int item)
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof lvi);
  lvi.iItem = item;
  lvi.mask = LVIF_PARAM;
  ListView_GetItem(hWnd, &lvi);
  return lvi.lParam;
}

//////////////////////////////////////////////

ComboFrameEx::ComboFrameEx(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  boxHeight = 500;
  create("ComboBox", "", style | WS_CHILD | CBS_OWNERDRAWFIXED | WS_TABSTOP | WS_VSCROLL, 0);
  setFont(FontSys::getSysFont());
  setId(id);
  ComboBox_SetItemHeight(hWnd, -1, 16);
  setHeight(21);
  prevSel = 0;
}

void ComboFrameEx::reset()
{
  SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
  items.clear();
  prevSel = 0;
}
int ComboFrameEx::addString(String text, uint32 color, char const* icon, uint32 data)
{
  int id = items.length();
  BoxItem& item = items.push();
  item.data = data;
  item.color = color;
  item.icon = (icon ? getApp()->getImageLibrary()->getListIndex(icon) : 0xFFFFFFFF);
  item.wtext = text.toWide();

  int pos = SendMessage(hWnd, CB_ADDSTRING, 0, (uint32) "");
  if (pos != CB_ERR)
    SendMessage(hWnd, CB_SETITEMDATA, pos, id);
  return pos;
}
int ComboFrameEx::getCount() const
{
  return SendMessage(hWnd, CB_GETCOUNT, 0, 0);
}
int ComboFrameEx::getItemData(int item) const
{
  return items[SendMessage(hWnd, CB_GETITEMDATA, item, 0)].data;
}
void ComboFrameEx::setItemData(int item, int data)
{
  items[SendMessage(hWnd, CB_GETITEMDATA, item, 0)].data = data;
}
int ComboFrameEx::getCurSel() const
{
  return SendMessage(hWnd, CB_GETCURSEL, 0, 0);
}
void ComboFrameEx::setCurSel(int sel)
{
  SendMessage(hWnd, CB_SETCURSEL, sel, 0);
  onMessage(WM_COMMAND, MAKELONG(id(), CBN_SELCHANGE), (uint32) hWnd);
}

void ComboFrameEx::onMove(uint32 data)
{
  if (hWnd)
  {
    uint32 flags = SWP_NOREPOSITION;
    HWND hWndInsertAfter = NULL;
    if (visible())
    {
      if (IsWindowVisible(hWnd))
        flags |= SWP_NOZORDER;
      else
      {
        flags |= SWP_SHOWWINDOW;
        hWndInsertAfter = HWND_TOP;
      }
    }
    else
      flags |= SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW;
    if (data)
      DeferWindowPos((HDWP) data, hWnd, hWndInsertAfter, left(), top(), width(), boxHeight, flags);
    else
      SetWindowPos(hWnd, hWndInsertAfter, left(), top(), width(), boxHeight, flags);
  }
}
uint32 ComboFrameEx::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DRAWITEM)
  {
    DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*) lParam;
    if (dis->itemID == -1)
      return TRUE;
    BoxItem& item = items[dis->itemData];

    uint32 clrTextSave, clrBkSave;
    if (item.color != 0 && (
      (dis->itemAction | ODA_SELECT) && (dis->itemState & ODS_SELECTED) &&
      !(dis->itemState & ODS_COMBOBOXEDIT)))
    {
      clrTextSave = SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      clrBkSave = SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
      clrTextSave = SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
      clrBkSave = SetBkColor(dis->hDC, item.color ? item.color : 0xFFFFFF);
    }
    ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &dis->rcItem, NULL, 0, NULL);

    RECT rc = dis->rcItem;
    rc.left += 2;
    rc.right -= 2;
    if (item.icon != 0xFFFFFFFF)
      getApp()->getImageLibrary()->drawAlpha(dis->hDC, item.icon,
        rc.left, (rc.top + rc.bottom) / 2 - 8, 16, 16);
    rc.left += 18;

    DrawTextW(dis->hDC, item.wtext, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    SetTextColor(dis->hDC, clrTextSave);
    SetBkColor(dis->hDC, clrBkSave);
    return TRUE;
  }
  else if (message == WM_COMMAND && lParam == (uint32) hWnd && HIWORD(wParam) == CBN_SELCHANGE)
  {
    int sel = SendMessage(hWnd, CB_GETCURSEL, 0, 0);
    int delta = (sel >= prevSel ? 1 : -1);
    prevSel = sel;
    bool hit = false;
    int count = SendMessage(hWnd, CB_GETCOUNT, 0, 0);
    while (items[SendMessage(hWnd, CB_GETITEMDATA, sel, 0)].color == 0)
    {
      sel += delta;
      if (sel < 0 || sel >= count)
      {
        if (hit)
        {
          sel = prevSel;
          break;
        }
        else
        {
          delta = -delta;
          sel += delta * 2;
          hit = true;
        }
      }
    }
    if (sel != prevSel)
    {
      SendMessage(hWnd, CB_SETCURSEL, sel, 0);
      prevSel = sel;
    }
  }
  return M_UNHANDLED;
}
