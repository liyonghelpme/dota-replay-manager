#include "stdafx.h"
#include "LocManager.h"

#define ITEM_GROW       32

inline CRect CenterOf (HWND hWnd)
{
  CRect rc;
  GetWindowRect (hWnd, rc);
  rc.left = rc.right = (rc.left + rc.right) / 2;
  rc.top = rc.bottom = (rc.top + rc.bottom) / 2;
  return rc;
}
CLocManager::LocItem* CLocManager::GetByHandle (HWND hwnd)
{
  for (int i = 0; i < numItems; i++)
    if (items[i]->wnd == hwnd)
      return items[i];
  if (numItems >= maxItems)
  {
    maxItems += ITEM_GROW;
    temp = new LocItem* [maxItems];
    memset (temp, 0, maxItems * sizeof (LocItem*));
    memcpy (temp, items, numItems * sizeof (LocItem*));
    delete[] items;
    items = temp;
    temp = NULL;
  }
  items[numItems] = new LocItem;
  items[numItems]->wnd = hwnd;
  items[numItems]->parent[SIDE_LEFT] = NULL;
  items[numItems]->parent[SIDE_TOP] = NULL;
  items[numItems]->parent[SIDE_RIGHT] = items[numItems];
  items[numItems]->pside[SIDE_RIGHT] = SIDE_LEFT;
  items[numItems]->parent[SIDE_BOTTOM] = items[numItems];
  items[numItems]->pside[SIDE_BOTTOM] = SIDE_TOP;
  for (int i = 0; i < 4; i++)
    items[numItems]->isratio[i] = false;
  items[numItems]->vis = false;
  return items[numItems++];
}
void CLocManager::SetRectSide (CRect& rc, int side, int val)
{
  if (side == SIDE_LEFT)
    rc.left = val;
  else if (side == SIDE_TOP)
    rc.top = val;
  else if (side == SIDE_RIGHT)
    rc.right = val;
  else if (side == SIDE_BOTTOM)
    rc.bottom = val;
}
int CLocManager::GetRectSide (CRect& rc, int side)
{
  if (side == SIDE_LEFT)
    return rc.left;
  else if (side == SIDE_TOP)
    return rc.top;
  else if (side == SIDE_RIGHT)
    return rc.right;
  else if (side == SIDE_BOTTOM)
    return rc.bottom;
  return 0;
}
int CLocManager::GetRectDim (CRect& rc, int side, float r)
{
  if (side == SIDE_LEFT || side == SIDE_RIGHT)
    return int (float (rc.right - rc.left) * r);
  else if (side == SIDE_TOP || side == SIDE_BOTTOM)
    return int (float (rc.bottom - rc.top) * r);
  return 0;
}
float CLocManager::GetRectRatio (CRect& rc, int side, int o)
{
  if (side == SIDE_LEFT || side == SIDE_RIGHT)
    return float (o) / float (rc.right - rc.left);
  else if (side == SIDE_TOP || side == SIDE_BOTTOM)
    return float (o) / float (rc.bottom - rc.top);
  return 0;
}

