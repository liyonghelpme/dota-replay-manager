#include "core/app.h"

#include "herochart.h"
#include "dota/dotadata.h"
#include "dota/colors.h"
#include "frameui/fontsys.h"
#include "replay/cache.h"
#include "replay/consts.h"
#include "graphics/imagelib.h"
#include "ui/mainwnd.h"

#define WM_SETTAVERN          (WM_USER+1802)
#define WM_SETHERO            (WM_USER+1803)

#define TEAM_HEIGHT         40
#define NAME_HEIGHT         30
#define TAVERN_HEIGHT       (32*3+2*2)
#define CHART_WIDTH         308

class HeroChart : public WindowFrame
{
  Dota* dota;
  int heroCounter[MAX_HERO_POINT];
  HFONT counterFont;
  struct TavernInfo
  {
    RECT rc;
    uint32 color;
    Dota::Hero* heroes[12];
  };
  struct TextTag
  {
    RECT rc;
    HFONT hFont;
    uint32 color;
    String text;
    void set(String txt, uint32 clr, HFONT font, int left, int top, int right, int height)
    {
      text = txt;
      color = clr;
      hFont = font;
      rc.left = left;
      rc.top = top;
      rc.right = right;
      rc.bottom = top + height;
    }
  };
  TavernInfo* taverns;
  TextTag* textTags;
  int numTaverns;
  int numTags;
  int curTavern;
  int contentHeight;
  int scrollPos;
  int toolHitTest(POINT pt, ToolInfo* ti);
  static bool cacheEnumerator(String name, GameCache* game, void* param);
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
  void doScroll(int pos);
public:
  HeroChart(Frame* parent);
  ~HeroChart();

  void rebuild();
  void recount();
  void setHero(int hero);

  Dota* getDota()
  {
    return dota;
  }
  int getHeroCount(int hero)
  {
    return heroCounter[hero];
  }
};
HeroChart::HeroChart(Frame* parent)
  : WindowFrame(parent)
{
  create("", WS_VSCROLL | WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);
  dota = NULL;
  taverns = NULL;
  textTags = NULL;
  numTaverns = 0;
  numTags = 0;
  contentHeight = 0;
  scrollPos = 0;
  curTavern = -1;

  setWidth(CHART_WIDTH + 22);

  rebuild();
  enableTooltips();
}
HeroChart::~HeroChart()
{
  delete[] taverns;
  delete[] textTags;
  if (dota)
    dota->release();
}

