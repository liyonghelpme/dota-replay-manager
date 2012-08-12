#include "stdafx.h"
#include "ListCtrlEx.h"

#define OFFSET_FIRST    2
#define OFFSET_OTHER    6

// CListCtrlEx inline functions

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
	//{{AFX_MSG_MAP(CColorListCtrl)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

inline CListCtrlEx::CListCtrlEx() : CListCtrl()
{
}

inline CListCtrlEx::~CListCtrlEx()
{
}

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEx implementation

BOOL CListCtrlEx::AddColumn(LPCTSTR strItem,int nItem,int nSubItem,int nMask,int nFmt)
{
	LV_COLUMN lvc;
	lvc.mask = nMask;
	lvc.fmt = nFmt;
	lvc.pszText = (LPTSTR) strItem;
	lvc.cx = GetStringWidth(lvc.pszText) + 15;
	if(nMask & LVCF_SUBITEM){
		if(nSubItem != -1)
			lvc.iSubItem = nSubItem;
		else
			lvc.iSubItem = nItem;
	}
	return InsertColumn(nItem,&lvc);
}

BOOL CListCtrlEx::AddItem(int nItem,int nSubItem,LPCTSTR strItem,int nImageIndex, DWORD color)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;
	lvItem.pszText = (LPTSTR) strItem;
	if(nImageIndex != -1){
		lvItem.mask |= LVIF_IMAGE;
		lvItem.iImage |= LVIF_IMAGE;
	}
	if(nSubItem == 0)
	{
		lvItem.mask |= LVIF_PARAM;
		lvItem.lParam = color;
		return InsertItem(&lvItem);
	}
	else
		return SetItem(&lvItem);
}

BOOL CListCtrlEx::SetItem( const LVITEM* pItem )
{
	return CListCtrl::SetItem(pItem);
}

/////////////////////////////////////////////////////////////////////////////
// SortTextItems	- Sort the list based on column text
// Returns		- Returns true for success
// nCol			- column that contains the text to be sorted
// bAscending		- indicate sort order
// low			- row to start scanning from - default row is 0
// high			- row to end scan. -1 indicates last row
BOOL CListCtrlEx::SortTextItems(int nCol, BOOL bAscending, int low, int high)
{
	if( nCol >= ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if( high == -1 )
		high = GetItemCount() - 1;

	int lo = low;
	int hi = high;
	CString midItem;

	if( hi <= lo ) return FALSE;

	midItem = GetItemText( (lo+hi)/2, nCol );

	// loop through the list until indices cross
	while( lo <= hi )
	{
		// rowText will hold all column text for one row
		CStringArray rowText;
		// find the first element that is greater than or equal to&nbsp;
		// the partition element starting from the left Index.
		if( bAscending )
			while( (lo < high) && (GetItemText(lo, nCol) < midItem) )
				++lo;
		else
			while( (lo < high) && (GetItemText(lo, nCol) > midItem) )
				++lo;
		// find an element that is smaller than or equal to&nbsp;
		// the partition element starting from the right Index.
		if( bAscending )
			while( (hi > low) && (GetItemText(hi, nCol) > midItem) )
				--hi;
		else
			while( (hi > low) && (GetItemText(hi, nCol) < midItem) )
				--hi;
		// if the indexes have not crossed, swap
		// and if the items are not equal
		if( lo <= hi )
		{
			// swap only if the items are not equal
			if(GetItemText(lo, nCol) != GetItemText(hi, nCol))
			{
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;
				int nColCount =
					((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
				rowText.SetSize( nColCount );
				int i;
				for( i=0; i<nColCount; i++)
					rowText[i] = GetItemText(lo, i);
				lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED |
						LVIS_FOCUSED | LVIS_SELECTED |
						LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;

				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;

				GetItem( &lvitemlo );
				GetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					SetItemText(lo, i, GetItemText(hi, i));

				lvitemhi.iItem = lo;
				SetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					SetItemText(hi, i, rowText[i]);

				lvitemlo.iItem = hi;
				SetItem( &lvitemlo );
			}

			++lo;
			--hi;
		}
	}
	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if( low < hi )
		SortTextItems( nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if( lo < high )
		SortTextItems( nCol, bAscending , lo, high );

	return TRUE;
}

void CListCtrlEx::DrawColoredItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rcItem(lpDrawItemStruct->rcItem);
	int nItem = lpDrawItemStruct->itemID;
	BOOL bFocus = (GetFocus() == this);
	COLORREF clrTextSave, clrBkSave;
	static _TCHAR szBuff[MAX_PATH];	
	COLORREF color;

// get item data

	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = szBuff;
	lvi.cchTextMax = sizeof(szBuff);
	lvi.stateMask = 0xFFFF;     // get all state flags
	CListCtrlEx::GetItem(&lvi);

	color = lvi.lParam;

	BOOL bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
	bSelected = bSelected || (lvi.state & LVIS_DROPHILITED);

// set colors if item is selected

	CRect rcAllLabels;
	CListCtrlEx::GetItemRect(nItem, rcAllLabels, LVIR_BOUNDS);

	CRect rcLabel;
	CListCtrlEx::GetItemRect(nItem, rcLabel, LVIR_LABEL);

	rcAllLabels.left = rcLabel.left;
	//if (m_bClientWidthSel && rcAllLabels.right<m_cxClient)
	//	rcAllLabels.right = m_cxClient;

	if (bSelected)
	{
		clrTextSave = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrBkSave = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

		pDC->FillRect(rcAllLabels, &CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
	}
	else
		pDC->FillRect(rcAllLabels, &CBrush(color));

// draw item label

	CListCtrlEx::GetItemRect(nItem, rcItem, LVIR_LABEL);
	rcItem.right -= 1;

	rcLabel = rcItem;
	rcLabel.left += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;

	pDC->DrawText(lvi.pszText,-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

// draw labels for extra columns

	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH;

	for(int nColumn = 1; CListCtrlEx::GetColumn(nColumn, &lvc); nColumn++)
	{
		rcItem.left = rcItem.right;
		rcItem.right += lvc.cx;

		int nRetLen = CListCtrlEx::GetItemText(nItem, nColumn,
						szBuff, sizeof(szBuff));
		if (nRetLen == 0)
			continue;

		UINT nJustify = DT_LEFT;

		switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
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

		pDC->DrawText(szBuff, -1, rcLabel, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
	}

// draw focus rectangle if item has focus

	if (lvi.state & LVIS_FOCUSED && bFocus)
		pDC->DrawFocusRect(rcAllLabels);

// set original colors if item was selected

	if (bSelected)
	{
		pDC->SetTextColor(clrTextSave);
		pDC->SetBkColor(clrBkSave);
	}
} // end of DrawColoredItem
