#include "core/app.h"

#include "searchwnd.h"
#include "dota/dotadata.h"
#include "ui/batchdlg.h"
#include "ui/searchres.h"
#include "ui/mainwnd.h"
#include "ui/viewitem.h"
#include "base/regexp.h"
#include "base/utils.h"
#include "replay/cache.h"

#define IDC_BROWSEPATH      1102
#define IDC_SEARCH          1103

class SearchFunc : public BatchFunc
{
public:
  struct StringInfo
  {
    int mode;
    String text;
    RegExp* re;
  };
  StringInfo fileName;
  StringInfo gameName;
  int gameModeMode;
  uint64 gameModeMask;
  struct IntInfo
  {
    uint32 min;
    uint32 max;
  };
  IntInfo numPlayers;
  IntInfo mapVersion;
  IntInfo wc3Version;
  IntInfo gameLength;
  uint64 dateMin;
  uint64 dateMax;
  struct PlayerInfo
  {
    StringInfo name;
    int hero;
  };
  PlayerInfo players[5];

  static bool checkString(String val, StringInfo& info);
  static bool checkInt(uint32 val, IntInfo& info);

  SearchResults* results;

  SearchFunc();
  ~SearchFunc();

  void handle(String file, GameCache* gc);
};

SearchFunc::SearchFunc()
{
  fileName.re = NULL;
  gameName.re = NULL;
  for (int i = 0; i < 5; i++)
    players[i].name.re = NULL;

  results = (SearchResults*) SendMessage(getApp()->getMainWindow(), WM_GETVIEW, MAINWND_SEARCHRES, 0);
}
SearchFunc::~SearchFunc()
{
  delete fileName.re;
  delete gameName.re;
  for (int i = 0; i < 5; i++)
    delete players[i].name.re;
}
bool SearchFunc::checkString(String val, StringInfo& info)
{
  if (info.mode == 0)
    return val.ifind(info.text) >= 0;
  else if (info.mode == 1)
    return val.icompare(info.text) == 0;
  else if (info.mode == 2)
    return val.substring(0, val.fromUtfPos(
      info.text.getUtfLength())).icompare(info.text) == 0;
  else if (info.mode == 3)
  {
    if (info.re == NULL)
      info.re = new RegExp(info.text, REGEXP_CASE_INSENSITIVE);
    return info.re->find(val) >= 0;
  }
  return false;
}
bool SearchFunc::checkInt(uint32 val, IntInfo& info)
{
  return (val >= info.min) && (val <= info.max);
}

void SearchFunc::handle(String file, GameCache* gc)
{
  if (!checkString(String::getFileTitle(file), fileName))
    return;
  if (gc->ftime < dateMin || gc->ftime > dateMax)
    return;

  if (!checkString(gc->game_name, gameName))
    return;
  if (gameModeMode == 0)
  {
    if ((gameModeMask & ~gc->game_mode) != 0)
      return;
  }
  else if (gameModeMode == 1)
  {
    if (gameModeMask != gc->game_mode)
      return;
  }
  else if (gameModeMode == 2)
  {
    if ((gc->game_mode & ~gameModeMask) != 0)
      return;
  }
  if (!checkInt(gc->players, numPlayers))
    return;
  if (!checkInt(gc->map_version, mapVersion))
    return;
  if (!checkInt(gc->wc3_version, wc3Version))
    return;
  if (!checkInt(gc->game_length, gameLength))
    return;
  for (int i = 0; i < 5; i++)
  {
    bool found = false;
    for (int j = 0; j < gc->players && !found; j++)
    {
      if (checkString(gc->pname[j], players[i].name) &&
          (players[i].hero == 0 || players[i].hero == gc->phero[j]))
        found = true;
    }
    if (!found)
      return;
  }

  results->addFile(file);
}

