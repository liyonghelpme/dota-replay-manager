#ifndef __DICT_OBJ_H__
#define __DICT_OBJ_H__

#include "strobj.h"
#include "types.h"

class Dictionary
{
  struct Node
  {
    bool hasData;
    String str;
    int a, b;
    uint32 data;
    Node* parent;
    Node* c[256];
    Node ();
    ~Node ();
  };
  Node* root;
  bool isEmpty (Node* node) const;
public:
  Dictionary ();
  ~Dictionary ();

  uint32 set (String key, uint32 value);
  uint32 get (String key) const;
  bool has (String key) const;
  uint32 del (String key);

  uint32 enumStart () const;
  uint32 enumNext (uint32 cur) const;
  String enumGetKey (uint32 cur) const;
  uint32 enumGetValue (uint32 cur) const;
  uint32 enumSetValue (uint32 cur, uint32 value);

  void clear ();
  bool isEmpty () const
  {
    return isEmpty (root);
  }
};

class SimpleDictionary
{
  struct Node
  {
    String key;
    uint32 value;
  };
  Node* nodes;
  int numNodes;
  int maxNodes;
  int locate (String key) const;
public:
  SimpleDictionary ();
  ~SimpleDictionary ();

  uint32 set (String key, uint32 value);
  uint32 get (String key) const;
  bool has (String key) const;
  uint32 del (String key);

  uint32 enumStart () const;
  uint32 enumNext (uint32 cur) const;
  String enumGetKey (uint32 cur) const;
  uint32 enumGetValue (uint32 cur) const;
  uint32 enumSetValue (uint32 cur, uint32 value);

  void clear ();
  bool isEmpty () const
  {
    return numNodes == 0;
  }
};

#endif // __DICT_OBJ_H__
