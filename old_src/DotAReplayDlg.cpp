// DotAReplayDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "DotAReplayDlg.h"
#include ".\dotareplaydlg.h"
#include "gameinfodlg.h"
#include "playerinfodlg.h"
#include "ilib.h"
#include "settingsdlg.h"
#include "timelinedlg.h"
#include "actionsdlg.h"
#include "presentdlg.h"
#include "playergolddlg.h"
#include "playerexpdlg.h"
#include "gamecache.h"
#include "searchdlg.h"
#include "playerstatsdlg.h"
#include "screenshotdlg.h"
#include "herochart.h"
#include <afxinet.h>
#include <winioctl.h>

#include "systray.h"
#include "registry.h"
#include "getdatadlg.h"
#include "chatfilters.h"
#include "draftdlg.h"
#include "actionlogdlg.h"
#include "newpresentdlg.h"

int _chatFilters = 0;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define NUM_TABS        11
#define TAB_GAME        0
#define TAB_CHAT        1
#define TAB_TIMELINE    2
#define TAB_PLAYERS     3
#define TAB_ACTIONS     4
#define TAB_GOLDGRAPH   5
#define TAB_EXPGRAPH    6
#define TAB_PRESENT     7
#define TAB_NEWPRESENT  8
#define TAB_DRAFT       9
#define TAB_ACTIONLOG   10

int ladderTabs[] = {TAB_GAME, TAB_CHAT, TAB_ACTIONS, TAB_ACTIONLOG};
const int NUM_LADDER_TABS = sizeof ladderTabs / sizeof ladderTabs[0];

DROPEFFECT CDropTarget::OnDragOver (CWnd* pWnd, COleDataObject* pDataObject,
  DWORD dwKeyState, CPoint point)
{
  return dlg->OnDragOver (pDataObject, dwKeyState, point);
}
DROPEFFECT CDropTarget::OnDropEx (CWnd* pWnd, COleDataObject* pDataObject,
  DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
  return dlg->OnDropEx (pDataObject, dropDefault, dropList, point);
}

static bool isLoadingView = false;
static bool isReplayOpen = false;


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
  CAboutDlg();

// Dialog Data
  enum { IDD = IDD_ABOUTBOX };

  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
  DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedWebsite();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
  ON_BN_CLICKED(IDC_WEBSITE, OnBnClickedWebsite)
END_MESSAGE_MAP()

// CDotAReplayDlg dialog

extern bool saveCache;
extern bool __fail;
CDotAReplayDlg* CDotAReplayDlg::instance = NULL;
DRView views[64];

CDotAReplayDlg::CDotAReplayDlg(void* dest, CProgressDlg* prgb, CWnd* pParent /*=NULL*/)
  : CDialog(CDotAReplayDlg::IDD, pParent)
{
  instance = this;
  dst = dest;
  progress = prgb;
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  memset (tabwnd, 0, sizeof tabwnd);
  curTab = 0;
  settings = new CSettingsDlg (this);
  updating = false;
  lastDate = 0;
  backingUp = false;
  initSize.x = -1;
  _chatFilters = chatFilters = CChatFilters::getDefaultFilter ();
  w3g = new W3GReplay ();
  if (saveCache)
    loadCache (progress, 30, 90);
}
CDotAReplayDlg::~CDotAReplayDlg ()
{
  if (instance == this)
    instance = NULL;
  delete tabwnd[TAB_GAME];
  delete tabwnd[TAB_PLAYERS];
  delete tabwnd[TAB_TIMELINE];
  delete tabwnd[TAB_ACTIONS];
  delete tabwnd[TAB_PRESENT];
  delete tabwnd[TAB_NEWPRESENT];
  delete tabwnd[TAB_GOLDGRAPH];
  delete tabwnd[TAB_EXPGRAPH];
  delete tabwnd[TAB_DRAFT];
  delete tabwnd[TAB_ACTIONLOG];
  delete settings;
  delete stats;
  delete scrn;
  delete hchart;
  delete w3g;
  if (saveCache && !__fail)
    storeCache (progress, 5, 90);
}

void CDotAReplayDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDotAReplayDlg, CDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_WM_DESTROY()
  ON_WM_CLOSE()
  ON_WM_TIMER()
  //}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_PARSEREPLAY, OnBnClickedParsereplay)
  ON_NOTIFY(TCN_SELCHANGE, IDC_TABS, OnTcnSelchangeTabs)
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_FILELIST, OnLvnItemActivateFilelist)
  ON_BN_CLICKED(IDC_BATCHHELP, OnBnClickedBatchhelp)
  ON_BN_CLICKED(IDC_BATCHCOPY, OnBnClickedBatchcopy)
  ON_BN_CLICKED(IDC_EXPLORE, OnBnClickedExplore)
  ON_BN_CLICKED(IDC_BROWSEREPLAY, OnBnClickedBrowsereplay)
  ON_BN_CLICKED(IDC_REFRESH, OnBnClickedRefresh)
  ON_NOTIFY(NM_RCLICK, IDC_FILELIST, OnNMRclickFilelist)
  ON_NOTIFY(LVN_ENDLABELEDIT, IDC_FILELIST, OnLvnEndlabeleditFilelist)
  ON_MESSAGE(WM_TRAYNOTIFY, OnTrayNotify)
  ON_WM_SIZE()
//  ON_WM_DROPFILES()
  ON_BN_CLICKED(IDC_SEARCH, OnBnClickedSearch)
  ON_NOTIFY(LVN_KEYDOWN, IDC_FILELIST, OnLvnKeydownFilelist)
  ON_BN_CLICKED(IDC_BYDATE, OnBnClickedBydate)
  ON_WM_GETMINMAXINFO()
  ON_WM_COPYDATA()
  ON_WM_MOVE()
  ON_BN_CLICKED(IDC_FINDNEXT, &CDotAReplayDlg::OnBnClickedFindnext)
  ON_BN_CLICKED(IDC_FINDPREV, &CDotAReplayDlg::OnBnClickedFindnext)
  ON_BN_CLICKED(IDC_FILTERS, &CDotAReplayDlg::OnBnClickedFilters)
  ON_CBN_SELCHANGE(IDC_SEARCHTYPE, &CDotAReplayDlg::OnCbnSelchangeSearchtype)
  ON_NOTIFY(LVN_BEGINDRAG, IDC_FILELIST, &CDotAReplayDlg::OnLvnBegindragFilelist)
  ON_BN_CLICKED(IDC_BACK, &CDotAReplayDlg::OnBnClickedBack)
  ON_BN_CLICKED(IDC_FORWARD, &CDotAReplayDlg::OnBnClickedForward)
END_MESSAGE_MAP()

#define ITEM_TYPE             0xFF000000
#define ITEM_SETTINGS         0x01000000
#define ITEM_FOLDER           0x02000000
#define ITEM_REPLAY           0x03000000
#define ITEM_LEVELUP          0x04000000
#define ITEM_NEWFOLDER        0x05000000
#define ITEM_SEARCHRES        0x10000000
#define ITEM_STATS            0x20000000
#define ITEM_SCREENSHOT       0x30000000
#define ITEM_HEROCHART        0x40000000
//#define ITEM_CUPHOLDER        0x50000000

#define ID_FILEM_COPY          1557
#define ID_FILEM_CUT           1558
#define ID_FILEM_PASTE         1559
#define ID_FILEM_DELETE        1560
#define ID_FILEM_OPEN          1561
#define ID_FILEM_FOLDER        1562
#define ID_FILEM_RENAME        1563
#define ID_FILEM_BACKUP        1564

// CDotAReplayDlg message handlers

