#include <string.h>

#include "regexp.h"

int CharacterClass::init(char const* src, bool plain)
{
  if (plain)
  {
    memset(mask, 0, sizeof mask);
    int length = 0;
    while (src[length])
      add(src[length++]);
    return length;
  }
  else
  {
    bool invert = false;
    if (*src == '^')
    {
      memset(mask, 0xFF, sizeof mask);
      invert = true;
      src++;
    }
    else
      memset(mask, 0, sizeof mask);
    int i = 0;
    for (; src[i] && (i == 0 || src[i] != ']'); i++)
    {
      if (src[i + 1] == '-' && src[i + 2] && src[i + 2] != ']')
      {
        for (char c = src[i]; c <= src[i + 2]; c++)
        {
          if (invert)
            add(c);
          else
            remove(c);
        }
        i += 2;
      }
      else
      {
        if (invert)
          remove(src[i]);
        else
          add(src[i]);
      }
    }
    return i + (invert ? 1 : 0);
  }
}

const CharacterClass CharacterClass::word("A-Za-z0-9_");
const CharacterClass CharacterClass::non_word("^A-Za-z0-9_");
const CharacterClass CharacterClass::digit("0-9");
const CharacterClass CharacterClass::hex_digit("A-Fa-f0-9");
const CharacterClass CharacterClass::non_digit("^0-9");
const CharacterClass CharacterClass::space(" \t\r\n\v\f");
const CharacterClass CharacterClass::non_space("^ \t\r\n\v\f");
const CharacterClass CharacterClass::dot("^\n");
const CharacterClass CharacterClass::any("^");

/////////////////////////////////////

namespace _re
{

static CharacterClass const* maskmap[256];
static bool maskinit = false;
void initMasks()
{
  maskmap['w'] = &CharacterClass::word;
  maskmap['W'] = &CharacterClass::non_word;
  maskmap['d'] = &CharacterClass::digit;
  maskmap['x'] = &CharacterClass::hex_digit;
  maskmap['D'] = &CharacterClass::non_digit;
  maskmap['s'] = &CharacterClass::space;
  maskmap['S'] = &CharacterClass::non_space;
  maskinit = true;
}

struct State
{
  enum {
    START = 1,
    RBRA,
    LBRA,
    OR,
    CAT,
    STAR,
    PLUS,
    QUEST,
    NOP,
    OPERAND,
    BOL,
    EOL,
    CHAR,
    CCLASS,
    END
  };
  int type;
  union
  {
    CharacterClass const* mask;
    char chr;
    int subid;
    State* left;
  };
  union
  {
    State* right;
    State* next;
  };
  int list;
};
struct Operand
{
  State* first;
  State* last;
};
struct Operator
{
  int type;
  int subid;
};
#define MAXSTACK    32
struct Compiler
{
  State* states;
  int maxStates;
  int numStates;
  CharacterClass* masks;
  int maxMasks;
  int numMasks;

  Operand andstack[MAXSTACK];
  int andsize;
  Operator atorstack[MAXSTACK];
  int atorsize;

  bool lastand;
  int brackets;
  int cursub;

  int optsize;

