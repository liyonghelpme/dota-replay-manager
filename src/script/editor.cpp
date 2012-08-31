#include "core/app.h"

#include "editor.h"

#include "graphics/imagelib.h"
#include "frameui/fontsys.h"
#include "script/data.h"

#include "script/parser.h"

enum {tNormal,
      tBlock,
      tComment,
      tCount
};
static uint32 blockColors[] = {
  0x000000,
  0x000080,
  0x008000,
  0xFF0000
};
static uint32 blockFonts[tCount] = {
  0,
  0,
  2
};

class ScriptSuggest : public Window
{
  ScriptEditor* editor;
  struct Option
  {
    String text;
    int icon;
  };
  static int ocompare(Option const& a, Option const& b)
  {
    return a.text.icompare(b.text);
  }
  Array<Option> options;
  int selected;
  HCURSOR cursor;
  uint32 onWndMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ScriptSuggest(ScriptEditor* wnd);
  void clear();
  void addOption(int type, String text);
  void display(int x, int y);

  void finish();

  bool onKey(int key);
  void onString(String str);
  void tryFinish(String str);

  bool visible() const
  {
    return IsWindowVisible(hWnd) != 0;
  }
};
uint32 ScriptSuggest::onWndMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_ERASEBKGND:
    return TRUE;
  case WM_SETCURSOR:
    SetCursor(cursor);
    return TRUE;
  case WM_MOUSEMOVE:
    selected = HIWORD(lParam) / 16;
    invalidate();
    return 0;
  case WM_LBUTTONDOWN:
    selected = HIWORD(lParam) / 16;
    finish();
    return 0;
  case WM_PAINT:
    {
      RECT rc;
      GetClientRect(hWnd, &rc);
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);
      SelectObject(hDC, FontSys::getSysFont());
      HIMAGELIST img = getApp()->getImageLibrary()->getImageList();
      rc.top = 0;
      SetBkMode(hDC, OPAQUE);
      for (int i = 0; i < options.length(); i++)
      {
        rc.bottom = rc.top + 16;
        ImageList_Draw(img, options[i].icon, hDC, rc.left, rc.top, ILD_NORMAL);
        rc.left = 16;
        if (selected == i)
        {
          SetBkColor(hDC, 0x6A240A);
          SetTextColor(hDC, 0xFFFFFF);
        }
        else
        {
          SetBkColor(hDC, 0xFFFFFF);
          SetTextColor(hDC, 0x000000);
        }
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
        rc.left += 4;
        DrawText(hDC, options[i].text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER);
        rc.left = 0;
        rc.top = rc.bottom;
      }
      if (rc.top < ps.rcPaint.bottom)
      {
        rc.bottom = ps.rcPaint.bottom;
        SetBkColor(hDC, 0xFFFFFF);
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
      }
      EndPaint(hWnd, &ps);
    }
    return 0;
  }
  return Window::onWndMessage(message, wParam, lParam);
}
void ScriptSuggest::finish()
{
  if (selected >= 0 && selected < options.length())
    editor->onSuggest(options[selected].text);
  hideWindow();
}
bool ScriptSuggest::onKey(int key)
{
  if (!visible())
    return false;
  if (key == VK_DOWN)
  {
    selected++;
    if (selected >= options.length())
      selected = options.length() - 1;
    invalidate();
    return true;
  }
  if (key == VK_UP)
  {
    selected--;
    if (selected < 0)
      selected = 0;
    invalidate();
    return true;
  }
  if (key == VK_RETURN)
  {
    hideWindow();
    if (selected >= 0)
    {
      editor->onSuggest(options[selected].text);
      return true;
    }
  }
  if (key == VK_ESCAPE)
  {
    hideWindow();
    return true;
  }
  if (key == VK_LEFT || key == VK_RIGHT || key == VK_DELETE)
    hideWindow();
  return false;
}
void ScriptSuggest::onString(String str)
{
  selected = 0;
  while (selected < options.length() && options[selected].text.icompare(str) < 0)
    selected++;
  if (str.isEmpty() || selected >= options.length() ||
      options[selected].text.substring(0, str.length()).icompare(str))
    selected = -1;
  invalidate();
}
void ScriptSuggest::tryFinish(String str)
{
  int option = 0;
  while (option < options.length() && options[option].text.icompare(str) < 0)
    option++;
  if (option < options.length() &&
     !options[option].text.substring(0, str.length()).icompare(str) &&
     (option >= options.length() - 1 ||
      options[option + 1].text.substr(0, str.length()).icompare(str)))
  {
    hideWindow();
    editor->onSuggest(options[option].text);
  }
}
ScriptSuggest::ScriptSuggest(ScriptEditor* wnd)
{
  editor = wnd;
  cursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
  create(0, 0, 10, 10, "", WS_POPUP | WS_DLGFRAME, WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
}
void ScriptSuggest::clear()
{
  hideWindow();
  options.clear();
  selected = -1;
}
void ScriptSuggest::addOption(int type, String text)
{
  Option& o = options.push();
  if (type == 0)
    o.icon = getApp()->getImageLibrary()->getListIndex("suggestvar");
  else if (type == 1)
    o.icon = getApp()->getImageLibrary()->getListIndex("suggeststruct");
  else if (type == 2)
    o.icon = getApp()->getImageLibrary()->getListIndex("suggestenum");
  else
    o.icon = getApp()->getImageLibrary()->getListIndex("Empty");
  o.text = text;
}
void ScriptSuggest::display(int x, int y)
{
  if (options.length())
  {
    options.sort(ocompare);
    RECT rc = {0, 0, 150, 16 * options.length()};
    AdjustWindowRectEx(&rc, WS_POPUP | WS_DLGFRAME, FALSE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    SetWindowPos(hWnd, NULL, x, y, rc.right - rc.left, rc.bottom - rc.top,
      SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
  }
}

void ScriptEditor::suggest()
{
  suggestPos = caret;
  int line, col;
  posToScreen(caret, line, col);
  POINT pt;
  pt.x = (col - scrollPos.x) * chSize.x + NUMBERS_WIDTH;
  pt.y = (line - scrollPos.y) * chSize.y + chSize.y;
  RECT rc;
  GetClientRect(hWnd, &rc);
  Position pos = makePosition(caret);
  if (pos.block && pos.block->prev && pos.offset == 0)
  {
    pos.block = pos.block->prev;
    pos.offset = pos.block->text.length();
  }
  if (pos.block && pos.offset > 0 && pos.block->text[0] == '{')
  {
    Array<String> forVar;
    Array<String> forEnum;
    Dictionary<ScriptType*> context;
    int level = 0;
    for (TextBlock* cur = pos.block->prev; cur; cur = cur->prev)
    {
      int kw = getBlockKeyword(cur);
      if (kw == BLOCK_ENDFOR || kw == BLOCK_ENDIF || kw == BLOCK_ENDALIGN)
        level++;
      else if (kw == BLOCK_FOR || kw == BLOCK_IF || kw == BLOCK_ALIGN)
      {
        level--;
        if (level < 0)
        {
          if (kw == BLOCK_FOR)
          {
            Array<String> parse;
            String text = cur->text;
            text.toLower();
            if (text.match("\\{for (\\w+) in ([a-zA-Z0-9_.]+)\\}", &parse))
            {
              forVar.push(parse[1]);
              forEnum.push(parse[2]);
            }
          }
          level = 0;
        }
      }
    }
    for (int i = forEnum.length() - 1; i >= 0; i--)
    {
      ScriptType* t = ScriptType::tGlobal->getSubType(forEnum[i]);
      if (t)
      {
        if (t->getEnumType())
          context.set(forVar[i], t->getEnumType());
      }
      else
      {
        int dot = forEnum[i].find('.');
        String var = forEnum[i].substring(0, dot >= 0 ? dot : String::toTheEnd);
        if (context.has(var))
        {
          ScriptType* sup = context.get(var);
          if (dot >= 0)
            sup = sup->getSubType(forEnum[i].substring(dot + 1));
          if (sup && sup->getEnumType())
            context.set(forVar[i], sup->getEnumType());
        }
      }
    }

    int start = pos.offset;
    while (start > 0 && (isalnum(pos.block->text[start - 1]) ||
        pos.block->text[start - 1] == '.' || pos.block->text[start - 1] == '_'))
      start--;
    String path = pos.block->text.substring(start, pos.offset);

    int dot = path.lastIndexOf('.');
    suggestPos -= pos.offset - (start + dot + 1);
    String curWord = path.substring(dot + 1);

    ScriptType* t;
    if (dot < 0)
      t = ScriptType::tGlobal;
    else
    {
      path.setLength(dot);
      t = ScriptType::tGlobal->getSubType(path);
      if (t == NULL)
      {
        dot = path.indexOf('.');
        String var = path.substring(0, dot >= 0 ? dot : String::toTheEnd);
        if (context.has(var))
        {
          t = context.get(var);
          if (dot >= 0)
            t = t->getSubType(path.substring(dot + 1));
        }
      }
    }
    if (t && pt.x >= 0 && pt.y >= 0 && pt.x < rc.right && pt.y < rc.bottom)
    {
      suggestList->clear();

      for (uint32 cur = t->dir.enumStart(); cur; cur = t->dir.enumNext(cur))
      {
        ScriptType* sub = t->getElement(t->dir.enumGetValue(cur));
        suggestList->addOption(sub->getType(), t->dir.enumGetKey(cur));
      }
      if (t == ScriptType::tGlobal)
      {
        for (uint32 cur = context.enumStart(); cur; cur = context.enumNext(cur))
          suggestList->addOption(context.enumGetValue(cur)->getType(), context.enumGetKey(cur));
      }

      ClientToScreen(hWnd, &pt);
      suggestList->display(pt.x - 16, pt.y + 3);
      if (!curWord.isEmpty())
        suggestList->tryFinish(curWord);
    }
  }
}
String ScriptEditor::getSuggestString()
{
  if (suggestPos < 0)
    return "!";
  Position pos = makePosition(suggestPos);
  if (pos.block != NULL)
  {
    String result = "";
    int end = pos.offset + caret - suggestPos;
    while (pos.offset < pos.block->text.length() && pos.offset < end)
      result += pos.block->text[pos.offset++];
    if (pos.offset != end)
      return "!";
    return result;
  }
  return "!";
}
void ScriptEditor::onSuggest(String text)
{
  if (suggestPos >= 0)
  {
    Position pos = makePosition(suggestPos);
    if (pos.block && pos.block->prev && pos.offset == 0)
    {
      pos.block = pos.block->prev;
      pos.offset = pos.block->text.length();
    }
    int suggestEnd = pos.offset;
    while (isalnum(pos.block->text[suggestEnd]) || pos.block->text[suggestEnd] == '_')
      suggestEnd++;
    suggestEnd += suggestPos - pos.offset;
    replace(suggestPos, suggestEnd, text);
  }
}

//////////////////////////////////////////////////////

ScriptEditor::Position ScriptEditor::makePosition(TextBlock* block, int offset)
{
  Position pos;
  pos.block = block;
  pos.offset = offset;
  return pos;
}
ScriptEditor::Position ScriptEditor::makePosition(int absolute)
{
  Position pos;
  pos.block = firstBlock;
  pos.offset = absolute;
  while (pos.block && pos.offset >= pos.block->text.length())
  {
    pos.offset -= pos.block->text.length();
    pos.block = pos.block->next;
  }
  if (pos.block == NULL && lastBlock)
  {
    pos.block = lastBlock;
    pos.offset = lastBlock->text.length();
  }
  return pos;
}
int ScriptEditor::getPosition(Position pos)
{
  if (pos.block == NULL && lastBlock)
  {
    pos.block = lastBlock;
    pos.offset = lastBlock->text.length();
  }
  while (pos.block && pos.block->prev)
  {
    pos.block = pos.block->prev;
    pos.offset += pos.block->text.length();
  }
  return pos.offset;
}
void ScriptEditor::updateExtent()
{
  extent.x = 0;
  extent.y = 1;
  int col = 0;
  for (TextBlock* block = firstBlock; block; block = block->next)
  {
    if (block->newlines)
    {
      for (int i = 0; i < block->text.length(); i++)
      {
        if (block->text[i] == '\n')
        {
          extent.y++;
          if (col > extent.x)
            extent.x = col;
          col = 0;
        }
        else
          col++;
      }
    }
    else
      col += block->text.length();
    if (col > extent.x)
      extent.x = col;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  SCROLLINFO si;
  memset(&si, 0, sizeof si);
  si.cbSize = sizeof si;
  si.fMask = SIF_PAGE | SIF_RANGE;
  si.nMin = 0;
  si.nPage = (rc.right - NUMBERS_WIDTH) / chSize.x;
  si.nMax = extent.x + 10;
  SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
  si.nPage = rc.bottom / chSize.y;
  si.nMax = extent.y + si.nPage - 2;
  SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

ScriptEditor::TextBlock* ScriptEditor::lexer(String text, TextBlock* next)
{
  TextBlock* block = pool.alloc();
  TextBlock* first = block;
  block->text = "";
  block->type = tNormal;
  int bracket = -1;
  for (int i = 0; true; i++)
  {
    while (i >= text.length() && next)
    {
      if ((block->type == tNormal && block->text.length() > 0 && (next->type == tNormal ||
        (block->text[block->text.length() - 1] == '$' && next->type == tComment))) ||
        block->type == tComment)
      {
        text += next->text;
        TextBlock* temp = next->next;
        pool.free(next);
        next = temp;
      }
      else
        break;
    }
    if (i >= text.length())
      break;
    block->text += text[i];
    if (text[i] == '\n')
    {
      block->newlines++;
      bracket = -1;
    }
    if (block->type == tComment)
    {
      if (text[i] == '\n')
      {
        block->next = pool.alloc();
        block->next->prev = block;
        block->next->text = "";
        block->next->type = tNormal;
        block = block->next;
      }
    }
    else
    {
      if (text[i] == '{')
      {
        if (bracket >= 0 && bracket == block->text.length() - 2)
          bracket = -1;
        else
          bracket = block->text.length() - 1;
      }
      else if (text[i] == '}' && bracket >= 0)
      {
        if (bracket > 0)
        {
          block->next = pool.alloc();
          block->next->prev = block;
          block->next->text = block->text.substring(bracket);
          block->text.setLength(bracket);
          block = block->next;
        }
        block->type = tBlock;
        block->next = pool.alloc();
        block->next->prev = block;
        block->next->text = "";
        block->next->type = tNormal;
        block = block->next;
        bracket = -1;
      }
      else if (text[i] == '$')
      {
        if (block->text.length() >= 2 && block->text[block->text.length() - 2] == '$')
        {
          if (block->text.length() > 2)
          {
            block->next = pool.alloc();
            block->next->prev = block;
            block->next->text = block->text.substring(block->text.length() - 2);
            block->text.setLength(block->text.length() - 2);
            block = block->next;
          }
          block->type = tComment;
          bracket = -1;
        }
      }
    }
  }
  if (block->text.length() == 0)
  {
    TextBlock* prev = block->prev;
    pool.free(block);
    block = prev;
  }
  if (block == NULL)
    return NULL;
  block->next = next;
  if (next)
    next->prev = block;
  else
    lastBlock = block;

  return first;
}
String ScriptEditor::getSelection()
{
  String text = "";
  Position begin = makePosition(selStart < caret ? selStart : caret);
  Position end = makePosition(selStart > caret ? selStart : caret);
  if (begin.block == end.block)
  {
    if (begin.block)
      text = begin.block->text.substring(begin.offset, end.offset);
  }
  else
  {
    if (begin.block)
      text = begin.block->text.substring(begin.offset);
    for (TextBlock* block = begin.block; block != end.block; block = block->next)
      if (block != begin.block)
        text += block->text;
    if (end.block)
      text += end.block->text.substr(0, end.offset);
  }
  return text;
}
void ScriptEditor::replace(int ibegin, int iend, String text, HistoryItem* mod, bool glue)
{
  if (ibegin > iend)
  {
    int temp = ibegin;
    ibegin = iend;
    iend = temp;
  }
  Position begin = makePosition(ibegin);
  Position end = makePosition(iend);
  selStart = ibegin + text.length();
  caret = selStart;
  TextBlock* prev = NULL;
  TextBlock* next = NULL;

  if (mod)
  {
    mod->begin = ibegin;
    mod->end = ibegin + text.length();
    mod->text = "";
    if (begin.block == end.block)
    {
      if (begin.block)
        mod->text = begin.block->text.substring(begin.offset, end.offset);
    }
    else
    {
      if (begin.block)
        mod->text = begin.block->text.substring(begin.offset);
      for (TextBlock* block = begin.block; block != end.block; block = block->next)
        if (block != begin.block)
          mod->text += block->text;
      if (end.block)
        mod->text += end.block->text.substr(0, end.offset);
    }
  }
  else
  {
    HistoryItem h;
    h.begin = ibegin;
    h.end = ibegin + text.length();
    h.text = "";
    h.glue = glue;
    if (begin.block == end.block)
    {
      if (begin.block)
        h.text = begin.block->text.substring(begin.offset, end.offset);
    }
    else
    {
      if (begin.block)
        h.text = begin.block->text.substring(begin.offset);
      for (TextBlock* block = begin.block; block != end.block; block = block->next)
        if (block != begin.block)
          h.text += block->text;
      if (end.block)
        h.text += end.block->text.substr(0, end.offset);
    }

    bool push = true;
    if (historyPos < history.length())
      history.resize(historyPos);
    else if (historyPos > 0 && h.text.isEmpty() && h.end == h.begin + 1 && !h.glue)
    {
      HistoryItem& p = history[historyPos - 1];
      if (p.text.isEmpty() && h.begin == p.end)
      {
        push = false;
        p.end = h.end;
      }
    }
    if (push)
    {
      if (history.length() >= 256)
      {
        for (int i = 1; i < history.length(); i++)
          history[i - 1] = history[i];
        history[history.length() - 1] = h;
        if (origHistory >= 0)
          origHistory--;
      }
      else
        history.push(h);
    }
    historyPos = history.length();
    if (historyPos < origHistory)
      origHistory = -1;
  }

  if (begin.block)
  {
    prev = begin.block->prev;
    if (end.block)
      next = end.block->next;
    text = begin.block->text.substring(0, begin.offset) + text;
    if (end.block)
    {
      text += end.block->text.substring(end.offset);
      end.block->next = NULL;
    }
    while (begin.block)
    {
      TextBlock* nextDel = begin.block->next;
      pool.free(begin.block);
      begin.block = nextDel;
    }
  }
  else
    prev = lastBlock;
  while (prev && prev->type == tNormal)
  {
    text = prev->text + text;
    TextBlock* temp = prev->prev;
    pool.free(prev);
    prev = temp;
  }
  begin.block = lexer(text, next);
  if (begin.block)
  {
    begin.block->prev = prev;
    if (prev)
      prev->next = begin.block;
    else
      firstBlock = begin.block;
  }
  else
  {
    if (prev)
      prev->next = next;
    else
      firstBlock = next;
    if (next)
      next->prev = prev;
    else
      lastBlock = prev;
  }

  updateExtent();
  updateCaret();

  if (id())
    notify(WM_COMMAND, MAKELONG(id(), EN_CHANGE), (uint32) hWnd);
}

uint32 ScriptEditor::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    delete target;
    target = NULL;
    return 0;
  case WM_SETTEXT:
    replace(0, getTextLength(), sanitize((char*) lParam));
    selStart = caret = 0;
    history.clear();
    historyPos = 0;
    origHistory = 0;
    updateCaret();
    return TRUE;
  case WM_GETTEXTLENGTH:
    {
      String text = "";
      for (TextBlock* block = firstBlock; block; block = block->next)
        text += block->text;
      return unsanitize(text).length();
    }
    break;
  case WM_GETTEXT:
    {
      String text = "";
      for (TextBlock* block = firstBlock; block; block = block->next)
        text += block->text;
      text = unsanitize(text);
      int bufSize = wParam;
      if (bufSize > text.length() + 1)
        bufSize = text.length() + 1;
      memcpy((char*) lParam, text.c_str(), bufSize);
      return bufSize;
    }
    break;
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      if (selStart != caret)
      {
        int sela = (selStart < caret ? selStart : caret);
        int selb = (selStart > caret ? selStart : caret);
        if (pt.x < NUMBERS_WIDTH)
          pt.x = NUMBERS_WIDTH;
        pt.x = (pt.x - NUMBERS_WIDTH) / chSize.x + scrollPos.x;
        pt.y = pt.y / chSize.y + scrollPos.y;
        int abs = posFromScreen(pt.y, pt.x);
        if (abs >= sela && abs < selb)
          SetCursor(cursors[1]);
        else
          SetCursor(cursors[0]);
      }
      else
        SetCursor(cursors[0]);
    }
    else
      SetCursor(cursors[1]);
    return TRUE;
  case WM_SIZE:
    updateExtent();
    return 0;
  case WM_ERASEBKGND:
    return TRUE;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);

      SetBkMode(hDC, OPAQUE);

      if (NUMBERS_WIDTH - scrollPos.x * chSize.x > ps.rcPaint.left)
      {
        //SelectObject(hDC, linePen);
        //MoveToEx(hDC, NUMBERS_WIDTH - scrollPos.x - 1, ps.rcPaint.top, NULL);
        //LineTo(hDC, NUMBERS_WIDTH - scrollPos.x - 1, ps.rcPaint.bottom);

        RECT lineRc;
        //lineRc.left = NUMBERS_WIDTH - scrollPos.x - 3;
        //lineRc.right = NUMBERS_WIDTH - scrollPos.x;
        //lineRc.top = ps.rcPaint.top;
        //lineRc.bottom = ps.rcPaint.bottom;

        //SetBkColor(hDC, 0xFFFFFF);
        //ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRc, NULL, 0, NULL);

        SetBkColor(hDC, 0xC0C0C0);
        lineRc.left = 0;
        lineRc.right = NUMBERS_WIDTH - scrollPos.x * chSize.x;
        lineRc.top = 0;

        int selLine, selCol;
        posToScreen(caret, selLine, selCol);

        for (int y = scrollPos.y; y < extent.y && (y - scrollPos.y) * chSize.y < ps.rcPaint.bottom; y++)
        {
          SelectObject(hDC, y == selLine ? fonts[1] : fonts[0]);
          SetTextColor(hDC, y == selLine ? 0x303030 : 0x505050);
          lineRc.bottom = lineRc.top + chSize.y;
          DrawText(hDC, String::format("%3d", y + 1), -1, &lineRc, DT_RIGHT);
          lineRc.top = lineRc.bottom;
        }
        if (lineRc.top < ps.rcPaint.bottom)
        {
          lineRc.bottom = ps.rcPaint.bottom;
          ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRc, NULL, 0, NULL);
        }
      }

      SetBkColor(hDC, 0xFFFFFF);

      uint32 selBkColor = 0x6A240A;
      uint32 selFgColor = 0xFFFFFF;
      if (GetFocus() != hWnd)
      {
        selBkColor = 0x505050;
        selFgColor = 0xC8D0D4;
      }

      Position sela = makePosition(selStart < caret ? selStart : caret);
      Position selb = makePosition(selStart > caret ? selStart : caret);
      RECT rcLine;
      rcLine.left = -scrollPos.x * chSize.x + NUMBERS_WIDTH;
      rcLine.top = -scrollPos.y * chSize.y;
      rcLine.right = rcLine.left;
      rcLine.bottom = rcLine.top + chSize.y;
      bool inSelection = false;
      TextBlock* pair = firstPair;
      for (TextBlock* block = firstBlock; block && rcLine.top < ps.rcPaint.bottom; block = block->next)
      {
        SelectObject(hDC, fonts[blockFonts[block->type]]);
        int cur = 0;
        while (true)
        {
          int next = block->text.find('\n', cur);
          int length = (next >= 0 ? next - cur : block->text.length() - cur);
          if (length && rcLine.bottom > ps.rcPaint.top)
          {
            int selFrom = (inSelection ? 0 : length);
            if (block == sela.block)
            {
              selFrom = sela.offset - cur;
              if (selFrom < 0) selFrom = 0;
              if (selFrom > length) selFrom = length;
            }
            int selTo = length;
            if (block == selb.block)
            {
              selTo = selb.offset - cur;
              if (selTo < 0) selTo = 0;
              if (selTo > length) selTo = length;
            }
            if (selFrom >= selTo)
            {
              selFrom = length;
              selTo = length;
            }
            uint32 color;
            if (block == pair)
              color = pairColor;
            else if (getBlockKeyword(block) != 0)
              color = blockColors[tCount];
            else
              color = blockColors[block->type];
            if (selFrom > 0)
            {
              SetBkColor(hDC, 0xFFFFFF);
              SetTextColor(hDC, color);
              rcLine.right = rcLine.left + chSize.x * selFrom;
              DrawText(hDC, block->text.c_str() + cur, selFrom, &rcLine, 0);
              rcLine.left = rcLine.right;
            }
            if (selFrom < selTo)
            {
              SetBkColor(hDC, selBkColor);
              SetTextColor(hDC, selFgColor);
              rcLine.right = rcLine.left + chSize.x * (selTo - selFrom);
              DrawText(hDC, block->text.c_str() + cur + selFrom, selTo - selFrom, &rcLine, 0);
              rcLine.left = rcLine.right;
            }
            if (selTo < length)
            {
              SetBkColor(hDC, 0xFFFFFF);
              SetTextColor(hDC, color);
              rcLine.right = rcLine.left + chSize.x * (length - selTo);
              DrawText(hDC, block->text.c_str() + cur + selTo, length - selTo, &rcLine, 0);
              rcLine.left = rcLine.right;
            }
          }
          if (next >= 0)
          {
            cur = next + 1;
            if (rcLine.left < ps.rcPaint.right)
            {
              rcLine.right = ps.rcPaint.right;
              SetBkColor(hDC, 0xFFFFFF);
              ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);
            }
            rcLine.top = rcLine.bottom;
            rcLine.bottom = rcLine.top + chSize.y;
            rcLine.left = -scrollPos.x * chSize.x + NUMBERS_WIDTH;
          }
          else
            break;
        }
        if (block == sela.block)
          inSelection = true;
        if (block == selb.block)
          inSelection = false;
        if (block == pair)
          pair = pair->nextPair;
      }
      SetBkColor(hDC, 0xFFFFFF);
      if (rcLine.left < ps.rcPaint.right)
      {
        rcLine.right = ps.rcPaint.right;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);
      }
      if (rcLine.bottom < ps.rcPaint.bottom)
      {
        rcLine.left = -scrollPos.x * chSize.x + NUMBERS_WIDTH;
        rcLine.right = ps.rcPaint.right;
        rcLine.top = rcLine.bottom;
        rcLine.bottom = ps.rcPaint.bottom;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);
      }

      EndPaint(hWnd, &ps);
    }
    return 0;
  case WM_SETFOCUS:
    placeCaret();
    return 0;
  case WM_KILLFOCUS:
    if ((HWND) wParam != suggestList->getHandle())
      suggestList->hideWindow();
    DestroyCaret();
    updateCaret();
    return 0;
  case WM_LBUTTONDBLCLK:
    {
      suggestList->hideWindow();
      int x = GET_X_LPARAM(lParam);
      if (x < NUMBERS_WIDTH)
        x = NUMBERS_WIDTH;
      x = (x - NUMBERS_WIDTH + chSize.x / 2) / chSize.x + scrollPos.x;
      int y = GET_Y_LPARAM(lParam) / chSize.y + scrollPos.y;
      int abs = posFromScreen(y, x);
      caret = abs + getWordSize(abs, 1);
      selStart = caret - getWordSize(caret, -1);
      updateCaret();
    }
    return 0;
  case WM_LBUTTONDOWN:
    {
      suggestList->hideWindow();
      int x = GET_X_LPARAM(lParam);
      if (x < NUMBERS_WIDTH)
        x = NUMBERS_WIDTH;
      x = (x - NUMBERS_WIDTH + chSize.x / 2) / chSize.x + scrollPos.x;
      int y = GET_Y_LPARAM(lParam) / chSize.y + scrollPos.y;
      int abs = posFromScreen(y, x);
      int sela = (selStart < caret ? selStart : caret);
      int selb = (selStart > caret ? selStart : caret);
      if (abs >= sela && abs < selb)
      {
        dragop = 1;
        if (DoDragDropEx(CF_TEXT, CreateGlobalText(unsanitize(getSelection())),
            DROPEFFECT_MOVE | DROPEFFECT_COPY, hWnd) == DROPEFFECT_NONE)
          dragop = 0;
      }
      if (dragop == 0)
      {
        caret = abs;
        if (!(wParam & MK_SHIFT))
          selStart = caret;
      }
      dragop = 0;
      SetCapture(hWnd);
      if (GetFocus() != hWnd)
        SetFocus(hWnd);
      updateCaret();
    }
    return 0;
  case WM_MOUSEMOVE:
    if (GetCapture() == hWnd && (wParam & MK_LBUTTON))
    {
      int x = GET_X_LPARAM(lParam);
      if (x < NUMBERS_WIDTH)
        x = NUMBERS_WIDTH;
      x = (x - NUMBERS_WIDTH + chSize.x / 2) / chSize.x + scrollPos.x;
      int y = GET_Y_LPARAM(lParam) / chSize.y + scrollPos.y;
      int abs = posFromScreen(y, x);
      caret = abs;
      updateCaret();
    }
    return 0;
  case WM_LBUTTONUP:
    ReleaseCapture();
    return 0;
  case WM_CHAR:
    if (isprint(wParam) && (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
    {
      if (suggestList->visible() && !isalnum(wParam))
        suggestList->finish();
      if (caret == selStart && !insertMode && caret < getTextLength())
        replace(caret, caret + 1, String((char) wParam));
      else
        replace(selStart, caret, String((char) wParam));
      if (wParam == '.')
        suggest();
      else if (suggestList->visible())
      {
        String text = getSuggestString();
        if (text.isAlNum())
          suggestList->onString(text);
        else
          suggestList->hideWindow();
      }
    }
    return 0;
  case WM_KEYDOWN:
    if (suggestList->onKey(wParam))
      wParam = 0;
    switch (wParam)
    {
    case VK_HOME:
      {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
          caret = 0;
        else
        {
          int line, col;
          posToScreen(caret, line, col);
          caret = posFromScreen(line, 0);
        }
        if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
          selStart = caret;
        updateCaret();
      }
      break;
    case VK_END:
      {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
          caret = getTextLength();
        else
        {
          int line, col;
          posToScreen(caret, line, col);
          caret = posFromScreen(line, 100000);
        }
        if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
          selStart = caret;
        updateCaret();
      }
      break;
    case VK_TAB:
      {
        bool swap = (selStart < caret);
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        int line1, col1, line2, col2;
        posToScreen(selStart < caret ? selStart : caret, line1, col1);
        posToScreen(selStart > caret ? selStart : caret, line2, col2);
        if (line1 != line2)
        {
          for (int line = line1; line <= line2; line++)
          {
            if (line < line2 || col2 != 0)
            {
              int pos = posFromScreen(line, 0);
              if (shift)
              {
                Position vpos = makePosition(pos);
                int count = 0;
                if (vpos.block->text[vpos.offset] == ' ')
                  count++;
                if (count && vpos.block->text[vpos.offset + 1] == ' ')
                  count++;
                if (count)
                {
                  replace(pos, pos + count, "");
                  if (line == line1)
                    col1 -= count;
                  if (line == line2)
                    col2 -= count;
                }
              }
              else
                replace(pos, pos, "  ");
            }
          }
          if (!shift)
          {
            if (col1 != 0)
              col1 += 2;
            if (col2 != 0)
              col2 += 2;
          }
          else
          {
            if (col1 < 0)
              col1 = 0;
            if (col2 < 0)
              col2 = 0;
          }
          caret = posFromScreen(line1, col1);
          selStart = posFromScreen(line2, col2);
          if (swap)
          {
            int temp = caret;
            caret = selStart;
            selStart = temp;
          }
          updateCaret();
        }
        else if (!shift)
        {
          if (col1 & 1)
            replace(selStart, caret, " ");
          else
            replace(selStart, caret, "  ");
        }
      }
      break;
    case VK_BACK:
      if (selStart != caret)
        replace(selStart, caret, "");
      else if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        replace(caret - getWordSize(caret, -1), caret, "");
      else if (caret > 0)
        replace(caret - 1, caret, "");
      break;
    case VK_DELETE:
      if (selStart != caret)
        replace(selStart, caret, "");
      else if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        replace(caret, caret + getWordSize(caret, 1), "");
      else if (caret < getTextLength())
        replace(caret, caret + 1, "");
      break;
    case VK_LEFT:
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        caret -= getWordSize(caret, -1);
      else if (caret > 0)
        caret--;
      if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
        selStart = caret;
      updateCaret();
      break;
    case VK_RIGHT:
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        caret += getWordSize(caret, 1);
      else if (caret < getTextLength())
        caret++;
      if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
        selStart = caret;
      updateCaret();
      break;
    case VK_UP:
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        doScroll(scrollPos.x, scrollPos.y - 1);
      else
      {
        int line, col;
        posToScreen(caret, line, col);
        if (line > 0)
          caret = posFromScreen(line - 1, col);
        if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
          selStart = caret;
        updateCaret();
      }
      break;
    case VK_DOWN:
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        doScroll(scrollPos.x, scrollPos.y + 1);
      else
      {
        int line, col;
        posToScreen(caret, line, col);
        caret = posFromScreen(line + 1, col);
        if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
          selStart = caret;
        updateCaret();
      }
      break;
    case VK_RETURN:
      if (caret == selStart && !insertMode && caret < getTextLength())
        replace(caret, caret + 1, "\n");
      else
        replace(selStart, caret, "\n");
      break;
    case VK_INSERT:
      insertMode = !insertMode;
      placeCaret();
      break;
    case 'Z':
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      {
        bool glue = true;
        bool first = true;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        {
          while (glue && historyPos < history.length())
          {
            HistoryItem& h = history[historyPos++];
            replace(h.begin, h.end, h.text, &h);
            glue = h.glue;
            h.glue = !first;
            first = false;
          }
        }
        else
        {
          while (glue && historyPos > 0)
          {
            HistoryItem& h = history[--historyPos];
            replace(h.begin, h.end, h.text, &h);
            glue = h.glue;
            h.glue = !first;
            first = false;
          }
        }
      }
      break;
    case 'A':
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      {
        selStart = 0;
        caret = getTextLength();
        updateCaret();
      }
      break;
    case 'C':
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      {
        if (caret != selStart)
          SetClipboard(CF_TEXT, CreateGlobalText(unsanitize(getSelection())));
      }
      break;
    case 'X':
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      {
        if (caret != selStart)
        {
          SetClipboard(CF_TEXT, CreateGlobalText(unsanitize(getSelection())));
          replace(selStart, caret, "");
        }
      }
      break;
    case 'V':
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      {
        ClipboardReader reader(CF_TEXT);
        if (reader.getData())
        {
          int pos = (selStart < caret ? selStart : caret);
          String text = sanitize(GetGlobalText(reader.getData()));
          replace(selStart, caret, text);
          selStart = caret = pos + text.length();
          updateCaret();
        }
      }
      break;
    case VK_SPACE:
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        suggest();
      break;
    default:
      return 0;
    }
    if (suggestList->visible())
    {
      String text = getSuggestString();
      if (text.isAlNum())
        suggestList->onString(text);
      else
        suggestList->hideWindow();
    }
    return 0;
  case WM_VSCROLL:
    {
      SCROLLINFO si;
      memset(&si, 0, sizeof si);
      si.cbSize = sizeof si;
      si.fMask = SIF_ALL;
      GetScrollInfo(hWnd, SB_VERT, &si);
      switch (LOWORD(wParam))
      {
      case SB_TOP:
        si.nPos = si.nMin;
        break;
      case SB_BOTTOM:
        si.nPos = si.nMax;
        break;
      case SB_LINEUP:
        si.nPos--;
        break;
      case SB_LINEDOWN:
        si.nPos++;
        break;
      case SB_PAGEUP:
        si.nPos -= si.nPage;
        break;
      case SB_PAGEDOWN:
        si.nPos += si.nPage;
        break;
      case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;
      }
      doScroll(scrollPos.x, si.nPos);
    }
    return 0;
  case WM_HSCROLL:
    {
      SCROLLINFO si;
      memset(&si, 0, sizeof si);
      si.cbSize = sizeof si;
      si.fMask = SIF_ALL;
      GetScrollInfo(hWnd, SB_HORZ, &si);
      switch (LOWORD(wParam))
      {
      case SB_LEFT:
        si.nPos = si.nMin;
        break;
      case SB_RIGHT:
        si.nPos = si.nMax;
        break;
      case SB_LINELEFT:
        si.nPos--;
        break;
      case SB_LINERIGHT:
        si.nPos++;
        break;
      case SB_PAGELEFT:
        si.nPos -= si.nPage;
        break;
      case SB_PAGERIGHT:
        si.nPos += si.nPage;
        break;
      case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;
      }
      doScroll(si.nPos, scrollPos.y);
    }
    return 0;
  case WM_MOUSEWHEEL:
    {
      int step;
      SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &step, 0);
      if (step < 0)
        step = 3;
      scrollAccum.y += GET_WHEEL_DELTA_WPARAM(wParam) * step;
      doScroll(scrollPos.x, scrollPos.y - scrollAccum.y / WHEEL_DELTA);
      scrollAccum.y %= WHEEL_DELTA;
    }
    return 0;
  case WM_MOUSEHWHEEL:
    {
      scrollAccum.x += GET_WHEEL_DELTA_WPARAM(wParam) * 4;
      doScroll(scrollPos.x + scrollAccum.x / WHEEL_DELTA, scrollPos.y);
      scrollAccum.x %= WHEEL_DELTA;
    }
    return 0;
  case WM_DRAGOVER:
    {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (x < NUMBERS_WIDTH) x = NUMBERS_WIDTH;
      dropPos = posFromScreen(y / chSize.y + scrollPos.y,
        (x - NUMBERS_WIDTH + chSize.x / 2) / chSize.x + scrollPos.x);

      RECT rc;
      GetClientRect(hWnd, &rc);
      int xto = scrollPos.x;
      if (x < 10)
        xto--;
      else if (x > rc.right - 10)
        xto++;
      int yto = scrollPos.y;
      if (y < 10)
        yto--;
      else if (y > rc.bottom - 10)
        yto++;
      doScroll(xto, yto);

      int line, col;
      posToScreen(dropPos, line, col);
      CreateCaret(hWnd, NULL, 2, chSize.y);
      SetCaretPos((col - scrollPos.x) * chSize.x + NUMBERS_WIDTH - 1, (line - scrollPos.y) * chSize.y);
      ShowCaret(hWnd);
    }
    return 0;
  case WM_DRAGLEAVE:
    dropPos = 0;
    updateCaret();
    return 0;
  case WM_DRAGDROP:
    if (dragop)
    {
      int sela = (selStart < caret ? selStart : caret);
      int selb = (selStart > caret ? selStart : caret);
      if (dropPos < sela || dropPos > selb)
      {
        String text = getSelection();
        if (lParam != DROPEFFECT_COPY)
        {
          replace(sela, selb, "");
          if (dropPos > selb)
            dropPos -= (selb - sela);
          replace(dropPos, dropPos, text, NULL, true);
        }
        else
          replace(dropPos, dropPos, text);
        selStart = dropPos;
        caret = dropPos + text.length();
      }
    }
    else
    {
      String text = sanitize(GetGlobalText((HGLOBAL) wParam));
      replace(dropPos, dropPos, text);
      return DROPEFFECT_COPY;
    }
    return lParam;
  }
  return M_UNHANDLED;
}
String ScriptEditor::sanitize(char const* text)
{
  String result = "";
  for (int i = 0; text[i]; i++)
  {
    if (text[i] != '\r')
      result += text[i];
  }
  return result;
}
String ScriptEditor::unsanitize(char const* text)
{
  String result = "";
  for (int i = 0; text[i]; i++)
  {
    if (text[i] == '\n')
      result += '\r';
    result += text[i];
  }
  return result;
}
int ScriptEditor::getBlockKeyword(TextBlock* block)
{
  if (block == NULL || block->type != tBlock || block->text[0] != '{')
    return 0;
  int pos = 1;
  while (pos < block->text.length() && !isspace(block->text[pos]) &&
      block->text[pos] != '}' && block->text[pos] != ':')
    pos++;
  String keyword = block->text.substring(1, pos);
  if (!keyword.icompare("if"))
    return BLOCK_IF;
  else if (!keyword.icompare("elseif"))
    return BLOCK_ELSEIF;
  else if (!keyword.icompare("else"))
    return BLOCK_ELSE;
  else if (!keyword.icompare("endif"))
    return BLOCK_ENDIF;
  else if (!keyword.icompare("for"))
    return BLOCK_FOR;
  else if (!keyword.icompare("endfor"))
    return BLOCK_ENDFOR;
  else if (!keyword.icompare("align"))
    return BLOCK_ALIGN;
  else if (!keyword.icompare("endalign"))
    return BLOCK_ENDALIGN;
  return 0;
}
void ScriptEditor::placeCaret()
{
  if (GetFocus() == hWnd)
  {
    int line, col;
    posToScreen(caret, line, col);
    if (insertMode || caret != selStart)
      CreateCaret(hWnd, NULL, 1, chSize.y);
    else
      CreateCaret(hWnd, NULL, chSize.x, chSize.y);
    SetCaretPos((col - scrollPos.x) * chSize.x + NUMBERS_WIDTH, (line - scrollPos.y) * chSize.y);
    ShowCaret(hWnd);
  }
}
void ScriptEditor::doScroll(int horz, int vert)
{
  SCROLLINFO si;
  memset(&si, 0, sizeof si);
  si.cbSize = sizeof si;
  si.fMask = SIF_RANGE | SIF_PAGE;
  GetScrollInfo(hWnd, SB_HORZ, &si);
  if (horz < si.nMin) horz = si.nMin;
  if (horz > si.nMax - si.nPage + 1) horz = si.nMax - si.nPage + 1;
  GetScrollInfo(hWnd, SB_VERT, &si);
  if (vert < si.nMin) vert = si.nMin;
  if (vert > si.nMax - si.nPage + 1) vert = si.nMax - si.nPage + 1;
  si.fMask = SIF_POS;
  if (horz != scrollPos.x)
  {
    si.nPos = horz;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
  }
  if (vert != scrollPos.y)
  {
    si.nPos = vert;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
  }
  if (horz != scrollPos.x || vert != scrollPos.y)
  {
    int deltaX = scrollPos.x - horz;
    int deltaY = scrollPos.y - vert;
    scrollPos.x = horz;
    scrollPos.y = vert;
    ScrollWindowEx(hWnd, chSize.x * deltaX, chSize.y * deltaY, NULL, NULL, NULL, NULL,
      SW_INVALIDATE);
    if (GetFocus() == hWnd)
    {
      int line, col;
      posToScreen(caret, line, col);
      SetCaretPos((col - scrollPos.x) * chSize.x + NUMBERS_WIDTH, (line - scrollPos.y) * chSize.y);
    }
  }
}
void ScriptEditor::updateCaret()
{
  placeCaret();

  int line, col;
  posToScreen(caret, line, col);
  SCROLLINFO si;
  memset(&si, 0, sizeof si);
  si.cbSize = sizeof si;
  si.fMask = SIF_PAGE | SIF_RANGE;

  int xto = scrollPos.x;
  GetScrollInfo(hWnd, SB_HORZ, &si);
  if (col < scrollPos.x)
    xto = (col > 2 ? col - 2 : 0);
  else if (col >= scrollPos.x + si.nPage)
    xto = (col - si.nPage + 10 < si.nMax ? col - si.nPage + 10 : si.nMax);

  int yto = scrollPos.y;
  GetScrollInfo(hWnd, SB_VERT, &si);
  if (line < scrollPos.y)
    yto = (line > 1 ? line - 1 : 0);
  else if (line >= scrollPos.y + si.nPage)
    yto = (line - si.nPage + 1 < si.nMax ? line - si.nPage + 1 : si.nMax);

  doScroll(xto, yto);

  Position pos = makePosition(caret);
  firstPair = NULL;
  if (pos.block && pos.block->type != tBlock && caret == selStart && pos.offset == 0)
    pos.block = pos.block->prev;
  if (pos.block && pos.block->type == tBlock && caret == selStart && GetFocus() == hWnd)
  {
    int kw = getBlockKeyword(pos.block);
    if (kw != 0)
    {
      pairColor = 0xFF00FF;
      firstPair = pos.block;
      pos.block->nextPair = NULL;
      int level = 0;
      if (kw != BLOCK_IF && kw != BLOCK_FOR && kw != BLOCK_ALIGN)
      {
        for (TextBlock* cur = pos.block->prev; cur && level >= 0; cur = cur->prev)
        {
          int okw = getBlockKeyword(cur);
          if (okw == BLOCK_ENDIF || okw == BLOCK_ENDFOR || okw == BLOCK_ENDALIGN)
            level++;
          else if (okw != 0)
          {
            if (level == 0)
            {
              if (((kw <= BLOCK_ENDIF) == (okw <= BLOCK_ENDIF)) &&
                  ((kw <= BLOCK_ENDFOR) == (okw <= BLOCK_ENDFOR)))
              {
                cur->nextPair = firstPair;
                firstPair = cur;
              }
              else
              {
                pairColor = 0x0000FF;
                break;
              }
            }
            if (okw == BLOCK_IF || okw == BLOCK_FOR || okw == BLOCK_ALIGN)
              level--;
          }
        }
        if (level != -1)
          pairColor = 0x0000FF;
      }
      TextBlock* lastPair = pos.block;
      level = 0;
      if (kw != BLOCK_ENDIF && kw != BLOCK_ENDFOR && kw != BLOCK_ENDALIGN)
      {
        for (TextBlock* cur = pos.block->next; cur && level >= 0; cur = cur->next)
        {
          int okw = getBlockKeyword(cur);
          if (okw == BLOCK_IF || okw == BLOCK_FOR || okw == BLOCK_ALIGN)
            level++;
          else if (okw != 0)
          {
            if (level == 0)
            {
              if (((kw <= BLOCK_ENDIF) == (okw <= BLOCK_ENDIF)) &&
                  ((kw <= BLOCK_ENDFOR) == (okw <= BLOCK_ENDFOR)))
              {
                lastPair->nextPair = cur;
                cur->nextPair = NULL;
                lastPair = cur;
              }
              else
              {
                pairColor = 0x0000FF;
                break;
              }
            }
            if (okw == BLOCK_ENDIF || okw == BLOCK_ENDFOR || okw == BLOCK_ENDALIGN)
              level--;
          }
        }
        if (level != -1)
          pairColor = 0x0000FF;
      }
    }
    else
    {
      pairColor = 0x8000FF;
      firstPair = pos.block;
      pos.block->nextPair = NULL;
    }
  }
  invalidate();
}
ScriptEditor::ScriptEditor(Frame* parent, int id, HFONT hFont)
  : WindowFrame(parent)
{
  if (hFont == NULL)
    hFont = FontSys::getFont(12, "Courier New", 0);
  fonts[0] = FontSys::changeFlags(0, hFont);
  fonts[1] = FontSys::changeFlags(FONT_BOLD, hFont);
  fonts[2] = FontSys::changeFlags(FONT_ITALIC, hFont);
  chSize = FontSys::getTextSize(fonts[0], " ");
  NUMBERS_WIDTH = chSize.x * 3;
  firstBlock = NULL;
  lastBlock = NULL;
  firstPair = NULL;
  pairColor = 0xFF00FF;
  caret = 0;
  selStart = 0;
  scrollPos.x = scrollPos.y = 0;
  extent.x = extent.y = 0;
  scrollAccum.x = scrollAccum.y = 0;
  historyPos = 0;
  insertMode = true;
  target = NULL;
  dropPos = 0;
  dragop = 0;
  suggestList = new ScriptSuggest(this);
  suggestPos = -1;
  cursors[0] = LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM));
  cursors[1] = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
  linePen = CreatePen(PS_SOLID, 0, 0x808080);
  origHistory = 0;

  if (WNDCLASSEX* wcx = createclass("ScriptEditorWindow"))
  {
    wcx->style |= CS_DBLCLKS;
    RegisterClassEx(wcx);
  }
  create("", WS_CHILD | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE);
  setId(id);

  target = new DropTarget(this, CF_TEXT, DROPEFFECT_COPY | DROPEFFECT_MOVE);
}
ScriptEditor::~ScriptEditor()
{
  while (firstBlock)
  {
    TextBlock* next = firstBlock->next;
    pool.free(firstBlock);
    firstBlock = next;
  }
  DeleteObject(linePen);
  delete suggestList;
}

