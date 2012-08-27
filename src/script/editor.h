#ifndef __SCRIPT_EDITOR__
#define __SCRIPT_EDITOR__

#include "frameui/framewnd.h"
#include "frameui/dragdrop.h"
#include "base/pool.h"

class ScriptEditor : public WindowFrame
{
  HFONT fonts[3];
  HCURSOR cursors[2];
  Point chSize;

  friend class ScriptSuggest;
  ScriptSuggest* suggestList;
  int suggestPos;
  void onSuggest(String text);
  String getSuggestString();

  struct HistoryItem
  {
    bool glue;
    int begin;
    int end;
    String text;
  };
  Array<HistoryItem> history;
  int historyPos;

  struct TextBlock
  {
    String text;
    int newlines;
    int type;

    TextBlock* prev;
    TextBlock* next;

    TextBlock* nextPair;

    TextBlock()
    {
      newlines = 0;
      prev = NULL;
      next = NULL;
      nextPair = NULL;
    }
  };
  TypedMemoryPool<TextBlock> pool;

  TextBlock* lexer(String text, TextBlock* next);

  TextBlock* firstBlock;
  TextBlock* lastBlock;
  TextBlock* firstPair;
  uint32 pairColor;

  struct Position
  {
    TextBlock* block;
    int offset;
  };
  Position makePosition(TextBlock* block, int offset);
  Position makePosition(int absolute);
  int getPosition(Position pos);
  void replace(int ibegin, int iend, String text, HistoryItem* mod = NULL, bool glue = false);

  Point extent;
  void updateExtent();

  int getBlockKeyword(TextBlock* block);

  int caret;
  int selStart;
  Point scrollPos;
  Point scrollAccum;
  bool insertMode;
  int dropPos;
  int dragop;

  String getSelection();
  String sanitize(String text);
  String unsanitize(String text);

  int getWordSize(int pos, int dir);

  DropTarget* target;

  void placeCaret();
  void updateCaret();
  void doScroll(int horz, int vert);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ScriptEditor(Frame* parent, HFONT hFont = NULL);
  ~ScriptEditor();

  int getTextLength() const;
  int posFromScreen(int line, int col) const;
  void posToScreen(int pos, int& line, int& col) const;

  void suggest();
};

#endif // __SCRIPT_EDITOR__
