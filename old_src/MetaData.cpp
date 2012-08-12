#include "stdafx.h"
#include "metadata.h"
#include "slk.h"

MetaData::MetaData (MPQFILE slk)
{
  int maxId = 256;
  numId = 0;
  id = new IdValue[maxId];
  memset (id, 0, maxId * sizeof (IdValue));
  trie = NULL;
  if (slk)
  {
    int cury = 0;
    int x = 0;
    int xID = 0;
    int xShadow = 0;
  }
}

MetaData::~MetaData ()
{
}
