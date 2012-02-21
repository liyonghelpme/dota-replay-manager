#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <locale>
#include <windows.h>
#include "strobj.h"
#include "winuser.h"

#define BASIC_ALLOC         16
#define ALLOC_STEP          1024

#define _size(b)    (*(int*)((b)-4))
#define _ref(b)     (*(int*)((b)-8))
#define _len(b)     (*(int*)((b)-12))
#define _del(b)     (delete ((b)-12))

static inline char* _new (int size)
{
  char* val = new char[size + 12];
  * (int*) (val + 8) = size;
  return val + 12;
}

static char __empty[] = {0, 0, 0, 0,
                         1, 0, 0, 0,
                         1, 0, 0, 0,
                         0};

String String::null;

String::String ()
{
  buf = __empty + 12;
  _ref (buf)++;
}

String::String (char const* str)
{
  int len = (int) strlen (str);
  buf = _new (len + 1);
  _len (buf) = len;
  _ref (buf) = 1;
  memcpy (buf, str, len + 1);
}

String::String (char const* str, int length)
{
  buf = _new (length + 1);
  _len (buf) = length;
  _ref (buf) = 1;
  memcpy (buf, str, length);
  buf[length] = 0;
}

String::String (char value)
{
  buf = _new (2);
  _len (buf) = 1;
  _ref (buf) = 1;
  buf[0] = value;
  buf[1] = 0;
}

String::String (String const& str)
  : buf (str.buf)
{
  _ref (buf)++;
}

String::String (int value)
{
  buf = _new (BASIC_ALLOC);
  _ref (buf) = 1;
  itoa (value, buf, 10);
  _len (buf) = (int) strlen (buf);
}

String::String (float value)
{
  buf = _new (BASIC_ALLOC);
  _ref (buf) = 1;
  sprintf (buf, "%f", value);
  _len (buf) = (int) strlen (buf);
}

String& String::operator = (String const& str)
{
  if (&str == this)
    return *this;

  deref ();
  buf = str.buf;
  if (buf)
    _ref (buf)++;
  return *this;
}

String& String::operator += (char ch)
{
  if (_ref (buf) > 1) splice ();
  if (_len (buf) + 1 >= _size (buf))
    realloc (_len (buf) + 1);

  buf[_len (buf)++] = ch;
  buf[_len (buf)] = 0;

  return *this;
}

int String::toInt () const
{
  return atoi (buf);
}
bool String::isInt () const
{
  int i = 0;
  while (buf[i] == ' ' || buf [i] == '\t')
    i++;
  if (buf[i] == '+' || buf[i] == '-')
    i++;
  return (buf[i] >= '0' && buf[i] <= '9');
}

float String::toFloat () const
{
  return (float) atof (buf);
}

bool String::isFloat () const
{
  int i = 0;
  while (buf[i] == ' ' || buf[i] == '\t')
    i++;
  if (buf[i] == '+' || buf[i] == '-')
    i++;
  int count = 0;
  while (buf[i] >= '0' && buf[i] <= '9')
    i++, count++;
  if (buf[i] == '.')
  {
    i++;
    while (buf[i] >= '0' && buf[i] <= '9')
      i++, count++;
  }
  return (count != 0);
}

void String::splice ()
{
  char* tmp = _new (_size (buf));
  _len (tmp) = _len (buf);
  _ref (tmp) = 1;
  memcpy (tmp, buf, _len (buf) + 1);
  deref ();
  buf = tmp;
}

void String::realloc (int newlen)
{
  newlen++;
  if (newlen <= _size (buf))
    return;
  int newsize = (_size (buf) < ALLOC_STEP ? _size (buf) * 2 : _size (buf) + ALLOC_STEP);
  if (newsize < newlen)
    newsize = newlen;
  char* tmp = _new (newsize);
  _len (tmp) = _len (buf);
  _ref (tmp) = _ref (buf);
  memcpy (tmp, buf, _len (buf) + 1);
  _del (buf);
  buf = tmp;
}

