#ifndef __String_H__
#define __String_H__

class String
{
  struct JSData
  {
    char* buf;
    int size;
    int ref;
  };
  mutable JSData* data;
  mutable int pos;
  int len;
  static JSData __nil;
  // make sure the data is not shared and can fit at least <size>
  // <keep> specifies whether to copy the data
  void split (int size, bool keep) const;
  String (int size);
  String (JSData* d);
public:
  String (); // empty string
  String (String const& s); // copy constructor
  String (char const* s); // assign a string
  ~String ();

  char const* c_str () const; // convert to char*
  operator char const* () const
  {
    return c_str ();
  }
  bool null () const // true if empty from creation or assignment
  {
    return data == &__nil;
  }
  bool empty () const
  {
    return *data->buf == 0;
  }
  int length () const
  {
    return len;
  }

  String& operator = (String const& s); // assignment operator, copies the reference
  String& copy (String const& s); // copies the data
  String& operator = (char const* s); // assignment operator

  String& operator += (String const& s); // concatenation
  String& operator += (char const* s);

  friend String operator + (char const* a, String const& b);
  String operator + (String const& s) const;
  String operator + (char const* s) const;

  friend bool operator == (char const* a, String const& b);
  bool operator == (String const& s) const;
  bool operator == (char const* s) const;
  friend bool operator != (char const* a, String const& b);
  bool operator != (String const& s) const;
  bool operator != (char const* s) const;

  friend int strcmp (String const& a, String const& b);
  friend int stricmp (String const& a, String const& b);

  char operator [] (int i) const
  {
    return (i == len ? 0 : data->buf[pos + i]);
  }
  String& setchar (int i, char ch);

  String& tolower ();
  String& toupper ();
  String lower () const;
  String upper () const;

  String substring (int from, int to) const;
  String& cut (int from, int to);

  String& trim (bool leading = true, bool trailing = true);
  String& dequote ();

  static String printf (char const* fmt, ...);
};

struct JString
{
  char* buf;
  int len;
  int size;
  int ref;

  static JString* create (char const* str);
  static void destroy (JString*);
  static 
};

#endif
