// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "SettingsDlg.h"
#include ".\settingsdlg.h"

#include "dotareplaydlg.h"
#include "utils.h"
#include "tageditordlg.h"

#include "registry.h"

#include "updatedlg.h"
#include "gamecache.h"
#include "sparser.h"

bool drawChat = true;
bool drawPings = true;
bool drawBuildings = true;
int chatStaysOn = 16;
int deathTreshold = 30;
bool viewWindow = false;
int repDelay = 3000;
int repDelayItem = 3000;
bool drawWards = true;
bool autoUpdate = true;
int wardLife = 360;
int maxFiles = 2000;
CString defaultPath;
int lastUpdate = 0;
bool showLevels = true;
bool setRegOpen = false;
bool showAssemble = true;
bool skillColors = true;
bool syncSelect = true;
bool useLog = false;
bool showDetails = false;
bool saveCache = true;
bool enableUrl = true;
int selColumns = 0;
bool smoothGold = true;
bool ignoreBasic = false;
bool chatHeroes = true;
LOGFONT chatFont;
int chatColors = 1;
bool extractCache = true;
bool __fail = false;
bool relTime = true;
bool chatAssists = true;
bool showEmptySlots = true;

DWORD chatBg = RGB (0, 0, 0);
DWORD chatFg = RGB (255, 255, 255);

char imageUrl[256];
char warReplay[256];
char warPath[256] = "";
char defaultWar[256] = "";
char warIcon[256];
char openWithThis[256];
char replayPath[256];
wchar_t ownNames[256];

MPQLOADER warloader = 0;

#define IDC_CHATCLR       3057

IMPLEMENT_DYNAMIC(CClickColor, CWnd)
void CClickColor::Create (CRect const& rc, CWnd* parent, int sid)
{
  bg = RGB (0, 0, 0);
  fg = RGB (255, 255, 255);
  id = sid;
  curTime = 5000;
  CreateEx (WS_EX_CLIENTEDGE, NULL, "", WS_CHILD, rc, parent, id);
  CDC* dc = GetDC ();
  CFont* oldfont = dc->GetCurrentFont ();
  oldfont->GetLogFont (&curfont);
  font.CreateFontIndirect (&curfont);
  ReleaseDC (dc);
  ShowWindow (SW_SHOW);
  SetTimer (2345, 1500, NULL);
}
BEGIN_MESSAGE_MAP(CClickColor, CWnd)
  ON_WM_ERASEBKGND()
  ON_WM_LBUTTONDOWN()
  ON_WM_LBUTTONUP()
  ON_WM_PAINT()
  ON_WM_TIMER()
