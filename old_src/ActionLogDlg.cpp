// ActionLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "ActionLogDlg.h"
#include "replay.h"
#include "ilib.h"

#include "orders.h"

#define OFFSET_FIRST    2
#define OFFSET_OTHER    6

void CActionLog::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  if (map == NULL)
    return;

	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	CRect rcItem (lpDrawItemStruct->rcItem);
	int nItem = lpDrawItemStruct->itemID;
	BOOL bFocus = (GetFocus () == this);
	static char szBuff[65536];	
	COLORREF clrTextSave, clrBkSave;
	COLORREF color = 0xFFFFFF;

	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = szBuff;
	lvi.cchTextMax = sizeof szBuff;
	lvi.stateMask = 0xFFFF;
	GetItem (&lvi);

	BOOL bSelected = (bFocus || (GetStyle () & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
	bSelected = bSelected || (lvi.state & LVIS_DROPHILITED);

	CRect rcAllLabels;
	GetItemRect (nItem, rcAllLabels, LVIR_BOUNDS);
	CRect rcLabel;
	GetItemRect (nItem, rcLabel, LVIR_LABEL);
  CRect wnd;
  GetClientRect (wnd);
  rcAllLabels.right = wnd.right;

	if (bSelected)
	{
		clrTextSave = pDC->SetTextColor (::GetSysColor (COLOR_HIGHLIGHTTEXT));
		clrBkSave = pDC->SetBkColor (::GetSysColor (COLOR_HIGHLIGHT));

		pDC->FillRect(rcAllLabels, &CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
	}
	else
		pDC->FillRect(rcAllLabels, &CBrush(color));

	GetItemRect (nItem, rcItem, LVIR_LABEL);

	rcLabel = rcItem;
  rcLabel.left = 0;
	rcLabel.left += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;

  pDC->DrawText (format_time (w3g, lvi.lParam), -1, rcLabel, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

	rcItem.left = rcItem.right + OFFSET_OTHER;
  rcItem.right = rcItem.left + 100 - OFFSET_OTHER - OFFSET_OTHER;

  int id = 0;
  for (int i = 0; i < 3; i++)
    id = id * 10 + int (szBuff[i] - '0');

  if (players[id].name[0])
  {
    if (!bSelected)
    {
      rcItem.left -= OFFSET_OTHER;
      rcItem.right += OFFSET_OTHER;
      pDC->FillRect (rcItem, &CBrush (getLightColor (players[id].color)));
      rcItem.left += OFFSET_OTHER;
      rcItem.right -= OFFSET_OTHER;
    }
    pDC->DrawText (players[id].name, -1, rcItem, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
  }
  rcItem.left += 100;
  rcItem.right = wnd.right;

  CImageList* list = map->getImageList ();

  static char buf[65536];
  if (rawcodes)
    pDC->DrawText (szBuff + 3, -1, rcItem, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
  else
  {
    int count = 0;
    int i = 3;
    while (true)
    {
      if (szBuff[i] == '[' || szBuff[i] == 0)
      {
        int id = 0;
        int savei = i;
        if (szBuff[i] == '[')
        {
          i++;
          while (szBuff[i] && szBuff[i] != ']')
            id = id * 256 + int ((unsigned char) szBuff[i++]);
          if (szBuff[i] != ']')
            id = 0;
        }
        UnitData* unit = NULL;
        if (id)
          unit = map->getData ()->getUnitById (id);
        if (szBuff[savei] != 0 && unit == NULL)
          i = savei;
        else
        {
          if (szBuff[savei])
            buf[count++] = szBuff[savei];
          buf[count] = 0;
          CSize sz = pDC->GetTextExtent (buf, count);
          pDC->DrawText (buf, count, rcItem, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
          rcItem.left += sz.cx;
          if (szBuff[savei] == 0)
            break;
          else
          {
            int img = map->getIconIndex (unit->getStringData ("Art", 0));
            if (img)
            {
              list->Draw (pDC, img, CPoint (rcItem.left + 1, (rcItem.top + rcItem.bottom) / 2 - 8), ILD_NORMAL);
              rcItem.left += 18;
            }
            strcpy (buf, unit->getStringData ("Name", 0));
            count = (int) strlen (buf);
          }
        }
      }
      buf[count++] = szBuff[i++];
    }
  }

	if (bSelected)
	{
		pDC->SetTextColor (clrTextSave);
		pDC->SetBkColor (clrBkSave);
	}
}

// CActionLogDlg dialog

IMPLEMENT_DYNAMIC(CActionLogDlg, CDialog)

CActionLogDlg::CActionLogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CActionLogDlg::IDD, pParent)
{
  w3g = NULL;
  map = NULL;
  Create (IDD, pParent);
}

CActionLogDlg::~CActionLogDlg()
{
  delete map;
}

void CActionLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CActionLogDlg, CDialog)
  ON_BN_CLICKED(IDC_LOADACTIONS, &CActionLogDlg::OnBnClickedLoadactions)
  ON_WM_DESTROY()
  ON_BN_CLICKED(IDC_RAWCODES, &CActionLogDlg::OnBnClickedRawcodes)
  ON_NOTIFY(NM_RCLICK, IDC_ACTIONS, OnNMRclickActions)
  ON_WM_SIZE()
  ON_BN_CLICKED(IDC_FINDNEXT, &CActionLogDlg::OnBnClickedFindnext)
  ON_BN_CLICKED(IDC_FINDPREV, &CActionLogDlg::OnBnClickedFindnext)
END_MESSAGE_MAP()


// CActionLogDlg message handlers
#define ID_ACT_COPY         123

BOOL CActionLogDlg::OnInitDialog ()
{
  CDialog::OnInitDialog ();

  searchPlayer.Attach (GetDlgItem (IDC_SEARCHPLAYER)->m_hWnd);
  searchPlayer.SetImageList (getImageList ());

  actions.Attach (GetDlgItem (IDC_ACTIONS)->m_hWnd);
  actions.SetExtendedStyle (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

  actions.InsertColumn (0, "");
  actions.InsertColumn (1, "");
  actions.InsertColumn (2, "");
  actions.SetColumnWidth (0, 50);
  actions.SetColumnWidth (1, 100);
  actions.SetColumnWidth (2, 2000);

  ((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->SetCurSel (0);

  loc.SetWindow (this);
  loc.SetItemAbsolute (IDC_SEARCHFOR, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_SEARCHPLAYER, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FINDNEXT, SIDE_LEFT, IDC_SEARCHPLAYER, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FINDNEXT, SIDE_RIGHT);
  loc.SetItemRelative (IDC_FINDPREV, SIDE_LEFT, IDC_SEARCHPLAYER, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_FINDPREV, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_ACTIONS, SIDE_RIGHT);
  loc.SetItemAbsolute (IDC_ACTIONS, SIDE_BOTTOM);
  loc.Start ();

  popupMenu.CreatePopupMenu ();
  MENUITEMINFO mii;
  memset (&mii, 0, sizeof mii);
  mii.cbSize = sizeof mii;

  mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
  mii.fType = MFT_STRING;
  mii.fState = MFS_DEFAULT;
  mii.dwTypeData = "Copy";
  mii.cch = (int) strlen (mii.dwTypeData);
  mii.wID = ID_ACT_COPY;
  popupMenu.InsertMenuItem (0, &mii, TRUE);

  return TRUE;
}
void CActionLogDlg::OnDestroy ()
{
  actions.Detach ();
  searchPlayer.Detach ();
}
void CActionLogDlg::setReplay (W3GReplay* tw3g)
{
  w3g = tw3g;
  GetDlgItem (IDC_SEARCHPLAYER)->EnableWindow (FALSE);
  GetDlgItem (IDC_SEARCHFOR)->EnableWindow (FALSE);
  GetDlgItem (IDC_SEARCHTYPE)->EnableWindow (FALSE);
  GetDlgItem (IDC_FINDNEXT)->EnableWindow (FALSE);
  GetDlgItem (IDC_FINDPREV)->EnableWindow (FALSE);
  searchPlayer.Clear ();
  searchPlayer.InsertItem ("");
  searchPlayer.SetCurSel (0);
  actions.DeleteAllItems ();
  actions.EnableWindow (FALSE);
  actions.map = NULL;
  actions.w3g = w3g;
  actions.SetImageList (NULL, LVSIL_SMALL);
  searchPlayer.Reset ();
  delete map;
  map = NULL;
  GetDlgItem (IDC_LOADACTIONS)->EnableWindow (TRUE);
  SetDlgItemText (IDC_LOADACTIONS, "Load actions");
}

struct SmallReplay
{
  W3GHeader hdr;
  SmallPlayer players[256];
  int size;
  int pos;
  char* data;
  bool pause;
  bool continue_game;
  unsigned long time;
};

static void _loadPlayer (SmallReplay& rep)
{
  unsigned char rec = rep.data[rep.pos++];
  unsigned char id = rep.data[rep.pos++];
  int nl = 0;
  while (rep.data[rep.pos] != 0)
    rep.players[id].name[nl++] = rep.data[rep.pos++];
  rep.players[id].name[nl] = 0;
  rep.pos++;

  if (rep.data[rep.pos] == 1) // custom game
    rep.pos += 2;
  else if (rep.data[rep.pos] == 8) // ladder game
    rep.pos += 9;
}
static void _loadGame (SmallReplay& rep)
{
  while (rep.data[rep.pos] != 0)
    rep.pos++;
  rep.pos += 2;
  while (rep.data[rep.pos] != 0)
    rep.pos++;
  rep.pos += 1;

  rep.pos += 4;
  rep.pos += 8;
  while (rep.data[rep.pos] == 0x16)
  {
    _loadPlayer (rep);
    rep.pos += 4;
  }

  int slot_records = rep.data[rep.pos + 3];
  rep.pos += 4;

  for (int i = 0; i < slot_records; i++)
  {
    unsigned char id = rep.data[rep.pos];
    rep.pos += 2;
    unsigned char slot_status = rep.data[rep.pos++];
    rep.pos += 2;
    unsigned char color = rep.data[rep.pos++];
    rep.pos += 1;
    if (rep.hdr.major_v >= 3)
      rep.pos++;
    if (rep.hdr.major_v >= 7)
      rep.pos++;
    if (slot_status == 2)
      rep.players[id].color = color;
  }

  rep.pos += 6;
}

void CActionLogDlg::addMessage (unsigned long time, int player, char const* fmt, ...)
{
  char* buf = mprintf ("");
  buf[0] = char ('0' + (player / 100));
  buf[1] = char ('0' + ((player / 10) % 10));
  buf[2] = char ('0' + (player % 10));
  va_list ap;
  va_start (ap, fmt);
  vsprintf (buf + 3, fmt, ap);
  va_end (ap);
  int pos = actions.InsertItem (actions.GetItemCount (), buf);
  actions.SetItemData (pos, time);
}

char const* fmtId (int id)
{
  char* buf = mprintf ("");
  for (int i = 0; i < 4; i++)
  {
    buf[3 - i] = char (id & 0xFF);
    id >>= 8;
  }
  buf[4] = 0;
  return buf;
}
bool isGoodId (int id)
{
  for (int i = 0; i < 4; i++)
  {
    int m = int ((unsigned char) (id & 0xFF));
    if ((m < '0' || m > '9') &&
        (m < 'A' || m > 'Z') &&
        (m < 'a' || m > 'z'))
      return false;
    id >>= 8;
  }
  return true;
}
char const* CActionLogDlg::formatOrder (int id)
{
  char const* order;
  if (map->getData ()->getUnitById (id) || isGoodId (id))
    order = mprintf ("[%s]", fmtId (id));
  else
  {
    order = orderId2String (id);
    if (order == NULL)
      order = mprintf ("0x%08X", id);
  }
  return order;
}
char const* format_coord (char const* data);

void CActionLogDlg::parseActions (int len, SmallReplay& rep)
{
  static char buf[512];
  int blen;
  int end = rep.pos + len;
  while (rep.pos < end)
  {
    unsigned char id = rep.data[rep.pos++];
    short length = * (short*) (rep.data + rep.pos);
    rep.pos += 2;
    int next = rep.pos + length;
    bool was_deselect = false;
    bool was_subgroup = false;

    int prev = 0;
    while (rep.pos < next)
    {
      unsigned char action = rep.data[rep.pos++];
      switch (action)
      {
      case 0x01:
        rep.pause = true;
        addMessage (rep.time, id, "0x01: Pause game");
        break;
      case 0x02:
        rep.pause = false;
        addMessage (rep.time, id, "0x02: Resume game");
      case 0x03:
        addMessage (rep.time, id, "0x03: Set game speed to %d", int (rep.data[rep.pos]));
        rep.pos++;
        break;
      case 0x04:
        addMessage (rep.time, id, "0x04: Increase game speed");
        break;
      case 0x05:
        addMessage (rep.time, id, "0x05: Decrease game speed");
        break;
      case 0x06:
        addMessage (rep.time, id, "0x06: Saving game to %s", rep.data + rep.pos);
        while (rep.data[rep.pos++])
          ;
        break;
      case 0x07:
        addMessage (rep.time, id, "0x07: Finished saving game");
        rep.pos += 4;
        break;
      case 0x10:
        {
          unsigned short flags;
          if (rep.hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (rep.data + rep.pos);
            rep.pos += 2;
          }
          else
            flags = rep.data[rep.pos++];
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          rep.pos += 4;
          addMessage (rep.time, id, "0x10: Immediate order: %s (flags: 0x%04X)",
            formatOrder (itemid), flags);
          if (rep.hdr.major_v >= 7)
            rep.pos += 8;
        }
        break;
      case 0x11:
        {
          unsigned short flags;
          if (rep.hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (rep.data + rep.pos);
            rep.pos += 2;
          }
          else
            flags = rep.data[rep.pos++];
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          rep.pos += 4;
          if (rep.hdr.major_v >= 7)
            rep.pos += 8;
          int x1 = * (int*) (rep.data + rep.pos);
          int y1 = * (int*) (rep.data + rep.pos + 4);
          if (x1 == -1 && y1 == -1)
            addMessage (rep.time, id, "0x11: Point order: %s (flags: 0x%04X)",
              formatOrder (itemid), flags);
          else
            addMessage (rep.time, id, "0x11: Point order: %s (X: %.1f, Y: %.1f, flags: 0x%04X)",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, flags);
          rep.pos += 8;
        }
        break;
      case 0x12:
        {
          unsigned short flags;
          if (rep.hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (rep.data + rep.pos);
            rep.pos += 2;
          }
          else
            flags = rep.data[rep.pos++];
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          rep.pos += 4;
          if (rep.hdr.major_v >= 7)
            rep.pos += 8;
          int x1 = * (int*) (rep.data + rep.pos);
          int y1 = * (int*) (rep.data + rep.pos + 4);
          int a1 = * (int*) (rep.data + rep.pos + 8);
          int b1 = * (int*) (rep.data + rep.pos + 12);
          if (x1 == -1 && y1 == -1)
          {
            if (a1 == -1 && b1 == -1)
              addMessage (rep.time, id, "0x12: Target order: %s (flags: 0x%04X)",
                formatOrder (itemid), flags);
            else
              addMessage (rep.time, id, "0x12: Target order: %s (Target: 0x%08X%08X, flags: 0x%04X)",
                formatOrder (itemid), a1, b1, flags);
          }
          else if (a1 == -1 && b1 == -1)
            addMessage (rep.time, id, "0x12: Target order: %s (X: %.1f, Y: %.1f, flags: 0x%04X)",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, flags);
          else
            addMessage (rep.time, id, "0x12: Target order: %s (X: %.1f, Y: %.1f, Target: 0x%08X%08X, flags: 0x%04X)",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, a1, b1, flags);
          rep.pos += 16;
        }
        break;
      case 0x13:
        {
          unsigned short flags;
          if (rep.hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (rep.data + rep.pos);
            rep.pos += 2;
          }
          else
            flags = rep.data[rep.pos++];
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          rep.pos += 4;
          if (rep.hdr.major_v >= 7)
            rep.pos += 8;
          int x1 = * (int*) (rep.data + rep.pos);
          int y1 = * (int*) (rep.data + rep.pos + 4);
          int a1 = * (int*) (rep.data + rep.pos + 8);
          int b1 = * (int*) (rep.data + rep.pos + 12);
          int a2 = * (int*) (rep.data + rep.pos + 16);
          int b2 = * (int*) (rep.data + rep.pos + 20);
          if (x1 == -1 && y1 == -1)
          {
            if (a1 == -1 && b1 == -1)
              addMessage (rep.time, id, "0x13: Target+object order: %s (Object: 0x%08X%08X, flags: 0x%04X)",
                formatOrder (itemid), a2, b2, flags);
            else
              addMessage (rep.time, id, "0x13: Target+object order: %s (Target: 0x%08X%08X, Object: 0x%08X%08X, flags: 0x%04X)",
                formatOrder (itemid), a1, b1, a2, b2, flags);
          }
          else if (a1 == -1 && b1 == -1)
            addMessage (rep.time, id, "0x13: Target+object order: %s (X: %.1f, Y: %.1f, Object: 0x%08X%08X, flags: 0x%04X)",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, a2, b2, flags);
          else
            addMessage (rep.time, id, "0x13: Target+object order: %s (X: %.1f, Y: %.1f, Target: 0x%08X%08X, Object: 0x%08X%08X, flags: 0x%04X)",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, a1, b1, a2, b2, flags);
          rep.pos += 24;
        }
        break;
      case 0x14:
        {
          unsigned short flags;
          if (rep.hdr.major_v >= 13)
          {
            flags = * (unsigned short*) (rep.data + rep.pos);
            rep.pos += 2;
          }
          else
            flags = rep.data[rep.pos++];
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          rep.pos += 4;
          if (rep.hdr.major_v >= 7)
            rep.pos += 8;
          unsigned long itemid2 = * (unsigned long*) (rep.data + rep.pos + 8);
          int x1 = * (int*) (rep.data + rep.pos);
          int y1 = * (int*) (rep.data + rep.pos + 4);
          int x2 = * (int*) (rep.data + rep.pos + 21);
          int y2 = * (int*) (rep.data + rep.pos + 25);
          if (x1 == -1 && y1 == -1)
          {
            if (x2 == -1 && y2 == -1)
              addMessage (rep.time, id, "0x14: Two points order: %s, %s, flags: 0x%04X",
                formatOrder (itemid), formatOrder (itemid2), flags);
            else
              addMessage (rep.time, id, "0x14: Two points order: %s, %s (X: %.1f, Y: %.1f), flags: 0x%04X",
                formatOrder (itemid), * (float*) &x2, * (float*) &y2, formatOrder (itemid2), flags);
          }
          else if (x2 == -1 && y2 == -1)
            addMessage (rep.time, id, "0x14: Two points order: %s (X: %.1f, Y: %.1f), %s, flags: 0x%04X",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1, formatOrder (itemid2), flags);
          else
            addMessage (rep.time, id, "0x14: Two points order: %s (X: %.1f, Y: %.1f), %s (X: %.1f, Y: %.1f), flags: 0x%04X",
              formatOrder (itemid), * (float*) &x1, * (float*) &y1,
              formatOrder (itemid2), * (float*) &x2, * (float*) &y2, flags);
          rep.pos += 29;
        }
        break;
      case 0x16:
        {
          unsigned char mode = rep.data[rep.pos++];
          short n = * (short*) (rep.data + rep.pos);
          rep.pos += 2;
          char* buf = mprintf (mode == 0x01 ? "0x16: Add to selection: " : "0x16: Remove from selection: ");
          for (int i = 0; i < n; i++)
          {
            if (i < 12)
            {
              if (i)
                strcat (buf, ", ");
              strcat (buf, mprintf ("0x%08X%08X",
                * (int*) (rep.data + rep.pos), * (int*) (rep.data + rep.pos + 4)));
            }
            rep.pos += 8;
          }
          addMessage (rep.time, id, "%s", buf);
        }
        break;
      case 0x17:
        {
          unsigned char mode = rep.data[rep.pos++];
          short n = * (short*) (rep.data + rep.pos);
          rep.pos += 2;
          char key = (mode == 9 ? '0' : '1' + mode);
          char* buf = mprintf ("0x17: Assign group %c: ", key);
          for (int i = 0; i < n; i++)
          {
            if (i < 12)
            {
              if (i)
                strcat (buf, ", ");
              strcat (buf, mprintf ("0x%08X%08X",
                * (int*) (rep.data + rep.pos), * (int*) (rep.data + rep.pos + 4)));
            }
            rep.pos += 8;
          }
          addMessage (rep.time, id, "%s", buf);
        }
        break;
      case 0x18:
        {
          unsigned char mode = rep.data[rep.pos];
          rep.pos += 2;
          char key = (mode == 9 ? '0' : '1' + mode);
          addMessage (rep.time, id, "0x18: Select group %c", key);
        }
        break;
      case 0x19:
        {
          unsigned long itemid = * (unsigned long*) (rep.data + rep.pos);
          unsigned long objid1 = * (unsigned long*) (rep.data + rep.pos + 4);
          unsigned long objid2 = * (unsigned long*) (rep.data + rep.pos + 8);
          rep.pos += 12;
          addMessage (rep.time, id, "0x19: Select Subgroup: %s 0x%08X%08X", formatOrder (itemid),
            objid1, objid2);
        }
        break;
      case 0x1A:
        if (rep.hdr.build_v > 6040 || rep.hdr.major_v > 14)
        {
          was_subgroup = (prev == 0x19 || prev == 0);
          addMessage (rep.time, id, "0x1A: Pre subselection");
        }
        else
          rep.pos += 9;
        break;
      case 0x1B:
        if (rep.hdr.build_v <= 6040 && rep.hdr.major_v <= 14)
          addMessage (rep.time, id, "0x1B: Select ground item 0x%08X%08X",
            * (int*) (rep.data + rep.pos + 1), * (int*) (rep.data + rep.pos + 5));
        rep.pos += 9;
        break;
      case 0x1C:
        if (rep.hdr.build_v > 6040 || rep.hdr.major_v > 14)
        {
          addMessage (rep.time, id, "0x1C: Select ground item 0x%08X%08X",
            * (int*) (rep.data + rep.pos + 1), * (int*) (rep.data + rep.pos + 5));
          rep.pos += 9;
        }
        else
        {
          addMessage (rep.time, id, "0x1C: Cancel hero revival 0x%08X%08X",
            * (int*) (rep.data + rep.pos), * (int*) (rep.data + rep.pos + 4));
          rep.pos += 8;
        }
        break;
      case 0x1D:
        if (rep.hdr.build_v > 6040 || rep.hdr.major_v > 14)
        {
          addMessage (rep.time, id, "0x1D: Cancel hero revival 0x%08X%08X",
            * (int*) (rep.data + rep.pos), * (int*) (rep.data + rep.pos + 4));
          rep.pos += 8;
        }
        else
        {
          if (rep.data[rep.pos] == 0)
            addMessage (rep.time, id, "0x1D: Cancel training/upgrading %s",
              formatOrder (* (int*) (rep.data + rep.pos + 1)));
          else
            addMessage (rep.time, id, "0x1D: Remove unit/upgrade #%d from queue (%s)",
              int (rep.data[rep.pos]), formatOrder (* (int*) (rep.data + rep.pos + 1)));
          rep.pos += 5;
        }
        break;
      case 0x1E:
        if (rep.data[rep.pos] == 0)
          addMessage (rep.time, id, "0x1E: Cancel training/upgrading %s",
            formatOrder (* (int*) (rep.data + rep.pos + 1)));
        else
          addMessage (rep.time, id, "0x1E: Remove unit/upgrade #%d from queue (%s)",
            int (rep.data[rep.pos]), formatOrder (* (int*) (rep.data + rep.pos + 1)));
        rep.pos += 5;
        break;
      case 0x21:
        rep.pos += 8;
        break;
      case 0x20:
        addMessage (rep.time, id, "0x20: Single player cheat 'TheDudeAbides' (Fast cooldown)");
        break;
      case 0x22:
        addMessage (rep.time, id, "0x22: Single player cheat 'SomebodySetUpUsTheBomb' (Instant defeat)");
        break;
      case 0x23:
        addMessage (rep.time, id, "0x23: Single player cheat 'WarpTen' (Speeds construction)");
        break;
      case 0x24:
        addMessage (rep.time, id, "0x24: Single player cheat 'IocainePowder' (Fast Death/Decay)");
        break;
      case 0x25:
        addMessage (rep.time, id, "0x25: Single player cheat 'PointBreak' (Removes food limit)");
        break;
      case 0x26:
        addMessage (rep.time, id, "0x26: Single player cheat 'WhosYourDaddy' (God mode)");
        break;
      case 0x27:
        addMessage (rep.time, id, "0x27: Single player cheat 'KeyserSoze' (Gives %d Gold)",
          * (int*) (rep.data + rep.pos + 1));
        rep.pos += 5;
        break;
      case 0x28:
        addMessage (rep.time, id, "0x28: Single player cheat 'LeafitToMe' (Gives %d Lumber)",
          * (int*) (rep.data + rep.pos + 1));
        rep.pos += 5;
        break;
      case 0x29:
        addMessage (rep.time, id, "0x29: Single player cheat 'ThereIsNoSpoon' (Unlimited Mana)");
        break;
      case 0x2A:
        addMessage (rep.time, id, "0x2A: Single player cheat 'StrengthAndHonor' (No defeat)");
        break;
      case 0x2B:
        addMessage (rep.time, id, "0x2B: Single player cheat 'ItVexesMe' (Disable victory conditions)");
        break;
      case 0x2C:
        addMessage (rep.time, id, "0x2C: Single player cheat 'WhoIsJohnGalt' (Enable research)");
        break;
      case 0x2D:
        addMessage (rep.time, id, "0x2D: Single player cheat 'GreedIsGood' (Gives %d Gold and Lumber)",
          * (int*) (rep.data + rep.pos + 1));
        rep.pos += 5;
        break;
      case 0x2E:
        addMessage (rep.time, id, "0x2E: Single player cheat 'DayLightSavings' (Set time of day to %.2f)",
          * (float*) (rep.data + rep.pos));
        rep.pos += 4;
        break;
      case 0x2F:
        addMessage (rep.time, id, "0x2F: Single player cheat 'ISeeDeadPeople' (Remove fog of war)");
        break;
      case 0x30:
        addMessage (rep.time, id, "0x30: Single player cheat 'Synergy' (Disable tech tree requirements)");
        break;
      case 0x31:
        addMessage (rep.time, id, "0x31: Single player cheat 'SharpAndShiny' (Research upgrades)");
        break;
      case 0x32:
        addMessage (rep.time, id, "0x32: Single player cheat 'AllYourBaseAreBelongToUs' (Instant victory)");
        break;
      case 0x50:
        {
          unsigned char slot = rep.data[rep.pos];
          unsigned long flags = * (unsigned long*) (rep.data + rep.pos + 1);
          rep.pos += 5;
          char* buf = mprintf ("0x50: Change ally options towards %s: ", rep.players[slot].name);
          bool first = true;
          if (flags & 0x1F)
          {
            if (!first) strcat (buf, ", ");
            first = false;
            strcat (buf, "allied");
          }
          if (flags & 0x20)
          {
            if (!first) strcat (buf, ", ");
            first = false;
            strcat (buf, "vision");
          }
          if (flags & 0x40)
          {
            if (!first) strcat (buf, ", ");
            first = false;
            strcat (buf, "control");
          }
          if (flags & 0x400)
          {
            if (!first) strcat (buf, ", ");
            first = false;
            strcat (buf, "victory");
          }
          if (first)
            strcat (buf, "enemy");
          addMessage (rep.time, id, buf);
        }
        break;
      case 0x51:
        {
          unsigned char slot = rep.data[rep.pos];
          unsigned long gold = * (unsigned long*) (rep.data + rep.pos + 1);
          unsigned long lumber = * (unsigned long*) (rep.data + rep.pos + 5);
          rep.pos += 9;
          addMessage (rep.time, id, "0x51: Send %d gold and %d lumber to %s", gold, lumber,
            rep.players[slot].name);
        }
        break;
      case 0x60:
        rep.pos += 8;
        addMessage (rep.time, id, "0x60: [Chat command] %s", rep.data + rep.pos);
        while (rep.data[rep.pos++])
          ;
        break;
      case 0x61:
        addMessage (rep.time, id, "0x61: Escape pressed");
        break;
      case 0x62:
        rep.pos += 8;
        if (rep.hdr.major_v >= 7)
          rep.pos += 4;
        break;
      case 0x65:
        break;
      case 0x66:
        break;
      case 0x67:
        if (rep.hdr.major_v < 7)
        {
          float posx = * (float*) (rep.data + rep.pos);
          float posy = * (float*) (rep.data + rep.pos + 4);
          rep.pos += 12;
          addMessage (rep.time, id, "0x67: Map ping (X: %.1f, Y: %.1f)", posx, posy);
        }
        break;
      case 0x68:
        if (rep.hdr.major_v >= 7)
        {
          float posx = * (float*) (rep.data + rep.pos);
          float posy = * (float*) (rep.data + rep.pos + 4);
          rep.pos += 12;
          addMessage (rep.time, id, "0x68: Map ping (X: %.1f, Y: %.1f)", posx, posy);
        }
        else
          rep.pos += 16;
        break;
      case 0x69:
      case 0x6A:
        rep.pos += 16;
        break;
      case 0x6B:
        {
          char* a = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          char* b = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          char* c = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          int value = * (int*) (rep.data + rep.pos);
          rep.pos += 4;
          if (map->getData ()->getUnitById (value) || isGoodId (value))
            addMessage (rep.time, 0, "0x6B: SyncStoredInteger (%s, %s, %s, [%s])",
              a, b, c, fmtId (value));
          else
            addMessage (rep.time, 0, "0x6B: SyncStoredInteger (%s, %s, %s, %d)",
              a, b, c, value);
        };
        break;
      case 0x70:
        {
          char* a = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          char* b = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          char* c = rep.data + rep.pos;
          while (rep.data[rep.pos++])
            ;
          addMessage (rep.time, 0, "0x6B: SyncStored? (%s, %s, %s)", a, b, c);
        };
        break;
      default:
        rep.pos++;
      }
      was_deselect = false;
      was_subgroup = false;
    }
    rep.pos = next;
  }
  rep.pos = end;
}

bool gzinflate2 (char* old, char* buf, int csize, int usize, gzmemory* mem = NULL);
bool CActionLogDlg::loadReplay (FILE* file)
{
  SmallReplay rep;
  memset (&rep, 0, sizeof rep);
  if (!rep.hdr.read_data (file))
    return false;
  fseek (file, rep.hdr.header_size, SEEK_SET);
  int tempSize = 1024;
  char* temp = new char[tempSize];
  int dataSize = 1024;
  rep.data = new char[dataSize];
  gzmemory mem;
  for (int i = 0; i < rep.hdr.blocks; i++)
  {
    W3GBlockHeader block;
    if (fread (&block, sizeof block, 1, file) != 1)
      return false;
    if (block.c_size > tempSize)
    {
      tempSize = block.c_size;
      delete[] temp;
      temp = new char[tempSize];
    }
    if (fread (temp, block.c_size, 1, file) != 1)
      return false;
    if (rep.size + block.u_size > dataSize)
    {
      while (dataSize < rep.size + block.u_size)
        dataSize *= 2;
      char* tdata = new char[dataSize];
      memcpy (tdata, rep.data, rep.size);
      delete[] rep.data;
      rep.data = tdata;
    }
    if (!gzinflate2 (temp, rep.data + rep.size, block.c_size, block.u_size, &mem))
      return false;
    rep.size += block.u_size;
  }
  rep.pos += 4;
  delete[] temp;

  _loadPlayer (rep);
  _loadGame (rep);

  int prev = 0;
  int leave_unknown = 0;
  while (rep.pos < rep.size)
  {
    int block_id = rep.data[rep.pos++];
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        short length = * (short*) (rep.data + rep.pos);
        short time_inc = * (short*) (rep.data + rep.pos + 2);
        rep.pos += 4;
        if (!rep.pause)
          rep.time += time_inc;
        if (length > 2)
          parseActions (length - 2, rep);
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (rep.hdr.major_v > 2)
      {
        unsigned char id = rep.data[rep.pos];
        short length = * (short*) (rep.data + rep.pos + 1);
        unsigned char flags = rep.data[rep.pos + 3];
        if (flags == 0x20)
        {
          static char text[512];
          unsigned long mode = * (unsigned long*) (rep.data + rep.pos + 4);
          if (mode > CHAT_PRIVATE)
            mode = CHAT_PRIVATE;
          strncpy (text, rep.data + rep.pos + 8, length - 6);
          text[length - 6] = 0;
          if (mode == CHAT_ALL)
            addMessage (rep.time, id, "[All] %s", text);
          else if (mode == CHAT_ALLIES)
            addMessage (rep.time, id, "[Allies] %s", text);
          else if (mode == CHAT_OBSERVERS)
            addMessage (rep.time, id, "[Observers] %s", text);
          else if (mode == CHAT_PRIVATE)
            addMessage (rep.time, id, "[Private] %s", text);
        }
        rep.pos += length + 3;
        break;
      }
    // unknown (Random number/seed for next frame)
    case 0x22:
      rep.pos += rep.data[rep.pos] + 1;
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      rep.pos += 4;
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      rep.pos += 10;
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      rep.pos += 8;
      break;
    // LeaveGame
    case 0x17:
      {
        unsigned char id = rep.data[rep.pos + 4];
        int reason = * (long*) (rep.data + rep.pos);
        int result = * (long*) (rep.data + rep.pos + 5);
        long unknown = * (long*) (rep.data + rep.pos + 9);
        rep.pos += 13;

        if (leave_unknown)
          leave_unknown = unknown - leave_unknown;
        addMessage (rep.time, id, "Left the game");
        leave_unknown = unknown;
      }
      break;
    case 0:
      rep.pos = rep.size;
      break;
    }
    prev = block_id;
  }

  memcpy (actions.players, rep.players, sizeof rep.players);
  delete[] rep.data;

  searchPlayer.Reset ();
  searchPlayer.InsertItem ("All actions", getImageIndex ("Unknown"), 0xFFFFFF, -1);
  searchPlayer.InsertItem ("System actions (SyncStoredInteger)", getImageIndex ("IconReplay"), 0xFFFFFF, 0);
  if (w3g->dota.isDota)
  {
    searchPlayer.InsertItem ("The Sentinel", getImageIndex ("RedBullet"), 0);
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
    searchPlayer.InsertItem ("The Scourge", getImageIndex ("GreenBullet"), 0);
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
  }
  else
  {
    for (int i = 0; i < w3g->numPlayers; i++)
      searchPlayer.InsertItem (w3g->players[w3g->pindex[i]].uname,
        getImageIndex (raceImage[w3g->players[w3g->pindex[i]].race]),
        getLightColor (w3g->players[w3g->pindex[i]].slot.color), w3g->pindex[i]);
  }
  searchPlayer.SetCurSel (0);

  return true;
}

void CActionLogDlg::OnBnClickedLoadactions()
{
  GetDlgItem (IDC_LOADACTIONS)->EnableWindow (FALSE);
  SetDlgItemText (IDC_LOADACTIONS, "Loading...");
  map = new MapData (w3g->game.map);
  if (map->isLoaded ())
  {
    actions.EnableWindow (TRUE);
    actions.map = map;
    actions.SetImageList (map->getImageList (), LVSIL_SMALL);
    FILE* file = fopen (w3g->filepath, "rb");
    if (file)
    {
      actions.LockWindowUpdate ();
      loadReplay (file);
      actions.UnlockWindowUpdate ();
      fclose (file);
      SetDlgItemText (IDC_LOADACTIONS, "Done");
    }
    GetDlgItem (IDC_SEARCHPLAYER)->EnableWindow (TRUE);
    GetDlgItem (IDC_SEARCHFOR)->EnableWindow (TRUE);
    GetDlgItem (IDC_SEARCHTYPE)->EnableWindow (TRUE);
    GetDlgItem (IDC_FINDNEXT)->EnableWindow (TRUE);
    GetDlgItem (IDC_FINDPREV)->EnableWindow (TRUE);
  }
  else
  {
    SetDlgItemText (IDC_LOADACTIONS, "Failed");
    MessageBox (mprintf ("Could not locate the map at %s.", w3g->game.map), "Error", MB_ICONHAND | MB_OK);
    delete map;
  }
}

void CActionLogDlg::OnBnClickedRawcodes()
{
  actions.rawcodes = (IsDlgButtonChecked (IDC_RAWCODES) != FALSE);
  actions.InvalidateRect (NULL);
}
void CActionLogDlg::OnSize (UINT nType, int cx, int cy)
{
  if (nType != SIZE_MINIMIZED)
    loc.Update ();
}

int transformString (char const* txt, char* buf, CActionLog const& actions, bool rawcodes)
{
  int id = 0;
  for (int i = 0; i < 3; i++)
    id = id * 10 + int (txt[i] - '0');
  int len = 0;
  if (rawcodes)
    strcpy (buf, txt + 3);
  else
  {
    for (int i = 3; txt[i]; i++)
    {
      if (txt[i] == '[')
      {
        int id = 0;
        int j = i;
        i++;
        while (txt[i] && txt[i] != ']')
          id = id * 256 + int ((unsigned char) txt[i++]);
        if (txt[i] == 0)
          i = j;
        else
        {
          UnitData* unit = actions.map->getData ()->getUnitById (id);
          if (unit)
          {
            buf[len++] = '[';
            strcpy (buf + len, unit->getStringData ("Name", 0));
            len += (int) strlen (buf + len);
          }
          else
            i = j;
        }
      }
      buf[len++] = txt[i];
    }
    buf[len] = 0;
  }
  return id;
}

void CActionLogDlg::OnNMRclickActions(NMHDR *pNMHDR, LRESULT *pResult)
{
  CPoint pt;
  GetCursorPos (&pt);
  int sel = actions.GetNextItem (-1, LVNI_SELECTED);
  if (sel < 0 || map == NULL)
    return;
  int res = popupMenu.TrackPopupMenuEx (TPM_HORIZONTAL | TPM_LEFTALIGN | TPM_RETURNCMD,
    pt.x, pt.y, this, NULL);
  switch (res)
  {
  case ID_ACT_COPY:
    {
      static char buf[4096];
      static char txt[4096];
      static char tmp[4096];
      int curlen = 0;
      int maxlen = 4096;
      wchar_t* wide = new wchar_t[maxlen];
      bool first = true;
      while (sel >= 0)
      {
        actions.GetItemText (sel, 0, txt, sizeof txt);
        int id = transformString (txt, tmp, actions, actions.rawcodes);
        if (id)
          sprintf (buf, "%s%6s <%s> %s", first ? "" : "\n", format_time (w3g, actions.GetItemData (sel)),
            actions.players[id].name, tmp);
        else
          sprintf (buf, "%s%6s %s", first ? "" : "\n", format_time (w3g, actions.GetItemData (sel)), tmp);

        int count = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, NULL, 0);
        if (count + curlen > maxlen)
        {
          while (count + curlen > maxlen)
            maxlen *= 2;
          wchar_t* temp = new wchar_t[maxlen];
          memcpy (temp, wide, sizeof (wchar_t) * curlen);
          delete[] wide;
          wide = temp;
        }
        curlen += MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, wide + curlen, maxlen - curlen) - 1;

        first = false;
        sel = actions.GetNextItem (sel, LVNI_SELECTED);
      }
      if (OpenClipboard ())
      {
        EmptyClipboard ();
        int sz = (int) wcslen (wide);
        HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
        if (handle)
        {
          wchar_t* copy = (wchar_t*) GlobalLock (handle);
          memcpy (copy, wide, sz * 2 + 2);
          GlobalUnlock (handle);
          SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
        }
        CloseClipboard ();
      }
      delete[] wide;
    }
    break;
  }
  *pResult = 0;
}


void CActionLogDlg::OnBnClickedFindnext()
{
  bool findprev = (LOWORD (GetCurrentMessage ()->wParam) == IDC_FINDPREV);
  static char txt[4096];
  static char buf[4096];
  static char searchFor[4096];
  if (w3g && map)
  {
    int searchType = ((CComboBox*) GetDlgItem (IDC_SEARCHTYPE))->GetCurSel ();
    int playerId = -1;
    int searchLen;
    GetDlgItemText (IDC_SEARCHFOR, searchFor, sizeof searchFor);
    searchLen = (int) strlen (searchFor);
    int sel = searchPlayer.GetCurSel ();
    if (sel >= 0)
      playerId = searchPlayer.GetItemDataEx (sel);
    int curLine = actions.GetNextItem (-1, LVNI_SELECTED) + (findprev ? -1 : 1);
    if (curLine >= actions.GetItemCount ())
      curLine = 0;
    if (curLine < 0)
      curLine = actions.GetItemCount () - 1;
    int searchStart = curLine;
    int searchCount = 0;
    while (searchCount == 0 || curLine != searchStart)
    {
      actions.GetItemText (curLine, 0, txt, sizeof txt);
      int id = transformString (txt, buf, actions, false);

      if (playerId < 0 || id == playerId)
      {
        if (searchType == 0)
        {
          if (stristr (txt + 3, searchFor) != NULL)
            break;
          if (stristr (buf, searchFor) != NULL)
            break;
        }
        else if (searchType == 1)
        {
          if (!stricmp (txt + 3, searchFor))
            break;
          if (!stricmp (buf, searchFor))
            break;
        }
        else if (searchType == 2)
        {
          if (!strnicmp (txt + 3, searchFor, searchLen))
            break;
          if (!strnicmp (buf, searchFor, searchLen))
            break;
        }
      }

      curLine += (findprev ? -1 : 1);
      searchCount++;
      if (curLine >= actions.GetItemCount ())
        curLine = 0;
      if (curLine < 0)
        curLine = actions.GetItemCount () - 1;
    }
    if (searchCount != 0 && curLine == searchStart)
      MessageBox ("Nothing found!");
    else
    {
      for (int i = actions.GetNextItem (-1, LVIS_SELECTED); i >= 0;
        i = actions.GetNextItem (i, LVIS_SELECTED))
        actions.SetItemState (i, 0, LVIS_SELECTED);
      actions.SetItemState (curLine, LVIS_SELECTED, LVIS_SELECTED);
      actions.EnsureVisible (curLine, FALSE);
      actions.SetFocus ();
    }
  }
}