void SearchWindow::fillHeroes(ComboFrameEx* cf)
{
  int selPoint = cf->getCurSel();
  if (selPoint >= 0)
    selPoint = cf->getItemData(selPoint);
  else
    selPoint = 0;
  cf->setBoxHeight(300);
  cf->reset();
  cf->addString("Any Hero", 0xFFFFFF, "Unknown", 0);

  int sel = 0;
  DotaLibrary* lib = getApp()->getDotaLibrary();
  Dota* dota = lib->getDota();
  for (int t = 0; t < lib->getNumTaverns(); t++)
  {
    if (lib->getTavernSide(t) == 0)
      cf->addString(lib->getTavernName(t), 0, "RedBullet");
    else
      cf->addString(lib->getTavernName(t), 0, "GreenBullet");
    for (int i = 1; i < MAX_HERO_POINT; i++)
    {
      Dota::Hero* hero = dota->getHero(i);
      if (hero && hero->tavern == t)
      {
        if (i == selPoint)
          sel = cf->getCount();
        cf->addString(hero->name, 0xFFFFFF, hero->icon, i);
      }
    }
  }
  dota->release();
  cf->setCurSel(sel);
}
void SearchWindow::fillStringMode(ComboFrame* cf)
{
  cf->addString("Contains");
  cf->addString("Equals");
  cf->addString("Starts with");
  cf->addString("Regular expression");
  cf->setCurSel(0);
}
void SearchWindow::createStringInfo(StringInfo& si, String tipText)
{
  StaticFrame* tip = new StaticFrame(tipText, this);
  si.mode = new ComboFrame(this);
  si.text = new EditFrame(this);

  si.mode->setWidth(120);
  si.text->setSize(150, 21);
  si.text->setPoint(PT_TOPLEFT, si.mode, PT_TOPRIGHT, 8, 0);
  tip->setPoint(PT_BOTTOMRIGHT, si.mode, PT_BOTTOMLEFT, -5, -5);

  fillStringMode(si.mode);
}
void SearchWindow::createRangeInfo(TextRangeInfo& ti, String tipText, int flags)
{
  ti.min = new EditFrame(this, 0, flags | ES_AUTOHSCROLL);
  ti.min->setSize(100, 21);
  StaticFrame* tip = new StaticFrame(tipText, this);
  tip->setPoint(PT_BOTTOMRIGHT, ti.min, PT_BOTTOMLEFT, -5, -5);
  tip = new StaticFrame("and", this);
  tip->setPoint(PT_BOTTOMLEFT, ti.min, PT_BOTTOMRIGHT, 5, -5);
  ti.max = new EditFrame(this, 0, flags | ES_AUTOHSCROLL);
  ti.max->setSize(100, 21);
  ti.max->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 5);
}
void SearchWindow::createRangeInfo(DateRangeInfo& di, String tipText, int flags)
{
  di.min = new DateTimeFrame(this, 0, flags);
  di.min->setSize(100, 21);
  StaticFrame* tip = new StaticFrame(tipText, this);
  tip->setPoint(PT_BOTTOMRIGHT, di.min, PT_BOTTOMLEFT, -5, -5);
  tip = new StaticFrame("and", this);
  tip->setPoint(PT_BOTTOMLEFT, di.min, PT_BOTTOMRIGHT, 5, -5);
  di.max = new DateTimeFrame(this, 0, flags);
  di.max->setSize(100, 21);
  di.max->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 5);
}
SearchWindow::SearchWindow(Frame* parent)
  : Frame(parent)
{
  path = new EditFrame(this);
  path->setPoint(PT_TOPLEFT, 90, 10);
  path->setSize(400, 21);

  ButtonFrame* browseBtn = new ButtonFrame("Browse", this, IDC_BROWSEPATH);
  browseBtn->setPoint(PT_TOPLEFT, path, PT_TOPRIGHT, 5, 0);
  browseBtn->setSize(65, 21);

  StaticFrame* tip = new StaticFrame("Search in", this);
  tip->setPoint(PT_BOTTOMRIGHT, path, PT_BOTTOMLEFT, -5, -5);

  createStringInfo(fileName, "File name");
  fileName.mode->setPoint(PT_TOPLEFT, path, PT_BOTTOMLEFT, 60, 10);
  fileName.text->setPoint(PT_RIGHT, browseBtn, PT_RIGHT, 0, 0);
  createRangeInfo(gameDate, "Game length between", DTS_SHORTDATEFORMAT | DTS_SHOWNONE);
  gameDate.min->setPoint(PT_TOPLEFT, fileName.mode, PT_BOTTOMLEFT, 0, 5);
  gameDate.min->setDate(sysTime());
  gameDate.min->setNoDate();
  gameDate.max->setDate(sysTime());
  gameDate.max->setNoDate();

  createStringInfo(gameName, "Game name");
  gameName.mode->setPoint(PT_TOPLEFT, gameDate.min, PT_BOTTOMLEFT, 0, 10);
  gameName.text->setPoint(PT_RIGHT, browseBtn, PT_RIGHT, 0, 0);
  createStringInfo(gameMode, "Game mode");
  gameMode.mode->reset();
  gameMode.mode->addString("Contains");
  gameMode.mode->addString("Equals");
  gameMode.mode->addString("Contained in");
  gameMode.mode->setCurSel(0);
  gameMode.mode->setPoint(PT_TOPLEFT, gameName.mode, PT_BOTTOMLEFT, 0, 10);
  gameMode.text->setPoint(PT_RIGHT, browseBtn, PT_RIGHT, 0, 0);

  createRangeInfo(numPlayers, "Number of players between", ES_NUMBER);
  numPlayers.min->setPoint(PT_TOPLEFT, gameMode.mode, PT_BOTTOMLEFT, 0, 10);
  createRangeInfo(mapVersion, "Map version between", 0);
  mapVersion.min->setPoint(PT_TOPLEFT, numPlayers.min, PT_BOTTOMLEFT, 0, 5);
  createRangeInfo(wc3Version, "Patch version between", 0);
  wc3Version.min->setPoint(PT_TOPLEFT, mapVersion.min, PT_BOTTOMLEFT, 0, 5);
  createRangeInfo(gameLength, "Game length between", DTS_TIMEFORMAT | DTS_UPDOWN | DTS_SHOWNONE);
  gameLength.min->setPoint(PT_TOPLEFT, wc3Version.min, PT_BOTTOMLEFT, 0, 5);
  gameLength.min->setFormat("H':'mm':'ss");
  gameLength.min->setDate(0);
  gameLength.min->setNoDate();
  gameLength.max->setFormat("H':'mm':'ss");
  gameLength.max->setDate(0);
  gameLength.max->setNoDate();

  SetFocus(gameDate.min->getHandle());
  SetFocus(gameDate.max->getHandle());
  SetFocus(gameLength.min->getHandle());
  SetFocus(gameLength.max->getHandle());
  SetFocus(NULL);

  for (int i = 0; i < 5; i++)
  {
    createStringInfo(players[i].name, "Player's name");
    players[i].hero = new ComboFrameEx(this);
    fillHeroes(players[i].hero);
    if (i == 0)
      players[i].name.mode->setPoint(PT_TOPLEFT, gameLength.min, PT_BOTTOMLEFT, -60, 15);
    else
      players[i].name.mode->setPoint(PT_TOPLEFT, players[i - 1].name.mode, PT_BOTTOMLEFT, 0, 5);
    tip = new StaticFrame(", hero", this);
    tip->setPoint(PT_BOTTOMLEFT, players[i].name.text, PT_BOTTOMRIGHT, 5, -5);
    players[i].hero->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 5);
    players[i].hero->setPoint(PT_RIGHT, browseBtn, PT_RIGHT, 0, 0);
  }

  ButtonFrame* searchButton = new ButtonFrame("Search!", this, IDC_SEARCH);
  searchButton->setHeight(30);
  searchButton->setPoint(PT_TOPLEFT, players[4].name.text, PT_BOTTOMLEFT, 0, 15);
  searchButton->setPoint(PT_TOPRIGHT, players[4].name.text, PT_BOTTOMRIGHT, 0, 15);
}