void String::append (char const* str)
{
  if (_ref (buf) > 1) splice ();
  int len = (int) strlen (str);
  realloc (_len (buf) + len);
  memcpy (buf + _len (buf), str, len + 1);
  _len (buf) += len;
}

String String::substr (int from, int len) const
{
  if (from < 0) from += _len (buf);
  if (from < 0) from = 0;
  if (from > _len (buf)) from = _len (buf);
  if (len == toTheEnd) len = _len (buf) - from;
  else if (len < 0) len += _len (buf) - from;
  else if (from + len > _len (buf)) len = _len (buf) - from;
  if (len < 0) len = 0;
  char* str = _new (len + 1);
  _len (str) = len;
  _ref (str) = 1;
  if (len)
    memcpy (str, buf + from, len);
  str[len] = 0;
  return frombuf (str);
}

String String::substring ( int from, int to ) const
{
  if (from < 0) from += _len (buf);
  if (from < 0) from = 0;
  if (from > _len (buf)) from = _len (buf);
  if (to < 0) to += _len (buf);
  else if (to == toTheEnd) to = _len (buf);
  else if (to > _len (buf)) to = _len (buf);
  int len = to - from;
  if (len < 0) len = 0;
  char* str = _new (len + 1);
  _len (str) = len;
  _ref (str) = 1;
  if (len)
    memcpy (str, buf + from, len);
  str[len] = 0;
  return frombuf (str);
}

String& String::removeLeadingSpaces ()
{
  int i = 0;
  while (buf[i] && isspace (buf[i]))
    i++;
  if (i)
  {
    if (_ref (buf) > 1) splice ();
    _len (buf) -= i;
    memmove (buf, buf + i, _len (buf) + 1);
  }
  return *this;
}

String& String::removeTrailingSpaces ()
{
  int i = _len (buf);
  while (i > 0 && isspace (buf[i - 1]))
    i--;
  if (i < _len (buf))
  {
    if (_ref (buf) > 1) splice ();
    buf[_len (buf) = i] = 0;
  }
  return *this;
}

String& String::trim ()
{
  removeTrailingSpaces ();
  removeLeadingSpaces ();
  return *this;
}

String& String::dequote ()
{
  if (_len (buf) >= 2 && (
    (buf[0] == '\"' && buf[_len (buf) - 1] == '\"') ||
    (buf[0] == '\'' && buf[_len (buf) - 1] == '\'')))
  {
    if (_ref (buf) > 1) splice ();
    memmove (buf, buf + 1, _len (buf) -= 2);
    buf[_len (buf)] = 0;
  }
  return *this;
}

String& String::cut (int from, int len)
{
  if (from < 0) from += _len (buf);
  if (from < 0) from = 0;
  if (from > _len (buf)) from = _len (buf);
  if (len == toTheEnd) len = _len (buf) - from;
  else if (len < 0) len += _len (buf) - from;
  else if (from + len > _len (buf)) len = _len (buf) - from;
  if (len > 0)
  {
    if (_ref (buf) > 1) splice ();
    if (from + len < _len (buf))
      memmove (buf + from, buf + from + len, _len (buf) - from - len);
    buf[_len (buf) -= len] = 0;
  }
  return *this;
}

