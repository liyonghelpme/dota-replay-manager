#include "dictionary.h"

static uint8 _mapFull[256];
static uint8 _mapAlNum[256];
uint8 const* mapAlNum = _mapAlNum;
static uint8 _mapAlNumNoCase[256];
uint8 const* mapAlNumNoCase = _mapAlNumNoCase;
static uint8 _mapAlNumSpace[256];
uint8 const* mapAlNumSpace = _mapAlNumSpace;
static bool _mapInit = false;

DictionaryBase::DictionaryBase(uint8 const* _map)
{
  if (!_mapInit)
  {
    for (int i = 0; i < 256; i++)
      _mapFull[i] = i;
    memset(_mapAlNum, 0, sizeof _mapAlNum);
    memset(_mapAlNumNoCase, 0, sizeof _mapAlNumNoCase);
    memset(_mapAlNumSpace, 0, sizeof _mapAlNumSpace);
    for (int i = 0; i < 10; i++)
    {
      uint8 c = uint8('0' + i);
      _mapAlNum[c] = _mapAlNumNoCase[c] = _mapAlNumSpace[c] = i + 1;
    }
    for (int i = 0; i < 26; i++)
    {
      uint8 c = uint8('a' + i);
      uint8 C = uint8('A' + i);
      _mapAlNum[c] = _mapAlNumNoCase[c] = _mapAlNumSpace[c] =
        _mapAlNumNoCase[C] = _mapAlNumSpace[C] = i + 11;
      _mapAlNum[C] = i + 37;
    }
    _mapInit = true;
  }
  map = _map;
  if (map == NULL)
    map = _mapFull;
  mapMax = 0;
  for (int i = 0; i < 256; i++)
    if (map[i] > mapMax)
      mapMax = map[i];
}
void DictionaryBase::clear()
{
  delnode(root);
  root = newnode(0);
}

DictionaryBase::Node* DictionaryBase::baseadd(uint8 const* key)
{
  Node* cur = root;
  while (*key)
  {
    while (*key && map[*key] == 0)
      key++;
    if (*key == 0)
      break;
    uint8 c = map[*key];
    if (cur->c[c] == NULL)
    {
      int rem = 0;
      for (int j = 0; key[j]; j++)
        if (map[key[j]])
          rem++;
      cur->c[c] = newnode(rem);
      cur->c[c]->parent = cur;
      cur = cur->c[c];
      rem = 0;
      for (; *key; key++)
        if (map[*key])
          cur->str[rem++] = *key;
    }
    else
    {
      cur = cur->c[c];
      int ni;
      for (ni = 0; ni < cur->len && *key; ni++, key++)
      {
        while (*key && map[*key] == 0)
          key++;
        if (*key == 0)
          break;
        if (map[*key] != map[cur->str[ni]])
          break;
      }
      if (ni < cur->len)
      {
        Node* n = newnode(ni);
        n->c[map[cur->str[ni]]] = cur;
        n->parent = cur->parent;
        memcpy(n->str, cur->str, ni);
        cur->str += ni;
        cur->len -= ni;
        n->parent->c[map[n->str[0]]] = n;
        cur->parent = n;
        cur = n;
      }
    }
  }
  return cur;
}
DictionaryBase::Node* DictionaryBase::basefind(uint8 const* key) const
{
  Node* cur = root;
  while (cur && *key)
  {
    for (int ni = 0; ni < cur->len; ni++, key++)
    {
      while (*key && map[*key] == 0)
        key++;
      if (map[*key] != map[cur->str[ni]])
        return NULL;
    }
    while (*key && map[*key] == 0)
      key++;
    if (*key)
      cur = cur->c[map[*key]];
  }
  return cur;
}