void SearchWindow::setPath(String _path)
{
  path->setText(_path);
  for (int i = 0; i < 5; i++)
    fillHeroes(players[i].hero);
}

uint32 SearchWindow::strToInt(String str, uint32 def)
{
  if (str.isInt())
    return str.toInt();
  return def;
}
uint32 SearchWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND)
  {
    if (LOWORD(wParam) == IDC_BROWSEPATH)
    {
      String searchPath;
      if (browseForFolder("Select search folder", searchPath))
        path->setText(searchPath);
    }
    else if (LOWORD(wParam) == IDC_SEARCH)
    {
      SearchFunc* func = new SearchFunc();

      func->fileName.mode = fileName.mode->getCurSel();
      func->fileName.text = fileName.text->getText();
      func->gameName.mode = gameName.mode->getCurSel();
      func->gameName.text = gameName.text->getText();
      func->gameModeMode = gameMode.mode->getCurSel();
      func->gameModeMask = parseMode(gameMode.text->getText());
      func->numPlayers.min = strToInt(numPlayers.min->getText(), 0);
      func->numPlayers.max = strToInt(numPlayers.max->getText(), 10);
      func->mapVersion.min = parseVersion(mapVersion.min->getText());
      func->mapVersion.max = parseVersion(mapVersion.max->getText());
      if (func->mapVersion.max == 0)
        func->mapVersion.max = max_uint32;
      func->wc3Version.min = parseVersion(wc3Version.min->getText());
      func->wc3Version.max = parseVersion(wc3Version.max->getText());
      if (func->wc3Version.max == 0)
        func->wc3Version.max = max_uint32;

      if (gameLength.min->isDateSet())
        func->gameLength.min = uint32(gameLength.min->getDate() * 1000);
      else
        func->gameLength.min = 0;
      if (gameLength.max->isDateSet())
        func->gameLength.max = uint32(gameLength.max->getDate() * 1000);
      else
        func->gameLength.max = max_uint32;

      if (gameDate.min->isDateSet())
      {
        uint64 time = gameDate.min->getDate();
        func->dateMin = time - (time % (3600 * 24));
        String fmt = format_systime(func->dateMin, "%c");
      }
      else
        func->dateMin = 0;

      if (gameDate.max->isDateSet())
      {
        uint64 time = gameDate.max->getDate();
        func->dateMax = time - (time % (3600 * 24)) + 3600 * 24 - 1;
      }
      else
        func->dateMax = max_uint64;

      for (int i = 0; i < 5; i++)
      {
        func->players[i].name.mode = players[i].name.mode->getCurSel();
        func->players[i].name.text = players[i].name.text->getText();
        func->players[i].hero = players[i].hero->getCurSel();
        if (func->players[i].hero < 0)
          func->players[i].hero = 0;
        else
          func->players[i].hero = players[i].hero->getItemData(func->players[i].hero);
      }

      SendMessage(getApp()->getMainWindow(), WM_REBUILDTREE, 1, 0);
      SendMessage(getApp()->getMainWindow(), WM_PUSHVIEW,
        (uint32) new SearchResViewItem(), 0);
      BatchDialog* batch = new BatchDialog(BatchDialog::mFunc, func, "Searching");
      batch->addFolder(String::fixPath(path->getText()));
      func->results->doBatch(batch);
      return 0;
    }
  }
  return M_UNHANDLED;
}
