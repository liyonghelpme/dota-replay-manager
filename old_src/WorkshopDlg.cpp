// WorkshopDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "WorkshopDlg.h"
#include "replay.h"

// CWorkshopDlg dialog

IMPLEMENT_DYNAMIC(CWorkshopDlg, CDialog)
CWorkshopDlg::CWorkshopDlg(W3GReplay* w3g, CWnd* pParent /*=NULL*/)
	: CDialog(CWorkshopDlg::IDD, pParent)
{
  replay = w3g;
}

CWorkshopDlg::~CWorkshopDlg()
{
}

void CWorkshopDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWorkshopDlg, CDialog)
  ON_WM_DESTROY()
  ON_WM_SIZE()
  ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

// CWorkshopDlg message handlers

BOOL CWorkshopDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  list.Attach (GetDlgItem (IDC_CHATLIST)->m_hWnd);

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_CHATLIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_CHATLIST, SIDE_BOTTOM);
  loc.SetItemRelative (IDOK, SIDE_TOP, IDOK, SIDE_BOTTOM);
  loc.SetItemRelative (IDCANCEL, SIDE_LEFT, IDCANCEL, SIDE_RIGHT);
  loc.SetItemRelative (IDCANCEL, SIDE_TOP, IDCANCEL, SIDE_BOTTOM);
  loc.Start ();

  for (int i = 0; i < replay->chat.getSize (); i++)
  {
    W3GMessage* msg = &replay->chat[i];
    if (msg->time > 15000) break;
    if (msg->index >= 0)
    {
      if (msg->mode == CHAT_ALL)
        list.SetItemData (list.AddString (mprintf ("%s [All] %s: %s", format_time (msg->time),
        replay->players[msg->id].name, msg->text)), msg->index);
      else if (msg->mode == CHAT_ALLIES)
        list.SetItemData (list.AddString (mprintf ("%s [Allies] %s: %s", format_time (msg->time),
        replay->players[msg->id].name, msg->text)), msg->index);
      else if (msg->mode == CHAT_OBSERVERS)
        list.SetItemData (list.AddString (mprintf ("%s [Observers] %s: %s", format_time (msg->time),
        replay->players[msg->id].name, msg->text)), msg->index);
      else if (msg->mode == CHAT_PRIVATE)
        list.SetItemData (list.AddString (mprintf ("%s [Private] %s: %s", format_time (msg->time),
        replay->players[msg->id].name, msg->text)), msg->index);
    }
  }

  return true;
}
void CWorkshopDlg::OnDestroy ()
{
  list.Detach ();
}

void CWorkshopDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}
void CWorkshopDlg::OnGetMinMaxInfo (MINMAXINFO* lpMMI)
{
  lpMMI->ptMinTrackSize.x = 500;
  lpMMI->ptMinTrackSize.y = 300;
}

extern char replayPath[256];
void CWorkshopDlg::OnOK ()
{
  CFileDialog dlg (FALSE, ".w3g", replay->filename[0] ? mprintf ("%s_cut.w3g", replay->filename) : NULL,
    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, "Warcraft III Replays (*.w3g)|*.w3g||", this);
  dlg.m_ofn.lpstrInitialDir = replayPath;
  if (dlg.DoModal () == IDOK)
  {
    FILE* f = fopen (dlg.GetPathName (), "wb");
    if (f == NULL)
      MessageBox ("Failed to open file!", "Error", MB_OK | MB_ICONHAND);
    else
    {
      int count = list.GetSelCount ();
      int* remove = new int[count + 1];
      list.GetSelItems (count, remove);
      replay->cutchat (remove, count, IsDlgButtonChecked (IDC_ALLMSG) != 0,
                                      IsDlgButtonChecked (IDC_OBSMSG) != 0,
                                      IsDlgButtonChecked (IDC_ALLPING) != 0,
                                      IsDlgButtonChecked (IDC_OBSPING) != 0);
      replay->saveas (f);
      fclose (f);
      delete[] remove;
      CDialog::OnOK ();
    }
  }
}
