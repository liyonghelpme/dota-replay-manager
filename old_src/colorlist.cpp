#include "stdafx.h"
#include "colorlist.h"
#include "ilib.h"
#include "utils.h"

IMPLEMENT_DYNAMIC(CColorRect, CWnd)
CColorRect::CColorRect (COLORREF c, CRect const& rc, CWnd* parent)
{
  clr = c;
  CreateEx (0, NULL, "", WS_CHILD, rc, parent, IDC_STATIC);
  ShowWindow (SW_SHOW);
}
BEGIN_MESSAGE_MAP(CColorRect, CWnd)
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
BOOL CColorRect::OnEraseBkgnd (CDC* pDC)
{
  CBrush br (clr);
  CRect rc;
  GetClientRect (rc);
  pDC->FillRect (rc, &br);
  return TRUE;
}

#define OFFSET_FIRST    2
#define OFFSET_OTHER    6

INT_PTR CColorList::OnToolHitTest (CPoint point, TOOLINFO* pTI) const
{
  if (pSavedDC == NULL)
    return -1;
  LVHITTESTINFO info;
  memset (&info, 0, sizeof info);
  info.pt = point;
  if (HitTest (&info) >= 0 && (info.flags & LVHT_ONITEM))
  {
    CRect iconRect;
    GetItemRect (info.iItem, iconRect, LVIR_ICON);
    if (iconRect.PtInRect (point))
    {
      LVITEM lvi;
      memset (&lvi, 0, sizeof lvi);
	    lvi.mask = LVIF_IMAGE;
	    lvi.iItem = info.iItem;
	    GetItem (&lvi);
      char const* text = getImageTip (lvi.iImage);
      if (text && text[0])
      {
        pTI->hwnd = m_hWnd;
        GetItemRect (info.iItem, &pTI->rect, LVIR_ICON);
        pTI->uId = lvi.iImage + 1000;
        pTI->lpszText = clonestr (text);
        return pTI->uId;
      }
    }
    else
    {
      CImageList* icons = GetImageList (LVSIL_SMALL);
      if (icons == NULL)
        return -1;
      CRect rc;
      GetItemRect (info.iItem, rc, LVIR_LABEL);
      rc.right -= 1;
      int order[256];
      int count = GetHeaderCtrl ()->GetItemCount ();
      GetColumnOrderArray (order, count);
	    LV_COLUMN lvc;
      memset (&lvc, 0, sizeof lvc);
	    lvc.mask = LVCF_FMT | LVCF_WIDTH;
      for (int i = 0; i < count; i++)
      {
        GetColumn (order[i], &lvc);
        rc.right -= lvc.cx;
        if (order[i] == 0)
          break;
      }
      for (int i = 0; i < count; i++)
      {
        GetColumn (order[i], &lvc);
        rc.left = rc.right;
        rc.right = rc.left + lvc.cx;
        if (rc.PtInRect (point))
        {
          if (order[i] == 0)
          {
            rc.left += OFFSET_FIRST;
            rc.right -= OFFSET_FIRST;
          }
          else
          {
            rc.left += OFFSET_OTHER;
            rc.right -= OFFSET_OTHER;
          }
          if (!rc.PtInRect (point) || ((lvc.fmt & LVCFMT_JUSTIFYMASK) != LVCFMT_LEFT))
            return -1;
        	char text[MAX_PATH];
          GetItemText (info.iItem, order[i], text, sizeof text);
          int length = (int) strlen (text);
          int prev = 0;
          for (int cur = 0; cur < length; cur++)
          {
            bool valid = false;
            int index = 0;
            int save = cur;
            if (text[cur] == '$')
            {
              cur++;
              while (text[cur] >= '0' && text[cur] <= '9')
                index = index * 10 + int (text[cur++] - '0');
              if (text[cur] == '$' && index >= 0 && index < icons->GetImageCount ())
                valid = true;
              else
                cur = save;
            }
            if (valid)
            {
              if (save > prev)
                rc.left += pSavedDC->GetTextExtent (text + prev, save - prev).cx;
              if (point.x < rc.left)
                return -1;
              if (rc.left < rc.right && point.x >= rc.left + 1 && point.x < rc.left + 17)
              {
                char const* text = getImageTip (index);
                if (text && text[0])
                {
                  pTI->hwnd = m_hWnd;
                  pTI->rect.top = (rc.top + rc.bottom) / 2 - 8;
                  pTI->rect.bottom = pTI->rect.top + 16;
                  pTI->rect.left = rc.left + 1;
                  pTI->rect.right = pTI->rect.left + 16;
                  pTI->uId = index + 1000;
                  pTI->lpszText = clonestr (text);
                  return pTI->uId;
                }
              }
              rc.left += 18;
              prev = cur + 1;
            }
          }
          return -1;
        }
      }
      return -1;
    }
  }
  return -1;
}
void CColorList::DrawItemText (CDC* pDC, char const* text, int length, RECT* rect, DWORD format)
{
  const int FORMATS = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER;
  CImageList* icons = GetImageList (LVSIL_SMALL);
  if (length < 0)
    length = (int) strlen (text);
  int prev = 0;
  if (format == DT_LEFT && icons)
  {
    RECT rc;
    memcpy (&rc, rect, sizeof (RECT));
    for (int cur = 0; cur < length; cur++)
    {
      bool valid = false;
      int index = 0;
      int save = cur;
      if (text[cur] == '$')
      {
        cur++;
        while (text[cur] >= '0' && text[cur] <= '9')
          index = index * 10 + int (text[cur++] - '0');
        if (text[cur] == '$' && index >= 0 && index < icons->GetImageCount ())
          valid = true;
        else
          cur = save;
      }
      if (valid)
      {
        if (save > prev)
        {
          pDC->DrawText (text + prev, save - prev, &rc, format | FORMATS);
          rc.left += pDC->GetTextExtent (text + prev, save - prev).cx;
        }
        if (rc.left < rc.right)
          icons->DrawEx (pDC, index, CPoint (rc.left + 1, (rc.top + rc.bottom) / 2 - 8),
            CSize (rc.right - rc.left, 16), CLR_NONE, CLR_NONE, ILD_NORMAL);
        rc.left += 18;
        prev = cur + 1;
      }
    }
  }
  if (prev < length)
	  pDC->DrawText (text + prev, length - prev, rect, format | FORMATS);
}
void CColorList::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  if (pSavedDC == NULL)
    pSavedDC = GetDC ();
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	CRect rcItem (lpDrawItemStruct->rcItem);
	int nItem = lpDrawItemStruct->itemID;
	BOOL bFocus = (GetFocus () == this);
	COLORREF clrTextSave, clrBkSave;
	static _TCHAR szBuff[MAX_PATH];	
	COLORREF color;

	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = szBuff;
	lvi.cchTextMax = sizeof szBuff;
	lvi.stateMask = 0xFFFF;
	GetItem (&lvi);

  if (simple)
    color = (lvi.iItem & 1 ? 0xFFFFFF : 0xFFEEEE);
  else
	  color = COLORREF (lvi.lParam) & 0x00FFFFFF;

  if (color == 0)
  {
    lvi.state &= ~(LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED);
    if (IsWindowEnabled ())
      color = 0xFFFFFF;
    else
      color = GetSysColor (COLOR_BTNFACE);
  }

	BOOL bSelected = (bFocus || (GetStyle () & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
	bSelected = bSelected || (lvi.state & LVIS_DROPHILITED);

	CRect rcAllLabels;
	GetItemRect (nItem, rcAllLabels, LVIR_BOUNDS);
	CRect rcLabel;
	GetItemRect (nItem, rcLabel, LVIR_LABEL);
  CRect rcIcon;
  GetItemRect (nItem, rcIcon, LVIR_ICON);
  CRect wnd;
  GetClientRect (wnd);
  rcAllLabels.right = wnd.right;

	if (bSelected)
	{
		clrTextSave = pDC->SetTextColor (::GetSysColor (COLOR_HIGHLIGHTTEXT));
		clrBkSave = pDC->SetBkColor (::GetSysColor (COLOR_HIGHLIGHT));

		pDC->FillRect(rcAllLabels, &CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
	}
	else
		pDC->FillRect(rcAllLabels, &CBrush(color));

	GetItemRect (nItem, rcItem, LVIR_LABEL);
	rcItem.right -= 1;

	rcLabel = rcItem;
	rcLabel.left += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;

	DrawItemText (pDC, lvi.pszText, -1, rcLabel, DT_LEFT);

	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH;

  int order[256];
  int count = GetHeaderCtrl ()->GetItemCount ();
  GetColumnOrderArray (order, count);
  for (int i = 0; i < count; i++)
  {
    GetColumn (order[i], &lvc);
    rcItem.right -= lvc.cx;
    if (order[i] == 0)
      break;
  }

  for (int i = 0; i < count; i++)
  {
    int nColumn = order[i];
    GetColumn (nColumn, &lvc);
		rcItem.left = rcItem.right;
		rcItem.right += lvc.cx;
    if (nColumn == 0)
      continue;

		int nRetLen = GetItemText (nItem, nColumn, szBuff, sizeof szBuff);
		if (nRetLen == 0)
			continue;

		UINT nJustify = DT_LEFT;

		switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
		{
			case LVCFMT_RIGHT:
				nJustify = DT_RIGHT;
				break;
			case LVCFMT_CENTER:
				nJustify = DT_CENTER;
				break;
			default:
				break;
		}

		rcLabel = rcItem;
		rcLabel.left += OFFSET_OTHER;
		rcLabel.right -= OFFSET_OTHER;

		DrawItemText (pDC, szBuff, -1, rcLabel, nJustify);
	}

  if (GetImageList (LVSIL_SMALL))
    GetImageList (LVSIL_SMALL)->Draw (pDC, lvi.iImage, CPoint (rcIcon.left, rcIcon.top), ILD_NORMAL);

	if (bSelected)
	{
		pDC->SetTextColor (clrTextSave);
		pDC->SetBkColor (clrBkSave);
	}
}

void CColorBox::InsertItem (char const* text, int image, COLORREF clr, DWORD data)
{
  int id = numItems++;
  items[id].color = clr;
  items[id].data = data;
  items[id].image = image;
  for (int i = 0; items[id].text[i] = text[i]; i++)
    ;
  SetItemData (InsertString (id, ""), (DWORD_PTR) id);
}
void CColorBox::InsertItem (wchar_t const* text, int image, COLORREF clr, DWORD data)
{
  int id = numItems++;
  items[id].color = clr;
  items[id].data = data;
  items[id].image = image;
  wcscpy (items[id].text, text);
  SetItemData (InsertString (id, ""), (DWORD_PTR) id);
}
void CColorBox::Reset ()
{
  CComboBox::ResetContent ();
  numItems = 0;
}

void CColorBox::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  CDC dc;
  dc.Attach(lpDrawItemStruct->hDC);

  BoxItem* item = &items[GetItemData (lpDrawItemStruct->itemID)];
  wchar_t const* lpszText = item->text;

  COLORREF crOldTextColor = dc.GetTextColor();
  COLORREF crOldBkColor = dc.GetBkColor();

  if (item->color != 0 &&
    ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
     (lpDrawItemStruct->itemState & ODS_SELECTED) &&
    !(lpDrawItemStruct->itemState & ODS_COMBOBOXEDIT)))
  {
    dc.SetTextColor (GetSysColor (COLOR_HIGHLIGHTTEXT));
    dc.SetBkColor (GetSysColor (COLOR_HIGHLIGHT));
    dc.FillSolidRect (&lpDrawItemStruct->rcItem, GetSysColor (COLOR_HIGHLIGHT));
  }
  else
  {
    dc.SetTextColor (GetSysColor (COLOR_BTNTEXT));
    if (item->color == 0)
      dc.FillSolidRect (&lpDrawItemStruct->rcItem, RGB (255, 255, 255));
    else
      dc.FillSolidRect (&lpDrawItemStruct->rcItem, item->color);
  }

  CRect itemRect = lpDrawItemStruct->rcItem;
  itemRect.left += OFFSET_FIRST;
  itemRect.right -= OFFSET_FIRST;
  if (list)
  {
    list->Draw (&dc, item->image, CPoint (itemRect.left, itemRect.top), ILD_NORMAL);
    itemRect.left += 16 + OFFSET_FIRST;
  }
  DrawTextW (dc.m_hDC, lpszText, (int) wcslen (lpszText), itemRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

  dc.SetTextColor (crOldTextColor);
  dc.SetBkColor (crOldBkColor);

  dc.Detach();
}

#include "replay.h"
void CBarList::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  CColorList::DrawItem (lpDrawItemStruct);
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	int nItem = lpDrawItemStruct->itemID;
  if (w3g && mx)
  {
	  CRect rc;
	  GetItemRect (nItem, rc, LVIR_BOUNDS);
    rc.left = start;
    rc.right = start + (end - start) * w3g->players[player].acounter[nItem] / mx;
    int mid = (rc.bottom + rc.top) / 2;
    rc.top = mid - 2;
    rc.bottom = mid + 2;
    pDC->FillRect (rc, CBrush::FromHandle ((HBRUSH) GetStockObject (BLACK_BRUSH)));
  }
}

BEGIN_MESSAGE_MAP(CImageBox, CWnd)
  ON_WM_PAINT()
END_MESSAGE_MAP()

CImageBox::CImageBox (CRect const& rc, CBitmap* bitmap, CWnd* parent)
{
  bmp = bitmap;
  CreateEx (0, NULL, "", WS_CHILD, rc, parent, IDC_STATIC);
  ShowWindow (SW_SHOW);
}
void CImageBox::OnPaint ()
{
  CPaintDC dc (this);
  CRect rc;
  GetClientRect (rc);
  if (bmp)
  {
    CDC other;
    other.CreateCompatibleDC (&dc);
    other.SelectObject (bmp);
    dc.BitBlt (0, 0, rc.right, rc.bottom, &other, 0, 0, SRCCOPY);
    other.DeleteDC ();
  }
  else
    dc.FillSolidRect (rc, 0);
}