BOOL CDotAReplayDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  if (dst)
    * (HWND*) dst = m_hWnd;

  progress->SetText ("Initializing window...", 90);

  SetIcon(m_hIcon, TRUE);

  chatbox.Attach (GetDlgItem (IDC_CHATBOX)->m_hWnd);
  chatbox.SetBackgroundColor (FALSE, RGB (20, 20, 20));
  chatbox.SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  searchPlayer.Attach (GetDlgItem (IDC_SEARCHPLAYER)->m_hWnd);
  searchPlayer.SetImageList (getImageList ());
  searchPlayer.SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  ((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->SetCurSel (0);

  GetDlgItem (IDC_SEARCHFOR)->SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  GetDlgItem (IDC_FINDNEXT)->SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  GetDlgItem (IDC_FINDPREV)->SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  GetDlgItem (IDC_FILTERS)->SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  GetDlgItem (IDC_SEARCHTYPE)->SetWindowPos (&tabs, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  tabs.Attach (GetDlgItem (IDC_TABS)->m_hWnd);

  tabs.InsertItem (TAB_GAME, "Game Info");
  tabs.InsertItem (TAB_CHAT, "Game Chat");
  tabs.InsertItem (TAB_TIMELINE, "Timeline");
  tabs.InsertItem (TAB_PLAYERS, "Builds");
  tabs.InsertItem (TAB_ACTIONS, "Actions");
  tabs.InsertItem (TAB_GOLDGRAPH, "Gold timeline");
  tabs.InsertItem (TAB_EXPGRAPH, "Exp timeline");
  tabs.InsertItem (TAB_PRESENT, "Presentation");
  tabs.InsertItem (TAB_NEWPRESENT, "ExtPresent");
  tabs.InsertItem (TAB_DRAFT, "Draft");
  tabs.InsertItem (TAB_ACTIONLOG, "Action Log");

  fload.Attach (GetDlgItem (IDC_FOLDERLOAD)->m_hWnd);

  CRect rc;
  chatbox.GetWindowRect (rc);
  tabs.ScreenToClient (rc);
  CRect rc2;
  searchPlayer.GetWindowRect (rc2);
  tabs.ScreenToClient (rc2);
  rc.top = rc2.top;

  CGameInfoDlg* gameDlg = new CGameInfoDlg (this, &tabs);
  gameDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CPlayerInfoDlg* playerDlg = new CPlayerInfoDlg (this, &tabs);
  playerDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CTimelineDlg* timeDlg = new CTimelineDlg (&tabs);
  timeDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CActionsDlg* actionDlg = new CActionsDlg (this, &tabs);
  actionDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CPlayerGoldDlg* goldDlg = new CPlayerGoldDlg (&tabs);
  goldDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CPlayerExpDlg* expDlg = new CPlayerExpDlg (&tabs);
  expDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CPresentDlg* presentDlg = new CPresentDlg (&tabs);
  presentDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CNewPresentDlg* newPresentDlg = new CNewPresentDlg (&tabs);
  newPresentDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CDraftDlg* draftDlg = new CDraftDlg (&tabs);
  draftDlg->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  CActionLogDlg* actionLog = new CActionLogDlg (&tabs);
  actionLog->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);

  tabwnd[TAB_GAME] = gameDlg;
  tabwnd[TAB_CHAT] = &chatbox;
  tabwnd[TAB_TIMELINE] = timeDlg;
  tabwnd[TAB_PLAYERS] = playerDlg;
  tabwnd[TAB_ACTIONS] = actionDlg;
  tabwnd[TAB_GOLDGRAPH] = goldDlg;
  tabwnd[TAB_EXPGRAPH] = expDlg;
  tabwnd[TAB_PRESENT] = presentDlg;
  tabwnd[TAB_NEWPRESENT] = newPresentDlg;
  tabwnd[TAB_DRAFT] = draftDlg;
  tabwnd[TAB_ACTIONLOG] = actionLog;

  tabwnd[curTab]->ShowWindow (SW_SHOW);

  tabs.GetWindowRect (rc);
  ScreenToClient (rc);
  stats = new CPlayerStatsDlg (this);
  stats->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  scrn = new CScreenshotDlg (this);
  scrn->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);
  hchart = new CHeroChart (this);
  hchart->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);

  files.Attach (GetDlgItem (IDC_FILETREE)->m_hWnd);
  files.SetImageList (getImageList (), TVSIL_NORMAL);
  files.SetItemHeight (18);

  curfold.Attach (GetDlgItem (IDC_FILELIST)->m_hWnd);
  curfold.SetImageList (getImageList (), LVSIL_SMALL);
  curfold.SetExtendedStyle (LVS_EX_HEADERDRAGDROP);

  CheckDlgButton (IDC_BYDATE, reg.readInt ("byDate", 0));

  updateTreeView ();

  settings->Create (CSettingsDlg::IDD, this);
  tabs.GetWindowRect (rc);
  ScreenToClient (rc);
  settings->SetWindowPos (NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
    SWP_NOZORDER);

  SetTimer (33, 1000, NULL);

  dropTarget.dlg = this;
  dropTarget.Register (this);

  viewMode = ITEM_REPLAY;

  fileMenu.CreatePopupMenu ();
  MENUITEMINFO mii;
  memset (&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "New Folder";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_FOLDER;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Delete";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_DELETE;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Rename";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_RENAME;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Paste";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_PASTE;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Copy";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_COPY;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Cut";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_CUT;
//  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Backup";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_BACKUP;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Open";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_FILEM_OPEN;
  fileMenu.InsertMenuItem (0, &mii, TRUE);

  trayMenu.CreatePopupMenu ();

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "Exit";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_TRAY_EXIT;
  trayMenu.InsertMenuItem (0, &mii, TRUE);

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Open";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_TRAY_OPEN;
  trayMenu.InsertMenuItem (0, &mii, TRUE);

  lastDate = getFileDate (settings->replayPath + "LastReplay.w3g");

  if (AfxGetApp ()->m_lpCmdLine[0])
  {
    CString str = AfxGetApp()->m_lpCmdLine;
    if (str [0] == '\"')
      str = str.Mid (1, str.GetLength () - 2);
    SetDlgItemText (IDC_REPLAYFILE, str);
    OnBnClickedParsereplay ();
    SetFocus ();
  }
  else
  {
    SetDlgItemText (IDC_REPLAYFILE, settings->replayPath);
    OnBnClickedParsereplay ();
    curfold.SetFocus ();
  }

  SetWindowLong (tabs.m_hWnd, GWL_STYLE, GetWindowLong (tabs.m_hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

  loc.SetWindow (this);

  loc.SetItemAbsolute (IDC_FILETREE, SIDE_RIGHT, PERCENT);
  loc.SetItemAbsolute (IDC_FILETREE, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_BYDATE, SIDE_TOP, IDC_BYDATE, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_REFRESH, SIDE_RIGHT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_REFRESH, SIDE_TOP, IDC_REFRESH, SIDE_BOTTOM);

  loc.SetItemRelative (IDC_REPLAYFILE, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_REPLAYFILE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_BROWSEREPLAY, SIDE_LEFT, IDC_BROWSEREPLAY, SIDE_RIGHT);
  loc.SetItemRelative (IDC_PARSEREPLAY, SIDE_LEFT, IDC_PARSEREPLAY, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SEPARATOR, SIDE_LEFT, IDC_SEPARATOR, SIDE_RIGHT);
  loc.SetItemRelative (IDC_BACK, SIDE_LEFT, IDC_BACK, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FORWARD, SIDE_LEFT, IDC_FORWARD, SIDE_RIGHT);

  loc.SetItemRelative (IDC_FILELIST, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FILELIST, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FILELIST, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_SEARCHTYPE, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SEARCHFOR, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEARCHFOR, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SEARCHPLAYER, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEARCHPLAYER, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FINDNEXT, SIDE_LEFT, IDC_SEARCHFOR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FINDNEXT, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FINDPREV, SIDE_LEFT, IDC_SEARCHFOR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FINDPREV, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FILTERS, SIDE_LEFT, IDC_SEARCHFOR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FILTERS, SIDE_RIGHT);
  loc.SetItemRelative (IDC_CHATBOX, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_CHATBOX, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_CHATBOX, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_TABS, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_TABS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_TABS, SIDE_BOTTOM);

  loc.SetItemRelative (IDC_EXPLORE, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_EXPLORE, SIDE_TOP, IDC_EXPLORE, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_SEARCH, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_SEARCH, SIDE_TOP, IDC_SEARCH, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_ETCHED, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemRelative (IDC_ETCHED, SIDE_TOP, IDC_ETCHED, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_BATCH, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_BATCH, SIDE_RIGHT);
  loc.SetItemRelative (IDC_BATCH, SIDE_TOP, IDC_BATCH, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_FOLDERLOAD, SIDE_LEFT, IDC_FILETREE, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FOLDERLOAD, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FOLDERLOAD, SIDE_TOP, IDC_FOLDERLOAD, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_BATCHCOPY, SIDE_LEFT, IDC_BATCHCOPY, SIDE_RIGHT);
  loc.SetItemRelative (IDC_BATCHCOPY, SIDE_TOP, IDC_BATCHCOPY, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_BATCHHELP, SIDE_LEFT, IDC_BATCHHELP, SIDE_RIGHT);
  loc.SetItemRelative (IDC_BATCHHELP, SIDE_TOP, IDC_BATCHHELP, SIDE_BOTTOM);

  loc.SetItemRelative (settings, SIDE_LEFT, GetDlgItem (IDC_FILETREE), SIDE_RIGHT);
  loc.SetItemAbsolute (settings, SIDE_RIGHT);
  loc.SetItemAbsolute (settings, SIDE_BOTTOM);
  loc.SetItemRelative (stats, SIDE_LEFT, GetDlgItem (IDC_FILETREE), SIDE_RIGHT);
  loc.SetItemAbsolute (stats, SIDE_RIGHT);
  loc.SetItemAbsolute (stats, SIDE_BOTTOM);
  loc.SetItemRelative (scrn, SIDE_LEFT, GetDlgItem (IDC_FILETREE), SIDE_RIGHT);
  loc.SetItemAbsolute (scrn, SIDE_RIGHT);
  loc.SetItemAbsolute (scrn, SIDE_BOTTOM);
  loc.SetItemRelative (hchart, SIDE_LEFT, GetDlgItem (IDC_FILETREE), SIDE_RIGHT);
  loc.SetItemAbsolute (hchart, SIDE_RIGHT);
  loc.SetItemAbsolute (hchart, SIDE_BOTTOM);

  loc.Start ();

  tabloc.SetWindow (&tabs);
  tabloc.SetItemAbsolute (gameDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (gameDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (timeDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (timeDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (playerDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (playerDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (actionDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (actionDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (goldDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (goldDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (expDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (expDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (presentDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (presentDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (newPresentDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (newPresentDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (draftDlg, SIDE_RIGHT);
  tabloc.SetItemAbsolute (draftDlg, SIDE_BOTTOM);
  tabloc.SetItemAbsolute (actionLog, SIDE_RIGHT);
  tabloc.SetItemAbsolute (actionLog, SIDE_BOTTOM);
  tabloc.Start ();

  GetWindowRect (rc);
  rc.right += 2;
  initSize.x = rc.Width ();
  initSize.y = rc.Height ();
  int px = reg.readInt ("wndX", rc.left);
  int py = reg.readInt ("wndY", rc.top);
  int wx = reg.readInt ("wndWidth", initSize.x);
  int wy = reg.readInt ("wndHeight", initSize.y);
  int show = reg.readInt ("wndShow", SW_SHOW);
  SetWindowPos (NULL, px, py, wx, wy, SWP_NOZORDER);
  ShowWindow (show);
  progress->hide ();

  return FALSE;
}

void CDotAReplayDlg::OnChangePath ()
{
  lastDate = getFileDate (settings->replayPath + "LastReplay.w3g");
  updateTreeView ();
}

void CDotAReplayDlg::OnDestroy ()
{
  chatbox.Detach ();
  searchPlayer.Detach ();
  tabs.Detach ();
  files.Detach ();
  fload.Detach ();
  curfold.Detach ();
  DeleteTrayIcon ();
}

void CDotAReplayDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialog::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDotAReplayDlg::OnPaint() 
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialog::OnPaint();
  }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDotAReplayDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

struct TextStreamState
{
  W3GReplay* w3g;
  int pos;
  int filter;
  int numLines;
  char curLine[8192];
  int chr;
  unsigned long last;
};
static char __tmp[8192];
extern DWORD chatBg;
extern DWORD chatFg;
extern LOGFONT chatFont;
extern int chatColors;
extern bool chatHeroes;
char const* san_player (W3GPlayer const& p)
{
  char* pname = sanitize (p.uname);
  DotaHero* hero = NULL;
  if (p.hero)
    hero = getHero (p.hero->id);
  if (chatHeroes && hero)
    return mprintf ("%s (%s)", pname, hero->abbr);
  else
    return pname;
}
inline void __append (char* buf, int& len, char const* str)
{
  while (*str)
    buf[len++] = *str++;
}
char const* sanitize_notify (TextStreamState* stream, wchar_t const* text)
{
  char* txt = sanitize (text);
  char* buf = mprintf ("");
  int len = 0;
  for (int i = 0; txt[i];)
  {
    if (txt[i] == '@')
    {
      if (txt[i + 1] == 's' || txt[i + 1] == 'u' || txt[i + 1] == 'n')
      {
        i++;
        if (txt[i] == 's')
          __append (buf, len, "{\\cf1 The Sentinel}");
        else if (txt[i] == 'u')
          __append (buf, len, "{\\cf7 The Scourge}");
        else if (txt[i] == 'n')
          __append (buf, len, "{\\cf13 Neutral Creeps}");
        i += 2;
      }
      else if (txt[i + 1] == 'r')
      {
        i += 2;
        if (txt[i] == '1')
          __append (buf, len, "{\\cf15 Haste}");
        else if (txt[i] == '2')
          __append (buf, len, "{\\cf16 Regeneration}");
        else if (txt[i] == '3')
          __append (buf, len, "{\\cf17 Double Damage}");
        else if (txt[i] == '4')
          __append (buf, len, "{\\cf18 Illusion}");
        else if (txt[i] == '5')
          __append (buf, len, "{\\cf19 Invisibility}");
        i += 2;
      }
      else if (txt[i + 1] == 'm')
      {
        i += 2;
        if (txt[i] == '3')
          __append (buf, len, "is on a {\\cf20 killing spree}!");
        else if (txt[i] == '4')
          __append (buf, len, "is {\\cf21 dominating}!");
        else if (txt[i] == '5')
          __append (buf, len, "has a {\\cf22 mega kill}!");
        else if (txt[i] == '6')
          __append (buf, len, "is {\\cf23 unstoppable}!!");
        else if (txt[i] == '7')
          __append (buf, len, "is {\\cf24 wicked sick}!!");
        else if (txt[i] == '8')
          __append (buf, len, "has a {\\cf25 monster kill}!!");
        else if (txt[i] == '9')
          __append (buf, len, "is {\\cf26 GODLIKE}!!!");
        else if (txt[i] == '0')
          __append (buf, len, "is {\\cf27 beyond GODLIKE}. Someone KILL HIM!!!!!!");
        i += 2;
      }
      else if (txt[i + 1] == 'k')
      {
        i += 2;
        if (txt[i] == '2')
          __append (buf, len, "just got a {\\cf28 Double Kill}!");
        else if (txt[i] == '3')
          __append (buf, len, "just got a {\\cf29 Triple Kill}!!!");
        else if (txt[i] == '4')
          __append (buf, len, "just got a {\\cf30 Ultra Kill}!!!");
        else if (txt[i] == '5')
          __append (buf, len, "is on a {\\cf31 Rampage}!!!");
        i += 2;
      }
      else
      {
        int id = 0;
        for (i++; txt[i] != '@' && txt[i] != '|'; i++)
          id = id * 10 + txt[i] - '0';
        int clr = (id >= 0 && id <= 255 && stream->w3g->players[id].name[0])
          ? stream->w3g->players[id].slot.color
          : 0;
        if (txt[i] == '|')
        {
          clr = 0;
          for (i++; txt[i] != '@'; i++)
            clr = clr * 10 + txt[i] - '0';
        }
        if (clr)
          __append (buf, len, mprintf ("{\\cf%d ", clr + 1));
        for (i++; txt[i] && txt[i] != ' ' && txt[i] != '\t' && txt[i] != '/'; i++)
          buf[len++] = txt[i];
        if (id >= 0 && id <= 255 && stream->w3g->players[id].name[0])
        {
          DotaHero* hero = NULL;
          if (stream->w3g->players[id].hero)
            hero = getHero (stream->w3g->players[id].hero->id);
          if (chatHeroes && hero)
            __append (buf, len, mprintf (" (%s)", hero->abbr));
        }
        if (clr)
          buf[len++] = '}';
      }
    }
    else
      buf[len++] = txt[i++];
  }
  buf[len] = 0;
  return buf;
}
DWORD CALLBACK StreamCallback (DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
  TextStreamState* stream = (TextStreamState*) dwCookie;
  char* buf = (char*) pbBuff;
  int size = (int) cb - 1;

  int len = 0;
  while (len < size)
  {
    if (stream->pos < 0 && stream->chr == 0)
    {
      strcpy (stream->curLine, "{\\rtf1\\ansi{\\fonttbl\\f0\\f");
      if (chatFont.lfPitchAndFamily == FF_DECORATIVE)
        strcat (stream->curLine, "decor");
      else if (chatFont.lfPitchAndFamily == FF_MODERN)
        strcat (stream->curLine, "modern");
      else if (chatFont.lfPitchAndFamily == FF_ROMAN)
        strcat (stream->curLine, "roman");
      else if (chatFont.lfPitchAndFamily == FF_SCRIPT)
        strcat (stream->curLine, "script");
      else if (chatFont.lfPitchAndFamily == FF_SWISS)
        strcat (stream->curLine, "swiss");
      else
        strcat (stream->curLine, "nil");
      strcat (stream->curLine, mprintf (" %s;}{\\colortbl;", chatFont.lfFaceName));
      for (int i = 0; i < 13; i++)
      {
        DWORD clr;
        if (chatColors == 0)
          clr = getDefaultColor (i);
        else if (chatColors == 1)
          clr = getSlotColor (i);
        else
          clr = getDarkColor (i);
        strcat (stream->curLine, mprintf ("\\red%d\\green%d\\blue%d;", (clr & 0xFF),
          ((clr >> 8) & 0xFF), ((clr >> 16) & 0xFF)));
      }
      HDC hdc = ::GetDC (NULL);
      int fontSize = -MulDiv (chatFont.lfHeight, 144, GetDeviceCaps (hdc, LOGPIXELSY));
      ::ReleaseDC (NULL, hdc);
      strcat (stream->curLine, mprintf ("\\red%d\\green%d\\blue%d;%s}\\f0\\cf14\\fs%d\\pard",
        (chatFg & 0xFF), ((chatFg >> 8) & 0xFF), ((chatFg >> 16) & 0xFF), getExtraColors (),
        fontSize));
      if (chatFont.lfUnderline)
        strcat (stream->curLine, "\\ul");
      if (chatFont.lfItalic)
        strcat (stream->curLine, "\\i");
      if (chatFont.lfStrikeOut)
        strcat (stream->curLine, "\\strike");
      if (chatFont.lfWeight > 400)
        strcat (stream->curLine, "\\b");
      strcat (stream->curLine, "\n");
      stream->pos = -1;
    }
    else if (stream->curLine[stream->chr] == 0)
    {
      stream->pos++;
      if (stream->pos > stream->w3g->chat.getSize ())
        break;
      else if (stream->pos == stream->w3g->chat.getSize ())
      {
        strcpy (stream->curLine, "}");
        stream->chr = 0;
      }
      else if (CChatFilters::isMessageAllowed (stream->filter, stream->w3g->chat[stream->pos].mode,
        stream->w3g->chat[stream->pos].notifyType))
      {
        switch (stream->w3g->chat[stream->pos].mode)
        {
        case CHAT_ALL:
          sprintf (stream->curLine, "%s\t [All] {\\ul\\cf%d %s:} %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            stream->w3g->players[stream->w3g->chat[stream->pos].id].slot.color + 1,
            san_player (stream->w3g->players[stream->w3g->chat[stream->pos].id]),
            sanitize (stream->w3g->chat[stream->pos].utext));
          break;
        case CHAT_ALLIES:
          sprintf (stream->curLine, "%s\t [Allies] {\\ul\\cf%d %s:} %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            stream->w3g->players[stream->w3g->chat[stream->pos].id].slot.color + 1,
            san_player (stream->w3g->players[stream->w3g->chat[stream->pos].id]),
            sanitize (stream->w3g->chat[stream->pos].utext));
          break;
        case CHAT_OBSERVERS:
          sprintf (stream->curLine, "%s\t [Observers] {\\ul\\cf%d %s:} %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            stream->w3g->players[stream->w3g->chat[stream->pos].id].slot.color + 1,
            san_player (stream->w3g->players[stream->w3g->chat[stream->pos].id]),
            sanitize (stream->w3g->chat[stream->pos].utext));
          break;
        case CHAT_PRIVATE:
          sprintf (stream->curLine, "%s\t [Private] {\\ul\\cf%d %s:} %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            stream->w3g->players[stream->w3g->chat[stream->pos].id].slot.color + 1,
            san_player (stream->w3g->players[stream->w3g->chat[stream->pos].id]),
            sanitize (stream->w3g->chat[stream->pos].utext));
          break;
        case CHAT_COMMAND:
          sprintf (stream->curLine, "%s\t [Game Command] {\\ul\\cf%d %s:} %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            stream->w3g->players[stream->w3g->chat[stream->pos].id].slot.color + 1,
            san_player (stream->w3g->players[stream->w3g->chat[stream->pos].id]),
            sanitize (stream->w3g->chat[stream->pos].utext));
          break;
        case CHAT_NOTIFY:
          sprintf (stream->curLine, "%s\t %s\\line\n",
            format_time (stream->w3g, stream->w3g->chat[stream->pos].time),
            sanitize_notify (stream, stream->w3g->chat[stream->pos].utext));
          break;
        default:
          stream->curLine[0] = 0;
          break;
        }
        if (stream->curLine[0] != 0 && stream->last + 30000 < stream->w3g->chat[stream->pos].time &&
          stream->last != 0 && CChatFilters::shouldInsertBlanks (stream->filter))
        {
          strcpy (__tmp, stream->curLine);
          sprintf (stream->curLine, "\\line\n%s", __tmp);
          stream->numLines++;
        }
        if (stream->curLine[0] != 0)
          stream->w3g->chat[stream->pos].line = stream->numLines++;
        stream->last = stream->w3g->chat[stream->pos].time;
        stream->chr = 0;
      }
    }
    int canwr = (int) strlen (stream->curLine) - stream->chr;
    if (canwr > size - len) canwr = size - len;
    if (canwr > 0)
    {
      memcpy (buf + len, stream->curLine + stream->chr, canwr);
      stream->chr += canwr;
      len += canwr;
    }
  }
  buf[len] = 0;
  *pcb = len;
  return 0;
}

extern bool enableUrl;

void CDotAReplayDlg::updateChat ()
{
  TextStreamState stream;
  stream.w3g = w3g;
  stream.pos = -1;
  stream.chr = 0;
  stream.last = 0;
  stream.filter = chatFilters;
  stream.numLines = 0;
  for (int i = 0; i < w3g->chat.getSize (); i++)
    w3g->chat[i].line = -1;
  EDITSTREAM es;
  es.dwCookie = (DWORD_PTR) &stream;
  es.pfnCallback = StreamCallback;
  chatbox.StreamIn (SF_RTF, es);
  chatbox.SetBackgroundColor (FALSE, chatBg);
}
void CDotAReplayDlg::updateSearchPlayer ()
{
  searchPlayer.Reset ();
  if (w3g->dota.isDota)
  {
    searchPlayer.InsertItem ("The Sentinel", getImageIndex ("RedBullet"), 0xFFFFFF, -1);
    for (int i = 0; i < w3g->dota.numSentinel; i++)
    {
      if (w3g->players[w3g->dota.sentinel[i]].hero)
        searchPlayer.InsertItem (wmprintf (L"%s (%s)", w3g->players[w3g->dota.sentinel[i]].uname,
          makeucd (getHero (w3g->players[w3g->dota.sentinel[i]].hero->id)->name)),
          getImageIndex (getHero (w3g->players[w3g->dota.sentinel[i]].hero->id)->imgTag),
          getLightColor (w3g->players[w3g->dota.sentinel[i]].slot.color), w3g->dota.sentinel[i]);
      else
        searchPlayer.InsertItem (wmprintf (L"%s (No Hero)", w3g->players[w3g->dota.sentinel[i]].uname),
          getImageIndex ("Empty"),
          getLightColor (w3g->players[w3g->dota.sentinel[i]].slot.color), w3g->dota.sentinel[i]);
    }
    searchPlayer.InsertItem ("The Scourge", getImageIndex ("GreenBullet"), 0xFFFFFF, -2);
    for (int i = 0; i < w3g->dota.numScourge; i++)
    {
      if (w3g->players[w3g->dota.scourge[i]].hero)
        searchPlayer.InsertItem (wmprintf (L"%s (%s)", w3g->players[w3g->dota.scourge[i]].uname,
          makeucd (getHero (w3g->players[w3g->dota.scourge[i]].hero->id)->name)),
          getImageIndex (getHero (w3g->players[w3g->dota.scourge[i]].hero->id)->imgTag),
          getLightColor (w3g->players[w3g->dota.scourge[i]].slot.color), w3g->dota.scourge[i]);
      else
        searchPlayer.InsertItem (wmprintf (L"%s (No Hero)", w3g->players[w3g->dota.scourge[i]].uname),
          getImageIndex ("Empty"),
          getLightColor (w3g->players[w3g->dota.scourge[i]].slot.color), w3g->dota.scourge[i]);
    }
    searchPlayer.InsertItem ("Neutral Creeps", getImageIndex ("random"), 0xFFFFFF, -3);
    searchPlayer.SetCurSel (0);
  }
  else
  {
    for (int i = 0; i < w3g->numPlayers; i++)
      searchPlayer.InsertItem (w3g->players[w3g->pindex[i]].uname,
        getImageIndex (raceImage[w3g->players[w3g->pindex[i]].race]),
        getLightColor (w3g->players[w3g->pindex[i]].slot.color), w3g->pindex[i]);
    searchPlayer.SetCurSel (0);
  }
}
void CDotAReplayDlg::OpenReplay ()
{
  w3g->clear ();
  searchPlayer.Reset ();
  _chatFilters = chatFilters = CChatFilters::getDefaultFilter ();
  CString filename;
  GetDlgItemText (IDC_REPLAYFILE, filename);
  FILE* file = fopen (filename, "rb");
  isReplayOpen = false;
  if (file != NULL)
  {
    if (w3g->load (file))
    {
      isReplayOpen = true;
      w3g->setLocation (filename);
      addGame (w3g, filename);
      w3g->readTime (filename);
      w3g->setpath (filename);

      updateChat ();
      updateSearchPlayer ();

      modifyTabs (w3g->dota.isDota);
      updateView (ITEM_REPLAY);
      ((CGameInfoDlg*) tabwnd[TAB_GAME])->setReplay (w3g, filename);
      ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (w3g);
      ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (w3g);
      ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (w3g);
      ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (w3g);
      ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (w3g);
      ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (w3g);
      ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (w3g);
      ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (w3g);
      ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (w3g);
      tabs.EnableWindow (TRUE);
      char title[256];
      _splitpath (filename, NULL, NULL, title, NULL);
      SetWindowText (mprintf ("DotA Replay Manager - %s", title));
    }
    else
    {
      ((CGameInfoDlg*) tabwnd[TAB_GAME])->setFailure ("Failed to parse replay!");
      ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (NULL);
      ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (NULL);
      ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (NULL);
      ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (NULL);
      ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (NULL);
      ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (NULL);
      ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (NULL);
      ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (NULL);
      ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (NULL);
    }
    fclose (file);
  }
  else if (filename.Left (7).CompareNoCase ("Player:") == 0)
  {
    updateView (ITEM_STATS);
    stats->setPlayer (makeucd (filename.Mid (7)));
  }
  else if (filename.Left (5) == "http:" && enableUrl)
  {
    CInternetSession inet;
    CHttpFile* file = dynamic_cast<CHttpFile*> (inet.OpenURL (filename));
    if (file != NULL)
    {
      int length = (int) file->GetLength ();
      char path[512];
      GetTempPath (512, path);
      strcat (path, "tmpreplay.w3g");
      FILE* floc = fopen (path, "wb");
      static char buf[4096];
      int size;
      while (size = file->Read (buf, 4096))
        fwrite (buf, size, 1, floc);
      fclose (floc);
      floc = fopen (path, "rb");
      if (floc != NULL)
      {
        updateView (ITEM_REPLAY);
        if (w3g->load (floc))
        {
          w3g->setLocation (path);
          SYSTEMTIME stime;
          file->QueryInfo (HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, &stime);
          w3g->readTime (&stime);
          w3g->setpath (filename);

          updateChat ();
          updateSearchPlayer ();

          modifyTabs (w3g->dota.isDota);
          ((CGameInfoDlg*) tabwnd[TAB_GAME])->setReplay (w3g, path);
          ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (w3g);
          ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (w3g);
          ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (w3g);
          ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (w3g);
          ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (w3g);
          ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (w3g);
          ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (w3g);
          ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (w3g);
          ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (w3g);
          tabs.EnableWindow (TRUE);
          char title[256];
          _splitpath (filename, NULL, NULL, title, NULL);
          SetWindowText (mprintf ("DotA Replay Manager - %s", title));
        }
        else
        {
          ((CGameInfoDlg*) tabwnd[TAB_GAME])->setFailure ("Failed to parse replay!");
          ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (NULL);
          ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (NULL);
          ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (NULL);
          ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (NULL);
          ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (NULL);
          ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (NULL);
          ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (NULL);
          ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (NULL);
          ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (NULL);
        }
        fclose (floc);
      }
    }
    else
    {
      updateView (ITEM_REPLAY);
      ((CGameInfoDlg*) tabwnd[TAB_GAME])->setFailure ("File not found!");
      ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (NULL);
      ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (NULL);
      ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (NULL);
      ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (NULL);
      ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (NULL);
      ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (NULL);
      ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (NULL);
      ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (NULL);
      ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (NULL);
    }
  }
  else
  {
    unslash (filename);
    if (PathFileExists (filename))
    {
      updateView (ITEM_FOLDER);
      fillFolder (filename, FILL_FOLDER);
    }
    else
    {
      updateView (ITEM_REPLAY);
      ((CGameInfoDlg*) tabwnd[TAB_GAME])->setFailure ("File not found!");
      ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->setReplay (NULL);
      ((CTimelineDlg*) tabwnd[TAB_TIMELINE])->setReplay (NULL);
      ((CActionsDlg*) tabwnd[TAB_ACTIONS])->setReplay (NULL);
      ((CPlayerGoldDlg*) tabwnd[TAB_GOLDGRAPH])->setReplay (NULL);
      ((CPlayerExpDlg*) tabwnd[TAB_EXPGRAPH])->setReplay (NULL);
      ((CPresentDlg*) tabwnd[TAB_PRESENT])->setReplay (NULL);
      ((CNewPresentDlg*) tabwnd[TAB_NEWPRESENT])->setReplay (NULL);
      ((CDraftDlg*) tabwnd[TAB_DRAFT])->setReplay (NULL);
      ((CActionLogDlg*) tabwnd[TAB_ACTIONLOG])->setReplay (NULL);
    }
  }
}

struct TreeItem
{
  CString path;
  HTREEITEM id;
  TreeItem () {}
  TreeItem (CString const& p, HTREEITEM i)
  {
    path = p;
    id = i;
  }
};
Array<TreeItem> treeStr;

void CDotAReplayDlg::OnBnClickedParsereplay ()
{
  isReplayOpen = false;
  CString filename;
  GetDlgItemText (IDC_REPLAYFILE, filename);
  filename.Replace ('/', '\\');
  if (filename.Find (':') < 0)
  {
    char path[1024];
    if (viewMode == ITEM_FOLDER || viewMode == ITEM_SEARCHRES)
      strcpy (path, viewPath);
    else
      strcpy (path, settings->replayPath);
    int len = (int) strlen (path);
    CString cur = "";
    for (int i = 0; filename[i]; i++)
    {
      if (filename[i] == '\\')
      {
        if (cur == "..")
        {
          if (len > 1 && path[len - 1] == '\\' && path[len - 2] != ':')
          {
            len--;
            while (len > 0 && path[len - 1] != '\\')
              len--;
            path[len] = 0;
          }
        }
        else if (cur != "" && cur != ".")
          strcat (path, cur + '\\');
        cur = "";
      }
      else
        cur += filename[i];
    }
    strcat (path, cur);
    filename = path;
    SetDlgItemText (IDC_REPLAYFILE, filename);
  }
  for (int i = 0; i < treeStr.getSize (); i++)
  {
    if (!stricmp (treeStr[i].path, filename))
    {
      if (files.GetSelectedItem () != treeStr[i].id)
        files.SelectItem (treeStr[i].id);
      else
        OpenReplay ();
      return;
    }
  }
  files.SelectItem (NULL);
  OpenReplay ();
}

void CDotAReplayDlg::OnTcnSelchangeTabs(NMHDR *pNMHDR, LRESULT *pResult)
{
  bool dota = (w3g == NULL || w3g->dota.isDota);
  int sel = tabs.GetCurSel ();
  int chatTab = SW_HIDE;
  if (dota)
  {
    if (tabwnd[curTab])
      tabwnd[curTab]->ShowWindow (SW_HIDE);
    if (tabwnd[sel])
      tabwnd[sel]->ShowWindow (SW_SHOW);
    if (sel == TAB_CHAT)
      chatTab = SW_SHOW;
  }
  else
  {
    if (tabwnd[ladderTabs[curTab]])
      tabwnd[ladderTabs[curTab]]->ShowWindow (SW_HIDE);
    if (tabwnd[ladderTabs[sel]])
      tabwnd[ladderTabs[sel]]->ShowWindow (SW_SHOW);
    if (ladderTabs[sel] == TAB_CHAT)
      chatTab = SW_SHOW;
  }
  GetDlgItem (IDC_SEARCHTYPE)->ShowWindow (chatTab);
  GetDlgItem (IDC_FINDNEXT)->ShowWindow (chatTab);
  GetDlgItem (IDC_FINDPREV)->ShowWindow (chatTab);
  GetDlgItem (IDC_FILTERS)->ShowWindow (chatTab);
  if (chatTab == SW_SHOW)
  {
    if (((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->GetCurSel () >= 3)
    {
      GetDlgItem (IDC_SEARCHFOR)->ShowWindow (SW_HIDE);
      GetDlgItem (IDC_SEARCHPLAYER)->ShowWindow (SW_SHOW);
    }
    else
    {
      GetDlgItem (IDC_SEARCHFOR)->ShowWindow (SW_SHOW);
      GetDlgItem (IDC_SEARCHPLAYER)->ShowWindow (SW_HIDE);
    }
  }
  else
  {
    GetDlgItem (IDC_SEARCHFOR)->ShowWindow (SW_HIDE);
    GetDlgItem (IDC_SEARCHPLAYER)->ShowWindow (SW_HIDE);
  }
  curTab = sel;
  *pResult = 0;
}

void CDotAReplayDlg::updateView (int mode)
{
  if (mode == ITEM_HEROCHART)
    loadDefault ();
  settings->ShowWindow (mode == ITEM_SETTINGS ? SW_SHOW : SW_HIDE);
  bool folder = (mode == ITEM_FOLDER || mode == ITEM_SEARCHRES);
  curfold.ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_BATCH)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_BATCHCOPY)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_BATCHHELP)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_EXPLORE)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_SEARCH)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_ETCHED)->ShowWindow (folder ? SW_SHOW : SW_HIDE);
  if (mode == ITEM_SEARCHRES)
  {
    GetDlgItem (IDC_EXPLORE)->EnableWindow (FALSE);
    GetDlgItem (IDC_SEARCH)->EnableWindow (FALSE);
  }
  else if (mode == ITEM_FOLDER)
  {
    GetDlgItem (IDC_EXPLORE)->EnableWindow (TRUE);
    GetDlgItem (IDC_SEARCH)->EnableWindow (TRUE);
  }
  for (int i = 0; i < NUM_TABS; i++)
    if (tabwnd[i])
      tabwnd[i]->ShowWindow (FALSE);
  if (tabwnd[0])
    tabwnd[0]->ShowWindow (mode == ITEM_REPLAY ? SW_SHOW : SW_HIDE);
  GetDlgItem (IDC_SEARCHTYPE)->ShowWindow (SW_HIDE);
  GetDlgItem (IDC_FINDNEXT)->ShowWindow (SW_HIDE);
  GetDlgItem (IDC_FINDPREV)->ShowWindow (SW_HIDE);
  GetDlgItem (IDC_FILTERS)->ShowWindow (SW_HIDE);
  GetDlgItem (IDC_SEARCHFOR)->ShowWindow (SW_HIDE);
  GetDlgItem (IDC_SEARCHPLAYER)->ShowWindow (SW_HIDE);
  curTab = 0;
  tabs.ShowWindow (mode == ITEM_REPLAY ? SW_SHOW : SW_HIDE);
  tabs.SetCurSel (0);
  viewMode = mode;
  stats->ShowWindow (mode == ITEM_STATS ? SW_SHOW : SW_HIDE);
  scrn->ShowWindow (mode == ITEM_SCREENSHOT ? SW_SHOW : SW_HIDE);
  hchart->ShowWindow (mode == ITEM_HEROCHART ? SW_SHOW : SW_HIDE);
  SetWindowText ("DotA Replay Manager");
}

struct FoundItem
{
  bool folder;
  char name[256];
  FoundItem () {}
  FoundItem (bool f, char const* n)
  {
    folder = f;
    strcpy (name, n);
  }
};
int __cdecl compItems (void const* va, void const* vb)
{
  FoundItem* a = (FoundItem*) va;
  FoundItem* b = (FoundItem*) vb;
  if (a->folder && !b->folder)
    return -1;
  if (!a->folder && b->folder)
    return 1;
  unsigned char* an = (unsigned char*) a->name;
  unsigned char* bn = (unsigned char*) b->name;
  while (*an && *bn)
  {
    if (*an >= '0' && *an <= '9' && *bn >= '0' && *bn <= '9')
    {
      int anum = 0, bnum = 0;
      while (*an >= '0' && *an <= '9')
        anum = anum * 10 + (*an++) - '0';
      while (*bn >= '0' && *bn <= '9')
        bnum = bnum * 10 + (*bn++) - '0';
      if (anum != bnum)
        return anum - bnum;
    }
    else
    {
      unsigned char ac = tolower (*an);
      unsigned char bc = tolower (*bn);
      if (ac != bc)
        return int (ac) - int (bc);
      an++;
      bn++;
    }
  }
  if (*an)
    return 1;
  else if (*bn)
    return -1;
  return 0;
}

static int FoundFiles;
extern int maxFiles;
extern bool showDetails;
extern int selColumns;

struct SearchInfo
{
  SearchStruct* ss;
  char prefix[256];
};

int CDotAReplayDlg::fillFolder (char const* path, int mode, void* lparam)
{
  WIN32_FIND_DATA data;
  HANDLE find = FindFirstFile (mprintf ("%s*", path), &data);
  BOOL result = (find != INVALID_HANDLE_VALUE);
  Array<FoundItem> lines;
  bool haveLevelUp = false;
  int numW3g = 0;
  while (result)
  {
    if (!strcmp (data.cFileName, ".."))
      haveLevelUp = true;
    else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && strcmp (data.cFileName, "."))
      lines.add (FoundItem (true, data.cFileName));
    else
    {
      char ext[256];
      _splitpath (data.cFileName, NULL, NULL, NULL, ext);
      if (!stricmp (ext, ".w3g"))
        lines.add (FoundItem (false, data.cFileName));
    }
    FoundFiles++;
    result = FindNextFile (find, &data);
  }
  FindClose (find);

  qsort (lines.ptr (), lines.getSize (), sizeof (FoundItem), compItems);
  if (mode == FILL_TREE)
  {
    HTREEITEM parent = (HTREEITEM) lparam;
    if (parent == NULL) parent = TVI_ROOT;
    for (int i = 0; i < lines.getSize (); i++)
    {
      if (lines[i].folder)
      {
        HTREEITEM cur = files.InsertItem (lines[i].name, getImageIndex ("IconClosedFolder"),
          getImageIndex ("IconOpenFolder"), parent);
        char npath[256];
        sprintf (npath, "%s%s\\", path, lines[i].name);
        int nw3g = 0;
        if (FoundFiles < maxFiles)
          nw3g = fillFolder (npath, FILL_TREE, cur);
        files.SetItemData (cur, (DWORD_PTR) (ITEM_FOLDER | treeStr.add (TreeItem (npath, cur))));
        files.SetItemText (cur, mprintf ("%s (%d)", lines[i].name, nw3g));
        numW3g += nw3g;
      }
      else
      {
        char title[256];
        _splitpath (lines[i].name, NULL, NULL, title, NULL);
        char npath[256];
        sprintf (npath, "%s%s", path, lines[i].name);
        HTREEITEM cur = files.InsertItem (title, getImageIndex ("IconReplay"), getImageIndex ("IconReplay"),
          parent);
        files.SetItemData (cur, (DWORD_PTR) (ITEM_REPLAY | treeStr.add (TreeItem (npath, cur))));
        numW3g++;
      }
    }
  }
  else if (mode == FILL_FOLDER)
  {
    curfold.DeleteAllItems ();
    int colSaved, colSize, colName, colRatio, colLength, colMode;
    if (showDetails)
    {
      SetWindowLong (curfold.m_hWnd, GWL_STYLE,
        (GetWindowLong (curfold.m_hWnd, GWL_STYLE) & (~(LVS_LIST | LVS_REPORT))) | LVS_REPORT);
      while (curfold.GetHeaderCtrl ()->GetItemCount ())
        curfold.DeleteColumn (0);
      curfold.InsertColumn (0, "Name");
      if (selColumns & COL_SAVED)
        colSaved = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Date saved");
      if (selColumns & COL_SIZE)
        colSize = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "File size", LVCFMT_RIGHT);
      if (selColumns & COL_NAME)
        colName = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game name");
      if (selColumns & COL_RATIO)
        colRatio = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game ratio");
      if (selColumns & COL_LENGTH)
        colLength = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game length", LVCFMT_RIGHT);
      if (selColumns & COL_MODE)
        colMode = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game mode", LVCFMT_RIGHT);
    }
    else
      SetWindowLong (curfold.m_hWnd, GWL_STYLE,
        (GetWindowLong (curfold.m_hWnd, GWL_STYLE) & (~(LVS_LIST | LVS_REPORT))) | LVS_LIST);
    viewPath = path;
    int base = 0;
    if (haveLevelUp)
    {
      curfold.SetItemData (curfold.InsertItem (0, "[Up One Level]", getImageIndex ("levelup")),
        (DWORD_PTR) ITEM_LEVELUP);
      base = 1;
    }
    GetDlgItem (IDC_BATCH)->ShowWindow (SW_HIDE);
    GetDlgItem (IDC_FOLDERLOAD)->ShowWindow (SW_SHOW);
    fload.SetRange (0, lines.getSize ());
    fload.SetPos (0);
    for (int i = 0; i < lines.getSize (); i++)
    {
      if (lines[i].folder)
        curfold.SetItemData (curfold.InsertItem (base + i, lines[i].name, getImageIndex ("IconClosedFolder")),
          (DWORD_PTR) ITEM_FOLDER);
      else
      {
        char title[256];
        _splitpath (lines[i].name, NULL, NULL, title, NULL);
        int pos = curfold.InsertItem (base + i, title, getImageIndex ("IconReplay"));
        curfold.SetItemData (pos, (DWORD_PTR) ITEM_REPLAY);
        if (showDetails)
        {
          char npath[256];
          sprintf (npath, "%s%s", path, lines[i].name);
          if (selColumns & COL_SAVED)
            curfold.SetItemText (pos, colSaved, fmtFileDate (npath));
          if (selColumns & COL_SIZE)
            curfold.SetItemText (pos, colSize, fmtFileSize (npath));
          if (selColumns & (COL_NAME | COL_RATIO | COL_LENGTH | COL_MODE))
          {
            int cpos = getGameInfo (npath, selColumns & GC_FLAGS);
            if (selColumns & COL_NAME)
              LVSetItemText (curfold, pos, colName, gcache[cpos].name);
            if (selColumns & COL_RATIO)
              curfold.SetItemText (pos, colRatio, gcache[cpos].ratio);
            if (selColumns & COL_LENGTH)
              curfold.SetItemText (pos, colLength, format_time (gcache[cpos].length, TIME_HOURS | TIME_SECONDS));
            if (selColumns & COL_MODE)
              curfold.SetItemText (pos, colMode, gcache[cpos].mode);
          }
        }
      }
      fload.SetPos (i + 1);
    }
    GetDlgItem (IDC_BATCH)->ShowWindow (SW_SHOW);
    GetDlgItem (IDC_FOLDERLOAD)->ShowWindow (SW_HIDE);
    if (showDetails)
      for (int i = 0; i < curfold.GetHeaderCtrl ()->GetItemCount (); i++)
        curfold.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
  }
  else if (mode == FILL_SEARCH)
  {
    SearchInfo* si = (SearchInfo*) lparam;
    static int colSaved, colSize, colName, colRatio, colLength, colMode;
    if (si->prefix[0] == 0)
    {
      curfold.DeleteAllItems ();
      SetDlgItemText (IDC_REPLAYPATH, "[Search results]");
    }
    if (si->prefix[0] == 0)
    {
      if (showDetails)
      {
        SetWindowLong (curfold.m_hWnd, GWL_STYLE,
          (GetWindowLong (curfold.m_hWnd, GWL_STYLE) & (~(LVS_LIST | LVS_REPORT))) | LVS_REPORT);
        while (curfold.GetHeaderCtrl ()->GetItemCount ())
          curfold.DeleteColumn (0);
        curfold.InsertColumn (0, "Name");
        if (selColumns & COL_SAVED)
          colSaved = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Date saved");
        if (selColumns & COL_SIZE)
          colSize = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "File size", LVCFMT_RIGHT);
        if (selColumns & COL_NAME)
          colName = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game name");
        if (selColumns & COL_RATIO)
          colRatio = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game ratio");
        if (selColumns & COL_LENGTH)
          colLength = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game length", LVCFMT_RIGHT);
        if (selColumns & COL_MODE)
          colMode = curfold.InsertColumn (curfold.GetHeaderCtrl ()->GetItemCount (), "Game mode", LVCFMT_RIGHT);
      }
      else
        SetWindowLong (curfold.m_hWnd, GWL_STYLE,
          (GetWindowLong (curfold.m_hWnd, GWL_STYLE) & (~(LVS_LIST | LVS_REPORT))) | LVS_LIST);
    }
    for (int i = 0; i < lines.getSize (); i++)
    {
      if (lines[i].folder)
      {
        char npath[256];
        sprintf (npath, "%s%s\\", path, lines[i].name);
        SearchInfo ni;
        ni.ss = si->ss;
        sprintf (ni.prefix, "%s%s/", si->prefix, lines[i].name);
        fillFolder (npath, FILL_SEARCH, &ni);
      }
      else
      {
        char npath[256];
        sprintf (npath, "%s%s", path, lines[i].name);
        if (matches (npath, si->ss))
        {
          char title[256];
          _splitpath (lines[i].name, NULL, NULL, title, NULL);
          int pos = curfold.InsertItem (curfold.GetItemCount (), mprintf ("%s%s", si->prefix, title),
            getImageIndex ("IconReplay"));
          curfold.SetItemData (pos, (DWORD_PTR) ITEM_REPLAY);
          if (showDetails)
          {
            if (selColumns & COL_SAVED)
              curfold.SetItemText (pos, colSaved, fmtFileDate (npath));
            if (selColumns & COL_SIZE)
              curfold.SetItemText (pos, colSize, fmtFileSize (npath));
            if (selColumns & (COL_NAME | COL_RATIO | COL_LENGTH | COL_MODE))
            {
              int cpos = getGameInfo (npath, selColumns & GC_FLAGS);
              if (selColumns & COL_NAME)
                LVSetItemText (curfold, pos, colName, gcache[cpos].name);
              if (selColumns & COL_RATIO)
                curfold.SetItemText (pos, colRatio, gcache[cpos].ratio);
              if (selColumns & COL_LENGTH)
                curfold.SetItemText (pos, colLength, format_time (gcache[cpos].length, TIME_HOURS | TIME_SECONDS));
              if (selColumns & COL_MODE)
                curfold.SetItemText (pos, colMode, gcache[cpos].mode);
            }
          }
        }
      }
    }
    if (showDetails)
      for (int i = 0; i < curfold.GetHeaderCtrl ()->GetItemCount (); i++)
        curfold.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
  }

  return numW3g;
}

int __cdecl compDates (void const* va, void const* vb)
{
  DateItem* a = (DateItem*) va;
  DateItem* b = (DateItem*) vb;
  __int64 da = a->time.GetTime ();
  __int64 db = b->time.GetTime ();
  if (da < db) return -1;
  if (da > db) return 1;
  return 0;
}
int CDotAReplayDlg::fillByDate (char const* path, HTREEITEM parent)
{
  Array<CString> lines;
  Array<DateItem> reps;
  batchCopy (path, lines, maxFiles);
  for (int i = 0; i < lines.getSize (); i++)
    reps.add (DateItem (lines[i], getFileDate (lines[i])));
  qsort (reps.ptr (), reps.getSize (), sizeof (DateItem), compDates);
  if (parent == NULL) parent = TVI_ROOT;
  int cur = 0;
  int count = 0;
  while (cur < reps.getSize ())
  {
    HTREEITEM item = files.InsertItem ("", getImageIndex ("IconClosedFolder"),
      getImageIndex ("IconOpenFolder"), parent);
    files.SetItemData (item, (DWORD_PTR) (ITEM_FOLDER |
      treeStr.add (TreeItem (reps[cur].time.Format ("|%Y"), item))));
    int pos = cur;
    int cnt = fillYear (reps, cur, reps[cur].time.GetYear (), item);
    files.SetItemText (item, mprintf ("%s (%d)", reps[pos].time.Format ("%Y"), cnt));
    count += cnt;
  }
  return count;
}
int CDotAReplayDlg::fillYear (Array<DateItem>& reps, int& cur, int year, HTREEITEM parent)
{
  int count = 0;
  while (cur < reps.getSize () && reps[cur].time.GetYear () == year)
  {
    HTREEITEM item = files.InsertItem ("", getImageIndex ("IconClosedFolder"),
      getImageIndex ("IconOpenFolder"), parent);
    files.SetItemData (item, (DWORD_PTR) (ITEM_FOLDER |
      treeStr.add (TreeItem (reps[cur].time.Format ("|%Y|%m"), item))));
    int pos = cur;
    int cnt = fillMonth (reps, cur, reps[cur].time.GetMonth (), item);
    files.SetItemText (item, mprintf ("%s (%d)", reps[pos].time.Format ("%B"), cnt));
    count += cnt;
  }
  return count;
}
int CDotAReplayDlg::fillMonth (Array<DateItem>& reps, int& cur, int month, HTREEITEM parent)
{
  int count = 0;
  __int64 day = CTime::GetCurrentTime ().GetTime () / (24 * 60 * 60);
  while (cur < reps.getSize () && reps[cur].time.GetMonth () == month)
  {
    HTREEITEM item = files.InsertItem ("", getImageIndex ("IconClosedFolder"),
      getImageIndex ("IconOpenFolder"), parent);
    files.SetItemData (item, (DWORD_PTR) (ITEM_FOLDER |
      treeStr.add (TreeItem (reps[cur].time.Format ("|%Y|%m|%d"), item))));
    int pos = cur;
    int cnt = fillDate (reps, cur, reps[cur].time.GetDay (), item);
    __int64 tday = reps[pos].time.GetTime () / (24 * 60 * 60);
    if (tday == day)
      files.SetItemText (item, mprintf ("Today, %s (%d)", reps[pos].time.Format ("%d %b %Y"), cnt));
    else if (tday == day - 1)
      files.SetItemText (item, mprintf ("Yesterday, %s (%d)", reps[pos].time.Format ("%d %b %Y"), cnt));
    else
      files.SetItemText (item, mprintf ("%s (%d)", reps[pos].time.Format ("%d %b %Y"), cnt));
    count += cnt;
  }
  return count;
}
int CDotAReplayDlg::fillDate (Array<DateItem>& reps, int& cur, int date, HTREEITEM parent)
{
  int cnt = 0;
  while (cur < reps.getSize () && reps[cur].time.GetDay () == date)
  {
    if (cur == 0 || reps[cur - 1].time.GetTime () != reps[cur].time.GetTime ())
    {
      char title[256];
      _splitpath (reps[cur].path, NULL, NULL, title, NULL);
      HTREEITEM item = files.InsertItem (mprintf ("%s %s", reps[cur].time.Format ("%H:%M:%S"), title),
        getImageIndex ("IconReplay"), getImageIndex ("IconReplay"), parent);
      files.SetItemData (item, (DWORD_PTR) (ITEM_REPLAY | treeStr.add (TreeItem (reps[cur].path, item))));
      cnt++;
    }
    cur++;
  }
  return cnt;
}

void CDotAReplayDlg::updateTreeView ()
{
  updating = true;
  Array<CString> openItems;
  for (int i = 0; i < treeStr.getSize (); i++)
    if (files.GetItemState (treeStr[i].id, TVIS_EXPANDED) & TVIS_EXPANDED)
      openItems.add (treeStr[i].path);
  CString selItem = "";
  if (files.GetSelectedItem () != NULL)
    selItem = treeStr[(int) files.GetItemData (files.GetSelectedItem ()) & (~ITEM_TYPE)].path;
  treeStr.clear ();
  files.DeleteAllItems ();
  HTREEITEM item = files.InsertItem ("Settings", getImageIndex ("IconSettings"), getImageIndex ("IconSettings"));
  files.SetItemData (item, (DWORD_PTR) (ITEM_SETTINGS | treeStr.add (TreeItem ("<settings>", item))));
  item = files.InsertItem ("Analyze screenshot", getImageIndex ("Screenshot"), getImageIndex ("Screenshot"));
  files.SetItemData (item, (DWORD_PTR) (ITEM_SCREENSHOT | treeStr.add (TreeItem ("[Screenshot]", item))));
  item = files.InsertItem ("Hero chart", getImageIndex ("BTNFelGuardBlue"), getImageIndex ("BTNFelGuardBlue"));
  files.SetItemData (item, (DWORD_PTR) (ITEM_HEROCHART | treeStr.add (TreeItem ("[Hero chart]", item))));
//  item = files.InsertItem ("Cupholder", getImageIndex ("coffee"), getImageIndex ("coffee"));
//  files.SetItemData (item, (DWORD_PTR) (ITEM_CUPHOLDER | treeStr.add (TreeItem ("[Cupholder]", item))));
  CString itemName = "";
  int pos = settings->replayPath.GetLength () - 2;
  while (pos >= 0 && settings->replayPath[pos] != '\\')
    itemName = settings->replayPath[pos--] + itemName;
  if (itemName == "")
    itemName = "Replays";
  else if (itemName[0] >= 'a' && itemName[0] <= 'z')
    itemName = char (itemName[0] - 'a' + 'A') + itemName.Mid (1);
  item = files.InsertItem (itemName, getImageIndex ("IconClosedFolder"), getImageIndex ("IconOpenFolder"));
  files.SetItemData (item, (DWORD_PTR) (ITEM_FOLDER | treeStr.add (TreeItem (settings->replayPath, item))));
  FoundFiles = 0;
  int count;
  if (IsDlgButtonChecked (IDC_BYDATE))
    count = fillByDate (settings->replayPath, item);
  else
    count = fillFolder (settings->replayPath, FILL_TREE, item);
  files.SetItemText (item, mprintf ("%s (%d)", (char const*) itemName, count));
  for (int i = 0; i < treeStr.getSize (); i++)
  {
    bool found = false;
    for (int j = 0; j < openItems.getSize () && !found; j++)
      if (!stricmp (openItems[j], treeStr[i].path))
        found = true;
    if (found)
      files.Expand (treeStr[i].id, TVE_EXPAND);
    if (treeStr[i].path == selItem)
      files.SelectItem (treeStr[i].id);
  }
  files.Expand (item, TVE_EXPAND);
  updating = false;
}

BOOL CDotAReplayDlg::OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  NMHDR* hdr = (NMHDR*) lParam;
  if (!updating)
  {
    if (hdr->hwndFrom == files.m_hWnd)
    {
      NMTREEVIEW* view = (NMTREEVIEW*) lParam;
      HTREEITEM item = view->itemNew.hItem;
      if (item != NULL)
      {
        switch (hdr->code)
        {
        case TVN_SELCHANGED:
          {
            int id = (int) files.GetItemData (item);
            if (treeStr[id & (~ITEM_TYPE)].path[0] != '|')
            {
              updateView (id & ITEM_TYPE);
              switch (id & ITEM_TYPE)
              {
              case ITEM_FOLDER:
                SetDlgItemText (IDC_REPLAYFILE, treeStr[id & (~ITEM_TYPE)].path);
                fillFolder (treeStr[id & (~ITEM_TYPE)].path, FILL_FOLDER);
                files.SetFocus ();
                break;
              case ITEM_REPLAY:
                SetDlgItemText (IDC_REPLAYFILE, treeStr[id & (~ITEM_TYPE)].path);
                OpenReplay ();
                files.SetFocus ();
                break;
              case ITEM_SCREENSHOT:
                scrn->refresh ();
                break;
              case ITEM_HEROCHART:
                hchart->refresh ();
                break;
              //case ITEM_CUPHOLDER:
              //  {
              //    for (char cd = 'A'; cd <= 'Z'; cd++)
              //    {
              //      if (GetDriveType (mprintf ("%c:\\", cd)) == DRIVE_CDROM)
              //      {
              //        char* dname = mprintf ("\\\\.\\%c:", cd);
              //        HANDLE hDevice = CreateFile (dname, GENERIC_READ | GENERIC_WRITE,
              //          FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
              //          FILE_ATTRIBUTE_NORMAL, 0);
              //        if (hDevice == INVALID_HANDLE_VALUE)
              //        {
              //          SetLastError (NO_ERROR);
              //          hDevice = CreateFile (dname, GENERIC_READ,
              //            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
              //            FILE_ATTRIBUTE_NORMAL, 0);
              //        }
              //        if (hDevice != INVALID_HANDLE_VALUE && GetLastError () == NO_ERROR)
              //        {
              //          DWORD b;
              //          DeviceIoControl (hDevice, IOCTL_STORAGE_EJECT_MEDIA, 0, 0, 0, 0, &b, 0);
              //        }
              //        if (hDevice != INVALID_HANDLE_VALUE)
              //          CloseHandle (hDevice);
              //      }
              //    }
              //  }
              //  break;
              }
            }
          }
          break;
        }
      }
    }
  }
  *pResult = 0;
  return CDialog::OnNotify (wParam, lParam, pResult);
}

void CDotAReplayDlg::OnLvnItemActivateFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int item = curfold.GetNextItem (-1, LVNI_SELECTED);
  if (item >= 0)
  {
    bool folder = (int (curfold.GetItemData (item)) & ITEM_TYPE) == ITEM_FOLDER;
    switch (int (curfold.GetItemData (item)) & ITEM_TYPE)
    {
    case ITEM_FOLDER:
      SetDlgItemText (IDC_REPLAYFILE, curfold.GetItemText (item, 0) + "\\");
      break;
    case ITEM_LEVELUP:
      SetDlgItemText (IDC_REPLAYFILE, "..\\");
      break;
    case ITEM_REPLAY:
      SetDlgItemText (IDC_REPLAYFILE, curfold.GetItemText (item, 0) + ".w3g");
      break;
    }
    OnBnClickedParsereplay ();
  }
  *pResult = 0;
}

void CDotAReplayDlg::OnTimer (UINT_PTR nIDEvent)
{
  CString path = settings->replayPath + "LastReplay.w3g";
  __int64 curt = getFileDate (path);
  if (curt > lastDate)
  {
    OnBnClickedRefresh ();
    if (settings->autoView ())
    {
      SetDlgItemText (IDC_REPLAYFILE, path);
      OnBnClickedParsereplay ();
      TrayNotify ("New replay", w3g->game.name);
      if (settings->autoCopy () && w3g->numPlayers > 0)
        copyReplay (path, settings->replayPath, settings->getCopyName (w3g, path));
    }
    else if (settings->autoCopy ())
    {
      FILE* file = fopen (path, "rb");
      if (file != NULL)
      {
        W3GReplay* rep = new W3GReplay;
        TrayNotify ("New replay", rep->game.name);
        if (rep->load (file, true))
        {
          rep->setLocation (path);
          addGame (w3g, path);
          rep->readTime (path);
          copyReplay (path, settings->replayPath, settings->getCopyName (rep, path));
          updateTreeView ();
        }
        delete rep;
        fclose (file);
      }
    }
    lastDate = curt;
  }
}

void CDotAReplayDlg::OnBnClickedBatchhelp()
{
  settings->OnBnClickedHelpformat ();
}

void CDotAReplayDlg::batchCopy (char const* path, Array<CString>& files, int limit)
{
  WIN32_FIND_DATA data;
  HANDLE find = FindFirstFile (mprintf ("%s*", path), &data);
  BOOL result = (find != INVALID_HANDLE_VALUE);
  int cnt = 0;
  while (result)
  {
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
      strcmp (data.cFileName, ".") && strcmp (data.cFileName, ".."))
    {
      char npath[512];
      sprintf (npath, "%s%s\\", path, data.cFileName);
      batchCopy (npath, files, limit < 0 ? limit : limit - cnt);
    }
    else
    {
      char ext[256];
      _splitpath (data.cFileName, NULL, NULL, NULL, ext);
      if (!stricmp (ext, ".w3g"))
        files.add (mprintf ("%s%s", path, data.cFileName));
    }
    cnt++;
    if (limit >= 0 && cnt >= limit)
      break;
    result = FindNextFile (find, &data);
  }
  FindClose (find);
}

#include "batchdlg.h"

void CDotAReplayDlg::OnBnClickedBatchcopy()
{
  int sel = curfold.GetNextItem (-1, LVNI_SELECTED);
  char fmt[1024];
  GetDlgItemText (IDC_BATCH, fmt, 1024);
  Array<CString> files;
  if (sel < 0)
    batchCopy (viewPath, files);
  else
  {
    while (sel >= 0)
    {
      bool folder = (int (curfold.GetItemData (sel)) & ITEM_TYPE) == ITEM_FOLDER;
      if (folder && curfold.GetItemText (sel, 0) != "..")
        batchCopy (viewPath + curfold.GetItemText (sel, 0) + "\\", files);
      else
        files.add (viewPath + curfold.GetItemText (sel, 0) + ".w3g");
      sel = curfold.GetNextItem (sel, LVNI_SELECTED);
    }
  }
  CBatchDlg dlg (&files, fmt, backingUp ? settings->replayPath : viewPath, settings, BATCH_PROCESS);
  dlg.DoModal ();
  OnBnClickedParsereplay ();
  updateTreeView ();
}

void CDotAReplayDlg::OnBnClickedExplore()
{
  ShellExecute (NULL, "explore", viewPath, NULL, NULL, SW_SHOWNORMAL);
}

void CDotAReplayDlg::OnBnClickedBrowsereplay()
{
  CString curPath;
  GetDlgItemText (IDC_REPLAYFILE, curPath);
  CFileDialog dlg (TRUE, ".w3g", NULL, 0,
    "Warcraft III Replay Files (*.w3g)|*.w3g|All Files (*.*)|*.*||", this, sizeof (OPENFILENAME));
  dlg.GetOFN ().lpstrInitialDir = curPath;
  if (dlg.DoModal () == IDOK)
  {
    SetDlgItemText (IDC_REPLAYFILE, dlg.GetPathName ());
    OnBnClickedParsereplay ();
  }
}

void CDotAReplayDlg::showPlayerInfo (int id)
{
  tabs.SetCurSel (TAB_PLAYERS);
  ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->selectPlayer (id);
  LRESULT pr;
  OnTcnSelchangeTabs (NULL, &pr);
}

void CDotAReplayDlg::showPlayerActions (int id)
{
  tabs.SetCurSel (TAB_ACTIONS);
  ((CActionsDlg*) tabwnd[TAB_ACTIONS])->selectPlayer (id);
  LRESULT pr;
  OnTcnSelchangeTabs (NULL, &pr);
}

void CDotAReplayDlg::selPlayerInfo (int id)
{
  if (tabwnd[TAB_PLAYERS])
    ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->selectPlayer (id);
}

void CDotAReplayDlg::selPlayerActions (int id)
{
  if (tabwnd[TAB_ACTIONS])
    ((CActionsDlg*) tabwnd[TAB_ACTIONS])->selectPlayer (id);
}

void CDotAReplayDlg::OnBnClickedRefresh()
{
  updateTreeView ();
}

HDROP CDotAReplayDlg::getCopiedFiles (COleDataObject* pDataObject)
{
  if (pDataObject == NULL)
  {
    if (!IsClipboardFormatAvailable (CF_HDROP))
      return NULL;
    if (!OpenClipboard ())
      return NULL;
    HDROP hdrop = (HDROP) GetClipboardData (CF_HDROP);
    CloseClipboard ();
    return hdrop;
  }
  else
    return (HDROP) pDataObject->GetGlobalData (CF_HDROP);
}

#define DEL_ASK         0
#define DEL_ASK_MANY    1
#define DEL_DONT_ASK    2

void CDotAReplayDlg::OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  bool many = curfold.GetSelectedCount () > 1;
  int sel = curfold.GetNextItem (-1, LVNI_SELECTED);
  CPoint pt;
  GetCursorPos (&pt);
  if (sel >= 0)
  {
    int item = int (curfold.GetItemData (sel)) & ITEM_TYPE;
    if (many)
    {
      while (sel >= 0 && item != ITEM_FOLDER && item != ITEM_REPLAY)
      {
        sel = curfold.GetNextItem (sel, LVNI_SELECTED);
        item = int (curfold.GetItemData (sel)) & ITEM_TYPE;
      }
    }
    fileMenu.EnableMenuItem (ID_FILEM_OPEN, many ? MF_GRAYED : MF_ENABLED);
    if (item == ITEM_FOLDER || item == ITEM_REPLAY)
    {
      fileMenu.EnableMenuItem (ID_FILEM_BACKUP, MF_ENABLED);
      fileMenu.EnableMenuItem (ID_FILEM_CUT, MF_ENABLED);
      fileMenu.EnableMenuItem (ID_FILEM_COPY, MF_ENABLED);
      fileMenu.EnableMenuItem (ID_FILEM_DELETE, MF_ENABLED);
      fileMenu.EnableMenuItem (ID_FILEM_RENAME, many ? MF_GRAYED : MF_ENABLED);
    }
    else
    {
      fileMenu.EnableMenuItem (ID_FILEM_BACKUP, MF_GRAYED);
      fileMenu.EnableMenuItem (ID_FILEM_CUT, MF_GRAYED);
      fileMenu.EnableMenuItem (ID_FILEM_COPY, MF_GRAYED);
      fileMenu.EnableMenuItem (ID_FILEM_DELETE, MF_GRAYED);
      fileMenu.EnableMenuItem (ID_FILEM_RENAME, MF_GRAYED);
    }
  }
  else
  {
    fileMenu.EnableMenuItem (ID_FILEM_BACKUP, MF_GRAYED);
    fileMenu.EnableMenuItem (ID_FILEM_OPEN, MF_GRAYED);
    fileMenu.EnableMenuItem (ID_FILEM_CUT, MF_GRAYED);
    fileMenu.EnableMenuItem (ID_FILEM_COPY, MF_GRAYED);
    fileMenu.EnableMenuItem (ID_FILEM_DELETE, MF_GRAYED);
    fileMenu.EnableMenuItem (ID_FILEM_RENAME, MF_GRAYED);
  }
  fileMenu.EnableMenuItem (ID_FILEM_PASTE, getCopiedFiles () != NULL ? MF_ENABLED : MF_GRAYED);
  int res = fileMenu.TrackPopupMenuEx (TPM_HORIZONTAL | TPM_LEFTALIGN | TPM_RETURNCMD,
    pt.x, pt.y, this, NULL);
  switch (res)
  {
  case ID_FILEM_OPEN:
    OnLvnItemActivateFilelist (NULL, pResult);
    break;
  case ID_FILEM_RENAME:
    curfold.EditLabel (sel);
    break;
  case ID_FILEM_DELETE:
    {
      int sel = curfold.GetNextItem (-1, LVNI_SELECTED);
      Array<char> list;
      while (sel >= 0)
      {
        int item = (int) curfold.GetItemData (sel);
        CString cur;
        if (item == ITEM_FOLDER)
          cur = viewPath + curfold.GetItemText (sel, 0);
        else
          cur = viewPath + curfold.GetItemText (sel, 0) + ".w3g";
        list.append (cur, cur.GetLength () + 1);
        sel = curfold.GetNextItem (sel, LVNI_SELECTED);
      }
      list.add (0);
      list.add (0);
      SHFILEOPSTRUCT fileop;
      memset (&fileop, 0, sizeof fileop);
      fileop.wFunc = FO_DELETE;
      fileop.pFrom = list.ptr ();
      SHFileOperation (&fileop);
      OnBnClickedParsereplay ();
      updateTreeView ();
    }
    break;
  case ID_FILEM_BACKUP:
    {
      CString prev;
      GetDlgItemText (IDC_BATCH, prev);
      SetDlgItemText (IDC_BATCH, settings->getBatchFormat ());
      backingUp = true;
      OnBnClickedBatchcopy ();
      backingUp = false;
      SetDlgItemText (IDC_BATCH, prev);
    }
    break;
  case ID_FILEM_FOLDER:
    {
      char path[256] = "New Folder";
      int digit = 1;
      while (PathFileExists (viewPath + path))
        sprintf (path, "New Folder (%d)", digit++);
      if (CreateDirectory (viewPath + path, NULL))
      {
        int pos = curfold.InsertItem (curfold.GetItemCount (), path, getImageIndex ("IconClosedFolder"));
        curfold.SetItemData (pos, (DWORD_PTR) ITEM_FOLDER);
        curfold.EditLabel (pos);
        updateTreeView ();
      }
    }
    break;
  case ID_FILEM_PASTE:
    {
      HDROP drop = getCopiedFiles ();
      pasteFiles (drop);
    }
    break;
  case ID_FILEM_COPY:
    {
      if (OpenClipboard ())
      {
        EmptyClipboard ();
        HGLOBAL handle = copyFiles ();
        if (handle)
          SetClipboardData (CF_HDROP, (HANDLE) handle);
        CloseClipboard ();
      }
    }
    break;
  }
  *pResult = 0;
}

HGLOBAL CDotAReplayDlg::copyFiles (int* list, int* count)
{
  if (count && list)
    *count = 0;
  int size = sizeof (DROPFILES) + 1;
  int pathlen = viewPath.GetLength ();
  int cur = curfold.GetNextItem (-1, LVNI_SELECTED);
  while (cur >= 0)
  {
    if (count && list)
      list[(*count)++] = cur;
    int item = (int) curfold.GetItemData (cur);
    if (item == ITEM_FOLDER)
      size += pathlen + curfold.GetItemText (cur, 0).GetLength () + 1;
    else
      size += pathlen + curfold.GetItemText (cur, 0).GetLength () + 5;
    cur = curfold.GetNextItem (cur, LVNI_SELECTED);
  }
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, size);
  if (handle)
  {
    char* buf = (char*) GlobalLock (handle);
    memset (buf, 0, size);
    DROPFILES* df = (DROPFILES*) buf;
    df->pFiles = sizeof (DROPFILES);
    int cur = curfold.GetNextItem (-1, LVNI_SELECTED);
    int pos = (int) sizeof (DROPFILES);
    while (cur >= 0)
    {
      int item = (int) curfold.GetItemData (cur);
      if (item == ITEM_FOLDER)
      {
        strcpy (buf + pos, viewPath + curfold.GetItemText (cur, 0));
        pos += pathlen + curfold.GetItemText (cur, 0).GetLength () + 1;
      }
      else
      {
        strcpy (buf + pos, viewPath + curfold.GetItemText (cur, 0) + ".w3g");
        pos += pathlen + curfold.GetItemText (cur, 0).GetLength () + 5;
      }
      cur = curfold.GetNextItem (cur, LVNI_SELECTED);
    }
    GlobalUnlock (handle);
    return handle;
  }
  return NULL;
}

void CDotAReplayDlg::pasteFiles (HDROP drop, bool move)
{
  int count = DragQueryFile (drop, -1, NULL, 0);
  Array<char> list;
  char path[512];
  for (int i = 0; i < count; i++)
  {
    DragQueryFile (drop, i, path, 512);
    list.append (path, (int) strlen (path) + 1);
  }
  list.add (0);
  list.add (0);
  SHFILEOPSTRUCT fileop;
  memset (&fileop, 0, sizeof fileop);
  fileop.wFunc = move ? FO_MOVE : FO_COPY;
  fileop.pFrom = list.ptr ();
  fileop.pTo = viewPath;
  SHFileOperation (&fileop);
  OnBnClickedParsereplay ();
  updateTreeView ();
}

void CDotAReplayDlg::OnLvnEndlabeleditFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  int sel = pDispInfo->item.iItem;
  int item = int (curfold.GetItemData (sel)) & ITEM_TYPE;
  *pResult = FALSE;
  if (pDispInfo->item.pszText != NULL)
  {
    CString oldn = viewPath + curfold.GetItemText (sel, 0);
    CString newn = viewPath + pDispInfo->item.pszText;
    if (item == ITEM_REPLAY)
    {
      oldn += ".w3g";
      newn += ".w3g";
    }
    else if (item == ITEM_FOLDER)
    {
      oldn += "\\";
      newn += "\\";
    }
    if (item == ITEM_REPLAY || item == ITEM_FOLDER)
    {
      int res = rename (oldn, newn);
      if (res == 0)
      {
        *pResult = TRUE;
        for (int i = 0; i < treeStr.getSize (); i++)
        {
          if (!stricmp (oldn, treeStr[i].path))
          {
            treeStr[i].path = newn;
            files.SetItemText (treeStr[i].id, pDispInfo->item.pszText);
            break;
          }
        }
      }
      else if (errno == 13)
        MessageBox ("A file with the name you specified already exists.", "Error", MB_OK | MB_ICONHAND);
      else if (errno == 2)
      {
        MessageBox ("Failed to rename the file.", "Error", MB_OK | MB_ICONHAND);
        OnBnClickedParsereplay ();
        updateTreeView ();
      }
      else if (errno == 22)
        MessageBox ("File name cannot contain any of the following characters:\n"
          "\\/:*?\"<>|", "Error", MB_OK | MB_ICONHAND);
    }
  }
}

void CDotAReplayDlg::OnSize (UINT nType, int cx, int cy)
{
  if (settings->useTray ())
  {
    if (nType == SIZE_MINIMIZED)
    {
      ShowWindow (SW_HIDE);
      CreateTrayIcon (::AfxGetInstanceHandle (), m_hWnd);
      char buf[256];
      GetWindowText (buf, sizeof buf);
      SetTrayText ("%s", buf);
    }
    else
      DeleteTrayIcon ();
  }
  if (loc.Update ())
  {
    GetDlgItem (IDC_BROWSEREPLAY)->Invalidate ();
    GetDlgItem (IDC_PARSEREPLAY)->Invalidate ();
    GetDlgItem (IDC_EXPLORE)->Invalidate ();
    GetDlgItem (IDC_SEARCH)->Invalidate ();
    GetDlgItem (IDC_BATCHCOPY)->Invalidate ();
    GetDlgItem (IDC_BATCHHELP)->Invalidate ();
    WINDOWPLACEMENT pl;
    GetWindowPlacement (&pl);
    if (pl.showCmd == SW_SHOWNORMAL)
      pl.showCmd = SW_SHOW;
    if (pl.showCmd == SW_MAXIMIZE || pl.showCmd == SW_SHOW)
      reg.writeInt ("wndShow", pl.showCmd);
    reg.writeInt ("wndX", pl.rcNormalPosition.left);
    reg.writeInt ("wndY", pl.rcNormalPosition.top);
    reg.writeInt ("wndWidth", pl.rcNormalPosition.right - pl.rcNormalPosition.left);
    reg.writeInt ("wndHeight", pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
  }
  tabloc.Update ();
}
void CDotAReplayDlg::OnMove (int x, int y)
{
  OnSize (-1, 0, 0);
}

LRESULT CDotAReplayDlg::OnTrayNotify (WPARAM wParam, LPARAM lParam)
{
  switch (lParam)
  {
  case WM_LBUTTONUP:
    ShowWindow (SW_SHOW);
    ShowWindow (SW_RESTORE);
    break;
  case WM_RBUTTONUP:
    {
      POINT pt;
      GetCursorPos (&pt);
      SetForegroundWindow ();
      int res = trayMenu.TrackPopupMenuEx (TPM_HORIZONTAL | TPM_LEFTALIGN | TPM_RETURNCMD,
        pt.x, pt.y, this, NULL);
      switch (res)
      {
      case ID_TRAY_OPEN:
        ShowWindow (SW_SHOW);
        ShowWindow (SW_RESTORE);
        break;
      case ID_TRAY_EXIT:
        DestroyWindow ();
        break;
      }
    }
    break;
  }
  return 0;
}

//void CDotAReplayDlg::OnDropFiles (HDROP hDropInfo)
//{
//  if (curfold.IsWindowVisible ())
//  {
//    CRect rc;
//    curfold.GetWindowRect (rc);
//    CPoint pt;
//    DragQueryPoint (hDropInfo, &pt);
//    if (rc.PtInRect (pt))
//    {
//      pasteFiles (hDropInfo);
//      return;
//    }
//  }
//  int count = DragQueryFile (hDropInfo, -1, NULL, 0);
//  if (count <= 0)
//    return;
//  char path[512];
//  DragQueryFile (hDropInfo, 0, path, 512);
//  SetDlgItemText (IDC_REPLAYFILE, path);
//  OnBnClickedParsereplay ();
//}

void CDotAReplayDlg::OnBnClickedSearch()
{
  CSearchDlg dlg;
  if (dlg.DoModal () == IDOK)
  {
    SearchInfo si;
    si.ss = &dlg.ss;
    si.prefix[0] = 0;
    fillFolder (viewPath, FILL_SEARCH, &si);
    updateView (ITEM_SEARCHRES);
    files.SelectItem (NULL);
  }
}

void CDotAReplayDlg::OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
  int sel = curfold.GetNextItem (-1, LVNI_SELECTED);
  if (pLVKeyDow->wVKey == 'C' && GetAsyncKeyState (VK_CONTROL) && sel >= 0)
  {
    if (OpenClipboard ())
    {
      EmptyClipboard ();
      int size = sizeof (DROPFILES) + 1;
      int pathlen = viewPath.GetLength ();
      int cur = curfold.GetNextItem (-1, LVNI_SELECTED);
      while (cur >= 0)
      {
        int item = (int) curfold.GetItemData (cur);
        if (item == ITEM_FOLDER)
          size += pathlen + curfold.GetItemText (cur, 0).GetLength () + 1;
        else
          size += pathlen + curfold.GetItemText (cur, 0).GetLength () + 5;
        cur = curfold.GetNextItem (cur, LVNI_SELECTED);
      }
      HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, size);
      if (handle)
      {
        char* buf = (char*) GlobalLock (handle);
        memset (buf, 0, size);
        DROPFILES* df = (DROPFILES*) buf;
        df->pFiles = sizeof (DROPFILES);
        int cur = curfold.GetNextItem (-1, LVNI_SELECTED);
        int pos = (int) sizeof (DROPFILES);
        while (cur >= 0)
        {
          int item = (int) curfold.GetItemData (cur);
          if (item == ITEM_FOLDER)
          {
            strcpy (buf + pos, viewPath + curfold.GetItemText (cur, 0));
            pos += pathlen + curfold.GetItemText (cur, 0).GetLength () + 1;
          }
          else if (item == ITEM_REPLAY)
          {
            strcpy (buf + pos, viewPath + curfold.GetItemText (cur, 0) + ".w3g");
            pos += pathlen + curfold.GetItemText (cur, 0).GetLength () + 5;
          }
          cur = curfold.GetNextItem (cur, LVNI_SELECTED);
        }
        GlobalUnlock (handle);
        SetClipboardData (CF_HDROP, (HANDLE) handle);
      }
      CloseClipboard ();
    }
  }
  else if (pLVKeyDow->wVKey == 'V' && GetAsyncKeyState (VK_CONTROL))
  {
    HDROP drop = getCopiedFiles ();
    pasteFiles (drop);
  }
  else if (pLVKeyDow->wVKey == VK_DELETE && sel >= 0)
  {
    Array<char> list;
    while (sel >= 0)
    {
      int item = (int) curfold.GetItemData (sel);
      CString cur;
      if (item == ITEM_FOLDER)
        cur = viewPath + curfold.GetItemText (sel, 0);
      else if (item == ITEM_REPLAY)
        cur = viewPath + curfold.GetItemText (sel, 0) + ".w3g";
      list.append (cur, cur.GetLength () + 1);
      sel = curfold.GetNextItem (sel, LVNI_SELECTED);
    }
    list.add (0);
    list.add (0);
    SHFILEOPSTRUCT fileop;
    memset (&fileop, 0, sizeof fileop);
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = list.ptr ();
    SHFileOperation (&fileop);
    OnBnClickedParsereplay ();
    updateTreeView ();
  }
  else if (pLVKeyDow->wVKey == VK_F2 && sel >= 0)
  {
    curfold.EditLabel (sel);
  }
  else if (pLVKeyDow->wVKey == VK_RETURN)
  {
    OnLvnItemActivateFilelist (NULL, pResult);
  }
  *pResult = 0;
}

void CDotAReplayDlg::showPlayerStats (char const* name)
{
  SetDlgItemText (IDC_REPLAYPATH, mprintf ("Player:%s", name));
  OnBnClickedParsereplay ();
}

void CDotAReplayDlg::showReplay (char const* path)
{
  SetDlgItemText (IDC_REPLAYPATH, path);
  OnBnClickedParsereplay ();
}

void CDotAReplayDlg::cacheAll ()
{
  Array<CString> files;
  batchCopy (settings->replayPath, files);
  CBatchDlg dlg (&files, NULL, NULL, settings, BATCH_CACHE);
  dlg.DoModal ();
}

void CDotAReplayDlg::OnBnClickedBydate()
{
  updateTreeView ();
  reg.writeInt ("byDate", IsDlgButtonChecked (IDC_BYDATE));
}

void CDotAReplayDlg::showGamePlayers (W3GReplay* w3g)
{
  updateView (ITEM_SCREENSHOT);
  scrn->fromGame (w3g);
}

void CDotAReplayDlg::modifyTabs (bool dota)
{
  tabs.DeleteAllItems ();
  if (dota)
  {
    tabs.InsertItem (TAB_GAME, "Game Info");
    tabs.InsertItem (TAB_CHAT, "Game Chat");
    tabs.InsertItem (TAB_TIMELINE, "Timeline");
    tabs.InsertItem (TAB_PLAYERS, "Builds");
    tabs.InsertItem (TAB_ACTIONS, "Actions");
    tabs.InsertItem (TAB_GOLDGRAPH, "Gold timeline");
    tabs.InsertItem (TAB_EXPGRAPH, "Exp timeline");
    tabs.InsertItem (TAB_PRESENT, "Presentation");
    tabs.InsertItem (TAB_NEWPRESENT, "ExtPresent");
    tabs.InsertItem (TAB_DRAFT, "Draft");
    tabs.InsertItem (TAB_ACTIONLOG, "Action Log");
  }
  else
  {
    tabs.InsertItem (0, "Game Info");
    tabs.InsertItem (1, "Game Chat");
    tabs.InsertItem (2, "Actions");
    tabs.InsertItem (3, "Action Log");
  }
}

const char projectURL[256] = "http://www.rivsoft.narod.ru/dotareplay.html";

void CAboutDlg::OnBnClickedWebsite()
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

void CDotAReplayDlg::OnGetMinMaxInfo (MINMAXINFO* lpMMI)
{
  if (initSize.x >= 0)
    lpMMI->ptMinTrackSize = initSize;
}

BOOL CDotAReplayDlg::OnCopyData (CWnd* pWnd, COPYDATASTRUCT* cd)
{
  if (cd->dwData == 1257)
  {
    CString str = (char*) cd->lpData;
    if (str [0] == '\"')
      str = str.Mid (1, str.GetLength () - 2);
    SetDlgItemText (IDC_REPLAYFILE, str);
    OnBnClickedParsereplay ();
    return TRUE;
  }
  return FALSE;
}

void CDotAReplayDlg::OnBnClickedFindnext()
{
  bool findprev = (LOWORD (GetCurrentMessage ()->wParam) == IDC_FINDPREV);
  static wchar_t searchFor[512];
  static char notifyMatch[64];
  if (w3g && w3g->chat.getSize ())
  {
    int searchType = ((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->GetCurSel ();
    int playerId;
    int searchLen;
    if (searchType < 3)
    {
      GetDlgItemTextW (m_hWnd, IDC_SEARCHFOR, searchFor, sizeof searchFor);
      searchLen = (int) wcslen (searchFor);
    }
    else
    {
      int sel = searchPlayer.GetCurSel ();
      playerId = int (searchPlayer.GetItemDataEx (sel));
      if (playerId >= 0)
        sprintf (notifyMatch, "@%d@", playerId);
      else if (playerId == -1)
        strcpy (notifyMatch, "@s@");
      else if (playerId == -2)
        strcpy (notifyMatch, "@u@");
      else if (playerId == -3)
        strcpy (notifyMatch, "@n@");
    }
    long selStart, selEnd;
    chatbox.GetSel (selStart, selEnd);
    long curLine = chatbox.LineFromChar (selStart);
    int curChat;
    int firstLine = -1;
    if (findprev)
    {
      for (curChat = w3g->chat.getSize () - 1; curChat >= 0; curChat--)
      {
        if (w3g->chat[curChat].line < curLine)
          break;
        if (firstLine < 0 && w3g->chat[curChat].line >= 0)
          firstLine = curChat;
      }
    }
    else
    {
      for (curChat = 0; curChat < w3g->chat.getSize (); curChat++)
      {
        if (w3g->chat[curChat].line > curLine)
          break;
        if (firstLine < 0 && w3g->chat[curChat].line >= 0)
          firstLine = curChat;
      }
    }
    if (curChat >= w3g->chat.getSize ())
      curChat = firstLine;
    if (curChat < 0)
      return;
    int searchStart = curChat;
    int searchCount = 0;
    while (searchCount == 0 || curChat != searchStart)
    {
      if (w3g->chat[curChat].line >= 0)
      {
        if (searchType == 4)
        {
          if (w3g->chat[curChat].mode == CHAT_NOTIFY &&
              strstr (w3g->chat[curChat].text, notifyMatch))
            break;
        }
        else if (searchType == 3)
        {
          if (w3g->chat[curChat].id == playerId && w3g->chat[curChat].mode != CHAT_NOTIFY)
            break;
        }
        else if (searchType == 0)
        {
          if (wcsistr (w3g->chat[curChat].utext, searchFor) != NULL)
            break;
        }
        else if (searchType == 1)
        {
          if (!wcsicmp (w3g->chat[curChat].utext, searchFor))
            break;
        }
        else if (searchType == 2)
        {
          if (!wcsnicmp (w3g->chat[curChat].utext, searchFor, searchLen))
            break;
        }
      }
      curChat += (findprev ? -1 : 1);
      if (curChat < 0) curChat = w3g->chat.getSize () - 1;
      if (curChat >= w3g->chat.getSize ()) curChat = 0;
      searchCount++;
    }
    if (searchCount != 0 && curChat == searchStart)
      MessageBox ("Nothing found!");
    else
    {
      int line = w3g->chat[curChat].line;
      int chra = chatbox.LineIndex (line);
      int chrb = chatbox.LineIndex (line + 1) - 1;
      if (chrb < chra) chrb = chra;
      chatbox.SetSel (chra, chrb);
      chatbox.SetFocus ();
    }
  }
}

void CDotAReplayDlg::OnBnClickedFilters()
{
  CChatFilters filter (chatFilters, !w3g->dota.isDota, this);
  if (filter.DoModal () == IDOK)
  {
    _chatFilters = chatFilters = filter.getFilters ();
    updateChat ();
  }
}

void CDotAReplayDlg::OnCbnSelchangeSearchtype()
{
  int type = ((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->GetCurSel ();
  int curtab = tabs.GetCurSel ();
  if (w3g != NULL && !w3g->dota.isDota)
    curtab = ladderTabs[curtab];
  if (curtab == TAB_CHAT)
  {
    GetDlgItem (IDC_SEARCHFOR)->ShowWindow (type < 3 ? SW_SHOW : SW_HIDE);
    GetDlgItem (IDC_SEARCHPLAYER)->ShowWindow (type >= 3 ? SW_SHOW : SW_HIDE);
  }
}

void CDotAReplayDlg::OnLvnBegindragFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = (LPNMLISTVIEW) pNMHDR;
  static int movedItems[4096];
  int numMovedItems;
  HGLOBAL handle = copyFiles (movedItems, &numMovedItems);
  if (handle)
  {
    COleDropSource source;
    COleDataSource data;
    data.CacheGlobalData (CF_HDROP, handle);
    DROPEFFECT de = data.DoDragDrop (DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
    if (de == DROPEFFECT_MOVE)
    {
      for (int i = numMovedItems - 1; i >= 0; i--)
      {
        int sel = movedItems[i];
        int item = (int) curfold.GetItemData (sel);
        CString cur;
        if (item == ITEM_FOLDER)
          cur = viewPath + curfold.GetItemText (sel, 0);
        else
          cur = viewPath + curfold.GetItemText (sel, 0) + ".w3g";
        DeleteFile (cur);
        curfold.DeleteItem (sel);
      }
    }
    else
    {
      for (int i = numMovedItems - 1; i >= 0; i--)
      {
        int sel = movedItems[i];
        int item = (int) curfold.GetItemData (sel);
        CString cur;
        if (item == ITEM_FOLDER)
          cur = viewPath + curfold.GetItemText (sel, 0);
        else
          cur = viewPath + curfold.GetItemText (sel, 0) + ".w3g";
        if (!PathFileExists (cur))
          curfold.DeleteItem (sel);
      }
    }
  }
  *pResult = 0;
}
DROPEFFECT CDotAReplayDlg::OnDragOver (COleDataObject* pDataObject,
  DWORD dwKeyState, CPoint point)
{
  if (pDataObject->GetGlobalData (CF_HDROP) == NULL)
    return DROPEFFECT_NONE;
  ClientToScreen (&point);
  CRect rc;
  curfold.GetWindowRect (rc);
  if (rc.PtInRect (point))
  {
    if (dwKeyState &  MK_CONTROL)
      return DROPEFFECT_COPY;
    else
      return DROPEFFECT_MOVE;
  }
  else
    return DROPEFFECT_COPY;
}
DROPEFFECT CDotAReplayDlg::OnDropEx (COleDataObject* pDataObject,
  DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
  HDROP drop = getCopiedFiles (pDataObject);
  if (drop)
  {
    ClientToScreen (&point);
    CRect rc;
    curfold.GetWindowRect (rc);
    if (rc.PtInRect (point))
    {
      if (dropDefault == DROPEFFECT_COPY)
        pasteFiles (drop, false);
      else if (dropDefault == DROPEFFECT_MOVE)
        pasteFiles (drop, true);
      else
        return DROPEFFECT_NONE;
      return DROPEFFECT_COPY;
    }
    else
    {
      int count = DragQueryFile (drop, -1, NULL, 0);
      if (count <= 0)
        return DROPEFFECT_NONE;
      char path[512];
      DragQueryFile (drop, 0, path, 512);
      SetDlgItemText (IDC_REPLAYFILE, path);
      OnBnClickedParsereplay ();
      return DROPEFFECT_COPY;
    }
  }
  return DROPEFFECT_NONE;
}

void CDotAReplayDlg::OnBnClickedBack()
{
  if (curView == firstView)
    return;
  curView--;
  if (curView < 0)
    curView = 63;
  if (curView == firstView)
    GetDlgItem (IDC_BACK)->EnableWindow (FALSE);
  GetDlgItem (IDC_FORWARD)->EnableWindow (TRUE);
  applyView (curView);
}
void CDotAReplayDlg::OnBnClickedForward()
{
  if (curView == lastView)
    return;
  curView++;
  if (curView >= 64)
    curView = 0;
  if (curView == lastView)
    GetDlgItem (IDC_FORWARD)->EnableWindow (FALSE);
  GetDlgItem (IDC_BACK)->EnableWindow (TRUE);
  applyView (curView);
}

void CDotAReplayDlg::applyView (int pos)
{
  isLoadingView = true;
  if (views[pos].path[0])
  {
    SetDlgItemText (IDC_REPLAYFILE, views[pos].path);
    OnBnClickedParsereplay ();
    if (isReplayOpen)
    {
      tabs.SetCurSel (views[pos].data[0]);
      if (views[pos].data[1])
      {
        ((CPlayerInfoDlg*) tabwnd[TAB_PLAYERS])->selectPlayer (views[pos].data[1]);
        ((CActionsDlg*) tabwnd[TAB_ACTIONS])->selectPlayer (views[pos].data[1]);
      }
      LRESULT pr;
      OnTcnSelchangeTabs (NULL, &pr);
    }
  }
  else if (views[pos].data[0] & ITEM_TYPE)
  {
    for (HTREEITEM item = files.GetNextItem (TVI_ROOT, TVGN_CHILD); item;
      item = files.GetNextItem (item, TVGN_NEXT))
    {
      if ((int (files.GetItemData (item)) & ITEM_TYPE) == views[pos].data[0])
      {
        files.SelectItem (item);
        if ((views[pos].data[0] & ITEM_TYPE) == ITEM_HEROCHART)
        {
        }
        break;
      }
    }
  }
  isLoadingView = false;
}
