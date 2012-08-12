#include "stdafx.h"
#include "dictobj.h"

Dictionary::Node::Node ()
{
  memset (c, 0, sizeof c);
  data = 0;
  parent = NULL;
  a = b = 0;
  hasData = false;
  str = "";
}
Dictionary::Node::~Node ()
{
  for (int i = 0; i < 256; i++)
    delete c[i];
}

Dictionary::Dictionary ()
{
  root = new Node;
}
Dictionary::~Dictionary ()
{
  delete root;
}

uint32 Dictionary::set (String key, uint32 value)
{
  Node* cur = root;
  int i = 0;
  while (key[i])
  {
    if (cur->c[key[i]] == NULL)
    {
      cur->c[key[i]] = new Node;
      cur->c[key[i]]->parent = cur;
      cur = cur->c[key[i]];
      cur->str = key;
      cur->a = i;
      cur->b = key.length ();
      i = cur->b;
    }
    else
    {
      cur = cur->c[key[i]];
      int ni;
      for (ni = cur->a; ni < cur->b && key[i]; ni++, i++)
        if (key[i] != cur->str[ni])
          break;
      if (ni < cur->b)
      {
        Node* n = new Node;
        n->c[cur->str[ni]] = cur;
        n->parent = cur->parent;
        n->str = cur->str;
        n->a = cur->a;
        n->b = ni;
        n->parent->c[n->str[n->a]] = n;
        cur->a = ni;
        cur->parent = n;
        cur = n;
      }
    }
  }
  uint32 result = cur->data;
  cur->hasData = true;
  cur->data = value;
  return result;
}
uint32 Dictionary::get (String key) const
{
  Node* cur = root;
  int i = 0;
  while (cur && key[i])
  {
    int ni;
    for (ni = cur->a; ni < cur->b && key[i]; ni++, i++)
      if (key[i] != cur->str[ni])
        return 0;
    if (ni < cur->b)
      return 0;
    if (key[i])
      cur = cur->c[key[i]];
  }
  if (cur)
    return cur->data;
  return 0;
}
bool Dictionary::has (String key) const
{
  Node* cur = root;
  int i = 0;
  while (cur && key[i])
  {
    for (int ni = cur->a; ni < cur->b && key[i]; ni++, i++)
      if (key[i] != cur->str[ni])
        return 0;
    if (key[i])
      cur = cur->c[key[i]];
  }
  return (cur && cur->hasData);
}
uint32 Dictionary::del (String key)
{
  Node* cur = root;
  int i = 0;
  while (cur && key[i])
  {
    for (int ni = cur->a; ni < cur->b && key[i]; ni++, i++)
      if (key[i] != cur->str[ni])
        return 0;
    if (key[i])
      cur = cur->c[key[i]];
  }
  if (cur)
  {
    cur->hasData = false;
    return cur->data;
  }
  return 0;
}

uint32 Dictionary::enumStart () const
{
  if (root->hasData)
    return uint32 (root);
  else
    return enumNext (uint32 (root));
}
uint32 Dictionary::enumNext (uint32 cur) const
{
  Node* node = (Node*) cur;
  int ch = 0;
  bool newNode = false;
  while (node && (!newNode || !node->hasData))
  {
    if (ch > 255)
    {
      ch = int (unsigned char (node->str[node->a])) + 1;
      node = node->parent;
      newNode = false;
    }
    else if (node->c[ch])
    {
      node = node->c[ch];
      ch = 0;
      newNode = true;
    }
    else
      ch++;
  }
  return (uint32) node;
}
String Dictionary::enumGetKey (uint32 cur) const
{
  Node* node = (Node*) cur;
  String key = "";
  while (node)
  {
    key = node->str.substring (node->a, node->b) + key;
    node = node->parent;
  }
  return key;
}
uint32 Dictionary::enumGetValue (uint32 cur) const
{
  Node* node = (Node*) cur;
  if (node)
    return node->data;
  return 0;
}
uint32 Dictionary::enumSetValue (uint32 cur, uint32 value)
{
  Node* node = (Node*) cur;
  uint32 result = 0;
  if (node)
  {
    result = node->data;
    node->hasData = true;
    node->data = value;
  }
  return result;
}
void Dictionary::clear ()
{
  delete root;
  root = new Node;
}

//Node* Dictionary::clone (Node* node)
//{
//  if (node == NULL)
//    return NULL;
//  Node* res = new Node;
//  res->hasData = node->hasData;
//  res->str = node->str;
//  res->a = node->a;
//  res->b = node->b;
//  res->parent = NULL;
//  for (int i = 0; i < 256; i++)
//  {
//    res->c[i] = clone (node->c[i]);
//    if (res->c[i])
//      res->c[i]->parent = res;
//  }
//  return res;
//}
//void Dictionary::copy (Dictionary const& dic)
//{
//  delete root;
//  root = clone (dic.root);
//}

bool Dictionary::isEmpty (Node* node) const
{
  if (node == NULL)
    return true;
  if (node->hasData)
    return false;
  for (int i = 0; i < 256; i++)
    if (!isEmpty (node->c[i]))
      return false;
  return true;
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
//void SimpleDictionary::copy (SimpleDictionary const& dic)
//{
//  if (dic.numNodes >= maxNodes)
//  {
//    delete[] nodes;
//    nodes = new Node[dic.numNodes];
//    maxNodes = dic.numNodes;
//  }
//  for (int
//}
