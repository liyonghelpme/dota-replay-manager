#ifndef __J_PARSE_H__
#define __J_PARSE_H__

#define JASS_ARRAY_SIZE     8192

#define JTYPE_INTEGER       1
#define JTYPE_REAL          2
#define JTYPE_STRING        3
#define JTYPE_CODE          4
#define JTYPE_HANDLE        5
struct JType
{
  JString name;
  int base;
};

struct JFunction
{
  JString;
  int pos;
};

typedef int JHANDLE;

struct JValue
{
  union
  {
    JString* svalue;
    int ivalue;
    float rvalue;
    JHANDLE hvalue;
    JFunction* cvalue;
  };
};
struct JVariable
{
  JString name;
  JType* type;
  bool array;
  union
  {
    JValue value;
    JValue* values;
  };
  JVariable (char const* n, JType* t, bool a)
  {
    strcpy (name, n);
    type = t;
    array = a;
    if (a)
    {
      values = new JValue[JASS_ARRAY_SIZE];
      memset (values, 0, sizeof (JValue) * JASS_ARRAY_SIZE);
    }
    else
      memset (&value, 0, sizeof (JValue));
  }
  ~JVariable ()
  {
    if (array)
    {
      if (type->base == JTYPE_STRING)
      {
        for (int i = 0; i < JASS_ARRAY_SIZE; i++)
          delete values[i].svalue;
      }
      else if (type->base == JTYPE_HANDLE)
      {
        for (int i = 0; i < JASS_ARRAY_SIZE; i++)
          values[i].hvalue->release ();
      }
    }
    if (type->base == JTYPE_STRING)
      delete[] v
  }
};

struct JScript
{
  TrieNode256* vTree;
  TrieNode256* fTree;
  JVariable* vars;
  JFunction* funcs;
  JType* types;
  int numVars;
  int numFuncs;
  int numTypes;
  int maxVars;
  int maxFuncs;
  int maxTypes;
};
void JExecute (JFunction* func);

#endif
