// UpdateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "UpdateDlg.h"
#include <afxinet.h>

#include "utils.h"
#include ".\updatedlg.h"

unsigned int lastVersion;
const unsigned int curVersion = parseVersion ("2.09");

const char versionURL[] = "http://www.rivsoft.narod.ru/dotareplay_ver.txt";
const char projectURL[256] = "http://www.rivsoft.narod.ru/dotareplay.html";
const char forumURL[] = "http://forums.dota-allstars.com/index.php?showtopic=271794";
const char forum2URL[] = "http://www.playdota.com/forums/showthread.php?p=110886";
const char logURL[] = "http://www.rivsoft.narod.ru/dotareplay.log";

DWORD WINAPI UpdateVersion (LPVOID param)
{
  try
  {
    CInternetSession inet;
    CInternetFile* file = dynamic_cast<CInternetFile*> (inet.OpenURL (versionURL));
    char buf[256];
    if (file != NULL)
    {
      lastVersion = parseVersion (file->ReadString (buf, 255));
      delete file;
    }
    else
      lastVersion = 0;
  }
  catch (CInternetException*)
  {
  }
  return 0;
}

void updateVersion ()
{
  HANDLE thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) UpdateVersion, (LPVOID) NULL, 0, NULL);
}

// CUpdateDlg dialog

IMPLEMENT_DYNAMIC(CUpdateDlg, CDialog)
CUpdateDlg::CUpdateDlg(bool forced, CWnd* pParent)
	: CDialog(CUpdateDlg::IDD, pParent)
{
  autoCheck = forced;
  end = false;
}

CUpdateDlg::~CUpdateDlg()
{
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CDialog)
  ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
  ON_WM_DESTROY()
  ON_BN_CLICKED(IDC_FORUM, OnBnClickedForum)
  ON_BN_CLICKED(IDC_FORUM2, &CUpdateDlg::OnBnClickedForum2)
END_MESSAGE_MAP()

// CUpdateDlg message handlers

DWORD WINAPI LoadUpdate (LPVOID param)
{
  CUpdateDlg* dlg = (CUpdateDlg*) param;

  if (dlg->autoCheck)
    UpdateVersion (NULL);

  char buf[2048];
  sprintf (buf, "Current version: %s\r\n"
                "Last version: %s\r\n",
                formatVersion (curVersion),
                formatVersion (lastVersion));
  dlg->SetDlgItemText (IDC_BUFFER, buf);

  if (curVersion < lastVersion)
  {
    CString log = buf;
    log += "\r\nLoading changelog...";
    dlg->SetDlgItemText (IDC_BUFFER, log);
    try
    {
      CInternetSession inet;
      CInternetFile* file = dynamic_cast<CInternetFile*> (inet.OpenURL (logURL));
      log = buf;
      log += "\r\nChangelog:\r\n";
      if (file != NULL)
      {
        while (file->ReadString (buf, sizeof buf - 5))
        {
          if (buf[0] == '*' && buf[1] == '*')
          {
            unsigned int ver = parseVersion (buf + 2);
            if (ver != 0 && ver <= curVersion)
              break;
          }
          log += buf;
        }
        log.Replace ("\n", "\r\n");
        dlg->SetDlgItemText (IDC_BUFFER, log);
        delete file;
      }
      else
        lastVersion = 0;
    }
    catch (CInternetException*)
    {
    }
  }

  return 0;
}

BOOL CUpdateDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  SetDlgItemText (IDC_BUFFER, "Checking...");
  thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) LoadUpdate, (LPVOID) this, 0, NULL);

  return TRUE;
}

void CUpdateDlg::OnBnClickedUpdate()
{
  HKEY hKey;
  TCHAR name[128];
  DWORD size = 128;
  RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("http\\shell\\open\\ddeexec\\Application"), 0, KEY_QUERY_VALUE, &hKey);
  RegQueryValueEx (hKey, _T(""), 0, 0, (LPBYTE)name, &size);
  RegCloseKey (hKey);
  if (!_tcsnicmp (name, _T("IExplore"), 8))
    ShellExecute (NULL, _T("open"), _T("about:blank"), NULL, NULL, SW_SHOWNORMAL);
  ShellExecute (NULL, _T("open"), projectURL, NULL, NULL, SW_SHOWNORMAL);
}

void CUpdateDlg::OnDestroy ()
{
  TerminateThread (thread, 0);
}

void CUpdateDlg::OnBnClickedForum()
{
  HKEY hKey;
  TCHAR name[128];
  DWORD size = 128;
  RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("http\\shell\\open\\ddeexec\\Application"), 0, KEY_QUERY_VALUE, &hKey);
  RegQueryValueEx (hKey, _T(""), 0, 0, (LPBYTE)name, &size);
  RegCloseKey (hKey);
  if (!_tcsnicmp (name, _T("IExplore"), 8))
    ShellExecute (NULL, _T("open"), _T("about:blank"), NULL, NULL, SW_SHOWNORMAL);
  ShellExecute (NULL, _T("open"), forumURL, NULL, NULL, SW_SHOWNORMAL);
}

void CUpdateDlg::OnBnClickedForum2()
{
  HKEY hKey;
  TCHAR name[128];
  DWORD size = 128;
  RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("http\\shell\\open\\ddeexec\\Application"), 0, KEY_QUERY_VALUE, &hKey);
  RegQueryValueEx (hKey, _T(""), 0, 0, (LPBYTE)name, &size);
  RegCloseKey (hKey);
  if (!_tcsnicmp (name, _T("IExplore"), 8))
    ShellExecute (NULL, _T("open"), _T("about:blank"), NULL, NULL, SW_SHOWNORMAL);
  ShellExecute (NULL, _T("open"), forum2URL, NULL, NULL, SW_SHOWNORMAL);
}
