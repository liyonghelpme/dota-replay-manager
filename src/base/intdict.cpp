#include "intdict.h"
#include <stdlib.h>
#include <string.h>

#define NODE_SIZE		1028
#define LEAF_SIZE		1285

IntDictionary::IntDictionary (int numBytes)
{
  maxDepth = numBytes;
  if (maxDepth < 1) maxDepth = 1;
  if (maxDepth > 4) maxDepth = 4;
  count = NODE_SIZE;
  if (maxDepth == 1)
  {
    count = LEAF_SIZE;
    size = count;
  }
  else
    size = 2048;
  buf = new uint8[size];
  memset (buf, 0, count);
}
IntDictionary::~IntDictionary ()
{
  delete[] buf;
}

void IntDictionary::clear ()
{
  delete[] buf;
  count = NODE_SIZE;
  if (maxDepth == 1)
  {
    count = LEAF_SIZE;
    size = count;
  }
  else
    size = 2048;
  buf = new uint8[size];
  memset (buf, 0, count);
}

uint32 IntDictionary::set (uint32 key, uint32 value)
{
  uint32 cur = 0;
  for (int i = 1; i < maxDepth; i++)
  {
    uint32* curc = (uint32*) (buf + cur + 4);
    int p = 0xFF & (key >> ((maxDepth - i) * 8));
    if (curc[p] == 0)
    {
      uint32 curSize = (i < maxDepth - 1 ? NODE_SIZE : LEAF_SIZE);
      if (curSize + count > size)
      {
        uint32 nsize = (size >= 262144 ? size + 262144 : size * 2);
        uint8* temp = new uint8[nsize];
        memcpy (temp, buf, count);
        delete[] buf;
        buf = temp;
        size = nsize;
        curc = (uint32*) (buf + cur + 4);
      }
      curc[p] = count;
      memset (buf + count, 0, curSize);
      *((uint32*) (buf + count)) = cur + p;
      count += curSize;
    }
    cur = curc[p];
  }
  int p = key & 0xFF;
  uint32* curc = (uint32*) (buf + cur + 4);
  uint32 prev = curc[p];
  curc[p] = value;
  buf[cur + NODE_SIZE + p] = 1;
  return prev;
}
uint32 IntDictionary::del (uint32 key)
{
  uint32 cur = 0;
  for (int i = 1; i < maxDepth && (i == 1 || cur); i++)
  {
    int p = 0xFF & (key >> ((maxDepth - i) * 8));
    cur = ((uint32*) (buf + cur + 4))[p];
  }
  if (maxDepth > 1 && cur == 0)
    return 0;
  buf[cur + NODE_SIZE + (key & 0xFF)] = 0;
  uint32 prev = ((uint32*) (buf + cur + 4))[key & 0xFF];
  ((uint32*) (buf + cur + 4))[key & 0xFF] = 0;
  return prev;
}
uint32 IntDictionary::get (uint32 key) const
{
  uint32 cur = 0;
  for (int i = 1; i <= maxDepth && (i == 1 || cur); i++)
  {
    int p = 0xFF & (key >> ((maxDepth - i) * 8));
    cur = ((uint32*) (buf + cur + 4))[p];
  }
  return cur;
}
bool IntDictionary::has (uint32 key) const
{
  uint32 cur = 0;
  for (int i = 1; i < maxDepth && (i == 1 || cur); i++)
  {
    int p = 0xFF & (key >> ((maxDepth - i) * 8));
    cur = ((uint32*) (buf + cur + 4))[p];
  }
  if (maxDepth > 1 && cur == 0)
    return false;
  return ((uint8*) (buf + cur + NODE_SIZE))[key & 0xFF] != 0;
}

uint32 IntDictionary::enumStart () const
{
  return enumNext (0);
}
uint32 IntDictionary::enumNext (uint32 cur) const
{
  uint32 p, depth;
  if (cur == 0)
  {
    p = 0;
    depth = 1;
  }
  else
  {
    p = cur % 257;
    cur -= p;
    depth = maxDepth;
  }
  while (depth > 0)
  {
    if (depth == maxDepth)
    {
      uint32* curc = (uint32*) (buf + cur + 4);
      uint8* curhas = (buf + cur + NODE_SIZE);
      while (p < 256 && curhas[p] == 0)
        p++;
      if (p >= 256)
      {
        cur = * (uint32*) (buf + cur);
        p = (cur % 257);
        cur -= p;
        p++;
        depth--;
      }
      else
        return cur + p + 1;
    }
    else
    {
      uint32* curc = (uint32*) (buf + cur + 4);
      while (p < 256 && curc[p] == 0)
        p++;
      if (p >= 256)
      {
        cur = * (uint32*) (buf + cur);
        p = (cur % 257) + 1;
        cur -= p;
        depth--;
      }
      else
      {
        cur = curc[p];
        p = 0;
        depth++;
      }
    }
  }
  return 0;
}
uint32 IntDictionary::enumGetKey (uint32 cur) const
{
  uint32 val = 0;
  for (int i = 0; i < maxDepth; i++)
  {
    uint32 p = cur % 257;
    val += (p - 1) << (i * 8);
    cur = * (uint32*) (buf + cur - p);
  }
  return val;
}
uint32 IntDictionary::enumGetValue (uint32 cur) const
{
  uint32 p = (cur % 257);
  cur -= p;
  return ((uint32*) (buf + cur + 4))[p - 1];
}
