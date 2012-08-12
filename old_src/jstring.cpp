#include "stdafx.h"
#include "String.h"

static char __nilstr[16] = "";
String::JSData String::__nil = {__nilstr, 2, 1};

String::String ()
{
  data = &__nil;
  data->ref++;
  pos = 0;
  len = 0;
}
String::~String ()
{
  if (--data->ref == 0)
    delete data;
}
String::String (int size)
{
  data = new JSData;
  data->buf = new char[size];
  data->size = size;
  data->ref = 1;
  pos = 0;
  len = 0;
}
String::String (JSData* d)
{
  data = d;
  data->ref++;
  pos = 0;
  len = 0;
}

void String::split (int size, bool keep) const
{
  bool set0 = keep && (len == 0);
  keep &= !set0;
  if (data->ref == 1)
  {
    if (size > data->size)
    {
      if (size < 1024)
        size *= 2;
      else
        size = (size + 1023) & (~1023);
      char* tmp = new char[size];
      if (keep)
        memcpy (tmp, data->buf + pos, len);
      delete[] data->buf;
      data->buf = tmp;
      data->size = size;
      pos = 0;
    }
    else if (pos + size > data->size)
    {
      if (keep)
        memmove (data->buf, data->buf + pos, len);
      pos = 0;
    }
    else if (!keep)
      pos = 0;
  }
  else
  {
    JSData* tmp = new JSData;
    if (keep && size < len + 1)
      size = len + 1;
    tmp->size = size;
    tmp->buf = new char[size];
    tmp->ref = 1;
    if (keep)
      memcpy (tmp->buf, data->buf + pos, len);
    pos = 0;
    data->ref--;
    data = tmp;
  }
  if (set0)
    *data->buf = 0;
}

String::String (String const& s)
{
  data = s.data;
  data->ref++;
  pos = s.pos;
  len = s.len;
}
String::String (char const* s)
{
  if (s == NULL || *s == 0)
  {
    data = &__nil;
    data->ref++;
    pos = 0;
    len = 0;
  }
  else
  {
    len = (int) strlen (s);
    pos = 0;
    data = new JSData;
    data->size = len + 16;
    data->buf = new char[data->size];
    memcpy (data->buf, s, len + 1);
    data->ref = 1;
  }
}

String& String::operator = (String const& s)
{
  if (&s != this)
  {
    if (--data->ref == 0)
      delete data;
    data = s.data;
    data->ref++;
    pos = s.pos;
    len = s.len;
  }
  return *this;
}
String& String::operator = (char const* s)
{
  if (s == NULL || *s == 0)
  {
    if (--data->ref == 0)
      delete data;
    data = &__nil;
    data->ref++;
    pos = 0;
    len = 0;
  }
  else
  {
    JSData* sdata = NULL;
    if (s >= data->buf && s < data->buf + data->size)
    {
      sdata = data;
      sdata->ref++;
    }
    len = (int) strlen (s);
    split (len + 1, false);
    memcpy (data->buf, s, len + 1);
    if (sdata && --sdata->ref == 0)
      delete sdata;
  }
  return *this;
}
String& String::operator += (String const& s)
{
  if (s.empty ())
    return *this;
  split (len + s.len + 1, true);
  memcpy (data->buf + pos + len, s.data->buf + s.pos, s.len);
  len += s.len;
  return *this;
}
String& String::operator += (char const* s)
{
  if (s == NULL || *s == 0)
    return *this;
  JSData* sdata = NULL;
  if (s >= data->buf && s < data->buf + data->size)
  {
    sdata = data;
    sdata->ref++;
  }
  int slen = (int) strlen (s);
  split (len + slen + 1, true);
  memcpy (data->buf + pos + len, s, slen + 1);
  len += slen;
  if (sdata && --sdata->ref == 0)
    delete sdata;
  return *this;
}

char const* String::c_str () const
{
  if (data->buf[pos + len] != 0)
  {
    split (0, true);
    data->buf[pos + len] = 0;
  }
  return data->buf + pos;
}

