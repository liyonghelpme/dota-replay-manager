// NewPresentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "NewPresentDlg.h"
#include "sparser.h"
#include "registry.h"

// CNewPresentDlg dialog

IMPLEMENT_DYNAMIC(CNewPresentDlg, CDialog)

CNewPresentDlg::CNewPresentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewPresentDlg::IDD, pParent)
{
  w3g = NULL;
  Create (IDD, pParent);
}

CNewPresentDlg::~CNewPresentDlg()
{
}

void CNewPresentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CNewPresentDlg, CDialog)
  ON_BN_CLICKED(IDC_UPDATE, &CNewPresentDlg::OnBnClickedUpdate)
  ON_WM_DESTROY()
  ON_WM_SIZE()
  ON_BN_CLICKED(IDC_COPY, &CNewPresentDlg::OnBnClickedCopy)
  ON_CBN_SELCHANGE(IDC_PRESET, &CNewPresentDlg::OnCbnSelchangePreset)
  ON_BN_CLICKED(IDC_NEWPR, &CNewPresentDlg::OnBnClickedNewpr)
  ON_BN_CLICKED(IDC_SAVEPR, &CNewPresentDlg::OnBnClickedSavepr)
  ON_BN_CLICKED(IDC_DELPR, &CNewPresentDlg::OnBnClickedDelpr)
END_MESSAGE_MAP()

void CNewPresentDlg::setReplay (W3GReplay* replay)
{
  w3g = replay;
}

static char defPresets[2][256] = {
  "dota\\ppreset1.txt",
  "dota\\ppreset2.txt"
};

// CNewPresentDlg message handlers
BOOL CNewPresentDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  presets.Attach (GetDlgItem (IDC_PRESET)->m_hWnd);
  presets.SetItemData (presets.AddString ("Full info (PlayDota BB-code)"), 0);
  presets.SetItemData (presets.AddString ("Compact (PlayDota BB-code)"), 1);
  int numExtra = reg.readInt ("numExtPresets", 0);
  for (int i = 1; i <= numExtra; i++)
    presets.SetItemData (presets.AddString (mprintf ("Preset %d", i)), i + 1);
  presets.SetCurSel (0);
  OnCbnSelchangePreset ();

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_PRESET, SIDE_RIGHT);
  loc.SetItemRelative (IDC_NEWPR, SIDE_LEFT, IDC_NEWPR, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SAVEPR, SIDE_LEFT, IDC_SAVEPR, SIDE_RIGHT);
  loc.SetItemRelative (IDC_DELPR, SIDE_LEFT, IDC_DELPR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SCRIPTBOX, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SCRIPTBOX, SIDE_BOTTOM, PERCENT);
  loc.SetItemRelative (IDC_UPDATE, SIDE_TOP, IDC_SCRIPTBOX, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_UPDATE, SIDE_RIGHT, PERCENT);
  loc.SetItemRelative (IDC_COPY, SIDE_TOP, IDC_SCRIPTBOX, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_COPY, SIDE_LEFT, IDC_UPDATE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_COPY, SIDE_RIGHT);
  loc.SetItemRelative (IDC_OUTPUT, SIDE_TOP, IDC_UPDATE, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_OUTPUT, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_OUTPUT, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}
void CNewPresentDlg::OnDestroy ()
{
  presets.Detach ();
}
void CNewPresentDlg::OnSize (UINT nType, int cx, int cy)
{
  if (nType != SIZE_MINIMIZED)
    loc.Update ();
}

void CNewPresentDlg::OnBnClickedUpdate()
{
  String text;
  CWnd* box = GetDlgItem (IDC_SCRIPTBOX);
  text.resize (box->GetWindowTextLength ());
  box->GetWindowText (text.getBuffer (), text.getBufferSize ());
  text.update ();
  ScriptParser parser (w3g);
  SetDlgItemText (IDC_OUTPUT, parser.parse (text));
}

