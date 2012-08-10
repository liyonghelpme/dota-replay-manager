#include "core/app.h"

#include "frameui/framewnd.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"

#include "listctrl.h"

SimpleListFrame::SimpleListFrame(Frame* parent, int id, int style, int styleEx)
  : WindowFrame(parent)
{
  style |= LVS_ALIGNLEFT | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_NOSCROLL |
           LVS_SINGLESEL;
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

////////////////////////////////////////////////////

ListFrame::ListFrame(Frame* parent, int id, int style, int styleEx)
  : WindowFrame(parent)
{
  style |= LVS_ALIGNLEFT | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS |
           LVS_SINGLESEL;
  simpleColors = false;
  create(WC_LISTVIEW, "", WS_CHILD | WS_TABSTOP | WS_HSCROLL | style, styleEx);
  setFont(FontSys::getSysFont());
  setId(id);
  ListView_SetExtendedListViewStyle(hWnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
  ListView_SetImageList(hWnd, getApp()->getImageLibrary()->getImageList(), LVSIL_SMALL);
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
int ListFrame::getItemTextWidth(HDC hDC, String text, uint32 format)
{
  int prev = 0;
  int width = 0;
  HIMAGELIST imgList = ListView_GetImageList(hWnd, LVSIL_SMALL);
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
          SIZE sz;
          GetTextExtentPoint32(hDC, text.c_str() + prev, save - prev, &sz);
          width += sz.cx;
        }
        width += 18;
        prev = cur + 1;
      }
    }
  }
  if (prev < text.length())
  {
    SIZE sz;
    GetTextExtentPoint32(hDC, text.c_str() + prev, text.length() - prev, &sz);
    width += sz.cx;
  }
  return width;
}
void ListFrame::drawItemText(HDC hDC, String text, RECT rc, uint32 format)
{
  uint32 FMT = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER;
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
          DrawText(hDC, text.c_str() + prev, save - prev, &rc, format | FMT);
          SIZE sz;
          GetTextExtentPoint32(hDC, text.c_str() + prev, save - prev, &sz);
          rc.left += sz.cx;
        }
        if (rc.left < rc.right)
          ImageList_DrawEx(imgList, index, hDC, rc.left + 1, (rc.top + rc.bottom) / 2 - 8,
            rc.right - rc.left > 16 ? 16 : rc.right - rc.left, 16, CLR_NONE, CLR_NONE, ILD_NORMAL);
        rc.left += 18;
        prev = cur + 1;
      }
    }
  }
  if (prev < text.length())
    DrawText(hDC, text.c_str() + prev, text.length() - prev, &rc, format | FMT);
}
void ListFrame::drawItem(DRAWITEMSTRUCT* dis)
{
  char buf[MAX_PATH];
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
  if (simpleColors)
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
  allLabels.right = wnd.right;

  uint32 clrTextSave, clrBkSave;
  if (selected)
  {
    clrTextSave = SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    clrBkSave = SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
  }
  else
    clrBkSave = SetBkColor(dis->hDC, color);
  ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &allLabels, NULL, 0, NULL);

  label.right -= 1;
  RECT item = label;
  item.left += 2;
  item.right -= 2;
  drawItemText(dis->hDC, lvi.pszText, item, DT_LEFT);

  HWND header = ListView_GetHeader(hWnd);

  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof lvc);
  lvc.mask = LVCF_FMT | LVCF_WIDTH;

  int count = Header_GetItemCount(header);
  int* order = new int[count];
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
    drawItemText(dis->hDC, buf, item, flags);
  }
  delete[] order;

  HIMAGELIST imgList = ListView_GetImageList(hWnd, LVSIL_SMALL);
  ImageList_Draw(imgList, lvi.iImage, dis->hDC, icon.left, icon.top, ILD_NORMAL);

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
}
void ListFrame::setColumnWidth(int i, int width)
{
  if ((width == LVSCW_AUTOSIZE_USEHEADER || width == LVSCW_AUTOSIZE) &&
      i != Header_GetItemCount(ListView_GetHeader(hWnd)) - 1)
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
      int w = getItemTextWidth(hDC, buf, flags);
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

//////////////////////////////////////////////

ComboFrameEx::ComboFrameEx(Frame* parent, int id, int style)
  : WindowFrame(parent)
{
  boxHeight = 500;
  create("ComboBox", "", style | WS_CHILD | CBS_OWNERDRAWFIXED | WS_TABSTOP, 0);
  setFont(FontSys::getSysFont());
  setId(id);
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
  item.text = text;
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
}

void ComboFrameEx::onMove()
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
uint32 ComboFrameEx::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_DRAWITEM)
  {
    DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*) lParam;
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
      ImageList_Draw(getApp()->getImageLibrary()->getImageList(), item.icon,
        dis->hDC, rc.left, (rc.top + rc.bottom) / 2 - 8, ILD_NORMAL);
    rc.left += 18;
    DrawText(dis->hDC, item.text.c_str(), item.text.length(), &rc,
      DT_LEFT | DT_SINGLELINE | DT_VCENTER);

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
