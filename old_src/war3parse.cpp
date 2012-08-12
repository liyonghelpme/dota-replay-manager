#include "stdafx.h"
#include "utils.h"
#include "war3parse.h"

static TrieNode* dataFields = NULL;
struct FieldType
{
  int id;
  int type;
  char name[256];
};
static FieldType fields[] = {
  {'uico', FIELD_STR, "Art"},
  {'iico', FIELD_STR, "Art"},
  {'aico', FIELD_STR, "Art"},
  {'igol', FIELD_INT, "goldcost"},
  {'uhab', FIELD_STR, "heroAbilList"},
  {'arpx', FIELD_STR, "Researchbuttonpos"},
};
static const int numFields = sizeof fields / sizeof fields[0];

int __cdecl compareFields (void const* a, void const* b)
{
  return ((FieldType*) a)->id - ((FieldType*) b)->id;
}

void initFields ()
{
  for (int i = 0; i < numFields; i++)
    dataFields = addString (dataFields, fields[i].name, fields[i].id);
  qsort (fields, numFields, sizeof fields[0], compareFields);
}
int getFieldType (int id)
{
  int a = 0;
  int b = numFields;
  while (a <= b)
  {
    int m = (a + b) / 2;
    if (fields[m].id == id) return fields[m].type;
    if (fields[m].id > id)
      b = m - 1;
    else
      a = m + 1;
  }
  return 0;
}
int findDataField (char const* name)
{
  return getValue (dataFields, name);
}
bool isIdString (char const* str)
{
  return !strcmp (str, "ID") ||
         !strcmp (str, "unitUIID") ||
         !strcmp (str, "unitID") ||
         !strcmp (str, "unitBalanceID") ||
         !strcmp (str, "unitAbilID") ||
         !strcmp (str, "alias");
}
