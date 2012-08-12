// SearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "SearchDlg.h"
#include "ilib.h"
#include "dota.h"
#include ".\searchdlg.h"
#include "gamecache.h"
#include "utils.h"

// CSearchDlg dialog

IMPLEMENT_DYNAMIC(CSearchDlg, CDialog)
CSearchDlg::CSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSearchDlg::IDD, pParent)
{
}

CSearchDlg::~CSearchDlg()
{
}

void CSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSearchDlg, CDialog)
  ON_WM_DESTROY()
  ON_CBN_SELCHANGE(IDC_PHERO1, OnCbnSelchangePhero1)
  ON_CBN_SELCHANGE(IDC_PHERO2, OnCbnSelchangePhero2)
  ON_CBN_SELCHANGE(IDC_PHERO3, OnCbnSelchangePhero3)
  ON_CBN_SELCHANGE(IDC_PHERO4, OnCbnSelchangePhero4)
  ON_CBN_SELCHANGE(IDC_PHERO5, OnCbnSelchangePhero5)
END_MESSAGE_MAP()


// CSearchDlg message handlers

BOOL CSearchDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  mode[0].Attach (GetDlgItem (IDC_PMODE1)->m_hWnd);
  mode[1].Attach (GetDlgItem (IDC_PMODE2)->m_hWnd);
  mode[2].Attach (GetDlgItem (IDC_PMODE3)->m_hWnd);
  mode[3].Attach (GetDlgItem (IDC_PMODE4)->m_hWnd);
  mode[4].Attach (GetDlgItem (IDC_PMODE5)->m_hWnd);
  mode[BOX_FILE].Attach (GetDlgItem (IDC_FFUNC)->m_hWnd);
  mode[BOX_NAME].Attach (GetDlgItem (IDC_NFUNC)->m_hWnd);
  mode[BOX_MODE].Attach (GetDlgItem (IDC_MFUNC)->m_hWnd);
  hero[0].Attach (GetDlgItem (IDC_PHERO1)->m_hWnd);
  hero[1].Attach (GetDlgItem (IDC_PHERO2)->m_hWnd);
  hero[2].Attach (GetDlgItem (IDC_PHERO3)->m_hWnd);
  hero[3].Attach (GetDlgItem (IDC_PHERO4)->m_hWnd);
  hero[4].Attach (GetDlgItem (IDC_PHERO5)->m_hWnd);
  dfrom.Attach (GetDlgItem (IDC_SAVEDA)->m_hWnd);
  dto.Attach (GetDlgItem (IDC_SAVEDB)->m_hWnd);
  lfrom.Attach (GetDlgItem (IDC_LENGTHA)->m_hWnd);
  lto.Attach (GetDlgItem (IDC_LENGTHB)->m_hWnd);
  for (int i = 0; i < 8; i++)
    mode[i].SetCurSel (0);
  dfrom.SetTime (&CTime (1971, 1, 1, 0, 0, 0));
  dto.SetTime (&CTime::GetCurrentTime ());
  lfrom.SetTime (&CTime (2000, 1, 1, 0, 0, 0));
  lto.SetTime (&CTime (2000, 1, 1, 23, 59, 59));

  for (int i = 0; i < 5; i++)
  {
    hero[i].SetImageList (getImageList ());
    int cur = 0;
    int cnt = getNumHeroes ();
    hero[i].InsertItem ("Any hero", getImageIndex ("Unknown"));
    for (int j = 0; j < getNumTaverns (); j++)
    {
      DotaTavern* tavern = getTavern (j);
      hero[i].InsertItem (tavern->name, getImageIndex (tavern->side == 0 ? "RedBullet" : "GreenBullet"), 0, -1);
      for (int cur = 0; cur < getNumHeroes (); cur++)
      {
        DotaHero* h = getHero (cur);
        if (h->tavern == j)
          hero[i].InsertItem (h->name, getImageIndex (h->imgTag), RGB (255, 255, 255), h->point);
      }
    }
    selhero[i] = 0;
    hero[i].SetCurSel (0);
  }

  return TRUE;
}

