#include "stdafx.h"
#include "utils.h"

BOOL LVSetItemText (CListCtrl const& l, int nItem, int nSubItem, wchar_t const* lpszText)
{
	ASSERT(::IsWindow(l.m_hWnd));
	ASSERT((l.GetStyle() & LVS_OWNERDATA)==0);
	LVITEMW lvi;
	lvi.iSubItem = nSubItem;
	lvi.pszText = (LPWSTR) lpszText;
	return (BOOL) SendMessage(l.m_hWnd, LVM_SETITEMTEXTW, nItem, (LPARAM)&lvi);
}
int LVGetItemText (CListCtrl const& l, int nItem, int nSubItem, wchar_t* lpszText, int nLen)
{
	ASSERT(::IsWindow(l.m_hWnd));
	LVITEMW lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.iSubItem = nSubItem;
	lvi.cchTextMax = nLen;
	lvi.pszText = lpszText;
	return (int) SendMessage(l.m_hWnd, LVM_GETITEMTEXTW, (WPARAM)nItem,
		(LPARAM)&lvi);
}
int LVInsertItem (CListCtrl const& l, int nItem, wchar_t const* lpszItem)
{
  ASSERT(::IsWindow(l.m_hWnd));
	LVITEMW item;
	item.mask = LVIF_TEXT;
	item.iItem = nItem;
	item.iSubItem = 0;
	item.pszText = (wchar_t*) lpszItem;
	item.state = 0;
	item.stateMask = 0;
	item.iImage = 0;
	item.lParam = 0;
  return (int) SendMessage(l.m_hWnd, LVM_INSERTITEMW, 0, (LPARAM)&item);
}
int LVInsertItem (CListCtrl const& l, int nItem, wchar_t const* lpszItem, int image)
{
  ASSERT(::IsWindow(l.m_hWnd));
	LVITEMW item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = nItem;
	item.iSubItem = 0;
	item.pszText = (wchar_t*) lpszItem;
	item.state = 0;
	item.stateMask = 0;
	item.iImage = image;
	item.lParam = 0;
  return (int) SendMessage(l.m_hWnd, LVM_INSERTITEMW, 0, (LPARAM)&item);
}

char tbufs[32][8192];
int curtb = 0;
char* format_time (long time, int flags)
{
  char* buf = tbufs[curtb++];
  char* res = buf;
  if (time < 0)
  {
    *buf++ = '-';
    time = -time;
  }
  if (curtb >= 32) curtb = 0;
  time /= 1000;
  if (time >= 3600 && (flags & TIME_HOURS))
    sprintf (buf, "%d:%02d", time / 3600, (time / 60) % 60);
  else
    sprintf (buf, "%d", time / 60);
  if (flags & TIME_SECONDS)
    strcat (buf, mprintf (":%02d", time % 60));
  return res;
}

char* mprintf (char const* fmt, ...)
{
  char* buf = tbufs[curtb++];
  if (curtb >= 32) curtb = 0;
  va_list ap;
  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);
  va_end (ap);
  return buf;
}
wchar_t* wmprintf (wchar_t const* fmt, ...)
{
  wchar_t* buf = (wchar_t*) tbufs[curtb++];
  if (curtb >= 32) curtb = 0;
  va_list ap;
  va_start (ap, fmt);
  vswprintf (buf, fmt, ap);
  va_end (ap);
  return buf;
}
wchar_t* makeucd (char const* str)
{
  wchar_t* buf = (wchar_t*) tbufs[curtb++];
  if (curtb >= 32) curtb = 0;
//  MultiByteToWideChar (CP_UTF8, 0, (LPCSTR) str, -1, buf, 512);
  for (int i = 0; buf[i] = str[i]; i++)
    ;
  return buf;
}

char* sanitize (wchar_t const* str)
{
  char* buf = tbufs[curtb++];
  if (curtb >= 32) curtb = 0;
  int sz = 0;
  for (int i = 0; str[i]; i++)
  {
    if (iswascii (str[i]) && str[i] != '{' && str[i] != '}')
    {
      buf[sz++] = (char) str[i];
      if (str[i] == '\\')
        buf[sz++] = '\\';
    }
    else
    {
      sprintf (buf + sz, "\\u%d?", (int) str[i]);
      while (buf[sz])
        sz++;
    }
  }
  buf[sz] = 0;
  return buf;
}

void ShowError ()
{
  LPVOID lpMsgBuf;
  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, GetLastError (), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
  MessageBox (NULL, (LPCTSTR) lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);
  LocalFree (lpMsgBuf);
}

int makeID (char const* str)
{
  if (str[0] == 0) return 0;
  return ((int) str[0] << 24) | ((int) str[1] << 16) | ((int) str[2] << 8) | str[3];
}

int TrieNode::c2i (char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c == ' ') return 36;
  return 0;
}
char TrieNode::i2c (int c)
{
  if (c < 10) return c + '0';
  return c - 10 + 'a';
}
int TrieNode62::c2i (char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 36;
  return 0;
}
char TrieNode62::i2c (int c)
{
  if (c < 10) return c + '0';
  if (c < 36) return c - 10 + 'a';
  return c - 36 + 'A';
}

