#pragma once

#include "glib.h"

// CTimePicture

class W3GReplay;

class CTimePicture : public CWnd
{
	DECLARE_DYNAMIC(CTimePicture)

  OpenGL* gl;
  unsigned long time;
  unsigned int tex;

  W3GReplay* w3g;
  int imgw;
  int imgh;

public:
	CTimePicture (CRect const& rc, CWnd* parent);
	virtual ~CTimePicture();

  int mapX (float x);
  int mapY (float y);
  int unmapX (int x);
  int unmapY (int y);

  void setReplay (W3GReplay* w3g);
  void setTime (unsigned long theTime);

  HCURSOR cursor;

protected:
  afx_msg int OnCreate (LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint ();
  afx_msg void OnDestroy ();
  afx_msg void OnMouseMove (UINT nFlags, CPoint point);
  afx_msg BOOL OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message);
  afx_msg void OnSize (UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
};


