#ifndef __UI_SEARCHWND__
#define __UI_SEARCHWND__

#include "frameui/controlframes.h"
#include "frameui/listctrl.h"

class SearchWindow : public Frame
{
  EditFrame* path;
  struct StringInfo
  {
    ComboFrame* mode;
    EditFrame* text;
  };
  StringInfo fileName;
  StringInfo gameName;
  StringInfo gameMode;
  struct TextRangeInfo
  {
    EditFrame* min;
    EditFrame* max;
  };
  struct DateRangeInfo
  {
    DateTimeFrame* min;
    DateTimeFrame* max;
  };
  TextRangeInfo numPlayers;
  TextRangeInfo mapVersion;
  TextRangeInfo wc3Version;
  DateRangeInfo gameLength;
  DateRangeInfo gameDate;
  struct PlayerInfo
  {
    StringInfo name;
    ComboFrameEx* hero;
  };
  PlayerInfo players[5];
  void fillHeroes(ComboFrameEx* cf);
  void fillStringMode(ComboFrame* cf);
  void createStringInfo(StringInfo& si, String tip);
  void createRangeInfo(TextRangeInfo& ti, String tip, int flags);
  void createRangeInfo(DateRangeInfo& di, String tip, int flags);

  uint32 strToInt(String str, uint32 def);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  SearchWindow(Frame* parent);

  void setPath(String path);
};

#endif // __UI_SEARCHWND__
