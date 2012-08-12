#include <stdafx.h>
#include "registry.h"

#include "resource.h"
#include "replay.h"
#include "settingsdlg.h"
#include "dotareplay.h"
#include "utils.h"

Registry reg;

void Registry::_realloc (int sz)
{
  sz = (sz + 15) & (~15);
  if (sz < size)
    return;
  Value* nv = new Value[sz];
  memset (nv, 0, sizeof (Value) * sz);
  if (size)
    memcpy (nv, values, sizeof (Value) * size);
  delete[] values;
  values = nv;
  size = sz;
}
void Registry::_brealloc (int sz)
{
  sz = (sz + 15) & (~15);
  if (sz < bsize)
    return;
  Value* nv = new Value[sz];
  memset (nv, 0, sizeof (Value) * sz);
  if (bsize)
    memcpy (nv, backup, sizeof (Value) * bsize);
  delete[] backup;
  backup = nv;
  bsize = sz;
}

Registry::Registry ()
{
  size = 0;
  count = 0;
  bsize = 0;
  bcount = 0;
  values = NULL;

  char const* pt = GetCommandLine ();
  char esym = ' ';
  if (*pt == '"')
  {
    pt++;
    esym = '"';
  }
  char ffull[512];
  strcpy (ffull, pt);
  int len = (int) strlen (ffull);
  int qpos = -1;
  for (int i = 0; i < len && qpos < 0; i++)
    if (ffull[i] == esym)
      qpos = i;
  ffull[qpos] = 0;
  char fpth[512];
  _splitpath (ffull, fpath, fpth, NULL, NULL);
  strcat (fpath, fpth);
  unslash (fpath);
}
void Registry::load ()
{
  MPQFILE file = MPQOpenFSys (mprintf ("%sconfig.cfg", fpath), MPQFILE_READ);
  if (file == 0)
  {
    MPQFILE sfile = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "config.dat", MPQFILE_READ);
    if (sfile)
    {
      file = MPQOpenFSys (mprintf ("%sconfig.cfg", fpath), MPQFILE_REWRITE);
      if (file)
      {
        static char buf[1024];
        int count;
        while (count = MPQFileRead (sfile, sizeof buf, buf))
          MPQFileWrite (file, count, buf);
        MPQFileSeek (file, 0, MPQSEEK_SET);
      }
      MPQCloseFile (sfile);
      MPQDeleteFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "config.dat");
    }
  }
  if (file == 0)
    return;
  int num = MPQReadInt (file);
  if (!MPQError ())
  {
    _realloc (num);
    for (int i = 0; i < num && !MPQError (); i++)
    {
      Value* val = &values[i];
      unsigned char nlen;
      MPQFileRead (file, 1, &nlen);
      if (MPQError ()) break;
      MPQFileRead (file, nlen, val->name);
      val->name[nlen] = 0;
      if (MPQError ()) break;
      MPQFileRead (file, 1, &val->type);
      if (MPQError ()) break;
      val->size = MPQReadInt (file);
      if (MPQError ()) break;
      if (val->size)
      {
        val->value = new unsigned char[val->size];
        MPQFileRead (file, val->size, val->value);
        if (MPQError ()) break;
      }
      count++;
    }
  }
  MPQCloseFile (file);
  curFile[0] = 0;
}
Registry::~Registry ()
{
  for (int i = 0; i < count; i++)
    delete[] values[i].value;
  delete[] values;
  for (int i = 0; i < bcount; i++)
    delete[] backup[i].value;
  delete[] backup;
}
Registry::Value* Registry::getValue (char const* name, bool add)
{
  char keyName[256];
  sprintf (keyName, "%s%s", curFile, name);
  for (int i = 0; i < count; i++)
  {
    Value* val = &values[i];
    if (!strcmp (val->name, keyName))
      return val;
  }
  if (add)
  {
    _realloc (count + 1);
    Value* val = &values[count++];
    strcpy (val->name, keyName);
    return val;
  }
  return NULL;
}
Registry::Value* Registry::store (char const* name)
{
  char keyName[256];
  sprintf (keyName, "%s%s", curFile, name);
  for (int i = 0; i < bcount; i++)
    if (!strcmp (backup[i].name, keyName))
      return NULL;
  _brealloc (bcount + 1);
  Value* dst = &backup[bcount++];
  strcpy (dst->name, keyName);
  return dst;
}
void Registry::bwriteInt (char const* name, DWORD x)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_DWORD;
    val->set_data (sizeof x, &x);
  }
}
void Registry::bwriteInt64 (char const* name, __int64 x)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_INT64;
    val->set_data (sizeof x, &x);
  }
}
void Registry::bwriteDouble (char const* name, double x)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_DOUBLE;
    val->set_data (sizeof x, &x);
  }
}
void Registry::bwriteString (char const* name, char const* str)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_STRING;
    val->set_data ((int) strlen (str) + 1, str);
  }
}
void Registry::bwriteString (char const* name, wchar_t const* str)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_WSTRING;
    val->set_data (((int) wcslen (str) + 1) * 2, str);
  }
}
void Registry::bwriteBinary (char const* name, void const* data, int length)
{
  Value* val = store (name);
  if (val)
  {
    val->type = VALUE_BINARY;
    val->set_data (length, data);
  }
}

void Registry::Value::set_data (int sz, void const* data)
{
  delete[] value;
  size = sz;
  value = new unsigned char[sz];
  memcpy (value, data, sz);
}

