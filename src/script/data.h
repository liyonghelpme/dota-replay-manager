#ifndef __SCRIPT_DATA__
#define __SCRIPT_DATA__

#include "base/dictionary.h"
#include "base/array.h"
#include "replay/replay.h"

class ScriptType
{
  Array<ScriptType*> types;
  ScriptType* enumType;
  int type;
  int ref;
public:
  Dictionary<uint32> dir;

  enum {tValue, tStruct, tEnum};
  ScriptType(int t);
  ScriptType(ScriptType* t);
  ~ScriptType();

  int getNumElements() const
  {
    return types.length();
  }
  ScriptType* getElement(int i) const
  {
    return types[i];
  }
  int getElementPos(char const* name) const
  {
    if (dir.has(name))
      return dir.get(name);
    return -1;
  }
  void setEnumType(ScriptType* t);
  ScriptType* getEnumType() const
  {
    return enumType;
  }
  int getType() const
  {
    return type;
  }

  void addElement(char const* name, ScriptType* element);
  ScriptType* addElement(char const* name, int type);

  ScriptType* getSubType(char const* name);

  static void initTypes();
  static ScriptType* tBasic;
  static ScriptType* tPlayer;
  static ScriptType* tGlobal;
};

class ScriptValue
{
  ScriptType* type;
  ScriptValue** elements;
  Array<ScriptValue*> list;
  String value;
  int ref;
public:
  ScriptValue(ScriptType* t);
  ~ScriptValue();

  bool poof()
  {
    if (ref == 0)
    {
      delete this;
      return true;
    }
    return false;
  }

  ScriptType* getType() const
  {
    return type;
  }

  void setValue(String v)
  {
    value = v;
  }
  void setValue(int v)
  {
    value = String(v);
  }
  String getValue() const
  {
    return value;
  }
  void addElement(char const* name, ScriptValue* value);
  ScriptValue* addElement(char const* name);
  ScriptValue* getElement(char const* name)
  {
    int pos = type->getElementPos(name);
    if (pos >= 0)
      return elements[pos];
    return NULL;
  }
  void addEnum(ScriptValue* element);
  ScriptValue* addEnum();
  int getEnumCount() const
  {
    return list.length();
  }
  ScriptValue* getEnum(int i) const
  {
    return list[i];
  }
};

class ScriptGlobal : public ScriptValue
{
  ScriptGlobal(ScriptType* type)
    : ScriptValue(type)
  {}
  Dictionary<ScriptValue*> vars;
public:
  static ScriptGlobal* create(W3GReplay* w3g);

  ScriptValue* getValue(char const* name)
  {
    if (vars.has(name))
      return vars.get(name);
    return getElement(name);
  }

  void setValue(char const* name, ScriptValue* value)
  {
    vars.set(name, value);
  }
  void unsetValue(char const* name)
  {
    vars.del(name);
  }
};

#endif // __SCRIPT_DATA__