void CSearchDlg::OnDestroy ()
{
  for (int i = 0; i < 8; i++)
    mode[i].Detach ();
  for (int i = 0; i < 5; i++)
    hero[i].Detach ();
  dfrom.Detach ();
  dto.Detach ();
  lfrom.Detach ();
  lto.Detach ();
}

void CSearchDlg::OnOK ()
{
  ss.ffunc = mode[BOX_FILE].GetCurSel ();
  GetDlgItemText (IDC_FTEXT, ss.ftext, 256);
  ss.nfunc = mode[BOX_NAME].GetCurSel ();
  GetDlgItemTextW (m_hWnd, IDC_NTEXT, ss.ntext, 256);
  ss.mfunc = mode[BOX_MODE].GetCurSel ();
  GetDlgItemText (IDC_MTEXT, ss.mtext, 256);
  for (int i = 0; i < 5; i++)
  {
    ss.pmode[i] = mode[i].GetCurSel ();
    ss.phero[i] = hero[i].GetItemDataEx (selhero[i]);
  }
  GetDlgItemTextW (m_hWnd, IDC_PNAME1, ss.pname[0], 256);
  GetDlgItemTextW (m_hWnd, IDC_PNAME2, ss.pname[1], 256);
  GetDlgItemTextW (m_hWnd, IDC_PNAME3, ss.pname[2], 256);
  GetDlgItemTextW (m_hWnd, IDC_PNAME4, ss.pname[3], 256);
  GetDlgItemTextW (m_hWnd, IDC_PNAME5, ss.pname[4], 256);

  char buf[256];
  BOOL trans;

  GetDlgItemText (IDC_COUNTA, buf, 256);
  if (buf[0] == 0)
    ss.numa = 0;
  else
  {
    ss.numa = GetDlgItemInt (IDC_COUNTA, &trans, FALSE);
    if (trans == FALSE)
    {
      MessageBox ("Invalid minimal player number!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_COUNTA)->SetFocus ();
      return;
    }
  }

  GetDlgItemText (IDC_COUNTB, buf, 256);
  if (buf[0] == 0)
    ss.numb = 16;
  else
  {
    ss.numb = GetDlgItemInt (IDC_COUNTB, &trans, FALSE);
    if (trans == FALSE)
    {
      MessageBox ("Invalid maximal player number!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_COUNTB)->SetFocus ();
      return;
    }
  }

  GetDlgItemText (IDC_VERSIONA, buf, 256);
  if (buf[0] == 0)
    ss.vera = 0;
  else
  {
    ss.vera = makeVersion (buf);
    if (ss.vera == 0)
    {
      MessageBox ("Invalid minimal map version!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_VERSIONA)->SetFocus ();
      return;
    }
  }

  GetDlgItemText (IDC_VERSIONB, buf, 256);
  if (buf[0] == 0)
    ss.verb = GC_BIGINT;
  else
  {
    ss.verb = makeVersion (buf);
    if (ss.verb == 0)
    {
      MessageBox ("Invalid maximal map version!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_VERSIONB)->SetFocus ();
      return;
    }
  }

  GetDlgItemText (IDC_PATCHA, buf, 256);
  if (buf[0] == 0)
    ss.pata = 0;
  else
  {
    ss.pata = makeVersion (buf, true);
    if (ss.pata == 0)
    {
      MessageBox ("Invalid minimal patch version!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_PATCHA)->SetFocus ();
      return;
    }
  }

  GetDlgItemText (IDC_PATCHB, buf, 256);
  if (buf[0] == 0)
    ss.patb = GC_BIGINT;
  else
  {
    ss.patb = makeVersion (buf, true);
    if (ss.patb == 0)
    {
      MessageBox ("Invalid maximal patch version!", "Error", MB_OK | MB_ICONHAND);
      GetDlgItem (IDC_PATCHB)->SetFocus ();
      return;
    }
  }

  CTime ts;

  lfrom.GetTime (ts);
  ss.lena = ((ts.GetHour () * 60 + ts.GetMinute ()) * 60 + ts.GetSecond ()) * 1000;
  lto.GetTime (ts);
  ss.lenb = ((ts.GetHour () * 60 + ts.GetMinute ()) * 60 + ts.GetSecond ()) * 1000;
  dfrom.GetTime (ts);
  ss.sava = CTime (ts.GetYear (), ts.GetMonth (), ts.GetDay (), 0, 0, 0).GetTime ();
  dto.GetTime (ts);
  ss.savb = CTime (ts.GetYear (), ts.GetMonth (), ts.GetDay (), 23, 59, 59).GetTime ();

  CDialog::OnOK ();
}

void CSearchDlg::OnCbnSelchangePhero1()
{
  int cur = hero[0].GetCurSel ();
  if (hero[0].GetItemDataEx (cur) == -1)
    hero[0].SetCurSel (selhero[0]);
  else
    selhero[0] = cur;
}

void CSearchDlg::OnCbnSelchangePhero2()
{
  int cur = hero[1].GetCurSel ();
  if (hero[1].GetItemDataEx (cur) == -1)
    hero[1].SetCurSel (selhero[1]);
  else
    selhero[1] = cur;
}

void CSearchDlg::OnCbnSelchangePhero3()
{
  int cur = hero[2].GetCurSel ();
  if (hero[2].GetItemDataEx (cur) == -1)
    hero[2].SetCurSel (selhero[2]);
  else
    selhero[2] = cur;
}

void CSearchDlg::OnCbnSelchangePhero4()
{
  int cur = hero[3].GetCurSel ();
  if (hero[3].GetItemDataEx (cur) == -1)
    hero[3].SetCurSel (selhero[3]);
  else
    selhero[3] = cur;
}

void CSearchDlg::OnCbnSelchangePhero5()
{
  int cur = hero[4].GetCurSel ();
  if (hero[4].GetItemDataEx (cur) == -1)
    hero[4].SetCurSel (selhero[4]);
  else
    selhero[4] = cur;
}


bool str_matches (int mode, char const* s, char const* w)
{
  switch (mode)
  {
  case 0:
    return stristr (s, w) != NULL;
  case 1:
    return strnicmp (s, w, strlen (w)) == 0;
  case 2:
    return stricmp (s, w) == 0;
  default:
    return false;
  }
}
bool str_matches (int mode, wchar_t const* s, wchar_t const* w)
{
  switch (mode)
  {
  case 0:
    return wstristr (s, w) != NULL;
  case 1:
    return wcsnicmp (s, w, wcslen (w)) == 0;
  case 2:
    return wcsicmp (s, w) == 0;
  default:
    return false;
  }
}

bool matches (char const* file, SearchStruct* ss)
{
  GameCache* game = &gcache[getGameInfo (file, GC_FLAGS)];
  char ftitle[256];
  _splitpath (file, NULL, NULL, ftitle, NULL);
  if (!str_matches (ss->ffunc, ftitle, ss->ftext))
    return false;
  if (!str_matches (ss->nfunc, game->name, ss->ntext))
    return false;
  if (!matchMode (game->mode, ss->mtext, ss->mfunc))
    return false;
  if (game->count < ss->numa || game->count > ss->numb)
    return false;
  if (game->map < ss->vera || game->map > ss->verb)
    return false;
  if (game->patch < ss->pata || game->patch > ss->patb)
    return false;
  if (game->length < ss->lena || game->length > ss->lenb)
    return false;
  if (game->mod < ss->sava || game->mod > ss->savb)
    return false;
  for (int i = 0; i < 5; i++)
  {
    bool found = false;
    for (int j = 0; j < game->count && !found; j++)
    {
      if (str_matches (ss->pmode[i], game->pname[j], ss->pname[i]) &&
          (ss->phero[i] == 0 || ss->phero[i] == game->phero[j]))
        found = true;
    }
    if (!found)
      return false;
  }
  return true;
}
