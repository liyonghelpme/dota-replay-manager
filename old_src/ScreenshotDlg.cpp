// ScreenshotDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "ScreenshotDlg.h"
#include "ilib.h"
#include "dota.h"
#include "gamecache.h"
#include "replay.h"
#include "dotareplaydlg.h"
#include ".\screenshotdlg.h"

// CScreenshotDlg dialog

IMPLEMENT_DYNAMIC(CScreenshotDlg, CDialog)
CScreenshotDlg::CScreenshotDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScreenshotDlg::IDD, pParent)
{
  dlg = (CDotAReplayDlg*) pParent;
  Create (IDD, pParent);
  ignore = NULL;
}

CScreenshotDlg::~CScreenshotDlg()
{
}

void CScreenshotDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenshotDlg, CDialog)
  ON_BN_CLICKED(IDC_REFRESH, refresh)
  ON_WM_DESTROY()
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_PLAYERS, OnLvnItemActivatePlayers)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_PLAYERS, OnLvnItemchangedPlayers)
  ON_BN_CLICKED(IDC_MODIFY, OnBnClickedModify)
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CScreenshotDlg message handlers

BOOL CScreenshotDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  list.Attach (GetDlgItem (IDC_PLAYERS)->m_hWnd);
  list.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
  list.InsertColumn (0, "Player");
  list.InsertColumn (1, "Games");
  list.InsertColumn (2, "Best K-D");
  list.InsertColumn (3, "Worst K-D");
  list.InsertColumn (4, "Won %");

  loc.SetWindow (this);
  loc.SetItemRelative (IDC_REFRESH, SIDE_LEFT, IDC_REFRESH, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_PLAYERS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_PLAYERS, SIDE_BOTTOM, PERCENT);
  loc.SetItemRelative (IDC_NOTES, SIDE_TOP, IDC_PLAYERS, SIDE_BOTTOM);
  loc.SetItemAbsolute (IDC_NOTES, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_NOTES, SIDE_BOTTOM);
  loc.Start ();

  return TRUE;
}

void CScreenshotDlg::OnDestroy ()
{
  list.Detach ();
}

static int slotY[] = {106, 149, 191, 234, 277, 343, 386, 428, 471, 514, 580, 623};
static int chrSize[256];
struct ParseInfo
{
  char result[256];
  int length;
  GLImage* font;
  unsigned char* bits;
  int width;
  int height;
};
COLORREF getGLPixel (ParseInfo* info, int x, int y)
{
  unsigned char* ptr = info->font->bits + ((info->font->height - y - 1) * info->font->width + x) * 3;
  return RGB (ptr[0], ptr[1], ptr[2]);
}
COLORREF getBMPixel (ParseInfo* info, int x, int y)
{
  x = (x * info->height) / 1024;
  y = (y * info->height) / 1024;
  if (x < 0 || x >= info->width) return 0;
  unsigned char* ptr = info->bits + (y * info->width + x) * 4;
  return RGB (ptr[2], ptr[1], ptr[0]);
}
int getClrDiff (COLORREF a, COLORREF b)
{
  int ca = ((a >> 16) & 0xFF);
  int cb = ((b >> 16) & 0xFF);
  ca = 255 - (255 - ca) * (255 - ca) / 255;
  cb = 255 - (255 - cb) * (255 - cb) / 255;
  int d = ca - cb;
  if (d < 0) d = -d;
  return d * d;
}
int getRDiff (ParseInfo* info, int slot, int start, int chr)
{
  int sum = 1;
  int chx = chr % 16;
  int chy = (chr / 16) - 2;
  chx *= 20;
  chy = chy * 20 + 18;
  for (int x = 0; x < 20; x++)
  {
    if (getGLPixel (info, chx + x, chy) == RGB (255, 0, 0))
    {
      chrSize[chr] = x;
      break;
    }
    for (int y = 0; y < 19; y++)
    {
      COLORREF a = getGLPixel (info, chx + x, chy - y);
      COLORREF b = getBMPixel (info, start + x, slotY[slot] - y);
      sum += getClrDiff (a, b);
    }
  }
  return sum;
}
int getDiff (ParseInfo* info, int slot, int start, int chr, int& shift)
{
  int best = getRDiff (info, slot, start, chr);
  shift = 0;
  int left = getRDiff (info, slot, start - 1, chr);
  if (left < best)
  {
    best = left;
    shift = -1;
  }
  int right = getRDiff (info, slot, start + 1, chr);
  if (right < best)
  {
    best = right;
    shift = 1;
  }
  return best;
}
void parseSlot (ParseInfo* info, int slot)
{
  info->length = 0;
  int pos = 47;
  while (info->length < 30)
  {
    int best = ' ';
    int bshift;
    int diff = getDiff (info, slot, pos, best, bshift);
    for (int chr = 0x21; chr <= 0x7E; chr++)
    {
      int shift;
      int cd = getDiff (info, slot, pos, chr, shift);
      if (diff * chrSize[chr] > cd * chrSize[best])
      {
        best = chr;
        bshift = shift;
        diff = cd;
      }
    }
    if (best == ' ')
      break;
    info->result[info->length++] = best;
    pos += chrSize[best] + bshift;
  }
  info->result[info->length] = 0;
}