TrieNode* addString (TrieNode* node, char const* str, int id)
{
  if (node == NULL)
  {
    node = new TrieNode;
    memset (node, 0, sizeof TrieNode);
  }
  if (*str == 0)
    node->id = id;
  else
  {
    int c = TrieNode::c2i (*str++);
    node->c[c] = addString (node->c[c], str, id);
  }
  return node;
}
TrieNode* saddString (TrieNode* node, char const* str, int id)
{
  if (node == NULL)
  {
    node = new TrieNode;
    memset (node, 0, sizeof TrieNode);
  }
  while (*str == ' ') str++;
  if (*str == 0)
    node->id = id;
  else
  {
    int c = TrieNode::c2i (*str++);
    node->c[c] = saddString (node->c[c], str, id);
  }
  return node;
}

TrieNode::~TrieNode ()
{
  for (int i = 0; i < 37; i++)
    delete c[i];
}

int getValue (TrieNode const* node, char const* str)
{
  while (node)
  {
    if (*str == 0)
      return node->id;
    node = node->c[TrieNode::c2i (*str++)];
  }
  return 0;
}
int sgetValue (TrieNode const* node, char const* str)
{
  while (node)
  {
    while (*str == ' ') str++;
    if (*str == 0)
      return node->id;
    node = node->c[TrieNode::c2i (*str++)];
  }
  return 0;
}

TrieNode256* addString (TrieNode256* node, char const* str, int id)
{
  if (node == NULL)
  {
    node = new TrieNode256;
    memset (node, 0, sizeof TrieNode256);
  }
  if (*str == 0)
    node->id = id;
  else
  {
    int c = (*str++) & 0xFF;
    node->c[c] = addString (node->c[c], str, id);
  }
  return node;
}

TrieNode256::~TrieNode256 ()
{
  for (int i = 0; i < 256; i++)
    delete c[i];
}

int getValue (TrieNode256 const* node, char const* str)
{
  while (node)
  {
    if (*str == 0)
      return node->id;
    node = node->c[(*str++) & 0xFF];
  }
  return 0;
}

TrieNode62* addString (TrieNode62* node, char const* str, int id)
{
  if (node == NULL)
  {
    node = new TrieNode62;
    memset (node, 0, sizeof TrieNode62);
  }
  if (*str == 0)
    node->id = id;
  else
  {
    int c = TrieNode62::c2i (*str++);
    node->c[c] = addString (node->c[c], str, id);
  }
  return node;
}

TrieNode62::~TrieNode62 ()
{
  for (int i = 0; i < 62; i++)
    delete c[i];
}

int getValue (TrieNode62 const* node, char const* str)
{
  while (node)
  {
    if (*str == 0)
      return node->id;
    node = node->c[TrieNode62::c2i (*str++)];
  }
  return 0;
}

int flipInt (int x)
{
  return ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) |
         ((x << 24) & 0xFF000000) | ((x << 8) & 0x00FF0000);
}

void stripstr (char* str)
{
  int shift = 0;
  while (str[shift] == ' ') shift++;
  int i = 0;
  while (str[i] = str[i + shift]) i++;
  while (i && (str[i - 1] == ' ' || str[i - 1] == '\r' || str[i - 1] == '\n'))
    str[--i] = 0;
}

unsigned int parseVersion (char const* str)
{
  if (str == NULL) return 0;
  int len = (int) strlen (str);
  while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || str[len - 1] == ' ' || str[len - 1] == '*'))
    len--;
  if (len < 4 || len > 5) return 0;
  if (str[0] < '0' || str[0] > '9' || str[1] != '.' ||
      str[2] < '0' || str[2] > '9' || str[3] < '0' || str[3] > '9' ||
      (len == 5 && (str[4] < 'a' || str[4] > 'z')))
    return 0;
  unsigned int v = (str[0] - '0') * 2600 +
                   (str[2] - '0') * 260 +
                   (str[3] - '0') * 26;
  if (len == 5) v += str[4] - 'a';
  return v;
}
char const* formatVersion (unsigned long v)
{
  char* res = mprintf ("");
  res[0] = char ((v / 2600) + '0');
  res[1] = '.';
  res[2] = char (((v / 260) % 10) + '0');
  res[3] = char (((v / 26) % 10) + '0');
  if (v % 26)
  {
    res[4] = char ('a' + (v % 26));
    res[5] = 0;
  }
  else
    res[4] = 0;
  return res;
}

wchar_t const* wcsistr (wchar_t const* str, wchar_t const* sub)
{
  if (str == NULL || sub == NULL)
    return NULL;
  if (*sub == 0)
    return str;
  for (int i = 0; str[i]; i++)
  {
    if (towlower (str[i]) == towlower (sub[0]))
    {
      bool good = true;
      for (int j = 1; sub[j] && good; j++)
        if (towlower (sub[j]) != towlower (str[i + j]))
          good = false;
      if (good)
        return str + i;
    }
  }
  return NULL;
}
char const* stristr (char const* str, char const* sub)
{
  if (str == NULL || sub == NULL)
    return NULL;
  if (*sub == 0)
    return str;
  for (int i = 0; str[i]; i++)
  {
    if (tolower (str[i]) == tolower (sub[0]))
    {
      bool good = true;
      for (int j = 1; sub[j] && good; j++)
        if (tolower (sub[j]) != tolower (str[i + j]))
          good = false;
      if (good)
        return str + i;
    }
  }
  return NULL;
}
wchar_t const* wstristr (wchar_t const* string, wchar_t const* strSearch)
{
  return wcsistr (string, strSearch);
}

char* clonestr (char const* str)
{
  char* buf = new char[strlen (str) + 1];
  strcpy (buf, str);
  return buf;
}