void HeroChart::rebuild()
{
  if (dota)
    dota->release();
  delete[] taverns;
  delete[] textTags;

  DotaLibrary* lib = getApp()->getDotaLibrary();
  dota = lib->getDota();
  numTaverns = 12;
  numTags = 8;
  taverns = new TavernInfo[numTaverns];
  textTags = new TextTag[numTags];

  HFONT hFontLarge = FontSys::getFont(18, "Georgia", FONT_BOLD);
  HFONT hFontMed = FontSys::getFont(16, "Georgia", FONT_BOLD);
  counterFont = FontSys::getFont(10, "Arial");

  textTags[0].set("Sentinel", getSlotColor(0), hFontLarge, 0, 0, CHART_WIDTH, TEAM_HEIGHT);
  textTags[1].set("Agility", getSlotColor(0), hFontMed, 0, textTags[0].rc.bottom, CHART_WIDTH, NAME_HEIGHT);
  textTags[2].set("Strength", getSlotColor(0), hFontMed,
    0, textTags[1].rc.bottom + TAVERN_HEIGHT, CHART_WIDTH, NAME_HEIGHT);
  textTags[3].set("Intelligence", getSlotColor(0), hFontMed,
    0, textTags[2].rc.bottom + TAVERN_HEIGHT, CHART_WIDTH, NAME_HEIGHT);
  textTags[4].set("Scourge", getSlotColor(6), hFontLarge,
    0, textTags[3].rc.bottom + TAVERN_HEIGHT, CHART_WIDTH, TEAM_HEIGHT);
  textTags[5].set("Agility", getSlotColor(6), hFontMed,
    0, textTags[4].rc.bottom, CHART_WIDTH, NAME_HEIGHT);
  textTags[6].set("Strength", getSlotColor(6), hFontMed,
    0, textTags[5].rc.bottom + TAVERN_HEIGHT, CHART_WIDTH, NAME_HEIGHT);
  textTags[7].set("Intelligence", getSlotColor(6), hFontMed,
    0, textTags[6].rc.bottom + TAVERN_HEIGHT, CHART_WIDTH, NAME_HEIGHT);

  for (int i = 0; i < 12; i++)
  {
    taverns[i].rc.left = 10;
    if (i % 2)
      taverns[i].rc.left += (32 * 4 + 2 * 3 + 20);
    taverns[i].rc.top = textTags[1 + (i / 6) + (i / 2)].rc.bottom;
    taverns[i].rc.right = taverns[i].rc.left + 32 * 4 + 2 * 3;
    taverns[i].color = getSlotColor((i / 6) * 6);
    for (int h = 0; h < 12; h++)
      taverns[i].heroes[h] = NULL;
    int maxY = 0;
    for (int h = 0; h < MAX_HERO_POINT; h++)
    {
      Dota::Hero* hero = dota->getHero(h);
      if (hero && hero->tavern == i)
      {
        taverns[i].heroes[hero->tavernSlot] = hero;
        int y = hero->tavernSlot / 4;
        if (y > maxY)
          maxY = y;
      }
    }
    taverns[i].rc.bottom = taverns[i].rc.top + 32 + maxY * (32 + 2);
  }
  recount();

  contentHeight = textTags[numTags - 1].rc.bottom + TAVERN_HEIGHT + 15;
  onMessage(WM_SIZE, 0, 0);
}
bool HeroChart::cacheEnumerator(String name, GameCache* game, void* param)
{
  for (int i = 0; i < game->players; i++)
  {
    if (game->phero[i])
      ((int*) param)[game->phero[i]]++;
  }
  return true;
}
void HeroChart::recount()
{
  memset(heroCounter, 0, sizeof heroCounter);
  getApp()->getCache()->enumCache(cacheEnumerator, heroCounter);
}
int HeroChart::toolHitTest(POINT pt, ToolInfo* ti)
{
  pt.y += scrollPos;
  for (int i = 0; i < numTaverns; i++)
  {
    if (pt.x >= taverns[i].rc.left && pt.y >= taverns[i].rc.top &&
      pt.x < taverns[i].rc.right && pt.y < taverns[i].rc.bottom)
    {
      int x = (pt.x - taverns[i].rc.left) / (32 + 2);
      int y = (pt.y - taverns[i].rc.top) / (32 + 2);
      int h = x + y * 4;
      if (h >= 0 && h < 12 && taverns[i].heroes[h])
      {
        ti->rc.left = taverns[i].rc.left + x * (32 + 2);
        ti->rc.top = taverns[i].rc.top + y * (32 + 2) - scrollPos;
        ti->rc.right = ti->rc.left + 32;
        ti->rc.bottom = ti->rc.top + 32;
        ti->text = taverns[i].heroes[h]->name;
        return taverns[i].heroes[h]->point;
      }
    }
  }
  return -1;
}
uint32 HeroChart::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_ERASEBKGND:
    {
      HDC hDC = (HDC) wParam;
      SetBkColor(hDC, 0x000000);
      RECT rc;
      GetClientRect(hWnd, &rc);
      ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    return TRUE;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);

      HDC hBitmapDC = CreateCompatibleDC(hDC);

      SetBkColor(hDC, 0x000000);
      SetBkMode(hDC, OPAQUE);
      for (int i = 0; i < numTags; i++)
      {
        SelectObject(hDC, textTags[i].hFont);
        SetTextColor(hDC, textTags[i].color);
        RECT rc = textTags[i].rc;
        rc.top -= scrollPos;
        rc.bottom -= scrollPos;
        DrawText(hDC, textTags[i].text, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      }

      ImageLibrary* ilib = getApp()->getImageLibrary();
      SelectObject(hDC, counterFont);
      SetTextColor(hDC, 0xFFFFFF);
      for (int i = 0; i < numTaverns; i++)
      {
        if (i == curTavern)
        {
          RECT rc = taverns[i].rc;
          rc.top -= scrollPos + 3;
          rc.bottom -= scrollPos - 3;
          rc.left -= 3;
          rc.right += 3;
          SetBkColor(hDC, taverns[i].color);
          ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
          SetBkColor(hDC, 0x000000);
          rc = taverns[i].rc;
          rc.top -= scrollPos;
          rc.bottom -= scrollPos;
          ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
        }
        for (int h = 0; h < 12; h++)
        {
          Dota::Hero* hero = taverns[i].heroes[h];
          if (hero)
          {
            int x = taverns[i].rc.left + (32 + 2) * (h % 4);
            int y = taverns[i].rc.top + (32 + 2) * (h / 4);
            HBITMAP hBitmap = ilib->getBitmap(String::format("big%s", hero->icon));
            if (hBitmap)
            {
              SelectObject(hBitmapDC, hBitmap);
              BitBlt(hDC, x, y - scrollPos, 32, 32, hBitmapDC, 0, 0, SRCCOPY);
              RECT tipRc = {x, y - scrollPos, x + 32, y + 32 - scrollPos};
              DrawText(hDC, String::format(" %d ", heroCounter[hero->point]),
                -1, &tipRc, DT_SINGLELINE | DT_RIGHT | DT_BOTTOM);
            }
          }
        }
      }

      DeleteDC(hBitmapDC);

      EndPaint(hWnd, &ps);
    }
    break;
  case WM_SIZE:
    {
      SCROLLINFO si;
      memset(&si, 0, sizeof si);
      si.cbSize = sizeof si;
      si.fMask = SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;
      si.nPage = height();
      si.nMin = 0;
      si.nMax = contentHeight;
      SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    }
    break;
  case WM_LBUTTONDOWN:
    {
      SetFocus(hWnd);
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam) + scrollPos;
      for (int i = 0; i < numTaverns; i++)
      {
        if (x >= taverns[i].rc.left && y >= taverns[i].rc.top &&
          x < taverns[i].rc.right && y < taverns[i].rc.bottom)
        {
          if (curTavern >= 0)
          {
            RECT rc = taverns[curTavern].rc;
            rc.left -= 3;
            rc.top -= scrollPos + 3;
            rc.right += 3;
            rc.bottom -= scrollPos - 3;
            curTavern = i;
            InvalidateRect(hWnd, &rc, TRUE);
          }
          curTavern = i;
          RECT rc = taverns[i].rc;
          rc.left -= 3;
          rc.top -= scrollPos + 3;
          rc.right += 3;
          rc.bottom -= scrollPos - 3;
          InvalidateRect(hWnd, &rc, TRUE);

          notify(WM_SETTAVERN, i, 0);

          int hx = (x - taverns[i].rc.left) / (32 + 2);
          int hy = (y - taverns[i].rc.top) / (32 + 2);
          int h = hx + hy * 4;
          if (h >= 0 && h < 12 && taverns[i].heroes[h])
            notify(WM_SETHERO, taverns[i].heroes[h]->point, 0);

          break;
        }
      }
    }
    return 0;
  case WM_MOUSEWHEEL:
    doScroll(scrollPos - GET_WHEEL_DELTA_WPARAM(wParam) * 30 / WHEEL_DELTA);
    break;
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
      doScroll(si.nPos);
    }
    break;
  default:
    return M_UNHANDLED;
  }
  return 0;
}
void HeroChart::doScroll(int pos)
{
  SCROLLINFO si;
  memset(&si, 0, sizeof si);
  si.cbSize = sizeof si;
  si.fMask = SIF_RANGE | SIF_PAGE;
  GetScrollInfo(hWnd, SB_VERT, &si);
  if (pos < si.nMin) pos = si.nMin;
  if (pos > si.nMax - si.nPage + 1) pos = si.nMax - si.nPage + 1;
  si.fMask = SIF_POS;
  if (pos != scrollPos)
  {
    si.nPos = pos;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    int delta = scrollPos - pos;
    scrollPos = pos;
    ScrollWindowEx(hWnd, 0, delta, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
  }
}
void HeroChart::setHero(int hero)
{
  if (dota)
  {
    Dota::Hero* h = dota->getHero(hero);
    if (h->tavern >= 0 && h->tavern < numTaverns)
    {
      curTavern = h->tavern;
      notify(WM_SETTAVERN, h->tavern, 0);
      notify(WM_SETHERO, hero, 0);
      InvalidateRect(hWnd, NULL, true);
    }
  }
}

////////////////////////////////////////

HeroChartFrame::HeroChartFrame(Frame* parent)
  : Frame(parent)
{
  viewItem = NULL;
  chart = new HeroChart(this);
  chart->setPoint(PT_TOPLEFT, 0, 0);
  chart->setPoint(PT_BOTTOMLEFT, 0, 0);

  heroes = new ListFrame(this, 0, LVS_NOSORTHEADER | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
  heroes->setColorMode(ListFrame::colorStripe);
  games = new ListFrame(this, 0, LVS_NOSORTHEADER | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
  games->setColorMode(ListFrame::colorStripe);

  heroes->setPoint(PT_TOPLEFT, chart, PT_TOPRIGHT, 4, 0);
  heroes->setPoint(PT_BOTTOMRIGHT, this, PT_RIGHT, 0, -2);
  games->setPoint(PT_TOPLEFT, heroes, PT_BOTTOMLEFT, 0, 4);
  games->setPoint(PT_BOTTOMRIGHT, 0, 0);

  heroes->insertColumn(0, "Hero");
  heroes->insertColumn(1, "Games");
  games->insertColumn(0, "Game");
  games->insertColumn(1, "Score");
  games->insertColumn(2, "Level");
  games->insertColumn(3, "Gold");
  games->insertColumn(4, "File");
  for (int i = 0; i < 2; i++)
    heroes->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
  for (int i = 0; i < 5; i++)
    games->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
}

void HeroChartFrame::setHero(int point)
{
  chart->setHero(point);
  if (viewItem)
    viewItem->setHero(point);
}

struct EnumData
{
  Array<String>* files;
  ListFrame* list;
  int hero;
};
bool HeroChartFrame::cacheEnumerator(String path, GameCache* game, void* param)
{
  EnumData* ed = (EnumData*) param;
  for (int i = 0; i < game->players; i++)
  {
    if (game->phero[i] == ed->hero)
    {
      ed->files->push(path);
      int pos = ed->list->addItem(game->game_name,
        getApp()->getImageLibrary()->getListIndex("IconReplay"), 0);
      ed->list->setItemText(pos, 1, String::format("%d/%d",
        game->pstats[i][STAT_KILLS], game->pstats[i][STAT_DEATHS]));
      ed->list->setItemText(pos, 2, String((int) game->plevel[i]));
      ed->list->setItemText(pos, 3, String((int) game->pgold[i]));
      ed->list->setItemText(pos, 4, path);
      break;
    }
  }
  return true;
}
uint32 HeroChartFrame::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NOTIFY:
    {
      NMHDR* pnm = (NMHDR*) lParam;
      if (pnm->hwndFrom == heroes->getHandle() && pnm->code == LVN_ITEMCHANGED)
      {
        int sel = ListView_GetNextItem(heroes->getHandle(), -1, LVNI_SELECTED);
        if (sel >= 0)
        {
          files.clear();
          games->clear();
          EnumData ed;
          ed.hero = heroes->getItemParam(sel);
          ed.files = &files;
          ed.list = games;
          getApp()->getCache()->enumCache(cacheEnumerator, &ed);
          for (int i = 0; i < 5; i++)
            games->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
        }
      }
      else if (pnm->hwndFrom == games->getHandle() && pnm->code == LVN_ITEMACTIVATE)
      {
        int sel = ListView_GetNextItem(games->getHandle(), -1, LVNI_SELECTED);
        if (sel >= 0 && sel < files.length())
          SendMessage(getApp()->getMainWindow(), WM_PUSHVIEW,
            (WPARAM) new ReplayViewItem(files[sel]), 0);
      }
    }
    break;
  case WM_SETTAVERN:
    {
      heroes->clear();
      Dota* dota = chart->getDota();
      for (int i = 0; i < MAX_HERO_POINT; i++)
      {
        Dota::Hero* hero = dota->getHero(i);
        if (hero && hero->tavern == wParam)
        {
          int pos = heroes->addItem(hero->name,
            getApp()->getImageLibrary()->getListIndex(hero->icon), hero->point);
          heroes->setItemText(pos, 1, String(chart->getHeroCount(i)));
        }
      }
      for (int i = 0; i < 2; i++)
        heroes->setColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
    break;
  case WM_SETHERO:
    for (int i = 0; i < heroes->getCount(); i++)
    {
      if (heroes->getItemParam(i) == wParam)
      {
        ListView_SetItemState(heroes->getHandle(), i, LVIS_SELECTED, LVIS_SELECTED);
        break;
      }
    }
    if (viewItem)
      viewItem->setHero(wParam);
    break;
  default:
    return M_UNHANDLED;
  }
  return 0;
}
