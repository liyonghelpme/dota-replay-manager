// TagEditorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "TagEditorDlg.h"
#include "dota.h"
#include "ilib.h"

// CTagEditorDlg dialog

IMPLEMENT_DYNAMIC(CTagEditorDlg, CDialog)

CTagEditorDlg::CTagEditorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTagEditorDlg::IDD, pParent)
{

}

CTagEditorDlg::~CTagEditorDlg()
{
}

void CTagEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTagEditorDlg, CDialog)
  ON_WM_DESTROY()
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CTagEditorDlg message handlers
BOOL CTagEditorDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  tags.Attach (GetDlgItem (IDC_TAGLIST)->m_hWnd);
  tags.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );

  tags.InsertColumn (0, "");
  tags.InsertColumn (1, "");
  tags.InsertColumn (2, "");
  tags.InsertColumn (3, "");
  tags.SetImageList (getImageList (), LVSIL_SMALL);
  CRect rc;
  tags.GetClientRect (rc);
  tags.SetColumnWidth (0, rc.right - 180);
  tags.SetColumnWidth (1, 60);
  tags.SetColumnWidth (1, 120);
  tags.SetColumnWidth (1, 180);

  tags.simple = false;
  tags.SetItemData (tags.InsertItem (0, "Heroes"), 0x808080);
  for (int i = 0; i < getNumHeroes (); i++)
  {
    DotaHero* hero = getHero (i);
    if (hero->numIds)
    {
      int i = tags.InsertItem (tags.GetItemCount (), hero->name, getImageIndex (hero->imgTag));
      //tags.SetItemText (i, 1, 
    }
  }

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_TAGLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_TAGLIST, SIDE_BOTTOM);
  loc.SetItemRelative (IDOK, SIDE_TOP, IDOK, SIDE_BOTTOM);
  loc.SetItemRelative (IDCANCEL, SIDE_TOP, IDCANCEL, SIDE_BOTTOM);
  loc.SetItemRelative (IDCANCEL, SIDE_LEFT, IDCANCEL, SIDE_RIGHT);
  loc.Start ();

  return TRUE;
}
void CTagEditorDlg::OnDestroy ()
{
  tags.Detach ();
  CRect rc;
  tags.GetClientRect (rc);
  tags.SetColumnWidth (0, rc.right - 180);
  tags.SetColumnWidth (1, 60);
  tags.SetColumnWidth (1, 120);
  tags.SetColumnWidth (1, 180);
}
void CTagEditorDlg::OnSize (UINT nType, int cx, int cy)
{
  if (nType != SIZE_MINIMIZED)
    loc.Update ();
}