  void init(char const* expr);
  State* operand(int type);
  void pushand(State* first, State* last);
  void pushator(int type);
  void evaluntil(int pri);
  int optimize(State* state);
  void floatstart();
};
void Compiler::init(char const* expr)
{
  if (!maskinit)
    initMasks();

  int length = strlen(expr);
  maxStates = length * 6 + 6;
  states = new State[maxStates];
  memset(states, 0, sizeof(State) * maxStates);
  numStates = 0;

  maxMasks = 0;
  for (int i = 0; expr[i]; i++)
    if (expr[i] == '[')
      maxMasks++;
  masks = new CharacterClass[maxMasks];
  numMasks = 0;

  andsize = 0;
  atorstack[0].type = 0;
  atorstack[0].subid = 0;
  atorsize = 1;
  lastand = false;
  brackets = 0;
  cursub = 0;

  optsize = 0;
}
State* Compiler::operand(int type)
{
  if (lastand)
    pushator(State::CAT);
  State* s = &states[numStates++];
  s->type = type;
  s->mask = NULL;
  s->next = NULL;
  pushand(s, s);
  lastand = true;
  return s;
}
void Compiler::pushand(State* first, State* last)
{
  andstack[andsize].first = first;
  andstack[andsize].last = last;
  andsize++;
}
void Compiler::pushator(int type)
{
  if (type == State::RBRA)
    brackets--;
  if (type == State::LBRA)
  {
    cursub++;
    brackets++;
    if (lastand)
      pushator(State::CAT);
  }
  else
    evaluntil(type);
  if (type != State::RBRA)
  {
    atorstack[atorsize].type = type;
    atorstack[atorsize].subid = cursub;
    atorsize++;
  }
  lastand = (type == State::STAR || type == State::PLUS ||
    type == State::QUEST || type == State::RBRA);
}
void Compiler::evaluntil(int pri)
{
  State* s1;
  State* s2;
  while (pri == State::RBRA || atorstack[atorsize - 1].type >= pri)
  {
    atorsize--;
    switch (atorstack[atorsize].type)
    {
    case State::LBRA:
      s1 = &states[numStates++];
      s2 = &states[numStates++];
      s1->type = State::LBRA;
      s1->subid = atorstack[atorsize].subid;
      s2->type = State::RBRA;
      s2->subid = atorstack[atorsize].subid;

      s1->next = andstack[andsize - 1].first;
      andstack[andsize - 1].first = s1;
      andstack[andsize - 1].last->next = s2;
      andstack[andsize - 1].last = s2;
      s2->next = NULL;
      return;
    case State::OR:
      andsize--;
      s1 = &states[numStates++];
      s2 = &states[numStates++];
      s1->type = State::OR;
      s1->left = andstack[andsize - 1].first;
      s1->right = andstack[andsize].first;
      s2->type = State::NOP;
      s2->mask = NULL;
      s2->next = NULL;
      andstack[andsize - 1].last->next = s2;
      andstack[andsize].last->next = s2;
      andstack[andsize - 1].first = s1;
      andstack[andsize - 1].last = s2;
      break;
    case State::CAT:
      andsize--;
      andstack[andsize - 1].last->next = andstack[andsize].first;
      andstack[andsize - 1].last = andstack[andsize].last;
      break;
    case State::STAR:
      s1 = &states[numStates++];
      s1->type = State::OR;
      s1->left = andstack[andsize - 1].first;
      s1->right = NULL;
      andstack[andsize - 1].last->next = s1;
      andstack[andsize - 1].first = s1;
      andstack[andsize - 1].last = s1;
      break;
    case State::PLUS:
      s1 = &states[numStates++];
      s1->type = State::OR;
      s1->left = andstack[andsize - 1].first;
      s1->right = NULL;
      andstack[andsize - 1].last->next = s1;
      andstack[andsize - 1].last = s1;
      break;
    case State::QUEST:
      s1 = &states[numStates++];
      s2 = &states[numStates++];
      s1->type = State::OR;
      s1->left = andstack[andsize - 1].first;
      s1->right = s2;
      s2->type = State::NOP;
      s2->mask = NULL;
      s2->next = NULL;
      andstack[andsize - 1].last->next = s2;
      andstack[andsize - 1].first = s1;
      andstack[andsize - 1].last = s2;
      break;
    }
  }
}
int Compiler::optimize(State* state)
{
  if (state->list >= 0)
    return state->list;
  if (state->type == State::NOP)
    state->list = optimize(state->next);
  else
  {
    state->list = optsize++;
    if (state->next)
      optimize(state->next);
    if (state->type == State::OR)
      optimize(state->left);
  }
  return state->list;
}

struct Match
{
  char const* start[32];
  char const* end[32];
};
struct Thread
{
  State* state;
  Match match;
};

}

struct RegExp::Prog
{
  _re::State* start;
  _re::State* states;
  int numStates;
  CharacterClass* masks;

  Prog(char const* expr);
  ~Prog();