bool String::isWordBoundary (int pos) const
{
  return pos <= 0 || pos >= _len (buf) || isIdChar (buf[pos]) != isIdChar (buf[pos - 1]);
}
int String::find (char const* str, int start, int options) const
{
  static int kmpbuf[256];
#define comp(a,b) (options&FIND_CASE_INSENSITIVE?tolower((unsigned char)a)==tolower((unsigned char)b):(a)==(b))
  if (start < 0) start += _len (buf);
  if (start < 0) start = 0;
  if (start > _len (buf)) start = _len (buf);
  if (*str == 0) return start;
  int len = (int) strlen (str);
  int* kmp = kmpbuf;
  if (len > 256)
    kmp = new int[len];
  kmp[0] = 0;
  int result = -1;
  if (options & FIND_REVERSE)
  {
    for (int i = 1; i < len; i++)
    {
      kmp[i] = kmp[i - 1];
      while (kmp[i] && !comp (str[len - 1 - kmp[i]], str[len - 1 - i]))
        kmp[i] = kmp[kmp[i] - 1];
      if (comp (str[len - 1 - kmp[i]], str[len - 1 - i]))
        kmp[i]++;
    }
    int cur = 0;
    for (int i = start - 1; i >= 0; i--)
    {
      while (cur && !comp (str[len - 1 - cur], buf[_len (buf) - 1 - i]))
        cur = kmp[cur - 1];
      if (comp (str[len - 1 - cur], buf[_len (buf) - 1 - i]))
        cur++;
      if (cur == len)
      {
        if ((options & FIND_WHOLE_WORD) == 0 || (
          isWordBoundary (i) && isWordBoundary (i + len)))
        {
          result = i;
          break;
        }
      }
    }
  }
  else
  {
    for (int i = 1; i < len; i++)
    {
      kmp[i] = kmp[i - 1];
      while (kmp[i] && !comp (str[kmp[i]], str[i]))
        kmp[i] = kmp[kmp[i] - 1];
      if (comp (str[kmp[i]], str[i]))
        kmp[i]++;
    }
    int cur = 0;
    for (int i = start; i < _len (buf); i++)
    {
      while (cur && !comp (str[cur], buf[i]))
        cur = kmp[cur - 1];
      if (comp (str[cur], buf[i]))
        cur++;
      if (cur == len)
      {
        if ((options & FIND_WHOLE_WORD) == 0 || (
          isWordBoundary (i - len + 1) && isWordBoundary (i + 1)))
        {
          result = i - len + 1;
          break;
        }
      }
    }
  }
  if (kmp != kmpbuf)
    delete[] kmp;
  return result;
}

int String::find (char ch, int start, int options) const
{
  if (start < 0) start += _len (buf);
  if (start < 0) start = 0;
  if (start > _len (buf)) start = _len (buf);
  if (options & FIND_CASE_INSENSITIVE)
    ch = tolower ((unsigned char) ch);
  if (options & FIND_REVERSE)
  {
    for (int i = start - 1; i >= 0; i--)
    {
      if ((options & FIND_CASE_INSENSITIVE ? tolower ((unsigned char) buf[i]) == ch : buf[i] == ch) &&
        ((options & FIND_WHOLE_WORD) == 0 || (
        isWordBoundary (i) && isWordBoundary (i + 1))))
        return i;
    }
  }
  else
  {
    for (int i = start; i < _len (buf); i++)
    {
      if ((options & FIND_CASE_INSENSITIVE ? tolower ((unsigned char) buf[i]) == ch : buf[i] == ch) &&
        ((options & FIND_WHOLE_WORD) == 0 || (
        isWordBoundary (i) && isWordBoundary (i + 1))))
        return i;
    }
  }
  return -1;
}

String& String::replace (int start, int len, String str)
{
  if (_ref (buf) > 1) splice ();
  if (start < 0) start += _len (buf);
  if (start < 0) start = 0;
  if (start > _len (buf)) start = _len (buf);
  if (len == toTheEnd) len = _len (buf) - start;
  else if (len < 0) len += _len (buf) - start;
  else if (start + len > _len (buf)) len = _len (buf) - start;
  if (len < 0) len = 0;
  int newlen = _len (buf) - len + _len (str.buf);
  realloc (newlen);
  int b = start + len;
  int bnew = start + _len (str.buf);
  if (b < _len (buf))
    memmove (buf + bnew, buf + b, _len (buf) - b);
  if (_len (str.buf))
    memcpy (buf + start, str.buf, _len (str.buf));
  buf[newlen] = 0;
  _len (buf) = newlen;
  return *this;
}
String& String::replace (int start, int len, char const* str)
{
  if (_ref (buf) > 1) splice ();
  int strLength = (int) strlen (str);
  if (start < 0) start += _len (buf);
  if (start < 0) start = 0;
  if (start > _len (buf)) start = _len (buf);
  if (len == toTheEnd) len = _len (buf) - start;
  else if (len < 0) len += _len (buf) - start;
  else if (start + len > _len (buf)) len = _len (buf) - start;
  if (len < 0) len = 0;
  int newlen = _len (buf) - len + strLength;
  realloc (newlen);
  int b = start + len;
  int bnew = start + strLength;
  if (b < _len (buf))
    memmove (buf + bnew, buf + b, _len (buf) - b);
  if (strLength)
    memcpy (buf + start, str, strLength);
  buf[newlen] = 0;
  _len (buf) = newlen;
  return *this;
}
String& String::replace (int pos, char ch)
{
  if (_ref (buf) > 1) splice ();
  if (pos < 0) pos += _len (buf);
  if (pos < 0) pos = 0;
  if (pos > _len (buf)) pos = _len (buf);
  if (pos == _len (buf))
    return *this += ch;
  buf[pos] = ch;
  return *this;
}

