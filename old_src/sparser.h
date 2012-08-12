#ifndef __SPARSER_H__
#define __SPARSER_H__

#include "strobj.h"
#include "replay.h"

class ScriptReader
{
  String curToken;
  int cur;
  String script;
public:
  ScriptReader (String text, int pos)
  {
    script = text;
    cur = pos;
    next ();
  }
  String token () const
  {
    return curToken;
  }
  String next ();
  int getPos () const
  {
    return cur;
  }
};

class ScriptScope;
class ScriptEnum
{
  int count;
  ScriptScope** scopes;
  String attach;
  ScriptScope* ascope;
  bool owned;
public:
  ScriptEnum (int num, String att, bool own);
  ~ScriptEnum ();
  int getCount () const
  {
    return count;
  }
  ScriptScope* getScope (int i) const
  {
    return scopes[i];
  }
  String getAttach () const
  {
    return attach;
  }
  ScriptScope* getAttachScope () const
  {
    return ascope;
  }
  void setScope (int i, ScriptScope* scope)
  {
    scopes[i] = scope;
  }
};
class ScriptScope
{
  String name;
  String value;
  ScriptScope* ref;
  ScriptScope* next;
  ScriptScope* prev;
  ScriptScope* firstChild;
  ScriptScope* lastChild;
  ScriptScope* parent;
  ScriptEnum* list;
public:
  String getValue () const
  {
    return ref->value;
  }
  ScriptScope* getChild (String child) const;
  ScriptEnum* getEnum () const
  {
    return ref->list;
  }
  void setReference (ScriptScope* r)
  {
    ref = r ? r : this;
  }
  ScriptScope* getReference ()
  {
    return ref == this ? NULL : ref;
  }

  ScriptScope (String name, String value);
  ~ScriptScope ();
  ScriptScope* reference (String name);
  void setValue (String val)
  {
    value = val;
  }
  void setEnum (ScriptEnum* enumerator)
  {
    list = enumerator;
  }
  ScriptScope* addChild (ScriptScope* child);
  ScriptScope* addChild (String name, String value);
  ScriptScope* remChild (String name);
};

class ScriptParser
{
  W3GReplay* w3g;
  ScriptScope* players[256];
  ScriptScope* tsentinel;
  ScriptScope* tscourge;
  ScriptScope* root;
  ScriptScope* makePlayer (ScriptScope* player, W3GPlayer* p);
  ScriptScope* eval (ScriptReader& reader);
  bool ceval (ScriptReader& reader);
  ScriptScope* makeItem (int item);
  ScriptScope* makeHero (int hero);
  String fmtChat ();
public:
  ScriptParser (W3GReplay* w3g);
  ~ScriptParser ();
  String parse (String script);
};

#endif // __SPARSER_H__