void CNewPresentDlg::OnBnClickedCopy()
{
  CString buf;
  GetDlgItemText (IDC_OUTPUT, buf);

  if (!OpenClipboard ())
    return;
  EmptyClipboard ();

  int count = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (LPCSTR) buf, -1, NULL, 0);
  wchar_t* msg = new wchar_t[count + 5];
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (LPCSTR) buf, -1, msg, count + 5);
  int sz = 0;
  for (; msg[sz]; sz++)
    ;
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    return;
  }

  LPTSTR copy = (LPTSTR) GlobalLock (handle);
  memcpy (copy, msg, sz * 2 + 2);
  GlobalUnlock (handle);
  delete[] msg;

  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
}
void CNewPresentDlg::loadFormat (MPQFILE file)
{
  String text = "";
  if (file)
  {
    char buf[4096];
    while (MPQFileGets (file, sizeof buf, buf))
    {
      int len = (int) strlen (buf);
      while (len && (buf[len - 1] == '\r' || buf[len - 1] == '\n'))
        len--;
      buf[len++] = '\r';
      buf[len++] = '\n';
      buf[len] = 0;
      text += buf;
    }
  }
  SetDlgItemText (IDC_SCRIPTBOX, text);
}
void CNewPresentDlg::OnCbnSelchangePreset()
{
  int cur = presets.GetCurSel ();
  int id = presets.GetItemData (cur);
  int numdef = sizeof defPresets / sizeof defPresets[0];
  if (id < numdef)
  {
    MPQFILE file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, defPresets[id], MPQFILE_READ);
    loadFormat (file);
    if (file)
      MPQCloseFile (file);
    GetDlgItem (IDC_SAVEPR)->EnableWindow (FALSE);
    GetDlgItem (IDC_DELPR)->EnableWindow (FALSE);
  }
  else
  {
    id = (id - numdef) + 1;
    MPQFILE file = MPQOpenFSys (mprintf ("%spreset%d.txt", reg.getPath (), id), MPQFILE_READ);
    loadFormat (file);
    if (file)
      MPQCloseFile (file);
    GetDlgItem (IDC_SAVEPR)->EnableWindow (TRUE);
    GetDlgItem (IDC_DELPR)->EnableWindow (TRUE);
  }
}

void CNewPresentDlg::OnBnClickedNewpr()
{
  int numExtra = reg.readInt ("numExtPresets", 0);
  reg.writeInt ("numExtPresets", ++numExtra);
  int pos = presets.AddString (mprintf ("Preset %d", numExtra));
  presets.SetItemData (pos, numExtra - 1 + (sizeof defPresets / sizeof defPresets[0]));
  FILE* f = fopen (mprintf ("%spreset%d.txt", reg.getPath (), numExtra), "wt");
  fclose (f);
  presets.SetCurSel (pos);
  OnCbnSelchangePreset ();
}

void CNewPresentDlg::OnBnClickedSavepr()
{
  int cur = presets.GetCurSel ();
  int id = presets.GetItemData (cur);
  int numdef = sizeof defPresets / sizeof defPresets[0];
  if (id >= numdef)
  {
    id = (id - numdef) + 1;
    String text;
    CWnd* box = GetDlgItem (IDC_SCRIPTBOX);
    text.resize (box->GetWindowTextLength ());
    box->GetWindowText (text.getBuffer (), text.getBufferSize ());
    text.update ();
    FILE* f = fopen (mprintf ("%spreset%d.txt", reg.getPath (), id), "wb");
    fwrite (text.c_str (), 1, text.length (), f);
    fclose (f);
  }
}

void CNewPresentDlg::OnBnClickedDelpr()
{
  int cur = presets.GetCurSel ();
  int id = presets.GetItemData (cur);
  int numdef = sizeof defPresets / sizeof defPresets[0];
  if (id >= numdef)
  {
    id = (id - numdef) + 1;
    if (MessageBox (mprintf ("Are you sure you want to delete Preset %d?", id),
      "Warning", MB_YESNO | MB_ICONWARNING) != IDYES)
      return;
    DeleteFile (mprintf ("%spreset%d.txt", reg.getPath (), id));
    int numExtra = reg.readInt ("numExtPresets", 0);
    reg.writeInt ("numExtPresets", --numExtra);
    while (id <= numExtra)
    {
      MoveFile (mprintf ("%spreset%d.txt", reg.getPath (), id + 1),
                mprintf ("%spreset%d.txt", reg.getPath (), id));
      id++;
    }
    presets.DeleteString (presets.GetCount () - 1);
    if (presets.GetCurSel () < 0)
      presets.SetCurSel (presets.GetCount () - 1);
    OnCbnSelchangePreset ();
  }
}
