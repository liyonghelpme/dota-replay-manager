#ifndef __BASE_DICTIONARY_H__
#define __BASE_DICTIONARY_H__

#include "base/types.h"
#include "base/string.h"

class DictionaryBase
{
protected:
  struct Node
  {
    bool hasData;
    String str;
    int a, b;
    Node* parent;
    Node* c[256];
    uint8 data[1];
  };
  virtual Node* newnode() const = NULL;
  virtual void delnode(Node* n) const = NULL;
  mutable String lastFindKey;
  mutable Node* lastFindNode;
  void baseinit();
  Node* baseadd(String key);
  Node* basefind(String key) const;
  
  Node* root;
public:
  DictionaryBase ();

  uint32 enumStart () const;
  uint32 enumNext (uint32 cur) const;
  String enumGetKey (uint32 cur) const;

  void clear();
};
template<class T = uint32>
class Dictionary : public DictionaryBase
{
  Node* newnode() const
  {
    Node* n = (Node*) malloc(sizeof(Node) + sizeof(T) - 1);
    memset(n, 0, sizeof(Node) + sizeof(T) - 1);
    new(&n->str) String();
    return n;
  }
  void delnode(Node* n) const
  {
    for (int i = 0; i < sizeof n->c / sizeof n->c[0]; i++)
      if (n->c[i])
        delnode(n->c[i]);
    if (n->hasData)
      ((T*) n->data)->~T();
    n->str.~String();
    free(n);
  }

public:
  Dictionary()
  {
    root = newnode();
  }
  ~Dictionary()
  {
    delnode(root);
  }
  void set (String key, T const& value)
  {
    Node* n = baseadd(key);
    if (!n->hasData)
    {
      n->hasData = true;
      new(n->data) T(value);
    }
    else
      *((T*) n->data) = value;
  }
  T& create(String key)
  {
    Node* n = baseadd(key);
    if (!n->hasData)
    {
      n->hasData = true;
      new(n->data) T();
    }
    return *((T*) n->data);
  }
  T const& get (String key) const
  {
    Node* n = basefind(key);
    return *((T*) n->data);
  }
  T& get (String key)
  {
    Node* n = basefind(key);
    return *((T*) n->data);
  }
  bool has (String key) const
  {
    Node* n = basefind(key);
    return (n && n->hasData);
  }
  void del (String key)
  {
    Node* n = basefind(key);
    if (n && n->hasData)
    {
      ((T*) n->data)->~T();
      n->hasData = false;
    }
  }

  T const& enumGetValue (uint32 cur) const
  {
    Node* n = (Node*) cur;
    return *((T*) n->data);
  }
  T& enumGetValue (uint32 cur)
  {
    Node* n = (Node*) cur;
    return *((T*) n->data);
  }
  void enumSetValue (uint32 cur, T const& value)
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