END_MESSAGE_MAP()
BOOL CClickColor::OnEraseBkgnd (CDC* pDC)
{
  CRect rc;
  GetClientRect (rc);
  pDC->FillSolidRect (rc, bg);
  return TRUE;
}
void CClickColor::OnLButtonDown (UINT nFlags, CPoint point)
{
  SetCapture ();
}
void CClickColor::OnLButtonUp (UINT nFlags, CPoint point)
{
  if (GetCapture ())
  {
    ReleaseCapture ();
    CWnd* parent = GetParent ();
    if (parent && id != IDC_STATIC)
      parent->SendNotifyMessage (WM_COMMAND, MAKEWPARAM (id, BN_CLICKED), (LPARAM) m_hWnd);
  }
}
void CClickColor::OnPaint ()
{
  CPaintDC dc (this);
  dc.SetTextColor (fg);
  dc.SetBkColor (bg);
  dc.SelectObject (&font);
  CRect rc;
  GetClientRect (rc);
  dc.DrawText (format_time (curTime), -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  rc.left += 58;
  dc.DrawText ("[All] ", -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  rc.left += dc.GetTextExtent ("[All] ").cx;
  if (chatColors == 0)
    dc.SetTextColor (getDefaultColor (curslot));
  else if (chatColors == 1)
    dc.SetTextColor (getSlotColor (curslot));
  else
    dc.SetTextColor (getDarkColor (curslot));
  dc.DrawText ("d07.RiV: ", -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  rc.left += dc.GetTextExtent ("d07.RiV: ").cx;
  dc.SetTextColor (fg);
  dc.DrawText ("click to change background", -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  rc.left += dc.GetTextExtent ("click to change background").cx;
}
void CClickColor::setFont (LOGFONT const* newfont)
{
  CDC* dc = GetDC ();
  font.DeleteObject ();
  if (font.CreateFontIndirect (newfont) == 0)
    font.CreateFontIndirect (&curfont);
  else
    memcpy (&curfont, newfont, sizeof curfont);
  ReleaseDC (dc);
  InvalidateRect (NULL);
}
void CClickColor::OnTimer (UINT_PTR nIDEvent)
{
  curslot = (curslot + 5) % 12;
  curTime = rand ();
  InvalidateRect (NULL);
}

void unslash (CString& str)
{
  str.Replace ('/', '\\');
  if (str.IsEmpty () || str[str.GetLength () - 1] != '\\')
    str += '\\';
}
void unslash (char* str)
{
  int i;
  for (i = 0; str[i]; i++)
    if (str[i] == '/')
      str[i] = '\\';
  if (i <= 0 || str[i - 1] != '\\')
  {
    str[i] = '\\';
    str[i + 1] = 0;
  }
}
void qualify (char* str)
{
  char buf[512];
  if (str[1] != ':')
  {
    GetCurrentDirectory (512, buf);
    unslash (buf);
    strcat (buf, str);
  }
  else
    strcpy (buf, str);
  int len = 0;
  int dots = 0;
  for (int pos = 0; buf[pos]; pos++)
  {
    if (buf[pos] == '\\')
    {
      if (dots == 2)
      {
        len -= 3;
        while (str[len - 1] != '\\')
          len--;
      }
      else if (dots == 1)
        len--;
      else
        str[len++] = '\\';
      dots = 0;
    }
    else if (buf[pos] == '.')
    {
      if (dots >= 0)
        dots++;
      str[len++] = '.';
    }
    else
    {
      str[len++] = buf[pos];
      dots = -1;
    }
  }
  str[len] = 0;
}
void qualify (CString& str)
{
  char buf[512];
  if (str[1] != ':')
  {
    GetCurrentDirectory (512, buf);
    unslash (buf);
    strcat (buf, str);
  }
  else
    strcpy (buf, str);
  int len = 0;
  int dots = 0;
  for (int pos = 0; buf[pos]; pos++)
  {
    if (buf[pos] == '\\')
    {
      if (dots == 2)
      {
        len -= 3;
        while (str[len - 1] != '\\')
          len--;
      }
      else if (dots == 1)
        len--;
      else
        str += '\\';
      dots = 0;
    }
    else if (buf[pos] == '.')
    {
      if (dots >= 0)
        dots++;
      str += '.';
      len++;
    }
    else
    {
      str += buf[pos];
      len++;
      dots = -1;
    }
  }
  str = str.Left (len);
}

// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialog)
CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent)
{
  dlg = (CDotAReplayDlg*) pParent;

  memset (&chatFont, 0, sizeof chatFont);
  HDC hdc = ::GetDC (NULL);
  strcpy (chatFont.lfFaceName, "Georgia");
  chatFont.lfHeight = -MulDiv (10, GetDeviceCaps (hdc, LOGPIXELSY), 72);
  ::ReleaseDC (NULL, hdc);

  started = false;

  {
    strcpy (openWithThis, GetCommandLine ());
    char esym = ' ';
    if (openWithThis[0] == '"')
      esym = '"';
    int len = (int) strlen (openWithThis);
    int qpos = -1;
    for (int i = 1; i < len && qpos < 0; i++)
      if (openWithThis[i] == esym)
        qpos = i;
    if (esym == '"')
      qpos++;
    openWithThis[qpos] = 0;
  }

  DWORD size = 256;
  char name[256];
  HKEY hKey;
  bool manual = false;
  if (RegOpenKeyEx (HKEY_CURRENT_USER, _T("Software\\Blizzard Entertainment\\Warcraft III"), 0, KEY_QUERY_VALUE,
    &hKey) == ERROR_SUCCESS)
  {
    if (RegQueryValueEx (hKey, _T("InstallPath"), 0, 0, (LPBYTE) name, &size) != ERROR_SUCCESS)
      manual = true;
    RegCloseKey (hKey);
  }
  else
    manual = true;
  if (manual)
  {
    sprintf (warIcon, "%s,2", openWithThis);
    GetCurrentDirectory (256, name);
    unslash (name);
    replayPath = name;
  }
  else
  {
    unslash (name);
    sprintf (warIcon, "%sreplays.ico", name);
    replayPath = name;
    replayPath += "replay\\";
    strcpy (warPath, name);
  }
  strcat (openWithThis, " \"%1\"");
  size = 256;
  bool manualf = false;
  if (RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("Warcraft3.Replay\\shell\\open\\command"), 0, KEY_QUERY_VALUE,
          &hKey) == ERROR_SUCCESS)
  {
    if (RegQueryValueEx (hKey, _T(""), 0, 0, (LPBYTE)warReplay, &size) != ERROR_SUCCESS)
      manualf = true;
    RegCloseKey (hKey);
  }
  else
    manualf = true;
  if (!manualf && !is_substr (warReplay, "War3.exe"))
  {
    if (!stricmp (warReplay, openWithThis))
      setRegOpen = true;
    manualf = true;
  }
  if (manualf)
  {
    if (manual)
      warReplay[0] = 0;
    else
      sprintf (warReplay, "\"%sWar3.exe\" -loadfile \"%%s\"", name);
  }
  else
  {
    int pos = 0;
    while (warReplay[pos])
      pos++;
    while (pos > 0 && warReplay[pos] != '1')
      pos--;
    warReplay[pos] = 's';
  }

  strcpy (defaultWar, warPath);
  defaultPath = replayPath;
  strcpy (::replayPath, replayPath);
  readSettings ();
  lastUpdate = reg.readInt ("lastUpdate", 0);
}

#include "gamecache.h"
bool loadDataEz (unsigned long version, char const* mappath);
void __ldd (char const* path)
{
  loadDataEz (makeVersion (path), mprintf ("DotA Allstars v%s.w3x", path));
}
void __ldd2 (char const* path)
{
  loadDataEz (makeVersion (path), mprintf ("DotA v%s.w3x", path));
}

void CSettingsDlg::readSettings ()
{
  char name[256];
  reg.readString ("replayPath", name, defaultPath);
  replayPath = name;
  reg.readString ("warPath", warPath, warPath);
  drawWards = (reg.readInt ("drawWards", drawWards ? 1 : 0) != 0);
  drawChat = (reg.readInt ("drawChat", drawChat ? 1 : 0) != 0);
  drawPings = (reg.readInt ("drawPings", drawPings ? 1 : 0) != 0);
  drawBuildings = (reg.readInt ("drawBuildings", drawBuildings ? 1 : 0) != 0);
  chatStaysOn = reg.readInt ("chatLife", 16);
  viewWindow = (reg.readInt ("viewWindow", viewWindow ? 1 : 0) != 0);
  deathTreshold = reg.readInt ("deathTreshold", 30);
  wardLife = reg.readInt ("wardLife", 360);
  repDelay = reg.readInt ("repDelay", 3000);
  repDelayItem = reg.readInt ("repDelayItem", 1000);
  maxFiles = reg.readInt ("maxFiles", 2000);
  autoUpdate = (reg.readInt ("autoUpdate", autoUpdate ? 1 : 0) != 0);
  showLevels = (reg.readInt ("showLevels", showLevels ? 1 : 0) != 0);
  showAssemble = (reg.readInt ("showAssemble", showAssemble ? 1 : 0) != 0);
  skillColors = (reg.readInt ("skillColors", skillColors ? 1 : 0) != 0);
  syncSelect = (reg.readInt ("syncSelect", syncSelect ? 1 : 0) != 0);
  useLog = (reg.readInt ("useLog", useLog ? 1 : 0) != 0);
  showDetails = (reg.readInt ("showDetails", showDetails ? 1 : 0) != 0);
  selColumns = reg.readInt ("selColumns", 0);
  saveCache = (reg.readInt ("saveCache", saveCache ? 1 : 0) != 0);
  enableUrl = (reg.readInt ("enableUrl", enableUrl ? 1 : 0) != 0);
  smoothGold = (reg.readInt ("smoothGold", smoothGold ? 1 : 0) != 0);
  relTime = (reg.readInt ("relTime", relTime ? 1 : 0) != 0);
  chatHeroes = (reg.readInt ("chatHeroes", chatHeroes ? 1 : 0) != 0);
  chatAssists = (reg.readInt ("chatAssists", chatAssists ? 1 : 0) != 0);
  showEmptySlots = (reg.readInt ("emptySlots", showEmptySlots ? 1 : 0) != 0);
  ignoreBasic = (reg.readInt ("ignoreBasic", ignoreBasic ? 1 : 0) != 0);
  chatBg = reg.readInt ("chatBg", chatBg);
  chatFg = reg.readInt ("chatFg", chatFg);
  reg.readBinary ("chatFont", &chatFont, sizeof chatFont);
  chatColors = reg.readInt ("chatColors", chatColors);
  reg.readString ("imageUrl", imageUrl, "http://www.dota-riv.jino-net.ru/dotaicon/");
  reg.readString ("ownNames", ownNames, L"");
  strcpy (::replayPath, replayPath);

  FILE* tf = fopen (mprintf ("%swar3x.mpq", warPath), "r");
  if (tf == NULL)
  {
    BROWSEINFO bi;
    memset (&bi, 0, sizeof bi);
    bi.lpszTitle = "Select Warcraft III folder";
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
    ITEMIDLIST* list = SHBrowseForFolder (&bi);
    if (list)
    {
      SHGetPathFromIDList (list, warPath);
      unslash (warPath);

      LPMALLOC ml;
      SHGetMalloc (&ml);
      ml->Free (list);
      ml->Release ();
    }
  }
  else
    fclose (tf);

  warloader = MPQCreateLoader ("Custom_V1");
  MPQLoadArchive (warloader, mprintf ("%swar3.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3x.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3xlocal.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3patch.mpq", warPath));

  if (getNumVersions () == 0)
    if (!createPrimary ())
      __fail = true;

  //__ldd2 ("6.70c");
  //__ldd2 ("6.70b");
  //__ldd2 ("6.70");
  //__ldd2 ("6.69c");
  //__ldd2 ("6.69b");
  //__ldd2 ("6.69");
  //__ldd2 ("6.68c");
  //__ldd ("6.67c");
  //__ldd ("6.67b");
  //__ldd ("6.66b");
  //__ldd ("6.66");
  //__ldd ("6.65");
  //__ldd ("6.64");
  //__ldd ("6.63b");
  //__ldd ("6.63");
  //__ldd ("6.62b");
  //__ldd ("6.62");
  //__ldd ("6.61c");
  //__ldd ("6.61b");
  //__ldd ("6.61");
  //__ldd ("6.60b");
  //__ldd ("6.60");
  //__ldd ("6.59d");
  //__ldd ("6.59c");
  //__ldd ("6.59b");
  //__ldd ("6.59");
  //__ldd ("6.58b");
  //__ldd ("6.58");
  //__ldd ("6.57b");
  //__ldd ("6.57");
  //__ldd ("6.56");
  //__ldd ("6.55b");
  //__ldd ("6.55");
  //__ldd ("6.54b");
  //__ldd ("6.54");
  //__ldd ("6.53");
  //__ldd ("6.52e");
  //__ldd ("6.52d");
  //__ldd ("6.52c");
  //__ldd ("6.52b");
  //__ldd ("6.52");
  //__ldd ("6.51");
  //__ldd ("6.50b");
  //__ldd ("6.50");
  //__ldd ("6.49c");
  //__ldd ("6.49b");
  //__ldd ("6.49");
  //__ldd ("6.48b");
  //__ldd ("6.48");
  //__ldd ("6.46b");
  //__ldd ("6.46");
  //__ldd ("6.45");
  //__ldd ("6.44b");
  //__ldd ("6.44");
  //__ldd ("6.43b");
  //__ldd ("6.42");
  //__ldd ("6.41");
  //__ldd ("6.40");
  //__ldd ("6.39");
  //__ldd ("6.38b");
  //__ldd ("6.37");
  //__ldd ("6.34");
  //__ldd ("6.32b");
}

CSettingsDlg::~CSettingsDlg()
{
  MPQReleaseLoader (warloader);
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
  ON_BN_CLICKED(IDC_UPDATEPATH, OnEnChangeReplaypath)
  ON_BN_CLICKED(IDC_HELPFORMAT, OnBnClickedHelpformat)
  ON_EN_CHANGE(IDC_COPYFORMAT, OnEnChangeCopyformat)
  ON_BN_CLICKED(IDC_AUTOVIEW, OnBnClickedAutoview)
  ON_BN_CLICKED(IDC_AUTOCOPY, OnBnClickedAutocopy)
  ON_WM_TIMER()
  ON_BN_CLICKED(IDC_ABOUT, OnBnClickedAbout)
  ON_BN_CLICKED(IDC_DRAWCHAT, OnBnClickedDrawchat)
  ON_EN_CHANGE(IDC_CHATLIFE, OnEnChangeChatlife)
  ON_BN_CLICKED(IDC_USETRAY, OnBnClickedUsetray)
  ON_BN_CLICKED(IDC_VIEWWINDOW, OnBnClickedViewwindow)
//  ON_EN_CHANGE(IDC_TIMEDEATH, OnEnChangeTimedeath)
  ON_EN_CHANGE(IDC_REPDELAY, OnEnChangeRepdelay)
  ON_BN_CLICKED(IDC_DRAWWARDS, OnBnClickedDrawwards)
  ON_EN_CHANGE(IDC_WARDLIFE, OnEnChangeWardlife)
  ON_BN_CLICKED(IDC_RESETPATH, OnBnClickedResetpath)
  ON_EN_CHANGE(IDC_MAXFILES, OnEnChangeMaxfiles)
  ON_EN_CHANGE(IDC_IMGURL, OnEnChangeImgurl)
  ON_BN_CLICKED(IDC_README, OnBnClickedReadme)
  ON_BN_CLICKED(IDC_DRAWPINGS, OnBnClickedDrawpings)
  ON_BN_CLICKED(IDC_UPDATES, OnBnClickedUpdates)
  ON_BN_CLICKED(IDC_AUTOUPDATE, OnBnClickedAutoupdate)
  ON_BN_CLICKED(IDC_SHOWLEVELS, OnBnClickedShowlevels)
  ON_BN_CLICKED(IDC_SETREGOPEN, OnBnClickedSetregopen)
  ON_WM_DESTROY()
  ON_NOTIFY(TCN_SELCHANGE, IDC_OPTTABS, OnTcnSelchangeOpttabs)
  ON_BN_CLICKED(IDC_SHOWASSEMBLE, OnBnClickedShowassemble)
  ON_BN_CLICKED(IDC_SKILLCOLORS, OnBnClickedSkillcolors)
  ON_BN_CLICKED(IDC_SYNCSEL, OnBnClickedSyncsel)
  ON_BN_CLICKED(IDC_USELOG, OnBnClickedUselog)
  ON_BN_CLICKED(IDC_SHOWDETAILS, OnBnClickedShowdetails)
  ON_BN_CLICKED(IDC_DET_SAVED, OnBnClickedDetSaved)
  ON_BN_CLICKED(IDC_DET_SIZE, OnBnClickedDetSize)
  ON_BN_CLICKED(IDC_DET_NAME, OnBnClickedDetName)
  ON_BN_CLICKED(IDC_DET_RATIO, OnBnClickedDetRatio)
  ON_BN_CLICKED(IDC_DET_LENGTH, OnBnClickedDetLength)
  ON_BN_CLICKED(IDC_SAVECACHE, OnBnClickedSavecache)
  ON_BN_CLICKED(IDC_DET_MODE, OnBnClickedDetMode)
  ON_BN_CLICKED(IDC_ENABLEURL, OnBnClickedEnableurl)
  ON_BN_CLICKED(IDC_CACHEALL, OnBnClickedCacheall)
  ON_BN_CLICKED(IDC_BROWSEPATH, OnBnClickedBrowsepath)
  ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
  ON_BN_CLICKED(IDC_SMOOTHGOLD, OnBnClickedSmoothgold)
  ON_BN_CLICKED(IDC_CHATHEROES, OnBnClickedChatheroes)
  ON_BN_CLICKED(IDC_IGNOREBASIC, OnBnClickedIgnorebasic)
  ON_EN_CHANGE(IDC_YOURNAME, OnEnChangeYourname)
  ON_EN_CHANGE(IDC_REPDELAYITEM, OnEnChangeRepdelayitem)
  ON_WM_SIZE()
  ON_EN_CHANGE(IDC_WARPATH, OnEnChangeWarpath)
  ON_BN_CLICKED(IDC_RESETWAR, OnBnClickedResetwar)
  ON_BN_CLICKED(IDC_BROWSEWAR, OnBnClickedBrowsewar)
  ON_BN_CLICKED(IDC_CHATCLR, OnBnClickedChatBG)
  ON_BN_CLICKED(IDC_CHATFONT, OnBnClickedChatfont)
  ON_CBN_SELCHANGE(IDC_CHATPLCLR, OnCbnSelchangeChatplclr)
  ON_BN_CLICKED(IDC_RELTIME, &CSettingsDlg::OnBnClickedReltime)
  ON_BN_CLICKED(IDC_CHATASSISTS, &CSettingsDlg::OnBnClickedChatassists)
  ON_BN_CLICKED(IDC_SHOWEMPTY, &CSettingsDlg::OnBnClickedShowempty)
  ON_BN_CLICKED(IDC_DRAWBUILDINGS, &CSettingsDlg::OnBnClickedDrawbuildings)
  //ON_BN_CLICKED(IDC_FORUMICONS, &CSettingsDlg::OnBnClickedForumicons)
END_MESSAGE_MAP()

struct TabItem
{
  int id;
  int tab;
};
const TabItem tabs[] = {
  {IDC_WARPATH_TIP, 0},
  {IDC_WARPATH, 0},
  {IDC_RESETWAR, 0},
  {IDC_BROWSEWAR, 0},
  {IDC_REPLAYPATH_TIP, 0},
  {IDC_REPLAYPATH, 0},
  {IDC_RESETPATH, 0},
  {IDC_BROWSEPATH, 0},
  {IDC_UPDATEPATH, 0},
  {IDC_MAXFILES_TIP, 0},
  {IDC_MAXFILES, 0},
  {IDC_VIEWWINDOW, 0},
  {IDC_AUTOVIEW, 0},
  {IDC_AUTOCOPY, 0},
  {IDC_COPYFORMAT, 0},
  {IDC_COPYFORMAT_TIP, 0},
  {IDC_HELPFORMAT, 0},
  {IDC_USETRAY, 0},
  {IDC_AUTOUPDATE, 0},
  {IDC_SETREGOPEN, 0},
  {IDC_SHOWDETAILS, 0},
  {IDC_DET_BOX, 0},
  {IDC_DET_SAVED, 0},
  {IDC_DET_SIZE, 0},
  {IDC_DET_WARN, 0},
  {IDC_DET_NAME, 0},
  {IDC_DET_RATIO, 0},
  {IDC_DET_LENGTH, 0},
  {IDC_DET_MODE, 0},
  {IDC_SAVECACHE, 0},
  {IDC_ENABLEURL, 0},

  {IDC_YOURNAME_TIP, 1},
  {IDC_YOURNAME, 1},
  {IDC_REPDELAY_TIP, 1},
  {IDC_REPDELAY, 1},
  {IDC_REPDELAYITEM_TIP, 1},
  {IDC_REPDELAYITEM, 1},
  {IDC_TIMELINE_BOX, 1},
  {IDC_DRAWWARDS, 1},
  {IDC_WARDLIFE_TIP, 1},
  {IDC_WARDLIFE, 1},
  {IDC_DRAWCHAT, 1},
  {IDC_CHATLIFE_TIP, 1},
  {IDC_CHATLIFE, 1},
  {IDC_DRAWPINGS, 1},
  {IDC_DRAWBUILDINGS, 1},
//  {IDC_TIMEDEATH_TIP, 1},
//  {IDC_TIMEDEATH, 1},
  {IDC_SHOWLEVELS, 1},
  {IDC_SKILLCOLORS, 1},
  {IDC_SHOWASSEMBLE, 1},
  {IDC_SYNCSEL, 1},
  {IDC_IMGURL_TIP, 1},
  {IDC_IMGURL, 1},
  {IDC_SMOOTHGOLD, 1},
  {IDC_RELTIME, 1},
  {IDC_CHATHEROES, 1},
  {IDC_CHATASSISTS, 1},
  {IDC_SHOWEMPTY, 1},
  {IDC_USELOG, 1},
  {IDC_IGNOREBASIC, 1},
  {IDC_CHATCLRTIP, 1},
  {IDC_CHATCLR, 1},
  {IDC_CHATFONT, 1},
  {IDC_CHATPLCLRTIP, 1},
  {IDC_CHATPLCLR, 1}
};
char tabNames[][32] = {
  "General",
  "Replay"
};
const int numTabItems = sizeof tabs / sizeof tabs[0];
const int numTabs = sizeof tabNames / sizeof tabNames[0];

void CSettingsDlg::getExtent (int t, CRect& rc)
{
  rc.left = 10000;
  rc.top = 10000;
  rc.right = 0;
  rc.bottom = 0;
  for (int i = 0; i < numTabItems; i++)
  {
    if (tabs[i].tab == t)
    {
      CRect rt;
      GetDlgItem (tabs[i].id)->GetWindowRect (rt);
      ScreenToClient (rt);
      if (rt.left < rc.left) rc.left = rt.left;
      if (rt.top < rc.top) rc.top = rt.top;
      if (rt.right > rc.right) rc.right = rt.right;
      if (rt.bottom > rc.bottom) rc.bottom = rt.bottom;
    }
  }
}

// CSettingsDlg message handlers

void CSettingsDlg::OnEnChangeReplaypath()
{
  GetDlgItemText (IDC_REPLAYPATH, replayPath);
  unslash (replayPath);
  dlg->OnChangePath ();
  reg.writeString ("replayPath", replayPath);
  strcpy (::replayPath, replayPath);
  setCacheDirectory (getCacheDirectory ());
}

BOOL CSettingsDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  CRect rc1, rc2, rc3;

  GetDlgItem (IDC_CHATCLRBOX)->GetWindowRect (rc1); ScreenToClient (rc1);
  chatClr.Create (rc1, this, IDC_CHATCLR);

  tab.Attach (GetDlgItem (IDC_OPTTABS)->m_hWnd);
  getExtent (0, rc1);
  tab.InsertItem (0, tabNames[0]);
  for (int i = 1; i < numTabs; i++)
  {
    getExtent (i, rc2);
    for (int j = 0; j < numTabItems; j++)
    {
      if (tabs[j].tab == i)
      {
        GetDlgItem (tabs[j].id)->GetWindowRect (rc3);
        ScreenToClient (rc3);
        GetDlgItem (tabs[j].id)->SetWindowPos (NULL, rc1.left + rc3.left - rc2.left, rc1.top + rc3.top - rc2.top,
          0, 0, SWP_NOZORDER | SWP_NOSIZE);
      }
    }
    tab.InsertItem (i, tabNames[i]);
  }
  GetClientRect (rc1);
  GetWindowRect (rc3);
  tab.GetWindowRect (rc2);
  ScreenToClient (rc2);
  SetWindowPos (NULL, 0, 0, rc2.right + rc2.left + (rc3.right - rc3.left - rc1.right),
    rc3.bottom - rc3.top, SWP_NOZORDER | SWP_NOMOVE);

  applySettings ();
  SetTimer (57, 10000, NULL);
  switchTab ();

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_OPTTABS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_OPTTABS, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_CACHEALL, SIDE_TOP, IDC_CACHEALL, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_RESET, SIDE_TOP, IDC_RESET, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_RESET, SIDE_LEFT, PERCENT);
  //loc.SetItemRelative (IDC_FORUMICONS, SIDE_TOP, IDC_RESET, SIDE_BOTTOM);
  //loc.SetItemAbsolute (IDC_FORUMICONS, SIDE_LEFT, PERCENT);
  loc.SetItemRelative (IDC_UPDATES, SIDE_TOP, IDC_UPDATES, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_UPDATES, SIDE_LEFT, IDC_UPDATES, SIDE_RIGHT);
  loc.SetItemRelative (IDC_README, SIDE_TOP, IDC_README, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_README, SIDE_LEFT, IDC_README, SIDE_RIGHT);
  loc.SetItemRelative (IDC_ABOUT, SIDE_TOP, IDC_ABOUT, SIDE_BOTTOM);
  loc.SetItemRelative (IDC_ABOUT, SIDE_LEFT, IDC_ABOUT, SIDE_RIGHT);
  loc.Start ();

  started = true;

  return TRUE;
}

void CSettingsDlg::applySettings ()
{
  SetDlgItemText (IDC_WARPATH, warPath);
  SetDlgItemText (IDC_REPLAYPATH, replayPath);
  char buf[1024];
  reg.readString ("copyFormat", buf, "Autocopy\\replay<n>");
  SetDlgItemText (IDC_COPYFORMAT, buf);

  CheckDlgButton (IDC_AUTOCOPY, reg.readInt ("autoCopy", 0));
  CheckDlgButton (IDC_AUTOVIEW, reg.readInt ("autoView", 1));
  CheckDlgButton (IDC_USETRAY, reg.readInt ("useTray", 1));

  CheckDlgButton (IDC_VIEWWINDOW, viewWindow ? TRUE : FALSE);
  CheckDlgButton (IDC_DRAWCHAT, drawChat ? TRUE : FALSE);
  CheckDlgButton (IDC_DRAWPINGS, drawPings ? TRUE : FALSE);
  CheckDlgButton (IDC_DRAWBUILDINGS, drawBuildings ? TRUE : FALSE);
  CheckDlgButton (IDC_DRAWWARDS, drawWards ? TRUE : FALSE);
  SetDlgItemText (IDC_CHATLIFE, mprintf ("%d", chatStaysOn));
  //SetDlgItemText (IDC_TIMEDEATH, mprintf ("%d", deathTreshold));
  SetDlgItemText (IDC_REPDELAY, mprintf ("%d", repDelay));
  SetDlgItemText (IDC_REPDELAYITEM, mprintf ("%d", repDelayItem));
  SetDlgItemText (IDC_WARDLIFE, mprintf ("%d", wardLife));

  SetDlgItemText (IDC_MAXFILES, mprintf ("%d", maxFiles));

  SetDlgItemTextW (m_hWnd, IDC_YOURNAME, ownNames);

  SetDlgItemText (IDC_IMGURL, imageUrl);
  CheckDlgButton (IDC_AUTOUPDATE, autoUpdate ? TRUE : FALSE);

  CheckDlgButton (IDC_SHOWLEVELS, showLevels ? TRUE : FALSE);
  CheckDlgButton (IDC_SETREGOPEN, setRegOpen ? TRUE : FALSE);
  CheckDlgButton (IDC_SHOWASSEMBLE, showAssemble ? TRUE : FALSE);
  CheckDlgButton (IDC_SKILLCOLORS, skillColors ? TRUE : FALSE);
  CheckDlgButton (IDC_SYNCSEL, syncSelect ? TRUE : FALSE);
  CheckDlgButton (IDC_USELOG, useLog ? TRUE : FALSE);
  CheckDlgButton (IDC_SAVECACHE, saveCache ? TRUE : FALSE);

  CheckDlgButton (IDC_SHOWDETAILS, showDetails ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_SAVED, (selColumns & COL_SAVED) ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_SIZE, (selColumns & COL_SIZE) ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_NAME, (selColumns & COL_NAME) ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_RATIO, (selColumns & COL_RATIO) ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_LENGTH, (selColumns & COL_LENGTH) ? TRUE : FALSE);
  CheckDlgButton (IDC_DET_MODE, (selColumns & COL_MODE) ? TRUE : FALSE);

  CheckDlgButton (IDC_ENABLEURL, enableUrl ? TRUE : FALSE);
  CheckDlgButton (IDC_SMOOTHGOLD, smoothGold ? TRUE : FALSE);
  CheckDlgButton (IDC_CHATHEROES, chatHeroes ? TRUE : FALSE);
  CheckDlgButton (IDC_CHATASSISTS, chatAssists ? TRUE : FALSE);
  CheckDlgButton (IDC_SHOWEMPTY, showEmptySlots ? TRUE : FALSE);
  CheckDlgButton (IDC_IGNOREBASIC, ignoreBasic ? TRUE : FALSE);
  CheckDlgButton (IDC_RELTIME, relTime ? TRUE : FALSE);

  chatClr.setColor (chatBg, chatFg);
  chatClr.setFont (&chatFont);
  ((CComboBox*) GetDlgItem (IDC_CHATPLCLR))->SetCurSel (chatColors);

  GetDlgItem (IDC_CHATLIFE)->EnableWindow (IsDlgButtonChecked (IDC_DRAWCHAT));
}

void CSettingsDlg::OnDestroy ()
{
  tab.Detach ();
}

void CSettingsDlg::switchTab ()
{
  int t = tab.GetCurSel ();
  for (int i = 0; i < numTabItems; i++)
    GetDlgItem (tabs[i].id)->ShowWindow (tabs[i].tab == t ? SW_SHOW : SW_HIDE);
}

CString CSettingsDlg::getBatchFormat ()
{
  CString str;
  GetDlgItemText (IDC_COPYFORMAT, str);
  return str;
}

CString CSettingsDlg::getCopyName (W3GReplay* w3g, char const* filename, char const* batch)
{
  char _fpath[256];
  char ftitle[256];
  _splitpath (filename, NULL, _fpath, ftitle, NULL);
  CString fpath = _fpath;
  unslash (fpath);
  if (!strnicmp (fpath, replayPath, replayPath.GetLength ()))
    fpath = fpath.Mid (replayPath.GetLength ());
  CString fname = "";
  char buf[1024];
  if (batch)
    strcpy (buf, batch);
  else
    GetDlgItemText (IDC_COPYFORMAT, buf, 1024);
  CTime wtime (getFileDate (filename));
  bool unique = false;
  for (int c = 0; buf[c]; c++)
  {
    if (buf[c] != '/' && buf[c] != ':' && buf[c] != '?' &&
        buf[c] != '<' && buf[c] != '>' && buf[c] != '|' && buf[c] >= ' ')
      fname += buf[c];
    else if (buf[c] == '/')
      fname += '\\';
    else if (buf[c] == '<')
    {
      CString tag = "";
      c++;
      while (buf[c] && buf[c] != '>')
        tag += buf[c++];
      if (buf[c] != '>')
        break;
      if (tag == "n" && !unique)
      {
        fname += "?";
        unique = true;
      }
      if (tag == "patch" && w3g)
        fname += formatVersion (getWc3Version (w3g->hdr.major_v, w3g->hdr.build_v));
      if (tag == "version" && w3g && w3g->dota.isDota)
      {
        if (w3g->dota.build)
          fname += mprintf ("%d.%02d%c", w3g->dota.major, w3g->dota.minor, char ('a' + w3g->dota.build));
        else
          fname += mprintf ("%d.%02d", w3g->dota.major, w3g->dota.minor);
      }
      if (tag == "map" && w3g)
      {
        char map[256];
        _splitpath (w3g->game.map, NULL, NULL, buf, NULL);
        for (int i = 0; map[i]; i++)
          if (map[i] != '/' && map[i] != ':' && map[i] != '?' &&
              map[i] != '<' && map[i] != '>' && map[i] != '|' && map[i] >= ' ')
            fname += map[i];
      }
      if (tag == "name" && w3g)
      {
        for (int i = 0; w3g->game.name[i]; i++)
          if (w3g->game.name[i] != '/' && w3g->game.name[i] != ':' && w3g->game.name[i] != '?' &&
              w3g->game.name[i] != '<' && w3g->game.name[i] != '>' && w3g->game.name[i] != '|' &&
              w3g->game.name[i] >= ' ')
            fname += w3g->game.name[i];
      }
      if (tag == "sentinel" && w3g && w3g->dota.isDota)
        fname += mprintf ("%d", w3g->dota.numSentinel);
      if (tag == "sentinel kills" && w3g && w3g->dota.isDota && w3g->dota.endgame)
        fname += mprintf ("%d", w3g->dota.sentinelKills);
      if (tag == "scourge" && w3g && w3g->dota.isDota)
        fname += mprintf ("%d", w3g->dota.numScourge);
      if (tag == "scourge kills" && w3g && w3g->dota.isDota && w3g->dota.endgame)
        fname += mprintf ("%d", w3g->dota.scourgeKills);
      if (tag == "length" && w3g)
        fname += format_time (w3g->time, TIME_HOURS | TIME_SECONDS);
      if (tag == "date")
        fname += wtime.Format ("%Y-%m-%d");
      if (tag == "date2")
        fname += wtime.Format ("%d-%m-%Y");
      if (tag == "time")
        fname += wtime.Format ("%H_%M");
      if (tag == "file")
        fname += ftitle;
      if (tag == "path")
        fname += fpath;
      //if (tag.Left (7) == "script " && scriptEnv)
      //  fname += (char const*) scriptEnv->exec ((char const*) tag.Mid (7), w3g);
      if (tag == "winner" && w3g && w3g->dota.isDota)
      {
        if (w3g->game.winner == WINNER_UNKNOWN)
          fname += "Unknown";
        else if (w3g->game.winner == WINNER_SENTINEL || w3g->game.winner == WINNER_GSENTINEL)
          fname += "Sentinel";
        else if (w3g->game.winner == WINNER_SCOURGE || w3g->game.winner == WINNER_GSCOURGE)
          fname += "Scourge";
      }
      if (w3g)
      {
        if (tag == "player host")
          fname += w3g->game.creator;
        if (tag == "player saver")
          fname += w3g->players[w3g->game.saver_id].name;
        if (tag == "hero host" || tag == "kills host" || tag == "deaths host" ||
            tag == "creeps host" || tag == "denies host" || tag == "win host")
        {
          for (int i = 0; i < w3g->numPlayers; i++)
          {
            if (!strcmp (w3g->players[w3g->pindex[i]].name, w3g->game.creator))
            {
              int slot = w3g->players[w3g->pindex[i]].slot.color;
              if (slot > 5) slot--;
              CString act = "";
              for (int j = 0; tag[j] != ' '; j++)
                act += tag[j];
              tag = act + mprintf (" %d", slot);
            }
          }
        }
        if (tag == "hero saver" || tag == "kills saver" || tag == "deaths saver" ||
            tag == "creeps saver" || tag == "denies saver" || tag == "win saver")
        {
          int slot = w3g->players[w3g->game.saver_id].slot.color;
          if (slot > 5) slot--;
          CString act = "";
          for (int j = 0; tag[j] != ' '; j++)
            act += tag[j];
          tag = act + mprintf (" %d", slot);
        }
        for (int i = 1; i <= 10; i++)
        {
          int s = i;
          if (s > 5) s++;
          for (int j = 0; j < w3g->numPlayers; j++)
          {
            if (w3g->players[w3g->pindex[j]].slot.color == s)
            {
              if (tag == mprintf ("player %d", i))
                fname += w3g->players[w3g->pindex[j]].name;
              if (tag == mprintf ("kills %d", i) && w3g->dota.endgame && w3g->dota.isDota)
                fname += mprintf ("%d", w3g->players[w3g->pindex[j]].stats[0]);
              if (tag == mprintf ("deaths %d", i) && w3g->dota.endgame && w3g->dota.isDota)
                fname += mprintf ("%d", w3g->players[w3g->pindex[j]].stats[1]);
              if (tag == mprintf ("creeps %d", i) && w3g->dota.endgame && w3g->dota.isDota)
                fname += mprintf ("%d", w3g->players[w3g->pindex[j]].stats[2]);
              if (tag == mprintf ("denies %d", i) && w3g->dota.endgame && w3g->dota.isDota)
                fname += mprintf ("%d", w3g->players[w3g->pindex[j]].stats[3]);
              if (tag == mprintf ("hero %d", i))
              {
                DotaHero* hero = (w3g->players[w3g->pindex[j]].hero
                  ? getHero (w3g->players[w3g->pindex[j]].hero->id)
                  : getHero (0));
                if (hero)
                  fname += hero->abbr;
              }
              if (tag == mprintf ("win %d", i))
              {
                if (w3g->game.winner != WINNER_UNKNOWN)
                {
                  int winteam = 0;
                  if (w3g->game.winner == WINNER_SCOURGE || w3g->game.winner == WINNER_GSCOURGE)
                    winteam = 1;
                  if (w3g->players[w3g->pindex[j]].slot.team == winteam)
                    fname += "Won";
                  else if (w3g->players[w3g->pindex[j]].slot.team == 1 - winteam)
                    fname += "Lost";
                }
              }
            }
          }
        }
      }
    }
  }
  while (fname[0] == '\\')
    fname = fname.Mid (1);
  fname = fname + ".w3g";
  return fname;
}

void CSettingsDlg::OnBnClickedHelpformat()
{
  MessageBox ("Specify the name of the destination file.\n"
    "Extension .w3g will be appended automatically\n\n"
    "You can use the following tags (tags are text in angle brackets):\n"
    "<n> - insert an integer number so that the filename becomes unique.\n"
    "<patch> - insert Warcraft patch version used in the game (e.g. \"1.21\")\n"
    "<version> - DotA version (e.g. \"6.49b\")\n"
    "<map> - map name (e.g. \"DotA Allstars 6.49b\")\n"
    "<name> - game name (e.g. \"dota -ap no noobs\")\n"
    "<sentinel> - number of sentinel players\n"
    "<scourge> - number of scourge players\n"
    "<sentinel kills> - sentinel score (may be wrong)\n"
    "<scourge kills> - scourge score (may be wrong)\n"
    "<length> - game length (e.g. \"1:05:54\")\n"
    "<date> - date of the replay file, YYYY-MM-DD (e.g. \"2007-06-26\")\n"
    "<date2> - date of the replay file, DD-MM-YYYY (e.g. \"26-06-2007\")\n"
    "<time> - time of the replay file (e.g. \"18_20\")\n"
    "<winner> - game winner (\"Unknown\", \"Sentinel\" or \"Scourge\")\n"
    "In the following tags ID can be either player index (1-10), \"host\" or \"saver\":\n"
    "<player ID> - player name\n"
    "<hero ID> - player's hero\n"
    "<win ID> - whether the player won or lost (\"Won\" or \"Lost\")\n"
    "<kills ID> - player's hero kills\n"
    "<deaths ID> - player's deaths\n"
    "<creeps ID> - player's creep kills\n"
    "<denies ID> - player's creep denies\n\n"
    "For batch copying replays the following tags are available:\n"
    "<file> - original file title (e.g. \"LastReplay\")\n"
    "<path> - original file path (e.g. \"Dota\\IHL\\\")", "Help");
}

void CSettingsDlg::OnTimer (UINT_PTR nIDEvent)
{
  bool showUpdate = false;
  if (autoUpdate)
  {
    __int64 cur = CTime::GetCurrentTime ().GetTime ();
    int day = int (cur / (60 * 60 * 24));
    if (day != lastUpdate)
    {
      if (lastVersion <= curVersion)
        updateVersion ();
      showUpdate = lastVersion > curVersion;
    }
  }
  reg.flush ();
  if (showUpdate)
  {
    __int64 cur = CTime::GetCurrentTime ().GetTime ();
    int day = int (cur / (60 * 60 * 24));
    reg.writeInt ("lastUpdate", lastUpdate = day);
    CUpdateDlg dlg;
    dlg.DoModal ();
  }
}

void CSettingsDlg::OnEnChangeCopyformat()
{
  char buf[1024];
  GetDlgItemText (IDC_COPYFORMAT, buf, 1024);
  reg.writeString ("copyFormat", buf);
}

bool CSettingsDlg::autoCopy ()
{
  return IsDlgButtonChecked (IDC_AUTOCOPY) != 0;
}
bool CSettingsDlg::autoView ()
{
  return IsDlgButtonChecked (IDC_AUTOVIEW) != 0;
}
bool CSettingsDlg::useTray ()
{
  return started && (IsDlgButtonChecked (IDC_USETRAY) != 0);
}

void CSettingsDlg::OnBnClickedAutoview()
{
  reg.writeInt ("autoView", IsDlgButtonChecked (IDC_AUTOVIEW));
}

void CSettingsDlg::OnBnClickedAutocopy()
{
  reg.writeInt ("autoCopy", IsDlgButtonChecked (IDC_AUTOCOPY));
}

void CSettingsDlg::OnBnClickedAbout()
{
  dlg->OnSysCommand (IDM_ABOUTBOX, 0);
}

void CSettingsDlg::OnBnClickedDrawchat()
{
  reg.writeInt ("drawChat", IsDlgButtonChecked (IDC_DRAWCHAT));
  drawChat = (IsDlgButtonChecked (IDC_DRAWCHAT) != FALSE);
  GetDlgItem (IDC_CHATLIFE)->EnableWindow (IsDlgButtonChecked (IDC_DRAWCHAT));
}

void CSettingsDlg::OnEnChangeChatlife()
{
  int val = GetDlgItemInt (IDC_CHATLIFE);
  if (val <= 0)
    SetDlgItemText (IDC_CHATLIFE, mprintf ("%d", chatStaysOn));
  else
    reg.writeInt ("chatLife", chatStaysOn = val);
}

void CSettingsDlg::OnBnClickedUsetray()
{
  reg.writeInt ("useTray", IsDlgButtonChecked (IDC_USETRAY));
}

void CSettingsDlg::OnBnClickedViewwindow()
{
  reg.writeInt ("viewWindow", IsDlgButtonChecked (IDC_VIEWWINDOW));
  viewWindow = (IsDlgButtonChecked (IDC_VIEWWINDOW) != FALSE);
}

//void CSettingsDlg::OnEnChangeTimedeath()
//{
//  int val = GetDlgItemInt (IDC_TIMEDEATH);
//  if (val <= 0)
//    SetDlgItemText (IDC_TIMEDEATH, mprintf ("%d", deathTreshold));
//  else
//    reg.writeInt ("deathTreshold", deathTreshold = val);
//}
//
void CSettingsDlg::OnEnChangeRepdelay()
{
  int val = GetDlgItemInt (IDC_REPDELAY);
  if (val <= 0)
    SetDlgItemText (IDC_REPDELAY, mprintf ("%d", repDelay));
  else
    reg.writeInt ("repDelay", repDelay = val);
}

void CSettingsDlg::OnBnClickedDrawwards()
{
  reg.writeInt ("drawWards", IsDlgButtonChecked (IDC_DRAWWARDS));
  drawWards = (IsDlgButtonChecked (IDC_DRAWWARDS) != FALSE);
}

void CSettingsDlg::OnEnChangeWardlife()
{
  int val = GetDlgItemInt (IDC_WARDLIFE);
  if (val <= 0)
    SetDlgItemText (IDC_WARDLIFE, mprintf ("%d", wardLife));
  else
    reg.writeInt ("wardLife", wardLife = val);
}

void CSettingsDlg::OnBnClickedResetpath()
{
  replayPath = defaultPath;
  SetDlgItemText (IDC_REPLAYPATH, replayPath);
  OnEnChangeReplaypath ();
}

void CSettingsDlg::OnEnChangeMaxfiles()
{
  int val = GetDlgItemInt (IDC_MAXFILES);
  if (val <= 0)
    SetDlgItemText (IDC_MAXFILES, mprintf ("%d", maxFiles));
  else
    reg.writeInt ("maxFiles", maxFiles = val);
}

void CSettingsDlg::OnEnChangeImgurl()
{
  GetDlgItemText (IDC_IMGURL, imageUrl, 256);
  reg.writeString ("imageUrl", imageUrl);
}

void CSettingsDlg::OnBnClickedReadme()
{
  char path[512];
  sprintf (path, "%sreadme.txt", reg.getPath ());

  ShellExecute (NULL, _T("open"), path, NULL, NULL, SW_SHOWNORMAL);
}

void CSettingsDlg::OnBnClickedDrawpings()
{
  reg.writeInt ("drawPings", IsDlgButtonChecked (IDC_DRAWPINGS));
  drawPings = (IsDlgButtonChecked (IDC_DRAWPINGS) != FALSE);
}
void CSettingsDlg::OnBnClickedDrawbuildings()
{
  reg.writeInt ("drawBuildings", IsDlgButtonChecked (IDC_DRAWBUILDINGS));
  drawBuildings = (IsDlgButtonChecked (IDC_DRAWBUILDINGS) != FALSE);
}

void CSettingsDlg::OnBnClickedUpdates()
{
  CUpdateDlg dlg (true);
  dlg.DoModal ();
}

void CSettingsDlg::OnBnClickedAutoupdate()
{
  reg.writeInt ("autoUpdate", IsDlgButtonChecked (IDC_AUTOUPDATE));
  autoUpdate = (IsDlgButtonChecked (IDC_AUTOUPDATE) != FALSE);
}

void CSettingsDlg::OnBnClickedShowlevels()
{
  reg.writeInt ("showLevels", IsDlgButtonChecked (IDC_SHOWLEVELS));
  showLevels = (IsDlgButtonChecked (IDC_SHOWLEVELS) != FALSE);
}

void CSettingsDlg::OnBnClickedSetregopen()
{
  setRegOpen = (IsDlgButtonChecked (IDC_SETREGOPEN) != FALSE);
  HKEY hKey;
  if (RegOpenKeyEx (HKEY_CLASSES_ROOT, _T("Warcraft3.Replay\\shell\\open\\command"), 0, KEY_SET_VALUE,
          &hKey) == ERROR_SUCCESS)
  {
    if (setRegOpen)
      RegSetValueEx (hKey, _T(""), 0, REG_SZ, (LPBYTE) openWithThis, (int) strlen (openWithThis) + 1);
    else
      RegSetValueEx (hKey, _T(""), 0, REG_SZ, (LPBYTE) warReplay, (int) strlen (warReplay) + 1);
    RegCloseKey (hKey);
  }
  else if (RegCreateKeyEx (HKEY_CLASSES_ROOT, ".w3g", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey,
      NULL) == ERROR_SUCCESS)
  {
    RegSetValueEx (hKey, _T(""), 0, REG_SZ, (BYTE const*) "Warcraft3.Replay", (int) strlen ("Warcraft3.Replay") + 1);
    RegCloseKey (hKey);
    if (RegCreateKeyEx (HKEY_CLASSES_ROOT, "Warcraft3.Replay\\shell\\open\\command", 0, NULL,
      REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
      if (setRegOpen)
        RegSetValueEx (hKey, _T(""), 0, REG_SZ, (LPBYTE) openWithThis, (int) strlen (openWithThis) + 1);
      else
        RegSetValueEx (hKey, _T(""), 0, REG_SZ, (LPBYTE) warReplay, (int) strlen (warReplay) + 1);
      RegCloseKey (hKey);
    }
    if (RegCreateKeyEx (HKEY_CLASSES_ROOT, "Warcraft3.Replay\\DefaultIcon", 0, NULL,
      REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
      RegSetValueEx (hKey, _T(""), 0, REG_SZ, (LPBYTE) warIcon, (int) strlen (warIcon) + 1);
      RegCloseKey (hKey);
    }
  }
  else
  {
    MessageBox ("Error writing into registry!", "Error", MB_ICONHAND | MB_OK);
    CheckDlgButton (IDC_SETREGOPEN, FALSE);
  }
}

void CSettingsDlg::OnTcnSelchangeOpttabs(NMHDR *pNMHDR, LRESULT *pResult)
{
  switchTab ();
  *pResult = 0;
}

void CSettingsDlg::OnBnClickedShowassemble()
{
  reg.writeInt ("showAssemble", IsDlgButtonChecked (IDC_SHOWASSEMBLE));
  showAssemble = (IsDlgButtonChecked (IDC_SHOWASSEMBLE) != FALSE);
}

void CSettingsDlg::OnBnClickedSkillcolors()
{
  reg.writeInt ("skillColors", IsDlgButtonChecked (IDC_SKILLCOLORS));
  skillColors = (IsDlgButtonChecked (IDC_SKILLCOLORS) != FALSE);
}

void CSettingsDlg::OnBnClickedSyncsel()
{
  reg.writeInt ("syncSelect", IsDlgButtonChecked (IDC_SYNCSEL));
  syncSelect = (IsDlgButtonChecked (IDC_SYNCSEL) != FALSE);
}

void CSettingsDlg::OnBnClickedUselog()
{
  reg.writeInt ("useLog", IsDlgButtonChecked (IDC_USELOG));
  useLog = (IsDlgButtonChecked (IDC_USELOG) != FALSE);
  GetDlgItem (IDC_IGNOREBASIC)->EnableWindow (useLog ? TRUE : FALSE);
}

void CSettingsDlg::OnBnClickedShowdetails()
{
  reg.writeInt ("showDetails", IsDlgButtonChecked (IDC_SHOWDETAILS));
  showDetails = (IsDlgButtonChecked (IDC_SHOWDETAILS) != FALSE);
}

void CSettingsDlg::OnBnClickedDetSaved()
{
  selColumns = (selColumns & (~COL_SAVED)) | (IsDlgButtonChecked (IDC_DET_SAVED) ? COL_SAVED : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedDetSize()
{
  selColumns = (selColumns & (~COL_SIZE)) | (IsDlgButtonChecked (IDC_DET_SIZE) ? COL_SIZE : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedDetName()
{
  selColumns = (selColumns & (~COL_NAME)) | (IsDlgButtonChecked (IDC_DET_NAME) ? COL_NAME : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedDetRatio()
{
  selColumns = (selColumns & (~COL_RATIO)) | (IsDlgButtonChecked (IDC_DET_RATIO) ? COL_RATIO : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedDetLength()
{
  selColumns = (selColumns & (~COL_LENGTH)) | (IsDlgButtonChecked (IDC_DET_LENGTH) ? COL_LENGTH : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedSavecache()
{
  reg.writeInt ("saveCache", IsDlgButtonChecked (IDC_SAVECACHE));
  saveCache = (IsDlgButtonChecked (IDC_SAVECACHE) != FALSE);
}

void CSettingsDlg::OnBnClickedDetMode()
{
  selColumns = (selColumns & (~COL_MODE)) | (IsDlgButtonChecked (IDC_DET_MODE) ? COL_MODE : 0);
  reg.writeInt ("selColumns", selColumns);
}

void CSettingsDlg::OnBnClickedEnableurl()
{
  reg.writeInt ("enableUrl", IsDlgButtonChecked (IDC_ENABLEURL));
  enableUrl = (IsDlgButtonChecked (IDC_ENABLEURL) != FALSE);
}

void CSettingsDlg::OnBnClickedCacheall()
{
  dlg->cacheAll ();
}

void CSettingsDlg::OnBnClickedBrowsepath()
{
  BROWSEINFO bi;
  memset (&bi, 0, sizeof bi);
  bi.lpszTitle = "Select replay folder";
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
  ITEMIDLIST* list = SHBrowseForFolder (&bi);
  if (list == NULL)
    return;

  char buf[512];
  SHGetPathFromIDList (list, buf);
  unslash (buf);
  replayPath = buf;
  SetDlgItemText (IDC_REPLAYPATH, replayPath);
  OnEnChangeReplaypath ();

  LPMALLOC ml;
  SHGetMalloc (&ml);
  ml->Free (list);
  ml->Release ();
}

void CSettingsDlg::OnBnClickedReset()
{
  reg.restore ();
  readSettings ();
  applySettings ();
}

void CSettingsDlg::OnBnClickedSmoothgold()
{
  reg.writeInt ("smoothGold", IsDlgButtonChecked (IDC_SMOOTHGOLD));
  smoothGold = (IsDlgButtonChecked (IDC_SMOOTHGOLD) != FALSE);
}
void CSettingsDlg::OnBnClickedChatheroes()
{
  reg.writeInt ("chatHeroes", IsDlgButtonChecked (IDC_CHATHEROES));
  chatHeroes = (IsDlgButtonChecked (IDC_CHATHEROES) != FALSE);
}

void CSettingsDlg::OnBnClickedChatassists()
{
  reg.writeInt ("chatAssists", IsDlgButtonChecked (IDC_CHATASSISTS));
  chatAssists = (IsDlgButtonChecked (IDC_CHATASSISTS) != FALSE);
}
void CSettingsDlg::OnBnClickedShowempty()
{
  reg.writeInt ("emptySlots", IsDlgButtonChecked (IDC_SHOWEMPTY));
  showEmptySlots = (IsDlgButtonChecked (IDC_SHOWEMPTY) != FALSE);
}

void CSettingsDlg::OnBnClickedIgnorebasic()
{
  reg.writeInt ("ignoreBasic", IsDlgButtonChecked (IDC_IGNOREBASIC));
  ignoreBasic = (IsDlgButtonChecked (IDC_IGNOREBASIC) != FALSE);
}
void CSettingsDlg::OnBnClickedReltime()
{
  reg.writeInt ("relTime", IsDlgButtonChecked (IDC_RELTIME));
  relTime = (IsDlgButtonChecked (IDC_RELTIME) != FALSE);
}

void CSettingsDlg::OnEnChangeYourname()
{
  GetDlgItemTextW (m_hWnd, IDC_YOURNAME, ownNames, 256);
  reg.writeString ("ownNames", ownNames);
}

void CSettingsDlg::OnEnChangeRepdelayitem()
{
  int val = GetDlgItemInt (IDC_REPDELAYITEM);
  if (val <= 0)
    SetDlgItemText (IDC_REPDELAYITEM, mprintf ("%d", repDelayItem));
  else
    reg.writeInt ("repDelayItem", repDelayItem = val);
}

void CSettingsDlg::OnSize (UINT nType, int cx, int cy)
{
  if (loc.Update ())
  {
    GetDlgItem (IDC_CACHEALL)->Invalidate ();
    GetDlgItem (IDC_RESET)->Invalidate ();
    GetDlgItem (IDC_UPDATES)->Invalidate ();
    GetDlgItem (IDC_README)->Invalidate ();
    GetDlgItem (IDC_ABOUT)->Invalidate ();
  }
}

void CSettingsDlg::OnEnChangeWarpath()
{
  GetDlgItemText (IDC_WARPATH, warPath, sizeof warPath);
  unslash (warPath);
  reg.writeString ("warPath", warPath);

  MPQReleaseLoader (warloader);
  warloader = MPQCreateLoader ("Custom_V1");
  MPQLoadArchive (warloader, mprintf ("%swar3.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3x.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3xlocal.mpq", warPath));
  MPQLoadArchive (warloader, mprintf ("%swar3patch.mpq", warPath));
}

void CSettingsDlg::OnBnClickedResetwar()
{
  strcpy (warPath, defaultWar);
  SetDlgItemText (IDC_WARPATH, warPath);
  reg.writeString ("warPath", warPath);
}

void CSettingsDlg::OnBnClickedBrowsewar()
{
  BROWSEINFO bi;
  memset (&bi, 0, sizeof bi);
  bi.lpszTitle = "Select Warcraft III folder";
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
  ITEMIDLIST* list = SHBrowseForFolder (&bi);
  if (list == NULL)
    return;

  SHGetPathFromIDList (list, warPath);
  unslash (warPath);
  SetDlgItemText (IDC_WARPATH, warPath);
  reg.writeString ("warPath", warPath);

  LPMALLOC ml;
  SHGetMalloc (&ml);
  ml->Free (list);
  ml->Release ();
}

void CSettingsDlg::OnBnClickedChatBG ()
{
  CColorDialog dlg (chatBg, 0, this);
  if (dlg.DoModal () == IDOK)
  {
    chatBg = dlg.GetColor ();
    chatClr.setColor (chatBg, chatFg);
    reg.writeInt ("chatBg", chatBg);
  }
}

void CSettingsDlg::OnBnClickedChatfont()
{
  CFontDialog dlg (&chatFont, CF_EFFECTS | CF_SCREENFONTS | CF_NOSCRIPTSEL, NULL, this);
  dlg.m_cf.rgbColors = chatFg;
  if (dlg.DoModal () == IDOK)
  {
    dlg.GetCurrentFont (&chatFont);
    chatFg = dlg.GetColor ();
    chatClr.setColor (chatBg, chatFg);
    chatClr.setFont (&chatFont);
    reg.writeInt ("chatFg", chatFg);
    reg.writeBinary ("chatFont", &chatFont, sizeof chatFont);
  }
}

void CSettingsDlg::OnCbnSelchangeChatplclr()
{
  chatColors = ((CComboBox*) GetDlgItem (IDC_CHATPLCLR))->GetCurSel ();
  chatClr.InvalidateRect (NULL);
  reg.writeInt ("chatColors", chatColors);
}

//void CSettingsDlg::OnBnClickedForumicons()
//{
//  CTagEditorDlg tags;
//  tags.DoModal ();
//}
