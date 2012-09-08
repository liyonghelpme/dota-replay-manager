#ifndef __UI_HEROCHART__
#define __UI_HEROCHART__

#include "frameui/listctrl.h"
#include "ui/viewitem.h"
#include "replay/cache.h"

class HeroChart;
class HeroChartFrame : public Frame
{
  Array<String> files;
  HeroChartViewItem* viewItem;
  HeroChart* chart;
  ListFrame* heroes;
  ListFrame* games;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  static bool cacheEnumerator(String path, GameCache* game, void* param);
public:
  HeroChartFrame(Frame* parent);

  void setViewItem(HeroChartViewItem* item)
  {
    viewItem = item;
  }
  void setHero(int point);
};

#endif // __UI_HEROCHART__
