#include <locale>
#include "parser.h"

namespace ExprParser
{
  enum {opEnd, opOpen, opClose, opAnd, opOr, opNot,
    opEq, opNeq, opLt, opGt, opLeq, opGeq, opVar, opLit};
  typedef char const* eptr;
  int prio[] = {0, 1, 0, 2, 2, 3,
    4, 4, 4, 4, 4, 4};
  bool toBool(String var)
  {
    return !var.isEmpty() && var.icompare("false");
  }
  String toString(bool val)
  {
    return val ? "true" : "false";
  }
  bool exec(int op, Array<String>& values)
  {
    if (op == opOpen)
      return false;
    if (op == opNot)
    {
      if (values.length() < 1)
        return false;
      String& v = values[values.length() - 1];
      v = toString(!toBool(v));
      return true;
    }
    if (values.length() < 2)
      return false;
    String& a = values[values.length() - 2];
    String b = values[values.length() - 1];
    values.pop();
    if (op == opAnd)
      a = (toBool(a) ? b : a);
    else if (op == opOr)
      a = (toBool(a) ? a : b);
    else if (op == opEq)
      a = toString(a.icompare(b) == 0);
    else if (op == opNeq)
      a = toString(a.icompare(b) != 0);
    else if (op == opLt)
      a = toString(String::smartCompare(a.toLower(), b.toLower()) < 0);
    else if (op == opGt)
      a = toString(String::smartCompare(a.toLower(), b.toLower()) > 0);
    else if (op == opLeq)
      a = toString(String::smartCompare(a.toLower(), b.toLower()) <= 0);
    else if (op == opGeq)
      a = toString(String::smartCompare(a.toLower(), b.toLower()) >= 0);
    else
      return false;
    return true;
  }
  int next(eptr& expr, String& id)
  {
    while (*expr && isspace(*expr))
      expr++;
    if (*expr == 0)
      return opEnd;
    if (isalpha(*expr) || *expr == '_')
    {
      id = "";
      while (isalnum(*expr) || *expr == '_' || *expr == '.')
        id += *expr++;
      if (!id.icompare("not"))
        return opNot;
      if (!id.icompare("and"))
        return opAnd;
      if (!id.icompare("or"))
        return opOr;
      return opVar;
    }
    if (isdigit(*expr) || *expr == '-')
    {
      id = "";
      if (*expr == '-')
        id += *expr++;
      while (isdigit(*expr))
        id += *expr++;
      return opLit;
    }
    if (*expr == '\'' || *expr == '"')
    {
      id = "";
      char end = *expr++;
      while (*expr && *expr != end)
      {
        if (*expr == '\\')
          expr++;
        if (*expr)
          id += *expr++;
      }
      if (*expr != end)
        return -1;
      expr++;
      return opLit;
    }
    if (*expr == '(')
    {
      expr++;
      return opOpen;
    }
    if (*expr == ')')
    {
      expr++;
      return opClose;
    }
    if (*expr == '=')
    {
      expr++;
      if (*expr == '=')
        expr++;
      return opEq;
    }
    if (*expr == '!')
    {
      expr++;
      if (*expr != '=')
        return opNot;
      expr++;
      return opNeq;
    }
    if (*expr == '<')
    {
      expr++;
      if (*expr != '=')
        return opLt;
      expr++;
      return opLeq;
    }
    if (*expr == '>')
    {
      expr++;
      if (*expr != '=')
        return opGt;
      expr++;
      return opGeq;
    }
    return -1;
  }
}

bool ScriptParser::eval(String expr, String& result, ScriptGlobal* global)
{
  ExprParser::eptr ep = expr.c_str();
  result = "false";
  Array<String> values;
  Array<int> ops;
  bool prevValue = false;
  String id;
  while (true)
  {
    int sym = ExprParser::next(ep, id);
    if (sym < 0)
      return false;
    else if (sym == ExprParser::opNot)
    {
      if (prevValue || (ops.length() > 0 && ExprParser::prio[ops[ops.length() - 1]] >
          ExprParser::prio[sym]))
        return false;
      ops.push(sym);
    }
    else if (sym == ExprParser::opVar)
    {
      if (prevValue)
        return false;
      ScriptValue* val = global->getGlobalValue(id);
      values.push(val ? val->getValue() : "");
      prevValue = true;
    }
    else if (sym == ExprParser::opLit)
    {
      if (prevValue)
        return false;
      values.push(id);
      prevValue = true;
    }
    else if (sym == ExprParser::opOpen)
    {
      if (prevValue)
        return false;
      ops.push(sym);
    }
    else if (sym == ExprParser::opClose)
    {
      if (!prevValue)
        return false;
      while (ops.length() > 0 && ops[ops.length() - 1] != ExprParser::opOpen)
      {
        if (!ExprParser::exec(ops[ops.length() - 1], values))
          return false;
        ops.pop();
      }
      if (ops.length() == 0)
        return false;
      ops.pop();
      prevValue = false;
    }
    else
    {
      if (!prevValue)
        return false;
      while (ops.length() > 0 && ExprParser::prio[ops[ops.length() - 1]] >= ExprParser::prio[sym])
      {
        if (!ExprParser::exec(ops[ops.length() - 1], values))
          return false;
        ops.pop();
      }
      ops.push(sym);
      prevValue = false;
    }
    if (sym == ExprParser::opEnd)
      break;
  }
  if (values.length() != 1 || ops.length() != 1)
    return false;
  result = values[0];
  return true;
}
