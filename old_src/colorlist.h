#ifndef __COLOR_LIST_H__
#define __COLOR_LIST_H__

class CColorRect : public CWnd
{
	DECLARE_DYNAMIC(CColorRect)
  COLORREF clr;

public:
	CColorRect (COLORREF c, CRect const& rc, CWnd* parent);

  void setColor (COLORREF c)
  {
    clr = c;
    InvalidateRect (NULL);
  }
  COLORREF getColor () const
  {
    return clr;
  }

protected:
  afx_msg BOOL OnEraseBkgnd (CDC* pDC);
	DECLARE_MESSAGE_MAP()
};

class CColorList : public CListCtrl
{
  void DrawItemText (CDC* pDC, char const* text, int length, RECT* rect, DWORD format);
  virtual INT_PTR OnToolHitTest (CPoint point, TOOLINFO* pTI) const;
  CDC* pSavedDC;
public:
  CColorList ()
  {
    simple = false;
    pSavedDC = NULL;
  }
  virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
  bool simple;
};

class W3GReplay;
class CBarList : public CColorList
{
public:
  int mx;
  int start;
  int end;
  W3GReplay* w3g;
  int player;
  CBarList ()
  {
    w3g = NULL;
  }
  virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
  void setReplay (W3GReplay* rep)
  {
    w3g = rep;
  }
  void setPlayer (int id)
  {
    player = id;
    InvalidateRect (NULL, TRUE);
  }
};

class CColorBox : public CComboBox
{
  CImageList* list;
  struct BoxItem
  {
    COLORREF color;
    DWORD data;
    int image;
    wchar_t text[256];
  };
  BoxItem items[256];
  int numItems;
public:
  CColorBox ()
  {
    list = NULL;
    numItems = 0;
  }
  void SetImageList (CImageList* theList)
  {
    list = theList;
  }
  void InsertItem (char const* text, int image = 0, COLORREF clr = 0xFFFFFF, DWORD data = 0);
  void InsertItem (wchar_t const* text, int image = 0, COLORREF clr = 0xFFFFFF, DWORD data = 0);
  void Reset ();
  DWORD GetItemDataEx (int item)
  {
    return items[GetItemData (item)].data;
  }
  virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
};

class CImageBox : public CWnd
{
  CBitmap* bmp;
public:
  CImageBox (CRect const& rc, CBitmap* bitmap, CWnd* parent);
  void setImage (CBitmap* bitmap)
  {
    bmp = bitmap;
    InvalidateRect (NULL);
  }
protected:
  afx_msg void OnPaint ();
  DECLARE_MESSAGE_MAP()
};

#endif // __COLOR_LIST_H__
