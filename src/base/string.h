#ifndef __BASE_STRING_H__
#define __BASE_STRING_H__

#include <string.h>
#include "base/array.h"

#define s_isalnum(x)       isalnum((unsigned char) (x))
#define s_isalpha(x)       isalpha((unsigned char) (x))
#define s_iscntrl(x)       iscntrl((unsigned char) (x))
#define s_isdigit(x)       isdigit((unsigned char) (x))
#define s_isgraph(x)       isgraph((unsigned char) (x))
#define s_isleadbyte(x)    isleadbyte((unsigned char) (x))
#define s_islower(x)       islower((unsigned char) (x))
#define s_isprint(x)       isprint((unsigned char) (x))
#define s_ispunct(x)       ispunct((unsigned char) (x))
#define s_isspace(x)       isspace((unsigned char) (x))
#define s_isupper(x)       isupper((unsigned char) (x))
#define s_isxdigit(x)      isxdigit((unsigned char) (x))
#define s_toupper(x)       toupper((unsigned char) (x))
#define s_tolower(x)       tolower((unsigned char) (x))

class String
{
protected:
  char* buf;

  void deref()
  {
    if (buf && --(*(int*)(buf - 8)) == 0)
      delete (buf - 12);
  }
  void realloc(int newLen);
  void append(char const* str);
  void splice();
  static String frombuf(char* buf)
  {
    String str;
    str.buf = buf;
    return str;
  }
public:
  String();
  String(const char* str);
  String(const char* str, int length);
  String(String const& str);
  explicit String(char value);
  explicit String(int value);
  explicit String(float value);

  String& resize(int length)
  {
    if ((*(int*)(buf - 8)) > 1) splice();
    realloc(length);
    return *this;
  }

  ~String()
  {
    deref();
  }

  bool isNull() const
  {
    return buf == NULL || buf == null.buf;
  }

  int compare(String str) const
  {
    return buf == str.buf ? 0 : strcmp(buf, str.buf);
  }
  int compare(char const* str) const
  {
    return strcmp(buf, str);
  }
  int icompare(String str) const
  {
    return buf == str.buf ? 0 : icompare(str.buf);
  }
  int icompare(char const* str) const;

  operator const char * () const
  {
    return buf;
  }

  const char* c_str() const
  {
    return buf;
  }
  char* getBuffer()
  {
    if ((*(int*)(buf - 8)) > 1) splice();
    return buf;
  }
  int getBufferSize()
  {
    return (*(int*)(buf - 4));
  }
  String& update()
  {
    * (int*)(buf - 12) = strlen(buf);
    return *this;
  }

  String& operator = (String const& str);
  String& operator = (char const* str);

  String& operator += (String str)
  {
    append(str.buf);
    return *this;
  }
  String& operator += (char const* str)
  {
    append(str);
    return *this;
  }

  friend String operator + (char const* str1, String str2)
  {
    return String(str1) += str2;
  }
  friend String operator * (int n, String str2)
  {
    return String(str2) *= n;
  }

  String& operator += (char ch);
  String& operator *= (int n);

  String operator + (String str) const
  {
    String s(*this);
    s.append(str.buf);
    return s;
  }
  String operator + (char const* str) const
  {
    String s(*this);
    s.append(str);
    return s;
  }
  String operator + (char ch) const
  {
    String s(*this);
    return s += ch;
  }
  String operator * (int n) const
  {
    String s(*this);
    return s *= n;
  }

  bool operator == (String str) const
  {
    return buf == str.buf || strcmp(buf, str.buf) == 0;
  }
  bool operator != (String str) const
  {
    return buf != str.buf && strcmp(buf, str.buf) != 0;
  }
  bool operator > (String str) const
  {
    return buf != str.buf && strcmp(buf, str.buf) > 0;
  }
  bool operator < (String str) const
  {
    return buf != str.buf && strcmp(buf, str.buf) < 0;
  }
  bool operator >= (String str) const
  {
    return buf == str.buf || strcmp(buf, str.buf) >= 0;
  }
  bool operator <= (String str) const
  {
    return buf == str.buf || strcmp(buf, str.buf) <= 0;
  }

  bool operator == (char const* str) const
  {
    return strcmp(buf, str) == 0;
  }
  friend bool operator == (char const* str, String str2)
  {
    return strcmp(str, str2.buf) == 0;
  }

  bool operator != (char const* str) const
  {
    return strcmp(buf, str) != 0;
  }
  friend bool operator != (char const* str, String str2)
  {
    return strcmp(str, str2.buf) != 0;
  }

  char operator [] (int index) const
  {
    return buf[index];
  }

  int length() const
  {
    return (*(int*)(buf - 12));
  }

  bool isEmpty() const
  {
    return *buf == 0;
  }

  String& toUpper()
  {
    return *this = upper();
  }
  String upper() const;

  String& toLower()
  {
    return *this = lower();
  }
  String lower() const;

  int toInt() const;
  float toFloat() const;

  bool isAlpha() const;
  bool isAlNum() const;
  bool isDigits() const;
  bool isHexDigits() const;
  bool isInt() const;
  bool isFloat() const;

  enum {toTheEnd = 0x7FFFFFFF};

  String substr(int from, int len = toTheEnd) const;
  String substring(int from, int to = toTheEnd) const;
  String& setLength(int newlen)
  {
    if ((*(int*)(buf - 8)) > 1) splice();
    (*(int*)(buf - 12)) = newlen;
    buf[newlen] = 0;
    return *this;
  }
  String& cut(int from, int len = toTheEnd);
  String& trim();
  String& dequote();

  String& removeLeadingSpaces();
  String& removeTrailingSpaces();

  bool isWordBoundary(int pos) const;
  int find(char const* str, int start = 0) const;
  int ifind(char const* str, int start = 0) const;
  int find(char ch, int start = 0) const;
  int ifind(char ch, int start = 0) const;
  int indexOf(char ch) const;
  int lastIndexOf(char ch) const;

  String& replace(int pos, char ch);
  String& replace(int start, int len, char const* str);
  String& insert(int pos, char ch);
  String& insert(int pos, char const* str);
  String& remove(int pos)
  {
    return cut(pos, 1);
  }
  String& remove(int pos, int len)
  {
    return cut(pos, len);
  }

  static String format(const char*, ...);
  String& printf(const char*, ...);

  static String getExtension(String fileName);
  static void setExtension(String& fileName, String ext);
  static bool isRelPath(String path);
  static String getPath(String fullName);
  static String getFileName(String fullName);
  static String getFileTitle(String fullName);
  static String buildFullName(String path, String name);
  static String fixPath(String path);

  static bool isWhitespace(unsigned char c);
  static bool isIdChar(unsigned char c);
  static bool isIdStart(unsigned char c);
  static bool isIdentifier(String str);

  static String fromWide(wchar_t* ptr);
  wchar_t* toWide() const;
  wchar_t* toWide(int& size) const;
  void toWide(wchar_t* ptr, int length) const;

  String& toAnsi();
  int getUtfLength() const;
  int fromUtfPos(int pos) const;

  static void parseString(String str, String& cmd, String& args);

  static String null;

  bool match(char const* re, Array<String>* sub = NULL) const;
  int rfind(char const* re, int start = 0, Array<String>* sub = NULL) const;
  String& rreplace(char const* re, char const* with);

  int split(Array<String>& res, char sep, bool quotes = false);
  int split(Array<String>& res, char const* seplist, bool quotes = false);

  static int smartCompare(String a, String b);

  bool toClipboard() const;
  bool fromClipboard();
};

#endif // __BASE_STRING_H__