  int maxThreads;
  _re::Thread* threads;
  int cur;
  int pos;
  int numThreads[2];
  char const* matchText;
  void addthread(_re::State* state, _re::Match const& match);
  void advance(_re::State* state, _re::Match const& match, char const* c, char const* ref);

  int run(char const* text, bool exact, bool (*callback) (_re::Match const& match, void* arg), void* arg);
};
RegExp::Prog::Prog(char const* expr)
{
  _re::Compiler comp;
  comp.init(expr);

  int pos = 0;
  while (true)
  {
    int type = _re::State::CHAR;
    CharacterClass const* mask = NULL;
    if (expr[pos] == '\\')
    {
      pos++;
      if (_re::maskmap[expr[pos]])
      {
        type = _re::State::CCLASS;
        mask = _re::maskmap[expr[pos]];
      }
    }
    else if (expr[pos] == 0)
      type = _re::State::END;
    else if (expr[pos] == '*')
      type = _re::State::STAR;
    else if (expr[pos] == '+')
      type = _re::State::PLUS;
    else if (expr[pos] == '?')
      type = _re::State::QUEST;
    else if (expr[pos] == '|')
      type = _re::State::OR;
    else if (expr[pos] == '(')
      type = _re::State::LBRA;
    else if (expr[pos] == ')')
      type = _re::State::RBRA;
    else if (expr[pos] == '^')
      type = _re::State::BOL;
    else if (expr[pos] == '$')
      type = _re::State::EOL;
    else if (expr[pos] == '.')
    {
      type = _re::State::CCLASS;
      mask = &CharacterClass::dot;
    }
    else if (expr[pos] == '[')
    {
      type = _re::State::CCLASS;
      CharacterClass* tmask = &comp.masks[comp.numMasks++];
      pos += tmask->init(expr + pos + 1) + 1;
      mask = tmask;
    }
    if (expr[pos] == 0)
      type = _re::State::END;
    if (type < _re::State::OPERAND)
      comp.pushator(type);
    else
    {
      _re::State* s = comp.operand(type);
      if (type == _re::State::CHAR)
        s->chr = expr[pos];
      else if (type == _re::State::CCLASS)
        s->mask = mask;
    }
    if (type == _re::State::END)
      break;
    else
      pos++;
  }
  comp.evaluntil(_re::State::START);

  for (int i = 0; i < comp.numStates; i++)
    comp.states[i].list = -1;
  int startpos = comp.optimize(comp.andstack[0].first);
  states = new _re::State[comp.optsize];
  masks = comp.masks;
  for (int i = 0; i < comp.numStates; i++)
  {
    int p = comp.states[i].list;
    if (p >= 0 && comp.states[i].type != _re::State::NOP)
    {
      states[p].type = comp.states[i].type;
      states[p].next = comp.states[i].next ? &states[comp.states[i].next->list] : NULL;
      if (states[p].type == _re::State::CHAR)
        states[p].chr = comp.states[i].chr;
      else if (states[p].type == _re::State::CCLASS)
        states[p].mask = comp.states[i].mask;
      else if (states[p].type == _re::State::OR)
        states[p].left = comp.states[i].left ? &states[comp.states[i].left->list] : NULL;
      else
        states[p].subid = comp.states[i].subid;
    }
  }
  delete[] comp.states;
  numStates = comp.optsize;
  start = &states[startpos];

  maxThreads = 32;
  threads = new _re::Thread[maxThreads * 2];
}
RegExp::Prog::~Prog()
{
  delete[] states;
  delete[] masks;
  delete[] threads;
}
void RegExp::Prog::addthread(_re::State* state, _re::Match const& match)
{
  if (state->list < 0)
  {
    if (numThreads[1 - cur] >= maxThreads)
    {
      _re::Thread* newthreads = new _re::Thread[maxThreads * 4];
      memcpy(newthreads, threads, sizeof(_re::Thread) * numThreads[0]);
      memcpy(newthreads + maxThreads * 2, threads + maxThreads, sizeof(_re::Thread) * numThreads[1]);
      delete[] threads;
      threads = newthreads;
      maxThreads *= 2;
    }
    _re::Thread* thread = &threads[(1 - cur) * maxThreads + numThreads[1 - cur]];
    state->list = numThreads[1 - cur];
    numThreads[1 - cur]++;
    thread->state = state;
    memcpy(&thread->match, &match, sizeof match);
  }
}
void RegExp::Prog::advance(_re::State* state, _re::Match const& match, char const* c, char const* ref)
{
  if (state->type == _re::State::OR)
  {
    advance(state->left, match, c, ref);
    advance(state->right, match, c, ref);
  }
  else if (state->type == _re::State::LBRA)
  {
    _re::Match m2;
    memcpy(&m2, &match, sizeof m2);
    m2.start[state->subid] = ref;
    advance(state->next, m2, c, ref);
  }
  else if (state->type == _re::State::RBRA)
  {
    _re::Match m2;
    memcpy(&m2, &match, sizeof m2);
    m2.end[state->subid] = ref;
    advance(state->next, m2, c, ref);
  }
  else if (state->type == _re::State::BOL)
  {
    if (ref == matchText || ref[-1] == '\n')
      advance(state->next, match, NULL, ref);
  }
  else if (state->type == _re::State::EOL)
  {
    if (*ref == 0 || *ref == '\r' || *ref == '\n')
      advance(state->next, match, NULL, ref);
  }
  else
  {
    if (c == NULL)
      addthread(state, match);
    else if ((state->type == _re::State::CHAR && *c == state->chr) ||
             (state->type == _re::State::CCLASS && state->mask->match(*c)))
      advance(state->next, match, NULL, ref + 1);
  }
}
int RegExp::Prog::run(char const* text, bool exact, bool (*callback) (_re::Match const& match, void* arg), void* arg)
{
  cur = 0;
  numThreads[0] = 0;
  numThreads[1] = 0;
  pos = 0;
  matchText = text;
  int count = 0;
  while (true)
  {
    for (int i = 0; i < numStates; i++)
    {
      if (pos > 0 && states[i].type == _re::State::END && states[i].list >= 0)
      {
        _re::Thread* thread = &threads[cur * maxThreads + states[i].list];
        thread->match.end[0] = text + pos;
        count++;
        if (callback)
        {
          if (!callback(thread->match, arg))
            return count;
        }
      }
      states[i].list = -1;
    }
    numThreads[1 - cur] = 0;
    for (int i = 0; i < numThreads[cur]; i++)
      advance(threads[cur * maxThreads + i].state, threads[cur * maxThreads + i].match, text + pos, text + pos);
    if (pos == 0 || !exact)
    {
      _re::Match match;
      memset(&match, 0, sizeof match);
      match.start[0] = text + pos;
      advance(start, match, text + pos, text + pos);
    }
    cur = 1 - cur;
    if (text[pos] == 0)
      break;
    pos++;
  }
  for (int i = 0; i < numStates; i++)
  {
    if (states[i].type == _re::State::END && states[i].list >= 0)
    {
      _re::Thread* thread = &threads[cur * maxThreads + states[i].list];
      thread->match.end[0] = text + pos;
      count++;
      if (callback)
      {
        if (!callback(thread->match, arg))
          return count;
      }
    }
  }
  return count;
}

