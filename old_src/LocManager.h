#ifndef __LOC_MANAGER_H__
#define __LOC_MANAGER_H__

#define SIDE_LEFT       0
#define SIDE_TOP        1
#define SIDE_RIGHT      2
#define SIDE_BOTTOM     3

#define VALUE           0
#define PERCENT         1

#define NUM_APOINTS     ((HWND) 8)
#define POINT0          ((HWND) 0)
#define POINT1          ((HWND) 1)
#define POINT2          ((HWND) 2)
#define POINT3          ((HWND) 3)
#define POINT4          ((HWND) 4)
#define POINT5          ((HWND) 5)
#define POINT6          ((HWND) 6)
#define POINT7          ((HWND) 7)

class CLocManager
{
  struct LocItem
  {
    HWND wnd;
    LocItem* parent[4];
    int pside[4];
    int offset[4];
    bool isratio[4];
    float oratio[4];
    bool vis;
    CRect rc;
  };
  void SetRectSide (CRect& rc, int side, int val);
  int GetRectSide (CRect& rc, int side);
  int GetRectDim (CRect& rc, int side, float r);
  float GetRectRatio (CRect& rc, int side, int o);
  LocItem** items;
  LocItem** temp;
  void DFS (LocItem* item);
  int numItems;
  int tempItems;
  int maxItems;
  LocItem* GetByHandle (HWND hwnd);
  HWND wnd;
public:
  CLocManager (CWnd* window = NULL);
  CLocManager (HWND window);
  ~CLocManager ();
  void SetWindow (CWnd* window);
  void SetWindow (HWND window);
  void SetItemRelative (int id, int side, int rel, int relto, int mode = VALUE);
  void SetItemAbsolute (int id, int side, int mode = VALUE);
  void SetItemRelative (CWnd* pwnd, int side, CWnd* rel, int relto, int mode = VALUE);
  void SetItemAbsolute (CWnd* pwnd, int side, int mode = VALUE);
  void SetItemRelative (HWND hwnd, int side, HWND rel, int relto, int mode = VALUE);
  void SetItemAbsolute (HWND hwnd, int side, int mode = VALUE);
  void Start ();
  bool Update ();
  bool Ready ()
  {
    return wnd != NULL;
  }
  void SetCenterPoint (HWND pt, int of);
  void SetCenterPoint (HWND pt, HWND of);
  void SetCenterPoint (HWND pt, CWnd* of);
};

class CCtrlGroup
{
  CWnd** items;
  int numItems;
  int maxItems;
public:
  CCtrlGroup ();
  ~CCtrlGroup ();
  void AddItem (CWnd* item);
  void ShowWindow (int nCmdShow);
  void EnableWindow (BOOL bEnable);
};

class CColManager
{
  CListCtrl* list;
  int* cols;
  int numCols;
  int maxCols;
  int wd;
public:
  CColManager (CListCtrl* listctrl = NULL);
  ~CColManager ();
  void SetListCtrl (CListCtrl* listctrl);
  void Read ();
  void Write ();
};

#endif // __LOC_MANAGER_H__
