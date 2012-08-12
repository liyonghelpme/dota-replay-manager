// GetDataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "GetDataDlg.h"
#include "utils.h"
#include "dota.h"
#include ".\getdatadlg.h"

// CGetDataDlg dialog

IMPLEMENT_DYNAMIC(CGetDataDlg, CDialog)
CGetDataDlg::CGetDataDlg(bool fixed, int version, char const* path, CWnd* pParent /*=NULL*/)
	: CDialog(CGetDataDlg::IDD, pParent)
{
  ver = version;
  fix = fixed;
  pth = path;
}

CGetDataDlg::~CGetDataDlg()
{
}

void CGetDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGetDataDlg, CDialog)
  ON_BN_CLICKED(IDC_MAPBROWSE, OnBnClickedMapbrowse)
  ON_BN_CLICKED(IDC_CLOSEST, OnBnClickedClosest)
  ON_BN_CLICKED(IDC_COPYVERSION, OnBnClickedCopyversion)
  ON_BN_CLICKED(IDC_LOADMAP, OnBnClickedLoadmap)
  ON_BN_CLICKED(IDC_LOADTHISMAP, OnBnClickedLoadthismap)
  ON_EN_CHANGE(IDC_VERSION, OnEnChangeVersion)
  ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CGetDataDlg message handlers

BOOL CGetDataDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  if (fix)
    GetDlgItem (IDC_VERSION)->EnableWindow (FALSE);
  else
  {
    SetDlgItemText (IDC_VERSION_TIP, "Create data for");
    CheckDlgButton (IDC_SAVEDATA, BST_CHECKED);
    GetDlgItem (IDC_SAVEDATA)->EnableWindow (FALSE);
  }
  if (ver)
    SetDlgItemText (IDC_VERSION, formatVersion (ver));
  if (pth)
    SetDlgItemText (IDC_MAPTOLOAD, pth);
  else
    GetDlgItem (IDC_LOADMAP)->EnableWindow (FALSE);

  if (getNumVersions () == 0)
  {
    GetDlgItem (IDC_CLOSEST)->EnableWindow (FALSE);
    GetDlgItem (IDC_COPYVER)->EnableWindow (FALSE);
    GetDlgItem (IDC_COPYVERSION)->EnableWindow (FALSE);
  }
  else
  {
    CComboBox* vlist = (CComboBox*) GetDlgItem (IDC_COPYVER);
    int index = 0;
    int best = getClosestVersion (ver);
    sortVersions ();
    for (int i = 0; i < getNumVersions (); i++)
    {
      if (best == getVersion (i))
        index = i;
      vlist->AddString (formatVersion (getVersion (i)));
    }
    vlist->SetCurSel (index);
    SetDlgItemText (IDC_CLOSEST, mprintf ("Copy data from closest version (%s)", formatVersion (best)));
  }

  return TRUE;
}

void CGetDataDlg::OnBnClickedMapbrowse()
{
  char buf[1024];
  GetDlgItemText (IDC_MAPTOLOAD, buf, sizeof buf);
  CFileDialog dlg (TRUE, ".w3x", buf, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
    "DotA Maps|DotA?Allstars?v6.*.w3x;DotA?v6.*.w3x|All Warcraft Maps|*.w3m;*.w3x|All Files|*.*||",
    this, sizeof (OPENFILENAME));
  if (dlg.DoModal () == IDOK)
    SetDlgItemText (IDC_MAPTOLOAD, dlg.GetPathName ());
}

void CGetDataDlg::OnBnClickedClosest()
{
  if (fix)
    GetDlgItem (IDC_SAVECHOICE)->EnableWindow (TRUE);
  char txt[256];
  GetDlgItemText (IDC_VERSION, txt, sizeof txt);
  if (parseVersion (txt)) GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CGetDataDlg::OnBnClickedCopyversion()
{
  if (fix)
    GetDlgItem (IDC_SAVECHOICE)->EnableWindow (FALSE);
  char txt[256];
  GetDlgItemText (IDC_VERSION, txt, sizeof txt);
  if (parseVersion (txt)) GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CGetDataDlg::OnBnClickedLoadmap()
{
  if (fix)
    GetDlgItem (IDC_SAVECHOICE)->EnableWindow (TRUE);
  char txt[256];
  GetDlgItemText (IDC_VERSION, txt, sizeof txt);
  if (parseVersion (txt)) GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CGetDataDlg::OnBnClickedLoadthismap()
{
  if (fix)
    GetDlgItem (IDC_SAVECHOICE)->EnableWindow (FALSE);
  char txt[256];
  GetDlgItemText (IDC_VERSION, txt, sizeof txt);
  if (parseVersion (txt)) GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CGetDataDlg::OnEnChangeVersion()
{
  char txt[256];
  GetDlgItemText (IDC_VERSION, txt, sizeof txt);
  int ver = parseVersion (txt);
  if (ver == 0)
    GetDlgItem (IDOK)->EnableWindow (FALSE);
  else
  {
    if (IsDlgButtonChecked (IDC_CLOSEST) ||
        IsDlgButtonChecked (IDC_COPYVERSION) ||
        IsDlgButtonChecked (IDC_LOADMAP) ||
        IsDlgButtonChecked (IDC_LOADTHISMAP))
      GetDlgItem (IDOK)->EnableWindow (TRUE);
    int best = getClosestVersion (ver);
    if (best)
      SetDlgItemText (IDC_CLOSEST, mprintf ("Copy data from closest version (%s)", formatVersion (best)));
  }
}

void CGetDataDlg::OnBnClickedOk()
{
  saved = IsDlgButtonChecked (IDC_SAVEDATA) != 0;
  alwaysdo = IsDlgButtonChecked (IDC_SAVECHOICE) != 0;
  GetDlgItemText (IDC_VERSION, path, sizeof path);
  ver = parseVersion (path);
  if (IsDlgButtonChecked (IDC_CLOSEST))
  {
    mode = Closest;
    copyfrom = getClosestVersion (ver);
  }
  else if (IsDlgButtonChecked (IDC_COPYVERSION))
  {
    mode = Version;
    GetDlgItemText (IDC_COPYVER, path, sizeof path);
    copyfrom = parseVersion (path);
  }
  else if (IsDlgButtonChecked (IDC_LOADMAP))
    mode = Used;
  else //if (IsDlgButtonChecked (IDC_LOADTHISMAP))
  {
    mode = Path;
    GetDlgItemText (IDC_MAPTOLOAD, path, sizeof path);
  }
  OnOK();
}
