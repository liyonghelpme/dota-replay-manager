// BatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "BatchDlg.h"
#include ".\batchdlg.h"
#include "settingsdlg.h"
#include "gamecache.h"

// CBatchDlg dialog

IMPLEMENT_DYNAMIC(CBatchDlg, CDialog)
CBatchDlg::CBatchDlg(Array<CString>* fs, char const* fm, char const* bs, CSettingsDlg* set,
                     int md, CWnd* pParent /*=NULL*/)
	: CDialog(CBatchDlg::IDD, pParent)
{
  files = fs;
  fmt = fm;
  base = bs;
  settings = set;
  mode = md;
}

CBatchDlg::~CBatchDlg()
{
}

void CBatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CBatchDlg, CDialog)
  ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
  ON_WM_DESTROY()
END_MESSAGE_MAP()

void FileThread::Start (CBatchDlg* theDialog)
{
  dlg = theDialog;
  end = false;
  thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) threadProc, (LPVOID) this, 0, NULL);
}
void FileThread::Stop ()
{
  if (!end)
  {
    end = true;
    if (WaitForSingleObject (thread, 3000) != WAIT_OBJECT_0)
    {
      TerminateThread (thread, 0);
      thread = INVALID_HANDLE_VALUE;
    }
  }
}
void FileThread::Close ()
{
  if (thread != INVALID_HANDLE_VALUE)
    CloseHandle (thread);
}

DWORD WINAPI FileThread::threadProc (LPVOID param)
{
  FileThread* ft = (FileThread*) param;

  char title[256];
  ft->dlg->GetWindowText (title, 256);

  int curFile = 0;
  char filename[1024];
  CString dest;
  W3GReplay* w3g = NULL;
  if (ft->dlg->mode == BATCH_PROCESS || ft->dlg->mode == BATCH_CACHE)
    w3g = new W3GReplay ();
  while (!ft->end && curFile < ft->dlg->files->getSize ())
  {
    if (ft->dlg->mode == BATCH_PROCESS)
    {
      strcpy (filename, (*ft->dlg->files)[curFile]);

      FILE* file = fopen (filename, "rb");
      if (file)
      {
        w3g->clear ();
        if (w3g->load (file, true))
        {
          addGame (w3g, filename);
          w3g->readTime (filename);
          dest = ft->dlg->settings->getCopyName (w3g, filename, ft->dlg->fmt);
        }
        else
          dest = ft->dlg->settings->getCopyName (NULL, filename, ft->dlg->fmt);
        fclose (file);

        CString fdest = copyReplay (filename, ft->dlg->base, dest);
        if (!ft->end)
        {
          ft->dlg->SetDlgItemText (IDC_FROMFILE, mprintf ("From: %s", filename));
          ft->dlg->SetDlgItemText (IDC_TOFILE, mprintf ("To: %s", (char const*) fdest));
        }
      }
    }
    else if (ft->dlg->mode == BATCH_CACHE)
    {
      strcpy (filename, (*ft->dlg->files)[curFile]);

      FILE* file = fopen (filename, "rb");
      if (file)
      {
        w3g->clear ();
        if (w3g->load (file, true))
          addGame (w3g, filename);
        fclose (file);
        if (!ft->end)
        {
          ft->dlg->SetDlgItemText (IDC_FROMFILE, mprintf ("File: %s", filename));
          ft->dlg->GetDlgItem (IDC_TOFILE)->ShowWindow (SW_HIDE);
        }
      }
    }

    curFile++;
    if (!ft->end)
    {
      ft->dlg->pbar.SetPos (curFile);
      ft->dlg->SetWindowText (mprintf ("%s (%d/%d)", title, curFile, ft->dlg->files->getSize ()));
    }
  }

  delete w3g;
  if (!ft->end)
  {
    ft->end = true;
    ft->dlg->PostMessage (WM_CLOSE);
  }
  return 0;
}

// CBatchDlg message handlers

void CBatchDlg::OnBnClickedCancel()
{
  thread.Stop ();
  OnCancel();
}

BOOL CBatchDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  if (mode == BATCH_CACHE)
    SetWindowText ("Caching replays");
  pbar.Attach (GetDlgItem (IDC_BATCHPROGRESS)->m_hWnd);
  pbar.SetRange32 (0, files->getSize ());
  pbar.SetPos (0);
  thread.Start (this);

  return TRUE;
}
void CBatchDlg::OnDestroy ()
{
  thread.Close ();
  pbar.Detach ();
}