int ScriptEditor::getTextLength() const
{
  int length = 0;
  for (TextBlock* block = firstBlock; block; block = block->next)
    length += block->text.length();
  return length;
}
int ScriptEditor::posFromScreen(int line, int col) const
{
  int pos = 0;
  int offset = col;
  for (TextBlock* block = firstBlock; block; block = block->next)
  {
    if (block->newlines && block->newlines >= line)
    {
      int cur = 0;
      while (true)
      {
        int next = block->text.find('\n', cur);
        int length = (next >= 0 ? next - cur : block->text.length() - cur);
        if (line == 0)
        {
          if (offset < length)
            return pos + cur + offset;
          else if (next >= 0)
            return pos + next;
        }
        if (next >= 0)
        {
          cur = next + 1;
          line--;
          offset = col;
        }
        else
        {
          offset -= length;
          break;
        }
      }
      int prev = 0;
      while (prev)
      {
        int next = block->text.find('\n', prev);
        int length = (next >= 0 ? next - prev : block->text.length() - prev);
        if (line == 0)
          return (offset < length ? offset : length) + pos;
        line--;
        offset = col;
        prev = (next >= 0 ? next + 1 : -1);
      }
    }
    else if (block->newlines)
      line -= block->newlines;
    else if (line == 0)
    {
      if (offset < block->text.length())
        return pos + offset;
      offset -= block->text.length();
    }
    pos += block->text.length();
  }
  return getTextLength();
}
void ScriptEditor::posToScreen(int pos, int& line, int& col) const
{
  line = 0;
  col = 0;
  for (TextBlock* block = firstBlock; block; block = block->next)
  {
    if (block->newlines)
    {
      for (int i = 0; i < block->text.length() && i < pos; i++)
      {
        if (block->text[i] == '\n')
        {
          line++;
          col = 0;
        }
        else
          col++;
      }
      if (pos < block->text.length())
        return;
    }
    else if (pos < block->text.length())
    {
      col += pos;
      return;
    }
    else
      col += block->text.length();
    pos -= block->text.length();
  }
}
int ScriptEditor::getWordSize(int pos, int dir)
{
  Position cur = makePosition(pos);
  if (dir < 0)
  {
    while (cur.block && cur.block->prev && cur.offset == 0)
    {
      cur.block = cur.block->prev;
      cur.offset = cur.block->text.length();
    }
  }
  else
  {
    while (cur.block && cur.block->next && cur.offset == cur.block->text.length())
    {
      cur.block = cur.block->next;
      cur.offset = 0;
    }
  }
  if (cur.block == NULL)
    return 0;
  pos = cur.offset + (dir < 0 ? -1 : 0);
  int count = 0;
  int prevType = 0;
  while (pos >= 0 && pos < cur.block->text.length())
  {
    int type = isalnum(cur.block->text[pos]) ? 1 :
      (isspace(cur.block->text[pos]) ? -1 : 0);
    if (count > 0 && type != prevType && prevType != -1)
      break;
    count++;
    prevType = type;
    pos += dir;
  }
  return count;
}

void ScriptEditor::setCursor(int line, int col)
{
  selStart = caret = posFromScreen(line - 1, col - 1);
  updateCaret();
}