RegExp::RegExp(char const* expr)
{
  prog = new Prog(expr);
}
RegExp::~RegExp()
{
  delete prog;
}

static bool matcher(_re::Match const& match, void* arg)
{
  if (arg)
  {
    Array<String>* sub = (Array<String>*) arg;
    sub->clear();
    for (int i = 0; i < sizeof(match.start) / sizeof(match.start[0]) && match.start[i]; i++)
      sub->push(String(match.start[i], match.end[i] - match.start[i]));
  }
  return true;
}
bool RegExp::match(char const* text, Array<String>* sub) const
{
  return prog->run(text, true, matcher, sub) > 0;
}
static bool finder(_re::Match const& match, void* arg)
{
  memcpy(arg, &match, sizeof match);
  return false;
}
int RegExp::find(char const* text, int start, Array<String>* sub) const
{
  _re::Match match;
  if (prog->run(text + start, false, finder, &match))
  {
    sub->clear();
    for (int i = 0; i < sizeof(match.start) / sizeof(match.start[0]) && match.start[i]; i++)
      sub->push(String(match.start[i], match.end[i] - match.start[i]));
    return match.start[0] - text;
  }
  else
    return -1;
}
namespace _re
{
struct ReplaceStruct
{
  String result;
  char const* text;
  char const* with;
  int last_start;
  int last_end;
  Match last_match;
};
}
static void addreplace(String& result, char const* with, _re::Match const& match)
{
  for (int i = 0; with[i]; i++)
  {
    if (with[i] == '\\')
    {
      i++;
      if (with[i] >= '0' && with[i] <= '9')
      {
        int m = int(with[i] - '0');
        if (match.start[m] && match.end[m])
          for (char const* s = match.start[m]; s < match.end[m]; s++)
            result += *s;
      }
      else
        result += with[i];
    }
    else
      result += with[i];
  }
}
static bool replacer(_re::Match const& match, void* arg)
{
  _re::ReplaceStruct& rs = * (_re::ReplaceStruct*) arg;
  int mstart = match.start[0] - rs.text;
  int mend = match.end[0] - rs.text;
  if (mstart == rs.last_start && mend > rs.last_end)
  {
    rs.last_end = mend;
    memcpy(&rs.last_match, &match, sizeof match);
  }
  else if (mstart >= rs.last_end)
  {
    for (int i = rs.last_end; i < mstart; i++)
      rs.result += rs.text[i];
    addreplace(rs.result, rs.with, rs.last_match);
    rs.last_start = mstart;
    rs.last_end = mend;
    memcpy(&rs.last_match, &match, sizeof match);
  }
  return true;
}
String RegExp::replace(char const* text, char const* with) const
{
  _re::ReplaceStruct rs;
  rs.text = text;
  rs.with = with;
  rs.last_start = 0;
  rs.last_end = 0;
  prog->run(text, false, replacer, &rs);
  if (rs.last_end)
    addreplace(rs.result, with, rs.last_match);
  rs.result += (text + rs.last_end);
  return rs.result;
}