String& String::insert (int pos, char ch)
{
  if (_ref (buf) > 1) splice ();
  if (pos < 0) pos += _len (buf);
  if (pos < 0) pos = 0;
  if (pos > _len (buf)) pos = _len (buf);
  realloc (_len (buf) + 1);
  if (pos < _len (buf))
    memmove (buf + pos + 1, buf + pos, _len (buf) - pos);
  buf[pos] = ch;
  buf[++_len (buf)] = 0;
  return *this;
}

String& String::insert (int pos, String str)
{
  if (_ref (buf) > 1) splice ();
  if (pos < 0) pos += _len (buf);
  if (pos < 0) pos = 0;
  if (pos > _len (buf)) pos = _len (buf);
  realloc (_len (buf) + _len (str.buf));
  if (pos < _len (buf))
    memmove (buf + pos + _len (str.buf), buf + pos, _len (buf) - pos);
  if (_len (str.buf))
    memcpy (buf + pos, str.buf, _len (str.buf));
  buf[_len (buf) += _len (str.buf)] = 0;
  return *this;
}

bool String::isAlpha () const
{
  for (int i = 0; i < _len (buf); i++)
    if (!isalpha ((unsigned char) buf[i]))
      return false;
  return true;
}

bool String::isAlNum () const
{
  for (int i = 0; i < _len (buf); i++)
    if (!isalnum ((unsigned char) buf[i]))
      return false;
  return true;
}

bool String::isDigits () const
{
  for (int i = 0; i < _len (buf); i++)
    if (!isdigit ((unsigned char) buf[i]))
      return false;
  return true;
}

bool  String :: isHexDigits () const
{
  for (int i = 0; i < _len (buf); i++)
    if (!isxdigit ((unsigned char) buf[i]))
      return false;
  return true;
}

String String::getExtension (String str)
{
  for (int i = _len (str.buf) - 1; i >= 0; i--)
  {
    if (str.buf[i] == '/' || str.buf[i] == '\\')
      return "";
    if (str.buf[i] == '.')
      return str.substr (i);
  }
  return "";
}

void String::setExtension (String& str, String ext)
{
  for (int i = _len (str.buf) - 1; i >= 0; i--)
  {
    if (str.buf[i] == '/' || str.buf[i] == '\\')
      return;
    if (str.buf[i] == '.')
      str.replace (i, toTheEnd, ext);
  }
}

bool String::isRelPath (String str)
{
  return str.find (':') < 0;
}

String String::getPath (String str)
{
  for (int i = _len (str.buf) - 1; i >= 0; i--)
    if (str.buf[i] == '/' || str.buf[i] == '\\')
      return str.substr (0, i + 1);
  return "";
}

String String::getFileName (String str)
{
  for (int i = _len (str.buf) - 1; i >= 0; i--)
    if (str.buf[i] == '/' || str.buf[i] == '\\')
      return str.substr (i + 1);
  return str;
}

String String::getFileTitle (String str)
{
  int dot = toTheEnd;
  for (int i = _len (str.buf) - 1; i >= 0; i--)
  {
    if (str.buf[i] == '/' || str.buf[i] == '\\')
      return str.substring (i + 1, dot);
    if (str.buf[i] == '.')
      dot = i;
  }
  return str.substring (0, dot);
}

