#ifndef __BASE_REGEXP_H__
#define __BASE_REGEXP_H__

#include "base/types.h"
#include "base/string.h"
#include "base/array.h"

class CharacterClass
{
  uint32 mask[8];
public:
  CharacterClass()
  {
    memset(mask, 0, sizeof mask);
  }
  CharacterClass(char const* src, bool plain = false)
  {
    init(src, plain);
  }
  CharacterClass(CharacterClass const& cc)
  {
    memcpy(mask, cc.mask, sizeof mask);
  }

  CharacterClass& operator = (CharacterClass const& cc)
  {
    memcpy(mask, cc.mask, sizeof mask);
    return *this;
  }

  int init(char const* src, bool plain = false);
  CharacterClass& add(char c)
  {
    mask[uint32(c) >> 5] |= (1 << (c & 31));
    return *this;
  }
  CharacterClass& remove(char c)
  {
    mask[uint32(c) >> 5] &= ~(1 << (c & 31));
    return *this;
  }
  bool match(char c) const
  {
    return (mask[c >> 5] & (1 << (c & 31))) != 0;
  }

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

class RegExp
{
  struct Prog;
  Prog* prog;
public:
  RegExp(char const* expr);
  ~RegExp();

  bool match(char const* text, Array<String>* sub = NULL) const;
  int find(char const* text, int start = 0, Array<String>* sub = NULL) const;
  String replace(char const* text, char const* with) const;
};

#endif // __BASE_REGEXP_H__
