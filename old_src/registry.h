#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#define VALUE_VOID      0
#define VALUE_DWORD     1
#define VALUE_STRING    2
#define VALUE_DOUBLE    3
#define VALUE_FILE      4
#define VALUE_INT64     5
#define VALUE_WSTRING   6
#define VALUE_BINARY    7

class Registry
{
  struct Value
  {
    char name[256];
    unsigned char type;
    int size;
    unsigned char* value;
    void set_data (int sz, void const* data);
  };
  char fpath[512];
  Value* values;
  int count;
  int size;
  Value* backup;
  int bcount;
  int bsize;
  Value* getValue (char const* name, bool add = false);
  void _realloc (int size);
  void _brealloc (int size);
  char curFile[256];
  Value* store (char const* name);
  void bwriteInt (char const* name, DWORD val);
  void bwriteInt64 (char const* name, __int64 val);
  void bwriteDouble (char const* name, double val);
  void bwriteString (char const* name, char const* str);
  void bwriteString (char const* name, wchar_t const* str);
  void bwriteBinary (char const* name, void const* data, int length);
public:
  Registry ();
  ~Registry ();

  void load ();

  void flush ();

  void openFile (char const* name);
  void delFile (char const* name);
  void closeFile ();

  void delKey (char const* name);

  char const* getPath () const
  {
    return fpath;
  }

  void writeInt (char const* name, DWORD val);
  int readInt (char const* name, int def = 0);
  void writeInt64 (char const* name, __int64 val);
  __int64 readInt64 (char const* name, __int64 def = 0);
  void writeDouble (char const* name, double val);
  double readDouble (char const* name, double def = 0);
  void writeString (char const* name, char const* str);
  void readString (char const* name, char* str, char const* def = NULL);
  void writeString (char const* name, wchar_t const* str);
  void readString (char const* name, wchar_t* str, wchar_t const* def = NULL);
  void writeBinary (char const* name, void const* data, int length);
  int readBinary (char const* name, void* data, int length);

  void restore ();
};

extern Registry reg;

#endif // __REGISTRY_H__