String operator + (char const* a, String const& b)
{
  int alen = (int) strlen (a);
  String res (alen + b.len + 16);
  memcpy (res.data->buf, a, alen);
  memcpy (res.data->buf + alen, b.data->buf + b.pos, b.len);
  res.len = alen + b.len;
  return res;
}
String String::operator + (String const& s) const
{
  String res (len + s.len + 16);
  memcpy (res.data->buf, data->buf + pos, len);
  memcpy (res.data->buf + len, s.data->buf + s.pos, s.len);
  res.len = len + s.len;
  return res;
}
String String::operator + (char const* s) const
{
  int slen = (int) strlen (s);
  String res (len + slen + 16);
  memcpy (res.data->buf, data->buf + pos, len);
  memcpy (res.data->buf + len, s, slen + 1);
  res.len = len + slen;
  return res;
}

bool operator == (char const* a, String const& b)
{
  if (strncmp (a, b.data->buf + b.pos, b.len))
    return false;
  return (a[b.len] == 0);
}
bool String::operator == (String const& s) const
{
  return (len == s.len && !strncmp (data->buf + pos, s.data->buf + s.pos, len));
}
bool String::operator == (char const* s) const
{
  if (strncmp (data->buf + pos, s, len))
    return false;
  return (s[len] == 0);
}
bool operator != (char const* a, String const& b)
{
  if (strncmp (a, b.data->buf + b.pos, b.len))
    return true;
  return (a[b.len] != 0);
}
bool String::operator != (String const& s) const
{
  return (len != s.len || strncmp (data->buf + pos, s.data->buf + s.pos, len));
}
bool String::operator != (char const* s) const
{
  if (strncmp (data->buf + pos, s, len))
    return true;
  return (s[len] != 0);
}

int strcmp (String const& a, String const& b)
{
  int res = strncmp (a.data->buf + a.pos, b.data->buf + b.pos, a.len < b.len ? a.len : b.len);
  if (res) return res;
  return a.len - b.len;
}
int stricmp (String const& a, String const& b)
{
  int res = strnicmp (a.data->buf + a.pos, b.data->buf + b.pos, a.len < b.len ? a.len : b.len);
  if (res) return res;
  return a.len - b.len;
}

String& String::setchar (int i, char ch)
{
  if (i >= 0 && i < len)
  {
    split (0, true);
    data->buf[pos + i] = ch;
  }
  return *this;
}

String& String::tolower ()
{
  split (0, true);
  for (int i = 0; i < len; i++)
    data->buf[pos + i] = ::tolower (data->buf[pos + i]);
  return *this;
}
String& String::toupper ()
{
  split (0, true);
  for (int i = 0; i < len; i++)
    data->buf[pos + i] = ::toupper (data->buf[pos + i]);
  return *this;
}
String String::lower () const
{
  String res (len + 16);
  for (int i = 0; i < len; i++)
    res.data->buf[i] = ::tolower (data->buf[pos + i]);
  res.len = len;
  return res;
}
String String::upper () const
{
  String res (len + 16);
  for (int i = 0; i < len; i++)
    res.data->buf[i] = ::toupper (data->buf[pos + i]);
  res.len = len;
  return res;
}
String String::substring (int from, int to) const
{
  if (from < 0) from = 0;
  if (to > len) to = len;
  if (to <= from)
    return String ();
  String res (data);
  res.pos = pos + from;
  res.len = to - from;
  return res;
}
String& String::cut (int from, int to)
{
  if (from < 0) from = 0;
  if (to > len) to = len;
  if (to > from)
  {
    split (0, true);
    if (to < len)
      memmove (data->buf + pos + from, data->buf + pos + to, len - to);
    len -= (to - from);
  }
  return *this;
}

String& String::trim (bool leading, bool trailing)
{
  if (leading)
  {
    while (len && (data->buf[pos] == ' ' || data->buf[pos] == '\t'))
    {
      pos++;
      len--;
    }
  }
  if (trailing)
  {
    while (len && (data->buf[pos + len - 1] == ' ' || data->buf[pos + len - 1] == '\t'))
      len--;
  }
  return *this;
}
String& String::dequote ()
{
  if (len >= 2)
  {
    if (data->buf[pos] == data->buf[pos + len - 1] &&
      (data->buf[pos] == '"' || data->buf[pos] == '\''))
    {
      pos++;
      len -= 2;
    }
  }
  return *this;
}

String String::printf (char const* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int len = _vscprintf (fmt, args);
  if (len == 0)
    return String ();
  String res (len + 16);
  vsprintf (res.data->buf, fmt, args);
  res.len = len;
  return res;
}
