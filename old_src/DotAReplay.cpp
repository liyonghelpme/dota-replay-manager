// DotAReplay.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "DotAReplayDlg.h"
#include "ilib.h"
#include "image.h"
#include "dota.h"
#include "registry.h"
#include "sparser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CDotAReplayApp

BEGIN_MESSAGE_MAP(CDotAReplayApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


CDotAReplayApp::CDotAReplayApp()
{
}

CDotAReplayApp theApp;

// CDotAReplayApp initialization

extern bool __fail;
//ScriptEnv* scriptEnv = NULL;

BOOL CDotAReplayApp::InitInstance()
{
  HANDLE map = CreateFileMapping (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, "DotAReplay");
  void* mapview = NULL;
  if (map != INVALID_HANDLE_VALUE)
  {
    bool exists = (GetLastError () == ERROR_ALREADY_EXISTS) && (m_lpCmdLine[0]);
    mapview = MapViewOfFile (map, FILE_MAP_ALL_ACCESS, 0, 0, 4);
    if (mapview && exists)
    {
      HWND hWnd = * (HWND*) mapview;
      COPYDATASTRUCT cd;
      cd.dwData = 1257;
      cd.cbData = (int) strlen (m_lpCmdLine) + 1;
      cd.lpData = m_lpCmdLine;
      SendMessage (hWnd, WM_COPYDATA, NULL, (LPARAM) &cd);
      ShowWindow (hWnd, SW_SHOW);
      SetForegroundWindow (hWnd);
      UnmapViewOfFile (mapview);
      CloseHandle (map);
      return FALSE;
    }
  }

  CProgressDlg progress;
  progress.show ();
  progress.SetText ("Initializing application...", 0);
  this->progress = &progress;

  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof (InitCtrls);
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx (&InitCtrls);
  AfxOleInit ();
  AfxInitRichEdit2 ();
  MPQInit ();
	CWinApp::InitInstance();

  //scriptEnv = new ScriptEnv ();

  progress.SetText ("Opening resources...", 5);
  bool ok = false;
  char resPath[512];
  char patchPath[512];
  sprintf (resPath, "%sresources.mpq", reg.getPath ());
  sprintf (patchPath, "%spatch.mpq", reg.getPath ());
  FILE* tOpen = fopen (resPath, "rb");
  if (tOpen == NULL)
  {
    tOpen = fopen (patchPath, "rb");
    if (tOpen)
    {
      fclose (tOpen);
      MoveFile (patchPath, resPath);
      ok = true;
    }
    else
      ok = false;
  }
  else
    ok = true;
  if (ok)
    ok = (res = MPQOpen (resPath, MPQFILE_MODIFY)) != 0;
  if (ok)
  {
    tOpen = fopen (patchPath, "rb");
    if (tOpen)
    {
      fclose (tOpen);
      MPQARCHIVE patch = MPQOpen (patchPath, MPQFILE_READ);
      if (patch)
      {
        progress.SetText ("Applying patch...", 5);
        for (int i = 0; i < MPQGetHashSize (patch); i++)
        {
          char const* name = MPQGetFileName (patch, i);
          if (name)
          {
            MPQFILE srcFile = MPQOpenFile (patch, name, MPQFILE_READ);
            if (srcFile)
            {
              MPQFILE dstFile = MPQOpenFile (res, name, MPQFILE_REWRITE);
              if (dstFile)
              {
                static char buf[1024];
                while (int length = MPQFileRead (srcFile, sizeof buf, buf))
                  MPQFileWrite (dstFile, length, buf);
                MPQCloseFile (dstFile);
              }
              MPQCloseFile (srcFile);
            }
          }
        }
        MPQClose (patch);
        DeleteFile (patchPath);
      }
    }

    progress.SetText ("Loading common data...", 15);
    ok = loadCommonData ();
  }
  if (ok)
  {
    progress.SetText ("Loading images...", 20);
    ok = loadImages ();
  }
  if (!ok)
  {
    progress.hide ();
    MessageBox (NULL, "Failed to initialize application!", "Error", MB_OK | MB_ICONHAND);
  }
  else
  {
    progress.SetText ("Loading settings...", 25);
    reg.load ();
    progress.SetText ("Loading game cache...", 30);
	  CDotAReplayDlg dlg (mapview, &progress);
	  m_pMainWnd = &dlg;
    if (!__fail)
    {
  	  dlg.DoModal();
      progress.show ();
      progress.SetText ("Saving settings...", 0);
      reg.flush ();
      progress.SetText ("Saving game cache...", 5);
    }
  }
  if (res)
  {
    progress.SetText ("Packing resources...", 90);
    MPQFlush (res);
    MPQClose (res);
  }
  progress.SetText ("Shutting down...", 95);
  deleteImages ();
  freeDotaData ();
  unloadImages ();
  MPQCleanup ();
  //delete scriptEnv;

  if (mapview)
    UnmapViewOfFile (mapview);
  if (map != INVALID_HANDLE_VALUE)
    CloseHandle (map);

	return FALSE;
}
// E:\Progs\DotAReplay\DotAReplay.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"

// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CDialog)
CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
{
  percent = 0;
  showCount = 0;
  Create (IDD, pParent);
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CProgressDlg::show ()
{
  showCount++;
  ShowWindow (SW_SHOW);
}
void CProgressDlg::hide ()
{
  if (!--showCount)
    ShowWindow (SW_HIDE);
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
  ON_WM_PAINT()
END_MESSAGE_MAP()


// CProgressDlg message handlers

void CProgressDlg::OnPaint ()
{
  CPaintDC dc (this);
  CRect rc;
  GetDlgItem (IDC_PRGTEXT)->GetWindowRect (rc);
  ScreenToClient (rc);
  rc.bottom += 2;
  dc.DrawEdge (rc, EDGE_SUNKEN, BF_ADJUST | BF_RECT);
  if (percent)
  {
    CRect pb = rc;
    pb.right = pb.left + (pb.right - pb.left) * percent / 100;
    dc.FillSolidRect (pb, RGB (128, 128, 128));
    pb.left = pb.right;
    pb.right = rc.right;
    dc.FillSolidRect (pb, GetSysColor (COLOR_BTNFACE));
  }
  else
    dc.FillSolidRect (rc, GetSysColor (COLOR_BTNFACE));
  dc.SetBkMode (TRANSPARENT);
  dc.SelectObject (&font);
  dc.DrawText (subtext, -1, rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

BOOL CProgressDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  HDC hdc = ::GetDC (NULL);
  LOGFONT lf;
  memset (&lf, 0, sizeof lf);
  strcpy (lf.lfFaceName, "Courier New");
  lf.lfWeight = FW_BOLD;
  lf.lfHeight = -MulDiv (10, GetDeviceCaps (hdc, LOGPIXELSY), 72);
  ::ReleaseDC (NULL, hdc);

  font.CreateFontIndirect (&lf);
  SetFont (&font);

  return TRUE;
}
void CProgressDlg::SetText (char const* text, int perc)
{
  if (perc >= 0)
    percent = perc;
  SetWindowText (text);
  strcpy (subtext, text);
  Invalidate (FALSE);
  UpdateWindow ();
}
void CProgressDlg::SetSupText (char const* text, int perc)
{
  if (perc >= 0)
    percent = perc;
  SetWindowText (text);
  SetDlgItemText (IDC_TIP, text);
  GetDlgItem (IDC_TIP)->Invalidate (FALSE);
  GetDlgItem (IDC_TIP)->UpdateWindow ();
  Invalidate (FALSE);
  UpdateWindow ();
}
void CProgressDlg::SetSubText (char const* text, int perc)
{
  if (perc >= 0)
    percent = perc;
  strcpy (subtext, text);
  Invalidate (FALSE);
  UpdateWindow ();
}
void CProgressDlg::SetProgress (int perc)
{
  percent = perc;
  Invalidate (FALSE);
  UpdateWindow ();
}