void CScreenshotDlg::refresh ()
{
  ignore = NULL;
  list.DeleteAllItems ();
  GetDlgItem (IDC_MODIFY)->EnableWindow (FALSE);
  SetDlgItemText (IDC_NAME, "");
  GetDlgItem (IDC_NAME)->EnableWindow (FALSE);
  SetDlgItemText (IDC_NOTES, "");
  if (IsClipboardFormatAvailable (CF_BITMAP) && OpenClipboard ())
  {
    GLImage font ("wc3font");
    ParseInfo info;
    info.font = &font;

    HBITMAP hbm = (HBITMAP) GetClipboardData (CF_BITMAP);
    if (hbm == NULL)
    {
      CloseClipboard ();
      return;
    }
    HDC dc = ::GetDC (NULL);
    BITMAP binfo;
    CBitmap* bmp = CBitmap::FromHandle (hbm);
    bmp->GetBitmap (&binfo);
    info.width = binfo.bmWidth;
    info.height = binfo.bmHeight;
    info.bits = new unsigned char[4 * info.width * info.height];
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof bi;
    bi.biWidth = info.width;
    bi.biHeight = -info.height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 1;
    bi.biYPelsPerMeter = 1;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    GetDIBits (dc, hbm, 0, info.height, info.bits, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    ::ReleaseDC (NULL, dc);
    CloseClipboard ();

    int pos = list.InsertItem (list.GetItemCount (), "Sentinel");
    list.SetItemData (pos, 0);
    for (int i = 0; i < 5; i++)
    {
      parseSlot (&info, i);
      pos = list.InsertItem (list.GetItemCount (), info.result);
      list.SetItemData (pos, getLightColor (i + 1));
    }
    pos = list.InsertItem (list.GetItemCount (), "Scourge");
    list.SetItemData (pos, 0);
    for (int i = 0; i < 5; i++)
    {
      parseSlot (&info, i + 5);
      pos = list.InsertItem (list.GetItemCount (), info.result);
      list.SetItemData (pos, getLightColor (i + 7));
    }
    char obs0[256];
    parseSlot (&info, 10);
    strcpy (obs0, info.result);
    parseSlot (&info, 11);
    if (obs0[0] || info.result[0])
    {
      pos = list.InsertItem (list.GetItemCount (), "Observers");
      list.SetItemData (pos, 0);
      pos = list.InsertItem (list.GetItemCount (), obs0);
      list.SetItemData (pos, getLightColor (12));
      pos = list.InsertItem (list.GetItemCount (), info.result);
      list.SetItemData (pos, getLightColor (12));
    }

    processList ();

    delete[] info.bits;
  }
  else
    makeEmptyList ();
}
void CScreenshotDlg::fromGame (W3GReplay* w3g)
{
  ignore = w3g;
  list.DeleteAllItems ();
  GetDlgItem (IDC_MODIFY)->EnableWindow (FALSE);
  SetDlgItemText (IDC_NAME, "");
  GetDlgItem (IDC_NAME)->EnableWindow (FALSE);
  SetDlgItemText (IDC_NOTES, "");
  if (w3g && w3g->dota.isDota)
  {
    int pos = list.InsertItem (list.GetItemCount (), "Sentinel");
    list.SetItemData (pos, 0);
    for (int i = 0; i < 5; i++)
      for (int j = 0; j < w3g->dota.numSentinel; j++)
        if (w3g->players[w3g->dota.sentinel[j]].slot.color == i + 1)
        {
          pos = list.InsertItem (list.GetItemCount (), w3g->players[w3g->dota.sentinel[j]].name);
          list.SetItemData (pos, getLightColor (i + 1));
        }
    pos = list.InsertItem (list.GetItemCount (), "Scourge");
    list.SetItemData (pos, 0);
    for (int i = 0; i < 5; i++)
      for (int j = 0; j < w3g->dota.numScourge; j++)
        if (w3g->players[w3g->dota.scourge[j]].slot.color == i + 7)
        {
          pos = list.InsertItem (list.GetItemCount (), w3g->players[w3g->dota.scourge[j]].name);
          list.SetItemData (pos, getLightColor (i + 7));
        }
    bool first = true;
    for (int i = 0; i < w3g->numPlayers; i++)
    {
      if (w3g->players[w3g->pindex[i]].slot.color > 11)
      {
        if (first)
        {
          pos = list.InsertItem (list.GetItemCount (), "Observers");
          list.SetItemData (pos, 0);
          first = false;
        }
        pos = list.InsertItem (list.GetItemCount (), w3g->players[w3g->pindex[i]].name);
        list.SetItemData (pos, getLightColor (12));
      }
    }

    processList ();
  }
  else
    makeEmptyList ();
}

bool isBetter (int ka, int da, int kb, int db)
{
  if (ka < 0 || da < 0) return false;
  if (kb < 0 || db < 0) return true;
  if (ka * db == da * kb)
    return ka == kb ? da < db : ka > kb;
  else
    return ka * db > da * kb;
}
bool isWorse (int ka, int da, int kb, int db)
{
  if (ka < 0 || da < 0) return false;
  if (kb < 0 || db < 0) return true;
  if (ka * db == da * kb)
    return ka == kb ? da > db : ka < kb;
  else
    return ka * db < da * kb;
}

void CScreenshotDlg::processList ()
{
  int needFlags = GC_NAME | GC_LENGTH | GC_COUNT | GC_NAMES | GC_HEROES |
    GC_STATS | GC_LEFT | GC_GOLD | GC_LEVEL | GC_LANE | GC_TEAM | GC_WIN | GC_APM;
  wchar_t plnames[12][256];
  int numpl = 0;
  for (int i = 0; i < list.GetItemCount (); i++)
  {
    if (list.GetItemData (i) == 0) continue;
    wchar_t name[256];
    LVGetItemText (list, i, 0, name, 256);
    wcscpy (plnames[numpl++], name);
    int numGames = 0;
    int bk = -1, bd = -1, wk = -1, wd = -1;
    int numWon = 0;
    int numLost = 0;
    for (int g = 0; g < gcsize; g++)
    {
      GameCache* gc = &gcache[g];
      if ((gc->flags & needFlags) != needFlags)
        continue;
      bool found = false;
      for (int c = 0; c < g && !found; c++)
        if ((gcache[c].flags & needFlags) == needFlags && gcache[c].mod == gc->mod)
          found = true;
      if (found)
        continue;
      for (int j = 0; j < gc->count; j++)
      {
        if (!wcsicmp (gc->pname[j], name) && gc->phero[j])
        {
          numGames++;
          if (isBetter (gc->pstats[j][0], gc->pstats[j][1], bk, bd))
          {
            bk = gc->pstats[j][0];
            bd = gc->pstats[j][1];
          }
          if (isWorse (gc->pstats[j][0], gc->pstats[j][1], wk, wd))
          {
            wk = gc->pstats[j][0];
            wd = gc->pstats[j][1];
          }
          int win = -1;
          if (gc->win == WINNER_SENTINEL || gc->win == WINNER_GSENTINEL || gc->win == WINNER_PSENTINEL)
            win = 0;
          else if (gc->win == WINNER_SCOURGE || gc->win == WINNER_GSCOURGE || gc->win == WINNER_PSCOURGE)
            win = 1;
          if (win >= 0)
          {
            if (gc->pteam[j] == win)
              numWon++;
            else
              numLost++;
          }
        }
      }
    }
    if (numGames > 0)
      list.SetItemText (i, 1, mprintf ("%d", numGames));
    if (bk >= 0 || bd >= 0)
      list.SetItemText (i, 2, mprintf ("%d-%d", bk, bd));
    if (wk >= 0 || wd >= 0)
      list.SetItemText (i, 3, mprintf ("%d-%d", wk, wd));
    if (numWon + numLost > 0)
      list.SetItemText (i, 4, mprintf ("%d%%", numWon * 100 / (numWon + numLost)));
    if (numGames)
      list.SetItemData (i, (list.GetItemData (i) & 0xFFFFFF) | 0xFF000000);
  }
  int count[4096];
  memset (count, 0, sizeof count);
  int mainMask = 0;
  for (int g = 0; g < gcsize; g++)
  {
    GameCache* gc = &gcache[g];
    if ((gc->flags & needFlags) != needFlags)
      continue;
    bool found = false;
    for (int c = 0; c < g && !found; c++)
      if ((gcache[c].flags & needFlags) == needFlags && gcache[c].mod == gc->mod)
        found = true;
    if (found)
      continue;
    int flags = 0;
    int cnt = 0;
    for (int i = 0; i < numpl; i++)
    {
      for (int j = 0; j < gc->count; j++)
      {
        if (!wcsicmp (gc->pname[j], plnames[i]) && gc->phero[j])
        {
          flags |= (1 << i);
          cnt++;
          break;
        }
      }
    }
    if (cnt > 1)
    {
      count[flags]++;
      if (ignore && ignore->timestamp == gc->mod)
        mainMask = flags;
    }
  }
  if (count[mainMask] == 1)
    count[mainMask] = 0;
  CString notes = "";
  for (int i = 1; i < (1 << numpl); i++)
  {
    if (count[i])
    {
      bool first = true;
      for (int j = 0; j < numpl; j++)
      {
        if (i & (1 << j))
        {
          if (!first)
            notes += ", ";
          first = false;
          notes += plnames[j];
        }
      }
      notes += mprintf (" have played %d games together. \r\n", count[i]);
    }
  }
  SetDlgItemText (IDC_NOTES, notes);
  for (int i = 0; i < list.GetHeaderCtrl ()->GetItemCount (); i++)
    list.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
}

void CScreenshotDlg::makeEmptyList ()
{
  int pos = list.InsertItem (list.GetItemCount (), "Sentinel");
  list.SetItemData (pos, 0);
  for (int i = 0; i < 5; i++)
  {
    pos = list.InsertItem (list.GetItemCount (), "");
    list.SetItemData (pos, getLightColor (i + 1));
  }
  pos = list.InsertItem (list.GetItemCount (), "Scourge");
  list.SetItemData (pos, 0);
  for (int i = 0; i < 5; i++)
  {
    pos = list.InsertItem (list.GetItemCount (), "");
    list.SetItemData (pos, getLightColor (i + 7));
  }
  for (int i = 0; i < list.GetHeaderCtrl ()->GetItemCount (); i++)
    list.SetColumnWidth (i, LVSCW_AUTOSIZE_USEHEADER);
}

void CScreenshotDlg::OnLvnItemActivatePlayers(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int item = list.GetNextItem (-1, LVNI_SELECTED);
  if (item >= 0 && (list.GetItemData (item) & 0xFF000000))
  {
    char name[256];
    list.GetItemText (item, 0, name, 256);
    dlg->showPlayerStats (name);
  }
  *pResult = 0;
}

void CScreenshotDlg::OnLvnItemchangedPlayers(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  int item = list.GetNextItem (-1, LVNI_SELECTED);
  if (item >= 0 && (list.GetItemData (item) & 0xFFFFFF))
  {
    char name[256];
    list.GetItemText (item, 0, name, 256);
    SetDlgItemText (IDC_NAME, name);
    GetDlgItem (IDC_MODIFY)->EnableWindow (TRUE);
    GetDlgItem (IDC_NAME)->EnableWindow (TRUE);
  }
  else
  {
    GetDlgItem (IDC_MODIFY)->EnableWindow (FALSE);
    SetDlgItemText (IDC_NAME, "");
    GetDlgItem (IDC_NAME)->EnableWindow (FALSE);
  }
  *pResult = 0;
}

void CScreenshotDlg::OnBnClickedModify()
{
  int item = list.GetNextItem (-1, LVNI_SELECTED);
  if (item >= 0 && (list.GetItemData (item) & 0xFFFFFF))
  {
    char name[256];
    GetDlgItemText (IDC_NAME, name, 256);
    list.SetItemText (item, 0, name);
  }
  processList ();
}

void CScreenshotDlg::OnSize (UINT nType, int cx, int cy)
{
  loc.Update ();
}