CLocManager::CLocManager (CWnd* window)
{
  maxItems = ITEM_GROW;
  numItems = 0;
  items = new LocItem* [maxItems];
  memset (items, 0, maxItems * sizeof (LocItem*));
  wnd = (window ? window->m_hWnd : NULL);
}
CLocManager::CLocManager (HWND window)
{
  maxItems = ITEM_GROW;
  numItems = 0;
  items = new LocItem* [maxItems];
  memset (items, 0, maxItems * sizeof (LocItem*));
  wnd = window;
}
void CLocManager::SetWindow (CWnd* window)
{
  wnd = (window ? window->m_hWnd : NULL);
}
void CLocManager::SetWindow (HWND window)
{
  wnd = window;
}
CLocManager::~CLocManager ()
{
  for (int i = 0; i < numItems; i++)
    delete items[i];
  delete[] items;
}
void CLocManager::SetItemRelative (int id, int side, int rel, int relto, int mode)
{
  LocItem* item = GetByHandle (GetDlgItem (wnd, id));
  item->parent[side] = GetByHandle (GetDlgItem (wnd, rel));
  item->pside[side] = relto;
  item->isratio[side] = (mode == PERCENT);
  if (item->parent[side] == item && item->parent[relto] == item)
    item->parent[relto] = NULL;
}
void CLocManager::SetItemAbsolute (int id, int side, int mode)
{
  LocItem* item = GetByHandle (GetDlgItem (wnd, id));
  item->parent[side] = NULL;
  item->isratio[side] = (mode == PERCENT);
}
void CLocManager::SetItemRelative (CWnd* pwnd, int side, CWnd* rel, int relto, int mode)
{
  LocItem* item = GetByHandle (pwnd->m_hWnd);
  item->parent[side] = GetByHandle (rel->m_hWnd);
  item->pside[side] = relto;
  item->isratio[side] = (mode == PERCENT);
  if (item->parent[side] == item && item->parent[relto] == item)
    item->parent[relto] = NULL;
}
void CLocManager::SetItemAbsolute (CWnd* pwnd, int side, int mode)
{
  LocItem* item = GetByHandle (pwnd->m_hWnd);
  item->parent[side] = NULL;
  item->isratio[side] = (mode == PERCENT);
}
void CLocManager::SetItemRelative (HWND hwnd, int side, HWND rel, int relto, int mode)
{
  LocItem* item = GetByHandle (hwnd);
  item->parent[side] = GetByHandle (rel);
  item->pside[side] = relto;
  item->isratio[side] = (mode == PERCENT);
  if (item->parent[side] == item && item->parent[relto] == item)
    item->parent[relto] = NULL;
}
void CLocManager::SetItemAbsolute (HWND hwnd, int side, int mode)
{
  LocItem* item = GetByHandle (hwnd);
  item->parent[side] = NULL;
  item->isratio[side] = (mode == PERCENT);
}
void CLocManager::SetCenterPoint (HWND pt, HWND of)
{
  LocItem* item = GetByHandle (pt);
  item->parent[0] = GetByHandle (of);
}
void CLocManager::SetCenterPoint (HWND pt, CWnd* of)
{
  LocItem* item = GetByHandle (pt);
  item->parent[0] = GetByHandle (of->m_hWnd);
}
void CLocManager::SetCenterPoint (HWND pt, int of)
{
  LocItem* item = GetByHandle (pt);
  item->parent[0] = GetByHandle (GetDlgItem (wnd, of));
}

void CLocManager::DFS (LocItem* item)
{
  if (item == NULL || item->vis) return;
  item->vis = true;
  for (int i = 0; i < 4; i++)
    DFS (item->parent[i]);
  temp[tempItems++] = item;
}
void ScreenToClient (HWND hWnd, RECT* rc)
{
  ScreenToClient (hWnd, (LPPOINT) &rc->left);
  ScreenToClient (hWnd, (LPPOINT) &rc->right);
}
void CLocManager::Start ()
{
  temp = new LocItem* [maxItems];
  memset (temp, 0, maxItems * sizeof (LocItem*));
  tempItems = 0;
  for (int i = 0; i < numItems; i++)
    DFS (items[i]);
  delete[] items;
  items = temp;
  temp = NULL;
  CRect rc;
  GetClientRect (wnd, rc);
  for (int i = 0; i < numItems; i++)
  {
    items[i]->vis = false;
    if (items[i]->wnd >= 0 && items[i]->wnd < NUM_APOINTS)
    {
      items[i]->rc = CenterOf (items[i]->parent[0]->wnd);
      ScreenToClient (wnd, items[i]->rc);
    }
    else
    {
      GetWindowRect (items[i]->wnd, items[i]->rc);
      ScreenToClient (wnd, items[i]->rc);
      for (int j = 0; j < 4; j++)
      {
        items[i]->offset[j] = GetRectSide (items[i]->rc, j);
        if (items[i]->parent[j])
        {
          if (items[i]->parent[j]->vis)
            items[i]->parent[j] = NULL;
          else
            items[i]->offset[j] -= GetRectSide (items[i]->parent[j]->rc, items[i]->pside[j]);
        }
        if (items[i]->parent[j] == NULL)
          items[i]->offset[j] -= GetRectSide (rc, j);
        if (items[i]->isratio[j])
          items[i]->oratio[j] = GetRectRatio (rc, j, items[i]->offset[j]);
      }
    }
  }
}
bool CLocManager::Update ()
{
  if (wnd == NULL)
    return false;
  CRect rc;
  GetClientRect (wnd, rc);
  for (int i = 0; i < numItems; i++)
  {
    if (items[i]->wnd >= 0 && items[i]->wnd < NUM_APOINTS)
    {
      items[i]->rc = CenterOf (items[i]->parent[0]->wnd);
      ScreenToClient (wnd, items[i]->rc);
    }
    else
    {
      GetWindowRect (items[i]->wnd, items[i]->rc);
      ScreenToClient (wnd, items[i]->rc);
      for (int j = 0; j < 4; j++)
      {
        if (items[i]->isratio[j])
          items[i]->offset[j] = GetRectDim (rc, j, items[i]->oratio[j]);
        if (items[i]->parent[j])
        {
          if (items[i]->parent[j] != items[i])
            SetRectSide (items[i]->rc, j, GetRectSide (items[i]->parent[j]->rc, items[i]->pside[j]) +
              items[i]->offset[j]);
        }
        else
          SetRectSide (items[i]->rc, j, GetRectSide (rc, j) + items[i]->offset[j]);
      }
      for (int j = 0; j < 4; j++)
      {
        if (items[i]->parent[j] == items[i])
          SetRectSide (items[i]->rc, j, GetRectSide (items[i]->rc, items[i]->pside[j]) +
            items[i]->offset[j]);
      }
      if (items[i]->wnd < 0 || items[i]->wnd >= NUM_APOINTS)
      {
        SetWindowPos (items[i]->wnd, NULL, items[i]->rc.left, items[i]->rc.top,
          items[i]->rc.Width (), items[i]->rc.Height (), SWP_NOZORDER);
      }
    }
  }
  return true;
}