//String RegExp::dump()
//{
//  String result;
//  for (int i = 0; i < prog->numStates; i++)
//  {
//    _re::State* s = &prog->states[i];
//    if (s == prog->start)
//      result += '*';
//    result.printf("%d: ", i);
//    if (s->type == _re::State::CHAR)
//      result.printf("%c", s->chr);
//    else if (s->type == _re::State::CCLASS)
//      result.printf("[class]");
//    else if (s->type == _re::State::END)
//      result.printf("[end]");
//    else if (s->type == _re::State::RBRA)
//      result.printf("RBRA(%d)", s->subid);
//    else if (s->type == _re::State::LBRA)
//      result.printf("LBRA(%d)", s->subid);
//    else if (s->type == _re::State::OR)
//      result.printf("OR");
//    else if (s->type == _re::State::BOL)
//      result.printf("BOL");
//    else if (s->type == _re::State::EOL)
//      result.printf("EOL");
//    if (s->type == _re::State::OR)
//      result.printf(" -> %d, %d\n", s->left ? s->left - prog->states : -1, s->right ? s->right - prog->states : -1);
//    else
//      result.printf(" -> %d\n", s->next ? s->next - prog->states : -1);
//  }
//  return result;
//}
//
//static bool addmatch(_re::Match const& match, void* arg)
//{
//  String& result = * (String*) arg;
//  result += "Match: ";
//  for (char const* c = match.start[0]; c < match.end[0]; c++)
//    result += *c;
//  result += "\n";
//  for (int i = 1; i < sizeof match.start / sizeof match.start[0]; i++)
//  {
//    if (match.start[i] && match.end[i])
//    {
//      result += "  Capture: ";
//      for (char const* c = match.start[i]; c < match.end[i]; c++)
//        result += *c;
//      result += "\n";
//    }
//  }
//  return true;
//}
//String RegExp::find(char const* text, bool exact)
//{
//  String result = "";
//  int found = prog->run(text, exact, addmatch, &result);
//  result.printf("Found %d matches\n", found);
//  return result;
//}