String String::buildFullName (String path, String name)
{
  if (_len (path.buf) == 0)
    return name;
  else if (path.buf[_len (path.buf) - 1] == '/' || path.buf[_len (path.buf) - 1] == '\\')
    return path + name;
  else
    return path + '/' + name;
}

bool String::isWhitespace (unsigned char c)
{
  return isspace (c) != 0;
}
bool String::isIdChar (unsigned char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool String::isIdStart (unsigned char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool String::isIdentifier (String str)
{
  if (_len (str.buf) == 0)
    return false;
  if (!isIdStart (str.buf[0]))
    return false;
  for (int i = 1; i < _len (str.buf); i++)
    if (!isIdChar (str.buf[i]))
      return false;
  return true;
}

String String::printf (char const* fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  char* buf = _new (_vscprintf (fmt, ap) + 1);
  _len (buf) = _size (buf) - 1;
  _ref (buf) = 1;
  vsprintf (buf, fmt, ap);
  return frombuf (buf);
}

void String::parseString (String str, String& cmd, String& args)
{
  int pos = 0;
  while (str.buf[pos] != 0 && (str.buf[pos] == ' ' || str.buf[pos] == '\t'))
    pos++;
  int cmdstart, cmdend;
  if (str.buf[pos] == '\'')
  {
    cmdstart = ++pos;
    while (str.buf[pos] != 0 && str.buf[pos] != '\'')
      pos++;
    cmdend = pos++;
  }
  else if (str.buf[pos] == '"')
  {
    cmdstart = ++pos;
    while (str.buf[pos] != 0 && str.buf[pos] != '"')
      pos++;
    cmdend = pos++;
  }
  else
  {
    cmdstart = pos;
    while (str.buf[pos] != 0 && str.buf[pos] != ' ' && str.buf[pos] != '\t')
      pos++;
    cmdend = pos;
  }
  cmd = str.substring (cmdstart, cmdend);
  args = str.substr (pos);
}

wchar_t* String::toWide () const
{
  int count = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, NULL, 0);
  wchar_t* wide = new wchar_t[count + 5];
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, wide, count + 5);
  return wide;
}
void String::toWide (wchar_t* ptr, int length) const
{
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, ptr, length);
}

bool String::toClipboard () const
{
  int count = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, NULL, 0);
  wchar_t* wide = new wchar_t[count + 5];
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, wide, count + 5);
  if (!OpenClipboard (NULL))
  {
    delete[] wide;
    return false;
  }
  EmptyClipboard ();
  int sz = (int) wcslen (wide);
  HGLOBAL handle = GlobalAlloc (GMEM_MOVEABLE, sz * 2 + 2);
  if (!handle)
  {
    CloseClipboard ();
    delete[] wide;
    return false;
  }
  wchar_t* copy = (wchar_t*) GlobalLock (handle);
  memcpy (copy, wide, sz * 2 + 2);
  GlobalUnlock (handle);
  SetClipboardData (CF_UNICODETEXT, (HANDLE) handle);
  CloseClipboard ();
  delete[] wide;
  return true;
}
bool String::fromClipboard ()
{
  if (!IsClipboardFormatAvailable (CF_UNICODETEXT))
    return false;
  if (!OpenClipboard (NULL))
    return false;
  wchar_t* wide = (wchar_t*) GetClipboardData (CF_UNICODETEXT);
  CloseClipboard ();
  if (wide)
  {
    if (_ref (buf) > 1) splice ();
    int count = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, (LPWSTR) wide, -1, NULL, 0, NULL, NULL);
    realloc (count);
    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, (LPWSTR) wide, -1, buf, _size (buf), NULL, NULL);
    int len = 0;
    for (int i = 0; buf[i]; i++)
      if (buf[i] != '\r')
        buf[len++] = buf[i];
    buf[len] = 0;
    _len (buf) = len;
    return true;
  }
  else
    return false;
}