void Registry::writeInt (char const* name, DWORD x)
{
  Value* val = getValue (name, true);
  val->type = VALUE_DWORD;
  val->set_data (sizeof x, &x);
}
int Registry::readInt (char const* name, int def)
{
  bwriteInt (name, def);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_DWORD)
    return * (int*) val->value;
  else
    return def;
}
void Registry::writeInt64 (char const* name, __int64 x)
{
  Value* val = getValue (name, true);
  val->type = VALUE_INT64;
  val->set_data (sizeof x, &x);
}
__int64 Registry::readInt64 (char const* name, __int64 def)
{
  bwriteInt64 (name, def);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_INT64)
    return * (int*) val->value;
  else
    return def;
}
void Registry::writeDouble (char const* name, double x)
{
  Value* val = getValue (name, true);
  val->type = VALUE_DOUBLE;
  val->set_data (sizeof x, &x);
}
double Registry::readDouble (char const* name, double def)
{
  bwriteDouble (name, def);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_DOUBLE)
    return * (double*) val->value;
  else
    return def;
}
void Registry::writeString (char const* name, char const* str)
{
  Value* val = getValue (name, true);
  val->type = VALUE_STRING;
  val->set_data ((int) strlen (str) + 1, str);
}
void Registry::readString (char const* name, char* str, char const* def)
{
  if (def)
    bwriteString (name, def);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_DWORD)
    val->type = VALUE_STRING;
  if (val && val->type == VALUE_STRING)
    strcpy (str, (char*) val->value);
  else if (val && val->type == VALUE_WSTRING)
  {
    wchar_t* vv = (wchar_t*) val->value;
    for (int i = 0; str[i] = (char) vv[i]; i++)
      ;
  }
  else if (def != NULL && str != def)
    strcpy (str, def);
}
void Registry::writeString (char const* name, wchar_t const* str)
{
  Value* val = getValue (name, true);
  val->type = VALUE_WSTRING;
  val->set_data (((int) wcslen (str) + 1) * 2, str);
}
void Registry::readString (char const* name, wchar_t* str, wchar_t const* def)
{
  bwriteString (name, def);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_DWORD)
    val->type = VALUE_STRING;
  if (val && val->type == VALUE_STRING)
  {
    char* vv = (char*) val->value;
    for (int i = 0; str[i] = vv[i]; i++)
      ;
  }
  else if (val && val->type == VALUE_WSTRING)
    wcscpy (str, (wchar_t*) val->value);
  else if (def != NULL && str != def)
    wcscpy (str, def);
}
void Registry::writeBinary (char const* name, void const* data, int length)
{
  Value* val = getValue (name, true);
  val->type = VALUE_BINARY;
  val->set_data (length, data);
}
int Registry::readBinary (char const* name, void* data, int length)
{
  bwriteBinary (name, data, length);
  Value* val = getValue (name, false);
  if (val && val->type == VALUE_BINARY && val->size <= length)
  {
    if (val->size <= length)
      memcpy (data, val->value, val->size);
    return val->size;
  }
  return 0;
}

void Registry::openFile (char const* name)
{
  strcpy (curFile, mprintf ("%s%s#", curFile, name));
}
void Registry::delFile (char const* name)
{
  char keyPath[256];
  sprintf (keyPath, "%s%s#", curFile, name);
  int len = (int) strlen (keyPath);
  for (int i = 0; i < count; i++)
  {
    Value* val = &values[i];
    if (!strncmp (val->name, keyPath, len))
    {
      delete[] val->value;
      if (i < count - 1)
      {
        memcpy (&values[i], &values[count - 1], sizeof (Value));
        memset (&values[count - 1], 0, sizeof (Value));
      }
      count--;
      i--;
    }
  }
}
void Registry::closeFile ()
{
  int len = (int) strlen (curFile);
  if (len == 0)
    return;
  len--;
  while (len && curFile[len - 1] != '#')
    len--;
  curFile[len] = 0;
}

void Registry::delKey (char const* name)
{
  char keyName[256];
  sprintf (keyName, "%s%s", curFile, name);
  for (int i = 0; i < count; i++)
  {
    Value* val = &values[i];
    if (!strcmp (val->name, keyName))
    {
      delete[] val->value;
      if (i < count - 1)
      {
        memcpy (&values[i], &values[count - 1], sizeof (Value));
        memset (&values[count - 1], 0, sizeof (Value));
      }
      count--;
      return;
    }
  }
}

void Registry::flush ()
{
//  MPQFILE file = MPQOpenFile (((CDotAReplayApp*) ::AfxGetApp ())->res, "config.dat", MPQFILE_REWRITE);
  MPQFILE file = MPQOpenFSys (mprintf ("%sconfig.cfg", fpath), MPQFILE_REWRITE);
  if (file == 0) return;
  MPQWriteInt (file, count);
  for (int i = 0; i < count; i++)
  {
    Value* val = &values[i];
    unsigned char len = (unsigned char) strlen (val->name);
    MPQFileWrite (file, 1, &len);
    MPQFileWrite (file, len, val->name);
    MPQFileWrite (file, 1, &val->type);
    MPQWriteInt (file, val->size);
    if (val->size)
      MPQFileWrite (file, val->size, val->value);
  }
  MPQCloseFile (file);
  //char fname[512];
  //sprintf (fname, "%sconfig.cfg", fpath);
  //FILE* sfile = fopen (fname, "wb");
  //if (sfile)
  //{
  //  static char buf[1024];
  //  int count;
  //  MPQFileSeek (file, 0, MPQSEEK_SET);
  //  while (count = MPQFileRead (file, sizeof buf, buf))
  //    fwrite (buf, 1, count, sfile);
  //  fclose (sfile);
  //}
}

void Registry::restore ()
{
  for (int i = 0; i < bcount; i++)
  {
    Value* val = getValue (backup[i].name);
    if (val)
    {
      val->type = backup[i].type;
      val->set_data (backup[i].size, backup[i].value);
    }
  }
}
