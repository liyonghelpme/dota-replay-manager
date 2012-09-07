#ifndef __BASE_REGEXP_H__
#define __BASE_REGEXP_H__

#include "base/types.h"
#include "base/string.h"
#include "base/array.h"

class CharacterClass
{
  bool invert;
  int count;
  struct range
  {
    uint32 begin;
    uint32 end;
  };
  static int __cdecl rangecomp(void const* a, void const* b);
  range* data;
public:
  CharacterClass()
  {
    invert = false;
    count = 0;
    data = NULL;
  }
  CharacterClass(char const* src, uint32* table = NULL)
  {
    init(src, table);
  }
  CharacterClass(CharacterClass const& cc);
  ~CharacterClass();

  CharacterClass& operator = (CharacterClass const& cc);

  uint8_ptr init(char const* src, uint32* table = NULL);
  bool match(uint32 c) const;

  static const CharacterClass word;
  static const CharacterClass non_word;
  static const CharacterClass digit;
  static const CharacterClass hex_digit;
  static const CharacterClass non_digit;
  static const CharacterClass space;
  static const CharacterClass non_space;
  static const CharacterClass dot;
  static const CharacterClass any;
};

#define REGEXP_CASE_INSENSITIVE       0x0001
#define REGEXP_DOTALL                 0x0002
#define REGEXP_MULTILINE              0x0004

class RegExp
{
  struct Prog;
  Prog* prog;
public:
  RegExp(char const* expr, uint32 flags = 0);
  ~RegExp();

  bool match(char const* text, Array<String>* sub = NULL) const;
  int find(char const* text, int start = 0, Array<String>* sub = NULL) const;
  String replace(char const* text, char const* with) const;
};

#endif // __BASE_REGEXP_H__
