#include "parser.h"

String ScriptParser::mkerror(CodeBlock* b, String text)
{
  errblock = b;
  return String::format("Line %d, pos %d: %s", b->line, b->col, text);
}
ScriptParser::ScriptParser(String script)
{
  errblock = NULL;
  errors = "";
  CodeBlock* outer = NULL;
  CodeBlock* last = NULL;
  code = NULL;
  String cur = "";
  int line = 0;
  int lineStart = 0;
  for (int pos = 0; pos <= script.length() && errors.isEmpty(); pos++)
  {
    if (script[pos] == '$' && script[pos + 1] == '$')
      while (script[pos] && script[pos] != '\r' && script[pos] != '\n')
        pos++;
    if (script[pos] == '\n')
    {
      line++;
      lineStart = pos + 1;
    }

    if ((script[pos] == '{' && script[pos + 1] == '{') ||
        (script[pos] == '}' && script[pos + 1] == '}'))
    {
      // do nothing
    }
    else if (((script[pos] == '{' || script[pos] == '}') &&
              (pos == 0 || script[pos - 1] != script[pos])) ||
              script[pos] == 0)
    {
      if (script[pos] == '}')
        cur += script[pos];
      if (!cur.isEmpty())
      {
        CodeBlock* next = pool.alloc();
        next->text = cur;
        next->next = NULL;
        next->child = NULL;
        next->parent = NULL;
        next->line = line + 1;
        next->col = (pos - lineStart) + 1;
        if (cur[0] != '{' || cur[cur.length() - 1] != '}')
          next->type = BLOCK_TEXT;
        else
        {
          next->col -= cur.length() - 1;
          next->text = cur.substring(1, cur.length() - 1).toLower();
          int subpos = 1;
          while (cur[subpos] != '}' && cur[subpos] != ':' && !s_isspace(cur[subpos]))
            subpos++;
          String keyword = cur.substring(1, subpos);
          if (!keyword.icompare("if"))
            next->type = BLOCK_IF;
          else if (!keyword.icompare("elseif"))
            next->type = BLOCK_ELSEIF;
          else if (!keyword.icompare("else"))
            next->type = BLOCK_ELSE;
          else if (!keyword.icompare("endif"))
            next->type = BLOCK_ENDIF;
          else if (!keyword.icompare("for"))
            next->type = BLOCK_FOR;
          else if (!keyword.icompare("endfor"))
            next->type = BLOCK_ENDFOR;
          else if (!keyword.icompare("align"))
            next->type = BLOCK_ALIGN;
          else if (!keyword.icompare("endalign"))
            next->type = BLOCK_ENDALIGN;
          else
            next->type = BLOCK_VAR;
        }
        if (next->type == BLOCK_ELSEIF || next->type == BLOCK_ELSE || next->type == BLOCK_ENDIF)
        {
          if (outer && (outer->type == BLOCK_IF || outer->type == BLOCK_ELSEIF || outer->type == BLOCK_ELSE) &&
            (outer->type < next->type || (outer->type == next->type && next->type == BLOCK_ELSEIF)))
          {
            outer->next = next;
            next->parent = outer->parent;
            if (next->type == BLOCK_ENDIF)
            {
              outer = next->parent;
              last = next;
            }
            else
            {
              outer = next;
              last = NULL;
            }
          }
          else
            errors = mkerror(next, "Illegal elseif/else/endif without matching if");
        }
        else if (next->type == BLOCK_ENDFOR)
        {
          if (outer && outer->type == BLOCK_FOR)
          {
            outer->next = next;
            next->parent = outer->parent;
            outer = next->parent;
            last = next;
          }
          else
            errors = mkerror(next, "Illegal endfor without matching for");
        }
        else if (next->type == BLOCK_ENDALIGN)
        {
          if (outer && outer->type == BLOCK_ALIGN)
          {
            outer->next = next;
            next->parent = outer->parent;
            outer = next->parent;
            last = next;
          }
          else
            errors = mkerror(next, "Illegal endalign without matching align");
        }
        else if (next->type == BLOCK_IF || next->type == BLOCK_FOR || next->type == BLOCK_ALIGN)
        {
          if (last)
            last->next = next;
          else if (outer)
            outer->child = next;
          else
            code = next;
          next->parent = outer;
          outer = next;
          last = NULL;
        }
        else
        {
          if (last)
            last->next = next;
          else if (outer)
            outer->child = next;
          else
            code = next;
          next->parent = outer;
          last = next;
        }
      }
      cur = "";
      if (script[pos] == '{')
        cur += script[pos];
    }
    else
      cur += script[pos];
  }
  if (outer && errors.isEmpty())
    errors = mkerror(outer, "Unfinished structure");
}
ScriptParser::~ScriptParser()
{
  CodeBlock* cur = code;
  while (cur)
  {
    if (cur->child)
    {
      CodeBlock* next = cur->child;
      cur->child = NULL;
      cur = next;
    }
    else if (cur->next)
    {
      CodeBlock* next = cur->next;
      pool.free(cur);
      cur = next;
    }
    else
    {
      CodeBlock* next = cur->parent;
      pool.free(cur);
      cur = next;
    }
  }
}

