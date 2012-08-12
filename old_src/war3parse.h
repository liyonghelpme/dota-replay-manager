#ifndef __WAR3_PARSE_H__
#define __WAR3_PARSE_H__

#define FIELD_INT     1
#define FIELD_REA     2
#define FIELD_STR     3
int getFieldType (int id);
int findDataField (char const* name);
void initFields ();
bool isIdString (char const* str);

struct FieldValue
{
  int id;
  int type;
  union
  {
    char str[256];
    int i;
    float f;
  };
};

struct WC3Object
{
  int id;
  int numValues;
  int maxValues;
  FieldValue* values;
  void addValue (int id, char const* str);
  void addValue (int id, int i);
  void addValue (int id, float f);
};
int getNumObjects ();
WC3Object* getObject (int i);
void clearObjects ();
void finishObjects ();
WC3Object* addObject ();

#endif