CCtrlGroup::CCtrlGroup ()
{
  maxItems = 16;
  numItems = 0;
  items = new CWnd* [maxItems];
  memset (items, 0, maxItems * sizeof (CWnd*));
}
CCtrlGroup::~CCtrlGroup ()
{
  delete[] items;
}

void CCtrlGroup::AddItem (CWnd* item)
{
  if (item == NULL) return;
  if (numItems >= maxItems)
  {
    maxItems *= 2;
    CWnd** temp = new CWnd* [maxItems];
    memset (temp, 0, maxItems * sizeof (CWnd*));
    memcpy (temp, items, numItems * sizeof (CWnd*));
    delete[] items;
    items = temp;
  }
  items[numItems++] = item;
}

void CCtrlGroup::ShowWindow (int nCmdShow)
{
  for (int i = 0; i < numItems; i++)
    items[i]->ShowWindow (nCmdShow);
}

void CCtrlGroup::EnableWindow (BOOL bEnable)
{
  for (int i = 0; i < numItems; i++)
    items[i]->EnableWindow (bEnable);
}

CColManager::CColManager (CListCtrl* listctrl)
{
  cols = NULL;
  list = listctrl;
  maxCols = 0;
}
CColManager::~CColManager ()
{
  delete[] cols;
}
void CColManager::SetListCtrl (CListCtrl* listctrl)
{
  list = listctrl;
}
void CColManager::Read ()
{
  if (list)
  {
    numCols = list->GetHeaderCtrl ()->GetItemCount ();
    if (numCols > maxCols)
    {
      maxCols = numCols + 16;
      delete[] cols;
      cols = new int[maxCols];
    }
    CRect rc;
    list->GetHeaderCtrl ()->GetWindowRect (rc);
    rc.right -= rc.left;
//    list->GetClientRect (rc);
    for (int i = 0; i < numCols; i++)
      cols[i] = list->GetColumnWidth (i);
    if (rc.right)
      wd = rc.right;
  }
}
void CColManager::Write ()
{
  if (list)
  {
    CRect rc;
//    list->GetClientRect (rc);
    list->GetHeaderCtrl ()->GetWindowRect (rc);
    rc.right -= rc.left;
    int sum = 0;
    for (int i = list->GetHeaderCtrl ()->GetItemCount () - 2; i >= 0; i--)
      sum += list->GetColumnWidth (i);
    if (rc.right < wd && sum < rc.right * 5 / 4)
    {
      list->SetColumnWidth (list->GetHeaderCtrl ()->GetItemCount () - 1, LVSCW_AUTOSIZE_USEHEADER);
      Read ();
    }
    else
    {
      for (int i = 0; i < numCols; i++)
        list->SetColumnWidth (i, (cols[i] * rc.right) / wd);
    }
  }
}