struct StackItem
{
  int type;
  int pos;
  ScriptValue* list;
  String var;
};

String ScriptParser::run(W3GReplay* w3g)
{
  if (!errors.isEmpty())
    return errors;

  ScriptGlobal* global = ScriptGlobal::create(w3g);

  Array<StackItem> stack;

  String result = "";
  CodeBlock* cur = code;
  while (cur)
  {
    bool sub = false;
    if (cur->type == BLOCK_FOR)
    {
      Array<String> match;
      if (cur->text.match("for (\\w+) in ([a-zA-Z0-9_.]+)", &match))
      {
        if (cur->child)
        {
          ScriptValue* val = global->getGlobalValue(match[2]);
          if (val && val->getEnumCount() > 0)
          {
            StackItem& fs = stack.push();
            fs.type = cur->type;
            fs.list = val;
            fs.pos = 0;
            fs.var = match[1];
            global->setGlobalValue(match[1], val->getEnum(0));
            sub = true;
          }
        }
      }
      else
        return mkerror(cur, "Invalid for loop (expected {for <name> in <list>})");
    }
    else if (cur->type == BLOCK_IF || cur->type == BLOCK_ELSEIF)
    {
      String result;
      if (!eval(cur->text.substring(cur->type == BLOCK_IF ? 2 : 6), result, global))
        return mkerror(cur, "Invalid condition");
      if (!result.isEmpty() && result.icompare("false"))
      {
        StackItem& fs = stack.push();
        fs.type = cur->type;
        sub = true;
      }
    }
    else if (cur->type == BLOCK_ALIGN)
    {
      StackItem& fs = stack.push();
      fs.type = cur->type;
      fs.pos = result.length();
      sub = true;
    }
    else if (cur->type == BLOCK_VAR)
    {
      ScriptValue* val = global->getGlobalValue(cur->text);
      if (val)
        result += val->getValue();
    }
    else if (cur->type == BLOCK_ELSE)
    {
      StackItem& fs = stack.push();
      fs.type = cur->type;
      sub = true;
    }
    else if (cur->type == BLOCK_TEXT)
      result += cur->text;
    if (cur->child && sub)
      cur = cur->child;
    else if (cur->next && !sub)
      cur = cur->next;
    else
    {
      if (!sub)
        cur = cur->parent;
      if (cur == NULL)
        break;
      int top = stack.length() - 1;
      if (top < 0 || stack[top].type != cur->type)
        return mkerror(cur, "Error handling structure");
      if (cur->type == BLOCK_FOR)
      {
        stack[top].pos++;
        if (stack[top].pos < stack[top].list->getEnumCount() && cur->child)
        {
          cur = cur->child;
          global->setGlobalValue(stack[top].var, stack[top].list->getEnum(stack[top].pos));
        }
        else
        {
          cur = cur->next;
          global->unsetGlobalValue(stack[top].var);
          stack.pop();
        }
      }
      else if (cur->type == BLOCK_IF || cur->type == BLOCK_ELSEIF || cur->type == BLOCK_ELSE)
      {
        stack.pop();
        while (cur && (cur->type == BLOCK_IF || cur->type == BLOCK_ELSEIF ||
            cur->type == BLOCK_ELSE))
          cur = cur->next;
      }
      else if (cur->type == BLOCK_ALIGN)
      {
        Array<String> match;
        if (cur->text.match("align (left|right) (\\d+)", &match))
        {
          int len = result.length() - stack[top].pos;
          int align = match[2].toInt();
          if (align > len)
          {
            if (match[1] == "left")
              result += String(' ') * (align - len);
            else
              result.insert(stack[top].pos, String(' ') * (align - len));
          }
        }
        else
          return mkerror(cur, "Invalid align (expected {align <left|right> <width>})");
        stack.pop();
        cur = cur->next;
      }
      else
        return mkerror(cur, "Unexpected block type");
    }
  }
  if (stack.length() != 0)
    return "Generic script execution error";

  delete global;

  return result;
}