uint32 DictionaryBase::enumStart() const
{
  if (root->c[0])
    return uint32(root);
  else
    return enumNext(uint32(root));
}
uint32 DictionaryBase::enumNext(uint32 cur) const
{
  Node* node = (Node*) cur;
  int ch = 1;
  bool newNode = false;
  while (node && (!newNode || !node->c[0]))
  {
    if (ch > mapMax)
    {
      ch = map[node->str[0]] + 1;
      node = node->parent;
      newNode = false;
    }
    else if (node->c[ch])
    {
      node = node->c[ch];
      ch = 1;
      newNode = true;
    }
    else
      ch++;
  }
  return (uint32) node;
}
String DictionaryBase::enumGetKey(uint32 cur) const
{
  Node* node = (Node*) cur;
  int len = 0;
  for (Node* node = (Node*) cur; node; node = node->parent)
    len += node->len;
  String key;
  key.resize(len);
  key.setLength(len);
  int pos = len;
  for (Node* node = (Node*) cur; node; node = node->parent)
  {
    pos -= node->len;
    if (node->len)
      memcpy(key.getBuffer() + pos, node->str, node->len);
  }
  return key;
}

///////////////////////////////////////////

SimpleDictionary::SimpleDictionary ()
{
  nodes = NULL;
  numNodes = 0;
  maxNodes = 0;
}
SimpleDictionary::~SimpleDictionary ()
{
  delete[] nodes;
}
void SimpleDictionary::clear ()
{
  delete[] nodes;
  nodes = NULL;
  numNodes = 0;
  maxNodes = 0;
}

int SimpleDictionary::locate (String key) const
{
  int left = 0;
  int right = numNodes;
  while (left < right)
  {
    int mid = (left + right) / 2;
    if (key == nodes[mid].key)
      return mid;
    if (key < nodes[mid].key)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

uint32 SimpleDictionary::set (String key, uint32 value)
{
  int left = 0;
  int right = numNodes;
  while (left < right)
  {
    int mid = (left + right) / 2;
    if (key == nodes[mid].key)
    {
      uint32 result = nodes[mid].value;
      nodes[mid].value = value;
      return result;
    }
    if (key < nodes[mid].key)
      right = mid;
    else
      left = mid + 1;
  }
  if (numNodes >= maxNodes)
  {
    if (maxNodes == 0)
      maxNodes = 4;
    else
      maxNodes *= 2;
    Node* temp = new Node[maxNodes];
    for (int i = 0; i < numNodes; i++)
    {
      temp[i].key = nodes[i].key;
      temp[i].value = nodes[i].value;
    }
    delete[] nodes;
    nodes = temp;
  }
  for (int i = numNodes; i > left; i--)
  {
    nodes[i].key = nodes[i - 1].key;
    nodes[i].value = nodes[i - 1].value;
  }
  nodes[left].key = key;
  nodes[left].value = value;
  numNodes++;
  return 0;
}
uint32 SimpleDictionary::get (String key) const
{
  int pos = locate (key);
  if (pos < 0)
    return 0;
  else
    return nodes[pos].value;
}
bool SimpleDictionary::has (String key) const
{
  return locate (key) >= 0;
}
uint32 SimpleDictionary::del (String key)
{
  int pos = locate (key);
  if (pos < 0) return 0;
  uint32 result = nodes[pos].value;
  for (int i = pos + 1; i < numNodes; i++)
  {
    nodes[i - 1].key = nodes[i].key;
    nodes[i - 1].value = nodes[i].value;
  }
  numNodes--;
  return result;
}

uint32 SimpleDictionary::enumStart () const
{
  return numNodes ? 1 : 0;
}
uint32 SimpleDictionary::enumNext (uint32 cur) const
{
  return cur == numNodes ? 0 : cur + 1;
}
String SimpleDictionary::enumGetKey (uint32 cur) const
{
  return cur ? nodes[cur - 1].key : "";
}
uint32 SimpleDictionary::enumGetValue (uint32 cur) const
{
  return cur ? nodes[cur - 1].value : 0;
}
uint32 SimpleDictionary::enumSetValue (uint32 cur, uint32 value)
{
  uint32 result = 0;
  if (cur)
  {
    result = nodes[cur - 1].value;
    nodes[cur - 1].value = value;
  }
  return result;
}
