#ifndef __UI_GRAPHWND__
#define __UI_GRAPHWND__

#include "frameui/framewnd.h"
#include "graphics/glib.h"

class GraphWindow : public WindowFrame
{
  Painter* painter;
  struct Point
  {
    int x;
    int y;
    bool dash;
  };
  struct Graph
  {
    Array<Point> points;
    uint32 color;
    bool dash;
    bool enabled;
  };
  Array<Graph> graphs;

  uint32 bkColor;
  uint32 lnColor;
  uint32 ruleColor;
  uint32 hMinorColor;
  uint32 vMinorColor;

  HCURSOR cursors[3];

  RECT rcSource;
  RECT rcSourceMax;
  RECT rcWindow;
  void extendRect(int x, int y);
  void rebuildRect();

  int marginX;
  int marginY;

  int clickMode;
  int clickX;
  int clickY;
  int dragX;
  int dragY;

  RECT tipRect;
  int tipGraph;
  String tipText;

  int mapx(int x);
  int mapy(int y);
  int unmapx(int x);
  int unmapy(int y);
  int unmaph(int w);
  int unmapv(int h);

  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
protected:
  void drawHRule(int y, char const* text);
  void drawVRule(int x, char const* text);
  int maph(int w);
  int mapv(int h);

  virtual String formatTip(int x, int y)
  {
    return "";
  }

  virtual void drawHRules(int start, int end);
  virtual void drawVRules(int start, int end);
public:
  GraphWindow(Frame* parent, uint32 color = 0xFFFFFF, uint32 lineColor = 0x000000);
  ~GraphWindow();

  void setRuleColor(uint32 color)
  {
    ruleColor = color;
    invalidate();
  }
  void setMinorColor(uint32 h, uint32 v)
  {
    hMinorColor = h;
    vMinorColor = v;
    invalidate();
  }
  void setMargin(int x, int y)
  {
    marginX = x;
    marginY = y;
    rebuildRect();
    invalidate();
  }

  void clear();
  void addGraph(int id, uint32 color, bool dash = false);
  void enableGraph(int id, bool enable);
  void addGraphPoint(int id, int x, int y, bool dash = false);
  void smoothGraph(int id, int factor);
  void combineGraphs(int id, int id2);

  int getGraphY(int id, int x);
};

#endif // __UI_GRAPHWND__
