#ifndef __BASE_DICTIONARY_H__
#define __BASE_DICTIONARY_H__

#include "base/types.h"
#include "base/string.h"

extern uint8 const* mapAlNum;
extern uint8 const* mapAlNumNoCase;
extern uint8 const* mapAlNumSpace;

class DictionaryBase
{
protected:
  struct Node
  {
    Node* parent;
    int len;
    uint8* data;
    uint8* str;
    Node* c[1];
  };
  uint8 const* map;
  int mapMax;
  virtual Node* newnode(int len) const = NULL;
  virtual void delnode(Node* n) const = NULL;
  Node* baseadd(uint8 const* key);
  Node* basefind(uint8 const* key) const;
  
  Node* root;
public:
  DictionaryBase (uint8 const* map);

  uint32 enumStart () const;
  uint32 enumNext (uint32 cur) const;
  String enumGetKey (uint32 cur) const;

  void clear();
};

template<class T>
class Dictionary : public DictionaryBase
{
  Node* newnode(int len) const
  {
    uint32 size = sizeof(Node) + sizeof(Node*) * mapMax + sizeof(T) + len;
    Node* n = (Node*) malloc(size);
    memset(n, 0, size);
    n->data = ((uint8*) n) + sizeof(Node) + sizeof(Node*) * mapMax;
    n->str = n->data + sizeof(T);
    n->len = len;
    return n;
  }
  void delnode(Node* n) const
  {
    for (int i = 1; i <= mapMax; i++)
      if (n->c[i])
        delnode(n->c[i]);
    if (n->c[0])
      ((T*) n->data)->~T();
    free(n);
  }
  T* nil;

public:
  Dictionary(uint8 const* map = NULL)
    : DictionaryBase(map)
  {
    root = newnode(0);
    nil = (T*) malloc(sizeof(T));
    memset(nil, 0, sizeof(T));
    new(nil) T();
  }
  ~Dictionary()
  {
    delnode(root);
    nil->~T();
    free(nil);
  }
  void set(char const* key, T const& value)
  {
    Node* n = baseadd((uint8 const*) key);
    if (!n->c[0])
    {
      n->c[0] = n;
      new(n->data) T(value);
    }
    else
      *((T*) n->data) = value;
  }
  T& create(char const* key)
  {
    Node* n = baseadd((uint8 const*) key);
    if (!n->c[0])
    {
      n->c[0] = n;
      new(n->data) T();
    }
    return *((T*) n->data);
  }
  T const& get(char const* key) const
  {
    Node* n = basefind((uint8 const*) key);
    return n ? *((T*) n->data) : *nil;
  }
  T& get (char const* key)
  {
    Node* n = basefind((uint8 const*) key);
    return n ? *((T*) n->data) : *nil;
  }
  bool has(char const* key) const
  {
    Node* n = basefind((uint8 const*) key);
    return (n && n->c[0]);
  }
  void del(char const* key)
  {
    Node* n = basefind((uint8 const*) key);
    if (n && n->c[0])
    {
      ((T*) n->data)->~T();
      n->c[0] = NULL;
    }
  }

  T const& enumGetValue(uint32 cur) const
  {
    Node* n = (Node*) cur;
    return *((T*) n->data);
  }
  T& enumGetValue(uint32 cur)
  {
    Node* n = (Node*) cur;
    return *((T*) n->data);
  }
  void enumSetValue(uint32 cur, T const& value)
  {
    Node* n = (Node*) cur;
    *((T*) n->data) = value;
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
};

#endif // __BASE_DICTIONARY_H__
