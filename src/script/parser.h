#ifndef __SCRIPT_PARSER__
#define __SCRIPT_PARSER__

#include "script/data.h"
#include "base/pool.h"

#define BLOCK_TEXT        0
#define BLOCK_IF          1
#define BLOCK_ELSEIF      2
#define BLOCK_ELSE        3
#define BLOCK_ENDIF       4
#define BLOCK_FOR         5
#define BLOCK_ENDFOR      6
#define BLOCK_VAR         7
#define BLOCK_ALIGN       8
#define BLOCK_ENDALIGN    9

class ScriptParser
{
  struct CodeBlock
  {
    int line, col;
    uint32 type;
    String text;
    CodeBlock* parent;
    CodeBlock* child;
    CodeBlock* next;
  };
  TypedMemoryPool<CodeBlock> pool;
  CodeBlock* code;
  CodeBlock* errblock;
  String errors;
  String mkerror(CodeBlock* b, String text);
  bool eval(String expr, String& result, ScriptGlobal* global);
public:
  ScriptParser(String script);
  ~ScriptParser();

  bool getError(int& line, int& col)
  {
    if (errblock)
    {
      line = errblock->line;
      col = errblock->col;
      return true;
    }
    return false;
  }

  String run(W3GReplay* w3g);
};

#endif // __SCRIPT_PARSER__
