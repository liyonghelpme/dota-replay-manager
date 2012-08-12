#ifndef __UTILS_H__
#define __UTILS_H__

#define TIME_HOURS      1
#define TIME_SECONDS    2
class W3GReplay;
char* format_time (W3GReplay* w3g, unsigned long time, int flags = TIME_SECONDS);
char* format_time (long time, int flags = TIME_SECONDS);
char* mprintf (char const* fmt, ...);
wchar_t* wmprintf (wchar_t const* fmt, ...);
wchar_t* makeucd (char const* str);
char* clonestr (char const* str);
char* sanitize (wchar_t const* str);
bool is_substr (char const* a, char const* b);
BOOL LVSetItemText(CListCtrl const& l, int nItem, int nSubItem, wchar_t const* lpszText);
int LVGetItemText(CListCtrl const& l, int nItem, int nSubItem, wchar_t* lpszText, int nLen);
int LVInsertItem (CListCtrl const& l, int nItem, wchar_t const* lpszItem);
int LVInsertItem (CListCtrl const& l, int nItem, wchar_t const* lpszItem, int image);
wchar_t const* wcsistr (wchar_t const* str, wchar_t const* sub);
char const* stristr (char const* string, char const* strSearch);
wchar_t const* wstristr (wchar_t const* string, wchar_t const* strSearch);

void ShowError ();

int makeID (char const* str);
char* make_id (unsigned long id);
int flipInt (int x);

struct TrieNode
{
  TrieNode* c[37];
  int id;
  static int c2i (char c);
  static char i2c (int i);
  ~TrieNode ();
};
TrieNode* addString (TrieNode* node, char const* str, int id);
int getValue (TrieNode const* node, char const* str);
TrieNode* saddString (TrieNode* node, char const* str, int id);
int sgetValue (TrieNode const* node, char const* str);

struct TrieNode62
{
  TrieNode62* c[62];
  int id;
  static int c2i (char c);
  static char i2c (int i);
  ~TrieNode62 ();
};
TrieNode62* addString (TrieNode62* node, char const* str, int id);
int getValue (TrieNode62 const* node, char const* str);

struct TrieNode256
{
  TrieNode256* c[256];
  int id;
  ~TrieNode256 ();
};
TrieNode256* addString (TrieNode256* node, char const* str, int id);
int getValue (TrieNode256 const* node, char const* str);

void stripstr (char* str);

unsigned int parseVersion (char const* str);
char const* formatVersion (unsigned long v);

#endif
