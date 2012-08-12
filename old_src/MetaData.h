#ifndef __META_DATA_H__
#define __META_DATA_H__

class MetaData
{
  TrieNode* trie;
  struct IdValue
  {
    int id;
    char val[256];
  };
  IdValue* id;
  int numId;
public:
  MetaData (MPQFILE slk);
  ~MetaData ();

  char const* getValue (int id);
  int getId (char const* value);
};

#endif
